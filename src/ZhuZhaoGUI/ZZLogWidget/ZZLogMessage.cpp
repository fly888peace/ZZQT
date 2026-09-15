#include "ZZLogMessage.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QMutexLocker>
#include <QTextStream>

/**************************************************
 * 一个日志的单例实现
 * 负责把 Qt 的日志打印重定向到一个界面控件上，
 * 同时把日志落盘到本地磁盘
 **************************************************/

namespace
{
// 单个日志文件的大小上限：超过就把当前这份改名归档，下一次写会自动生成新的 log.txt
constexpr qint64 kMaxLogFileSize = 1024 * 1024;   // 1 MB

// 最多留多少份历史归档：log_1.txt ~ log_99.txt
constexpr int kMaxArchiveCount = 99;

// 第 index 份归档的完整路径，例如 D:/…/src/bin/log/2026-09-15/log_3.txt
QString archiveFilePath(const QString& logDir, int index)
{
    return QString("%1/log_%2.txt").arg(logDir).arg(index);
}

// ----------------------------------------------------------
//  归档：把已经写满的 logFile 改名成 log_N.txt
//
//  调用前提：logFile 处于【已关闭】状态，且 logDir 目录一定存在。
//  调用者已经持有了 outputMessage 里那把互斥锁，所以这里不用再加锁。
// ----------------------------------------------------------
void rollLogFile(QFile& logFile, const QString& logDir)
{
    // 常规路径：找一个还没被占用的编号
    for (int index = 1; index <= kMaxArchiveCount; ++index)
    {
        const QString target = archiveFilePath(logDir, index);
        if (QFile::exists(target))
        {
            continue;
        }
        // 源工程在这里用的是 if (logfile.size() < 4) —— 拿「文件小于 4 字节」
        // 来推断「这个编号还没人用」，这是个魔数式的猜测。
        // 想判断文件在不在，就该用 QFile::exists()。
        // 另外 rename() 的返回值一定要看：改名会因为目标被占用、磁盘只读等原因失败，
        // 源工程把它丢掉了，那种情况下日志会以一种完全看不见的方式被丢掉。
        if (logFile.rename(target))
        {
            return;
        }
    }

    // 兜底：99 个编号全占满了。
    // 源工程跑完上面的循环什么都不做就返回了 —— log.txt 会继续一路长大，同样没人告诉你。
    // 这里的做法是：删掉最老的 log_1.txt，后面的每一份往前挪一格，
    // 空出来的 log_99.txt 给当前这份用。
    QFile::remove(archiveFilePath(logDir, 1));
    for (int index = 2; index <= kMaxArchiveCount; ++index)
    {
        // 每一步的目标位置都在上一步被腾空了：
        // 1 号刚被删，2 挪到 1，于是 2 空出来，3 挪到 2 …… 依次传递下去。
        QFile::rename(archiveFilePath(logDir, index), archiveFilePath(logDir, index - 1));
    }
    if (!logFile.rename(archiveFilePath(logDir, kMaxArchiveCount)))
    {
        // 连兜底都没成功（比如磁盘满了）：索性删掉当前这份写满的，
        // 保证下一次还能从头开始写日志，总比卡在一个 1MB 的文件上强
        logFile.remove();
    }
}

// ----------------------------------------------------------
//  被 Qt 回调的那个日志处理函数
//
//  它的地址在 main() 里经过
//      ZZLogMessage::Instance()->installMessageHandler()
//  交给了 Qt 库内部的全局变量。此后任何一处调用
//  qDebug() / qWarning() / qCritical()，Qt 都会转过头来调用它 ——
//  由 Qt 决定什么时候调，不是我们主动调的。
//  函数指针的类型 QtMessageHandler 是 Qt 规定的：
//      void (*)(QtMsgType, const QMessageLogContext&, const QString&)
//
//  放进匿名 namespace 的意义：让这个名字只在本 .cpp 里可见（内部链接）。
//  源工程把 outputMessage 写成全局函数，符号会暴露到整个链接域，
//  万一别的库里也有个同名函数，链接阶段就要打架。
// ----------------------------------------------------------
void outputMessage(QtMsgType type, const QMessageLogContext& context, const QString& msg)
{
    Q_UNUSED(context);   // 行号我们走 __FILE__/__LINE__ 宏拿，这里的 context 用不上

    ZZLogMessage* instance = ZZLogMessage::Instance();
    QString messageHtml;

    // ============================================================
    //  下面这一对大括号是【互斥锁的作用域】，写盘和归档都在里面排队串行完成。
    //  靠着这一个块，emit 顺理成章地落到了锁外面（见下面 ⑤）。
    // ============================================================
    {
        // 这把锁保护的是「大家往同一个 log.txt 里追加写」这件事。
        // static 局部变量 = 全进程只有这一份，所有线程抢的都是同一把锁。
        // 如果漏掉 static，每个线程进来都造一把自己的新锁，
        // 加锁等于没加 —— 这个坑我们在 ZZLogMessage::Instance() 里也修过一次。
        static QMutex mutex;
        QMutexLocker locker(&mutex);   // 出了这个块（正常走出或抛异常）自动解锁

        // ① 按日志级别定一段前缀和一个显示颜色
        QString levelText;
        QString color;
        switch (type)
        {
        case QtDebugMsg:    levelText = "[Debug]";    color = "darkgray";   break;
        case QtWarningMsg:  levelText = "[Warning]";  color = "darkorange"; break;
        case QtCriticalMsg: levelText = "[Error]";    color = "red";        break;
        case QtFatalMsg:    levelText = "[Fatal]";    color = "red";        break;
        default:            levelText = "[Default]";  color = "red";        break;
        }

        // ② 拼两版：干净版写进文件，彩色 HTML 版发给界面
        const QString timeText = QDateTime::currentDateTime().toString("[yyyy-MM-dd hh:mm:ss]");
        const QString message = timeText + levelText + msg;
        messageHtml = QString("<font color=%1>%2</font>").arg(color, message);

        // ③ 追加写进当天的 log.txt
        //    logPath() 内部保证目录已经存在；拼接时自己补 "/"，
        //    所以不会出现源工程那种 log/2026-09-15//log_1.txt 的双斜杠
        //    （logPath() 结尾自带 "/"，源工程拼接时又写了一个 "/log_%1.txt"）。
        const QString logDir = instance->logPath();
        QFile file(logDir + "/" + instance->logName());

        if (file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text))
        {
            {
                QTextStream textStream(&file);
                // 打开时带了 QIODevice::Text，Windows 下 Qt 会把 "\n" 自动换成 "\r\n"，
                // 所以这里只写 "\n" 就够了。
                // 源工程没开 Text 却手写 "\r\n"，是绕了一圈才凑对的写法。
                textStream << message << "\n";
                // 这个内层块结束时 textStream 析构，顺手把缓冲区冲进磁盘。
                // 收窄作用域就是为了让析构发生在 file.close() 之前，不用手动 flush()。
            }

            // ④ 文件大小必须在 close() 【之前】取。
            //    源工程是把活干反了：先 close()（ZZLogMessage.cpp:81），后 size()（:87）。
            const qint64 currentSize = file.size();
            file.close();

            // 没到上限，本次日志处理结束
            if (currentSize >= kMaxLogFileSize)
            {
                // 到上限了：把这份改名归档，下次自然会长出一个新的 log.txt
                rollLogFile(file, logDir);
            }
        }
        // 打开失败（比如程序装在 C:\Program Files 下没写权限）：
        // 文件这条路走不通，但下面的 ⑤ 照旧，界面仍然能看到日志。
        // 【这里绝对不能用 QDEBUG 报这个错】—— QDEBUG 又会触发本函数，无限递归。
    }

    // ⑤ 把处理好的日志发给界面显示。
    //    必须放在锁外面：sigDebugHtmlData 接到 QTextBrowser::append 是直连
    //    （两边都在主线程），也就是说 append 是【拿着这把锁】去更新界面的；
    //    如果 append 的过程中又有人打日志，就会回来抢同一把锁 —— 死锁。
    emit instance->sigDebugHtmlData(messageHtml);
}

} // namespace

// ============================================================
//  单例
// ============================================================
ZZLogMessage* ZZLogMessage::m_pLogInstance = nullptr;

ZZLogMessage* ZZLogMessage::Instance()
{
    // 懒汉式 + 双检锁（DCLP）：第一次有人调用才 new
    if (m_pLogInstance == nullptr)
    {
        // 第一道检查：绝大多数调用走到这里就直接返回了，连锁都不碰，省时间
        static QMutex mutex;              // ← static 是关键：全进程共用这一把
        QMutexLocker locker(&mutex);
        if (m_pLogInstance == nullptr)
        {
            // 第二道检查：等锁的时候可能已经有别人把活干完了，再看一眼。
            // 省掉这一眼，就会 new 出第二个对象来。
            m_pLogInstance = new ZZLogMessage();
        }
    }
    // 源工程这里写的是 QMutex muter;（没有 static）—— 每调一次造一把新锁，
    // 两个线程各拿各的锁同时通过第二道检查，双检锁形同虚设。
    //
    // 顺带一提：C++11 之后有更省心的写法 —— 在函数里直接
    //     static ZZLogMessage instance;  return &instance;
    // 编译器保证它只初始化一次且线程安全。这里保留双检锁，
    // 是为了和课程讲的写法对齐（ZZListener.cpp 里那个饿汉单例又是一种写法，可以对着看）。
    return m_pLogInstance;
}

ZZLogMessage::ZZLogMessage(QObject *parent)
    : QObject(parent)
    , m_pOldHandler(nullptr)
{
}

ZZLogMessage::~ZZLogMessage() = default;

// ============================================================
//  安装消息处理器
// ============================================================
void ZZLogMessage::installMessageHandler()
{
    // qInstallMessageHandler 干两件事：
    //   ① 把 outputMessage 的地址存进 Qt 库内部的那个全局变量
    //   ② 把【上一个】处理器的地址当作返回值交出来
    // 源工程把这个返回值丢掉了，卸载时就不知道该还原给谁。
    m_pOldHandler = qInstallMessageHandler(outputMessage);

    // 给 uninstallMessageHandler 补一个调用点 —— 它在源工程里全工程零调用，
    // 等于声明了一个永远不会被执行的函数。
    QObject::connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit,
                     this, &ZZLogMessage::uninstallMessageHandler);
}

// ============================================================
//  卸载消息处理器
// ============================================================
void ZZLogMessage::uninstallMessageHandler()
{
    // 精确还原成接任之前的那位。
    // 源工程这里无脑传 nullptr，等于「以后谁都别接管，一律走 Qt 默认输出」——
    // 如果在本项目之前还装过别的处理器，就被悄悄抹掉了。
    qInstallMessageHandler(m_pOldHandler);
}

// ============================================================
//  日志目录：绝对路径 <程序所在目录>/log/yyyy-MM-dd
//
//  为什么必须用 QCoreApplication::applicationDirPath() 打底：
//  程序的工作目录是【启动它的人】给的。在 Qt Creator 里点运行，工作目录是构建目录；
//  双击 exe，工作目录是 exe 所在的 src/bin；从命令行在 D 盘根目录敲一个绝对路径启动，
//  工作目录就变成了 D:\。
//  源工程用的是相对路径 "log/2026-09-15"，日志文件会随着启动方式不同散落在各处
//  （源工程整个 src 目录里一次都没出现过 applicationDirPath）。
// ============================================================
QString ZZLogMessage::logPath()
{
    const QString today = QDateTime::currentDateTime().toString("yyyy-MM-dd");

    QMutexLocker locker(&m_pathMutex);   // 保护下面两个缓存字段
    if (m_cachedDate != today)
    {
        // 跨天了（或者第一次调用），重算一次
        const QString dirPath = QCoreApplication::applicationDirPath() + "/log/" + today;
        if (!QDir(dirPath).exists())
        {
            // 用【绝对路径】创建，不依赖当前工作目录。
            // 源工程写的是 dir.mkpath("./") —— mkpath 的参数是相对 QDir 自己的路径，
            // 而 "." 指的就是它自己，绕了一圈。
            QDir().mkpath(dirPath);
        }
        m_cachedDate = today;
        m_cachedPath = dirPath;
    }
    return m_cachedPath;
}

// ============================================================
//  当前正在写的日志文件名（归档出来的那些叫 log_N.txt）
// ============================================================
QString ZZLogMessage::logName()
{
    return "log.txt";
}
