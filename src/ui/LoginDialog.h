#pragma once
#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>

class LoginDialog : public QDialog {
    Q_OBJECT
public:
    explicit LoginDialog(QWidget* parent = nullptr);

signals:
    void loginSuccess(const QString& token);

private slots:
    void onLoginClicked();
    void onRegisterClicked();
    // 没有 onNetworkReply

private:
    QLineEdit* m_usernameEdit;
    QLineEdit* m_passwordEdit;
    QPushButton* m_loginBtn;
    QPushButton* m_registerBtn;
    QPushButton* m_cancelBtn;

    void showMessage(const QString& msg, bool isError = true);
};