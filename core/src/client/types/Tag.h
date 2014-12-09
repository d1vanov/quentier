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
    QN_DECLARE_LOCAL
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

    virtual void clear() Q_DECL_OVERRIDE;

    virtual bool hasGuid() const Q_DECL_OVERRIDE;
    virtual const QString & guid() const Q_DECL_OVERRIDE;
    virtual void setGuid(const QString & guid) Q_DECL_OVERRIDE;

    virtual bool hasUpdateSequenceNumber() const Q_DECL_OVERRIDE;
    virtual qint32 updateSequenceNumber() const Q_DECL_OVERRIDE;
    virtual void setUpdateSequenceNumber(const qint32 usn) Q_DECL_OVERRIDE;

    virtual bool checkParameters(QString & errorDescription) const Q_DECL_OVERRIDE;

    bool isDeleted() const;
    void setDeleted(const bool deleted);

    bool hasName() const;
    const QString & name() const;
    void setName(const QString & name);

    bool hasParentGuid() const;
    const QString & parentGuid() const;
    void setParentGuid(const QString & parentGuid);

private:
    virtual QTextStream & Print(QTextStream & strm) const Q_DECL_OVERRIDE;

private:
    QSharedDataPointer<TagData> d;
};

} // namespace qute_note

Q_DECLARE_METATYPE(qute_note::Tag)

#endif // __QUTE_NOTE__CLIENT__TYPES__TAG_H
