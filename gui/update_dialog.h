#pragma once

#include <QDialog>
#include <QLabel>
#include <QTextBrowser>
#include <QCheckBox>
#include <QPushButton>
#include <QProgressBar>

#include "update_manager.h"

namespace mkpass {

class UpdateDialog : public QDialog {
    Q_OBJECT
public:
    UpdateDialog(const ReleaseInfo &info, UpdateManager *manager, QWidget *parent = nullptr);
    ~UpdateDialog() override;

protected:
    void closeEvent(QCloseEvent *event) override;
    void reject() override;

private slots:
    void onDownloadClicked();
    void onCancelDownloadClicked();
    void onRemindLaterClicked();
    void onViewOnGitHubClicked();
    void onDownloadProgress(qint64 received, qint64 total);
    void onDownloadFinished(const QString &localFilePath);
    void onDownloadFailed(const QString &errorReason);

private:
    void setupUI();
    void applyUserPreferences();

    ReleaseInfo info_;
    UpdateManager *manager_;
    bool isDownloading_ = false;

    QLabel *headerLabel_ = nullptr;
    QLabel *assetDetailsLabel_ = nullptr;
    QTextBrowser *releaseNotesBrowser_ = nullptr;
    QCheckBox *skipVersionCheckBox_ = nullptr;
    QCheckBox *disableChecksCheckBox_ = nullptr;

    QWidget *actionButtonsWidget_ = nullptr;
    QPushButton *downloadButton_ = nullptr;
    QPushButton *remindLaterButton_ = nullptr;
    QPushButton *viewGitHubButton_ = nullptr;

    QWidget *progressWidget_ = nullptr;
    QLabel *progressStatusLabel_ = nullptr;
    QProgressBar *progressBar_ = nullptr;
    QPushButton *cancelDownloadButton_ = nullptr;
};

} // namespace mkpass
