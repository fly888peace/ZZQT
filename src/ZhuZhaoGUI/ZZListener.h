#ifndef ZZ_LISTENER_H
#define ZZ_LISTENER_H

#include <QMap>
#include <QVector>   // Qt 6 里 QVector 只是 QList 的别名，保留这个拼写是为了跟课程源码逐行对照
#include <QtGlobal>  // Q_ASSERT

/**************************************************
 * 观察者模式在本项目里的两个角色：
 *     ZZListener      —— 监听者（抽象基类）。谁想收消息，谁就继承它
 *     ListenerManger  —— 事件总管（单例）。负责「登记」和「广播」
 *
 * 一条消息的完整生命周期：
 *   ① 登记：监听者在构造函数里调 registerMessage(想收的消息, this)，把自己记到总管的名册上
 *   ② 广播：任何地方调 notify(某条消息)
 *            → 总管从名册里查出登记过这条消息的人
 *            → 挨个调它们的 RespondMessage(消息)
 *
 * 本项目一共 4 条消息、1 个监听者（MainWindow），是该模式的「最小可用规模」。
 * 它的价值在于：ZZConfigWidget / ZZOneParamWidget 这些子控件不必持有 MainWindow 的指针，
 * 喊一嗓子 notify 就走通了，省掉一大片互相牵连的信号槽。
 **************************************************/

// 消息表
// 每一条都必须是不重复的 2 的幂：这样调用者才能用「位或」一次登记好几条，
// registerMessage 也才能用「位与」把混在一起的消息拆回单条。
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

    // 虚析构：保证「拿基类指针 delete 派生类对象」时，派生类自己的析构函数会被调用
    virtual ~ZZListener() = default;

    // 纯虚函数 —— 本类因此是「抽象类」，不能实例化（ZZListener obj; 或 new ZZListener 都不行）。
    // 继承者必须实现它，这个函数就是监听者唯一的对外接口。
    virtual void RespondMessage(int message) = 0;
};

class ListenerManger
{
    typedef QMap<int, QVector<ZZListener*>> mmap;   // 类型重定义，省得下面反复写这一长串
public:
    // 获取单例对象的指针
    static ListenerManger* Instance();

    // 事件到来，通知所有登记过的监听者
    void notify(int message);

    // 登记：listener 想收哪几条消息，就把它们用位或拼起来传进来
    void registerMessage(int message, ZZListener* listener);

    // 单例禁止拷贝构造与赋值 ——「唯一实例」有两个本身就是自相矛盾的
    ListenerManger(const ListenerManger&) = delete;
    ListenerManger& operator=(const ListenerManger&) = delete;

private:
    // 构造 / 析构设为 private：外部只能通过 Instance() 拿到那个唯一实例。
    // 析构不做 delete 处理（饿汉式，进程结束时由操作系统回收），所以这里也没定义成虚函数。
    ListenerManger() = default;
    ~ListenerManger() = default;

    static ListenerManger* m_listenerManger;              // 唯一实例的指针
    QMap<int, QVector<ZZListener*>> m_messageToLister;    // 名册：消息 → 监听者列表
};

#endif // ZZ_LISTENER_H
