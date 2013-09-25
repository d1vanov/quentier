#include "EvernoteServiceManagerImpl.h"
#include "UserStoreImpl.h"
#include "NoteStoreImpl.h"

namespace qute_note {

EvernoteServiceManager::EvernoteServiceManagerImpl::EvernoteServiceManagerImpl() :
    m_pUserStore(new UserStore),
    m_pNoteStore(new NoteStore)
{}

}
