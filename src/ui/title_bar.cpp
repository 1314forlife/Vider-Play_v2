#include "title_bar.h"
#include <QHBoxLayout>
#include <QMouseEvent>

TitleBar::TitleBar(QWidget* parent) : QWidget(parent)
{
    setFixedHeight(40);
    setObjectName("titleBar");

    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(12, 0, 12, 0);
    layout->setSpacing(8);

    m_titleLabel = new QLabel("视频播放器", this);
    m_titleLabel->setObjectName("titleLabel");
    layout->addWidget(m_titleLabel);

    layout->addStretch();

    m_minBtn = new QPushButton("─", this);
    m_minBtn->setObjectName("minBtn");
    m_minBtn->setFixedSize(30, 30);
    layout->addWidget(m_minBtn);

    m_maxBtn = new QPushButton("□", this);
    m_maxBtn->setObjectName("maxBtn");
    m_maxBtn->setFixedSize(30, 30);
    layout->addWidget(m_maxBtn);

    m_closeBtn = new QPushButton("×", this);
    m_closeBtn->setObjectName("closeBtn");
    m_closeBtn->setFixedSize(30, 30);
    layout->addWidget(m_closeBtn);

    connect(m_minBtn, &QPushButton::clicked, this, &TitleBar::minimizeClicked);
    connect(m_maxBtn, &QPushButton::clicked, this, &TitleBar::maximizeClicked);
    connect(m_closeBtn, &QPushButton::clicked, this, &TitleBar::closeClicked);
}

void TitleBar::setTitle(const QString& title)
{
    m_titleLabel->setText(title);
}

void TitleBar::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragging = true;
        // 关键修改：获取顶层窗口
        QWidget* topWindow = window();
        if (topWindow) {
            m_dragPosition = event->globalPosition().toPoint() - topWindow->frameGeometry().topLeft();
        }
        event->accept();
    }
}

void TitleBar::mouseMoveEvent(QMouseEvent* event)
{
    if (m_dragging) {
        // 关键修改：移动顶层窗口
        QWidget* topWindow = window();
        if (topWindow) {
            topWindow->move(event->globalPosition().toPoint() - m_dragPosition);
        }
        event->accept();
    }
}

void TitleBar::mouseDoubleClickEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        emit maximizeClicked();
    }
}