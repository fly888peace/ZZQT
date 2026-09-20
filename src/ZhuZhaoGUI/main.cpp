#include "MainWindow.h"
#include "ZZLogWidget/ZZLogMessage.h"

#include <QApplication>
#include <QStyleFactory>
#include <QTranslator>

// 程序入口。顺序有讲究：日志处理器与翻译器都必须早于 MainWindow 创建。
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setStyle(QStyleFactory::create("fusion"));   // windows / windowsvista / fusion

    // 这一行之后，全工程的 qDebug / qWarning / qCritical 都改由 ZZLogMessage 处理
    ZZLogMessage::Instance()->installMessageHandler();
    QDEBUG("启动日志系统");

    // 翻译器必须在 MainWindow 之前装：控件里的 tr() 在构造那一刻就去查了
    QTranslator translator;
    if (!translator.load(":/Resouce/translate/language_ch.qm"))
    {
        QWARNING("中文翻译文件加载失败");
    }
    a.installTranslator(&translator);

    MainWindow w;
    w.show();
    QDEBUG("主窗口已显示");

    return QApplication::exec();
}
