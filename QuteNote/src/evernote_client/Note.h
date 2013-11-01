#ifndef __QUTE_NOTE__EVERNOTE_CLIENT__NOTE_H
#define __QUTE_NOTE__EVERNOTE_CLIENT__NOTE_H

#include <memory>
#include <cstdint>
#include <cstddef>
#include <vector>
#include <ctime>
#include "../tools/TypeWithError.h"
#include "SynchronizedDataElement.h"

namespace qute_note {

class Guid;
class Resource;
class ResourceMetadata;
class Notebook;
class Tag;

class Note: public TypeWithError,
            public SynchronizedDataElement
{
public:
    Note(const Notebook & notebook);
    Note(const Note & other);
    Note & operator =(const Note & other);

    virtual bool isEmpty() const;

    const QString title() const;
    void setTitle(const QString & title);

    /**
     * @brief content - returns content of the note in ENML format
     * @return
     */
    const QString content() const;
    void setContent(const QString & content);

    time_t createdTimestamp() const;
    time_t updatedTimestamp() const;

    const Guid notebookGuid() const;

    bool hasAttachedResources() const;
    size_t numAttachedResources() const;
    const Resource * getResourceByIndex(const size_t index) const;
    bool addResource(const Resource & resource, QString & errorMessage);
    void getResourcesMetadata(std::vector<ResourceMetadata> & resourcesMetadata) const;

    bool labeledByAnyTag() const;
    size_t numTags() const;
    const Tag * getTagByIndex(const size_t index);
    bool addTag(const Tag & tag, QString & errorMessage);

private:
    Note() = delete;

    class NoteImpl;
    std::unique_ptr<NoteImpl> m_pImpl;
};

}

#endif // __QUTE_NOTE__EVERNOTE_CLIENT__NOTE_H
