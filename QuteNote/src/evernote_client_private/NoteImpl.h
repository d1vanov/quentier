#ifndef __QUTE_NOTE__EVERNOTE_CLIENT__NOTE_IMPL_H
#define __QUTE_NOTE__EVERNOTE_CLIENT__NOTE_IMPL_H

#include "../evernote_client/Note.h"
#include "../evernote_client/Resource.h"
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

    const std::vector<Resource> & resources() const;
    std::vector<Resource> & resources();

    const Guid & notebookGuid() const;

private:
    NoteImpl() = delete;

    Guid    m_notebookGuid;
    QString m_title;
    QString m_content;
    std::vector<Resource> m_resources;
};

}

#endif // __QUTE_NOTE__EVERNOTE_CLIENT__NOTE_IMPL_H
