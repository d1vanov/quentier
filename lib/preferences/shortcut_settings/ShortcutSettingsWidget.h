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

/**
 * The contents of this file were heavily inspired by the source code of
 * Qt Creator, a Qt and C/C++ IDE by The Qt Company Ltd., 2016. That source code
 * is licensed under GNU GPL v.3 license with two permissive exceptions although
 * they don't apply to this derived work.
 */

#ifndef QUENTIER_DIALOGS_SHORTCUT_SETTINGS_WIDGET_H
#define QUENTIER_DIALOGS_SHORTCUT_SETTINGS_WIDGET_H

#include <quentier/types/Account.h>
#include <quentier/utility/Printable.h>

#include <QHash>
#include <QList>
#include <QPointer>
#include <QWidget>

class QTreeWidgetItem;

namespace Ui {
class ShortcutSettingsWidget;
}

namespace quentier {

class ActionsInfo;
class ShortcutManager;

class ShortcutItem final : public Printable
{
public:
    QTextStream & print(QTextStream & strm) const override;

    int m_actionKey = -1;
    QString m_nonStandardActionKey;
    QString m_actionName;
    QString m_context;
    QString m_category;
    bool m_isModified = false;
    QKeySequence m_keySequence;
    QTreeWidgetItem * m_pTreeWidgetItem = nullptr;
};

class ShortcutSettingsWidget : public QWidget
{
    Q_OBJECT
public:
    ShortcutSettingsWidget(QWidget * parent = nullptr);
    ~ShortcutSettingsWidget() override;

    void initialize(
        const Account & currentAccount, const ActionsInfo & actionsInfo,
        ShortcutManager * pShortcutManager);

private Q_SLOTS:
    void onCurrentActionChanged(
        QTreeWidgetItem * pCurrentItem, QTreeWidgetItem * pPreviousItem);

    void resetToDefault();
    void resetAll();
    void onActionFilterChanged(const QString & filter);
    void showConflicts(const QString &);
    void setCurrentItemKeySequence(const QKeySequence & key);
    void onCurrentItemKeySequenceEdited();

private:
    [[nodiscard]] bool markCollisions(ShortcutItem & item);
    void setModified(QTreeWidgetItem & item, bool modified);

    [[nodiscard]] bool filterColumn(
        const QString & filter, QTreeWidgetItem & item, int column);

    [[nodiscard]] bool filterItem(
        const QString & filter, QTreeWidgetItem & item);

    void clear();

    void warnOfConflicts();

    [[nodiscard]] ShortcutItem * shortcutItemFromTreeItem(
        QTreeWidgetItem * pItem) const;

private:
    Ui::ShortcutSettingsWidget * m_pUi;
    QPointer<ShortcutManager> m_pShortcutManager;
    Account m_currentAccount;
    QList<ShortcutItem *> m_shortcutItems;
};

} // namespace quentier

#endif // QUENTIER_DIALOGS_SHORTCUT_SETTINGS_WIDGET_H
