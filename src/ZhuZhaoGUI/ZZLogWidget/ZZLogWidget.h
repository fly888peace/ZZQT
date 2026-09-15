#ifndef ZZLOGWIDGET_H
#define ZZLOGWIDGET_H

#include <QWidget>

// 前置声明就够了：头文件里只有指针成员，不需要知道这两个类的完整长相。
// 好处是包含本头文件的 .cpp 不会被迫引入 QPushButton / QTextBrowser 的实现细节，
// 编译更快，也不会有循环包含。
class QPushButton;
class QTextBrowser;

/****************************************************************
 * ZZLogWidget 类
 * 日志显示面板 —— 一块 QTextBrowser + 两个按钮
 *
 * 它本身【不生产】日志，只负责订阅：
 *   ZZLogMessage 拿到一条日志 → emit sigDebugHtmlData(html)
 *   → 这里 connect 过去的 QTextBrowser::append 把它显示出来
 *
 * 创建者：MainWindow（MainWindow.cpp 的 InitWidget()）
 ****************************************************************/

class ZZLogWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ZZLogWidget(QWidget *parent = nullptr);

private:
    // 拼控件 + 布局。protected/private 都无所谓，外部不该调它
    bool InitWidget();

    // 把一批重复的收尾操作收进来：滚动到最新一行
    void MoveScrollBarToBottom();

private slots:
    void OnClearBtnClicked();    // 清空按钮
    void OnHelperBtnClicked();   // 帮助按钮：打开课程网页
    void OnLogMessageAppended(const QString& html);   // 收到一条日志

private:
    QPushButton* m_pClearBtn;
    QPushButton* m_pHelperBtn;
    QTextBrowser* m_pLogTextBrowser;
};

#endif // ZZLOGWIDGET_H
