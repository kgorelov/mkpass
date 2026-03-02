#include "password_dialog.h"
#include "qrcode/qrcodegen.hpp"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QApplication>
#include <QClipboard>
#include <QIcon>
#include <QLabel>
#include <QPainter>
#include <QImage>

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

    qrCodeButton = new QPushButton;
    qrCodeButton->setIcon(QIcon::fromTheme("view-grid", QIcon(":/icons/qr.svg"))); // You need to add a qr.svg icon
    connect(qrCodeButton, &QPushButton::clicked, this, &PasswordDialog::generateQrCode);
    passwordLayout->addWidget(qrCodeButton);

    mainLayout->addLayout(passwordLayout);

    qrCodeLabel = new QLabel;
    qrCodeLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(qrCodeLabel);

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

void PasswordDialog::generateQrCode() {
    qrcodegen::QrCode qr = qrcodegen::QrCode::encodeText(generatedPassword.toStdString().c_str(), qrcodegen::QrCode::Ecc::MEDIUM);

    int size = qr.getSize();
    QImage image(size, size, QImage::Format_Mono);
    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            image.setPixel(x, y, qr.getModule(x, y) ? 0 : 1);
        }
    }

    qrCodeLabel->setPixmap(QPixmap::fromImage(image.scaled(200, 200, Qt::KeepAspectRatio, Qt::FastTransformation)));
}