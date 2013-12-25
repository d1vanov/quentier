#ifndef __QUTE_NOTE__EVERNOTE_CLIENT_PRIVATE__NOTE_P_H
#define __QUTE_NOTE__EVERNOTE_CLIENT_PRIVATE__NOTE_P_H

#include "Location.h"
#include "../evernote_client/Guid.h"
#include <vector>

namespace qute_note {

class NotePrivate
{
public:
    NotePrivate(const Guid & notebookGuid);
    NotePrivate(const NotePrivate & other);
    NotePrivate & operator=(const NotePrivate & other);

    Guid     m_notebookGuid;
    QString  m_title;
    QString  m_content;
    QString  m_author;
    std::vector<Guid>  m_resourceGuids;
    std::vector<Guid>  m_tagGuids;
    time_t   m_createdTimestamp;
    time_t   m_updatedTimestamp;
    time_t   m_subjectDateTimestamp;
    Location m_location;
    QString  m_source;
    QString  m_sourceUrl;
    QString  m_sourceApplication;

private:
    NotePrivate() = delete;
};

}

#endif // __QUTE_NOTE__EVERNOTE_CLIENT_PRIVATE__NOTE_P_H
