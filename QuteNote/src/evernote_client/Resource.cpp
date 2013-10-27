#include "Resource.h"
#include "../evernote_client_private/ResourceImpl.h"
#include "../tools/QuteNoteCheckPtr.h"
#include <QTranslator>

#define CHECK_POINTER_TO_IMPL() \
    QUTE_NOTE_CHECK_PTR(m_pImpl.get(), \
                        QObject::tr("Pointer to ResourceImpl is empty in Resource object " \
                        "when trying to set binary data"))

namespace qute_note {

Resource::Resource(const QString & name, const Guid & noteGuid) :
    m_pImpl(new ResourceImpl(noteGuid, name, QByteArray(), QString()))
{}

Resource::~Resource()
{}

const QString Resource::name() const
{
    if (m_pImpl.get() == nullptr) {
        return QString();
    }
    else {
        return m_pImpl->name();
    }
}

void Resource::setData(const QByteArray & resourceBinaryData, const QString & mimeType)
{
    CHECK_POINTER_TO_IMPL()
    m_pImpl->setData(resourceBinaryData, mimeType);
}

const QString Resource::dataHash() const
{
    // TODO: implement
    return QString();
}

std::size_t Resource::dataSize() const
{
    // TODO: implement;
    return static_cast<std::size_t>(0);
}

const Guid Resource::noteGuid() const
{
    if (m_pImpl == nullptr) {
        return Guid();
    }
    else {
        return m_pImpl->noteGuid();
    }
}

const QMimeData & Resource::mimeData() const
{
    CHECK_POINTER_TO_IMPL()
    return m_pImpl->mimeData();
}

const QString Resource::mimeType() const
{
    CHECK_POINTER_TO_IMPL()
    return m_pImpl->mimeType();
}

const std::size_t Resource::width() const
{
    // TODO: implement;
    return static_cast<std::size_t>(0);
}

const std::size_t Resource::height() const
{
    // TODO: implement;
    return static_cast<std::size_t>(0);
}

#undef CHECK_POINTER_TO_IMPL

}
