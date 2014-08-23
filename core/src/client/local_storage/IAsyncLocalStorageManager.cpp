#include "IAsyncLocalStorageManager.h"

namespace qute_note {

IAsyncLocalStorageManager::~IAsyncLocalStorageManager()
{}

IAsyncLocalStorageManager::IAsyncLocalStorageManager(IAsyncLocalStorageManager &&)
{}

IAsyncLocalStorageManager & IAsyncLocalStorageManager::operator=(IAsyncLocalStorageManager && )
{
    return *this;
}

}
