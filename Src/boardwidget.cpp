#include "boardwidget.h"

#include <QColor>
#include <QEasingCurve>
#include <QFont>
#include <QPainter>
#include <QPaintEvent>
#include <QVariantAnimation>

#include <cmath>

namespace {
QColor tileColor(int value)
{
    switch (value) {
    case 2: return QColor(236, 228, 219);
    case 4: return QColor(232, 217, 186);
    case 8: return QColor(231, 178, 125);
    case 16: return QColor(232, 152, 107);
    case 32: return QColor(230, 127, 98);
    case 64: return QColor(227, 99, 66);
    case 128: return QColor(236, 208, 105);
    case 256: return QColor(237, 212, 115);
    case 512: return QColor(239, 202, 87);
    case 1024: return QColor(240, 200, 78);
    default: return value >= 2048 ? QColor(248, 211, 71)
                                  : QColor(216, 206, 196);
    }
}

QColor textColor(int value)
{
    return value >= 8 ? QColor(255, 255, 255)
                      : QColor(114, 101, 84);
}
}

BoardWidget::BoardWidget(QWidget *parent)
    : QWidget(parent)
{
    setMinimumSize(460, 460);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    slideAnimation = new QVariantAnimation(this);
    slideAnimation->setDuration(125);
    slideAnimation->setStartValue(0.0);
    slideAnimation->setEndValue(1.0);
    slideAnimation->setEasingCurve(QEasingCurve::OutCubic);
    connect(slideAnimation, &QVariantAnimation::valueChanged,
            this, [this](const QVariant &value) {
        slideProgress = value.toReal();
        update();
    });
    connect(slideAnimation, &QVariantAnimation::finished,
            this, &BoardWidget::startPulseAnimation);

    pulseAnimation = new QVariantAnimation(this);
    pulseAnimation->setDuration(105);
    pulseAnimation->setStartValue(0.0);
    pulseAnimation->setEndValue(1.0);
    pulseAnimation->setEasingCurve(QEasingCurve::Linear);
    connect(pulseAnimation, &QVariantAnimation::valueChanged,
            this, [this](const QVariant &value) {
        pulseProgress = value.toReal();
        update();
    });
    connect(pulseAnimation, &QVariantAnimation::finished, this, [this] {
        animating = false;
        sliding = false;
        movingTiles.clear();
        mergedTiles.clear();
        newTile = {-1, -1};
        update();
        emit animationFinished();
    });
}

void BoardWidget::setBoard(const Board &board)
{
    cancelAnimation();
    displayedBoard = board;
    update();
}

void BoardWidget::animateMove(const Board &previous,
                              const Board &current,
                              BoardCommand command)
{
    cancelAnimation();
    displayedBoard = current;
    buildTrace(previous, current, command);
    animating = true;
    sliding = true;
    slideProgress = 0.0;
    pulseProgress = 0.0;
    slideAnimation->start();
}

void BoardWidget::cancelAnimation()
{
    slideAnimation->stop();
    pulseAnimation->stop();
    animating = false;
    sliding = false;
    movingTiles.clear();
    mergedTiles.clear();
    newTile = {-1, -1};
}

bool BoardWidget::isAnimating() const
{
    return animating;
}

QPoint BoardWidget::linePosition(BoardCommand command,
                                 int outer,
                                 int inner) const
{
    switch (command) {
    case BOARD_CMD_LEFT: return {inner, outer};
    case BOARD_CMD_RIGHT: return {BOARD_COLS - 1 - inner, outer};
    case BOARD_CMD_UP: return {outer, inner};
    case BOARD_CMD_DOWN: return {outer, BOARD_ROWS - 1 - inner};
    }
    return {};
}

void BoardWidget::buildTrace(const Board &previous,
                             const Board &current,
                             BoardCommand command)
{
    int movedGrid[BOARD_ROWS][BOARD_COLS] = {};
    movingTiles.clear();
    mergedTiles.clear();
    newTile = {-1, -1};

    for (int outer = 0; outer < BOARD_ROWS; ++outer) {
        QVector<QPair<QPoint, int>> sourceTiles;
        for (int inner = 0; inner < BOARD_COLS; ++inner) {
            const QPoint position = linePosition(command, outer, inner);
            const int value = previous.grid[position.y()][position.x()];
            if (value != 0)
                sourceTiles.append({position, value});
        }

        int sourceIndex = 0;
        int destinationIndex = 0;
        while (sourceIndex < sourceTiles.size()) {
            const QPoint destination = linePosition(command, outer,
                                                     destinationIndex++);
            const bool merges = sourceIndex + 1 < sourceTiles.size() &&
                sourceTiles[sourceIndex].second == sourceTiles[sourceIndex + 1].second;
            const int value = sourceTiles[sourceIndex].second;
            movingTiles.append({value, sourceTiles[sourceIndex].first, destination});
            if (merges) {
                movingTiles.append({value, sourceTiles[sourceIndex + 1].first, destination});
                movedGrid[destination.y()][destination.x()] = value * 2;
                mergedTiles.append(destination);
                sourceIndex += 2;
            } else {
                movedGrid[destination.y()][destination.x()] = value;
                sourceIndex += 1;
            }
        }
    }

    for (int row = 0; row < BOARD_ROWS; ++row) {
        for (int column = 0; column < BOARD_COLS; ++column) {
            if (movedGrid[row][column] == 0 && current.grid[row][column] != 0) {
                newTile = {column, row};
                return;
            }
        }
    }
}

QRectF BoardWidget::cellRect(const QPoint &position) const
{
    constexpr qreal padding = 10.0;
    constexpr qreal spacing = 8.0;
    const qreal boardSize = qMin(width(), height());
    const qreal left = (width() - boardSize) / 2.0;
    const qreal top = (height() - boardSize) / 2.0;
    const qreal cellSize = (boardSize - padding * 2.0 - spacing * 3.0) / 4.0;
    return {left + padding + position.x() * (cellSize + spacing),
            top + padding + position.y() * (cellSize + spacing),
            cellSize, cellSize};
}

void BoardWidget::drawTile(QPainter &painter,
                           const QRectF &rect,
                           int value) const
{
    painter.setPen(Qt::NoPen);
    painter.setBrush(tileColor(value));
    painter.drawRoundedRect(rect, 8.0, 8.0);
    if (value == 0)
        return;

    QFont font = painter.font();
    font.setWeight(QFont::Black);
    font.setPixelSize(value < 1000 ? 29 : value < 10000 ? 24 : 20);
    painter.setFont(font);
    painter.setPen(textColor(value));
    painter.drawText(rect, Qt::AlignCenter, QString::number(value));
}

void BoardWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    const qreal boardSize = qMin(width(), height());
    const QRectF background((width() - boardSize) / 2.0,
                            (height() - boardSize) / 2.0,
                            boardSize, boardSize);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor("#bbada0"));
    painter.drawRoundedRect(background, 10.0, 10.0);

    for (int row = 0; row < BOARD_ROWS; ++row) {
        for (int column = 0; column < BOARD_COLS; ++column)
            drawTile(painter, cellRect({column, row}), 0);
    }

    if (animating && sliding) {
        for (const MovingTile &tile : movingTiles) {
            const QRectF source = cellRect(tile.source);
            const QRectF destination = cellRect(tile.destination);
            const QRectF current(
                source.x() + (destination.x() - source.x()) * slideProgress,
                source.y() + (destination.y() - source.y()) * slideProgress,
                source.width(), source.height());
            drawTile(painter, current, tile.value);
        }
        return;
    }

    for (int row = 0; row < BOARD_ROWS; ++row) {
        for (int column = 0; column < BOARD_COLS; ++column) {
            const int value = displayedBoard.grid[row][column];
            if (value == 0)
                continue;
            const QPoint position(column, row);
            qreal scale = 1.0;
            if (animating && position == newTile)
                scale = 0.2 + pulseProgress * 0.8;
            if (animating && mergedTiles.contains(position))
                scale = 1.0 + 0.16 * std::sin(pulseProgress * 3.14159265358979323846);
            QRectF rect = cellRect(position);
            const QPointF center = rect.center();
            rect.setSize(rect.size() * scale);
            rect.moveCenter(center);
            drawTile(painter, rect, value);
        }
    }
}

void BoardWidget::startPulseAnimation()
{
    sliding = false;
    pulseProgress = 0.0;
    update();
    pulseAnimation->start();
}
