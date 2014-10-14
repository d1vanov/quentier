#include "DataElementWithShortcutData.h"

namespace qute_note {

DataElementWithShortcutData::DataElementWithShortcutData() :
    m_hasShortcut(false)
{}

DataElementWithShortcutData::~DataElementWithShortcutData()
{}

DataElementWithShortcutData::DataElementWithShortcutData(const DataElementWithShortcutData & other) :
    m_hasShortcut(other.m_hasShortcut)
{}

DataElementWithShortcutData::DataElementWithShortcutData(DataElementWithShortcutData && other) :
    m_hasShortcut(std::move(other.m_hasShortcut))
{}

} // namespace qute_note
