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

Note NoteStore::CreateNoteFromEdamNote(const evernote::edam::Note & edamNote) const
{
    if (!edamNote.__isset.guid) {
        Guid emptyGuid;
        Note note(emptyGuid, emptyGuid);
        note.SetError("EDAM note's guid is empty");
        return std::move(note);
    }

    if (!edamNote.__isset.notebookGuid) {
        Guid emptyGuid;
        Note note(emptyGuid, emptyGuid);
        note.SetError("EDAM note's notebook guid is empty");
        return std::move(note);
    }

    Note note(Guid(edamNote.guid), Guid(edamNote.notebookGuid));
    return std::move(note);
}

void NoteStore::ConvertFromEdamNote(const evernote::edam::Note & edamNote, Note & note)
{
    QString errorPrefix = QObject::tr("Can't convert from EDAM note: ");

    if (!edamNote.__isset.guid) {
        errorPrefix.append("EDAM note's guid is not set");
        note.SetError(errorPrefix);
        return;
    }

    if (!edamNote.__isset.notebookGuid) {
        errorPrefix.append("EDAM note's notebook guid is not set");
        note.SetError(errorPrefix);
        return;
    }

    if (note.notebookGuid() != Guid(edamNote.notebookGuid)) {
        errorPrefix.append("notebook guids mismatch");
        note.SetError(errorPrefix);
        return;
    }

    if (note.guid() != Guid(edamNote.guid)) {
        errorPrefix.append("note guids mismatch");
        note.SetError(errorPrefix);
        return;
    }

    if (!edamNote.__isset.updateSequenceNum) {
        errorPrefix.append("EDAM note's update sequence number is not set");
        note.SetError(errorPrefix);
        return;
    }

    note.setUpdateSequenceNumber(edamNote.updateSequenceNum);
    note.setDirty(true);

    if (edamNote.__isset.deleted) {
        note.setDeleted(edamNote.deleted);
    }

    if (edamNote.__isset.title) {
        note.setTitle(QString::fromStdString(edamNote.title));
    }
    else {
        note.setTitle("");
    }

    if (edamNote.__isset.content) {
        note.setContent(QString::fromStdString(edamNote.content));
    }
    else {
        note.setContent("");
    }

    if (!edamNote.__isset.created) {
        errorPrefix.append("EDAM note's created timestamp is not set");
        note.SetError(errorPrefix);
        return;
    }

    note.setCreationTimestamp(static_cast<time_t>(edamNote.created));

    if (edamNote.__isset.updated) {
        note.setModificationTimestamp(static_cast<time_t>(edamNote.updated));
    }
    else {
        note.setModificationTimestamp(static_cast<time_t>(0));
    }

    // TODO: continue from here: convert note attributes, resources and tags

    /*
    if (edamNote.__isset.tagGuids) {

    }
    */
}

NoteStore::~NoteStore()
{}

#define PROCESS_EDAM_EXCEPTIONS(item) \
    catch(const evernote::edam::EDAMUserException & userException) \
    { \
        errorDescription = QObject::tr("Caught EDAM user exception: "); \
        errorDescription.append(userException.what()); \
        return std::move(item); \
    } \
    catch(const evernote::edam::EDAMNotFoundException & notFoundException) \
    { \
        errorDescription = QObject::tr("Caught EDAM not found exception: "); \
        errorDescription.append(notFoundException.what()); \
        return std::move(item); \
    } \
    catch(const evernote::edam::EDAMSystemException & systemException) \
    { \
        errorDescription = QObject::tr("Caught EDAM system exception: "); \
        errorDescription.append(systemException.what()); \
        return std::move(item); \
    }

Guid NoteStore::CreateNoteGuid(const Note & note, QString & errorDescription)
{
    Guid noteGuid;

    Guid notebookGuid = note.notebookGuid();
    if (notebookGuid.isEmpty()) {
        errorDescription = QObject::tr("Notebook guid is empty");
        return std::move(noteGuid);
    }

    evernote::edam::Note edamNote;
    edamNote.notebookGuid = notebookGuid.ToQString().toStdString();
    edamNote.__isset.notebookGuid = true;

    Q_D(NoteStore);
    try {
        d->m_pNoteStoreClient->createNote(edamNote, d->m_authToken, edamNote);
    }
    PROCESS_EDAM_EXCEPTIONS(noteGuid);

    return std::move(noteGuid);
}

Guid NoteStore::CreateNotebookGuid(const Notebook &notebook, QString & errorDescription)
{
    Guid notebookGuid;

    QString notebookName = notebook.name();
    if (notebookName.isEmpty()) {
        errorDescription = QObject::tr("Notebook name is empty");
        return std::move(notebookGuid);
    }

    evernote::edam::Notebook edamNotebook;
    edamNotebook.name = notebookName.toStdString();
    edamNotebook.__isset.name = true;

    Q_D(NoteStore);
    try {
        d->m_pNoteStoreClient->createNotebook(edamNotebook, d->m_authToken, edamNotebook);
    }
    PROCESS_EDAM_EXCEPTIONS(notebookGuid);

    return std::move(notebookGuid);
}

Guid NoteStore::CreateTagGuid(const Tag & tag, QString & errorDescription)
{
    Guid tagGuid;

    QString tagName = tag.name();
    if (tagName.isEmpty()) {
        errorDescription = QObject::tr("Name is empty");
        return std::move(tagGuid);
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
    PROCESS_EDAM_EXCEPTIONS(tagGuid);

    return std::move(tagGuid);
}

#undef PROCESS_EDAM_EXCEPTIONS

}
