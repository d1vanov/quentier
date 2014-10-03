#include "NoteStoreDataElementData.h"

namespace qute_note {

NoteStoreDataElementData::NoteStoreDataElementData() :
    LocalStorageDataElementData(),
    m_isDirty(true)
{}

NoteStoreDataElementData::~NoteStoreDataElementData()
{}

NoteStoreDataElementData::NoteStoreDataElementData(const NoteStoreDataElementData & other) :
    LocalStorageDataElementData(other),
    m_isDirty(other.m_isDirty)
{}

NoteStoreDataElementData::NoteStoreDataElementData(NoteStoreDataElementData && other) :
    LocalStorageDataElementData(std::move(other)),
    m_isDirty(std::move(other.m_isDirty))
{}

} // namespace qute_note
