/*
 * Copyright 2024 Dmitry Ivanov
 *
 * This file is part of Quentier.
 *
 * Quentier is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * Quentier is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Quentier. If not, see <http://www.gnu.org/licenses/>.
 */

#include "Utils.h"

#include <QAction>
#include <QDebug>
#include <QMenu>
#include <QtGlobal>

namespace quentier {

void addContextMenuAction(
    QString name, QMenu & menu, QObject * target,
    std::function<void()> callback, QVariant data, const ActionState state)
{
    Q_ASSERT(callback);
    QAction * action = new QAction(std::move(name), &menu);
    action->setData(std::move(data));
    action->setEnabled(state == ActionState::Enabled);
    QObject::connect(action, &QAction::triggered, target, callback);
    menu.addAction(action);
}

QDebug & operator<<(QDebug & dbg, const ActionState state)
{
    switch (state) {
    case ActionState::Enabled:
        dbg << "enabled";
        break;
    case ActionState::Disabled:
        dbg << "disabled";
        break;
    }

    return dbg;
}

} // namespace quentier
