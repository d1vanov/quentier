#ifndef __QUTE_NOTE__TOOLS_IQUTE_NOTE_EXCEPTION_H
#define __QUTE_NOTE__TOOLS_IQUTE_NOTE_EXCEPTION_H

#include "Printable.h"
#include <exception>

namespace qute_note {

class QUTE_NOTE_EXPORT IQuteNoteException: public Printable,
                                           public std::exception
{
public:
    explicit IQuteNoteException(const QString & message);
    virtual ~IQuteNoteException();

    const QString errorMessage() const;
#ifdef _MSC_VER
    virtual const char * what() const Q_DECL_OVERRIDE;
#else
    virtual const char * what() const noexcept Q_DECL_OVERRIDE;
#endif

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
 
