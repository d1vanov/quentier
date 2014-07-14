#include "DataElementWithShortcut.h"

namespace qute_note {

DataElementWithShortcut::DataElementWithShortcut() :
    m_hasShortcut(false)
{}

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
