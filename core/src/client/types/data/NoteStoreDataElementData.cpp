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

NoteStoreDataElementData & NoteStoreDataElementData::operator=(const NoteStoreDataElementData & other)
{
    LocalStorageDataElementData::operator=(other);

    if (this != std::addressof(other)) {
        m_isDirty = other.m_isDirty;
    }

    return *this;
}

NoteStoreDataElementData & NoteStoreDataElementData::operator=(NoteStoreDataElementData && other)
{
    LocalStorageDataElementData::operator=(std::move(other));

    if (this != std::addressof(other)) {
        m_isDirty = std::move(other.m_isDirty);
    }

    return *this;
}



} // namespace qute_note
