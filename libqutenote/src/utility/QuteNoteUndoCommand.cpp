#include <qute_note/utility/QuteNoteUndoCommand.h>
#include <qute_note/logging/QuteNoteLogger.h>

namespace qute_note {

QuteNoteUndoCommand::QuteNoteUndoCommand(QUndoCommand * parent) :
    QUndoCommand(parent),
    m_onceUndoExecuted(false)
{}

QuteNoteUndoCommand::QuteNoteUndoCommand(const QString & text, QUndoCommand * parent) :
    QUndoCommand(text, parent),
    m_onceUndoExecuted(false)
{}

QuteNoteUndoCommand::~QuteNoteUndoCommand()
{}

void QuteNoteUndoCommand::undo()
{
    m_onceUndoExecuted = true;
    undoImpl();
}

void QuteNoteUndoCommand::redo()
{
    if (Q_UNLIKELY(!m_onceUndoExecuted)) {
        QNTRACE("Ignoring the attempt to execute redo for command" << text()
                << "as there was no previous undo");
        return;
    }

    redoImpl();
}

} // namespace qute_note
