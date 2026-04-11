#include "src/stdafx.h"
#include "src/ui/mainwindow.h"
#include <QApplication>

int main(int argc,char* argv[])
{
    qRegisterMetaType<FrameData>();
    QApplication a(argc,argv);

    // 高DPI适配（4K屏幕不模糊）

    a.setAttribute(Qt::AA_EnableHighDpiScaling);
    a.setAttribute(Qt::AA_UseHighDpiPixmaps);

    MainWindow w;
    w.show();

    return a.exec();
}