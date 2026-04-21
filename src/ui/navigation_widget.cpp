#include "navigation_widget.h"
#include <QVBoxLayout>

NavigationWidget::NavigationWidget(QWidget* parent) : QWidget(parent)
{
    setFixedWidth(80);
    setObjectName("navWidget");
     setAttribute(Qt::WA_TransparentForMouseEvents, false);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 20, 8, 20);
    layout->setSpacing(8);

    // 播放按钮
    m_playerBtn = new QPushButton("▶\n播放", this);
    m_playerBtn->setObjectName("navBtn");
    m_playerBtn->setCheckable(true);
    m_playerBtn->setChecked(true);

    // 下载按钮
    m_downloadBtn = new QPushButton("↓\n下载", this);
    m_downloadBtn->setObjectName("navBtn");
    m_downloadBtn->setCheckable(true);

    // 主题按钮
    m_themeBtn = new QPushButton("🎨\n主题", this);
    m_themeBtn->setObjectName("navBtn");
    m_themeBtn->setCheckable(true);

    // 设置按钮
    m_settingsBtn = new QPushButton("⚙️\n设置", this);
    m_settingsBtn->setObjectName("navBtn");
    m_settingsBtn->setCheckable(true);

    layout->addWidget(m_playerBtn);
    layout->addSpacing(20);
    layout->addWidget(m_downloadBtn);
    layout->addSpacing(20);
    layout->addWidget(m_themeBtn);
    layout->addSpacing(20);
    layout->addWidget(m_settingsBtn);
    layout->addStretch();

    // 连接信号
    connect(m_playerBtn, &QPushButton::clicked, this, &NavigationWidget::playerClicked);
    connect(m_downloadBtn, &QPushButton::clicked, this, &NavigationWidget::downloadClicked);
    connect(m_themeBtn, &QPushButton::clicked, this, &NavigationWidget::themeClicked);
    connect(m_settingsBtn, &QPushButton::clicked, this, &NavigationWidget::settingsClicked);
}

void NavigationWidget::setActiveButton(int index)
{
    m_playerBtn->setChecked(index == 0);
    m_downloadBtn->setChecked(index == 1);
    m_themeBtn->setChecked(index == 2);
    m_settingsBtn->setChecked(index == 3);
}