#ifndef __QUTE_NOTE__CLIENT__TYPES__I_DATA_ELEMENT_WITH_SHORTCUT_H
#define __QUTE_NOTE__CLIENT__TYPES__I_DATA_ELEMENT_WITH_SHORTCUT_H

#include "INoteStoreDataElement.h"

namespace qute_note {

/**
 * Class adding one more attribute to each note store data element subclassing it:
 * shortcut. Shortcut is basically a quick access link in application GUI. However,
 * the sign whether some note store data element is shortcutted is stored in local storage db.
 */
class QUTE_NOTE_EXPORT IDataElementWithShortcut : public virtual INoteStoreDataElement
{
public:
    virtual bool hasShortcut() const = 0;
    virtual void setShortcut(const bool shortcut) = 0;

    virtual ~IDataElementWithShortcut() {}
};

#define _DECLARE_HAS_SHORTCUT \
    virtual bool hasShortcut() const;

#define _DECLARE_SET_SHORTCUT \
    virtual void setShortcut(const bool shortcut);

#define QN_DECLARE_SHORTCUT \
    _DECLARE_HAS_SHORTCUT \
    _DECLARE_SET_SHORTCUT

#define _DEFINE_HAS_SHORTCUT(type) \
    bool type::hasShortcut() const { \
        return d->m_hasShortcut; \
    }

#define _DEFINE_SET_SHORTCUT(type) \
    void type::setShortcut(const bool shortcut) { \
        d->m_hasShortcut = shortcut; \
    }

#define QN_DEFINE_SHORTCUT(type) \
    _DEFINE_HAS_SHORTCUT(type) \
    _DEFINE_SET_SHORTCUT(type)

} // namespace qute_note

#endif // __QUTE_NOTE__CLIENT__TYPES__I_DATA_ELEMENT_WITH_SHORTCUT_H
