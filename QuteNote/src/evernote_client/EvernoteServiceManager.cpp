#include "EvernoteServiceManager.h"
#include "EvernoteServiceManagerImpl.h"
#include "NoteStoreImpl.h"
#include "UserStoreImpl.h"

namespace qute_note {

EvernoteServiceManager::EvernoteServiceManager() :
    m_pImpl(new EvernoteServiceManagerImpl)
{}

}
