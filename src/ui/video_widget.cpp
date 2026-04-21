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

    if (fullscreen) {
        // 保存当前几何信息
        m_normalGeometry = geometry();
        // 进入全屏
        setParent(nullptr);  // 暂时从父窗口脱离
        showFullScreen();
    } else {
        // 退出全屏
        setParent(parentWidget());
        setGeometry(m_normalGeometry);
        showNormal();
    }

    emit fullscreenChanged(m_fullscreen);  // ← 添加这一行
}



bool VideoWidget::isFullscreen() const
{
    return m_fullscreen;
}

void VideoWidget::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);

    // 通知大小改变
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