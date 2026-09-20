#include "ZZListener.h"
#include "ZZLogWidget/ZZLogMessage.h"

// 饿汉式单例：程序加载时（main 之前）就把实例 new 出来。
//   好处：天生线程安全 —— 静态初始化在 main 启动前完成，不存在竞争窗口
//   代价：一次都没用过，这块内存也已经躺在那儿了
// 对照：本项目另一个单例 ZZLogMessage 用的是「懒汉式 + 双检锁」
ListenerManger* ListenerManger::m_listenerManger = new ListenerManger();

ListenerManger* ListenerManger::Instance()
{
    // 兜底断言：正常永远不为空。Release 构建下 Q_ASSERT 会被整个抹掉，不产生代码
    Q_ASSERT(m_listenerManger != nullptr);
    return m_listenerManger;
}

void ListenerManger::notify(int message)
{
    // 按消息去名册里查：这条消息有人登记过吗
    mmap::iterator iter = m_messageToLister.find(message);

    if (iter == m_messageToLister.end())
    {
        // 源工程这里是被注释掉的 cout，属于静默失败：按钮点了没反应，还查不出为什么。
        // 改成留一条声音，会同时写进 log.txt 并显示在 ZZLogWidget 里。
        QWARNING(QString("无人订阅该消息: 0x%1").arg(message, 2, 16, QChar('0')));
        return;
    }

    // 约定：registerMessage 只在各对象的构造阶段调用。
    // 运行期再调会就地改动 iter.value() 这个 QVector，迭代器立刻失效。
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
    // 名册里一共 4 条消息，写成表格逐个过一遍：
    // 以后新增消息只改这一处，不会再出现「复制了 4 个 if 却漏改一个」
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

        // QMap::operator[] 查不到 key 就当场插入一个默认构造的空 QVector 并交出引用，
        // 所以源工程里那段 if/else 造新 vector 的分支可以省掉。
        QVector<ZZListener*>& listeners = m_messageToLister[single];

        // 查重：同一个人重复登记同一条消息只留一份。
        // 源工程没有这一步，重复登记会让 RespondMessage 被重复调用。
        if (!listeners.contains(listener))
        {
            listeners.push_back(listener);
        }
    }
}
