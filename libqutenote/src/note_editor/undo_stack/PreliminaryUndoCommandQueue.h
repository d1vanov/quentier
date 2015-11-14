#ifndef __LIB_QUTE_NOTE__NOTE_EDITOR__UNDO_STACK__PRELIMINARY_UNDO_COMMAND_QUEUE_H
#define __LIB_QUTE_NOTE__NOTE_EDITOR__UNDO_STACK__PRELIMINARY_UNDO_COMMAND_QUEUE_H

#include "INoteEditorUndoCommand.h"
#include <qute_note/utility/Qt4Helper.h>
#include <QObject>
#include <QQueue>

QT_FORWARD_DECLARE_CLASS(QUndoStack)

namespace qute_note {

/**
 * @brief The PreliminaryUndoCommandQueue class encapsulates the queue
 * of various note editor's undo commands some of which are not yet ready
 * to be passed on to the generic undo stack because the internal state of such
 * commands is not yet completely specified due to, for example, waiting for
 * some asynchronous events to happen; it is not reasonable and not safe to try
 * executing undo/redo of these commands until they are ready for that. And that's
 * where this class comes into play: it holds the undo commands in a queue and
 * as they become ready, passes them to the undo stack where the commands are actually used
 * for undoing/redoing things.
 *
 * Note that even the commands which are ready right after the creation should still
 * be passed to the object instance of this class instead of QUndoStack to ensure
 * that all commands would only be pushed onto the undo stack in the correct order.
 */
class PreliminaryUndoCommandQueue: public QObject
{
    Q_OBJECT
public:
    explicit PreliminaryUndoCommandQueue(QUndoStack * stack, QObject * parent = Q_NULLPTR);
    virtual ~PreliminaryUndoCommandQueue();

    void push(INoteEditorUndoCommand * command);

public Q_SLOTS:
    void checkState();

private:
    QUndoStack * m_pStack;
    QQueue<INoteEditorUndoCommand*> m_queue;
};

} // namespace qute_note

#endif // __LIB_QUTE_NOTE__NOTE_EDITOR__UNDO_STACK__PRELIMINARY_UNDO_COMMAND_QUEUE_H
