#ifndef __LIB_QUTE_NOTE__NOTE_EDITOR__UNDO_STACK__I_NOTE_EDITOR_UNDO_COMMAND_H
#define __LIB_QUTE_NOTE__NOTE_EDITOR__UNDO_STACK__I_NOTE_EDITOR_UNDO_COMMAND_H

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

#endif // __LIB_QUTE_NOTE__NOTE_EDITOR__UNDO_STACK__I_NOTE_EDITOR_UNDO_COMMAND_H
