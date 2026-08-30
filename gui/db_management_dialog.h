#pragma once

#include <QDialog>
#include <QStringList>

class QLineEdit;
class QTableWidget;
class QPushButton;

class DbManagementDialog : public QDialog {
    Q_OBJECT

public:
    DbManagementDialog(QWidget *parent = nullptr);

private slots:
    void filterChanged(const QString &text);
    void selectionChanged();
    void deleteClicked();
    void detailsClicked();

private:
    void loadServices();
    void updateFilter();

    QLineEdit *filterLineEdit;
    QTableWidget *servicesTableWidget;
    QPushButton *detailsButton;
    QPushButton *deleteButton;
    QPushButton *closeButton;

    QStringList allServices;
};
