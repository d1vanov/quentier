#include "EvernoteServiceManager.h"
#include "EvernoteServiceManagerImpl.h"

namespace qute_note {

EvernoteServiceManager::EvernoteServiceManager() :
    m_pImpl(new EvernoteServiceManagerImpl)
{}

}
