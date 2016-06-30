#include <quentier/utility/QuentierUndoCommand.h>
#include <quentier/logging/QuentierLogger.h>

namespace quentier {

QuentierUndoCommand::QuentierUndoCommand(QUndoCommand * parent) :
    QUndoCommand(parent),
    m_onceUndoExecuted(false)
{}

QuentierUndoCommand::QuentierUndoCommand(const QString & text, QUndoCommand * parent) :
    QUndoCommand(text, parent),
    m_onceUndoExecuted(false)
{}

QuentierUndoCommand::~QuentierUndoCommand()
{}

void QuentierUndoCommand::undo()
{
    QNTRACE("QuentierUndoCommand::undo");
    m_onceUndoExecuted = true;
    undoImpl();
}

void QuentierUndoCommand::redo()
{
    QNTRACE("QuentierUndoCommand::redo");

    if (Q_UNLIKELY(!m_onceUndoExecuted)) {
        QNTRACE("Ignoring the attempt to execute redo for command" << text()
                << "as there was no previous undo");
        return;
    }

    redoImpl();
}

} // namespace quentier
