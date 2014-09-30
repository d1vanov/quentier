#ifndef __QUTE_NOTE__CLIENT__TYPES__TAG_H
#define __QUTE_NOTE__CLIENT__TYPES__TAG_H

#include "IDataElementWithShortcut.h"
#include <QEverCloud.h>
#include <QSharedDataPointer>

namespace qute_note {

QT_FORWARD_DECLARE_CLASS(TagData)

class QUTE_NOTE_EXPORT Tag: public IDataElementWithShortcut
{
public:
    QN_DECLARE_LOCAL_GUID
    QN_DECLARE_DIRTY
    QN_DECLARE_SHORTCUT

public:
    Tag();
    Tag(const Tag & other);
    Tag(Tag && other);
    Tag & operator=(const Tag & other);
    Tag & operator=(Tag && other);

    Tag(const qevercloud::Tag & other);
    Tag(qevercloud::Tag && other);

    virtual ~Tag();

    bool operator==(const Tag & other) const;
    bool operator!=(const Tag & other) const;

    virtual void clear() override;

    virtual bool hasGuid() const override;
    virtual const QString & guid() const override;
    virtual void setGuid(const QString & guid) override;

    virtual bool hasUpdateSequenceNumber() const override;
    virtual qint32 updateSequenceNumber() const override;
    virtual void setUpdateSequenceNumber(const qint32 usn) override;

    virtual bool checkParameters(QString & errorDescription) const override;

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
    virtual QTextStream & Print(QTextStream & strm) const override;

private:
    QSharedDataPointer<TagData> d;
};

} // namespace qute_note

#endif // __QUTE_NOTE__CLIENT__TYPES__TAG_H
