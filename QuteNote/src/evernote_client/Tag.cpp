#include "Tag.h"
#include "TagImpl.h"

namespace qute_note {

Tag::Tag(const std::string & name) :
    m_pImpl(new TagImpl(name))
{}

Tag::Tag(const std::string & name, const Guid & parentGuid) :
    m_pImpl(new TagImpl(name, parentGuid))
{}

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
