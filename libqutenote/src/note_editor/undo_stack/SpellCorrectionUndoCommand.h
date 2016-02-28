#ifndef __LIB_QUTE_NOTE__NOTE_EDITOR__UNDO_STACK__SPELL_CORRECTION_UNDO_COMMAND_H
#define __LIB_QUTE_NOTE__NOTE_EDITOR__UNDO_STACK__SPELL_CORRECTION_UNDO_COMMAND_H

#include "INoteEditorUndoCommand.h"

namespace qute_note {

class SpellCorrectionUndoCommand: public INoteEditorUndoCommand
{
public:
    SpellCorrectionUndoCommand(NoteEditorPrivate & noteEditor, QUndoCommand * parent = Q_NULLPTR);
    SpellCorrectionUndoCommand(NoteEditorPrivate & noteEditor, const QString & text, QUndoCommand * parent = Q_NULLPTR);
    virtual ~SpellCorrectionUndoCommand();

    virtual void redoImpl() Q_DECL_OVERRIDE;
    virtual void undoImpl() Q_DECL_OVERRIDE;
};

} // namespace qute_note

#endif // __LIB_QUTE_NOTE__NOTE_EDITOR__UNDO_STACK__SPELL_CORRECTION_UNDO_COMMAND_H
