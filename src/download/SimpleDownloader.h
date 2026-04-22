#ifndef SIMPLEDOWNLOADER_H
#define SIMPLEDOWNLOADER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QFile>
#include <QUrl>

class SimpleDownloader : public QObject
{
    Q_OBJECT

public:
    explicit SimpleDownloader(QObject* parent = nullptr);

    // 开始下载
    void download(const QString& url, const QString& savePath);

signals:
    void progress(int percent);
    void finished(bool success, const QString& errorMsg);

private slots:
    void onReadyRead();
    void onFinished();
    void onProgress(qint64 received, qint64 total);

private:
    QNetworkAccessManager* m_nam;
    QNetworkReply* m_reply;
    QFile* m_file;
};

#endif // SIMPLEDOWNLOADER_H