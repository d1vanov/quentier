#include "Tag.h"
#include "../evernote_client_private/TagImpl.h"

namespace qute_note {

Tag::Tag(const std::string & name) :
    m_pImpl(new TagImpl(name))
{}

Tag::Tag(const std::string & name, const Guid & parentGuid) :
    m_pImpl(new TagImpl(name, parentGuid))
{}

Tag::Tag(const Tag & other) :
    m_pImpl(nullptr)
{
    m_pImpl.reset(new TagImpl(other.name(), other.parentGuid()));
}

Tag & Tag::operator=(const Tag & other)
{
    if (this != &other) {
        m_pImpl.reset(new TagImpl(other.name(), other.parentGuid()));
    }

    return *this;
}

const Guid Tag::parentGuid() const
{
    if (m_pImpl != nullptr) {
        return m_pImpl->parentGuid();
    }
    else {
        return Guid();
    }
}

const std::string Tag::name() const
{
    if (m_pImpl != nullptr) {
        return m_pImpl->name();
    }
    else {
        return std::string();
    }
}

void Tag::setName(const std::string & name)
{
    if (m_pImpl != nullptr) {
        m_pImpl->setName(name);
    }
}

void Tag::setParentGuid(const Guid & parentGuid)
{
    if (m_pImpl != nullptr) {
        m_pImpl->setParentGuid(parentGuid);
    }
}

}
