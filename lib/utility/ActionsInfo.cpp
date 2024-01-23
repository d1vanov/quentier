/*
 * Copyright 2017-2024 Dmitry Ivanov
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
#include <quentier/utility/Compat.h>

#include <QMenu>

#include <algorithm>
#include <cmath>
#include <utility>

namespace quentier {

ActionsInfo::ActionsInfo(QList<QMenu *> menus) : m_menus{std::move(menus)} {}

ActionsInfo::ActionInfo ActionsInfo::findActionInfo(
    const QString & actionName, const QString & context) const
{
    QNDEBUG(
        "utility",
        "ActionsInfo::findActionInfo: action name = "
            << actionName << ", context = " << context);

    // First look for menu whose name matches the "context"
    const QMenu * targetMenu = nullptr;
    for (const auto * menu: std::as_const(m_menus)) {
        if (Q_UNLIKELY(!menu)) {
            continue;
        }

        if (menu->title() == context) {
            targetMenu = menu;
            break;
        }
    }

    if (!targetMenu) {
        return {};
    }

    // Now look for action with matching text
    const auto actions = targetMenu->actions();
    for (auto * action: std::as_const(actions)) {
        if (Q_UNLIKELY(!action)) {
            continue;
        }

        if (action->text() != actionName) {
            continue;
        }

        auto info = fromAction(action, targetMenu->title());
        QNDEBUG("utility", "Found action info: " << info);
        return info;
    }

    return ActionInfo();
}

ActionsInfo::ActionInfo ActionsInfo::fromAction(
    const QAction * action, const QString & category) const
{
    if (Q_UNLIKELY(!action)) {
        return ActionInfo();
    }

    ActionInfo info;
    info.m_name = action->objectName();
    info.m_localizedName = action->text();
    info.m_localizedName.remove(QChar::fromLatin1('&'), Qt::CaseInsensitive);

    info.m_category = category;
    info.m_category.remove(QChar::fromLatin1('&'), Qt::CaseInsensitive);

    auto actionData = action->data();

    if (actionData.canConvert<ActionKeyWithContext>()) {
        auto keyWithContext = actionData.value<ActionKeyWithContext>();
        info.m_shortcutKey = keyWithContext.m_key;
        info.m_context = keyWithContext.m_context;
    }
    else if (actionData.canConvert<ActionNonStandardKeyWithContext>()) {
        ActionNonStandardKeyWithContext nonStandardKeyWithContext =
            actionData.value<ActionNonStandardKeyWithContext>();

        info.m_nonStandardShortcutKey =
            nonStandardKeyWithContext.m_nonStandardActionKey;

        info.m_context = nonStandardKeyWithContext.m_context;
    }

    info.m_shortcut = action->shortcut();
    return info;
}

ActionsInfo::Iterator::Iterator(
    const int menuIndex, const int actionIndex,
    const ActionsInfo & actionsInfo) :
    m_actionsInfo(actionsInfo),
    m_menuIndex(menuIndex), m_actionIndex(actionIndex)
{}

ActionsInfo::ActionInfo ActionsInfo::Iterator::actionInfo() const
{
    if (Q_UNLIKELY(m_menuIndex >= m_actionsInfo.m_menus.size())) {
        return ActionInfo();
    }

    const auto * menu = m_actionsInfo.m_menus.at(m_menuIndex);
    if (Q_UNLIKELY(!menu)) {
        return ActionInfo();
    }

    const auto actions = menu->actions();
    if (Q_UNLIKELY(m_actionIndex >= actions.size())) {
        return ActionInfo();
    }

    return m_actionsInfo.fromAction(actions.at(m_actionIndex), menu->title());
}

bool ActionsInfo::Iterator::operator==(const Iterator & other) const noexcept
{
    return &m_actionsInfo == &other.m_actionsInfo &&
        m_menuIndex == other.m_menuIndex &&
        m_actionIndex == other.m_actionIndex;
}

bool ActionsInfo::Iterator::operator!=(const Iterator & other) const noexcept
{
    return !operator==(other);
}

ActionsInfo::Iterator ActionsInfo::Iterator::operator++()
{
    Iterator it(*this);
    increment();
    return it;
}

ActionsInfo::Iterator & ActionsInfo::Iterator::operator++(int)
{
    increment();
    return *this;
}

void ActionsInfo::Iterator::increment()
{
    if (Q_UNLIKELY(m_menuIndex >= m_actionsInfo.m_menus.size())) {
        return;
    }

    const auto * menu = m_actionsInfo.m_menus.at(m_menuIndex);
    if (Q_UNLIKELY(!menu)) {
        return;
    }

    const auto actions = menu->actions();
    if (Q_UNLIKELY(m_actionIndex >= actions.size())) {
        return;
    }

    ++m_actionIndex;

    if (m_actionIndex == actions.size()) {
        m_actionIndex = 0;
        ++m_menuIndex;
    }
}

ActionsInfo::Iterator ActionsInfo::begin() const
{
    return Iterator{0, 0, *this};
}

ActionsInfo::Iterator ActionsInfo::end() const
{
    return Iterator{m_menus.size(), 0, *this};
}

QTextStream & ActionsInfo::ActionInfo::print(QTextStream & strm) const
{
    strm << "ActionInfo: name = " << m_name
         << ", localized name = " << m_localizedName
         << ", context = " << m_context << ", shortcut key = " << m_shortcutKey
         << ", non-standard shortcut key = " << m_nonStandardShortcutKey
         << ", shortcut = " << m_shortcut.toString(QKeySequence::PortableText);

    return strm;
}

bool ActionsInfo::ActionInfo::isEmpty() const
{
    return m_name.isEmpty() || m_context.isEmpty() ||
        (m_shortcutKey < 0 && m_nonStandardShortcutKey.isEmpty());
}

} // namespace quentier
