#ifndef DOWNLOAD_WIDGET_H
#define DOWNLOAD_WIDGET_H

#include <QWidget>
#include <QListWidget>
#include <QListWidgetItem>
#include <QProgressBar>
#include <QLabel>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QMenu>
#include <QAction>
#include "src/download/download_manager.h"
//#include "src/download/SimpleDownloader.h"
#include "src/download/download_manager_v2.h"


class DownloadWidget : public QWidget
{
    Q_OBJECT

public:
    explicit DownloadWidget(QWidget* parent = nullptr);
    ~DownloadWidget();



private slots:
    void onAddDownload();
    void onClearCompleted();
    void onPauseAll();
    void onResumeAll();
    void onTaskAdded(int taskId);
    void onTaskUpdated(int taskId);
    void onTaskRemoved(int taskId);

    // 右键菜单
    void showContextMenu(const QPoint& pos);
    void onPauseTask();
    void onResumeTask();
    void onRetryTask();
    void onRemoveTask();
    void onOpenFile();
    void onOpenFolder();
    void onTaskSpeedV2(int taskId, int speedKBps);

    void onTaskAddedV2(int taskId);
    void onTaskProgressV2(int taskId, int percent);
    void onTaskFinishedV2(int taskId, bool success, const QString& errorMsg);

private:
    struct TaskWidgets {
        int taskId = -1;
        QListWidgetItem* item = nullptr;
        QWidget* widget = nullptr;
        QLabel* nameLabel = nullptr;
        QLabel* infoLabel = nullptr;
        QProgressBar* progressBar = nullptr;
        QLabel* statusLabel = nullptr;
        QPushButton* controlBtn = nullptr;
        QPushButton* removeBtn = nullptr;
    };

    QMap<int, TaskWidgets*> m_taskWidgetsV2;
    DownloadManagerV2* m_downloadManager;

    void createTaskWidget(int taskId, const DownloadTask& task);
    void updateTaskWidget(int taskId, const DownloadTask& task);
    void removeTaskWidget(int taskId);
    void updateSummary();
    QString getStatusText(int status);
    QString getStatusColor(int status);

    QListWidget* m_taskList;
    QPushButton* m_addBtn;
    QPushButton* m_clearBtn;
    QPushButton* m_pauseAllBtn;
    QPushButton* m_resumeAllBtn;
    QLabel* m_summaryLabel;
    QMap<int, TaskWidgets*> m_taskWidgets;
    int m_currentTaskId = -1;  // 右键菜单选中的任务
};

#endif // DOWNLOAD_WIDGET_H