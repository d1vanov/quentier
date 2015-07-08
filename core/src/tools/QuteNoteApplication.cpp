#include "QuteNoteApplication.h"
#include <logging/QuteNoteLogger.h>
#include <tools/SysInfo.h>
#include <exception>

namespace qute_note {

QuteNoteApplication::QuteNoteApplication(int & argc, char * argv[]) :
    QApplication(argc, argv)
{}

QuteNoteApplication::~QuteNoteApplication()
{}

bool QuteNoteApplication::notify(QObject * object, QEvent * event)
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

bool QuteNoteApplication::hasGui() const
{
    bool withGui = false;
    QApplication * app = qobject_cast<QApplication*>(QCoreApplication::instance());
    if (app) {
        withGui = (app->type() == QApplication::GuiClient);
    }

    return withGui;
}

} // namespace qute_note
