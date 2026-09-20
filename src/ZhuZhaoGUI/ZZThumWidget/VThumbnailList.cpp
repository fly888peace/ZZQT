#include "VThumbnailList.h"
#include "LitImgItemWidget.h"

#include <QFileDialog>
#include <QListWidget>
#include <QListWidgetItem>
#include <QScrollBar>
#include <QVBoxLayout>
#include <new>

namespace
{
// 细窄滚动条的样式（纵向列表用竖向滚动条，所以定的是 width）
const char* const kScrollBarQss =
    "QScrollBar{background:transparent; width:5px; margin:0px 0px 0px 0px;}"
    "QScrollBar::handle{background:rgba(223, 223, 225, 200); border:0px; border-radius:5px; margin:0px 0px 0px 0px;}"
    "QScrollBar::handle:hover{background:lightgray;}"
    "QScrollBar::handle:pressed{background:rgba(200, 200, 200, 255);}"
    "QScrollBar::sub-page{background:transparent;}"
    "QScrollBar::add-page{background:transparent;}"
    "QScrollBar::up-arrow{background:transparent;}"
    "QScrollBar::down-arrow{background:transparent;}"
    "QScrollBar::sub-line{background:transparent; width:0px;}"
    "QScrollBar::add-line{background:transparent; width:0px;}";
}

VThumbnailList::VThumbnailList(QWidget* parent)
    : QWidget(parent)
    , m_pListWidget(nullptr)
{
    if (!InitWidget())
    {
        throw std::bad_alloc();
    }
}

VThumbnailList::~VThumbnailList() = default;

bool VThumbnailList::InitWidget()
{
    m_pListWidget = new QListWidget(this);
    m_pListWidget->horizontalScrollBar()->setStyleSheet(kScrollBarQss);
    m_pListWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

    // 纵向列表：竖向滚动条按需出现，横向一律关掉（与 H 版正好对调）
    m_pListWidget->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_pListWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    m_pListWidget->setFlow(QListView::TopToBottom);       // ← 与 HThumbnailList 的差别之一
    m_pListWidget->setViewMode(QListView::ListMode);
    m_pListWidget->setSelectionMode(QListView::SingleSelection);
    m_pListWidget->setFocusPolicy(Qt::NoFocus);
    m_pListWidget->setSelectionRectVisible(false);
    m_pListWidget->setVerticalScrollMode(QListView::ScrollPerPixel);
    m_pListWidget->setDragEnabled(false);
    m_pListWidget->setSpacing(4);

    QVBoxLayout* pListLayout = new QVBoxLayout();
    pListLayout->setSpacing(0);
    pListLayout->setContentsMargins(0, 0, 0, 0);
    pListLayout->addWidget(m_pListWidget);
    this->setLayout(pListLayout);

    connect(m_pListWidget, &QListWidget::currentRowChanged,
            this, &VThumbnailList::OnCurrentRowChanged);

    return true;
}

void VThumbnailList::addImage(const QString& strPath)
{
    addImage(QImage(strPath));
}

void VThumbnailList::addImage(const QImage& qImage)
{
    LitImgItemWidget* pImgItemWidget = new LitImgItemWidget();
    pImgItemWidget->setImage(qImage);

    QListWidgetItem* pItem = new QListWidgetItem();
    // 纵向列表里，格子的宽高都由列表宽度推（做成正方形）—— 与 H 版的算法不同
    pItem->setSizeHint(QSize(this->width() - 10, this->width() - 10));
    m_pListWidget->addItem(pItem);
    m_pListWidget->setItemWidget(pItem, pImgItemWidget);

    m_listImage.push_back(qImage);
}

void VThumbnailList::addImages(const QList<QImage>& qImages)
{
    for (const QImage& img : qImages)
    {
        addImage(img);
    }
}

void VThumbnailList::openFolder()
{
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

void VThumbnailList::clearAllImages()
{
    m_pListWidget->clear();
    m_strImagePaths.clear();
    m_listImage.clear();
}

void VThumbnailList::nextImage()
{
    int nCurrRow = m_pListWidget->currentRow();
    if (nCurrRow == m_listImage.size() - 1)
    {
        nCurrRow = -1;   // 最后一张，绕回开头
    }
    m_pListWidget->setCurrentRow(nCurrRow + 1);
}

void VThumbnailList::lastImage()
{
    int nCurrRow = m_pListWidget->currentRow();
    if (nCurrRow == 0)
    {
        nCurrRow = m_pListWidget->count();   // 第一张，绕到最后
    }
    m_pListWidget->setCurrentRow(nCurrRow - 1);
}

QImage VThumbnailList::getNextImage()
{
    nextImage();
    return getCurrentImage();
}

QImage VThumbnailList::getLastImage()
{
    lastImage();
    return getCurrentImage();
}

QImage VThumbnailList::getCurrentImage()
{
    const int nCurrRow = m_pListWidget->currentRow();
    if (nCurrRow < 0 || nCurrRow >= m_listImage.size())   // 修：source 写的是 > length()，差一
    {
        return QImage();
    }
    return m_listImage[nCurrRow];
}

void VThumbnailList::OnCurrentRowChanged(int nCurRow)
{
    if (nCurRow < 0 || nCurRow >= m_listImage.size())
    {
        return;
    }

    QImage curImage = m_listImage[nCurRow];
    emit SigSelectImageChanged(curImage);
}
