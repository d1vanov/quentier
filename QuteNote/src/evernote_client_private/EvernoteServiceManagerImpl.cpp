#include "EvernoteServiceManagerImpl.h"
#include "UserStoreImpl.h"
#include "NoteStore_p.h"

namespace qute_note {

EvernoteServiceManager::EvernoteServiceManagerImpl::EvernoteServiceManagerImpl() :
    m_pUserStore(new UserStore),
    m_pNoteStore(nullptr)
{}

}
