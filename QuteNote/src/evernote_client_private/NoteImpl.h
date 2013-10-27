#ifndef __QUTE_NOTE__EVERNOTE_CLIENT__NOTE_IMPL_H
#define __QUTE_NOTE__EVERNOTE_CLIENT__NOTE_IMPL_H

#include "../evernote_client/Note.h"
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

    const Guid & notebookGuid() const;

private:
    NoteImpl() = delete;

    Guid   m_notebookGuid;
};

}

#endif // __QUTE_NOTE__EVERNOTE_CLIENT__NOTE_IMPL_H
