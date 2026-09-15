#include "ZZListener.h"
#include "ZZLogWidget/ZZLogMessage.h"   // QDEBUG 宏 —— ZZLogMessage 已经写好了，
                                        // 这里从 Qt 自带的 qWarning 换成工程统一的日志出口

// ==========================================================
// 饿汉式单例：程序一加载（main 之前）就把实例 new 出来了
//
//   好处：天生线程安全 —— 静态初始化在 main 启动前完成，不存在谁跟谁抢的窗口期
//   代价：哪怕一次都没用过它，这块内存也已经躺在那儿了
//   生命周期：一直活到进程结束，故意不做 delete（析构函数也是 private，
//            外面想 delete 都编译不过；退出后由操作系统统一回收）
//
// 对照：本项目另一个单例 ZZLogMessage 用的是「懒汉式 + 双检锁」，
//      第一次调用 Instance() 时才创建。两种写法在阶段 B 的后半段会同时存在。
// ==========================================================
ListenerManger* ListenerManger::m_listenerManger = new ListenerManger();

ListenerManger* ListenerManger::Instance()
{
    // 兜底断言：m_listenerManger 只可能在上面那行初始化，正常永远不为空。
    // 注意 Q_ASSERT 在 Release 构建（定义了 QT_NO_DEBUG）下会被整个抹掉，不产生任何代码。
    Q_ASSERT(m_listenerManger != nullptr);
    return m_listenerManger;
}

void ListenerManger::notify(int message)
{
    // 按消息去名册里查：这条消息有人登记过吗？
    mmap::iterator iter = m_messageToLister.find(message);

    if (iter == m_messageToLister.end())
    {
        // 没人订阅这条消息。
        // 源工程这里是一段被注释掉的 cout，等于「静默失败」——
        // 按钮点了没反应，还查不出为什么。这里改成留一条声音：
        // 走工程自己的日志出口，会同时写进 log.txt 并显示在 ZZLogWidget 里。
        // （早先这里写的是 Qt 自带的 qWarning，那时候 ZZLogMessage 还没落地。）
        QWARNING(QString("无人订阅该消息: 0x%1").arg(message, 2, 16, QChar('0')));
        return;
    }

    // 通知所有登记过这条消息的监听者
    //
    // 约定：注册只在各对象的构造阶段做，运行期不再调 registerMessage。
    // 否则下面这个循环正在遍历的 QVector 会被就地改动，迭代器立刻失效。
    for (ZZListener* listener : iter.value())
    {
        if (listener != nullptr)
        {
            listener->RespondMessage(message);   // 调用监听者的处理接口
        }
    }
}

void ListenerManger::registerMessage(int message, ZZListener* listener)
{
    // 名册里一共就这 4 条消息，写成表格逐个过一遍。
    // 好处：以后新增消息只改这个数组一处，不会再出现「复制了 4 个 if 却漏改一个」。
    static constexpr int kAllMessages[] = {
        MESSAGE::ZHUZHAO_UPDATE_SRCIMAGE,
        MESSAGE::ZHUZHAO_UPDATE_RESULTI,
        MESSAGE::ZHUZHAO_RESET,
        MESSAGE::ZHUZHAO_RUNONCE
    };

    for (const int single : kAllMessages)
    {
        // 位与：message 里没点亮 single 这一位，说明本次登记跟它无关
        if ((message & single) != single)
        {
            continue;
        }

        // 取这条消息名下的监听者列表。
        // QMap::operator[] 有个好用的脾气：查不到 key 就当场插入一个「默认构造的空 QVector」
        // 并把它的引用交出来。所以「这条消息第一次出现」和「已经有人登记过」两种情况的
        // 写法合并成了一行，源工程里那段 if/else 造新 vector 的分支就不需要了。
        QVector<ZZListener*>& listeners = m_messageToLister[single];

        // 查重：同一个人重复登记同一条消息，只保留一份。
        // 源工程没有这一步，重复登记会导致 RespondMessage 被重复调用。
        if (!listeners.contains(listener))
        {
            listeners.push_back(listener);
        }
    }
}
