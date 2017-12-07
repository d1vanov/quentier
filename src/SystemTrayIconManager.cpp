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
#include "DefaultSettings.h"
#include "MainWindow.h"
#include <quentier/utility/ApplicationSettings.h>
#include <quentier/logging/QuentierLogger.h>
#include <QApplication>
#include <QMenu>
#include <QActionGroup>
#include <QWidget>
#include <QStringList>

namespace quentier {

SystemTrayIconManager::SystemTrayIconManager(AccountManager & accountManager,
                                             QObject * parent) :
    QObject(parent),
    m_accountManager(accountManager),
    m_pSystemTrayIcon(Q_NULLPTR),
    m_pTrayIconContextMenu(Q_NULLPTR),
    m_pAccountsTrayIconSubMenu(Q_NULLPTR),
    m_pTrayIconKindSubMenu(Q_NULLPTR),
    m_pAvailableAccountsActionGroup(Q_NULLPTR),
    m_pTrayIconKindsActionGroup(Q_NULLPTR)
{
    createConnections();
    restoreTrayIconState();
    setupContextMenu();
}

bool SystemTrayIconManager::isSystemTrayAvailable() const
{
    QByteArray overrideSystemTrayAvailability = qgetenv(OVERRIDE_SYSTEM_TRAY_AVAILABILITY_ENV_VAR);
    if (!overrideSystemTrayAvailability.isEmpty())
    {
        bool overrideValue = (overrideSystemTrayAvailability != QByteArray("0"));
        QNDEBUG(QStringLiteral("Using overridden system tray availability: ")
                << (overrideValue ? QStringLiteral("true") : QStringLiteral("false")));
        return overrideValue;
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
        ErrorString errorDescription(QT_TR_NOOP("Can't show the system tray icon, "
                                                "the system tray is said to be unavailable"));
        QNINFO(errorDescription);
        Q_EMIT notifyError(errorDescription);
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

bool SystemTrayIconManager::shouldCloseToSystemTray() const
{
    QNDEBUG(QStringLiteral("SystemTrayIconManager::shouldCloseToSystemTray"));

    if (!isSystemTrayAvailable()) {
        QNDEBUG(QStringLiteral("The system tray is not available, can't close the app to tray"));
        return false;
    }

    if (!isShown()) {
        QNDEBUG(QStringLiteral("No system tray icon is shown, can't close the app to tray"));
        return false;
    }

    bool result = DEFAULT_CLOSE_TO_SYSTEM_TRAY;

    Account currentAccount = m_accountManager.currentAccount();
    ApplicationSettings appSettings(currentAccount, QUENTIER_UI_SETTINGS);
    appSettings.beginGroup(SYSTEM_TRAY_SETTINGS_GROUP_NAME);
    QVariant resultData = appSettings.value(CLOSE_TO_SYSTEM_TRAY_SETTINGS_KEY);
    appSettings.endGroup();

    if (resultData.isValid())
    {
        result = resultData.toBool();
        QNTRACE(QStringLiteral("Value from settings for the current account: ")
                << (result ? QStringLiteral("true") : QStringLiteral("false")));
    }
    else
    {
        QNTRACE(QStringLiteral("Found no stored setting, will use the default value: ")
                << (result ? QStringLiteral("true") : QStringLiteral("false")));
    }

    return result;
}

bool SystemTrayIconManager::shouldMinimizeToSystemTray() const
{
    QNDEBUG(QStringLiteral("SystemTrayIconManager::shouldMinimizeToSystemTray"));

    if (!isSystemTrayAvailable()) {
        QNDEBUG(QStringLiteral("The system tray is not available, can't minimize the app to tray"));
        return false;
    }

    if (!isShown()) {
        QNDEBUG(QStringLiteral("No system tray icon is shown, can't minimize the app to tray"));
        return false;
    }

    bool result = DEFAULT_MINIMIZE_TO_SYSTEM_TRAY;

    Account currentAccount = m_accountManager.currentAccount();
    ApplicationSettings appSettings(currentAccount, QUENTIER_UI_SETTINGS);
    appSettings.beginGroup(SYSTEM_TRAY_SETTINGS_GROUP_NAME);
    QVariant resultData = appSettings.value(MINIMIZE_TO_SYSTEM_TRAY_SETTINGS_KEY);
    appSettings.endGroup();

    if (resultData.isValid())
    {
        result = resultData.toBool();
        QNTRACE(QStringLiteral("Value from settings for the current account: ")
                << (result ? QStringLiteral("true") : QStringLiteral("false")));
    }
    else
    {
        QNTRACE(QStringLiteral("Found no stored setting, will use the default value: ")
                << (result ? QStringLiteral("true") : QStringLiteral("false")));
    }

    return result;
}

bool SystemTrayIconManager::shouldStartMinimizedToSystemTray() const
{
    QNDEBUG(QStringLiteral("SystemTrayIconManager::shouldStartMinimizedToSystemTray"));

    if (!isSystemTrayAvailable()) {
        QNDEBUG(QStringLiteral("The system tray is not available, can't start "
                               "the app minimized to system tray"));
        return false;
    }

    if (!isShown()) {
        QNDEBUG(QStringLiteral("No system tray icon is shown, can't start the app "
                               "minimized to system tray"));
        return false;
    }

    bool result = DEFAULT_START_MINIMIZED_TO_SYSTEM_TRAY;

    Account currentAccount = m_accountManager.currentAccount();
    ApplicationSettings appSettings(currentAccount, QUENTIER_UI_SETTINGS);
    appSettings.beginGroup(SYSTEM_TRAY_SETTINGS_GROUP_NAME);
    QVariant resultData = appSettings.value(START_MINIMIZED_TO_SYSTEM_TRAY_SETTINGS_KEY);
    appSettings.endGroup();

    if (resultData.isValid())
    {
        result = resultData.toBool();
        QNTRACE(QStringLiteral("Value from settings for the current account: ")
                << (result ? QStringLiteral("true") : QStringLiteral("false")));
    }
    else
    {
        QNTRACE(QStringLiteral("Found no stored setting, will use the default value: ")
                << (result ? QStringLiteral("true") : QStringLiteral("false")));
    }

    return result;
}

SystemTrayIconManager::TrayAction SystemTrayIconManager::singleClickTrayAction() const
{
    TrayAction action = DEFAULT_SINGLE_CLICK_TRAY_ACTION;

    Account currentAccount = m_accountManager.currentAccount();
    ApplicationSettings appSettings(currentAccount, QUENTIER_UI_SETTINGS);
    appSettings.beginGroup(SYSTEM_TRAY_SETTINGS_GROUP_NAME);
    QVariant actionData = appSettings.value(SINGLE_CLICK_TRAY_ACTION_SETTINGS_KEY);
    appSettings.endGroup();

    if (actionData.isValid())
    {
        bool conversionResult = false;
        action = static_cast<TrayAction>(actionData.toInt(&conversionResult));
        if (Q_UNLIKELY(!conversionResult))
        {
            QNWARNING(QStringLiteral("Can't read the left mouse button tray action: "
                                     "failed to convert the value read from settings to int: ")
                      << actionData);
            action = DEFAULT_SINGLE_CLICK_TRAY_ACTION;
        }
        else
        {
            QNDEBUG(QStringLiteral("Action read from settings: ") << action);
        }
    }

    return action;
}

SystemTrayIconManager::TrayAction SystemTrayIconManager::middleClickTrayAction() const
{
    TrayAction action = DEFAULT_MIDDLE_CLICK_TRAY_ACTION;

    Account currentAccount = m_accountManager.currentAccount();
    ApplicationSettings appSettings(currentAccount, QUENTIER_UI_SETTINGS);
    appSettings.beginGroup(SYSTEM_TRAY_SETTINGS_GROUP_NAME);
    QVariant actionData = appSettings.value(MIDDLE_CLICK_TRAY_ACTION_SETTINGS_KEY);
    appSettings.endGroup();

    if (actionData.isValid())
    {
        bool conversionResult = false;
        action = static_cast<TrayAction>(actionData.toInt(&conversionResult));
        if (Q_UNLIKELY(!conversionResult))
        {
            QNWARNING(QStringLiteral("Can't read the middle mouse button tray action: "
                                     "failed to convert the value read from settings to int: ")
                      << actionData);
            action = DEFAULT_MIDDLE_CLICK_TRAY_ACTION;
        }
        else
        {
            QNDEBUG(QStringLiteral("Action read from settings: ") << action);
        }
    }

    return action;
}

SystemTrayIconManager::TrayAction SystemTrayIconManager::doubleClickTrayAction() const
{
    TrayAction action = DEFAULT_DOUBLE_CLICK_TRAY_ACTION;

    Account currentAccount = m_accountManager.currentAccount();
    ApplicationSettings appSettings(currentAccount, QUENTIER_UI_SETTINGS);
    appSettings.beginGroup(SYSTEM_TRAY_SETTINGS_GROUP_NAME);
    QVariant actionData = appSettings.value(DOUBLE_CLICK_TRAY_ACTION_SETTINGS_KEY);
    appSettings.endGroup();

    if (actionData.isValid())
    {
        bool conversionResult = false;
        action = static_cast<TrayAction>(actionData.toInt(&conversionResult));
        if (Q_UNLIKELY(!conversionResult))
        {
            QNWARNING(QStringLiteral("Can't read the right mouse button tray action: "
                                     "failed to convert the value read from settings to int: ")
                      << actionData);
            action = DEFAULT_DOUBLE_CLICK_TRAY_ACTION;
        }
        else
        {
            QNDEBUG(QStringLiteral("Action read from settings: ") << action);
        }
    }

    return action;
}

void SystemTrayIconManager::onSystemTrayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
    QNDEBUG(QStringLiteral("SystemTrayIconManager::onSystemTrayIconActivated: reason = ")
            << reason);

    QString settingKey;
    TrayAction defaultAction;
    bool shouldShowContextMenu = false;

    switch(reason)
    {
    case QSystemTrayIcon::Trigger:
        settingKey = SINGLE_CLICK_TRAY_ACTION_SETTINGS_KEY;
        defaultAction = DEFAULT_SINGLE_CLICK_TRAY_ACTION;
        break;
    case QSystemTrayIcon::MiddleClick:
        settingKey = MIDDLE_CLICK_TRAY_ACTION_SETTINGS_KEY;
        defaultAction = DEFAULT_MIDDLE_CLICK_TRAY_ACTION;
        break;
    case QSystemTrayIcon::DoubleClick:
        settingKey = DOUBLE_CLICK_TRAY_ACTION_SETTINGS_KEY;
        defaultAction = DEFAULT_DOUBLE_CLICK_TRAY_ACTION;
        break;
    case QSystemTrayIcon::Context:
        shouldShowContextMenu = true;
        break;
    default:
    {
        QNINFO(QStringLiteral("Unidentified activation reason for the system tray icon: ")
               << reason);
        return;
    }
    }

    if (shouldShowContextMenu)
    {
        if (Q_UNLIKELY(!m_pTrayIconContextMenu)) {
            QNWARNING(QStringLiteral("Can't show the tray icon context menu: context menu is null"));
            return;
        }

        m_pTrayIconContextMenu->exec(QCursor::pos());
        return;
    }

    TrayAction action = defaultAction;

    Account currentAccount = m_accountManager.currentAccount();
    ApplicationSettings appSettings(currentAccount, QUENTIER_UI_SETTINGS);
    appSettings.beginGroup(SYSTEM_TRAY_SETTINGS_GROUP_NAME);
    QVariant actionData = appSettings.value(settingKey);
    appSettings.endGroup();

    if (actionData.isValid())
    {
        bool conversionResult = false;
        action = static_cast<TrayAction>(actionData.toInt(&conversionResult));
        if (Q_UNLIKELY(!conversionResult)) {
            QNWARNING(QStringLiteral("Can't read the tray action setting per activation reason: "
                                     "failed to convert the value read from settings to int: ")
                      << actionData);
            action = defaultAction;
        }
        else {
            QNDEBUG(QStringLiteral("Action for setting ") << settingKey
                    << QStringLiteral(": ") << action);
        }
    }

    switch(action)
    {
    case TrayActionDoNothing:
    {
        QNDEBUG(QStringLiteral("The action is \"do nothing\", obeying"));
        return;
    }
    case TrayActionShowHide:
    {
        MainWindow * pMainWindow = qobject_cast<MainWindow*>(parent());
        if (Q_UNLIKELY(!pMainWindow)) {
            QNWARNING(QStringLiteral("Can't show/hide the main window from system tray: "
                                     "can't cast the parent of SystemTrayIconManager to MainWindow"));
            return;
        }

        if (pMainWindow->isHidden()) {
            pMainWindow->show();
        }
        else {
            pMainWindow->hide();
        }

        break;
    }
    case TrayActionNewTextNote:
    {
        MainWindow * pMainWindow = qobject_cast<MainWindow*>(parent());
        if (Q_UNLIKELY(!pMainWindow)) {
            QNWARNING(QStringLiteral("Can't ensure the main window is shown on request "
                                     "to create a new text note from system tray: "
                                     "can't cast the parent of SystemTrayIconManager to MainWindow"));
        }
        else if (pMainWindow->isHidden())
        {
            pMainWindow->show();
        }

        Q_EMIT newTextNoteAdditionRequested();
        break;
    }
    case TrayActionShowContextMenu:
    {
        if (Q_UNLIKELY(!m_pTrayIconContextMenu)) {
            QNWARNING(QStringLiteral("Can't show the tray icon context menu: context menu is null"));
            return;
        }

        m_pTrayIconContextMenu->exec(QCursor::pos());
        break;
    }
    default:
    {
        QNWARNING(QStringLiteral("Detected unrecognized tray action: ") << action);
        break;
    }
    }
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
    Q_EMIT newTextNoteAdditionRequested();
}

void SystemTrayIconManager::onSwitchAccountContextMenuAction(bool checked)
{
    QNDEBUG(QStringLiteral("SystemTrayIconManager::onSwitchAccountContextMenuAction: checked = ")
            << (checked ? QStringLiteral("true") : QStringLiteral("false")));

    if (!checked) {
        QNTRACE(QStringLiteral("Ignoring the unchecking of account"));
        return;
    }

    QAction * pAction = qobject_cast<QAction*>(sender());
    if (Q_UNLIKELY(!pAction)) {
        ErrorString errorDescription(QT_TR_NOOP("Internal error: account switching "
                                                "action is unexpectedly null"));
        QNWARNING(errorDescription);
        Q_EMIT notifyError(errorDescription);
        return;
    }

    QVariant indexData = pAction->data();
    bool conversionResult = false;
    int index = indexData.toInt(&conversionResult);
    if (Q_UNLIKELY(!conversionResult)) {
        ErrorString errorDescription(QT_TR_NOOP("Internal error: can't get identification data from "
                                                "the account switching action"));
        QNWARNING(errorDescription);
        Q_EMIT notifyError(errorDescription);
        return;
    }

    const QVector<Account> & availableAccounts = m_accountManager.availableAccounts();
    const int numAvailableAccounts = availableAccounts.size();

    if ((index < 0) || (index >= numAvailableAccounts)) {
        ErrorString errorDescription(QT_TR_NOOP("Internal error: wrong index into available accounts "
                                                "in account switching action"));
        QNWARNING(errorDescription);
        Q_EMIT notifyError(errorDescription);
        return;
    }

    const Account & availableAccount = availableAccounts[index];

    QNTRACE(QStringLiteral("Emitting the request to switch account: ") << availableAccount);
    Q_EMIT accountSwitchRequested(availableAccount);
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

void SystemTrayIconManager::onSwitchTrayIconContextMenuAction(bool checked)
{
    QNDEBUG(QStringLiteral("SystemTrayIconManager::onSwitchTrayIconContextMenuAction: checked = ")
            << (checked ? QStringLiteral("true") : QStringLiteral("false")));

    if (!checked) {
        QNTRACE(QStringLiteral("Ignoring the unchecking of current tray icon kind"));
        return;
    }

    QAction * pAction = qobject_cast<QAction*>(sender());
    if (Q_UNLIKELY(!pAction)) {
        ErrorString errorDescription(QT_TR_NOOP("Internal error: tray icon kind switching "
                                                "action is unexpectedly null"));
        QNWARNING(errorDescription);
        Q_EMIT notifyError(errorDescription);
        return;
    }

    Account currentAccount = m_accountManager.currentAccount();
    ApplicationSettings appSettings(currentAccount, QUENTIER_UI_SETTINGS);
    appSettings.beginGroup(SYSTEM_TRAY_SETTINGS_GROUP_NAME);
    appSettings.setValue(SYSTEM_TRAY_ICON_KIND_KEY, pAction->data().toString());
    appSettings.endGroup();

    setupSystemTrayIcon();
}

void SystemTrayIconManager::onQuitContextMenuAction()
{
    QNDEBUG(QStringLiteral("SystemTrayIconManager::onQuitContextMenuAction"));
    Q_EMIT quitRequested();
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
    QObject::connect(&m_accountManager, QNSIGNAL(AccountManager,switchedAccount,Account),
                     this, QNSLOT(SystemTrayIconManager,onAccountSwitched,Account));
    QObject::connect(this, QNSIGNAL(SystemTrayIconManager,switchAccount,Account),
                     &m_accountManager, QNSLOT(AccountManager,switchAccount,Account));
    QObject::connect(&m_accountManager, QNSIGNAL(AccountManager,accountUpdated,Account),
                     this, QNSLOT(SystemTrayIconManager,onAccountUpdated,Account));
    QObject::connect(&m_accountManager, QNSIGNAL(AccountManager,accountAdded,Account),
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
    if (!m_pSystemTrayIcon)
    {
        m_pSystemTrayIcon = new QSystemTrayIcon(this);

        QObject::connect(m_pSystemTrayIcon,
                         QNSIGNAL(QSystemTrayIcon,activated,
                                  QSystemTrayIcon::ActivationReason),
                         this,
                         QNSLOT(SystemTrayIconManager,onSystemTrayIconActivated,
                                QSystemTrayIcon::ActivationReason));
    }

    Account currentAccount = m_accountManager.currentAccount();
    ApplicationSettings appSettings(currentAccount, QUENTIER_UI_SETTINGS);
    appSettings.beginGroup(SYSTEM_TRAY_SETTINGS_GROUP_NAME);
    QString trayIconKind = appSettings.value(SYSTEM_TRAY_ICON_KIND_KEY).toString();
    appSettings.endGroup();

    if (trayIconKind.isEmpty()) {
        QNDEBUG(QStringLiteral("The tray icon kind is empty, will use the default tray icon"));
        trayIconKind = DEFAULT_TRAY_ICON_KIND;
    }
    else if (trayIconKind == QStringLiteral("dark")) {
        QNDEBUG(QStringLiteral("Will use the simple dark tray icon"));
    }
    else if (trayIconKind == QStringLiteral("light")) {
        QNDEBUG(QStringLiteral("Will use the simple light tray icon"));
    }
    else if (trayIconKind == QStringLiteral("colored")) {
        QNDEBUG(QStringLiteral("Will use the colored tray icon"));
    }
    else {
        QNDEBUG(QStringLiteral("Unidentified tray icon kind (") << trayIconKind << QStringLiteral(", will fallback to the default"));
        trayIconKind = DEFAULT_TRAY_ICON_KIND;
    }

    QString whichIcon;
    if (trayIconKind == QStringLiteral("dark")) {
        whichIcon = QStringLiteral("_simple_dark");
    }
    else if (trayIconKind == QStringLiteral("light")) {
        whichIcon = QStringLiteral("_simple_light");
    }

    QIcon icon;

#define ADD_ICON(size) \
    icon.addFile(QStringLiteral(":/app_icons/quentier_icon") + whichIcon + QStringLiteral("_" #size ".png"), QSize(size, size))

    ADD_ICON(512);
    ADD_ICON(256);
    ADD_ICON(128);
    ADD_ICON(64);
    ADD_ICON(48);
    ADD_ICON(32);
    ADD_ICON(16);

    m_pSystemTrayIcon->setIcon(icon);

#undef ADD_ICON
}

void SystemTrayIconManager::setupContextMenu()
{
    QNDEBUG(QStringLiteral("SystemTrayIconManager::setupContextMenu"));

    MainWindow * pMainWindow = qobject_cast<MainWindow*>(parent());
    if (Q_UNLIKELY(!pMainWindow))
    {
        ErrorString errorDescription(QT_TR_NOOP("Can't set up the tray icon's context menu: "
                                                "internal error, the parent of SystemTrayIconManager "
                                                "is not a MainWindow"));
        QNWARNING(errorDescription);
        Q_EMIT notifyError(errorDescription);

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

    setupTrayIconKindSubMenu();

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

    Account currentAccount = m_accountManager.currentAccount();
    const QVector<Account> & availableAccounts = m_accountManager.availableAccounts();

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

void SystemTrayIconManager::setupTrayIconKindSubMenu()
{
    QNDEBUG(QStringLiteral("SystemTrayIconManager::setupTrayIconKindSubMenu"));

    if (Q_UNLIKELY(!m_pTrayIconContextMenu)) {
        QNDEBUG(QStringLiteral("No primary tray icon context menu"));
        return;
    }

    if (!m_pTrayIconKindSubMenu) {
        m_pTrayIconKindSubMenu = m_pTrayIconContextMenu->addMenu(tr("Tray icon kind"));
    }
    else {
        m_pTrayIconKindSubMenu->clear();
    }

    delete m_pTrayIconKindsActionGroup;
    m_pTrayIconKindsActionGroup = new QActionGroup(this);
    m_pTrayIconKindsActionGroup->setExclusive(true);

    QString currentTrayIconKind = DEFAULT_TRAY_ICON_KIND;

    Account currentAccount = m_accountManager.currentAccount();
    ApplicationSettings appSettings(currentAccount, QUENTIER_UI_SETTINGS);
    appSettings.beginGroup(SYSTEM_TRAY_SETTINGS_GROUP_NAME);
    currentTrayIconKind = appSettings.value(SYSTEM_TRAY_ICON_KIND_KEY).toString();
    appSettings.endGroup();

    if ( (currentTrayIconKind != QStringLiteral("dark")) &&
         (currentTrayIconKind != QStringLiteral("light")) &&
         (currentTrayIconKind != QStringLiteral("colored")) )
    {
        QNDEBUG(QStringLiteral("Wrong/unrecognized value of current tray icon kind setting: ")
                << currentTrayIconKind << QStringLiteral(", fallback to default"));
        currentTrayIconKind = DEFAULT_TRAY_ICON_KIND;
    }

    QNDEBUG(QStringLiteral("Current tray icon kind = ") << currentTrayIconKind);

    QStringList actionNames;
    actionNames << QStringLiteral("dark")
                << QStringLiteral("light")
                << QStringLiteral("colored");
    for(auto it = actionNames.constBegin(), end = actionNames.constEnd(); it != end; ++it)
    {
        const QString & actionName = *it;

        QAction * pAction = new QAction(actionName, Q_NULLPTR);
        m_pTrayIconKindSubMenu->addAction(pAction);
        pAction->setData(actionName);
        pAction->setCheckable(true);
        pAction->setChecked(actionName == currentTrayIconKind);

        QObject::connect(pAction, QNSIGNAL(QAction,triggered,bool),
                         this, QNSLOT(SystemTrayIconManager,onSwitchTrayIconContextMenuAction,bool));

        m_pTrayIconKindsActionGroup->addAction(pAction);
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

    QNDEBUG(QStringLiteral("Main window is minimized: ")
            << (mainWindowIsMinimized ? QStringLiteral("true") : QStringLiteral("false"))
            << QStringLiteral(", main window is visible: ")
            << (mainWindowIsVisible ? QStringLiteral("true") : QStringLiteral("false")));

    if (pShowAction)
    {
        pShowAction->setEnabled(!mainWindowIsVisible);
        QNDEBUG(QStringLiteral("Show action is ")
                << (pShowAction->isEnabled() ? QStringLiteral("enabled") : QStringLiteral("disabled")));
    }
    else
    {
        QNDEBUG(QStringLiteral("Show action was not found"));
    }

    if (pHideAction)
    {
        pHideAction->setEnabled(mainWindowIsVisible);
        QNDEBUG(QStringLiteral("Hide action is ")
                << (pHideAction->isEnabled() ? QStringLiteral("enabled") : QStringLiteral("disabled")));
    }
    else
    {
        QNDEBUG(QStringLiteral("Hide action was not found"));
    }
}

void SystemTrayIconManager::onShowHideMainWindowContextMenuAction(const bool show)
{
    MainWindow * pMainWindow = qobject_cast<MainWindow*>(parent());
    if (Q_UNLIKELY(!pMainWindow))
    {
        ErrorString errorDescription(QT_TR_NOOP("Can't show/hide the main window: internal error, the parent "
                                                "of SystemTrayIconManager is not a MainWindow"));
        QNWARNING(errorDescription);
        Q_EMIT notifyError(errorDescription);
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

    Account currentAccount = m_accountManager.currentAccount();
    ApplicationSettings appSettings(currentAccount, QUENTIER_UI_SETTINGS);
    appSettings.beginGroup(SYSTEM_TRAY_SETTINGS_GROUP_NAME);
    appSettings.setValue(SHOW_SYSTEM_TRAY_ICON_SETTINGS_KEY, QVariant(isShown()));
    appSettings.endGroup();
}

void SystemTrayIconManager::restoreTrayIconState()
{
    QNDEBUG(QStringLiteral("SystemTrayIconManager::restoreTrayIconState"));

    if (!isSystemTrayAvailable()) {
        QNDEBUG(QStringLiteral("The system tray is not available, won't show the system tray icon"));
        hide();
        return;
    }

    Account currentAccount = m_accountManager.currentAccount();
    ApplicationSettings appSettings(currentAccount, QUENTIER_UI_SETTINGS);
    appSettings.beginGroup(SYSTEM_TRAY_SETTINGS_GROUP_NAME);
    QVariant data = appSettings.value(SHOW_SYSTEM_TRAY_ICON_SETTINGS_KEY);
    appSettings.endGroup();

    bool shouldShow = DEFAULT_SHOW_SYSTEM_TRAY_ICON;
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
