#include "ZZLogMessage.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QMutexLocker>
#include <QTextStream>

// 全局日志器实现：接管 Qt 日志出口 → 写盘 + 上屏

namespace
{
// 单个日志文件的大小上限：超过就把当前这份改名归档，下次写会自动生成新的 log.txt
constexpr qint64 kMaxLogFileSize = 1024 * 1024;   // 1 MB

// 最多留多少份历史归档：log_1.txt ~ log_99.txt
constexpr int kMaxArchiveCount = 99;

// 第 index 份归档的完整路径，例如 D:/…/src/bin/log/2026-09-15/log_3.txt
QString archiveFilePath(const QString& logDir, int index)
{
    return QString("%1/log_%2.txt").arg(logDir).arg(index);
}

// 把已经写满的 logFile 改名成 log_N.txt。
// 调用前提：logFile 处于已关闭状态，且调用者已持有 outputMessage 里那把互斥锁。
void rollLogFile(QFile& logFile, const QString& logDir)
{
    // 常规路径：找一个还没被占用的编号
    for (int index = 1; index <= kMaxArchiveCount; ++index)
    {
        const QString target = archiveFilePath(logDir, index);

        // 修：源工程用 if (logfile.size() < 4) 判断「这个编号还没人用」，
        //     拿文件大小去推断存在性是个魔数式的猜测 —— 判断存在就该用 exists()。
        if (QFile::exists(target))
        {
            continue;
        }
        // 修：源工程丢掉了 rename() 的返回值。改名会因目标被占用、磁盘只读等原因失败，
        //     那种情况下日志会以一种完全看不见的方式被丢掉。
        if (logFile.rename(target))
        {
            return;
        }
    }

    // 兜底：99 个编号全占满了。
    // 源工程跑完上面的循环什么都不做就返回 —— log.txt 会继续一路长大，同样没人告诉你。
    // 这里删掉最老的 log_1.txt，后面每份往前挪一格，空出来的 log_99.txt 给当前这份用。
    QFile::remove(archiveFilePath(logDir, 1));
    for (int index = 2; index <= kMaxArchiveCount; ++index)
    {
        // 每一步的目标位置都在上一步被腾空了：1 号刚被删，2 挪到 1，于是 2 空出来……
        QFile::rename(archiveFilePath(logDir, index), archiveFilePath(logDir, index - 1));
    }
    if (!logFile.rename(archiveFilePath(logDir, kMaxArchiveCount)))
    {
        // 连兜底都失败（比如磁盘满了）：删掉当前这份写满的，保证下次还能从头写日志
        logFile.remove();
    }
}

// 被 Qt 回调的日志处理函数。
// 它的地址在 main() 里经 installMessageHandler() 交给 Qt 库内部的全局变量；
// 此后任何一处 qDebug/qWarning/qCritical 都会让 Qt 转过头来调用它 ——
// 由 Qt 决定什么时候调，不是我们主动调的。
// 放进匿名 namespace = 内部链接，符号不暴露到整个链接域
// （源工程写成全局函数，万一别的库也有同名函数，链接阶段就要打架）。
void outputMessage(QtMsgType type, const QMessageLogContext& context, const QString& msg)
{
    Q_UNUSED(context);   // 行号我们走 __FILE__/__LINE__ 宏拿，这里的 context 用不上

    ZZLogMessage* instance = ZZLogMessage::Instance();
    QString messageHtml;

    // 下面这对大括号是【互斥锁的作用域】：写盘与归档都在里面排队串行完成，
    // 靠着这一个块，emit 顺理成章地落到了锁外面（见下面 ⑤）。
    {
        // 这把锁保护「多个线程往同一个 log.txt 追加写」这件事。
        // static 是关键：全进程只有这一份，所有线程抢的都是同一把锁。
        // 漏掉 static，每个线程进来都造一把自己的新锁，加锁等于没加。
        static QMutex mutex;
        QMutexLocker locker(&mutex);

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

        // ③ 追加写进当天的 log.txt（logPath() 内部保证目录已存在）
        const QString logDir = instance->logPath();
        QFile file(logDir + "/" + instance->logName());

        if (file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text))
        {
            {
                QTextStream textStream(&file);
                // 开文件时带了 QIODevice::Text，Windows 下 Qt 会自动把 "\n" 换成 "\r\n"，
                // 所以这里只写 "\n" 就够。源工程没开 Text 却手写 "\r\n"，是绕了一圈才凑对的写法。
                textStream << message << "\n";
                // 内层块结束 → textStream 析构 → 缓冲区冲进磁盘。
                // 收窄作用域就是为了让析构发生在 close() 之前，不用手动 flush()。
            }

            // ④ 文件大小必须在 close() 【之前】取。
            //    修：源工程先把活干反了 —— 先 close()（:81），后 size()（:87）。
            const qint64 currentSize = file.size();
            file.close();

            if (currentSize >= kMaxLogFileSize)
            {
                rollLogFile(file, logDir);
            }
        }
        // 打开失败（比如程序装在 C:\Program Files 下没写权限）：文件这条路走不通，
        // 但下面的 ⑤ 照旧，界面仍然能看到日志。
        // 【这里绝对不能用 QDEBUG 报这个错】—— QDEBUG 又会触发本函数，无限递归。
    }

    // ⑤ 把处理好的日志发给界面显示。必须放在锁外面：
    //    同线程时 Qt::AutoConnection 会【直接调用】槽，也就是 append 同步跑完；
    //    如果还在锁里，append 期间若有人再打日志，就会回来抢同一把锁 —— 死锁。
    //    跨线程时（算法线程打日志）连接自动转 Queued，排队到界面所属的那条线程执行。
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
        // 修：源工程写的是 QMutex muter;（没有 static）—— 每调一次造一把新锁，
        //     两个线程各拿各的锁同时通过第二道检查，双检锁形同虚设。
        static QMutex mutex;   // static 是关键：全进程共用这一把
        QMutexLocker locker(&mutex);

        // 第二道检查：等锁的时候可能已经有别人把活干完了，再看一眼。
        // 省掉这一眼，就会 new 出第二个对象来。
        if (m_pLogInstance == nullptr)
        {
            m_pLogInstance = new ZZLogMessage();
        }
    }
    // 注：上面外层那次读是无锁的，严格按 C++11 内存模型属于 data race
    //     （实践中 x86 对齐指针的读写是原子的，所以能跑）。
    //     要彻底安全可以用 std::atomic<ZZLogMessage*>，或者干脆写局部静态变量：
    //         static ZZLogMessage instance;  return &instance;
    //     C++11 起编译器保证它只初始化一次且线程安全。
    //     这里保留双检锁是为了和课程讲的写法对齐（ZZListener.cpp 那个饿汉单例可对照看）。
    return m_pLogInstance;
}

ZZLogMessage::ZZLogMessage(QObject *parent)
    : QObject(parent)
    , m_pOldHandler(nullptr)
{
}

ZZLogMessage::~ZZLogMessage() = default;

void ZZLogMessage::installMessageHandler()
{
    // qInstallMessageHandler 干两件事：
    //   ① 把 outputMessage 的地址存进 Qt 库内部那个全局变量
    //   ② 把【上一个】处理器的地址当作返回值交出来
    // 源工程把这个返回值丢掉了，卸载时就不知道该还原给谁。
    m_pOldHandler = qInstallMessageHandler(outputMessage);

    // 给 uninstallMessageHandler 补一个调用点 —— 它在源工程里全工程零调用
    QObject::connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit,
                     this, &ZZLogMessage::uninstallMessageHandler);
}

void ZZLogMessage::uninstallMessageHandler()
{
    // 精确还原成接任之前的那位。
    // 修：源工程这里无脑传 nullptr，等于「以后谁都别接管，一律走 Qt 默认输出」——
    //     如果在本项目之前还装过别的处理器，就被悄悄抹掉了。
    qInstallMessageHandler(m_pOldHandler);
}

QString ZZLogMessage::logPath()
{
    const QString today = QDateTime::currentDateTime().toString("yyyy-MM-dd");

    QMutexLocker locker(&m_pathMutex);   // 保护下面两个缓存字段
    if (m_cachedDate != today)
    {
        // 跨天了（或第一次调用），重算一次。
        // 修：源工程用相对路径 "log/2026-09-15"，工作目录由启动者决定 ——
        //     在 Qt Creator 里点运行落在构建目录，双击 exe 落在 src/bin，日志会散落各处。
        //     这里以 exe 所在目录打底，位置固定。
        const QString dirPath = QCoreApplication::applicationDirPath() + "/log/" + today;
        if (!QDir(dirPath).exists())
        {
            QDir().mkpath(dirPath);   // 用绝对路径创建，不依赖当前工作目录
        }
        m_cachedDate = today;
        m_cachedPath = dirPath;
    }
    return m_cachedPath;   // 末尾不带斜杠，调用方自己拼 "/"
}

// 当前正在写的日志文件名（归档出来的那些叫 log_N.txt）
QString ZZLogMessage::logName()
{
    return "log.txt";
}
