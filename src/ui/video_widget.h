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

    void setFullscreen(bool fullscreen);

    bool isFullscreen() const;

protected:
    void resizeEvent(QResizeEvent* event) override;
    void showEvent(QShowEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
signals:
    void sigSizeChanged(int width, int height);
    void fullscreenChanged(bool fullscreen);

private:
    bool m_nativeWindow = false;

    bool m_fullscreen = false;
    QRect m_normalGeometry;  // 保存正常状态的位置和大小
};

#endif