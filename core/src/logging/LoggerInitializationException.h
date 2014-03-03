#ifndef LOGGERINITIALIZATIONEXCEPTION_H
#define LOGGERINITIALIZATIONEXCEPTION_H

#include "../tools/IQuteNoteException.h"

namespace qute_note {

class LoggerInitializationException : public IQuteNoteException
{
public:
    explicit LoggerInitializationException(const QString & message);

private:
    const QString exceptionDisplayName() const;
};

} // namespace qute_note

#endif // LOGGERINITIALIZATIONEXCEPTION_H
