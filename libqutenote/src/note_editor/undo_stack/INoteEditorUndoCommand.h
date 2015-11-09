#ifndef __LIB_QUTE_NOTE__NOTE_EDITOR__UNDO_STACK__I_NOTE_EDITOR_UNDO_COMMAND_H
#define __LIB_QUTE_NOTE__NOTE_EDITOR__UNDO_STACK__I_NOTE_EDITOR_UNDO_COMMAND_H

#include <qute_note/utility/Qt4Helper.h>
#include <QUndoCommand>

namespace qute_note {

QT_FORWARD_DECLARE_CLASS(NoteEditor)

class INoteEditorUndoCommand: public QUndoCommand
{
protected:
    INoteEditorUndoCommand(NoteEditor & noteEditor, QUndoCommand * parent = Q_NULLPTR);
    INoteEditorUndoCommand(NoteEditor & noteEditor, const QString & text, QUndoCommand * parent = Q_NULLPTR);
    virtual ~INoteEditorUndoCommand();

    NoteEditor &    m_noteEditor;
};

} // namespace qute_note

#endif // __LIB_QUTE_NOTE__NOTE_EDITOR__UNDO_STACK__I_NOTE_EDITOR_UNDO_COMMAND_H
