#include "update_manager.h"
#include "platform_utils.h"
#include "semver.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QCryptographicHash>
#include <QStandardPaths>
#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QDesktopServices>
#include <QUrl>
#include <QSysInfo>
#include <QCoreApplication>
#include <ctime>

namespace mkpass {

namespace {

QString BuildUserAgent() {
    return QString("mkpass/%1 (%2-%3)")
        .arg(MKPASS_VERSION)
        .arg(QSysInfo::kernelType())
        .arg(QSysInfo::buildCpuArchitecture());
}

QUrl GetUpdateUrl() {
    const char *envUrl = std::getenv("MKPASS_UPDATE_CHECK_URL");
    if (envUrl && *envUrl) {
        return QUrl(QString::fromUtf8(envUrl));
    }
    return QUrl("https://api.github.com/repos/kgorelov/mkpass/releases/latest");
}

} // namespace

UpdateManager::UpdateManager(QObject *parent)
    : QObject(parent),
      config_(GetConfigFilePath()) {
    config_.load();
}

UpdateManager::~UpdateManager() {
    cancelDownload();
    if (currentReply_) {
        currentReply_->abort();
        currentReply_->deleteLater();
        currentReply_ = nullptr;
    }
}

void UpdateManager::checkForUpdatesInBackground() {
    config_.load();
    if (!mkpass::IsUpdateCheckingEnabled(config_)) {
        return;
    }

    mkpass::UpdateState state;
    state.load();
    uint32_t intervalDays = mkpass::GetUpdateCheckIntervalDays(config_);
    if (!state.should_check(static_cast<int>(intervalDays))) {
        return;
    }

    isManualCheck_ = false;
    sendReleaseRequest();
}

void UpdateManager::checkForUpdatesInteractive() {
    config_.load();
    isManualCheck_ = true;
    sendReleaseRequest();
}

void UpdateManager::sendReleaseRequest() {
    if (currentReply_) {
        currentReply_->abort();
        currentReply_->deleteLater();
        currentReply_ = nullptr;
    }

    mkpass::UpdateState state;
    state.load();
    QNetworkRequest request(GetUpdateUrl());
    request.setRawHeader("User-Agent", BuildUserAgent().toUtf8());
    request.setRawHeader("Accept", "application/vnd.github.v3+json");
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);

    if (!state.last_etag.empty()) {
        request.setRawHeader("If-None-Match", QByteArray::fromStdString(state.last_etag));
    }

    currentReply_ = networkManager_.get(request);
    connect(currentReply_, &QNetworkReply::finished, this, &UpdateManager::onReleaseReplyFinished);
}

void UpdateManager::onReleaseReplyFinished() {
    if (!currentReply_) return;
    QNetworkReply *reply = currentReply_;
    currentReply_ = nullptr;
    reply->deleteLater();

    int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    mkpass::UpdateState state;
    state.load();

    if (statusCode == 304) {
        state.last_check_timestamp = std::time(nullptr);
        state.save();
        if (isManualCheck_) {
            emit upToDate();
        }
        return;
    }

    if (reply->error() != QNetworkReply::NoError) {
        if (isManualCheck_) {
            emit checkFailed(reply->errorString());
        }
        return;
    }

    QByteArray responseData = reply->readAll();
    QByteArray etagHeader = reply->rawHeader("ETag");

    ReleaseInfo info;
    if (!ParseReleaseJson(responseData, info)) {
        if (isManualCheck_) {
            emit checkFailed("Failed to parse release information from GitHub.");
        }
        return;
    }

    if (!etagHeader.isEmpty()) {
        state.last_etag = etagHeader.toStdString();
    }
    state.last_check_timestamp = std::time(nullptr);

    auto currentVer = SemVer::Parse(MKPASS_VERSION);
    auto remoteVer = SemVer::Parse(info.version.toStdString());

    if (currentVer && remoteVer) {
        if (*remoteVer <= *currentVer) {
            state.save();
            if (isManualCheck_) {
                emit upToDate();
            }
            return;
        }
    } else {
        if (info.version.toStdString() == MKPASS_VERSION) {
            state.save();
            if (isManualCheck_) {
                emit upToDate();
            }
            return;
        }
    }

    state.latest_known_version = info.version.toStdString();
    state.save();

    if (!isManualCheck_ && !state.skipped_version.empty() && state.skipped_version == info.version.toStdString()) {
        return;
    }

    emit updateAvailable(info);
}

bool UpdateManager::ParseReleaseJson(const QByteArray& jsonData, ReleaseInfo& outInfo) {
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(jsonData, &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        return false;
    }

    QJsonObject obj = doc.object();
    QString tagName = obj.value("tag_name").toString().trimmed();
    if (tagName.isEmpty()) {
        return false;
    }

    outInfo.tagName = tagName;
    QString versionStr = tagName;
    if (versionStr.startsWith('v', Qt::CaseInsensitive)) {
        versionStr = versionStr.mid(1).trimmed();
    }
    outInfo.version = versionStr;
    outInfo.releaseNotes = obj.value("body").toString();
    outInfo.releaseUrl = obj.value("html_url").toString();

    outInfo.assets.clear();
    QJsonArray assetsArr = obj.value("assets").toArray();
    for (const auto& item : assetsArr) {
        if (!item.isObject()) continue;
        QJsonObject assetObj = item.toObject();
        ReleaseAsset asset;
        asset.name = assetObj.value("name").toString();
        asset.downloadUrl = assetObj.value("browser_download_url").toString();
        asset.size = static_cast<qint64>(assetObj.value("size").toDouble());
        if (!asset.name.isEmpty() && !asset.downloadUrl.isEmpty()) {
            outInfo.assets.append(asset);
        }
    }

    outInfo.optimalAsset = SelectOptimalAsset(outInfo.assets);
    return true;
}

ReleaseAsset UpdateManager::SelectOptimalAsset(const QVector<ReleaseAsset>& assets, TargetPlatform platform) {
    if (assets.isEmpty()) return {};

    if (platform == TargetPlatform::Auto) {
#ifdef _WIN32
        QString appDir = QCoreApplication::applicationDirPath();
        if (QFile::exists(appDir + "/unins000.exe")) {
            platform = TargetPlatform::WindowsSetup;
        } else {
            platform = TargetPlatform::WindowsSetup;
        }
#elif defined(__APPLE__)
        platform = TargetPlatform::LinuxTarball;
#else
        if (qEnvironmentVariableIsSet("APPIMAGE")) {
            platform = TargetPlatform::LinuxAppImage;
        } else if (QFile::exists("/etc/debian_version")) {
            platform = TargetPlatform::LinuxDeb;
        } else if (QFile::exists("/etc/redhat-release") || QFile::exists("/etc/fedora-release")) {
            platform = TargetPlatform::LinuxRpm;
        } else {
            platform = TargetPlatform::LinuxTarball;
        }
#endif
    }

    auto findMatching = [&](const QStringList& patterns) -> ReleaseAsset {
        for (const auto& pattern : patterns) {
            for (const auto& asset : assets) {
                if (asset.name.contains(pattern, Qt::CaseInsensitive)) {
                    return asset;
                }
            }
        }
        return {};
    };

    ReleaseAsset match;
    switch (platform) {
        case TargetPlatform::WindowsSetup:
            match = findMatching({"-setup.exe", "setup.exe", ".exe", ".msi", ".zip"});
            break;
        case TargetPlatform::WindowsMsi:
            match = findMatching({".msi", "-setup.exe", ".exe", ".zip"});
            break;
        case TargetPlatform::WindowsZip:
            match = findMatching({"-windows-x64.zip", ".zip", "-setup.exe", ".msi"});
            break;
        case TargetPlatform::LinuxAppImage:
            match = findMatching({".AppImage", "mkpass-gui_", ".deb", ".rpm", "-bundle.tar.gz", ".tar.gz"});
            break;
        case TargetPlatform::LinuxDeb:
            match = findMatching({"mkpass-gui_", "mkpass_", ".deb", ".AppImage", "-bundle.tar.gz"});
            break;
        case TargetPlatform::LinuxRpm:
            match = findMatching({"mkpass-gui-", "mkpass-", ".rpm", ".AppImage", "-bundle.tar.gz"});
            break;
        case TargetPlatform::LinuxTarball:
            match = findMatching({"-bundle.tar.gz", ".tar.gz", ".AppImage", ".deb", ".rpm"});
            break;
        default:
            break;
    }

    if (!match.name.isEmpty()) {
        return match;
    }

    for (const auto& asset : assets) {
        if (!asset.name.contains("SHA512SUMS", Qt::CaseInsensitive)) {
            return asset;
        }
    }
    return assets.first();
}

bool UpdateManager::VerifyChecksum(const QString& filePath, const QByteArray& checksumsData, const QString& fileName) {
    QString checksumsText = QString::fromUtf8(checksumsData);
    QStringList lines = checksumsText.split('\n', Qt::SkipEmptyParts);

    QString matchedHash;
    for (const QString& line : lines) {
        QString trimmed = line.trimmed();
        if (trimmed.isEmpty() || trimmed.startsWith('#')) continue;

        int spaceIdx = trimmed.indexOf(' ');
        if (spaceIdx <= 0) continue;

        QString hashVal = trimmed.left(spaceIdx).trimmed().toLower();
        QString recordedName = trimmed.mid(spaceIdx).trimmed();
        if (recordedName.startsWith('*')) {
            recordedName = recordedName.mid(1).trimmed();
        }

        if (recordedName.compare(fileName, Qt::CaseInsensitive) == 0 ||
            recordedName.endsWith("/" + fileName, Qt::CaseInsensitive) ||
            recordedName.endsWith("\\" + fileName, Qt::CaseInsensitive)) {
            matchedHash = hashVal;
            break;
        }
    }

    if (matchedHash.isEmpty()) {
        return false;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    if (matchedHash.length() != 128) {
        return false;
    }

    QCryptographicHash hash(QCryptographicHash::Sha512);
    if (!hash.addData(&file)) {
        return false;
    }
    QByteArray computedHash = hash.result().toHex().toLower();
    file.close();

    return (matchedHash == computedHash);
}

void UpdateManager::startDownload(const ReleaseAsset &asset, const QVector<ReleaseAsset>& allAssets) {
    cancelDownload();

    if (asset.downloadUrl.isEmpty()) {
        emit downloadFailed("No download URL available for the selected asset.");
        return;
    }

    pendingTargetFileName_ = asset.name;
    QString tempDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    if (tempDir.isEmpty()) {
        tempDir = QDir::tempPath();
    }
    pendingDownloadPath_ = QDir(tempDir).filePath(asset.name);

    downloadFile_ = std::make_unique<QFile>(pendingDownloadPath_);
    if (!downloadFile_->open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        emit downloadFailed(QString("Failed to open temporary file for writing: %1").arg(pendingDownloadPath_));
        downloadFile_.reset();
        return;
    }

    assetDownloaded_ = false;
    checksumsDownloaded_ = false;
    checksumsFailed_ = false;
    pendingChecksumsData_.clear();

    QNetworkRequest assetReq(QUrl(asset.downloadUrl));
    assetReq.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    downloadReply_ = networkManager_.get(assetReq);

    connect(downloadReply_, &QNetworkReply::downloadProgress, this, &UpdateManager::downloadProgress);
    connect(downloadReply_, &QNetworkReply::readyRead, this, &UpdateManager::onAssetReadyRead);
    connect(downloadReply_, &QNetworkReply::finished, this, &UpdateManager::onAssetDownloadFinished);

    ReleaseAsset checksumAsset;
    for (const auto& a : allAssets) {
        if (a.name.compare("SHA512SUMS.txt", Qt::CaseInsensitive) == 0 ||
            a.name.contains("SHA512SUMS", Qt::CaseInsensitive)) {
            checksumAsset = a;
            break;
        }
    }

    if (!checksumAsset.downloadUrl.isEmpty()) {
        QNetworkRequest checkReq(QUrl(checksumAsset.downloadUrl));
        checkReq.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
        checksumsReply_ = networkManager_.get(checkReq);
        connect(checksumsReply_, &QNetworkReply::finished, this, &UpdateManager::onChecksumsDownloadFinished);
    } else {
        checksumsFailed_ = true;
    }
}

void UpdateManager::onAssetReadyRead() {
    if (downloadFile_ && downloadReply_) {
        downloadFile_->write(downloadReply_->readAll());
    }
}

void UpdateManager::onAssetDownloadFinished() {
    if (!downloadReply_) return;
    QNetworkReply *reply = downloadReply_;
    downloadReply_ = nullptr;
    reply->deleteLater();

    if (downloadFile_) {
        downloadFile_->write(reply->readAll());
        downloadFile_->flush();
        downloadFile_->close();
    }

    if (reply->error() != QNetworkReply::NoError) {
        if (downloadFile_) {
            downloadFile_->remove();
            downloadFile_.reset();
        }
        emit downloadFailed(QString("Download error: %1").arg(reply->errorString()));
        return;
    }

    assetDownloaded_ = true;
    checkDownloadCompletion();
}

void UpdateManager::onChecksumsDownloadFinished() {
    if (!checksumsReply_) return;
    QNetworkReply *reply = checksumsReply_;
    checksumsReply_ = nullptr;
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        checksumsFailed_ = true;
    } else {
        pendingChecksumsData_ = reply->readAll();
        checksumsDownloaded_ = true;
    }
    checkDownloadCompletion();
}

void UpdateManager::checkDownloadCompletion() {
    if (!assetDownloaded_) return;
    if (!checksumsDownloaded_ && !checksumsFailed_) return;

    if (checksumsFailed_ || pendingChecksumsData_.isEmpty()) {
        if (downloadFile_) {
            downloadFile_->remove();
            downloadFile_.reset();
        }
        emit downloadFailed("Could not download or find SHA512SUMS.txt for integrity verification.");
        return;
    }

    if (!VerifyChecksum(pendingDownloadPath_, pendingChecksumsData_, pendingTargetFileName_)) {
        if (downloadFile_) {
            downloadFile_->remove();
            downloadFile_.reset();
        }
        emit downloadFailed("Downloaded update failed integrity verification. Installation aborted for security.");
        return;
    }

    emit downloadFinished(pendingDownloadPath_);
}

void UpdateManager::cancelDownload() {
    if (downloadReply_) {
        downloadReply_->abort();
        downloadReply_->deleteLater();
        downloadReply_ = nullptr;
    }
    if (checksumsReply_) {
        checksumsReply_->abort();
        checksumsReply_->deleteLater();
        checksumsReply_ = nullptr;
    }
    if (downloadFile_) {
        downloadFile_->close();
        downloadFile_->remove();
        downloadFile_.reset();
    }
    assetDownloaded_ = false;
    checksumsDownloaded_ = false;
    checksumsFailed_ = false;
    pendingChecksumsData_.clear();
}

void UpdateManager::installDownloadedUpdate(const QString &localFilePath) {
    if (!QFile::exists(localFilePath)) {
        return;
    }

#ifdef _WIN32
    if (localFilePath.endsWith(".exe", Qt::CaseInsensitive)) {
        QProcess::startDetached(localFilePath, QStringList() << "/SILENT");
        QCoreApplication::quit();
    } else if (localFilePath.endsWith(".msi", Qt::CaseInsensitive)) {
        QProcess::startDetached("msiexec.exe", QStringList() << "/i" << localFilePath);
        QCoreApplication::quit();
    } else {
        QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(localFilePath).absolutePath()));
    }
#elif defined(__APPLE__)
    QDesktopServices::openUrl(QUrl::fromLocalFile(localFilePath));
#else
    if (localFilePath.endsWith(".AppImage", Qt::CaseInsensitive)) {
        QFile::setPermissions(localFilePath,
            QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner |
            QFile::ReadGroup | QFile::ExeGroup |
            QFile::ReadOther | QFile::ExeOther);
        QProcess::startDetached(localFilePath, QStringList());
        QCoreApplication::quit();
    } else if (localFilePath.endsWith(".deb", Qt::CaseInsensitive) ||
               localFilePath.endsWith(".rpm", Qt::CaseInsensitive)) {
        bool started = QProcess::startDetached("xdg-open", QStringList() << localFilePath);
        if (!started) {
            QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(localFilePath).absolutePath()));
        } else {
            QCoreApplication::quit();
        }
    } else {
        QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(localFilePath).absolutePath()));
    }
#endif
}

} // namespace mkpass
