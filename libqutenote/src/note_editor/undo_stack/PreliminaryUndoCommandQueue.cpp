#include "PreliminaryUndoCommandQueue.h"
#include <qute_note/logging/QuteNoteLogger.h>
#include <qute_note/utility/QuteNoteCheckPtr.h>
#include <QUndoStack>

namespace qute_note {

PreliminaryUndoCommandQueue::PreliminaryUndoCommandQueue(QUndoStack * stack, QObject * parent) :
    QObject(parent),
    m_pStack(stack),
    m_queue()
{}

PreliminaryUndoCommandQueue::~PreliminaryUndoCommandQueue()
{
    // Need to remove all commands still left in the queue
    // as this object owns each of them until it is passed on to the stack
    while(!m_queue.empty()) {
        INoteEditorUndoCommand * command = m_queue.dequeue();
        delete command;
    }
}

void PreliminaryUndoCommandQueue::push(INoteEditorUndoCommand * command)
{
    QUTE_NOTE_CHECK_PTR(command, "null pointer to note editor undo command passed "
                        "to preliminary undo command queue");

    QNDEBUG("PreliminaryUndoCommandQueue::push: " << command->text());

    m_queue.enqueue(command);
    checkState();
}

void PreliminaryUndoCommandQueue::checkState()
{
    QNDEBUG("PreliminaryUndoCommandQueue::checkState");

    // Something has changed - either the new command was pushed
    // or the state of some command changed i.e. it has become ready
    // for pushing onto the stack; check which commands from the beginning
    // of the queue are ready and pass them onto the stack until the first
    // not yet ready command is encountered
    while(!m_queue.isEmpty())
    {
        INoteEditorUndoCommand * command = m_queue.head();
        if (!command->ready()) {
            break;
        }

        command = m_queue.dequeue();
        m_pStack->push(command);
        QNTRACE("Pushed command \"" << command->text() << "\" onto undo stack");
    }

    QNTRACE("PreliminaryUndoCommandQueue::checkState done");
}

} // namespace qute_note
