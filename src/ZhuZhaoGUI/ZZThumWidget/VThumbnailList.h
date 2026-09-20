#ifndef VTHUMBNAILLIST_H
#define VTHUMBNAILLIST_H

#include <QWidget>
#include <QList>
#include <QImage>
#include <QStringList>

class QListWidget;

// 纵向排列的缩略图列表，用来显示 4 张输入图。
//
// 与 HThumbnailList 的差异只有三处，其余完全一致：
//   ① setFlow(TopToBottom)       —— 从上往下排
//   ② 滚动条策略对调             —— 竖向按需，横向关掉
//   ③ setSizeHint 的算法不同      —— 宽度用列表宽度推，高度取同一个值做成正方形
class VThumbnailList : public QWidget
{
    Q_OBJECT

public:
    explicit VThumbnailList(QWidget* parent = nullptr);
    ~VThumbnailList() override;

    bool InitWidget();

    void addImage(const QString& strPath);
    void addImage(const QImage& qImage);
    void addImages(const QList<QImage>& qImages);

    void openFolder();
    void clearAllImages();

    void nextImage();
    void lastImage();
    QImage getNextImage();
    QImage getLastImage();
    QImage getCurrentImage();

signals:
    void SigSelectImageChanged(QImage& qImage);

protected slots:
    void OnCurrentRowChanged(int nCurRow);

private:
    QListWidget* m_pListWidget;
    QStringList m_strImagePaths;
    QList<QImage> m_listImage;
};

#endif // VTHUMBNAILLIST_H
