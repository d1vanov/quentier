#include "MainWindow.h"
#include "ui_MainWindow.h"

MainWindow::MainWindow(const QString & minidumpLocation,
                       const QString & symbolsFileLocation,
                       const QString & stackwalkBinaryLocation,
                       QWidget * parent) :
    QMainWindow(parent),
    m_pUi(new Ui::MainWindow)
{
    m_pUi->setupUi(this);
    setWindowTitle(tr("Quentier crashed"));

    m_pUi->minidumpFilePathLineEdit->setText(minidumpLocation);

    // TODO:
    // 1) read the first line from symbols file
    // 2) create a temporary folder and copy the symbols there - that's needed by stackwalk tool
    // 3) run stackwalk tool on unpacked symbols and minidump
    //
    // If anything goes wrong, just print the error into the field meant for the backtrace
    Q_UNUSED(symbolsFileLocation)
    Q_UNUSED(stackwalkBinaryLocation)
}

MainWindow::~MainWindow()
{
    delete m_pUi;
}
