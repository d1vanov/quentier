#ifndef __QUTE_NOTE__EVERNOTE_CLIENT__NOTE_IMPL_H
#define __QUTE_NOTE__EVERNOTE_CLIENT__NOTE_IMPL_H

#include "../evernote_client/Note.h"
#include "../evernote_client/Resource.h"
#include "../evernote_client/ResourceMetadata.h"
#include "../evernote_client/Guid.h"

namespace qute_note {

class Notebook;

// TODO: implement all required functionality
class Note::NoteImpl
{
public:
    NoteImpl(const Notebook & notebook);
    NoteImpl(const NoteImpl & other);
    NoteImpl & operator=(const NoteImpl & other);

    const QString & title() const;
    void setTitle(const QString & title);

    const QString & content() const;
    void setContent(const QString & content);

    time_t createdTimestamp() const;
    time_t updatedTimestamp() const;

    const std::vector<Resource> & resources() const;

    void getResourcesMetadata(std::vector<ResourceMetadata> & resourcesMetadata) const;

    const Guid & notebookGuid() const;

private:
    NoteImpl() = delete;

    void initializeTimestamps();
    void updateTimestamp();

    Guid    m_notebookGuid;
    QString m_title;
    QString m_content;
    std::vector<Resource> m_resources;
    time_t m_createdTimestamp;
    time_t m_updatedTimestamp;
};

}

#endif // __QUTE_NOTE__EVERNOTE_CLIENT__NOTE_IMPL_H
