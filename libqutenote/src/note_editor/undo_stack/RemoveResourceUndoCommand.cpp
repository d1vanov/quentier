#include "RemoveResourceUndoCommand.h"
#include "../NoteEditor_p.h"
#include <qute_note/logging/QuteNoteLogger.h>

namespace qute_note {

RemoveResourceUndoCommand::RemoveResourceUndoCommand(const ResourceWrapper & resource, const QString & htmlWithRemovedResource,
                                                     NoteEditorPrivate & noteEditorPrivate, QUndoCommand * parent) :
    INoteEditorUndoCommand(noteEditorPrivate, parent),
    m_resource(resource),
    m_htmlWithRemovedResource(htmlWithRemovedResource)
{
    setText(QObject::tr("Remove attachment"));
}

RemoveResourceUndoCommand::RemoveResourceUndoCommand(const ResourceWrapper & resource, const QString & htmlWithRemovedResource,
                                                     NoteEditorPrivate & noteEditorPrivate, const QString & text, QUndoCommand * parent) :
    INoteEditorUndoCommand(noteEditorPrivate, text, parent),
    m_resource(resource),
    m_htmlWithRemovedResource(htmlWithRemovedResource)
{}

RemoveResourceUndoCommand::~RemoveResourceUndoCommand()
{}

void RemoveResourceUndoCommand::undoImpl()
{
    QNDEBUG("RemoveResourceUndoCommand::undoImpl");

    m_noteEditorPrivate.addResourceToNote(m_resource);
    m_noteEditorPrivate.popEditorPage();
}

void RemoveResourceUndoCommand::redoImpl()
{
    QNDEBUG("RemoveResourceUndoCommand::redoImpl");

    m_noteEditorPrivate.switchEditorPage(/* should convert from note = */ false);
    m_noteEditorPrivate.removeResourceFromNote(m_resource);
    m_noteEditorPrivate.skipPushingUndoCommandOnNextContentChange();
    m_noteEditorPrivate.setNoteHtml(m_htmlWithRemovedResource);
}

} // namespace qute_note
