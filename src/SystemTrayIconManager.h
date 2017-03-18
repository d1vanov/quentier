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

#ifndef QUENTIER_SYSTEM_TRAY_ICON_MANAGER_H
#define QUENTIER_SYSTEM_TRAY_ICON_MANAGER_H

#include <quentier/types/Account.h>
#include <quentier/types/ErrorString.h>
#include <quentier/utility/Macros.h>
#include <QObject>
#include <QPointer>
#include <QSystemTrayIcon>

QT_FORWARD_DECLARE_CLASS(QMenu)
QT_FORWARD_DECLARE_CLASS(QActionGroup)

namespace quentier {

QT_FORWARD_DECLARE_CLASS(AccountManager)

class SystemTrayIconManager : public QObject
{
    Q_OBJECT
public:
    explicit SystemTrayIconManager(AccountManager & accountManager,
                                   QObject * parent = Q_NULLPTR);

    /**
     * @brief isSystemTrayAvailable
     * @return either the output of QSystemTrayIcon::isSystemTrayAvailable
     * or the override of this check from the application settings
     */
    bool isSystemTrayAvailable() const;

    bool isShown() const;

    void show();
    void hide();

    bool shouldCloseToSystemTray() const;
    bool shouldMinimizeToSystemTray() const;
    bool shouldStartMinimizedToSystemTray() const;

Q_SIGNALS:
    void notifyError(ErrorString errorDescription);

    void newTextNoteAdditionRequested();
    void quitRequested();

    void accountSwitchRequested(Account account);

    // private signals
    void switchAccount(Account account);

private Q_SLOTS:
    // Slots for AccountManager signals
    void onAccountSwitched(Account account);
    void onAccountUpdated(Account account);
    void onAccountAdded(Account account);

    // Slots for context menu actions
    void onNewTextNoteContextMenuAction();
    void onSwitchAccountContextMenuAction(bool checked);
    void onShowMainWindowContextMenuAction();
    void onHideMainWindowContextMenuAction();
    void onSwitchTrayIconContextMenuAction(bool checked);
    void onQuitContextMenuAction();

    // Slots for MainWindow signals
    void onMainWindowShown();
    void onMainWindowHidden();

private:
    void createConnections();
    void setupSystemTrayIcon();

    void setupContextMenu();
    void setupAccountsSubMenu();
    void setupTrayIconKindSubMenu();
    void evaluateShowHideMenuActions();

    void onShowHideMainWindowContextMenuAction(const bool show);

    void persistTrayIconState();
    void restoreTrayIconState();

private:
    QPointer<AccountManager>    m_pAccountManager;
    QSystemTrayIcon *           m_pSystemTrayIcon;
    QMenu *                     m_pTrayIconContextMenu;
    QMenu *                     m_pAccountsTrayIconSubMenu;
    QMenu *                     m_pTrayIconKindSubMenu;
    QActionGroup *              m_pAvailableAccountsActionGroup;
    QActionGroup *              m_pTrayIconKindsActionGroup;
};

} // namespace quentier

#endif // QUENTIER_SYSTEM_TRAY_ICON_MANAGER_H
