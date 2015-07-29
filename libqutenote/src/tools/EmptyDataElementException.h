#ifndef __QUTE_NOTE__TOOLS__EMPTY_DATA_ELEMENT_EXCEPTION_H
#define __QUTE_NOTE__TOOLS__EMPTY_DATA_ELEMENT_EXCEPTION_H

#include <qute_note/utility/IQuteNoteException.h>

namespace qute_note {

class EmptyDataElementException : public IQuteNoteException
{
public:
    explicit EmptyDataElementException(const QString & message);

protected:
    virtual const QString exceptionDisplayName() const;
};

}

#endif // __QUTE_NOTE__TOOLS__EMPTY_DATA_ELEMENT_EXCEPTION_H
