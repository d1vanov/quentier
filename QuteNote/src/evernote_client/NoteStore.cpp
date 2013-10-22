#include "../evernote_client_private/NoteStoreImpl.h"

namespace qute_note {

NoteStore::NoteStore() :
    m_pImpl(new NoteStoreImpl)
{}

}
