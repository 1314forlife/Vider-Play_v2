#include <QApplication>
#include <QFile>
#include <QDebug>
#include "src/ui/main_window.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    // 这里直接用你现在的资源路径
    QFile styleFile(":/src/resource/style.qss");
    if (styleFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QString styleSheet = styleFile.readAll();
        app.setStyleSheet(styleSheet);
        styleFile.close();
        qDebug() << "✅ QSS 加载成功！";
    } else {
        qDebug() << "❌ QSS 加载失败：" << styleFile.errorString();
        qDebug() << "❌ 尝试路径：" << styleFile.fileName();
    }

    MainWindow window;
    window.show();
    return app.exec();
}