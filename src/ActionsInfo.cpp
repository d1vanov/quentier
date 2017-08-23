/*
 * Copyright 2017 Dmitry Ivanov
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

#include "ActionsInfo.h"
#include <quentier/logging/QuentierLogger.h>
#include <QMenu>
#include <algorithm>
#include <cmath>

namespace quentier {

ActionsInfo::ActionsInfo(const QList<QMenu*> & menus) :
    m_menus(menus)
{}

const ActionsInfo::ActionInfo ActionsInfo::findActionInfo(const QString & actionName, const QString & context) const
{
    QNDEBUG(QStringLiteral("ActionsInfo::findActionInfo: action name = ") << actionName
            << QStringLiteral(", context = ") << context);

    // First look for menu whose name matches the "context"
    const QMenu * pTargetMenu = Q_NULLPTR;
    for(auto it = m_menus.constBegin(), end = m_menus.constEnd(); it != end; ++it)
    {
        const QMenu * pMenu = *it;
        if (Q_UNLIKELY(!pMenu)) {
            continue;
        }

        if (pMenu->title() == context) {
            pTargetMenu = pMenu;
            break;
        }
    }

    if (!pTargetMenu) {
        return ActionInfo();
    }

    // Now look for action with matching text
    const QList<QAction*> actions = pTargetMenu->actions();
    for(auto it = actions.constBegin(), end = actions.constEnd(); it != end; ++it)
    {
        QAction * pAction = *it;
        if (Q_UNLIKELY(!pAction)) {
            continue;
        }

        if (pAction->text() != actionName) {
            continue;
        }

        ActionInfo info = fromAction(pAction);
        QNDEBUG(QStringLiteral("Found action info: ") << info);
        return info;
    }

    return ActionInfo();
}

ActionsInfo::Iterator ActionsInfo::begin() const
{
    return Iterator(0, 0);
}

ActionsInfo::Iterator ActionsInfo::end() const
{
    return Iterator(std::max(0, m_menus.size() - 1), 0);
}

ActionsInfo::ActionInfo ActionsInfo::fromAction(const QAction * pAction) const
{
    if (Q_UNLIKELY(!pAction)) {
        return ActionInfo();
    }

    ActionInfo info;
    info.m_name = pAction->objectName();
    info.m_localizedName = pAction->text();
    info.m_context = context;
    // TODO: somehow retrieve the shortcut standard or non-standard key
    info.m_shortcut = pAction->shortcut();
    return info;
}

ActionsInfo::Iterator::Iterator(const int menuIndex, const int actionIndex,
                                const ActionsInfo & actionsInfo) :
    m_actionsInfo(actionsInfo),
    m_menuIndex(menuIndex),
    m_actionIndex(actionIndex)
{}

const ActionsInfo::ActionInfo ActionsInfo::Iterator::actionInfo() const
{
    if (m_menuIndex >= m_actionsInfo.m_menus.size()) {
        return ActionInfo();
    }

    const QMenu * pMenu = m_actionsInfo.m_menus.at(m_menuIndex);
    if (Q_UNLIKELY(!pMenu)) {
        return ActionInfo();
    }

    const QList<QAction*> actions = pMenu->actions();
    if (m_actionIndex >= actions.size()) {
        return ActionInfo();
    }

    return m_actionsInfo.fromAction(actions.at(m_actionIndex));
}

} // namespace quentier
