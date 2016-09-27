#ifndef LIB_QUENTIER_NOTE_EDITOR_UNDO_STACK_REPLACE_UNDO_COMMAND_H
#define LIB_QUENTIER_NOTE_EDITOR_UNDO_STACK_REPLACE_UNDO_COMMAND_H

#include "INoteEditorUndoCommand.h"
#include "../NoteEditorPage.h"

namespace quentier {

class ReplaceUndoCommand: public INoteEditorUndoCommand
{
    Q_OBJECT
    typedef NoteEditorPage::Callback Callback;
public:
    ReplaceUndoCommand(const QString & textToReplace, const bool matchCase, NoteEditorPrivate & noteEditorPrivate,
                       Callback callback, QUndoCommand * parent = Q_NULLPTR);
    ReplaceUndoCommand(const QString & textToReplace, const bool matchCase, NoteEditorPrivate & noteEditorPrivate,
                       const QString & text, Callback callback, QUndoCommand * parent = Q_NULLPTR);
    virtual ~ReplaceUndoCommand();

    virtual void redoImpl() Q_DECL_OVERRIDE;
    virtual void undoImpl() Q_DECL_OVERRIDE;

private:
    QString     m_textToReplace;
    bool        m_matchCase;
    Callback    m_callback;
};

} // namespace quentier

#endif // LIB_QUENTIER_NOTE_EDITOR_UNDO_STACK_REPLACE_UNDO_COMMAND_H
