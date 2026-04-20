#ifndef NETWORK_STREAM_MANAGER_H
#define NETWORK_STREAM_MANAGER_H

#include <QObject>
#include <QProcess>
#include <QTimer>

class NetworkStreamManager : public QObject
{
    Q_OBJECT

public:
    explicit NetworkStreamManager(QObject *parent = nullptr);
    ~NetworkStreamManager();

    void startStream(const QString &bilibiliUrl);
    void stopStream();
    bool isRunning() const;

signals:
    void streamReady(const QString &localUrl);  // tcp://127.0.0.1:12345
    void streamError(const QString &error);
    void logMessage(const QString &msg);

private:
    void setupProcess();

private:
    QProcess *m_process;
    QString m_localUrl;
    int m_port;
    bool m_isReady;
};

#endif