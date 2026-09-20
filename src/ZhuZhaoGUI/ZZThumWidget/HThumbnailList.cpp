#include "HThumbnailList.h"
#include "LitImgItemWidget.h"

#include <QFileDialog>
#include <QListWidget>
#include <QListWidgetItem>
#include <QScrollBar>
#include <QVBoxLayout>
#include <new>

namespace
{
// 细窄滚动条的样式（横向列表用横向滚动条，所以定的是 height）
const char* const kScrollBarQss =
    "QScrollBar{background:transparent; height:5px; margin:0px 0px 0px 0px;}"
    "QScrollBar::handle{background:rgba(223, 223, 225, 200); border:0px; border-radius:5px; margin:0px 0px 0px 0px;}"
    "QScrollBar::handle:hover{background:lightgray;}"
    "QScrollBar::handle:pressed{background:rgba(200, 200, 200, 255);}"
    "QScrollBar::sub-page{background:transparent;}"
    "QScrollBar::add-page{background:transparent;}"
    "QScrollBar::up-arrow{background:transparent;}"
    "QScrollBar::down-arrow{background:transparent;}"
    "QScrollBar::sub-line{background:transparent; height:0px;}"
    "QScrollBar::add-line{background:transparent; height:0px;}";
}

HThumbnailList::HThumbnailList(QWidget* parent)
    : QWidget(parent)
    , m_pListWidget(nullptr)
{
    if (!InitWidget())
    {
        throw std::bad_alloc();
    }
}

HThumbnailList::~HThumbnailList() = default;

bool HThumbnailList::InitWidget()
{
    m_pListWidget = new QListWidget(this);
    m_pListWidget->horizontalScrollBar()->setStyleSheet(kScrollBarQss);
    m_pListWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

    // 横向列表：横向滚动条按需出现，纵向一律关掉
    m_pListWidget->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_pListWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    m_pListWidget->setFlow(QListView::LeftToRight);       // ← 与 VThumbnailList 的差别之一
    m_pListWidget->setViewMode(QListView::ListMode);
    m_pListWidget->setSelectionMode(QListView::SingleSelection);
    m_pListWidget->setFocusPolicy(Qt::NoFocus);
    m_pListWidget->setSelectionRectVisible(false);
    m_pListWidget->setHorizontalScrollMode(QListView::ScrollPerPixel);
    m_pListWidget->setDragEnabled(false);
    m_pListWidget->setSpacing(4);

    QVBoxLayout* pListLayout = new QVBoxLayout();
    pListLayout->setSpacing(0);
    pListLayout->setContentsMargins(0, 0, 0, 0);
    pListLayout->addWidget(m_pListWidget);
    this->setLayout(pListLayout);

    connect(m_pListWidget, &QListWidget::currentRowChanged,
            this, &HThumbnailList::OnCurrentRowChanged);

    return true;
}

void HThumbnailList::addImage(const QString& strPath)
{
    addImage(QImage(strPath));
}

void HThumbnailList::addImage(const QImage& qImage)
{
    // 一张图配一个自绘控件，再用 setItemWidget 把它绑到列表项上
    LitImgItemWidget* pImgItemWidget = new LitImgItemWidget();
    pImgItemWidget->setImage(qImage);

    QListWidgetItem* pItem = new QListWidgetItem();
    // 横向列表里，格子的宽度用列表高度推（尽量做成正方形），高度比列表略矮一点留白
    pItem->setSizeHint(QSize(this->height() + 20, this->height() - 10));
    m_pListWidget->addItem(pItem);
    m_pListWidget->setItemWidget(pItem, pImgItemWidget);

    m_listImage.push_back(qImage);
}

void HThumbnailList::addImages(const QList<QImage>& qImages)
{
    for (const QImage& img : qImages)
    {
        addImage(img);
    }
}

void HThumbnailList::openFolder()
{
    // 修：源工程写的是 new QFileDialog(this) 且从不 delete，这里改成栈对象
    QFileDialog fileDialog(this);
    fileDialog.setWindowTitle(tr("OpenImageFolder"));
    fileDialog.setDirectory(".");
    fileDialog.setNameFilter(tr("Images(*.png *.jpg *.jpeg *.bmp)"));
    fileDialog.setFileMode(QFileDialog::ExistingFiles);
    fileDialog.setViewMode(QFileDialog::Detail);

    if (fileDialog.exec())
    {
        clearAllImages();
        m_strImagePaths = fileDialog.selectedFiles();
        for (const QString& strImagePath : m_strImagePaths)
        {
            addImage(strImagePath);
        }
    }
}

void HThumbnailList::clearAllImages()
{
    // clear() 会连带删掉所有 item 以及绑在它们身上的自绘控件
    m_pListWidget->clear();
    m_strImagePaths.clear();
    m_listImage.clear();
}

void HThumbnailList::nextImage()
{
    int nCurrRow = m_pListWidget->currentRow();
    if (nCurrRow == m_listImage.size() - 1)
    {
        nCurrRow = -1;   // 已经在最后一张，绕回开头（-1 + 1 = 0）
    }
    m_pListWidget->setCurrentRow(nCurrRow + 1);
}

void HThumbnailList::lastImage()
{
    int nCurrRow = m_pListWidget->currentRow();
    if (nCurrRow == 0)
    {
        nCurrRow = m_pListWidget->count();   // 已经在第一张，绕到最后（count - 1）
    }
    m_pListWidget->setCurrentRow(nCurrRow - 1);
}

QImage HThumbnailList::getNextImage()
{
    nextImage();
    return getCurrentImage();
}

QImage HThumbnailList::getLastImage()
{
    lastImage();
    return getCurrentImage();
}

QImage HThumbnailList::getCurrentImage()
{
    const int nCurrRow = m_pListWidget->currentRow();
    // 修：源工程写的是 nCurrRow > length()，边界差一，正好等于 size() 时会越界访问
    if (nCurrRow < 0 || nCurrRow >= m_listImage.size())
    {
        return QImage();
    }
    return m_listImage[nCurrRow];
}

void HThumbnailList::OnCurrentRowChanged(int nCurRow)
{
    // 同一个越界问题，这里也要 >=
    if (nCurRow < 0 || nCurRow >= m_listImage.size())
    {
        return;
    }

    QImage curImage = m_listImage[nCurRow];
    emit SigSelectImageChanged(curImage);
}
