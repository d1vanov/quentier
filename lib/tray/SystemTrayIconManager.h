/*
 * Copyright 2017-2020 Dmitry Ivanov
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

#ifndef QUENTIER_LIB_TRAY_SYSTEM_TRAY_ICON_MANAGER_H
#define QUENTIER_LIB_TRAY_SYSTEM_TRAY_ICON_MANAGER_H

#include <quentier/types/Account.h>
#include <quentier/types/ErrorString.h>

#include <QObject>
#include <QSystemTrayIcon>

QT_FORWARD_DECLARE_CLASS(QMenu)
QT_FORWARD_DECLARE_CLASS(QActionGroup)

namespace quentier {

QT_FORWARD_DECLARE_CLASS(AccountManager)

class SystemTrayIconManager final : public QObject
{
    Q_OBJECT
public:
    explicit SystemTrayIconManager(
        AccountManager & accountManager, QObject * parent = nullptr);

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

    /**
     * @brief setPreferenceCloseToSystemTray
     * Set user preference about closing to tray to given value.
     */
    void setPreferenceCloseToSystemTray(bool value) const;

    enum TrayAction
    {
        TrayActionDoNothing = 0,
        TrayActionShowHide,
        TrayActionNewTextNote,
        TrayActionShowContextMenu
    };

    TrayAction singleClickTrayAction() const;
    TrayAction middleClickTrayAction() const;
    TrayAction doubleClickTrayAction() const;

Q_SIGNALS:
    void notifyError(ErrorString errorDescription);

    void newTextNoteAdditionRequested();
    void quitRequested();

    void accountSwitchRequested(Account account);

    // Signals which SystemTrayIconManager would emit to notify the main window
    // that its showing or hiding was requested from system tray; the main
    // window widget must connect its slots to these signals
    void showRequested();
    void hideRequested();

    // private signals
    void switchAccount(Account account);

public Q_SLOTS:
    // Slots for MainWindow signals
    void onMainWindowShown();
    void onMainWindowHidden();

private Q_SLOTS:
    void onSystemTrayIconActivated(QSystemTrayIcon::ActivationReason reason);

    // Slots for AccountManager signals
    void onAccountSwitched(Account account);
    void onAccountUpdated(Account account);
    void onAccountAdded(Account account);
    void onAccountRemoved(Account account);

    // Slots for context menu actions
    void onNewTextNoteContextMenuAction();
    void onSwitchAccountContextMenuAction(bool checked);
    void onShowMainWindowContextMenuAction();
    void onHideMainWindowContextMenuAction();
    void onSwitchTrayIconContextMenuAction(bool checked);
    void onQuitContextMenuAction();

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

    /**
     * @brief getPreferenceCloseToSystemTray
     * @return          User preference about closing to tray to given value.
     *                  If no valid value is found, default value is returned.
     */
    bool getPreferenceCloseToSystemTray() const;

private:
    AccountManager & m_accountManager;
    QSystemTrayIcon * m_pSystemTrayIcon = nullptr;
    QMenu * m_pTrayIconContextMenu = nullptr;
    QMenu * m_pAccountsTrayIconSubMenu = nullptr;
    QMenu * m_pTrayIconKindSubMenu = nullptr;
    QActionGroup * m_pAvailableAccountsActionGroup = nullptr;
    QActionGroup * m_pTrayIconKindsActionGroup = nullptr;
};

} // namespace quentier

#endif // QUENTIER_LIB_TRAY_SYSTEM_TRAY_ICON_MANAGER_H
