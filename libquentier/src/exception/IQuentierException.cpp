#include <quentier/exception/IQuentierException.h>
#include <ostream>

namespace quentier {

IQuentierException::IQuentierException(const QNLocalizedString & message) :
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

QString IQuentierException::localizedErrorMessage() const
{
    return m_message.localizedString();
}

QString IQuentierException::nonLocalizedErrorMessage() const
{
    return m_message.nonLocalizedString();
}

#if defined(_MSC_VER)
const char * IQuentierException::what() const
#elif __cplusplus >= 201103L
const char * IQuentierException::what() const noexcept
#else
const char * IQuentierException::what() const throw()
#endif
{
    return qPrintable(m_message.nonLocalizedString());
}

QTextStream & IQuentierException::print(QTextStream & strm) const
{
    strm << "\n" << " " << "<" << exceptionDisplayName() << ">";
    strm << "\n" << " " << " message: " << m_message.nonLocalizedString();
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
