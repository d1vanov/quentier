#ifndef LIB_QUTE_NOTE_EXCEPTION_SHARED_NOTEBOOK_ADAPTER_ACCESS_EXCEPTION_H
#define LIB_QUTE_NOTE_EXCEPTION_SHARED_NOTEBOOK_ADAPTER_ACCESS_EXCEPTION_H

#include <qute_note/exception/IQuteNoteException.h>

namespace qute_note {

class QUTE_NOTE_EXPORT SharedNotebookAdapterAccessException: public IQuteNoteException
{
public:
    explicit SharedNotebookAdapterAccessException(const QString & message);

protected:
    virtual const QString exceptionDisplayName() const Q_DECL_OVERRIDE;
};

} // namespace qute_note

#endif // LIB_QUTE_NOTE_EXCEPTION_SHARED_NOTEBOOK_ADAPTER_ACCESS_EXCEPTION_H
