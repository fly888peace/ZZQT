#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

// ==========================================================
//  【骨架占位文件】
//
//  这个文件现在只为了让工程能编出一个空窗口，方便你随时验证环境。
//  按课程讲到 MainWindow 时，请把本文件内容整个替换成自己的实现，
//  课程版会变成：
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
};

#endif // MAINWINDOW_H
