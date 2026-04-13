#include "gui.h"
#include "mkpass.h"
#include "context.h"
#include "character_classes.h"
#include "db.h"
#include "platform_utils.h"
#include "password_dialog.h"
#include "progress_dialog.h"

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

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent) {
    setupUI();
    generationWatcher = new QFutureWatcher<std::string>(this);
    connect(generationWatcher, &QFutureWatcher<std::string>::finished, this, &MainWindow::generationFinished);

    mkpass::ConfigDB db(GetConfigDBPath());
    QStringList services;
    for (const auto& service : db.get_all_service_names()) {
        services << QString::fromStdString(service);
    }
    QCompleter *completer = new QCompleter(services, this);
    serviceLineEdit->setCompleter(completer);

    connect(serviceLineEdit, &QLineEdit::textChanged, this, &MainWindow::serviceChanged);
}

MainWindow::~MainWindow() {
}

void MainWindow::setupUI() {
    setWindowTitle("mkpass");
    setMinimumWidth(500);

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

    lowerCaseCheckBox->setChecked(true);
    upperCaseCheckBox->setChecked(true);
    digitsCheckBox->setChecked(true);
    symbolsCheckBox->setChecked(true);

    checkBoxesLayout->addWidget(lowerCaseCheckBox, 0, 0);
    checkBoxesLayout->addWidget(upperCaseCheckBox, 0, 1);
    checkBoxesLayout->addWidget(digitsCheckBox, 1, 0);
    checkBoxesLayout->addWidget(symbolsCheckBox, 1, 1);
    checkBoxesLayout->addWidget(customCheckBox, 2, 0);

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

    connect(masterPasswordLineEdit, &QLineEdit::textChanged, this, &MainWindow::checkPasswords);
    connect(repeatPasswordLineEdit, &QLineEdit::textChanged, this, &MainWindow::checkPasswords);
    connect(algorithmComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::updateAlgorithmSpecificUI);
    connect(customCheckBox, &QCheckBox::toggled, this, &MainWindow::updateCustomCharsState);

    updateAlgorithmSpecificUI();
}

void MainWindow::generatePassword() {
    generateButton->setEnabled(false);

    progressDialog = new ProgressDialog(this);
    progressDialog->show();

    Context ctx;
    ctx.password = masterPasswordLineEdit->text().toStdString();
    ctx.service = serviceLineEdit->text().toStdString();
    ctx.length = lengthSpinBox->value();
    ctx.algorithm = static_cast<Algorithm>(algorithmComboBox->currentData().toInt());

    if (ctx.algorithm == Algorithm::Argon2 || ctx.algorithm == Algorithm::SlowSha512) {
        if (lowerCaseCheckBox->isChecked()) ctx.char_classes.push_back(CharacterClass::LOWERCASE);
        if (upperCaseCheckBox->isChecked()) ctx.char_classes.push_back(CharacterClass::UPPERCASE);
        if (digitsCheckBox->isChecked()) ctx.char_classes.push_back(CharacterClass::DIGITS);
        if (symbolsCheckBox->isChecked()) ctx.char_classes.push_back(CharacterClass::SYMBOLS);
        if (customCheckBox->isChecked()) {
            ctx.char_classes.push_back(CharacterClass::CUSTOM);
            ctx.custom_chars = customCharsLineEdit->text().toStdString();
        }
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

    if (algorithm == Algorithm::Argon2 || algorithm == Algorithm::SlowSha512) {
        if (lowerCaseCheckBox->isChecked()) char_classes.push_back(CharacterClass::LOWERCASE);
        if (upperCaseCheckBox->isChecked()) char_classes.push_back(CharacterClass::UPPERCASE);
        if (digitsCheckBox->isChecked()) char_classes.push_back(CharacterClass::DIGITS);
        if (symbolsCheckBox->isChecked()) char_classes.push_back(CharacterClass::SYMBOLS);
        if (customCheckBox->isChecked()) {
            char_classes.push_back(CharacterClass::CUSTOM);
            custom_chars = customCharsLineEdit->text().toStdString();
        }
    } else if (algorithm == Algorithm::Passphrase_Wordnet_Pattern) {
        length = 0;
    }

    db.save_service_entry({
        serviceLineEdit->text().toStdString(),
        algorithm,
        length,
        char_classes,
        custom_chars
    });
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
    } else {
        // Reset to default values based on algorithm
        if (newAlgo == Algorithm::Argon2 || newAlgo == Algorithm::SlowSha512) {
            lengthSpinBox->setValue(16);
            lowerCaseCheckBox->setChecked(true);
            upperCaseCheckBox->setChecked(true);
            digitsCheckBox->setChecked(true);
            symbolsCheckBox->setChecked(true);
            customCheckBox->setChecked(false);
            customCharsLineEdit->setText("");
        } else if (newAlgo == Algorithm::Passphrase_Diceware_EFF_Large) {
            lengthSpinBox->setValue(6);
        } else if (newAlgo == Algorithm::Old) {
            lengthSpinBox->setValue(8);
        }
    }
    updateAlgorithmSpecificUI();
}

void MainWindow::checkPasswords() {
    QString masterPassword = masterPasswordLineEdit->text();
    QString repeatPassword = repeatPasswordLineEdit->text();

    if (repeatPassword.isEmpty()) {
        masterPasswordLineEdit->setStyleSheet("");
        repeatPasswordLineEdit->setStyleSheet("");
        generateButton->setEnabled(true);
        statusBar()->clearMessage();
        return;
    }

    if (masterPassword == repeatPassword) {
        masterPasswordLineEdit->setStyleSheet("background-color: green");
        repeatPasswordLineEdit->setStyleSheet("background-color: green");
        generateButton->setEnabled(true);
        statusBar()->showMessage("OK: Passwords match.");
    } else if (masterPassword.startsWith(repeatPassword)) {
        masterPasswordLineEdit->setStyleSheet("");
        repeatPasswordLineEdit->setStyleSheet("");
        generateButton->setEnabled(false);
        statusBar()->showMessage("Warning: passwords don't match");
    } else {
        masterPasswordLineEdit->setStyleSheet("background-color: red");
        repeatPasswordLineEdit->setStyleSheet("background-color: red");
        generateButton->setEnabled(false);
        statusBar()->showMessage("Error: passwords don't match");
    }
}

void MainWindow::updateAlgorithmSpecificUI() {
    Algorithm algorithm = static_cast<Algorithm>(algorithmComboBox->currentData().toInt());

    bool showCharClasses = (algorithm == Algorithm::Argon2 || algorithm == Algorithm::SlowSha512);
    bool showLength = (algorithm != Algorithm::Passphrase_Wordnet_Pattern);

    characterClassesGroupBox->setVisible(showCharClasses);
    lengthWidget->setVisible(showLength);

    if (showLength) {
        if (algorithm == Algorithm::Passphrase_Diceware_EFF_Large) {
            lengthLabel->setText("Passphrase words count:");
            lengthSpinBox->setRange(1, 20);
        } else {
            lengthLabel->setText("Password length:");
            lengthSpinBox->setRange(1, 128);
        }
    }

    updateCustomCharsState();
}

void MainWindow::updateCustomCharsState() {
    customCharsLineEdit->setVisible(customCheckBox->isChecked());
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
