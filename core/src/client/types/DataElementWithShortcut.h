#ifndef __QUTE_NOTE__CLIENT__TYPES__DATA_ELEMENT_WITH_SHORTCUT_H
#define __QUTE_NOTE__CLIENT__TYPES__DATA_ELEMENT_WITH_SHORTCUT_H

#include "NoteStoreDataElement.h"

namespace qute_note {

class DataElementWithShortcut : public NoteStoreDataElement
{
public:
    DataElementWithShortcut();
    virtual ~DataElementWithShortcut();

    bool hasShortcut() const;
    void setShortcut(const bool shortcut);

protected:
    DataElementWithShortcut(const DataElementWithShortcut & other) = default;
    DataElementWithShortcut(DataElementWithShortcut && other) = default;
    DataElementWithShortcut & operator=(const DataElementWithShortcut & other) = default;
    DataElementWithShortcut & operator=(DataElementWithShortcut && other) = default;

private:
    bool  m_hasShortcut;
};

} // namespace qute_note

#endif // __QUTE_NOTE__CLIENT__TYPES__DATA_ELEMENT_WITH_SHORTCUT_H
