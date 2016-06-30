#ifndef LIB_QUENTIER_TYPES_DATA_RESOURCE_WRAPPER_DATA_H
#define LIB_QUENTIER_TYPES_DATA_RESOURCE_WRAPPER_DATA_H

#include "NoteStoreDataElementData.h"
#include <QEverCloud.h>

namespace quentier {

class ResourceWrapperData : public NoteStoreDataElementData
{
public:
    ResourceWrapperData();
    ResourceWrapperData(const ResourceWrapperData & other);
    ResourceWrapperData(ResourceWrapperData && other);
    ResourceWrapperData(const qevercloud::Resource & other);
    ResourceWrapperData(qevercloud::Resource && other);
    virtual ~ResourceWrapperData();

    qevercloud::Resource    m_qecResource;

private:
    ResourceWrapperData & operator=(const ResourceWrapperData & other) Q_DECL_DELETE;
    ResourceWrapperData & operator=(ResourceWrapperData && other) Q_DECL_DELETE;
};

} // namespace quentier

#endif // LIB_QUENTIER_TYPES_DATA_RESOURCE_WRAPPER_DATA_H
