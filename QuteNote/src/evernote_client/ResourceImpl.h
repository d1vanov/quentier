#ifndef __QUTE_NOTE__EVERNOTE_CLIENT__RESOURCE_IMPL_H
#define __QUTE_NOTE__EVERNOTE_CLIENT__RESOURCE_IMPL_H

#include "Resource.h"
#include "Guid.h"
#include <QString>
#include <QByteArray>

namespace qute_note {

class Resource::ResourceImpl
{
public:
    ResourceImpl(const Guid & noteGuid, const QString & resourceName,
                 const QByteArray * pResourceBinaryData);
    ResourceImpl(const ResourceImpl & other);
    ResourceImpl & operator=(const ResourceImpl & other);
    ~ResourceImpl();

    const Guid & noteGuid() const;

    const QString & name() const;

    const QByteArray * data() const;
    void setData(const QByteArray * pResourceBinaryData);

private:
    ResourceImpl() = delete;

    Guid      m_noteGuid;
    QString   m_resourceName;
    const QByteArray * m_pResourceBinaryData;
};

}

#endif // __QUTE_NOTE__EVERNOTE_CLIENT__RESOURCE_IMPL_H
