#ifndef __QUTE_NOTE__CLIENT__TYPES__RESOURCE_ADAPTER_H
#define __QUTE_NOTE__CLIENT__TYPES__RESOURCE_ADAPTER_H

#include "IResource.h"

namespace qute_note {

/**
 * @brief The ResourceAdapter class uses reference to external qevercloud::Resource
 * and adapts its interface to that of IResource. The instances of this class
 * should be used only within the same scope as the referenced external
 * qevercloud::Resource object, otherwise it is possible to run into undefined behaviour.
 * ResourceAdapter class is aware of constness of external object it references,
 * it would throw ResourceAdapterAccessException exception in attempts to use
 * referenced const object in non-const context
 *
 * @see ResourceAdapterAccessException
 */
class ResourceAdapter final: public IResource
{
public:
    ResourceAdapter(qevercloud::Resource & externalEnResource);
    ResourceAdapter(const qevercloud::Resource & externalEnResource);
    ResourceAdapter(const ResourceAdapter & other);
    ResourceAdapter(ResourceAdapter && other);
    virtual ~ResourceAdapter() final override;

private:
    virtual const qevercloud::Resource & GetEnResource() const final override;
    virtual qevercloud::Resource & GetEnResource() final override;

    virtual QTextStream & Print(QTextStream & strm) const;

    ResourceAdapter() = delete;
    ResourceAdapter & operator=(const ResourceAdapter & other) = delete;

    qevercloud::Resource * m_pEnResource;
    bool m_isConst;
};

} // namespace qute_note

#endif // __QUTE_NOTE__CLIENT__TYPES__RESOURCE_ADAPTER_H
