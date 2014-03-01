#ifndef __QUTE_NOTE__CLIENT__TYPES__USER_ADAPTER_ACCESS_EXCEPTION_H
#define __QUTE_NOTE__CLIENT__TYPES__USER_ADAPTER_ACCESS_EXCEPTION_H

#include "../../tools/IQuteNoteException.h"

namespace qute_note {

class UserAdapterAccessException final: public IQuteNoteException
{
public:
    explicit UserAdapterAccessException(const QString & message);

private:
    const QString exceptionDisplayName() const;
};

} // namespace qute_note

#endif // __QUTE_NOTE__CLIENT__TYPES__USER_ADAPTER_ACCESS_EXCEPTION_H
