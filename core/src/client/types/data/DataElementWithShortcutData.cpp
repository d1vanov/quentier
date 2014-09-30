#include "DataElementWithShortcutData.h"

namespace qute_note {

DataElementWithShortcutData::DataElementWithShortcutData() :
    NoteStoreDataElementData(),
    m_hasShortcut(false)
{}

DataElementWithShortcutData::~DataElementWithShortcutData()
{}

DataElementWithShortcutData::DataElementWithShortcutData(const DataElementWithShortcutData & other) :
    NoteStoreDataElementData(other),
    m_hasShortcut(other.m_hasShortcut)
{}

DataElementWithShortcutData::DataElementWithShortcutData(DataElementWithShortcutData && other) :
    NoteStoreDataElementData(std::move(other)),
    m_hasShortcut(std::move(other.m_hasShortcut))
{}

DataElementWithShortcutData & DataElementWithShortcutData::operator=(const DataElementWithShortcutData & other)
{
    NoteStoreDataElementData::operator=(other);

    if (this != std::addressof(other)) {
        m_hasShortcut = other.m_hasShortcut;
    }

    return *this;
}

DataElementWithShortcutData & DataElementWithShortcutData::operator=(DataElementWithShortcutData && other)
{
    NoteStoreDataElementData::operator=(std::move(other));

    if (this != std::addressof(other)) {
        m_hasShortcut = std::move(other.m_hasShortcut);
    }

    return *this;
}

} // namespace qute_note
