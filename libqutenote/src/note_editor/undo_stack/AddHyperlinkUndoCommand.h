#ifndef __LIB_QUTE_NOTE__NOTE_EDITOR__UNDO_STACK__ADD_HYPERLINK_UNDO_COMMAND_H
#define __LIB_QUTE_NOTE__NOTE_EDITOR__UNDO_STACK__ADD_HYPERLINK_UNDO_COMMAND_H

#include "INoteEditorUndoCommand.h"

namespace qute_note {

class AddHyperlinkUndoCommand: public INoteEditorUndoCommand
{
public:
    AddHyperlinkUndoCommand(const QString & htmlWithHyperlink, const int pageXOffset, const int pageYOffset,
                            NoteEditorPrivate & noteEditor, QUndoCommand * parent = Q_NULLPTR);
    AddHyperlinkUndoCommand(const QString & htmlWithHyperlink, const int pageXOffset, const int pageYOffset,
                            NoteEditorPrivate & noteEditor, const QString & text, QUndoCommand * parent = Q_NULLPTR);
    virtual ~AddHyperlinkUndoCommand();

    virtual void redoImpl() Q_DECL_OVERRIDE;
    virtual void undoImpl() Q_DECL_OVERRIDE;

private:
    QString     m_htmlWithHyperlink;
    int         m_pageXOffset;
    int         m_pageYOffset;
};

} // namespace qute_note

#endif // __LIB_QUTE_NOTE__NOTE_EDITOR__UNDO_STACK__ADD_HYPERLINK_UNDO_COMMAND_H
