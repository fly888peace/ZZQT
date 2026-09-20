#ifndef ZZLOGWIDGET_H
#define ZZLOGWIDGET_H

#include <QWidget>

// 前置声明就够了：头文件里只有指针成员，不需要知道这两个类的完整定义。
// 好处是包含本头文件的 .cpp 不会被迫引入它们的实现细节，编译更快，也不会有循环包含。
class QPushButton;
class QTextBrowser;

// 日志显示面板 —— 一块 QTextBrowser + 两个按钮。
// 它本身【不生产】日志，只负责订阅：
//   ZZLogMessage 拿到一条日志 → emit sigDebugHtmlData(html) → 本控件把它显示出来
// 创建者：MainWindow::InitWidget()
class ZZLogWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ZZLogWidget(QWidget *parent = nullptr);

private:
    // 拼控件 + 布局
    bool InitWidget();

    // 滚动到最新一行
    void MoveScrollBarToBottom();

private slots:
    void OnClearBtnClicked();                        // 清空
    void OnHelperBtnClicked();                       // 打开课程网页
    void OnLogMessageAppended(const QString& html);  // 收到一条日志

private:
    QPushButton* m_pClearBtn;
    QPushButton* m_pHelperBtn;
    QTextBrowser* m_pLogTextBrowser;
};

#endif // ZZLOGWIDGET_H
