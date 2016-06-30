#ifndef LIB_QUENTIER_TYPES_DATA_LOCAL_STORAGE_DATA_ELEMENT_DATA_H
#define LIB_QUENTIER_TYPES_DATA_LOCAL_STORAGE_DATA_ELEMENT_DATA_H

#include <quentier/utility/Qt4Helper.h>
#include <QUuid>
#include <QSharedData>

namespace quentier {

class LocalStorageDataElementData: public QSharedData
{
public:
    LocalStorageDataElementData();
    virtual ~LocalStorageDataElementData();

    LocalStorageDataElementData(const LocalStorageDataElementData & other);
    LocalStorageDataElementData(LocalStorageDataElementData && other);

    QUuid m_localUid;

private:
    LocalStorageDataElementData & operator=(const LocalStorageDataElementData & other) Q_DECL_DELETE;
    LocalStorageDataElementData & operator=(LocalStorageDataElementData && other) Q_DECL_DELETE;
};

} // namespace quentier

#endif // LIB_QUENTIER_TYPES_DATA_LOCAL_STORAGE_DATA_ELEMENT_DATA_H
