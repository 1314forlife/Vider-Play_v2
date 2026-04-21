#ifndef THEME_MANAGER_H
#define THEME_MANAGER_H

#include <QObject>
#include <QMap>
#include <QString>

class ThemeManager : public QObject
{
    Q_OBJECT
public:
    static ThemeManager& instance();

    // 添加主题
    void addTheme(const QString& name, const QString& qssPath);

    // 加载主题
    bool loadTheme(const QString& themeName);

    // 获取可用主题列表
    QStringList availableThemes() const;

    // 获取当前主题名称
    QString currentTheme() const { return m_currentTheme; }

signals:
    void themeChanged(const QString& themeName);

private:
    ThemeManager() = default;
    ~ThemeManager() = default;
    ThemeManager(const ThemeManager&) = delete;
    ThemeManager& operator=(const ThemeManager&) = delete;

    bool applyTheme(const QString& qssPath);

    QMap<QString, QString> m_themes;  // 主题名 -> QSS文件路径
    QString m_currentTheme;
};

#endif