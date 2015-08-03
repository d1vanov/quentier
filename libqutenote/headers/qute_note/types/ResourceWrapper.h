#ifndef __LIB_QUTE_NOTE__TYPES__RESOURCE_WRAPPER_H
#define __LIB_QUTE_NOTE__TYPES__RESOURCE_WRAPPER_H

#include "IResource.h"
#include <QSharedDataPointer>

namespace qute_note {

QT_FORWARD_DECLARE_CLASS(ResourceWrapperData)

/**
 * @brief The ResourceWrapper class creates and manages its own instance of
 * qevercloud::Resource object
 */
class QUTE_NOTE_EXPORT ResourceWrapper: public IResource
{
public:
    ResourceWrapper();
    ResourceWrapper(const IResource & other);
    ResourceWrapper(const ResourceWrapper & other);
    ResourceWrapper(ResourceWrapper && other);
    ResourceWrapper(qevercloud::Resource && other);
    ResourceWrapper(const qevercloud::Resource & other);
    ResourceWrapper & operator=(const ResourceWrapper & other);
    ResourceWrapper & operator=(ResourceWrapper && other);
    virtual ~ResourceWrapper();

    friend class Note;

private:
    virtual const qevercloud::Resource & GetEnResource() const Q_DECL_OVERRIDE;
    virtual qevercloud::Resource & GetEnResource() Q_DECL_OVERRIDE;

    virtual QTextStream & Print(QTextStream & strm) const;

    QSharedDataPointer<ResourceWrapperData> d;
};

} // namespace qute_note

Q_DECLARE_METATYPE(qute_note::ResourceWrapper)

#endif // __LIB_QUTE_NOTE__TYPES__RESOURCE_WRAPPER_H
