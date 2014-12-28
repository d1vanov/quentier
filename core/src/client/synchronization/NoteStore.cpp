#include "NoteStore.h"
#include <client/types/Notebook.h>
#include <client/types/Note.h>
#include <client/types/Tag.h>
#include <client/types/SavedSearch.h>

namespace qute_note {

NoteStore::NoteStore() :
    m_qecNoteStore()
{}

void NoteStore::setNoteStoreUrl(const QString & noteStoreUrl)
{
    m_qecNoteStore.setNoteStoreUrl(noteStoreUrl);
}

void NoteStore::setAuthenticationToken(const QString & authToken)
{
    m_qecNoteStore.setAuthenticationToken(authToken);
}

bool NoteStore::createNotebook(Notebook & notebook, QString & errorDescription)
{
    try
    {
        notebook = m_qecNoteStore.createNotebook(notebook);
        return true;
    }
    catch(const qevercloud::EDAMUserException & userException)
    {
        // TODO: convert from exception to errorDescription string
    }
    catch(const qevercloud::EDAMSystemException & systemException)
    {
        // TODO: convert from exception to errorDescription string
    }

    return false;
}

bool NoteStore::updateNotebook(Notebook & notebook, QString & errorDescription)
{
    try
    {
        qint32 usn = m_qecNoteStore.updateNotebook(notebook);
        notebook.setUpdateSequenceNumber(usn);
        return true;
    }
    catch(const qevercloud::EDAMUserException & userException)
    {
        // TODO: convert from exception to errorDescription string
    }
    catch(const qevercloud::EDAMSystemException & systemException)
    {
        // TODO: convert from exception to errorDescription string
    }
    catch(const qevercloud::EDAMNotFoundException & notFoundException)
    {
        // TODO: convert from exception to errorDescription string
    }

    return false;
}

bool NoteStore::createNote(Note & note, QString & errorDescription)
{
    try
    {
        note = m_qecNoteStore.createNote(note);
        return true;
    }
    catch(const qevercloud::EDAMUserException & userException)
    {
        // TODO: convert from exception to errorDescription string
    }
    catch(const qevercloud::EDAMSystemException & systemException)
    {
        // TODO: convert from exception to errorDescription string
    }

    return false;
}

bool NoteStore::updateNote(Note & note, QString & errorDescription)
{
    try
    {
        note = m_qecNoteStore.updateNote(note);
        return true;
    }
    catch(const qevercloud::EDAMUserException & userException)
    {
        // TODO: convert from exception to errorDescription string
    }
    catch(const qevercloud::EDAMSystemException & systemException)
    {
        // TODO: convert from exception to errorDescription string
    }
    catch(const qevercloud::EDAMNotFoundException & notFoundException)
    {
        // TODO: convert from exception to errorDescription string
    }

    return false;
}

bool NoteStore::createTag(Tag & tag, QString & errorDescription)
{
    try
    {
        tag = m_qecNoteStore.createTag(tag);
        return true;
    }
    catch(const qevercloud::EDAMUserException & userException)
    {
        // TODO: convert from exception to errorDescription string
    }
    catch(const qevercloud::EDAMSystemException & systemException)
    {
        // TODO: convert from exception to errorDescription string
    }

    return false;
}

bool NoteStore::updateTag(Tag & tag, QString & errorDescription)
{
    try
    {
        qint32 usn = m_qecNoteStore.updateTag(tag);
        tag.setUpdateSequenceNumber(usn);
        return true;
    }
    catch(const qevercloud::EDAMUserException & userException)
    {
        // TODO: convert from exception to errorDescription string
    }
    catch(const qevercloud::EDAMSystemException & systemException)
    {
        // TODO: convert from exception to errorDescription string
    }
    catch(const qevercloud::EDAMNotFoundException & notFoundException)
    {
        // TODO: convert from exception to errorDescription string
    }

    return false;
}

bool NoteStore::createSavedSearch(SavedSearch & savedSearch, QString & errorDescription)
{
    try
    {
        savedSearch = m_qecNoteStore.createSearch(savedSearch);
        return true;
    }
    catch(const qevercloud::EDAMUserException & userException)
    {
        // TODO: convert from exception to errorDescription string
    }
    catch(const qevercloud::EDAMSystemException & systemException)
    {
        // TODO: convert from exception to errorDescription string
    }

    return false;
}

bool NoteStore::updateSavedSearch(SavedSearch & savedSearch, QString & errorDescription)
{
    try
    {
        qint32 usn = m_qecNoteStore.updateSearch(savedSearch);
        savedSearch.setUpdateSequenceNumber(usn);
        return true;
    }
    catch(const qevercloud::EDAMUserException & userException)
    {
        // TODO: convert from exception to errorDescription string
    }
    catch(const qevercloud::EDAMSystemException & systemException)
    {
        // TODO: convert from exception to errorDescription string
    }
    catch(const qevercloud::EDAMNotFoundException & notFoundException)
    {
        // TODO: convert from exception to errorDescription string
    }

    return false;
}

bool NoteStore::getSyncChunk(const qint32 afterUSN, const qint32 maxEntries,
                             const qevercloud::SyncChunkFilter & filter,
                             qevercloud::SyncChunk & syncChunk)
{
    try
    {
        syncChunk = m_qecNoteStore.getFilteredSyncChunk(afterUSN, maxEntries, filter);
        return true;
    }
    catch(const qevercloud::EDAMUserException & userException)
    {
        // TODO: convert from exception to errorDescription string
    }
    catch(const qevercloud::EDAMSystemException & systemException)
    {
        // TODO: convert from exception to errorDescription string
    }

    return false;
}

} // namespace qute_note
