#ifndef __QUTE_NOTE__EVERNOTE_CLIENT__EVERNOTE_SERVICE_MANAGER_IMPL_H
#define __QUTE_NOTE__EVERNOTE_CLIENT__EVERNOTE_SERVICE_MANAGER_IMPL_H

#include "EvernoteServiceManager.h"

namespace qute_note {

class UserStore;
class NoteStore;

// TODO: implement all required funtionality
class EvernoteServiceManager::EvernoteServiceManagerImpl final
{
public:
    EvernoteServiceManagerImpl();

private:
    std::unique_ptr<UserStore> m_pUserStore;
    std::unique_ptr<NoteStore> m_pNoteStore;
};

}

#endif // __QUTE_NOTE__EVERNOTE_CLIENT__EVERNOTE_SERVICE_MANAGER_IMPL_H
