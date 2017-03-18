#include "PreferencesDialog.h"
#include "ui_PreferencesDialog.h"
#include "../AccountManager.h"
#include "../SettingsNames.h"
#include "../DefaultSettings.h"
#include <quentier/logging/QuentierLogger.h>
#include <quentier/utility/ApplicationSettings.h>
#include <quentier/types/ErrorString.h>

namespace quentier {

PreferencesDialog::PreferencesDialog(AccountManager & accountManager,
                                     QWidget *parent) :
    QDialog(parent),
    m_pUi(new Ui::PreferencesDialog),
    m_pAccountManager(&accountManager)
{
    m_pUi->setupUi(this);
    m_pUi->statusTextLineEdit->hide();

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

    if (!m_pAccountManager.isNull())
    {
        Account currentAccount = m_pAccountManager->currentAccount();
        ApplicationSettings appSettings(currentAccount, QUENTIER_UI_SETTINGS);
        appSettings.beginGroup(SYSTEM_TRAY_SETTINGS_GROUP_NAME);
        appSettings.setValue(SHOW_SYSTEM_TRAY_ICON_SETTINGS_KEY, checked);
        appSettings.endGroup();
    }
    else
    {
        ErrorString error(QT_TRANSLATE_NOOP("", "Can't persist the show system tray icon option: "
                                                "the account manager is null"));
        QNWARNING(error);
        showError(error.localizedString());
    }

    emit showSystemTrayIconOptionChanged(checked);
}

void PreferencesDialog::onCloseToSystemTrayCheckboxToggled(bool checked)
{
    QNDEBUG(QStringLiteral("PreferencesDialog::onCloseToSystemTrayCheckboxToggled: checked = ")
            << (checked ? QStringLiteral("true") : QStringLiteral("false")));

    if (Q_UNLIKELY(m_pAccountManager.isNull()))
    {
        ErrorString error(QT_TRANSLATE_NOOP("", "Can't persist the close to system tray "
                                                "icon option: the account manager is null"));
        QNWARNING(error);
        showError(error.localizedString());
        return;
    }

    Account currentAccount = m_pAccountManager->currentAccount();
    ApplicationSettings appSettings(currentAccount, QUENTIER_UI_SETTINGS);
    appSettings.beginGroup(SYSTEM_TRAY_SETTINGS_GROUP_NAME);
    appSettings.setValue(CLOSE_TO_SYSTEM_TRAY_SETTINGS_KEY, checked);
    appSettings.endGroup();
}

void PreferencesDialog::onMinimizeToSystemTrayCheckboxToggled(bool checked)
{
    QNDEBUG(QStringLiteral("PreferencesDialog::onMinimizeToSystemTrayCheckboxToggled: checked = ")
            << (checked ? QStringLiteral("true") : QStringLiteral("false")));

    if (Q_UNLIKELY(m_pAccountManager.isNull()))
    {
        ErrorString error(QT_TRANSLATE_NOOP("", "Can't persist the minimize to system tray "
                                                "icon option: the account manager is null"));
        QNWARNING(error);
        showError(error.localizedString());
        return;
    }

    Account currentAccount = m_pAccountManager->currentAccount();
    ApplicationSettings appSettings(currentAccount, QUENTIER_UI_SETTINGS);
    appSettings.beginGroup(SYSTEM_TRAY_SETTINGS_GROUP_NAME);
    appSettings.setValue(MINIMIZE_TO_SYSTEM_TRAY_SETTINGS_KEY, checked);
    appSettings.endGroup();
}

void PreferencesDialog::onStartMinimizedToSystemTrayCheckboxToggled(bool checked)
{
    QNDEBUG(QStringLiteral("PreferencesDialog::onStartMinimizedToSystemTrayCheckboxToggled: checked = ")
            << (checked ? QStringLiteral("true") : QStringLiteral("false")));

    if (Q_UNLIKELY(m_pAccountManager.isNull())) {
        ErrorString error(QT_TRANSLATE_NOOP("", "Can't persist the minimize to system "
                                                "tray icon option: the account manager is null"));
        QNWARNING(error);
        showError(error.localizedString());
        return;
    }

    Account currentAccount = m_pAccountManager->currentAccount();
    ApplicationSettings appSettings(currentAccount, QUENTIER_UI_SETTINGS);
    appSettings.beginGroup(SYSTEM_TRAY_SETTINGS_GROUP_NAME);
    appSettings.setValue(START_MINIMIZED_TO_SYSTEM_TRAY_SETTINGS_KEY, checked);
    appSettings.endGroup();
}

void PreferencesDialog::setupCurrentSettingsState()
{
    QNDEBUG(QStringLiteral("PreferencesDialog::setupCurrentSettingsState"));

    // NOTE: since this method is only to be called from the constructor and the constructor is passed
    // AccountManager by reference, assume that by this moment the weak link to AccountManager won't expire

    Account currentAccount = m_pAccountManager->currentAccount();
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

    // TODO: continue
}

void PreferencesDialog::showError(const QString & error)
{
    QNDEBUG(QStringLiteral("PreferencesDialog::showError"));

    m_pUi->statusTextLineEdit->setText(error);
    m_pUi->statusTextLineEdit->show();
}

} // namespace quentier
