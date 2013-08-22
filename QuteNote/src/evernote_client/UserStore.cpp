#include "UserStoreImpl.h"

namespace qute_note {

UserStore::UserStore() :
    m_pImpl(new UserStoreImpl)
{}

}
