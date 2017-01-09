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

#ifndef LIB_QUENTIER_UTILITY_SHORTCUT_MANAGER_PRIVATE_H
#define LIB_QUENTIER_UTILITY_SHORTCUT_MANAGER_PRIVATE_H

#include <quentier/utility/Macros.h>
#include <QObject>
#include <QKeySequence>

namespace quentier {

QT_FORWARD_DECLARE_CLASS(ShortcutManager)

class ShortcutManagerPrivate: public QObject
{
    Q_OBJECT
public:
    explicit ShortcutManagerPrivate(ShortcutManager & shortcutManager);

    QKeySequence shortcut(const int key, const QString & context) const;
    QKeySequence shortcut(const QString & nonStandardKey, const QString & context) const;

Q_SIGNALS:
    void shortcutChanged(int key, QKeySequence shortcut, QString context);
    void nonStandardShortcutChanged(QString nonStandardKey, QKeySequence shortcut, QString context);

public Q_SLOTS:
    void setUserShortcut(int key, QKeySequence shortcut, QString context);
    void setNonStandardUserShortcut(QString nonStandardKey, QKeySequence shortcut, QString context);

    void setDefaultShortcut(int key, QKeySequence shortcut, QString context);
    void setNonStandardDefaultShortcut(QString nonStandardKey, QKeySequence shortcut, QString context);

private:
    QKeySequence defaultShortcut(const int key, const QString & keyString, const QString & context) const;
    QKeySequence defaultShortcut(const QString & nonStandardKey, const QString & context) const;

    QString keyToString(const int key) const;
    QString shortcutGroupString(const QString & context, const bool defaultShortcut, const bool nonStandardShortcut) const;

private:
    ShortcutManager * const q_ptr;
    Q_DECLARE_PUBLIC(ShortcutManager)
};

} // namespace quentier

#endif // LIB_QUENTIER_UTILITY_SHORTCUT_MANAGER_PRIVATE_H
