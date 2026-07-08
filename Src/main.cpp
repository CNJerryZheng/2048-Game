#include "mainwindow.h"

#include <QApplication>
#include <QCoreApplication>
#include <QDir>

int main(int argc, char *argv[])
{
    QApplication application(argc, argv);
    QDir::setCurrent(QCoreApplication::applicationDirPath());
    MainWindow window;
    window.show();
    return application.exec();
}
