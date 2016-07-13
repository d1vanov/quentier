#include "DecryptUndoCommand.h"
#include "../NoteEditor_p.h"
#include <quentier/logging/QuentierLogger.h>

namespace quentier {

#define GET_PAGE() \
    NoteEditorPage * page = qobject_cast<NoteEditorPage*>(m_noteEditorPrivate.page()); \
    if (Q_UNLIKELY(!page)) { \
        QNLocalizedString error = QT_TR_NOOP("can't undo/redo the encrypted text decryption: no note editor page"); \
        QNWARNING(error); \
        emit notifyError(error); \
        return; \
    }

DecryptUndoCommand::DecryptUndoCommand(const EncryptDecryptUndoCommandInfo & info, const QSharedPointer<DecryptedTextManager> & decryptedTextManager,
                                       NoteEditorPrivate & noteEditorPrivate, const Callback & callback, QUndoCommand * parent) :
    INoteEditorUndoCommand(noteEditorPrivate, parent),
    m_info(info),
    m_decryptedTextManager(decryptedTextManager),
    m_callback(callback)
{
    setText(tr("Decrypt text"));
}

DecryptUndoCommand::DecryptUndoCommand(const EncryptDecryptUndoCommandInfo & info, const QSharedPointer<DecryptedTextManager> & decryptedTextManager,
                                       NoteEditorPrivate & noteEditorPrivate, const Callback & callback, const QString & text, QUndoCommand * parent) :
    INoteEditorUndoCommand(noteEditorPrivate, text, parent),
    m_info(info),
    m_decryptedTextManager(decryptedTextManager),
    m_callback(callback)
{}

DecryptUndoCommand::~DecryptUndoCommand()
{}

void DecryptUndoCommand::redoImpl()
{
    QNDEBUG("DecryptUndoCommand::redoImpl");

    GET_PAGE()

    if (!m_info.m_decryptPermanently) {
        m_decryptedTextManager->addEntry(m_info.m_encryptedText, m_info.m_decryptedText,
                                         m_info.m_rememberForSession, m_info.m_passphrase,
                                         m_info.m_cipher, m_info.m_keyLength);
    }

    page->executeJavaScript("encryptDecryptManager.redo();", m_callback);
}

void DecryptUndoCommand::undoImpl()
{
    QNDEBUG("DecryptUndoCommand::undoImpl");

    GET_PAGE()

    if (!m_info.m_decryptPermanently) {
        m_decryptedTextManager->removeEntry(m_info.m_encryptedText);
    }

    page->executeJavaScript("encryptDecryptManager.undo();", m_callback);
}

} // namespace quentier
