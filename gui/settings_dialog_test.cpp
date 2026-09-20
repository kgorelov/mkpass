#include <gtest/gtest.h>
#include <QApplication>
#include <QTableWidgetItem>
#include <fstream>

#include "settings_dialog.h"
#include "platform_utils.h"
#include "passphrase_patterns.h"

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
        test_config_path_ = GetTmpDir() + "/mkpass-gui-settings-test.conf";
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

    EXPECT_EQ(genTbl->rowCount(), 3);
    EXPECT_EQ(genTbl->item(0, 1)->text(), "algorithm");
    EXPECT_EQ(genTbl->item(1, 1)->text(), "length");
    EXPECT_EQ(genTbl->item(2, 1)->text(), "enable_old_algorithm");

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
