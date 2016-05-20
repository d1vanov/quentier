#ifndef LIB_QUTE_NOTE_TYPES_DATA_LOCAL_STORAGE_DATA_ELEMENT_DATA_H
#define LIB_QUTE_NOTE_TYPES_DATA_LOCAL_STORAGE_DATA_ELEMENT_DATA_H

#include <qute_note/utility/Qt4Helper.h>
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

    QUuid m_localUid;

private:
    LocalStorageDataElementData & operator=(const LocalStorageDataElementData & other) Q_DECL_DELETE;
    LocalStorageDataElementData & operator=(LocalStorageDataElementData && other) Q_DECL_DELETE;
};

} // namespace qute_note

#endif // LIB_QUTE_NOTE_TYPES_DATA_LOCAL_STORAGE_DATA_ELEMENT_DATA_H
