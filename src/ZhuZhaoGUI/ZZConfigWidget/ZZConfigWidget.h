#ifndef ZZCONFIGWIDGET_H
#define ZZCONFIGWIDGET_H

#include <QWidget>
#include <QList>
#include <QImage>

class QLabel;
class QPushButton;
class ZZOneParamWidget;

// 参数配置面板：4 组输入图参数 + Reset / RunOnce 两个按钮。
//
// GetPhotometricStereoParams() 是本控件对外的唯一出口 ——
// 数据只从这一扇门取出，外面拿不到内部控件。
class ZZConfigWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ZZConfigWidget(QWidget* parent = nullptr);

    // 取出 4 组参数：图 + Slant + Tilt
    void GetPhotometricStereoParams(QList<QImage>& srcImages,
                                    QList<float>& Slants,
                                    QList<float>& Tilts);

protected:
    bool InitWidget();

protected slots:
    void OnResetBtnClicked(bool clicked);
    void OnRunOnceBtnClicked(bool clicked);

private:
    QLabel* m_pTitleLabel;
    QList<ZZOneParamWidget*> m_ParamWidgetList;   // 4 个参数控件，顺序即图序
    QPushButton* m_pResetBtn;
    QPushButton* m_pRunOnceBtn;
};

#endif // ZZCONFIGWIDGET_H
