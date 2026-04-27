#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <QMainWindow>
#include <QPushButton>
#include <QTimer>
#include <QStackedWidget>
#include <memory>
#include <QSlider>
#include "src/engine/playback_state.h"
#include "src/network/network_stream_manager.h"
#include "src/ui/download_widget.h"

class PlayEngine;
class VideoWidget;
class ProgressBar;
class TitleBar;
class NavigationWidget;
class FurinaLottie;
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
    void onSliderPressed();
    void onSliderReleased();
    void onNetworkPlay();
    void onFullscreenChanged(bool fullscreen);
    void onVolumeChanged(int volume);

    // 页面切换
    void switchToPlayer();
    void switchToDownload();
    void switchToTheme();
    void switchToSettings();

    // 窗口控制
    void toggleMaximize();

    void onStreamReady(const QString& streamUrl);
    void onStreamError(const QString& err);
    void playDirect(const QString& url);

private:
    void setupUI();
    void setupConnections();
    void setupTitleBar();
    void setupNavigation();
    void setupStackedWidget();

    NetworkStreamManager* m_networkManager = nullptr;

    // 创建各个页面
    QWidget* createPlayerPage();
    QWidget* createDownloadPage();
    QWidget* createThemePage();
    QWidget* createSettingsPage();

    DownloadWidget* m_downloadWidget = nullptr;

    // UI 组件
    TitleBar* m_titleBar = nullptr;
    NavigationWidget* m_navigation = nullptr;
    QStackedWidget* m_centralStack = nullptr;

    // 播放器组件（保留原有）
    VideoWidget* m_videoWidget = nullptr;
    ProgressBar* m_progressBar = nullptr;
    QPushButton* m_playPauseBtn = nullptr;
    QPushButton* m_stopBtn = nullptr;
    QPushButton* m_openBtn = nullptr;
    QSlider* m_volumeSlider = nullptr;

    PlayEngine* m_engine = nullptr;
    bool m_isSeeking = false;

    QRect m_normalGeometry;  // 保存正常窗口的位置和大小

    FurinaLottie* m_furinaLottie = nullptr;
    QPushButton* m_wallpaperBtn = nullptr;
};

#endif