#include "progress_bar.h"
#include <QHBoxLayout>
#include "src/common/logger.h"
#include <QStyle>

ProgressBar::ProgressBar(QWidget* parent) : QWidget(parent) {
    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);

    // 进度条
    m_slider = new QSlider(Qt::Horizontal, this);
    m_slider->setRange(0, 100);
    m_slider->setValue(0);
    m_slider->setEnabled(false);

    // 样式
    m_slider->setStyleSheet(
        "QSlider::groove:horizontal {"
        "  height: 4px;"
        "  background: #3a3a3a;"
        "  border-radius: 2px;"
        "}"
        "QSlider::handle:horizontal {"
        "  background: #5a5a5a;"
        "  width: 12px;"
        "  height: 12px;"
        "  margin: -4px 0;"
        "  border-radius: 6px;"
        "}"
        "QSlider::handle:horizontal:hover {"
        "  background: #7a7a7a;"
        "}"
        "QSlider::sub-page:horizontal {"
        "  background: #4a90d9;"
        "  border-radius: 2px;"
        "}"
        );

    // 时间标签
    m_timeLabel = new QLabel("00:00 / 00:00", this);
    m_timeLabel->setFixedWidth(110);
    m_timeLabel->setAlignment(Qt::AlignRight);
    m_timeLabel->setStyleSheet(
        "color: #ccc;"
        "font-size: 12px;"
        "font-family: monospace;"
        );

    layout->addWidget(m_slider, 1);
    layout->addWidget(m_timeLabel);

    // 连接信号
    connect(m_slider, &QSlider::sliderPressed, this, &ProgressBar::onSliderPressed);
    connect(m_slider, &QSlider::sliderReleased, this, &ProgressBar::onSliderReleased);
    connect(m_slider, &QSlider::sliderMoved, this, &ProgressBar::onSliderMoved);

    // 更新定时器（用于平滑更新显示）
    m_updateTimer = new QTimer(this);
    m_updateTimer->setInterval(500);
    connect(m_updateTimer, &QTimer::timeout, this, &ProgressBar::onUpdateTimeLabel);
}

ProgressBar::~ProgressBar() {
    if (m_updateTimer) {
        m_updateTimer->stop();
    }
}

void ProgressBar::setDuration(int64_t duration) {
    m_duration = duration;
    updateTimeLabel();
    m_slider->setEnabled(duration > 0);
}

void ProgressBar::setCurrent(int64_t current) {
    LOG_DEBUG("ProgressBar", QString("setCurrent: current=%1, duration=%2, isSeeking=%3")
                  .arg(current).arg(m_duration).arg(m_isSeeking));
    if (m_isSeeking) return;

    m_current = current;

    if (m_duration > 0) {
        int percent = (current * 100) / m_duration;
        m_slider->setValue(percent);
    }


    // 🔥 直接更新时间文本，不用定时器
    updateTimeLabel();
    // // 启动延迟更新，避免频繁刷新
    // m_updateTimer->start();
}

void ProgressBar::reset() {
    m_isSeeking = false;
    m_current = 0;
    m_slider->setValue(0);
    m_slider->setEnabled(false);
    updateTimeLabel();
}

void ProgressBar::setEnabled(bool enabled) {
    m_slider->setEnabled(enabled && m_duration > 0);
}

void ProgressBar::onSliderPressed() {
    m_isSeeking = true;
}

void ProgressBar::onSliderReleased() {
    if (m_duration > 0) {
        int percent = m_slider->value();
        int64_t seekPos = m_duration * percent / 100;
        LOG_INFO("ProgressBar", QString("释放滑块，请求 Seek: %1 ms").arg(seekPos / 1000));
        emit sigSeekRequested(seekPos);
    }
    m_isSeeking = false;
}

void ProgressBar::onSliderMoved(int value) {
    if (m_duration > 0) {
        int64_t previewPos = m_duration * value / 100;
        QString previewTime = formatTime(previewPos);
        QString totalTime = formatTime(m_duration);
        m_timeLabel->setText(previewTime + " / " + totalTime);
    }
}

void ProgressBar::onUpdateTimeLabel() {
    m_updateTimer->stop();
    updateTimeLabel();
}

void ProgressBar::updateTimeLabel() {
    QString currentStr = formatTime(m_current);
    QString totalStr = formatTime(m_duration);
    LOG_DEBUG("ProgressBar", QString("updateTimeLabel: %1 / %2, current=%3, duration=%4")
                                 .arg(currentStr).arg(totalStr).arg(m_current).arg(m_duration));
    m_timeLabel->setText(currentStr + " / " + totalStr);
}

QString ProgressBar::formatTime(int64_t microseconds) {
    int64_t seconds = microseconds / 1000000;
    int hours = seconds / 3600;
    int minutes = (seconds % 3600) / 60;
    int secs = seconds % 60;

    if (hours > 0) {
        return QString("%1:%2:%3")
        .arg(hours, 2, 10, QChar('0'))
            .arg(minutes, 2, 10, QChar('0'))
            .arg(secs, 2, 10, QChar('0'));
    } else {
        return QString("%1:%2")
        .arg(minutes, 2, 10, QChar('0'))
            .arg(secs, 2, 10, QChar('0'));
    }
}