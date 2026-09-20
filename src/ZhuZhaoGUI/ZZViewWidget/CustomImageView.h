#ifndef CUSTOMIMAGEVIEW_H
#define CUSTOMIMAGEVIEW_H

#include <QWidget>
#include <QImage>

class CustomGraphicsView;

// 视觉窗口的外壳：把 CustomGraphicsView 包一层，
// 对外提供槽函数，供两个缩略图列表连接。
// 分层的原因：缩略图列表只关心「我选中了一张图」，不该知道 GraphicsView 的存在。
class CustomImageView : public QWidget
{
    Q_OBJECT

public:
    explicit CustomImageView(QWidget* parent = nullptr);
    ~CustomImageView() override;

    bool InitWidget();

    const QImage& GetImage() { return m_qImage; }
    void SetImage(QImage& qImage);        // 直接给图
    void SetImage(QString& strPath);      // 给图片路径

public slots:
    void OnSendImage(QImage& qImage);     // 缩略图列表 → 本控件的入口

private:
    CustomGraphicsView* m_pGraphicsview;   // 里面真正的视图
    QImage m_qImage;                       // 当前显示的图
};

#endif // CUSTOMIMAGEVIEW_H
