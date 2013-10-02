#include "TagImpl.h"

namespace qute_note {

Tag::TagImpl::TagImpl(const std::string & name) :
    m_name(name),
    m_parentGuid()
{}

Tag::TagImpl::TagImpl(const std::string & name, const Guid & parentGuid) :
    m_name(name),
    m_parentGuid(parentGuid)
{}

const std::string Tag::TagImpl::name() const
{
    return m_name;
}

const Guid Tag::TagImpl::parentGuid() const
{
    return m_parentGuid;
}

void Tag::TagImpl::setName(const std::string & name)
{
    m_name = name;
}

void Tag::TagImpl::setParentGuid(const Guid & parentGuid)
{
    m_parentGuid = parentGuid;
}

}
