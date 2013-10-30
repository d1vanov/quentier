#include "NoteImpl.h"
#include "../evernote_client/Notebook.h"

namespace qute_note {

Note::NoteImpl::NoteImpl(const Notebook & notebook) :
    m_notebookGuid(notebook.guid()),
    m_title(),
    m_content(),
    m_resources()
{}

Note::NoteImpl::NoteImpl(const NoteImpl & other) :
    m_notebookGuid(other.m_notebookGuid),
    m_title(other.m_title),
    m_content(other.m_content),
    m_resources(other.m_resources)
{}

Note::NoteImpl & Note::NoteImpl::operator=(const NoteImpl & other)
{
    if (this != &other) {
        m_notebookGuid = other.m_notebookGuid;
        m_title = other.m_title;
        m_content = other.m_content;
        m_resources = other.m_resources;
    }

    return *this;
}

const QString & Note::NoteImpl::title() const
{
    return m_title;
}

void Note::NoteImpl::setTitle(const QString & title)
{
    m_title = title;
}

const QString & Note::NoteImpl::content() const
{
    return m_content;
}

void Note::NoteImpl::setContent(const QString & content)
{
    m_content = content;
}

const std::vector<Resource> & Note::NoteImpl::resources() const
{
    return m_resources;
}

std::vector<Resource> & Note::NoteImpl::resources()
{
    return m_resources;
}

const Guid & Note::NoteImpl::notebookGuid() const
{
    return m_notebookGuid;
}



}
