#ifndef __QUTE_NOTE__EVERNOTE_CLIENT__NOTE_IMPL_H
#define __QUTE_NOTE__EVERNOTE_CLIENT__NOTE_IMPL_H

#include "../evernote_client/Note.h"
#include "../evernote_client/Resource.h"
#include "../evernote_client/ResourceMetadata.h"
#include "../evernote_client/Tag.h"
#include "../evernote_client/Guid.h"
#include "Location.h"

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

    const QString & author() const;
    void setAuthor(const QString & author);

    const QString & source() const;
    void setSource(const QString & source);

    const QString & sourceUrl() const;
    void setSourceUrl(const QString & sourceUrl);

    const QString & sourceApplication() const;
    void setSourceApplication(const QString & sourceApplication);

    time_t createdTimestamp() const;
    time_t updatedTimestamp() const;
    time_t subjectDateTimestamp() const;

    const Location & location() const;
    Location & location();

    const std::vector<Resource> & resources() const;
    void getResourcesMetadata(std::vector<ResourceMetadata> & resourcesMetadata) const;
    bool addResource(const Resource & resource, QString & errorMessage);

    const std::vector<Tag> & tags() const;
    bool addTag(const Tag & tag, QString & errorMessage);

    const Guid & notebookGuid() const;

    bool isDeleted() const;
    void setDeletedFlag(const bool isDeleted);

private:
    NoteImpl() = delete;

    void initializeTimestamps();
    void updateTimestamp();

    Guid     m_notebookGuid;
    QString  m_title;
    QString  m_content;
    QString  m_author;
    std::vector<Resource> m_resources;
    std::vector<Tag>      m_tags;
    time_t   m_createdTimestamp;
    time_t   m_updatedTimestamp;
    time_t   m_subjectDateTimestamp;
    Location m_location;
    QString  m_source;
    QString  m_sourceUrl;
    QString  m_sourceApplication;
    bool     m_isDeleted;
};

}

#endif // __QUTE_NOTE__EVERNOTE_CLIENT__NOTE_IMPL_H
