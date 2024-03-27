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

#include "SystemTrayIconManager.h"

#include <lib/account/AccountManager.h>
#include <lib/preferences/defaults/SystemTray.h>
#include <lib/preferences/keys/Files.h>
#include <lib/preferences/keys/SystemTray.h>
#include <lib/view/Utils.h>

#include <quentier/logging/QuentierLogger.h>
#include <quentier/utility/ApplicationSettings.h>

#include <QActionGroup>
#include <QApplication>
#include <QMenu>
#include <QStringList>
#include <QTextStream>
#include <QWidget>

#include <utility>

namespace quentier {

namespace {

constexpr auto gDefaultSingleClickTrayAction =
#ifdef Q_WE_MAC
    SystemTrayIconManager::TrayActionDoNothing;
#else
    SystemTrayIconManager::TrayActionShowContextMenu;
#endif

constexpr auto gDefaultMiddleClickTrayAction =
    SystemTrayIconManager::TrayActionShowHide;

constexpr auto gDefaultDoubleClickTrayAction =
    SystemTrayIconManager::TrayActionDoNothing;

} // namespace

SystemTrayIconManager::SystemTrayIconManager(
    AccountManager & accountManager, QObject * parent) :
    QObject{parent}, m_accountManager{accountManager}
{
    createConnections();
    restoreTrayIconState();
    setupContextMenu();
}

bool SystemTrayIconManager::isSystemTrayAvailable() const
{
    const auto overrideSystemTrayAvailability =
        qgetenv(preferences::keys::overrideSystemTrayAvailabilityEnvVar.data());

    if (!overrideSystemTrayAvailability.isEmpty()) {
        bool overrideValue =
            (overrideSystemTrayAvailability != QByteArray("0"));

        QNDEBUG(
            "tray::SystemTrayIconManager",
            "Using overridden system tray availability: "
                << (overrideValue ? "true" : "false"));

        return overrideValue;
    }

    return QSystemTrayIcon::isSystemTrayAvailable();
}

bool SystemTrayIconManager::isShown() const
{
    return m_systemTrayIcon ? m_systemTrayIcon->isVisible() : false;
}

void SystemTrayIconManager::show()
{
    QNDEBUG("tray::SystemTrayIconManager", "SystemTrayIconManager::show");

    if (isShown()) {
        QNDEBUG(
            "tray::SystemTrayIconManager",
            "System tray icon is already shown, nothing to do");
        return;
    }

    if (!isSystemTrayAvailable()) {
        ErrorString errorDescription{
            QT_TR_NOOP("Can't show the system tray icon, the system tray is "
                       "said to be unavailable")};
        QNINFO("tray::SystemTrayIconManager", errorDescription);
        Q_EMIT notifyError(std::move(errorDescription));
        return;
    }

    if (!m_systemTrayIcon) {
        setupSystemTrayIcon();
    }

    m_systemTrayIcon->show();
    persistTrayIconState();
}

void SystemTrayIconManager::hide()
{
    QNDEBUG("tray::SystemTrayIconManager", "SystemTrayIconManager::hide");

    if (!m_systemTrayIcon || !isShown()) {
        QNDEBUG(
            "tray::SystemTrayIconManager",
            "System tray icon is already not shown, nothing to do");
        return;
    }

    m_systemTrayIcon->hide();
    persistTrayIconState();
}

void SystemTrayIconManager::setPreferenceCloseToSystemTray(
    const bool value) const
{
    QNDEBUG(
        "preferences", "SystemTrayIconManager::setPreferenceCloseToSystemTray");

    ApplicationSettings appSettings{
        m_accountManager.currentAccount(),
        preferences::keys::files::userInterface};

    appSettings.beginGroup(preferences::keys::systemTrayGroup);
    appSettings.setValue(preferences::keys::closeToSystemTray, value);
    appSettings.endGroup();

    QNDEBUG(
        "tray::SystemTrayIconManager",
        preferences::keys::closeToSystemTray.data()
            << " preference value for the current account set to: "
            << (value ? "true" : "false"));
}

bool SystemTrayIconManager::getPreferenceCloseToSystemTray() const
{
    QNTRACE(
        "preferences", "SystemTrayIconManager::getPreferenceCloseToSystemTray");

    ApplicationSettings appSettings{
        m_accountManager.currentAccount(),
        preferences::keys::files::userInterface};

    appSettings.beginGroup(preferences::keys::systemTrayGroup);
    const QVariant resultData =
        appSettings.value(preferences::keys::closeToSystemTray);
    appSettings.endGroup();

    bool value = resultData.isValid()
        ? resultData.toBool()
        : preferences::defaults::closeToSystemTray;

    QNTRACE(
        "tray::SystemTrayIconManager",
        preferences::keys::closeToSystemTray.data()
            << " preference value for the current account: "
            << (value ? "true" : "false"));

    return value;
}

bool SystemTrayIconManager::shouldCloseToSystemTray() const
{
    QNDEBUG(
        "tray::SystemTrayIconManager",
        "SystemTrayIconManager::shouldCloseToSystemTray");

    if (!isSystemTrayAvailable()) {
        QNDEBUG(
            "tray::SystemTrayIconManager",
            "The system tray is not available, can't close the app to tray");
        return false;
    }

    if (!isShown()) {
        QNDEBUG(
            "tray::SystemTrayIconManager",
            "No system tray icon is shown, can't close the app to tray");
        return false;
    }

    bool result = getPreferenceCloseToSystemTray();
    return result;
}

bool SystemTrayIconManager::shouldMinimizeToSystemTray() const
{
    QNDEBUG(
        "tray::SystemTrayIconManager",
        "SystemTrayIconManager::shouldMinimizeToSystemTray");

    if (!isSystemTrayAvailable()) {
        QNDEBUG(
            "tray::SystemTrayIconManager",
            "The system tray is not available, can't minimize the app to tray");
        return false;
    }

    if (!isShown()) {
        QNDEBUG(
            "tray::SystemTrayIconManager",
            "No system tray icon is shown, can't minimize the app to tray");
        return false;
    }

    bool result = preferences::defaults::minimizeToSystemTray;

    ApplicationSettings appSettings{
        m_accountManager.currentAccount(),
        preferences::keys::files::userInterface};

    appSettings.beginGroup(preferences::keys::systemTrayGroup);

    const QVariant resultData =
        appSettings.value(preferences::keys::minimizeToSystemTray);

    appSettings.endGroup();

    if (resultData.isValid()) {
        result = resultData.toBool();
        QNTRACE(
            "tray::SystemTrayIconManager",
            "Value from settings for the current account: "
                << (result ? "true" : "false"));
    }
    else {
        QNTRACE(
            "tray::SystemTrayIconManager",
            "Found no stored setting, will use the default value: "
                << (result ? "true" : "false"));
    }

    return result;
}

bool SystemTrayIconManager::shouldStartMinimizedToSystemTray() const
{
    QNDEBUG(
        "preferences",
        "SystemTrayIconManager::shouldStartMinimizedToSystemTray");

    if (!isSystemTrayAvailable()) {
        QNDEBUG(
            "tray::SystemTrayIconManager",
            "The system tray is not available, can't start the app minimized "
            "to system tray");
        return false;
    }

    if (!isShown()) {
        QNDEBUG(
            "tray::SystemTrayIconManager",
            "No system tray icon is shown, can't start the app minimized to "
            "system tray");
        return false;
    }

    bool result = preferences::defaults::startMinimizedToSystemTray;

    ApplicationSettings appSettings{
        m_accountManager.currentAccount(),
        preferences::keys::files::userInterface};

    appSettings.beginGroup(preferences::keys::systemTrayGroup);

    const QVariant resultData =
        appSettings.value(preferences::keys::startMinimizedToSystemTray);

    appSettings.endGroup();

    if (resultData.isValid()) {
        result = resultData.toBool();
        QNTRACE(
            "tray::SystemTrayIconManager",
            "Value from settings for the current account: "
                << (result ? "true" : "false"));
    }
    else {
        QNTRACE(
            "tray::SystemTrayIconManager",
            "Found no stored setting, will use the default value: "
                << (result ? "true" : "false"));
    }

    return result;
}

SystemTrayIconManager::TrayAction SystemTrayIconManager::singleClickTrayAction()
    const
{
    TrayAction action = gDefaultSingleClickTrayAction;

    ApplicationSettings appSettings{
        m_accountManager.currentAccount(),
        preferences::keys::files::userInterface};

    appSettings.beginGroup(preferences::keys::systemTrayGroup);

    const QVariant actionData =
        appSettings.value(preferences::keys::singleClickTrayAction);

    appSettings.endGroup();

    if (actionData.isValid()) {
        bool conversionResult = false;
        action = static_cast<TrayAction>(actionData.toInt(&conversionResult));
        if (Q_UNLIKELY(!conversionResult)) {
            QNWARNING(
                "tray::SystemTrayIconManager",
                "Can't read the left mouse button tray action: failed to "
                    << "convert the value read from settings to int: "
                    << actionData);
            action = gDefaultSingleClickTrayAction;
        }
        else {
            QNDEBUG(
                "tray::SystemTrayIconManager",
                "Action read from settings: " << action);
        }
    }

    return action;
}

SystemTrayIconManager::TrayAction SystemTrayIconManager::middleClickTrayAction()
    const
{
    TrayAction action = gDefaultMiddleClickTrayAction;

    ApplicationSettings appSettings{
        m_accountManager.currentAccount(),
        preferences::keys::files::userInterface};

    appSettings.beginGroup(preferences::keys::systemTrayGroup);

    const QVariant actionData =
        appSettings.value(preferences::keys::middleClickTrayAction);

    appSettings.endGroup();

    if (actionData.isValid()) {
        bool conversionResult = false;
        action = static_cast<TrayAction>(actionData.toInt(&conversionResult));
        if (Q_UNLIKELY(!conversionResult)) {
            QNWARNING(
                "tray::SystemTrayIconManager",
                "Can't read the middle mouse button tray action: failed to "
                    << "convert the value read from settings to int: "
                    << actionData);
            action = gDefaultMiddleClickTrayAction;
        }
        else {
            QNDEBUG(
                "tray::SystemTrayIconManager",
                "Action read from settings: " << action);
        }
    }

    return action;
}

SystemTrayIconManager::TrayAction SystemTrayIconManager::doubleClickTrayAction()
    const
{
    TrayAction action = gDefaultDoubleClickTrayAction;

    ApplicationSettings appSettings{
        m_accountManager.currentAccount(),
        preferences::keys::files::userInterface};

    appSettings.beginGroup(preferences::keys::systemTrayGroup);

    const QVariant actionData =
        appSettings.value(preferences::keys::doubleClickTrayAction);

    appSettings.endGroup();

    if (actionData.isValid()) {
        bool conversionResult = false;
        action = static_cast<TrayAction>(actionData.toInt(&conversionResult));
        if (Q_UNLIKELY(!conversionResult)) {
            QNWARNING(
                "tray::SystemTrayIconManager",
                "Can't read the right mouse button tray action: failed to "
                    << "convert the value read from settings to int: "
                    << actionData);
            action = gDefaultDoubleClickTrayAction;
        }
        else {
            QNDEBUG(
                "tray::SystemTrayIconManager",
                "Action read from settings: " << action);
        }
    }

    return action;
}

void SystemTrayIconManager::onSystemTrayIconActivated(
    QSystemTrayIcon::ActivationReason reason)
{
    QNDEBUG(
        "tray::SystemTrayIconManager",
        "SystemTrayIconManager::onSystemTrayIconActivated: reason = "
            << reason);

    std::string_view key;
    TrayAction defaultAction;
    bool shouldShowContextMenu = false;

    switch (reason) {
    case QSystemTrayIcon::Trigger:
        key = preferences::keys::singleClickTrayAction;
        defaultAction = gDefaultSingleClickTrayAction;
        break;
    case QSystemTrayIcon::MiddleClick:
        key = preferences::keys::middleClickTrayAction;
        defaultAction = gDefaultMiddleClickTrayAction;
        break;
    case QSystemTrayIcon::DoubleClick:
        key = preferences::keys::doubleClickTrayAction;
        defaultAction = gDefaultDoubleClickTrayAction;
        break;
    case QSystemTrayIcon::Context:
        shouldShowContextMenu = true;
        break;
    default:
    {
        QNINFO(
            "tray::SystemTrayIconManager",
            "Unidentified activation reason for the system tray icon: "
                << reason);
        return;
    }
    }

    if (shouldShowContextMenu) {
        if (Q_UNLIKELY(!m_trayIconContextMenu)) {
            QNWARNING(
                "tray::SystemTrayIconManager",
                "Can't show the tray icon context menu: context menu is null");
            return;
        }

        m_trayIconContextMenu->exec(QCursor::pos());
        return;
    }

    TrayAction action = defaultAction;

    ApplicationSettings appSettings{
        m_accountManager.currentAccount(),
        preferences::keys::files::userInterface};

    appSettings.beginGroup(preferences::keys::systemTrayGroup);
    const QVariant actionData = appSettings.value(key);
    appSettings.endGroup();

    if (actionData.isValid()) {
        bool conversionResult = false;
        action = static_cast<TrayAction>(actionData.toInt(&conversionResult));
        if (Q_UNLIKELY(!conversionResult)) {
            QNWARNING(
                "tray::SystemTrayIconManager",
                "Can't read the tray action setting per activation reason: "
                    << "failed to convert the value read from settings to int: "
                    << actionData);
            action = defaultAction;
        }
        else {
            QNDEBUG(
                "tray::SystemTrayIconManager",
                "Action for setting " << key.data() << ": " << action);
        }
    }

    switch (action) {
    case TrayActionDoNothing:
    {
        QNDEBUG(
            "tray::SystemTrayIconManager",
            "The action is \"do nothing\", obeying");
        return;
    }
    case TrayActionShowHide:
    {
        auto * mainWindow = qobject_cast<QWidget *>(parent());
        if (Q_UNLIKELY(!mainWindow)) {
            QNWARNING(
                "tray::SystemTrayIconManager",
                "Can't show/hide the main window from system tray: can't cast "
                "parent of SystemTrayIconManager to QWidget");
            return;
        }

        if (mainWindow->isHidden()) {
            Q_EMIT showRequested();
        }
        else {
            Q_EMIT hideRequested();
        }

        break;
    }
    case TrayActionNewTextNote:
    {
        auto * mainWindow = qobject_cast<QWidget *>(parent());
        if (Q_UNLIKELY(!mainWindow)) {
            QNWARNING(
                "tray::SystemTrayIconManager",
                "Can't ensure the main window is shown on request to create a "
                "new text note from system tray: can't cast the parent of "
                "SystemTrayIconManager to QWidget");
        }
        else if (mainWindow->isHidden()) {
            Q_EMIT showRequested();
        }

        Q_EMIT newTextNoteAdditionRequested();
        break;
    }
    case TrayActionShowContextMenu:
    {
        if (Q_UNLIKELY(!m_trayIconContextMenu)) {
            QNWARNING(
                "tray::SystemTrayIconManager",
                "Can't show the tray icon context menu: context menu is null");
            return;
        }

        m_trayIconContextMenu->exec(QCursor::pos());
        break;
    }
    default:
    {
        QNWARNING(
            "tray::SystemTrayIconManager",
            "Detected unrecognized tray action: " << action);
        break;
    }
    }
}

void SystemTrayIconManager::onAccountSwitched(const Account & account)
{
    QNDEBUG(
        "tray::SystemTrayIconManager",
        "SystemTrayIconManager::onAccountSwitched: " << account);
    setupAccountsSubMenu();
}

void SystemTrayIconManager::onAccountUpdated(const Account & account)
{
    QNDEBUG(
        "tray::SystemTrayIconManager",
        "SystemTrayIconManager::onAccountUpdated: " << account);
    setupAccountsSubMenu();
}

void SystemTrayIconManager::onAccountAdded(const Account & account)
{
    QNDEBUG(
        "tray::SystemTrayIconManager",
        "SystemTrayIconManager::onAccountAdded: " << account);
    setupAccountsSubMenu();
}

void SystemTrayIconManager::onAccountRemoved(const Account & account)
{
    QNDEBUG(
        "tray::SystemTrayIconManager",
        "SystemTrayIconManager::onAccountRemoved: " << account);
    setupAccountsSubMenu();
}

void SystemTrayIconManager::onNewTextNoteContextMenuAction()
{
    QNDEBUG(
        "tray::SystemTrayIconManager",
        "SystemTrayIconManager::onNewTextNoteContextMenuAction");
    Q_EMIT newTextNoteAdditionRequested();
}

void SystemTrayIconManager::onSwitchAccountContextMenuAction(const bool checked)
{
    QNDEBUG(
        "tray::SystemTrayIconManager",
        "SystemTrayIconManager::onSwitchAccountContextMenuAction: checked = "
            << (checked ? "true" : "false"));

    if (!checked) {
        QNTRACE(
            "tray::SystemTrayIconManager",
            "Ignoring the unchecking of account");
        return;
    }

    auto * action = qobject_cast<QAction *>(sender());
    if (Q_UNLIKELY(!action)) {
        ErrorString errorDescription{
            QT_TR_NOOP("Internal error: account switching action is "
                       "unexpectedly null")};
        QNWARNING("tray::SystemTrayIconManager", errorDescription);
        Q_EMIT notifyError(errorDescription);
        return;
    }

    auto indexData = action->data();
    bool conversionResult = false;
    const int index = indexData.toInt(&conversionResult);
    if (Q_UNLIKELY(!conversionResult)) {
        ErrorString errorDescription{
            QT_TR_NOOP("Internal error: can't get identification data from "
                       "the account switching action")};
        QNWARNING("tray::SystemTrayIconManager", errorDescription);
        Q_EMIT notifyError(errorDescription);
        return;
    }

    const auto & availableAccounts = m_accountManager.availableAccounts();
    const int numAvailableAccounts = availableAccounts.size();

    if (index < 0 || index >= numAvailableAccounts) {
        ErrorString errorDescription{
            QT_TR_NOOP("Internal error: wrong index into available accounts "
                       "in account switching action")};
        QNWARNING("tray::SystemTrayIconManager", errorDescription);
        Q_EMIT notifyError(errorDescription);
        return;
    }

    const auto & availableAccount = availableAccounts[index];

    QNTRACE(
        "tray::SystemTrayIconManager",
        "Emitting the request to switch account: " << availableAccount);

    Q_EMIT accountSwitchRequested(availableAccount);
}

void SystemTrayIconManager::onShowMainWindowContextMenuAction()
{
    QNDEBUG(
        "tray::SystemTrayIconManager",
        "SystemTrayIconManager::onShowMainWindowContextMenuAction");
    onShowHideMainWindowContextMenuAction(/* show = */ true);
}

void SystemTrayIconManager::onHideMainWindowContextMenuAction()
{
    QNDEBUG(
        "tray::SystemTrayIconManager",
        "SystemTrayIconManager::onHideMainWindowContextMenuAction");
    onShowHideMainWindowContextMenuAction(/* show = */ false);
}

void SystemTrayIconManager::onSwitchTrayIconContextMenuAction(
    const bool checked)
{
    QNDEBUG(
        "tray::SystemTrayIconManager",
        "SystemTrayIconManager::onSwitchTrayIconContextMenuAction: checked = "
            << (checked ? "true" : "false"));

    if (!checked) {
        QNTRACE(
            "tray::SystemTrayIconManager",
            "Ignoring the unchecking of current tray icon kind");
        return;
    }

    auto * action = qobject_cast<QAction *>(sender());
    if (Q_UNLIKELY(!action)) {
        ErrorString errorDescription{
            QT_TR_NOOP("Internal error: tray icon kind switching action is "
                       "unexpectedly null")};
        QNWARNING("tray::SystemTrayIconManager", errorDescription);
        Q_EMIT notifyError(errorDescription);
        return;
    }

    ApplicationSettings appSettings{
        m_accountManager.currentAccount(),
        preferences::keys::files::userInterface};

    appSettings.beginGroup(preferences::keys::systemTrayGroup);
    appSettings.setValue(
        preferences::keys::systemTrayIconKind, action->data().toString());
    appSettings.endGroup();

    setupSystemTrayIcon();
}

void SystemTrayIconManager::onQuitContextMenuAction()
{
    QNDEBUG(
        "tray::SystemTrayIconManager",
        "SystemTrayIconManager::onQuitContextMenuAction");
    Q_EMIT quitRequested();
}

void SystemTrayIconManager::onMainWindowShown()
{
    QNDEBUG(
        "tray::SystemTrayIconManager",
        "SystemTrayIconManager::onMainWindowShown");
    evaluateShowHideMenuActions();
}

void SystemTrayIconManager::onMainWindowHidden()
{
    QNDEBUG(
        "tray::SystemTrayIconManager",
        "SystemTrayIconManager::onMainWindowHidden");
    evaluateShowHideMenuActions();
}

void SystemTrayIconManager::createConnections()
{
    QNDEBUG(
        "tray::SystemTrayIconManager",
        "SystemTrayIconManager::createConnections");

    // AccountManager connections
    QObject::connect(
        &m_accountManager, &AccountManager::switchedAccount, this,
        &SystemTrayIconManager::onAccountSwitched);

    QObject::connect(
        this, &SystemTrayIconManager::switchAccount, &m_accountManager,
        &AccountManager::switchAccount);

    QObject::connect(
        &m_accountManager, &AccountManager::accountUpdated, this,
        &SystemTrayIconManager::onAccountUpdated);

    QObject::connect(
        &m_accountManager, &AccountManager::accountAdded, this,
        &SystemTrayIconManager::onAccountAdded);

    QObject::connect(
        &m_accountManager, &AccountManager::accountRemoved, this,
        &SystemTrayIconManager::onAccountRemoved);
}

void SystemTrayIconManager::setupSystemTrayIcon()
{
    if (!m_systemTrayIcon) {
        m_systemTrayIcon = new QSystemTrayIcon{this};

        QObject::connect(
            m_systemTrayIcon, &QSystemTrayIcon::activated, this,
            &SystemTrayIconManager::onSystemTrayIconActivated);
    }

    ApplicationSettings appSettings{
        m_accountManager.currentAccount(),
        preferences::keys::files::userInterface};

    appSettings.beginGroup(preferences::keys::systemTrayGroup);

    QString trayIconKind =
        appSettings.value(preferences::keys::systemTrayIconKind).toString();

    appSettings.endGroup();

    if (trayIconKind.isEmpty()) {
        QNDEBUG(
            "tray::SystemTrayIconManager",
            "The tray icon kind is empty, will use the default tray icon");
        trayIconKind = QString::fromUtf8(
            preferences::defaults::trayIconKind.data(),
            preferences::defaults::trayIconKind.size());
    }
    else if (trayIconKind == QStringLiteral("dark")) {
        QNDEBUG(
            "tray::SystemTrayIconManager",
            "Will use the simple dark tray icon");
    }
    else if (trayIconKind == QStringLiteral("light")) {
        QNDEBUG(
            "tray::SystemTrayIconManager",
            "Will use the simple light tray icon");
    }
    else if (trayIconKind == QStringLiteral("colored")) {
        QNDEBUG(
            "tray::SystemTrayIconManager", "Will use the colored tray icon");
    }
    else {
        QNDEBUG(
            "tray::SystemTrayIconManager",
            "Unidentified tray icon kind ("
                << trayIconKind << ", will fallback to the default");
        trayIconKind = QString::fromUtf8(
            preferences::defaults::trayIconKind.data(),
            preferences::defaults::trayIconKind.size());
    }

    QString whichIcon;
    if (trayIconKind == QStringLiteral("dark")) {
        whichIcon = QStringLiteral("_simple_dark");
    }
    else if (trayIconKind == QStringLiteral("light")) {
        whichIcon = QStringLiteral("_simple_light");
    }

    QIcon icon;

    const auto addIcon = [&icon, whichIcon](const int size) {
        QString str;
        QTextStream strm{&str};

        strm << ":/app_icons/quentier_icon" << whichIcon << "_" << size
             << ".png";
        strm.flush();

        icon.addFile(str, QSize{size, size});
    };

    addIcon(512);
    addIcon(256);
    addIcon(128);
    addIcon(64);
    addIcon(48);
    addIcon(32);
    addIcon(16);

    m_systemTrayIcon->setIcon(icon);
}

void SystemTrayIconManager::setupContextMenu()
{
    QNDEBUG(
        "tray::SystemTrayIconManager",
        "SystemTrayIconManager::setupContextMenu");

    auto * mainWindow = qobject_cast<QWidget *>(parent());
    if (Q_UNLIKELY(!mainWindow)) {
        ErrorString errorDescription{
            QT_TR_NOOP("Can't set up the tray icon's context menu: internal "
                       "error, the parent of SystemTrayIconManager is not a "
                       "QWidget")};

        QNWARNING("tray::SystemTrayIconManager", errorDescription);
        Q_EMIT notifyError(std::move(errorDescription));

        if (m_systemTrayIcon) {
            m_systemTrayIcon->setContextMenu(nullptr);
        }

        return;
    }

    if (!isSystemTrayAvailable()) {
        QNDEBUG(
            "tray::SystemTrayIconManager",
            "The system tray is not available, can't set up the context menu "
            "for the system tray icon");

        if (m_systemTrayIcon) {
            m_systemTrayIcon->setContextMenu(nullptr);
        }

        return;
    }

    if (!m_trayIconContextMenu) {
        m_trayIconContextMenu = new QMenu(qobject_cast<QWidget *>(parent()));
    }
    else {
        m_trayIconContextMenu->clear();
    }

    addContextMenuAction(
        tr("New text note"), *m_trayIconContextMenu, this,
        [this] { onNewTextNoteContextMenuAction(); }, {}, ActionState::Enabled);

    m_trayIconContextMenu->addSeparator();

    setupAccountsSubMenu();

    m_trayIconContextMenu->addSeparator();

    addContextMenuAction(
        tr("Show"), *m_trayIconContextMenu, this,
        [this] { onShowMainWindowContextMenuAction(); }, {},
        mainWindow->isHidden() ? ActionState::Enabled : ActionState::Disabled);

    addContextMenuAction(
        tr("Hide"), *m_trayIconContextMenu, this,
        [this] { onHideMainWindowContextMenuAction(); }, {},
        mainWindow->isHidden() ? ActionState::Disabled : ActionState::Enabled);

    m_trayIconContextMenu->addSeparator();

    setupTrayIconKindSubMenu();

    m_trayIconContextMenu->addSeparator();

    addContextMenuAction(
        tr("Quit"), *m_trayIconContextMenu, this,
        [this] { onQuitContextMenuAction(); }, {}, ActionState::Enabled);

    if (!m_systemTrayIcon) {
        setupSystemTrayIcon();
    }

    m_systemTrayIcon->setContextMenu(m_trayIconContextMenu);
}

void SystemTrayIconManager::setupAccountsSubMenu()
{
    QNDEBUG(
        "tray::SystemTrayIconManager",
        "SystemTrayIconManager::setupAccountsSubMenu");

    if (Q_UNLIKELY(!m_trayIconContextMenu)) {
        QNDEBUG(
            "tray::SystemTrayIconManager", "No primary tray icon context menu");
        return;
    }

    if (!m_accountsTrayIconSubMenu) {
        m_accountsTrayIconSubMenu =
            m_trayIconContextMenu->addMenu(tr("Switch account"));
    }
    else {
        m_accountsTrayIconSubMenu->clear();
    }

    delete m_availableAccountsActionGroup;
    m_availableAccountsActionGroup = new QActionGroup{this};
    m_availableAccountsActionGroup->setExclusive(true);

    auto currentAccount = m_accountManager.currentAccount();
    const auto & availableAccounts = m_accountManager.availableAccounts();

    for (int i = 0, size = availableAccounts.size(); i < size; ++i) {
        const auto & availableAccount = availableAccounts[i];
        QString availableAccountRepresentationName = availableAccount.name();

        if (availableAccount.type() == Account::Type::Local) {
            availableAccountRepresentationName += QStringLiteral(" (");
            availableAccountRepresentationName += tr("local");
            availableAccountRepresentationName += QStringLiteral(")");
        }

        auto * action =
            new QAction{availableAccountRepresentationName, nullptr};

        m_accountsTrayIconSubMenu->addAction(action);
        action->setData(i);
        action->setCheckable(true);

        if (availableAccount == currentAccount) {
            action->setChecked(true);
        }

        QObject::connect(
            action, &QAction::triggered, this,
            &SystemTrayIconManager::onSwitchAccountContextMenuAction);

        m_availableAccountsActionGroup->addAction(action);
    }
}

void SystemTrayIconManager::setupTrayIconKindSubMenu()
{
    QNDEBUG(
        "tray::SystemTrayIconManager",
        "SystemTrayIconManager::setupTrayIconKindSubMenu");

    if (Q_UNLIKELY(!m_trayIconContextMenu)) {
        QNDEBUG(
            "tray::SystemTrayIconManager", "No primary tray icon context menu");
        return;
    }

    if (!m_trayIconKindSubMenu) {
        m_trayIconKindSubMenu =
            m_trayIconContextMenu->addMenu(tr("Tray icon kind"));
    }
    else {
        m_trayIconKindSubMenu->clear();
    }

    delete m_trayIconKindsActionGroup;
    m_trayIconKindsActionGroup = new QActionGroup(this);
    m_trayIconKindsActionGroup->setExclusive(true);

    QString currentTrayIconKind = QString::fromUtf8(
        preferences::defaults::trayIconKind.data(),
        preferences::defaults::trayIconKind.size());

    ApplicationSettings appSettings{
        m_accountManager.currentAccount(),
        preferences::keys::files::userInterface};

    appSettings.beginGroup(preferences::keys::systemTrayGroup);

    if (appSettings.contains(preferences::keys::systemTrayIconKind)) {
        currentTrayIconKind =
            appSettings.value(preferences::keys::systemTrayIconKind).toString();
    }

    appSettings.endGroup();

    if (currentTrayIconKind != QStringLiteral("dark") &&
        currentTrayIconKind != QStringLiteral("light") &&
        currentTrayIconKind != QStringLiteral("colored"))
    {
        QNDEBUG(
            "tray::SystemTrayIconManager",
            "Wrong/unrecognized value of current tray icon kind setting: "
                << currentTrayIconKind << ", fallback to default");

        currentTrayIconKind = QString::fromUtf8(
            preferences::defaults::trayIconKind.data(),
            preferences::defaults::trayIconKind.size());
    }

    QNDEBUG(
        "tray::SystemTrayIconManager",
        "Current tray icon kind = " << currentTrayIconKind);

    QStringList actionNames;
    actionNames << QStringLiteral("dark") << QStringLiteral("light")
                << QStringLiteral("colored");

    for (const auto & actionName: qAsConst(actionNames)) {
        auto * action = new QAction{actionName, nullptr};
        m_trayIconKindSubMenu->addAction(action);
        action->setData(actionName);
        action->setCheckable(true);
        action->setChecked(actionName == currentTrayIconKind);

        QObject::connect(
            action, &QAction::triggered, this,
            &SystemTrayIconManager::onSwitchTrayIconContextMenuAction);

        m_trayIconKindsActionGroup->addAction(action);
    }
}

void SystemTrayIconManager::evaluateShowHideMenuActions()
{
    QNDEBUG(
        "tray::SystemTrayIconManager",
        "SystemTrayIconManager::evaluateShowHideMenuActions");

    if (Q_UNLIKELY(!m_trayIconContextMenu)) {
        QNDEBUG("tray::SystemTrayIconManager", "No tray icon context menu");
        return;
    }

    auto * mainWindow = qobject_cast<QWidget *>(parent());
    if (Q_UNLIKELY(!mainWindow)) {
        QNDEBUG("tray::SystemTrayIconManager", "Parent is not QWidget");
        return;
    }

    auto actions = m_trayIconContextMenu->actions();
    QAction * showAction = nullptr;
    QAction * hideAction = nullptr;

    QString showText = tr("Show");
    QString hideText = tr("Hide");

    for (auto * action: std::as_const(actions)) {
        if (Q_UNLIKELY(!action)) {
            continue;
        }

        QString text = action->text();
        // NOTE: required to workaround
        // https://bugs.kde.org/show_bug.cgi?id=337491
        text.remove(QChar::fromLatin1('&'), Qt::CaseInsensitive);

        if (text == showText) {
            showAction = action;
        }
        else if (text == hideText) {
            hideAction = action;
        }

        if (showAction && hideAction) {
            break;
        }
    }

    bool mainWindowIsVisible = mainWindow->isVisible();
    const Qt::WindowStates mainWindowState = mainWindow->windowState();
    const bool mainWindowIsMinimized = (mainWindowState & Qt::WindowMinimized);
    if (mainWindowIsMinimized) {
        mainWindowIsVisible = false;
    }

    QNDEBUG(
        "tray::SystemTrayIconManager",
        "Main window is minimized: "
            << (mainWindowIsMinimized ? "true" : "false")
            << ", main window is visible: "
            << (mainWindowIsVisible ? "true" : "false"));

    if (showAction) {
        showAction->setEnabled(!mainWindowIsVisible);
        QNDEBUG(
            "tray::SystemTrayIconManager",
            "Show action is "
                << (showAction->isEnabled() ? "enabled" : "disabled"));
    }
    else {
        QNDEBUG("tray::SystemTrayIconManager", "Show action was not found");
    }

    if (hideAction) {
        hideAction->setEnabled(mainWindowIsVisible);
        QNDEBUG(
            "tray::SystemTrayIconManager",
            "Hide action is "
                << (hideAction->isEnabled() ? "enabled" : "disabled"));
    }
    else {
        QNDEBUG("tray::SystemTrayIconManager", "Hide action was not found");
    }
}

void SystemTrayIconManager::onShowHideMainWindowContextMenuAction(
    const bool show)
{
    auto * mainWindow = qobject_cast<QWidget *>(parent());
    if (Q_UNLIKELY(!mainWindow)) {
        ErrorString errorDescription{
            QT_TR_NOOP("Can't show/hide the main window: internal error, "
                       "the parent of SystemTrayIconManager is not a QWidget")};
        QNWARNING("tray::SystemTrayIconManager", errorDescription);
        Q_EMIT notifyError(std::move(errorDescription));
        return;
    }

    bool mainWindowIsVisible = mainWindow->isVisible();
    Qt::WindowStates mainWindowState = mainWindow->windowState();
    const bool mainWindowIsMinimized = (mainWindowState & Qt::WindowMinimized);
    if (mainWindowIsMinimized) {
        mainWindowIsVisible = false;
    }

    if (show && mainWindowIsVisible) {
        QNDEBUG(
            "tray::SystemTrayIconManager",
            "The main window is already shown, nothing to do");
        return;
    }
    else if (!show && !mainWindowIsVisible) {
        QNDEBUG(
            "tray::SystemTrayIconManager",
            "The main window is already hidden, nothing to do");
        return;
    }

    if (show) {
        if (mainWindowIsMinimized) {
            mainWindowState = mainWindowState & (~Qt::WindowMinimized);
            mainWindow->setWindowState(mainWindowState);
        }

        if (!mainWindow->isVisible()) {
            Q_EMIT showRequested();
        }
    }
    else {
        Q_EMIT hideRequested();
    }
}

void SystemTrayIconManager::persistTrayIconState()
{
    QNDEBUG(
        "tray::SystemTrayIconManager",
        "SystemTrayIconManager::persistTrayIconState");

    ApplicationSettings appSettings{
        m_accountManager.currentAccount(),
        preferences::keys::files::userInterface};

    appSettings.beginGroup(preferences::keys::systemTrayGroup);

    appSettings.setValue(
        preferences::keys::showSystemTrayIcon, QVariant{isShown()});

    appSettings.endGroup();
}

void SystemTrayIconManager::restoreTrayIconState()
{
    QNDEBUG(
        "tray::SystemTrayIconManager",
        "SystemTrayIconManager::restoreTrayIconState");

    if (!isSystemTrayAvailable()) {
        QNDEBUG(
            "tray::SystemTrayIconManager",
            "The system tray is not available, won't show the system tray "
            "icon");
        hide();
        return;
    }

    ApplicationSettings appSettings{
        m_accountManager.currentAccount(),
        preferences::keys::files::userInterface};

    appSettings.beginGroup(preferences::keys::systemTrayGroup);
    const QVariant data =
        appSettings.value(preferences::keys::showSystemTrayIcon);
    appSettings.endGroup();

    bool shouldShow = preferences::defaults::showSystemTrayIcon;
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
