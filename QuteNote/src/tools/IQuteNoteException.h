#ifndef __QUTE_NOTE__TOOLS_IQUTE_NOTE_EXCEPTION_H
#define __QUTE_NOTE__TOOLS_IQUTE_NOTE_EXCEPTION_H

#include "Printable.h"

namespace qute_note {

class IQuteNoteException: public Printable
{
public:
    explicit IQuteNoteException(const QString & message);
    virtual ~IQuteNoteException();

    const QString errorMessage() const;

    virtual QTextStream & Print(QTextStream & strm) const;

protected:
    IQuteNoteException(const IQuteNoteException & other);
    IQuteNoteException & operator =(const IQuteNoteException & other);

    virtual const QString exceptionDisplayName() const = 0;

private:
    IQuteNoteException() = delete;

    QString m_message;
};

} // namespace qute_note

#endif // __QUTE_NOTE__TOOLS_IQUTE_NOTE_EXCEPTION_H
 
