#ifndef __QUTE_NOTE__CLIENT__TYPES__DATA_ELEMENT_WITH_SHORTCUT_H
#define __QUTE_NOTE__CLIENT__TYPES__DATA_ELEMENT_WITH_SHORTCUT_H

#include "INoteStoreDataElement.h"

namespace qute_note {

/**
 * Class adding one more attribute to each note store data element subclassing it:
 * shortcut. Shortcut is basically a quick access link in application GUI. However,
 * the sign whether some note store data element is shortcutted is stored in local storage db.
 */
class QUTE_NOTE_EXPORT DataElementWithShortcut : public INoteStoreDataElement
{
public:
    DataElementWithShortcut();
    virtual ~DataElementWithShortcut();

    bool hasShortcut() const;
    void setShortcut(const bool shortcut);

protected:
    DataElementWithShortcut(const DataElementWithShortcut & other);
    DataElementWithShortcut(DataElementWithShortcut && other);
    DataElementWithShortcut & operator=(const DataElementWithShortcut & other);
    DataElementWithShortcut & operator=(DataElementWithShortcut && other);

private:
    bool  m_hasShortcut;
};

} // namespace qute_note

#endif // __QUTE_NOTE__CLIENT__TYPES__DATA_ELEMENT_WITH_SHORTCUT_H
