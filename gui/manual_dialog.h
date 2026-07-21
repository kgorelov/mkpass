#pragma once

#include <QDialog>

class QTextBrowser;

class ManualDialog : public QDialog {
    Q_OBJECT

public:
    explicit ManualDialog(QWidget *parent = nullptr);
    ~ManualDialog() override = default;

private:
    QTextBrowser *textBrowser;
};
