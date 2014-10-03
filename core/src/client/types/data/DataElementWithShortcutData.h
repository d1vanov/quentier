#ifndef __QUTE_NOTE__CORE__CLIENT__TYPES__DATA__DATA_ELEMENT_WITH_SHORTCUT_DATA_H
#define __QUTE_NOTE__CORE__CLIENT__TYPES__DATA__DATA_ELEMENT_WITH_SHORTCUT_DATA_H

#include "NoteStoreDataElementData.h"

namespace qute_note {

class DataElementWithShortcutData : public NoteStoreDataElementData
{
public:
    DataElementWithShortcutData();
    virtual ~DataElementWithShortcutData();

    DataElementWithShortcutData(const DataElementWithShortcutData & other);
    DataElementWithShortcutData(DataElementWithShortcutData && other);

    bool    m_hasShortcut;

private:
    DataElementWithShortcutData & operator=(const DataElementWithShortcutData & other) = delete;
    DataElementWithShortcutData & operator=(DataElementWithShortcutData && other) = delete;
};

} // namespace qute_note

#endif // __QUTE_NOTE__CORE__CLIENT__TYPES__DATA__DATA_ELEMENT_WITH_SHORTCUT_DATA_H
