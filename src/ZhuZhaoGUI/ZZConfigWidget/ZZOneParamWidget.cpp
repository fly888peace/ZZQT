#include "ZZOneParamWidget.h"
#include "ZZListener.h"

#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFont>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <new>

namespace
{
// 这一组参数里每个控件的统一高度
// 修：源工程用的是 #define OneParamWidgetHeiht (28)，拼写错了（Heiht），
//     而且宏没有类型也没有作用域。常量就用 constexpr。
constexpr int kParamHeight = 28;
}

ZZOneParamWidget::ZZOneParamWidget(const QString& paramName, QWidget* parent)
    : QWidget(parent)
    , m_strParamName(paramName)
    , m_pTitleLabel(nullptr)
    , m_pSlantsLabel(nullptr)
    , m_pSlantsSpin(nullptr)
    , m_pTiltsLabel(nullptr)
    , m_pTiltsSpin(nullptr)
    , m_pLoadImageBtn(nullptr)
{
    if (!InitWidget())
    {
        throw std::bad_alloc();
    }
}

bool ZZOneParamWidget::InitWidget()
{
    QFont titleFont("Microsoft YaHei", 8);
    titleFont.setBold(true);

    // ---------- 创建控件 ----------
    m_pTitleLabel = new QLabel(this);
    m_pTitleLabel->setText(m_strParamName);
    m_pTitleLabel->setFont(titleFont);

    m_pSlantsLabel = new QLabel(this);
    m_pSlantsLabel->setText(tr("Slant:"));
    m_pSlantsLabel->setFixedHeight(kParamHeight);
    m_pSlantsSpin = new QDoubleSpinBox(this);
    m_pSlantsSpin->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_pSlantsSpin->setFixedHeight(kParamHeight);
    m_pSlantsSpin->setRange(-360, 360);
    m_pSlantsSpin->setValue(0);

    m_pTiltsLabel = new QLabel(this);
    m_pTiltsLabel->setText(tr("Tilt:"));
    m_pTiltsLabel->setFixedHeight(kParamHeight);
    m_pTiltsSpin = new QDoubleSpinBox(this);
    m_pTiltsSpin->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_pTiltsSpin->setFixedHeight(kParamHeight);
    m_pTiltsSpin->setRange(-360, 360);
    m_pTiltsSpin->setValue(0);

    m_pLoadImageBtn = new QPushButton(this);
    m_pLoadImageBtn->setText(tr("load"));
    m_pLoadImageBtn->setFixedSize(80, kParamHeight);
    m_pLoadImageBtn->setIconSize(QSize(24, 24));
    m_pLoadImageBtn->setIcon(QIcon(":/Resouce/icon/loadimg.png"));

    connect(m_pLoadImageBtn, &QPushButton::clicked,
            this, &ZZOneParamWidget::OnSigLoadImageBtnClicked);

    // ---------- 布局 ----------
    // 上行：标题靠左，右边用 stretch 把剩余空间吃掉
    QHBoxLayout* pTitleLayout = new QHBoxLayout();
    pTitleLayout->setContentsMargins(0, 0, 0, 0);
    pTitleLayout->setSpacing(0);
    pTitleLayout->addWidget(m_pTitleLabel);
    pTitleLayout->addStretch();
    QWidget* pTitleWidget = new QWidget(this);
    pTitleWidget->setLayout(pTitleLayout);

    // 下行：Slant 框 / Tilt 框 / 加载按钮 排成一行
    QHBoxLayout* pParamLayout = new QHBoxLayout();
    pParamLayout->setContentsMargins(0, 0, 0, 0);
    pParamLayout->setSpacing(0);
    pParamLayout->addWidget(m_pSlantsLabel);
    pParamLayout->addWidget(m_pSlantsSpin);
    pParamLayout->addSpacing(8);
    pParamLayout->addWidget(m_pTiltsLabel);
    pParamLayout->addWidget(m_pTiltsSpin);
    pParamLayout->addSpacing(8);
    pParamLayout->addWidget(m_pLoadImageBtn);
    QWidget* pParamWidget = new QWidget(this);
    pParamWidget->setLayout(pParamLayout);

    QVBoxLayout* pMainLayout = new QVBoxLayout();
    pMainLayout->setContentsMargins(0, 0, 0, 0);
    pMainLayout->setSpacing(0);
    pMainLayout->addWidget(pTitleWidget);
    pMainLayout->addWidget(pParamWidget);
    this->setLayout(pMainLayout);

    return true;
}

QImage ZZOneParamWidget::GetQImage() const
{
    return m_qImage;
}

void ZZOneParamWidget::SetSlantAngle(float fSlant)
{
    m_pSlantsSpin->setValue(fSlant);
}

float ZZOneParamWidget::GetSlantAngle() const
{
    return static_cast<float>(m_pSlantsSpin->value());
}

void ZZOneParamWidget::SetTiltAngle(float fTilt)
{
    m_pTiltsSpin->setValue(fTilt);
}

float ZZOneParamWidget::GetTiltAngle() const
{
    return static_cast<float>(m_pTiltsSpin->value());
}

void ZZOneParamWidget::Reset()
{
    m_pSlantsSpin->setValue(0.0);
    m_pTiltsSpin->setValue(0.0);
}

void ZZOneParamWidget::OnSigLoadImageBtnClicked(bool clicked)
{
    Q_UNUSED(clicked)

    const QString strImgPath = QFileDialog::getOpenFileName(
        this, tr("Select Image"), ".", tr("Images(*.jpg *.png *.bmp)"));
    if (strImgPath.isEmpty())
    {
        return;
    }

    m_qImage = QImage(strImgPath);
    if (m_qImage.isNull())
    {
        QMessageBox::information(this, tr("Error"), tr("Load Image Failed!"));
        return;
    }

    // 只喊一嗓子，不直接去刷新别人。刷新谁、怎么刷新，交给 MainWindow 决定。
    ListenerManger::Instance()->notify(MESSAGE::ZHUZHAO_UPDATE_SRCIMAGE);
}
