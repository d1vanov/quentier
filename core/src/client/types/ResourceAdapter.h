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
class QUTE_NOTE_EXPORT ResourceAdapter : public IResource
{
public:
    ResourceAdapter(qevercloud::Resource & externalEnResource);
    ResourceAdapter(const qevercloud::Resource & externalEnResource);
    ResourceAdapter(const ResourceAdapter & other);
    ResourceAdapter(ResourceAdapter && other);
    ResourceAdapter & operator=(const ResourceAdapter & other);
    virtual ~ResourceAdapter() Q_DECL_OVERRIDE;

    friend class Note;

private:
    virtual const qevercloud::Resource & GetEnResource() const Q_DECL_OVERRIDE;
    virtual qevercloud::Resource & GetEnResource() Q_DECL_OVERRIDE;

    virtual QTextStream & Print(QTextStream & strm) const;

    ResourceAdapter() Q_DECL_DELETE;

    qevercloud::Resource * m_pEnResource;
    bool m_isConst;
};

} // namespace qute_note

#endif // __QUTE_NOTE__CLIENT__TYPES__RESOURCE_ADAPTER_H
