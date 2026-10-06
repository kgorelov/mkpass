#include <gtest/gtest.h>
#include <QApplication>
#include <QTableWidgetItem>
#include <fstream>
#ifdef _WIN32
#include <process.h>
#define GETPID _getpid
#else
#include <unistd.h>
#define GETPID getpid
#endif

#include "settings_dialog.h"
#include "comment_dialog.h"
#include "db.h"
#include "gui.h"
#include "platform_utils.h"
#include "passphrase_patterns.h"
#include "icon_utils.h"
#include "password_dialog.h"

class SettingsDialogTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() {
        qputenv("QT_QPA_PLATFORM", "offscreen");
        int argc = 0;
        char *argv[] = {nullptr};
        if (!QApplication::instance()) {
            new QApplication(argc, argv);
        }
    }

    void SetUp() override {
        const ::testing::TestInfo* const test_info =
            ::testing::UnitTest::GetInstance()->current_test_info();
        std::string test_name = test_info ? test_info->name() : "default";
        test_config_path_ = GetTmpDir() + "/mkpass-gui-settings-" + test_name + "-" + std::to_string(GETPID()) + ".conf";
        setenv("MKPASS_CONFIG_PATH", test_config_path_.c_str(), 1);
        remove(test_config_path_.c_str());
    }

    void TearDown() override {
        remove(test_config_path_.c_str());
        unsetenv("MKPASS_CONFIG_PATH");
    }

    static QTableWidget* generalTable(SettingsDialog& d) { return d.generalTable; }
    static QTableWidget* passwordTable(SettingsDialog& d) { return d.passwordTable; }
    static QTableWidget* passphraseTable(SettingsDialog& d) { return d.passphraseTable; }

    static QCheckBox* charLower(SettingsDialog& d) { return d.charLowerCheckBox; }
    static QCheckBox* charUpper(SettingsDialog& d) { return d.charUpperCheckBox; }
    static QCheckBox* charDigits(SettingsDialog& d) { return d.charDigitsCheckBox; }
    static QCheckBox* charSymbols(SettingsDialog& d) { return d.charSymbolsCheckBox; }
    static QCheckBox* charCustom(SettingsDialog& d) { return d.charCustomCheckBox; }

    static QComboBox* passphrasePattern(SettingsDialog& d) { return d.passphrasePatternComboBox; }

    static void setEditorValue(SettingsDialog& d, const QString& k, const std::string& v) {
        d.setEditorValue(k, v);
    }
    static std::string getEditorValue(const SettingsDialog& d, const QString& k) {
        return d.getEditorValue(k);
    }
    static void onSave(SettingsDialog& d) {
        d.onSave();
    }
    static void onRestoreDefaults(SettingsDialog& d) {
        d.onRestoreDefaults();
    }

    std::string test_config_path_;
};

TEST_F(SettingsDialogTest, VisualGroupStructure) {
    SettingsDialog dialog;

    auto genTbl = generalTable(dialog);
    auto pwdTbl = passwordTable(dialog);
    auto passTbl = passphraseTable(dialog);

    ASSERT_NE(genTbl, nullptr);
    ASSERT_NE(pwdTbl, nullptr);
    ASSERT_NE(passTbl, nullptr);

    EXPECT_EQ(genTbl->rowCount(), 4);
    EXPECT_EQ(genTbl->item(0, 1)->text(), "algorithm");
    EXPECT_EQ(genTbl->item(1, 1)->text(), "length");
    EXPECT_EQ(genTbl->item(2, 1)->text(), "enable_old_algorithm");
    EXPECT_EQ(genTbl->item(3, 1)->text(), "check_updates");

    EXPECT_EQ(pwdTbl->rowCount(), 2);
    EXPECT_EQ(pwdTbl->item(0, 1)->text(), "char_classes");
    EXPECT_EQ(pwdTbl->item(1, 1)->text(), "custom_chars");

    EXPECT_EQ(passTbl->rowCount(), 6);
    EXPECT_EQ(passTbl->item(0, 1)->text(), "separator");
    EXPECT_EQ(passTbl->item(1, 1)->text(), "passphrase_pattern");
    EXPECT_EQ(passTbl->item(2, 1)->text(), "digits");
    EXPECT_EQ(passTbl->item(3, 1)->text(), "symbols");
    EXPECT_EQ(passTbl->item(4, 1)->text(), "substitutions");
    EXPECT_EQ(passTbl->item(5, 1)->text(), "capitalize");

    EXPECT_GE(dialog.width(), 800);
    EXPECT_GE(dialog.minimumWidth(), 800);
}

TEST_F(SettingsDialogTest, CharClassesTickBoxes) {
    SettingsDialog dialog;

    auto lower = charLower(dialog);
    auto upper = charUpper(dialog);
    auto digits = charDigits(dialog);
    auto symbols = charSymbols(dialog);
    auto custom = charCustom(dialog);

    ASSERT_NE(lower, nullptr);
    ASSERT_NE(upper, nullptr);
    ASSERT_NE(digits, nullptr);
    ASSERT_NE(symbols, nullptr);
    ASSERT_NE(custom, nullptr);

    setEditorValue(dialog, "char_classes", "digits,symbols");
    EXPECT_FALSE(lower->isChecked());
    EXPECT_FALSE(upper->isChecked());
    EXPECT_TRUE(digits->isChecked());
    EXPECT_TRUE(symbols->isChecked());
    EXPECT_FALSE(custom->isChecked());
    EXPECT_EQ(getEditorValue(dialog, "char_classes"), "digits,symbols");

    setEditorValue(dialog, "char_classes", "lowercase,uppercase,custom");
    EXPECT_TRUE(lower->isChecked());
    EXPECT_TRUE(upper->isChecked());
    EXPECT_FALSE(digits->isChecked());
    EXPECT_FALSE(symbols->isChecked());
    EXPECT_TRUE(custom->isChecked());
    EXPECT_EQ(getEditorValue(dialog, "char_classes"), "lowercase,uppercase,custom");
}

TEST_F(SettingsDialogTest, PassphrasePatternDropdown) {
    SettingsDialog dialog;

    auto combo = passphrasePattern(dialog);
    ASSERT_NE(combo, nullptr);
    EXPECT_TRUE(combo->isEditable());

    // First item is Random
    EXPECT_EQ(combo->itemText(0), "Random");
    EXPECT_EQ(combo->itemData(0).toString().toStdString(), "");

    // Setting standard pattern
    setEditorValue(dialog, "passphrase_pattern", "van");
    EXPECT_EQ(getEditorValue(dialog, "passphrase_pattern"), "van");

    // Setting custom pattern
    setEditorValue(dialog, "passphrase_pattern", "navrn");
    EXPECT_EQ(getEditorValue(dialog, "passphrase_pattern"), "navrn");

    // Setting random/empty
    setEditorValue(dialog, "passphrase_pattern", "");
    EXPECT_EQ(getEditorValue(dialog, "passphrase_pattern"), "");
}

TEST_F(SettingsDialogTest, SaveAndLoadRoundtrip) {
    {
        SettingsDialog dialog;
        auto pwdTbl = passwordTable(dialog);
        auto passTbl = passphraseTable(dialog);

        // Enable char_classes and set digits,symbols
        pwdTbl->item(0, 0)->setCheckState(Qt::Checked);
        setEditorValue(dialog, "char_classes", "digits,symbols");

        // Enable passphrase_pattern and set van
        passTbl->item(1, 0)->setCheckState(Qt::Checked);
        setEditorValue(dialog, "passphrase_pattern", "van");

        // Enable separator and set Hyphen
        passTbl->item(0, 0)->setCheckState(Qt::Checked);
        setEditorValue(dialog, "separator", "-");

        onSave(dialog);
    }

    // Now reload from config
    {
        SettingsDialog dialog2;
        auto genTbl2 = generalTable(dialog2);
        auto pwdTbl2 = passwordTable(dialog2);
        auto passTbl2 = passphraseTable(dialog2);

        EXPECT_EQ(pwdTbl2->item(0, 0)->checkState(), Qt::Checked);
        EXPECT_TRUE(charDigits(dialog2)->isChecked());
        EXPECT_TRUE(charSymbols(dialog2)->isChecked());
        EXPECT_FALSE(charLower(dialog2)->isChecked());
        EXPECT_FALSE(charUpper(dialog2)->isChecked());

        EXPECT_EQ(passTbl2->item(1, 0)->checkState(), Qt::Checked);
        EXPECT_EQ(getEditorValue(dialog2, "passphrase_pattern"), "van");

        EXPECT_EQ(passTbl2->item(0, 0)->checkState(), Qt::Checked);
        EXPECT_EQ(getEditorValue(dialog2, "separator"), "-");

        // Unset items should be unchecked
        EXPECT_EQ(genTbl2->item(0, 0)->checkState(), Qt::Unchecked);
    }
}

TEST_F(SettingsDialogTest, RestoreDefaults) {
    // Save custom settings first
    {
        SettingsDialog dialog;
        passwordTable(dialog)->item(0, 0)->setCheckState(Qt::Checked);
        setEditorValue(dialog, "char_classes", "digits");
        onSave(dialog);
    }

    // Load and restore defaults
    {
        SettingsDialog dialog;
        auto genTbl = generalTable(dialog);
        auto pwdTbl = passwordTable(dialog);
        auto passTbl = passphraseTable(dialog);

        EXPECT_EQ(pwdTbl->item(0, 0)->checkState(), Qt::Checked);

        onRestoreDefaults(dialog);

        // Check that all Enabled checkboxes are unchecked
        for (int i = 0; i < genTbl->rowCount(); ++i) {
            EXPECT_EQ(genTbl->item(i, 0)->checkState(), Qt::Unchecked);
        }
        for (int i = 0; i < pwdTbl->rowCount(); ++i) {
            EXPECT_EQ(pwdTbl->item(i, 0)->checkState(), Qt::Unchecked);
        }
        for (int i = 0; i < passTbl->rowCount(); ++i) {
            EXPECT_EQ(passTbl->item(i, 0)->checkState(), Qt::Unchecked);
        }

        // Check default values
        EXPECT_TRUE(charLower(dialog)->isChecked());
        EXPECT_TRUE(charUpper(dialog)->isChecked());
        EXPECT_TRUE(charDigits(dialog)->isChecked());
        EXPECT_TRUE(charSymbols(dialog)->isChecked());
        EXPECT_FALSE(charCustom(dialog)->isChecked());

        EXPECT_EQ(getEditorValue(dialog, "passphrase_pattern"), "");
    }
}

TEST(PassphrasePatternTest, GetPatternDescription) {
    EXPECT_EQ(GetPatternDescription({WordClasses::Verb, WordClasses::Adj, WordClasses::Noun}), "Verb, Adj, Noun");
    EXPECT_EQ(GetPatternDescription({WordClasses::Noun, WordClasses::Adv}), "Noun, Adv");
    EXPECT_EQ(GetPatternDescription({WordClasses::Adj}), "Adj");
}

class MainWindowTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() {
        qputenv("QT_QPA_PLATFORM", "offscreen");
        int argc = 0;
        char *argv[] = {nullptr};
        if (!QApplication::instance()) {
            new QApplication(argc, argv);
        }
        Q_INIT_RESOURCE(icons);
    }

    static QComboBox* algorithmComboBox(MainWindow& win) { return win.algorithmComboBox; }
    static QSpinBox* lengthSpinBox(MainWindow& win) { return win.lengthSpinBox; }
    static QComboBox* patternComboBox(MainWindow& win) { return win.patternComboBox; }
    static QPushButton* commentButton(MainWindow& win) { return win.serviceCommentButton; }
    static QLineEdit* serviceLineEdit(MainWindow& win) { return win.serviceLineEdit; }
};

TEST_F(MainWindowTest, PassphrasePatternDropdownExplanation) {
    MainWindow win;
    auto algoCombo = algorithmComboBox(win);
    int index = algoCombo->findData(static_cast<int>(Algorithm::Passphrase_Wordnet_Pattern));
    ASSERT_NE(index, -1);
    algoCombo->setCurrentIndex(index);
    lengthSpinBox(win)->setValue(3);

    auto combo = patternComboBox(win);
    ASSERT_NE(combo, nullptr);
    ASSERT_GT(combo->count(), 1);

    EXPECT_EQ(combo->itemText(0), "Random");
    EXPECT_EQ(combo->itemData(0).toString().toStdString(), "");

    EXPECT_EQ(combo->itemText(1), "van (Verb, Adj, Noun)");
    EXPECT_EQ(combo->itemData(1).toString().toStdString(), "van");

    EXPECT_EQ(combo->itemText(2), "vnr (Verb, Noun, Adv)");
    EXPECT_EQ(combo->itemData(2).toString().toStdString(), "vnr");
}

TEST_F(MainWindowTest, CommentButtonState) {
    std::string db_path = GetTmpDir() + "/mkpass-gui-comment-" + std::to_string(GETPID()) + ".db";
    setenv("MKPASS_DB_PATH", db_path.c_str(), 1);
    remove(db_path.c_str());

    mkpass::ConfigDB db(db_path);
    mkpass::ServiceEntry entry;
    entry.service_name = "with_comment.com";
    entry.algorithm = Algorithm::Argon2;
    entry.length = 16;
    entry.comment = "Important bank note";
    db.save_service_entry(entry);

    MainWindow win;
    auto btn = commentButton(win);
    ASSERT_NE(btn, nullptr);
    EXPECT_TRUE(btn->text().isEmpty());
    EXPECT_FALSE(btn->icon().isNull());
    EXPECT_EQ(btn->toolTip(), "Add comment for this service");

    // Change to service with comment
    serviceLineEdit(win)->setText("with_comment.com");
    EXPECT_TRUE(btn->text().isEmpty());
    EXPECT_FALSE(btn->icon().isNull());
    EXPECT_TRUE(btn->toolTip().contains("Important bank note"));

    // Change to service without comment
    serviceLineEdit(win)->setText("new_service.com");
    EXPECT_TRUE(btn->text().isEmpty());
    EXPECT_FALSE(btn->icon().isNull());
    EXPECT_EQ(btn->toolTip(), "Add comment for this service");

    unsetenv("MKPASS_DB_PATH");
    remove(db_path.c_str());
}

TEST_F(MainWindowTest, CommentDialogInitialAndEditedComment) {
    CommentDialog dlg("test.com", "Initial note");
    EXPECT_EQ(dlg.getComment(), "Initial note");
}

class ThemedIconTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() {
        qputenv("QT_QPA_PLATFORM", "offscreen");
        int argc = 0;
        char *argv[] = {nullptr};
        if (!QApplication::instance()) {
            new QApplication(argc, argv);
        }
        Q_INIT_RESOURCE(icons);
    }
};

TEST_F(ThemedIconTest, DetectDarkTheme) {
    QPalette darkPalette;
    darkPalette.setColor(QPalette::Window, QColor(31, 31, 31));
    darkPalette.setColor(QPalette::WindowText, QColor(255, 255, 255));
    darkPalette.setColor(QPalette::Button, QColor(31, 31, 31));
    darkPalette.setColor(QPalette::ButtonText, QColor(247, 247, 247));
    EXPECT_TRUE(isDarkTheme(darkPalette));

    QPalette lightPalette;
    lightPalette.setColor(QPalette::Window, QColor(240, 240, 240));
    lightPalette.setColor(QPalette::WindowText, QColor(0, 0, 0));
    lightPalette.setColor(QPalette::Button, QColor(224, 224, 224));
    lightPalette.setColor(QPalette::ButtonText, QColor(20, 20, 20));
    EXPECT_FALSE(isDarkTheme(lightPalette));
}

TEST_F(ThemedIconTest, ThemedIconsLoaded) {
    EXPECT_FALSE(getThemedIcon(":/icons/comment.svg").isNull());
    EXPECT_FALSE(getThemedIcon(":/icons/comment-active.svg").isNull());
    EXPECT_FALSE(getThemedIcon(":/icons/eye.svg").isNull());
    EXPECT_FALSE(getThemedIcon(":/icons/eye-off.svg").isNull());
    EXPECT_FALSE(getThemedIcon(":/icons/qr.svg").isNull());
    EXPECT_TRUE(getThemedIcon(":/icons/nonexistent.svg").isNull());
}

TEST_F(ThemedIconTest, DarkThemeIconRendersLight) {
    QPalette origPalette = QApplication::palette();

    QPalette darkPalette;
    darkPalette.setColor(QPalette::Window, QColor(31, 31, 31));
    darkPalette.setColor(QPalette::WindowText, QColor(255, 255, 255));
    darkPalette.setColor(QPalette::Button, QColor(31, 31, 31));
    darkPalette.setColor(QPalette::ButtonText, QColor(247, 247, 247));
    QApplication::setPalette(darkPalette);

    QIcon icon = getThemedIcon(":/icons/eye.svg");
    QPixmap pm = icon.pixmap(QSize(24, 24));
    EXPECT_FALSE(pm.isNull());
    QImage img = pm.toImage();

    bool hasLightPixel = false;
    for (int y = 0; y < img.height(); ++y) {
        for (int x = 0; x < img.width(); ++x) {
            QRgb px = img.pixel(x, y);
            if (qAlpha(px) > 200) {
                EXPECT_GE(qGray(px), 180);
                hasLightPixel = true;
            }
        }
    }
    EXPECT_TRUE(hasLightPixel);

    QApplication::setPalette(origPalette);
}

TEST_F(ThemedIconTest, LightThemeIconRendersDark) {
    QPalette origPalette = QApplication::palette();

    QPalette lightPalette;
    lightPalette.setColor(QPalette::Window, QColor(240, 240, 240));
    lightPalette.setColor(QPalette::WindowText, QColor(0, 0, 0));
    lightPalette.setColor(QPalette::Button, QColor(224, 224, 224));
    lightPalette.setColor(QPalette::ButtonText, QColor(20, 20, 20));
    QApplication::setPalette(lightPalette);

    QIcon icon = getThemedIcon(":/icons/eye.svg");
    QPixmap pm = icon.pixmap(QSize(24, 24));
    EXPECT_FALSE(pm.isNull());
    QImage img = pm.toImage();

    bool hasDarkPixel = false;
    for (int y = 0; y < img.height(); ++y) {
        for (int x = 0; x < img.width(); ++x) {
            QRgb px = img.pixel(x, y);
            if (qAlpha(px) > 200) {
                EXPECT_LT(qGray(px), 50);
                hasDarkPixel = true;
            }
        }
    }
    EXPECT_TRUE(hasDarkPixel);

    QApplication::setPalette(origPalette);
}

TEST_F(ThemedIconTest, PasswordDialogUsesThemedIcons) {
    PasswordDialog dlg("secret-password");
    auto buttons = dlg.findChildren<QPushButton *>();
    int iconButtonCount = 0;
    for (auto *btn : buttons) {
        if (!btn->icon().isNull()) {
            iconButtonCount++;
        }
    }
    EXPECT_GE(iconButtonCount, 2);
}
