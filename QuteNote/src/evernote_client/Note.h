#ifndef __QUTE_NOTE__EVERNOTE_CLIENT__NOTE_H
#define __QUTE_NOTE__EVERNOTE_CLIENT__NOTE_H

#include <memory>

namespace qute_note {

class Note
{
public:
    Note();

private:
    class NoteImpl;
    std::unique_ptr<NoteImpl> m_pImpl;
};

}

#endif // __QUTE_NOTE__EVERNOTE_CLIENT__NOTE_H
