#include "MainWindow.h"
#include "ZZLogWidget/ZZLogWidget.h"
#include "ZZLogWidget/ZZLogMessage.h"

// ==========================================================
//  当前状态：临时挂载 ZZLogWidget，只为验证 D3 的日志链路。
//
//  D4（主窗口布局）会把这里整套改写：
//      - setMinimumSize(1000, 600) + setWindowTitle(...)
//      - InitWidget() 里拼出 QSplitter 三分天下的完整布局
//      - 继承 ZZListener，并在构造函数末尾注册监听消息、首次触发一次刷新
// ==========================================================

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_pLogWidget(nullptr)
{
    this->setMinimumSize(1000, 600);
    this->setWindowTitle(tr("ZhuzhaoGUI - 光度立体缺陷检测"));

    // ZZLogWidget 的构造函数里会 connect ZZLogMessage 的 sigDebugHtmlData，
    // 从它被 new 出来的这一刻起，日志才具备「上屏」的能力。
    m_pLogWidget = new ZZLogWidget(this);

    // QMainWindow 的地盘要统一交给 centralWidget 管，
    // 不能直接往上 addWidget —— 中间隔了菜单栏/工具栏/状态栏这些区域。
    this->setCentralWidget(m_pLogWidget);
}

MainWindow::~MainWindow() = default;
