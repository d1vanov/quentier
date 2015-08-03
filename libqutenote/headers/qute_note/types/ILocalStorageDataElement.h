#ifndef __LIB_QUTE_NOTE__TYPES__I_LOCAL_STORAGE_DATA_ELEMENT_H
#define __LIB_QUTE_NOTE__TYPES__I_LOCAL_STORAGE_DATA_ELEMENT_H

#include <qute_note/utility/Linkage.h>
#include <QString>
#include <QUuid>

namespace qute_note {

class QUTE_NOTE_EXPORT ILocalStorageDataElement
{
public:
    virtual const QString localGuid() const = 0;
    virtual void setLocalGuid(const QString & guid) = 0;
    virtual void unsetLocalGuid() = 0;

    virtual ~ILocalStorageDataElement() {}
};

#define _DEFINE_LOCAL_GUID_GETTER(type) \
    const QString type::localGuid() const { \
        if (d->m_localGuid.isNull()) { \
            return QString(); \
        } \
        else { \
            return d->m_localGuid.toString(); \
        } \
    }

#define _DEFINE_LOCAL_GUID_SETTER(type) \
    void type::setLocalGuid(const QString & guid) { \
        d->m_localGuid = guid; \
    }

#define _DEFINE_LOCAL_GUID_UNSETTER(type) \
    void type::unsetLocalGuid() { \
        d->m_localGuid = QUuid(); \
    }

#define QN_DECLARE_LOCAL_GUID \
    virtual const QString localGuid() const; \
    virtual void setLocalGuid(const QString & guid); \
    virtual void unsetLocalGuid();

#define QN_DEFINE_LOCAL_GUID(type) \
    _DEFINE_LOCAL_GUID_GETTER(type) \
    _DEFINE_LOCAL_GUID_SETTER(type) \
    _DEFINE_LOCAL_GUID_UNSETTER(type)

} // namespace qute_note

#endif // __LIB_QUTE_NOTE__TYPES__I_LOCAL_STORAGE_DATA_ELEMENT_H
