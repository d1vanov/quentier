#include "ResourceWrapperData.h"

namespace quentier {

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

ResourceWrapperData::~ResourceWrapperData()
{}

} // namespace quentier
