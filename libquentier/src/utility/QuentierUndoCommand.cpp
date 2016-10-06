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

#include <quentier/utility/QuentierUndoCommand.h>
#include <quentier/logging/QuentierLogger.h>

namespace quentier {

QuentierUndoCommand::QuentierUndoCommand(QUndoCommand * parent) :
    QObject(Q_NULLPTR),
    QUndoCommand(parent),
    m_onceUndoExecuted(false)
{}

QuentierUndoCommand::QuentierUndoCommand(const QString & text, QUndoCommand * parent) :
    QObject(Q_NULLPTR),
    QUndoCommand(text, parent),
    m_onceUndoExecuted(false)
{}

QuentierUndoCommand::~QuentierUndoCommand()
{}

void QuentierUndoCommand::undo()
{
    QNTRACE(QStringLiteral("QuentierUndoCommand::undo"));
    m_onceUndoExecuted = true;
    undoImpl();
}

void QuentierUndoCommand::redo()
{
    QNTRACE(QStringLiteral("QuentierUndoCommand::redo"));

    if (Q_UNLIKELY(!m_onceUndoExecuted)) {
        QNTRACE(QStringLiteral("Ignoring the attempt to execute redo for command ") << text()
                << QStringLiteral(" as there was no previous undo"));
        return;
    }

    redoImpl();
}

} // namespace quentier
