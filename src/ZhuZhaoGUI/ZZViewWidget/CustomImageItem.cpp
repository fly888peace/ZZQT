#include "CustomImageItem.h"

#include <QGraphicsSceneHoverEvent>
#include <QtGlobal>

CustomImageItem::CustomImageItem(QGraphicsItem* parent)
    : QGraphicsPixmapItem(parent)
{
    // 不打开这个开关，hoverMoveEvent 根本不会被调用
    this->setAcceptHoverEvents(true);
}

void CustomImageItem::hoverMoveEvent(QGraphicsSceneHoverEvent* event)
{
    // 事件坐标是 item 的局部坐标，左上角为原点，所以直接就是像素下标。
    // 修：源工程只把负坐标夹到 0，没有夹上界，鼠标停在图像右下角之外时
    //     会去取越界像素（QImage 返回无效颜色，标签上就是一片 0）。
    const int x = qBound(0, static_cast<int>(event->pos().x()), qMax(0, w - 1));
    const int y = qBound(0, static_cast<int>(event->pos().y()), qMax(0, h - 1));

    int r = 0;
    int g = 0;
    int b = 0;
    pixmap().toImage().pixelColor(x, y).getRgb(&r, &g, &b);

    emit RGBValue(QString(" W:%1,H:%2 | X:%3,Y:%4 | R:%5,G:%6,B:%7")
                      .arg(w).arg(h).arg(x).arg(y).arg(r).arg(g).arg(b));
}
