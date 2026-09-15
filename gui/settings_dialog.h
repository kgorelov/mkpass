#pragma once

#include <QDialog>
#include <QTableWidget>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QSpinBox>
#include <QVector>
#include <QString>

#include "config.h"

class SettingsDialog : public QDialog {
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget *parent = nullptr);
    ~SettingsDialog() override = default;

private slots:
    void onRestoreDefaults();
    void onSave();

private:
    void setupUI();
    void loadFromConfig();
    void updateRowAppearance(int row);
    void updateAlgorithmChoices();
    std::string getEditorValue(int row) const;
    void setEditorValue(int row, const std::string& value);

    QTableWidget *tableWidget;
    QPushButton *restoreDefaultsButton;
    QPushButton *saveButton;
    QPushButton *cancelButton;

    QComboBox *algorithmComboBox;
    QLineEdit *charClassesLineEdit;
    QLineEdit *customCharsLineEdit;
    QSpinBox *lengthSpinBox;
    QComboBox *separatorComboBox;
    QLineEdit *passphrasePatternLineEdit;
    QComboBox *digitsComboBox;
    QComboBox *symbolsComboBox;
    QComboBox *substitutionsComboBox;
    QComboBox *capitalizeComboBox;
    QComboBox *enableOldAlgoComboBox;

    struct SettingRowDef {
        QString key;
        QWidget *editor;
    };
    QVector<SettingRowDef> rowDefs_;
    mkpass::Config config_;
};
