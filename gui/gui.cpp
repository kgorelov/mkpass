#include "gui.h"
#include "mkpass.h"
#include "context.h"
#include "character_classes.h"
#include "db.h"
#include "platform_utils.h"
#include "password_dialog.h"
#include "progress_dialog.h"
#include "db_management_dialog.h"
#include "passphrase_patterns.h"
#include "word_classes.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLineEdit>
#include <QCheckBox>
#include <QSpinBox>
#include <QPushButton>
#include <QComboBox>
#include <QGroupBox>
#include <QStatusBar>
#include <QApplication>
#include <QClipboard>
#include <QIcon>
#include <QtConcurrent/QtConcurrent>
#include <QFuture>
#include <QFutureWatcher>
#include <QCompleter>
#include <QStringListModel>
#include <QLabel>
#include <QMenuBar>
#include <QMenu>
#include <QMessageBox>
#include <QPixmap>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), serviceCompleter(nullptr) {
    setupUI();
    generationWatcher = new QFutureWatcher<std::string>(this);
    connect(generationWatcher, &QFutureWatcher<std::string>::finished, this, &MainWindow::generationFinished);

    refreshCompleter();

    connect(serviceLineEdit, &QLineEdit::textChanged, this, &MainWindow::serviceChanged);
}

MainWindow::~MainWindow() {
}

void MainWindow::setupUI() {
    setWindowTitle("mkpass");
    setMinimumWidth(500);

    QMenuBar *menuBar = this->menuBar();
    QMenu *dbMenu = menuBar->addMenu("Database");
    QAction *manageAction = dbMenu->addAction("Management");
    connect(manageAction, &QAction::triggered, this, &MainWindow::manageDatabase);

    QMenu *helpMenu = menuBar->addMenu("Help");
    QAction *helpAction = helpMenu->addAction("About");
    connect(helpAction, &QAction::triggered, this, &MainWindow::showHelp);

    QWidget *centralWidget = new QWidget;
    setCentralWidget(centralWidget);

    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);

    QFormLayout *formLayout = new QFormLayout;

    masterPasswordLineEdit = new QLineEdit;
    masterPasswordLineEdit->setEchoMode(QLineEdit::Password);
    formLayout->addRow("Master Password:", masterPasswordLineEdit);

    repeatPasswordLineEdit = new QLineEdit;
    repeatPasswordLineEdit->setEchoMode(QLineEdit::Password);
    formLayout->addRow("Repeat Password:", repeatPasswordLineEdit);

    serviceLineEdit = new QLineEdit;
    formLayout->addRow("Service:", serviceLineEdit);

    algorithmComboBox = new QComboBox;
    algorithmComboBox->addItem("Password (Argon2)", static_cast<int>(Algorithm::Argon2));
    algorithmComboBox->addItem("Password (SHA512 HMAC)", static_cast<int>(Algorithm::SlowSha512));
    algorithmComboBox->addItem("OldPassword", static_cast<int>(Algorithm::Old));
    algorithmComboBox->addItem("Passphrase Diceware (Argon2)", static_cast<int>(Algorithm::Passphrase_Diceware_EFF_Large));
    algorithmComboBox->addItem("Passphrase Wordnet Pattern (Argon2)", static_cast<int>(Algorithm::Passphrase_Wordnet_Pattern));
    formLayout->addRow("Algorithm:", algorithmComboBox);

    characterClassesGroupBox = new QGroupBox("Character Classes");
    QVBoxLayout *characterClassesLayout = new QVBoxLayout;

    QGridLayout *checkBoxesLayout = new QGridLayout;
    lowerCaseCheckBox = new QCheckBox("Lower-case");
    upperCaseCheckBox = new QCheckBox("Upper-case");
    digitsCheckBox = new QCheckBox("Digits");
    symbolsCheckBox = new QCheckBox("Symbols");
    customCheckBox = new QCheckBox("Custom:");
    allowSubstitutionsCheckBox = new QCheckBox("Allow substitutions (e.g. a -> 4)");
    capitalizeCheckBox = new QCheckBox("Capitalize words");

    lowerCaseCheckBox->setChecked(true);
    upperCaseCheckBox->setChecked(true);
    digitsCheckBox->setChecked(true);
    symbolsCheckBox->setChecked(true);

    checkBoxesLayout->addWidget(lowerCaseCheckBox, 0, 0);
    checkBoxesLayout->addWidget(upperCaseCheckBox, 0, 1);
    checkBoxesLayout->addWidget(digitsCheckBox, 1, 0);
    checkBoxesLayout->addWidget(symbolsCheckBox, 1, 1);
    checkBoxesLayout->addWidget(customCheckBox, 2, 0);
    checkBoxesLayout->addWidget(allowSubstitutionsCheckBox, 3, 0);
    checkBoxesLayout->addWidget(capitalizeCheckBox, 3, 1);

    characterClassesLayout->addLayout(checkBoxesLayout);

    customCharsLineEdit = new QLineEdit;

    QHBoxLayout *customLayout = new QHBoxLayout;
    customLayout->addWidget(customCharsLineEdit);

    characterClassesLayout->addLayout(customLayout);

    characterClassesGroupBox->setLayout(characterClassesLayout);

    formLayout->addRow(characterClassesGroupBox);

    lengthSpinBox = new QSpinBox;
    lengthSpinBox->setRange(1, 128);
    lengthSpinBox->setValue(16);
    lengthWidget = new QWidget;
    QHBoxLayout *lengthLayout = new QHBoxLayout(lengthWidget);
    lengthLayout->setContentsMargins(0, 0, 0, 0);
    lengthLabel = new QLabel("Password length:");
    lengthLayout->addWidget(lengthLabel);
    lengthLayout->addWidget(lengthSpinBox);
    formLayout->addRow(lengthWidget);

    separatorComboBox = new QComboBox;
    separatorComboBox->addItem("None", QString(""));
    separatorComboBox->addItem("Hyphen (-)", QString("-"));
    separatorComboBox->addItem("Space ( )", QString(" "));
    separatorComboBox->addItem("Slash (/)", QString("/"));
    separatorComboBox->setCurrentIndex(0); // Default to None
    separatorWidget = new QWidget;
    QHBoxLayout *separatorLayout = new QHBoxLayout(separatorWidget);
    separatorLayout->setContentsMargins(0, 0, 0, 0);
    separatorLabel = new QLabel("Word separator:");
    separatorLayout->addWidget(separatorLabel);
    separatorLayout->addWidget(separatorComboBox);
    formLayout->addRow(separatorWidget);

    patternComboBox = new QComboBox;
    patternWidget = new QWidget;
    QHBoxLayout *patternLayout = new QHBoxLayout(patternWidget);
    patternLayout->setContentsMargins(0, 0, 0, 0);
    patternLabel = new QLabel("Passphrase pattern:");
    patternLayout->addWidget(patternLabel);
    patternLayout->addWidget(patternComboBox);
    formLayout->addRow(patternWidget);

    mainLayout->addLayout(formLayout);

    QHBoxLayout *buttonLayout = new QHBoxLayout;
    generateButton = new QPushButton("Generate");
    closeButton = new QPushButton("Close");
    buttonLayout->addStretch();
    buttonLayout->addWidget(generateButton);
    buttonLayout->addWidget(closeButton);

    mainLayout->addLayout(buttonLayout);

    statusBar()->showMessage("Ready");

    connect(closeButton, &QPushButton::clicked, this, &MainWindow::close);
    connect(generateButton, &QPushButton::clicked, this, &MainWindow::generatePassword);

    connect(masterPasswordLineEdit, &QLineEdit::textChanged, this, &MainWindow::validateInputs);
    connect(repeatPasswordLineEdit, &QLineEdit::textChanged, this, &MainWindow::validateInputs);
    connect(serviceLineEdit, &QLineEdit::textChanged, this, &MainWindow::validateInputs);
    connect(algorithmComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::updateAlgorithmSpecificUI);
    connect(customCheckBox, &QCheckBox::toggled, this, &MainWindow::updateCustomCharsState);
    connect(digitsCheckBox, &QCheckBox::toggled, this, &MainWindow::updateSubstitutionsState);
    connect(symbolsCheckBox, &QCheckBox::toggled, this, &MainWindow::updateSubstitutionsState);
    connect(lengthSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::updatePatternsList);

    updateAlgorithmSpecificUI();
    validateInputs();
}

void MainWindow::updatePatternsList() {
    Algorithm algorithm = static_cast<Algorithm>(algorithmComboBox->currentData().toInt());
    if (algorithm != Algorithm::Passphrase_Wordnet_Pattern) {
        return;
    }

    QString currentPattern = patternComboBox->currentData().toString();
    patternComboBox->clear();
    patternComboBox->addItem("Random", "");

    int length = lengthSpinBox->value();
    PatternsList patterns = GetPassphrasePatterns(length);
    for (const auto& p : patterns) {
        std::string pStr = mkpass::PatternToString(p);
        patternComboBox->addItem(QString::fromStdString(pStr), QString::fromStdString(pStr));
    }

    int index = patternComboBox->findData(currentPattern);
    if (index != -1) {
        patternComboBox->setCurrentIndex(index);
    } else {
        patternComboBox->setCurrentIndex(0); // Default to Random
    }
}

void MainWindow::generatePassword() {
    generateButton->setEnabled(false);

    progressDialog = new ProgressDialog(this);
    progressDialog->show();

    QString service = serviceLineEdit->text();
    while (!service.isEmpty() && service.at(service.length() - 1).isSpace()) {
        service.chop(1);
    }
    serviceLineEdit->setText(service);

    Context ctx;
    ctx.password = masterPasswordLineEdit->text().toStdString();
    ctx.service = serviceLineEdit->text().toStdString();
    ctx.length = lengthSpinBox->value();
    ctx.algorithm = static_cast<Algorithm>(algorithmComboBox->currentData().toInt());
    ctx.separator = separatorComboBox->currentData().toString().toStdString();
    ctx.capitalize_words = capitalizeCheckBox->isChecked();
    ctx.allow_substitutions = allowSubstitutionsCheckBox->isChecked();
    if (ctx.algorithm == Algorithm::Passphrase_Wordnet_Pattern) {
        ctx.passphrase_pattern = mkpass::StringToPattern(patternComboBox->currentData().toString().toStdString());
    }

    if (algorithmComboBox->currentData().toInt() == static_cast<int>(Algorithm::Argon2) ||
        algorithmComboBox->currentData().toInt() == static_cast<int>(Algorithm::SlowSha512)) {
        if (lowerCaseCheckBox->isChecked()) ctx.char_classes.push_back(CharacterClass::LOWERCASE);
        if (upperCaseCheckBox->isChecked()) ctx.char_classes.push_back(CharacterClass::UPPERCASE);
        if (digitsCheckBox->isChecked()) ctx.char_classes.push_back(CharacterClass::DIGITS);
        if (symbolsCheckBox->isChecked()) ctx.char_classes.push_back(CharacterClass::SYMBOLS);
        if (customCheckBox->isChecked()) {
            ctx.char_classes.push_back(CharacterClass::CUSTOM);
            ctx.custom_chars = customCharsLineEdit->text().toStdString();
        }
    } else {
        if (digitsCheckBox->isChecked()) ctx.char_classes.push_back(CharacterClass::DIGITS);
        if (symbolsCheckBox->isChecked()) ctx.char_classes.push_back(CharacterClass::SYMBOLS);
    }

    QFuture<std::string> future = QtConcurrent::run(MkPass, ctx);
    generationWatcher->setFuture(future);
}

void MainWindow::generationFinished() {
    if (progressDialog) {
        progressDialog->close();
        progressDialog->deleteLater();
        progressDialog = nullptr;
    }

    generatedPassword = generationWatcher->result();
    PasswordDialog dialog(QString::fromStdString(generatedPassword), this);
    dialog.exec();
    statusBar()->showMessage("Generated.");
    generateButton->setEnabled(true);

    mkpass::ConfigDB db(GetConfigDBPath());
    std::vector<CharacterClass> char_classes;
    std::optional<std::string> custom_chars;
    Algorithm algorithm = static_cast<Algorithm>(algorithmComboBox->currentData().toInt());
    unsigned length = lengthSpinBox->value();
    std::string separator = separatorComboBox->currentData().toString().toStdString();
    bool capitalize_words = capitalizeCheckBox->isChecked();

    if (algorithm == Algorithm::Argon2 || algorithm == Algorithm::SlowSha512) {
        if (lowerCaseCheckBox->isChecked()) char_classes.push_back(CharacterClass::LOWERCASE);
        if (upperCaseCheckBox->isChecked()) char_classes.push_back(CharacterClass::UPPERCASE);
        if (digitsCheckBox->isChecked()) char_classes.push_back(CharacterClass::DIGITS);
        if (symbolsCheckBox->isChecked()) char_classes.push_back(CharacterClass::SYMBOLS);
        if (customCheckBox->isChecked()) {
            char_classes.push_back(CharacterClass::CUSTOM);
            custom_chars = customCharsLineEdit->text().toStdString();
        }
    } else if (algorithm == Algorithm::Passphrase_Wordnet_Pattern || algorithm == Algorithm::Passphrase_Diceware_EFF_Large) {
        if (digitsCheckBox->isChecked()) char_classes.push_back(CharacterClass::DIGITS);
        if (symbolsCheckBox->isChecked()) char_classes.push_back(CharacterClass::SYMBOLS);
    }

    std::vector<WordClasses> pattern;
    if (algorithm == Algorithm::Passphrase_Wordnet_Pattern) {
        pattern = mkpass::StringToPattern(patternComboBox->currentData().toString().toStdString());
    }

    db.save_service_entry({
        serviceLineEdit->text().toStdString(),
        algorithm,
        length,
        char_classes,
        custom_chars,
        separator,
        pattern,
        allowSubstitutionsCheckBox->isChecked(),
        capitalize_words
    });

    refreshCompleter();
}

void MainWindow::serviceChanged(const QString &service) {
    mkpass::ConfigDB db(GetConfigDBPath());
    auto entry = db.get_service_entry(service.toStdString());

    Algorithm currentAlgo = static_cast<Algorithm>(algorithmComboBox->currentData().toInt());
    Algorithm newAlgo = entry ? entry->algorithm : Algorithm::Argon2;

    int index = algorithmComboBox->findData(static_cast<int>(newAlgo));
    if (index != -1) {
        algorithmComboBox->setCurrentIndex(index);
    }

    if (entry) {
        lengthSpinBox->setValue(entry->length);

        lowerCaseCheckBox->setChecked(false);
        upperCaseCheckBox->setChecked(false);
        digitsCheckBox->setChecked(false);
        symbolsCheckBox->setChecked(false);
        customCheckBox->setChecked(false);
        for (const auto& cc : entry->char_classes) {
            if (cc == CharacterClass::LOWERCASE) lowerCaseCheckBox->setChecked(true);
            if (cc == CharacterClass::UPPERCASE) upperCaseCheckBox->setChecked(true);
            if (cc == CharacterClass::DIGITS) digitsCheckBox->setChecked(true);
            if (cc == CharacterClass::SYMBOLS) symbolsCheckBox->setChecked(true);
            if (cc == CharacterClass::CUSTOM) customCheckBox->setChecked(true);
        }
        if (entry->custom_chars) {
            customCharsLineEdit->setText(QString::fromStdString(*entry->custom_chars));
        } else {
            customCharsLineEdit->setText("");
        }

        int sepIndex = separatorComboBox->findData(QString::fromStdString(entry->separator));
        if (sepIndex != -1) {
            separatorComboBox->setCurrentIndex(sepIndex);
        }
        capitalizeCheckBox->setChecked(entry->capitalize_words);

        if (!entry->passphrase_pattern.empty()) {
            // updatePatternsList will be called in updateAlgorithmSpecificUI,
            // but we need the patterns to be there so findData works.
            updatePatternsList();
            int pIndex = patternComboBox->findData(QString::fromStdString(mkpass::PatternToString(entry->passphrase_pattern)));
            if (pIndex != -1) {
                patternComboBox->setCurrentIndex(pIndex);
            }
        } else {
            patternComboBox->setCurrentIndex(0); // Default to Random
        }

        allowSubstitutionsCheckBox->setChecked(entry->allow_substitutions);
    } else {
        // Reset to default values based on algorithm
        allowSubstitutionsCheckBox->setChecked(false);
        capitalizeCheckBox->setChecked(true);
        if (newAlgo == Algorithm::Argon2 || newAlgo == Algorithm::SlowSha512) {
            lengthSpinBox->setValue(16);
            lowerCaseCheckBox->setChecked(true);
            upperCaseCheckBox->setChecked(true);
            digitsCheckBox->setChecked(true);
            symbolsCheckBox->setChecked(true);
            customCheckBox->setChecked(false);
            customCharsLineEdit->setText("");
        } else if (newAlgo == Algorithm::Passphrase_Diceware_EFF_Large || newAlgo == Algorithm::Passphrase_Wordnet_Pattern) {
            digitsCheckBox->setChecked(false);
            symbolsCheckBox->setChecked(false);
            lengthSpinBox->setValue(3);
        } else if (newAlgo == Algorithm::Old) {
            lengthSpinBox->setValue(8);
        }
        separatorComboBox->setCurrentIndex(0); // Default to None
    }
    updateAlgorithmSpecificUI();
}

void MainWindow::validateInputs() {
    QString masterPassword = masterPasswordLineEdit->text();
    QString repeatPassword = repeatPasswordLineEdit->text();
    QString service = serviceLineEdit->text();
    while (!service.isEmpty() && service.at(service.length() - 1).isSpace()) {
        service.chop(1);
    }

    bool passwordsMatch = true;
    if (repeatPassword.isEmpty()) {
        masterPasswordLineEdit->setStyleSheet("");
        repeatPasswordLineEdit->setStyleSheet("");
    } else {
        if (masterPassword == repeatPassword) {
            masterPasswordLineEdit->setStyleSheet("background-color: green");
            repeatPasswordLineEdit->setStyleSheet("background-color: green");
        } else if (masterPassword.startsWith(repeatPassword)) {
            masterPasswordLineEdit->setStyleSheet("");
            repeatPasswordLineEdit->setStyleSheet("");
            passwordsMatch = false;
        } else {
            masterPasswordLineEdit->setStyleSheet("background-color: red");
            repeatPasswordLineEdit->setStyleSheet("background-color: red");
            passwordsMatch = false;
        }
    }

    if (masterPassword.isEmpty()) {
        generateButton->setEnabled(false);
        statusBar()->showMessage("Error: Master password must not be empty");
    } else if (service.isEmpty()) {
        generateButton->setEnabled(false);
        statusBar()->showMessage("Error: Service name must not be empty");
    } else if (!passwordsMatch) {
        generateButton->setEnabled(false);
        if (masterPassword.startsWith(repeatPassword)) {
            statusBar()->showMessage("Warning: passwords don't match");
        } else {
            statusBar()->showMessage("Error: passwords don't match");
        }
    } else {
        generateButton->setEnabled(true);
        if (repeatPassword.isEmpty()) {
            statusBar()->clearMessage();
        } else {
            statusBar()->showMessage("OK: Passwords match.");
        }
    }
}

void MainWindow::updateAlgorithmSpecificUI() {
    Algorithm algorithm = static_cast<Algorithm>(algorithmComboBox->currentData().toInt());

    bool isPassword = (algorithm == Algorithm::Argon2 || algorithm == Algorithm::SlowSha512);
    bool isPassphrase = (algorithm == Algorithm::Passphrase_Diceware_EFF_Large || algorithm == Algorithm::Passphrase_Wordnet_Pattern);

    // Apply defaults if switching to passphrase
    static Algorithm lastAlgo = Algorithm::Argon2;
    if (isPassphrase && !(lastAlgo == Algorithm::Passphrase_Diceware_EFF_Large || lastAlgo == Algorithm::Passphrase_Wordnet_Pattern)) {
        digitsCheckBox->setChecked(false);
        symbolsCheckBox->setChecked(false);
        allowSubstitutionsCheckBox->setChecked(false);
        capitalizeCheckBox->setChecked(true);
        if (algorithm == Algorithm::Passphrase_Diceware_EFF_Large) {
            lengthSpinBox->setValue(3);
        }
        separatorComboBox->setCurrentIndex(0);
        patternComboBox->setCurrentIndex(0); // Default to Random
    } else if (isPassword && !(lastAlgo == Algorithm::Argon2 || lastAlgo == Algorithm::SlowSha512)) {
        digitsCheckBox->setChecked(true);
        symbolsCheckBox->setChecked(true);
        lowerCaseCheckBox->setChecked(true);
        upperCaseCheckBox->setChecked(true);
        lengthSpinBox->setValue(16);
    }
    lastAlgo = algorithm;

    characterClassesGroupBox->setVisible(isPassword || isPassphrase);
    lowerCaseCheckBox->setVisible(isPassword);
    upperCaseCheckBox->setVisible(isPassword);
    customCheckBox->setVisible(isPassword);

    digitsCheckBox->setVisible(isPassword || isPassphrase);
    allowSubstitutionsCheckBox->setVisible(isPassphrase);
    capitalizeCheckBox->setVisible(isPassphrase);

    bool showLength = true;

    bool showSeparator = isPassphrase;
    bool showPattern = (algorithm == Algorithm::Passphrase_Wordnet_Pattern);

    lengthWidget->setVisible(showLength);
    separatorWidget->setVisible(showSeparator);
    patternWidget->setVisible(showPattern);

    if (showLength) {
        if (algorithm == Algorithm::Passphrase_Diceware_EFF_Large) {
            lengthLabel->setText("Passphrase words count:");
            lengthSpinBox->setRange(1, 20);
        } else if (algorithm == Algorithm::Passphrase_Wordnet_Pattern) {
            lengthLabel->setText("Passphrase words count:");
            lengthSpinBox->setRange(1, GetMaxPassphrasePatternLength());
            updatePatternsList();
        } else {
            lengthLabel->setText("Password length:");
            lengthSpinBox->setRange(1, 128);
        }
    }

    updateCustomCharsState();
    updateSubstitutionsState();
}

void MainWindow::updateSubstitutionsState() {
    Algorithm algorithm = static_cast<Algorithm>(algorithmComboBox->currentData().toInt());
    bool isPassphrase = (algorithm == Algorithm::Passphrase_Diceware_EFF_Large || algorithm == Algorithm::Passphrase_Wordnet_Pattern);

    if (isPassphrase) {
        bool enabled = digitsCheckBox->isChecked() || symbolsCheckBox->isChecked();
        allowSubstitutionsCheckBox->setEnabled(enabled);
        if (!enabled) {
            allowSubstitutionsCheckBox->setChecked(false);
        }
    } else {
        allowSubstitutionsCheckBox->setEnabled(true);
    }
}

void MainWindow::updateCustomCharsState() {
    customCharsLineEdit->setVisible(customCheckBox->isChecked());
}

void MainWindow::refreshCompleter() {
    mkpass::ConfigDB db(GetConfigDBPath());
    QStringList services;
    for (const auto& service : db.get_all_service_names()) {
        services << QString::fromStdString(service);
    }
    if (serviceCompleter) {
        serviceCompleter->deleteLater();
    }
    serviceCompleter = new QCompleter(services, this);
    serviceCompleter->setCaseSensitivity(Qt::CaseInsensitive);
    serviceCompleter->setFilterMode(Qt::MatchContains);
    serviceLineEdit->setCompleter(serviceCompleter);
}

void MainWindow::manageDatabase() {
    DbManagementDialog dialog(this);
    dialog.exec();

    refreshCompleter();
}

void MainWindow::showHelp() {
    QMessageBox msgBox(this);
    msgBox.setWindowTitle("About mkpass");
    msgBox.setText("<b>mkpass</b><br><br>A secure password generator.");
    msgBox.setIconPixmap(QPixmap(":/app_icon").scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    msgBox.exec();
}

void MainWindow::closeEvent(QCloseEvent *event) {
    QApplication::clipboard()->clear();
    QApplication::clipboard()->clear(QClipboard::Selection);
    QApplication::clipboard()->setText("");
    QMainWindow::closeEvent(event);
}

int run_gui(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setWindowIcon(QIcon(":/app_icon"));
    MainWindow win;
    win.show();
    return app.exec();
}
