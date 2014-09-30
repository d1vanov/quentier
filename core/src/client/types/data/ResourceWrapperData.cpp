#include "ResourceWrapperData.h"

namespace qute_note {

ResourceWrapperData::ResourceWrapperData() :
    NoteStoreDataElementData(),
    m_qecResource()
{}

ResourceWrapperData::ResourceWrapperData(const ResourceWrapperData & other) :
    NoteStoreDataElementData(other),
    m_qecResource(other.m_qecResource)
{}

ResourceWrapperData::ResourceWrapperData(ResourceWrapperData && other) :
    NoteStoreDataElementData(std::move(other)),
    m_qecResource(std::move(other.m_qecResource))
{}

ResourceWrapperData::ResourceWrapperData(const qevercloud::Resource & other) :
    NoteStoreDataElementData(),
    m_qecResource(other)
{}

ResourceWrapperData::ResourceWrapperData(qevercloud::Resource && other) :
    NoteStoreDataElementData(),
    m_qecResource(std::move(other))
{}

ResourceWrapperData & ResourceWrapperData::operator=(const ResourceWrapperData & other)
{
    NoteStoreDataElementData::operator=(other);

    if (this != std::addressof(other)) {
        m_qecResource = other.m_qecResource;
    }

    return *this;
}

ResourceWrapperData & ResourceWrapperData::operator=(ResourceWrapperData && other)
{
    NoteStoreDataElementData::operator=(std::move(other));

    if (this != std::addressof(other)) {
        m_qecResource = std::move(other.m_qecResource);
    }

    return *this;
}

ResourceWrapperData::~ResourceWrapperData()
{}

} // namespace qute_note
