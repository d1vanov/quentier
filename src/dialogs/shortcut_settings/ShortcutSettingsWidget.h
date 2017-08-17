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
#include <QWidget>
#include <QList>
#include <QPointer>

QT_FORWARD_DECLARE_CLASS(QTreeWidgetItem)
QT_FORWARD_DECLARE_CLASS(QMenu)

namespace Ui {
class ShortcutSettingsWidget;
}

namespace quentier {

QT_FORWARD_DECLARE_CLASS(ShortcutManager)

struct ShortcutItem
{
    ShortcutItem() :
        m_actionKey(-1),
        m_nonStandardActionKey(),
        m_keySequence(),
        m_pTreeWidgetItem(Q_NULLPTR)
    {}

    int                 m_actionKey;
    QString             m_nonStandardActionKey;
    QKeySequence        m_keySequence;
    QTreeWidgetItem *   m_pTreeWidgetItem;
};

class ShortcutSettingsWidget: public QWidget
{
    Q_OBJECT
public:
    ShortcutSettingsWidget(QWidget * parent = Q_NULLPTR);
    virtual ~ShortcutSettingsWidget();

    void initialize(ShortcutManager * pShortcutManager,
                    const QList<QMenu*> & menusWithActions);
    void apply();
    
private Q_SLOTS:
    void onCurrentActionChanged(QTreeWidgetItem * pCurrentItem);
    void resetToDefault();
    void resetAll();
    void onActionFilterChanged(const QString & filter);
    void showConflicts(const QString &);
    void setKeySequence(const QKeySequence & key);

private:
    bool validateShortcutEdit() const;
    bool markCollisions(ShortcutItem * pItem);

    void clear();

private:
    Ui::ShortcutSettingsWidget *    m_pUi;
    QPointer<ShortcutManager>       m_pShortcutManager;
    QList<ShortcutItem*>            m_shortcutItems;
};

} // namespace quentier

#endif // QUENTIER_DIALOGS_SHORTCUT_SETTINGS_WIDGET_H
