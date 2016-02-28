#ifndef __LIB_QUTE_NOTE__NOTE_EDITOR__UNDO_STACK__SPELL_CORRECTION_UNDO_COMMAND_H
#define __LIB_QUTE_NOTE__NOTE_EDITOR__UNDO_STACK__SPELL_CORRECTION_UNDO_COMMAND_H

#include "INoteEditorUndoCommand.h"
#include "../NoteEditorPage.h"

namespace qute_note {

class SpellCorrectionUndoCommand: public INoteEditorUndoCommand
{
    typedef NoteEditorPage::Callback Callback;
public:
    SpellCorrectionUndoCommand(NoteEditorPrivate & noteEditor, const Callback & callback,
                               QUndoCommand * parent = Q_NULLPTR);
    SpellCorrectionUndoCommand(NoteEditorPrivate & noteEditor, const Callback & callback,
                               const QString & text, QUndoCommand * parent = Q_NULLPTR);
    virtual ~SpellCorrectionUndoCommand();

    virtual void redoImpl() Q_DECL_OVERRIDE;
    virtual void undoImpl() Q_DECL_OVERRIDE;

private:
    Callback    m_callback;
};

} // namespace qute_note

#endif // __LIB_QUTE_NOTE__NOTE_EDITOR__UNDO_STACK__SPELL_CORRECTION_UNDO_COMMAND_H
