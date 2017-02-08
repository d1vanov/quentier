#include "ReplaceAllUndoCommand.h"
#include "../NoteEditor_p.h"
#include <quentier/logging/QuentierLogger.h>

namespace quentier {

#define GET_PAGE() \
    NoteEditorPage * page = qobject_cast<NoteEditorPage*>(m_noteEditorPrivate.page()); \
    if (Q_UNLIKELY(!page)) { \
        ErrorString error(QT_TRANSLATE_NOOP("", "can't undo/redo text replacement: can't get note editor page")); \
        QNWARNING(error); \
        emit notifyError(error); \
        return; \
    }

ReplaceAllUndoCommand::ReplaceAllUndoCommand(const QString & textToReplace, const bool matchCase, NoteEditorPrivate & noteEditorPrivate,
                                             Callback callback, QUndoCommand * parent) :
    INoteEditorUndoCommand(noteEditorPrivate, parent),
    m_textToReplace(textToReplace),
    m_matchCase(matchCase),
    m_callback(callback)
{
    setText(tr("Replace all"));
}

ReplaceAllUndoCommand::ReplaceAllUndoCommand(const QString & textToReplace, const bool matchCase, NoteEditorPrivate & noteEditorPrivate,
                                             const QString & text, Callback callback, QUndoCommand * parent) :
    INoteEditorUndoCommand(noteEditorPrivate, text, parent),
    m_textToReplace(textToReplace),
    m_matchCase(matchCase),
    m_callback(callback)
{}

ReplaceAllUndoCommand::~ReplaceAllUndoCommand()
{}

void ReplaceAllUndoCommand::redoImpl()
{
    QNDEBUG(QStringLiteral("ReplaceAllUndoCommand::redoImpl"));

    GET_PAGE()

    QString javascript = QStringLiteral("findReplaceManager.redoReplaceAll();");
    page->executeJavaScript(javascript, m_callback);

    if (m_noteEditorPrivate.searchHighlightEnabled()) {
        m_noteEditorPrivate.setSearchHighlight(m_textToReplace, m_matchCase, /* force = */ true);
    }
}

void ReplaceAllUndoCommand::undoImpl()
{
    QNDEBUG(QStringLiteral("ReplaceAllUndoCommand::undoImpl"));

    GET_PAGE()

    QString javascript = QStringLiteral("findReplaceManager.undoReplaceAll();");
    page->executeJavaScript(javascript, m_callback);

    if (m_noteEditorPrivate.searchHighlightEnabled()) {
        m_noteEditorPrivate.setSearchHighlight(m_textToReplace, m_matchCase, /* force = */ true);
    }
}

} // namespace quentier
