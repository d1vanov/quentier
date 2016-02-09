#include "AddResourceUndoCommand.h"
#include "../NoteEditor_p.h"
#include <qute_note/logging/QuteNoteLogger.h>

namespace qute_note {

#define GET_PAGE() \
    NoteEditorPage * page = qobject_cast<NoteEditorPage*>(m_noteEditorPrivate.page()); \
    if (Q_UNLIKELY(!page)) { \
        QString error = QT_TR_NOOP("Can't undo/redo adding the attachment: can't get note editor page"); \
        QNWARNING(error); \
        return; \
    }

AddResourceUndoCommand::AddResourceUndoCommand(const ResourceWrapper & resource, const Callback & callback,
                                               NoteEditorPrivate & noteEditorPrivate, QUndoCommand * parent) :
    INoteEditorUndoCommand(noteEditorPrivate, parent),
    m_resource(resource),
    m_callback(callback)
{
    setText(QObject::tr("Add attachment"));
}

AddResourceUndoCommand::AddResourceUndoCommand(const ResourceWrapper & resource, const Callback & callback,
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
    QNDEBUG("AddResourceUndoCommand::undoImpl");

    m_noteEditorPrivate.removeResourceFromNote(m_resource);

    GET_PAGE()
    page->executeJavaScript("resourceManager.undo();", m_callback);
}

void AddResourceUndoCommand::redoImpl()
{
    QNDEBUG("AddResourceUndoCommand::redoImpl");

    m_noteEditorPrivate.addResourceToNote(m_resource);

    GET_PAGE()
    page->executeJavaScript("resourceManager.redo();", m_callback);
}

} // namespace qute_note
