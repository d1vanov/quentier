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
#include "MainWindow.h"
#include <quentier/utility/ApplicationSettings.h>
#include <quentier/logging/QuentierLogger.h>
#include <QApplication>
#include <QMenu>
#include <QActionGroup>
#include <QWidget>

#define SHOW_SYSTEM_TRAY_ICON_KEY QStringLiteral("ShowIconInSystemTray")
#define OVERRIDE_SYSTEM_TRAY_AVAILABILITY_KEY QStringLiteral("OverrideSystemTrayAvailability")

namespace quentier {

SystemTrayIconManager::SystemTrayIconManager(AccountManager & accountManager,
                                             QObject * parent) :
    QObject(parent),
    m_pAccountManager(&accountManager),
    m_pSystemTrayIcon(Q_NULLPTR),
    m_pTrayIconContextMenu(Q_NULLPTR),
    m_pAccountsTrayIconSubMenu(Q_NULLPTR),
    m_pAvailableAccountsActionGroup(Q_NULLPTR)
{
    createConnections();
    restoreTrayIconState();
    setupContextMenu();
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
        setupSystemTrayIcon();
    }

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
    setupAccountsSubMenu();
}

void SystemTrayIconManager::onAccountUpdated(Account account)
{
    QNDEBUG(QStringLiteral("SystemTrayIconManager::onAccountUpdated: ") << account);
    setupAccountsSubMenu();
}

void SystemTrayIconManager::onAccountAdded(Account account)
{
    QNDEBUG(QStringLiteral("SystemTrayIconManager::onAccountAdded: ") << account);
    setupAccountsSubMenu();
}

void SystemTrayIconManager::onNewTextNoteContextMenuAction()
{
    QNDEBUG(QStringLiteral("SystemTrayIconManager::onNewTextNoteContextMenuAction"));
    emit newTextNoteAdditionRequested();
}

void SystemTrayIconManager::onSwitchAccountContextMenuAction(bool checked)
{
    QNDEBUG(QStringLiteral("SystemTrayIconManager::onSwitchAccountContextMenuAction: checked = ")
            << (checked ? QStringLiteral("true") : QStringLiteral("false")));

    if (!checked) {
        QNTRACE(QStringLiteral("Ignoring the unchecking of account"));
        return;
    }

    QAction * action = qobject_cast<QAction*>(sender());
    if (Q_UNLIKELY(!action)) {
        ErrorString errorDescription(QT_TRANSLATE_NOOP("", "Internal error: account switching "
                                                       "action is unexpectedly null"));
        QNWARNING(errorDescription);
        emit notifyError(errorDescription);
        return;
    }

    QVariant indexData = action->data();
    bool conversionResult = false;
    int index = indexData.toInt(&conversionResult);
    if (Q_UNLIKELY(!conversionResult)) {
        ErrorString errorDescription(QT_TRANSLATE_NOOP("", "Internal error: can't get "
                                                       "identification data from "
                                                       "the account switching action"));
        QNWARNING(errorDescription);
        emit notifyError(errorDescription);
        return;
    }

    const QVector<Account> & availableAccounts = m_pAccountManager->availableAccounts();
    const int numAvailableAccounts = availableAccounts.size();

    if ((index < 0) || (index >= numAvailableAccounts)) {
        ErrorString errorDescription(QT_TRANSLATE_NOOP("", "Internal error: wrong index "
                                                       "into available accounts "
                                                       "in account switching action"));
        QNWARNING(errorDescription);
        emit notifyError(errorDescription);
        return;
    }

    const Account & availableAccount = availableAccounts[index];

    QNTRACE(QStringLiteral("Emitting the request to switch account: ") << availableAccount);
    emit accountSwitchRequested(availableAccount);
}

void SystemTrayIconManager::onShowMainWindowContextMenuAction()
{
    QNDEBUG(QStringLiteral("SystemTrayIconManager::onShowMainWindowContextMenuAction"));
    onShowHideMainWindowContextMenuAction(/* show = */ true);
}

void SystemTrayIconManager::onHideMainWindowContextMenuAction()
{
    QNDEBUG(QStringLiteral("SystemTrayIconManager::onHideMainWindowContextMenuAction"));
    onShowHideMainWindowContextMenuAction(/* show = */ false);
}

void SystemTrayIconManager::onQuitContextMenuAction()
{
    QNDEBUG(QStringLiteral("SystemTrayIconManager::onQuitContextMenuAction"));
    emit quitRequested();
}

void SystemTrayIconManager::onMainWindowShown()
{
    QNDEBUG(QStringLiteral("SystemTrayIconManager::onMainWindowShown"));
    evaluateShowHideMenuActions();
}

void SystemTrayIconManager::onMainWindowHidden()
{
    QNDEBUG(QStringLiteral("SystemTrayIconManager::onMainWindowHidden"));
    evaluateShowHideMenuActions();
}

void SystemTrayIconManager::createConnections()
{
    QNDEBUG(QStringLiteral("SystemTrayIconManager::createConnections"));

    // AccountManager connections
    QObject::connect(m_pAccountManager.data(), QNSIGNAL(AccountManager,switchedAccount,Account),
                     this, QNSLOT(SystemTrayIconManager,onAccountSwitched,Account));
    QObject::connect(this, QNSIGNAL(SystemTrayIconManager,switchAccount,Account),
                     m_pAccountManager.data(), QNSLOT(AccountManager,switchAccount,Account));
    QObject::connect(m_pAccountManager.data(), QNSIGNAL(AccountManager,accountUpdated,Account),
                     this, QNSLOT(SystemTrayIconManager,onAccountUpdated,Account));
    QObject::connect(m_pAccountManager.data(), QNSIGNAL(AccountManager,accountAdded,Account),
                     this, QNSLOT(SystemTrayIconManager,onAccountAdded,Account));

    // MainWindow connections
    MainWindow * pMainWindow = qobject_cast<MainWindow*>(parent());
    if (Q_UNLIKELY(!pMainWindow)) {
        QNWARNING(QStringLiteral("Internal error: can't cast the parent of SystemTrayIconManager to MainWindow"));
        return;
    }

    QObject::connect(pMainWindow, QNSIGNAL(MainWindow,shown),
                     this, QNSLOT(SystemTrayIconManager,onMainWindowShown));
    QObject::connect(pMainWindow, QNSIGNAL(MainWindow,hidden),
                     this, QNSLOT(SystemTrayIconManager,onMainWindowHidden));
}

void SystemTrayIconManager::setupSystemTrayIcon()
{
    m_pSystemTrayIcon = new QSystemTrayIcon(this);

    QIcon appIcon = qApp->windowIcon();
    m_pSystemTrayIcon->setIcon(appIcon);
}

void SystemTrayIconManager::setupContextMenu()
{
    QNDEBUG(QStringLiteral("SystemTrayIconManager::setupContextMenu"));

    MainWindow * pMainWindow = qobject_cast<MainWindow*>(parent());
    if (Q_UNLIKELY(!pMainWindow))
    {
        ErrorString errorDescription(QT_TRANSLATE_NOOP("", "Can't set up the tray icon's context menu: "
                                                       "internal error, the parent of SystemTrayIconManager "
                                                       "is not a MainWindow"));
        QNWARNING(errorDescription);
        emit notifyError(errorDescription);

        if (m_pSystemTrayIcon) {
            m_pSystemTrayIcon->setContextMenu(Q_NULLPTR);
        }

        return;
    }

    if (!isSystemTrayAvailable())
    {
        QNDEBUG(QStringLiteral("The system tray is not available, can't set up "
                               "the context menu for the system tray icon"));

        if (m_pSystemTrayIcon) {
            m_pSystemTrayIcon->setContextMenu(Q_NULLPTR);
        }

        return;
    }

    if (!m_pTrayIconContextMenu) {
        m_pTrayIconContextMenu = new QMenu(pMainWindow);
    }
    else {
        m_pTrayIconContextMenu->clear();
    }

#define ADD_CONTEXT_MENU_ACTION(name, menu, slot, enabled) \
    { \
        QAction * pAction = new QAction(name, menu); \
        pAction->setEnabled(enabled); \
        QObject::connect(pAction, QNSIGNAL(QAction,triggered), this, QNSLOT(SystemTrayIconManager,slot)); \
        menu->addAction(pAction); \
    }

    ADD_CONTEXT_MENU_ACTION(tr("New text note"), m_pTrayIconContextMenu,
                            onNewTextNoteContextMenuAction, true);

    m_pTrayIconContextMenu->addSeparator();

    setupAccountsSubMenu();

    m_pTrayIconContextMenu->addSeparator();

    ADD_CONTEXT_MENU_ACTION(tr("Show"), m_pTrayIconContextMenu,
                            onShowMainWindowContextMenuAction, pMainWindow->isHidden());

    ADD_CONTEXT_MENU_ACTION(tr("Hide"), m_pTrayIconContextMenu,
                            onHideMainWindowContextMenuAction, !pMainWindow->isHidden());

    m_pTrayIconContextMenu->addSeparator();

    ADD_CONTEXT_MENU_ACTION(tr("Quit"), m_pTrayIconContextMenu,
                            onQuitContextMenuAction, true);

    if (!m_pSystemTrayIcon) {
        setupSystemTrayIcon();
    }

    m_pSystemTrayIcon->setContextMenu(m_pTrayIconContextMenu);
}

void SystemTrayIconManager::setupAccountsSubMenu()
{
    QNDEBUG(QStringLiteral("SystemTrayIconManager::setupAccountsSubMenu"));

    if (Q_UNLIKELY(!m_pTrayIconContextMenu)) {
        QNDEBUG(QStringLiteral("No primary tray icon context menu"));
        return;
    }

    if (!m_pAccountsTrayIconSubMenu) {
        m_pAccountsTrayIconSubMenu = m_pTrayIconContextMenu->addMenu(tr("Switch account"));
    }
    else {
        m_pAccountsTrayIconSubMenu->clear();
    }

    delete m_pAvailableAccountsActionGroup;
    m_pAvailableAccountsActionGroup = new QActionGroup(this);
    m_pAvailableAccountsActionGroup->setExclusive(true);

    if (Q_UNLIKELY(m_pAccountManager.isNull())) {
        QNDEBUG(QStringLiteral("No account manager"));
        return;
    }

    Account currentAccount = m_pAccountManager->currentAccount();
    const QVector<Account> & availableAccounts = m_pAccountManager->availableAccounts();

    for(int i = 0, size = availableAccounts.size(); i < size; ++i)
    {
        const Account & availableAccount = availableAccounts[i];

        QString availableAccountRepresentationName = availableAccount.name();

        if (availableAccount.type() == Account::Type::Local) {
            availableAccountRepresentationName += QStringLiteral(" (");
            availableAccountRepresentationName += tr("local");
            availableAccountRepresentationName += QStringLiteral(")");
        }

        QAction * pAction = new QAction(availableAccountRepresentationName, Q_NULLPTR);
        m_pAccountsTrayIconSubMenu->addAction(pAction);
        pAction->setData(i);
        pAction->setCheckable(true);

        if (availableAccount == currentAccount) {
            pAction->setChecked(true);
        }

        QObject::connect(pAction, QNSIGNAL(QAction,triggered,bool),
                         this, QNSLOT(SystemTrayIconManager,onSwitchAccountContextMenuAction,bool));

        m_pAvailableAccountsActionGroup->addAction(pAction);
    }
}

void SystemTrayIconManager::evaluateShowHideMenuActions()
{
    QNDEBUG(QStringLiteral("SystemTrayIconManager::evaluateShowHideMenuActions"));

    if (Q_UNLIKELY(!m_pTrayIconContextMenu)) {
        QNDEBUG(QStringLiteral("No tray icon context menu"));
        return;
    }

    MainWindow * pMainWindow = qobject_cast<MainWindow*>(parent());
    if (Q_UNLIKELY(!pMainWindow)) {
        QNDEBUG(QStringLiteral("Parent is not MainWindow"));
        return;
    }

    QList<QAction*> actions = m_pTrayIconContextMenu->actions();
    QAction * pShowAction = Q_NULLPTR;
    QAction * pHideAction = Q_NULLPTR;

    QString showText = tr("Show");
    QString hideText = tr("Hide");

    for(auto it = actions.begin(), end = actions.end(); it != end; ++it)
    {
        QAction * pAction = *it;
        if (Q_UNLIKELY(!pAction)) {
            continue;
        }

        QString text = pAction->text();
        if (text == showText) {
            pShowAction = pAction;
        }
        else if (text == hideText) {
            pHideAction = pAction;
        }

        if (pShowAction && pHideAction) {
            break;
        }
    }

    bool mainWindowIsVisible = pMainWindow->isVisible();
    Qt::WindowStates mainWindowState = pMainWindow->windowState();
    bool mainWindowIsMinimized = (mainWindowState & Qt::WindowMinimized);
    if (mainWindowIsMinimized) {
        mainWindowIsVisible = false;
    }

    QNDEBUG(QStringLiteral("Main window is minimized: ") << (mainWindowIsMinimized ? QStringLiteral("true") : QStringLiteral("false"))
            << QStringLiteral(", main window is visible: ") << (mainWindowIsVisible ? QStringLiteral("true") : QStringLiteral("false")));

    if (pShowAction) {
        pShowAction->setEnabled(!mainWindowIsVisible);
        QNDEBUG(QStringLiteral("Show action is ") << (pShowAction->isEnabled() ? QStringLiteral("enabled") : QStringLiteral("disabled")));
    }
    else {
        QNDEBUG(QStringLiteral("Show action was not found"));
    }

    if (pHideAction) {
        pHideAction->setEnabled(mainWindowIsVisible);
        QNDEBUG(QStringLiteral("Hide action is ") << (pHideAction->isEnabled() ? QStringLiteral("enabled") : QStringLiteral("disabled")));
    }
    else {
        QNDEBUG(QStringLiteral("Hide action was not found"));
    }
}

void SystemTrayIconManager::onShowHideMainWindowContextMenuAction(const bool show)
{
    MainWindow * pMainWindow = qobject_cast<MainWindow*>(parent());
    if (Q_UNLIKELY(!pMainWindow))
    {
        ErrorString errorDescription(QT_TRANSLATE_NOOP("", "Can't show/hide the main window: "
                                                       "internal error, the parent of SystemTrayIconManager "
                                                       "is not a MainWindow"));
        QNWARNING(errorDescription);
        emit notifyError(errorDescription);
        return;
    }

    bool mainWindowIsVisible = pMainWindow->isVisible();
    Qt::WindowStates mainWindowState = pMainWindow->windowState();
    bool mainWindowIsMinimized = (mainWindowState & Qt::WindowMinimized);
    if (mainWindowIsMinimized) {
        mainWindowIsVisible = false;
    }

    if (show && mainWindowIsVisible) {
        QNDEBUG(QStringLiteral("The main window is already shown, nothing to do"));
        return;
    }
    else if (!show && !mainWindowIsVisible) {
        QNDEBUG(QStringLiteral("The main window is already hidden, nothing to do"));
        return;
    }

    if (show)
    {
        if (mainWindowIsMinimized) {
            mainWindowState = mainWindowState & (~Qt::WindowMinimized);
            pMainWindow->setWindowState(mainWindowState);
        }

        if (!pMainWindow->isVisible()) {
            pMainWindow->show();
        }
    }
    else
    {
        pMainWindow->hide();
    }
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
