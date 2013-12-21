#ifndef __QUTE_NOTE__EVERNOTE_CLIENT_PRIVATE__THRIFT_EXCEPTION_H
#define __QUTE_NOTE__EVERNOTE_CLIENT_PRIVATE__THRIFT_EXCEPTION_H

#include "../tools/IQuteNoteException.h"

namespace qute_note {

class ThriftException : public IQuteNoteException
{
public:
    explicit ThriftException(const QString & message);

protected:
    virtual const QString exceptionDisplayName() const;
};

}

#endif // __QUTE_NOTE__EVERNOTE_CLIENT_PRIVATE__THRIFT_EXCEPTION_H
