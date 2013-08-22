#include "NoteStoreImpl.h"

namespace qute_note {

NoteStore::NoteStore() :
    m_pImpl(new NoteStoreImpl)
{}

}
