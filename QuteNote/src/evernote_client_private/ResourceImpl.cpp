#include "ResourceImpl.h"
#include <QStringList>

namespace qute_note {

Resource::ResourceImpl::ResourceImpl(const Guid & noteGuid,
                                     const QByteArray & resourceBinaryData,
                                     const QString & resourceMimeType) :
    m_noteGuid(noteGuid),
    m_resourceMimeType(resourceMimeType),
    m_resourceMimeData()
{
    m_resourceMimeData.setData(resourceMimeType, resourceBinaryData);
}

Resource::ResourceImpl::ResourceImpl(const Guid & noteGuid,
                                     const QMimeData & resourceMimeData) :
    m_noteGuid(noteGuid),
    m_resourceMimeType(),
    m_resourceMimeData()
{
    QStringList formats = m_resourceMimeData.formats();
    if (!formats.isEmpty()) {
        m_resourceMimeType = formats.first();
    }

    m_resourceMimeData.setData(m_resourceMimeType, resourceMimeData.data(m_resourceMimeType));
}

Resource::ResourceImpl::ResourceImpl(const ResourceImpl & other) :
    m_noteGuid(other.m_noteGuid),
    m_resourceMimeType(other.m_resourceMimeType),
    m_resourceMimeData()
{
    QByteArray resourceBinaryData = other.m_resourceMimeData.data(m_resourceMimeType);
    m_resourceMimeData.setData(m_resourceMimeType, resourceBinaryData);
}

Resource::ResourceImpl & Resource::ResourceImpl::operator=(const ResourceImpl & other)
{
    if (this != &other)
    {
        m_noteGuid = other.m_noteGuid;
        m_resourceMimeType = other.m_resourceMimeType;

        QByteArray resourceBinaryData = other.m_resourceMimeData.data(m_resourceMimeType);
        m_resourceMimeData.setData(m_resourceMimeType, resourceBinaryData);
    }

    return *this;
}

Resource::ResourceImpl::~ResourceImpl()
{}

const Guid & Resource::ResourceImpl::noteGuid() const
{
    return m_noteGuid;
}

const QMimeData & Resource::ResourceImpl::mimeData() const
{
    return m_resourceMimeData;
}

void Resource::ResourceImpl::setData(const QByteArray & resourceBinaryData,
                                     const QString & resourceMimeType)
{
    m_resourceMimeType = resourceMimeType;
    m_resourceMimeData.setData(m_resourceMimeType, resourceBinaryData);
}

const QString & Resource::ResourceImpl::mimeType() const
{
    return m_resourceMimeType;
}

}


