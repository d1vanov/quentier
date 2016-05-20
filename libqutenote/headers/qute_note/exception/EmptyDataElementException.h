#ifndef LIB_QUTE_NOTE_EXCEPTION_EMPTY_DATA_ELEMENT_EXCEPTION_H
#define LIB_QUTE_NOTE_EXCEPTION_EMPTY_DATA_ELEMENT_EXCEPTION_H

#include <qute_note/exception/IQuteNoteException.h>

namespace qute_note {

class QUTE_NOTE_EXPORT EmptyDataElementException: public IQuteNoteException
{
public:
    explicit EmptyDataElementException(const QString & message);

protected:
    virtual const QString exceptionDisplayName() const Q_DECL_OVERRIDE;
};

}

#endif // LIB_QUTE_NOTE_EXCEPTION_EMPTY_DATA_ELEMENT_EXCEPTION_H
