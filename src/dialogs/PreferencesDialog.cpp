#include "PreferencesDialog.h"
#include "ui_PreferencesDialog.h"
#include "../AccountManager.h"
#include "../SettingsNames.h"
#include <quentier/logging/QuentierLogger.h>
#include <quentier/utility/ApplicationSettings.h>

#define SHOW_SYSTEM_TRAY_ICON_SETTINGS_KEY QStringLiteral("ShowSystemTrayIcon")

namespace quentier {

PreferencesDialog::PreferencesDialog(AccountManager & accountManager,
                                     QWidget *parent) :
    QDialog(parent),
    m_pUi(new Ui::PreferencesDialog),
    m_pAccountManager(&accountManager)
{
    m_pUi->setupUi(this);

    setWindowTitle(tr("Preferences"));

#ifdef Q_WS_MAC
    // It makes little sense to minimize to tray on Mac
    // because the minimized app goes to dock
    m_pUi->minimizeToTrayCheckBox->setDisabled(true);
    m_pUi->minimizeToTrayCheckBox->setHidden(true);
#endif

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
        appSettings.beginGroup(QStringLiteral("SystemTray"));
        appSettings.setValue(SHOW_SYSTEM_TRAY_ICON_SETTINGS_KEY, checked);
        appSettings.endGroup();
    }
    else
    {
        QNWARNING(QStringLiteral("Can't persist the show system tray icon option: "
                                 "the account manager is null"));
    }

    emit showSystemTrayIconOptionChanged(checked);
}

void PreferencesDialog::createConnections()
{
    QNDEBUG(QStringLiteral("PreferencesDialog::createConnections"));

    QObject::connect(m_pUi->showSystemTrayIconCheckBox, QNSIGNAL(QCheckBox,toggled,bool),
                     this, QNSLOT(PreferencesDialog,onShowSystemTrayIconCheckboxToggled,bool));

    // TODO: continue
}

} // namespace quentier
