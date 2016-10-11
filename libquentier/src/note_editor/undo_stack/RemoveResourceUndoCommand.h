#ifndef LIB_QUENTIER_NOTE_EDITOR_UNDO_STACK_REMOVE_RESOURCE_UNDO_COMMAND_H
#define LIB_QUENTIER_NOTE_EDITOR_UNDO_STACK_REMOVE_RESOURCE_UNDO_COMMAND_H

#include "INoteEditorUndoCommand.h"
#include "../NoteEditorPage.h"
#include <quentier/utility/Qt4Helper.h>
#include <quentier/types/Resource.h>

namespace quentier {

class RemoveResourceUndoCommand: public INoteEditorUndoCommand
{
    Q_OBJECT
    typedef NoteEditorPage::Callback Callback;
public:
    RemoveResourceUndoCommand(const Resource & resource, const Callback & callback, NoteEditorPrivate & noteEditorPrivate,
                              QUndoCommand * parent = Q_NULLPTR);
    RemoveResourceUndoCommand(const Resource & resource, const Callback & callback, NoteEditorPrivate & noteEditorPrivate,
                              const QString & text, QUndoCommand * parent = Q_NULLPTR);
    virtual ~RemoveResourceUndoCommand();

    virtual void undoImpl() Q_DECL_OVERRIDE;
    virtual void redoImpl() Q_DECL_OVERRIDE;

private:
    Resource     m_resource;
    Callback     m_callback;
};

} // namespace quentier

#endif // LIB_QUENTIER_NOTE_EDITOR_UNDO_STACK_REMOVE_RESOURCE_UNDO_COMMAND_H
