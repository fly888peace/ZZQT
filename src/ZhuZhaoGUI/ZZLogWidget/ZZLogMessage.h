#ifndef ZZLOGMESSAGE_H
#define ZZLOGMESSAGE_H

#include <QObject>
#include <QString>
#include <QMutex>
#include <QtGlobal>   // QtMessageHandler 这个类型定义在这里
#include <QDebug>     // 下面的宏展开后会用到 qDebug() / qWarning() / qCritical()

/****************************************************************
 * ZZLogMessage 类
 * 全局日志器（懒汉式单例）。全工程的调试信息都由它统一处理。
 *
 * 它干三件事：
 *   ① 接管 Qt 的日志出口 —— qDebug/qWarning/qCritical/qFatal 发出的所有信息
 *   ② 写盘 —— 追加进当天日志目录下的 log.txt，单个文件超过 1MB 自动轮转归档
 *   ③ 上屏 —— 把信息加工成带颜色的 HTML，用信号 sigDebugHtmlData 发给界面显示
 *
 * 本文件涉及的东西就四样：
 *   QDEBUG / QWARNING / QCRITICAL  —— 三个宏（本 :36-38 行）
 *   class ZZLogMessage             —— 单例类本体（本 :73 行起）
 *
 * 谁调用谁：
 *   main.cpp         → ZZLogMessage::Instance()->installMessageHandler()  安装接管
 *   ZZLogWidget.cpp  → connect(sigDebugHtmlData → QTextBrowser::append)   订阅显示
 *   任意 .cpp        → QDEBUG("...")                                      打日志
 *
 * 注意：宏定义在本文件里，所以【用到 QDEBUG 的 .cpp 必须 include 本头文件】。
 ****************************************************************/

// ==============================================================
//  日志输出宏
//    QDEBUG(message)     普通调试信息，界面上显示灰字
//    QWARNING(message)   警告，橙字
//    QCRITICAL(message)  错误，红字
//
//  三个写法上的讲究：
//
//  1) 用 do { ... } while(0) 包起来
//     宏展开以后是一条完整的语句，可以安全地用在 if/else 里：
//         if (ok) QDEBUG("成功"); else QDEBUG("失败");
//     源工程那四个宏自带结尾分号（ZZLogMessage.h:13-16），
//     一旦写进 if/else，MSVC 会报 C2181「没有匹配 if 的非法 else」。
//
//  2) (message) 外面套一层括号
//     防止调用处传进来的表达式里有优先级比 << 更低的运算符（比如三目 ?: ）时算乱。
//
//  3) __FUNCTION__ / __LINE__ 是编译器内建宏
//     宏在预处理阶段被【原样复制】到调用点，所以这两个宏展开得到的
//     永远是「写这条日志的那一行」，而不是 ZZLogMessage 内部的行号。
//     这就是宏能拿到调用点信息的全部原因。
//
//  为什么没有 QFATAL：
//     Qt 的 qFatal() 是【printf 风格】的函数：qFatal("failed: %d", err)，
//     返回类型是 void，不能像 qDebug() 那样用 << 串内容。
//     源工程把它写成 qFatal() << ...（ZZLogMessage.h:16），一调用就编译不过。
//     而且 qFatal 打完日志会【直接终止进程】（默认 abort），包成宏容易误伤。
//     真到「程序活不下去了」的地步，直接用 Qt 原生写法：
//         qFatal("相机打不开: %s", msg.toUtf8().constData());
// ==============================================================
#define QDEBUG(message)     do { qDebug()    << " [FUNCTION]: " << __FUNCTION__ << " [LINE]: " << __LINE__ << " [LOG]: " << (message); } while (0)
#define QWARNING(message)   do { qWarning()  << " [FUNCTION]: " << __FUNCTION__ << " [LINE]: " << __LINE__ << " [LOG]: " << (message); } while (0)
#define QCRITICAL(message)  do { qCritical() << " [FUNCTION]: " << __FUNCTION__ << " [LINE]: " << __LINE__ << " [LOG]: " << (message); } while (0)

// QWARNING / QCRITICAL 在源工程里一次都没被调用过（只有 QDEBUG 用了 2 次）。
// 这里仍然成套保留：三个宏就是日志系统的三档出口，
// 颜色不同、级别不同，缺哪一档以后想补还得回来改宏定义区。

// ==============================================================
//  单例类本体
// ==============================================================
class ZZLogMessage : public QObject
{
    Q_OBJECT

public:
    // 拿唯一实例的那扇门。整个进程从头到尾只有这么一个对象，
    // 而且故意不做 delete —— 进程退出时由操作系统统一回收。
    static ZZLogMessage* Instance();

    // 把 outputMessage（定义在 ZZLogMessage.cpp 里）的地址交给 Qt 接管 —— 安装
    void installMessageHandler();
    // 还回去，恢复 Qt 默认的输出行为 —— 卸载
    void uninstallMessageHandler();

    // 当前日志要写进哪个目录（绝对路径，末尾【不带】斜杠）。目录不存在会就地创建。
    QString logPath();
    // 当前正在写的那个文件的名字
    QString logName();

private:
    // 构造/析构设为 private：外面只能通过 Instance() 拿到那一个对象
    explicit ZZLogMessage(QObject *parent = nullptr);
    ~ZZLogMessage() override;

    // 单例禁止拷贝与赋值 —— 「唯一」和「能复制」是自相矛盾的
    ZZLogMessage(const ZZLogMessage&) = delete;
    ZZLogMessage& operator=(const ZZLogMessage&) = delete;

    static ZZLogMessage* m_pLogInstance;   // 全局唯一实例的地址

    // 接任之前的那位「原处理器」。
    // qInstallMessageHandler() 会把上一个处理器的地址返回出来，
    // 存住它，卸载的时候才知道该还原成谁（源工程把这个返回值丢掉了）。
    QtMessageHandler m_pOldHandler;

    // 下面三个字段是为了不给每条日志都做一次「拼日期 + 查磁盘」。
    // logPath() 每条日志都会被调用，源工程在里面直接 mkpath，
    // 等于每条日志都要碰一次磁盘；这里按天缓存。
    QMutex m_pathMutex;
    QString m_cachedDate;
    QString m_cachedPath;

signals:
    // 加工好的彩色 HTML 日志，发给 ZZLogWidget 显示。
    //
    // 源工程还有一条一模一样的 sigDebugStrData（纯文本版），
    // 但全工程没有任何一处 connect 它 —— 一条没人接的信号，删掉。
    void sigDebugHtmlData(const QString& html);
};

#endif // ZZLOGMESSAGE_H
