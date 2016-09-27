#ifndef LIB_QUENTIER_NOTE_EDITOR_UNDO_STACK_IMAGE_RESIZE_UNDO_COMMAND_H
#define LIB_QUENTIER_NOTE_EDITOR_UNDO_STACK_IMAGE_RESIZE_UNDO_COMMAND_H

#include "INoteEditorUndoCommand.h"

namespace quentier {

class ImageResizeUndoCommand: public INoteEditorUndoCommand
{
    Q_OBJECT
public:
    ImageResizeUndoCommand(NoteEditorPrivate & noteEditor, QUndoCommand * parent = Q_NULLPTR);
    ImageResizeUndoCommand(NoteEditorPrivate & noteEditor, const QString & text, QUndoCommand * parent = Q_NULLPTR);
    virtual ~ImageResizeUndoCommand();

    virtual void redoImpl() Q_DECL_OVERRIDE;
    virtual void undoImpl() Q_DECL_OVERRIDE;
};

} // namespace quentier

#endif // LIB_QUENTIER_NOTE_EDITOR_UNDO_STACK_IMAGE_RESIZE_UNDO_COMMAND_H
