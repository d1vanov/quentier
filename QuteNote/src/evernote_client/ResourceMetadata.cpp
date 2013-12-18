#include "ResourceMetadata.h"
#include "Note.h"
#include "../tools/EmptyDataElementException.h"
#include "../evernote_client_private/Location.h"

namespace qute_note {

class ResourceMetadataPrivate
{
public:
    ResourceMetadataPrivate(const Note & note, const QString & resourceMimeType,
                            const QString & dataHash, const std::size_t dataSize,
                            const std::size_t width, const std::size_t height,
                            const bool isAttachment);
    ResourceMetadataPrivate(const ResourceMetadataPrivate & other);
    ResourceMetadataPrivate & operator=(const ResourceMetadataPrivate & other);

    Guid         m_noteGuid;
    QString      m_resourceMimeType;
    QString      m_resourceDataHash;
    std::size_t  m_resourceDataSize;
    std::size_t  m_resourceDisplayWidth;
    std::size_t  m_resourceDisplayHeight;
    bool         m_isAttachment;
    time_t       m_timestamp;
    QString      m_sourceUrl;
    Location     m_location;
    QString      m_filename;

private:
    ResourceMetadataPrivate() = delete;
};

ResourceMetadata::ResourceMetadata(const Note & note, const QString & resourceMimeType,
                                   const QString & dataHash, const std::size_t dataSize,
                                   const std::size_t width, const std::size_t height,
                                   const bool isAttachment) :
    d_ptr(new ResourceMetadataPrivate(note, resourceMimeType, dataHash, dataSize,
                                      width, height, isAttachment))
{}

ResourceMetadata::ResourceMetadata(const ResourceMetadata & other) :
    d_ptr((other.d_func() == nullptr) ? nullptr : new ResourceMetadataPrivate(*(other.d_func())))
{}

ResourceMetadata & ResourceMetadata::operator=(const ResourceMetadata & other)
{
    if (this != &other) {
        *d_func() = *other.d_func();
    }

    return *this;
}

ResourceMetadata::~ResourceMetadata()
{}

const Guid ResourceMetadata::noteGuid() const
{
    Q_D(const ResourceMetadata);
    return d->m_noteGuid;
}

const QString ResourceMetadata::mimeType() const
{
    Q_D(const ResourceMetadata);
    return d->m_resourceMimeType;
}

const QString ResourceMetadata::dataHash() const
{
    Q_D(const ResourceMetadata);
    return d->m_resourceDataHash;
}

const std::size_t ResourceMetadata::dataSize() const
{
    Q_D(const ResourceMetadata);
    return d->m_resourceDataSize;
}

const std::size_t ResourceMetadata::width() const
{
    Q_D(const ResourceMetadata);
    return d->m_resourceDisplayWidth;
}

const std::size_t ResourceMetadata::height() const
{
    Q_D(const ResourceMetadata);
    return d->m_resourceDisplayHeight;
}

bool ResourceMetadata::isAttachment() const
{
    Q_D(const ResourceMetadata);
    return d->m_isAttachment;
}

time_t ResourceMetadata::timestamp() const
{
    Q_D(const ResourceMetadata);
    return d->m_timestamp;
}

void ResourceMetadata::setTimestamp(const time_t timestamp)
{
    Q_D(ResourceMetadata);
    d->m_timestamp = timestamp;
}

const QString ResourceMetadata::sourceUrl() const
{
    Q_D(const ResourceMetadata);
    return d->m_sourceUrl;
}

void ResourceMetadata::setSourceUrl(const QString & sourceUrl)
{
    Q_D(ResourceMetadata);
    d->m_sourceUrl = sourceUrl;
}

double ResourceMetadata::altitude() const
{
    Q_D(const ResourceMetadata);
    return d->m_location.altitude();
}

void ResourceMetadata::setAltitude(const double altitude)
{
    Q_D(ResourceMetadata);
    d->m_location.setAltitude(altitude);
}

double ResourceMetadata::latitude() const
{
    Q_D(const ResourceMetadata);
    return d->m_location.latitude();
}

void ResourceMetadata::setLatitude(const double latitude)
{
    Q_D(ResourceMetadata);
    d->m_location.setLatitude(latitude);
}

double ResourceMetadata::longitude() const
{
    Q_D(const ResourceMetadata);
    return d->m_location.longitude();
}

void ResourceMetadata::setLongitude(const double longitude)
{
    Q_D(ResourceMetadata);
    d->m_location.setLongitude(longitude);
}

const QString ResourceMetadata::filename() const
{
    Q_D(const ResourceMetadata);
    return d->m_filename;
}

void ResourceMetadata::setFilename(const QString & filename)
{
    Q_D(ResourceMetadata);
    d->m_filename = filename;
}

QTextStream & ResourceMetadata::Print(QTextStream & strm) const
{
    Q_D(const ResourceMetadata);

    strm << "ResourceMetadata: { \n";
    strm << SynchronizedDataElement::Print(strm).readAll();
    strm << "note guid = ";
    strm << d->m_noteGuid.ToQString() << ", \n";
    strm << "mime type = ";
    strm << d->m_resourceMimeType << ", \n";
    strm << "hash = ";
    strm << d->m_resourceDataHash << ", \n";
    strm << "size = ";
    strm << d->m_resourceDataSize << ", \n";
    strm << "display width = ";
    strm << d->m_resourceDisplayWidth << ", \n";
    strm << "display height = ";
    strm << d->m_resourceDisplayHeight << ", \n";
    strm << (isAttachment() ? "is attachment" : "is not attachment");
    strm << ", \n";
    strm << "timestamp = ";
    strm << d->m_timestamp << ", \n";
    strm << "source URL = ";
    strm << d->m_sourceUrl << ", \n";
    strm << " " << d->m_location << ", \n";
    strm << "filename = ";
    strm << d->m_filename << "\n }; \n";

    return strm;
}

ResourceMetadataPrivate::ResourceMetadataPrivate(const Note & note,
                                                 const QString & resourceMimeType,
                                                 const QString & dataHash,
                                                 const std::size_t dataSize,
                                                 const std::size_t width,
                                                 const std::size_t height,
                                                 const bool isAttachment) :
    m_noteGuid(note.guid()),
    m_resourceMimeType(resourceMimeType),
    m_resourceDataHash(dataHash),
    m_resourceDataSize(dataSize),
    m_resourceDisplayWidth(width),
    m_resourceDisplayHeight(height),
    m_isAttachment(isAttachment)
{
    if (note.isEmpty()) {
        throw EmptyDataElementException("Attempt to create ResourceMetadata object for empty Note");
    }
}

ResourceMetadataPrivate::ResourceMetadataPrivate(const ResourceMetadataPrivate & other) :
    m_noteGuid(other.m_noteGuid),
    m_resourceMimeType(other.m_resourceMimeType),
    m_resourceDataHash(other.m_resourceDataHash),
    m_resourceDataSize(other.m_resourceDataSize),
    m_resourceDisplayWidth(other.m_resourceDisplayWidth),
    m_resourceDisplayHeight(other.m_resourceDisplayHeight),
    m_isAttachment(other.m_isAttachment),
    m_timestamp(other.m_timestamp),
    m_sourceUrl(other.m_sourceUrl),
    m_location(other.m_location),
    m_filename(other.m_filename)
{}

ResourceMetadataPrivate & ResourceMetadataPrivate::operator=(const ResourceMetadataPrivate & other)
{
    if (this != &other)
    {
        m_noteGuid = other.m_noteGuid;
        m_resourceMimeType = other.m_resourceMimeType;
        m_resourceDataHash = other.m_resourceDataHash;
        m_resourceDataSize = other.m_resourceDataSize;
        m_resourceDisplayWidth = other.m_resourceDisplayWidth;
        m_resourceDisplayHeight = other.m_resourceDisplayHeight;
        m_isAttachment = other.m_isAttachment;
        m_timestamp = other.m_timestamp;
        m_sourceUrl = other.m_sourceUrl;
        m_location = other.m_location;
        m_filename = other.m_filename;
    }

    return *this;
}

}
