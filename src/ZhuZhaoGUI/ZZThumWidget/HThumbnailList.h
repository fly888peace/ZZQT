#ifndef HTHUMBNAILLIST_H
#define HTHUMBNAILLIST_H

#include <QWidget>
#include <QList>
#include <QImage>
#include <QStringList>

class QListWidget;

// 横向排列的缩略图列表，用来显示算法输出的三张结果图。
// 每一格 = 一个 QListWidgetItem + 一个自绘的 LitImgItemWidget，
// 两者靠 QListWidget::setItemWidget() 绑在一起。
//
// 本类与 VThumbnailList 只差三处：flow 方向、滚动条策略、item 尺寸算法，
// 其余代码完全一致 —— 这是课程刻意做的对照，方便横向比较两个方向的差异。
class HThumbnailList : public QWidget
{
    Q_OBJECT

public:
    explicit HThumbnailList(QWidget* parent = nullptr);
    ~HThumbnailList() override;

    bool InitWidget();

    void addImage(const QString& strPath);        // 从文件加一张
    void addImage(const QImage& qImage);          // 直接加一张
    void addImages(const QList<QImage>& qImages); // 加一批

    void openFolder();          // 弹文件对话框批量选图
    void clearAllImages();

    void nextImage();           // 循环切到下一张
    void lastImage();           // 循环切到上一张
    QImage getNextImage();
    QImage getLastImage();
    QImage getCurrentImage();

signals:
    // 选中的图变了。用引用传出，同线程直连时省掉一次深拷贝。
    void SigSelectImageChanged(QImage& qImage);

protected slots:
    void OnCurrentRowChanged(int nCurRow);

private:
    QListWidget* m_pListWidget;
    QStringList m_strImagePaths;   // 从文件加载时的原始路径
    QList<QImage> m_listImage;     // 与列表行一一对应的图像
};

#endif // HTHUMBNAILLIST_H
