#ifndef __LIB_QUTE_NOTE__NOTE_EDITOR__UNDO_STACK__EDIT_HYPERLINK_UNDO_COMMAND_H
#define __LIB_QUTE_NOTE__NOTE_EDITOR__UNDO_STACK__EDIT_HYPERLINK_UNDO_COMMAND_H

#include "INoteEditorUndoCommand.h"

namespace qute_note {

class EditHyperlinkUndoCommand: public INoteEditorUndoCommand
{
public:
    EditHyperlinkUndoCommand(const QString & enmlBefore, const QString & enmlAfter,
                             NoteEditorPrivate & noteEditorPrivate, QUndoCommand * parent = Q_NULLPTR);
    EditHyperlinkUndoCommand(const QString & enmlBefore, const QString & enmlAfter,
                             NoteEditorPrivate & noteEditorPrivate, const QString & text,
                             QUndoCommand * parent = Q_NULLPTR);
    virtual ~EditHyperlinkUndoCommand();

    virtual void redo() Q_DECL_OVERRIDE;
    virtual void undo() Q_DECL_OVERRIDE;

private:
    QString     m_enmlBefore;
    QString     m_enmlAfter;
};

} // namespace qute_note

#endif // __LIB_QUTE_NOTE__NOTE_EDITOR__UNDO_STACK__EDIT_HYPERLINK_UNDO_COMMAND_H
