#pragma once

#include <QDialog>

class QLineEdit;
class QPushButton;

class PasswordDialog : public QDialog {
    Q_OBJECT

public:
    explicit PasswordDialog(const QString &password, QWidget *parent = nullptr);

private slots:
    void togglePasswordVisibility();
    void copyPasswordToClipboard();

private:
    void setupUI();

    QLineEdit *passwordLineEdit;
    QPushButton *showHideButton;
    QPushButton *copyButton;
    QPushButton *closeButton;

    QString generatedPassword;
};
