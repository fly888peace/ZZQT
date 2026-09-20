#ifndef LITIMGITEMWIDGET_H
#define LITIMGITEMWIDGET_H

#include <QWidget>
#include <QImage>

// 缩略图列表里「一格」的内容控件，被 QListWidget::setItemWidget() 绑到列表项上。
// 它不用布局也不用 QLabel —— 整格内容全靠重写 paintEvent 自己画出来，
// 这样才好控制选中红框这类装饰。
class LitImgItemWidget : public QWidget
{
    Q_OBJECT

public:
    explicit LitImgItemWidget(QWidget* parent = nullptr);
    ~LitImgItemWidget() override;

    // 修：源工程参数是非 const 引用，改 const 引用后临时对象也能传进来
    void setImage(const QImage& qImage);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QImage m_qImg;
};

#endif // LITIMGITEMWIDGET_H
