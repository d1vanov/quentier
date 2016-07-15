/*
 * Copyright 2016 Dmitry Ivanov
 *
 * This file is part of libquentier
 *
 * libquentier is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * libquentier is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with libquentier. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LIB_QUENTIER_UTILITY_QUENTIER_UNDO_COMMAND_H
#define LIB_QUENTIER_UTILITY_QUENTIER_UNDO_COMMAND_H

#include <quentier/utility/Qt4Helper.h>
#include <quentier/utility/QNLocalizedString.h>
#include <QObject>
#include <QUndoCommand>

namespace quentier {

/**
 * @brief The QuentierUndoCommand class has the sole purpose of working around
 * one quirky aspect of Qt's undo/redo framework: when you push QUndoCommand
 * to QUndoStack, it calls "redo" method of that command. This class offers
 * subclasses to implement their own methods for actual "undo" and "redo" commands while
 * ignoring the attempts to "redo" anything if there were no previous "undo" call
 * prior to that.
 *
 * The rationale behind the current behaviour seems to be the compliance with
 * "command pattern behaviour" when you create the command to execute the action
 * instead of just executing it immediately. This design is enforced by Qt's undo/redo
 * framework, there's no option to choose not to call "redo" when pushing to the stack.
 *
 * One thing which this design fails to see is the fact that the command may be
 * already executed externally by the moment the QUndoCommand
 * can be created. Suppose we can get the information about how to undo (and then again redo)
 * that command. We create the corresponding QUndoCommand, set up the stuff for its
 * undo/redo methods and push it to QUndoStack for future use... but at the same time
 * QUndoStack calls "redo" method of the command. Really not the behaviour you'd like to have.
 *
 * QuentierUndoCommand is also QObject, it is for error reporting via @link notifyError @endlink signal
 */
class QuentierUndoCommand: public QObject,
                           public QUndoCommand
{
    Q_OBJECT
public:
    QuentierUndoCommand(QUndoCommand * parent = Q_NULLPTR);
    QuentierUndoCommand(const QString & text, QUndoCommand * parent = Q_NULLPTR);
    virtual ~QuentierUndoCommand();

    virtual void undo() Q_DECL_OVERRIDE Q_DECL_FINAL;
    virtual void redo() Q_DECL_OVERRIDE Q_DECL_FINAL;

    bool onceUndoExecuted() const { return m_onceUndoExecuted; }

Q_SIGNALS:
    void notifyError(QNLocalizedString error);

protected:
    virtual void undoImpl() = 0;
    virtual void redoImpl() = 0;

private:
    bool    m_onceUndoExecuted;
};

} // namespace quentier

#endif // LIB_QUENTIER_UTILITY_QUENTIER_UNDO_COMMAND_H

