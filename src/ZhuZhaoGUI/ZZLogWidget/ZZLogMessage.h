#ifndef ZZLOGMESSAGE_H
#define ZZLOGMESSAGE_H

#include <QObject>
#include <QString>
#include <QMutex>
#include <QtGlobal>   // QtMessageHandler 这个类型定义在这里
#include <QDebug>     // 下面三个宏展开后要用 qDebug() / qWarning() / qCritical()

// 全局日志器（懒汉式单例），全工程的调试信息都由它统一处理：
//   ① 接管 Qt 日志出口 —— qDebug/qWarning/qCritical/qFatal 发出的信息都流进来
//   ② 写盘 —— 追加进 log/yyyy-MM-dd/log.txt，单文件超过 1MB 自动轮转归档
//   ③ 上屏 —— 加工成带颜色的 HTML，经 sigDebugHtmlData 发给 ZZLogWidget
//
// 谁调用谁：
//   main.cpp        → Instance()->installMessageHandler()   安装接管
//   ZZLogWidget.cpp → connect(sigDebugHtmlData → append)    订阅显示
//   任意 .cpp       → QDEBUG("...")                          打日志
//
// 宏定义在本文件里，所以【用到 QDEBUG 的 .cpp 必须 include 本头文件】。

// 日志输出宏。三档出口，颜色与级别各不同。
//
// 两处写法讲究：
//  1) 用 do{...}while(0) 包起来，展开后是一条完整语句，能安全写进 if/else。
//     源工程那四个宏自带结尾分号，写进 if/else 会报 MSVC C2181「没有匹配 if 的非法 else」。
//  2) (message) 外套一层括号，防止实参里有优先级比 << 低的运算符时算乱顺序。
//
// __FUNCTION__ / __LINE__ 是编译器内建宏。宏在预处理阶段被原样复制到调用点，
// 所以它们展开得到的永远是「写这条日志的那一行」，而不是本文件内部的行号。
//
// 这里故意没有 QFATAL：qFatal() 是 printf 风格（qFatal("err %d", code)），返回 void，
// 不能像 qDebug() 那样用 << 串内容；源工程把它写成 qFatal() << ...（ZZLogMessage.h:16），
// 一调用就编译不过。而且它打完日志会直接 abort 进程，不适合包成通用宏。
// 真到程序活不下去时用 Qt 原生写法：
//     qFatal("相机打不开: %s", msg.toUtf8().constData());
#define QDEBUG(message)     do { qDebug()    << " [FUNCTION]: " << __FUNCTION__ << " [LINE]: " << __LINE__ << " [LOG]: " << (message); } while (0)
#define QWARNING(message)   do { qWarning()  << " [FUNCTION]: " << __FUNCTION__ << " [LINE]: " << __LINE__ << " [LOG]: " << (message); } while (0)
#define QCRITICAL(message)  do { qCritical() << " [FUNCTION]: " << __FUNCTION__ << " [LINE]: " << __LINE__ << " [LOG]: " << (message); } while (0)

class ZZLogMessage : public QObject
{
    Q_OBJECT

public:
    // 拿唯一实例的那扇门。整个进程只有这一个对象，故意不做 delete，退出时由操作系统回收
    static ZZLogMessage* Instance();

    void installMessageHandler();     // 把 outputMessage 的地址交给 Qt 接管
    void uninstallMessageHandler();   // 还回去，恢复 Qt 默认输出行为

    QString logPath();   // 当天日志目录（绝对路径，末尾不带斜杠），不存在会就地创建
    QString logName();   // 当前正在写的那个文件的名字

private:
    // 构造 / 析构私有：外面只能通过 Instance() 拿到那一个对象
    explicit ZZLogMessage(QObject *parent = nullptr);
    ~ZZLogMessage() override;

    // 单例禁止拷贝与赋值 ——「唯一」和「能复制」是自相矛盾的
    ZZLogMessage(const ZZLogMessage&) = delete;
    ZZLogMessage& operator=(const ZZLogMessage&) = delete;

    static ZZLogMessage* m_pLogInstance;   // 全局唯一实例的地址

    // 接任之前的那位「原处理器」。qInstallMessageHandler() 会把上一个处理器的地址返回出来，
    // 存住它，卸载的时候才知道该还原成谁（源工程把这个返回值丢掉了）。
    QtMessageHandler m_pOldHandler;

    // 按天缓存路径。logPath() 每条日志都会被调用，源工程在里面直接 mkpath，
    // 等于每条日志都要碰一次磁盘。
    QMutex m_pathMutex;
    QString m_cachedDate;
    QString m_cachedPath;

signals:
    // 加工好的彩色 HTML 日志，发给 ZZLogWidget 显示。
    // 源工程还有一条一模一样的 sigDebugStrData（纯文本版），全工程没有任何一处 connect 它，已删。
    void sigDebugHtmlData(const QString& html);
};

#endif // ZZLOGMESSAGE_H
