#pragma once

#include <QColor>
#include <QFont>
#include <QString>

namespace Theme {

QColor tileBackground(int value);
QColor tileForeground(int value);
QFont tileFont(int value, const QFont &baseFont);
QColor currentUserHighlight();
QString applicationStyleSheet();

}
