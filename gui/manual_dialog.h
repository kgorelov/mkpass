#pragma once

#include <QDialog>

class QTextBrowser;

class ManualDialog : public QDialog {
    Q_OBJECT

public:
    explicit ManualDialog(QWidget *parent = nullptr);
    ~ManualDialog() override = default;

private slots:
    void zoomIn();
    void zoomOut();

private:
    QTextBrowser *textBrowser;
};
