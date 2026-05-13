#pragma once

#include <QDialog>
#include <QWidget>
#include <QTimer>

class QLabel;

class SpinnerWidget : public QWidget {
    Q_OBJECT
public:
    explicit SpinnerWidget(QWidget *parent = nullptr);
    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QTimer *timer;
    int angle;
};

class ProgressDialog : public QDialog {
    Q_OBJECT

public:
    explicit ProgressDialog(QWidget *parent = nullptr);

private:
    void setupUI();

    QLabel *statusLabel;
    SpinnerWidget *spinner;
};
