#include "../evernote_client_private/UserImpl.h"

namespace qute_note {

User::User() :
    m_pImpl(new UserImpl)
{}

}
