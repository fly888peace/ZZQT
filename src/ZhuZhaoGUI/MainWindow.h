#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class ZZLogWidget;   // 前置声明：头文件里只是个指针，不需要知道它的内部细节

// ==========================================================
//  MainWindow —— 主窗口
//
//  当前是【临时挂载状态】：只把 ZZLogWidget 摆进来先跑通，
//  用来验证 D3 日志域写出来的东西到底能不能用。
//
//  D4（主窗口布局）那一节会把这里整套改成课程版：
//      - 继承 ZZListener（监听者模式）
//      - 持有 ZZConfigWidget / ZZLogWidget / CustomImageView
//        / HThumbnailList / VThumbnailList / ZZProcessThread 六个成员
//      - 实现 InitWidget() / RespondMessage() / OnProcessThreadFinished()
//
//  注意文件名大小写：课程里是 MainWindow.h（大写 M、W），
//  与 Qt Creator 新建时的默认 mainwindow.h 不同，别搞混。
// ==========================================================

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private:
    ZZLogWidget* m_pLogWidget;
};

#endif // MAINWINDOW_H
