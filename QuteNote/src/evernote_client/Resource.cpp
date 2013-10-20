#include "Resource.h"
#include "ResourceImpl.h"
#include "../tools/QuteNoteCheckPtr.h"

#define CHECK_POINTER_TO_IMPL() \
    QUTE_NOTE_CHECK_PTR(m_pImpl.get(), \
                        "Pointer to ResourceImpl is empty in Resource object " \
                        "when trying to set binary data")

namespace qute_note {

Resource::Resource(const QString & name, const Guid & noteGuid) :
    m_pImpl(new ResourceImpl(noteGuid, name, nullptr))
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

const QByteArray Resource::data() const
{
    if (m_pImpl.get() == nullptr) {
        return QByteArray();
    }
    else {
        const QByteArray * pResourceBinaryData = m_pImpl->data();
        if (pResourceBinaryData != nullptr) {
            return *pResourceBinaryData;
        }
        else {
            return QByteArray();
        }
    }
}

void Resource::setData(const QByteArray & resourceBinaryData)
{
    CHECK_POINTER_TO_IMPL()
    m_pImpl->setData(&resourceBinaryData);
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
