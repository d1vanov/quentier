#ifndef __QUTE_NOTE__LOCAL_STORAGE__DATABASE_OPENING_EXCEPTION_H
#define __QUTE_NOTE__LOCAL_STORAGE__DATABASE_OPENING_EXCEPTION_H

#include "../tools/IQuteNoteException.h"

namespace qute_note {

class DatabaseOpeningException : public IQuteNoteException
{
public:
    explicit DatabaseOpeningException(const QString & message);

protected:
    virtual const QString exceptionDisplayName() const;
};

}

#endif // __QUTE_NOTE__LOCAL_STORAGE__DATABASE_OPENING_EXCEPTION_H
