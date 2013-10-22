#include "EvernoteServiceManager.h"
#include "../evernote_client_private/EvernoteServiceManagerImpl.h"
#include "../evernote_client_private/NoteStoreImpl.h"
#include "../evernote_client_private/UserStoreImpl.h"

namespace qute_note {

EvernoteServiceManager::EvernoteServiceManager() :
    m_pImpl(new EvernoteServiceManagerImpl)
{}

}
