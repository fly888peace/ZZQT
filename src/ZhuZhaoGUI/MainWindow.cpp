#include "MainWindow.h"

// ==========================================================
//  【骨架占位文件】
//  按课程实现 MainWindow 时，把这里的内容整个替换掉。
//  课程版构造函数里会做三件事：
//      1. setMinimumSize(1000, 600) + setWindowTitle(...)
//      2. InitWidget() 拼装所有子控件与布局
//      3. 通过 ListenerManger::Instance()->registerMessage(...) 注册监听事件
// ==========================================================

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
}

MainWindow::~MainWindow() = default;
