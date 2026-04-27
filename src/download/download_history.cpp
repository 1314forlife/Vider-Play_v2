#include "download_history.h"
#include <QDebug>
#include <QDir>
#include <QStandardPaths>
#include <sqlite3.h>

DownloadHistory& DownloadHistory::instance()
{
    static DownloadHistory instance;
    return instance;
}

DownloadHistory::~DownloadHistory()
{
    if (m_db && m_isOpen) {
        sqlite3_close((sqlite3*)m_db);
        m_db = nullptr;
        m_isOpen = false;
    }
}

bool DownloadHistory::init()
{
    if (m_isOpen) return true;

    QString dbPath = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/download_history.db";
    QDir().mkpath(QFileInfo(dbPath).absolutePath());

    int rc = sqlite3_open(dbPath.toUtf8().constData(), (sqlite3**)&m_db);
    if (rc != SQLITE_OK) {
        qDebug() << "Failed to open database:" << sqlite3_errmsg((sqlite3*)m_db);
        return false;
    }

    const char* sql = "CREATE TABLE IF NOT EXISTS download_history ("
                      "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                      "url TEXT,"
                      "fileName TEXT,"
                      "savePath TEXT,"
                      "fileSize INTEGER,"
                      "finishTime INTEGER"
                      ")";
    char* errMsg = nullptr;
    rc = sqlite3_exec((sqlite3*)m_db, sql, nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        qDebug() << "SQL error:" << errMsg;
        sqlite3_free(errMsg);
        return false;
    }

    m_isOpen = true;
    return true;
}

void DownloadHistory::addRecord(const QString& url, const QString& fileName,
                                const QString& savePath, qint64 fileSize)
{
    qDebug() << "=== addRecord ENTER ===";
    qDebug() << "  url:" << url;
    qDebug() << "  fileName:" << fileName;
    qDebug() << "  savePath:" << savePath;
    qDebug() << "  fileSize:" << fileSize;

    if (!m_isOpen) init();

    const char* sql = "INSERT INTO download_history (url, fileName, savePath, fileSize, finishTime) "
                      "VALUES (?, ?, ?, ?, ?)";
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2((sqlite3*)m_db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) return;

    sqlite3_bind_text(stmt, 1, url.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, fileName.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, savePath.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 4, fileSize);
    sqlite3_bind_int64(stmt, 5, QDateTime::currentSecsSinceEpoch());

    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

QList<DownloadRecord> DownloadHistory::getAllRecords()
{
    QList<DownloadRecord> list;
    if (!m_isOpen) init();

    const char* sql = "SELECT id, url, fileName, savePath, fileSize, finishTime FROM download_history "
                      "ORDER BY finishTime DESC";
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2((sqlite3*)m_db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) return list;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        DownloadRecord rec;
        rec.id = sqlite3_column_int(stmt, 0);
        rec.url = QString::fromUtf8((const char*)sqlite3_column_text(stmt, 1));
        rec.fileName = QString::fromUtf8((const char*)sqlite3_column_text(stmt, 2));
        rec.savePath = QString::fromUtf8((const char*)sqlite3_column_text(stmt, 3));
        rec.fileSize = sqlite3_column_int64(stmt, 4);
        rec.finishTime = QDateTime::fromSecsSinceEpoch(sqlite3_column_int64(stmt, 5));
        list.append(rec);
    }
    sqlite3_finalize(stmt);
    return list;
}

void DownloadHistory::clearHistory()
{
    if (!m_isOpen) init();
    const char* sql = "DELETE FROM download_history";
    sqlite3_exec((sqlite3*)m_db, sql, nullptr, nullptr, nullptr);
}

void DownloadHistory::removeRecord(int id)
{
    if (!m_isOpen) init();
    const char* sql = "DELETE FROM download_history WHERE id = ?";
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2((sqlite3*)m_db, sql, -1, &stmt, nullptr);
    sqlite3_bind_int(stmt, 1, id);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}