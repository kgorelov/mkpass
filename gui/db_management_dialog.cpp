#include "db_management_dialog.h"
#include "db.h"
#include "platform_utils.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QHeaderView>
#include <QPushButton>
#include <QLabel>
#include <QMessageBox>
#include <QStyledItemDelegate>
#include <QTextDocument>
#include <QPainter>
#include <QApplication>
#include <cctype>

namespace {

class RichTextDelegate : public QStyledItemDelegate {
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override {
        QStyleOptionViewItem opt = option;
        initStyleOption(&opt, index);

        painter->save();

        // Draw background
        QStyle *style = opt.widget ? opt.widget->style() : QApplication::style();
        style->drawControl(QStyle::CE_ItemViewItem, &opt, painter, opt.widget);

        // Draw HTML text
        QTextDocument doc;
        QString html = index.data(Qt::UserRole + 1).toString();
        if (html.isEmpty()) {
            html = opt.text;
        }

        doc.setDefaultFont(opt.font);

        // Adjust text color based on selection state
        QPalette::ColorGroup cg = (opt.state & QStyle::State_Enabled) ? QPalette::Active : QPalette::Disabled;
        if (opt.state & QStyle::State_Selected) {
            QColor textColor = opt.palette.color(cg, QPalette::HighlightedText);
            doc.setDefaultStyleSheet(QString("body { color: %1; }").arg(textColor.name()));
        } else {
            QColor textColor = opt.palette.color(cg, QPalette::Text);
            doc.setDefaultStyleSheet(QString("body { color: %1; }").arg(textColor.name()));
        }

        doc.setHtml(html);

        // Clip painter to the text rectangle and center text vertically
        QRect textRect = style->subElementRect(QStyle::SE_ItemViewItemText, &opt, opt.widget);
        int docHeight = doc.size().height();
        int yOffset = 0;
        if (docHeight < textRect.height()) {
            yOffset = (textRect.height() - docHeight) / 2;
        }
        painter->translate(textRect.left(), textRect.top() + yOffset);
        QRect clip(0, 0, textRect.width(), textRect.height() - yOffset);
        painter->setClipRect(clip);

        doc.drawContents(painter, clip);

        painter->restore();
    }

    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override {
        QStyleOptionViewItem opt = option;
        initStyleOption(&opt, index);

        QTextDocument doc;
        QString html = index.data(Qt::UserRole + 1).toString();
        if (html.isEmpty()) {
            html = opt.text;
        }
        doc.setDefaultFont(opt.font);
        doc.setHtml(html);
        return QSize(doc.idealWidth(), QStyledItemDelegate::sizeHint(option, index).height());
    }
};

QString getAlgorithmName(Algorithm algo) {
    switch (algo) {
        case Algorithm::Argon2:
            return "Argon2";
        case Algorithm::SlowSha512:
            return "SHA512 HMAC";
        case Algorithm::Old:
            return "OldPassword";
        case Algorithm::Passphrase_Diceware_EFF_Large:
            return "Diceware (Argon2)";
        case Algorithm::Passphrase_Wordnet_Pattern:
            return "Wordnet Pattern (Argon2)";
        default:
            return "Unknown";
    }
}

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

DbManagementDialog::DbManagementDialog(QWidget *parent)
    : QDialog(parent) {
    setWindowTitle("Database Management");
    setMinimumSize(500, 500);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    mainLayout->addWidget(new QLabel("Filter services:"));
    filterLineEdit = new QLineEdit;
    mainLayout->addWidget(filterLineEdit);

    servicesTableWidget = new QTableWidget;
    servicesTableWidget->setColumnCount(3);
    servicesTableWidget->setHorizontalHeaderLabels({"Service Name", "Algorithm", "Length"});
    servicesTableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    servicesTableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    servicesTableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    servicesTableWidget->verticalHeader()->setVisible(false);
    servicesTableWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    servicesTableWidget->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    servicesTableWidget->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);

    servicesTableWidget->setItemDelegateForColumn(0, new RichTextDelegate(servicesTableWidget));

    mainLayout->addWidget(servicesTableWidget);

    QHBoxLayout *buttonLayout = new QHBoxLayout;
    deleteButton = new QPushButton("Delete");
    deleteButton->setEnabled(false);
    closeButton = new QPushButton("Close");

    buttonLayout->addStretch();
    buttonLayout->addWidget(deleteButton);
    buttonLayout->addWidget(closeButton);

    mainLayout->addLayout(buttonLayout);

    connect(filterLineEdit, &QLineEdit::textChanged, this, &DbManagementDialog::filterChanged);
    connect(servicesTableWidget, &QTableWidget::itemSelectionChanged, this, &DbManagementDialog::selectionChanged);
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
    servicesTableWidget->setRowCount(0);
    QString filterText = filterLineEdit->text();
    mkpass::ConfigDB db(GetConfigDBPath());
    for (const auto& service : allServices) {
        if (service.contains(filterText, Qt::CaseInsensitive)) {
            auto entry = db.get_service_entry(service.toStdString());
            if (entry) {
                int row = servicesTableWidget->rowCount();
                servicesTableWidget->insertRow(row);

                QTableWidgetItem *item0 = new QTableWidgetItem(service);
                item0->setData(Qt::UserRole, service);
                item0->setData(Qt::UserRole + 1, highlightServiceName(service.toStdString()));
                servicesTableWidget->setItem(row, 0, item0);

                QTableWidgetItem *item1 = new QTableWidgetItem(getAlgorithmName(entry->algorithm));
                servicesTableWidget->setItem(row, 1, item1);

                QTableWidgetItem *item2 = new QTableWidgetItem(QString::number(entry->length));
                item2->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
                servicesTableWidget->setItem(row, 2, item2);
            }
        }
    }
}

void DbManagementDialog::filterChanged(const QString &text) {
    updateFilter();
}

void DbManagementDialog::selectionChanged() {
    deleteButton->setEnabled(servicesTableWidget->currentRow() >= 0);
}

void DbManagementDialog::deleteClicked() {
    int row = servicesTableWidget->currentRow();
    if (row < 0) return;

    QTableWidgetItem *item = servicesTableWidget->item(row, 0);
    if (!item) return;

    QString serviceName = item->data(Qt::UserRole).toString();
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
