#include "DatabaseOpeningException.h"

namespace qute_note {

DatabaseOpeningException::DatabaseOpeningException(const QString & message) :
    IQuteNoteException(message)
{}

const QString DatabaseOpeningException::exceptionDisplayName() const
{
    return QString("DatabaseOpeningException");
}

}
