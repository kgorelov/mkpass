#include "progress_dialog.h"

#include <QVBoxLayout>
#include <QLabel>

ProgressDialog::ProgressDialog(QWidget *parent)
    : QDialog(parent) {
    setupUI();
}

void ProgressDialog::setupUI() {
    setWindowTitle("Generating Password");
    setModal(true);
    setMinimumWidth(300);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    statusLabel = new QLabel("Generating... Please wait.");
    statusLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(statusLabel);

    // Don't add any buttons, it will be closed programmatically
}
