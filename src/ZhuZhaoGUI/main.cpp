#include "MainWindow.h"
#include "ZZLogWidget/ZZLogMessage.h"   // QDEBUG 宏、ZZLogMessage::Instance()

#include <QApplication>
#include <QStyleFactory>
#include <QTranslator>

// ==========================================================
//  main —— 程序的入口
//
//  这里按「先装基础设施，再造界面」的顺序做四件事：
//    ① 建 QApplication（一切 Qt 对象的地基）
//    ② 装样式 fusion
//    ③ 装日志处理器（ZZLogMessage）
//    ④ 装中文翻译器
//  然后创建 MainWindow，进事件循环。
// ==========================================================

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // 设置应用程序整体风格：windows / windowsvista / fusion
    // fusion 是三家里最"不像本机"的，各平台长得一致
    a.setStyle(QStyleFactory::create("fusion"));

    // ----------------------------------------------------------
    //  安装日志接管。
    //  这一行之后，全工程每一句 qDebug / qWarning / qCritical
    //  都不再往「应用程序输出」窗口糊了，改由 ZZLogMessage 统一处理。
    //  位置越靠前越好 —— 装之前打的那些日志是接不住的。
    // ----------------------------------------------------------
    ZZLogMessage::Instance()->installMessageHandler();

    // 此刻 MainWindow 还没创建，ZZLogWidget 里那个 QTextBrowser 也就还没
    // connect 到 sigDebugHtmlData 上。所以这条日志只会写进文件，窗口里看不到。
    QDEBUG("启动日志系统");

    // ----------------------------------------------------------
    //  中文翻译。
    //  必须在 MainWindow 创建【之前】装好：界面控件里的 tr("中文")
    //  是在构造那一刻去翻译器里查的，装晚了那批字符串就已经是原文了。
    //  translator 用栈变量就够：它一直活到 main 返回（也就是 exec() 结束之后）。
    // ----------------------------------------------------------
    QTranslator translator;
    // load() 的返回值标了 [[nodiscard]]，不能不接 —— 顺手把「翻译文件没加载成功」
    // 这个情况做成一条看得见的警告（源工程直接忽略了返回值，界面上的中文没了也无从查起）
    if (!translator.load(":/Resouce/translate/language_ch.qm"))
    {
        QWARNING("中文翻译文件加载失败: :/Resouce/translate/language_ch.qm");
    }
    a.installTranslator(&translator);

    MainWindow w;
    w.show();

    // 界面建好了，ZZLogWidget 已经订阅 —— 从这条开始，日志才会同时出现在窗口里
    QDEBUG("主窗口已显示");

    return QApplication::exec();
}
