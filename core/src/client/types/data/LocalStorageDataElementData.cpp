#include "LocalStorageDataElementData.h"

namespace qute_note {

LocalStorageDataElementData::LocalStorageDataElementData() :
    m_localGuid(QUuid::createUuid())
{}

LocalStorageDataElementData::~LocalStorageDataElementData()
{}

LocalStorageDataElementData::LocalStorageDataElementData(const LocalStorageDataElementData & other) :
    m_localGuid(other.m_localGuid)
{}

LocalStorageDataElementData::LocalStorageDataElementData(LocalStorageDataElementData && other) :
    m_localGuid(std::move(other.m_localGuid))
{}

LocalStorageDataElementData & LocalStorageDataElementData::operator=(const LocalStorageDataElementData & other)
{
    if (this != std::addressof(other)) {
        m_localGuid = other.m_localGuid;
    }

    return *this;
}

LocalStorageDataElementData & LocalStorageDataElementData::operator=(LocalStorageDataElementData && other)
{
    if (this != std::addressof(other)) {
        m_localGuid = std::move(other.m_localGuid);
    }

    return *this;
}

} // namespace qute_note
