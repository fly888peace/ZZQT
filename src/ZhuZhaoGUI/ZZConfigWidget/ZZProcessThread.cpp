#include "ZZProcessThread.h"
#include "ImageConvert.h"

#include "../ZZLogWidget/ZZLogMessage.h"
#include "PhotometricStereo.h"

#include <QMutexLocker>
#include <QtGlobal>
#include <vector>

ZZProcessThread::ZZProcessThread(QObject* parent)
    : QThread(parent)
{
}

void ZZProcessThread::SetPhotometricStereoParams(const QList<QImage>& srcImages,
                                                 const QList<float>& Slants,
                                                 const QList<float>& Tilts)
{
    QMutexLocker locker(&m_mutex);

    m_srcImages = srcImages;
    m_Slants = Slants;
    m_Tilts = Tilts;
}

void ZZProcessThread::GetResultImages(QList<QImage>& dstImages)
{
    QMutexLocker locker(&m_mutex);
    dstImages = m_dstImages;
}

void ZZProcessThread::run()
{
    // 本函数跑在【子线程】里。QThread::start() 会新起一条线程来执行它，
    // 主线程的界面刷新不受影响。
    // 也正因为如此，下面打的日志是从子线程发出来的 ——
    // ZZLogWidget 那边靠 Qt::AutoConnection 自动转成排队投递，界面照样安全。

    // ---------- ① 把参数拷到本地 ----------
    // 先拷再算：既缩短持锁时间，也保证算法运行期间参数不会被主线程改掉
    QList<QImage> srcImagesQt;
    QList<float> slantsQt;
    QList<float> tiltsQt;
    {
        QMutexLocker locker(&m_mutex);
        srcImagesQt = m_srcImages;
        slantsQt = m_Slants;
        tiltsQt = m_Tilts;
    }

    // 三个列表理论上等长（都来自 ZZConfigWidget::GetPhotometricStereoParams），
    // 取最小值兜个底，避免下标越界
    const int count = qMin(srcImagesQt.size(), qMin(slantsQt.size(), tiltsQt.size()));
    if (count < 3)
    {
        // 光度立体至少要 3 个不同光照方向才能解出法向量
        QWARNING(QString("输入图不足 3 张（当前 %1 张），跳过本次计算").arg(count));
        return;
    }

    // ---------- ② QImage → cv::Mat，整理成算法库接受的格式 ----------
    std::vector<cv::Mat> srcImages;
    std::vector<float> srcSlants;
    std::vector<float> srcTilts;
    for (int i = 0; i < count; ++i)
    {
        // 算法只吃单通道灰度图，先统一转格式再喂进去
        const QImage grayImage = srcImagesQt.at(i).convertToFormat(QImage::Format_Grayscale8);
        srcImages.push_back(QImage2cvMat(grayImage));
        srcSlants.push_back(slantsQt.at(i));
        srcTilts.push_back(tiltsQt.at(i));
    }

    // ---------- ③ 调用算法动态库 ----------
    cv::Mat dstHeightFieldImg;
    cv::Mat dstGradientImg;
    cv::Mat dstAlbedoImg;
    ZhuZhao::PhotometricStereo(srcImages,
                               dstHeightFieldImg,
                               dstGradientImg,
                               dstAlbedoImg,
                               count,
                               srcSlants,
                               srcTilts);

    // ---------- ④ cv::Mat → QImage，写回结果 ----------
    QList<QImage> dstImages;
    dstImages.push_back(cvMat2QImage(dstHeightFieldImg));
    dstImages.push_back(cvMat2QImage(dstGradientImg));
    dstImages.push_back(cvMat2QImage(dstAlbedoImg));

    QMutexLocker locker(&m_mutex);
    m_dstImages = dstImages;

    // 函数返回后 QThread 会自动 emit finished，主线程的槽随即被调用
}
