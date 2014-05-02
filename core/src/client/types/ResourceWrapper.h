#ifndef __QUTE_NOTE__CLIENT__TYPES__RESOURCE_WRAPPER_H
#define __QUTE_NOTE__CLIENT__TYPES__RESOURCE_WRAPPER_H

#include "IResource.h"

namespace qute_note {

/**
 * @brief The ResourceWrapper class creates and manages its own instance of
 * qevercloud::Resource object
 */
class ResourceWrapper final: public IResource
{
public:
    ResourceWrapper();
    ResourceWrapper(const IResource & other);
    ResourceWrapper(const qevercloud::Resource & other);
    ResourceWrapper(qevercloud::Resource && other);
    ResourceWrapper & operator=(const IResource & other);
    virtual ~ResourceWrapper() final override;

private:
    virtual const qevercloud::Resource & GetEnResource() const final override;
    virtual qevercloud::Resource & GetEnResource() final override;

    virtual QTextStream & Print(QTextStream & strm) const;

    qevercloud::Resource m_enResource;
};

} // namespace qute_note

#endif // __QUTE_NOTE__CLIENT__TYPES__RESOURCE_WRAPPER_H
