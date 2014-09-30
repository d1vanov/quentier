#include "DataElementWithShortcut.h"

namespace qute_note {

DataElementWithShortcut::DataElementWithShortcut() :
    m_hasShortcut(false)
{}

DataElementWithShortcut::DataElementWithShortcut(const DataElementWithShortcut & other) :
    m_hasShortcut(other.m_hasShortcut)
{}

DataElementWithShortcut::DataElementWithShortcut(DataElementWithShortcut && other) :
    m_hasShortcut(std::move(other.hasShortcut()))
{}

DataElementWithShortcut & DataElementWithShortcut::operator=(const DataElementWithShortcut & other)
{
    if (this != std::addressof(other)) {
        m_hasShortcut = other.m_hasShortcut;
    }

    return *this;
}

DataElementWithShortcut & DataElementWithShortcut::operator=(DataElementWithShortcut && other)
{
    m_hasShortcut = std::move(other.m_hasShortcut);
    return *this;
}

DataElementWithShortcut::~DataElementWithShortcut()
{}

bool DataElementWithShortcut::hasShortcut() const
{
    return m_hasShortcut;
}

void DataElementWithShortcut::setShortcut(const bool shortcut)
{
    m_hasShortcut = shortcut;
}

} // namespace qute_note
