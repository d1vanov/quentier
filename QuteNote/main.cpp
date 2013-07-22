#include "MainWindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow & w = MainWindow::Instance();
    // w.SetDefaultLayoutSettings();
    w.show();
    
    return a.exec();
}
