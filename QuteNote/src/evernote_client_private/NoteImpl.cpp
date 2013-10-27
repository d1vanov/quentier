#include "NoteImpl.h"
#include "../evernote_client/Notebook.h"

namespace qute_note {

Note::NoteImpl::NoteImpl(const Notebook & notebook) :
    m_notebookGuid(notebook.guid())
{}

Note::NoteImpl::NoteImpl(const NoteImpl & other) :
    m_notebookGuid(other.m_notebookGuid)
{}

Note::NoteImpl & Note::NoteImpl::operator=(const NoteImpl & other)
{
    if (this != &other) {
        m_notebookGuid = other.m_notebookGuid;
    }

    return *this;
}

const Guid & Note::NoteImpl::notebookGuid() const
{
    return m_notebookGuid;
}



}
