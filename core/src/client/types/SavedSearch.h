#ifndef __QUTE_NOTE__CLIENT__TYPES__SAVED_SEARCH_H
#define __QUTE_NOTE__CLIENT__TYPES__SAVED_SEARCH_H

#include "NoteStoreDataElement.h"
#include <QEverCloud.h>

namespace qute_note {

class SavedSearch final: public NoteStoreDataElement
{
public:
    typedef qevercloud::QueryFormat::type QueryFormat;
    typedef qevercloud::SavedSearchScope  SavedSearchScope;

public:
    SavedSearch() = default;
    SavedSearch(const SavedSearch & other) = default;
    SavedSearch(SavedSearch && other) = default;
    SavedSearch & operator=(const SavedSearch & other) = default;
    SavedSearch & operator=(SavedSearch && other) = default;

    SavedSearch(const qevercloud::SavedSearch & search);
    SavedSearch(qevercloud::SavedSearch && search);

    virtual ~SavedSearch() final override;

    operator qevercloud::SavedSearch & ();
    operator const qevercloud::SavedSearch & () const;

    bool operator==(const SavedSearch & other) const;
    bool operator!=(const SavedSearch & other) const;

    virtual void clear() final override;

    virtual bool hasGuid() const final override;
    virtual const QString guid() const final override;
    virtual void setGuid(const QString & guid) final override;

    virtual bool hasUpdateSequenceNumber() const final override;
    virtual qint32 updateSequenceNumber() const final override;
    virtual void setUpdateSequenceNumber(const qint32 usn) final override;

    virtual bool checkParameters(QString & errorDescription) const final override;

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

private:
    virtual QTextStream & Print(QTextStream & strm) const;

private:
    qevercloud::SavedSearch m_qecSearch;
};

} // namespace qute_note

#endif // __QUTE_NOTE__CLIENT__TYPES__SAVED_SEARCH_H
