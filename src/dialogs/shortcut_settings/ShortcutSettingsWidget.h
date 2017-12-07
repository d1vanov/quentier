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

/**
 * The contents of this file were heavily inspired by the source code of Qt Creator,
 * a Qt and C/C++ IDE by The Qt Company Ltd., 2016. That source code is licensed under GNU GPL v.3
 * license with two permissive exceptions although they don't apply to this derived work.
 */

#ifndef QUENTIER_DIALOGS_SHORTCUT_SETTINGS_WIDGET_H
#define QUENTIER_DIALOGS_SHORTCUT_SETTINGS_WIDGET_H

#include <quentier/utility/Macros.h>
#include <quentier/utility/Printable.h>
#include <quentier/types/Account.h>
#include <QWidget>
#include <QList>
#include <QPointer>
#include <QHash>

QT_FORWARD_DECLARE_CLASS(QTreeWidgetItem)

namespace Ui {
class ShortcutSettingsWidget;
}

namespace quentier {

QT_FORWARD_DECLARE_CLASS(ActionsInfo)
QT_FORWARD_DECLARE_CLASS(ShortcutManager)

class ShortcutItem: public Printable
{
public:
    ShortcutItem() :
        m_actionKey(-1),
        m_nonStandardActionKey(),
        m_actionName(),
        m_context(),
        m_category(),
        m_isModified(false),
        m_keySequence(),
        m_pTreeWidgetItem(Q_NULLPTR)
    {}

    virtual QTextStream & print(QTextStream & strm) const Q_DECL_OVERRIDE;

    int                 m_actionKey;
    QString             m_nonStandardActionKey;
    QString             m_actionName;
    QString             m_context;
    QString             m_category;
    bool                m_isModified;
    QKeySequence        m_keySequence;
    QTreeWidgetItem *   m_pTreeWidgetItem;
};

class ShortcutSettingsWidget: public QWidget
{
    Q_OBJECT
public:
    ShortcutSettingsWidget(QWidget * parent = Q_NULLPTR);
    virtual ~ShortcutSettingsWidget();

    void initialize(const Account & currentAccount, const ActionsInfo & actionsInfo,
                    ShortcutManager * pShortcutManager);

private Q_SLOTS:
    void onCurrentActionChanged(QTreeWidgetItem * pCurrentItem,
                                QTreeWidgetItem * pPreviousItem);
    void resetToDefault();
    void resetAll();
    void onActionFilterChanged(const QString & filter);
    void showConflicts(const QString &);
    void setCurrentItemKeySequence(const QKeySequence & key);
    void onCurrentItemKeySequenceEdited();

private:

    bool markCollisions(ShortcutItem & item);
    void setModified(QTreeWidgetItem & item, bool modified);

    bool filterColumn(const QString & filter, QTreeWidgetItem & item, int column);
    bool filterItem(const QString & filter, QTreeWidgetItem & item);

    void clear();

    void warnOfConflicts();

    ShortcutItem * shortcutItemFromTreeItem(QTreeWidgetItem * pItem) const;

private:
    Ui::ShortcutSettingsWidget *    m_pUi;
    QPointer<ShortcutManager>       m_pShortcutManager;
    Account                         m_currentAccount;
    QList<ShortcutItem*>            m_shortcutItems;
};

} // namespace quentier

#endif // QUENTIER_DIALOGS_SHORTCUT_SETTINGS_WIDGET_H
