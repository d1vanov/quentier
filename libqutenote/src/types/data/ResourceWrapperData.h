#ifndef __LIB_QUTE_NOTE__TYPES__DATA__RESOURCE_WRAPPER_DATA_H
#define __LIB_QUTE_NOTE__TYPES__DATA__RESOURCE_WRAPPER_DATA_H

#include "NoteStoreDataElementData.h"
#include <QEverCloud.h>

namespace qute_note {

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

} // namespace qute_note

#endif // __LIB_QUTE_NOTE__TYPES__DATA__RESOURCE_WRAPPER_DATA_H
