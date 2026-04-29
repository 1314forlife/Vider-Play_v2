#ifndef LICENSECLIENT_H
#define LICENSECLIENT_H

#pragma once
#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonObject>

class LicenseClient : public QObject {
    Q_OBJECT
public:
    static LicenseClient* instance();

    // 登录
    void login(const QString& username, const QString& password);

    // 获取许可证密钥
    void requestLicense(const QString& videoId);

    // 设置/获取 token
    void setToken(const QString& token);
    QString getToken() const { return m_token; }
    bool isLoggedIn() const { return !m_token.isEmpty(); }

signals:
    void loginSuccess(const QString& token);
    void loginFailed(const QString& error);
    void licenseReady(const QString& licenseKey);
    void licenseFailed(const QString& error);

private:
    LicenseClient();
    static LicenseClient* m_instance;
    QNetworkAccessManager* m_nam;
    QString m_token;
    QString m_serverUrl = "http://localhost:8080";

    void handleLoginReply(QNetworkReply* reply);
    void handleLicenseReply(QNetworkReply* reply);
};

#endif // LICENSECLIENT_H
