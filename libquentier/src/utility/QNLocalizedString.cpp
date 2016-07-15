/*
 * Copyright 2016 Dmitry Ivanov
 *
 * This file is part of libquentier
 *
 * libquentier is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * libquentier is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with libquentier. If not, see <http://www.gnu.org/licenses/>.
 */

#include <quentier/utility/QNLocalizedString.h>
#include <QApplication>

namespace quentier {

QNLocalizedString::QNLocalizedString() :
    m_localizedString(),
    m_nonLocalizedString()
{}

QNLocalizedString::QNLocalizedString(const char * nonLocalizedString, QObject * pTranslationContext,
                                     const char * disambiguation, const int plurals) :
    m_localizedString(translateString(nonLocalizedString, pTranslationContext, disambiguation, plurals)),
    m_nonLocalizedString(QString::fromUtf8(nonLocalizedString))
{}

QNLocalizedString::QNLocalizedString(const QNLocalizedString & other) :
    m_localizedString(other.m_localizedString),
    m_nonLocalizedString(other.m_nonLocalizedString)
{}

QNLocalizedString & QNLocalizedString::operator=(const QNLocalizedString & other)
{
    if (this != &other) {
        m_localizedString = other.m_localizedString;
        m_nonLocalizedString = other.m_nonLocalizedString;
    }

    return *this;
}

QNLocalizedString & QNLocalizedString::operator=(const char * nonLocalizedString)
{
    m_localizedString = QObject::tr(nonLocalizedString);
    m_nonLocalizedString = QString::fromUtf8(nonLocalizedString);
    return *this;
}

QNLocalizedString::~QNLocalizedString()
{}

bool QNLocalizedString::isEmpty() const
{
    return m_nonLocalizedString.isEmpty();
}

void QNLocalizedString::clear()
{
    m_localizedString.resize(0);
    m_nonLocalizedString.resize(0);
}

void QNLocalizedString::set(const char * nonLocalizedString, QObject * pTranslationContext,
                            const char * disambiguation, const int plurals)
{
    m_localizedString = translateString(nonLocalizedString, pTranslationContext, disambiguation, plurals);
    m_nonLocalizedString = QString::fromUtf8(nonLocalizedString);
}

QNLocalizedString & QNLocalizedString::operator+=(const QNLocalizedString & str)
{
    append(str);
    return *this;
}

QNLocalizedString & QNLocalizedString::operator+=(const char * nonLocalizedString)
{
    append(nonLocalizedString);
    return *this;
}

QNLocalizedString & QNLocalizedString::operator+=(const QString & nonTranslatableString)
{
    m_localizedString += nonTranslatableString;
    m_nonLocalizedString += nonTranslatableString;
    return *this;
}

void QNLocalizedString::append(const QNLocalizedString & str)
{
    m_localizedString += str.m_localizedString;
    m_nonLocalizedString += str.m_nonLocalizedString;
}

void QNLocalizedString::append(const QString & localizedString, const char * nonLocalizedString)
{
    m_localizedString += localizedString;
    m_nonLocalizedString += QString::fromUtf8(nonLocalizedString);
}

void QNLocalizedString::append(const char * nonLocalizedString, QObject * pTranslationContext,
                               const char * disambiguation, const int plurals)
{
    m_localizedString += translateString(nonLocalizedString, pTranslationContext, disambiguation, plurals);
    m_nonLocalizedString += QString::fromUtf8(nonLocalizedString);
}

void QNLocalizedString::prepend(const QNLocalizedString & str)
{
    m_localizedString.prepend(str.m_localizedString);
    m_nonLocalizedString.prepend(str.m_nonLocalizedString);
}

void QNLocalizedString::prepend(const QString & localizedString, const char * nonLocalizedString)
{
    m_localizedString.prepend(localizedString);
    m_nonLocalizedString.prepend(QString::fromUtf8(nonLocalizedString));
}

void QNLocalizedString::prepend(const char * nonLocalizedString, QObject * pTranslationContext,
                                const char * disambiguation, const int plurals)
{
    m_localizedString.prepend(translateString(nonLocalizedString, pTranslationContext, disambiguation, plurals));
    m_nonLocalizedString.prepend(QString::fromUtf8(nonLocalizedString));
}

QTextStream & QNLocalizedString::print(QTextStream & strm) const
{
    strm << m_nonLocalizedString;
    return strm;
}

QString QNLocalizedString::translateString(const char * nonLocalizedString, QObject * pTranslationContext,
                                           const char * disambiguation, const int plurals)
{
    const char * context = (pTranslationContext ? pTranslationContext->metaObject()->className() : "QObject");

#if QT_VERSION >= 0x050000
    return QApplication::translate(context, nonLocalizedString, disambiguation, plurals);
#else
    return QApplication::translate(context, nonLocalizedString, disambiguation, QCoreApplication::UnicodeUTF8, plurals);
#endif
}

} // namespace quentier
