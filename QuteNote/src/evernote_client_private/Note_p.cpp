#include "Note_p.h"
#include "../evernote_client/Notebook.h"

namespace qute_note {

NotePrivate::NotePrivate()
{}

NotePrivate::NotePrivate(const Guid & notebookGuid) :
    m_notebookGuid(notebookGuid)
{}

NotePrivate::NotePrivate(const NotePrivate & other) :
    m_notebookGuid(other.m_notebookGuid),
    m_title(other.m_title),
    m_content(other.m_content),
    m_author(other.m_author),
    m_resourceGuids(other.m_resourceGuids),
    m_tagGuids(other.m_tagGuids),
    m_creationTimestamp(other.m_creationTimestamp),
    m_modificationTimestamp(other.m_modificationTimestamp),
    m_subjectDateTimestamp(other.m_subjectDateTimestamp),
    m_location(other.m_location),
    m_source(other.m_source),
    m_sourceUrl(other.m_sourceUrl),
    m_sourceApplication(other.m_sourceApplication)
{}

NotePrivate & NotePrivate::operator =(const NotePrivate & other)
{
    if (this != &other)
    {
        m_notebookGuid = other.m_notebookGuid;
        m_title = other.m_title;
        m_content = other.m_content;
        m_author = other.m_author;
        m_resourceGuids = other.m_resourceGuids;
        m_tagGuids = other.m_tagGuids;
        m_creationTimestamp = other.m_creationTimestamp;
        m_modificationTimestamp = other.m_modificationTimestamp;
        m_subjectDateTimestamp = other.m_subjectDateTimestamp;
        m_location = other.m_location;
        m_source = other.m_source;
        m_sourceUrl = other.m_sourceUrl;
        m_sourceApplication = other.m_sourceApplication;
    }

    return *this;
}

}
