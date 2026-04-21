#include "theme_manager.h"
#include <QFile>
#include <QApplication>
#include <QDebug>

ThemeManager& ThemeManager::instance()
{
    static ThemeManager instance;
    return instance;
}

void ThemeManager::addTheme(const QString& name, const QString& qssPath)
{
    m_themes[name] = qssPath;
}

bool ThemeManager::loadTheme(const QString& themeName)
{
    if (!m_themes.contains(themeName)) {
        qWarning() << "Theme not found:" << themeName;
        return false;
    }

    QString qssPath = m_themes[themeName];
    if (applyTheme(qssPath)) {
        m_currentTheme = themeName;
        emit themeChanged(themeName);
        return true;
    }
    return false;
}

bool ThemeManager::applyTheme(const QString& qssPath)
{
    QFile file(qssPath);
    if (!file.open(QFile::ReadOnly)) {
        qWarning() << "Cannot open theme file:" << qssPath;
        return false;
    }

    QString styleSheet = QLatin1String(file.readAll());
    qApp->setStyleSheet(styleSheet);
    file.close();
    return true;
}

QStringList ThemeManager::availableThemes() const
{
    return m_themes.keys();
}