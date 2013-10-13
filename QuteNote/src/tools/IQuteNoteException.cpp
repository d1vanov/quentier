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

std::ostream & IQuteNoteException::Print(std::ostream & strm) const 
{
    strm << "\n" << " " << "<" << exceptionDisplayName().toStdString() << ">";
    strm << "\n" << " " << " message: " << m_message.toStdString();
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
