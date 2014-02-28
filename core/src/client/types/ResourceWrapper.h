#ifndef __QUTE_NOTE__CLIENT__TYPES__RESOURCE_WRAPPER_H
#define __QUTE_NOTE__CLIENT__TYPES__RESOURCE_WRAPPER_H

#include "IResource.h"
#include <Types_types.h>

namespace qute_note {

/**
 * @brief The ResourceWrapper class creates and manages its own instance of
 * evernote::edam::Resource object
 */
class ResourceWrapper final: public IResource
{
public:
    ResourceWrapper();
    ResourceWrapper(const ResourceWrapper & other);
    ResourceWrapper & operator=(const ResourceWrapper & other);
    virtual ~ResourceWrapper() final override;

    virtual const evernote::edam::Resource & GetEnResource() const final override;
    virtual evernote::edam::Resource & GetEnResource() final override;

private:
    evernote::edam::Resource m_en_resource;
};

} // namespace qute_note

#endif // __QUTE_NOTE__CLIENT__TYPES__RESOURCE_WRAPPER_H
