#include <QApplication>
#include <QDebug>
#include "src/ui/main_window.h"
#include "src/theme/theme_manager.h"
#include "src/download/download_history.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    // 初始化 SQLite 历史记录数据库
    if (DownloadHistory::instance().init()) {
        qDebug() << "✅ 下载历史数据库初始化成功";
    } else {
        qDebug() << "❌ 下载历史数据库初始化失败";
    }

    // 初始化主题管理器
    ThemeManager& themeMgr = ThemeManager::instance();

    // 注册主题
    themeMgr.addTheme("default", ":/src/resource/themes/default.qss");
    themeMgr.addTheme("furina", ":/src/resource/themes/furina.qss");

    // 加载芙宁娜主题
    if (themeMgr.loadTheme("furina")) {
        qDebug() << "✅ 芙宁娜主题加载成功";
    } else {
        qDebug() << "❌ 主题加载失败，使用默认样式";
    }

    MainWindow window;
    window.show();
    return app.exec();
}