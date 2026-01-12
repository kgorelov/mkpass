#include "password_dialog.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QApplication>
#include <QClipboard>
#include <QIcon>

PasswordDialog::PasswordDialog(const QString &password, QWidget *parent)
    : QDialog(parent), generatedPassword(password) {
    setupUI();
}

void PasswordDialog::setupUI() {
    setWindowTitle("Generated Password");
    setModal(true);
    setMinimumWidth(400);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QHBoxLayout *passwordLayout = new QHBoxLayout;
    passwordLineEdit = new QLineEdit(generatedPassword);
    passwordLineEdit->setEchoMode(QLineEdit::Password);
    passwordLineEdit->setReadOnly(true);
    passwordLayout->addWidget(passwordLineEdit);

    showHideButton = new QPushButton;
    showHideButton->setIcon(QIcon::fromTheme("view-reveal", QIcon(":/icons/eye.svg")));
    connect(showHideButton, &QPushButton::clicked, this, &PasswordDialog::togglePasswordVisibility);
    passwordLayout->addWidget(showHideButton);

    mainLayout->addLayout(passwordLayout);

    QHBoxLayout *buttonLayout = new QHBoxLayout;
    copyButton = new QPushButton("Copy");
    connect(copyButton, &QPushButton::clicked, this, &PasswordDialog::copyPasswordToClipboard);
    buttonLayout->addWidget(copyButton);

    closeButton = new QPushButton("Close");
    connect(closeButton, &QPushButton::clicked, this, &PasswordDialog::accept);
    buttonLayout->addWidget(closeButton);

    mainLayout->addLayout(buttonLayout);
}

void PasswordDialog::togglePasswordVisibility() {
    if (passwordLineEdit->echoMode() == QLineEdit::Password) {
        passwordLineEdit->setEchoMode(QLineEdit::Normal);
        showHideButton->setIcon(QIcon::fromTheme("view-conceal", QIcon(":/icons/eye-off.svg")));
    } else {
        passwordLineEdit->setEchoMode(QLineEdit::Password);
        showHideButton->setIcon(QIcon::fromTheme("view-reveal", QIcon(":/icons/eye.svg")));
    }
}

void PasswordDialog::copyPasswordToClipboard() {
    QApplication::clipboard()->setText(generatedPassword);
}
