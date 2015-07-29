#ifndef __QUTE_NOTE__CORE__NOTE_EDITOR__PLUGIN_INITIALIZATION_EXCEPTION_H
#define __QUTE_NOTE__CORE__NOTE_EDITOR__PLUGIN_INITIALIZATION_EXCEPTION_H

#include <qute_note/utility/IQuteNoteException.h>
#include <tools/Linkage.h>

namespace qute_note {

class QUTE_NOTE_EXPORT PluginInitializationException: public IQuteNoteException
{
public:
    explicit PluginInitializationException(const QString & message);

protected:
    virtual const QString exceptionDisplayName() const;
};

} // namespace qute_note

#endif // __QUTE_NOTE__CORE__NOTE_EDITOR__PLUGIN_INITIALIZATION_EXCEPTION_H
