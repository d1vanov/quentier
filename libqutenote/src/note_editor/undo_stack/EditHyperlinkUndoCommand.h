#ifndef __LIB_QUTE_NOTE__NOTE_EDITOR__UNDO_STACK__EDIT_HYPERLINK_UNDO_COMMAND_H
#define __LIB_QUTE_NOTE__NOTE_EDITOR__UNDO_STACK__EDIT_HYPERLINK_UNDO_COMMAND_H

#include "INoteEditorUndoCommand.h"

namespace qute_note {

class EditHyperlinkUndoCommand: public INoteEditorUndoCommand
{
public:
    EditHyperlinkUndoCommand(NoteEditorPrivate & noteEditorPrivate, QUndoCommand * parent = Q_NULLPTR);
    EditHyperlinkUndoCommand(NoteEditorPrivate & noteEditorPrivate, const QString & text, QUndoCommand * parent = Q_NULLPTR);
    virtual ~EditHyperlinkUndoCommand();

    void setHtmlBefore(const QString & htmlBefore);
    void setHtmlAfter(const QString & htmlAfter);

    virtual void redoImpl() Q_DECL_OVERRIDE;
    virtual void undoImpl() Q_DECL_OVERRIDE;

private:
    void init();

private:
    QString     m_htmlBefore;
    QString     m_htmlAfter;

    bool        m_htmlBeforeSet;
    bool        m_htmlAfterSet;
};

} // namespace qute_note

#endif // __LIB_QUTE_NOTE__NOTE_EDITOR__UNDO_STACK__EDIT_HYPERLINK_UNDO_COMMAND_H
