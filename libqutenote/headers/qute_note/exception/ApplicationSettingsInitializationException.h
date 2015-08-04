#ifndef __LIB_QUTE_NOTE__EXCEPTION__APPLICATION_SETTINGS_INITIALIZATION_EXCEPTION_H
#define __LIB_QUTE_NOTE__EXCEPTION__APPLICATION_SETTINGS_INITIALIZATION_EXCEPTION_H

#include <qute_note/exception/IQuteNoteException.h>

namespace qute_note {

class QUTE_NOTE_EXPORT ApplicationSettingsInitializationException: public IQuteNoteException
{
public:
    explicit ApplicationSettingsInitializationException(const QString & message);

protected:
    virtual const QString exceptionDisplayName() const;
};

} // namespace qute_note

#endif // __LIB_QUTE_NOTE__EXCEPTION__APPLICATION_SETTINGS_INITIALIZATION_EXCEPTION_H
