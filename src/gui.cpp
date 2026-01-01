#include "gui.h"
#include "mkpass.h"
#include "context.h"
#include "character_classes.h"
#include "db.h"

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

    QHBoxLayout *characterClassesLayout = new QHBoxLayout;
    upperCaseCheckBox = new QCheckBox("Upper-case");
    lowerCaseCheckBox = new QCheckBox("Lower-case");
    digitsCheckBox = new QCheckBox("Digits");
    symbolsCheckBox = new QCheckBox("Symbols");
    upperCaseCheckBox->setChecked(true);
    lowerCaseCheckBox->setChecked(true);
    digitsCheckBox->setChecked(true);
    symbolsCheckBox->setChecked(true);
    characterClassesLayout->addWidget(upperCaseCheckBox);
    characterClassesLayout->addWidget(lowerCaseCheckBox);
    characterClassesLayout->addWidget(digitsCheckBox);
    characterClassesLayout->addWidget(symbolsCheckBox);
    formLayout->addRow("Character Classes:", characterClassesLayout);

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
}

void MainWindow::generatePassword() {
    generateButton->setEnabled(false);
    statusBar()->showMessage("Generating... Please wait.");

    Context ctx;
    ctx.password = masterPasswordLineEdit->text().toStdString();
    ctx.service = serviceLineEdit->text().toStdString();
    ctx.length = lengthSpinBox->value();
    ctx.algorithm = static_cast<Algorithm>(algorithmComboBox->currentData().toInt());

    if (upperCaseCheckBox->isChecked()) ctx.char_classes.push_back(CharacterClass::UPPERCASE);
    if (lowerCaseCheckBox->isChecked()) ctx.char_classes.push_back(CharacterClass::LOWERCASE);
    if (digitsCheckBox->isChecked()) ctx.char_classes.push_back(CharacterClass::DIGITS);
    if (symbolsCheckBox->isChecked()) ctx.char_classes.push_back(CharacterClass::SYMBOLS);

    QFuture<std::string> future = QtConcurrent::run(MkPass, ctx);
    generationWatcher->setFuture(future);
}

void MainWindow::generationFinished() {
    generatedPassword = generationWatcher->result();
    QApplication::clipboard()->setText(QString::fromStdString(generatedPassword));
    statusBar()->showMessage("Generated. Copied to clipboard.");
    generateButton->setEnabled(true);

    mkpass::ConfigDB db;
    std::vector<CharacterClass> char_classes;
    if (upperCaseCheckBox->isChecked()) char_classes.push_back(CharacterClass::UPPERCASE);
    if (lowerCaseCheckBox->isChecked()) char_classes.push_back(CharacterClass::LOWERCASE);
    if (digitsCheckBox->isChecked()) char_classes.push_back(CharacterClass::DIGITS);
    if (symbolsCheckBox->isChecked()) char_classes.push_back(CharacterClass::SYMBOLS);

    db.save_service_entry({
        serviceLineEdit->text().toStdString(),
        static_cast<Algorithm>(algorithmComboBox->currentData().toInt()),
        static_cast<unsigned>(lengthSpinBox->value()),
        char_classes
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

        upperCaseCheckBox->setChecked(false);
        lowerCaseCheckBox->setChecked(false);
        digitsCheckBox->setChecked(false);
        symbolsCheckBox->setChecked(false);
        for (const auto& cc : entry->char_classes) {
            if (cc == CharacterClass::UPPERCASE) upperCaseCheckBox->setChecked(true);
            if (cc == CharacterClass::LOWERCASE) lowerCaseCheckBox->setChecked(true);
            if (cc == CharacterClass::DIGITS) digitsCheckBox->setChecked(true);
            if (cc == CharacterClass::SYMBOLS) symbolsCheckBox->setChecked(true);
        }
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
        statusBar()->clearMessage();
    } else {
        masterPasswordLineEdit->setStyleSheet("background-color: red");
        repeatPasswordLineEdit->setStyleSheet("background-color: red");
        generateButton->setEnabled(false);
        statusBar()->showMessage("Error: passwords don't match");
    }
}

int run_gui(int argc, char *argv[]) {
    QApplication app(argc, argv);
    MainWindow win;
    win.show();
    return app.exec();
}
