#ifndef __QUTE_NOTE__CLIENT__TYPES__SAVED_SEARCH_H
#define __QUTE_NOTE__CLIENT__TYPES__SAVED_SEARCH_H

#include "NoteStoreDataElement.h"
#include <Types_types.h>

namespace qute_note {

class SavedSearch final: public NoteStoreDataElement
{
public:
    SavedSearch();
    SavedSearch(const SavedSearch & other);
    SavedSearch & operator=(const SavedSearch & other);
    virtual ~SavedSearch() final override;

    virtual void clear() final override;

    virtual bool hasGuid() const final override;
    virtual const QString guid() const final override;
    virtual void setGuid(const QString & guid) final override;

    virtual bool hasUpdateSequenceNumber() const final override;
    virtual qint32 updateSequenceNumber() const final override;
    virtual void setUpdateSequenceNumber(const qint32 usn) final override;

    virtual bool checkParameters(QString & errorDescription) const final override;

    bool hasName() const;
    const QString name() const;
    void setName(const QString & name);

    bool hasQuery() const;
    const QString query() const;
    void setQuery(const QString & query);

    bool hasQueryFormat() const;
    qint8 queryFormat() const;
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
    evernote::edam::SavedSearch m_enSearch;
};

} // namespace qute_note

#endif // __QUTE_NOTE__CLIENT__TYPES__SAVED_SEARCH_H
