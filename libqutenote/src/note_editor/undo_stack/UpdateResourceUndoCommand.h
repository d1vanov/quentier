#ifndef LIB_QUTE_NOTE_NOTE_EDITOR_UNDO_STACK_UPDATE_RESOURCE_UNDO_COMMAND_H
#define LIB_QUTE_NOTE_NOTE_EDITOR_UNDO_STACK_UPDATE_RESOURCE_UNDO_COMMAND_H

#include "INoteEditorUndoCommand.h"
#include <qute_note/types/ResourceWrapper.h>

namespace qute_note {

class UpdateResourceUndoCommand: public INoteEditorUndoCommand
{
public:
    UpdateResourceUndoCommand(const ResourceWrapper & resourceBefore, const ResourceWrapper & resourceAfter,
                             NoteEditorPrivate & noteEditorPrivate, QUndoCommand * parent = Q_NULLPTR);
    UpdateResourceUndoCommand(const ResourceWrapper & resourceBefore, const ResourceWrapper & resourceAfter,
                              NoteEditorPrivate & noteEditorPrivate, const QString & text, QUndoCommand * parent = Q_NULLPTR);
    virtual ~UpdateResourceUndoCommand();

    virtual void undoImpl() Q_DECL_OVERRIDE;
    virtual void redoImpl() Q_DECL_OVERRIDE;

private:
    void init();

private:
    ResourceWrapper     m_resourceBefore;
    ResourceWrapper     m_resourceAfter;
};

} // namespace qute_note

#endif // LIB_QUTE_NOTE_NOTE_EDITOR_UNDO_STACK_UPDATE_RESOURCE_UNDO_COMMAND_H
