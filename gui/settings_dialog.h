#pragma once

#include <QDialog>
#include <QTableWidget>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QSpinBox>
#include <QCheckBox>
#include <QScrollArea>
#include <QGroupBox>
#include <QVector>
#include <QString>

#include "config.h"
#include "passphrase_patterns.h"

QString GetPatternDescription(const PassphrasePattern& pattern);

class SettingsDialog : public QDialog {
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget *parent = nullptr);
    ~SettingsDialog() override = default;

    friend class SettingsDialogTest;

private slots:
    void onRestoreDefaults();
    void onSave();

private:
    void setupUI();
    void loadFromConfig();
    void updateRowAppearance(QTableWidget *table, int row);
    void updateAlgorithmChoices();
    std::string getEditorValue(const QString& key) const;
    void setEditorValue(const QString& key, const std::string& value);

    QScrollArea *scrollArea;
    QTableWidget *generalTable;
    QTableWidget *passwordTable;
    QTableWidget *passphraseTable;

    QPushButton *restoreDefaultsButton;
    QPushButton *saveButton;
    QPushButton *cancelButton;

    // General editors
    QComboBox *algorithmComboBox;
    QSpinBox *lengthSpinBox;
    QComboBox *enableOldAlgoComboBox;

    // Password editors
    QWidget *charClassesWidget;
    QCheckBox *charLowerCheckBox;
    QCheckBox *charUpperCheckBox;
    QCheckBox *charDigitsCheckBox;
    QCheckBox *charSymbolsCheckBox;
    QCheckBox *charCustomCheckBox;
    QLineEdit *customCharsLineEdit;

    // Passphrase editors
    QComboBox *separatorComboBox;
    QComboBox *passphrasePatternComboBox;
    QComboBox *digitsComboBox;
    QComboBox *symbolsComboBox;
    QComboBox *substitutionsComboBox;
    QComboBox *capitalizeComboBox;

    struct SettingRowDef {
        QString key;
        QWidget *editor;
        QTableWidget *table;
        int rowInTable;
    };
    QVector<SettingRowDef> rowDefs_;
    mkpass::Config config_;
};
