#include "download_widget.h"
#include <QInputDialog>
#include <QMessageBox>
#include <QFileDialog>
#include <QDesktopServices>
#include <QUrl>
#include <QFileInfo>
#include <QDateTime>
#include <QProcess>
#include <QMenu>
#include <QStandardPaths>
#include "src/common/logger.h"


DownloadWidget::DownloadWidget(QWidget* parent)
    : QWidget(parent)
{
    LOG_INFO("DownloadWidget", "Creating download widget...");

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(10);
    mainLayout->setContentsMargins(10, 10, 10, 10);

    // ========== 顶部工具栏 ==========
    QHBoxLayout* toolLayout = new QHBoxLayout();
    toolLayout->setSpacing(10);

    m_addBtn = new QPushButton("➕ 新建下载", this);
    m_addBtn->setObjectName("downloadAddBtn");
    toolLayout->addWidget(m_addBtn);

    m_pauseAllBtn = new QPushButton("⏸ 全部暂停", this);
    m_pauseAllBtn->setObjectName("downloadCtrlBtn");
    toolLayout->addWidget(m_pauseAllBtn);

    m_resumeAllBtn = new QPushButton("▶ 全部开始", this);
    m_resumeAllBtn->setObjectName("downloadCtrlBtn");
    toolLayout->addWidget(m_resumeAllBtn);

    m_clearBtn = new QPushButton("🗑 清理完成", this);
    m_clearBtn->setObjectName("downloadCtrlBtn");
    toolLayout->addWidget(m_clearBtn);

    toolLayout->addStretch();

    m_summaryLabel = new QLabel(this);
    m_summaryLabel->setObjectName("summaryLabel");
    toolLayout->addWidget(m_summaryLabel);

    mainLayout->addLayout(toolLayout);

    // ========== 下载任务列表 ==========
    m_taskList = new QListWidget(this);
    m_taskList->setObjectName("downloadList");
    m_taskList->setSelectionMode(QAbstractItemView::NoSelection);
    m_taskList->setSpacing(5);
    m_taskList->setContextMenuPolicy(Qt::CustomContextMenu);
    mainLayout->addWidget(m_taskList);

    // ========== 连接信号 ==========
    connect(m_addBtn, &QPushButton::clicked, this, &DownloadWidget::onAddDownload);
    // connect(m_clearBtn, &QPushButton::clicked, this, &DownloadWidget::onClearCompleted);
    // connect(m_pauseAllBtn, &QPushButton::clicked, this, &DownloadWidget::onPauseAll);
    // connect(m_resumeAllBtn, &QPushButton::clicked, this, &DownloadWidget::onResumeAll);
    connect(m_taskList, &QListWidget::customContextMenuRequested,
            this, &DownloadWidget::showContextMenu);

    // 连接下载管理器信号
    DownloadManager& dm = DownloadManager::instance();
    connect(&dm, &DownloadManager::taskAdded, this, &DownloadWidget::onTaskAdded);
    connect(&dm, &DownloadManager::taskUpdated, this, &DownloadWidget::onTaskUpdated);
    connect(&dm, &DownloadManager::taskRemoved, this, &DownloadWidget::onTaskRemoved);

    // 加载已有任务
    int loadedCount = 0;
    for (const auto& task : dm.getTasks()) {
        onTaskAdded(task.id);
        loadedCount++;
    }

    LOG_INFO("DownloadWidget", "Download widget created, loaded " + QString::number(loadedCount) + " tasks");
    updateSummary();

    m_downloadManager = new DownloadManagerV2(this);

    connect(m_downloadManager, &DownloadManagerV2::taskAdded,
            this, &DownloadWidget::onTaskAddedV2);
    connect(m_downloadManager, &DownloadManagerV2::taskProgress,
            this, &DownloadWidget::onTaskProgressV2);
    connect(m_downloadManager, &DownloadManagerV2::taskSpeed,
            this, &DownloadWidget::onTaskSpeedV2);
    connect(m_downloadManager, &DownloadManagerV2::taskFinished,
            this, &DownloadWidget::onTaskFinishedV2);

}

DownloadWidget::~DownloadWidget()
{
    LOG_INFO("DownloadWidget", "Download widget destroyed");

    // 断开所有信号
    DownloadManager& dm = DownloadManager::instance();
    disconnect(&dm, nullptr, this, nullptr);

    // 释放所有任务控件
    for (auto it = m_taskWidgets.begin(); it != m_taskWidgets.end(); ++it) {
        if (it.value()) {
            if (it.value()->item) delete it.value()->item;
            if (it.value()->widget) it.value()->widget->deleteLater();
            delete it.value();
        }
    }
    m_taskWidgets.clear();
}

void DownloadWidget::onAddDownload()
{
    bool ok;
    QString url = QInputDialog::getText(this, "新建下载",
                                        "请输入下载链接:", QLineEdit::Normal, "", &ok);
    if (!ok || url.isEmpty()) return;

    QString savePath = QFileDialog::getSaveFileName(this, "保存文件",
                                                    QStandardPaths::writableLocation(QStandardPaths::DownloadLocation) + "/test.mp4");

    if (savePath.isEmpty()) return;

    // 直接使用成员变量 m_downloadManager（已经在构造函数中创建）
    m_downloadManager->addDownload(url, savePath);

    qDebug() << "Download added via V2";
}

void DownloadWidget::onPauseAll()
{
    LOG_INFO("DownloadWidget", "Pausing all tasks");

    DownloadManager& dm = DownloadManager::instance();
    int paused = 0;

    for (const auto& task : dm.getTasks()) {
        if (task.status == 1) {  // 下载中
            dm.pauseTask(task.id);
            paused++;
        }
    }

    LOG_DEBUG("DownloadWidget", "Paused " + QString::number(paused) + " tasks");
}

void DownloadWidget::onResumeAll()
{
    LOG_INFO("DownloadWidget", "Resuming all tasks");

    DownloadManager& dm = DownloadManager::instance();
    int resumed = 0;

    for (const auto& task : dm.getTasks()) {
        if (task.status == 2) {  // 暂停
            dm.resumeTask(task.id);
            resumed++;
        }
    }

    LOG_DEBUG("DownloadWidget", "Resumed " + QString::number(resumed) + " tasks");
}

void DownloadWidget::onTaskAdded(int taskId)
{
    LOG_DEBUG("DownloadWidget", "Task added: " + QString::number(taskId));

    DownloadManager& dm = DownloadManager::instance();
    const DownloadTask* task = nullptr;
    for (const auto& t : dm.getTasks()) {
        if (t.id == taskId) {
            task = &t;
            break;
        }
    }

    if (!task) {
        LOG_WARN("DownloadWidget", "Task not found: " + QString::number(taskId));
        return;
    }

    createTaskWidget(taskId, *task);
    updateSummary();
}

void DownloadWidget::onTaskUpdated(int taskId)
{
    LOG_DEBUG("DownloadWidget", "Task updated: " + QString::number(taskId));

    DownloadManager& dm = DownloadManager::instance();
    const DownloadTask* task = nullptr;
    for (const auto& t : dm.getTasks()) {
        if (t.id == taskId) {
            task = &t;
            break;
        }
    }

    if (!task) {
        LOG_WARN("DownloadWidget", "Task not found for update: " + QString::number(taskId));
        return;
    }

    updateTaskWidget(taskId, *task);
    updateSummary();
}

void DownloadWidget::onTaskRemoved(int taskId)
{
    LOG_DEBUG("DownloadWidget", "Task removed: " + QString::number(taskId));
    removeTaskWidget(taskId);
    updateSummary();
}

void DownloadWidget::createTaskWidget(int taskId, const DownloadTask& task)
{
    if (taskId <= 0) {
        LOG_ERROR("DownloadWidget", "createTaskWidget: invalid taskId");
        return;
    }

    if (m_taskWidgets.contains(taskId)) {
        LOG_WARN("DownloadWidget", "Task widget already exists for id: " + QString::number(taskId));
        return;
    }

    LOG_DEBUG("DownloadWidget", "Creating widget for task: " + QString::number(taskId));

    // ✅ 关键修复：在堆上分配，不是在栈上
    TaskWidgets* tw = new TaskWidgets();
    tw->taskId = taskId;

    // 创建列表项
    tw->item = new QListWidgetItem();
    if (!tw->item) {
        LOG_ERROR("DownloadWidget", "Failed to create QListWidgetItem");
        delete tw;
        return;
    }
    tw->item->setSizeHint(QSize(0, 80));

    // 创建任务控件
    tw->widget = new QWidget();
    if (!tw->widget) {
        LOG_ERROR("DownloadWidget", "Failed to create widget");
        delete tw->item;
        delete tw;
        return;
    }

    QHBoxLayout* layout = new QHBoxLayout(tw->widget);
    layout->setContentsMargins(15, 10, 15, 10);
    layout->setSpacing(15);

    // 左侧：文件信息
    QVBoxLayout* infoLayout = new QVBoxLayout();
    infoLayout->setSpacing(5);

    QString displayName = task.fileName;
    if (displayName.isEmpty()) {
        displayName = "未知文件_" + QString::number(taskId);
    }

    tw->nameLabel = new QLabel(displayName);
    tw->nameLabel->setObjectName("taskName");
    tw->nameLabel->setWordWrap(true);
    infoLayout->addWidget(tw->nameLabel);

    tw->infoLabel = new QLabel();
    tw->infoLabel->setObjectName("taskInfo");
    infoLayout->addWidget(tw->infoLabel);

    layout->addLayout(infoLayout, 2);

    // 中间：进度条
    QVBoxLayout* progressLayout = new QVBoxLayout();
    progressLayout->setSpacing(5);

    tw->progressBar = new QProgressBar();
    tw->progressBar->setObjectName("taskProgress");
    tw->progressBar->setRange(0, 100);
    tw->progressBar->setValue(task.progress);
    tw->progressBar->setTextVisible(true);
    tw->progressBar->setFormat("%p%");
    progressLayout->addWidget(tw->progressBar);

    tw->statusLabel = new QLabel();
    tw->statusLabel->setObjectName("taskStatus");
    progressLayout->addWidget(tw->statusLabel);

    layout->addLayout(progressLayout, 3);

    // 右侧：控制按钮
    QVBoxLayout* btnLayout = new QVBoxLayout();
    btnLayout->setSpacing(5);

    tw->controlBtn = new QPushButton();
    tw->controlBtn->setObjectName("taskControlBtn");
    tw->controlBtn->setFixedSize(60, 28);
    btnLayout->addWidget(tw->controlBtn);

    tw->removeBtn = new QPushButton("删除");
    tw->removeBtn->setObjectName("taskRemoveBtn");
    tw->removeBtn->setFixedSize(60, 28);
    btnLayout->addWidget(tw->removeBtn);

    btnLayout->addStretch();
    layout->addLayout(btnLayout);

    // 连接按钮信号
    connect(tw->controlBtn, &QPushButton::clicked, [this, taskId]() {
        DownloadManager& dm = DownloadManager::instance();
        for (const auto& t : dm.getTasks()) {
            if (t.id == taskId) {
                if (t.status == 1) dm.pauseTask(taskId);
                else if (t.status == 2) dm.resumeTask(taskId);
                else if (t.status == 4) dm.retryTask(taskId);
                break;
            }
        }
    });

    connect(tw->removeBtn, &QPushButton::clicked, [this, taskId]() {
        QMessageBox::StandardButton reply = QMessageBox::question(
            this,
            "确认删除",
            "是否同时删除已下载的文件？",
            QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel
            );

        if (reply == QMessageBox::Yes) {
            m_downloadManager->removeTask(taskId, true);  // 删除文件
        } else if (reply == QMessageBox::No) {
            m_downloadManager->removeTask(taskId, false);  // 只删除任务
        } else {
            return;  // 取消
        }

        // 从 UI 移除
        if (m_taskWidgetsV2.contains(taskId)) {
            TaskWidgets* tw = m_taskWidgetsV2[taskId];
            if (tw->item) delete tw->item;
            if (tw->widget) tw->widget->deleteLater();
            delete tw;
            m_taskWidgetsV2.remove(taskId);
        }
    });

    // ✅ 存入 map（存储指针）
    m_taskWidgets[taskId] = tw;

    // 添加到列表
    if (m_taskList && tw->item && tw->widget) {
        m_taskList->addItem(tw->item);
        m_taskList->setItemWidget(tw->item, tw->widget);
    }

    updateTaskWidget(taskId, task);
}

void DownloadWidget::updateTaskWidget(int taskId, const DownloadTask& task)
{
    if (!m_taskWidgets.contains(taskId)) return;

    TaskWidgets* tw = m_taskWidgets[taskId];
    if (!tw) return;

    // 更新文件名
    if (tw->nameLabel) {
        QString displayName = task.fileName;
        if (displayName.isEmpty()) {
            displayName = "未知文件_" + QString::number(taskId);
        }
        tw->nameLabel->setText(displayName);
    }

    // 更新进度条
    if (tw->progressBar) {
        tw->progressBar->setValue(task.progress);
    }

    // 更新信息标签
    if (tw->infoLabel) {
        QString info = QString("%1 / %2")
        .arg(DownloadTask::formatSize(task.downloadedSize))
            .arg(DownloadTask::formatSize(task.totalSize));
        if (task.status == 1 && task.speed > 0) {
            info += QString("  |  %1  |  剩余 %2 秒")
                        .arg(DownloadTask::formatSpeed(task.speed))
                        .arg(task.remainingSeconds());
        }
        tw->infoLabel->setText(info);
    }

    // 更新状态标签
    if (tw->statusLabel) {
        tw->statusLabel->setText(task.getStatusText());
    }

    // 更新控制按钮
    if (tw->controlBtn) {
        if (task.status == 1) {
            tw->controlBtn->setText("暂停");
            tw->controlBtn->setEnabled(true);
        } else if (task.status == 2) {
            tw->controlBtn->setText("开始");
            tw->controlBtn->setEnabled(true);
        } else if (task.status == 4) {
            tw->controlBtn->setText("重试");
            tw->controlBtn->setEnabled(true);
        } else {
            tw->controlBtn->setText("完成");
            tw->controlBtn->setEnabled(false);
        }
    }

    // 更新进度条颜色
    if (tw->progressBar) {
        if (task.status == 3) {
            tw->progressBar->setStyleSheet("QProgressBar::chunk { background-color: #4CAF50; }");
        } else if (task.status == 4) {
            tw->progressBar->setStyleSheet("QProgressBar::chunk { background-color: #F44336; }");
        } else {
            tw->progressBar->setStyleSheet("QProgressBar::chunk { background-color: #4A90E2; }");
        }
    }
}

void DownloadWidget::removeTaskWidget(int taskId)
{
    if (!m_taskWidgets.contains(taskId)) return;

    TaskWidgets* tw = m_taskWidgets[taskId];
    if (!tw) {
        m_taskWidgets.remove(taskId);
        return;
    }

    // 从列表控件中移除
    if (tw->item && m_taskList) {
        m_taskList->removeItemWidget(tw->item);
        delete tw->item;
    }

    // 删除控件（widget 会连带删除里面的子控件）
    if (tw->widget) {
        tw->widget->deleteLater();
    }

    delete tw;
    m_taskWidgets.remove(taskId);

    LOG_DEBUG("DownloadWidget", "Removed task widget: " + QString::number(taskId));
}

void DownloadWidget::updateSummary()
{
    DownloadManager& dm = DownloadManager::instance();
    int total = 0;
    int downloading = 0;
    int paused = 0;
    int completed = 0;
    int error = 0;

    for (const auto& task : dm.getTasks()) {
        total++;
        switch (task.status) {
        case 1: downloading++; break;
        case 2: paused++; break;
        case 3: completed++; break;
        case 4: error++; break;
        }
    }

    QString summary = QString("总计: %1 | 下载中: %2 | 暂停: %3 | 已完成: %4")
                          .arg(total).arg(downloading).arg(paused).arg(completed);

    if (error > 0) {
        summary += QString(" | 错误: %1").arg(error);
    }

    if (m_summaryLabel) {
        m_summaryLabel->setText(summary);
    }

    LOG_DEBUG("DownloadWidget", "Summary updated: " + summary);
}

void DownloadWidget::showContextMenu(const QPoint& pos)
{
    QListWidgetItem* item = m_taskList->itemAt(pos);
    if (!item) return;

    // 找到对应的任务ID
    for (auto it = m_taskWidgets.begin(); it != m_taskWidgets.end(); ++it) {
        if (it.value()->item == item) {  // ✅ 使用 ->
            m_currentTaskId = it.key();
            break;
        }
    }

    if (m_currentTaskId == -1) return;

    // 获取任务信息
    const DownloadTask* task = nullptr;
    for (const auto& t : DownloadManager::instance().getTasks()) {
        if (t.id == m_currentTaskId) {
            task = &t;
            break;
        }
    }

    if (!task) return;

    QMenu menu(this);
    menu.setStyleSheet("QMenu { background-color: #1E2A3A; color: #7EC8E3; }"
                       "QMenu::item:selected { background-color: #4A90E2; color: white; }");

    if (task->status == 1) {
        menu.addAction("⏸ 暂停", this, &DownloadWidget::onPauseTask);
    } else if (task->status == 2) {
        menu.addAction("▶ 开始", this, &DownloadWidget::onResumeTask);
    } else if (task->status == 4) {
        menu.addAction("🔄 重试", this, &DownloadWidget::onRetryTask);
    }

    if (task->status == 3) {
        menu.addAction("📂 打开文件", this, &DownloadWidget::onOpenFile);
        menu.addAction("📁 打开文件夹", this, &DownloadWidget::onOpenFolder);
    }

    menu.addSeparator();
    menu.addAction("🗑 删除任务", this, &DownloadWidget::onRemoveTask);

    menu.exec(m_taskList->mapToGlobal(pos));
}

void DownloadWidget::onPauseTask()
{
    if (m_currentTaskId != -1) {
        LOG_DEBUG("DownloadWidget", "Pause task from context menu: " + QString::number(m_currentTaskId));
        DownloadManager::instance().pauseTask(m_currentTaskId);
    }
}

void DownloadWidget::onResumeTask()
{
    if (m_currentTaskId != -1) {
        LOG_DEBUG("DownloadWidget", "Resume task from context menu: " + QString::number(m_currentTaskId));
        DownloadManager::instance().resumeTask(m_currentTaskId);
    }
}

void DownloadWidget::onRetryTask()
{
    if (m_currentTaskId != -1) {
        LOG_DEBUG("DownloadWidget", "Retry task from context menu: " + QString::number(m_currentTaskId));
        DownloadManager::instance().retryTask(m_currentTaskId);
    }
}

void DownloadWidget::onRemoveTask()
{
    if (m_currentTaskId != -1) {
        LOG_DEBUG("DownloadWidget", "Remove task from context menu: " + QString::number(m_currentTaskId));
        bool deleteFile = QMessageBox::question(this, "确认删除",
                                                "是否同时删除已下载的文件？",
                                                QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel) == QMessageBox::Yes;
        DownloadManager::instance().removeTask(m_currentTaskId, deleteFile);
    }
}

void DownloadWidget::onOpenFile()
{
    if (m_currentTaskId != -1) {
        for (const auto& task : DownloadManager::instance().getTasks()) {
            if (task.id == m_currentTaskId && task.status == 3) {
                LOG_DEBUG("DownloadWidget", "Open file: " + task.savePath);
                QDesktopServices::openUrl(QUrl::fromLocalFile(task.savePath));
                break;
            }
        }
    }
}

void DownloadWidget::onOpenFolder()
{
    if (m_currentTaskId != -1) {
        for (const auto& task : DownloadManager::instance().getTasks()) {
            if (task.id == m_currentTaskId && task.status == 3) {
                QFileInfo info(task.savePath);
                LOG_DEBUG("DownloadWidget", "Open folder: " + info.absolutePath());
                QDesktopServices::openUrl(QUrl::fromLocalFile(info.absolutePath()));
                break;
            }
        }
    }
}

void DownloadWidget::onTaskSpeedV2(int taskId, int speedKBps)
{
    if (!m_taskWidgetsV2.contains(taskId)) return;

    TaskWidgets* tw = m_taskWidgetsV2[taskId];
    if (!tw) return;

    // 检查是否已完成（进度条已经是100且状态是完成）
    int percent = tw->progressBar ? tw->progressBar->value() : 0;
    if (percent >= 100) {
        return;  // 已完成，不显示速度
    }

    // 格式化速度
    QString speedText;
    if (speedKBps < 1024) {
        speedText = QString("%1 KB/s").arg(speedKBps);
    } else {
        speedText = QString("%1 MB/s").arg(speedKBps / 1024.0, 0, 'f', 1);
    }

    // 同时显示进度和速度
    if (tw->infoLabel) {
        tw->infoLabel->setText(QString("进度 %1% | %2").arg(percent).arg(speedText));
        qDebug() << "Speed displayed:" << speedText;
    }
}
void DownloadWidget::onClearCompleted()
{
    LOG_DEBUG("DownloadWidget", "onClearCompleted called - TODO");
    // 暂时空实现
}

// void DownloadWidget::onPauseAll()
// {
//     LOG_DEBUG("DownloadWidget", "onPauseAll called - TODO");
//     // 暂时空实现
// }

// void DownloadWidget::onResumeAll()
// {
//     LOG_DEBUG("DownloadWidget", "onResumeAll called - TODO");
//     // 暂时空实现
// }

void DownloadWidget::onTaskAddedV2(int taskId)
{
    qDebug() << "UI: Creating widget for task" << taskId;

    // 检查是否已存在
    if (m_taskWidgetsV2.contains(taskId)) {
        qDebug() << "Task already exists:" << taskId;
        return;
    }

    // 创建任务控件
    TaskWidgets* tw = new TaskWidgets();
    tw->taskId = taskId;

    // 创建列表项
    tw->item = new QListWidgetItem();
    tw->item->setSizeHint(QSize(0, 80));

    // 创建主控件
    tw->widget = new QWidget();
    QHBoxLayout* layout = new QHBoxLayout(tw->widget);
    layout->setContentsMargins(15, 10, 15, 10);
    layout->setSpacing(15);

    // 左侧：文件信息
    QVBoxLayout* infoLayout = new QVBoxLayout();
    tw->nameLabel = new QLabel(QString("任务 %1").arg(taskId));
    tw->nameLabel->setObjectName("taskName");
    infoLayout->addWidget(tw->nameLabel);

    tw->infoLabel = new QLabel("下载中...");
    tw->infoLabel->setObjectName("taskInfo");
    infoLayout->addWidget(tw->infoLabel);
    layout->addLayout(infoLayout, 2);

    // 中间：进度条
    QVBoxLayout* progressLayout = new QVBoxLayout();
    tw->progressBar = new QProgressBar();
    tw->progressBar->setRange(0, 100);
    tw->progressBar->setValue(0);
    tw->progressBar->setTextVisible(true);
    tw->progressBar->setFormat("%p%");
    progressLayout->addWidget(tw->progressBar);

    tw->statusLabel = new QLabel("等待中");
    progressLayout->addWidget(tw->statusLabel);
    layout->addLayout(progressLayout, 3);

    // 右侧：控制按钮
    QVBoxLayout* btnLayout = new QVBoxLayout();
    tw->controlBtn = new QPushButton("暂停");
    tw->controlBtn->setFixedSize(60, 28);
    tw->removeBtn = new QPushButton("删除");
    tw->removeBtn->setFixedSize(60, 28);
    btnLayout->addWidget(tw->controlBtn);
    btnLayout->addWidget(tw->removeBtn);
    layout->addLayout(btnLayout);

    // 连接按钮信号（需要从 manager 获取任务信息）
    // 删除时让用户选择
    connect(tw->removeBtn, &QPushButton::clicked, [this, taskId]() {
        QMessageBox::StandardButton reply = QMessageBox::question(
            this, "确认删除", "是否同时删除已下载的文件？",
            QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel
            );

        if (reply == QMessageBox::Yes) {
            m_downloadManager->removeTask(taskId, true);   // 删除文件
        } else if (reply == QMessageBox::No) {
            m_downloadManager->removeTask(taskId, false);  // 只删任务
        } else {
            return;
        }

        // 从 UI 移除
        if (m_taskWidgetsV2.contains(taskId)) {
            delete m_taskWidgetsV2[taskId]->item;
            m_taskWidgetsV2[taskId]->widget->deleteLater();
            delete m_taskWidgetsV2[taskId];
            m_taskWidgetsV2.remove(taskId);
        }
    });

    // 添加到列表
    m_taskList->addItem(tw->item);
    m_taskList->setItemWidget(tw->item, tw->widget);

    m_taskWidgetsV2[taskId] = tw;
}
void DownloadWidget::onTaskProgressV2(int taskId, int percent)
{
    if (!m_taskWidgetsV2.contains(taskId)) return;

    TaskWidgets* tw = m_taskWidgetsV2[taskId];
    if (!tw) return;

    // 更新进度条
    if (tw->progressBar) {
        tw->progressBar->setValue(percent);
    }

    // 更新状态标签
    if (tw->statusLabel && percent < 100) {
        tw->statusLabel->setText(QString("下载中 %1%").arg(percent));
    }

    // 更新信息标签（可以显示速度，暂时先显示百分比）
    // if (tw->infoLabel && percent < 100) {
    //     tw->infoLabel->setText(QString("下载进度: %1%").arg(percent));
    // }
}



void DownloadWidget::onTaskFinishedV2(int taskId, bool success, const QString& errorMsg)
{
    if (!m_taskWidgetsV2.contains(taskId)) return;

    TaskWidgets* tw = m_taskWidgetsV2[taskId];
    if (!tw) return;

    if (success) {
        if (tw->statusLabel) {
            tw->statusLabel->setText("✅ 完成");
        }
        if (tw->infoLabel) {
            // 下载完成，只显示完成，不显示速度
            tw->infoLabel->setText("下载完成");
        }
        if (tw->progressBar) {
            tw->progressBar->setValue(100);
        }
        if (tw->controlBtn) {
            tw->controlBtn->setText("完成");
            tw->controlBtn->setEnabled(false);
        }
        qDebug() << "Task" << taskId << "succeeded!";
    } else {
        if (tw->statusLabel) {
            tw->statusLabel->setText("❌ 失败");
        }
        if (tw->infoLabel) {
            tw->infoLabel->setText(QString("失败: %1").arg(errorMsg));
        }
        if (tw->controlBtn) {
            tw->controlBtn->setText("重试");
        }
        qDebug() << "Task" << taskId << "failed:" << errorMsg;
    }
}


