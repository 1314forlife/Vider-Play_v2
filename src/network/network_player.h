#ifndef NETWORK_PLAYER_H
#define NETWORK_PLAYER_H

#include <QObject>
#include <QProcess>

class NetworkPlayer : public QObject {
    Q_OBJECT
public:
    explicit NetworkPlayer(QObject* parent = nullptr);
    ~NetworkPlayer();

    void play(const QString& url);
    void stop();

signals:
    void sigDataReady(const QByteArray& data);
    void sigError(const QString& error);

private slots:
    void onYtFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onYtError(QProcess::ProcessError error);

private:
    QProcess* m_ytProcess;
    QProcess* m_ffmpegProcess;
    QString m_currentUrl;
};

#endif