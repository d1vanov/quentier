#include <quentier/exception/IQuentierException.h>
#include <ostream>

namespace quentier {

IQuentierException::IQuentierException(const QString & message) :
    Printable(),
    m_message(message)
{}

#ifdef _MSC_VER
IQuentierException::~IQuentierException()
#elif __cplusplus >= 201103L
IQuentierException::~IQuentierException() noexcept
#else
IQuentierException::~IQuentierException() throw()
#endif
{}

const QString IQuentierException::errorMessage() const
{
    return m_message;
}

#if defined(_MSC_VER)
const char * IQuentierException::what() const
#elif __cplusplus >= 201103L
const char * IQuentierException::what() const noexcept
#else
const char * IQuentierException::what() const throw()
#endif
{
    return qPrintable(m_message);
}

QTextStream & IQuentierException::print(QTextStream & strm) const
{
    strm << "\n" << " " << "<" << exceptionDisplayName() << ">";
    strm << "\n" << " " << " message: " << m_message;
    return strm;
}

IQuentierException::IQuentierException(const IQuentierException & other) :
    Printable(other),
    m_message(other.m_message)
{}

IQuentierException & IQuentierException::operator =(const IQuentierException & other)
{
    if (this != &other) {
        m_message = other.m_message;
    }

    return *this;
}



} // namespace quentier
