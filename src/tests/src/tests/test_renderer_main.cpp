#include <QApplication>
#include "src/ui/main_window.h"
#include "src/common/logger.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    // 初始化日志
    Logger::instance().setLevel(LOG_DEBUG);
    MainWindow window;
    window.show();

    LOG_INFO("视频渲染器测试启动");

    return app.exec();
}