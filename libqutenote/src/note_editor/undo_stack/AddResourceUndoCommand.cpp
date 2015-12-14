#include "AddResourceUndoCommand.h"
#include "../NoteEditor_p.h"
#include <qute_note/logging/QuteNoteLogger.h>

namespace qute_note {

AddResourceUndoCommand::AddResourceUndoCommand(const ResourceWrapper & resource, const QString & htmlWithAddedResource,
                                               NoteEditorPrivate & noteEditorPrivate, QUndoCommand * parent) :
    INoteEditorUndoCommand(noteEditorPrivate, parent),
    m_resource(resource),
    m_html(htmlWithAddedResource)
{
    setText(QObject::tr("Add attachment"));
}

AddResourceUndoCommand::AddResourceUndoCommand(const ResourceWrapper & resource, const QString & htmlWithAddedResource,
                                               NoteEditorPrivate & noteEditorPrivate, const QString & text, QUndoCommand * parent) :
    INoteEditorUndoCommand(noteEditorPrivate, text, parent),
    m_resource(resource),
    m_html(htmlWithAddedResource)
{}

AddResourceUndoCommand::~AddResourceUndoCommand()
{}

void AddResourceUndoCommand::undoImpl()
{
    QNDEBUG("AddResourceUndoCommand::undoImpl");

    m_noteEditorPrivate.popEditorPage();
    m_noteEditorPrivate.removeResourceFromNote(m_resource);
}

void AddResourceUndoCommand::redoImpl()
{
    QNDEBUG("AddResourceUndoCommand::redoImpl");

    m_noteEditorPrivate.switchEditorPage(/* should convert from note = */ false);
    m_noteEditorPrivate.addResourceToNote(m_resource);
    m_noteEditorPrivate.skipPushingUndoCommandOnNextContentChange();
    m_noteEditorPrivate.setNoteHtml(m_html);
}

} // namespace qute_note
