#include "AddHyperlinkUndoCommand.h"
#include "../NoteEditor_p.h"
#include <quentier/logging/QuentierLogger.h>

namespace quentier {

#define GET_PAGE() \
    NoteEditorPage * page = qobject_cast<NoteEditorPage*>(m_noteEditorPrivate.page()); \
    if (Q_UNLIKELY(!page)) { \
        ErrorString error(QT_TRANSLATE_NOOP("", "can't undo/redo adding the hyperlink to the selected text: no note editor page")); \
        QNWARNING(error); \
        emit notifyError(error); \
        return; \
    }

AddHyperlinkUndoCommand::AddHyperlinkUndoCommand(NoteEditorPrivate & noteEditor, const Callback & callback, QUndoCommand * parent) :
    INoteEditorUndoCommand(noteEditor, parent),
    m_callback(callback)
{
    setText(tr("Add hyperlink"));
}

AddHyperlinkUndoCommand::AddHyperlinkUndoCommand(NoteEditorPrivate & noteEditor, const Callback & callback, const QString & text, QUndoCommand * parent) :
    INoteEditorUndoCommand(noteEditor, text, parent),
    m_callback(callback)
{}

AddHyperlinkUndoCommand::~AddHyperlinkUndoCommand()
{}

void AddHyperlinkUndoCommand::redoImpl()
{
    QNDEBUG(QStringLiteral("AddHyperlinkUndoCommand::redoImpl"));

    GET_PAGE()
    page->executeJavaScript(QStringLiteral("hyperlinkManager.redo();"), m_callback);
}

void AddHyperlinkUndoCommand::undoImpl()
{
    QNDEBUG(QStringLiteral("AddHyperlinkUndoCommand::undoImpl"));

    GET_PAGE()
    page->executeJavaScript(QStringLiteral("hyperlinkManager.undo();"), m_callback);
}

} // namespace quentier
