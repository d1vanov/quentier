#include "ResourceImpl.h"

namespace qute_note {

Resource::ResourceImpl::ResourceImpl(const Guid & noteGuid, const QString & resourceName,
                                     const QByteArray * pResourceBinaryData) :
    m_noteGuid(noteGuid),
    m_resourceName(resourceName),
    m_pResourceBinaryData(pResourceBinaryData)
{}

Resource::ResourceImpl::ResourceImpl(const ResourceImpl & other) :
    m_noteGuid(other.m_noteGuid),
    m_resourceName(other.m_resourceName),
    m_pResourceBinaryData(other.m_pResourceBinaryData)
{}

Resource::ResourceImpl & Resource::ResourceImpl::operator=(const ResourceImpl & other)
{
    if (this != &other) {
        m_noteGuid = other.m_noteGuid;
        m_resourceName = other.m_resourceName;
        m_pResourceBinaryData = other.m_pResourceBinaryData;
    }

    return *this;
}

Resource::ResourceImpl::~ResourceImpl()
{}

const Guid & Resource::ResourceImpl::noteGuid() const
{
    return m_noteGuid;
}

const QString & Resource::ResourceImpl::name() const
{
    return m_resourceName;
}

const QByteArray * Resource::ResourceImpl::data() const
{
    return m_pResourceBinaryData;
}

void Resource::ResourceImpl::setData(const QByteArray * pResourceBinaryData)
{
    m_pResourceBinaryData = pResourceBinaryData;
}

}


