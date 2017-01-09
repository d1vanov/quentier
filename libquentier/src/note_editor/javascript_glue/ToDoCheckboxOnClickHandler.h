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

#ifndef LIB_QUENTIER_NOTE_EDITOR_JAVASCRIPT_GLUE_TO_DO_CHECKBOX_ON_CLICK_HANDLER_H
#define LIB_QUENTIER_NOTE_EDITOR_JAVASCRIPT_GLUE_TO_DO_CHECKBOX_ON_CLICK_HANDLER_H

#include <quentier/utility/Macros.h>
#include <quentier/utility/QNLocalizedString.h>
#include <QObject>

namespace quentier {

class ToDoCheckboxOnClickHandler: public QObject
{
    Q_OBJECT
public:
    explicit ToDoCheckboxOnClickHandler(QObject * parent = Q_NULLPTR);

Q_SIGNALS:
    void toDoCheckboxClicked(quint64 enToDoCheckboxId);
    void notifyError(QNLocalizedString error);

public Q_SLOTS:
    void onToDoCheckboxClicked(QString enToDoCheckboxId);
};

} // namespace quentier

#endif // LIB_QUENTIER_NOTE_EDITOR_JAVASCRIPT_GLUE_TO_DO_CHECKBOX_ON_CLICK_HANDLER_H
