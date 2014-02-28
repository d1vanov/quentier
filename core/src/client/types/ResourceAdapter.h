#ifndef __QUTE_NOTE__CLIENT__TYPES__RESOURCE_ADAPTER_H
#define __QUTE_NOTE__CLIENT__TYPES__RESOURCE_ADAPTER_H

#include "IResource.h"

namespace qute_note {

/**
 * @brief The ResourceAdapter class uses reference to external evernote::edam::Resource
 * and adapts its interface to that of IResource
 */
class ResourceAdapter final: public IResource
{
public:
    ResourceAdapter(evernote::edam::Resource & externalEnResource);
    ResourceAdapter(const evernote::edam::Resource & externalEnResource);
    ResourceAdapter(const ResourceAdapter & other);
    virtual ~ResourceAdapter() final override;

    virtual const evernote::edam::Resource & GetEnResource() const final override;
    virtual evernote::edam::Resource & GetEnResource() final override;

private:
    ResourceAdapter() = delete;
    ResourceAdapter & operator=(const ResourceAdapter & other) = delete;

    evernote::edam::Resource * m_pEnResource;
    bool m_isConst;
};

} // namespace qute_note

#endif // __QUTE_NOTE__CLIENT__TYPES__RESOURCE_ADAPTER_H
