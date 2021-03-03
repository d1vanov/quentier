/*
 * Copyright 2017-2021 Dmitry Ivanov
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

#ifndef QUENTIER_LIB_UTILITY_ACTIONS_INFO_H
#define QUENTIER_LIB_UTILITY_ACTIONS_INFO_H

#include <quentier/utility/Printable.h>

#include <QHash>
#include <QKeySequence>
#include <QList>

class QAction;
class QMenu;

namespace quentier {

struct ActionKeyWithContext
{
    int m_key = -1;
    QString m_context;
};

struct ActionNonStandardKeyWithContext
{
    QString m_nonStandardActionKey;
    QString m_context;
};

class ActionsInfo
{
public:
    class ActionInfo : public Printable
    {
    public:
        QTextStream & print(QTextStream & strm) const override;

        [[nodiscard]] bool isEmpty() const;

        QString m_name;
        QString m_localizedName;
        QString m_context;
        QString m_category;
        int m_shortcutKey = -1;
        QString m_nonStandardShortcutKey;
        QKeySequence m_shortcut;
    };

public:
    ActionsInfo(const QList<QMenu *> & menus);

    [[nodiscard]] ActionInfo findActionInfo(
        const QString & actionName, const QString & context) const;

    class Iterator
    {
    public:
        Iterator(
            const int menuIndex, const int actionIndex,
            const ActionsInfo & actionsInfo);

        [[nodiscard]] ActionInfo actionInfo() const;

        [[nodiscard]] bool operator==(const Iterator & other) const;
        [[nodiscard]] bool operator!=(const Iterator & other) const;

        Iterator operator++();
        Iterator & operator++(int);

    private:
        void increment();

    private:
        const ActionsInfo & m_actionsInfo;
        int m_menuIndex;
        int m_actionIndex;
    };

    friend class Iterator;

    [[nodiscard]] Iterator begin() const;
    [[nodiscard]] Iterator end() const;

private:
    [[nodiscard]] ActionInfo fromAction(
        const QAction * pAction, const QString & category) const;

private:
    Q_DISABLE_COPY(ActionsInfo)

private:
    QList<QMenu *> m_menus;
};

} // namespace quentier

Q_DECLARE_METATYPE(quentier::ActionKeyWithContext)
Q_DECLARE_METATYPE(quentier::ActionNonStandardKeyWithContext)

#endif // QUENTIER_LIB_UTILITY_ACTIONS_INFO_H
