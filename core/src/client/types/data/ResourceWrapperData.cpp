#include "ResourceWrapperData.h"

namespace qute_note {

ResourceWrapperData::ResourceWrapperData() :
    m_qecResource()
{}

ResourceWrapperData::ResourceWrapperData(const ResourceWrapperData & other) :
    m_qecResource(other.m_qecResource)
{}

ResourceWrapperData::ResourceWrapperData(ResourceWrapperData && other) :
    m_qecResource(std::move(other.m_qecResource))
{}

ResourceWrapperData::ResourceWrapperData(const qevercloud::Resource & other) :
    m_qecResource(other)
{}

ResourceWrapperData::ResourceWrapperData(qevercloud::Resource && other) :
    m_qecResource(std::move(other))
{}

ResourceWrapperData & ResourceWrapperData::operator=(const ResourceWrapperData & other)
{
    if (this != std::addressof(other)) {
        m_qecResource = other.m_qecResource;
    }

    return *this;
}

ResourceWrapperData & ResourceWrapperData::operator=(ResourceWrapperData && other)
{
    if (this != std::addressof(other)) {
        m_qecResource = std::move(other.m_qecResource);
    }

    return *this;
}

ResourceWrapperData::~ResourceWrapperData()
{}

} // namespace qute_note
