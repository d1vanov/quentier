#ifndef __QUTE_NOTE__CLIENT__TYPES__LOCAL_STORAGE_DATA_ELEMENT_H
#define __QUTE_NOTE__CLIENT__TYPES__LOCAL_STORAGE_DATA_ELEMENT_H

#include <tools/Linkage.h>
#include <QString>
#include <QUuid>

namespace qute_note {

class QUTE_NOTE_EXPORT LocalStorageDataElement
{
public:
    LocalStorageDataElement();
    virtual ~LocalStorageDataElement();

    const QString localGuid() const;
    void setLocalGuid(const QString & guid);
    void unsetLocalGuid();

protected:
    LocalStorageDataElement(const LocalStorageDataElement & other) = default;
    LocalStorageDataElement(LocalStorageDataElement && other);
    LocalStorageDataElement & operator=(const LocalStorageDataElement & other) = default;
    LocalStorageDataElement & operator=(LocalStorageDataElement && other);

private:
    QUuid  m_localGuid;
};

} // namespace qute_note

#endif // __QUTE_NOTE__CLIENT__TYPES__LOCAL_STORAGE_DATA_ELEMENT_H
