#ifndef __QUTE_NOTE__CLIENT__TYPES__I_NOTE_STORE_DATA_ELEMENT_H
#define __QUTE_NOTE__CLIENT__TYPES__I_NOTE_STORE_DATA_ELEMENT_H

#include "ILocalStorageDataElement.h"
#include <qute_note/utility/Printable.h>
#include <QtGlobal>
#include <QUuid>

namespace qute_note {

class QUTE_NOTE_EXPORT INoteStoreDataElement: public ILocalStorageDataElement,
                                              public Printable
{
public:
    virtual void clear() = 0;

    virtual bool hasGuid() const = 0;
    virtual const QString & guid() const = 0;
    virtual void setGuid(const QString & guid) = 0;

    virtual bool hasUpdateSequenceNumber() const = 0;
    virtual qint32 updateSequenceNumber() const = 0;
    virtual void setUpdateSequenceNumber(const qint32 usn) = 0;

    virtual bool checkParameters(QString & errorDescription) const = 0;

    virtual bool isDirty() const = 0;
    virtual void setDirty(const bool dirty) = 0;

    virtual bool isLocal() const = 0;
    virtual void setLocal(const bool local) = 0;

    virtual ~INoteStoreDataElement() {}

protected:
    virtual QTextStream & Print(QTextStream & strm) const = 0;
};

#define _DECLARE_IS_DIRTY \
    virtual bool isDirty() const;

#define _DECLARE_SET_DIRTY \
    virtual void setDirty(const bool isDirty);

#define QN_DECLARE_DIRTY \
    _DECLARE_IS_DIRTY \
    _DECLARE_SET_DIRTY

#define _DEFINE_IS_DIRTY(type) \
    bool type::isDirty() const { \
        return d->m_isDirty; \
    }

#define _DEFINE_SET_DIRTY(type) \
    void type::setDirty(const bool dirty) { \
        d->m_isDirty = dirty; \
    }

#define QN_DEFINE_DIRTY(type) \
    _DEFINE_IS_DIRTY(type) \
    _DEFINE_SET_DIRTY(type)

#define _DECLARE_IS_LOCAL \
    virtual bool isLocal() const;

#define _DECLARE_SET_LOCAL \
    virtual void setLocal(const bool isLocal);

#define QN_DECLARE_LOCAL \
    _DECLARE_IS_LOCAL \
    _DECLARE_SET_LOCAL

#define _DEFINE_IS_LOCAL(type) \
    bool type::isLocal() const { \
        return d->m_isLocal; \
    }

#define _DEFINE_SET_LOCAL(type) \
    void type::setLocal(const bool local) { \
        d->m_isLocal = local; \
    }

#define QN_DEFINE_LOCAL(type) \
    _DEFINE_IS_LOCAL(type) \
    _DEFINE_SET_LOCAL(type)

} // namespace qute_note

#endif // __QUTE_NOTE__CLIENT__TYPES__I_NOTE_STORE_DATA_ELEMENT_H
