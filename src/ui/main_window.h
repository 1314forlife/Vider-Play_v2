#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <QMainWindow>
#include <QPushButton>
#include <QTimer>
#include <memory>
#include "src/engine/playback_state.h"

class PlayEngine;
class VideoWidget;
class ProgressBar;
enum class PlaybackState;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

protected:
    void resizeEvent(QResizeEvent* event) override;
    void closeEvent(QCloseEvent* event) override;

private slots:
    void onOpenFile();
    void onPlayPause();
    void onStop();
    void onProgressChanged(int64_t current, int64_t total);
    void onSeekRequested(int64_t position);
    void onEngineStateChanged(PlaybackState state);
    void onEngineError(const QString& error);
    void onSliderPressed();   // 🔥 添加
    void onSliderReleased();  // 🔥 添加
    void onNetworkPlay();

private:
    void setupUI();
    void setupConnections();
    void updateButtonState();

    VideoWidget* m_videoWidget = nullptr;
    QPushButton* m_openBtn = nullptr;
    ProgressBar* m_progressBar = nullptr;
    QPushButton* m_playPauseBtn = nullptr;
    QPushButton* m_stopBtn = nullptr;

    PlayEngine* m_engine = nullptr;
    bool m_isSeeking = false;  // 🔥 添加
};

#endif