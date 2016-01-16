#include "ReplaceUndoCommand.h"
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


ReplaceUndoCommand::ReplaceUndoCommand(NoteEditorPrivate & noteEditorPrivate, QUndoCommand * parent) :
    INoteEditorUndoCommand(noteEditorPrivate, parent)
{
    setText(QObject::tr("Replace text"));
}

ReplaceUndoCommand::ReplaceUndoCommand(NoteEditorPrivate & noteEditorPrivate, const QString & text, QUndoCommand * parent) :
    INoteEditorUndoCommand(noteEditorPrivate, text, parent)
{}

ReplaceUndoCommand::~ReplaceUndoCommand()
{}

void ReplaceUndoCommand::redoImpl()
{
    QNDEBUG("ReplaceUndoCommand::redoImpl");

    GET_PAGE()

    m_noteEditorPrivate.skipPushingUndoCommandOnNextContentChange();

    QString javascript = "Replacer.redo();";
    page->executeJavaScript(javascript);
}

void ReplaceUndoCommand::undoImpl()
{
    QNDEBUG("ReplaceUndoCommand::undoImpl");

    GET_PAGE()

    m_noteEditorPrivate.skipPushingUndoCommandOnNextContentChange();

    QString javascript = "Replacer.undo();";
    page->executeJavaScript(javascript);
}

} // namespace qute_note
