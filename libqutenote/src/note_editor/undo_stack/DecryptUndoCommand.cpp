#include "DecryptUndoCommand.h"
#include "../NoteEditor_p.h"
#include <qute_note/logging/QuteNoteLogger.h>

namespace qute_note {

DecryptUndoCommand::DecryptUndoCommand(const EncryptDecryptUndoCommandInfo & info, QSharedPointer<DecryptedTextManager> decryptedTextManager,
                                       const QString & htmlWithDecryptedText, const int pageXOffset, const int pageYOffset,
                                       NoteEditorPrivate & noteEditorPrivate, QUndoCommand * parent) :
    INoteEditorUndoCommand(noteEditorPrivate, parent),
    m_info(info),
    m_decryptedTextManager(decryptedTextManager),
    m_htmlWithDecryptedText(htmlWithDecryptedText),
    m_pageXOffset(pageXOffset),
    m_pageYOffset(pageYOffset)
{
    QUndoCommand::setText(QObject::tr("Decrypt text"));
}

DecryptUndoCommand::DecryptUndoCommand(const EncryptDecryptUndoCommandInfo & info, QSharedPointer<DecryptedTextManager> decryptedTextManager,
                                       const QString & htmlWithDecryptedText, const int pageXOffset, const int pageYOffset,
                                       NoteEditorPrivate & noteEditorPrivate, const QString & text, QUndoCommand * parent) :
    INoteEditorUndoCommand(noteEditorPrivate, text, parent),
    m_info(info),
    m_decryptedTextManager(decryptedTextManager),
    m_htmlWithDecryptedText(htmlWithDecryptedText),
    m_pageXOffset(pageXOffset),
    m_pageYOffset(pageYOffset)
{}

DecryptUndoCommand::~DecryptUndoCommand()
{}

void DecryptUndoCommand::redoImpl()
{
    QNDEBUG("DecryptUndoCommand::redoImpl");

    if (!m_info.m_decryptPermanently) {
        m_decryptedTextManager->addEntry(m_info.m_encryptedText, m_info.m_decryptedText,
                                         m_info.m_rememberForSession, m_info.m_passphrase,
                                         m_info.m_cipher, m_info.m_keyLength);
    }

    m_noteEditorPrivate.switchEditorPage(/* should convert from note = */ false);
    m_noteEditorPrivate.setPageOffsetsForNextLoad(m_pageXOffset, m_pageYOffset);

    if (m_info.m_decryptPermanently) {
        m_noteEditorPrivate.setNoteHtml(m_htmlWithDecryptedText);
    }
    else {
        m_noteEditorPrivate.updateFromNote();
    }
}

void DecryptUndoCommand::undoImpl()
{
    QNDEBUG("DecryptUndoCommand::undoImpl");

    if (!m_info.m_decryptPermanently) {
        m_decryptedTextManager->removeEntry(m_info.m_encryptedText);
    }

    m_noteEditorPrivate.popEditorPage();
}

} // namespace qute_note
