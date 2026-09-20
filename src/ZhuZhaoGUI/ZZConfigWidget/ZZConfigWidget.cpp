#include "ZZConfigWidget.h"
#include "ZZListener.h"
#include "ZZOneParamWidget.h"

#include <QFont>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <new>

namespace
{
// 单个参数控件的高度
constexpr int kOneParamWidgetHeight = 60;
}

ZZConfigWidget::ZZConfigWidget(QWidget* parent)
    : QWidget(parent)
    , m_pTitleLabel(nullptr)
    , m_pResetBtn(nullptr)
    , m_pRunOnceBtn(nullptr)
{
    this->setMinimumSize(400, 300);
    this->setAutoFillBackground(true);

    if (!InitWidget())
    {
        throw std::bad_alloc();
    }
}

bool ZZConfigWidget::InitWidget()
{
    // 4 个参数控件长得一样，只差标题；用 lambda 收一下，避免复制 4 遍
    auto CreateOneParam = [this](const QString& strParamName) -> ZZOneParamWidget*
    {
        auto pParamWidget = new ZZOneParamWidget(strParamName, this);
        pParamWidget->setFixedHeight(kOneParamWidgetHeight);
        m_ParamWidgetList.push_back(pParamWidget);
        return pParamWidget;
    };

    // ---------- 标题 ----------
    QFont titleFont("Microsoft YaHei", 12);
    titleFont.setBold(true);

    m_pTitleLabel = new QLabel(this);
    m_pTitleLabel->setText(tr("Config Widget"));
    m_pTitleLabel->setFont(titleFont);
    m_pTitleLabel->setFixedHeight(36);

    // ---------- 两个按钮 ----------
    m_pResetBtn = new QPushButton(this);
    m_pResetBtn->setText(tr("Reset"));
    m_pResetBtn->setFixedSize(90, 28);
    m_pResetBtn->setIconSize(QSize(24, 24));
    m_pResetBtn->setIcon(QIcon(":/Resouce/icon/reset.png"));
    connect(m_pResetBtn, &QPushButton::clicked, this, &ZZConfigWidget::OnResetBtnClicked);

    m_pRunOnceBtn = new QPushButton(this);
    m_pRunOnceBtn->setText(tr("RunOnce"));
    // 修：源工程这里连着调了两次 setFixedSize（先 60x30 再 90x28），第一次是白写的
    m_pRunOnceBtn->setFixedSize(90, 28);
    m_pRunOnceBtn->setIconSize(QSize(24, 24));
    m_pRunOnceBtn->setIcon(QIcon(":/Resouce/icon/runonce.png"));
    connect(m_pRunOnceBtn, &QPushButton::clicked, this, &ZZConfigWidget::OnRunOnceBtnClicked);

    QHBoxLayout* pBtnLayout = new QHBoxLayout();
    pBtnLayout->setContentsMargins(0, 0, 0, 0);
    pBtnLayout->addStretch();
    pBtnLayout->addWidget(m_pResetBtn);
    pBtnLayout->addSpacing(8);
    pBtnLayout->addWidget(m_pRunOnceBtn);
    QWidget* pBtnWidget = new QWidget(this);
    pBtnWidget->setLayout(pBtnLayout);
    pBtnWidget->setFixedHeight(36);
    pBtnWidget->setStyleSheet("background-color:darkgray");

    // ---------- 标题行 ----------
    QHBoxLayout* pTitleLayout = new QHBoxLayout();
    pTitleLayout->setContentsMargins(0, 0, 0, 0);
    pTitleLayout->addWidget(m_pTitleLabel);
    pTitleLayout->addStretch();
    QWidget* pTitleWidget = new QWidget(this);
    pTitleWidget->setLayout(pTitleLayout);

    // ---------- 4 组参数 ----------
    auto pFirstParamWidget = CreateOneParam(tr("FirstParam"));
    auto pSecondParamWidget = CreateOneParam(tr("SecondParam"));
    auto pThirdParamWidget = CreateOneParam(tr("ThirdParam"));
    auto pFourthParamWidget = CreateOneParam(tr("FourthParam"));

    // 角度初值写死在代码里，与 src/bin/images 里那几组示例图配套。
    // 换数据时要同步改这里（每组的真实角度记录在 images/Tilts_Slants.txt）。
    pFirstParamWidget->SetTiltAngle(6.1f);
    pFirstParamWidget->SetSlantAngle(41.4f);
    pSecondParamWidget->SetTiltAngle(95.0f);
    pSecondParamWidget->SetSlantAngle(42.6f);
    pThirdParamWidget->SetTiltAngle(-176.1f);
    pThirdParamWidget->SetSlantAngle(41.7f);
    pFourthParamWidget->SetTiltAngle(-86.8f);
    pFourthParamWidget->SetSlantAngle(40.9f);

    QVBoxLayout* pTopLayout = new QVBoxLayout();
    pTopLayout->setContentsMargins(10, 10, 10, 0);
    pTopLayout->setSpacing(0);
    pTopLayout->addWidget(pTitleWidget);
    pTopLayout->addWidget(pFirstParamWidget);
    pTopLayout->addWidget(pSecondParamWidget);
    pTopLayout->addWidget(pThirdParamWidget);
    pTopLayout->addWidget(pFourthParamWidget);
    QWidget* pTopWidget = new QWidget(this);
    pTopWidget->setLayout(pTopLayout);

    // ---------- 整体 ----------
    QVBoxLayout* pMainLayout = new QVBoxLayout();
    pMainLayout->setContentsMargins(0, 0, 0, 0);
    pMainLayout->setSpacing(0);
    pMainLayout->addWidget(pTopWidget);
    pMainLayout->addStretch();        // 参数区挤在上面，按钮条沉到底部
    pMainLayout->addWidget(pBtnWidget);
    this->setLayout(pMainLayout);

    return true;
}

void ZZConfigWidget::GetPhotometricStereoParams(QList<QImage>& srcImages,
                                                QList<float>& Slants,
                                                QList<float>& Tilts)
{
    srcImages.clear();
    Slants.clear();
    Tilts.clear();

    for (ZZOneParamWidget* pParamWidget : m_ParamWidgetList)
    {
        srcImages.push_back(pParamWidget->GetQImage());
        Slants.push_back(pParamWidget->GetSlantAngle());
        Tilts.push_back(pParamWidget->GetTiltAngle());
    }
}

void ZZConfigWidget::OnResetBtnClicked(bool clicked)
{
    Q_UNUSED(clicked)

    for (ZZOneParamWidget* pParamWidget : m_ParamWidgetList)
    {
        pParamWidget->Reset();
    }
    ListenerManger::Instance()->notify(MESSAGE::ZHUZHAO_UPDATE_SRCIMAGE);
}

void ZZConfigWidget::OnRunOnceBtnClicked(bool clicked)
{
    Q_UNUSED(clicked)

    // 只喊一嗓子。组参数、起线程都由 MainWindow 在 RespondMessage 里做。
    ListenerManger::Instance()->notify(MESSAGE::ZHUZHAO_RUNONCE);
}
