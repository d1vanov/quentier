#ifndef __QUTE_NOTE__EVERNOTE_CLIENT__RESOURCE_METADATA_H
#define __QUTE_NOTE__EVERNOTE_CLIENT__RESOURCE_METADATA_H

#include "SynchronizedDataElement.h"
#include <ctime>
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
                     const std::size_t height = static_cast<std::size_t>(0),
                     const bool isAttachment = false);
    ResourceMetadata(const ResourceMetadata & other);
    ResourceMetadata & operator=(const ResourceMetadata & other);
    virtual ~ResourceMetadata() override;

    /**
     * @brief noteGuid - returns the guid of the note with which the resource is associated
     */
    const Guid noteGuid() const;

    /**
     * @brief mimeType - returns string representation of the mime tye of the resource
     */
    const QString mimeType() const;

    /**
     * @brief dataHash - returns MD5 hash of the resource
     */
    const QString dataHash() const;

    /**
     * @brief dataSize - returns the number of bytes in binary data corresponding to the resource
     */
    const std::size_t dataSize() const;

    /**
     * @brief width - if set, the displayed width of the resource in pixels, zero otherwise.
     */
    const std::size_t width() const;

    /**
     * @brief height - if set, the displayed height of the resource
     */
    const std::size_t height() const;

    bool isAttachment() const;

    time_t timestamp() const;
    void setTimestamp(const time_t timestamp);

    const QString sourceUrl() const;
    void setSourceUrl(const QString & sourceUrl);

    double altitude() const;
    void setAltitude(const double altitude);

    double latitude() const;
    void setLatitude(const double latitude);

    double longitude() const;
    void setLongitude(const double longitude);

    const QString filename() const;
    void setFilename(const QString & filename);

    virtual QTextStream & Print(QTextStream & strm) const;

private:
    ResourceMetadata() = delete;

    const QScopedPointer<ResourceMetadataPrivate> d_ptr;
    Q_DECLARE_PRIVATE(ResourceMetadata)
};

}

#endif // __QUTE_NOTE__EVERNOTE_CLIENT__RESOURCE_METADATA_H
