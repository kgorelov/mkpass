#pragma once

#include <QApplication>
#include <QIcon>
#include <QPalette>
#include <QString>

bool isDarkTheme(const QPalette &palette = QApplication::palette());
QIcon getThemedIcon(const QString &resourcePath);
