#include "config.h"
#include "platform_utils.h"
#include "db.h"
#include "toml.hpp"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <stdexcept>

namespace mkpass {

namespace {

std::optional<bool> ParseBool(const std::string& str) {
    std::string s = str;
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) { return !std::isspace(ch); }));
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(), s.end());
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });

    if (s == "true" || s == "1" || s == "yes" || s == "y") return true;
    if (s == "false" || s == "0" || s == "no" || s == "n") return false;
    return std::nullopt;
}

} // namespace

Config::Config(std::string file_path) : path_(std::move(file_path)) {
    if (path_.empty()) {
        path_ = get_default_config_path();
    }
}

std::string Config::get_default_config_path() {
    return GetConfigFilePath();
}

bool Config::is_valid_key(const std::string& key) {
    static const std::vector<std::string> valid_keys = {
        "algorithm",
        "char_classes",
        "custom_chars",
        "length",
        "separator",
        "passphrase_pattern",
        "pattern",
        "digits",
        "symbols",
        "substitutions",
        "capitalize",
        "enable_old_algorithm"
    };
    return std::find(valid_keys.begin(), valid_keys.end(), key) != valid_keys.end();
}

std::vector<std::string> Config::get_all_keys() {
    return {
        "algorithm",
        "char_classes",
        "custom_chars",
        "length",
        "separator",
        "passphrase_pattern",
        "digits",
        "symbols",
        "substitutions",
        "capitalize",
        "enable_old_algorithm"
    };
}

std::string Config::get_built_in_default(const std::string& key) {
    if (key == "algorithm") return "password/argon2";
    if (key == "char_classes") return "lowercase,uppercase,digits,symbols";
    if (key == "custom_chars") return "";
    if (key == "length") return "16";
    if (key == "separator") return "";
    if (key == "passphrase_pattern" || key == "pattern") return "";
    if (key == "digits") return "false";
    if (key == "symbols") return "false";
    if (key == "substitutions") return "false";
    if (key == "capitalize") return "true";
    if (key == "enable_old_algorithm") return "false";
    throw std::invalid_argument("Unknown configuration key: " + key);
}

bool Config::exists() const {
    std::string p = path_.empty() ? get_default_config_path() : path_;
    return std::filesystem::exists(p);
}

bool Config::load() {
    options_ = ConfigOptions{};
    if (path_.empty()) {
        path_ = get_default_config_path();
    }
    if (!std::filesystem::exists(path_)) {
        return true;
    }

    try {
        auto tbl = toml::parse_file(path_);
        if (auto val = tbl["algorithm"].value<std::string>()) {
            options_.algorithm = ParseAlgorithm(*val);
        } else if (auto val_int = tbl["algorithm"].value<int64_t>()) {
            options_.algorithm = ParseAlgorithm(std::to_string(*val_int));
        }

        if (auto val = tbl["char_classes"].value<std::string>()) {
            options_.char_classes = ParseCharacterClasses(*val);
        } else if (auto val_int = tbl["char_classes"].value<int64_t>()) {
            options_.char_classes = ParseCharacterClasses(std::to_string(*val_int));
        }

        if (auto val = tbl["custom_chars"].value<std::string>()) {
            options_.custom_chars = *val;
        }

        if (auto val = tbl["length"].value<int64_t>()) {
            if (*val > 0) {
                options_.length = static_cast<size_t>(*val);
            }
        }

        if (auto val = tbl["separator"].value<std::string>()) {
            options_.separator = *val;
        }

        if (auto val = tbl["passphrase_pattern"].value<std::string>()) {
            options_.passphrase_pattern = StringToPattern(*val);
        } else if (auto val2 = tbl["pattern"].value<std::string>()) {
            options_.passphrase_pattern = StringToPattern(*val2);
        }

        if (auto val = tbl["digits"].value<bool>()) {
            options_.digits = *val;
        }

        if (auto val = tbl["symbols"].value<bool>()) {
            options_.symbols = *val;
        }

        if (auto val = tbl["substitutions"].value<bool>()) {
            options_.substitutions = *val;
        }

        if (auto val = tbl["capitalize"].value<bool>()) {
            options_.capitalize = *val;
        }

        if (auto val = tbl["enable_old_algorithm"].value<bool>()) {
            options_.enable_old_algorithm = *val;
        }

        return true;
    } catch (...) {
        return false;
    }
}

bool Config::save() {
    try {
        if (path_.empty()) {
            path_ = get_default_config_path();
        }
        std::filesystem::path p(path_);
        if (p.has_parent_path()) {
            std::filesystem::create_directories(p.parent_path());
        }

        toml::table tbl;
        if (options_.algorithm) {
            tbl.insert_or_assign("algorithm", AlgorithmToIdentifier(*options_.algorithm));
        }
        if (options_.char_classes) {
            tbl.insert_or_assign("char_classes", CharacterClassesToIdentifierString(*options_.char_classes));
        }
        if (options_.custom_chars) {
            tbl.insert_or_assign("custom_chars", *options_.custom_chars);
        }
        if (options_.length) {
            tbl.insert_or_assign("length", static_cast<int64_t>(*options_.length));
        }
        if (options_.separator) {
            tbl.insert_or_assign("separator", *options_.separator);
        }
        if (options_.passphrase_pattern) {
            tbl.insert_or_assign("passphrase_pattern", PatternToString(*options_.passphrase_pattern));
        }
        if (options_.digits) {
            tbl.insert_or_assign("digits", *options_.digits);
        }
        if (options_.symbols) {
            tbl.insert_or_assign("symbols", *options_.symbols);
        }
        if (options_.substitutions) {
            tbl.insert_or_assign("substitutions", *options_.substitutions);
        }
        if (options_.capitalize) {
            tbl.insert_or_assign("capitalize", *options_.capitalize);
        }
        if (options_.enable_old_algorithm) {
            tbl.insert_or_assign("enable_old_algorithm", *options_.enable_old_algorithm);
        }

        std::ofstream out(path_);
        if (!out.is_open()) {
            return false;
        }
        out << "# mkpass configuration file\n\n";
        out << tbl << "\n";
        return true;
    } catch (...) {
        return false;
    }
}

std::optional<std::string> Config::get_raw(const std::string& key) const {
    if (key == "algorithm") {
        return options_.algorithm ? std::optional<std::string>(AlgorithmToIdentifier(*options_.algorithm)) : std::nullopt;
    }
    if (key == "char_classes") {
        return options_.char_classes ? std::optional<std::string>(CharacterClassesToIdentifierString(*options_.char_classes)) : std::nullopt;
    }
    if (key == "custom_chars") {
        return options_.custom_chars;
    }
    if (key == "length") {
        return options_.length ? std::optional<std::string>(std::to_string(*options_.length)) : std::nullopt;
    }
    if (key == "separator") {
        return options_.separator;
    }
    if (key == "passphrase_pattern" || key == "pattern") {
        return options_.passphrase_pattern ? std::optional<std::string>(PatternToString(*options_.passphrase_pattern)) : std::nullopt;
    }
    if (key == "digits") {
        return options_.digits ? std::optional<std::string>(*options_.digits ? "true" : "false") : std::nullopt;
    }
    if (key == "symbols") {
        return options_.symbols ? std::optional<std::string>(*options_.symbols ? "true" : "false") : std::nullopt;
    }
    if (key == "substitutions") {
        return options_.substitutions ? std::optional<std::string>(*options_.substitutions ? "true" : "false") : std::nullopt;
    }
    if (key == "capitalize") {
        return options_.capitalize ? std::optional<std::string>(*options_.capitalize ? "true" : "false") : std::nullopt;
    }
    if (key == "enable_old_algorithm") {
        return options_.enable_old_algorithm ? std::optional<std::string>(*options_.enable_old_algorithm ? "true" : "false") : std::nullopt;
    }
    throw std::invalid_argument("Unknown configuration key: " + key);
}

void Config::set_raw(const std::string& key, const std::string& value) {
    if (key == "algorithm") {
        auto algo = ParseAlgorithm(value);
        if (!algo) {
            throw std::invalid_argument("Invalid algorithm: " + value + ". Valid values: password/argon2, password/sha512, passphrase/diceware, passphrase/wordnet, password/old");
        }
        options_.algorithm = algo;
    } else if (key == "char_classes") {
        auto cc = ParseCharacterClasses(value);
        if (cc.empty()) {
            throw std::invalid_argument("Invalid char_classes: " + value + ". Valid tokens: lowercase, uppercase, digits, symbols, custom");
        }
        options_.char_classes = cc;
    } else if (key == "custom_chars") {
        options_.custom_chars = value;
    } else if (key == "length") {
        try {
            std::string v = value;
            v.erase(v.begin(), std::find_if(v.begin(), v.end(), [](unsigned char ch) { return !std::isspace(ch); }));
            v.erase(std::find_if(v.rbegin(), v.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(), v.end());
            if (v.empty() || v[0] == '-' || !std::all_of(v.begin(), v.end(), [](unsigned char c) { return std::isdigit(c); })) {
                throw std::exception();
            }
            size_t pos = 0;
            unsigned long len = std::stoul(v, &pos);
            if (pos != v.size() || len == 0) {
                throw std::exception();
            }
            options_.length = len;
        } catch (...) {
            throw std::invalid_argument("Invalid length: " + value + ". Must be a positive integer.");
        }
    } else if (key == "separator") {
        options_.separator = value;
    } else if (key == "passphrase_pattern" || key == "pattern") {
        std::string v = value;
        v.erase(v.begin(), std::find_if(v.begin(), v.end(), [](unsigned char ch) { return !std::isspace(ch); }));
        v.erase(std::find_if(v.rbegin(), v.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(), v.end());
        std::transform(v.begin(), v.end(), v.begin(), [](unsigned char c) { return std::tolower(c); });

        if (v.empty() || v == "random" || v == "1") {
            options_.passphrase_pattern = std::vector<WordClasses>{};
        } else {
            for (char c : v) {
                if (c != 'n' && c != 'v' && c != 'a' && c != 'r') {
                    throw std::invalid_argument("Invalid passphrase pattern: " + value + ". Allowed characters: n (noun), v (verb), a (adj), r (adv).");
                }
            }
            options_.passphrase_pattern = StringToPattern(v);
        }
    } else if (key == "digits") {
        auto b = ParseBool(value);
        if (!b) {
            throw std::invalid_argument("Invalid boolean for digits: " + value);
        }
        options_.digits = b;
    } else if (key == "symbols") {
        auto b = ParseBool(value);
        if (!b) {
            throw std::invalid_argument("Invalid boolean for symbols: " + value);
        }
        options_.symbols = b;
    } else if (key == "substitutions") {
        auto b = ParseBool(value);
        if (!b) {
            throw std::invalid_argument("Invalid boolean for substitutions: " + value);
        }
        options_.substitutions = b;
    } else if (key == "capitalize") {
        auto b = ParseBool(value);
        if (!b) {
            throw std::invalid_argument("Invalid boolean for capitalize: " + value);
        }
        options_.capitalize = b;
    } else if (key == "enable_old_algorithm") {
        auto b = ParseBool(value);
        if (!b) {
            throw std::invalid_argument("Invalid boolean for enable_old_algorithm: " + value);
        }
        options_.enable_old_algorithm = b;
    } else {
        throw std::invalid_argument("Unknown configuration key: " + key);
    }
}

bool Config::unset_raw(const std::string& key) {
    if (key == "algorithm") {
        bool was_set = options_.algorithm.has_value();
        options_.algorithm = std::nullopt;
        return was_set;
    }
    if (key == "char_classes") {
        bool was_set = options_.char_classes.has_value();
        options_.char_classes = std::nullopt;
        return was_set;
    }
    if (key == "custom_chars") {
        bool was_set = options_.custom_chars.has_value();
        options_.custom_chars = std::nullopt;
        return was_set;
    }
    if (key == "length") {
        bool was_set = options_.length.has_value();
        options_.length = std::nullopt;
        return was_set;
    }
    if (key == "separator") {
        bool was_set = options_.separator.has_value();
        options_.separator = std::nullopt;
        return was_set;
    }
    if (key == "passphrase_pattern" || key == "pattern") {
        bool was_set = options_.passphrase_pattern.has_value();
        options_.passphrase_pattern = std::nullopt;
        return was_set;
    }
    if (key == "digits") {
        bool was_set = options_.digits.has_value();
        options_.digits = std::nullopt;
        return was_set;
    }
    if (key == "symbols") {
        bool was_set = options_.symbols.has_value();
        options_.symbols = std::nullopt;
        return was_set;
    }
    if (key == "substitutions") {
        bool was_set = options_.substitutions.has_value();
        options_.substitutions = std::nullopt;
        return was_set;
    }
    if (key == "capitalize") {
        bool was_set = options_.capitalize.has_value();
        options_.capitalize = std::nullopt;
        return was_set;
    }
    if (key == "enable_old_algorithm") {
        bool was_set = options_.enable_old_algorithm.has_value();
        options_.enable_old_algorithm = std::nullopt;
        return was_set;
    }
    throw std::invalid_argument("Unknown configuration key: " + key);
}

bool Config::is_set(const std::string& key) const {
    return get_raw(key).has_value();
}

std::string Config::print() const {
    if (exists()) {
        std::ifstream in(path_);
        if (in.is_open()) {
            std::stringstream ss;
            ss << in.rdbuf();
            return ss.str();
        }
    }

    toml::table tbl;
    if (options_.algorithm) {
        tbl.insert_or_assign("algorithm", AlgorithmToIdentifier(*options_.algorithm));
    }
    if (options_.char_classes) {
        tbl.insert_or_assign("char_classes", CharacterClassesToIdentifierString(*options_.char_classes));
    }
    if (options_.custom_chars) {
        tbl.insert_or_assign("custom_chars", *options_.custom_chars);
    }
    if (options_.length) {
        tbl.insert_or_assign("length", static_cast<int64_t>(*options_.length));
    }
    if (options_.separator) {
        tbl.insert_or_assign("separator", *options_.separator);
    }
    if (options_.passphrase_pattern) {
        tbl.insert_or_assign("passphrase_pattern", PatternToString(*options_.passphrase_pattern));
    }
    if (options_.digits) {
        tbl.insert_or_assign("digits", *options_.digits);
    }
    if (options_.symbols) {
        tbl.insert_or_assign("symbols", *options_.symbols);
    }
    if (options_.substitutions) {
        tbl.insert_or_assign("substitutions", *options_.substitutions);
    }
    if (options_.capitalize) {
        tbl.insert_or_assign("capitalize", *options_.capitalize);
    }
    if (options_.enable_old_algorithm) {
        tbl.insert_or_assign("enable_old_algorithm", *options_.enable_old_algorithm);
    }

    std::stringstream ss;
    ss << tbl << "\n";
    return ss.str();
}

bool IsOldAlgorithmEnabled(const Config& config) {
    if (const char* env1 = std::getenv("MKPASS_ENABLE_OLD_ALGORITHM")) {
        auto b = ParseBool(env1);
        if (b.has_value()) return *b;
    }
    if (const char* env2 = std::getenv("enable_old_algorithm")) {
        auto b = ParseBool(env2);
        if (b.has_value()) return *b;
    }
    return config.options().enable_old_algorithm.value_or(false);
}

} // namespace mkpass
