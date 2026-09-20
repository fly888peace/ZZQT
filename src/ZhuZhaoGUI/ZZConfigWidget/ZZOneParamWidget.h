#ifndef ZZONEPARAMWIDGET_H
#define ZZONEPARAMWIDGET_H

#include <QWidget>
#include <QImage>

class QLabel;
class QDoubleSpinBox;
class QPushButton;

// 「一组参数」控件：标题 + Slant / Tilt 两个角度输入框 + 一个加载图片按钮。
// ZZConfigWidget 里创建 4 个，分别对应 4 张不同光照方向的输入图。
//
// 加载完图片它【不直接】去刷新别人，只 notify 一条消息 ——
// 这就是观察者模式带来的解耦：本控件既不认识 MainWindow，也不认识缩略图列表。
class ZZOneParamWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ZZOneParamWidget(const QString& paramName, QWidget* parent = nullptr);

    QImage GetQImage() const;
    void SetSlantAngle(float fSlant);
    float GetSlantAngle() const;
    void SetTiltAngle(float fTilt);
    float GetTiltAngle() const;
    void Reset();

protected:
    bool InitWidget();

protected slots:
    void OnSigLoadImageBtnClicked(bool clicked);

private:
    QString m_strParamName;
    QLabel* m_pTitleLabel;
    QLabel* m_pSlantsLabel;
    QDoubleSpinBox* m_pSlantsSpin;
    QLabel* m_pTiltsLabel;
    QDoubleSpinBox* m_pTiltsSpin;
    QPushButton* m_pLoadImageBtn;
    QImage m_qImage;   // 已加载的图，等 GetPhotometricStereoParams() 来取
};

#endif // ZZONEPARAMWIDGET_H
