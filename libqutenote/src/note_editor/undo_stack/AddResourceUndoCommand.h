#ifndef LIB_QUTE_NOTE_NOTE_EDITOR_UNDO_STACK_ADD_RESOURCE_UNDO_COMMAND_H
#define LIB_QUTE_NOTE_NOTE_EDITOR_UNDO_STACK_ADD_RESOURCE_UNDO_COMMAND_H

#include "INoteEditorUndoCommand.h"
#include "../NoteEditorPage.h"
#include <qute_note/utility/Qt4Helper.h>
#include <qute_note/types/ResourceWrapper.h>

namespace qute_note {

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

} // namespace qute_note

#endif // LIB_QUTE_NOTE_NOTE_EDITOR_UNDO_STACK_ADD_RESOURCE_UNDO_COMMAND_H
