#ifndef __LIB_QUTE_NOTE__EXCEPTION__NOTE_EDITOR_PLUGIN_INITIALIZATION_EXCEPTION_H
#define __LIB_QUTE_NOTE__EXCEPTION__NOTE_EDITOR_PLUGIN_INITIALIZATION_EXCEPTION_H

#include <qute_note/exception/IQuteNoteException.h>

namespace qute_note {

class QUTE_NOTE_EXPORT NoteEditorPluginInitializationException: public IQuteNoteException
{
public:
    explicit NoteEditorPluginInitializationException(const QString & message);

protected:
    virtual const QString exceptionDisplayName() const Q_DECL_OVERRIDE;
};

} // namespace qute_note

#endif // __LIB_QUTE_NOTE__EXCEPTION__NOTE_EDITOR_PLUGIN_INITIALIZATION_EXCEPTION_H
