#pragma once

#include <QDialog>
#include "db.h"

class QPushButton;

class ServiceDetailsDialog : public QDialog {
    Q_OBJECT

public:
    explicit ServiceDetailsDialog(const mkpass::ServiceEntry &entry, QWidget *parent = nullptr);

private:
    void setupUI(const mkpass::ServiceEntry &entry);

    QPushButton *closeButton;
};
