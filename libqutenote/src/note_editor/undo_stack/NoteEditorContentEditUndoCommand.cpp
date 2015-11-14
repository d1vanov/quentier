#include "NoteEditorContentEditUndoCommand.h"
#include "../NoteEditor_p.h"
#include <qute_note/logging/QuteNoteLogger.h>

namespace qute_note {

NoteEditorContentEditUndoCommand::NoteEditorContentEditUndoCommand(NoteEditorPrivate & noteEditorPrivate,
                                                                   const QList<ResourceWrapper> & resources,
                                                                   QUndoCommand * parent) :
    INoteEditorUndoCommand(noteEditorPrivate, parent),
    m_resources(resources)
{
    init();
}

NoteEditorContentEditUndoCommand::NoteEditorContentEditUndoCommand(NoteEditorPrivate & noteEditorPrivate,
                                                                   const QList<ResourceWrapper> & resources,
                                                                   const QString & text, QUndoCommand * parent) :
    INoteEditorUndoCommand(noteEditorPrivate, text, parent),
    m_resources(resources)
{
    init();
}

NoteEditorContentEditUndoCommand::~NoteEditorContentEditUndoCommand()
{}

void NoteEditorContentEditUndoCommand::redoImpl()
{
    QNDEBUG("NoteEditorContentEditUndoCommand::redoImpl (" << text() << ")");
    m_noteEditorPrivate.redoPageAction();
}

void NoteEditorContentEditUndoCommand::undoImpl()
{
    QNDEBUG("NoteEditorContentEditUndoCommand::undoImpl (" << text() << ")");
    m_noteEditorPrivate.undoPageAction();
    m_noteEditorPrivate.setNoteResources(m_resources);
}

void NoteEditorContentEditUndoCommand::init()
{
    QUndoCommand::setText(QObject::tr("Note text edit"));
}

} // namespace qute_note
