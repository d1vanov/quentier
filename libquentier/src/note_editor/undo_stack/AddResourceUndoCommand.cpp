#include "AddResourceUndoCommand.h"
#include "../NoteEditor_p.h"
#include <quentier/logging/QuentierLogger.h>

namespace quentier {

#define GET_PAGE() \
    NoteEditorPage * page = qobject_cast<NoteEditorPage*>(m_noteEditorPrivate.page()); \
    if (Q_UNLIKELY(!page)) { \
        ErrorString error(QT_TRANSLATE_NOOP("", "can't undo/redo adding the attachment: no note editor page")); \
        QNWARNING(error); \
        emit notifyError(error); \
        return; \
    }

AddResourceUndoCommand::AddResourceUndoCommand(const Resource & resource, const Callback & callback,
                                               NoteEditorPrivate & noteEditorPrivate, QUndoCommand * parent) :
    INoteEditorUndoCommand(noteEditorPrivate, parent),
    m_resource(resource),
    m_callback(callback)
{
    setText(tr("Add attachment"));
}

AddResourceUndoCommand::AddResourceUndoCommand(const Resource & resource, const Callback & callback,
                                               NoteEditorPrivate & noteEditorPrivate, const QString & text,
                                               QUndoCommand * parent) :
    INoteEditorUndoCommand(noteEditorPrivate, text, parent),
    m_resource(resource),
    m_callback(callback)
{}

AddResourceUndoCommand::~AddResourceUndoCommand()
{}

void AddResourceUndoCommand::undoImpl()
{
    QNDEBUG(QStringLiteral("AddResourceUndoCommand::undoImpl"));

    m_noteEditorPrivate.removeResourceFromNote(m_resource);

    GET_PAGE()
    page->executeJavaScript(QStringLiteral("resourceManager.undo();"), m_callback);
}

void AddResourceUndoCommand::redoImpl()
{
    QNDEBUG(QStringLiteral("AddResourceUndoCommand::redoImpl"));

    m_noteEditorPrivate.addResourceToNote(m_resource);

    GET_PAGE()
    page->executeJavaScript(QStringLiteral("resourceManager.redo();"), m_callback);
}

} // namespace quentier
