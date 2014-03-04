#ifndef __QUTE_NOTE__EVERNOTE_CLIENT__LOCAL_STORAGE__DATABASE_SQL_ERROR_EXCEPTION
#define __QUTE_NOTE__EVERNOTE_CLIENT__LOCAL_STORAGE__DATABASE_SQL_ERROR_EXCEPTION

#include <tools/IQuteNoteException.h>

namespace qute_note {

class DatabaseSqlErrorException : public IQuteNoteException
{
public:
    explicit DatabaseSqlErrorException(const QString & message);

protected:
    virtual const QString exceptionDisplayName() const;
};

}

#endif // __QUTE_NOTE__EVERNOTE_CLIENT__LOCAL_STORAGE__DATABASE_SQL_ERROR_EXCEPTION
