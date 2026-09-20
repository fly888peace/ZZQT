#ifndef ZZ_PROCESSTHREAD_H
#define ZZ_PROCESSTHREAD_H

#include <QThread>
#include <QList>
#include <QImage>
#include <QMutex>

// 算法执行线程 —— 全工程唯一使用 QThread 的地方。
//
// 用法（见 MainWindow::RespondMessage / OnProcessThreadFinished）：
//   主线程 SetPhotometricStereoParams() 塞参数 → start() → run() 在子线程跑算法
//   → 跑完 QThread 自己 emit finished → 主线程取结果刷界面
//
// 两处与源工程不同：
//   ① 参数与结果会被主线程、子线程同时碰，所以下面全部读写都过同一把锁
//   ② 补上了 Q_OBJECT。源工程缺它 —— 当时只用到继承来的 finished 信号才编得过，
//      一旦想新增自己的信号就会链接失败
class ZZProcessThread : public QThread
{
    Q_OBJECT

public:
    explicit ZZProcessThread(QObject* parent = nullptr);

    void SetPhotometricStereoParams(const QList<QImage>& srcImages,
                                    const QList<float>& Slants,
                                    const QList<float>& Tilts);

    void GetResultImages(QList<QImage>& dstImages);

protected:
    void run() override;

private:
    // 修：源工程有个 volatile bool m_bIsStop 成员，定义了却从头到尾没被读过，
    //     这里直接删掉。（volatile 也挡不住多线程竞争，真要做可中止的任务
    //     得用 std::atomic 或者 QAtomicInt。）
    QMutex m_mutex;             // 保护下面两组数据

    QList<QImage> m_srcImages;  // 输入：4 张灰度图
    QList<float> m_Slants;
    QList<float> m_Tilts;
    QList<QImage> m_dstImages;  // 输出：高度图 / 梯度图 / 反照率图
};

#endif // ZZ_PROCESSTHREAD_H
