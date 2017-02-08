#include "HideDecryptedTextUndoCommand.h"
#include "../NoteEditor_p.h"
#include <quentier/logging/QuentierLogger.h>

namespace quentier {

#define GET_PAGE() \
    NoteEditorPage * page = qobject_cast<NoteEditorPage*>(m_noteEditorPrivate.page()); \
    if (Q_UNLIKELY(!page)) { \
        ErrorString error(QT_TRANSLATE_NOOP("", "can't undo/redo the decrypted text hiding: can't get note editor page")); \
        QNWARNING(error); \
        emit notifyError(error); \
        return; \
    }

HideDecryptedTextUndoCommand::HideDecryptedTextUndoCommand(NoteEditorPrivate & noteEditorPrivate, const Callback & callback, QUndoCommand * parent) :
    INoteEditorUndoCommand(noteEditorPrivate, parent),
    m_callback(callback)
{
    setText(tr("Hide decrypted text"));
}

HideDecryptedTextUndoCommand::HideDecryptedTextUndoCommand(NoteEditorPrivate & noteEditorPrivate, const Callback & callback, const QString & text, QUndoCommand * parent) :
    INoteEditorUndoCommand(noteEditorPrivate, text, parent),
    m_callback(callback)
{}

HideDecryptedTextUndoCommand::~HideDecryptedTextUndoCommand()
{}

void HideDecryptedTextUndoCommand::redoImpl()
{
    QNDEBUG(QStringLiteral("HideDecryptedTextUndoCommand::redoImpl"));

    GET_PAGE()
    page->executeJavaScript(QStringLiteral("encryptDecryptManager.redo();"), m_callback);
}

void HideDecryptedTextUndoCommand::undoImpl()
{
    QNDEBUG(QStringLiteral("HideDecryptedTextUndoCommand::undoImpl"));

    GET_PAGE()
    page->executeJavaScript(QStringLiteral("encryptDecryptManager.undo();"), m_callback);
}

} // namespace quentier
