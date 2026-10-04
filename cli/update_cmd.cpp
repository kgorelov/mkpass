#include "update_cmd.h"
#include "platform_utils.h"
#include "semver.h"
#include "update_state.h"
#include "sha512.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <filesystem>
#include <ctime>
#include <cstdlib>
#include <cstdio>
#include <cctype>

#ifdef _WIN32
#define popen _popen
#define pclose _pclose
#endif

namespace mkpass {

namespace {

bool update_check_only = false;
bool update_auto_yes = false;

std::string UnescapeJson(std::string_view s) {
    std::string res;
    res.reserve(s.size());
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '\\' && i + 1 < s.size()) {
            ++i;
            switch (s[i]) {
                case '"': res += '"'; break;
                case '\\': res += '\\'; break;
                case '/': res += '/'; break;
                case 'b': res += '\b'; break;
                case 'f': res += '\f'; break;
                case 'n': res += '\n'; break;
                case 'r': res += '\r'; break;
                case 't': res += '\t'; break;
                default: res += s[i]; break;
            }
        } else {
            res += s[i];
        }
    }
    return res;
}

std::optional<std::string> ExtractJsonString(const std::string& json, const std::string& key, size_t startPos = 0) {
    std::string search = "\"" + key + "\"";
    size_t pos = json.find(search, startPos);
    if (pos == std::string::npos) return std::nullopt;

    pos = json.find(':', pos + search.size());
    if (pos == std::string::npos) return std::nullopt;
    ++pos;

    while (pos < json.size() && std::isspace(static_cast<unsigned char>(json[pos]))) {
        ++pos;
    }
    if (pos >= json.size() || json[pos] != '"') {
        return std::nullopt;
    }
    ++pos;

    size_t start = pos;
    bool escaped = false;
    while (pos < json.size()) {
        if (escaped) {
            escaped = false;
        } else if (json[pos] == '\\') {
            escaped = true;
        } else if (json[pos] == '"') {
            break;
        }
        ++pos;
    }
    if (pos >= json.size()) return std::nullopt;

    return UnescapeJson(std::string_view(json.data() + start, pos - start));
}

std::optional<int64_t> ExtractJsonInt(const std::string& json, const std::string& key, size_t startPos = 0) {
    std::string search = "\"" + key + "\"";
    size_t pos = json.find(search, startPos);
    if (pos == std::string::npos) return std::nullopt;

    pos = json.find(':', pos + search.size());
    if (pos == std::string::npos) return std::nullopt;
    ++pos;

    while (pos < json.size() && std::isspace(static_cast<unsigned char>(json[pos]))) {
        ++pos;
    }
    if (pos >= json.size()) return std::nullopt;

    size_t start = pos;
    if (json[pos] == '-' || json[pos] == '+') {
        ++pos;
    }
    while (pos < json.size() && std::isdigit(static_cast<unsigned char>(json[pos]))) {
        ++pos;
    }
    if (pos == start || (pos == start + 1 && (json[start] == '-' || json[start] == '+'))) {
        return std::nullopt;
    }

    try {
        return std::stoll(json.substr(start, pos - start));
    } catch (...) {
        return std::nullopt;
    }
}

int RunProcess(const std::string& cmd, std::string* output = nullptr) {
    FILE* fp = popen(cmd.c_str(), "r");
    if (!fp) return -1;
    if (output) {
        char buf[4096];
        while (fgets(buf, sizeof(buf), fp)) {
            output->append(buf);
        }
    }
    return pclose(fp);
}

std::string GetUpdateApiUrl() {
    const char* envUrl = std::getenv("MKPASS_UPDATE_CHECK_URL");
    if (envUrl && *envUrl) {
        return std::string(envUrl);
    }
    return "https://api.github.com/repos/kgorelov/mkpass/releases/latest";
}

struct HttpResponse {
    int statusCode = 0;
    std::string etag;
    std::string body;
};

HttpResponse FetchHttp(const std::string& url, const std::string& ifNoneMatch = "") {
    HttpResponse resp;

    // Direct local file support for tests
    if (url.rfind("file://", 0) == 0) {
        std::string filePath = url.substr(7);
        std::ifstream file(filePath);
        if (file.is_open()) {
            std::stringstream ss;
            ss << file.rdbuf();
            resp.statusCode = 200;
            resp.body = ss.str();
            return resp;
        }
    }

    std::string userAgent = "mkpass/" MKPASS_VERSION;
#ifdef _WIN32
    userAgent += " (windows-x64)";
#elif defined(__APPLE__)
    userAgent += " (macos)";
#else
    userAgent += " (linux-x86_64)";
#endif

    std::string cmd = "curl -sSL -i -H \"User-Agent: " + userAgent + "\" "
                      "-H \"Accept: application/vnd.github.v3+json\" ";
    if (!ifNoneMatch.empty()) {
        cmd += "-H \"If-None-Match: " + ifNoneMatch + "\" ";
    }
    cmd += "\"" + url + "\"";

    std::string rawOutput;
    int exitCode = RunProcess(cmd, &rawOutput);
    if (exitCode != 0 || rawOutput.empty()) {
        // Fallback to wget if curl fails
        std::string wgetCmd = "wget -q -S -O - \"" + url + "\" 2>&1";
        rawOutput.clear();
        int wgetExit = RunProcess(wgetCmd, &rawOutput);
        if (wgetExit != 0 || rawOutput.empty()) {
            resp.statusCode = 0;
            return resp;
        }
    }

    // Split headers and body
    size_t headerEnd = rawOutput.find("\r\n\r\n");
    size_t delimLen = 4;
    if (headerEnd == std::string::npos) {
        headerEnd = rawOutput.find("\n\n");
        delimLen = 2;
    }

    if (headerEnd != std::string::npos) {
        std::string headers = rawOutput.substr(0, headerEnd);
        resp.body = rawOutput.substr(headerEnd + delimLen);

        // Parse status code from HTTP/x.x <code>
        size_t httpPos = headers.rfind("HTTP/");
        if (httpPos != std::string::npos) {
            size_t spacePos = headers.find(' ', httpPos);
            if (spacePos != std::string::npos && spacePos + 3 <= headers.size()) {
                try {
                    resp.statusCode = std::stoi(headers.substr(spacePos + 1, 3));
                } catch (...) {
                    resp.statusCode = 200;
                }
            }
        }

        // Parse ETag
        size_t etagPos = headers.find("etag:");
        if (etagPos == std::string::npos) etagPos = headers.find("ETag:");
        if (etagPos != std::string::npos) {
            size_t colonPos = headers.find(':', etagPos);
            size_t lineEnd = headers.find('\n', etagPos);
            if (colonPos != std::string::npos && colonPos < lineEnd) {
                std::string etagVal = headers.substr(colonPos + 1, lineEnd - colonPos - 1);
                while (!etagVal.empty() && std::isspace(static_cast<unsigned char>(etagVal.front()))) etagVal.erase(etagVal.begin());
                while (!etagVal.empty() && std::isspace(static_cast<unsigned char>(etagVal.back()))) etagVal.pop_back();
                resp.etag = etagVal;
            }
        }
    } else {
        resp.statusCode = 200;
        resp.body = rawOutput;
    }

    return resp;
}

bool DownloadUrlToFile(const std::string& url, const std::string& destPath) {
    if (url.rfind("file://", 0) == 0) {
        std::string srcPath = url.substr(7);
        try {
            std::filesystem::copy_file(srcPath, destPath, std::filesystem::copy_options::overwrite_existing);
            return true;
        } catch (...) {
            return false;
        }
    }

    std::string cmd = "curl -sSL -o \"" + destPath + "\" \"" + url + "\"";
    int ret = RunProcess(cmd);
    if (ret != 0) {
        std::string wgetCmd = "wget -q -O \"" + destPath + "\" \"" + url + "\"";
        ret = RunProcess(wgetCmd);
    }
    return (ret == 0);
}

} // namespace

std::string ComputeFileSha512(const std::string& filePath) {
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) return "";

    void* state = sha512_init();
    if (!state) return "";

    unsigned char buf[16384];
    while (file.read(reinterpret_cast<char*>(buf), sizeof(buf))) {
        sha512_update(state, buf, static_cast<int>(file.gcount()));
    }
    if (file.gcount() > 0) {
        sha512_update(state, buf, static_cast<int>(file.gcount()));
    }

    unsigned char digest[64];
    sha512_finalize(state, digest);

    std::ostringstream ss;
    ss << std::hex << std::setfill('0');
    for (int i = 0; i < 64; ++i) {
        ss << std::setw(2) << static_cast<int>(digest[i]);
    }
    return ss.str();
}

bool VerifyChecksumSha512(const std::string& filePath, const std::string& sumsContent, const std::string& targetFileName) {
    std::string computed = ComputeFileSha512(filePath);
    if (computed.empty()) return false;

    for (char& c : computed) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

    std::istringstream stream(sumsContent);
    std::string line;
    while (std::getline(stream, line)) {
        while (!line.empty() && (line.back() == '\r' || line.back() == ' ' || line.back() == '\t')) {
            line.pop_back();
        }
        if (line.empty() || line[0] == '#') continue;

        size_t spacePos = line.find(' ');
        if (spacePos == std::string::npos || spacePos == 0) continue;

        std::string expectedHash = line.substr(0, spacePos);
        for (char& c : expectedHash) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

        size_t namePos = line.find_first_not_of(" \t*", spacePos);
        if (namePos == std::string::npos) continue;
        std::string recordedName = line.substr(namePos);

        auto ends_with = [](const std::string& str, const std::string& suffix) {
            return str.size() >= suffix.size() &&
                   str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
        };

        if (recordedName == targetFileName ||
            ends_with(recordedName, "/" + targetFileName) ||
            ends_with(recordedName, "\\" + targetFileName)) {
            return (expectedHash == computed);
        }
    }
    return false;
}

bool ParseCliReleaseJson(const std::string& json, CliReleaseInfo& out) {
    auto tag = ExtractJsonString(json, "tag_name");
    if (!tag || tag->empty()) return false;

    out.tag_name = *tag;
    std::string ver = *tag;
    if (!ver.empty() && (ver[0] == 'v' || ver[0] == 'V')) {
        ver.erase(ver.begin());
    }
    out.version = ver;

    out.release_notes = ExtractJsonString(json, "body").value_or("");
    out.release_url = ExtractJsonString(json, "html_url").value_or("");

    out.assets.clear();
    size_t assetsPos = json.find("\"assets\"");
    if (assetsPos != std::string::npos) {
        size_t arrStart = json.find('[', assetsPos);
        size_t arrEnd = json.find(']', assetsPos);
        if (arrStart != std::string::npos && arrEnd != std::string::npos && arrEnd > arrStart) {
            size_t cur = arrStart;
            while (cur < arrEnd) {
                size_t objStart = json.find('{', cur);
                if (objStart == std::string::npos || objStart >= arrEnd) break;

                size_t objEnd = json.find('}', objStart);
                if (objEnd == std::string::npos || objEnd > arrEnd) break;

                std::string assetJson = json.substr(objStart, objEnd - objStart + 1);
                auto name = ExtractJsonString(assetJson, "name");
                auto downloadUrl = ExtractJsonString(assetJson, "browser_download_url");
                auto sizeVal = ExtractJsonInt(assetJson, "size");

                if (name && downloadUrl && !name->empty() && !downloadUrl->empty()) {
                    CliReleaseAsset asset;
                    asset.name = *name;
                    asset.download_url = *downloadUrl;
                    asset.size = sizeVal.value_or(0);
                    out.assets.push_back(asset);
                }

                cur = objEnd + 1;
            }
        }
    }

    return true;
}

CliReleaseAsset SelectCliOptimalAsset(const std::vector<CliReleaseAsset>& assets) {
    if (assets.empty()) return {};

    auto contains = [](const std::string& str, const std::string& sub) {
        return str.find(sub) != std::string::npos;
    };

    auto findMatching = [&](const std::vector<std::string>& patterns) -> CliReleaseAsset {
        for (const auto& pat : patterns) {
            for (const auto& a : assets) {
                if (contains(a.name, pat)) {
                    return a;
                }
            }
        }
        return {};
    };

    CliReleaseAsset match;

#ifdef _WIN32
    match = findMatching({"-setup.exe", ".exe", ".msi", ".zip"});
#elif defined(__APPLE__)
    match = findMatching({"-macos", ".dmg", ".tar.gz"});
#else
    // Linux
    if (std::getenv("APPIMAGE") != nullptr) {
        match = findMatching({".AppImage", "mkpass_", ".deb", ".rpm", "-bundle.tar.gz"});
    } else if (std::filesystem::exists("/etc/debian_version")) {
        match = findMatching({"mkpass_", "mkpass-gui_", ".deb", ".AppImage", "-bundle.tar.gz"});
    } else if (std::filesystem::exists("/etc/redhat-release") || std::filesystem::exists("/etc/fedora-release")) {
        match = findMatching({"mkpass-", "mkpass-gui-", ".rpm", ".AppImage", "-bundle.tar.gz"});
    } else {
        match = findMatching({"-cli.tar.gz", "-bundle.tar.gz", ".tar.gz", ".AppImage"});
    }
#endif

    if (!match.name.empty()) {
        return match;
    }

    for (const auto& a : assets) {
        if (!contains(a.name, "SHA512SUMS")) {
            return a;
        }
    }

    return assets.front();
}

CLI::App* RegisterUpdateCommand(CLI::App& app) {
    auto update_cmd = app.add_subcommand("update", "Check for updates or install the latest release");
    update_cmd->add_flag("--check-only", update_check_only, "Check if an update is available without downloading or installing");
    update_cmd->add_flag("-y,--yes", update_auto_yes, "Automatically proceed with installation without confirmation");
    return update_cmd;
}

int RunUpdateCommand(CLI::App* update_cmd) {
    (void)update_cmd;

    Config cfg(GetConfigFilePath());
    cfg.load();

    UpdateState state;
    state.load();

    std::string apiUrl = GetUpdateApiUrl();
    std::cout << "Checking for updates...\n";

    HttpResponse resp = FetchHttp(apiUrl, state.last_etag);

    if (resp.statusCode == 304) {
        state.last_check_timestamp = std::time(nullptr);
        state.save();
        std::cout << "mkpass is up to date (v" << MKPASS_VERSION << ").\n";
        return 0;
    }

    if (resp.statusCode != 200) {
        if (resp.statusCode == 403 || resp.statusCode == 429) {
            std::cerr << "GitHub API rate limit exceeded. Please try again later.\n";
        } else if (resp.statusCode == 0) {
            std::cerr << "Error: Could not connect to update service. Check network or ensure curl is installed.\n";
        } else {
            std::cerr << "Error: Failed to fetch update info (HTTP " << resp.statusCode << ").\n";
        }
        return 1;
    }

    CliReleaseInfo info;
    if (!ParseCliReleaseJson(resp.body, info)) {
        std::cerr << "Error: Failed to parse release information.\n";
        return 1;
    }

    if (!resp.etag.empty()) {
        state.last_etag = resp.etag;
    }
    state.last_check_timestamp = std::time(nullptr);

    auto currentVer = SemVer::Parse(MKPASS_VERSION);
    auto remoteVer = SemVer::Parse(info.version);

    if (currentVer && remoteVer) {
        if (*remoteVer <= *currentVer) {
            state.save();
            std::cout << "mkpass is up to date (v" << MKPASS_VERSION << ").\n";
            return 0;
        }
    } else {
        if (info.version == MKPASS_VERSION) {
            state.save();
            std::cout << "mkpass is up to date (v" << MKPASS_VERSION << ").\n";
            return 0;
        }
    }

    state.latest_known_version = info.version;
    state.save();

    std::cout << "Update available: v" << info.version << " (current: v" << MKPASS_VERSION << ")\n";
    if (!info.release_url.empty()) {
        std::cout << "Release: " << info.release_url << "\n";
    }

    if (!info.release_notes.empty()) {
        std::cout << "\nRelease Notes:\n" << info.release_notes << "\n\n";
    }

    if (update_check_only) {
        return 0;
    }

    auto asset = SelectCliOptimalAsset(info.assets);
    if (asset.name.empty() || asset.download_url.empty()) {
        std::cerr << "No suitable installer asset found for your platform in this release.\n";
        if (!info.release_url.empty()) {
            std::cerr << "Please visit: " << info.release_url << "\n";
        }
        return 1;
    }

    std::string sumsUrl;
    for (const auto& a : info.assets) {
        if (a.name == "SHA512SUMS.txt") {
            sumsUrl = a.download_url;
            break;
        }
    }

    if (!update_auto_yes) {
        std::cout << "Target deliverable: " << asset.name;
        if (asset.size > 0) {
            std::cout << " (" << (asset.size / (1024 * 1024)) << " MB)";
        }
        std::cout << "\nDo you want to download and install this update? [y/N]: ";
        std::string answer;
        if (!std::getline(std::cin, answer)) {
            std::cout << "Update cancelled.\n";
            return 0;
        }
        while (!answer.empty() && std::isspace(static_cast<unsigned char>(answer.front()))) answer.erase(answer.begin());
        while (!answer.empty() && std::isspace(static_cast<unsigned char>(answer.back()))) answer.pop_back();
        if (answer != "y" && answer != "Y" && answer != "yes" && answer != "YES") {
            std::cout << "Update cancelled.\n";
            return 0;
        }
    }

    std::string tempDir = GetTmpDir();
    std::string tempAssetPath = tempDir + "/" + asset.name;
    std::string tempSumsPath = tempDir + "/SHA512SUMS.txt";

    std::cout << "Downloading " << asset.name << "...\n";
    if (!DownloadUrlToFile(asset.download_url, tempAssetPath)) {
        std::cerr << "Error: Failed to download " << asset.name << "\n";
        return 1;
    }

    if (!sumsUrl.empty()) {
        std::cout << "Downloading integrity manifest (SHA512SUMS.txt)...\n";
        if (DownloadUrlToFile(sumsUrl, tempSumsPath)) {
            std::ifstream sumsFile(tempSumsPath);
            std::stringstream sumsStream;
            sumsStream << sumsFile.rdbuf();
            std::string sumsContent = sumsStream.str();

            if (!VerifyChecksumSha512(tempAssetPath, sumsContent, asset.name)) {
                std::remove(tempAssetPath.c_str());
                std::remove(tempSumsPath.c_str());
                std::cerr << "Error: Downloaded update failed integrity verification. Installation aborted for security.\n";
                return 1;
            }
            std::cout << "Integrity verified (SHA-512 matched).\n";
            std::remove(tempSumsPath.c_str());
        }
    }

    std::cout << "Starting installer...\n";

#ifdef _WIN32
    if (asset.name.find(".exe") != std::string::npos) {
        system(("start \"\" \"" + tempAssetPath + "\" /SILENT").c_str());
    } else if (asset.name.find(".msi") != std::string::npos) {
        system(("msiexec.exe /i \"" + tempAssetPath + "\" /qn").c_str());
    } else {
        std::cout << "Update downloaded to: " << tempAssetPath << "\n";
    }
#elif defined(__APPLE__)
    std::cout << "Update downloaded to: " << tempAssetPath << "\n";
#else
    if (asset.name.find(".AppImage") != std::string::npos) {
        std::filesystem::permissions(tempAssetPath, std::filesystem::perms::owner_all);
        std::cout << "AppImage downloaded to: " << tempAssetPath << "\n";
    } else if (asset.name.find(".deb") != std::string::npos) {
        std::cout << "Installing Debian package...\n";
        int ret = system(("sudo dpkg -i \"" + tempAssetPath + "\"").c_str());
        if (ret != 0) {
            std::cout << "Notice: If sudo failed, run:\n  sudo dpkg -i " << tempAssetPath << "\n";
        }
    } else if (asset.name.find(".rpm") != std::string::npos) {
        std::cout << "Installing RPM package...\n";
        int ret = system(("sudo rpm -Uvh \"" + tempAssetPath + "\"").c_str());
        if (ret != 0) {
            std::cout << "Notice: If sudo failed, run:\n  sudo rpm -Uvh " << tempAssetPath << "\n";
        }
    } else {
        std::cout << "Update archive downloaded to: " << tempAssetPath << "\n";
        std::cout << "Extract to install:\n  tar -xzf \"" << tempAssetPath << "\" -C /tmp\n";
    }
#endif

    std::cout << "Update completed.\n";
    return 0;
}

void MaybePrintPassiveUpdateNotification(const Config& cfg) {
    if (!mkpass::IsUpdateCheckingEnabled(cfg)) {
        return;
    }

    if (!IsStderrTerminal()) {
        return;
    }

    UpdateState state;
    state.load();

    if (state.latest_known_version.empty()) {
        return;
    }

    auto currentVer = SemVer::Parse(MKPASS_VERSION);
    auto remoteVer = SemVer::Parse(state.latest_known_version);

    if (currentVer && remoteVer && *remoteVer > *currentVer) {
        std::cerr << "[mkpass] Update available: v" << state.latest_known_version
                  << " (current: v" << MKPASS_VERSION << "). Run 'mkpass update' to install.\n";
    }
}

} // namespace mkpass
