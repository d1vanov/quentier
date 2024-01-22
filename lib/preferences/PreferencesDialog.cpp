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
    QDialog{parent},
    m_ui{new Ui::PreferencesDialog}, m_accountManager{accountManager},
    m_systemTrayIconManager{systemTrayIconManager},
    m_trayActionsModel{new QStringListModel(this)},
    m_networkProxyTypesModel{new QStringListModel(this)},
    m_startAtLoginOptionsModel{new QStringListModel(this)}
{
    m_ui->setupUi(this);

    m_ui->disableNativeMenuBarRestartWarningLabel->setHidden(true);
    m_ui->statusTextLabel->setHidden(true);

    m_ui->okPushButton->setDefault(false);
    m_ui->okPushButton->setAutoDefault(false);

    setWindowTitle(tr("Preferences"));

    setupInitialPreferencesState(actionsInfo, shortcutManager);
    createConnections();
    installEventFilters();
    adjustSize();
}

PreferencesDialog::~PreferencesDialog()
{
    delete m_ui;
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

    ApplicationSettings appSettings{
        m_accountManager.currentAccount(),
        preferences::keys::files::userInterface};

    {
        appSettings.beginGroup(preferences::keys::systemTrayGroup);
        ApplicationSettings::GroupCloser groupCloser{appSettings};
        appSettings.setValue(preferences::keys::showSystemTrayIcon, checked);
    }

    // Change the enabled/disabled status of all the tray actions but the one
    // to show/hide the system tray icon
    m_ui->closeToTrayCheckBox->setEnabled(checked);
    m_ui->minimizeToTrayCheckBox->setEnabled(checked);
    m_ui->startFromTrayCheckBox->setEnabled(checked);

    m_ui->traySingleClickActionComboBox->setEnabled(checked);
    m_ui->trayMiddleClickActionComboBox->setEnabled(checked);
    m_ui->trayDoubleClickActionComboBox->setEnabled(checked);

    const bool shown = m_systemTrayIconManager.isShown();
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

    ApplicationSettings appSettings{
        m_accountManager.currentAccount(),
        preferences::keys::files::userInterface};

    appSettings.beginGroup(preferences::keys::systemTrayGroup);
    ApplicationSettings::GroupCloser groupCloser{appSettings};
    appSettings.setValue(preferences::keys::closeToSystemTray, checked);
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

    ApplicationSettings appSettings{
        m_accountManager.currentAccount(),
        preferences::keys::files::userInterface};

    appSettings.beginGroup(preferences::keys::systemTrayGroup);
    ApplicationSettings::GroupCloser groupCloser{appSettings};
    appSettings.setValue(preferences::keys::minimizeToSystemTray, checked);
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

    ApplicationSettings appSettings{
        m_accountManager.currentAccount(),
        preferences::keys::files::userInterface};

    appSettings.beginGroup(preferences::keys::systemTrayGroup);
    ApplicationSettings::GroupCloser groupCloser{appSettings};
    appSettings.setValue(
        preferences::keys::startMinimizedToSystemTray, checked);
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

    ApplicationSettings appSettings{
        m_accountManager.currentAccount(),
        preferences::keys::files::userInterface};

    appSettings.beginGroup(preferences::keys::systemTrayGroup);
    ApplicationSettings::GroupCloser groupCloser{appSettings};
    appSettings.setValue(preferences::keys::singleClickTrayAction, action);
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

    ApplicationSettings appSettings{
        m_accountManager.currentAccount(),
        preferences::keys::files::userInterface};

    appSettings.beginGroup(preferences::keys::systemTrayGroup);
    ApplicationSettings::GroupCloser groupCloser{appSettings};
    appSettings.setValue(preferences::keys::middleClickTrayAction, action);
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

    ApplicationSettings appSettings{
        m_accountManager.currentAccount(),
        preferences::keys::files::userInterface};

    appSettings.beginGroup(preferences::keys::systemTrayGroup);
    ApplicationSettings::GroupCloser groupCloser{appSettings};
    appSettings.setValue(preferences::keys::doubleClickTrayAction, action);
}

void PreferencesDialog::onShowNoteThumbnailsCheckboxToggled(bool checked)
{
    QNDEBUG(
        "preferences",
        "PreferencesDialog::onShowNoteThumbnailsCheckboxToggled: checked = "
            << (checked ? "checked" : "unchecked"));

    ApplicationSettings appSettings{
        m_accountManager.currentAccount(),
        preferences::keys::files::userInterface};

    appSettings.beginGroup(preferences::keys::appearanceGroup);
    ApplicationSettings::GroupCloser groupCloser{appSettings};
    appSettings.setValue(preferences::keys::showNoteThumbnails, checked);

    Q_EMIT showNoteThumbnailsOptionChanged();
}

void PreferencesDialog::onDisableNativeMenuBarCheckboxToggled(bool checked)
{
    QNDEBUG(
        "preferences",
        "PreferencesDialog::onDisableNativeMenuBarCheckboxToggled: "
            << "checked = " << (checked ? "checked" : "unchecked"));

    ApplicationSettings appSettings{
        m_accountManager.currentAccount(),
        preferences::keys::files::userInterface};

    appSettings.beginGroup(preferences::keys::appearanceGroup);
    ApplicationSettings::GroupCloser groupCloser{appSettings};
    appSettings.setValue(preferences::keys::disableNativeMenuBar, checked);

    m_ui->disableNativeMenuBarRestartWarningLabel->setVisible(true);

    Q_EMIT disableNativeMenuBarOptionChanged();
}

void PreferencesDialog::onPanelColorWidgetUserError(QString errorMessage)
{
    if (Q_UNLIKELY(errorMessage.isEmpty())) {
        return;
    }

    m_ui->statusTextLabel->setText(errorMessage);
    m_ui->statusTextLabel->show();

    if (m_clearAndHideStatusBarTimerId != 0) {
        killTimer(m_clearAndHideStatusBarTimerId);
    }

    m_clearAndHideStatusBarTimerId = startTimer(15000);
}

void PreferencesDialog::onIconThemeChanged(int iconThemeIndex)
{
    QString iconThemeName = m_ui->iconThemeComboBox->itemText(iconThemeIndex);
    Q_EMIT iconThemeChanged(iconThemeName);
}

void PreferencesDialog::onStartAtLoginCheckboxToggled(bool checked)
{
    QNDEBUG(
        "preferences",
        "PreferencesDialog::onStartAtLoginCheckboxToggled: "
            << (checked ? "checked" : "unchecked"));

    ErrorString errorDescription;
    const bool res = setStartQuentierAtLoginOption(
        checked, errorDescription,
        static_cast<StartQuentierAtLoginOption::type>(
            m_ui->startAtLoginOptionComboBox->currentIndex()));

    if (Q_UNLIKELY(!res)) {
        m_ui->statusTextLabel->setText(
            tr("Failed to change start at login option") +
            QStringLiteral(": ") + errorDescription.localizedString());

        m_ui->statusTextLabel->show();
        checked = isQuentierSetToStartAtLogin().first;
    }
    else {
        m_ui->statusTextLabel->setText(QString());
        m_ui->statusTextLabel->hide();
    }

    m_ui->startAtLoginOptionComboBox->setEnabled(checked);
}

void PreferencesDialog::onStartAtLoginOptionChanged(int option)
{
    QNDEBUG(
        "preferences",
        "PreferencesDialog::onStartAtLoginOptionChanged: "
            << "option = " << option);

    ErrorString errorDescription;
    const bool res = setStartQuentierAtLoginOption(
        m_ui->startAtLoginCheckBox->isChecked(), errorDescription,
        static_cast<StartQuentierAtLoginOption::type>(option));

    if (Q_UNLIKELY(!res)) {
        setupStartAtLoginPreferences();

        m_ui->statusTextLabel->setText(
            tr("Failed to change start at login option") +
            QStringLiteral(": ") + errorDescription.localizedString());

        m_ui->statusTextLabel->show();
    }
    else {
        m_ui->statusTextLabel->setText(QString());
        m_ui->statusTextLabel->hide();
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
    {
        appSettings.beginGroup(preferences::keys::checkForUpdatesGroup);
        ApplicationSettings::GroupCloser groupCloser{appSettings};
        appSettings.setValue(preferences::keys::checkForUpdates, checked);
    }

    Q_EMIT checkForUpdatesOptionChanged(checked);
}

void PreferencesDialog::onCheckForUpdatesOnStartupCheckboxToggled(bool checked)
{
    QNDEBUG(
        "preferences",
        "PreferencesDialog::onCheckForUpdatesOnStartupCheckboxToggled: "
            << (checked ? "checked" : "unchecked"));

    ApplicationSettings appSettings;
    {
        appSettings.beginGroup(preferences::keys::checkForUpdatesGroup);
        ApplicationSettings::GroupCloser groupCloser{appSettings};
        appSettings.setValue(
            preferences::keys::checkForUpdatesOnStartup, checked);
    }

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
    {
        appSettings.beginGroup(preferences::keys::checkForUpdatesGroup);
        ApplicationSettings::GroupCloser groupCloser{appSettings};

        appSettings.setValue(
            preferences::keys::useContinuousUpdateChannel, checked);
    }

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
    {
        appSettings.beginGroup(preferences::keys::checkForUpdatesGroup);
        ApplicationSettings::GroupCloser groupCloser{appSettings};
        appSettings.setValue(preferences::keys::checkForUpdatesInterval, msec);
    }

    Q_EMIT checkForUpdatesIntervalChanged(msec);
}

void PreferencesDialog::onUpdateChannelChanged(int index)
{
    QNDEBUG(
        "preferences", "PreferencesDialog::onUpdateChannelChanged: " << index);

    QString channel =
        (index == 0) ? QStringLiteral("master") : QStringLiteral("development");

    ApplicationSettings appSettings;
    {
        appSettings.beginGroup(preferences::keys::checkForUpdatesGroup);
        ApplicationSettings::GroupCloser groupCloser{appSettings};
        appSettings.setValue(
            preferences::keys::checkForUpdatesChannel, channel);
    }

    Q_EMIT updateChannelChanged(channel);
}

void PreferencesDialog::onUpdateProviderChanged(int index)
{
    auto provider = static_cast<UpdateProvider>(index);

    QNDEBUG(
        "preferences",
        "PreferencesDialog::onUpdateProviderChanged: " << provider);

    ApplicationSettings appSettings;
    {
        appSettings.beginGroup(preferences::keys::checkForUpdatesGroup);
        ApplicationSettings::GroupCloser groupCloser{appSettings};
        appSettings.setValue(preferences::keys::checkForUpdatesProvider, index);
    }

    bool usingGitHubProvider = (provider == UpdateProvider::GITHUB);
    m_ui->useContinuousUpdateChannelCheckBox->setEnabled(usingGitHubProvider);
    m_ui->updateChannelComboBox->setEnabled(usingGitHubProvider);

    Q_EMIT updateProviderChanged(provider);
}
#endif // WITH_UPDATE_MANAGER

void PreferencesDialog::onFilterByNotebookCheckboxToggled(bool checked)
{
    QNDEBUG(
        "preferences",
        "PreferencesDialog::onFilterByNotebookCheckboxToggled: "
            << (checked ? "checked" : "unchecked"));

    ApplicationSettings appSettings{
        m_accountManager.currentAccount(),
        preferences::keys::files::userInterface};

    {
        appSettings.beginGroup(
            preferences::keys::sidePanelsFilterBySelectionGroup);
        ApplicationSettings::GroupCloser groupCloser{appSettings};
        appSettings.setValue(
            preferences::keys::sidePanelsFilterBySelectedNotebook, checked);
    }

    Q_EMIT filterByNotebookOptionChanged(checked);
}

void PreferencesDialog::onFilterByTagCheckboxToggled(bool checked)
{
    QNDEBUG(
        "preferences",
        "PreferencesDialog::onFilterByTagCheckboxToggled: "
            << (checked ? "checked" : "unchecked"));

    ApplicationSettings appSettings{
        m_accountManager.currentAccount(),
        preferences::keys::files::userInterface};

    {
        appSettings.beginGroup(
            preferences::keys::sidePanelsFilterBySelectionGroup);
        ApplicationSettings::GroupCloser groupCloser{appSettings};
        appSettings.setValue(
            preferences::keys::sidePanelsFilterBySelectedTag, checked);
    }

    Q_EMIT filterByTagOptionChanged(checked);
}

void PreferencesDialog::onFilterBySavedSearchCheckboxToggled(bool checked)
{
    QNDEBUG(
        "preferences",
        "PreferencesDialog::onFilterBySavedSearchCheckboxToggled: "
            << (checked ? "checked" : "unchecked"));

    ApplicationSettings appSettings{
        m_accountManager.currentAccount(),
        preferences::keys::files::userInterface};

    {
        appSettings.beginGroup(
            preferences::keys::sidePanelsFilterBySelectionGroup);
        ApplicationSettings::GroupCloser groupCloser{appSettings};
        appSettings.setValue(
            preferences::keys::sidePanelsFilterBySelectedSavedSearch, checked);
    }

    Q_EMIT filterBySavedSearchOptionChanged(checked);
}

void PreferencesDialog::onFilterByFavoritedItemsCheckboxToggled(bool checked)
{
    QNDEBUG(
        "preferences",
        "PreferencesDialog::onFilterByFavoritedItemsCheckboxToggled: "
            << (checked ? "checked" : "unchecked"));

    ApplicationSettings appSettings{
        m_accountManager.currentAccount(),
        preferences::keys::files::userInterface};

    {
        appSettings.beginGroup(
            preferences::keys::sidePanelsFilterBySelectionGroup);
        ApplicationSettings::GroupCloser groupCloser{appSettings};
        appSettings.setValue(
            preferences::keys::sidePanelsFilterBySelectedFavoritedItems,
            checked);
    }

    Q_EMIT filterByFavoritedItemsOptionChanged(checked);
}

void PreferencesDialog::onNoteEditorUseLimitedFontsCheckboxToggled(bool checked)
{
    QNDEBUG(
        "preferences",
        "PreferencesDialog::onNoteEditorUseLimitedFontsCheckboxToggled: "
            << (checked ? "checked" : "unchecked"));

    ApplicationSettings appSettings{
        m_accountManager.currentAccount(),
        preferences::keys::files::userInterface};

    {
        appSettings.beginGroup(preferences::keys::noteEditorGroup);
        ApplicationSettings::GroupCloser groupCloser{appSettings};
        appSettings.setValue(
            preferences::keys::noteEditorUseLimitedSetOfFonts, checked);
    }

    Q_EMIT noteEditorUseLimitedFontsOptionChanged(checked);
}

void PreferencesDialog::onNoteEditorFontColorCodeEntered()
{
    const QString colorCode = m_ui->noteEditorFontColorLineEdit->text();

    QNDEBUG(
        "preferences",
        "PreferencesDialog::onNoteEditorFontColorCodeEntered: " << colorCode);

    const QColor prevColor = noteEditorFontColor();
    const QColor color{colorCode};

    bool res = onNoteEditorColorEnteredImpl(
        color, prevColor, preferences::keys::noteEditorFontColor,
        *m_ui->noteEditorFontColorLineEdit,
        *m_ui->noteEditorFontColorDemoFrame);

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

    if (!m_noteEditorFontColorDialog.isNull()) {
        QNDEBUG("preferences", "Dialog is already opened");
        return;
    }

    auto pColorDialog = std::make_unique<QColorDialog>(this);
    pColorDialog->setWindowModality(Qt::WindowModal);
    pColorDialog->setCurrentColor(noteEditorFontColor());

    m_noteEditorFontColorDialog = pColorDialog.get();

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

    m_ui->noteEditorFontColorLineEdit->setText(color.name());
    setNoteEditorFontColorToDemoFrame(color);

    const QColor previousFontColor = noteEditorFontColor();
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
    const QString colorCode = m_ui->noteEditorBackgroundColorLineEdit->text();

    QNDEBUG(
        "preferences",
        "PreferencesDialog::onNoteEditorBackgroundColorCodeEntered: "
            << colorCode);

    const QColor prevColor = noteEditorBackgroundColor();
    const QColor color{colorCode};

    const bool res = onNoteEditorColorEnteredImpl(
        color, prevColor, preferences::keys::noteEditorBackgroundColor,
        *m_ui->noteEditorBackgroundColorLineEdit,
        *m_ui->noteEditorBackgroundColorDemoFrame);

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

    if (!m_noteEditorBackgroundColorDialog.isNull()) {
        QNDEBUG("preferences", "Dialog is already opened");
        return;
    }

    auto pColorDialog = std::make_unique<QColorDialog>(this);
    pColorDialog->setWindowModality(Qt::WindowModal);
    pColorDialog->setCurrentColor(noteEditorBackgroundColor());

    m_noteEditorBackgroundColorDialog = pColorDialog.get();

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

    m_ui->noteEditorBackgroundColorLineEdit->setText(color.name());
    setNoteEditorBackgroundColorToDemoFrame(color);

    const QColor previousBackgroundColor = noteEditorBackgroundColor();
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
    const QString colorCode = m_ui->noteEditorHighlightColorLineEdit->text();

    QNDEBUG(
        "preferences",
        "PreferencesDialog::onNoteEditorHighlightColorCodeEntered: "
            << colorCode);

    const QColor prevColor = noteEditorHighlightColor();
    const QColor color{colorCode};

    const bool res = onNoteEditorColorEnteredImpl(
        color, prevColor, preferences::keys::noteEditorHighlightColor,
        *m_ui->noteEditorHighlightColorLineEdit,
        *m_ui->noteEditorHighlightColorDemoFrame);

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

    if (!m_noteEditorHighlightColorDialog.isNull()) {
        QNDEBUG("preferences", "Dialog is already opened");
        return;
    }

    auto pColorDialog = std::make_unique<QColorDialog>(this);
    pColorDialog->setWindowModality(Qt::WindowModal);
    pColorDialog->setCurrentColor(noteEditorHighlightColor());

    m_noteEditorHighlightColorDialog = pColorDialog.get();

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

    m_ui->noteEditorHighlightColorLineEdit->setText(color.name());
    setNoteEditorHighlightColorToDemoFrame(color);

    const QColor previousHighlightColor = noteEditorHighlightColor();
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
    const QString colorCode =
        m_ui->noteEditorHighlightedTextColorLineEdit->text();

    QNDEBUG(
        "preferences",
        "PreferencesDialog::onNoteEditorHighlightedTextColorCodeEntered: "
            << colorCode);

    const QColor prevColor = noteEditorHighlightedTextColor();
    const QColor color{colorCode};

    const bool res = onNoteEditorColorEnteredImpl(
        color, prevColor, preferences::keys::noteEditorHighlightedTextColor,
        *m_ui->noteEditorHighlightedTextColorLineEdit,
        *m_ui->noteEditorHighlightedTextColorDemoFrame);

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

    if (!m_noteEditorHighlightedTextColorDialog.isNull()) {
        QNDEBUG("preferences", "Dialog is already opened");
        return;
    }

    auto pColorDialog = std::make_unique<QColorDialog>(this);
    pColorDialog->setWindowModality(Qt::WindowModal);
    pColorDialog->setCurrentColor(noteEditorHighlightedTextColor());

    m_noteEditorHighlightedTextColorDialog = pColorDialog.get();

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

    m_ui->noteEditorHighlightedTextColorLineEdit->setText(color.name());
    setNoteEditorHighlightedTextColorToDemoFrame(color);

    const QColor previousHighlightedTextColor =
        noteEditorHighlightedTextColor();

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

    ApplicationSettings appSettings{
        m_accountManager.currentAccount(),
        preferences::keys::files::userInterface};

    {
        appSettings.beginGroup(preferences::keys::noteEditorGroup);
        ApplicationSettings::GroupCloser groupCloser{appSettings};
        appSettings.remove(preferences::keys::noteEditorFontColor);
        appSettings.remove(preferences::keys::noteEditorBackgroundColor);
        appSettings.remove(preferences::keys::noteEditorHighlightColor);
        appSettings.remove(preferences::keys::noteEditorHighlightedTextColor);
    }

    QPalette pal = palette();

    const QColor fontColor = pal.color(QPalette::WindowText);
    setNoteEditorFontColorToDemoFrame(fontColor);
    m_ui->noteEditorFontColorLineEdit->setText(fontColor.name());

    const QColor backgroundColor = pal.color(QPalette::Base);
    setNoteEditorBackgroundColorToDemoFrame(backgroundColor);
    m_ui->noteEditorBackgroundColorLineEdit->setText(backgroundColor.name());

    const QColor highlightColor = pal.color(QPalette::Highlight);
    setNoteEditorHighlightColorToDemoFrame(highlightColor);
    m_ui->noteEditorHighlightColorLineEdit->setText(highlightColor.name());

    const QColor highlightedTextColor = pal.color(QPalette::HighlightedText);
    setNoteEditorHighlightedTextColorToDemoFrame(highlightedTextColor);

    m_ui->noteEditorHighlightedTextColorLineEdit->setText(
        highlightedTextColor.name());

    Q_EMIT noteEditorColorsReset();
}

void PreferencesDialog::onDownloadNoteThumbnailsCheckboxToggled(bool checked)
{
    QNDEBUG(
        "preferences",
        "PreferencesDialog::onDownloadNoteThumbnailsCheckboxToggled: "
            << (checked ? "checked" : "unchecked"));

    ApplicationSettings appSettings{
        m_accountManager.currentAccount(),
        preferences::keys::files::synchronization};

    {
        appSettings.beginGroup(preferences::keys::synchronizationGroup);
        ApplicationSettings::GroupCloser groupCloser{appSettings};
        appSettings.setValue(
            preferences::keys::downloadNoteThumbnails, checked);
    }

    Q_EMIT synchronizationDownloadNoteThumbnailsOptionChanged(checked);
}

void PreferencesDialog::onDownloadInkNoteImagesCheckboxToggled(bool checked)
{
    QNDEBUG(
        "preferences",
        "PreferencesDialog::onDownloadInkNoteImagesCheckboxToggled: "
            << (checked ? "checked" : "unchecked"));

    ApplicationSettings appSettings{
        m_accountManager.currentAccount(),
        preferences::keys::files::synchronization};

    {
        appSettings.beginGroup(preferences::keys::synchronizationGroup);
        ApplicationSettings::GroupCloser groupCloser{appSettings};
        appSettings.setValue(preferences::keys::downloadInkNoteImages, checked);
    }

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

    ApplicationSettings syncSettings{
        m_accountManager.currentAccount(),
        preferences::keys::files::synchronization};

    {
        syncSettings.beginGroup(preferences::keys::synchronizationGroup);
        ApplicationSettings::GroupCloser groupCloser{syncSettings};
        syncSettings.setValue(
            preferences::keys::runSyncPeriodMinutes, runSyncEachNumMinutes);
    }

    Q_EMIT runSyncPeriodicallyOptionChanged(runSyncEachNumMinutes);
}

void PreferencesDialog::onRunSyncOnStartupOptionChanged(bool checked)
{
    QNDEBUG(
        "preferences",
        "PreferencesDialog::onRunSyncOnStartupOptionChanged: checked = "
            << (checked ? "true" : "false"));

    ApplicationSettings syncSettings{
        m_accountManager.currentAccount(),
        preferences::keys::files::synchronization};

    {
        syncSettings.beginGroup(preferences::keys::synchronizationGroup);
        ApplicationSettings::GroupCloser groupCloser{syncSettings};
        syncSettings.setValue(preferences::keys::runSyncOnStartup, checked);
    }

    Q_EMIT runSyncOnStartupChanged(checked);
}

void PreferencesDialog::onNetworkProxyTypeChanged(int type)
{
    QNDEBUG(
        "preferences",
        "PreferencesDialog::onNetworkProxyTypeChanged: type = " << type);

    checkAndSetNetworkProxy();
}

void PreferencesDialog::onNetworkProxyHostChanged()
{
    QNDEBUG(
        "preferences",
        "PreferencesDialog::onNetworkProxyHostChanged: "
            << m_ui->networkProxyHostLineEdit->text());

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
            << m_ui->networkProxyUserLineEdit->text());

    checkAndSetNetworkProxy();
}

void PreferencesDialog::onNetworkProxyPasswordChanged()
{
    QNDEBUG(
        "preferences",
        "PreferencesDialog::onNetworkProxyPasswordChanged: "
            << m_ui->networkProxyPasswordLineEdit->text());

    checkAndSetNetworkProxy();
}

void PreferencesDialog::onNetworkProxyPasswordVisibilityToggled(bool checked)
{
    QNDEBUG(
        "preferences",
        "PreferencesDialog::onNetworkProxyPasswordVisibilityToggled: "
            << "checked = " << (checked ? "true" : "false"));

    m_ui->networkProxyPasswordLineEdit->setEchoMode(
        checked ? QLineEdit::Normal : QLineEdit::Password);
}

bool PreferencesDialog::eventFilter(QObject * object, QEvent * event)
{
    if (object == m_ui->noteEditorFontColorDemoFrame) {
        if (event && (event->type() == QEvent::MouseButtonDblClick)) {
            onNoteEditorFontColorDialogRequested();
            return true;
        }
    }
    else if (object == m_ui->noteEditorBackgroundColorDemoFrame) {
        if (event && (event->type() == QEvent::MouseButtonDblClick)) {
            onNoteEditorBackgroundColorDialogRequested();
            return true;
        }
    }
    else if (object == m_ui->noteEditorHighlightColorDemoFrame) {
        if (event && (event->type() == QEvent::MouseButtonDblClick)) {
            onNoteEditorHighlightColorDialogRequested();
            return true;
        }
    }
    else if (object == m_ui->noteEditorHighlightedTextColorDemoFrame) {
        if (event && (event->type() == QEvent::MouseButtonDblClick)) {
            onNoteEditorHighlightedTextColorDialogRequested();
            return true;
        }
    }

    return QDialog::eventFilter(object, event);
}

void PreferencesDialog::timerEvent(QTimerEvent * event)
{
    if (Q_UNLIKELY(!event)) {
        return;
    }

    int timerId = event->timerId();
    killTimer(timerId);

    if (timerId == m_clearAndHideStatusBarTimerId) {
        m_ui->statusTextLabel->setText({});
        m_ui->statusTextLabel->hide();
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
    ApplicationSettings::GroupCloser groupCloser{globalAppSettings};
    globalAppSettings.setValue(
        preferences::keys::enableLogViewerInternalLogs, checked);
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
    m_ui->panelColorsHandlerWidget->initialize(currentAccount);

    // 4) Behaviour tab
    setupStartAtLoginPreferences();
    setupCheckForUpdatesPreferences();
    setupFilteringPreferences();

    // 5) Synchronization tab
    if (currentAccount.type() == Account::Type::Local) {
        // Remove the synchronization tab entirely
        m_ui->preferencesTabWidget->removeTab(
            m_ui->preferencesTabWidget->indexOf(m_ui->syncTab));
    }
    else {
        ApplicationSettings syncSettings{
            currentAccount, preferences::keys::files::synchronization};

        syncSettings.beginGroup(preferences::keys::synchronizationGroup);
        auto groupCloser =
            std::make_unique<ApplicationSettings::GroupCloser>(syncSettings);

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

        groupCloser.reset();

        m_ui->downloadNoteThumbnailsCheckBox->setChecked(
            downloadNoteThumbnails);

        m_ui->downloadInkNoteImagesCheckBox->setChecked(downloadInkNoteImages);

        setupNetworkProxyPreferences();
        setupRunSyncPeriodicallyComboBox(runSyncEachNumMinutes);
        m_ui->runSyncOnStartupCheckBox->setChecked(runSyncOnStartup);

        m_ui->synchronizationTabStatusLabel->hide();
    }

    // 6) Shortcuts tab
    m_ui->shortcutSettingsWidget->initialize(
        currentAccount, actionsInfo, &shortcutManager);

    // 7) Auxiliary tab
    ApplicationSettings globalAppSettings;
    globalAppSettings.beginGroup(preferences::keys::loggingGroup);
    auto groupCloser =
        std::make_unique<ApplicationSettings::GroupCloser>(globalAppSettings);

    const QVariant enableLogViewerInternalLogsValue =
        globalAppSettings.value(preferences::keys::enableLogViewerInternalLogs);

    groupCloser.reset();

    bool enableLogViewerInternalLogs = false;
    if (enableLogViewerInternalLogsValue.isValid()) {
        enableLogViewerInternalLogs = enableLogViewerInternalLogsValue.toBool();
    }

    m_ui->enableInternalLogViewerLogsCheckBox->setChecked(
        enableLogViewerInternalLogs);
}

void PreferencesDialog::setupSystemTrayPreferences()
{
#ifdef Q_WS_MAC
    // It makes little sense to minimize to tray on Mac
    // because the minimized app goes to dock
    m_ui->minimizeToTrayCheckBox->setDisabled(true);
    m_ui->minimizeToTrayCheckBox->setHidden(true);
#endif

    if (!m_systemTrayIconManager.isSystemTrayAvailable()) {
        QNDEBUG(
            "preferences",
            "The system tray is not available, setting up "
                << "the blank system tray settings");

        m_ui->showSystemTrayIconCheckBox->setChecked(false);
        m_ui->showSystemTrayIconCheckBox->setDisabled(true);

        m_ui->closeToTrayCheckBox->setChecked(false);
        m_ui->closeToTrayCheckBox->setDisabled(true);

        m_ui->minimizeToTrayCheckBox->setChecked(false);
        m_ui->minimizeToTrayCheckBox->setDisabled(true);

        m_ui->startFromTrayCheckBox->setChecked(false);
        m_ui->startFromTrayCheckBox->setDisabled(true);

        m_trayActionsModel->setStringList(QStringList());
        m_ui->traySingleClickActionComboBox->setModel(m_trayActionsModel);
        m_ui->trayMiddleClickActionComboBox->setModel(m_trayActionsModel);
        m_ui->trayDoubleClickActionComboBox->setModel(m_trayActionsModel);

        return;
    }

    ApplicationSettings appSettings{
        m_accountManager.currentAccount(),
        preferences::keys::files::userInterface};

    appSettings.beginGroup(preferences::keys::systemTrayGroup);
    auto groupCloser =
        std::make_unique<ApplicationSettings::GroupCloser>(appSettings);

    bool shouldShowSystemTrayIcon = preferences::defaults::showSystemTrayIcon;

    const QVariant shouldShowSystemTrayIconData =
        appSettings.value(preferences::keys::showSystemTrayIcon);

    if (shouldShowSystemTrayIconData.isValid()) {
        shouldShowSystemTrayIcon = shouldShowSystemTrayIconData.toBool();
    }

    bool shouldCloseToTray = preferences::defaults::closeToSystemTray;

    const QVariant shouldCloseToTrayData =
        appSettings.value(preferences::keys::closeToSystemTray);

    if (shouldCloseToTrayData.isValid()) {
        shouldCloseToTray = shouldCloseToTrayData.toBool();
    }

    bool shouldMinimizeToTray = preferences::defaults::minimizeToSystemTray;

    const QVariant shouldMinimizeToTrayData =
        appSettings.value(preferences::keys::minimizeToSystemTray);

    if (shouldMinimizeToTrayData.isValid()) {
        shouldMinimizeToTray = shouldMinimizeToTrayData.toBool();
    }

    bool shouldStartMinimizedToTray =
        preferences::defaults::startMinimizedToSystemTray;

    const QVariant shouldStartMinimizedToTrayData =
        appSettings.value(preferences::keys::startMinimizedToSystemTray);

    if (shouldStartMinimizedToTrayData.isValid()) {
        shouldStartMinimizedToTray = shouldStartMinimizedToTrayData.toBool();
    }

    groupCloser.reset();

    m_ui->showSystemTrayIconCheckBox->setChecked(shouldShowSystemTrayIcon);
    m_ui->closeToTrayCheckBox->setChecked(shouldCloseToTray);
    m_ui->minimizeToTrayCheckBox->setChecked(shouldMinimizeToTray);
    m_ui->startFromTrayCheckBox->setChecked(shouldStartMinimizedToTray);

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

    m_trayActionsModel->setStringList(trayActions);

    m_ui->traySingleClickActionComboBox->setModel(m_trayActionsModel);
    m_ui->trayMiddleClickActionComboBox->setModel(m_trayActionsModel);
    m_ui->trayDoubleClickActionComboBox->setModel(m_trayActionsModel);

    auto singleClickTrayAction =
        m_systemTrayIconManager.singleClickTrayAction();

    m_ui->traySingleClickActionComboBox->setCurrentIndex(
        singleClickTrayAction);

    auto middleClickTrayAction =
        m_systemTrayIconManager.middleClickTrayAction();

    m_ui->trayMiddleClickActionComboBox->setCurrentIndex(
        middleClickTrayAction);

    auto doubleClickTrayAction =
        m_systemTrayIconManager.doubleClickTrayAction();

    m_ui->trayDoubleClickActionComboBox->setCurrentIndex(
        doubleClickTrayAction);

    // If the tray icon is now shown, disable all the tray actions but the one
    // to show/hide the system tray icon
    if (!m_systemTrayIconManager.isShown()) {
        m_ui->closeToTrayCheckBox->setDisabled(true);
        m_ui->minimizeToTrayCheckBox->setDisabled(true);
        m_ui->startFromTrayCheckBox->setDisabled(true);

        m_ui->traySingleClickActionComboBox->setDisabled(true);
        m_ui->trayMiddleClickActionComboBox->setDisabled(true);
        m_ui->trayDoubleClickActionComboBox->setDisabled(true);
    }
}

void PreferencesDialog::setupStartAtLoginPreferences()
{
    QNDEBUG("preferences", "PreferencesDialog::setupStartAtLoginPreferences");

    auto startAtLoginOptions = isQuentierSetToStartAtLogin();
    m_ui->startAtLoginCheckBox->setChecked(startAtLoginOptions.first);

    QStringList startAtLoginOptionsList;
    startAtLoginOptionsList.reserve(3);
    startAtLoginOptionsList << tr("Start minimized to tray");
    startAtLoginOptionsList << tr("Start minimized to taskbar");
    startAtLoginOptionsList << tr("Start maximized or windowed");

    m_startAtLoginOptionsModel->setStringList(startAtLoginOptionsList);
    m_ui->startAtLoginOptionComboBox->setModel(m_startAtLoginOptionsModel);

    m_ui->startAtLoginOptionComboBox->setCurrentIndex(
        static_cast<int>(startAtLoginOptions.second));

    m_ui->startAtLoginOptionComboBox->setEnabled(startAtLoginOptions.first);
}

void PreferencesDialog::setupCheckForUpdatesPreferences()
{
    QNDEBUG(
        "preferences", "PreferencesDialog::setupCheckForUpdatesPreferences");

#ifndef WITH_UPDATE_MANAGER
    m_ui->checkForUpdatesCheckBox->setVisible(false);
    m_ui->checkForUpdatesOnStartupCheckBox->setVisible(false);
    m_ui->useContinuousUpdateChannelCheckBox->setVisible(false);
    m_ui->checkForUpdatesIntervalLabel->setVisible(false);
    m_ui->checkForUpdatesIntervalComboBox->setVisible(false);
    m_ui->updateProviderLabel->setVisible(false);
    m_ui->updateProviderComboBox->setVisible(false);
    m_ui->updateChannelLabel->setVisible(false);
    m_ui->updateChannelComboBox->setVisible(false);
    m_ui->checkForUpdatesPushButton->setVisible(false);
    m_ui->updateSettingsGroupBox->setVisible(false);
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
    m_ui->updateProviderComboBox->hide();
#endif

    m_ui->checkForUpdatesCheckBox->setChecked(checkForUpdatesEnabled);

    m_ui->checkForUpdatesOnStartupCheckBox->setChecked(
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

    m_ui->checkForUpdatesIntervalComboBox->setModel(
        pCheckForUpdatesIntervalComboBoxModel);

    m_ui->checkForUpdatesIntervalComboBox->setCurrentIndex(
        checkForUpdatesIntervalOption);

    m_ui->useContinuousUpdateChannelCheckBox->setChecked(
        useContinuousUpdateChannel);

    QStringList updateChannels;
    updateChannels.reserve(2);
    updateChannels << tr("Stable");
    updateChannels << tr("Unstable");

    auto * pUpdateChannelsComboBoxModel = new QStringListModel(this);
    pUpdateChannelsComboBoxModel->setStringList(updateChannels);
    m_ui->updateChannelComboBox->setModel(pUpdateChannelsComboBoxModel);

    if (updateChannel == QStringLiteral("development")) {
        m_ui->updateChannelComboBox->setCurrentIndex(1);
    }
    else {
        m_ui->updateChannelComboBox->setCurrentIndex(0);
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
    m_ui->updateProviderComboBox->setModel(pUpdateProvidersComboBoxModel);

    m_ui->updateProviderComboBox->setCurrentIndex(
        static_cast<int>(updateProvider));

    if (updateProvider == UpdateProvider::APPIMAGE) {
        m_ui->useContinuousUpdateChannelCheckBox->setEnabled(false);
        m_ui->updateChannelComboBox->setEnabled(false);
    }
#endif // WITH_UPDATE_MANAGER
}

void PreferencesDialog::setupFilteringPreferences()
{
    QNDEBUG("preferences", "PreferencesDialog::setupFilteringPreferences");

    ApplicationSettings appSettings{
        m_accountManager.currentAccount(),
        preferences::keys::files::userInterface};

    appSettings.beginGroup(preferences::keys::sidePanelsFilterBySelectionGroup);
    auto groupCloser =
        std::make_unique<ApplicationSettings::GroupCloser>(appSettings);

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

    groupCloser.reset();

    m_ui->filterBySelectedNotebookCheckBox->setChecked(filterByNotebook);
    m_ui->filterBySelectedTagCheckBox->setChecked(filterByTag);
    m_ui->filterBySelectedSavedSearchCheckBox->setChecked(filterBySavedSearch);
    m_ui->filterBySelectedFavoritedItemsCheckBox->setChecked(
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

    m_ui->runSyncPeriodicallyComboBox->setModel(
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

    m_ui->runSyncPeriodicallyComboBox->setCurrentIndex(currentIndex);

    QObject::connect(
        m_ui->runSyncPeriodicallyComboBox,
        qOverload<int>(&QComboBox::currentIndexChanged), this,
        &PreferencesDialog::onRunSyncPeriodicallyOptionChanged);
}

void PreferencesDialog::setupAppearancePreferences(
    const ActionsInfo & actionsInfo)
{
    QNDEBUG("preferences", "PreferencesDialog::setupAppearancePreferences");

    ApplicationSettings appSettings{
        m_accountManager.currentAccount(),
        preferences::keys::files::userInterface};

    appSettings.beginGroup(preferences::keys::appearanceGroup);
    auto groupCloser =
        std::make_unique<ApplicationSettings::GroupCloser>(appSettings);

    const QVariant showThumbnails = appSettings.value(
        preferences::keys::showNoteThumbnails,
        QVariant::fromValue(preferences::defaults::showNoteThumbnails));

    const QVariant disableNativeMenuBar = appSettings.value(
        preferences::keys::disableNativeMenuBar,
        QVariant::fromValue(preferences::defaults::disableNativeMenuBar()));

    const QVariant iconTheme =
        appSettings.value(preferences::keys::iconTheme, tr("Native"));

    groupCloser.reset();

    m_ui->showNoteThumbnailsCheckBox->setChecked(showThumbnails.toBool());

    m_ui->disableNativeMenuBarCheckBox->setChecked(
        disableNativeMenuBar.toBool());

    bool hasNativeIconTheme = false;
    if (!actionsInfo.findActionInfo(tr("Native"), tr("View")).isEmpty()) {
        hasNativeIconTheme = true;
    }

    if (hasNativeIconTheme) {
        m_ui->iconThemeComboBox->addItem(tr("Native"));
    }

    m_ui->iconThemeComboBox->addItem(QStringLiteral("breeze"));
    m_ui->iconThemeComboBox->addItem(QStringLiteral("breeze-dark"));
    m_ui->iconThemeComboBox->addItem(QStringLiteral("oxygen"));
    m_ui->iconThemeComboBox->addItem(QStringLiteral("tango"));

    int iconThemeIndex =
        m_ui->iconThemeComboBox->findText(iconTheme.toString());

    if (iconThemeIndex >= 0) {
        m_ui->iconThemeComboBox->setCurrentIndex(iconThemeIndex);
    }

#if QT_VERSION >= QT_VERSION_CHECK(5, 7, 0)
    QObject::connect(
        m_ui->iconThemeComboBox,
        qOverload<int>(&QComboBox::currentIndexChanged), this,
        &PreferencesDialog::onIconThemeChanged);
#else
    QObject::connect(
        m_ui->iconThemeComboBox, SIGNAL(currentIndexChanged(int)), this,
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

    m_ui->networkProxyTypeComboBox->setModel(
        new QStringListModel(networkProxyTypes, this));

    if ((proxyType == QNetworkProxy::NoProxy) ||
        (proxyType == QNetworkProxy::DefaultProxy))
    {
        m_ui->networkProxyTypeComboBox->setCurrentIndex(0);
    }
    else if (proxyType == QNetworkProxy::HttpProxy) {
        m_ui->networkProxyTypeComboBox->setCurrentIndex(1);
    }
    else if (proxyType == QNetworkProxy::Socks5Proxy) {
        m_ui->networkProxyTypeComboBox->setCurrentIndex(2);
    }
    else {
        QNINFO(
            "preferences",
            "Detected unsupported proxy type: "
                << proxyType << ", falling back to no proxy");

        m_ui->networkProxyTypeComboBox->setCurrentIndex(0);
    }

    m_ui->networkProxyHostLineEdit->setText(proxyHost);
    m_ui->networkProxyPortSpinBox->setMinimum(0);

    m_ui->networkProxyPortSpinBox->setMaximum(
        std::max(std::numeric_limits<quint16>::max() - 1, 0));

    m_ui->networkProxyPortSpinBox->setValue(std::max(0, proxyPort));
    m_ui->networkProxyUserLineEdit->setText(proxyUsername);
    m_ui->networkProxyPasswordLineEdit->setText(proxyPassword);
}

void PreferencesDialog::setupNoteEditorPreferences()
{
    QNDEBUG("preferences", "PreferencesDialog::setupNoteEditorPreferences");

    ApplicationSettings appSettings{
        m_accountManager.currentAccount(),
        preferences::keys::files::userInterface};

    appSettings.beginGroup(preferences::keys::noteEditorGroup);
    auto groupCloser =
        std::make_unique<ApplicationSettings::GroupCloser>(appSettings);

    const bool useLimitedFonts =
        appSettings.value(preferences::keys::noteEditorUseLimitedSetOfFonts)
            .toBool();

    const QString fontColorCode =
        appSettings.value(preferences::keys::noteEditorFontColor).toString();

    const QString backgroundColorCode =
        appSettings.value(preferences::keys::noteEditorBackgroundColor)
            .toString();

    const QString highlightColorCode =
        appSettings.value(preferences::keys::noteEditorHighlightColor)
            .toString();

    const QString highlightedTextColorCode =
        appSettings.value(preferences::keys::noteEditorHighlightedTextColor)
            .toString();

    groupCloser.reset();

    m_ui->limitedFontsCheckBox->setChecked(useLimitedFonts);

    const QPalette pal = palette();

    const QColor fontColor{fontColorCode};
    if (fontColor.isValid()) {
        setNoteEditorFontColorToDemoFrame(fontColor);
        m_ui->noteEditorFontColorLineEdit->setText(fontColorCode);
    }
    else {
        const QColor color = pal.color(QPalette::WindowText);
        setNoteEditorFontColorToDemoFrame(color);
        m_ui->noteEditorFontColorLineEdit->setText(color.name());
    }

    const QColor backgroundColor{backgroundColorCode};
    if (backgroundColor.isValid()) {
        setNoteEditorBackgroundColorToDemoFrame(backgroundColor);
        m_ui->noteEditorBackgroundColorLineEdit->setText(backgroundColorCode);
    }
    else {
        const QColor color = pal.color(QPalette::Base);
        setNoteEditorBackgroundColorToDemoFrame(color);
        m_ui->noteEditorBackgroundColorLineEdit->setText(color.name());
    }

    const QColor highlightColor{highlightColorCode};
    if (highlightColor.isValid()) {
        setNoteEditorHighlightColorToDemoFrame(highlightColor);
        m_ui->noteEditorHighlightColorLineEdit->setText(highlightColorCode);
    }
    else {
        const QColor color = pal.color(QPalette::Highlight);
        setNoteEditorHighlightColorToDemoFrame(color);
        m_ui->noteEditorHighlightColorLineEdit->setText(color.name());
    }

    const QColor highlightedTextColor{highlightedTextColorCode};
    if (highlightedTextColor.isValid()) {
        setNoteEditorHighlightedTextColorToDemoFrame(highlightedTextColor);

        m_ui->noteEditorHighlightedTextColorLineEdit->setText(
            highlightedTextColorCode);
    }
    else {
        const QColor color = pal.color(QPalette::HighlightedText);
        setNoteEditorHighlightedTextColorToDemoFrame(color);
        m_ui->noteEditorHighlightedTextColorLineEdit->setText(color.name());
    }
}

void PreferencesDialog::createConnections()
{
    QNDEBUG("preferences", "PreferencesDialog::createConnections");

    QObject::connect(
        m_ui->okPushButton, &QPushButton::clicked, this,
        &PreferencesDialog::accept);

    QObject::connect(
        m_ui->showSystemTrayIconCheckBox, &QCheckBox::toggled, this,
        &PreferencesDialog::onShowSystemTrayIconCheckboxToggled);

    QObject::connect(
        m_ui->closeToTrayCheckBox, &QCheckBox::toggled, this,
        &PreferencesDialog::onCloseToSystemTrayCheckboxToggled);

    QObject::connect(
        m_ui->minimizeToTrayCheckBox, &QCheckBox::toggled, this,
        &PreferencesDialog::onMinimizeToSystemTrayCheckboxToggled);

    QObject::connect(
        m_ui->startFromTrayCheckBox, &QCheckBox::toggled, this,
        &PreferencesDialog::onStartMinimizedToSystemTrayCheckboxToggled);

    QObject::connect(
        m_ui->startAtLoginCheckBox, &QCheckBox::toggled, this,
        &PreferencesDialog::onStartAtLoginCheckboxToggled);

    QObject::connect(
        m_ui->startAtLoginOptionComboBox,
        qOverload<int>(&QComboBox::currentIndexChanged), this,
        &PreferencesDialog::onStartAtLoginOptionChanged);

    QObject::connect(
        m_ui->traySingleClickActionComboBox,
        qOverload<int>(&QComboBox::currentIndexChanged), this,
        &PreferencesDialog::onSingleClickTrayActionChanged);

    QObject::connect(
        m_ui->trayMiddleClickActionComboBox,
        qOverload<int>(&QComboBox::currentIndexChanged), this,
        &PreferencesDialog::onMiddleClickTrayActionChanged);

    QObject::connect(
        m_ui->trayDoubleClickActionComboBox,
        qOverload<int>(&QComboBox::currentIndexChanged), this,
        &PreferencesDialog::onDoubleClickTrayActionChanged);

#ifdef WITH_UPDATE_MANAGER
    QObject::connect(
        m_ui->checkForUpdatesCheckBox, &QCheckBox::toggled, this,
        &PreferencesDialog::onCheckForUpdatesCheckboxToggled);

    QObject::connect(
        m_ui->checkForUpdatesOnStartupCheckBox, &QCheckBox::toggled, this,
        &PreferencesDialog::onCheckForUpdatesOnStartupCheckboxToggled);

    QObject::connect(
        m_ui->useContinuousUpdateChannelCheckBox, &QCheckBox::toggled, this,
        &PreferencesDialog::onUseContinuousUpdateChannelCheckboxToggled);

    QObject::connect(
        m_ui->checkForUpdatesIntervalComboBox,
        qOverload<int>(&QComboBox::currentIndexChanged), this,
        &PreferencesDialog::onCheckForUpdatesIntervalChanged);

    QObject::connect(
        m_ui->updateChannelComboBox,
        qOverload<int>(&QComboBox::currentIndexChanged), this,
        &PreferencesDialog::onUpdateChannelChanged);

    QObject::connect(
        m_ui->updateProviderComboBox,
        qOverload<int>(&QComboBox::currentIndexChanged), this,
        &PreferencesDialog::onUpdateProviderChanged);

    QObject::connect(
        m_ui->checkForUpdatesPushButton, &QPushButton::pressed, this,
        &PreferencesDialog::checkForUpdatesRequested);

#endif // WITH_UPDATE_MANAGER

    QObject::connect(
        m_ui->filterBySelectedNotebookCheckBox, &QCheckBox::toggled, this,
        &PreferencesDialog::onFilterByNotebookCheckboxToggled);

    QObject::connect(
        m_ui->filterBySelectedTagCheckBox, &QCheckBox::toggled, this,
        &PreferencesDialog::onFilterByTagCheckboxToggled);

    QObject::connect(
        m_ui->filterBySelectedSavedSearchCheckBox, &QCheckBox::toggled, this,
        &PreferencesDialog::onFilterBySavedSearchCheckboxToggled);

    QObject::connect(
        m_ui->filterBySelectedFavoritedItemsCheckBox, &QCheckBox::toggled,
        this, &PreferencesDialog::onFilterByFavoritedItemsCheckboxToggled);

    QObject::connect(
        m_ui->showNoteThumbnailsCheckBox, &QCheckBox::toggled, this,
        &PreferencesDialog::onShowNoteThumbnailsCheckboxToggled);

    QObject::connect(
        m_ui->disableNativeMenuBarCheckBox, &QCheckBox::toggled, this,
        &PreferencesDialog::onDisableNativeMenuBarCheckboxToggled);

    QObject::connect(
        m_ui->limitedFontsCheckBox, &QCheckBox::toggled, this,
        &PreferencesDialog::onNoteEditorUseLimitedFontsCheckboxToggled);

    QObject::connect(
        m_ui->noteEditorFontColorLineEdit, &QLineEdit::editingFinished, this,
        &PreferencesDialog::onNoteEditorFontColorCodeEntered);

    QObject::connect(
        m_ui->noteEditorFontColorPushButton, &QPushButton::clicked, this,
        &PreferencesDialog::onNoteEditorFontColorDialogRequested);

    QObject::connect(
        m_ui->noteEditorBackgroundColorLineEdit, &QLineEdit::editingFinished,
        this, &PreferencesDialog::onNoteEditorBackgroundColorCodeEntered);

    QObject::connect(
        m_ui->noteEditorBackgroundColorPushButton, &QPushButton::clicked, this,
        &PreferencesDialog::onNoteEditorBackgroundColorDialogRequested);

    QObject::connect(
        m_ui->noteEditorHighlightColorLineEdit, &QLineEdit::editingFinished,
        this, &PreferencesDialog::onNoteEditorHighlightColorCodeEntered);

    QObject::connect(
        m_ui->noteEditorHighlightColorPushButton, &QPushButton::clicked, this,
        &PreferencesDialog::onNoteEditorHighlightColorDialogRequested);

    QObject::connect(
        m_ui->noteEditorHighlightedTextColorLineEdit,
        &QLineEdit::editingFinished, this,
        &PreferencesDialog::onNoteEditorHighlightedTextColorCodeEntered);

    QObject::connect(
        m_ui->noteEditorHighlightedTextColorPushButton, &QPushButton::clicked,
        this,
        &PreferencesDialog::onNoteEditorHighlightedTextColorDialogRequested);

    QObject::connect(
        m_ui->noteEditorResetColorsPushButton, &QPushButton::clicked, this,
        &PreferencesDialog::onNoteEditorColorsReset);

    QObject::connect(
        m_ui->downloadNoteThumbnailsCheckBox, &QCheckBox::toggled, this,
        &PreferencesDialog::onDownloadNoteThumbnailsCheckboxToggled);

    QObject::connect(
        m_ui->downloadInkNoteImagesCheckBox, &QCheckBox::toggled, this,
        &PreferencesDialog::onDownloadInkNoteImagesCheckboxToggled);

    QObject::connect(
        m_ui->networkProxyTypeComboBox,
        qOverload<int>(&QComboBox::currentIndexChanged), this,
        &PreferencesDialog::onNetworkProxyTypeChanged);

    QObject::connect(
        m_ui->networkProxyPortSpinBox, qOverload<int>(&QSpinBox::valueChanged),
        this, &PreferencesDialog::onNetworkProxyPortChanged);

    QObject::connect(
        m_ui->networkProxyHostLineEdit, &QLineEdit::editingFinished, this,
        &PreferencesDialog::onNetworkProxyHostChanged);

    QObject::connect(
        m_ui->networkProxyUserLineEdit, &QLineEdit::editingFinished, this,
        &PreferencesDialog::onNetworkProxyUsernameChanged);

    QObject::connect(
        m_ui->networkProxyPasswordLineEdit, &QLineEdit::editingFinished, this,
        &PreferencesDialog::onNetworkProxyPasswordChanged);

    QObject::connect(
        m_ui->networkProxyPasswordShowCheckBox, &QCheckBox::toggled, this,
        &PreferencesDialog::onNetworkProxyPasswordVisibilityToggled);

    QObject::connect(
        m_ui->enableInternalLogViewerLogsCheckBox, &QCheckBox::toggled, this,
        &PreferencesDialog::onEnableLogViewerInternalLogsCheckboxToggled);

    QObject::connect(
        m_ui->runSyncOnStartupCheckBox, &QCheckBox::toggled, this,
        &PreferencesDialog::onRunSyncOnStartupOptionChanged);

    QObject::connect(
        m_ui->panelColorsHandlerWidget,
        &PanelColorsHandlerWidget::fontColorChanged, this,
        &PreferencesDialog::panelFontColorChanged);

    QObject::connect(
        m_ui->panelColorsHandlerWidget,
        &PanelColorsHandlerWidget::backgroundColorChanged, this,
        &PreferencesDialog::panelBackgroundColorChanged);

    QObject::connect(
        m_ui->panelColorsHandlerWidget,
        &PanelColorsHandlerWidget::useBackgroundGradientSettingChanged, this,
        &PreferencesDialog::panelUseBackgroundGradientSettingChanged);

    QObject::connect(
        m_ui->panelColorsHandlerWidget,
        &PanelColorsHandlerWidget::backgroundLinearGradientChanged, this,
        &PreferencesDialog::panelBackgroundLinearGradientChanged);

    QObject::connect(
        m_ui->panelColorsHandlerWidget,
        &PanelColorsHandlerWidget::notifyUserError, this,
        &PreferencesDialog::onPanelColorWidgetUserError);
}

void PreferencesDialog::installEventFilters()
{
    QNDEBUG("preferences", "PreferencesDialog::installEventFilters");

    m_ui->noteEditorFontColorDemoFrame->installEventFilter(this);
    m_ui->noteEditorBackgroundColorDemoFrame->installEventFilter(this);
    m_ui->noteEditorHighlightColorDemoFrame->installEventFilter(this);
    m_ui->noteEditorHighlightedTextColorDemoFrame->installEventFilter(this);
}

void PreferencesDialog::checkAndSetNetworkProxy()
{
    QNDEBUG("preferences", "PreferencesDialog::checkAndSetNetworkProxy");

    QNetworkProxy proxy;

    const int proxyTypeIndex = m_ui->networkProxyTypeComboBox->currentIndex();
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

    const QUrl proxyUrl = m_ui->networkProxyHostLineEdit->text();
    if (!proxyUrl.isValid()) {
        m_ui->synchronizationTabStatusLabel->setText(
            QStringLiteral("<span style=\"color:#ff0000;\">") +
            tr("Network proxy host is not a valid URL") +
            QStringLiteral("</span>"));

        m_ui->synchronizationTabStatusLabel->show();

        QNDEBUG(
            "preferences",
            "Invalid network proxy host: "
                << m_ui->networkProxyHostLineEdit->text()
                << ", resetting the application proxy to no proxy");

        proxy = QNetworkProxy(QNetworkProxy::NoProxy);

        persistNetworkProxySettingsForAccount(
            m_accountManager.currentAccount(), proxy);

        QNetworkProxy::setApplicationProxy(proxy);
        return;
    }

    proxy.setHostName(m_ui->networkProxyHostLineEdit->text());

    const int proxyPort = m_ui->networkProxyPortSpinBox->value();
    if (Q_UNLIKELY(
            (proxyPort < 0) ||
            (proxyPort >= std::numeric_limits<quint16>::max())))
    {
        m_ui->synchronizationTabStatusLabel->setText(
            QStringLiteral("<span style=\"color:#ff0000;\">") +
            tr("Network proxy port is outside the allowed range") +
            QStringLiteral("</span>"));

        m_ui->synchronizationTabStatusLabel->show();

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
    proxy.setUser(m_ui->networkProxyUserLineEdit->text());
    proxy.setPassword(m_ui->networkProxyPasswordLineEdit->text());

    QNDEBUG(
        "preferences",
        "Setting the application proxy: host = "
            << proxy.hostName() << ", port = " << proxy.port()
            << ", username = " << proxy.user());

    m_ui->synchronizationTabStatusLabel->clear();
    m_ui->synchronizationTabStatusLabel->hide();

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
        color, *m_ui->noteEditorFontColorDemoFrame);
}

void PreferencesDialog::setNoteEditorBackgroundColorToDemoFrame(
    const QColor & color)
{
    setNoteEditorColorToDemoFrameImpl(
        color, *m_ui->noteEditorBackgroundColorDemoFrame);
}

void PreferencesDialog::setNoteEditorHighlightColorToDemoFrame(
    const QColor & color)
{
    setNoteEditorColorToDemoFrameImpl(
        color, *m_ui->noteEditorHighlightColorDemoFrame);
}

void PreferencesDialog::setNoteEditorHighlightedTextColorToDemoFrame(
    const QColor & color)
{
    setNoteEditorColorToDemoFrameImpl(
        color, *m_ui->noteEditorHighlightedTextColorDemoFrame);
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
    const QColor color =
        noteEditorColorImpl(preferences::keys::noteEditorFontColor);

    if (color.isValid()) {
        return color;
    }

    return palette().color(QPalette::WindowText);
}

QColor PreferencesDialog::noteEditorBackgroundColor() const
{
    const QColor color =
        noteEditorColorImpl(preferences::keys::noteEditorBackgroundColor);

    if (color.isValid()) {
        return color;
    }

    return palette().color(QPalette::Base);
}

QColor PreferencesDialog::noteEditorHighlightColor() const
{
    const QColor color =
        noteEditorColorImpl(preferences::keys::noteEditorHighlightColor);

    if (color.isValid()) {
        return color;
    }

    return palette().color(QPalette::Highlight);
}

QColor PreferencesDialog::noteEditorHighlightedTextColor() const
{
    const QColor color =
        noteEditorColorImpl(preferences::keys::noteEditorHighlightedTextColor);

    if (color.isValid()) {
        return color;
    }

    return palette().color(QPalette::HighlightedText);
}

QColor PreferencesDialog::noteEditorColorImpl(const char * key) const
{
    ApplicationSettings appSettings{
        m_accountManager.currentAccount(),
        preferences::keys::files::userInterface};

    appSettings.beginGroup(preferences::keys::noteEditorGroup);
    ApplicationSettings::GroupCloser groupCloser{appSettings};
    return QColor{appSettings.value(key).toString()};
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
    ApplicationSettings appSettings{
        m_accountManager.currentAccount(),
        preferences::keys::files::userInterface};

    appSettings.beginGroup(preferences::keys::noteEditorGroup);
    ApplicationSettings::GroupCloser groupCloser{appSettings};
    appSettings.setValue(key, color.name());
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
