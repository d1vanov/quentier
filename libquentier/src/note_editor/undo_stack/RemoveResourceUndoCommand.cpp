#include "RemoveResourceUndoCommand.h"
#include "../NoteEditor_p.h"
#include <quentier/logging/QuentierLogger.h>

namespace quentier {

#define GET_PAGE() \
    NoteEditorPage * page = qobject_cast<NoteEditorPage*>(m_noteEditorPrivate.page()); \
    if (Q_UNLIKELY(!page)) { \
        QNLocalizedString error = QT_TR_NOOP("can't undo/redo remove attachment: can't get note editor page"); \
        QNWARNING(error); \
        emit notifyError(error); \
        return; \
    }

RemoveResourceUndoCommand::RemoveResourceUndoCommand(const ResourceWrapper & resource, const Callback & callback,
                                                     NoteEditorPrivate & noteEditorPrivate, QUndoCommand * parent) :
    INoteEditorUndoCommand(noteEditorPrivate, parent),
    m_resource(resource),
    m_callback(callback)
{
    setText(tr("Remove attachment"));
}

RemoveResourceUndoCommand::RemoveResourceUndoCommand(const ResourceWrapper & resource, const Callback & callback,
                                                     NoteEditorPrivate & noteEditorPrivate, const QString & text,
                                                     QUndoCommand * parent) :
    INoteEditorUndoCommand(noteEditorPrivate, text, parent),
    m_resource(resource),
    m_callback(callback)
{}

RemoveResourceUndoCommand::~RemoveResourceUndoCommand()
{}

void RemoveResourceUndoCommand::undoImpl()
{
    QNDEBUG(QStringLiteral("RemoveResourceUndoCommand::undoImpl"));

    m_noteEditorPrivate.addResourceToNote(m_resource);

    GET_PAGE()
    page->executeJavaScript(QStringLiteral("resourceManager.undo();"));
    page->executeJavaScript(QStringLiteral("setupGenericResourceOnClickHandler();"), m_callback);
}

void RemoveResourceUndoCommand::redoImpl()
{
    QNDEBUG(QStringLiteral("RemoveResourceUndoCommand::redoImpl"));

    m_noteEditorPrivate.removeResourceFromNote(m_resource);

    GET_PAGE()
    page->executeJavaScript(QStringLiteral("resourceManager.redo();"), m_callback);
}

} // namespace quentier
