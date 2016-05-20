#ifndef LIB_QUTE_NOTE_EXCEPTION_DATABASE_SQL_ERROR_EXCEPTION_H
#define LIB_QUTE_NOTE_EXCEPTION_DATABASE_SQL_ERROR_EXCEPTION_H

#include <qute_note/exception/IQuteNoteException.h>

namespace qute_note {

class QUTE_NOTE_EXPORT DatabaseSqlErrorException: public IQuteNoteException
{
public:
    explicit DatabaseSqlErrorException(const QString & message);

protected:
    virtual const QString exceptionDisplayName() const Q_DECL_OVERRIDE;
};

}

#endif // LIB_QUTE_NOTE_EXCEPTION_DATABASE_SQL_ERROR_EXCEPTION_H
