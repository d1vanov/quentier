#include "../evernote_client_private/NoteStore_p.h"
#include "../evernote_sdk/src/NoteStore.h"
#include "../evernote_sdk/src/Errors_types.h"
#include "Note.h"
#include "Notebook.h"
#include "Tag.h"
#include "ResourceMetadata.h"
#include <QDateTime>

namespace qute_note {

NoteStore::NoteStore(const QString & authenticationToken, const QString & host,
                     const int port, const QString & noteStorePath) :
    d_ptr(new NoteStorePrivate(authenticationToken, host, port, noteStorePath))
{}

NoteStore::~NoteStore()
{}

void NoteStore::CreateNote(Note & note)
{
    if (note.notebookGuid().isEmpty()) {
        return;
    }

    evernote::edam::Note edamNote;
    Q_D(NoteStore);

    try
    {
        d->m_pNoteStoreClient->createNote(edamNote, d->m_authToken, edamNote);
    }
    catch(const evernote::edam::EDAMUserException & userException)
    {
        QString error = "Caught EDAM_UserException: ";
        error.append(userException.what());
        note.SetError(error);
    }
    catch(const evernote::edam::EDAMNotFoundException & notFoundException)
    {
        QString error = "Caught EDAM_NotFoundException: ";
        error.append(notFoundException.what());
        note.SetError(error);
    }

    note.assignGuid(edamNote.guid);
}

}
