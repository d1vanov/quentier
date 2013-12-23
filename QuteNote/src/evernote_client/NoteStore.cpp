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

#define PROCESS_EDAM_EXCEPTIONS(item) \
    catch(const evernote::edam::EDAMUserException & userException) \
    { \
        QString error = QObject::tr("Caught EDAM user exception: "); \
        error.append(userException.what()); \
        item.SetError(error); \
        return; \
    } \
    catch(const evernote::edam::EDAMNotFoundException & notFoundException) \
    { \
        QString error = QObject::tr("Caught EDAM not found exception: "); \
        error.append(notFoundException.what()); \
        item.SetError(error); \
        return; \
    } \
    catch(const evernote::edam::EDAMSystemException & systemException) \
    { \
        QString error = QObject::tr("Caught EDAM system exception: "); \
        error.append(systemException.what()); \
        item.SetError(error); \
        return; \
    }

void NoteStore::CreateNote(Note & note)
{
    Guid notebookGuid = note.notebookGuid();

    if (notebookGuid.isEmpty()) {
        note.SetError("Notebook guid is empty");
        return;
    }

    evernote::edam::Note edamNote;
    edamNote.notebookGuid = notebookGuid.ToQString().toStdString();
    edamNote.__isset.notebookGuid = true;

    Q_D(NoteStore);

    try {
        d->m_pNoteStoreClient->createNote(edamNote, d->m_authToken, edamNote);
    }
    PROCESS_EDAM_EXCEPTIONS(note);

    note.assignGuid(edamNote.guid);
}

void NoteStore::CreateNotebook(Notebook & notebook)
{
    QString notebookName = notebook.name();

    if (notebookName.isEmpty()) {
        notebook.SetError("Name is empty");
        return;
    }

    evernote::edam::Notebook edamNotebook;
    edamNotebook.name = notebookName.toStdString();
    edamNotebook.__isset.name = true;

    Q_D(NoteStore);

    try {
        d->m_pNoteStoreClient->createNotebook(edamNotebook, d->m_authToken, edamNotebook);
    }
    PROCESS_EDAM_EXCEPTIONS(notebook);

    notebook.assignGuid(edamNotebook.guid);
}

void NoteStore::CreateTag(Tag & tag)
{
    QString tagName = tag.name();

    if (tagName.isEmpty()) {
        tag.SetError("Name is empty");
        return;
    }

    evernote::edam::Tag edamTag;
    edamTag.name = tagName.toStdString();
    edamTag.__isset.name = true;

    Guid parentGuid = tag.parentGuid();
    if (!parentGuid.isEmpty()) {
        edamTag.parentGuid = parentGuid.ToQString().toStdString();
        edamTag.__isset.parentGuid = true;
    }

    Q_D(NoteStore);

    try {
        d->m_pNoteStoreClient->createTag(edamTag, d->m_authToken, edamTag);
    }
    PROCESS_EDAM_EXCEPTIONS(tag);

    tag.assignGuid(edamTag.guid);
}

#undef PROCESS_EDAM_EXCEPTIONS

}
