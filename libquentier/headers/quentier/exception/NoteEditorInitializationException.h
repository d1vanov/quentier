#ifndef LIB_QUENTIER_EXCEPTION_NOTE_EDITOR_INITIALIZATION_EXCEPTION_H
#define LIB_QUENTIER_EXCEPTION_NOTE_EDITOR_INITIALIZATION_EXCEPTION_H

#include <quentier/exception/IQuentierException.h>

namespace quentier {

class QUENTIER_EXPORT NoteEditorInitializationException: public IQuentierException
{
public:
    explicit NoteEditorInitializationException(const QString & message);

protected:
    virtual const QString exceptionDisplayName() const Q_DECL_OVERRIDE;
};

} // namespace quentier

#endif // LIB_QUENTIER_EXCEPTION_NOTE_EDITOR_INITIALIZATION_EXCEPTION_H
