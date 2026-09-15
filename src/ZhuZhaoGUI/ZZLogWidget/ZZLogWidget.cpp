#include "ZZLogWidget.h"
#include "ZZLogMessage.h"   // QDEBUG 宏 + ZZLogMessage::Instance()

#include <QDesktopServices>
#include <QHBoxLayout>
#include <QIcon>
#include <QPushButton>
#include <QScrollBar>
#include <QSizePolicy>
#include <QTextBrowser>
#include <QUrl>
#include <QVBoxLayout>

// 界面上最多保留多少条日志 —— 超了以后 Qt 会自动把最前面（最老）的那些 block 丢掉。
// 源工程这块只增不减：连续刷日志能把内存一路吃上去，唯一的解法是手动点那个"清空"按钮。
// 这里是 QTextEdit 自带的功能（maximumBlockCount），一行设置搞定，不用自己数行数。
constexpr int kMaxDisplayLines = 2000;

ZZLogWidget::ZZLogWidget(QWidget *parent)
    : QWidget(parent)
    , m_pClearBtn(nullptr)
    , m_pHelperBtn(nullptr)
    , m_pLogTextBrowser(nullptr)
{
    this->setMinimumSize(300, 200);

    //  源工程（ZZLogWidget.cpp:14-17）写的是
    //      if (InitWidget() == false)  throw std::bad_alloc();
    //  而 InitWidget() 最后一行就是无条件的 return true —— 那个 false 分支永远进不去，
    //  throw 是一段死代码。这里直接调用，不再摆一个假分支。
    InitWidget();
}

bool ZZLogWidget::InitWidget()
{
    // ============ ① 创建控件 ============
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
    // 超过 kMaxDisplayLines 行，Qt 会自动把最老的那几个 block 丢掉。
    // 注意 maximumBlockCount 是【QTextDocument】的属性，QTextBrowser 自己没有这个接口，
    // 一路要走到它内部的那个 document 上。
    m_pLogTextBrowser->document()->setMaximumBlockCount(kMaxDisplayLines);

    // ============ ② 建立信号槽 ============
    connect(m_pClearBtn, &QPushButton::clicked, this, &ZZLogWidget::OnClearBtnClicked);
    connect(m_pHelperBtn, &QPushButton::clicked, this, &ZZLogWidget::OnHelperBtnClicked);

    // 订阅日志：把 ZZLogMessage 这条信号接到【本对象的槽】上，再自己决定怎么显示。
    // 源工程是一步连到 QTextBrowser::append 上的（ZZLogWidget.cpp:44），
    // 那样也跑得通，但接过来以后就没机会做「滚动到底」这类收尾动作了。
    connect(ZZLogMessage::Instance(), &ZZLogMessage::sigDebugHtmlData,
            this, &ZZLogWidget::OnLogMessageAppended);

    // ============ ③ 布局 ============
    // 上半：日志文本框（占满剩余空间）
    // 下半：两个按钮排成一行，装在一个固定高度 32 的小 QWidget 里
    QHBoxLayout* pBtnLayout = new QHBoxLayout();          // 注意：不指定 parent，下面 pBtnWidget 装的是它
    pBtnLayout->setContentsMargins(0, 0, 0, 0);
    pBtnLayout->setSpacing(0);
    pBtnLayout->addWidget(m_pHelperBtn);
    pBtnLayout->addWidget(m_pClearBtn);

    QWidget* pBtnWidget = new QWidget(this);
    pBtnWidget->setLayout(pBtnLayout);
    pBtnWidget->setFixedHeight(32);

    QVBoxLayout* pMainLayout = new QVBoxLayout(this);
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
    // 记住 D3.5 的结论：槽函数永远在【接收者所属线程】里执行。
    // this（ZZLogWidget）是 main 线程里创建的，ZZLogMessage 的 Instance() 也是主线程里 new 的，
    // 两边同一个线程 → 直连（Direct Connection）→ 这句话就跑在打日志的那一行上，
    // 中间不经过事件队列。所以这里的 update 界面是安全的。
    m_pLogTextBrowser->append(html);
    MoveScrollBarToBottom();
}

void ZZLogWidget::MoveScrollBarToBottom()
{
    // 新日志追加在最下面，把滚动条拉到底让用户直接看到最新一条
    QScrollBar* pScrollBar = m_pLogTextBrowser->verticalScrollBar();
    if (pScrollBar != nullptr)
    {
        pScrollBar->setValue(pScrollBar->maximum());
    }
}
