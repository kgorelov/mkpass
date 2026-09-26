#pragma once

#include <QDialog>
#include <QString>

class QPlainTextEdit;
class QPushButton;

class CommentDialog : public QDialog {
    Q_OBJECT

public:
    explicit CommentDialog(const QString &serviceName, const QString &comment, QWidget *parent = nullptr);

    QString getComment() const;

private:
    QPlainTextEdit *commentTextEdit;
    QPushButton *okButton;
    QPushButton *cancelButton;
};
