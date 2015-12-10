#ifndef __LIB_QUTE_NOTE__NOTE_EDITOR__I_UNDOABLE_ACTION_DELEGATE_H
#define __LIB_QUTE_NOTE__NOTE_EDITOR__I_UNDOABLE_ACTION_DELEGATE_H

#include <qute_note/utility/Qt4Helper.h>
#include <QObject>

namespace qute_note {

/**
 * @brief The IUndoableActionDelegate class is the base class for all note editor's undoable action delegates.
 *
 * A lot of things done with note editor may seem easy at first but the ability to wrap them around the undo stack
 * properly requires pretty complicated logics and the sequence of asynchronous callbacks to be completed. For localizing
 * the complexity such actions and wrapping them around the undo stack happens mostly in dedicated classes - delegates.
 * This base class exists in order to provide some means for the serialization of such actions and undo/redo actions. For example,
 * executing "Undo" while some undoable action delegate is executing is a bad idea which can break the undo stack.
 * In order to prevent that the note editor should postpone the undo action if some other action is in the making right now.
 * The signal this class emits serves exactly the purpose of letting the note editor know that the undoable action delegate
 * has now finished.
 */
class IUndoableActionDelegate: public QObject
{
    Q_OBJECT
public:
    explicit IUndoableActionDelegate(QObject * parent = Q_NULLPTR) : QObject(parent) {}
    virtual ~IUndoableActionDelegate() { emit undoableActionReady(); }

Q_SIGNALS:
    /**
     * @brief undoableActionReady signal is emitted when the delegate executing some action and wrapping it around the undo stack
     * has finished its work so if there are pending undo action calls, they can be executed now
     */
    void undoableActionReady();
};

} // namespace qute_note

#endif // __LIB_QUTE_NOTE__NOTE_EDITOR__I_UNDOABLE_ACTION_DELEGATE_H
