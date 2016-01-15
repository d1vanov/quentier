#include "ReplaceUndoCommand.h"
#include "../NoteEditor_p.h"
#include <qute_note/logging/QuteNoteLogger.h>

namespace qute_note {

ReplaceUndoCommand::ReplaceUndoCommand(const QString & originalText, const QString & replacementText, const bool matchCase,
                                       NoteEditorPrivate & noteEditorPrivate, QUndoCommand * parent) :
    INoteEditorUndoCommand(noteEditorPrivate, parent),
    m_originalText(originalText),
    m_replacementText(replacementText),
    m_matchCase(matchCase)
{
    setText(QObject::tr("Replace text"));
}

ReplaceUndoCommand::ReplaceUndoCommand(const QString & originalText, const QString & replacementText, const bool matchCase,
                                       NoteEditorPrivate & noteEditorPrivate, const QString & text, QUndoCommand * parent) :
    INoteEditorUndoCommand(noteEditorPrivate, text, parent),
    m_originalText(originalText),
    m_replacementText(replacementText),
    m_matchCase(matchCase)
{}

ReplaceUndoCommand::~ReplaceUndoCommand()
{}

void ReplaceUndoCommand::redoImpl()
{
    QNDEBUG("ReplaceUndoCommand::redoImpl");
    m_noteEditorPrivate.doReplace(m_originalText, m_replacementText, m_matchCase);
}

void ReplaceUndoCommand::undoImpl()
{
    QNDEBUG("ReplaceUndoCommand::undoImpl");

    NoteEditorPage * page = qobject_cast<NoteEditorPage*>(m_noteEditorPrivate.page());
    if (Q_UNLIKELY(!page)) {
        QString error = QT_TR_NOOP("Can't undo text replacement: can't get note editor page");
        QNWARNING(error);
        return;
    }

    m_noteEditorPrivate.skipPushingUndoCommandOnNextContentChange();

    QString javascript = "Replacer.undo();";
    page->executeJavaScript(javascript);
}

} // namespace qute_note
