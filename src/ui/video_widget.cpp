#include "video_widget.h"
#include <QResizeEvent>
#include <QMouseEvent>
#include <QDebug>

VideoWidget::VideoWidget(QWidget* parent) : QWidget(parent) {
    // 确保是原生窗口，这样才能获取有效的 winId
    setAttribute(Qt::WA_NativeWindow);
    setAttribute(Qt::WA_PaintOnScreen);

    // 设置背景为黑色
    setStyleSheet("background: black;");

    // 设置大小策略
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    m_nativeWindow = true;
}

VideoWidget::~VideoWidget() {
}

void* VideoWidget::getWindowId() const {
    // 返回原生窗口句柄
    return (void*)winId();
}

void VideoWidget::setFullscreen(bool fullscreen)
{
    if (m_fullscreen == fullscreen) return;
    m_fullscreen = fullscreen;
    // 只发射信号，让 MainWindow 处理实际的全屏操作
    emit fullscreenChanged(m_fullscreen);
}



bool VideoWidget::isFullscreen() const
{
    return m_fullscreen;
}

void VideoWidget::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);

    static QSize lastSize;
    if (event->size() == lastSize) return;
    lastSize = event->size();

    emit resized();
    emit sigSizeChanged(event->size().width(), event->size().height());
}


void VideoWidget::showEvent(QShowEvent* event) {
    QWidget::showEvent(event);

    // 确保窗口显示后发送一次大小
    emit sigSizeChanged(width(), height());
}

void VideoWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        setFullscreen(!m_fullscreen);
    }
    QWidget::mouseDoubleClickEvent(event);
}

