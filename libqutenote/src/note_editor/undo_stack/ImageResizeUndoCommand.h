#ifndef __LIB_QUTE_NOTE__NOTE_EDITOR__UNDO_STACK__IMAGE_RESIZE_UNDO_COMMAND_H
#define __LIB_QUTE_NOTE__NOTE_EDITOR__UNDO_STACK__IMAGE_RESIZE_UNDO_COMMAND_H

#include "INoteEditorUndoCommand.h"

namespace qute_note {

class ImageResizeUndoCommand: public INoteEditorUndoCommand
{
public:
    ImageResizeUndoCommand(NoteEditorPrivate & noteEditor, QUndoCommand * parent = Q_NULLPTR);
    ImageResizeUndoCommand(NoteEditorPrivate & noteEditor, const QString & text, QUndoCommand * parent = Q_NULLPTR);
    virtual ~ImageResizeUndoCommand();

    virtual void redoImpl() Q_DECL_OVERRIDE;
    virtual void undoImpl() Q_DECL_OVERRIDE;
};

} // namespace qute_note

#endif // __LIB_QUTE_NOTE__NOTE_EDITOR__UNDO_STACK__IMAGE_RESIZE_UNDO_COMMAND_H
