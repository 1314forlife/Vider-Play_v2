#include "video_widget.h"
#include <QResizeEvent>
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