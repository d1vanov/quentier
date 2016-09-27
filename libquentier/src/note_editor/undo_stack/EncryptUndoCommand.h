#ifndef LIB_QUENTIER_NOTE_EDITOR_UNDO_STACK_ENCRYPT_UNDO_COMMAND_H
#define LIB_QUENTIER_NOTE_EDITOR_UNDO_STACK_ENCRYPT_UNDO_COMMAND_H

#include "INoteEditorUndoCommand.h"
#include "../NoteEditorPage.h"

namespace quentier {

class EncryptUndoCommand: public INoteEditorUndoCommand
{
    Q_OBJECT
    typedef NoteEditorPage::Callback Callback;
public:
    EncryptUndoCommand(NoteEditorPrivate & noteEditorPrivate, const Callback & callback,
                       QUndoCommand * parent = Q_NULLPTR);
    EncryptUndoCommand(NoteEditorPrivate & noteEditorPrivate, const Callback & callback,
                       const QString & text, QUndoCommand * parent = Q_NULLPTR);
    virtual ~EncryptUndoCommand();

    virtual void redoImpl() Q_DECL_OVERRIDE;
    virtual void undoImpl() Q_DECL_OVERRIDE;

private:
    Callback    m_callback;
};

} // namespace quentier

#endif // LIB_QUENTIER_NOTE_EDITOR_UNDO_STACK_ENCRYPT_UNDO_COMMAND_H
