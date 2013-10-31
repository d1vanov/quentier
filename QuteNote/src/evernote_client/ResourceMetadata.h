#ifndef __QUTE_NOTE__EVERNOTE_CLIENT__RESOURCE_METADATA_H
#define __QUTE_NOTE__EVERNOTE_CLIENT__RESOURCE_METADATA_H

#include "Resource.h"

namespace qute_note {

/**
 * @brief The ResourceMetadata class is a mere placeholder for metadata of
 * resource object which doesn't contain the actual binary data of the resource
 * to prevent unnecessary copying of it when you, for example, need to get hashes
 * for all resources attached to a particular note.
 *
 * WARNING: user of this class must be careful about the state of the resource. Once
 * the latter one is deleted, the corresponding metadata holder immediately becomes
 * invalid.
 *
 * NOTE: ResourceMetadata contains a const reference to the Resource object
 * which means, one can access the resource through its metadata. But the keyword
 * here is 'const'. One cannot change the resource via its metadata object.
 */
class ResourceMetadata
{
public:
    ResourceMetadata(const Resource & resource);
    ResourceMetadata(const ResourceMetadata & other);

    const Resource & resource() const;

    /**
     * @brief dataHash - returns the MD5-hash of binary data corresponding
     * to the resource
     */
    const QString dataHash() const;

    /**
     * @brief dataSize - the number of bytes in binary data corresponding
     * to the resource
     */
    std::size_t dataSize() const;

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
    ResourceMetadata() = delete;    // resource metadata cannot exist without the actual resource
    ResourceMetadata & operator=(const ResourceMetadata & other) = delete;

    const Resource & m_resource;
};

}

#endif // __QUTE_NOTE__EVERNOTE_CLIENT__RESOURCE_METADATA_H
