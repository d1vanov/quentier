#ifndef __QUTE_NOTE__CORE__CLIENT__TYPES__DATA__LOCAL_STORAGE_DATA_ELEMENT_DATA_H
#define __QUTE_NOTE__CORE__CLIENT__TYPES__DATA__LOCAL_STORAGE_DATA_ELEMENT_DATA_H

#include <QUuid>

namespace qute_note {

class LocalStorageDataElementData
{
public:
    LocalStorageDataElementData();
    virtual ~LocalStorageDataElementData();

    LocalStorageDataElementData(const LocalStorageDataElementData & other);
    LocalStorageDataElementData(LocalStorageDataElementData && other);
    LocalStorageDataElementData & operator=(const LocalStorageDataElementData & other);
    LocalStorageDataElementData & operator=(LocalStorageDataElementData && other);

    QUuid m_localGuid;
};

} // namespace qute_note

#endif // __QUTE_NOTE__CORE__CLIENT__TYPES__DATA__LOCAL_STORAGE_DATA_ELEMENT_DATA_H
