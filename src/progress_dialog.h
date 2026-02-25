#pragma once

#include <QDialog>

class QLabel;

class ProgressDialog : public QDialog {
    Q_OBJECT

public:
    explicit ProgressDialog(QWidget *parent = nullptr);

private:
    void setupUI();

    QLabel *statusLabel;
};
