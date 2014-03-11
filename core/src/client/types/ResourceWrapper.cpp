#include "ResourceWrapper.h"

namespace qute_note {

ResourceWrapper::ResourceWrapper() :
    IResource(),
    m_en_resource()
{}

ResourceWrapper::ResourceWrapper(const ResourceWrapper & other) :
    IResource(other),
    m_en_resource(other.m_en_resource)
{}

ResourceWrapper & ResourceWrapper::operator=(const ResourceWrapper & other)
{
    if (this != &other)
    {
        IResource::operator =(other);
        m_en_resource = other.m_en_resource;
    }

    return *this;
}

ResourceWrapper::~ResourceWrapper()
{}

const evernote::edam::Resource & ResourceWrapper::GetEnResource() const
{
    return m_en_resource;
}

evernote::edam::Resource & ResourceWrapper::GetEnResource()
{
    return m_en_resource;
}

QTextStream & ResourceWrapper::Print(QTextStream & strm) const
{
    strm << "ResourceWrapper { \n";
    strm << IResource::ToQString();
    strm << "} \n";

    return strm;
}

} // namespace qute_note
