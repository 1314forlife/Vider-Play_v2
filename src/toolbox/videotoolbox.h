// src/toolbox/videotoolbox.h
#ifndef VIDEOTOOLBOX_H
#define VIDEOTOOLBOX_H

#include <QWidget>
#include <QListWidget>
#include <QStackedWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>

class VideoToolBox : public QWidget
{
    Q_OBJECT
public:
    explicit VideoToolBox(QWidget *parent = nullptr);
    ~VideoToolBox();

    void setEmbedMode(bool embed);

signals:
    void videoProcessed(const QString &outputPath);

private:
    void initUI();
    void initConnections();

    QListWidget *m_toolList;
    QStackedWidget *m_stackWidget;

    // 工具页面（暂时用占位，后续逐步实现）
    QWidget *m_transcodePage;
    QWidget *m_watermarkPage;
    QWidget *m_cropPage;
    QWidget *m_repairPage;
    QWidget *m_convertPage;
    QWidget *m_compressPage;
    QWidget *m_audioPage;
    QWidget *m_screenshotPage;

    QLabel *m_statusLabel;
};

#endif // VIDEOTOOLBOX_H