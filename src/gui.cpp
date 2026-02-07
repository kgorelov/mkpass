#include "gui.h"
#include "mkpass.h"
#include "context.h"
#include "character_classes.h"
#include "db.h"
#include "password_dialog.h"

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
#include <QtConcurrent/QtConcurrent>
#include <QFuture>
#include <QFutureWatcher>
#include <QCompleter>
#include <QStringListModel>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent) {
    setupUI();
    generationWatcher = new QFutureWatcher<std::string>(this);
    connect(generationWatcher, &QFutureWatcher<std::string>::finished, this, &MainWindow::generationFinished);

    mkpass::ConfigDB db;
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
    setWindowTitle("mkpass GUI");
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
    algorithmComboBox->addItem("Argon2", static_cast<int>(Algorithm::Argon2));
    algorithmComboBox->addItem("SlowSha512", static_cast<int>(Algorithm::SlowSha512));
    algorithmComboBox->addItem("Old", static_cast<int>(Algorithm::Old));
    formLayout->addRow("Algorithm:", algorithmComboBox);

    QGroupBox *characterClassesGroupBox = new QGroupBox("Character Classes");
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
    formLayout->addRow("Password Length:", lengthSpinBox);

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
    connect(algorithmComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::updateCharacterClassesState);
    connect(customCheckBox, &QCheckBox::toggled, this, &MainWindow::updateCustomCharsState);

    updateCustomCharsState();
}

void MainWindow::generatePassword() {
    generateButton->setEnabled(false);
    statusBar()->showMessage("Generating... Please wait.");

    Context ctx;
    ctx.password = masterPasswordLineEdit->text().toStdString();
    ctx.service = serviceLineEdit->text().toStdString();
    ctx.length = lengthSpinBox->value();
    ctx.algorithm = static_cast<Algorithm>(algorithmComboBox->currentData().toInt());

    if (lowerCaseCheckBox->isChecked()) ctx.char_classes.push_back(CharacterClass::LOWERCASE);
    if (upperCaseCheckBox->isChecked()) ctx.char_classes.push_back(CharacterClass::UPPERCASE);
    if (digitsCheckBox->isChecked()) ctx.char_classes.push_back(CharacterClass::DIGITS);
    if (symbolsCheckBox->isChecked()) ctx.char_classes.push_back(CharacterClass::SYMBOLS);
    if (customCheckBox->isChecked()) {
        ctx.char_classes.push_back(CharacterClass::CUSTOM);
        ctx.custom_chars = customCharsLineEdit->text().toStdString();
    }

    QFuture<std::string> future = QtConcurrent::run(MkPass, ctx);
    generationWatcher->setFuture(future);
}

void MainWindow::generationFinished() {
    generatedPassword = generationWatcher->result();
    PasswordDialog dialog(QString::fromStdString(generatedPassword), this);
    dialog.exec();
    statusBar()->showMessage("Generated.");
    generateButton->setEnabled(true);

    mkpass::ConfigDB db;
    std::vector<CharacterClass> char_classes;
    if (lowerCaseCheckBox->isChecked()) char_classes.push_back(CharacterClass::LOWERCASE);
    if (upperCaseCheckBox->isChecked()) char_classes.push_back(CharacterClass::UPPERCASE);
    if (digitsCheckBox->isChecked()) char_classes.push_back(CharacterClass::DIGITS);
    if (symbolsCheckBox->isChecked()) char_classes.push_back(CharacterClass::SYMBOLS);

    std::optional<std::string> custom_chars;
    if (customCheckBox->isChecked()) {
        char_classes.push_back(CharacterClass::CUSTOM);
        custom_chars = customCharsLineEdit->text().toStdString();
    }

    db.save_service_entry({
        serviceLineEdit->text().toStdString(),
        static_cast<Algorithm>(algorithmComboBox->currentData().toInt()),
        static_cast<unsigned>(lengthSpinBox->value()),
        char_classes,
        custom_chars
    });
}

void MainWindow::serviceChanged(const QString &service) {
    mkpass::ConfigDB db;
    auto entry = db.get_service_entry(service.toStdString());
    if (entry) {
        int index = algorithmComboBox->findData(static_cast<int>(entry->algorithm));
        if (index != -1) {
            algorithmComboBox->setCurrentIndex(index);
        }
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
        updateCustomCharsState();
    } else {
        // Reset to default values
        int index = algorithmComboBox->findData(static_cast<int>(Algorithm::Argon2));
        if (index != -1) {
            algorithmComboBox->setCurrentIndex(index);
        }
        lengthSpinBox->setValue(16);
        lowerCaseCheckBox->setChecked(true);
        upperCaseCheckBox->setChecked(true);
        digitsCheckBox->setChecked(true);
        symbolsCheckBox->setChecked(true);
        customCheckBox->setChecked(false);
        customCharsLineEdit->setText("");
        updateCustomCharsState();
    }
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

void MainWindow::updateCharacterClassesState() {
    Algorithm algorithm = static_cast<Algorithm>(algorithmComboBox->currentData().toInt());
    bool enabled = algorithm != Algorithm::Old;

    lowerCaseCheckBox->setEnabled(enabled);
    upperCaseCheckBox->setEnabled(enabled);
    digitsCheckBox->setEnabled(enabled);
    symbolsCheckBox->setEnabled(enabled);
    customCheckBox->setEnabled(enabled);
    if (enabled) {
        updateCustomCharsState();
    } else {
        customCharsLineEdit->setVisible(false);
    }
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
    MainWindow win;
    win.show();
    return app.exec();
}
