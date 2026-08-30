#include "service_details_dialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QPushButton>
#include <cctype>

namespace {

QString highlightServiceName(const std::string& name) {
    size_t trailing_start = name.length();
    while (trailing_start > 0 && std::isspace(static_cast<unsigned char>(name[trailing_start - 1]))) {
        trailing_start--;
    }

    QString html = "<html><body>";
    for (size_t i = 0; i < name.length(); ++i) {
        unsigned char c = name[i];
        if (i >= trailing_start) {
            if (c == ' ') {
                html += "<span style='background-color: #ffb3b3; color: #b30000; font-family: monospace; font-weight: bold;'>&nbsp;</span>";
            } else if (c == '\t') {
                html += "<span style='background-color: #ffb3b3; color: #b30000; font-family: monospace; font-weight: bold;'>[TAB]</span>";
            } else if (c == '\r') {
                html += "<span style='background-color: #ffb3b3; color: #b30000; font-family: monospace; font-weight: bold;'>[CR]</span>";
            } else if (c == '\n') {
                html += "<span style='background-color: #ffb3b3; color: #b30000; font-family: monospace; font-weight: bold;'>[LF]</span>";
            } else {
                html += QString("<span style='background-color: #ffb3b3; color: #b30000; font-family: monospace; font-weight: bold;'>\\x%1</span>")
                            .arg(c, 2, 16, QChar('0'));
            }
        } else if (c < 32 || c >= 127) {
            if (c == '\t') {
                html += "<span style='background-color: #ffe0b2; color: #e65100; font-family: monospace; font-weight: bold;'>[TAB]</span>";
            } else if (c == '\r') {
                html += "<span style='background-color: #ffe0b2; color: #e65100; font-family: monospace; font-weight: bold;'>[CR]</span>";
            } else if (c == '\n') {
                html += "<span style='background-color: #ffe0b2; color: #e65100; font-family: monospace; font-weight: bold;'>[LF]</span>";
            } else {
                html += QString("<span style='background-color: #ffcdd2; color: #c62828; font-family: monospace; font-weight: bold;'>\\x%1</span>")
                            .arg(c, 2, 16, QChar('0'));
            }
        } else {
            if (c == '&') html += "&amp;";
            else if (c == '<') html += "&lt;";
            else if (c == '>') html += "&gt;";
            else if (c == '"') html += "&quot;";
            else if (c == '\'') html += "&#39;";
            else html += QChar(c);
        }
    }
    html += "</body></html>";
    return html;
}

} // namespace

ServiceDetailsDialog::ServiceDetailsDialog(const mkpass::ServiceEntry &entry, QWidget *parent)
    : QDialog(parent) {
    setWindowTitle(QString("Service Details - %1").arg(QString::fromStdString(entry.service_name)));
    setMinimumWidth(450);
    setupUI(entry);
}

void ServiceDetailsDialog::setupUI(const mkpass::ServiceEntry &entry) {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    QFormLayout *formLayout = new QFormLayout;
    formLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);

    auto details = mkpass::GetServiceEntryDetails(entry);
    for (const auto& [k, v] : details) {
        QLabel *keyLabel = new QLabel(QString("<b>%1:</b>").arg(QString::fromStdString(k)));
        QLabel *valLabel = new QLabel;
        if (k == "Service name") {
            valLabel->setText(highlightServiceName(entry.service_name));
        } else {
            valLabel->setText(QString::fromStdString(v));
        }
        valLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
        formLayout->addRow(keyLabel, valLabel);
    }

    mainLayout->addLayout(formLayout);
    mainLayout->addSpacing(10);

    QHBoxLayout *buttonLayout = new QHBoxLayout;
    buttonLayout->addStretch();
    closeButton = new QPushButton("Close");
    buttonLayout->addWidget(closeButton);
    mainLayout->addLayout(buttonLayout);

    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);
}
