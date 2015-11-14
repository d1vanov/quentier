#include "DecryptUndoCommand.h"
#include "../NoteEditor_p.h"
#include <qute_note/note_editor/DecryptedTextManager.h>
#include <qute_note/logging/QuteNoteLogger.h>

namespace qute_note {

DecryptUndoCommand::DecryptUndoCommand(const EncryptDecryptUndoCommandInfo & info,
                                       DecryptedTextManager & decryptedTextManager,
                                       NoteEditorPrivate & noteEditorPrivate,
                                       QUndoCommand * parent) :
    INoteEditorUndoCommand(noteEditorPrivate, parent),
    m_info(info),
    m_decryptedTextManager(decryptedTextManager)
{
    init();
}

DecryptUndoCommand::DecryptUndoCommand(const EncryptDecryptUndoCommandInfo & info,
                                       DecryptedTextManager & decryptedTextManager,
                                       NoteEditorPrivate & noteEditorPrivate,
                                       const QString & text, QUndoCommand * parent) :
    INoteEditorUndoCommand(noteEditorPrivate, text, parent),
    m_info(info),
    m_decryptedTextManager(decryptedTextManager)
{
    init();
}

DecryptUndoCommand::~DecryptUndoCommand()
{}

void DecryptUndoCommand::redo()
{
    QNDEBUG("DecryptUndoCommand::redo");

    m_decryptedTextManager.addEntry(m_info.m_encryptedText, m_info.m_decryptedText,
                                    m_info.m_rememberForSession, m_info.m_passphrase,
                                    m_info.m_cipher, m_info.m_keyLength);

    m_noteEditorPrivate.decryptEncryptedText(m_info.m_encryptedText, m_info.m_decryptedText,
                                             m_info.m_rememberForSession, m_info.m_decryptPermanently);
}

void DecryptUndoCommand::undo()
{
    QNDEBUG("DecryptUndoCommand::undo");

    m_decryptedTextManager.removeEntry(m_info.m_encryptedText);
    m_noteEditorPrivate.updateFromNote();   // Force re-conversion from ENML to HTML
}

void DecryptUndoCommand::init()
{
    QUndoCommand::setText(QObject::tr("Decrypt text"));
}

} // namespace qute_note
