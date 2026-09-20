#ifndef IMAGECONVERT_H
#define IMAGECONVERT_H

#include <QImage>
#include <QDebug>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>

// QImage 与 cv::Mat 的双向转换。在 Qt 程序里用 OpenCV 时几乎必写的一段。
//
// 修（关键）：源工程把这两个函数定义在头文件里却没写 inline。
// 头文件里的普通函数定义，会在每一个包含它的 .cpp 里各生成一份，
// 只要出现第二个 .cpp 也 include 这个头，链接期就报 LNK2005「重复定义」。
// 现在没炸，只是因为恰好只有一个 .cpp 包含它。加 inline 从根上解决。
//
// 两个函数的实现直接写在头文件里（不是只留声明），是为了省掉一次函数调用，
// 也省掉一个 .cpp 文件。

// 把 OpenCV 的 cv::Mat 转成 QImage。
// @param mat     待转换的图像，支持 CV_8UC1 / CV_8UC3 / CV_8UC4
// @param clone   true 表示结果与 mat 不共享内存（改结果不会动原图）
// @param rb_swap 只对 CV_8UC3 有效：OpenCV 习惯 BGR 序、Qt 是 RGB 序，默认调换
inline QImage cvMat2QImage(const cv::Mat& mat, bool clone = true, bool rb_swap = true)
{
    if (mat.empty())
    {
        return QImage();
    }

    const uchar* pSrc = mat.data;
    const int nStep = static_cast<int>(mat.step);

    switch (mat.type())
    {
    case CV_8UC1:
    {
        const QImage image(pSrc, mat.cols, mat.rows, nStep, QImage::Format_Grayscale8);
        return clone ? image.copy() : image;
    }
    case CV_8UC3:
    {
        const QImage image(pSrc, mat.cols, mat.rows, nStep, QImage::Format_RGB888);
        if (rb_swap)
        {
            // rgbSwapped() 本身返回一个新对象，天然与 mat 脱离关系
            return image.rgbSwapped();
        }
        return clone ? image.copy() : image;
    }
    case CV_8UC4:
    {
        const QImage image(pSrc, mat.cols, mat.rows, nStep, QImage::Format_ARGB32);
        return clone ? image.copy() : image;
    }
    default:
        qWarning("cvMat2QImage: 不支持的 Mat 类型");
        return QImage();
    }
}

// 把 QImage 转成 cv::Mat。
// @param image   待转换的图像，支持 ARGB32 / RGB32 / ARGB32_Premultiplied / RGB888 / Indexed8 / Grayscale8
// @param clone   true 表示结果与 QImage 不共享内存。**强烈建议保持 true**：
//                cv::Mat 只包装指针不复制数据，一旦 image 析构，共享内存的 mat 立刻悬空。
// @param rb_swap 只对 RGB888 有效
inline cv::Mat QImage2cvMat(const QImage& image, bool clone = true, bool rb_swap = true)
{
    cv::Mat mat;

    // constBits() 返回 const 指针，cv::Mat 要非 const 的，只能 const_cast 掉。
    // 这只是打通类型，函数不会往这块内存里写（除非调用方自己改 mat）。
    uchar* pBits = const_cast<uchar*>(image.constBits());
    const size_t nStep = static_cast<size_t>(image.bytesPerLine());

    switch (image.format())
    {
    case QImage::Format_ARGB32:
    case QImage::Format_RGB32:
    case QImage::Format_ARGB32_Premultiplied:
        mat = cv::Mat(image.height(), image.width(), CV_8UC4, pBits, nStep);
        break;
    case QImage::Format_RGB888:
        mat = cv::Mat(image.height(), image.width(), CV_8UC3, pBits, nStep);
        if (rb_swap)
        {
            // Qt 存的是 RGB 序，OpenCV 习惯 BGR 序。
            // 注意 COLOR_RGB2BGR 与 COLOR_BGR2RGB 在这三个通道上是同一个交换动作，效果一样。
            cv::cvtColor(mat, mat, cv::COLOR_RGB2BGR);
        }
        break;
    case QImage::Format_Indexed8:
    case QImage::Format_Grayscale8:
        mat = cv::Mat(image.height(), image.width(), CV_8UC1, pBits, nStep);
        break;
    default:
        qWarning("QImage2cvMat: 不支持的 QImage 格式");
        break;
    }

    return clone ? mat.clone() : mat;
}

#endif // IMAGECONVERT_H
