#include "db_management_dialog.h"
#include "db.h"
#include "platform_utils.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QLabel>
#include <QMessageBox>

DbManagementDialog::DbManagementDialog(QWidget *parent)
    : QDialog(parent) {
    setWindowTitle("Database Management");
    setMinimumSize(400, 500);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    mainLayout->addWidget(new QLabel("Filter services:"));
    filterLineEdit = new QLineEdit;
    mainLayout->addWidget(filterLineEdit);

    servicesListWidget = new QListWidget;
    mainLayout->addWidget(servicesListWidget);

    QHBoxLayout *buttonLayout = new QHBoxLayout;
    deleteButton = new QPushButton("Delete");
    deleteButton->setEnabled(false);
    closeButton = new QPushButton("Close");

    buttonLayout->addStretch();
    buttonLayout->addWidget(deleteButton);
    buttonLayout->addWidget(closeButton);

    mainLayout->addLayout(buttonLayout);

    connect(filterLineEdit, &QLineEdit::textChanged, this, &DbManagementDialog::filterChanged);
    connect(servicesListWidget, &QListWidget::itemSelectionChanged, this, &DbManagementDialog::selectionChanged);
    connect(deleteButton, &QPushButton::clicked, this, &DbManagementDialog::deleteClicked);
    connect(closeButton, &QPushButton::clicked, this, &DbManagementDialog::close);

    loadServices();
}

void DbManagementDialog::loadServices() {
    mkpass::ConfigDB db(GetConfigDBPath());
    allServices.clear();
    for (const auto& service : db.get_all_service_names()) {
        allServices << QString::fromStdString(service);
    }
    updateFilter();
}

void DbManagementDialog::updateFilter() {
    servicesListWidget->clear();
    QString filterText = filterLineEdit->text();
    for (const auto& service : allServices) {
        if (service.contains(filterText, Qt::CaseInsensitive)) {
            servicesListWidget->addItem(service);
        }
    }
}

void DbManagementDialog::filterChanged(const QString &text) {
    updateFilter();
}

void DbManagementDialog::selectionChanged() {
    deleteButton->setEnabled(servicesListWidget->currentItem() != nullptr);
}

void DbManagementDialog::deleteClicked() {
    QListWidgetItem *item = servicesListWidget->currentItem();
    if (!item) return;

    QString serviceName = item->text();
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "Confirm Deletion",
                                  QString("Do you want to remove service %1?").arg(serviceName),
                                  QMessageBox::Yes|QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        mkpass::ConfigDB db(GetConfigDBPath());
        db.delete_service_entry(serviceName.toStdString());
        loadServices();
    }
}
