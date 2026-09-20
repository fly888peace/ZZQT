#include "CustomGraphicsView.h"
#include "CustomImageItem.h"

#include <QGraphicsScene>
#include <QHBoxLayout>
#include <QMutexLocker>
#include <QMouseEvent>
#include <QPainter>
#include <QResizeEvent>
#include <QWheelEvent>
#include <climits>
#include <new>

namespace
{
// 缩放倍率的上下限（相对原图）
constexpr double kZoomMax = 50.0;
constexpr double kZoomMin = 0.1;

// 左下角信息条的高度
constexpr int kInfoBarHeight = 25;
}

CustomGraphicsView::CustomGraphicsView(QWidget* parent)
    : QGraphicsView(parent)
    , m_pScene(nullptr)
    , m_pImageItem(nullptr)
    , m_pPosInfoWidget(nullptr)
    , m_pPosInfoLabel(nullptr)
{
    // 平移由自己控制，不用滚动条
    this->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    this->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    this->setRenderHint(QPainter::Antialiasing);
    // 缩放时以视图中心为锚点，视觉上更符合直觉
    this->setTransformationAnchor(QGraphicsView::AnchorViewCenter);
    // 拖动时背景不跟着重绘会留下残影，全视口更新最省事
    this->setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    this->setDragMode(QGraphicsView::ScrollHandDrag);
    // 场景范围给到极大，图像元素可以自由摆放，定位靠 centerOn
    this->setSceneRect(INT_MIN / 2, INT_MIN / 2, INT_MAX, INT_MAX);
    setBackground();
    centerOn(0, 0);

    if (!InitWidget())
    {
        throw std::bad_alloc();
    }
}

CustomGraphicsView::~CustomGraphicsView() = default;

bool CustomGraphicsView::InitWidget()
{
    m_pScene = new QGraphicsScene(this);
    // 注意不传 parent：item 加进 scene 之后由 scene 接管生命周期
    m_pImageItem = new CustomImageItem();
    this->setScene(m_pScene);
    m_pScene->addItem(m_pImageItem);

    // 浮在视图左下角的信息条
    m_pPosInfoWidget = new QWidget(this);
    m_pPosInfoLabel = new QLabel(this);
    m_pPosInfoLabel->setStyleSheet("color:rgb(200,255,200); "
                                   "background-color:rgba(50,50,50,160); "
                                   "font: Microsoft YaHei;"
                                   "font-size: 15px;");
    m_pPosInfoLabel->setText(" W:0,H:0 | X:0,Y:0 | R:0,G:0,B:0");

    m_pPosInfoWidget->setFixedHeight(kInfoBarHeight);
    m_pPosInfoWidget->setGeometry(0, this->height() - kInfoBarHeight, this->width(), kInfoBarHeight);
    m_pPosInfoWidget->setStyleSheet("background-color:rgba(0,0,0,0);");

    QHBoxLayout* pInfoLayout = new QHBoxLayout();
    pInfoLayout->setSpacing(0);
    pInfoLayout->setContentsMargins(0, 0, 0, 0);
    pInfoLayout->addWidget(m_pPosInfoLabel);
    m_pPosInfoWidget->setLayout(pInfoLayout);

    // 图像元素报坐标 → 标签显示
    connect(m_pImageItem, &CustomImageItem::RGBValue, this,
            [this](const QString& infoVal) { m_pPosInfoLabel->setText(infoVal); });

    return true;
}

void CustomGraphicsView::SetImage(const QImage& qImage)
{
    // 图像可能从算法线程送过来，这里把「换图」整套动作串行化
    static QMutex mutex;
    QMutexLocker locker(&mutex);

    m_image = QPixmap::fromImage(qImage);
    m_pImageItem->w = m_image.width();
    m_pImageItem->h = m_image.height();
    m_pImageItem->setPixmap(m_image);

    fitFrame();   // 先缩放到与窗口适配
    onCenter();   // 再居中
    show();
}

void CustomGraphicsView::wheelEvent(QWheelEvent* event)
{
    const QPoint scrollAmount = event->angleDelta();

    // 正值表示滚轮远离使用者（放大），负值表示朝向使用者（缩小）
    if (scrollAmount.y() > 0 && m_dZoomValue >= kZoomMax)
    {
        return;   // 已经到放大上限
    }
    if (scrollAmount.y() < 0 && m_dZoomValue <= kZoomMin)
    {
        return;   // 已经到缩小上限
    }

    onZoom(scrollAmount.y() > 0 ? 1.1f : 0.9f);
}

void CustomGraphicsView::mouseDoubleClickEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton)
    {
        fitFrame();   // 自适应到窗口大小
        onCenter();   // 居中
    }
    QGraphicsView::mouseDoubleClickEvent(event);
}

void CustomGraphicsView::paintEvent(QPaintEvent* event)
{
    // 棋盘格底纹是自己画的，不走 setBackgroundBrush ——
    // 后者会让底纹跟着图像一起缩放，看起来像是图像的一部分
    QPainter painter(this->viewport());
    painter.drawTiledPixmap(QRect(QPoint(0, 0), QPoint(this->width(), this->height())), m_tilePixmap);
    QGraphicsView::paintEvent(event);
}

void CustomGraphicsView::resizeEvent(QResizeEvent* event)
{
    fitFrame();
    onCenter();
    // 信息条贴着底部，窗口尺寸一变就要跟着挪
    m_pPosInfoWidget->setGeometry(0, this->height() - kInfoBarHeight, this->width(), kInfoBarHeight);
    QGraphicsView::resizeEvent(event);
}

void CustomGraphicsView::onCenter()
{
    // 让视图中心对准图像元素的中心，再把元素摆回原点
    this->centerOn(m_pImageItem->pixmap().width() / 2, m_pImageItem->pixmap().height() / 2);
    m_pImageItem->setPos(0, 0);
}

void CustomGraphicsView::onZoom(float fScaleFactor)
{
    // 记下相对原图的总倍率，缩放上下限就是靠它判断的
    m_dZoomValue *= fScaleFactor;
    // scale() 缩放的是整个视图，视图里的元素自然跟着放大缩小
    this->scale(fScaleFactor, fScaleFactor);
}

void CustomGraphicsView::fitFrame()
{
    if (this->width() < 1 || m_image.width() < 1)
    {
        return;
    }

    const double winWidth = this->width();
    const double winHeight = this->height();
    const double scaleWidth = (m_image.width() + 1) / winWidth;
    const double scaleHeight = (m_image.height() + 1) / winHeight;
    // 取需要缩得更多的那个方向，才能保证两个方向都装得下
    const double target = (scaleWidth >= scaleHeight) ? (1.0 / scaleWidth) : (1.0 / scaleHeight);

    onZoom(static_cast<float>(target / m_dZoomValue));   // 只补上还差的那部分比例
    m_dZoomValue = target;                               // 回写，否则倍率会一路漂走
}

void CustomGraphicsView::setBackground(bool enabled, bool invertColor)
{
    if (!enabled)
    {
        return;
    }

    m_tilePixmap.fill(invertColor ? QColor(220, 220, 220) : QColor(35, 35, 35));

    QPainter tilePainter(&m_tilePixmap);
    const QColor color(50, 50, 50, 255);
    const QColor invertedColor(210, 210, 210, 255);
    tilePainter.fillRect(0, 0, 18, 18, invertColor ? invertedColor : color);
    tilePainter.fillRect(18, 18, 18, 18, invertColor ? invertedColor : color);
    tilePainter.end();
}
