#pragma once

#include <QString>

class QPushButton;

namespace WidgetHelpers {

QPushButton *makeButton(const QString &text);
QString formatTime(int totalSeconds);

}
