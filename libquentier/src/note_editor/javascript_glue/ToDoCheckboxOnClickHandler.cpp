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

#include "ToDoCheckboxOnClickHandler.h"
#include <quentier/logging/QuentierLogger.h>

namespace quentier {

ToDoCheckboxOnClickHandler::ToDoCheckboxOnClickHandler(QObject * parent) :
    QObject(parent)
{}

void ToDoCheckboxOnClickHandler::onToDoCheckboxClicked(QString enToDoCheckboxId)
{
    QNDEBUG(QStringLiteral("ToDoCheckboxOnClickHandler::onToDoCheckboxClicked: ") << enToDoCheckboxId);

    bool conversionResult = false;
    quint64 id = enToDoCheckboxId.toULongLong(&conversionResult);
    if (Q_UNLIKELY(!conversionResult)) {
        QNLocalizedString error = QT_TR_NOOP("error handling todo checkbox click event: can't convert id from string to number");
        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    emit toDoCheckboxClicked(id);
}

} // namespace quentier
