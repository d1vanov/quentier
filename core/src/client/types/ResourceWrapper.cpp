#include "ResourceWrapper.h"

namespace qute_note {

ResourceWrapper::ResourceWrapper() :
    IResource(),
    m_en_resource()
{}

ResourceWrapper::ResourceWrapper(const IResource & other) :
    IResource(other),
    m_en_resource(other.GetEnResource())
{}

ResourceWrapper & ResourceWrapper::operator=(const IResource & other)
{
    if (this != &other)
    {
        IResource::operator =(other);
        m_en_resource = other.GetEnResource();
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
    IResource::Print(strm);
    strm << "} \n";

    return strm;
}

} // namespace qute_note
