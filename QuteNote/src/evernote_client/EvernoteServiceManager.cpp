#include "EvernoteServiceManager.h"
#include "../evernote_client_private/EvernoteServiceManagerImpl.h"
#include "../evernote_client_private/NoteStore_p.h"
#include "../evernote_client_private/UserStoreImpl.h"

namespace qute_note {

EvernoteServiceManager::EvernoteServiceManager() :
    m_pImpl(new EvernoteServiceManagerImpl)
{}

}
