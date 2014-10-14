#include "SynchronizableDataElementData.h"

namespace qute_note {

SynchronizableDataElementData::SynchronizableDataElementData() :
    m_synchronizable(true)
{}

SynchronizableDataElementData::~SynchronizableDataElementData()
{}

SynchronizableDataElementData::SynchronizableDataElementData(const SynchronizableDataElementData & other) :
    m_synchronizable(other.m_synchronizable)
{}

SynchronizableDataElementData::SynchronizableDataElementData(SynchronizableDataElementData && other) :
    m_synchronizable(std::move(other.m_synchronizable))
{}

} // namespace qute_note
