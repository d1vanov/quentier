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

#ifndef LIB_QUENTIER_UTILITY_PRINTABLE_H
#define LIB_QUENTIER_UTILITY_PRINTABLE_H

#include <quentier/utility/Linkage.h>
#include <quentier/utility/Qt4Helper.h>
#include <QString>
#include <QTextStream>
#include <QDebug>
#include <QHash>
#include <QSet>

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
#include <qt5qevercloud/QEverCloud.h>
#include <qt5qevercloud/QEverCloudOAuth.h>
#else
#include <qt4qevercloud/QEverCloud.h>
#include <qt4qevercloud/QEverCloudOAuth.h>
#endif

namespace quentier {

/**
 * @brief The Printable class is the interface for Quentier's internal classes
 * which should be able to write themselves into QTextStream and/or convert to QString
 */
class QUENTIER_EXPORT Printable
{
public:
    virtual QTextStream & print(QTextStream & strm) const = 0;

    virtual const QString toString() const;

    friend QUENTIER_EXPORT QTextStream & operator << (QTextStream & strm,
                                                       const Printable & printable);
    friend QUENTIER_EXPORT QDebug & operator << (QDebug & debug,
                                                  const Printable & printable);
protected:
    Printable();
    Printable(const Printable & other);
    virtual ~Printable();
};

} // namespace quentier

// printing operators for existing classes not inheriting from Printable

template <class T>
const QString ToString(const T & object)
{
    QString str;
    QTextStream strm(&str, QIODevice::WriteOnly);
    strm << object;
    return str;
}

template <class TKey, class TValue>
const QString ToString(const QHash<TKey, TValue> & object)
{
    QString str;
    QTextStream strm(&str, QIODevice::WriteOnly);
    strm << "QHash: \n";

    typedef typename QHash<TKey,TValue>::const_iterator CIter;
    CIter hashEnd = object.end();
    for(CIter it = object.begin(); it != hashEnd; ++it) {
        strm << "[" << it.key() << "] = " << it.value() << ";\n";
    }
    return str;
}

template <class T>
const QString ToString(const QSet<T> & object)
{
    QString str;
    QTextStream strm(&str, QIODevice::WriteOnly);
    strm << "QSet: \n";

    typedef typename QSet<T>::const_iterator CIter;
    CIter setEnd = object.end();
    for(CIter it = object.begin(); it != setEnd; ++it) {
        strm << "[" << *it << "];\n";
    }
    return str;
}

#define QUENTIER_DECLARE_PRINTABLE(type, ...) \
    QTextStream & operator << (QTextStream & strm, const type & obj); \
    inline QDebug & operator << (QDebug & debug, const type & obj) \
    { \
        debug << ToString<type, ##__VA_ARGS__>(obj); \
        return debug; \
    }

QUENTIER_DECLARE_PRINTABLE(qevercloud::BusinessUserInfo)
QUENTIER_DECLARE_PRINTABLE(qevercloud::PremiumInfo)
QUENTIER_DECLARE_PRINTABLE(qevercloud::Accounting)
QUENTIER_DECLARE_PRINTABLE(qevercloud::UserAttributes)
QUENTIER_DECLARE_PRINTABLE(qevercloud::NoteAttributes)
QUENTIER_DECLARE_PRINTABLE(qevercloud::PrivilegeLevel::type)
QUENTIER_DECLARE_PRINTABLE(qevercloud::QueryFormat::type)
QUENTIER_DECLARE_PRINTABLE(qevercloud::SharedNotebookPrivilegeLevel::type)
QUENTIER_DECLARE_PRINTABLE(qevercloud::NoteSortOrder::type)
QUENTIER_DECLARE_PRINTABLE(qevercloud::NotebookRestrictions)
QUENTIER_DECLARE_PRINTABLE(qevercloud::SharedNotebookInstanceRestrictions::type)
QUENTIER_DECLARE_PRINTABLE(qevercloud::ResourceAttributes)
QUENTIER_DECLARE_PRINTABLE(qevercloud::Resource)
QUENTIER_DECLARE_PRINTABLE(qevercloud::SyncChunk)
QUENTIER_DECLARE_PRINTABLE(qevercloud::Tag)
QUENTIER_DECLARE_PRINTABLE(qevercloud::SavedSearch)
QUENTIER_DECLARE_PRINTABLE(qevercloud::LinkedNotebook)
QUENTIER_DECLARE_PRINTABLE(qevercloud::Notebook)
QUENTIER_DECLARE_PRINTABLE(qevercloud::Publishing)
QUENTIER_DECLARE_PRINTABLE(qevercloud::SharedNotebook)
QUENTIER_DECLARE_PRINTABLE(qevercloud::BusinessNotebook)
QUENTIER_DECLARE_PRINTABLE(qevercloud::User)
QUENTIER_DECLARE_PRINTABLE(qevercloud::SharedNotebookRecipientSettings)
QUENTIER_DECLARE_PRINTABLE(qevercloud::ReminderEmailConfig::type)
QUENTIER_DECLARE_PRINTABLE(qevercloud::PremiumOrderStatus::type)
QUENTIER_DECLARE_PRINTABLE(qevercloud::BusinessUserRole::type)
QUENTIER_DECLARE_PRINTABLE(qevercloud::SponsoredGroupRole::type)
QUENTIER_DECLARE_PRINTABLE(qevercloud::Note)
QUENTIER_DECLARE_PRINTABLE(qevercloud::EDAMErrorCode::type)
QUENTIER_DECLARE_PRINTABLE(qevercloud::EvernoteOAuthWebView::OAuthResult)

#endif // LIB_QUENTIER_UTILITY_PRINTABLE_H
