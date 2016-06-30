#include <quentier/exception/SharedNotebookAdapterAccessException.h>

namespace quentier {

SharedNotebookAdapterAccessException::SharedNotebookAdapterAccessException(const QString & message) :
    IQuentierException(message)
{}

const QString SharedNotebookAdapterAccessException::exceptionDisplayName() const
{
    return "SharedNotebookAdapterAccessException";
}

} // namespace quentier
