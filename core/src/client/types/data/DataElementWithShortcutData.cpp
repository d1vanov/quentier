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

} // namespace qute_note
