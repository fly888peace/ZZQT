#ifndef ZZ_LISTENER_H
#define ZZ_LISTENER_H

#include <QMap>
#include <QVector>   // Qt 6 里 QVector 只是 QList 的别名，保留这个拼写是为了跟课程源码对照

// 观察者模式在本项目里的两个角色：
//   ZZListener     —— 监听者抽象基类，谁要收消息谁就继承它
//   ListenerManger —— 事件总管（单例），负责「登记」与「广播」
//
// 一条消息的流转：registerMessage(消息, this) 登记 → notify(消息) 广播
//                  → 逐个调监听者的 RespondMessage(消息)
// 本项目只有 4 条消息、1 个监听者（MainWindow），是该模式的最小可用规模。

// 消息表。每条必须是不重复的 2 的幂：
// 这样调用者才能用「位或」一次登记好几条，registerMessage 也才能用「位与」拆回单条。
enum MESSAGE {
    ZHUZHAO_UPDATE_SRCIMAGE = 0x01,   // 刷新输入图像列表
    ZHUZHAO_UPDATE_RESULTI  = 0x02,   // 刷新算法运行结果
    ZHUZHAO_RESET           = 0x04,   // 重置界面参数
    ZHUZHAO_RUNONCE         = 0x08    // 单次运行
};

class ZZListener
{
public:
    ZZListener() = default;

    // 虚析构：保证「拿基类指针 delete 派生类对象」时派生类的析构会被调用
    virtual ~ZZListener() = default;

    // 纯虚函数 —— 本类因此是抽象类，不能实例化；派生类必须实现它。
    // 这个函数就是监听者唯一的对外接口。
    virtual void RespondMessage(int message) = 0;
};

class ListenerManger
{
    typedef QMap<int, QVector<ZZListener*>> mmap;   // 类型重定义，省得反复写这一长串
public:
    // 获取单例对象指针
    static ListenerManger* Instance();

    // 事件到来，通知所有登记过的监听者
    void notify(int message);

    // 登记：想收哪几条消息，就把它们用位或拼起来传进来
    void registerMessage(int message, ZZListener* listener);

    // 单例禁止拷贝与赋值 ——「唯一实例」有两个本身就自相矛盾
    ListenerManger(const ListenerManger&) = delete;
    ListenerManger& operator=(const ListenerManger&) = delete;

private:
    // 构造 / 析构私有：外部只能通过 Instance() 拿到那个唯一实例。
    // 饿汉式，实例活到进程结束，析构实际不会被调用。
    ListenerManger() = default;
    ~ListenerManger() = default;

    static ListenerManger* m_listenerManger;              // 唯一实例的指针
    QMap<int, QVector<ZZListener*>> m_messageToLister;    // 名册：消息 → 监听者列表
};

#endif // ZZ_LISTENER_H
