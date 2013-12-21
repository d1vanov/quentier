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
    virtual Note CreateNote(const QString & title, const QString & content, const Notebook & notebook,
                            const std::vector<Tag> & tags = std::vector<Tag>(),
                            const std::vector<ResourceMetadata> & resourcesMetadata = std::vector<ResourceMetadata>()) = 0;

    // Will be uncommented when implementation is done, to not break the compilability
    /*
    virtual Notebook CreateNotebook(const QString & name) = 0;

    virtual Tag CreateTag(const String & name, const Guid & parentGuid = Guid()) = 0;

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
