#include "MainWindow.h"

#include <QLayout>
#include <QSplitter>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QImage>
#include <new>

#include "ZZConfigWidget/ZZConfigWidget.h"
#include "ZZConfigWidget/ZZProcessThread.h"
#include "ZZLogWidget/ZZLogWidget.h"
#include "ZZThumWidget/HThumbnailList.h"
#include "ZZThumWidget/VThumbnailList.h"
#include "ZZViewWidget/CustomImageView.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ZZListener()
    , m_pConfigWidget(nullptr)
    , m_pLogWidget(nullptr)
    , m_pImageView(nullptr)
    , m_pHThumList(nullptr)
    , m_pVThumList(nullptr)
    , m_pRunProcess(nullptr)
{
    this->setMinimumSize(1000, 600);
    this->setWindowTitle(tr("ZhuzhaoGUI - 光度立体缺陷检测"));

    if (!InitWidget())
    {
        throw std::bad_alloc();
    }

    // 注册本窗口关心的三类消息
    ListenerManger::Instance()->registerMessage(
        MESSAGE::ZHUZHAO_UPDATE_SRCIMAGE |
        MESSAGE::ZHUZHAO_UPDATE_RESULTI |
        MESSAGE::ZHUZHAO_RUNONCE,
        this);

    // 首次刷新一次，保证界面与数据初态一致
    ListenerManger::Instance()->notify(MESSAGE::ZHUZHAO_UPDATE_SRCIMAGE);
}

MainWindow::~MainWindow() = default;

bool MainWindow::InitWidget()
{
    m_pConfigWidget = new ZZConfigWidget(this);
    m_pLogWidget = new ZZLogWidget(this);
    m_pImageView = new CustomImageView(this);

    m_pHThumList = new HThumbnailList(this);
    m_pHThumList->setFixedHeight(120);   // 结果图横向条：锁死高度
    m_pVThumList = new VThumbnailList(this);
    m_pVThumList->setFixedWidth(120);    // 输入图纵向栏：锁死宽度

    // 两个列表选中不同图，都送给同一个视觉窗口
    connect(m_pHThumList, &HThumbnailList::SigSelectImageChanged,
            m_pImageView, &CustomImageView::OnSendImage);
    connect(m_pVThumList, &VThumbnailList::SigSelectImageChanged,
            m_pImageView, &CustomImageView::OnSendImage);

    // 算法线程：全工程唯一一处跨对象信号槽
    m_pRunProcess = new ZZProcessThread(this);
    connect(m_pRunProcess, &ZZProcessThread::finished,
            this, &MainWindow::OnProcessThreadFinished);

    // ---- 三层嵌套布局 ----
    // 左列：配置面板 / 日志面板，纵向可拖动
    QSplitter* pLeftSplitter = new QSplitter(Qt::Vertical);
    pLeftSplitter->addWidget(m_pConfigWidget);
    pLeftSplitter->addWidget(m_pLogWidget);

    // 右侧内层：视觉窗口 + 结果缩略图条
    // 修：源工程这里写成 new QVBoxLayout(this)，等于先把布局装到主窗口上，
    //     后面再 setLayout 给别的控件会触发 Qt 的重复布局警告。布局都不带 parent。
    QVBoxLayout* pViewLayout = new QVBoxLayout();
    pViewLayout->setContentsMargins(0, 0, 0, 0);
    pViewLayout->addWidget(m_pImageView);
    pViewLayout->addWidget(m_pHThumList);
    pViewLayout->addStretch();           // 让多出来的高度不落到控件上

    // 右侧外层：输入图纵栏 + 内层
    QHBoxLayout* pRightLayout = new QHBoxLayout();
    pRightLayout->setContentsMargins(0, 0, 0, 0);
    pRightLayout->addWidget(m_pVThumList);
    pRightLayout->addLayout(pViewLayout);    // 布局装布局 = addLayout
    QWidget* pRightWidget = new QWidget(this);
    pRightWidget->setLayout(pRightLayout);   // 布局装控件 = setLayout

    // 左右分栏
    QSplitter* pMainSplitter = new QSplitter(Qt::Horizontal);
    pMainSplitter->addWidget(pLeftSplitter);
    pMainSplitter->addWidget(pRightWidget);

    QHBoxLayout* pMainLayout = new QHBoxLayout();
    pMainLayout->setContentsMargins(0, 0, 0, 0);
    pMainLayout->addWidget(pMainSplitter);
    QWidget* pCenterWidget = new QWidget(this);
    pCenterWidget->setLayout(pMainLayout);

    // QMainWindow 的地盘必须交给 centralWidget，不能直接往上 addWidget
    this->setCentralWidget(pCenterWidget);
    return true;
}

void MainWindow::OnProcessThreadFinished()
{
    // 算法跑完：取回三张结果图，刷新横向缩略图条
    QList<QImage> dstImages;
    m_pRunProcess->GetResultImages(dstImages);
    m_pHThumList->clearAllImages();
    m_pHThumList->addImages(dstImages);
}

void MainWindow::RespondMessage(int message)
{
    if (message & MESSAGE::ZHUZHAO_UPDATE_SRCIMAGE)
    {
        // 输入图变了：把 4 组参数里的图取出来刷纵向列表
        QList<QImage> srcImages;
        QList<float> slants;
        QList<float> tilts;
        m_pConfigWidget->GetPhotometricStereoParams(srcImages, slants, tilts);
        m_pVThumList->clearAllImages();
        m_pVThumList->addImages(srcImages);
    }
    if (message & MESSAGE::ZHUZHAO_UPDATE_RESULTI)
    {
        // 结果图单独刷新用（当前结果由 finished 信号直接处理）
    }
    if (message & MESSAGE::ZHUZHAO_RUNONCE)
    {
        // 组参数 → 交给子线程 → start()
        QList<QImage> srcImages;
        QList<float> slants;
        QList<float> tilts;
        m_pConfigWidget->GetPhotometricStereoParams(srcImages, slants, tilts);
        m_pRunProcess->SetPhotometricStereoParams(srcImages, slants, tilts);
        m_pRunProcess->start();
    }
}
