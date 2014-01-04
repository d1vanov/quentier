#include "NoteStore.h"
#include <QObject>

namespace qute_note {

bool Notebook::CheckParameters(QString & errorDescription) const
{
    if (!en_notebook.__isset.guid) {
        errorDescription = QObject::tr("Notebook's guid is not set");
        return false;
    }

    if (en_notebook.guid.empty()) {
        errorDescription = QObject::tr("Notebook's guid is empty");
        return false;
    }

    if (!en_notebook.__isset.updateSequenceNum) {
        errorDescription = QObject::tr("Notebook's update sequence number is not set");
        return false;
    }

    if ( (en_notebook.updateSequenceNum < 0) ||
         (en_notebook.updateSequenceNum == std::numeric_limits<int32_t>::max()) ||
         (en_notebook.updateSequenceNum == std::numeric_limits<int32_t>::min()) )
    {
        errorDescription = QObject::tr("Notebook's update sequence number is invalid");
        return false;
    }

    if (!en_notebook.__isset.name) {
        errorDescription = QObject::tr("Notebook's name is not set");
        return false;
    }

    if (en_notebook.name.empty()) {
        errorDescription = QObject::tr("Notebook's name is empty");
        return false;
    }

    if (!en_notebook.__isset.serviceCreated) {
        errorDescription = QObject::tr("Notebook's creation timestamp is not set");
        return false;
    }

    if (!en_notebook.__isset.serviceUpdated) {
        errorDescription = QObject::tr("Notebook's modification timestamp is not set");
        return false;
    }

    return true;
}

bool Note::CheckParameters(QString & errorDescription) const
{
    if (!en_note.__isset.guid) {
        errorDescription = QObject::tr("Note's guid is not set");
        return false;
    }

    if (en_note.guid.empty()) {
        errorDescription = QObject::tr("Note's guid is empty");
        return false;
    }

    if (en_note.__isset.updateSequenceNum) {
        errorDescription = QObject::tr("Note's update sequence number is not set");
        return false;
    }

    if ( (en_note.updateSequenceNum < 0) ||
         (en_note.updateSequenceNum == std::numeric_limits<int32_t>::max()) ||
         (en_note.updateSequenceNum == std::numeric_limits<int32_t>::min()) )
    {
        errorDescription = QObject::tr("Note's update sequence number is invalid");
        return false;
    }

    if (!en_note.__isset.title) {
        errorDescription = QObject::tr("Note's title is not set");
        return false;
    }

    if (en_note.title.empty()) {
        errorDescription = QObject::tr("Note's title is empty");
        return false;
    }

    if (!en_note.__isset.content) {
        errorDescription = QObject::tr("Note's content is not set");
        return false;
    }

    if (en_note.content.empty()) {
        errorDescription = QObject::tr("Note's content is empty");
        return false;
    }

    if (!en_note.__isset.notebookGuid) {
        errorDescription = QObject::tr("Note's notebook Guid is not set");
        return false;
    }

    if (en_note.notebookGuid.empty()) {
        errorDescription = QObject::tr("Note's notebook Guid is empty");
        return false;
    }

    if (!en_note.__isset.created) {
        errorDescription = QObject::tr("Note's creation timestamp is not set");
        return false;
    }

    if (!en_note.__isset.updated) {
        errorDescription = QObject::tr("Note's modification timestamp is not set");
        return false;
    }

    return true;
}

}
