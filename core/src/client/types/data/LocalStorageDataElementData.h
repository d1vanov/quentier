#ifndef __QUTE_NOTE__CORE__CLIENT__TYPES__DATA__LOCAL_STORAGE_DATA_ELEMENT_DATA_H
#define __QUTE_NOTE__CORE__CLIENT__TYPES__DATA__LOCAL_STORAGE_DATA_ELEMENT_DATA_H

#include <QUuid>
#include <QSharedData>

namespace qute_note {

class LocalStorageDataElementData: public QSharedData
{
public:
    LocalStorageDataElementData();
    virtual ~LocalStorageDataElementData();

    LocalStorageDataElementData(const LocalStorageDataElementData & other);
    LocalStorageDataElementData(LocalStorageDataElementData && other);

    QUuid m_localGuid;

private:
    LocalStorageDataElementData & operator=(const LocalStorageDataElementData & other) = delete;
    LocalStorageDataElementData & operator=(LocalStorageDataElementData && other) = delete;
};

} // namespace qute_note

#endif // __QUTE_NOTE__CORE__CLIENT__TYPES__DATA__LOCAL_STORAGE_DATA_ELEMENT_DATA_H
