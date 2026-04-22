#ifndef FURINA_LOTTIE_H
#define FURINA_LOTTIE_H

#include <QWidget>
#include <QPropertyAnimation>
#include <QParallelAnimationGroup>
#include <QTimer>
#include <QPixmap>
#include <QPainter>
#include <QMap>

// 图层结构
struct Layer {
    QPixmap pixmap;
    QString name;
    qreal offsetX = 0;
    qreal offsetY = 0;
    qreal rotation = 0;
    qreal scale = 1.0;
    qreal opacity = 1.0;
    int animType = 0;  // 0=静止 1=飘动 2=呼吸 3=眨眼
    qreal animParam = 0;
};

// 花瓣粒子
struct Petal {
    QPointF pos;
    qreal size;
    qreal rotation;
    qreal opacity;
    qreal speedY;
    qreal speedX;
    bool active;
};

class FurinaLottie : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(qreal opacity READ opacity WRITE setOpacity)
    Q_PROPERTY(qreal breath READ breath WRITE setBreath)
    Q_PROPERTY(qreal blink READ blink WRITE setBlink)

public:
    explicit FurinaLottie(QWidget* parent = nullptr);
    ~FurinaLottie();

    void showGoddess();
    void hideGoddess();
    void triggerSplash();

    qreal opacity() const { return m_opacity; }
    void setOpacity(qreal opacity);

    qreal breath() const { return m_breath; }
    void setBreath(qreal breath);

    qreal blink() const { return m_blink; }
    void setBlink(qreal blink);

protected:
    void paintEvent(QPaintEvent* event) override;
    void timerEvent(QTimerEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    void initLayers();
    void initPetals();
    void initAnimations();
    void updatePetals();
    void drawLayers(QPainter* painter);
    void drawPetals(QPainter* painter);
    void drawGlow(QPainter* painter);

    // 图层
    QList<Layer> m_layers;
    QMap<QString, int> m_layerIndex;

    // 花瓣
    QList<Petal> m_petals;
    int m_petalCount = 60;

    // 动画
    QParallelAnimationGroup* m_entranceAnim = nullptr;
    QPropertyAnimation* m_opacityAnim = nullptr;
    QPropertyAnimation* m_breathAnim = nullptr;
    QPropertyAnimation* m_blinkAnim = nullptr;

    // 参数
    qreal m_opacity = 0.0;
    qreal m_breath = 0.0;
    qreal m_blink = 0.0;
    bool m_visible = false;

    // 定时器
    int m_petalTimerId = 0;
};

#endif // FURINA_LOTTIE_H