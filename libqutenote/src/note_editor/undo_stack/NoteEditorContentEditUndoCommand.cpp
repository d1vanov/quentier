#include "NoteEditorContentEditUndoCommand.h"
#include "../NoteEditor_p.h"
#include <qute_note/logging/QuteNoteLogger.h>

namespace qute_note {

NoteEditorContentEditUndoCommand::NoteEditorContentEditUndoCommand(NoteEditorPrivate & noteEditorPrivate,
                                                                   const QList<ResourceWrapper> & resources,
                                                                   QUndoCommand * parent) :
    INoteEditorUndoCommand(noteEditorPrivate, parent),
    m_resources(resources)
{}

NoteEditorContentEditUndoCommand::NoteEditorContentEditUndoCommand(NoteEditorPrivate & noteEditorPrivate,
                                                                   const QList<ResourceWrapper> & resources,
                                                                   const QString & text, QUndoCommand * parent) :
    INoteEditorUndoCommand(noteEditorPrivate, text, parent),
    m_resources(resources)
{}

NoteEditorContentEditUndoCommand::~NoteEditorContentEditUndoCommand()
{}

void NoteEditorContentEditUndoCommand::redo()
{
    QNDEBUG("NoteEditorContentEditUndoCommand::redo (" << text() << ")");
    m_noteEditorPrivate.redoPageAction();
}

void NoteEditorContentEditUndoCommand::undo()
{
    QNDEBUG("NoteEditorContentEditUndoCommand::undo (" << text() << ")");
    m_noteEditorPrivate.undoPageAction();
    m_noteEditorPrivate.setNoteResources(m_resources);
}

} // namespace qute_note
