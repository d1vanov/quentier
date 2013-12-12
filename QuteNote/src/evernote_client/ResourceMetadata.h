#ifndef __QUTE_NOTE__EVERNOTE_CLIENT__RESOURCE_METADATA_H
#define __QUTE_NOTE__EVERNOTE_CLIENT__RESOURCE_METADATA_H

#include "SynchronizedDataElement.h"
#include <QScopedPointer>

namespace qute_note {

class Note;

class ResourceMetadataPrivate;
class ResourceMetadata final: public SynchronizedDataElement
{
public:
    ResourceMetadata(const Note & note, const QString & resourceMimeType,
                     const QString & dataHash, const std::size_t dataSize,
                     const std::size_t width = static_cast<std::size_t>(0),
                     const std::size_t height = static_cast<std::size_t>(0));
    ResourceMetadata(const ResourceMetadata & other);
    ResourceMetadata & operator=(const ResourceMetadata & other);
    virtual ~ResourceMetadata();

    /**
     * @brief noteGuid - returns the guid of the note with which the resource is associated
     */
    const Guid noteGuid() const;

    /**
     * @brief mimeType - returns string representation of the mime tye of the resource
     */
    const QString mimeType() const;

    /**
     * @brief width - if set, the displayed width of the resource in pixels, zero otherwise.
     */
    const std::size_t width() const;

    /**
     * @brief height - if set, the displayed height of the resource
     */
    const std::size_t height() const;

private:
    ResourceMetadata() = delete;

    const QScopedPointer<ResourceMetadataPrivate> d_ptr;
    Q_DECLARE_PRIVATE(ResourceMetadata)
};

}

#endif // __QUTE_NOTE__EVERNOTE_CLIENT__RESOURCE_METADATA_H
