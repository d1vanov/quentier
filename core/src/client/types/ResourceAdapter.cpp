#include "ResourceAdapter.h"
#include "ResourceAdapterAccessException.h"
#include <tools/QuteNoteCheckPtr.h>

namespace qute_note {

ResourceAdapter::ResourceAdapter(evernote::edam::Resource & externalEnResource) :
    IResource(),
    m_pEnResource(&externalEnResource),
    m_isConst(false)
{}

ResourceAdapter::ResourceAdapter(const evernote::edam::Resource & externalEnResource) :
    IResource(),
    m_pEnResource(const_cast<evernote::edam::Resource*>(&externalEnResource)),
    m_isConst(true)
{}

ResourceAdapter::ResourceAdapter(const ResourceAdapter & other) :
    IResource(other),
    m_pEnResource(other.m_pEnResource),
    m_isConst(other.m_isConst)
{}

ResourceAdapter::~ResourceAdapter()
{}

const evernote::edam::Resource & ResourceAdapter::GetEnResource() const
{
    QUTE_NOTE_CHECK_PTR(m_pEnResource, "Null pointer to external resource in ResourceAdapter");
    return static_cast<const evernote::edam::Resource&>(*m_pEnResource);
}

evernote::edam::Resource & ResourceAdapter::GetEnResource()
{
    if (m_isConst) {
        throw ResourceAdapterAccessException("Attempt to access non-const reference "
                                             "to resource from constant ResourceAdapter");
    }

    QUTE_NOTE_CHECK_PTR(m_pEnResource, "Null pointer to external resource in ResourceAdapter");
    return static_cast<evernote::edam::Resource&>(*m_pEnResource);
}

QTextStream & ResourceAdapter::Print(QTextStream & strm) const
{
    strm << "ResourceAdapter { \n";
    IResource::Print(strm);
    strm << "} \n";

    return strm;
}

} // namespace qute_note
