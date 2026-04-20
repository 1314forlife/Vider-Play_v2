#ifndef PROGRESS_BAR_H
#define PROGRESS_BAR_H

#include <QWidget>
#include <QSlider>
#include <QLabel>
#include <QTimer>

class ProgressBar : public QWidget {
    Q_OBJECT
public:
    explicit ProgressBar(QWidget* parent = nullptr);
    ~ProgressBar();

    void setDuration(int64_t duration);      // 设置总时长（微秒）
    void setCurrent(int64_t current);       // 设置当前进度（微秒）
    void setEnabled(bool enabled);
    void reset();

    int64_t duration() const { return m_duration; }
    int64_t current() const { return m_current; }

signals:
    void sigSeekRequested(int64_t position);  // 请求跳转到指定位置
    void sliderPressed();   // 🔥 添加
    void sliderReleased();  // 🔥 添加

private slots:
    void onSliderPressed();
    void onSliderReleased();
    void onSliderMoved(int value);
    void onUpdateTimeLabel();

private:
    void updateTimeLabel();
    QString formatTime(int64_t microseconds);

    QSlider* m_slider = nullptr;
    QLabel* m_timeLabel = nullptr;

    int64_t m_duration = 0;      // 总时长（微秒）
    int64_t m_current = 0;       // 当前进度（微秒）
    bool m_isSeeking = false;    // 是否正在拖动

    QTimer* m_updateTimer = nullptr;  // 用于平滑更新
};

#endif