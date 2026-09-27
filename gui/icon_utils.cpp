#include "icon_utils.h"

#include <QGuiApplication>
#include <QIconEngine>
#include <QPainter>
#include <QPixmap>
#include <QStyleHints>
#include <QWidget>

bool isDarkTheme(const QPalette &palette) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    if (const auto *hints = QGuiApplication::styleHints()) {
        if (hints->colorScheme() == Qt::ColorScheme::Dark) {
            return true;
        }
        if (hints->colorScheme() == Qt::ColorScheme::Light) {
            return false;
        }
    }
#endif
    const QColor windowColor = palette.color(QPalette::Window);
    const QColor windowTextColor = palette.color(QPalette::WindowText);
    const QColor buttonColor = palette.color(QPalette::Button);
    const QColor buttonTextColor = palette.color(QPalette::ButtonText);

    if (windowTextColor.lightness() > windowColor.lightness() ||
        buttonTextColor.lightness() > buttonColor.lightness()) {
        return true;
    }
    return (windowColor.lightness() < 128 || buttonColor.lightness() < 128);
}

namespace {

QPixmap tintPixmap(const QPixmap &src, const QColor &color) {
    if (src.isNull()) {
        return src;
    }
    QPixmap result = src;
    QPainter p(&result);
    p.setCompositionMode(QPainter::CompositionMode_SourceIn);
    p.fillRect(QRectF(0, 0, result.width(), result.height()), color);
    p.end();
    return result;
}

class ThemedIconEngine : public QIconEngine {
public:
    ThemedIconEngine(const QString &resourcePath, const QIcon &baseIcon)
        : m_resourcePath(resourcePath), m_baseIcon(baseIcon) {}

    void paint(QPainter *painter, const QRect &rect, QIcon::Mode mode, QIcon::State state) override {
        if (!painter || rect.isEmpty()) {
            return;
        }

        QPalette pal = QApplication::palette();
        if (const auto *widget = dynamic_cast<const QWidget *>(painter->device())) {
            pal = widget->palette();
        }

        qreal dpr = painter->device() ? painter->device()->devicePixelRatioF() : 1.0;
        QSize pixelSize = (QSizeF(rect.size()) * dpr).toSize();
        if (pixelSize.isEmpty()) {
            pixelSize = rect.size();
        }

        QPixmap pm = m_baseIcon.pixmap(pixelSize, mode, state);
        if (isDarkTheme(pal)) {
            QColor iconColor = pal.color(mode == QIcon::Disabled ? QPalette::Disabled : QPalette::Normal, QPalette::ButtonText);
            if (mode != QIcon::Disabled && iconColor.lightness() < 180) {
                iconColor = QColor(224, 224, 224);
            }
            pm = tintPixmap(pm, iconColor);
        }
        pm.setDevicePixelRatio(dpr);
        painter->drawPixmap(rect.topLeft(), pm);
    }

    QPixmap pixmap(const QSize &size, QIcon::Mode mode, QIcon::State state) override {
        QPixmap pm = m_baseIcon.pixmap(size, mode, state);
        const QPalette &pal = QApplication::palette();
        if (isDarkTheme(pal)) {
            QColor iconColor = pal.color(mode == QIcon::Disabled ? QPalette::Disabled : QPalette::Normal, QPalette::ButtonText);
            if (mode != QIcon::Disabled && iconColor.lightness() < 180) {
                iconColor = QColor(224, 224, 224);
            }
            pm = tintPixmap(pm, iconColor);
        }
        return pm;
    }

    QSize actualSize(const QSize &size, QIcon::Mode mode, QIcon::State state) override {
        return m_baseIcon.actualSize(size, mode, state);
    }

    QIconEngine *clone() const override {
        return new ThemedIconEngine(*this);
    }

#if QT_VERSION >= QT_VERSION_CHECK(5, 7, 0)
    void virtual_hook(int id, void *data) override {
        if (id == QIconEngine::IsNullHook) {
            *reinterpret_cast<bool *>(data) = m_baseIcon.isNull();
            return;
        }
        QIconEngine::virtual_hook(id, data);
    }
#endif

private:
    QString m_resourcePath;
    QIcon m_baseIcon;
};

} // namespace

QIcon getThemedIcon(const QString &resourcePath) {
    QIcon base(resourcePath);
    if (base.isNull()) {
        return base;
    }
    return QIcon(new ThemedIconEngine(resourcePath, base));
}
