#ifndef __QUTE_NOTE__CLIENT__TYPES__LOCAL_STORAGE_DATA_ELEMENT_H
#define __QUTE_NOTE__CLIENT__TYPES__LOCAL_STORAGE_DATA_ELEMENT_H

#include <QString>
#include <QUuid>

namespace qute_note {

class LocalStorageDataElement
{
public:
    LocalStorageDataElement();
    virtual ~LocalStorageDataElement();

    const QString localGuid() const;
    void setLocalGuid(const QString & guid);

protected:
    LocalStorageDataElement(const LocalStorageDataElement & other) = default;
    LocalStorageDataElement(LocalStorageDataElement && other) = default;
    LocalStorageDataElement & operator=(const LocalStorageDataElement & other) = default;
    LocalStorageDataElement & operator=(LocalStorageDataElement && other) = default;

private:
    QUuid  m_localGuid;
};

} // namespace qute_note

#endif // __QUTE_NOTE__CLIENT__TYPES__LOCAL_STORAGE_DATA_ELEMENT_H
