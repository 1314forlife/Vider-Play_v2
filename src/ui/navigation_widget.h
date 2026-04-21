#ifndef NAVIGATION_WIDGET_H
#define NAVIGATION_WIDGET_H

#include <QWidget>
#include <QPushButton>

class NavigationWidget : public QWidget
{
    Q_OBJECT
public:
    explicit NavigationWidget(QWidget* parent = nullptr);

    void setActiveButton(int index);

signals:
    void playerClicked();
    void downloadClicked();
    void themeClicked();
    void settingsClicked();

private:
    QPushButton* m_playerBtn = nullptr;
    QPushButton* m_downloadBtn = nullptr;
    QPushButton* m_themeBtn = nullptr;
    QPushButton* m_settingsBtn = nullptr;
};

#endif