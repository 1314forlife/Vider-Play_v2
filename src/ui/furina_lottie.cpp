#include "furina_lottie.h"
#include <QVBoxLayout>
#include <QDebug>
#include <QPainterPath>
#include <QTimerEvent>
#include <random>
#include <cmath>

static std::random_device rd;
static std::mt19937 gen(rd());
static std::uniform_real_distribution<> dis(0, 1);

FurinaLottie::FurinaLottie(QWidget* parent) : QWidget(parent)
{
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_TransparentForMouseEvents, false);
    setMinimumSize(400, 400);

    initLayers();
    initPetals();
    initAnimations();

    m_petalTimerId = startTimer(33);
}

FurinaLottie::~FurinaLottie() {}

void FurinaLottie::initLayers()
{
    struct LayerDef {
        const char* path;
        const char* name;
        int animType;
    };

    LayerDef layers[] = {
        {":/src/resource/layers/background.png", "background", 1},
        {":/src/resource/layers/back_hair.png", "back_hair", 1},
        {":/src/resource/layers/neck.png", "neck", 0},
        {":/src/resource/layers/handwear.png", "handwear", 1},
        {":/src/resource/layers/legwear.png", "legwear", 1},
        {":/src/resource/layers/topwear.png", "topwear", 5},        // 左右摆动
        {":/src/resource/layers/face.png", "face", 0 },              // 完整脸部，静止
        {":/src/resource/layers/nose.png", "nose", 0},              // 如果有
        {":/src/resource/layers/mouth.png", "mouth", 3},            // 嘴巴，轻微移动
        {":/src/resource/layers/front_hair.png", "front_hair", 1}
    };

    int count = sizeof(layers) / sizeof(layers[0]);
    for (int i = 0; i < count; i++) {
        Layer layer;
        layer.pixmap = QPixmap(layers[i].path);
        layer.name = layers[i].name;
        layer.animType = layers[i].animType;

        if (!layer.pixmap.isNull()) {
            m_layers.append(layer);
            m_layerIndex[layer.name] = m_layers.size() - 1;
            qDebug() << "Loaded:" << layers[i].path;
        } else {
            qDebug() << "Failed to load:" << layers[i].path;
        }
    }
}

void FurinaLottie::initPetals()
{
    for (int i = 0; i < m_petalCount; i++) {
        Petal p;
        p.pos = QPointF(dis(gen) * width(), dis(gen) * height());
        p.size = 4 + dis(gen) * 8;
        p.rotation = dis(gen) * 360;
        p.opacity = 0.3 + dis(gen) * 0.6;
        p.speedY = 0.5 + dis(gen) * 1.5;
        p.speedX = (dis(gen) - 0.5) * 0.3;
        p.active = true;
        m_petals.append(p);
    }
}

void FurinaLottie::initAnimations()
{
    m_opacityAnim = new QPropertyAnimation(this, "opacity");
    m_opacityAnim->setDuration(800);
    m_opacityAnim->setEasingCurve(QEasingCurve::InOutCubic);

    m_breathAnim = new QPropertyAnimation(this, "breath");
    m_breathAnim->setDuration(3000);
    m_breathAnim->setStartValue(0.92);
    m_breathAnim->setEndValue(1.08);
    m_breathAnim->setLoopCount(-1);
    m_breathAnim->setEasingCurve(QEasingCurve::InOutSine);

    m_blinkAnim = new QPropertyAnimation(this, "blink");
    m_blinkAnim->setDuration(4000);
    m_blinkAnim->setStartValue(0);
    m_blinkAnim->setEndValue(0);
    m_blinkAnim->setKeyValueAt(0.5, 1);
    m_blinkAnim->setLoopCount(-1);

    m_entranceAnim = new QParallelAnimationGroup(this);
    m_entranceAnim->addAnimation(m_opacityAnim);

    connect(m_entranceAnim, &QParallelAnimationGroup::finished, [this]() {
        m_breathAnim->start();
        m_blinkAnim->start();
    });
}

void FurinaLottie::showGoddess()
{
    if (m_visible) return;
    m_visible = true;

    m_opacityAnim->setStartValue(0.0);
    m_opacityAnim->setEndValue(1.0);
    m_entranceAnim->start();
}

void FurinaLottie::hideGoddess()
{
    if (!m_visible) return;

    m_breathAnim->stop();
    m_blinkAnim->stop();

    m_opacityAnim->setStartValue(1.0);
    m_opacityAnim->setEndValue(0.0);
    m_opacityAnim->setDuration(500);
    m_opacityAnim->start();

    m_visible = false;
}

void FurinaLottie::setOpacity(qreal opacity)
{
    m_opacity = opacity;
    update();
}

void FurinaLottie::setBreath(qreal breath)
{
    m_breath = breath;
    update();
}

void FurinaLottie::setBlink(qreal blink)
{
    m_blink = blink;
    update();
}

void FurinaLottie::triggerSplash()
{
    for (int i = 0; i < 10; i++) {
        Petal p;
        p.pos = QPointF(width()/2 + (dis(gen)-0.5)*100,
                        height()/2 + (dis(gen)-0.5)*100);
        p.size = 3 + dis(gen) * 6;
        p.opacity = 0.8;
        p.speedY = -1 - dis(gen) * 2;
        p.speedX = (dis(gen) - 0.5) * 2;
        p.active = true;
        m_petals.append(p);
    }
}

void FurinaLottie::drawLayers(QPainter* painter)
{
    int centerX = width() / 2;
    int centerY = height() / 2;

    for (auto& layer : m_layers) {
        painter->save();

        if (layer.name == "eyewhit") {
            painter->translate(centerX, centerY);
            painter->setOpacity(m_opacity);
            painter->drawPixmap(-layer.pixmap.width()/2, -layer.pixmap.height()/2, layer.pixmap);
            painter->restore();
            continue;  // 跳过后续动画处理
        }

        switch (layer.animType) {
        case 1: // 头发：大幅度飘动
            if (layer.name == "back_hair") {
                layer.offsetX = sin(m_breath * M_PI * 1.2) * 12;
                layer.offsetY = cos(m_breath * M_PI * 1.8) * 8;
                layer.rotation = sin(m_breath * M_PI * 1.5) * 5;
            } else if (layer.name == "front_hair") {
                layer.offsetX = sin(m_breath * M_PI * 2.5) * 8;
                layer.offsetY = cos(m_breath * M_PI * 2.0) * 5;
                layer.rotation = sin(m_breath * M_PI * 2.0) * 3;
            } else {
                layer.offsetX = sin(m_breath * M_PI * 2) * 5;
                layer.offsetY = cos(m_breath * M_PI * 1.5) * 3;
            }
            break;

        case 5: // topwear：左右轻微摆动
            layer.offsetX = sin(m_breath * M_PI * 2.5) * 3;  // 左右摆动3像素
            layer.rotation = sin(m_breath * M_PI * 2.5) * 1; // 轻微倾斜
            break;

        case 2: // 呼吸缩放
            layer.scale = m_breath;
            break;

        case 3: // 表情
            layer.offsetY = sin(m_breath * M_PI * 4) * 1;
            break;

        case 4: // 眨眼
            if (m_blink > 0.5) {
                layer.scale = 0.01;
            } else {
                layer.scale = 1.0;
            }
            break;
        }

        painter->translate(centerX + layer.offsetX, centerY + layer.offsetY);
        painter->rotate(layer.rotation);
        painter->scale(layer.scale, layer.scale);
        painter->setOpacity(layer.opacity * m_opacity);
        painter->drawPixmap(-layer.pixmap.width()/2, -layer.pixmap.height()/2, layer.pixmap);

        painter->restore();
    }
}

void FurinaLottie::drawPetals(QPainter* painter)
{
    painter->setPen(Qt::NoPen);

    for (const auto& p : m_petals) {
        painter->save();
        painter->translate(p.pos);
        painter->rotate(p.rotation);

        QLinearGradient grad(-p.size/2, -p.size/2, p.size/2, p.size/2);
        grad.setColorAt(0, QColor(255, 192, 203, static_cast<int>(p.opacity * 180)));
        grad.setColorAt(1, QColor(255, 105, 180, static_cast<int>(p.opacity * 120)));
        painter->setBrush(grad);

        QPainterPath path;
        path.moveTo(0, -p.size);
        path.cubicTo(p.size * 0.5, -p.size * 0.3, p.size * 0.5, p.size * 0.3, 0, p.size);
        path.cubicTo(-p.size * 0.5, p.size * 0.3, -p.size * 0.5, -p.size * 0.3, 0, -p.size);
        painter->drawPath(path);

        painter->restore();
    }
}

void FurinaLottie::drawGlow(QPainter* painter)
{
    QRadialGradient grad(width()/2, height()/2, qMin(width(), height())/2);
    grad.setColorAt(0, QColor(255, 200, 220, static_cast<int>(40 * m_opacity)));
    grad.setColorAt(1, Qt::transparent);
    painter->setPen(Qt::NoPen);
    painter->setBrush(grad);
    painter->drawEllipse(width()/2, height()/2, qMin(width(), height())/2, qMin(width(), height())/2);
}

void FurinaLottie::updatePetals()
{
    for (auto& p : m_petals) {
        p.pos += QPointF(p.speedX, p.speedY);
        p.rotation += 2;
        p.opacity *= 0.995;

        if (p.pos.y() > height() + 50 || p.opacity < 0.05) {
            p.pos = QPointF(dis(gen) * width(), -20);
            p.opacity = 0.3 + dis(gen) * 0.6;
            p.speedY = 0.5 + dis(gen) * 1.5;
        }
        if (p.pos.x() < -50 || p.pos.x() > width() + 50) {
            p.pos = QPointF(dis(gen) * width(), -20);
        }
    }
    update();
}

void FurinaLottie::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    if (!m_visible && m_opacity < 0.01) return;

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    drawLayers(&painter);
    drawPetals(&painter);
    drawGlow(&painter);
}

void FurinaLottie::timerEvent(QTimerEvent* event)
{
    if (event->timerId() == m_petalTimerId) {
        updatePetals();
    }
    QWidget::timerEvent(event);
}

void FurinaLottie::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
}