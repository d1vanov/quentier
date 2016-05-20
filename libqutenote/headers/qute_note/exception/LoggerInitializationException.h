#ifndef LIB_QUTE_NOTE_EXCEPTION_LOGGER_INITIALIZATION_EXCEPTION_H
#define LIB_QUTE_NOTE_EXCEPTION_LOGGER_INITIALIZATION_EXCEPTION_H

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

#endif // LIB_QUTE_NOTE_EXCEPTION_LOGGER_INITIALIZATION_EXCEPTION_H
