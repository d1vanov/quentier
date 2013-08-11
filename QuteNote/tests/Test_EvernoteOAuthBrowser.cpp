#include "../src/gui/EvernoteOAuthBrowser.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    EvernoteOAuthBrowser * pBrowser = new EvernoteOAuthBrowser;
    pBrowser->load(QUrl("http://www.google.ru"));
    pBrowser->show();

    return a.exec();
}
