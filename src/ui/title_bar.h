#ifndef TITLE_BAR_H
#define TITLE_BAR_H

#include <QWidget>
#include <QPushButton>
#include <QLabel>

class TitleBar : public QWidget
{
    Q_OBJECT
public:
    explicit TitleBar(QWidget* parent = nullptr);

    void setTitle(const QString& title);

signals:
    void minimizeClicked();
    void maximizeClicked();
    void closeClicked();

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;

private:
    QLabel* m_titleLabel = nullptr;
    QPushButton* m_minBtn = nullptr;
    QPushButton* m_maxBtn = nullptr;
    QPushButton* m_closeBtn = nullptr;

    QPoint m_dragPosition;
    bool m_dragging = false;
};

#endif