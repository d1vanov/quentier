#include "../evernote_client_private/NoteStore_p.h"
#include "../evernote_sdk/src/NoteStore.h"
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

Note NoteStore::CreateNote(const QString & title, const QString & content,
                           const Notebook & notebook, const std::vector<Tag> & tags,
                           const std::vector<ResourceMetadata> & resourcesMetadata)
{
    Note note(notebook);

    if (notebook.isEmpty()) {
        note.SetError("Notebook is empty");
        return std::move(note);
    }

    if (title.isEmpty()) {
        note.setTitle(QObject::tr("Unnamed note"));
    }
    else {
        note.setTitle(title);
    }

    note.setContent(content);

    if (!tags.empty())
    {
        QString errorMessage;
        bool res = false;
        for(const auto & tag: tags)
        {
            res = note.addTag(tag, errorMessage);
            if (!res) {
                note.SetError(errorMessage);
                return std::move(note);
            }
        }
    }

    if (!resourcesMetadata.empty())
    {
        QString errorMessage;
        bool res = false;
        for(const auto & resourceMetadata: resourcesMetadata)
        {
            res = note.addResourceMetadata(resourceMetadata, errorMessage);
            if (!res) {
                note.SetError(errorMessage);
                return std::move(note);
            }
        }
    }

    // TODO: fill note with author name, source, etc

    evernote::edam::Note edamNote;
    Q_D(NoteStore);
    d->m_pNoteStoreClient->createNote(edamNote, d->m_authToken, edamNote);

    // TODO: convert from edamNote to Note

    return std::move(note);
}

}
