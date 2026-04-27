#ifndef DOWNLOAD_HISTORY_H
#define DOWNLOAD_HISTORY_H

#include <QString>
#include <QDateTime>
#include <QList>

struct DownloadRecord {
    int id;
    QString url;
    QString fileName;
    QString savePath;
    qint64 fileSize;
    QDateTime finishTime;
};

class DownloadHistory
{
public:
    static DownloadHistory& instance();

    bool init();
    void addRecord(const QString& url, const QString& fileName,
                   const QString& savePath, qint64 fileSize);
    QList<DownloadRecord> getAllRecords();
    void clearHistory();
    void removeRecord(int id);

private:
    DownloadHistory() = default;
    ~DownloadHistory();
    bool execSQL(const QString& sql);
    bool execSQL(const QString& sql, const QList<QVariant>& params);

    void* m_db = nullptr;  // sqlite3*
    bool m_isOpen = false;
};

#endif // DOWNLOAD_HISTORY_H