#ifndef __LIB_QUTE_NOTE__NOTE_EDITOR__UNDO_STACK__ENCRYPT_UNDO_COMMAND_H
#define __LIB_QUTE_NOTE__NOTE_EDITOR__UNDO_STACK__ENCRYPT_UNDO_COMMAND_H

#include "INoteEditorUndoCommand.h"

namespace qute_note {

class EncryptUndoCommand: public INoteEditorUndoCommand
{
public:
    EncryptUndoCommand(NoteEditorPrivate & noteEditorPrivate, QUndoCommand * parent = Q_NULLPTR);
    EncryptUndoCommand(NoteEditorPrivate & noteEditorPrivate, const QString & text, QUndoCommand * parent = Q_NULLPTR);
    virtual ~EncryptUndoCommand();

    virtual void redoImpl() Q_DECL_OVERRIDE;
    virtual void undoImpl() Q_DECL_OVERRIDE;

    void setHtmlWithEncryption(const QString & html);

private:
    void init();

private:
    QString     m_htmlWithEncryption;
};

} // namespace qute_note

#endif // __LIB_QUTE_NOTE__NOTE_EDITOR__UNDO_STACK__ENCRYPT_UNDO_COMMAND_H
