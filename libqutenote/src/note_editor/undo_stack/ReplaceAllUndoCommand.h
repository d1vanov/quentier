#ifndef __LIB_QUTE_NOTE__NOTE_EDITOR__UNDO_STACK__REPLACE_ALL_UNDO_COMMAND_H
#define __LIB_QUTE_NOTE__NOTE_EDITOR__UNDO_STACK__REPLACE_ALL_UNDO_COMMAND_H

#include "INoteEditorUndoCommand.h"
#include "../NoteEditorPage.h"

namespace qute_note {

class ReplaceAllUndoCommand: public INoteEditorUndoCommand
{
    typedef NoteEditorPage::Callback Callback;
public:
    ReplaceAllUndoCommand(const QString & textToReplace, const bool matchCase, NoteEditorPrivate & noteEditorPrivate,
                          Callback callback, QUndoCommand * parent = Q_NULLPTR);
    ReplaceAllUndoCommand(const QString & textToReplace, const bool matchCase, NoteEditorPrivate & noteEditorPrivate,
                          const QString & text, Callback callback, QUndoCommand * parent = Q_NULLPTR);
    virtual ~ReplaceAllUndoCommand();

    virtual void redoImpl() Q_DECL_OVERRIDE;
    virtual void undoImpl() Q_DECL_OVERRIDE;

private:
    QString     m_textToReplace;
    bool        m_matchCase;
    Callback    m_callback;
};

} // namespace qute_note

#endif // __LIB_QUTE_NOTE__NOTE_EDITOR__UNDO_STACK__REPLACE_ALL_UNDO_COMMAND_H
