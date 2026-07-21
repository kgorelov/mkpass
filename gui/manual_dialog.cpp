#include "manual_dialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QTextBrowser>
#include <QFile>
#include <QCoreApplication>
#include <QFont>

ManualDialog::ManualDialog(QWidget *parent)
    : QDialog(parent, Qt::Window) {
    setWindowTitle("mkpass User Manual");
    resize(850, 650);
    setModal(false);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(8);

    QHBoxLayout *toolbarLayout = new QHBoxLayout();

    QPushButton *zoomInButton = new QPushButton("+", this);
    zoomInButton->setToolTip("Increase font size");
    zoomInButton->setFixedWidth(36);

    QPushButton *zoomOutButton = new QPushButton("-", this);
    zoomOutButton->setToolTip("Decrease font size");
    zoomOutButton->setFixedWidth(36);

    QPushButton *closeButton = new QPushButton("Close", this);
    closeButton->setToolTip("Close user manual");

    toolbarLayout->addWidget(zoomInButton);
    toolbarLayout->addWidget(zoomOutButton);
    toolbarLayout->addStretch();
    toolbarLayout->addWidget(closeButton);

    layout->addLayout(toolbarLayout);

    textBrowser = new QTextBrowser(this);
    textBrowser->setOpenExternalLinks(true);

    QFont font = textBrowser->font();
    font.setPointSize(12);
    textBrowser->setFont(font);

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

    connect(zoomInButton, &QPushButton::clicked, this, &ManualDialog::zoomIn);
    connect(zoomOutButton, &QPushButton::clicked, this, &ManualDialog::zoomOut);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::close);
}

void ManualDialog::zoomIn() {
    textBrowser->zoomIn(1);
}

void ManualDialog::zoomOut() {
    textBrowser->zoomOut(1);
}
