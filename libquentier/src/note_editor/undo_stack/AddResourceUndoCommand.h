#ifndef LIB_QUENTIER_NOTE_EDITOR_UNDO_STACK_ADD_RESOURCE_UNDO_COMMAND_H
#define LIB_QUENTIER_NOTE_EDITOR_UNDO_STACK_ADD_RESOURCE_UNDO_COMMAND_H

#include "INoteEditorUndoCommand.h"
#include "../NoteEditorPage.h"
#include <quentier/utility/Qt4Helper.h>
#include <quentier/types/ResourceWrapper.h>

namespace quentier {

class AddResourceUndoCommand: public INoteEditorUndoCommand
{
    typedef NoteEditorPage::Callback Callback;
public:
    AddResourceUndoCommand(const ResourceWrapper & resource, const Callback & callback, NoteEditorPrivate & noteEditorPrivate,
                           QUndoCommand * parent = Q_NULLPTR);
    AddResourceUndoCommand(const ResourceWrapper & resource, const Callback & callback, NoteEditorPrivate & noteEditorPrivate,
                           const QString & text, QUndoCommand * parent = Q_NULLPTR);
    virtual ~AddResourceUndoCommand();

    virtual void undoImpl() Q_DECL_OVERRIDE;
    virtual void redoImpl() Q_DECL_OVERRIDE;

private:
    void init();

private:
    ResourceWrapper     m_resource;
    Callback            m_callback;
};

} // namespace quentier

#endif // LIB_QUENTIER_NOTE_EDITOR_UNDO_STACK_ADD_RESOURCE_UNDO_COMMAND_H
