#ifndef NETWORK_DIALOG_H
#define NETWORK_DIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QProgressBar>
#include "src/network/network_stream_manager.h"

class NetworkDialog : public QDialog
{
    Q_OBJECT

public:
    explicit NetworkDialog(QWidget* parent = nullptr);
    ~NetworkDialog();

    QString getUrl() const { return m_urlEdit->text(); }

signals:
    void urlReady(const QString& url);

private slots:
    void onPlay();
    void onCancel();
    void onLogMessage(const QString& msg);
    void onError(const QString& error);
    void onPlayReady(const QString& localUrl);

private:
    QLineEdit* m_urlEdit;
    QPushButton* m_playBtn;
    QPushButton* m_cancelBtn;
    QLabel* m_statusLabel;
    QProgressBar* m_progressBar;
    NetworkStreamManager* m_streamMgr;
};

#endif // NETWORK_DIALOG_H