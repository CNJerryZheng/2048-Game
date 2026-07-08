#pragma once

#include <QPoint>
#include <QRectF>
#include <QVector>
#include <QWidget>

extern "C" {
#include "board.h"
}

class QVariantAnimation;

class BoardWidget : public QWidget
{
    Q_OBJECT

public:
    explicit BoardWidget(QWidget *parent = nullptr);

    void setBoard(const Board &board);
    void animateMove(const Board &previous,
                     const Board &current,
                     BoardCommand command);
    void cancelAnimation();
    bool isAnimating() const;

signals:
    void animationFinished();

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    struct MovingTile
    {
        int value = 0;
        QPoint source;
        QPoint destination;
    };

    void buildTrace(const Board &previous,
                    const Board &current,
                    BoardCommand command);
    QPoint linePosition(BoardCommand command, int outer, int inner) const;
    QRectF cellRect(const QPoint &position) const;
    void drawTile(QPainter &painter, const QRectF &rect, int value) const;
    void startPulseAnimation();

    Board displayedBoard = {};
    QVector<MovingTile> movingTiles;
    QVector<QPoint> mergedTiles;
    QPoint newTile = {-1, -1};
    QVariantAnimation *slideAnimation = nullptr;
    QVariantAnimation *pulseAnimation = nullptr;
    qreal slideProgress = 0.0;
    qreal pulseProgress = 0.0;
    bool animating = false;
    bool sliding = false;
};
