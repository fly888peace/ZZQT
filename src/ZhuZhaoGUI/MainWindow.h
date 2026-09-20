#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "ZZListener.h"

class ZZConfigWidget;
class ZZLogWidget;
class CustomImageView;
class HThumbnailList;
class VThumbnailList;
class ZZProcessThread;

// 主窗口：搭布局、注册监听、集中响应消息
// 修：源工程写的是 `class MainWindow : public QMainWindow, ZZListener`，
//     第二个基类走的是 class 默认的 private 继承，语义上是错的。
class MainWindow : public QMainWindow, public ZZListener
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

    void RespondMessage(int message) override;

protected:
    bool InitWidget();

protected slots:
    void OnProcessThreadFinished();

private:
    ZZConfigWidget* m_pConfigWidget;
    ZZLogWidget* m_pLogWidget;
    CustomImageView* m_pImageView;
    HThumbnailList* m_pHThumList;
    VThumbnailList* m_pVThumList;
    ZZProcessThread* m_pRunProcess;
};

#endif // MAINWINDOW_H
