#include "Resource.h"
#include "../evernote_client_private/ResourceImpl.h"
#include "../tools/QuteNoteCheckPtr.h"
#include <QTranslator>

#define CHECK_POINTER_TO_IMPL() \
    QUTE_NOTE_CHECK_PTR(m_pImpl.get(), \
                        QObject::tr("Pointer to ResourceImpl is empty in Resource object " \
                        "when trying to set binary data"))

namespace qute_note {

Resource::Resource(const Guid & noteGuid, const QByteArray & resourceBinaryData,
                   const QString & resourceMimeType) :
    m_pImpl(new ResourceImpl(noteGuid, resourceBinaryData, resourceMimeType))
{}

Resource::Resource(const Guid & noteGuid, const QMimeData & resourceMimeData) :
    m_pImpl(new ResourceImpl(noteGuid, resourceMimeData))
{}

Resource::Resource(const Resource & other) :
    m_pImpl(new ResourceImpl(other.noteGuid(), other.mimeData()))
{}

Resource & Resource::operator=(const Resource &other)
{
    if (this != &other) {
        m_pImpl.reset(new ResourceImpl(other.noteGuid(),
                                       other.mimeData().data(other.mimeType()),
                                       other.mimeType()));
    }

    return *this;
}

Resource::~Resource()
{}

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
