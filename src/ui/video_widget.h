#ifndef VIDEO_WIDGET_H
#define VIDEO_WIDGET_H

#include <QWidget>

/**
 * 视频显示控件
 * 用于嵌入 SDL 渲染器
 */
class VideoWidget : public QWidget {
    Q_OBJECT
public:
    explicit VideoWidget(QWidget* parent = nullptr);
    ~VideoWidget();

    // 获取窗口句柄（用于 SDL 渲染）
    void* getWindowId() const;

protected:
    void resizeEvent(QResizeEvent* event) override;
    void showEvent(QShowEvent* event) override;

signals:
    void sigSizeChanged(int width, int height);

private:
    bool m_nativeWindow = false;
};

#endif