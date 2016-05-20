#ifndef LIB_QUTE_NOTE_NOTE_EDITOR_UNDO_STACK_I_NOTE_EDITOR_UNDO_COMMAND_H
#define LIB_QUTE_NOTE_NOTE_EDITOR_UNDO_STACK_I_NOTE_EDITOR_UNDO_COMMAND_H

#include <qute_note/utility/QuteNoteUndoCommand.h>

namespace qute_note {

QT_FORWARD_DECLARE_CLASS(NoteEditorPrivate)

class INoteEditorUndoCommand: public QuteNoteUndoCommand
{
public:
    virtual ~INoteEditorUndoCommand();

protected:
    INoteEditorUndoCommand(NoteEditorPrivate & noteEditorPrivate, QUndoCommand * parent = Q_NULLPTR);
    INoteEditorUndoCommand(NoteEditorPrivate & noteEditorPrivate, const QString & text, QUndoCommand * parent = Q_NULLPTR);

    NoteEditorPrivate &    m_noteEditorPrivate;
};

} // namespace qute_note

#endif // LIB_QUTE_NOTE_NOTE_EDITOR_UNDO_STACK_I_NOTE_EDITOR_UNDO_COMMAND_H
