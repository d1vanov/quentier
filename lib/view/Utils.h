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

#pragma once

#include <QString>
#include <QVariant>

#include <functional>

class QDebug;
class QMenu;

namespace quentier {

enum class ActionState
{
    Enabled,
    Disabled
};

QDebug & operator<<(QDebug & dbg, ActionState state);

void addContextMenuAction(
    QString name, QMenu & menu, QObject * target,
    std::function<void()> callback, QVariant data, ActionState state);

} // namespace quentier
