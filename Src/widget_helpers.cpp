#include "widget_helpers.h"

#include <QPushButton>

namespace WidgetHelpers {

QPushButton *makeButton(const QString &text)
{
    auto *button = new QPushButton(text);
    button->setMinimumHeight(42);
    button->setCursor(Qt::PointingHandCursor);
    button->setFocusPolicy(Qt::NoFocus);
    return button;
}

QString formatTime(int totalSeconds)
{
    const int hours = totalSeconds / 3600;
    const int minutes = totalSeconds % 3600 / 60;
    const int seconds = totalSeconds % 60;
    return hours > 0
        ? QString("%1:%2:%3").arg(hours).arg(minutes, 2, 10, QLatin1Char('0'))
              .arg(seconds, 2, 10, QLatin1Char('0'))
        : QString("%1:%2").arg(minutes, 2, 10, QLatin1Char('0'))
              .arg(seconds, 2, 10, QLatin1Char('0'));
}

}
