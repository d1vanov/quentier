#ifndef __QUTE_NOTE__CORE__LOGGING__LOGGER_INITIALIZATION_EXCEPTION_H
#define __QUTE_NOTE__CORE__LOGGING__LOGGER_INITIALIZATION_EXCEPTION_H

#include <tools/IQuteNoteException.h>

namespace qute_note {

class QUTE_NOTE_EXPORT LoggerInitializationException : public IQuteNoteException
{
public:
    explicit LoggerInitializationException(const QString & message);

private:
    const QString exceptionDisplayName() const;
};

} // namespace qute_note

#endif // __QUTE_NOTE__CORE__LOGGING__LOGGER_INITIALIZATION_EXCEPTION_H
