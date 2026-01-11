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

#pragma once

#include <quentier/types/Account.h>
#include <quentier/types/ErrorString.h>

#include <QObject>
#include <QSystemTrayIcon>

class QMenu;
class QActionGroup;

namespace quentier {

class AccountManager;

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
    [[nodiscard]] bool isSystemTrayAvailable() const;

    [[nodiscard]] bool isShown() const;

    void show();
    void hide();

    [[nodiscard]] bool shouldCloseToSystemTray() const;
    [[nodiscard]] bool shouldMinimizeToSystemTray() const;
    [[nodiscard]] bool shouldStartMinimizedToSystemTray() const;

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

    [[nodiscard]] TrayAction singleClickTrayAction() const;
    [[nodiscard]] TrayAction middleClickTrayAction() const;
    [[nodiscard]] TrayAction doubleClickTrayAction() const;

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
    void onAccountSwitched(const Account & account);
    void onAccountUpdated(const Account & account);
    void onAccountAdded(const Account & account);
    void onAccountRemoved(const Account & account);

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
    [[nodiscard]] bool getPreferenceCloseToSystemTray() const;

private:
    AccountManager & m_accountManager;
    QSystemTrayIcon * m_systemTrayIcon = nullptr;
    QMenu * m_trayIconContextMenu = nullptr;
    QMenu * m_accountsTrayIconSubMenu = nullptr;
    QMenu * m_trayIconKindSubMenu = nullptr;
    QActionGroup * m_availableAccountsActionGroup = nullptr;
    QActionGroup * m_trayIconKindsActionGroup = nullptr;
};

} // namespace quentier
