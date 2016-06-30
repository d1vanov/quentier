#include <quentier/exception/EmptyDataElementException.h>

namespace quentier {

EmptyDataElementException::EmptyDataElementException(const QString & message) :
    IQuentierException(message)
{}

const QString EmptyDataElementException::exceptionDisplayName() const
{
    return QString("EmptyDataElementException");
}



}
