#ifndef __QUTE_NOTE__CORE__CLIENT__TYPES__I_SYNCHRONIZABLE_DATA_ELEMENT_H
#define __QUTE_NOTE__CORE__CLIENT__TYPES__I_SYNCHRONIZABLE_DATA_ELEMENT_H

#include "INoteStoreDataElement.h"

namespace qute_note {

/**
 * Class adding the property for the data element to be synchronized
 * with the remote storage of the Evernote account of just stay local
 */
class ISynchronizableDataElement: public virtual INoteStoreDataElement
{
public:
    virtual bool isSynchronizable() const = 0;
    virtual void setSynchronizable(const bool synchronizable) = 0;

    virtual ~ISynchronizableDataElement() = default;
};

#define _DECLARE_IS_SYNCHRONIZABLE \
    virtual bool isSynchronizable() const;

#define _DECLARE_SET_SYNCHRONIZABLE \
    virtual void setSynchronizable(const bool synchronizable);

#define QN_DECLARE_SYNCHRONIZABLE \
    _DECLARE_IS_SYNCHRONIZABLE \
    _DECLARE_SET_SYNCHRONIZABLE

#define _DEFINE_IS_SYNCHRONIZABLE(type) \
    bool type::isSynchronizable() const { \
        return d->m_synchronizable; \
    }

#define _DEFINE_SET_SYNCHRONIZABLE(type) \
    void type::setSynchronizable(const bool synchronizable) { \
        d->m_synchronizable = synchronizable; \
    }

#define QN_DEFINE_SYNCHRONIZABLE(type) \
    _DEFINE_IS_SYNCHRONIZABLE(type) \
    _DEFINE_SET_SYNCHRONIZABLE(type)

} // namespace qute_note

#endif // __QUTE_NOTE__CORE__CLIENT__TYPES__I_SYNCHRONIZABLE_DATA_ELEMENT_H
