#ifndef __LIB_QUTE_NOTE__LOGGING__LOGGER_INITIALIZATION_EXCEPTION_H
#define __LIB_QUTE_NOTE__LOGGING__LOGGER_INITIALIZATION_EXCEPTION_H

#include <qute_note/utility/IQuteNoteException.h>

namespace qute_note {

class QUTE_NOTE_EXPORT LoggerInitializationException: public IQuteNoteException
{
public:
    explicit LoggerInitializationException(const QString & message);

private:
    const QString exceptionDisplayName() const;
};

} // namespace qute_note

#endif // __LIB_QUTE_NOTE__LOGGING__LOGGER_INITIALIZATION_EXCEPTION_H
