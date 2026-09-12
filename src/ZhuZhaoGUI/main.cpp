#include "MainWindow.h"

#include <QApplication>

// ==========================================================
// 骨架版 main：只负责创建并显示主窗口，保证工程能跑起来。
// 课程后续会让你在这里追加：
//   - QStyleFactory 设置界面样式（fusion）
//   - 安装 ZZLogMessage 日志处理器
//   - 安装 QTranslator 加载 language_ch.qm
// 那时按课程内容改写这个文件即可。
// ==========================================================

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    MainWindow w;
    w.show();

    return QApplication::exec();
}
