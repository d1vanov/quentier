#include "TableActionUndoCommand.h"
#include "../NoteEditor_p.h"
#include <quentier/logging/QuentierLogger.h>

namespace quentier {

#define GET_PAGE() \
    NoteEditorPage * page = qobject_cast<NoteEditorPage*>(m_noteEditorPrivate.page()); \
    if (Q_UNLIKELY(!page)) { \
        QNLocalizedString error = QT_TR_NOOP("can't table action: can't get note editor page"); \
        QNWARNING(error); \
        emit notifyError(error); \
        return; \
    }

TableActionUndoCommand::TableActionUndoCommand(NoteEditorPrivate & noteEditorPrivate, Callback callback, QUndoCommand * parent) :
    INoteEditorUndoCommand(noteEditorPrivate, parent),
    m_callback(callback)
{
    setText(tr("Table action"));
}

TableActionUndoCommand::TableActionUndoCommand(NoteEditorPrivate & noteEditorPrivate, const QString & text, Callback callback, QUndoCommand * parent) :
    INoteEditorUndoCommand(noteEditorPrivate, text, parent),
    m_callback(callback)
{}

TableActionUndoCommand::~TableActionUndoCommand()
{}

void TableActionUndoCommand::redoImpl()
{
    QNDEBUG(QStringLiteral("TableActionUndoCommand::redoImpl"));

    GET_PAGE()

    QString javascript = QStringLiteral("tableManager.redo();");
    page->executeJavaScript(javascript, m_callback);
}

void TableActionUndoCommand::undoImpl()
{
    QNDEBUG(QStringLiteral("TableActionUndoCommand::undoImpl"));

    GET_PAGE()

    QString javascript = QStringLiteral("tableManager.undo();");
    page->executeJavaScript(javascript, m_callback);
}

} // namespace quentier
