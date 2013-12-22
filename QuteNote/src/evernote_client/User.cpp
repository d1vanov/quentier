#include "User.h"
#include "../evernote_client_private/User_p.h"

namespace qute_note {

User::User() :
    d_ptr(new UserPrivate)
{}

User::~User()
{}

QTextStream & User::Print(QTextStream & strm) const
{
    // TODO: implement
    return strm;
}

}
