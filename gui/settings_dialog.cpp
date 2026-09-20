#include "settings_dialog.h"
#include "platform_utils.h"
#include "algorithms.h"
#include "character_classes.h"
#include "passphrase_patterns.h"
#include "word_classes.h"
#include "db.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QLabel>
#include <cctype>

namespace {

QString GetPatternDescription(const PassphrasePattern& pattern) {
    QStringList parts;
    for (auto wc : pattern) {
        switch (wc) {
            case WordClasses::Noun: parts << "Noun"; break;
            case WordClasses::Verb: parts << "Verb"; break;
            case WordClasses::Adj:  parts << "Adj"; break;
            case WordClasses::Adv:  parts << "Adv"; break;
        }
    }
    return parts.join(", ");
}

QTableWidget* createSettingsTable(int rowCount, QWidget *parent) {
    QTableWidget *table = new QTableWidget(rowCount, 3, parent);
    table->setHorizontalHeaderLabels({"Enabled", "Variable Name", "Default Value"});
    table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
    table->setColumnWidth(0, 65);
    table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Fixed);
    table->setColumnWidth(1, 175);
    table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    table->verticalHeader()->setVisible(false);
    table->setSelectionMode(QAbstractItemView::NoSelection);
    table->setShowGrid(true);
    table->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    table->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    int headerHeight = table->horizontalHeader()->sizeHint().height();
    if (headerHeight < 28) headerHeight = 28;
    int rowHeight = table->fontMetrics().height() + 16;
    if (rowHeight < 34) rowHeight = 34;

    int totalHeight = headerHeight;
    for (int i = 0; i < rowCount; ++i) {
        table->setRowHeight(i, rowHeight);
        totalHeight += rowHeight;
    }
    totalHeight += 4;
    table->setFixedHeight(totalHeight);

    return table;
}

} // namespace

SettingsDialog::SettingsDialog(QWidget *parent)
    : QDialog(parent),
      scrollArea(nullptr),
      generalTable(nullptr),
      passwordTable(nullptr),
      passphraseTable(nullptr),
      restoreDefaultsButton(nullptr),
      saveButton(nullptr),
      cancelButton(nullptr),
      algorithmComboBox(nullptr),
      lengthSpinBox(nullptr),
      enableOldAlgoComboBox(nullptr),
      charClassesWidget(nullptr),
      charLowerCheckBox(nullptr),
      charUpperCheckBox(nullptr),
      charDigitsCheckBox(nullptr),
      charSymbolsCheckBox(nullptr),
      charCustomCheckBox(nullptr),
      customCharsLineEdit(nullptr),
      separatorComboBox(nullptr),
      passphrasePatternComboBox(nullptr),
      digitsComboBox(nullptr),
      symbolsComboBox(nullptr),
      substitutionsComboBox(nullptr),
      capitalizeComboBox(nullptr),
      config_(GetConfigFilePath()) {
    setupUI();
    loadFromConfig();
}

void SettingsDialog::setupUI() {
    setWindowTitle("Preferences");
    setModal(true);
    setMinimumWidth(640);
    resize(680, 580);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);

    QWidget *contentWidget = new QWidget(scrollArea);
    QVBoxLayout *contentLayout = new QVBoxLayout(contentWidget);
    contentLayout->setContentsMargins(4, 4, 4, 4);
    contentLayout->setSpacing(12);

    auto make_bool_combo = [](QWidget *parent) {
        QComboBox *cb = new QComboBox(parent);
        cb->addItem("false", QString("false"));
        cb->addItem("true", QString("true"));
        return cb;
    };

    // 1. General Group
    QGroupBox *generalGroupBox = new QGroupBox("General", contentWidget);
    QVBoxLayout *genLayout = new QVBoxLayout(generalGroupBox);
    generalTable = createSettingsTable(3, generalGroupBox);
    genLayout->addWidget(generalTable);

    algorithmComboBox = new QComboBox(generalTable);
    lengthSpinBox = new QSpinBox(generalTable);
    lengthSpinBox->setRange(1, 128);
    lengthSpinBox->setValue(16);
    enableOldAlgoComboBox = make_bool_combo(generalTable);

    connect(enableOldAlgoComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &SettingsDialog::updateAlgorithmChoices);

    contentLayout->addWidget(generalGroupBox);

    // 2. Password Options Group
    QGroupBox *passwordGroupBox = new QGroupBox("Password Options", contentWidget);
    QVBoxLayout *pwdLayout = new QVBoxLayout(passwordGroupBox);
    passwordTable = createSettingsTable(2, passwordGroupBox);
    pwdLayout->addWidget(passwordTable);

    charClassesWidget = new QWidget(passwordTable);
    QHBoxLayout *ccLayout = new QHBoxLayout(charClassesWidget);
    ccLayout->setContentsMargins(6, 0, 6, 0);
    ccLayout->setSpacing(10);
    charLowerCheckBox = new QCheckBox("Lower-case", charClassesWidget);
    charUpperCheckBox = new QCheckBox("Upper-case", charClassesWidget);
    charDigitsCheckBox = new QCheckBox("Digits", charClassesWidget);
    charSymbolsCheckBox = new QCheckBox("Symbols", charClassesWidget);
    charCustomCheckBox = new QCheckBox("Custom", charClassesWidget);
    ccLayout->addWidget(charLowerCheckBox);
    ccLayout->addWidget(charUpperCheckBox);
    ccLayout->addWidget(charDigitsCheckBox);
    ccLayout->addWidget(charSymbolsCheckBox);
    ccLayout->addWidget(charCustomCheckBox);
    ccLayout->addStretch();

    customCharsLineEdit = new QLineEdit(passwordTable);
    customCharsLineEdit->setPlaceholderText("e.g. !@#$%");

    contentLayout->addWidget(passwordGroupBox);

    // 3. Passphrase Options Group
    QGroupBox *passphraseGroupBox = new QGroupBox("Passphrase Options", contentWidget);
    QVBoxLayout *passLayout = new QVBoxLayout(passphraseGroupBox);
    passphraseTable = createSettingsTable(6, passphraseGroupBox);
    passLayout->addWidget(passphraseTable);

    separatorComboBox = new QComboBox(passphraseTable);
    separatorComboBox->addItem("None", QString(""));
    separatorComboBox->addItem("Hyphen (-)", QString("-"));
    separatorComboBox->addItem("Space ( )", QString(" "));
    separatorComboBox->addItem("Slash (/)", QString("/"));

    passphrasePatternComboBox = new QComboBox(passphraseTable);
    passphrasePatternComboBox->setEditable(true);
    passphrasePatternComboBox->setInsertPolicy(QComboBox::NoInsert);
    passphrasePatternComboBox->addItem("Random", QString(""));
    size_t maxLen = GetMaxPassphrasePatternLength();
    for (size_t l = 1; l <= maxLen; ++l) {
        PatternsList patterns = GetPassphrasePatterns(l);
        for (const auto& p : patterns) {
            std::string pStr = mkpass::PatternToString(p);
            QString desc = GetPatternDescription(p);
            QString label = QString("%1 (%2)").arg(QString::fromStdString(pStr), desc);
            passphrasePatternComboBox->addItem(label, QString::fromStdString(pStr));
        }
    }

    digitsComboBox = make_bool_combo(passphraseTable);
    symbolsComboBox = make_bool_combo(passphraseTable);
    substitutionsComboBox = make_bool_combo(passphraseTable);
    capitalizeComboBox = make_bool_combo(passphraseTable);

    contentLayout->addWidget(passphraseGroupBox);

    scrollArea->setWidget(contentWidget);
    mainLayout->addWidget(scrollArea);

    rowDefs_ = {
        // General
        {"algorithm", algorithmComboBox, generalTable, 0},
        {"length", lengthSpinBox, generalTable, 1},
        {"enable_old_algorithm", enableOldAlgoComboBox, generalTable, 2},

        // Password
        {"char_classes", charClassesWidget, passwordTable, 0},
        {"custom_chars", customCharsLineEdit, passwordTable, 1},

        // Passphrase
        {"separator", separatorComboBox, passphraseTable, 0},
        {"passphrase_pattern", passphrasePatternComboBox, passphraseTable, 1},
        {"digits", digitsComboBox, passphraseTable, 2},
        {"symbols", symbolsComboBox, passphraseTable, 3},
        {"substitutions", substitutionsComboBox, passphraseTable, 4},
        {"capitalize", capitalizeComboBox, passphraseTable, 5}
    };

    for (const auto& def : rowDefs_) {
        QTableWidgetItem *checkItem = new QTableWidgetItem();
        checkItem->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
        checkItem->setCheckState(Qt::Unchecked);
        def.table->setItem(def.rowInTable, 0, checkItem);

        QTableWidgetItem *nameItem = new QTableWidgetItem(def.key);
        nameItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        def.table->setItem(def.rowInTable, 1, nameItem);

        def.table->setCellWidget(def.rowInTable, 2, def.editor);
    }

    auto connectTable = [this](QTableWidget *tbl) {
        connect(tbl, &QTableWidget::itemChanged, this, [this](QTableWidgetItem *item) {
            if (item && item->column() == 0) {
                updateRowAppearance(item->tableWidget(), item->row());
            }
        });
    };
    connectTable(generalTable);
    connectTable(passwordTable);
    connectTable(passphraseTable);

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

void SettingsDialog::updateRowAppearance(QTableWidget *table, int row) {
    if (!table) return;
    QTableWidgetItem *checkItem = table->item(row, 0);
    if (!checkItem) return;
    bool checked = (checkItem->checkState() == Qt::Checked);

    QTableWidgetItem *nameItem = table->item(row, 1);
    if (nameItem) {
        nameItem->setForeground(checked ? palette().color(QPalette::Text) : QBrush(Qt::gray));
    }

    QWidget *editor = table->cellWidget(row, 2);
    if (editor) {
        editor->setEnabled(checked);
    }
}

void SettingsDialog::loadFromConfig() {
    config_ = mkpass::Config(GetConfigFilePath());
    config_.load();

    updateAlgorithmChoices();

    generalTable->blockSignals(true);
    passwordTable->blockSignals(true);
    passphraseTable->blockSignals(true);

    for (const auto& def : rowDefs_) {
        bool is_set = config_.is_set(def.key.toStdString());
        std::string val;
        if (is_set) {
            val = *config_.get_raw(def.key.toStdString());
        } else {
            val = mkpass::Config::get_built_in_default(def.key.toStdString());
        }
        setEditorValue(def.key, val);

        QTableWidgetItem *checkItem = def.table->item(def.rowInTable, 0);
        if (checkItem) {
            checkItem->setCheckState(is_set ? Qt::Checked : Qt::Unchecked);
        }
        updateRowAppearance(def.table, def.rowInTable);
    }

    generalTable->blockSignals(false);
    passwordTable->blockSignals(false);
    passphraseTable->blockSignals(false);
}

std::string SettingsDialog::getEditorValue(const QString& key) const {
    if (key == "algorithm") {
        return algorithmComboBox->currentData().toString().toStdString();
    } else if (key == "char_classes") {
        std::vector<std::string> tokens;
        if (charLowerCheckBox->isChecked()) tokens.push_back("lowercase");
        if (charUpperCheckBox->isChecked()) tokens.push_back("uppercase");
        if (charDigitsCheckBox->isChecked()) tokens.push_back("digits");
        if (charSymbolsCheckBox->isChecked()) tokens.push_back("symbols");
        if (charCustomCheckBox->isChecked()) tokens.push_back("custom");

        std::string result;
        for (size_t i = 0; i < tokens.size(); ++i) {
            if (i > 0) {
                result += ",";
            }
            result += tokens[i];
        }
        return result;
    } else if (key == "custom_chars") {
        return customCharsLineEdit->text().toStdString();
    } else if (key == "length") {
        return std::to_string(lengthSpinBox->value());
    } else if (key == "separator") {
        return separatorComboBox->currentData().toString().toStdString();
    } else if (key == "passphrase_pattern") {
        QString currentText = passphrasePatternComboBox->currentText().trimmed();
        if (currentText.isEmpty() || currentText.compare("Random", Qt::CaseInsensitive) == 0) {
            return "";
        }
        int idx = passphrasePatternComboBox->currentIndex();
        if (idx >= 0 && currentText == passphrasePatternComboBox->itemText(idx)) {
            return passphrasePatternComboBox->currentData().toString().toStdString();
        }
        int findIdx = passphrasePatternComboBox->findText(currentText);
        if (findIdx != -1) {
            return passphrasePatternComboBox->itemData(findIdx).toString().toStdString();
        }
        return currentText.toLower().toStdString();
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

void SettingsDialog::setEditorValue(const QString& key, const std::string& value) {
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
        auto classes = ParseCharacterClasses(value);
        charLowerCheckBox->setChecked(false);
        charUpperCheckBox->setChecked(false);
        charDigitsCheckBox->setChecked(false);
        charSymbolsCheckBox->setChecked(false);
        charCustomCheckBox->setChecked(false);
        for (auto cc : classes) {
            switch (cc) {
                case CharacterClass::LOWERCASE: charLowerCheckBox->setChecked(true); break;
                case CharacterClass::UPPERCASE: charUpperCheckBox->setChecked(true); break;
                case CharacterClass::DIGITS:    charDigitsCheckBox->setChecked(true); break;
                case CharacterClass::SYMBOLS:   charSymbolsCheckBox->setChecked(true); break;
                case CharacterClass::CUSTOM:    charCustomCheckBox->setChecked(true); break;
            }
        }
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
        std::string v = value;
        while (!v.empty() && std::isspace(static_cast<unsigned char>(v.front()))) v.erase(v.begin());
        while (!v.empty() && std::isspace(static_cast<unsigned char>(v.back()))) v.pop_back();

        if (v.empty() || v == "random" || v == "1") {
            passphrasePatternComboBox->setCurrentIndex(0);
            passphrasePatternComboBox->setEditText("Random");
        } else {
            int idx = passphrasePatternComboBox->findData(QString::fromStdString(v));
            if (idx != -1) {
                passphrasePatternComboBox->setCurrentIndex(idx);
            } else {
                passphrasePatternComboBox->setEditText(QString::fromStdString(v));
            }
        }
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
    generalTable->blockSignals(true);
    passwordTable->blockSignals(true);
    passphraseTable->blockSignals(true);

    for (const auto& def : rowDefs_) {
        std::string dflt = mkpass::Config::get_built_in_default(def.key.toStdString());
        setEditorValue(def.key, dflt);

        QTableWidgetItem *checkItem = def.table->item(def.rowInTable, 0);
        if (checkItem) {
            checkItem->setCheckState(Qt::Unchecked);
        }
        updateRowAppearance(def.table, def.rowInTable);
    }

    generalTable->blockSignals(false);
    passwordTable->blockSignals(false);
    passphraseTable->blockSignals(false);
}

void SettingsDialog::onSave() {
    for (const auto& def : rowDefs_) {
        bool enabled = (def.table->item(def.rowInTable, 0)->checkState() == Qt::Checked);
        if (enabled) {
            std::string val = getEditorValue(def.key);
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
