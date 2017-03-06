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

#include "SystemTrayIconManager.h"
#include "AccountManager.h"
#include "SettingsNames.h"
#include <quentier/utility/ApplicationSettings.h>
#include <quentier/logging/QuentierLogger.h>
#include <QApplication>

#define SHOW_SYSTEM_TRAY_ICON_KEY QStringLiteral("ShowIconInSystemTray")
#define OVERRIDE_SYSTEM_TRAY_AVAILABILITY_KEY QStringLiteral("OverrideSystemTrayAvailability")

namespace quentier {

SystemTrayIconManager::SystemTrayIconManager(AccountManager & accountManager,
                                             QObject * parent) :
    QObject(parent),
    m_pAccountManager(&accountManager),
    m_pSystemTrayIcon(Q_NULLPTR)
{
    createConnections();
    restoreTrayIconState();
}

bool SystemTrayIconManager::isSystemTrayAvailable() const
{
    if (!m_pAccountManager.isNull())
    {
        Account currentAccount = m_pAccountManager->currentAccount();
        ApplicationSettings appSettings(currentAccount, QUENTIER_UI_SETTINGS);
        appSettings.beginGroup(QStringLiteral("SystemTray"));
        QVariant overrideSystemTrayAvailability = appSettings.value(OVERRIDE_SYSTEM_TRAY_AVAILABILITY_KEY);
        appSettings.endGroup();

        if (overrideSystemTrayAvailability.isValid())
        {
            bool overrideValue = overrideSystemTrayAvailability.toBool();
            QNDEBUG(QStringLiteral("Using overridden system tray availability: ")
                    << (overrideValue ? QStringLiteral("true") : QStringLiteral("false")));
            return overrideValue;
        }
    }

    return QSystemTrayIcon::isSystemTrayAvailable();
}

bool SystemTrayIconManager::isShown() const
{
    return (m_pSystemTrayIcon ? m_pSystemTrayIcon->isVisible() : false);
}

void SystemTrayIconManager::show()
{
    QNDEBUG(QStringLiteral("SystemTrayIconManager::show"));

    if (isShown()) {
        QNDEBUG(QStringLiteral("System tray icon is already shown, nothing to do"));
        return;
    }

    if (!isSystemTrayAvailable()) {
        ErrorString errorDescription(QT_TRANSLATE_NOOP("", "Can't show the system tray icon, "
                                                       "the system tray is said to be unavailable"));
        QNINFO(errorDescription);
        emit notifyError(errorDescription);
        return;
    }

    if (!m_pSystemTrayIcon) {
        m_pSystemTrayIcon = new QSystemTrayIcon(this);
    }

    QIcon appIcon = qApp->windowIcon();
    m_pSystemTrayIcon->setIcon(appIcon);

    m_pSystemTrayIcon->show();
    persistTrayIconState();
}

void SystemTrayIconManager::hide()
{
    QNDEBUG(QStringLiteral("SystemTrayIconManager::hide"));

    if (!m_pSystemTrayIcon || !isShown()) {
        QNDEBUG(QStringLiteral("System tray icon is already not shown, nothing to do"));
        return;
    }

    m_pSystemTrayIcon->hide();
    persistTrayIconState();
}

void SystemTrayIconManager::onAccountSwitched(Account account)
{
    QNDEBUG(QStringLiteral("SystemTrayIconManager::onAccountSwitched: ") << account);

    // TODO: implement, set up the account-specific things
}

void SystemTrayIconManager::createConnections()
{
    QNDEBUG(QStringLiteral("SystemTrayIconManager::createConnections"));

    // AccountManager connections
    QObject::connect(m_pAccountManager.data(), QNSIGNAL(AccountManager,switchedAccount,Account),
                     this, QNSLOT(SystemTrayIconManager,onAccountSwitched,Account));
    QObject::connect(this, QNSIGNAL(SystemTrayIconManager,switchAccount,Account),
                     m_pAccountManager.data(), QNSLOT(AccountManager,switchAccount,Account));
}

void SystemTrayIconManager::persistTrayIconState()
{
    QNDEBUG(QStringLiteral("SystemTrayIconManager::persistTrayIconState"));

    if (Q_UNLIKELY(m_pAccountManager.isNull())) {
        QNWARNING(QStringLiteral("Can't persist the system tray icon state: no AccountManager"));
        return;
    }

    Account currentAccount = m_pAccountManager->currentAccount();

    ApplicationSettings appSettings(currentAccount, QUENTIER_UI_SETTINGS);
    appSettings.beginGroup(QStringLiteral("SystemTray"));
    appSettings.setValue(SHOW_SYSTEM_TRAY_ICON_KEY, QVariant(isShown()));
    appSettings.endGroup();
}

void SystemTrayIconManager::restoreTrayIconState()
{
    QNDEBUG(QStringLiteral("SystemTrayIconManager::restoreTrayIconState"));

    if (Q_UNLIKELY(m_pAccountManager.isNull())) {
        QNWARNING(QStringLiteral("Can't restore the system tray icon state: no AccountManager"));
        return;
    }

    Account currentAccount = m_pAccountManager->currentAccount();

    ApplicationSettings appSettings(currentAccount, QUENTIER_UI_SETTINGS);
    appSettings.beginGroup(QStringLiteral("SystemTray"));
    QVariant data = appSettings.value(SHOW_SYSTEM_TRAY_ICON_KEY);
    appSettings.endGroup();

    bool shouldShow = true;
    if (data.isValid()) {
        shouldShow = data.toBool();
    }

    if (shouldShow) {
        show();
    }
    else {
        hide();
    }
}

} // namespace quentier
