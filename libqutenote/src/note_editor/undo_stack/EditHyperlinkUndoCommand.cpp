#include "EditHyperlinkUndoCommand.h"
#include "../NoteEditor_p.h"
#include <qute_note/logging/QuteNoteLogger.h>

namespace qute_note {

EditHyperlinkUndoCommand::EditHyperlinkUndoCommand(const QString & enmlBefore,
                                                   const QString & enmlAfter,
                                                   NoteEditorPrivate & noteEditorPrivate,
                                                   QUndoCommand * parent) :
    INoteEditorUndoCommand(noteEditorPrivate, parent),
    m_enmlBefore(enmlBefore),
    m_enmlAfter(enmlAfter)
{}

EditHyperlinkUndoCommand::EditHyperlinkUndoCommand(const QString & enmlBefore,
                                                   const QString & enmlAfter,
                                                   NoteEditorPrivate & noteEditorPrivate,
                                                   const QString & text, QUndoCommand * parent) :
    INoteEditorUndoCommand(noteEditorPrivate, text, parent),
    m_enmlBefore(enmlBefore),
    m_enmlAfter(enmlAfter)
{}

EditHyperlinkUndoCommand::~EditHyperlinkUndoCommand()
{}

void EditHyperlinkUndoCommand::redo()
{
    QNDEBUG("EditHyperlinkUndoCommand::redo");
    m_noteEditorPrivate.updateEnml(m_enmlAfter);
}

void EditHyperlinkUndoCommand::undo()
{
    QNDEBUG("EditHyperlinkUndoCommand::undo");
    m_noteEditorPrivate.updateEnml(m_enmlBefore);
}

} // namespace qute_note
