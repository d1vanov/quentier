#ifndef __QUTE_NOTE__EVERNOTE_CLIENT__TAG_H
#define __QUTE_NOTE__EVERNOTE_CLIENT__TAG_H

#include "../tools/TypeWithError.h"
#include "SynchronizedDataElement.h"
#include <QScopedPointer>

namespace qute_note {

QT_FORWARD_DECLARE_CLASS(Guid)
QT_FORWARD_DECLARE_CLASS(INoteStore)

QT_FORWARD_DECLARE_CLASS(TagPrivate)
class Tag final: public TypeWithError,
                 public SynchronizedDataElement
{
public:
    Tag Create(const QString & name, INoteStore & noteStore, const Tag * parent = nullptr);

    Tag(const Tag & other);
    Tag(Tag && other);
    Tag & operator=(const Tag & other);
    Tag & operator=(Tag && other);
    virtual ~Tag() override;

    bool operator==(const Tag & other) const;

    virtual void Clear() override;
    virtual bool isEmpty() const override;

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

    QScopedPointer<TagPrivate> d_ptr;
    Q_DECLARE_PRIVATE(Tag)
};

} // namespace qute_note

#endif // __QUTE_NOTE__EVERNOTE_CLIENT__TAG_H
