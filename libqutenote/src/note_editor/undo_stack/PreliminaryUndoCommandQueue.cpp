#include "PreliminaryUndoCommandQueue.h"
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
    m_queue.enqueue(command);
    checkState();
}

void PreliminaryUndoCommandQueue::checkState()
{
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
    }
}

} // namespace qute_note
