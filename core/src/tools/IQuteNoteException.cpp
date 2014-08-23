#include "IQuteNoteException.h"
#include <ostream>

namespace qute_note {

IQuteNoteException::IQuteNoteException(const QString & message) :
    m_message(message)
{}

IQuteNoteException::~IQuteNoteException()
{}

const QString IQuteNoteException::errorMessage() const
{
    return m_message;
}

#if defined(_MSC_VER)
const char * IQuteNoteException::what() const
#else
const char * IQuteNoteException::what() const noexcept
#endif
{
    return qPrintable(m_message);
}

QTextStream & IQuteNoteException::Print(QTextStream & strm) const
{
    strm << "\n" << " " << "<" << exceptionDisplayName() << ">";
    strm << "\n" << " " << " message: " << m_message;
    return strm;
}

IQuteNoteException::IQuteNoteException(const IQuteNoteException & other) :
    m_message(other.m_message)
{}

IQuteNoteException & IQuteNoteException::operator =(const IQuteNoteException & other)
{
    if (this != &other) {
        m_message = other.m_message;
    }

    return *this;
}



} // namespace qute_note
