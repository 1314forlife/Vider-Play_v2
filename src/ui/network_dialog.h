#ifndef NETWORK_DIALOG_H
#define NETWORK_DIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QProgressBar>
#include "src/network/network_stream_manager.h"

class NetworkStreamManager;  // 前向声明，不是 NetworkPlayer

class NetworkDialog : public QDialog {
    Q_OBJECT
public:
    explicit NetworkDialog(QWidget* parent = nullptr);
    ~NetworkDialog();

    QString getUrl() const { return m_url; }

signals:
    void urlReady(const QString& url);

private slots:
    void onPlay();
    void onLogMessage(const QString& msg);
    void onError(const QString& error);
    void onPlayReady(const QString& localUrl);

private:
    QLineEdit* m_urlEdit;
    QPushButton* m_playBtn;
    QPushButton* m_cancelBtn;
    QLabel* m_statusLabel;
    QProgressBar* m_progressBar;
    QString m_url;
    NetworkStreamManager* m_streamMgr;  // 改为这个类型
};

#endif