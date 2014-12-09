#include "NoteStoreDataElementData.h"

namespace qute_note {

NoteStoreDataElementData::NoteStoreDataElementData() :
    LocalStorageDataElementData(),
    m_isDirty(true),
    m_isLocal(false)
{}

NoteStoreDataElementData::~NoteStoreDataElementData()
{}

NoteStoreDataElementData::NoteStoreDataElementData(const NoteStoreDataElementData & other) :
    LocalStorageDataElementData(other),
    m_isDirty(other.m_isDirty),
    m_isLocal(other.m_isLocal)
{}

NoteStoreDataElementData::NoteStoreDataElementData(NoteStoreDataElementData && other) :
    LocalStorageDataElementData(std::move(other)),
    m_isDirty(std::move(other.m_isDirty)),
    m_isLocal(std::move(other.m_isLocal))
{}

} // namespace qute_note
