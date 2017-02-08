#include <quentier/types/ErrorString.h>
#include "data/ErrorStringData.h"
#include <QApplication>

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

QString ErrorString::localizedString() const
{
    if (isEmpty()) {
        return QString();
    }

    QString baseStr;
    if (!d->m_base.isEmpty()) {
        baseStr = qApp->translate("", d->m_base.toLocal8Bit().constData());
    }

    QString additionalBasesStr;
    for(auto it = d->m_additionalBases.constBegin(), end = d->m_additionalBases.constEnd(); it != end; ++it)
    {
        const QString & additionalBase = *it;
        if (additionalBase.isEmpty()) {
            continue;
        }

        QString translatedStr = qApp->translate("", additionalBase.toLocal8Bit().constData());
        if (additionalBasesStr.isEmpty()) {
            additionalBasesStr = translatedStr;
        }
        else {
            additionalBasesStr += QStringLiteral(", ");
            additionalBasesStr += translatedStr;
        }
    }

    QString result = baseStr;
    if (!result.isEmpty()) {
        // Capitalize the first letter
        result = result.left(1).toUpper() + result.mid(1);
    }

    if (!result.isEmpty() && !additionalBasesStr.isEmpty()) {
        result += QStringLiteral(", ");
    }

    if (result.isEmpty())
    {
        result += additionalBasesStr;

        if (!result.isEmpty()) {
            // Capitalize the first letter
            result = result.left(1).toUpper() + result.mid(1);
        }
    }
    else
    {
        result += additionalBasesStr.toLower();
    }

    if (d->m_details.isEmpty()) {
        return result;
    }

    if (!result.isEmpty()) {
        result += QStringLiteral(": ");
        result += d->m_details.toLower();
        return result;
    }

    result += d->m_details;
    if (!result.isEmpty()) {
        // Capitalize the first letter
        result = result.left(1).toUpper() + result.mid(1);
    }

    return result;
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
