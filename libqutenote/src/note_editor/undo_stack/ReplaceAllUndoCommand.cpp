#include "ReplaceAllUndoCommand.h"
#include "../NoteEditor_p.h"
#include <qute_note/logging/QuteNoteLogger.h>

namespace qute_note {

#define GET_PAGE() \
    NoteEditorPage * page = qobject_cast<NoteEditorPage*>(m_noteEditorPrivate.page()); \
    if (Q_UNLIKELY(!page)) { \
        QString error = QT_TR_NOOP("Can't undo/redo text replacement: can't get note editor page"); \
        QNWARNING(error); \
        return; \
    }

ReplaceAllUndoCommand::ReplaceAllUndoCommand(const QString & textToReplace, const bool matchCase,
                                             NoteEditorPrivate & noteEditorPrivate, Callback callback, QUndoCommand * parent) :
    INoteEditorUndoCommand(noteEditorPrivate, parent),
    m_textToReplace(textToReplace),
    m_matchCase(matchCase),
    m_callback(callback)
{
    setText(QObject::tr("Replace all"));
}

ReplaceAllUndoCommand::ReplaceAllUndoCommand(const QString & textToReplace, const bool matchCase,
                                             NoteEditorPrivate & noteEditorPrivate, const QString & text, Callback callback, QUndoCommand * parent) :
    INoteEditorUndoCommand(noteEditorPrivate, text, parent),
    m_textToReplace(textToReplace),
    m_matchCase(matchCase),
    m_callback(callback)
{}

ReplaceAllUndoCommand::~ReplaceAllUndoCommand()
{}

void ReplaceAllUndoCommand::redoImpl()
{
    QNDEBUG("ReplaceAllUndoCommand::redoImpl");

    GET_PAGE()

    QString javascript = "Replacer.redoReplaceAll();";
    page->executeJavaScript(javascript, m_callback);
}

void ReplaceAllUndoCommand::undoImpl()
{
    QNDEBUG("ReplaceAllUndoCommand::undoImpl");

    GET_PAGE()

    QString javascript = "Replacer.undoReplaceAll();";
    page->executeJavaScript(javascript, m_callback);

    if (m_noteEditorPrivate.searchHighlightEnabled()) {
        m_noteEditorPrivate.setSearchHighlight(m_textToReplace, m_matchCase, /* force = */ true);
    }
}

} // namespace qute_note
