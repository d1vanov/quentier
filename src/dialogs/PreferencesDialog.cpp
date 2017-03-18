#include "PreferencesDialog.h"
#include "ui_PreferencesDialog.h"
#include "../AccountManager.h"
#include "../SystemTrayIconManager.h"
#include "../SettingsNames.h"
#include "../DefaultSettings.h"
#include <quentier/logging/QuentierLogger.h>
#include <quentier/utility/ApplicationSettings.h>
#include <QStringListModel>

namespace quentier {

QString trayActionToString(SystemTrayIconManager::TrayAction action);

PreferencesDialog::PreferencesDialog(AccountManager & accountManager,
                                     SystemTrayIconManager & systemTrayIconManager,
                                     QWidget *parent) :
    QDialog(parent),
    m_pUi(new Ui::PreferencesDialog),
    m_accountManager(accountManager),
    m_systemTrayIconManager(systemTrayIconManager),
    m_pTrayActionsModel(new QStringListModel(this))
{
    m_pUi->setupUi(this);

    setWindowTitle(tr("Preferences"));

#ifdef Q_WS_MAC
    // It makes little sense to minimize to tray on Mac
    // because the minimized app goes to dock
    m_pUi->minimizeToTrayCheckBox->setDisabled(true);
    m_pUi->minimizeToTrayCheckBox->setHidden(true);
#endif

    setupCurrentSettingsState();
    createConnections();
}

PreferencesDialog::~PreferencesDialog()
{
    delete m_pUi;
}

void PreferencesDialog::onShowSystemTrayIconCheckboxToggled(bool checked)
{
    QNDEBUG(QStringLiteral("PreferencesDialog::onShowSystemTrayIconCheckboxToggled: checked = ")
            << (checked ? QStringLiteral("true") : QStringLiteral("false")));

    Account currentAccount = m_accountManager.currentAccount();
    ApplicationSettings appSettings(currentAccount, QUENTIER_UI_SETTINGS);
    appSettings.beginGroup(SYSTEM_TRAY_SETTINGS_GROUP_NAME);
    appSettings.setValue(SHOW_SYSTEM_TRAY_ICON_SETTINGS_KEY, checked);
    appSettings.endGroup();

    emit showSystemTrayIconOptionChanged(checked);
}

void PreferencesDialog::onCloseToSystemTrayCheckboxToggled(bool checked)
{
    QNDEBUG(QStringLiteral("PreferencesDialog::onCloseToSystemTrayCheckboxToggled: checked = ")
            << (checked ? QStringLiteral("true") : QStringLiteral("false")));

    Account currentAccount = m_accountManager.currentAccount();
    ApplicationSettings appSettings(currentAccount, QUENTIER_UI_SETTINGS);
    appSettings.beginGroup(SYSTEM_TRAY_SETTINGS_GROUP_NAME);
    appSettings.setValue(CLOSE_TO_SYSTEM_TRAY_SETTINGS_KEY, checked);
    appSettings.endGroup();
}

void PreferencesDialog::onMinimizeToSystemTrayCheckboxToggled(bool checked)
{
    QNDEBUG(QStringLiteral("PreferencesDialog::onMinimizeToSystemTrayCheckboxToggled: checked = ")
            << (checked ? QStringLiteral("true") : QStringLiteral("false")));

    Account currentAccount = m_accountManager.currentAccount();
    ApplicationSettings appSettings(currentAccount, QUENTIER_UI_SETTINGS);
    appSettings.beginGroup(SYSTEM_TRAY_SETTINGS_GROUP_NAME);
    appSettings.setValue(MINIMIZE_TO_SYSTEM_TRAY_SETTINGS_KEY, checked);
    appSettings.endGroup();
}

void PreferencesDialog::onStartMinimizedToSystemTrayCheckboxToggled(bool checked)
{
    QNDEBUG(QStringLiteral("PreferencesDialog::onStartMinimizedToSystemTrayCheckboxToggled: checked = ")
            << (checked ? QStringLiteral("true") : QStringLiteral("false")));

    Account currentAccount = m_accountManager.currentAccount();
    ApplicationSettings appSettings(currentAccount, QUENTIER_UI_SETTINGS);
    appSettings.beginGroup(SYSTEM_TRAY_SETTINGS_GROUP_NAME);
    appSettings.setValue(START_MINIMIZED_TO_SYSTEM_TRAY_SETTINGS_KEY, checked);
    appSettings.endGroup();
}

void PreferencesDialog::onSingleClickTrayActionChanged(int action)
{
    QNDEBUG(QStringLiteral("PreferencesDialog::onSingleClickTrayActionChanged: ") << action);

    Account currentAccount = m_accountManager.currentAccount();
    ApplicationSettings appSettings(currentAccount, QUENTIER_UI_SETTINGS);
    appSettings.beginGroup(SYSTEM_TRAY_SETTINGS_GROUP_NAME);
    appSettings.setValue(SINGLE_CLICK_TRAY_ACTION_SETTINGS_KEY, action);
    appSettings.endGroup();
}

void PreferencesDialog::onMiddleClickTrayActionChanged(int action)
{
    QNDEBUG(QStringLiteral("PreferencesDialog::onMiddleClickTrayActionChanged: ") << action);

    Account currentAccount = m_accountManager.currentAccount();
    ApplicationSettings appSettings(currentAccount, QUENTIER_UI_SETTINGS);
    appSettings.beginGroup(SYSTEM_TRAY_SETTINGS_GROUP_NAME);
    appSettings.setValue(MIDDLE_CLICK_TRAY_ACTION_SETTINGS_KEY, action);
    appSettings.endGroup();
}

void PreferencesDialog::onDoubleClickTrayActionChanged(int action)
{
    QNDEBUG(QStringLiteral("PreferencesDialog::onDoubleClickTrayActionChanged: ") << action);

    Account currentAccount = m_accountManager.currentAccount();
    ApplicationSettings appSettings(currentAccount, QUENTIER_UI_SETTINGS);
    appSettings.beginGroup(SYSTEM_TRAY_SETTINGS_GROUP_NAME);
    appSettings.setValue(DOUBLE_CLICK_TRAY_ACTION_SETTINGS_KEY, action);
    appSettings.endGroup();
}

void PreferencesDialog::setupCurrentSettingsState()
{
    QNDEBUG(QStringLiteral("PreferencesDialog::setupCurrentSettingsState"));

    Account currentAccount = m_accountManager.currentAccount();
    ApplicationSettings appSettings(currentAccount, QUENTIER_UI_SETTINGS);
    appSettings.beginGroup(SYSTEM_TRAY_SETTINGS_GROUP_NAME);

    bool shouldShowSystemTrayIcon = DEFAULT_SHOW_SYSTEM_TRAY_ICON;
    QVariant shouldShowSystemTrayIconData = appSettings.value(SHOW_SYSTEM_TRAY_ICON_SETTINGS_KEY);
    if (shouldShowSystemTrayIconData.isValid()) {
        shouldShowSystemTrayIcon = shouldShowSystemTrayIconData.toBool();
    }

    bool shouldCloseToTray = DEFAULT_CLOSE_TO_SYSTEM_TRAY;
    QVariant shouldCloseToTrayData = appSettings.value(CLOSE_TO_SYSTEM_TRAY_SETTINGS_KEY);
    if (shouldCloseToTrayData.isValid()) {
        shouldCloseToTray = shouldCloseToTrayData.toBool();
    }

    bool shouldMinimizeToTray = DEFAULT_MINIMIZE_TO_SYSTEM_TRAY;
    QVariant shouldMinimizeToTrayData = appSettings.value(MINIMIZE_TO_SYSTEM_TRAY_SETTINGS_KEY);
    if (shouldMinimizeToTrayData.isValid()) {
        shouldMinimizeToTray = shouldMinimizeToTrayData.toBool();
    }

    bool shouldStartMinimizedToTray = DEFAULT_START_MINIMIZED_TO_SYSTEM_TRAY;
    QVariant shouldStartMinimizedToTrayData = appSettings.value(START_MINIMIZED_TO_SYSTEM_TRAY_SETTINGS_KEY);
    if (shouldStartMinimizedToTrayData.isValid()) {
        shouldStartMinimizedToTray = shouldStartMinimizedToTrayData.toBool();
    }

    appSettings.endGroup();

    m_pUi->showSystemTrayIconCheckBox->setChecked(shouldShowSystemTrayIcon);
    m_pUi->closeToTrayCheckBox->setChecked(shouldCloseToTray);
    m_pUi->minimizeToTrayCheckBox->setChecked(shouldMinimizeToTray);
    m_pUi->startFromTrayCheckBox->setChecked(shouldStartMinimizedToTray);

    QStringList trayActions;
    trayActions.reserve(4);
    trayActions.push_back(trayActionToString(SystemTrayIconManager::TrayActionDoNothing));
    trayActions.push_back(trayActionToString(SystemTrayIconManager::TrayActionShowHide));
    trayActions.push_back(trayActionToString(SystemTrayIconManager::TrayActionNewTextNote));
    trayActions.push_back(trayActionToString(SystemTrayIconManager::TrayActionShowContextMenu));
    m_pTrayActionsModel->setStringList(trayActions);

    m_pUi->traySingleClickActionComboBox->setModel(m_pTrayActionsModel);
    m_pUi->trayMiddleClickActionComboBox->setModel(m_pTrayActionsModel);
    m_pUi->trayDoubleClickActionComboBox->setModel(m_pTrayActionsModel);

    SystemTrayIconManager::TrayAction singleClickTrayAction = m_systemTrayIconManager.singleClickTrayAction();
    m_pUi->traySingleClickActionComboBox->setCurrentIndex(singleClickTrayAction);

    SystemTrayIconManager::TrayAction middleClickTrayAction = m_systemTrayIconManager.middleClickTrayAction();
    m_pUi->trayMiddleClickActionComboBox->setCurrentIndex(middleClickTrayAction);

    SystemTrayIconManager::TrayAction doubleClickTrayAction = m_systemTrayIconManager.doubleClickTrayAction();
    m_pUi->trayDoubleClickActionComboBox->setCurrentIndex(doubleClickTrayAction);
}

void PreferencesDialog::createConnections()
{
    QNDEBUG(QStringLiteral("PreferencesDialog::createConnections"));

    QObject::connect(m_pUi->okPushButton, QNSIGNAL(QPushButton,clicked),
                     this, QNSLOT(PreferencesDialog,accept));

    QObject::connect(m_pUi->showSystemTrayIconCheckBox, QNSIGNAL(QCheckBox,toggled,bool),
                     this, QNSLOT(PreferencesDialog,onShowSystemTrayIconCheckboxToggled,bool));
    QObject::connect(m_pUi->closeToTrayCheckBox, QNSIGNAL(QCheckBox,toggled,bool),
                     this, QNSLOT(PreferencesDialog,onCloseToSystemTrayCheckboxToggled,bool));
    QObject::connect(m_pUi->minimizeToTrayCheckBox, QNSIGNAL(QCheckBox,toggled,bool),
                     this, QNSLOT(PreferencesDialog,onMinimizeToSystemTrayCheckboxToggled,bool));
    QObject::connect(m_pUi->startFromTrayCheckBox, QNSIGNAL(QCheckBox,toggled,bool),
                     this, QNSLOT(PreferencesDialog,onStartMinimizedToSystemTrayCheckboxToggled,bool));

    QObject::connect(m_pUi->traySingleClickActionComboBox, SIGNAL(currentIndexChanged(int)),
                     this, SLOT(onSingleClickTrayActionChanged(int)));
    QObject::connect(m_pUi->trayMiddleClickActionComboBox, SIGNAL(currentIndexChanged(int)),
                     this, SLOT(onMiddleClickTrayActionChanged(int)));
    QObject::connect(m_pUi->trayDoubleClickActionComboBox, SIGNAL(currentIndexChanged(int)),
                     this, SLOT(onDoubleClickTrayActionChanged(int)));

    // TODO: continue
}

QString trayActionToString(SystemTrayIconManager::TrayAction action)
{
    switch(action)
    {
    case SystemTrayIconManager::TrayActionDoNothing:
        return PreferencesDialog::tr("Do nothing");
    case SystemTrayIconManager::TrayActionNewTextNote:
        return PreferencesDialog::tr("Create new text note");
    case SystemTrayIconManager::TrayActionShowContextMenu:
        return PreferencesDialog::tr("Show context menu");
    case SystemTrayIconManager::TrayActionShowHide:
        return PreferencesDialog::tr("Show/hide Quentier");
    default:
        return QString();
    }
}

} // namespace quentier
