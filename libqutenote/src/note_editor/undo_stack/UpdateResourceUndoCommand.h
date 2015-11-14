#ifndef __LIB_QUTE_NOTE__NOTE_EDITOR__UNDO_STACK__UPDATE_RESOURCE_UNDO_COMMAND_H
#define __LIB_QUTE_NOTE__NOTE_EDITOR__UNDO_STACK__UPDATE_RESOURCE_UNDO_COMMAND_H

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

    virtual void undo() Q_DECL_OVERRIDE;
    virtual void redo() Q_DECL_OVERRIDE;

private:
    void init();

private:
    ResourceWrapper     m_resourceBefore;
    ResourceWrapper     m_resourceAfter;
};

} // namespace qute_note

#endif // __LIB_QUTE_NOTE__NOTE_EDITOR__UNDO_STACK__UPDATE_RESOURCE_UNDO_COMMAND_H
