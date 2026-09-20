#ifndef CUSTOMIMAGEITEM_H
#define CUSTOMIMAGEITEM_H

#include <QObject>
#include <QGraphicsPixmapItem>

// 视觉窗口里显示图像的那个元素（Scene 中的一个 item）。
// 鼠标在它上面移动时，把当前坐标与该点像素的 RGB 用信号报出去。
class CustomImageItem : public QObject, public QGraphicsPixmapItem
{
    Q_OBJECT

public:
    // 修：源工程的参数写的是 QWidget* parent，但基类 QGraphicsPixmapItem 要的是
    //     QGraphicsItem*。因为调用处一直传 nullptr 才没暴露出来。
    explicit CustomImageItem(QGraphicsItem* parent = nullptr);

    int w = 0;   // 当前图像的宽
    int h = 0;   // 当前图像的高

protected:
    void hoverMoveEvent(QGraphicsSceneHoverEvent* event) override;

signals:
    void RGBValue(const QString& infoVal);
};

#endif // CUSTOMIMAGEITEM_H
