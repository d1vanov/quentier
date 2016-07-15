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

#include <quentier/utility/ShortcutManager.h>
#include "ShortcutManager_p.h"

namespace quentier {

ShortcutManager::ShortcutManager(QObject * parent) :
    QObject(parent),
    d_ptr(new ShortcutManagerPrivate(*this))
{}

QKeySequence ShortcutManager::shortcut(const int key, const QString & context) const
{
    Q_D(const ShortcutManager);
    return d->shortcut(key, context);
}

QKeySequence ShortcutManager::shortcut(const QString & nonStandardKey, const QString & context) const
{
    Q_D(const ShortcutManager);
    return d->shortcut(nonStandardKey, context);
}

void ShortcutManager::setUserShortcut(int key, QKeySequence shortcut, QString context)
{
    Q_D(ShortcutManager);
    d->setUserShortcut(key, shortcut, context);
}

void ShortcutManager::setNonStandardUserShortcut(QString nonStandardKey, QKeySequence shortcut, QString context)
{
    Q_D(ShortcutManager);
    d->setNonStandardUserShortcut(nonStandardKey, shortcut, context);
}

void ShortcutManager::setDefaultShortcut(int key, QKeySequence shortcut, QString context)
{
    Q_D(ShortcutManager);
    d->setDefaultShortcut(key, shortcut, context);
}

void ShortcutManager::setNonStandardDefaultShortcut(QString nonStandardKey, QKeySequence shortcut, QString context)
{
    Q_D(ShortcutManager);
    d->setNonStandardDefaultShortcut(nonStandardKey, shortcut, context);
}

} // namespace quentier
