#include "settings_dialog.h"
#include "platform_utils.h"
#include "algorithms.h"
#include "character_classes.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QLabel>

SettingsDialog::SettingsDialog(QWidget *parent)
    : QDialog(parent),
      tableWidget(nullptr),
      restoreDefaultsButton(nullptr),
      saveButton(nullptr),
      cancelButton(nullptr),
      algorithmComboBox(nullptr),
      charClassesLineEdit(nullptr),
      customCharsLineEdit(nullptr),
      lengthSpinBox(nullptr),
      separatorComboBox(nullptr),
      passphrasePatternLineEdit(nullptr),
      digitsComboBox(nullptr),
      symbolsComboBox(nullptr),
      substitutionsComboBox(nullptr),
      capitalizeComboBox(nullptr),
      enableOldAlgoComboBox(nullptr),
      config_(GetConfigFilePath()) {
    setupUI();
    loadFromConfig();
}

void SettingsDialog::setupUI() {
    setWindowTitle("Preferences");
    setModal(true);
    resize(620, 480);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    tableWidget = new QTableWidget(this);
    tableWidget->setColumnCount(3);
    tableWidget->setHorizontalHeaderLabels({"Enabled", "Variable Name", "Default Value"});
    tableWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    tableWidget->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    tableWidget->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    tableWidget->verticalHeader()->setVisible(false);
    tableWidget->setSelectionMode(QAbstractItemView::NoSelection);
    tableWidget->setShowGrid(true);

    algorithmComboBox = new QComboBox(this);

    charClassesLineEdit = new QLineEdit(this);
    charClassesLineEdit->setPlaceholderText("lowercase,uppercase,digits,symbols");

    customCharsLineEdit = new QLineEdit(this);
    customCharsLineEdit->setPlaceholderText("e.g. !@#$%");

    lengthSpinBox = new QSpinBox(this);
    lengthSpinBox->setRange(1, 128);
    lengthSpinBox->setValue(16);

    separatorComboBox = new QComboBox(this);
    separatorComboBox->addItem("None", QString(""));
    separatorComboBox->addItem("Hyphen (-)", QString("-"));
    separatorComboBox->addItem("Space ( )", QString(" "));
    separatorComboBox->addItem("Slash (/)", QString("/"));

    passphrasePatternLineEdit = new QLineEdit(this);
    passphrasePatternLineEdit->setPlaceholderText("e.g. navrn (or empty for Random)");

    auto make_bool_combo = [this]() {
        QComboBox *cb = new QComboBox(this);
        cb->addItem("false", QString("false"));
        cb->addItem("true", QString("true"));
        return cb;
    };

    digitsComboBox = make_bool_combo();
    symbolsComboBox = make_bool_combo();
    substitutionsComboBox = make_bool_combo();
    capitalizeComboBox = make_bool_combo();
    enableOldAlgoComboBox = make_bool_combo();

    connect(enableOldAlgoComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &SettingsDialog::updateAlgorithmChoices);

    rowDefs_ = {
        {"algorithm", algorithmComboBox},
        {"char_classes", charClassesLineEdit},
        {"custom_chars", customCharsLineEdit},
        {"length", lengthSpinBox},
        {"separator", separatorComboBox},
        {"passphrase_pattern", passphrasePatternLineEdit},
        {"digits", digitsComboBox},
        {"symbols", symbolsComboBox},
        {"substitutions", substitutionsComboBox},
        {"capitalize", capitalizeComboBox},
        {"enable_old_algorithm", enableOldAlgoComboBox}
    };

    tableWidget->setRowCount(rowDefs_.size());

    for (int i = 0; i < rowDefs_.size(); ++i) {
        QTableWidgetItem *checkItem = new QTableWidgetItem();
        checkItem->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
        checkItem->setCheckState(Qt::Unchecked);
        tableWidget->setItem(i, 0, checkItem);

        QTableWidgetItem *nameItem = new QTableWidgetItem(rowDefs_[i].key);
        nameItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        tableWidget->setItem(i, 1, nameItem);

        tableWidget->setCellWidget(i, 2, rowDefs_[i].editor);
    }

    connect(tableWidget, &QTableWidget::itemChanged, this, [this](QTableWidgetItem *item) {
        if (item && item->column() == 0) {
            updateRowAppearance(item->row());
        }
    });

    mainLayout->addWidget(tableWidget);

    QHBoxLayout *buttonLayout = new QHBoxLayout;
    restoreDefaultsButton = new QPushButton("Restore Defaults", this);
    saveButton = new QPushButton("Save", this);
    saveButton->setDefault(true);
    cancelButton = new QPushButton("Cancel", this);

    connect(restoreDefaultsButton, &QPushButton::clicked, this, &SettingsDialog::onRestoreDefaults);
    connect(saveButton, &QPushButton::clicked, this, &SettingsDialog::onSave);
    connect(cancelButton, &QPushButton::clicked, this, &SettingsDialog::reject);

    buttonLayout->addWidget(restoreDefaultsButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(saveButton);
    buttonLayout->addWidget(cancelButton);

    mainLayout->addLayout(buttonLayout);
}

void SettingsDialog::updateAlgorithmChoices() {
    bool old_enabled = mkpass::IsOldAlgorithmEnabled(config_);
    if (enableOldAlgoComboBox && enableOldAlgoComboBox->currentData().toString() == "true") {
        old_enabled = true;
    }

    QString currentVal = algorithmComboBox ? algorithmComboBox->currentData().toString() : QString();
    algorithmComboBox->blockSignals(true);
    algorithmComboBox->clear();
    algorithmComboBox->addItem("Password (Argon2)", QString("password/argon2"));
    algorithmComboBox->addItem("Password (SHA512 HMAC)", QString("password/sha512"));
    if (old_enabled) {
        algorithmComboBox->addItem("OldPassword", QString("password/old"));
    }
    algorithmComboBox->addItem("Passphrase Diceware (Argon2)", QString("passphrase/diceware"));
    algorithmComboBox->addItem("Passphrase Wordnet Pattern (Argon2)", QString("passphrase/wordnet"));

    int idx = algorithmComboBox->findData(currentVal);
    if (idx != -1) {
        algorithmComboBox->setCurrentIndex(idx);
    } else {
        algorithmComboBox->setCurrentIndex(0);
    }
    algorithmComboBox->blockSignals(false);
}

void SettingsDialog::updateRowAppearance(int row) {
    QTableWidgetItem *checkItem = tableWidget->item(row, 0);
    if (!checkItem) return;
    bool checked = (checkItem->checkState() == Qt::Checked);

    QTableWidgetItem *nameItem = tableWidget->item(row, 1);
    if (nameItem) {
        nameItem->setForeground(checked ? palette().color(QPalette::Text) : QBrush(Qt::gray));
    }

    QWidget *editor = tableWidget->cellWidget(row, 2);
    if (editor) {
        editor->setEnabled(checked);
    }
}

void SettingsDialog::loadFromConfig() {
    config_ = mkpass::Config(GetConfigFilePath());
    config_.load();

    updateAlgorithmChoices();

    tableWidget->blockSignals(true);
    for (int i = 0; i < rowDefs_.size(); ++i) {
        const auto& def = rowDefs_[i];
        bool is_set = config_.is_set(def.key.toStdString());
        std::string val;
        if (is_set) {
            val = *config_.get_raw(def.key.toStdString());
        } else {
            val = mkpass::Config::get_built_in_default(def.key.toStdString());
        }
        setEditorValue(i, val);

        QTableWidgetItem *checkItem = tableWidget->item(i, 0);
        if (checkItem) {
            checkItem->setCheckState(is_set ? Qt::Checked : Qt::Unchecked);
        }
        updateRowAppearance(i);
    }
    tableWidget->blockSignals(false);
}

std::string SettingsDialog::getEditorValue(int row) const {
    const QString& key = rowDefs_[row].key;
    if (key == "algorithm") {
        return algorithmComboBox->currentData().toString().toStdString();
    } else if (key == "char_classes") {
        return charClassesLineEdit->text().toStdString();
    } else if (key == "custom_chars") {
        return customCharsLineEdit->text().toStdString();
    } else if (key == "length") {
        return std::to_string(lengthSpinBox->value());
    } else if (key == "separator") {
        return separatorComboBox->currentData().toString().toStdString();
    } else if (key == "passphrase_pattern") {
        return passphrasePatternLineEdit->text().toStdString();
    } else if (key == "digits") {
        return digitsComboBox->currentData().toString().toStdString();
    } else if (key == "symbols") {
        return symbolsComboBox->currentData().toString().toStdString();
    } else if (key == "substitutions") {
        return substitutionsComboBox->currentData().toString().toStdString();
    } else if (key == "capitalize") {
        return capitalizeComboBox->currentData().toString().toStdString();
    } else if (key == "enable_old_algorithm") {
        return enableOldAlgoComboBox->currentData().toString().toStdString();
    }
    return "";
}

void SettingsDialog::setEditorValue(int row, const std::string& value) {
    const QString& key = rowDefs_[row].key;
    if (key == "algorithm") {
        int idx = algorithmComboBox->findData(QString::fromStdString(value));
        if (idx != -1) {
            algorithmComboBox->setCurrentIndex(idx);
        } else {
            if (value == "password/old" || value == "old" || value == "3") {
                algorithmComboBox->addItem("OldPassword", QString("password/old"));
                algorithmComboBox->setCurrentIndex(algorithmComboBox->count() - 1);
            }
        }
    } else if (key == "char_classes") {
        charClassesLineEdit->setText(QString::fromStdString(value));
    } else if (key == "custom_chars") {
        customCharsLineEdit->setText(QString::fromStdString(value));
    } else if (key == "length") {
        try {
            lengthSpinBox->setValue(std::stoi(value));
        } catch (...) {
            lengthSpinBox->setValue(16);
        }
    } else if (key == "separator") {
        int idx = separatorComboBox->findData(QString::fromStdString(value));
        if (idx != -1) {
            separatorComboBox->setCurrentIndex(idx);
        } else {
            separatorComboBox->addItem(QString("Custom ('%1')").arg(QString::fromStdString(value)), QString::fromStdString(value));
            separatorComboBox->setCurrentIndex(separatorComboBox->count() - 1);
        }
    } else if (key == "passphrase_pattern") {
        passphrasePatternLineEdit->setText(QString::fromStdString(value));
    } else if (key == "digits") {
        int idx = digitsComboBox->findData(QString::fromStdString(value));
        if (idx != -1) digitsComboBox->setCurrentIndex(idx);
    } else if (key == "symbols") {
        int idx = symbolsComboBox->findData(QString::fromStdString(value));
        if (idx != -1) symbolsComboBox->setCurrentIndex(idx);
    } else if (key == "substitutions") {
        int idx = substitutionsComboBox->findData(QString::fromStdString(value));
        if (idx != -1) substitutionsComboBox->setCurrentIndex(idx);
    } else if (key == "capitalize") {
        int idx = capitalizeComboBox->findData(QString::fromStdString(value));
        if (idx != -1) capitalizeComboBox->setCurrentIndex(idx);
    } else if (key == "enable_old_algorithm") {
        int idx = enableOldAlgoComboBox->findData(QString::fromStdString(value));
        if (idx != -1) enableOldAlgoComboBox->setCurrentIndex(idx);
    }
}

void SettingsDialog::onRestoreDefaults() {
    tableWidget->blockSignals(true);
    for (int i = 0; i < rowDefs_.size(); ++i) {
        std::string dflt = mkpass::Config::get_built_in_default(rowDefs_[i].key.toStdString());
        setEditorValue(i, dflt);

        QTableWidgetItem *checkItem = tableWidget->item(i, 0);
        if (checkItem) {
            checkItem->setCheckState(Qt::Unchecked);
        }
        updateRowAppearance(i);
    }
    tableWidget->blockSignals(false);
}

void SettingsDialog::onSave() {
    for (int i = 0; i < rowDefs_.size(); ++i) {
        const auto& def = rowDefs_[i];
        bool enabled = (tableWidget->item(i, 0)->checkState() == Qt::Checked);
        if (enabled) {
            std::string val = getEditorValue(i);
            try {
                config_.set_raw(def.key.toStdString(), val);
            } catch (const std::exception& e) {
                QMessageBox::warning(this, "Validation Error",
                    QString("Invalid value for '%1': %2").arg(def.key).arg(e.what()));
                return;
            }
        } else {
            config_.unset_raw(def.key.toStdString());
        }
    }

    if (!config_.save()) {
        QMessageBox::critical(this, "Save Error", "Failed to save configuration file.");
        return;
    }

    accept();
}
