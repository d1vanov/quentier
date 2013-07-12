#include "EvernoteServiceManager.h"
#include "../tools/Singleton.h"

EvernoteServiceManager & EvernoteServiceManager::Instance()
{
    return qutenote_tools::Singleton<EvernoteServiceManager>::Instance();
}
