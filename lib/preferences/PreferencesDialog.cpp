/*
 * Copyright 2017-2022 Dmitry Ivanov
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

#include "PreferencesDialog.h"

#include "defaults/Appearance.h"
#include "defaults/SidePanelsFiltering.h"
#include "defaults/Synchronization.h"
#include "defaults/SystemTray.h"
#include "keys/Appearance.h"
#include "keys/Files.h"
#include "keys/Logging.h"
#include "keys/NoteEditor.h"
#include "keys/SidePanelsFiltering.h"
#include "keys/Synchronization.h"
#include "keys/SystemTray.h"
#include "keys/Updates.h"

#include "panel_colors/PanelColorsHandlerWidget.h"
using quentier::PanelColorsHandlerWidget;

#include "shortcut_settings/ShortcutSettingsWidget.h"
using quentier::ShortcutSettingsWidget;

#include "ui_PreferencesDialog.h"

#include <lib/account/AccountManager.h>
#include <lib/network/NetworkProxySettingsHelpers.h>
#include <lib/tray/SystemTrayIconManager.h>
#include <lib/utility/ActionsInfo.h>
#include <lib/utility/ColorCodeValidator.h>
#include <lib/utility/StartAtLogin.h>

#include <quentier/logging/QuentierLogger.h>
#include <quentier/utility/ApplicationSettings.h>
#include <quentier/utility/ShortcutManager.h>

#include <QColorDialog>
#include <QDir>
#include <QFileInfo>
#include <QNetworkProxy>
#include <QStringListModel>

#include <algorithm>
#include <limits>
#include <memory>

namespace quentier {

QString trayActionToString(SystemTrayIconManager::TrayAction action);

PreferencesDialog::PreferencesDialog(
    AccountManager & accountManager, ShortcutManager & shortcutManager,
    SystemTrayIconManager & systemTrayIconManager, ActionsInfo & actionsInfo,
    QWidget * parent) :
    QDialog(parent),
    m_pUi(new Ui::PreferencesDialog), m_accountManager(accountManager),
    m_systemTrayIconManager(systemTrayIconManager),
    m_pTrayActionsModel(new QStringListModel(this)),
    m_pNetworkProxyTypesModel(new QStringListModel(this)),
    m_pStartAtLoginOptionsModel(new QStringListModel(this))
{
    m_pUi->setupUi(this);

    m_pUi->disableNativeMenuBarRestartWarningLabel->setHidden(true);
    m_pUi->statusTextLabel->setHidden(true);

    m_pUi->okPushButton->setDefault(false);
    m_pUi->okPushButton->setAutoDefault(false);

    setWindowTitle(tr("Preferences"));

    setupInitialPreferencesState(actionsInfo, shortcutManager);
    createConnections();
    installEventFilters();
    adjustSize();
}

PreferencesDialog::~PreferencesDialog()
{
    delete m_pUi;
}

void PreferencesDialog::onShowSystemTrayIconCheckboxToggled(bool checked)
{
    QNDEBUG(
        "preferences",
        "PreferencesDialog::onShowSystemTrayIconCheckboxToggled: checked = "
            << (checked ? "true" : "false"));

    if (!m_systemTrayIconManager.isSystemTrayAvailable()) {
        QNDEBUG(
            "preferences",
            "Ignoring the change of show system tray icon "
                << "checkbox: the system tray is not available");
        return;
    }

    ApplicationSettings appSettings(
        m_accountManager.currentAccount(),
        preferences::keys::files::userInterface);

    appSettings.beginGroup(preferences::keys::systemTrayGroup);
    appSettings.setValue(preferences::keys::showSystemTrayIcon, checked);
    appSettings.endGroup();

    // Change the enabled/disabled status of all the tray actions but the one
    // to show/hide the system tray icon
    m_pUi->closeToTrayCheckBox->setEnabled(checked);
    m_pUi->minimizeToTrayCheckBox->setEnabled(checked);
    m_pUi->startFromTrayCheckBox->setEnabled(checked);

    m_pUi->traySingleClickActionComboBox->setEnabled(checked);
    m_pUi->trayMiddleClickActionComboBox->setEnabled(checked);
    m_pUi->trayDoubleClickActionComboBox->setEnabled(checked);

    bool shown = m_systemTrayIconManager.isShown();
    if (checked && !shown) {
        m_systemTrayIconManager.show();
    }
    else if (!checked && shown) {
        m_systemTrayIconManager.hide();
    }
}

void PreferencesDialog::onCloseToSystemTrayCheckboxToggled(bool checked)
{
    QNDEBUG(
        "preferences",
        "PreferencesDialog::onCloseToSystemTrayCheckboxToggled: checked = "
            << (checked ? "true" : "false"));

    if (!m_systemTrayIconManager.isSystemTrayAvailable()) {
        QNDEBUG(
            "preferences",
            "Ignoring the change of close to system tray "
                << "checkbox: the system tray is not available");
        return;
    }

    ApplicationSettings appSettings(
        m_accountManager.currentAccount(),
        preferences::keys::files::userInterface);

    appSettings.beginGroup(preferences::keys::systemTrayGroup);
    appSettings.setValue(preferences::keys::closeToSystemTray, checked);
    appSettings.endGroup();
}

void PreferencesDialog::onMinimizeToSystemTrayCheckboxToggled(bool checked)
{
    QNDEBUG(
        "preferences",
        "PreferencesDialog::onMinimizeToSystemTrayCheckboxToggled: "
            << "checked = " << (checked ? "true" : "false"));

    if (!m_systemTrayIconManager.isSystemTrayAvailable()) {
        QNDEBUG(
            "preferences",
            "Ignoring the change of minimize to system tray "
                << "checkbox: the system tray is not available");
        return;
    }

    ApplicationSettings appSettings(
        m_accountManager.currentAccount(),
        preferences::keys::files::userInterface);

    appSettings.beginGroup(preferences::keys::systemTrayGroup);
    appSettings.setValue(preferences::keys::minimizeToSystemTray, checked);
    appSettings.endGroup();
}

void PreferencesDialog::onStartMinimizedToSystemTrayCheckboxToggled(
    bool checked)
{
    QNDEBUG(
        "preferences",
        "PreferencesDialog::onStartMinimizedToSystemTrayCheckboxToggled: "
            << "checked = " << (checked ? "true" : "false"));

    if (!m_systemTrayIconManager.isSystemTrayAvailable()) {
        QNDEBUG(
            "preferences",
            "Ignoring the change of start minimized to "
                << "system tray checkbox: the system tray is not available");
        return;
    }

    ApplicationSettings appSettings(
        m_accountManager.currentAccount(),
        preferences::keys::files::userInterface);

    appSettings.beginGroup(preferences::keys::systemTrayGroup);
    appSettings.setValue(
        preferences::keys::startMinimizedToSystemTray, checked);
    appSettings.endGroup();
}

void PreferencesDialog::onSingleClickTrayActionChanged(int action)
{
    QNDEBUG(
        "preferences",
        "PreferencesDialog::onSingleClickTrayActionChanged: " << action);

    if (!m_systemTrayIconManager.isSystemTrayAvailable()) {
        QNDEBUG(
            "preferences",
            "Ignoring the change of single click tray "
                << "action: the system tray is not available");
        return;
    }

    ApplicationSettings appSettings(
        m_accountManager.currentAccount(),
        preferences::keys::files::userInterface);

    appSettings.beginGroup(preferences::keys::systemTrayGroup);
    appSettings.setValue(preferences::keys::singleClickTrayAction, action);
    appSettings.endGroup();
}

void PreferencesDialog::onMiddleClickTrayActionChanged(int action)
{
    QNDEBUG(
        "preferences",
        "PreferencesDialog::onMiddleClickTrayActionChanged: " << action);

    if (!m_systemTrayIconManager.isSystemTrayAvailable()) {
        QNDEBUG(
            "preferences",
            "Ignoring the change of middle click tray "
                << "action: the system tray is not available");
        return;
    }

    ApplicationSettings appSettings(
        m_accountManager.currentAccount(),
        preferences::keys::files::userInterface);

    appSettings.beginGroup(preferences::keys::systemTrayGroup);
    appSettings.setValue(preferences::keys::middleClickTrayAction, action);
    appSettings.endGroup();
}

void PreferencesDialog::onDoubleClickTrayActionChanged(int action)
{
    QNDEBUG(
        "preferences",
        "PreferencesDialog::onDoubleClickTrayActionChanged: " << action);

    if (!m_systemTrayIconManager.isSystemTrayAvailable()) {
        QNDEBUG(
            "preferences",
            "Ignoring the change of double click tray "
                << "action: the system tray is not available");
        return;
    }

    ApplicationSettings appSettings(
        m_accountManager.currentAccount(),
        preferences::keys::files::userInterface);

    appSettings.beginGroup(preferences::keys::systemTrayGroup);
    appSettings.setValue(preferences::keys::doubleClickTrayAction, action);
    appSettings.endGroup();
}

void PreferencesDialog::onShowNoteThumbnailsCheckboxToggled(bool checked)
{
    QNDEBUG(
        "preferences",
        "PreferencesDialog::onShowNoteThumbnailsCheckboxToggled: checked = "
            << (checked ? "checked" : "unchecked"));

    ApplicationSettings appSettings(
        m_accountManager.currentAccount(),
        preferences::keys::files::userInterface);

    appSettings.beginGroup(preferences::keys::appearanceGroup);
    appSettings.setValue(preferences::keys::showNoteThumbnails, checked);
    appSettings.endGroup();

    Q_EMIT showNoteThumbnailsOptionChanged();
}

void PreferencesDialog::onDisableNativeMenuBarCheckboxToggled(bool checked)
{
    QNDEBUG(
        "preferences",
        "PreferencesDialog::onDisableNativeMenuBarCheckboxToggled: "
            << "checked = " << (checked ? "checked" : "unchecked"));

    ApplicationSettings appSettings(
        m_accountManager.currentAccount(),
        preferences::keys::files::userInterface);

    appSettings.beginGroup(preferences::keys::appearanceGroup);
    appSettings.setValue(preferences::keys::disableNativeMenuBar, checked);
    appSettings.endGroup();

    m_pUi->disableNativeMenuBarRestartWarningLabel->setVisible(true);

    Q_EMIT disableNativeMenuBarOptionChanged();
}

void PreferencesDialog::onPanelColorWidgetUserError(QString errorMessage)
{
    if (Q_UNLIKELY(errorMessage.isEmpty())) {
        return;
    }

    m_pUi->statusTextLabel->setText(errorMessage);
    m_pUi->statusTextLabel->show();

    if (m_clearAndHideStatusBarTimerId != 0) {
        killTimer(m_clearAndHideStatusBarTimerId);
    }

    m_clearAndHideStatusBarTimerId = startTimer(15000);
}

void PreferencesDialog::onIconThemeChanged(int iconThemeIndex)
{
    QString iconThemeName = m_pUi->iconThemeComboBox->itemText(iconThemeIndex);
    Q_EMIT iconThemeChanged(iconThemeName);
}

void PreferencesDialog::onStartAtLoginCheckboxToggled(bool checked)
{
    QNDEBUG(
        "preferences",
        "PreferencesDialog::onStartAtLoginCheckboxToggled: "
            << (checked ? "checked" : "unchecked"));

    ErrorString errorDescription;

    bool res = setStartQuentierAtLoginOption(
        checked, errorDescription,
        static_cast<StartQuentierAtLoginOption::type>(
            m_pUi->startAtLoginOptionComboBox->currentIndex()));

    if (Q_UNLIKELY(!res)) {
        m_pUi->statusTextLabel->setText(
            tr("Failed to change start at login option") +
            QStringLiteral(": ") + errorDescription.localizedString());

        m_pUi->statusTextLabel->show();
        checked = isQuentierSetToStartAtLogin().first;
    }
    else {
        m_pUi->statusTextLabel->setText(QString());
        m_pUi->statusTextLabel->hide();
    }

    m_pUi->startAtLoginOptionComboBox->setEnabled(checked);
}

void PreferencesDialog::onStartAtLoginOptionChanged(int option)
{
    QNDEBUG(
        "preferences",
        "PreferencesDialog::onStartAtLoginOptionChanged: "
            << "option = " << option);

    ErrorString errorDescription;

    bool res = setStartQuentierAtLoginOption(
        m_pUi->startAtLoginCheckBox->isChecked(), errorDescription,
        static_cast<StartQuentierAtLoginOption::type>(option));

    if (Q_UNLIKELY(!res)) {
        setupStartAtLoginPreferences();

        m_pUi->statusTextLabel->setText(
            tr("Failed to change start at login option") +
            QStringLiteral(": ") + errorDescription.localizedString());

        m_pUi->statusTextLabel->show();
    }
    else {
        m_pUi->statusTextLabel->setText(QString());
        m_pUi->statusTextLabel->hide();
    }
}

#ifdef WITH_UPDATE_MANAGER
void PreferencesDialog::onCheckForUpdatesCheckboxToggled(bool checked)
{
    QNDEBUG(
        "preferences",
        "PreferencesDialog::onCheckForUpdatesCheckboxToggled: "
            << (checked ? "checked" : "unchecked"));

    ApplicationSettings appSettings;
    appSettings.beginGroup(preferences::keys::checkForUpdatesGroup);
    appSettings.setValue(preferences::keys::checkForUpdates, checked);
    appSettings.endGroup();

    Q_EMIT checkForUpdatesOptionChanged(checked);
}

void PreferencesDialog::onCheckForUpdatesOnStartupCheckboxToggled(bool checked)
{
    QNDEBUG(
        "preferences",
        "PreferencesDialog::onCheckForUpdatesOnStartupCheckboxToggled: "
            << (checked ? "checked" : "unchecked"));

    ApplicationSettings appSettings;
    appSettings.beginGroup(preferences::keys::checkForUpdatesGroup);
    appSettings.setValue(preferences::keys::checkForUpdatesOnStartup, checked);
    appSettings.endGroup();

    Q_EMIT checkForUpdatesOnStartupOptionChanged(checked);
}

void PreferencesDialog::onUseContinuousUpdateChannelCheckboxToggled(
    bool checked)
{
    QNDEBUG(
        "preferences",
        "PreferencesDialog::onUseContinuousUpdateChannelCheckboxToggled: "
            << (checked ? "checked" : "unchecked"));

    ApplicationSettings appSettings;
    appSettings.beginGroup(preferences::keys::checkForUpdatesGroup);

    appSettings.setValue(
        preferences::keys::useContinuousUpdateChannel, checked);

    appSettings.endGroup();

    Q_EMIT useContinuousUpdateChannelOptionChanged(checked);
}

void PreferencesDialog::onCheckForUpdatesIntervalChanged(int option)
{
    auto msec = checkForUpdatesIntervalMsecFromOption(
        static_cast<CheckForUpdatesInterval>(option));

    QNDEBUG(
        "preferences",
        "PreferencesDialog::onCheckForUpdatesIntervalChanged: option = "
            << option << ", msec = " << msec);

    ApplicationSettings appSettings;
    appSettings.beginGroup(preferences::keys::checkForUpdatesGroup);
    appSettings.setValue(preferences::keys::checkForUpdatesInterval, msec);
    appSettings.endGroup();

    Q_EMIT checkForUpdatesIntervalChanged(msec);
}

void PreferencesDialog::onUpdateChannelChanged(int index)
{
    QNDEBUG(
        "preferences", "PreferencesDialog::onUpdateChannelChanged: " << index);

    QString channel =
        (index == 0) ? QStringLiteral("master") : QStringLiteral("development");

    ApplicationSettings appSettings;
    appSettings.beginGroup(preferences::keys::checkForUpdatesGroup);
    appSettings.setValue(preferences::keys::checkForUpdatesChannel, channel);
    appSettings.endGroup();

    Q_EMIT updateChannelChanged(channel);
}

void PreferencesDialog::onUpdateProviderChanged(int index)
{
    auto provider = static_cast<UpdateProvider>(index);

    QNDEBUG(
        "preferences",
        "PreferencesDialog::onUpdateProviderChanged: " << provider);

    ApplicationSettings appSettings;
    appSettings.beginGroup(preferences::keys::checkForUpdatesGroup);
    appSettings.setValue(preferences::keys::checkForUpdatesProvider, index);
    appSettings.endGroup();

    bool usingGitHubProvider = (provider == UpdateProvider::GITHUB);
    m_pUi->useContinuousUpdateChannelCheckBox->setEnabled(usingGitHubProvider);
    m_pUi->updateChannelComboBox->setEnabled(usingGitHubProvider);

    Q_EMIT updateProviderChanged(provider);
}
#endif // WITH_UPDATE_MANAGER

void PreferencesDialog::onFilterByNotebookCheckboxToggled(bool checked)
{
    QNDEBUG(
        "preferences",
        "PreferencesDialog::onFilterByNotebookCheckboxToggled: "
            << (checked ? "checked" : "unchecked"));

    ApplicationSettings appSettings(
        m_accountManager.currentAccount(),
        preferences::keys::files::userInterface);

    appSettings.beginGroup(preferences::keys::sidePanelsFilterBySelectionGroup);
    appSettings.setValue(
        preferences::keys::sidePanelsFilterBySelectedNotebook, checked);
    appSettings.endGroup();

    Q_EMIT filterByNotebookOptionChanged(checked);
}

void PreferencesDialog::onFilterByTagCheckboxToggled(bool checked)
{
    QNDEBUG(
        "preferences",
        "PreferencesDialog::onFilterByTagCheckboxToggled: "
            << (checked ? "checked" : "unchecked"));

    ApplicationSettings appSettings(
        m_accountManager.currentAccount(),
        preferences::keys::files::userInterface);

    appSettings.beginGroup(preferences::keys::sidePanelsFilterBySelectionGroup);
    appSettings.setValue(
        preferences::keys::sidePanelsFilterBySelectedTag, checked);
    appSettings.endGroup();

    Q_EMIT filterByTagOptionChanged(checked);
}

void PreferencesDialog::onFilterBySavedSearchCheckboxToggled(bool checked)
{
    QNDEBUG(
        "preferences",
        "PreferencesDialog::onFilterBySavedSearchCheckboxToggled: "
            << (checked ? "checked" : "unchecked"));

    ApplicationSettings appSettings(
        m_accountManager.currentAccount(),
        preferences::keys::files::userInterface);

    appSettings.beginGroup(preferences::keys::sidePanelsFilterBySelectionGroup);
    appSettings.setValue(
        preferences::keys::sidePanelsFilterBySelectedSavedSearch, checked);
    appSettings.endGroup();

    Q_EMIT filterBySavedSearchOptionChanged(checked);
}

void PreferencesDialog::onFilterByFavoritedItemsCheckboxToggled(bool checked)
{
    QNDEBUG(
        "preferences",
        "PreferencesDialog::onFilterByFavoritedItemsCheckboxToggled: "
            << (checked ? "checked" : "unchecked"));

    ApplicationSettings appSettings(
        m_accountManager.currentAccount(),
        preferences::keys::files::userInterface);

    appSettings.beginGroup(preferences::keys::sidePanelsFilterBySelectionGroup);

    appSettings.setValue(
        preferences::keys::sidePanelsFilterBySelectedFavoritedItems, checked);

    appSettings.endGroup();

    Q_EMIT filterByFavoritedItemsOptionChanged(checked);
}

void PreferencesDialog::onNoteEditorUseLimitedFontsCheckboxToggled(bool checked)
{
    QNDEBUG(
        "preferences",
        "PreferencesDialog::onNoteEditorUseLimitedFontsCheckboxToggled: "
            << (checked ? "checked" : "unchecked"));

    ApplicationSettings appSettings(
        m_accountManager.currentAccount(),
        preferences::keys::files::userInterface);

    appSettings.beginGroup(preferences::keys::noteEditorGroup);
    appSettings.setValue(
        preferences::keys::noteEditorUseLimitedSetOfFonts, checked);
    appSettings.endGroup();

    Q_EMIT noteEditorUseLimitedFontsOptionChanged(checked);
}

void PreferencesDialog::onNoteEditorFontColorCodeEntered()
{
    QString colorCode = m_pUi->noteEditorFontColorLineEdit->text();

    QNDEBUG(
        "preferences",
        "PreferencesDialog::onNoteEditorFontColorCodeEntered: " << colorCode);

    QColor prevColor = noteEditorFontColor();
    QColor color(colorCode);

    bool res = onNoteEditorColorEnteredImpl(
        color, prevColor, preferences::keys::noteEditorFontColor,
        *m_pUi->noteEditorFontColorLineEdit,
        *m_pUi->noteEditorFontColorDemoFrame);

    if (!res) {
        return;
    }

    Q_EMIT noteEditorFontColorChanged(color);
}

void PreferencesDialog::onNoteEditorFontColorDialogRequested()
{
    QNDEBUG(
        "preferences",
        "PreferencesDialog::onNoteEditorFontColorDialogRequested");

    if (!m_pNoteEditorFontColorDialog.isNull()) {
        QNDEBUG("preferences", "Dialog is already opened");
        return;
    }

    auto pColorDialog = std::make_unique<QColorDialog>(this);
    pColorDialog->setWindowModality(Qt::WindowModal);
    pColorDialog->setCurrentColor(noteEditorFontColor());

    m_pNoteEditorFontColorDialog = pColorDialog.get();

    QObject::connect(
        pColorDialog.get(), &QColorDialog::colorSelected, this,
        &PreferencesDialog::onNoteEditorFontColorSelected);

    Q_UNUSED(pColorDialog->exec())
}

void PreferencesDialog::onNoteEditorFontColorSelected(const QColor & color)
{
    QNDEBUG(
        "preferences",
        "PreferencesDialog::onNoteEditorFontColorSelected: " << color.name());

    m_pUi->noteEditorFontColorLineEdit->setText(color.name());
    setNoteEditorFontColorToDemoFrame(color);

    QColor previousFontColor = noteEditorFontColor();
    if (previousFontColor.isValid() && color.isValid() &&
        (previousFontColor.name() == color.name()))
    {
        QNDEBUG("preferences", "Note editor font color did not change");
        return;
    }

    saveNoteEditorFontColor(color);
    Q_EMIT noteEditorFontColorChanged(color);
}

void PreferencesDialog::onNoteEditorBackgroundColorCodeEntered()
{
    QString colorCode = m_pUi->noteEditorBackgroundColorLineEdit->text();

    QNDEBUG(
        "preferences",
        "PreferencesDialog::onNoteEditorBackgroundColorCodeEntered: "
            << colorCode);

    QColor prevColor = noteEditorBackgroundColor();
    QColor color(colorCode);

    bool res = onNoteEditorColorEnteredImpl(
        color, prevColor, preferences::keys::noteEditorBackgroundColor,
        *m_pUi->noteEditorBackgroundColorLineEdit,
        *m_pUi->noteEditorBackgroundColorDemoFrame);

    if (!res) {
        return;
    }

    Q_EMIT noteEditorBackgroundColorChanged(color);
}

void PreferencesDialog::onNoteEditorBackgroundColorDialogRequested()
{
    QNDEBUG(
        "preferences",
        "PreferencesDialog::onNoteEditorBackgroundColorDialogRequested");

    if (!m_pNoteEditorBackgroundColorDialog.isNull()) {
        QNDEBUG("preferences", "Dialog is already opened");
        return;
    }

    auto pColorDialog = std::make_unique<QColorDialog>(this);
    pColorDialog->setWindowModality(Qt::WindowModal);
    pColorDialog->setCurrentColor(noteEditorBackgroundColor());

    m_pNoteEditorBackgroundColorDialog = pColorDialog.get();

    QObject::connect(
        pColorDialog.get(), &QColorDialog::colorSelected, this,
        &PreferencesDialog::onNoteEditorBackgroundColorSelected);

    Q_UNUSED(pColorDialog->exec())
}

void PreferencesDialog::onNoteEditorBackgroundColorSelected(
    const QColor & color)
{
    QNDEBUG(
        "preferences",
        "PreferencesDialog::onNoteEditorBackgroundColorSelected: "
            << color.name());

    m_pUi->noteEditorBackgroundColorLineEdit->setText(color.name());
    setNoteEditorBackgroundColorToDemoFrame(color);

    QColor previousBackgroundColor = noteEditorBackgroundColor();
    if (previousBackgroundColor.isValid() && color.isValid() &&
        (previousBackgroundColor.name() == color.name()))
    {
        QNDEBUG("preferences", "Note editor background color did not change");
        return;
    }

    saveNoteEditorBackgroundColor(color);
    Q_EMIT noteEditorBackgroundColorChanged(color);
}

void PreferencesDialog::onNoteEditorHighlightColorCodeEntered()
{
    QString colorCode = m_pUi->noteEditorHighlightColorLineEdit->text();

    QNDEBUG(
        "preferences",
        "PreferencesDialog::onNoteEditorHighlightColorCodeEntered: "
            << colorCode);

    QColor prevColor = noteEditorHighlightColor();
    QColor color(colorCode);

    bool res = onNoteEditorColorEnteredImpl(
        color, prevColor, preferences::keys::noteEditorHighlightColor,
        *m_pUi->noteEditorHighlightColorLineEdit,
        *m_pUi->noteEditorHighlightColorDemoFrame);

    if (!res) {
        return;
    }

    Q_EMIT noteEditorHighlightColorChanged(color);
}

void PreferencesDialog::onNoteEditorHighlightColorDialogRequested()
{
    QNDEBUG(
        "preferences",
        "PreferencesDialog::onNoteEditorHighlightColorDialogRequested");

    if (!m_pNoteEditorHighlightColorDialog.isNull()) {
        QNDEBUG("preferences", "Dialog is already opened");
        return;
    }

    auto pColorDialog = std::make_unique<QColorDialog>(this);
    pColorDialog->setWindowModality(Qt::WindowModal);
    pColorDialog->setCurrentColor(noteEditorHighlightColor());

    m_pNoteEditorHighlightColorDialog = pColorDialog.get();

    QObject::connect(
        pColorDialog.get(), &QColorDialog::colorSelected, this,
        &PreferencesDialog::onNoteEditorHighlightColorSelected);

    Q_UNUSED(pColorDialog->exec())
}

void PreferencesDialog::onNoteEditorHighlightColorSelected(const QColor & color)
{
    QNDEBUG(
        "preferences",
        "PreferencesDialog::onNoteEditorHighlightColorSelected: "
            << color.name());

    m_pUi->noteEditorHighlightColorLineEdit->setText(color.name());
    setNoteEditorHighlightColorToDemoFrame(color);

    QColor previousHighlightColor = noteEditorHighlightColor();
    if (previousHighlightColor.isValid() && color.isValid() &&
        (previousHighlightColor.name() == color.name()))
    {
        QNDEBUG("preferences", "Note editor highlight color did not change");
        return;
    }

    saveNoteEditorHighlightColor(color);
    Q_EMIT noteEditorHighlightColorChanged(color);
}

void PreferencesDialog::onNoteEditorHighlightedTextColorCodeEntered()
{
    QString colorCode = m_pUi->noteEditorHighlightedTextColorLineEdit->text();

    QNDEBUG(
        "preferences",
        "PreferencesDialog::onNoteEditorHighlightedTextColorCodeEntered: "
            << colorCode);

    QColor prevColor = noteEditorHighlightedTextColor();
    QColor color(colorCode);

    bool res = onNoteEditorColorEnteredImpl(
        color, prevColor, preferences::keys::noteEditorHighlightedTextColor,
        *m_pUi->noteEditorHighlightedTextColorLineEdit,
        *m_pUi->noteEditorHighlightedTextColorDemoFrame);

    if (!res) {
        return;
    }

    Q_EMIT noteEditorHighlightedTextColorChanged(color);
}

void PreferencesDialog::onNoteEditorHighlightedTextColorDialogRequested()
{
    QNDEBUG(
        "preferences",
        "PreferencesDialog::onNoteEditorHighlightedTextColorDialogRequested");

    if (!m_pNoteEditorHighlightedTextColorDialog.isNull()) {
        QNDEBUG("preferences", "Dialog is already opened");
        return;
    }

    auto pColorDialog = std::make_unique<QColorDialog>(this);
    pColorDialog->setWindowModality(Qt::WindowModal);
    pColorDialog->setCurrentColor(noteEditorHighlightedTextColor());

    m_pNoteEditorHighlightedTextColorDialog = pColorDialog.get();

    QObject::connect(
        pColorDialog.get(), &QColorDialog::colorSelected, this,
        &PreferencesDialog::onNoteEditorHighlightedTextColorSelected);

    Q_UNUSED(pColorDialog->exec())
}

void PreferencesDialog::onNoteEditorHighlightedTextColorSelected(
    const QColor & color)
{
    QNDEBUG(
        "preferences",
        "PreferencesDialog::onNoteEditorHighlightedTextColorSelected: "
            << color.name());

    m_pUi->noteEditorHighlightedTextColorLineEdit->setText(color.name());
    setNoteEditorHighlightedTextColorToDemoFrame(color);

    QColor previousHighlightedTextColor = noteEditorHighlightedTextColor();
    if (previousHighlightedTextColor.isValid() && color.isValid() &&
        (previousHighlightedTextColor.name() == color.name()))
    {
        QNDEBUG(
            "preferences",
            "Note editor highlighted text color did not "
                << "change");
        return;
    }

    saveNoteEditorHighlightedTextColor(color);
    Q_EMIT noteEditorHighlightedTextColorChanged(color);
}

void PreferencesDialog::onNoteEditorColorsReset()
{
    QNDEBUG("preferences", "PreferencesDialog::onNoteEditorColorsReset");

    ApplicationSettings appSettings(
        m_accountManager.currentAccount(),
        preferences::keys::files::userInterface);

    appSettings.beginGroup(preferences::keys::noteEditorGroup);
    appSettings.remove(preferences::keys::noteEditorFontColor);
    appSettings.remove(preferences::keys::noteEditorBackgroundColor);
    appSettings.remove(preferences::keys::noteEditorHighlightColor);
    appSettings.remove(preferences::keys::noteEditorHighlightedTextColor);
    appSettings.endGroup();

    QPalette pal = palette();

    QColor fontColor = pal.color(QPalette::WindowText);
    setNoteEditorFontColorToDemoFrame(fontColor);
    m_pUi->noteEditorFontColorLineEdit->setText(fontColor.name());

    QColor backgroundColor = pal.color(QPalette::Base);
    setNoteEditorBackgroundColorToDemoFrame(backgroundColor);
    m_pUi->noteEditorBackgroundColorLineEdit->setText(backgroundColor.name());

    QColor highlightColor = pal.color(QPalette::Highlight);
    setNoteEditorHighlightColorToDemoFrame(highlightColor);
    m_pUi->noteEditorHighlightColorLineEdit->setText(highlightColor.name());

    QColor highlightedTextColor = pal.color(QPalette::HighlightedText);
    setNoteEditorHighlightedTextColorToDemoFrame(highlightedTextColor);

    m_pUi->noteEditorHighlightedTextColorLineEdit->setText(
        highlightedTextColor.name());

    Q_EMIT noteEditorColorsReset();
}

void PreferencesDialog::onDownloadNoteThumbnailsCheckboxToggled(bool checked)
{
    QNDEBUG(
        "preferences",
        "PreferencesDialog::onDownloadNoteThumbnailsCheckboxToggled: "
            << (checked ? "checked" : "unchecked"));

    ApplicationSettings appSettings(
        m_accountManager.currentAccount(),
        preferences::keys::files::synchronization);

    appSettings.beginGroup(preferences::keys::synchronizationGroup);
    appSettings.setValue(preferences::keys::downloadNoteThumbnails, checked);
    appSettings.endGroup();

    Q_EMIT synchronizationDownloadNoteThumbnailsOptionChanged(checked);
}

void PreferencesDialog::onDownloadInkNoteImagesCheckboxToggled(bool checked)
{
    QNDEBUG(
        "preferences",
        "PreferencesDialog::onDownloadInkNoteImagesCheckboxToggled: "
            << (checked ? "checked" : "unchecked"));

    ApplicationSettings appSettings(
        m_accountManager.currentAccount(),
        preferences::keys::files::synchronization);

    appSettings.beginGroup(preferences::keys::synchronizationGroup);
    appSettings.setValue(preferences::keys::downloadInkNoteImages, checked);
    appSettings.endGroup();

    Q_EMIT synchronizationDownloadInkNoteImagesOptionChanged(checked);
}

void PreferencesDialog::onRunSyncPeriodicallyOptionChanged(int index)
{
    QNDEBUG(
        "preferences",
        "PreferencesDialog::onRunSyncPeriodicallyOptionChanged: index = "
            << index);

    int runSyncEachNumMinutes = 0;
    switch (index) {
    case 1:
        runSyncEachNumMinutes = 5;
        break;
    case 2:
        runSyncEachNumMinutes = 15;
        break;
    case 3:
        runSyncEachNumMinutes = 30;
        break;
    case 4:
        runSyncEachNumMinutes = 60;
        break;
        // NOTE: intentional fall-through
    case 0:
    default:
        break;
    }

    ApplicationSettings syncSettings(
        m_accountManager.currentAccount(),
        preferences::keys::files::synchronization);

    syncSettings.beginGroup(preferences::keys::synchronizationGroup);

    syncSettings.setValue(
        preferences::keys::runSyncPeriodMinutes, runSyncEachNumMinutes);

    syncSettings.endGroup();

    Q_EMIT runSyncPeriodicallyOptionChanged(runSyncEachNumMinutes);
}

void PreferencesDialog::onRunSyncOnStartupOptionChanged(bool checked)
{
    QNDEBUG(
        "preferences",
        "PreferencesDialog::onRunSyncOnStartupOptionChanged: checked = "
            << (checked ? "true" : "false"));

    ApplicationSettings syncSettings(
        m_accountManager.currentAccount(),
        preferences::keys::files::synchronization);

    syncSettings.beginGroup(preferences::keys::synchronizationGroup);
    syncSettings.setValue(preferences::keys::runSyncOnStartup, checked);
    syncSettings.endGroup();

    Q_EMIT runSyncOnStartupChanged(checked);
}

void PreferencesDialog::onNetworkProxyTypeChanged(int type)
{
    QNDEBUG(
        "preferences",
        "PreferencesDialog::onNetworkProxyTypeChanged: "
            << "type = " << type);

    checkAndSetNetworkProxy();
}

void PreferencesDialog::onNetworkProxyHostChanged()
{
    QNDEBUG(
        "preferences",
        "PreferencesDialog::onNetworkProxyHostChanged: "
            << m_pUi->networkProxyHostLineEdit->text());

    checkAndSetNetworkProxy();
}

void PreferencesDialog::onNetworkProxyPortChanged(int port)
{
    QNDEBUG(
        "preferences",
        "PreferencesDialog::onNetworkProxyPortChanged: " << port);

    checkAndSetNetworkProxy();
}

void PreferencesDialog::onNetworkProxyUsernameChanged()
{
    QNDEBUG(
        "preferences",
        "PreferencesDialog::onNetworkProxyUsernameChanged: "
            << m_pUi->networkProxyUserLineEdit->text());

    checkAndSetNetworkProxy();
}

void PreferencesDialog::onNetworkProxyPasswordChanged()
{
    QNDEBUG(
        "preferences",
        "PreferencesDialog::onNetworkProxyPasswordChanged: "
            << m_pUi->networkProxyPasswordLineEdit->text());

    checkAndSetNetworkProxy();
}

void PreferencesDialog::onNetworkProxyPasswordVisibilityToggled(bool checked)
{
    QNDEBUG(
        "preferences",
        "PreferencesDialog::onNetworkProxyPasswordVisibilityToggled: "
            << "checked = " << (checked ? "true" : "false"));

    m_pUi->networkProxyPasswordLineEdit->setEchoMode(
        checked ? QLineEdit::Normal : QLineEdit::Password);
}

bool PreferencesDialog::eventFilter(QObject * pObject, QEvent * pEvent)
{
    if (pObject == m_pUi->noteEditorFontColorDemoFrame) {
        if (pEvent && (pEvent->type() == QEvent::MouseButtonDblClick)) {
            onNoteEditorFontColorDialogRequested();
            return true;
        }
    }
    else if (pObject == m_pUi->noteEditorBackgroundColorDemoFrame) {
        if (pEvent && (pEvent->type() == QEvent::MouseButtonDblClick)) {
            onNoteEditorBackgroundColorDialogRequested();
            return true;
        }
    }
    else if (pObject == m_pUi->noteEditorHighlightColorDemoFrame) {
        if (pEvent && (pEvent->type() == QEvent::MouseButtonDblClick)) {
            onNoteEditorHighlightColorDialogRequested();
            return true;
        }
    }
    else if (pObject == m_pUi->noteEditorHighlightedTextColorDemoFrame) {
        if (pEvent && (pEvent->type() == QEvent::MouseButtonDblClick)) {
            onNoteEditorHighlightedTextColorDialogRequested();
            return true;
        }
    }

    return QDialog::eventFilter(pObject, pEvent);
}

void PreferencesDialog::timerEvent(QTimerEvent * pEvent)
{
    if (Q_UNLIKELY(!pEvent)) {
        return;
    }

    int timerId = pEvent->timerId();
    killTimer(timerId);

    if (timerId == m_clearAndHideStatusBarTimerId) {
        m_pUi->statusTextLabel->setText({});
        m_pUi->statusTextLabel->hide();
        m_clearAndHideStatusBarTimerId = 0;
    }
}

void PreferencesDialog::onEnableLogViewerInternalLogsCheckboxToggled(
    bool checked)
{
    QNDEBUG(
        "preferences",
        "PreferencesDialog::onEnableLogViewerInternalLogsCheckboxToggled: "
            << "checked = " << (checked ? "true" : "false"));

    ApplicationSettings globalAppSettings;
    globalAppSettings.beginGroup(preferences::keys::loggingGroup);
    globalAppSettings.setValue(
        preferences::keys::enableLogViewerInternalLogs, checked);
    globalAppSettings.endGroup();
}

void PreferencesDialog::setupInitialPreferencesState(
    ActionsInfo & actionsInfo, ShortcutManager & shortcutManager)
{
    QNDEBUG("preferences", "PreferencesDialog::setupInitialPreferencesState");

    Account currentAccount = m_accountManager.currentAccount();

    // 1) System tray tab
    setupSystemTrayPreferences();

    // 2) Note editor tab
    setupNoteEditorPreferences();

    // 3) Appearance tab
    setupAppearancePreferences(actionsInfo);
    m_pUi->panelColorsHandlerWidget->initialize(currentAccount);

    // 4) Behaviour tab
    setupStartAtLoginPreferences();
    setupCheckForUpdatesPreferences();
    setupFilteringPreferences();

    // 5) Synchronization tab
    if (currentAccount.type() == Account::Type::Local) {
        // Remove the synchronization tab entirely
        m_pUi->preferencesTabWidget->removeTab(
            m_pUi->preferencesTabWidget->indexOf(m_pUi->syncTab));
    }
    else {
        ApplicationSettings syncSettings(
            currentAccount, preferences::keys::files::synchronization);

        syncSettings.beginGroup(preferences::keys::synchronizationGroup);

        bool downloadNoteThumbnails =
            preferences::defaults::downloadNoteThumbnails;

        if (syncSettings.contains(preferences::keys::downloadNoteThumbnails)) {
            downloadNoteThumbnails =
                syncSettings.value(preferences::keys::downloadNoteThumbnails)
                    .toBool();
        }

        bool downloadInkNoteImages =
            preferences::defaults::downloadInkNoteImages;

        if (syncSettings.contains(preferences::keys::downloadInkNoteImages)) {
            downloadInkNoteImages =
                syncSettings.value(preferences::keys::downloadInkNoteImages)
                    .toBool();
        }

        int runSyncEachNumMinutes = -1;
        if (syncSettings.contains(preferences::keys::runSyncPeriodMinutes)) {
            QVariant data =
                syncSettings.value(preferences::keys::runSyncPeriodMinutes);

            bool conversionResult = false;
            runSyncEachNumMinutes = data.toInt(&conversionResult);
            if (Q_UNLIKELY(!conversionResult)) {
                QNDEBUG(
                    "preferences",
                    "Failed to convert the number of "
                        << "minutes to run sync over to int: " << data);
                runSyncEachNumMinutes = -1;
            }
        }

        const bool runSyncOnStartup = [&] {
            if (syncSettings.contains(preferences::keys::runSyncOnStartup)) {
                return syncSettings.value(preferences::keys::runSyncOnStartup)
                    .toBool();
            }

            return preferences::defaults::runSyncOnStartup;
        }();

        if (runSyncEachNumMinutes < 0) {
            runSyncEachNumMinutes = preferences::defaults::runSyncPeriodMinutes;
        }

        syncSettings.endGroup();

        m_pUi->downloadNoteThumbnailsCheckBox->setChecked(
            downloadNoteThumbnails);

        m_pUi->downloadInkNoteImagesCheckBox->setChecked(downloadInkNoteImages);

        setupNetworkProxyPreferences();
        setupRunSyncPeriodicallyComboBox(runSyncEachNumMinutes);
        m_pUi->runSyncOnStartupCheckBox->setChecked(runSyncOnStartup);

        m_pUi->synchronizationTabStatusLabel->hide();
    }

    // 6) Shortcuts tab
    m_pUi->shortcutSettingsWidget->initialize(
        currentAccount, actionsInfo, &shortcutManager);

    // 7) Auxiliary tab
    ApplicationSettings globalAppSettings;
    globalAppSettings.beginGroup(preferences::keys::loggingGroup);

    QVariant enableLogViewerInternalLogsValue =
        globalAppSettings.value(preferences::keys::enableLogViewerInternalLogs);

    globalAppSettings.endGroup();

    bool enableLogViewerInternalLogs = false;
    if (enableLogViewerInternalLogsValue.isValid()) {
        enableLogViewerInternalLogs = enableLogViewerInternalLogsValue.toBool();
    }

    m_pUi->enableInternalLogViewerLogsCheckBox->setChecked(
        enableLogViewerInternalLogs);
}

void PreferencesDialog::setupSystemTrayPreferences()
{
#ifdef Q_WS_MAC
    // It makes little sense to minimize to tray on Mac
    // because the minimized app goes to dock
    m_pUi->minimizeToTrayCheckBox->setDisabled(true);
    m_pUi->minimizeToTrayCheckBox->setHidden(true);
#endif

    if (!m_systemTrayIconManager.isSystemTrayAvailable()) {
        QNDEBUG(
            "preferences",
            "The system tray is not available, setting up "
                << "the blank system tray settings");

        m_pUi->showSystemTrayIconCheckBox->setChecked(false);
        m_pUi->showSystemTrayIconCheckBox->setDisabled(true);

        m_pUi->closeToTrayCheckBox->setChecked(false);
        m_pUi->closeToTrayCheckBox->setDisabled(true);

        m_pUi->minimizeToTrayCheckBox->setChecked(false);
        m_pUi->minimizeToTrayCheckBox->setDisabled(true);

        m_pUi->startFromTrayCheckBox->setChecked(false);
        m_pUi->startFromTrayCheckBox->setDisabled(true);

        m_pTrayActionsModel->setStringList(QStringList());
        m_pUi->traySingleClickActionComboBox->setModel(m_pTrayActionsModel);
        m_pUi->trayMiddleClickActionComboBox->setModel(m_pTrayActionsModel);
        m_pUi->trayDoubleClickActionComboBox->setModel(m_pTrayActionsModel);

        return;
    }

    ApplicationSettings appSettings(
        m_accountManager.currentAccount(),
        preferences::keys::files::userInterface);

    appSettings.beginGroup(preferences::keys::systemTrayGroup);

    bool shouldShowSystemTrayIcon = preferences::defaults::showSystemTrayIcon;

    QVariant shouldShowSystemTrayIconData =
        appSettings.value(preferences::keys::showSystemTrayIcon);

    if (shouldShowSystemTrayIconData.isValid()) {
        shouldShowSystemTrayIcon = shouldShowSystemTrayIconData.toBool();
    }

    bool shouldCloseToTray = preferences::defaults::closeToSystemTray;

    QVariant shouldCloseToTrayData =
        appSettings.value(preferences::keys::closeToSystemTray);

    if (shouldCloseToTrayData.isValid()) {
        shouldCloseToTray = shouldCloseToTrayData.toBool();
    }

    bool shouldMinimizeToTray = preferences::defaults::minimizeToSystemTray;

    QVariant shouldMinimizeToTrayData =
        appSettings.value(preferences::keys::minimizeToSystemTray);

    if (shouldMinimizeToTrayData.isValid()) {
        shouldMinimizeToTray = shouldMinimizeToTrayData.toBool();
    }

    bool shouldStartMinimizedToTray =
        preferences::defaults::startMinimizedToSystemTray;

    QVariant shouldStartMinimizedToTrayData =
        appSettings.value(preferences::keys::startMinimizedToSystemTray);

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

    trayActions.push_back(
        trayActionToString(SystemTrayIconManager::TrayActionDoNothing));

    trayActions.push_back(
        trayActionToString(SystemTrayIconManager::TrayActionShowHide));

    trayActions.push_back(
        trayActionToString(SystemTrayIconManager::TrayActionNewTextNote));

    trayActions.push_back(
        trayActionToString(SystemTrayIconManager::TrayActionShowContextMenu));

    m_pTrayActionsModel->setStringList(trayActions);

    m_pUi->traySingleClickActionComboBox->setModel(m_pTrayActionsModel);
    m_pUi->trayMiddleClickActionComboBox->setModel(m_pTrayActionsModel);
    m_pUi->trayDoubleClickActionComboBox->setModel(m_pTrayActionsModel);

    auto singleClickTrayAction =
        m_systemTrayIconManager.singleClickTrayAction();

    m_pUi->traySingleClickActionComboBox->setCurrentIndex(
        singleClickTrayAction);

    auto middleClickTrayAction =
        m_systemTrayIconManager.middleClickTrayAction();

    m_pUi->trayMiddleClickActionComboBox->setCurrentIndex(
        middleClickTrayAction);

    auto doubleClickTrayAction =
        m_systemTrayIconManager.doubleClickTrayAction();

    m_pUi->trayDoubleClickActionComboBox->setCurrentIndex(
        doubleClickTrayAction);

    // If the tray icon is now shown, disable all the tray actions but the one
    // to show/hide the system tray icon
    if (!m_systemTrayIconManager.isShown()) {
        m_pUi->closeToTrayCheckBox->setDisabled(true);
        m_pUi->minimizeToTrayCheckBox->setDisabled(true);
        m_pUi->startFromTrayCheckBox->setDisabled(true);

        m_pUi->traySingleClickActionComboBox->setDisabled(true);
        m_pUi->trayMiddleClickActionComboBox->setDisabled(true);
        m_pUi->trayDoubleClickActionComboBox->setDisabled(true);
    }
}

void PreferencesDialog::setupStartAtLoginPreferences()
{
    QNDEBUG("preferences", "PreferencesDialog::setupStartAtLoginPreferences");

    auto startAtLoginOptions = isQuentierSetToStartAtLogin();
    m_pUi->startAtLoginCheckBox->setChecked(startAtLoginOptions.first);

    QStringList startAtLoginOptionsList;
    startAtLoginOptionsList.reserve(3);
    startAtLoginOptionsList << tr("Start minimized to tray");
    startAtLoginOptionsList << tr("Start minimized to taskbar");
    startAtLoginOptionsList << tr("Start maximized or windowed");

    m_pStartAtLoginOptionsModel->setStringList(startAtLoginOptionsList);
    m_pUi->startAtLoginOptionComboBox->setModel(m_pStartAtLoginOptionsModel);

    m_pUi->startAtLoginOptionComboBox->setCurrentIndex(
        static_cast<int>(startAtLoginOptions.second));

    m_pUi->startAtLoginOptionComboBox->setEnabled(startAtLoginOptions.first);
}

void PreferencesDialog::setupCheckForUpdatesPreferences()
{
    QNDEBUG(
        "preferences", "PreferencesDialog::setupCheckForUpdatesPreferences");

#ifndef WITH_UPDATE_MANAGER
    m_pUi->checkForUpdatesCheckBox->setVisible(false);
    m_pUi->checkForUpdatesOnStartupCheckBox->setVisible(false);
    m_pUi->useContinuousUpdateChannelCheckBox->setVisible(false);
    m_pUi->checkForUpdatesIntervalLabel->setVisible(false);
    m_pUi->checkForUpdatesIntervalComboBox->setVisible(false);
    m_pUi->updateProviderLabel->setVisible(false);
    m_pUi->updateProviderComboBox->setVisible(false);
    m_pUi->updateChannelLabel->setVisible(false);
    m_pUi->updateChannelComboBox->setVisible(false);
    m_pUi->checkForUpdatesPushButton->setVisible(false);
    m_pUi->updateSettingsGroupBox->setVisible(false);
#else
    bool checkForUpdatesEnabled = false;
    bool checkForUpdatesOnStartupEnabled = false;
    bool useContinuousUpdateChannel = false;
    int checkForUpdatesIntervalOption = -1;
    QString updateChannel;
    UpdateProvider updateProvider;

    readPersistentUpdateSettings(
        checkForUpdatesEnabled, checkForUpdatesOnStartupEnabled,
        useContinuousUpdateChannel, checkForUpdatesIntervalOption,
        updateChannel, updateProvider);

#if !QUENTIER_PACKAGED_AS_APP_IMAGE
    // Only GitHub provider is available, so should hide the combo box allowing
    // one to choose between providers
    m_pUi->updateProviderComboBox->hide();
#endif

    m_pUi->checkForUpdatesCheckBox->setChecked(checkForUpdatesEnabled);

    m_pUi->checkForUpdatesOnStartupCheckBox->setChecked(
        checkForUpdatesOnStartupEnabled);

    QStringList checkForUpdatesIntervalOptions;
    checkForUpdatesIntervalOptions.reserve(8);
    checkForUpdatesIntervalOptions << tr("15 minutes");
    checkForUpdatesIntervalOptions << tr("30 minutes");
    checkForUpdatesIntervalOptions << tr("hour");
    checkForUpdatesIntervalOptions << tr("2 hours");
    checkForUpdatesIntervalOptions << tr("4 hours");
    checkForUpdatesIntervalOptions << tr("day");
    checkForUpdatesIntervalOptions << tr("week");
    checkForUpdatesIntervalOptions << tr("month");

    auto * pCheckForUpdatesIntervalComboBoxModel = new QStringListModel(this);

    pCheckForUpdatesIntervalComboBoxModel->setStringList(
        checkForUpdatesIntervalOptions);

    m_pUi->checkForUpdatesIntervalComboBox->setModel(
        pCheckForUpdatesIntervalComboBoxModel);

    m_pUi->checkForUpdatesIntervalComboBox->setCurrentIndex(
        checkForUpdatesIntervalOption);

    m_pUi->useContinuousUpdateChannelCheckBox->setChecked(
        useContinuousUpdateChannel);

    QStringList updateChannels;
    updateChannels.reserve(2);
    updateChannels << tr("Stable");
    updateChannels << tr("Unstable");

    auto * pUpdateChannelsComboBoxModel = new QStringListModel(this);
    pUpdateChannelsComboBoxModel->setStringList(updateChannels);
    m_pUi->updateChannelComboBox->setModel(pUpdateChannelsComboBoxModel);

    if (updateChannel == QStringLiteral("development")) {
        m_pUi->updateChannelComboBox->setCurrentIndex(1);
    }
    else {
        m_pUi->updateChannelComboBox->setCurrentIndex(0);
    }

    QStringList updateProviders;
#if QUENTIER_PACKAGED_AS_APP_IMAGE
    updateProviders.reserve(2);
#endif
    updateProviders << updateProviderName(UpdateProvider::GITHUB);

#if QUENTIER_PACKAGED_AS_APP_IMAGE
    updateProviders << updateProviderName(UpdateProvider::APPIMAGE);
#endif

    auto * pUpdateProvidersComboBoxModel = new QStringListModel(this);
    pUpdateProvidersComboBoxModel->setStringList(updateProviders);
    m_pUi->updateProviderComboBox->setModel(pUpdateProvidersComboBoxModel);

    m_pUi->updateProviderComboBox->setCurrentIndex(
        static_cast<int>(updateProvider));

    if (updateProvider == UpdateProvider::APPIMAGE) {
        m_pUi->useContinuousUpdateChannelCheckBox->setEnabled(false);
        m_pUi->updateChannelComboBox->setEnabled(false);
    }
#endif // WITH_UPDATE_MANAGER
}

void PreferencesDialog::setupFilteringPreferences()
{
    QNDEBUG("preferences", "PreferencesDialog::setupFilteringPreferences");

    ApplicationSettings appSettings(
        m_accountManager.currentAccount(),
        preferences::keys::files::userInterface);

    appSettings.beginGroup(preferences::keys::sidePanelsFilterBySelectionGroup);

    bool filterByNotebook = preferences::defaults::filterBySelectedNotebooks;

    const auto filterByNotebookValue = appSettings.value(
        preferences::keys::sidePanelsFilterBySelectedNotebook);

    if (filterByNotebookValue.isValid()) {
        filterByNotebook = filterByNotebookValue.toBool();
    }

    bool filterByTag = preferences::defaults::filterBySelectedTags;

    const auto filterByTagValue =
        appSettings.value(preferences::keys::sidePanelsFilterBySelectedTag);

    if (filterByTagValue.isValid()) {
        filterByTag = filterByTagValue.toBool();
    }

    bool filterBySavedSearch =
        preferences::defaults::filterBySelectedSavedSearch;

    const auto filterBySavedSearchValue = appSettings.value(
        preferences::keys::sidePanelsFilterBySelectedSavedSearch);

    if (filterBySavedSearchValue.isValid()) {
        filterBySavedSearch = filterBySavedSearchValue.toBool();
    }

    bool filterByFavoritedItems =
        preferences::defaults::filterBySelectedFavoritedItems;

    const auto filterByFavoritedItemsValue = appSettings.value(
        preferences::keys::sidePanelsFilterBySelectedFavoritedItems);

    if (filterByFavoritedItemsValue.isValid()) {
        filterByFavoritedItems = filterByFavoritedItemsValue.toBool();
    }

    appSettings.endGroup();

    m_pUi->filterBySelectedNotebookCheckBox->setChecked(filterByNotebook);
    m_pUi->filterBySelectedTagCheckBox->setChecked(filterByTag);
    m_pUi->filterBySelectedSavedSearchCheckBox->setChecked(filterBySavedSearch);
    m_pUi->filterBySelectedFavoritedItemsCheckBox->setChecked(
        filterByFavoritedItems);
}

void PreferencesDialog::setupRunSyncPeriodicallyComboBox(int currentNumMinutes)
{
    QNDEBUG(
        "preferences",
        "PreferencesDialog::setupRunSyncPeriodicallyComboBox: "
            << currentNumMinutes);

    QStringList runSyncPeriodicallyOptions;
    runSyncPeriodicallyOptions.reserve(5);
    runSyncPeriodicallyOptions << tr("Manually");
    runSyncPeriodicallyOptions << tr("Every 5 minutes");
    runSyncPeriodicallyOptions << tr("Every 15 minutes");
    runSyncPeriodicallyOptions << tr("Every 30 minutes");
    runSyncPeriodicallyOptions << tr("Every hour");

    auto * pRunSyncPeriodicallyComboBoxModel = new QStringListModel(this);

    pRunSyncPeriodicallyComboBoxModel->setStringList(
        runSyncPeriodicallyOptions);

    m_pUi->runSyncPeriodicallyComboBox->setModel(
        pRunSyncPeriodicallyComboBoxModel);

    int currentIndex = 2;
    switch (currentNumMinutes) {
    case 0:
        currentIndex = 0;
        break;
    case 5:
        currentIndex = 1;
        break;
    case 15:
        currentIndex = 2;
        break;
    case 30:
        currentIndex = 3;
        break;
    case 60:
        currentIndex = 4;
        break;
    default:
    {
        QNDEBUG(
            "preferences",
            "Unrecognized option, using the default "
                << "one");
        break;
    }
    }

    m_pUi->runSyncPeriodicallyComboBox->setCurrentIndex(currentIndex);

#if QT_VERSION >= QT_VERSION_CHECK(5, 7, 0)
    QObject::connect(
        m_pUi->runSyncPeriodicallyComboBox,
        qOverload<int>(&QComboBox::currentIndexChanged), this,
        &PreferencesDialog::onRunSyncPeriodicallyOptionChanged);
#else
    QObject::connect(
        m_pUi->runSyncPeriodicallyComboBox, SIGNAL(currentIndexChanged(int)),
        this, SLOT(onRunSyncPeriodicallyOptionChanged(int)));
#endif
}

void PreferencesDialog::setupAppearancePreferences(
    const ActionsInfo & actionsInfo)
{
    QNDEBUG("preferences", "PreferencesDialog::setupAppearancePreferences");

    ApplicationSettings appSettings(
        m_accountManager.currentAccount(),
        preferences::keys::files::userInterface);

    appSettings.beginGroup(preferences::keys::appearanceGroup);

    QVariant showThumbnails = appSettings.value(
        preferences::keys::showNoteThumbnails,
        QVariant::fromValue(preferences::defaults::showNoteThumbnails));

    QVariant disableNativeMenuBar = appSettings.value(
        preferences::keys::disableNativeMenuBar,
        QVariant::fromValue(preferences::defaults::disableNativeMenuBar()));

    QVariant iconTheme =
        appSettings.value(preferences::keys::iconTheme, tr("Native"));

    appSettings.endGroup();

    m_pUi->showNoteThumbnailsCheckBox->setChecked(showThumbnails.toBool());

    m_pUi->disableNativeMenuBarCheckBox->setChecked(
        disableNativeMenuBar.toBool());

    bool hasNativeIconTheme = false;
    if (!actionsInfo.findActionInfo(tr("Native"), tr("View")).isEmpty()) {
        hasNativeIconTheme = true;
    }

    if (hasNativeIconTheme) {
        m_pUi->iconThemeComboBox->addItem(tr("Native"));
    }

    m_pUi->iconThemeComboBox->addItem(QStringLiteral("breeze"));
    m_pUi->iconThemeComboBox->addItem(QStringLiteral("breeze-dark"));
    m_pUi->iconThemeComboBox->addItem(QStringLiteral("oxygen"));
    m_pUi->iconThemeComboBox->addItem(QStringLiteral("tango"));

    int iconThemeIndex =
        m_pUi->iconThemeComboBox->findText(iconTheme.toString());

    if (iconThemeIndex >= 0) {
        m_pUi->iconThemeComboBox->setCurrentIndex(iconThemeIndex);
    }

#if QT_VERSION >= QT_VERSION_CHECK(5, 7, 0)
    QObject::connect(
        m_pUi->iconThemeComboBox,
        qOverload<int>(&QComboBox::currentIndexChanged), this,
        &PreferencesDialog::onIconThemeChanged);
#else
    QObject::connect(
        m_pUi->iconThemeComboBox, SIGNAL(currentIndexChanged(int)), this,
        SLOT(onIconThemeChanged(int)));
#endif
}

void PreferencesDialog::setupNetworkProxyPreferences()
{
    QNDEBUG("preferences", "PreferencesDialog::setupNetworkProxyPreferences");

    auto proxyType = QNetworkProxy::DefaultProxy;
    QString proxyHost;
    int proxyPort = 0;
    QString proxyUsername;
    QString proxyPassword;

    parseNetworkProxySettings(
        m_accountManager.currentAccount(), proxyType, proxyHost, proxyPort,
        proxyUsername, proxyPassword);

    QStringList networkProxyTypes;
    networkProxyTypes.reserve(3);

    // Allow only "No proxy", "Http proxy" and "Socks5 proxy"
    // If proxy type parsed from the settings is different, fall back to
    // "No proxy". Also treat QNetworkProxy::DefaultProxy as "No proxy" because
    // the default proxy type of QNetworkProxy is QNetworkProxy::NoProxy

    QString noProxyItem = tr("No proxy");
    QString httpProxyItem = tr("Http proxy");
    QString socks5ProxyItem = tr("Socks5 proxy");

    networkProxyTypes << noProxyItem;
    networkProxyTypes << httpProxyItem;
    networkProxyTypes << socks5ProxyItem;

    m_pUi->networkProxyTypeComboBox->setModel(
        new QStringListModel(networkProxyTypes, this));

    if ((proxyType == QNetworkProxy::NoProxy) ||
        (proxyType == QNetworkProxy::DefaultProxy))
    {
        m_pUi->networkProxyTypeComboBox->setCurrentIndex(0);
    }
    else if (proxyType == QNetworkProxy::HttpProxy) {
        m_pUi->networkProxyTypeComboBox->setCurrentIndex(1);
    }
    else if (proxyType == QNetworkProxy::Socks5Proxy) {
        m_pUi->networkProxyTypeComboBox->setCurrentIndex(2);
    }
    else {
        QNINFO(
            "preferences",
            "Detected unsupported proxy type: "
                << proxyType << ", falling back to no proxy");

        m_pUi->networkProxyTypeComboBox->setCurrentIndex(0);
    }

    m_pUi->networkProxyHostLineEdit->setText(proxyHost);
    m_pUi->networkProxyPortSpinBox->setMinimum(0);

    m_pUi->networkProxyPortSpinBox->setMaximum(
        std::max(std::numeric_limits<quint16>::max() - 1, 0));

    m_pUi->networkProxyPortSpinBox->setValue(std::max(0, proxyPort));
    m_pUi->networkProxyUserLineEdit->setText(proxyUsername);
    m_pUi->networkProxyPasswordLineEdit->setText(proxyPassword);
}

void PreferencesDialog::setupNoteEditorPreferences()
{
    QNDEBUG("preferences", "PreferencesDialog::setupNoteEditorPreferences");

    ApplicationSettings appSettings(
        m_accountManager.currentAccount(),
        preferences::keys::files::userInterface);

    appSettings.beginGroup(preferences::keys::noteEditorGroup);

    bool useLimitedFonts =
        appSettings.value(preferences::keys::noteEditorUseLimitedSetOfFonts)
            .toBool();

    QString fontColorCode =
        appSettings.value(preferences::keys::noteEditorFontColor).toString();

    QString backgroundColorCode =
        appSettings.value(preferences::keys::noteEditorBackgroundColor)
            .toString();

    QString highlightColorCode =
        appSettings.value(preferences::keys::noteEditorHighlightColor)
            .toString();

    QString highlightedTextColorCode =
        appSettings.value(preferences::keys::noteEditorHighlightedTextColor)
            .toString();

    appSettings.endGroup();

    m_pUi->limitedFontsCheckBox->setChecked(useLimitedFonts);

    QPalette pal = palette();

    QColor fontColor(fontColorCode);
    if (fontColor.isValid()) {
        setNoteEditorFontColorToDemoFrame(fontColor);
        m_pUi->noteEditorFontColorLineEdit->setText(fontColorCode);
    }
    else {
        QColor color = pal.color(QPalette::WindowText);
        setNoteEditorFontColorToDemoFrame(color);
        m_pUi->noteEditorFontColorLineEdit->setText(color.name());
    }

    QColor backgroundColor(backgroundColorCode);
    if (backgroundColor.isValid()) {
        setNoteEditorBackgroundColorToDemoFrame(backgroundColor);
        m_pUi->noteEditorBackgroundColorLineEdit->setText(backgroundColorCode);
    }
    else {
        QColor color = pal.color(QPalette::Base);
        setNoteEditorBackgroundColorToDemoFrame(color);
        m_pUi->noteEditorBackgroundColorLineEdit->setText(color.name());
    }

    QColor highlightColor(highlightColorCode);
    if (highlightColor.isValid()) {
        setNoteEditorHighlightColorToDemoFrame(highlightColor);
        m_pUi->noteEditorHighlightColorLineEdit->setText(highlightColorCode);
    }
    else {
        QColor color = pal.color(QPalette::Highlight);
        setNoteEditorHighlightColorToDemoFrame(color);
        m_pUi->noteEditorHighlightColorLineEdit->setText(color.name());
    }

    QColor highlightedTextColor(highlightedTextColorCode);
    if (highlightedTextColor.isValid()) {
        setNoteEditorHighlightedTextColorToDemoFrame(highlightedTextColor);

        m_pUi->noteEditorHighlightedTextColorLineEdit->setText(
            highlightedTextColorCode);
    }
    else {
        QColor color = pal.color(QPalette::HighlightedText);
        setNoteEditorHighlightedTextColorToDemoFrame(color);
        m_pUi->noteEditorHighlightedTextColorLineEdit->setText(color.name());
    }
}

void PreferencesDialog::createConnections()
{
    QNDEBUG("preferences", "PreferencesDialog::createConnections");

    QObject::connect(
        m_pUi->okPushButton, &QPushButton::clicked, this,
        &PreferencesDialog::accept);

    QObject::connect(
        m_pUi->showSystemTrayIconCheckBox, &QCheckBox::toggled, this,
        &PreferencesDialog::onShowSystemTrayIconCheckboxToggled);

    QObject::connect(
        m_pUi->closeToTrayCheckBox, &QCheckBox::toggled, this,
        &PreferencesDialog::onCloseToSystemTrayCheckboxToggled);

    QObject::connect(
        m_pUi->minimizeToTrayCheckBox, &QCheckBox::toggled, this,
        &PreferencesDialog::onMinimizeToSystemTrayCheckboxToggled);

    QObject::connect(
        m_pUi->startFromTrayCheckBox, &QCheckBox::toggled, this,
        &PreferencesDialog::onStartMinimizedToSystemTrayCheckboxToggled);

    QObject::connect(
        m_pUi->startAtLoginCheckBox, &QCheckBox::toggled, this,
        &PreferencesDialog::onStartAtLoginCheckboxToggled);

#if QT_VERSION >= QT_VERSION_CHECK(5, 7, 0)
    QObject::connect(
        m_pUi->startAtLoginOptionComboBox,
        qOverload<int>(&QComboBox::currentIndexChanged), this,
        &PreferencesDialog::onStartAtLoginOptionChanged);

    QObject::connect(
        m_pUi->traySingleClickActionComboBox,
        qOverload<int>(&QComboBox::currentIndexChanged), this,
        &PreferencesDialog::onSingleClickTrayActionChanged);

    QObject::connect(
        m_pUi->trayMiddleClickActionComboBox,
        qOverload<int>(&QComboBox::currentIndexChanged), this,
        &PreferencesDialog::onMiddleClickTrayActionChanged);

    QObject::connect(
        m_pUi->trayDoubleClickActionComboBox,
        qOverload<int>(&QComboBox::currentIndexChanged), this,
        &PreferencesDialog::onDoubleClickTrayActionChanged);
#else
    QObject::connect(
        m_pUi->startAtLoginOptionComboBox, SIGNAL(currentIndexChanged(int)),
        this, SLOT(onStartAtLoginOptionChanged(int)));

    QObject::connect(
        m_pUi->traySingleClickActionComboBox, SIGNAL(currentIndexChanged(int)),
        this, SLOT(onSingleClickTrayActionChanged(int)));

    QObject::connect(
        m_pUi->trayMiddleClickActionComboBox, SIGNAL(currentIndexChanged(int)),
        this, SLOT(onMiddleClickTrayActionChanged(int)));

    QObject::connect(
        m_pUi->trayDoubleClickActionComboBox, SIGNAL(currentIndexChanged(int)),
        this, SLOT(onDoubleClickTrayActionChanged(int)));
#endif

#ifdef WITH_UPDATE_MANAGER
    QObject::connect(
        m_pUi->checkForUpdatesCheckBox, &QCheckBox::toggled, this,
        &PreferencesDialog::onCheckForUpdatesCheckboxToggled);

    QObject::connect(
        m_pUi->checkForUpdatesOnStartupCheckBox, &QCheckBox::toggled, this,
        &PreferencesDialog::onCheckForUpdatesOnStartupCheckboxToggled);

    QObject::connect(
        m_pUi->useContinuousUpdateChannelCheckBox, &QCheckBox::toggled, this,
        &PreferencesDialog::onUseContinuousUpdateChannelCheckboxToggled);

#if QT_VERSION >= QT_VERSION_CHECK(5, 7, 0)
    QObject::connect(
        m_pUi->checkForUpdatesIntervalComboBox,
        qOverload<int>(&QComboBox::currentIndexChanged), this,
        &PreferencesDialog::onCheckForUpdatesIntervalChanged);

    QObject::connect(
        m_pUi->updateChannelComboBox,
        qOverload<int>(&QComboBox::currentIndexChanged), this,
        &PreferencesDialog::onUpdateChannelChanged);

    QObject::connect(
        m_pUi->updateProviderComboBox,
        qOverload<int>(&QComboBox::currentIndexChanged), this,
        &PreferencesDialog::onUpdateProviderChanged);
#else
    QObject::connect(
        m_pUi->checkForUpdatesIntervalComboBox,
        SIGNAL(currentIndexChanged(int)), this,
        SLOT(onCheckForUpdatesIntervalChanged(int)));

    QObject::connect(
        m_pUi->updateChannelComboBox, SIGNAL(currentIndexChanged(int)), this,
        SLOT(onUpdateChannelChanged(int)));

    QObject::connect(
        m_pUi->updateProviderComboBox, SIGNAL(currentIndexChanged(int)), this,
        SLOT(onUpdateProviderChanged(int)));
#endif

    QObject::connect(
        m_pUi->checkForUpdatesPushButton, &QPushButton::pressed, this,
        &PreferencesDialog::checkForUpdatesRequested);

#endif // WITH_UPDATE_MANAGER

    QObject::connect(
        m_pUi->filterBySelectedNotebookCheckBox, &QCheckBox::toggled, this,
        &PreferencesDialog::onFilterByNotebookCheckboxToggled);

    QObject::connect(
        m_pUi->filterBySelectedTagCheckBox, &QCheckBox::toggled, this,
        &PreferencesDialog::onFilterByTagCheckboxToggled);

    QObject::connect(
        m_pUi->filterBySelectedSavedSearchCheckBox, &QCheckBox::toggled, this,
        &PreferencesDialog::onFilterBySavedSearchCheckboxToggled);

    QObject::connect(
        m_pUi->filterBySelectedFavoritedItemsCheckBox, &QCheckBox::toggled,
        this, &PreferencesDialog::onFilterByFavoritedItemsCheckboxToggled);

    QObject::connect(
        m_pUi->showNoteThumbnailsCheckBox, &QCheckBox::toggled, this,
        &PreferencesDialog::onShowNoteThumbnailsCheckboxToggled);

    QObject::connect(
        m_pUi->disableNativeMenuBarCheckBox, &QCheckBox::toggled, this,
        &PreferencesDialog::onDisableNativeMenuBarCheckboxToggled);

    QObject::connect(
        m_pUi->limitedFontsCheckBox, &QCheckBox::toggled, this,
        &PreferencesDialog::onNoteEditorUseLimitedFontsCheckboxToggled);

    QObject::connect(
        m_pUi->noteEditorFontColorLineEdit, &QLineEdit::editingFinished, this,
        &PreferencesDialog::onNoteEditorFontColorCodeEntered);

    QObject::connect(
        m_pUi->noteEditorFontColorPushButton, &QPushButton::clicked, this,
        &PreferencesDialog::onNoteEditorFontColorDialogRequested);

    QObject::connect(
        m_pUi->noteEditorBackgroundColorLineEdit, &QLineEdit::editingFinished,
        this, &PreferencesDialog::onNoteEditorBackgroundColorCodeEntered);

    QObject::connect(
        m_pUi->noteEditorBackgroundColorPushButton, &QPushButton::clicked, this,
        &PreferencesDialog::onNoteEditorBackgroundColorDialogRequested);

    QObject::connect(
        m_pUi->noteEditorHighlightColorLineEdit, &QLineEdit::editingFinished,
        this, &PreferencesDialog::onNoteEditorHighlightColorCodeEntered);

    QObject::connect(
        m_pUi->noteEditorHighlightColorPushButton, &QPushButton::clicked, this,
        &PreferencesDialog::onNoteEditorHighlightColorDialogRequested);

    QObject::connect(
        m_pUi->noteEditorHighlightedTextColorLineEdit,
        &QLineEdit::editingFinished, this,
        &PreferencesDialog::onNoteEditorHighlightedTextColorCodeEntered);

    QObject::connect(
        m_pUi->noteEditorHighlightedTextColorPushButton, &QPushButton::clicked,
        this,
        &PreferencesDialog::onNoteEditorHighlightedTextColorDialogRequested);

    QObject::connect(
        m_pUi->noteEditorResetColorsPushButton, &QPushButton::clicked, this,
        &PreferencesDialog::onNoteEditorColorsReset);

    QObject::connect(
        m_pUi->downloadNoteThumbnailsCheckBox, &QCheckBox::toggled, this,
        &PreferencesDialog::onDownloadNoteThumbnailsCheckboxToggled);

    QObject::connect(
        m_pUi->downloadInkNoteImagesCheckBox, &QCheckBox::toggled, this,
        &PreferencesDialog::onDownloadInkNoteImagesCheckboxToggled);

#if QT_VERSION >= QT_VERSION_CHECK(5, 7, 0)
    QObject::connect(
        m_pUi->networkProxyTypeComboBox,
        qOverload<int>(&QComboBox::currentIndexChanged), this,
        &PreferencesDialog::onNetworkProxyTypeChanged);

    QObject::connect(
        m_pUi->networkProxyPortSpinBox, qOverload<int>(&QSpinBox::valueChanged),
        this, &PreferencesDialog::onNetworkProxyPortChanged);
#else
    QObject::connect(
        m_pUi->networkProxyTypeComboBox, SIGNAL(currentIndexChanged(int)), this,
        SLOT(onNetworkProxyTypeChanged(int)));

    QObject::connect(
        m_pUi->networkProxyPortSpinBox, SIGNAL(valueChanged(int)), this,
        SLOT(onNetworkProxyPortChanged(int)));
#endif

    QObject::connect(
        m_pUi->networkProxyHostLineEdit, &QLineEdit::editingFinished, this,
        &PreferencesDialog::onNetworkProxyHostChanged);

    QObject::connect(
        m_pUi->networkProxyUserLineEdit, &QLineEdit::editingFinished, this,
        &PreferencesDialog::onNetworkProxyUsernameChanged);

    QObject::connect(
        m_pUi->networkProxyPasswordLineEdit, &QLineEdit::editingFinished, this,
        &PreferencesDialog::onNetworkProxyPasswordChanged);

    QObject::connect(
        m_pUi->networkProxyPasswordShowCheckBox, &QCheckBox::toggled, this,
        &PreferencesDialog::onNetworkProxyPasswordVisibilityToggled);

    QObject::connect(
        m_pUi->enableInternalLogViewerLogsCheckBox, &QCheckBox::toggled, this,
        &PreferencesDialog::onEnableLogViewerInternalLogsCheckboxToggled);

    QObject::connect(
        m_pUi->runSyncOnStartupCheckBox, &QCheckBox::toggled, this,
        &PreferencesDialog::onRunSyncOnStartupOptionChanged);

    QObject::connect(
        m_pUi->panelColorsHandlerWidget,
        &PanelColorsHandlerWidget::fontColorChanged, this,
        &PreferencesDialog::panelFontColorChanged);

    QObject::connect(
        m_pUi->panelColorsHandlerWidget,
        &PanelColorsHandlerWidget::backgroundColorChanged, this,
        &PreferencesDialog::panelBackgroundColorChanged);

    QObject::connect(
        m_pUi->panelColorsHandlerWidget,
        &PanelColorsHandlerWidget::useBackgroundGradientSettingChanged, this,
        &PreferencesDialog::panelUseBackgroundGradientSettingChanged);

    QObject::connect(
        m_pUi->panelColorsHandlerWidget,
        &PanelColorsHandlerWidget::backgroundLinearGradientChanged, this,
        &PreferencesDialog::panelBackgroundLinearGradientChanged);

    QObject::connect(
        m_pUi->panelColorsHandlerWidget,
        &PanelColorsHandlerWidget::notifyUserError, this,
        &PreferencesDialog::onPanelColorWidgetUserError);
}

void PreferencesDialog::installEventFilters()
{
    QNDEBUG("preferences", "PreferencesDialog::installEventFilters");

    m_pUi->noteEditorFontColorDemoFrame->installEventFilter(this);
    m_pUi->noteEditorBackgroundColorDemoFrame->installEventFilter(this);
    m_pUi->noteEditorHighlightColorDemoFrame->installEventFilter(this);
    m_pUi->noteEditorHighlightedTextColorDemoFrame->installEventFilter(this);
}

void PreferencesDialog::checkAndSetNetworkProxy()
{
    QNDEBUG("preferences", "PreferencesDialog::checkAndSetNetworkProxy");

    QNetworkProxy proxy;

    int proxyTypeIndex = m_pUi->networkProxyTypeComboBox->currentIndex();
    switch (proxyTypeIndex) {
    case 1:
        proxy.setType(QNetworkProxy::HttpProxy);
        break;
    case 2:
        proxy.setType(QNetworkProxy::Socks5Proxy);
        break;
    default:
    {
        proxy.setType(QNetworkProxy::NoProxy);

        persistNetworkProxySettingsForAccount(
            m_accountManager.currentAccount(), proxy);

        break;
    }
    }

    QUrl proxyUrl = m_pUi->networkProxyHostLineEdit->text();
    if (!proxyUrl.isValid()) {
        m_pUi->synchronizationTabStatusLabel->setText(
            QStringLiteral("<span style=\"color:#ff0000;\">") +
            tr("Network proxy host is not a valid URL") +
            QStringLiteral("</span>"));

        m_pUi->synchronizationTabStatusLabel->show();

        QNDEBUG(
            "preferences",
            "Invalid network proxy host: "
                << m_pUi->networkProxyHostLineEdit->text()
                << ", resetting the application proxy to no proxy");

        proxy = QNetworkProxy(QNetworkProxy::NoProxy);

        persistNetworkProxySettingsForAccount(
            m_accountManager.currentAccount(), proxy);

        QNetworkProxy::setApplicationProxy(proxy);
        return;
    }

    proxy.setHostName(m_pUi->networkProxyHostLineEdit->text());

    int proxyPort = m_pUi->networkProxyPortSpinBox->value();
    if (Q_UNLIKELY(
            (proxyPort < 0) ||
            (proxyPort >= std::numeric_limits<quint16>::max())))
    {
        m_pUi->synchronizationTabStatusLabel->setText(
            QStringLiteral("<span style=\"color:#ff0000;\">") +
            tr("Network proxy port is outside the allowed range") +
            QStringLiteral("</span>"));

        m_pUi->synchronizationTabStatusLabel->show();

        QNDEBUG(
            "preferences",
            "Invalid network proxy port: "
                << proxyPort
                << ", resetting the application proxy to no proxy");

        proxy = QNetworkProxy(QNetworkProxy::NoProxy);

        persistNetworkProxySettingsForAccount(
            m_accountManager.currentAccount(), proxy);

        QNetworkProxy::setApplicationProxy(proxy);
        return;
    }

    proxy.setPort(static_cast<quint16>(proxyPort));
    proxy.setUser(m_pUi->networkProxyUserLineEdit->text());
    proxy.setPassword(m_pUi->networkProxyPasswordLineEdit->text());

    QNDEBUG(
        "preferences",
        "Setting the application proxy: host = "
            << proxy.hostName() << ", port = " << proxy.port()
            << ", username = " << proxy.user());

    m_pUi->synchronizationTabStatusLabel->clear();
    m_pUi->synchronizationTabStatusLabel->hide();

    persistNetworkProxySettingsForAccount(
        m_accountManager.currentAccount(), proxy);

    QNetworkProxy::setApplicationProxy(proxy);
}

bool PreferencesDialog::onNoteEditorColorEnteredImpl(
    const QColor & color, const QColor & prevColor, const char * key,
    QLineEdit & colorLineEdit, QFrame & demoFrame)
{
    if (!color.isValid()) {
        colorLineEdit.setText(prevColor.name());
        return false;
    }

    if (prevColor.isValid() && (prevColor.name() == color.name())) {
        QNDEBUG("preferences", "Color did not change");
        return false;
    }

    setNoteEditorColorToDemoFrameImpl(color, demoFrame);
    saveNoteEditorColorImpl(color, key);
    return true;
}

void PreferencesDialog::setNoteEditorFontColorToDemoFrame(const QColor & color)
{
    setNoteEditorColorToDemoFrameImpl(
        color, *m_pUi->noteEditorFontColorDemoFrame);
}

void PreferencesDialog::setNoteEditorBackgroundColorToDemoFrame(
    const QColor & color)
{
    setNoteEditorColorToDemoFrameImpl(
        color, *m_pUi->noteEditorBackgroundColorDemoFrame);
}

void PreferencesDialog::setNoteEditorHighlightColorToDemoFrame(
    const QColor & color)
{
    setNoteEditorColorToDemoFrameImpl(
        color, *m_pUi->noteEditorHighlightColorDemoFrame);
}

void PreferencesDialog::setNoteEditorHighlightedTextColorToDemoFrame(
    const QColor & color)
{
    setNoteEditorColorToDemoFrameImpl(
        color, *m_pUi->noteEditorHighlightedTextColorDemoFrame);
}

void PreferencesDialog::setNoteEditorColorToDemoFrameImpl(
    const QColor & color, QFrame & frame)
{
    QPalette pal = frame.palette();
    pal.setColor(QPalette::Background, color);
    frame.setPalette(pal);
}

QColor PreferencesDialog::noteEditorFontColor() const
{
    QColor color = noteEditorColorImpl(preferences::keys::noteEditorFontColor);
    if (color.isValid()) {
        return color;
    }

    return palette().color(QPalette::WindowText);
}

QColor PreferencesDialog::noteEditorBackgroundColor() const
{
    QColor color =
        noteEditorColorImpl(preferences::keys::noteEditorBackgroundColor);

    if (color.isValid()) {
        return color;
    }

    return palette().color(QPalette::Base);
}

QColor PreferencesDialog::noteEditorHighlightColor() const
{
    QColor color =
        noteEditorColorImpl(preferences::keys::noteEditorHighlightColor);

    if (color.isValid()) {
        return color;
    }

    return palette().color(QPalette::Highlight);
}

QColor PreferencesDialog::noteEditorHighlightedTextColor() const
{
    QColor color =
        noteEditorColorImpl(preferences::keys::noteEditorHighlightedTextColor);

    if (color.isValid()) {
        return color;
    }

    return palette().color(QPalette::HighlightedText);
}

QColor PreferencesDialog::noteEditorColorImpl(const char * key) const
{
    ApplicationSettings appSettings(
        m_accountManager.currentAccount(),
        preferences::keys::files::userInterface);

    appSettings.beginGroup(preferences::keys::noteEditorGroup);
    QColor color(appSettings.value(key).toString());
    appSettings.endGroup();

    return color;
}

void PreferencesDialog::saveNoteEditorFontColor(const QColor & color)
{
    QNDEBUG(
        "preferences",
        "PreferencesDialog::saveNoteEditorFontColor: " << color.name());

    saveNoteEditorColorImpl(color, preferences::keys::noteEditorFontColor);
}

void PreferencesDialog::saveNoteEditorBackgroundColor(const QColor & color)
{
    QNDEBUG(
        "preferences",
        "PreferencesDialog::saveNoteEditorBackgroundColor: " << color.name());

    saveNoteEditorColorImpl(
        color, preferences::keys::noteEditorBackgroundColor);
}

void PreferencesDialog::saveNoteEditorHighlightColor(const QColor & color)
{
    QNDEBUG(
        "preferences",
        "PreferencesDialog::saveNoteEditorHighlightColor: " << color.name());

    saveNoteEditorColorImpl(color, preferences::keys::noteEditorHighlightColor);
}

void PreferencesDialog::saveNoteEditorHighlightedTextColor(const QColor & color)
{
    QNDEBUG(
        "preferences",
        "PreferencesDialog::saveNoteEditorHighlightedTextColor: "
            << color.name());

    saveNoteEditorColorImpl(
        color, preferences::keys::noteEditorHighlightedTextColor);
}

void PreferencesDialog::saveNoteEditorColorImpl(
    const QColor & color, const char * key)
{
    ApplicationSettings appSettings(
        m_accountManager.currentAccount(),
        preferences::keys::files::userInterface);

    appSettings.beginGroup(preferences::keys::noteEditorGroup);
    appSettings.setValue(key, color.name());
    appSettings.endGroup();
}

QString trayActionToString(SystemTrayIconManager::TrayAction action)
{
    switch (action) {
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
