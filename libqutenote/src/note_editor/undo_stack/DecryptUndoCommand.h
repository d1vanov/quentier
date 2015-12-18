#ifndef __LIB_QUTE_NOTE__NOTE_EDITOR__UNDO_STACK__DECRYPT_UNDO_COMMAND_H
#define __LIB_QUTE_NOTE__NOTE_EDITOR__UNDO_STACK__DECRYPT_UNDO_COMMAND_H

#include "INoteEditorUndoCommand.h"
#include "EncryptDecryptUndoCommandInfo.h"

namespace qute_note {

QT_FORWARD_DECLARE_CLASS(DecryptedTextManager)

class DecryptUndoCommand: public INoteEditorUndoCommand
{
public:
    DecryptUndoCommand(const EncryptDecryptUndoCommandInfo & info, DecryptedTextManager & decryptedTextManager,
                       const QString & htmlWithDecryptedText, const int pageXOffset, const int pageYOffset,
                       NoteEditorPrivate & noteEditorPrivate, QUndoCommand * parent = Q_NULLPTR);
    DecryptUndoCommand(const EncryptDecryptUndoCommandInfo & info, DecryptedTextManager & decryptedTextManager,
                       const QString & htmlWithDecryptedText, const int pageXOffset, const int pageYOffset,
                       NoteEditorPrivate & noteEditorPrivate, const QString & text, QUndoCommand * parent = Q_NULLPTR);
    virtual ~DecryptUndoCommand();

    virtual void redoImpl() Q_DECL_OVERRIDE;
    virtual void undoImpl() Q_DECL_OVERRIDE;

private:
    EncryptDecryptUndoCommandInfo   m_info;
    DecryptedTextManager &          m_decryptedTextManager;
    QString                         m_htmlWithDecryptedText;

    int                             m_pageXOffset;
    int                             m_pageYOffset;
};

} // namespace qute_note

#endif // __LIB_QUTE_NOTE__NOTE_EDITOR__UNDO_STACK__DECRYPT_UNDO_COMMAND_H
