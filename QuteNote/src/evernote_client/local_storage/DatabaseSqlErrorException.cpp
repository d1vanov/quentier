#include "DatabaseSqlErrorException.h"

namespace qute_note {

DatabaseSqlErrorException::DatabaseSqlErrorException(const QString & message) :
    IQuteNoteException(message)
{}

const QString DatabaseSqlErrorException::exceptionDisplayName() const
{
    return QString("DatabaseSqlErrorException");
}

}
