#ifndef LIB_QUENTIER_NOTE_EDITOR_UNDO_STACK_I_NOTE_EDITOR_UNDO_COMMAND_H
#define LIB_QUENTIER_NOTE_EDITOR_UNDO_STACK_I_NOTE_EDITOR_UNDO_COMMAND_H

#include <quentier/utility/QuentierUndoCommand.h>

namespace quentier {

QT_FORWARD_DECLARE_CLASS(NoteEditorPrivate)

class INoteEditorUndoCommand: public QuentierUndoCommand
{
    Q_OBJECT
public:
    virtual ~INoteEditorUndoCommand();

protected:
    INoteEditorUndoCommand(NoteEditorPrivate & noteEditorPrivate, QUndoCommand * parent = Q_NULLPTR);
    INoteEditorUndoCommand(NoteEditorPrivate & noteEditorPrivate, const QString & text, QUndoCommand * parent = Q_NULLPTR);

    NoteEditorPrivate &    m_noteEditorPrivate;
};

} // namespace quentier

#endif // LIB_QUENTIER_NOTE_EDITOR_UNDO_STACK_I_NOTE_EDITOR_UNDO_COMMAND_H
