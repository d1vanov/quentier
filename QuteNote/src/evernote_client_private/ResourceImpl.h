#ifndef __QUTE_NOTE__EVERNOTE_CLIENT__RESOURCE_IMPL_H
#define __QUTE_NOTE__EVERNOTE_CLIENT__RESOURCE_IMPL_H

#include "../evernote_client/Resource.h"
#include "../evernote_client/Guid.h"
#include <QString>
#include <QByteArray>

namespace qute_note {

// FIXME: it's a stub until the storage mechanism is implemented. The this class will need a complete revamp
class Resource::ResourceImpl
{
public:
    ResourceImpl(const Guid & noteGuid,
                 const QByteArray & resourceBinaryData,
                 const QString & resourceMimeType);

    ResourceImpl(const Guid & noteGuid, const QMimeData & resourceMimeData);

    ResourceImpl(const ResourceImpl & other);

    ResourceImpl & operator=(const ResourceImpl & other);

    ~ResourceImpl();

    const Guid & noteGuid() const;

    const QString & name() const;

    const QMimeData & mimeData() const;
    void setData(const QByteArray & resourceBinaryData,
                 const QString & resourceMimeType);

    const QString & mimeType() const;

private:
    ResourceImpl() = delete;

    Guid        m_noteGuid;
    QString     m_resourceMimeType;
    QMimeData   m_resourceMimeData;
};

}

#endif // __QUTE_NOTE__EVERNOTE_CLIENT__RESOURCE_IMPL_H
