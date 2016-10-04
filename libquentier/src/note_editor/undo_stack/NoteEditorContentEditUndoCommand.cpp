#include "NoteEditorContentEditUndoCommand.h"
#include "../NoteEditor_p.h"
#include <quentier/logging/QuentierLogger.h>

namespace quentier {

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
    QNDEBUG(QStringLiteral("NoteEditorContentEditUndoCommand::redoImpl (") << text() << QStringLiteral(")"));
    m_noteEditorPrivate.redoPageAction();
}

void NoteEditorContentEditUndoCommand::undoImpl()
{
    QNDEBUG(QStringLiteral("NoteEditorContentEditUndoCommand::undoImpl (") << text() << QStringLiteral(")"));
    m_noteEditorPrivate.undoPageAction();
    m_noteEditorPrivate.setNoteResources(m_resources);
}

void NoteEditorContentEditUndoCommand::init()
{
    setText(tr("Note text edit"));
}

} // namespace quentier
