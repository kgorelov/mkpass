#include "update_dialog.h"
#include "platform_utils.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDesktopServices>
#include <QMessageBox>
#include <QUrl>
#include <QCloseEvent>

namespace mkpass {

UpdateDialog::UpdateDialog(const ReleaseInfo &info, UpdateManager *manager, QWidget *parent)
    : QDialog(parent),
      info_(info),
      manager_(manager) {
    setupUI();

    if (manager_) {
        connect(manager_, &UpdateManager::downloadProgress, this, &UpdateDialog::onDownloadProgress);
        connect(manager_, &UpdateManager::downloadFinished, this, &UpdateDialog::onDownloadFinished);
        connect(manager_, &UpdateManager::downloadFailed, this, &UpdateDialog::onDownloadFailed);
    }
}

UpdateDialog::~UpdateDialog() {
    if (manager_) {
        disconnect(manager_, &UpdateManager::downloadProgress, this, &UpdateDialog::onDownloadProgress);
        disconnect(manager_, &UpdateManager::downloadFinished, this, &UpdateDialog::onDownloadFinished);
        disconnect(manager_, &UpdateManager::downloadFailed, this, &UpdateDialog::onDownloadFailed);
    }
}

void UpdateDialog::setupUI() {
    setWindowTitle("Software Update");
    setWindowIcon(QIcon(":/app_icon"));
    resize(620, 480);
    setMinimumSize(540, 400);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(12);

    QHBoxLayout *headerLayout = new QHBoxLayout;
    QLabel *iconLabel = new QLabel(this);
    iconLabel->setPixmap(QPixmap(":/app_icon").scaled(48, 48, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    iconLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    QString headerText = QString(
        "<b><font size=\"+1\">A new version of mkpass is available!</font></b><br>"
        "mkpass <b>%1</b> is now available (you have %2). Would you like to download it now?"
    ).arg(info_.version, MKPASS_VERSION);

    headerLabel_ = new QLabel(headerText, this);
    headerLabel_->setWordWrap(true);

    headerLayout->addWidget(iconLabel);
    headerLayout->addSpacing(12);
    headerLayout->addWidget(headerLabel_, 1);
    mainLayout->addLayout(headerLayout);

    QLabel *notesLabel = new QLabel("Release Notes:", this);
    mainLayout->addWidget(notesLabel);

    releaseNotesBrowser_ = new QTextBrowser(this);
    releaseNotesBrowser_->setOpenExternalLinks(true);
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    releaseNotesBrowser_->setMarkdown(info_.releaseNotes);
#else
    releaseNotesBrowser_->setPlainText(info_.releaseNotes);
#endif
    mainLayout->addWidget(releaseNotesBrowser_, 1);

    if (!info_.optimalAsset.name.isEmpty()) {
        double sizeMB = info_.optimalAsset.size / (1024.0 * 1024.0);
        assetDetailsLabel_ = new QLabel(
            QString("Package: <b>%1</b> (%2 MB)")
                .arg(info_.optimalAsset.name)
                .arg(QString::number(sizeMB, 'f', 1)),
            this);
        mainLayout->addWidget(assetDetailsLabel_);
    }

    skipVersionCheckBox_ = new QCheckBox(QString("Skip this version (v%1)").arg(info_.version), this);
    disableChecksCheckBox_ = new QCheckBox("Don't check for updates automatically", this);
    mainLayout->addWidget(skipVersionCheckBox_);
    mainLayout->addWidget(disableChecksCheckBox_);

    // Progress Section (hidden initially)
    progressWidget_ = new QWidget(this);
    QVBoxLayout *progressLayout = new QVBoxLayout(progressWidget_);
    progressLayout->setContentsMargins(0, 0, 0, 0);

    progressStatusLabel_ = new QLabel("Downloading update...", progressWidget_);
    progressBar_ = new QProgressBar(progressWidget_);
    progressBar_->setRange(0, 100);
    progressBar_->setValue(0);

    QHBoxLayout *cancelLayout = new QHBoxLayout;
    cancelDownloadButton_ = new QPushButton("Cancel", progressWidget_);
    connect(cancelDownloadButton_, &QPushButton::clicked, this, &UpdateDialog::onCancelDownloadClicked);
    cancelLayout->addStretch();
    cancelLayout->addWidget(cancelDownloadButton_);

    progressLayout->addWidget(progressStatusLabel_);
    progressLayout->addWidget(progressBar_);
    progressLayout->addLayout(cancelLayout);
    progressWidget_->hide();
    mainLayout->addWidget(progressWidget_);

    // Action buttons section
    actionButtonsWidget_ = new QWidget(this);
    QHBoxLayout *buttonsLayout = new QHBoxLayout(actionButtonsWidget_);
    buttonsLayout->setContentsMargins(0, 0, 0, 0);

    viewGitHubButton_ = new QPushButton("View on GitHub", actionButtonsWidget_);
    connect(viewGitHubButton_, &QPushButton::clicked, this, &UpdateDialog::onViewOnGitHubClicked);

    remindLaterButton_ = new QPushButton("Remind Me Later", actionButtonsWidget_);
    connect(remindLaterButton_, &QPushButton::clicked, this, &UpdateDialog::onRemindLaterClicked);

    downloadButton_ = new QPushButton("Download and Install", actionButtonsWidget_);
    downloadButton_->setDefault(true);
    connect(downloadButton_, &QPushButton::clicked, this, &UpdateDialog::onDownloadClicked);

    buttonsLayout->addWidget(viewGitHubButton_);
    buttonsLayout->addStretch();
    buttonsLayout->addWidget(remindLaterButton_);
    buttonsLayout->addWidget(downloadButton_);
    mainLayout->addWidget(actionButtonsWidget_);
}

void UpdateDialog::onDownloadClicked() {
    if (!manager_) return;

    isDownloading_ = true;
    actionButtonsWidget_->hide();
    progressWidget_->show();
    skipVersionCheckBox_->setEnabled(false);
    disableChecksCheckBox_->setEnabled(false);

    progressStatusLabel_->setText(QString("Downloading %1...").arg(info_.optimalAsset.name));
    progressBar_->setRange(0, 100);
    progressBar_->setValue(0);

    manager_->startDownload(info_.optimalAsset, info_.assets);
}

void UpdateDialog::onCancelDownloadClicked() {
    if (manager_) {
        manager_->cancelDownload();
    }
    isDownloading_ = false;
    progressWidget_->hide();
    actionButtonsWidget_->show();
    skipVersionCheckBox_->setEnabled(true);
    disableChecksCheckBox_->setEnabled(true);
}

void UpdateDialog::onDownloadProgress(qint64 received, qint64 total) {
    if (total > 0) {
        progressBar_->setRange(0, 100);
        int percent = static_cast<int>((received * 100) / total);
        progressBar_->setValue(percent);
        double recMB = received / (1024.0 * 1024.0);
        double totMB = total / (1024.0 * 1024.0);
        progressStatusLabel_->setText(
            QString("Downloading %1: %2 MB / %3 MB (%4%)")
                .arg(info_.optimalAsset.name)
                .arg(QString::number(recMB, 'f', 1))
                .arg(QString::number(totMB, 'f', 1))
                .arg(percent));
    } else {
        progressBar_->setRange(0, 0);
        double recMB = received / (1024.0 * 1024.0);
        progressStatusLabel_->setText(
            QString("Downloading %1: %2 MB")
                .arg(info_.optimalAsset.name)
                .arg(QString::number(recMB, 'f', 1)));
    }
}

void UpdateDialog::onDownloadFinished(const QString &localFilePath) {
    progressStatusLabel_->setText("Checksum verified. Starting installer...");
    applyUserPreferences();
    if (manager_) {
        manager_->installDownloadedUpdate(localFilePath);
    }
    accept();
}

void UpdateDialog::onDownloadFailed(const QString &errorReason) {
    isDownloading_ = false;
    progressWidget_->hide();
    actionButtonsWidget_->show();
    skipVersionCheckBox_->setEnabled(true);
    disableChecksCheckBox_->setEnabled(true);

    QMessageBox::critical(this, "Update Error", errorReason);
}

void UpdateDialog::onRemindLaterClicked() {
    applyUserPreferences();
    reject();
}

void UpdateDialog::onViewOnGitHubClicked() {
    QUrl url(info_.releaseUrl.isEmpty() ? "https://github.com/kgorelov/mkpass/releases" : info_.releaseUrl);
    QDesktopServices::openUrl(url);
}

void UpdateDialog::applyUserPreferences() {
    if (skipVersionCheckBox_ && skipVersionCheckBox_->isChecked()) {
        UpdateState state;
        state.load();
        state.skipped_version = info_.version.toStdString();
        state.save();
    }
    if (disableChecksCheckBox_ && disableChecksCheckBox_->isChecked()) {
        Config cfg(GetConfigFilePath());
        cfg.load();
        cfg.set_raw("check_updates", "false");
        cfg.save();
    }
}

void UpdateDialog::closeEvent(QCloseEvent *event) {
    if (isDownloading_ && manager_) {
        manager_->cancelDownload();
    }
    applyUserPreferences();
    event->accept();
}

void UpdateDialog::reject() {
    if (isDownloading_ && manager_) {
        manager_->cancelDownload();
    }
    applyUserPreferences();
    QDialog::reject();
}

} // namespace mkpass
