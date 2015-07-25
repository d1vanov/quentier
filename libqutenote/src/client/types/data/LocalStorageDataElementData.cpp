#include "LocalStorageDataElementData.h"

namespace qute_note {

LocalStorageDataElementData::LocalStorageDataElementData() :
    QSharedData(),
    m_localGuid(QUuid::createUuid())
{}

LocalStorageDataElementData::~LocalStorageDataElementData()
{}

LocalStorageDataElementData::LocalStorageDataElementData(const LocalStorageDataElementData & other) :
    QSharedData(other),
    m_localGuid(other.m_localGuid)
{}

LocalStorageDataElementData::LocalStorageDataElementData(LocalStorageDataElementData && other) :
    QSharedData(std::move(other)),
    m_localGuid(std::move(other.m_localGuid))
{}

} // namespace qute_note
