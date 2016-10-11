#ifndef LIB_QUENTIER_NOTE_EDITOR_UNDO_STACK_UPDATE_RESOURCE_UNDO_COMMAND_H
#define LIB_QUENTIER_NOTE_EDITOR_UNDO_STACK_UPDATE_RESOURCE_UNDO_COMMAND_H

#include "INoteEditorUndoCommand.h"
#include <quentier/types/Resource.h>

namespace quentier {

class UpdateResourceUndoCommand: public INoteEditorUndoCommand
{
    Q_OBJECT
public:
    UpdateResourceUndoCommand(const Resource & resourceBefore, const Resource & resourceAfter,
                             NoteEditorPrivate & noteEditorPrivate, QUndoCommand * parent = Q_NULLPTR);
    UpdateResourceUndoCommand(const Resource & resourceBefore, const Resource & resourceAfter,
                              NoteEditorPrivate & noteEditorPrivate, const QString & text, QUndoCommand * parent = Q_NULLPTR);
    virtual ~UpdateResourceUndoCommand();

    virtual void undoImpl() Q_DECL_OVERRIDE;
    virtual void redoImpl() Q_DECL_OVERRIDE;

private:
    void init();

private:
    Resource     m_resourceBefore;
    Resource     m_resourceAfter;
};

} // namespace quentier

#endif // LIB_QUENTIER_NOTE_EDITOR_UNDO_STACK_UPDATE_RESOURCE_UNDO_COMMAND_H
