#pragma once

#include <QDialog>

class QLineEdit;
class QPushButton;

class QLabel;

class PasswordDialog : public QDialog {
    Q_OBJECT

public:
    explicit PasswordDialog(const QString &password, QWidget *parent = nullptr);

private slots:
    void togglePasswordVisibility();
    void copyPasswordToClipboard();
    void toggleQrCode();

private:
    void setupUI();

    QLineEdit *passwordLineEdit;
    QPushButton *showHideButton;
    QPushButton *qrCodeButton;
    QPushButton *copyButton;
    QPushButton *closeButton;
    QLabel *qrCodeLabel;

    QString generatedPassword;
};
