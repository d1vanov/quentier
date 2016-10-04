#include "EditHyperlinkUndoCommand.h"
#include "../NoteEditor_p.h"
#include <quentier/logging/QuentierLogger.h>

namespace quentier {

#define GET_PAGE() \
    NoteEditorPage * page = qobject_cast<NoteEditorPage*>(m_noteEditorPrivate.page()); \
    if (Q_UNLIKELY(!page)) { \
        QNLocalizedString error = QT_TR_NOOP("can't undo/redo hyperlink edit: no note editor page"); \
        QNWARNING(error); \
        emit notifyError(error); \
        return; \
    }

EditHyperlinkUndoCommand::EditHyperlinkUndoCommand(NoteEditorPrivate & noteEditorPrivate, const Callback & callback, QUndoCommand * parent) :
    INoteEditorUndoCommand(noteEditorPrivate, parent),
    m_callback(callback)
{
    setText(tr("Edit hyperlink"));
}

EditHyperlinkUndoCommand::EditHyperlinkUndoCommand(NoteEditorPrivate & noteEditorPrivate, const Callback & callback, const QString & text, QUndoCommand * parent) :
    INoteEditorUndoCommand(noteEditorPrivate, text, parent),
    m_callback(callback)
{}

EditHyperlinkUndoCommand::~EditHyperlinkUndoCommand()
{}

void EditHyperlinkUndoCommand::redoImpl()
{
    QNDEBUG(QStringLiteral("EditHyperlinkUndoCommand::redoImpl"));

    GET_PAGE()
    page->executeJavaScript(QStringLiteral("hyperlinkManager.redo();"), m_callback);
}

void EditHyperlinkUndoCommand::undoImpl()
{
    QNDEBUG(QStringLiteral("EditHyperlinkUndoCommand::undoImpl"));

    GET_PAGE()
    page->executeJavaScript(QStringLiteral("hyperlinkManager.undo();"), m_callback);
}

} // namespace quentier
