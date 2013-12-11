#ifndef __QUTE_NOTE__EVERNOTE_CLIENT__TAG_H
#define __QUTE_NOTE__EVERNOTE_CLIENT__TAG_H

#include "../tools/TypeWithError.h"
#include "SynchronizedDataElement.h"
#include <QScopedPointer>

namespace qute_note {

class Guid;

class TagPrivate;
class Tag: public TypeWithError,
           public SynchronizedDataElement
{
public:
    Tag(const QString & name);
    Tag(const QString & name, const Tag & parent);
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
    Tag() = delete;

    const QScopedPointer<TagPrivate> d_ptr;
    Q_DECLARE_PRIVATE(Tag)
};

}

#endif // __QUTE_NOTE__EVERNOTE_CLIENT__TAG_H
