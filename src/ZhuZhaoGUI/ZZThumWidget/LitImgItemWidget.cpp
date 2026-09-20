#include "LitImgItemWidget.h"

#include <QPainter>
#include <QPixmap>

namespace
{
// 图像四周留出的边距，给选中红框让位
constexpr int kMargin = 2;
constexpr int kPenWidth = 4;
}

LitImgItemWidget::LitImgItemWidget(QWidget* parent)
    : QWidget(parent)
{
    // 不设这个策略，焦点会被列表项拦走，hasFocus() 永远为假，红框就出不来
    this->setFocusPolicy(Qt::StrongFocus);
}

LitImgItemWidget::~LitImgItemWidget() = default;

void LitImgItemWidget::setImage(const QImage& qImage)
{
    m_qImg = qImage;
    update();   // 立刻重绘，否则要等到下一次尺寸变化才看得到
}

void LitImgItemWidget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event)

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    const int deviceWidth = this->width();
    const int deviceHeight = this->height();
    const QRect imageRect(kMargin, kMargin,
                          deviceWidth - 2 * kMargin,
                          deviceHeight - 2 * kMargin);

    // 空图用问号占位图兜底。
    // 修：源工程写的是 "://Resouce/icon/nullImg.png"（多了一个斜杠），
    //     而且它把占位图直接赋给了成员 m_qImg —— 等于把「空」这个状态覆盖掉了，
    //     之后就算真给它一张图，判断也还是走占位分支。
    if (m_qImg.isNull())
    {
        painter.drawPixmap(imageRect, QPixmap(":/Resouce/icon/nullImg.png"));
    }
    else
    {
        painter.drawPixmap(imageRect, QPixmap::fromImage(m_qImg));
    }

    // 外框：有焦点画红框（选中），否则黑框
    QPen pen;
    pen.setColor(this->hasFocus() ? Qt::red : Qt::black);
    pen.setWidth(kPenWidth);
    painter.setPen(pen);
    painter.drawRect(imageRect);
}
