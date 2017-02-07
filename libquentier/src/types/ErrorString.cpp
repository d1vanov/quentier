#include <quentier/types/ErrorString.h>
#include "data/ErrorStringData.h"

namespace quentier {

ErrorString::ErrorString(const QString & base, const QString & details) :
    Printable(),
    d(new ErrorStringData)
{
    d->m_base = base;
    d->m_details = details;
}

ErrorString::ErrorString(const ErrorString & other) :
    Printable(),
    d(other.d)
{}

ErrorString & ErrorString::operator=(const ErrorString & other)
{
    if (this != &other) {
        d = other.d;
    }

    return *this;
}

ErrorString::~ErrorString()
{}

const QString & ErrorString::base() const
{
    return d->m_base;
}

QString & ErrorString::base()
{
    return d->m_base;
}

const QString & ErrorString::details() const
{
    return d->m_details;
}

QString & ErrorString::details()
{
    return d->m_details;
}

const QStringList & ErrorString::additionalBases() const
{
    return d->m_additionalBases;
}

QStringList & ErrorString::additionalBases()
{
    return d->m_additionalBases;
}

bool ErrorString::isEmpty() const
{
    return d->m_base.isEmpty() && d->m_details.isEmpty() && d->m_additionalBases.isEmpty();
}

void ErrorString::clear()
{
    d->m_base.clear();
    d->m_details.clear();
    d->m_additionalBases.clear();
}

QTextStream & ErrorString::print(QTextStream & strm) const
{
    strm << d->m_base;

    for(auto it = d->m_additionalBases.constBegin(), end = d->m_additionalBases.constEnd(); it != end; ++it)
    {
        QString previousStr = d->m_base;
        if (Q_LIKELY(it != d->m_additionalBases.constBegin())) {
            auto prevIt = it;
            --prevIt;
            previousStr = *prevIt;
        }

        if (Q_UNLIKELY(previousStr.isEmpty())) {
            strm << *it;
        }
        else {
            strm << QStringLiteral(", ") << *it;
        }
    }

    if (!d->m_details.isEmpty()) {
        strm << QStringLiteral(": ") << d->m_details;
    }

    return strm;
}

} // namespace quentier
