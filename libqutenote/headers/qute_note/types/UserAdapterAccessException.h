#ifndef __LIB_QUTE_NOTE__TYPES__USER_ADAPTER_ACCESS_EXCEPTION_H
#define __LIB_QUTE_NOTE__TYPES__USER_ADAPTER_ACCESS_EXCEPTION_H

#include <qute_note/exception/IQuteNoteException.h>

namespace qute_note {

class QUTE_NOTE_EXPORT UserAdapterAccessException: public IQuteNoteException
{
public:
    explicit UserAdapterAccessException(const QString & message);

private:
    const QString exceptionDisplayName() const;
};

} // namespace qute_note

#endif // __LIB_QUTE_NOTE__TYPES__USER_ADAPTER_ACCESS_EXCEPTION_H
