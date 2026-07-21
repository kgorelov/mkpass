#include "manual_dialog.h"
#include <QVBoxLayout>
#include <QTextBrowser>
#include <QFile>
#include <QCoreApplication>

ManualDialog::ManualDialog(QWidget *parent)
    : QDialog(parent, Qt::Window) {
    setWindowTitle("mkpass User Manual");
    resize(850, 650);
    setModal(false);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(10, 10, 10, 10);

    textBrowser = new QTextBrowser(this);
    textBrowser->setOpenExternalLinks(true);

    QString htmlContent;

    QFile resFile(":/help.html");
    if (resFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        htmlContent = QString::fromUtf8(resFile.readAll());
        resFile.close();
    } else {
        QStringList candidatePaths = {
            QCoreApplication::applicationDirPath() + "/help/help.html",
            QCoreApplication::applicationDirPath() + "/../docs/qt_help.html",
            "gui/help/help.html",
            "docs/qt_help.html"
        };

        for (const QString &path : candidatePaths) {
            QFile file(path);
            if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                htmlContent = QString::fromUtf8(file.readAll());
                file.close();
                break;
            }
        }
    }

    if (!htmlContent.isEmpty()) {
        textBrowser->setHtml(htmlContent);
    } else {
        textBrowser->setHtml("<h2>User Manual Not Found</h2><p>Could not locate help manual resource.</p>");
    }

    layout->addWidget(textBrowser);
}
