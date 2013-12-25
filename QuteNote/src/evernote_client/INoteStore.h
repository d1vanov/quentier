#ifndef __QUTE_NOTE__EVERNOTE_CLIENT__INOTE_STORE_H
#define __QUTE_NOTE__EVERNOTE_CLIENT__INOTE_STORE_H

#include "Guid.h"
#include <ctime>
#include <QtGlobal>

namespace qute_note
{

QT_FORWARD_DECLARE_CLASS(Note)
QT_FORWARD_DECLARE_CLASS(Notebook)
QT_FORWARD_DECLARE_CLASS(Tag)
QT_FORWARD_DECLARE_CLASS(ResourceMetadata)

/**
 * @brief The INoteStore class is the interface for note store class responsible
 * for creation, lookup and deletion of notes and notebooks
 */
class INoteStore
{
public:
    /**
     * @brief CreateNote - attempts to "register" partially filled Note object
     * in NoteStore. This method would assign Guid to Note.
     * @param note - partially filled Note object used as a source of information
     * for NoteStore to create note
     * @param errorDescription - contains description of error in case of failure
     * to create note guid
     * @return created Note's guid: either empty in case of failure or valid one
     * in case of successful creation
     */
    virtual Guid CreateNoteGuid(const Note & note, QString & errorDescription) = 0;

    /**
     * @brief CreateNotebook - attempts to "register" partially filled Notebook object
     * in NoteStore. This method would assign Guid to Notebook.
     * @param notebook - partially filled Notebook object used as a source of information
     * for NoteStore to create notebook
     * @param errorDescription - contains description of error in case of failure
     * to create notebook guid
     * @return created Notebook's guid: either empty in case of failure or valid one
     * in case of successful creation
     */
    virtual Guid CreateNotebookGuid(const Notebook & notebook, QString & errorDescription) = 0;

    /**
     * @brief CreateTag - attempts to "register" partially filled Tag object
     * in NoteStore. This method would assign Guid to Tag.
     * @param tag - partially filled Tag object used as a source of information
     * for NoteStore to create tag
     * @param errorDescription - contains description of error in case of failure
     * to create tag guid
     * @return created Tag's guid: either empty in case of failure or valid one
     * in case of successful creation
     */
    virtual Guid CreateTagGuid(const Tag & tag, QString & errorDescription) = 0;

    /*
    virtual DeleteNote(const Note & note) = 0;

    virtual Note GetNote(const Guid & guid) const = 0;

    virtual Notebook GetNotebook(const Guid & guid) const = 0;

    virtual Notebook GetDefaultNotebook() const = 0;

    virtual Tag GetTag(const Guid & guid) const = 0;

    virtual void UpdateNote(const Note & update) = 0;

    virtual void UpdateNotebook(const Notebook & update) = 0;

    virtual void UpdateTag(const Tag & update) = 0;
    */
};

}

#endif // __QUTE_NOTE__EVERNOTE_CLIENT__INOTE_STORE_H
