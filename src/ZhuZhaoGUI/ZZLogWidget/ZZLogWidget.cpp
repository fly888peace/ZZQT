#include "ZZLogWidget.h"
#include "ZZLogMessage.h"

#include <QDesktopServices>
#include <QHBoxLayout>
#include <QIcon>
#include <QPushButton>
#include <QScrollBar>
#include <QTextBrowser>
#include <QTextDocument>
#include <QUrl>
#include <QVBoxLayout>

// 界面上最多保留多少条日志，超了 Qt 自动丢掉最老的那些 block。
// 修：源工程这块只增不减，连续刷日志能把内存一路吃上去，只能靠手动点「清空」。
constexpr int kMaxDisplayLines = 2000;

ZZLogWidget::ZZLogWidget(QWidget *parent)
    : QWidget(parent)
    , m_pClearBtn(nullptr)
    , m_pHelperBtn(nullptr)
    , m_pLogTextBrowser(nullptr)
{
    this->setMinimumSize(300, 200);

    // 源工程（ZZLogWidget.cpp:14-17）写的是 if (InitWidget() == false) throw ...，
    // 而 InitWidget() 最后一行是无条件 return true —— 那个 false 分支永远进不去，
    // throw 是死代码。这里直接调用，不再摆一个假分支。
    InitWidget();
}

bool ZZLogWidget::InitWidget()
{
    // ---------- ① 创建控件 ----------
    m_pClearBtn = new QPushButton(this);
    m_pClearBtn->setText(tr("ClearLog"));
    m_pClearBtn->setFixedSize(120, 28);
    m_pClearBtn->setIconSize(QSize(24, 24));
    m_pClearBtn->setIcon(QIcon(":/Resouce/icon/clear.png"));

    m_pHelperBtn = new QPushButton(this);
    m_pHelperBtn->setText(tr("Helper"));
    m_pHelperBtn->setFixedHeight(28);
    m_pHelperBtn->setIconSize(QSize(24, 24));
    m_pHelperBtn->setIcon(QIcon(":/Resouce/icon/helper.png"));
    // 横向能抻、纵向不许动：这个按钮要吃掉布局里剩下的横向空间
    m_pHelperBtn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_pHelperBtn->setStyleSheet("QPushButton{text-align:left;padding-left:12px;}");

    m_pLogTextBrowser = new QTextBrowser(this);
    // maximumBlockCount 是 QTextDocument 的属性，QTextBrowser 自己没有这个接口，
    // 一路要走到它内部的那个 document 上
    m_pLogTextBrowser->document()->setMaximumBlockCount(kMaxDisplayLines);

    // ---------- ② 建立信号槽 ----------
    connect(m_pClearBtn, &QPushButton::clicked, this, &ZZLogWidget::OnClearBtnClicked);
    connect(m_pHelperBtn, &QPushButton::clicked, this, &ZZLogWidget::OnHelperBtnClicked);

    // 订阅日志。
    // 源工程是一步直连到 QTextBrowser::append 上的（ZZLogWidget.cpp:44），
    // 那样也跑得通，但接过来以后就没机会做「滚动到底」这类收尾动作了。
    connect(ZZLogMessage::Instance(), &ZZLogMessage::sigDebugHtmlData,
            this, &ZZLogWidget::OnLogMessageAppended);

    // ---------- ③ 布局 ----------
    // 上半：日志文本框（占满剩余空间）；下半：两个按钮装在固定高 32 的小 QWidget 里。
    // 布局都不传 parent，最后统一 setLayout 到宿主控件上。
    QHBoxLayout* pBtnLayout = new QHBoxLayout();
    pBtnLayout->setContentsMargins(0, 0, 0, 0);
    pBtnLayout->setSpacing(0);
    pBtnLayout->addWidget(m_pHelperBtn);
    pBtnLayout->addWidget(m_pClearBtn);
    QWidget* pBtnWidget = new QWidget(this);
    pBtnWidget->setLayout(pBtnLayout);
    pBtnWidget->setFixedHeight(32);

    // 修：源工程写的是 new QVBoxLayout(this)，等于先把布局装到本控件上了，
    //     下面这行 setLayout 又来一次，Qt 会报重复布局的警告。
    QVBoxLayout* pMainLayout = new QVBoxLayout();
    pMainLayout->setContentsMargins(0, 0, 0, 0);
    pMainLayout->setSpacing(0);
    pMainLayout->addWidget(m_pLogTextBrowser);
    pMainLayout->addWidget(pBtnWidget);
    this->setLayout(pMainLayout);

    return true;
}

void ZZLogWidget::OnClearBtnClicked()
{
    m_pLogTextBrowser->clear();
    QDEBUG("清空日志显示");
}

void ZZLogWidget::OnHelperBtnClicked()
{
    QDesktopServices::openUrl(QUrl("https://www.roundvision.cc/machinevision/zhuzhao/", QUrl::TolerantMode));
}

void ZZLogWidget::OnLogMessageAppended(const QString& html)
{
    // 槽函数永远在【接收者所属的那条线程】执行，本控件的归属线程是主线程：
    //   主线程打日志   → AutoConnection 判为同线程   → 直连，当场同步执行
    //   算法线程打日志 → 跨线程 → 自动转 Queued，排队到主线程执行
    // 两种情况下在这里改控件都安全。
    m_pLogTextBrowser->append(html);
    MoveScrollBarToBottom();
}

void ZZLogWidget::MoveScrollBarToBottom()
{
    // 新日志追加在最下面，把滚动条拉到底，让用户直接看到最新一条
    QScrollBar* pScrollBar = m_pLogTextBrowser->verticalScrollBar();
    if (pScrollBar != nullptr)
    {
        pScrollBar->setValue(pScrollBar->maximum());
    }
}
