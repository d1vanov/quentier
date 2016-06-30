#include <quentier/utility/QuentierApplication.h>
#include <quentier/logging/QuentierLogger.h>
#include <quentier/utility/SysInfo.h>
#include <exception>

namespace quentier {

QuentierApplication::QuentierApplication(int & argc, char * argv[]) :
    QApplication(argc, argv)
{}

QuentierApplication::~QuentierApplication()
{}

bool QuentierApplication::notify(QObject * object, QEvent * event)
{
    try
    {
        return QApplication::notify(object, event);
    }
    catch(const std::exception & e)
    {
        QNCRITICAL("Caught unhandled properly exception: " << e.what()
                   << ", backtrace: " << SysInfo::GetSingleton().GetStackTrace());
        return false;
    }
}

} // namespace quentier
