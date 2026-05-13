#include "progress_dialog.h"

#include <QVBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPen>

SpinnerWidget::SpinnerWidget(QWidget *parent)
    : QWidget(parent), angle(0) {
    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, [this]() {
        angle = (angle + 30) % 360;
        update();
    });
    timer->start(100);
}

QSize SpinnerWidget::sizeHint() const {
    return QSize(40, 40);
}

void SpinnerWidget::paintEvent(QPaintEvent *) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.translate(width() / 2, height() / 2);
    painter.rotate(angle);

    QPen pen(QColor(156, 39, 176)); // Matches the button color in web version (#9C27B0)
    pen.setWidth(4);
    pen.setCapStyle(Qt::RoundCap);

    for (int i = 0; i < 8; ++i) {
        painter.rotate(45);
        QColor color = pen.color();
        color.setAlpha(32 + i * 28);
        pen.setColor(color);
        painter.setPen(pen);
        painter.drawLine(0, -10, 0, -15);
    }
}

ProgressDialog::ProgressDialog(QWidget *parent)
    : QDialog(parent) {
    setupUI();
}

void ProgressDialog::setupUI() {
    setWindowTitle("Generating Password");
    setModal(true);
    setMinimumWidth(300);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setAlignment(Qt::AlignCenter);
    mainLayout->setSpacing(20);

    spinner = new SpinnerWidget(this);
    mainLayout->addWidget(spinner);

    statusLabel = new QLabel("Generating... Please wait.");
    statusLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(statusLabel);

    // Don't add any buttons, it will be closed programmatically
}
