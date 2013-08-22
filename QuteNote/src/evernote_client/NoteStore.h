#ifndef __QUTE_NOTE__EVERNOTE_CLIENT__NOTE_STORE_H
#define __QUTE_NOTE__EVERNOTE_CLIENT__NOTE_STORE_H

#include <memory>

namespace qute_note {

class NoteStore
{
public:
    NoteStore();

private:
    class NoteStoreImpl;
    std::unique_ptr<NoteStoreImpl> m_pImpl;
};

}

#endif // __QUTE_NOTE__EVERNOTE_CLIENT__NOTE_STORE_H
