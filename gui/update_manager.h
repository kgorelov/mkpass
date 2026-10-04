#pragma once

#include <QObject>
#include <QString>
#include <QVector>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QFile>
#include <memory>

#include "config.h"
#include "update_state.h"

#ifndef MKPASS_VERSION
#define MKPASS_VERSION "0.1.0"
#endif

namespace mkpass {

enum class TargetPlatform {
    Auto,
    WindowsSetup,
    WindowsMsi,
    WindowsZip,
    LinuxAppImage,
    LinuxDeb,
    LinuxRpm,
    LinuxTarball
};

struct ReleaseAsset {
    QString name;
    QString downloadUrl;
    qint64 size = 0;
};

struct ReleaseInfo {
    QString version;
    QString tagName;
    QString releaseNotes;
    QString releaseUrl;
    QVector<ReleaseAsset> assets;
    ReleaseAsset optimalAsset;
};

class UpdateManager : public QObject {
    Q_OBJECT
public:
    explicit UpdateManager(QObject *parent = nullptr);
    ~UpdateManager() override;

    void checkForUpdatesInBackground();
    void checkForUpdatesInteractive();

    static bool ParseReleaseJson(const QByteArray& jsonData, ReleaseInfo& outInfo);
    static ReleaseAsset SelectOptimalAsset(const QVector<ReleaseAsset>& assets, TargetPlatform platform = TargetPlatform::Auto);
    static bool VerifyChecksum(const QString& filePath, const QByteArray& checksumsData, const QString& fileName);

signals:
    void updateAvailable(const ReleaseInfo &info);
    void upToDate();
    void checkFailed(const QString &errorReason);
    void downloadProgress(qint64 received, qint64 total);
    void downloadFinished(const QString &localFilePath);
    void downloadFailed(const QString &errorReason);

public slots:
    void startDownload(const ReleaseAsset &asset, const QVector<ReleaseAsset>& allAssets);
    void cancelDownload();
    void installDownloadedUpdate(const QString &localFilePath);

private slots:
    void onReleaseReplyFinished();
    void onAssetReadyRead();
    void onAssetDownloadFinished();
    void onChecksumsDownloadFinished();

private:
    void sendReleaseRequest();
    void checkDownloadCompletion();

    QNetworkAccessManager networkManager_;
    mkpass::Config config_;
    bool isManualCheck_ = false;

    QNetworkReply *currentReply_ = nullptr;
    QNetworkReply *downloadReply_ = nullptr;
    QNetworkReply *checksumsReply_ = nullptr;
    std::unique_ptr<QFile> downloadFile_;

    QString pendingDownloadPath_;
    QString pendingTargetFileName_;
    QByteArray pendingChecksumsData_;
    bool assetDownloaded_ = false;
    bool checksumsDownloaded_ = false;
    bool checksumsFailed_ = false;
};

} // namespace mkpass
