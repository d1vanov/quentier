#include <quentier/utility/QNLocalizedString.h>

namespace quentier {

QNLocalizedString QNLocalizedString::create(const char * str, QObject * trSource)
{
    QNLocalizedString localizedString((trSource ? trSource->tr(str) : QObject::tr(str)), str);
    return localizedString;
}

QNLocalizedString::QNLocalizedString() :
    QString(),
    m_nonLocalizedStr(Q_NULLPTR)
{}

QNLocalizedString::QNLocalizedString(const QNLocalizedString & other) :
    QString(other),
    m_nonLocalizedStr(other.m_nonLocalizedStr)
{}

QNLocalizedString::QNLocalizedString(const char * str) :
    QString(QObject::tr(str)),
    m_nonLocalizedStr(str)
{}

QNLocalizedString & QNLocalizedString::operator=(const QNLocalizedString & other)
{
    if (this != &other) {
        QString::operator=(other);
        m_nonLocalizedStr = other.m_nonLocalizedStr;
    }

    return *this;
}

QNLocalizedString::~QNLocalizedString()
{}

QTextStream & QNLocalizedString::print(QTextStream & strm) const
{
    strm << "QNLocalizedString { \"" << (m_nonLocalizedStr ? m_nonLocalizedStr : "<null>") << "\" };";
    return strm;
}

QNLocalizedString::QNLocalizedString(const QString & localizedString, const char * nonLocalizedString) :
    QString(localizedString),
    m_nonLocalizedStr(nonLocalizedString)
{}

} // namespace quentier
