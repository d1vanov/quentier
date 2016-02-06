#include "TableActionUndoCommand.h"
#include "../NoteEditor_p.h"
#include <qute_note/logging/QuteNoteLogger.h>

namespace qute_note {

#define GET_PAGE() \
    NoteEditorPage * page = qobject_cast<NoteEditorPage*>(m_noteEditorPrivate.page()); \
    if (Q_UNLIKELY(!page)) { \
        QString error = QT_TR_NOOP("Can't table action: can't get note editor page"); \
        QNWARNING(error); \
        return; \
    }

TableActionUndoCommand::TableActionUndoCommand(NoteEditorPrivate & noteEditorPrivate, Callback callback, QUndoCommand * parent) :
    INoteEditorUndoCommand(noteEditorPrivate, parent),
    m_callback(callback)
{
    setText(QObject::tr("Table action"));
}

TableActionUndoCommand::TableActionUndoCommand(NoteEditorPrivate & noteEditorPrivate, const QString & text, Callback callback, QUndoCommand * parent) :
    INoteEditorUndoCommand(noteEditorPrivate, text, parent),
    m_callback(callback)
{}

TableActionUndoCommand::~TableActionUndoCommand()
{}

void TableActionUndoCommand::redoImpl()
{
    QNDEBUG("TableActionUndoCommand::redoImpl");

    GET_PAGE()

    QString javascript = "tableManager.redo();";
    page->executeJavaScript(javascript, m_callback);
}

void TableActionUndoCommand::undoImpl()
{
    QNDEBUG("TableActionUndoCommand::undoImpl");

    GET_PAGE()

    QString javascript = "tableManager.undo();";
    page->executeJavaScript(javascript, m_callback);
}

} // namespace qute_note
