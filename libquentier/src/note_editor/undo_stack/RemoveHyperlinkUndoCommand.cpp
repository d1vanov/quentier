#include "RemoveHyperlinkUndoCommand.h"
#include "../NoteEditor_p.h"
#include <quentier/logging/QuentierLogger.h>

namespace quentier {

#define GET_PAGE() \
    NoteEditorPage * page = qobject_cast<NoteEditorPage*>(m_noteEditorPrivate.page()); \
    if (Q_UNLIKELY(!page)) { \
        QNLocalizedString error = QT_TR_NOOP("can't undo/redo hyperlink removal: no note editor's page"); \
        QNWARNING(error); \
        emit notifyError(error); \
        return; \
    }

RemoveHyperlinkUndoCommand::RemoveHyperlinkUndoCommand(NoteEditorPrivate & noteEditor, const Callback & callback,
                                                       QUndoCommand * parent) :
    INoteEditorUndoCommand(noteEditor, parent),
    m_callback(callback)
{
    setText(tr("Remove hyperlink"));
}

RemoveHyperlinkUndoCommand::RemoveHyperlinkUndoCommand(NoteEditorPrivate & noteEditor, const Callback & callback,
                                                       const QString & text, QUndoCommand * parent) :
    INoteEditorUndoCommand(noteEditor, text, parent),
    m_callback(callback)
{}

RemoveHyperlinkUndoCommand::~RemoveHyperlinkUndoCommand()
{}

void RemoveHyperlinkUndoCommand::redoImpl()
{
    QNDEBUG("RemoveHyperlinkUndoCommand::redoImpl");

    GET_PAGE()
    page->executeJavaScript("hyperlinkManager.redo();", m_callback);
}

void RemoveHyperlinkUndoCommand::undoImpl()
{
    QNDEBUG("RemoveHyperlinkUndoCommand::undoImpl");

    GET_PAGE()
    page->executeJavaScript("hyperlinkManager.undo();", m_callback);
}

} // namespace quentier
