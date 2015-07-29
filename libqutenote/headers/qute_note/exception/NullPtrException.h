#ifndef __LIB_QUTE_NOTE__EXCEPTION__NULL_PTR_EXCEPTION_H
#define __LIB_QUTE_NOTE__EXCEPTION__NULL_PTR_EXCEPTION_H

#include <qute_note/exception/IQuteNoteException.h>

namespace qute_note {

class QUTE_NOTE_EXPORT NullPtrException: public IQuteNoteException
{
public:
    explicit NullPtrException(const QString & message);

protected:
    virtual const QString exceptionDisplayName() const;
};

}

#endif // __LIB_QUTE_NOTE__EXCEPTION__NULL_PTR_EXCEPTION_H
