#include "data/ResourceWrapperData.h"
#include <qute_note/types/ResourceWrapper.h>

namespace qute_note {

ResourceWrapper::ResourceWrapper() :
    IResource(),
    d(new ResourceWrapperData)
{}

ResourceWrapper::ResourceWrapper(const IResource & other) :
    IResource(other),
    d(new ResourceWrapperData(other.GetEnResource()))
{}

ResourceWrapper::ResourceWrapper(const ResourceWrapper & other) :
    IResource(other),
    d(other.d)
{}

ResourceWrapper::ResourceWrapper(ResourceWrapper && other) :
    IResource(std::move(other)),
    d(other.d)
{}

ResourceWrapper::ResourceWrapper(const qevercloud::Resource & other) :
    IResource(),
    d(new ResourceWrapperData(other))
{}

ResourceWrapper::ResourceWrapper(qevercloud::Resource && other) :
    IResource(),
    d(new ResourceWrapperData(other))
{}

ResourceWrapper & ResourceWrapper::operator=(const ResourceWrapper & other)
{
    IResource::operator=(other);

    if (this != &other) {
        d = other.d;
    }

    return *this;
}

ResourceWrapper & ResourceWrapper::operator=(ResourceWrapper && other)
{
    IResource::operator=(other);

    if (this != &other) {
        d = other.d;
    }

    return *this;
}

ResourceWrapper::~ResourceWrapper()
{}

const qevercloud::Resource & ResourceWrapper::GetEnResource() const
{
    return d->m_qecResource;
}

qevercloud::Resource & ResourceWrapper::GetEnResource()
{
    return d->m_qecResource;
}

QTextStream & ResourceWrapper::print(QTextStream & strm) const
{
    strm << "ResourceWrapper { \n";
    IResource::print(strm);
    strm << "} \n";

    return strm;
}

} // namespace qute_note
