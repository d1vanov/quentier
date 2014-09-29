#ifndef __QUTE_NOTE__CORE__CLIENT__TYPES__DATA__RESOURCE_WRAPPER_DATA_H
#define __QUTE_NOTE__CORE__CLIENT__TYPES__DATA__RESOURCE_WRAPPER_DATA_H

#include "LocalStorageDataElementData.h"
#include <QEverCloud.h>
#include <QSharedData>

namespace qute_note {

class ResourceWrapperData : public LocalStorageDataElementData,
                            public QSharedData
{
public:
    ResourceWrapperData();
    ResourceWrapperData(const ResourceWrapperData & other);
    ResourceWrapperData(ResourceWrapperData && other);
    ResourceWrapperData(const qevercloud::Resource & other);
    ResourceWrapperData(qevercloud::Resource && other);
    ResourceWrapperData & operator=(const ResourceWrapperData & other);
    ResourceWrapperData & operator=(ResourceWrapperData && other);
    virtual ~ResourceWrapperData();

    qevercloud::Resource    m_qecResource;
};

} // namespace qute_note

#endif // __QUTE_NOTE__CORE__CLIENT__TYPES__DATA__RESOURCE_WRAPPER_DATA_H
