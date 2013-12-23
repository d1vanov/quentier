#ifndef __QUTE_NOTE__EVERNOTE_CLIENT__TAG_H
#define __QUTE_NOTE__EVERNOTE_CLIENT__TAG_H

#include "../tools/TypeWithError.h"
#include "SynchronizedDataElement.h"
#include <QScopedPointer>

namespace qute_note {

QT_FORWARD_DECLARE_CLASS(Guid)
QT_FORWARD_DECLARE_CLASS(INoteStore)

QT_FORWARD_DECLARE_CLASS(TagPrivate)
class Tag: public TypeWithError,
           public SynchronizedDataElement
{
public:
    Tag Create(const QString & name, INoteStore & noteStore, const Tag * parent = nullptr);

    Tag(const Tag & other);
    Tag & operator=(const Tag & other);
    bool operator==(const Tag & other) const;
    virtual ~Tag();

    virtual bool isEmpty() const;

    /**
     * @brief parentGuid - guid of parent tag
     * @return guid of parent tag if it exists or empty guid otherwise
     */
    const Guid parentGuid() const;

    /**
     * @brief name - name of tag
     */
    const QString name() const;

    void setParent(const Tag & parent);
    void rename(const QString & name);

private:
    Tag(const QString & name, const Tag * parent = nullptr);
    Tag() = delete;

    const QScopedPointer<TagPrivate> d_ptr;
    Q_DECLARE_PRIVATE(Tag)
};

}

#endif // __QUTE_NOTE__EVERNOTE_CLIENT__TAG_H
