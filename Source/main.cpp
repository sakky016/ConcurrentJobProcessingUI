#include <QApplication>
#include "mainwindow.h"

//---------------------------------------------------------------------------------
// M A I N
//---------------------------------------------------------------------------------
int main(int argc, char *argv[])
{    
    QApplication a(argc, argv);
    MainWindow w;
    w.showMaximized();
    return a.exec();
}
