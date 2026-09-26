#include "comment_dialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>

CommentDialog::CommentDialog(const QString &serviceName, const QString &comment, QWidget *parent)
    : QDialog(parent) {
    if (serviceName.isEmpty()) {
        setWindowTitle("Service Comment");
    } else {
        setWindowTitle(QString("Service Comment - %1").arg(serviceName));
    }
    setMinimumWidth(400);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QLabel *label = new QLabel("Comment (optional):", this);
    mainLayout->addWidget(label);

    commentTextEdit = new QPlainTextEdit(this);
    commentTextEdit->setPlainText(comment);
    commentTextEdit->setPlaceholderText("Enter notes or description for this service...");
    mainLayout->addWidget(commentTextEdit);

    QHBoxLayout *buttonLayout = new QHBoxLayout;
    buttonLayout->addStretch();

    okButton = new QPushButton("OK", this);
    okButton->setDefault(true);
    cancelButton = new QPushButton("Cancel", this);

    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);

    mainLayout->addLayout(buttonLayout);

    connect(okButton, &QPushButton::clicked, this, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);
}

QString CommentDialog::getComment() const {
    return commentTextEdit->toPlainText().trimmed();
}
