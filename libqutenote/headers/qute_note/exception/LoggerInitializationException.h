#ifndef __LIB_QUTE_NOTE__EXCEPTION__LOGGER_INITIALIZATION_EXCEPTION_H
#define __LIB_QUTE_NOTE__EXCEPTION__LOGGER_INITIALIZATION_EXCEPTION_H

#include <qute_note/exception/IQuteNoteException.h>

namespace qute_note {

class QUTE_NOTE_EXPORT LoggerInitializationException: public IQuteNoteException
{
public:
    explicit LoggerInitializationException(const QString & message);

protected:
    const QString exceptionDisplayName() const Q_DECL_OVERRIDE;
};

} // namespace qute_note

#endif // __LIB_QUTE_NOTE__EXCEPTION__LOGGER_INITIALIZATION_EXCEPTION_H
