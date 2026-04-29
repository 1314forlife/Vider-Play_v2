#include "LoginDialog.h"
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>

LoginDialog::LoginDialog(QWidget* parent) : QDialog(parent) {
    // ✅ 关键：隐藏原生标题栏
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint | Qt::WindowCloseButtonHint);
    // setAttribute(Qt::WA_TranslucentBackground);

    setWindowTitle("芙宁娜·水之颂 - 登录");
    setFixedSize(340, 300);
    setModal(true);
    setObjectName("loginDialog");

    setWindowTitle("芙宁娜·水之颂 - 登录");
    setFixedSize(340, 300); // 稍调高度，避免按钮太挤
    setModal(true);
    setObjectName("loginDialog");

    // 标题
    QLabel* titleLabel = new QLabel("芙宁娜·水之颂", this);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setObjectName("loginTitle");
    titleLabel->setMinimumHeight(40); // 固定标题高度

    // 用户名输入框
    m_usernameEdit = new QLineEdit(this);
    m_usernameEdit->setPlaceholderText("用户名");
    m_usernameEdit->setObjectName("loginEdit");
    m_usernameEdit->setMinimumHeight(38); // 统一输入框高度

    // 密码输入框
    m_passwordEdit = new QLineEdit(this);
    m_passwordEdit->setPlaceholderText("密码");
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    m_passwordEdit->setObjectName("loginEdit");
    m_passwordEdit->setMinimumHeight(38);

    // 按钮
    m_loginBtn = new QPushButton("登录", this);
    m_registerBtn = new QPushButton("注册", this);
    m_cancelBtn = new QPushButton("取消", this);
    m_loginBtn->setObjectName("loginBtn");
    m_registerBtn->setObjectName("registerBtn");
    m_cancelBtn->setObjectName("cancelBtn");
    m_loginBtn->setMinimumHeight(36);
    m_registerBtn->setMinimumHeight(36);
    m_cancelBtn->setMinimumHeight(36);

    // 按钮布局：设置拉伸比例，让前两个按钮更宽
    QHBoxLayout* btnLayout = new QHBoxLayout();
    btnLayout->addWidget(m_loginBtn, 2);  // 拉伸因子 2
    btnLayout->addWidget(m_registerBtn, 2); // 拉伸因子 2
    btnLayout->addWidget(m_cancelBtn, 1);  // 拉伸因子 1（更窄）
    btnLayout->setSpacing(12); // 按钮间距

    // 主布局：调整边距和间距，消除冗余留白
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(30, 25, 30, 25); // 左右稍宽，上下适中
    mainLayout->setSpacing(18); // 控件间距
    mainLayout->addWidget(titleLabel);
    mainLayout->addWidget(m_usernameEdit);
    mainLayout->addWidget(m_passwordEdit);
    mainLayout->addLayout(btnLayout);
    mainLayout->addStretch(1); // 底部加弹性空间，避免按钮贴边

    // 信号槽
    connect(m_loginBtn, &QPushButton::clicked, this, &LoginDialog::onLoginClicked);
    connect(m_registerBtn, &QPushButton::clicked, this, &LoginDialog::onRegisterClicked);
    connect(m_cancelBtn, &QPushButton::clicked, this, &LoginDialog::reject);
}

void LoginDialog::onLoginClicked() {
    QString username = m_usernameEdit->text().trimmed();
    QString password = m_passwordEdit->text();

    if (username.isEmpty() || password.isEmpty()) {
        showMessage("请输入用户名和密码");
        return;
    }

    m_loginBtn->setEnabled(false);
    m_loginBtn->setText("登录中...");

    QNetworkAccessManager* nam = new QNetworkAccessManager(this);
    QUrl url("http://localhost:8080/api/login");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject body;
    body["username"] = username;
    body["password"] = password;

    QNetworkReply* reply = nam->post(request, QJsonDocument(body).toJson());
    connect(reply, &QNetworkReply::finished, this, [this, reply, nam]() {
        if (reply->error() != QNetworkReply::NoError) {
            showMessage("网络错误: " + reply->errorString());
            m_loginBtn->setEnabled(true);
            m_loginBtn->setText("登录");
            reply->deleteLater();
            nam->deleteLater();
            return;
        }

        QByteArray data = reply->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        QJsonObject obj = doc.object();

        if (obj.contains("token")) {
            QString token = obj["token"].toString();
            emit loginSuccess(token);
            accept();
        } else if (obj.contains("error")) {
            showMessage(obj["error"].toString());
        } else {
            showMessage("登录失败");
        }

        m_loginBtn->setEnabled(true);
        m_loginBtn->setText("登录");
        reply->deleteLater();
        nam->deleteLater();
    });
}

void LoginDialog::onRegisterClicked() {
    QString username = m_usernameEdit->text().trimmed();
    QString password = m_passwordEdit->text();

    if (username.isEmpty() || password.isEmpty()) {
        showMessage("请输入用户名和密码");
        return;
    }

    if (password.length() < 6) {
        showMessage("密码长度至少6位");
        return;
    }

    m_registerBtn->setEnabled(false);
    m_registerBtn->setText("注册中...");

    QNetworkAccessManager* nam = new QNetworkAccessManager(this);
    QUrl url("http://localhost:8080/api/register");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject body;
    body["username"] = username;
    body["password"] = password;

    QNetworkReply* reply = nam->post(request, QJsonDocument(body).toJson());
    connect(reply, &QNetworkReply::finished, this, [this, reply, nam]() {
        if (reply->error() != QNetworkReply::NoError) {
            showMessage("网络错误: " + reply->errorString());
            m_registerBtn->setEnabled(true);
            m_registerBtn->setText("注册");
            reply->deleteLater();
            nam->deleteLater();
            return;
        }

        QByteArray data = reply->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        QJsonObject obj = doc.object();

        if (obj.contains("status") && obj["status"].toString() == "ok") {
            QMessageBox::information(this, "注册成功", "注册成功！请登录");
            m_registerBtn->setEnabled(true);
            m_registerBtn->setText("注册");
        } else if (obj.contains("error")) {
            showMessage(obj["error"].toString());
        } else {
            showMessage("注册失败");
        }

        m_registerBtn->setEnabled(true);
        m_registerBtn->setText("注册");
        reply->deleteLater();
        nam->deleteLater();
    });
}

void LoginDialog::showMessage(const QString& msg, bool isError) {
    QMessageBox::warning(this, isError ? "错误" : "提示", msg);
}