#include "Tag.h"
#include "TagImpl.h"

namespace qute_note {

Tag::Tag(const std::string & name) :
    m_pImpl(new TagImpl(name))
{}

Tag::Tag(const std::string & name, const Guid & parentGuid) :
    m_pImpl(new TagImpl(name, parentGuid))
{}

Guid Tag::parentGuid() const
{
    if (m_pImpl != nullptr) {
        return m_pImpl->parentGuid();
    }
    else {
        return Guid();
    }
}




}
