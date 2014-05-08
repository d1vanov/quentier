#ifndef __QUTE_NOTE__CLIENT__TYPES__TAG_H
#define __QUTE_NOTE__CLIENT__TYPES__TAG_H

#include "NoteStoreDataElement.h"
#include <QEverCloud.h>

namespace qute_note {

class Tag final: public NoteStoreDataElement
{
public:
    Tag();
    Tag(const Tag & other) = default;
    Tag(Tag && other) = default;
    Tag & operator=(const Tag & other) = default;
    Tag & operator=(Tag && other) = default;

    Tag(const qevercloud::Tag & other);
    Tag(qevercloud::Tag && other);

    virtual ~Tag() final override;

    bool operator==(const Tag & other) const;
    bool operator!=(const Tag & other) const;

    virtual void clear() final override;

    virtual bool hasGuid() const final override;
    virtual const QString & guid() const final override;
    virtual void setGuid(const QString & guid) final override;

    virtual bool hasUpdateSequenceNumber() const final override;
    virtual qint32 updateSequenceNumber() const final override;
    virtual void setUpdateSequenceNumber(const qint32 usn) final override;

    virtual bool checkParameters(QString & errorDescription) const final override;

    bool isLocal() const;
    void setLocal(const bool local);

    bool isDeleted() const;
    void setDeleted(const bool deleted);

    bool hasName() const;
    const QString & name() const;
    void setName(const QString & name);

    bool hasParentGuid() const;
    const QString & parentGuid() const;
    void setParentGuid(const QString & parentGuid);

private:
    virtual QTextStream & Print(QTextStream & strm) const final override;

private:
    qevercloud::Tag m_qecTag;
    bool m_isLocal;
    bool m_isDeleted;
};

} // namespace qute_note

#endif // __QUTE_NOTE__CLIENT__TYPES__TAG_H
