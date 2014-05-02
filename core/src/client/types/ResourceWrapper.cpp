#include "ResourceWrapper.h"

namespace qute_note {

ResourceWrapper::ResourceWrapper() :
    IResource(),
    m_enResource()
{}

ResourceWrapper::ResourceWrapper(const IResource & other) :
    IResource(other),
    m_enResource(other.GetEnResource())
{}

ResourceWrapper::ResourceWrapper(const qevercloud::Resource & other) :
    IResource(),
    m_enResource(other)
{}

ResourceWrapper::ResourceWrapper(qevercloud::Resource && other) :
    IResource(),
    m_enResource(std::move(other))
{}

ResourceWrapper & ResourceWrapper::operator=(const IResource & other)
{
    if (this != &other)
    {
        IResource::operator =(other);
        m_enResource = other.GetEnResource();
    }

    return *this;
}

ResourceWrapper::~ResourceWrapper()
{}

const qevercloud::Resource & ResourceWrapper::GetEnResource() const
{
    return m_enResource;
}

qevercloud::Resource & ResourceWrapper::GetEnResource()
{
    return m_enResource;
}

QTextStream & ResourceWrapper::Print(QTextStream & strm) const
{
    strm << "ResourceWrapper { \n";
    IResource::Print(strm);
    strm << "} \n";

    return strm;
}

} // namespace qute_note
