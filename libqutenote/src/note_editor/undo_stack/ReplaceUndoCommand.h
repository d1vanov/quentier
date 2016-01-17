#ifndef __LIB_QUTE_NOTE__NOTE_EDITOR__UNDO_STACK__REPLACE_UNDO_COMMAND_H
#define __LIB_QUTE_NOTE__NOTE_EDITOR__UNDO_STACK__REPLACE_UNDO_COMMAND_H

#include "INoteEditorUndoCommand.h"

namespace qute_note {

class ReplaceUndoCommand: public INoteEditorUndoCommand
{
public:
    ReplaceUndoCommand(const QString & textToReplace, const bool matchCase,
                       NoteEditorPrivate & noteEditorPrivate, QUndoCommand * parent = Q_NULLPTR);
    ReplaceUndoCommand(const QString & textToReplace, const bool matchCase,
                       NoteEditorPrivate & noteEditorPrivate, const QString & text, QUndoCommand * parent = Q_NULLPTR);
    virtual ~ReplaceUndoCommand();

    virtual void redoImpl() Q_DECL_OVERRIDE;
    virtual void undoImpl() Q_DECL_OVERRIDE;

private:
    QString     m_textToReplace;
    bool        m_matchCase;
};

} // namespace qute_note

#endif // __LIB_QUTE_NOTE__NOTE_EDITOR__UNDO_STACK__REPLACE_UNDO_COMMAND_H
