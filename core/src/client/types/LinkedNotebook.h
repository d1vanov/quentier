#ifndef __QUTE_NOTE__CLIENT__TYPES__LINKED_NOTEBOOK_H
#define __QUTE_NOTE__CLIENT__TYPES__LINKED_NOTEBOOK_H

#include "NoteStoreDataElement.h"
#include <QEverCloud.h>

namespace qute_note {

class LinkedNotebook final: public NoteStoreDataElement
{
public:
    LinkedNotebook() = default;
    LinkedNotebook(const LinkedNotebook & other) = default;
    LinkedNotebook(LinkedNotebook && other) = default;
    LinkedNotebook & operator=(const LinkedNotebook & other) = default;
    LinkedNotebook & operator=(LinkedNotebook && other) = default;

    LinkedNotebook(const qevercloud::LinkedNotebook & linkedNotebook);
    LinkedNotebook(qevercloud::LinkedNotebook && linkedNotebook);

    virtual ~LinkedNotebook() final override;

    operator qevercloud::LinkedNotebook & ();
    operator const qevercloud::LinkedNotebook & () const;

    bool operator==(const LinkedNotebook & other) const;
    bool operator!=(const LinkedNotebook & other) const;

    virtual void clear();

    virtual bool hasGuid() const final override;
    virtual const QString & guid() const final override;
    virtual void setGuid(const QString & guid) final override;

    virtual bool hasUpdateSequenceNumber() const final override;
    virtual qint32 updateSequenceNumber() const final override;
    virtual void setUpdateSequenceNumber(const qint32 usn) final override;

    virtual bool checkParameters(QString & errorDescription) const final override;

    bool hasShareName() const;
    const QString & shareName() const;
    void setShareName(const QString & shareName);

    bool hasUsername() const;
    const QString & username() const;
    void setUsername(const QString & username);

    bool hasShardId() const;
    const QString & shardId() const;
    void setShardId(const QString & shardId);

    bool hasShareKey() const;
    const QString & shareKey() const;
    void setShareKey(const QString & shareKey);

    bool hasUri() const;
    const QString & uri() const;
    void setUri(const QString & uri);

    bool hasNoteStoreUrl() const;
    const QString & noteStoreUrl() const;
    void setNoteStoreUrl(const QString & noteStoreUr);

    bool hasWebApiUrlPrefix() const;
    const QString & webApiUrlPrefix() const;
    void setWebApiUrlPrefix(const QString & webApiUrlPrefix);

    bool hasStack() const;
    const QString & stack() const;
    void setStack(const QString & stack);

    bool hasBusinessId() const;
    qint32 businessId() const;
    void setBusinessId(const qint32 businessId);

private:
    virtual QTextStream & Print(QTextStream & strm) const final override;

    qevercloud::LinkedNotebook m_qecLinkedNotebook;
};

} // namespace qute_note

#endif // __QUTE_NOTE__CLIENT__TYPES__LINKED_NOTEBOOK_H
