#ifndef __LIB_QUTE_NOTE__NOTE_EDITOR__UNDO_STACK__EDIT_HYPERLINK_UNDO_COMMAND_H
#define __LIB_QUTE_NOTE__NOTE_EDITOR__UNDO_STACK__EDIT_HYPERLINK_UNDO_COMMAND_H

#include "INoteEditorUndoCommand.h"

namespace qute_note {

class EditHyperlinkUndoCommand: public INoteEditorUndoCommand
{
public:
    EditHyperlinkUndoCommand(const quint64 hyperlinkId, const QString & linkBefore, const QString & textBefore,
                             const QString & linkAfter, const QString & textAfter, NoteEditorPrivate & noteEditorPrivate,
                             QUndoCommand * parent = Q_NULLPTR);
    EditHyperlinkUndoCommand(const quint64 hyperlinkId, const QString & linkBefore, const QString & textBefore,
                             const QString & linkAfter, const QString & textAfter, NoteEditorPrivate & noteEditorPrivate,
                             const QString & text, QUndoCommand * parent = Q_NULLPTR);
    virtual ~EditHyperlinkUndoCommand();

    virtual void redoImpl() Q_DECL_OVERRIDE;
    virtual void undoImpl() Q_DECL_OVERRIDE;

private:
    quint64     m_hyperlinkId;

    QString     m_linkBefore;
    QString     m_textBefore;

    QString     m_linkAfter;
    QString     m_textAfter;
};

} // namespace qute_note

#endif // __LIB_QUTE_NOTE__NOTE_EDITOR__UNDO_STACK__EDIT_HYPERLINK_UNDO_COMMAND_H
