#ifndef CUSTOMGRAPHICSVIEW_H
#define CUSTOMGRAPHICSVIEW_H

#include <QGraphicsView>
#include <QLabel>
#include <QPixmap>

class CustomImageItem;

// 视觉窗口的核心：滚轮缩放、双击自适应、棋盘格背景、
// 左下角实时显示鼠标所在像素的坐标与 RGB。
class CustomGraphicsView : public QGraphicsView
{
    Q_OBJECT

public:
    explicit CustomGraphicsView(QWidget* parent = nullptr);
    ~CustomGraphicsView() override;

    bool InitWidget();

    // 对外接口：换一张图
    void SetImage(const QImage& qImage);

protected:
    void wheelEvent(QWheelEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

public slots:
    void onCenter();              // 视图居中
    void onZoom(float factor);    // 按比例缩放

private:
    void fitFrame();              // 自适应：算出「图与窗口恰好贴合」还差多少比例
    void setBackground(bool enabled = true, bool invertColor = false);

private:
    double m_dZoomValue = 1.0;   // 当前相对原图的倍率，用来夹住缩放上下限

    QGraphicsScene* m_pScene;           // 场景
    CustomImageItem* m_pImageItem;      // 图像元素
    QWidget* m_pPosInfoWidget;          // 左下角信息条的外壳
    QLabel* m_pPosInfoLabel;            // 显示坐标与 RGB 的标签

    QPixmap m_image;                                  // 当前显示的图像
    QPixmap m_tilePixmap = QPixmap(36, 36);           // 棋盘格底纹
};

#endif // CUSTOMGRAPHICSVIEW_H
