#ifndef __QUTE_NOTE__TOOLS_QUTE_NOTE_NULL_PTR_EXCEPTION_H
#define __QUTE_NOTE__TOOLS_QUTE_NOTE_NULL_PTR_EXCEPTION_H

#include "IQuteNoteException.h"

namespace qute_note {

class QUTE_NOTE_EXPORT QuteNoteNullPtrException: public IQuteNoteException
{
public:
    explicit QuteNoteNullPtrException(const QString & message);

protected:
    virtual const QString exceptionDisplayName() const;
};

}

#endif // __QUTE_NOTE__TOOLS_QUTE_NOTE_NULL_PTR_EXCEPTION_H
