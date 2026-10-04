#include <gtest/gtest.h>
#include <QApplication>
#include <QTemporaryFile>
#include <QCryptographicHash>
#include <QFile>
#include <QFileInfo>

#include "update_manager.h"
#include "update_dialog.h"

class UpdateManagerTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() {
        qputenv("QT_QPA_PLATFORM", "offscreen");
        int argc = 0;
        char *argv[] = {nullptr};
        if (!QApplication::instance()) {
            new QApplication(argc, argv);
        }
    }
};

TEST_F(UpdateManagerTest, SelectOptimalAssetWindows) {
    QVector<mkpass::ReleaseAsset> assets = {
        {"SHA256SUMS.txt", "https://example.com/SHA256SUMS.txt", 500},
        {"mkpass-0.2.0-windows-x64.zip", "https://example.com/mkpass-0.2.0-windows-x64.zip", 10000000},
        {"mkpass-0.2.0-windows-x64.msi", "https://example.com/mkpass-0.2.0-windows-x64.msi", 12000000},
        {"mkpass-0.2.0-windows-x64-setup.exe", "https://example.com/mkpass-0.2.0-windows-x64-setup.exe", 15000000},
        {"mkpass-0.2.0-linux-x86_64.AppImage", "https://example.com/mkpass-0.2.0-linux-x86_64.AppImage", 20000000}
    };

    auto setupAsset = mkpass::UpdateManager::SelectOptimalAsset(assets, mkpass::TargetPlatform::WindowsSetup);
    EXPECT_EQ(setupAsset.name, "mkpass-0.2.0-windows-x64-setup.exe");

    auto msiAsset = mkpass::UpdateManager::SelectOptimalAsset(assets, mkpass::TargetPlatform::WindowsMsi);
    EXPECT_EQ(msiAsset.name, "mkpass-0.2.0-windows-x64.msi");

    auto zipAsset = mkpass::UpdateManager::SelectOptimalAsset(assets, mkpass::TargetPlatform::WindowsZip);
    EXPECT_EQ(zipAsset.name, "mkpass-0.2.0-windows-x64.zip");
}

TEST_F(UpdateManagerTest, SelectOptimalAssetLinux) {
    QVector<mkpass::ReleaseAsset> assets = {
        {"SHA256SUMS.txt", "https://example.com/SHA256SUMS.txt", 500},
        {"mkpass-0.2.0-linux-x86_64-bundle.tar.gz", "https://example.com/bundle.tar.gz", 15000000},
        {"mkpass-0.2.0-linux-x86_64.AppImage", "https://example.com/mkpass.AppImage", 25000000},
        {"mkpass-gui_0.2.0-1_amd64.deb", "https://example.com/mkpass-gui.deb", 8000000},
        {"mkpass_0.2.0-1_amd64.deb", "https://example.com/mkpass-cli.deb", 4000000},
        {"mkpass-gui-0.2.0-1.x86_64.rpm", "https://example.com/mkpass-gui.rpm", 8000000},
        {"mkpass-0.2.0-1.x86_64.rpm", "https://example.com/mkpass-cli.rpm", 4000000}
    };

    auto debAsset = mkpass::UpdateManager::SelectOptimalAsset(assets, mkpass::TargetPlatform::LinuxDeb);
    EXPECT_EQ(debAsset.name, "mkpass-gui_0.2.0-1_amd64.deb");

    auto rpmAsset = mkpass::UpdateManager::SelectOptimalAsset(assets, mkpass::TargetPlatform::LinuxRpm);
    EXPECT_EQ(rpmAsset.name, "mkpass-gui-0.2.0-1.x86_64.rpm");

    auto appImageAsset = mkpass::UpdateManager::SelectOptimalAsset(assets, mkpass::TargetPlatform::LinuxAppImage);
    EXPECT_EQ(appImageAsset.name, "mkpass-0.2.0-linux-x86_64.AppImage");

    auto tarballAsset = mkpass::UpdateManager::SelectOptimalAsset(assets, mkpass::TargetPlatform::LinuxTarball);
    EXPECT_EQ(tarballAsset.name, "mkpass-0.2.0-linux-x86_64-bundle.tar.gz");
}

TEST_F(UpdateManagerTest, SelectOptimalAssetEmptyOrNoMatch) {
    QVector<mkpass::ReleaseAsset> empty;
    auto res = mkpass::UpdateManager::SelectOptimalAsset(empty, mkpass::TargetPlatform::WindowsSetup);
    EXPECT_TRUE(res.name.isEmpty());

    QVector<mkpass::ReleaseAsset> onlyChecksum = {
        {"SHA256SUMS.txt", "https://example.com/SHA256SUMS.txt", 500}
    };
    auto fallback = mkpass::UpdateManager::SelectOptimalAsset(onlyChecksum, mkpass::TargetPlatform::LinuxDeb);
    EXPECT_EQ(fallback.name, "SHA256SUMS.txt");
}

TEST_F(UpdateManagerTest, ParseReleaseJsonSuccess) {
    QByteArray json = R"({
        "tag_name": "v0.3.0",
        "body": "## Release Notes\n* New features added",
        "html_url": "https://github.com/kgorelov/mkpass/releases/tag/v0.3.0",
        "assets": [
            {
                "name": "mkpass-0.3.0-windows-x64-setup.exe",
                "browser_download_url": "https://github.com/kgorelov/mkpass/releases/download/v0.3.0/mkpass-0.3.0-windows-x64-setup.exe",
                "size": 12345678
            },
            {
                "name": "SHA256SUMS.txt",
                "browser_download_url": "https://github.com/kgorelov/mkpass/releases/download/v0.3.0/SHA256SUMS.txt",
                "size": 256
            }
        ]
    })";

    mkpass::ReleaseInfo info;
    EXPECT_TRUE(mkpass::UpdateManager::ParseReleaseJson(json, info));
    EXPECT_EQ(info.tagName, "v0.3.0");
    EXPECT_EQ(info.version, "0.3.0");
    EXPECT_EQ(info.releaseNotes, "## Release Notes\n* New features added");
    EXPECT_EQ(info.releaseUrl, "https://github.com/kgorelov/mkpass/releases/tag/v0.3.0");
    EXPECT_EQ(info.assets.size(), 2);
}

TEST_F(UpdateManagerTest, ParseReleaseJsonMalformed) {
    mkpass::ReleaseInfo info;
    EXPECT_FALSE(mkpass::UpdateManager::ParseReleaseJson("{ invalid json ", info));
    EXPECT_FALSE(mkpass::UpdateManager::ParseReleaseJson("[]", info));
    EXPECT_FALSE(mkpass::UpdateManager::ParseReleaseJson(R"({"body": "missing tag"})", info));
}

TEST_F(UpdateManagerTest, VerifyChecksumValidAndInvalid) {
    QTemporaryFile tempFile;
    ASSERT_TRUE(tempFile.open());
    const QByteArray payload = "mkpass update payload test content\n";
    tempFile.write(payload);
    tempFile.flush();
    QString filePath = tempFile.fileName();
    QString fileName = QFileInfo(tempFile).fileName();

    QByteArray expectedHash = QCryptographicHash::hash(payload, QCryptographicHash::Sha256).toHex();

    // 1. Valid checksums file entry (<hash>  <filename>)
    QByteArray checksumsData = expectedHash + "  " + fileName.toUtf8() + "\n";
    EXPECT_TRUE(mkpass::UpdateManager::VerifyChecksum(filePath, checksumsData, fileName));

    // 2. Valid with asterisk (<hash> *<filename>)
    QByteArray checksumsAsterisk = expectedHash + " *" + fileName.toUtf8() + "\n";
    EXPECT_TRUE(mkpass::UpdateManager::VerifyChecksum(filePath, checksumsAsterisk, fileName));

    // 3. Valid with uppercase hash
    QByteArray checksumsUpper = expectedHash.toUpper() + "  " + fileName.toUtf8() + "\n";
    EXPECT_TRUE(mkpass::UpdateManager::VerifyChecksum(filePath, checksumsUpper, fileName));

    // 4. Invalid hash
    QByteArray badHashData = "0000000000000000000000000000000000000000000000000000000000000000  " + fileName.toUtf8() + "\n";
    EXPECT_FALSE(mkpass::UpdateManager::VerifyChecksum(filePath, badHashData, fileName));

    // 5. Different file name in checksums
    QByteArray wrongFileChecksums = expectedHash + "  different_file.exe\n";
    EXPECT_FALSE(mkpass::UpdateManager::VerifyChecksum(filePath, wrongFileChecksums, fileName));

    // 6. Non-existent file
    EXPECT_FALSE(mkpass::UpdateManager::VerifyChecksum("/nonexistent/file/path.exe", checksumsData, fileName));
}

TEST_F(UpdateManagerTest, UpdateDialogConstruction) {
    mkpass::ReleaseInfo info;
    info.version = "0.2.0";
    info.tagName = "v0.2.0";
    info.releaseNotes = "### Improvements\n- Faster generation";
    info.releaseUrl = "https://github.com/kgorelov/mkpass/releases/tag/v0.2.0";
    info.optimalAsset = {"mkpass-0.2.0-setup.exe", "https://example.com/setup.exe", 10485760};

    mkpass::UpdateManager manager;
    mkpass::UpdateDialog dialog(info, &manager);
    EXPECT_EQ(dialog.windowTitle(), "Software Update");
}
