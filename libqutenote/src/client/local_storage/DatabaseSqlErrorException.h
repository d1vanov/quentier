#ifndef __QUTE_NOTE__CLIENT__LOCAL_STORAGE__DATABASE_SQL_ERROR_EXCEPTION_H
#define __QUTE_NOTE__CLIENT__LOCAL_STORAGE__DATABASE_SQL_ERROR_EXCEPTION_H

#include <qute_note/exception/IQuteNoteException.h>

namespace qute_note {

class DatabaseSqlErrorException : public IQuteNoteException
{
public:
    explicit DatabaseSqlErrorException(const QString & message);

protected:
    virtual const QString exceptionDisplayName() const;
};

}

#endif // __QUTE_NOTE__CLIENT__LOCAL_STORAGE__DATABASE_SQL_ERROR_EXCEPTION_H
