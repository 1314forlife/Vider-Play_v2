#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QWidget>
#include <QPushButton>
#include <QFileDialog>
#include "src/core/videoplayer.h"
#include "src/core/sdlrender.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private:
    void onOpenFileClicked(); // 打开文件按键
    void onPlayClicked(); //播放停止按钮
    void onStopClicked(); //停止按钮
    void onPlayStateChanged(bool isPlaying); // 播放状态变化
    void onPlayFinished();     // 播放完成

protected:
    void showEvent(QShowEvent *event) override;

private:
    // UI控件
    QWidget* m_videoWidget = nullptr;    // 视频渲染容器
    QPushButton* m_btnOpen = nullptr;    // 打开文件
    QPushButton* m_btnPlay = nullptr;    // 播放/暂停
    QPushButton* m_btnStop = nullptr;    // 停止

    // 核心模块
    VideoPlayer* m_player = nullptr;     // 播放器
    SDLRender* m_sdlRender = nullptr;    // SDL渲染器

    // 状态变量
    QString m_currentFilePath;
};



#endif // MAINWINDOW_H
