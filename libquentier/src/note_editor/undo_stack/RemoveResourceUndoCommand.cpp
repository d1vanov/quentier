#include "RemoveResourceUndoCommand.h"
#include "../NoteEditor_p.h"
#include <quentier/logging/QuentierLogger.h>

namespace quentier {

#define GET_PAGE() \
    NoteEditorPage * page = qobject_cast<NoteEditorPage*>(m_noteEditorPrivate.page()); \
    if (Q_UNLIKELY(!page)) { \
        QString error = QT_TR_NOOP("Can't undo/redo remove attachment: can't get note editor page"); \
        QNWARNING(error); \
        return; \
    }

RemoveResourceUndoCommand::RemoveResourceUndoCommand(const ResourceWrapper & resource, const Callback & callback,
                                                     NoteEditorPrivate & noteEditorPrivate, QUndoCommand * parent) :
    INoteEditorUndoCommand(noteEditorPrivate, parent),
    m_resource(resource),
    m_callback(callback)
{
    setText(QObject::tr("Remove attachment"));
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
    QNDEBUG("RemoveResourceUndoCommand::undoImpl");

    m_noteEditorPrivate.addResourceToNote(m_resource);

    GET_PAGE()
    page->executeJavaScript("resourceManager.undo();");
    page->executeJavaScript("setupGenericResourceOnClickHandler();", m_callback);
}

void RemoveResourceUndoCommand::redoImpl()
{
    QNDEBUG("RemoveResourceUndoCommand::redoImpl");

    m_noteEditorPrivate.removeResourceFromNote(m_resource);

    GET_PAGE()
    page->executeJavaScript("resourceManager.redo();", m_callback);
}

} // namespace quentier
