#include "AddHyperlinkUndoCommand.h"
#include <qute_note/logging/QuteNoteLogger.h>

namespace qute_note {

AddHyperlinkUndoCommand::AddHyperlinkUndoCommand(NoteEditorPrivate & noteEditor, QUndoCommand * parent) :
    INoteEditorUndoCommand(noteEditor, parent)
{}

AddHyperlinkUndoCommand::AddHyperlinkUndoCommand(NoteEditorPrivate & noteEditor, const QString & text, QUndoCommand * parent) :
    INoteEditorUndoCommand(noteEditor, text, parent)
{}

AddHyperlinkUndoCommand::~AddHyperlinkUndoCommand()
{}

void AddHyperlinkUndoCommand::redoImpl()
{
    QNDEBUG("AddHyperlinkUndoCommand::redoImpl");
    // TODO: implement
}

void AddHyperlinkUndoCommand::undoImpl()
{
    QNDEBUG("AddHyperlinkUndoCommand::undoImpl");
    // TODO: implement
}

void AddHyperlinkUndoCommand::setHtmlWithHyperlink(const QString & html)
{
    QNDEBUG("AddHyperlinkUndoCommand::setHtmlWithHyperlink");
    Q_UNUSED(html)
    // TODO: implement
}

} // namespace qute_note
