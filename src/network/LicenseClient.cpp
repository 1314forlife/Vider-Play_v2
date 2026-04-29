#include "LicenseClient.h"
#include <QJsonDocument>
#include <QJsonParseError>
#include <QDebug>

LicenseClient* LicenseClient::m_instance = nullptr;

LicenseClient* LicenseClient::instance() {
    if (!m_instance) {
        m_instance = new LicenseClient();
    }
    return m_instance;
}

LicenseClient::LicenseClient() {
    m_nam = new QNetworkAccessManager(this);
}

void LicenseClient::login(const QString& username, const QString& password) {
    QUrl url(m_serverUrl + "/api/login");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject body;
    body["username"] = username;
    body["password"] = password;

    QNetworkReply* reply = m_nam->post(request, QJsonDocument(body).toJson());
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        handleLoginReply(reply);
        reply->deleteLater();
    });
}

void LicenseClient::requestLicense(const QString& videoId) {
    if (m_token.isEmpty()) {
        emit licenseFailed("Not logged in");
        return;
    }

    QUrl url(m_serverUrl + "/api/license?video_id=" + videoId);
    QNetworkRequest request(url);
    request.setRawHeader("Authorization", ("Bearer " + m_token).toUtf8());

    QNetworkReply* reply = m_nam->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        handleLicenseReply(reply);
        reply->deleteLater();
    });
}

void LicenseClient::setToken(const QString& token) {
    m_token = token;
}

void LicenseClient::handleLoginReply(QNetworkReply* reply) {
    if (reply->error() != QNetworkReply::NoError) {
        emit loginFailed(reply->errorString());
        return;
    }

    QByteArray data = reply->readAll();
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);

    if (error.error != QJsonParseError::NoError) {
        emit loginFailed("Parse error");
        return;
    }

    QJsonObject obj = doc.object();
    if (obj.contains("token")) {
        m_token = obj["token"].toString();
        emit loginSuccess(m_token);
    } else if (obj.contains("error")) {
        emit loginFailed(obj["error"].toString());
    } else {
        emit loginFailed("Unknown error");
    }
}

void LicenseClient::handleLicenseReply(QNetworkReply* reply) {
    if (reply->error() != QNetworkReply::NoError) {
        emit licenseFailed(reply->errorString());
        return;
    }

    QByteArray data = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonObject obj = doc.object();

    if (obj.contains("license_key")) {
        emit licenseReady(obj["license_key"].toString());
    } else if (obj.contains("error")) {
        emit licenseFailed(obj["error"].toString());
    } else {
        emit licenseFailed("Unknown error");
    }
}