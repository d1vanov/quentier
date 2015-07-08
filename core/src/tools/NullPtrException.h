#ifndef __QUTE_NOTE__TOOLS__NULL_PTR_EXCEPTION_H
#define __QUTE_NOTE__TOOLS__NULL_PTR_EXCEPTION_H

#include "IQuteNoteException.h"

namespace qute_note {

class QUTE_NOTE_EXPORT NullPtrException: public IQuteNoteException
{
public:
    explicit NullPtrException(const QString & message);

protected:
    virtual const QString exceptionDisplayName() const;
};

}

#endif // __QUTE_NOTE__TOOLS__NULL_PTR_EXCEPTION_H
