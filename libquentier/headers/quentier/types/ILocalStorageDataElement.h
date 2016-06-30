#ifndef LIB_QUENTIER_TYPES_I_LOCAL_STORAGE_DATA_ELEMENT_H
#define LIB_QUENTIER_TYPES_I_LOCAL_STORAGE_DATA_ELEMENT_H

#include <quentier/utility/Linkage.h>
#include <quentier/utility/Qt4Helper.h>
#include <quentier/utility/UidGenerator.h>
#include <QString>
#include <QUuid>

namespace quentier {

class QUENTIER_EXPORT ILocalStorageDataElement
{
public:
    virtual const QString localUid() const = 0;
    virtual void setLocalUid(const QString & guid) = 0;
    virtual void unsetLocalUid() = 0;

    virtual ~ILocalStorageDataElement() {}
};

#define DEFINE_LOCAL_UID_GETTER(type) \
    const QString type::localUid() const { \
        return UidGenerator::UidToString(d->m_localUid); \
    }

#define DEFINE_LOCAL_UID_SETTER(type) \
    void type::setLocalUid(const QString & uid) { \
        d->m_localUid = uid; \
    }

#define DEFINE_LOCAL_UID_UNSETTER(type) \
    void type::unsetLocalUid() { \
        d->m_localUid = QUuid(); \
    }

#define QN_DECLARE_LOCAL_UID \
    virtual const QString localUid() const Q_DECL_OVERRIDE; \
    virtual void setLocalUid(const QString & guid) Q_DECL_OVERRIDE; \
    virtual void unsetLocalUid() Q_DECL_OVERRIDE;

#define QN_DEFINE_LOCAL_UID(type) \
    DEFINE_LOCAL_UID_GETTER(type) \
    DEFINE_LOCAL_UID_SETTER(type) \
    DEFINE_LOCAL_UID_UNSETTER(type)

} // namespace quentier

#endif // LIB_QUENTIER_TYPES_I_LOCAL_STORAGE_DATA_ELEMENT_H
