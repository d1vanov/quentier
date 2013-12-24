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
     * @param note - after the call would contain either valid note in case of
     * successful creation or invalid note otherwise
     */
    virtual void CreateNote(Note & note) = 0;

    /**
     * @brief CreateNotebook - attempts to "register" partially filled Notebook object
     * in NoteStore. This method would assign Guid to Notebook.
     * @param notebook - after the call would contain either valid notebook in case of
     * successful creation or invalid notebook otherwise
     */
    virtual void CreateNotebook(Notebook & notebook) = 0;

    /**
     * @brief CreateTag - attempts to "register" partially filled Tag object
     * in NoteStore. This method would assign Guid to Tag.
     * @param tag - after the call would contain either valid tag in case of successful
     * creation or invalid tag otherwise
     */
    virtual void CreateTag(Tag & tag) = 0;
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
