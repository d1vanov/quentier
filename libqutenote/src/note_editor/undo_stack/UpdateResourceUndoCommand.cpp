#include "UpdateResourceUndoCommand.h"
#include "../NoteEditor_p.h"
#include <qute_note/logging/QuteNoteLogger.h>

namespace qute_note {

UpdateResourceUndoCommand::UpdateResourceUndoCommand(const ResourceWrapper & resourceBefore, const ResourceWrapper & resourceAfter,
                                                     NoteEditorPrivate & noteEditorPrivate, QUndoCommand * parent) :
    INoteEditorUndoCommand(noteEditorPrivate, parent),
    m_resourceBefore(resourceBefore),
    m_resourceAfter(resourceAfter)
{
    init();
}

UpdateResourceUndoCommand::UpdateResourceUndoCommand(const ResourceWrapper & resourceBefore, const ResourceWrapper & resourceAfter,
                                                     NoteEditorPrivate & noteEditorPrivate, const QString & text, QUndoCommand * parent) :
    INoteEditorUndoCommand(noteEditorPrivate, text, parent),
    m_resourceBefore(resourceBefore),
    m_resourceAfter(resourceAfter)
{
    init();
}

UpdateResourceUndoCommand::~UpdateResourceUndoCommand()
{}

void UpdateResourceUndoCommand::undo()
{
    QNDEBUG("UpdateResourceUndoCommand::undo");

    m_noteEditorPrivate.replaceResourceInNote(m_resourceBefore);
    m_noteEditorPrivate.updateFromNote();
}

void UpdateResourceUndoCommand::redo()
{
    QNDEBUG("UpdateResourceUndoCommand::redo");

    m_noteEditorPrivate.replaceResourceInNote(m_resourceAfter);
    m_noteEditorPrivate.updateFromNote();
}

void UpdateResourceUndoCommand::init()
{
    QUndoCommand::setText(QObject::tr("Edit attachment"));
}

} // namespace qute_note
