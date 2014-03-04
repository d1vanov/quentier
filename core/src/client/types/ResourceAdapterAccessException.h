#ifndef __QUTE_NOTE__CLIENT__TYPES__RESOURCE_ADAPTER_ACCESS_EXCEPTION_H
#define __QUTE_NOTE__CLIENT__TYPES__RESOURCE_ADAPTER_ACCESS_EXCEPTION_H

#include <tools/IQuteNoteException.h>

namespace qute_note {

class ResourceAdapterAccessException : public IQuteNoteException
{
public:
    explicit ResourceAdapterAccessException(const QString & message);

private:
    virtual const QString exceptionDisplayName() const;
};

} // namespace qute_note

#endif // __QUTE_NOTE__CLIENT__TYPES__RESOURCE_ADAPTER_ACCESS_EXCEPTION_H
