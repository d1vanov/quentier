#include "ResourceMetadata.h"
#include "Guid.h"

namespace qute_note {

ResourceMetadata::ResourceMetadata(const Resource & resource) :
    m_resource(resource)
{}

ResourceMetadata::ResourceMetadata(const ResourceMetadata & other) :
    m_resource(other.m_resource)
{}

const Resource & ResourceMetadata::resource() const
{
    return m_resource;
}

const QString ResourceMetadata::dataHash() const
{
    return m_resource.dataHash();
}

std::size_t ResourceMetadata::dataSize() const
{
    return m_resource.dataSize();
}

const Guid ResourceMetadata::noteGuid() const
{
    return m_resource.noteGuid();
}

const QString ResourceMetadata::mimeType() const
{
    return m_resource.mimeType();
}

const std::size_t ResourceMetadata::width() const
{
    return m_resource.width();
}

const std::size_t ResourceMetadata::height() const
{
    return m_resource.height();
}


}
