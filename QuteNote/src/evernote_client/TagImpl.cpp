#include "TagImpl.h"

namespace qute_note {

TagImpl::TagImpl(const std::string & name) :
    m_name(name),
    m_parentGuid()
{}

TagImpl::TagImpl(const std::string & name, const Guid & parentGuid) :
    m_name(name),
    m_parentGuid(parentGuid)
{}

std::string TagImpl::name()
{
    return m_name;
}

Guid TagImpl::parentGuid()
{
    return m_parentGuid;
}

void TagImpl::setName(const std::string & name)
{
    m_name = name;
}

void TagImpl::setParentGuid(const Guid & parentGuid)
{
    m_parentGuid = parentGuid;
}

}
