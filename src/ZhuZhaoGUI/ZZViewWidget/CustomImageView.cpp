#include "CustomImageView.h"
#include "CustomGraphicsView.h"

#include <QHBoxLayout>
#include <new>

CustomImageView::CustomImageView(QWidget* parent)
    : QWidget(parent)
    , m_pGraphicsview(nullptr)
{
    if (!InitWidget())
    {
        throw std::bad_alloc();
    }
}

CustomImageView::~CustomImageView() = default;

bool CustomImageView::InitWidget()
{
    m_pGraphicsview = new CustomGraphicsView(this);

    // 外壳只负责摆位，所有缩放/自适应逻辑都在 CustomGraphicsView 里
    QHBoxLayout* pViewLayout = new QHBoxLayout();
    pViewLayout->setSpacing(0);
    pViewLayout->setContentsMargins(0, 0, 0, 0);
    pViewLayout->addWidget(m_pGraphicsview);
    this->setLayout(pViewLayout);

    return true;
}

void CustomImageView::SetImage(QImage& qImage)
{
    m_qImage = qImage;
    m_pGraphicsview->SetImage(qImage);
}

void CustomImageView::SetImage(QString& strPath)
{
    const QImage img(strPath);
    if (!img.isNull())
    {
        m_qImage = img;
        m_pGraphicsview->SetImage(img);
    }
}

void CustomImageView::OnSendImage(QImage& qImage)
{
    SetImage(qImage);
}
