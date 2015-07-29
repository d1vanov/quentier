#ifndef __LIB_QUTE_NOTE__EXCEPTION__IQUTE_NOTE_EXCEPTION_H
#define __LIB_QUTE_NOTE__EXCEPTION__IQUTE_NOTE_EXCEPTION_H

#include <tools/Printable.h>
#include <exception>

namespace qute_note {

class QUTE_NOTE_EXPORT IQuteNoteException: public Printable,
                                           public std::exception
{
public:
    explicit IQuteNoteException(const QString & message);

#ifdef _MSC_VER
    virtual ~IQuteNoteException();
#elif __cplusplus >= 201103L
    virtual ~IQuteNoteException() noexcept;
#else
    virtual ~IQuteNoteException() throw();
#endif

    const QString errorMessage() const;

#ifdef _MSC_VER
    virtual const char * what() const Q_DECL_OVERRIDE;
#elif __cplusplus >= 201103L
    virtual const char * what() const noexcept Q_DECL_OVERRIDE;
#else
    virtual const char * what() const Q_DECL_OVERRIDE throw();
#endif

    virtual QTextStream & Print(QTextStream & strm) const;

protected:
    IQuteNoteException(const IQuteNoteException & other);
    IQuteNoteException & operator =(const IQuteNoteException & other);

    virtual const QString exceptionDisplayName() const = 0;

private:
    IQuteNoteException() Q_DECL_DELETE;

    QString m_message;
};

} // namespace qute_note

#endif // __LIB_QUTE_NOTE__EXCEPTION__IQUTE_NOTE_EXCEPTION_H

