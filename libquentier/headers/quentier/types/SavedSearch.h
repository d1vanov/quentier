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

#ifndef LIB_QUENTIER_TYPES_SAVED_SEARCH_H
#define LIB_QUENTIER_TYPES_SAVED_SEARCH_H

#include "IFavoritableDataElement.h"
#include <QEverCloud.h>
#include <QSharedDataPointer>

namespace quentier {

QT_FORWARD_DECLARE_CLASS(SavedSearchData)

class QUENTIER_EXPORT SavedSearch: public IFavoritableDataElement
{
public:
    QN_DECLARE_LOCAL_UID
    QN_DECLARE_DIRTY
    QN_DECLARE_LOCAL
    QN_DECLARE_FAVORITED

public:
    typedef qevercloud::QueryFormat::type QueryFormat;
    typedef qevercloud::SavedSearchScope  SavedSearchScope;

public:
    SavedSearch();
    SavedSearch(const SavedSearch & other);
    SavedSearch(SavedSearch && other);
    SavedSearch & operator=(const SavedSearch & other);
    SavedSearch & operator=(SavedSearch && other);

    SavedSearch(const qevercloud::SavedSearch & search);
    SavedSearch(qevercloud::SavedSearch && search);

    virtual ~SavedSearch();

    operator qevercloud::SavedSearch & ();
    operator const qevercloud::SavedSearch & () const;

    bool operator==(const SavedSearch & other) const;
    bool operator!=(const SavedSearch & other) const;

    virtual void clear() Q_DECL_OVERRIDE;

    static bool validateName(const QString & name, QNLocalizedString * pErrorDescription = Q_NULLPTR);

    virtual bool hasGuid() const Q_DECL_OVERRIDE;
    virtual const QString & guid() const Q_DECL_OVERRIDE;
    virtual void setGuid(const QString & guid) Q_DECL_OVERRIDE;

    virtual bool hasUpdateSequenceNumber() const Q_DECL_OVERRIDE;
    virtual qint32 updateSequenceNumber() const Q_DECL_OVERRIDE;
    virtual void setUpdateSequenceNumber(const qint32 usn) Q_DECL_OVERRIDE;

    virtual bool checkParameters(QNLocalizedString & errorDescription) const Q_DECL_OVERRIDE;

    bool hasName() const;
    const QString & name() const;
    void setName(const QString & name);

    bool hasQuery() const;
    const QString & query() const;
    void setQuery(const QString & query);

    bool hasQueryFormat() const;
    QueryFormat queryFormat() const;
    void setQueryFormat(const qint8 queryFormat);

    bool hasIncludeAccount() const;
    bool includeAccount() const;
    void setIncludeAccount(const bool includeAccount);

    bool hasIncludePersonalLinkedNotebooks() const;
    bool includePersonalLinkedNotebooks() const;
    void setIncludePersonalLinkedNotebooks(const bool includePersonalLinkedNotebooks);

    bool hasIncludeBusinessLinkedNotebooks() const;
    bool includeBusinessLinkedNotebooks() const;
    void setIncludeBusinessLinkedNotebooks(const bool includeBusinessLinkedNotebooks);

    virtual QTextStream & print(QTextStream & strm) const Q_DECL_OVERRIDE;

private:
    QSharedDataPointer<SavedSearchData> d;
};

} // namespace quentier

Q_DECLARE_METATYPE(quentier::SavedSearch)

#endif // LIB_QUENTIER_TYPES_SAVED_SEARCH_H
