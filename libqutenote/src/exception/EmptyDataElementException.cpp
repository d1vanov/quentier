#include <qute_note/exception/EmptyDataElementException.h>

namespace qute_note {

EmptyDataElementException::EmptyDataElementException(const QString & message) :
    IQuteNoteException(message)
{}

const QString EmptyDataElementException::exceptionDisplayName() const
{
    return QString("EmptyDataElementException");
}



}
