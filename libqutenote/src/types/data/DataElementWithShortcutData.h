#ifndef LIB_QUTE_NOTE_TYPES_DATA_DATA_ELEMENT_WITH_SHORTCUT_DATA_H
#define LIB_QUTE_NOTE_TYPES_DATA_DATA_ELEMENT_WITH_SHORTCUT_DATA_H

#include "NoteStoreDataElementData.h"
#include <qute_note/utility/Qt4Helper.h>

namespace qute_note {

class DataElementWithShortcutData: public NoteStoreDataElementData
{
public:
    DataElementWithShortcutData();
    virtual ~DataElementWithShortcutData();

    DataElementWithShortcutData(const DataElementWithShortcutData & other);
    DataElementWithShortcutData(DataElementWithShortcutData && other);

    bool    m_hasShortcut;

private:
    DataElementWithShortcutData & operator=(const DataElementWithShortcutData & other) Q_DECL_DELETE;
    DataElementWithShortcutData & operator=(DataElementWithShortcutData && other) Q_DECL_DELETE;
};

} // namespace qute_note

#endif // LIB_QUTE_NOTE_TYPES_DATA_DATA_ELEMENT_WITH_SHORTCUT_DATA_H
