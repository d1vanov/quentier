/*
 * Copyright 2017-2019 Dmitry Ivanov
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
#include "SettingsNames.h"
#include "DefaultSettings.h"

#include "shortcut_settings/ShortcutSettingsWidget.h"
using quentier::ShortcutSettingsWidget;
#include "ui_PreferencesDialog.h"

#include <lib/account/AccountManager.h>
#include <lib/tray/SystemTrayIconManager.h>
#include <lib/utility/ActionsInfo.h>
#include <lib/utility/NetworkProxySettingsHelpers.h>
#include <lib/utility/MainWindowSideBorderOption.h>
#include <lib/utility/ColorCodeValidator.h>
#include <lib/utility/StartAtLogin.h>

#include <quentier/logging/QuentierLogger.h>
#include <quentier/utility/ApplicationSettings.h>
#include <quentier/utility/ShortcutManager.h>

#include <QStringListModel>
#include <QFileInfo>
#include <QDir>
#include <QNetworkProxy>
#include <QColorDialog>
#include <QScopedPointer>

#include <algorithm>
#include <limits>

namespace quentier {

QString trayActionToString(SystemTrayIconManager::TrayAction action);

PreferencesDialog::PreferencesDialog(AccountManager & accountManager,
                                     ShortcutManager & shortcutManager,
                                     SystemTrayIconManager & systemTrayIconManager,
                                     ActionsInfo & actionsInfo,
                                     QWidget * parent) :
    QDialog(parent),
    m_pUi(new Ui::PreferencesDialog),
    m_accountManager(accountManager),
    m_systemTrayIconManager(systemTrayIconManager),
    m_pTrayActionsModel(new QStringListModel(this)),
    m_pNetworkProxyTypesModel(new QStringListModel(this)),
    m_pAvailableMainWindowBorderOptionsModel(new QStringListModel(this)),
    m_pStartAtLoginOptionsModel(new QStringListModel(this))
{
    m_pUi->setupUi(this);
    // workaroundQtBugWithTabWidget();

    m_pUi->statusTextLabel->setHidden(true);

    m_pUi->okPushButton->setDefault(false);
    m_pUi->okPushButton->setAutoDefault(false);

    setWindowTitle(tr("Preferences"));

    setupCurrentSettingsState(actionsInfo, shortcutManager);
    createConnections();
    adjustSize();
}

PreferencesDialog::~PreferencesDialog()
{
    delete m_pUi;
}

void PreferencesDialog::onShowSystemTrayIconCheckboxToggled(bool checked)
{
    QNDEBUG(QStringLiteral("PreferencesDialog::onShowSystemTrayIconCheckboxToggled: ")
            << QStringLiteral("checked = ")
            << (checked ? QStringLiteral("true") : QStringLiteral("false")));

    if (!m_systemTrayIconManager.isSystemTrayAvailable()) {
        QNDEBUG(QStringLiteral("Ignoring the change of show system tray icon "
                               "checkbox: the system tray is not available"));
        return;
    }

    Account currentAccount = m_accountManager.currentAccount();
    ApplicationSettings appSettings(currentAccount, QUENTIER_UI_SETTINGS);
    appSettings.beginGroup(SYSTEM_TRAY_SETTINGS_GROUP_NAME);
    appSettings.setValue(SHOW_SYSTEM_TRAY_ICON_SETTINGS_KEY, checked);
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
    QNDEBUG(QStringLiteral("PreferencesDialog::onCloseToSystemTrayCheckboxToggled: ")
            << QStringLiteral("checked = ")
            << (checked ? QStringLiteral("true") : QStringLiteral("false")));

    if (!m_systemTrayIconManager.isSystemTrayAvailable()) {
        QNDEBUG(QStringLiteral("Ignoring the change of close to system tray "
                               "checkbox: the system tray is not available"));
        return;
    }

    Account currentAccount = m_accountManager.currentAccount();
    ApplicationSettings appSettings(currentAccount, QUENTIER_UI_SETTINGS);
    appSettings.beginGroup(SYSTEM_TRAY_SETTINGS_GROUP_NAME);
    appSettings.setValue(CLOSE_TO_SYSTEM_TRAY_SETTINGS_KEY, checked);
    appSettings.endGroup();
}

void PreferencesDialog::onMinimizeToSystemTrayCheckboxToggled(bool checked)
{
    QNDEBUG(QStringLiteral("PreferencesDialog::onMinimizeToSystemTrayCheckboxToggled: ")
            << QStringLiteral("checked = ")
            << (checked ? QStringLiteral("true") : QStringLiteral("false")));

    if (!m_systemTrayIconManager.isSystemTrayAvailable()) {
        QNDEBUG(QStringLiteral("Ignoring the change of minimize to system tray "
                               "checkbox: the system tray is not available"));
        return;
    }

    Account currentAccount = m_accountManager.currentAccount();
    ApplicationSettings appSettings(currentAccount, QUENTIER_UI_SETTINGS);
    appSettings.beginGroup(SYSTEM_TRAY_SETTINGS_GROUP_NAME);
    appSettings.setValue(MINIMIZE_TO_SYSTEM_TRAY_SETTINGS_KEY, checked);
    appSettings.endGroup();
}

void PreferencesDialog::onStartMinimizedToSystemTrayCheckboxToggled(bool checked)
{
    QNDEBUG(QStringLiteral("PreferencesDialog::")
            << QStringLiteral("onStartMinimizedToSystemTrayCheckboxToggled: ")
            << QStringLiteral("checked = ")
            << (checked ? QStringLiteral("true") : QStringLiteral("false")));

    if (!m_systemTrayIconManager.isSystemTrayAvailable()) {
        QNDEBUG(QStringLiteral("Ignoring the change of start minimized to system "
                               "tray checkbox: the system tray is not available"));
        return;
    }

    Account currentAccount = m_accountManager.currentAccount();
    ApplicationSettings appSettings(currentAccount, QUENTIER_UI_SETTINGS);
    appSettings.beginGroup(SYSTEM_TRAY_SETTINGS_GROUP_NAME);
    appSettings.setValue(START_MINIMIZED_TO_SYSTEM_TRAY_SETTINGS_KEY, checked);
    appSettings.endGroup();
}

void PreferencesDialog::onSingleClickTrayActionChanged(int action)
{
    QNDEBUG(QStringLiteral("PreferencesDialog::onSingleClickTrayActionChanged: ")
            << action);

    if (!m_systemTrayIconManager.isSystemTrayAvailable()) {
        QNDEBUG(QStringLiteral("Ignoring the change of single click tray action: "
                               "the system tray is not available"));
        return;
    }

    Account currentAccount = m_accountManager.currentAccount();
    ApplicationSettings appSettings(currentAccount, QUENTIER_UI_SETTINGS);
    appSettings.beginGroup(SYSTEM_TRAY_SETTINGS_GROUP_NAME);
    appSettings.setValue(SINGLE_CLICK_TRAY_ACTION_SETTINGS_KEY, action);
    appSettings.endGroup();
}

void PreferencesDialog::onMiddleClickTrayActionChanged(int action)
{
    QNDEBUG(QStringLiteral("PreferencesDialog::onMiddleClickTrayActionChanged: ")
            << action);

    if (!m_systemTrayIconManager.isSystemTrayAvailable()) {
        QNDEBUG(QStringLiteral("Ignoring the change of middle click tray action: "
                               "the system tray is not available"));
        return;
    }

    Account currentAccount = m_accountManager.currentAccount();
    ApplicationSettings appSettings(currentAccount, QUENTIER_UI_SETTINGS);
    appSettings.beginGroup(SYSTEM_TRAY_SETTINGS_GROUP_NAME);
    appSettings.setValue(MIDDLE_CLICK_TRAY_ACTION_SETTINGS_KEY, action);
    appSettings.endGroup();
}

void PreferencesDialog::onDoubleClickTrayActionChanged(int action)
{
    QNDEBUG(QStringLiteral("PreferencesDialog::onDoubleClickTrayActionChanged: ")
            << action);

    if (!m_systemTrayIconManager.isSystemTrayAvailable()) {
        QNDEBUG(QStringLiteral("Ignoring the change of double click tray action: "
                               "the system tray is not available"));
        return;
    }

    Account currentAccount = m_accountManager.currentAccount();
    ApplicationSettings appSettings(currentAccount, QUENTIER_UI_SETTINGS);
    appSettings.beginGroup(SYSTEM_TRAY_SETTINGS_GROUP_NAME);
    appSettings.setValue(DOUBLE_CLICK_TRAY_ACTION_SETTINGS_KEY, action);
    appSettings.endGroup();
}

void PreferencesDialog::onShowNoteThumbnailsCheckboxToggled(bool checked)
{
    QNDEBUG(QStringLiteral("PreferencesDialog::onShowNoteThumbnailsCheckboxToggled: ")
            << QStringLiteral("checked = ")
            << (checked ? QStringLiteral("checked") : QStringLiteral("unchecked")));

    ApplicationSettings appSettings(m_accountManager.currentAccount(),
                                    QUENTIER_UI_SETTINGS);
    appSettings.beginGroup(LOOK_AND_FEEL_SETTINGS_GROUP_NAME);
    appSettings.setValue(SHOW_NOTE_THUMBNAILS_SETTINGS_KEY,
                         QVariant::fromValue(checked));
    appSettings.endGroup();

    Q_EMIT showNoteThumbnailsOptionChanged();
}

void PreferencesDialog::onShowMainWindowLeftBorderOptionChanged(int option)
{
    QNDEBUG(QStringLiteral("PreferencesDialog::")
            << QStringLiteral("onShowMainWindowLeftBorderOptionChanged: ")
            << QStringLiteral("option = ") << option);

    Account currentAccount = m_accountManager.currentAccount();
    ApplicationSettings appSettings(currentAccount, QUENTIER_UI_SETTINGS);
    appSettings.beginGroup(LOOK_AND_FEEL_SETTINGS_GROUP_NAME);
    appSettings.setValue(SHOW_LEFT_MAIN_WINDOW_BORDER_OPTION_KEY, option);
    appSettings.endGroup();

    Q_EMIT showMainWindowLeftBorderOptionChanged(option);
}

void PreferencesDialog::onShowMainWindowRightBorderOptionChanged(int option)
{
    QNDEBUG(QStringLiteral("PreferencesDialog::")
            << QStringLiteral("onShowMainWindowRightBorderOptionChanged: ")
            << QStringLiteral("option = ") << option);

    Account currentAccount = m_accountManager.currentAccount();
    ApplicationSettings appSettings(currentAccount, QUENTIER_UI_SETTINGS);
    appSettings.beginGroup(LOOK_AND_FEEL_SETTINGS_GROUP_NAME);
    appSettings.setValue(SHOW_RIGHT_MAIN_WINDOW_BORDER_OPTION_KEY, option);
    appSettings.endGroup();

    Q_EMIT showMainWindowRightBorderOptionChanged(option);
}

void PreferencesDialog::onLeftMainWindowBorderWidthChanged(int width)
{
    QNDEBUG(QStringLiteral("PreferencesDialog::onLeftMainWindowBorderWidthChanged: ")
            << QStringLiteral("width = ") << width);

    Account currentAccount = m_accountManager.currentAccount();
    ApplicationSettings appSettings(currentAccount, QUENTIER_UI_SETTINGS);
    appSettings.beginGroup(LOOK_AND_FEEL_SETTINGS_GROUP_NAME);
    appSettings.setValue(LEFT_MAIN_WINDOW_BORDER_WIDTH_KEY, width);
    appSettings.endGroup();

    Q_EMIT mainWindowLeftBorderWidthChanged(width);
}

void PreferencesDialog::onRightMainWindowBorderWidthChanged(int width)
{
    QNDEBUG(QStringLiteral("PreferencesDialog::onRightMainWindowBorderWidthChanged: ")
            << QStringLiteral("width = ") << width);

    Account currentAccount = m_accountManager.currentAccount();
    ApplicationSettings appSettings(currentAccount, QUENTIER_UI_SETTINGS);
    appSettings.beginGroup(LOOK_AND_FEEL_SETTINGS_GROUP_NAME);
    appSettings.setValue(RIGHT_MAIN_WINDOW_BORDER_WIDTH_KEY, width);
    appSettings.endGroup();

    Q_EMIT mainWindowRightBorderWidthChanged(width);
}

void PreferencesDialog::onLeftMainWindowBorderColorCodeChanged(
    const QString & colorCode)
{
    QNDEBUG(QStringLiteral("PreferencesDialog::onLeftMainWindowBorderColorCodeChanged: ")
            << QStringLiteral("color code = ") << colorCode);

    if (!colorCode.isEmpty() && !QColor::isValidColor(colorCode)) {
        return;
    }

    Account currentAccount = m_accountManager.currentAccount();
    ApplicationSettings appSettings(currentAccount, QUENTIER_UI_SETTINGS);
    appSettings.beginGroup(LOOK_AND_FEEL_SETTINGS_GROUP_NAME);
    appSettings.setValue(LEFT_MAIN_WINDOW_BORDER_OVERRIDE_COLOR, colorCode);
    appSettings.endGroup();

    Q_EMIT mainWindowLeftBorderColorChanged(colorCode);
}

void PreferencesDialog::onRightMainWindowBorderColorCodeChanged(
    const QString & colorCode)
{
    QNDEBUG(QStringLiteral("PreferencesDialog::onRightMainWindowBorderColorCodeChanged: ")
            << QStringLiteral("color code = ") << colorCode);

    if (!colorCode.isEmpty() && !QColor::isValidColor(colorCode)) {
        return;
    }

    Account currentAccount = m_accountManager.currentAccount();
    ApplicationSettings appSettings(currentAccount, QUENTIER_UI_SETTINGS);
    appSettings.beginGroup(LOOK_AND_FEEL_SETTINGS_GROUP_NAME);
    appSettings.setValue(RIGHT_MAIN_WINDOW_BORDER_OVERRIDE_COLOR, colorCode);
    appSettings.endGroup();

    Q_EMIT mainWindowRightBorderColorChanged(colorCode);
}

void PreferencesDialog::onLeftMainWindowBorderColorPickerRequested()
{
    QNDEBUG(QStringLiteral("PreferencesDialog::"
                           "onLeftMainWindowBorderColorPickerRequested"));

    QScopedPointer<QColorDialog> pColorDialog(new QColorDialog(this));
    pColorDialog->setWindowModality(Qt::WindowModal);

    QString currentColorCode = m_pUi->leftMainWindowBorderColorLineEdit->text();
    if (!currentColorCode.isEmpty() && QColor::isValidColor(currentColorCode)) {
        pColorDialog->setCurrentColor(QColor(currentColorCode));
    }

    QObject::connect(pColorDialog.data(),
                     QNSIGNAL(QColorDialog,colorSelected,QColor),
                     this,
                     QNSLOT(PreferencesDialog,
                            onLeftMainWindowBorderColorSelected,QColor));
    Q_UNUSED(pColorDialog->exec())
}

void PreferencesDialog::onRightMainWindowBorderColorPickerRequested()
{
    QNDEBUG(QStringLiteral("PreferencesDialog::"
                           "onRightMainWindowBorderColorPickerRequested"));

    QScopedPointer<QColorDialog> pColorDialog(new QColorDialog(this));
    pColorDialog->setWindowModality(Qt::WindowModal);

    QString currentColorCode = m_pUi->rightMainWindowBorderColorLineEdit->text();
    if (!currentColorCode.isEmpty() && QColor::isValidColor(currentColorCode)) {
        pColorDialog->setCurrentColor(QColor(currentColorCode));
    }

    QObject::connect(pColorDialog.data(),
                     QNSIGNAL(QColorDialog,colorSelected,QColor),
                     this,
                     QNSLOT(PreferencesDialog,
                            onRightMainWindowBorderColorSelected,QColor));
    Q_UNUSED(pColorDialog->exec())
}

void PreferencesDialog::onLeftMainWindowBorderColorSelected(const QColor & color)
{
    QString colorCode = color.name();
    QNDEBUG(QStringLiteral("PreferencesDialog::onLeftMainWindowBorderColorSelected: ")
            << QStringLiteral("color code = ") << colorCode);

    if (!color.isValid()) {
        colorCode.resize(0);
    }

    m_pUi->leftMainWindowBorderColorLineEdit->setText(colorCode);
}

void PreferencesDialog::onRightMainWindowBorderColorSelected(const QColor & color)
{
    QString colorCode = color.name();
    QNDEBUG(QStringLiteral("PreferencesDialog::onRightMainWindowBorderColorSelected: ")
            << QStringLiteral("color code = ") << colorCode);

    if (!color.isValid()) {
        colorCode.resize(0);
    }

    m_pUi->rightMainWindowBorderColorLineEdit->setText(colorCode);
}

void PreferencesDialog::onStartAtLoginCheckboxToggled(bool checked)
{
    QNDEBUG(QStringLiteral("PreferencesDialog::onStartAtLoginCheckboxToggled: ")
            << (checked ? QStringLiteral("checked") : QStringLiteral("unchecked")));

    ErrorString errorDescription;
    bool res = setStartQuentierAtLoginOption(
        checked, errorDescription,
        static_cast<StartQuentierAtLoginOption::type>(
            m_pUi->startAtLoginOptionComboBox->currentIndex()));

    if (Q_UNLIKELY(!res)) {
        m_pUi->statusTextLabel->setText(tr("Failed to change start at login option") +
                                        QStringLiteral(": ") +
                                        errorDescription.localizedString());
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
    QNDEBUG(QStringLiteral("PreferencesDialog::onStartAtLoginOptionChanged: ")
            << QStringLiteral("option = ") << option);

    ErrorString errorDescription;
    bool res = setStartQuentierAtLoginOption(
        m_pUi->startAtLoginCheckBox->isChecked(), errorDescription,
        static_cast<StartQuentierAtLoginOption::type>(option));
    if (Q_UNLIKELY(!res)) {
        setupStartAtLoginSettings();
        m_pUi->statusTextLabel->setText(tr("Failed to change start at login option") +
                                        QStringLiteral(": ") +
                                        errorDescription.localizedString());
        m_pUi->statusTextLabel->show();
    }
    else {
        m_pUi->statusTextLabel->setText(QString());
        m_pUi->statusTextLabel->hide();
    }
}

void PreferencesDialog::onNoteEditorUseLimitedFontsCheckboxToggled(bool checked)
{
    QNDEBUG(QStringLiteral("PreferencesDialog::")
            << QStringLiteral("onNoteEditorUseLimitedFontsCheckboxToggled: ")
            << (checked ? QStringLiteral("checked") : QStringLiteral("unchecked")));

    Account currentAccount = m_accountManager.currentAccount();
    ApplicationSettings appSettings(currentAccount, QUENTIER_UI_SETTINGS);
    appSettings.beginGroup(NOTE_EDITOR_SETTINGS_GROUP_NAME);
    appSettings.setValue(USE_LIMITED_SET_OF_FONTS, checked);
    appSettings.endGroup();

    Q_EMIT noteEditorUseLimitedFontsOptionChanged(checked);
}

void PreferencesDialog::onNoteEditorFontColorCodeEntered()
{
    QString colorCode = m_pUi->noteEditorFontColorLineEdit->text();

    QNDEBUG(QStringLiteral("PreferencesDialog::onNoteEditorFontColorCodeEntered: ")
            << colorCode);

    QColor prevColor = noteEditorFontColor();

    QColor color(colorCode);
    bool res = onNoteEditorColorEnteredImpl(color, prevColor,
                                            NOTE_EDITOR_FONT_COLOR_SETTINGS_KEY,
                                            *m_pUi->noteEditorFontColorLineEdit,
                                            *m_pUi->noteEditorFontColorDemoFrame);
    if (!res) {
        return;
    }

    Q_EMIT noteEditorFontColorChanged(color);
}

void PreferencesDialog::onNoteEditorFontColorDialogRequested()
{
    QNDEBUG(QStringLiteral("PreferencesDialog::onNoteEditorFontColorDialogRequested"));

    if (!m_pNoteEditorFontColorDialog.isNull()) {
        QNDEBUG(QStringLiteral("Dialog is already opened"));
        return;
    }

    QScopedPointer<QColorDialog> pColorDialog(new QColorDialog(this));
    pColorDialog->setWindowModality(Qt::WindowModal);
    pColorDialog->setCurrentColor(noteEditorFontColor());

    m_pNoteEditorFontColorDialog = pColorDialog.data();

    QObject::connect(pColorDialog.data(),
                     QNSIGNAL(QColorDialog,colorSelected,QColor),
                     this,
                     QNSLOT(PreferencesDialog,onNoteEditorFontColorSelected,
                            QColor));
    Q_UNUSED(pColorDialog->exec())
}

void PreferencesDialog::onNoteEditorFontColorSelected(const QColor & color)
{
    QNDEBUG(QStringLiteral("PreferencesDialog::onNoteEditorFontColorSelected: ")
            << color.name());

    m_pUi->noteEditorFontColorLineEdit->setText(color.name());
    setNoteEditorFontColorToDemoFrame(color);

    QColor previousFontColor = noteEditorFontColor();
    if (previousFontColor.isValid() && color.isValid() &&
        (previousFontColor.name() == color.name()))
    {
        QNDEBUG(QStringLiteral("Note editor font color did not change"));
        return;
    }

    saveNoteEditorFontColor(color);
    Q_EMIT noteEditorFontColorChanged(color);
}

void PreferencesDialog::onNoteEditorBackgroundColorCodeEntered()
{
    QString colorCode = m_pUi->noteEditorBackgroundColorLineEdit->text();

    QNDEBUG(QStringLiteral("PreferencesDialog::")
            << QStringLiteral("onNoteEditorBackgroundColorCodeEntered: ")
            << colorCode);

    QColor prevColor = noteEditorBackgroundColor();

    QColor color(colorCode);
    bool res = onNoteEditorColorEnteredImpl(
        color, prevColor,
        NOTE_EDITOR_BACKGROUND_COLOR_SETTINGS_KEY,
        *m_pUi->noteEditorBackgroundColorLineEdit,
        *m_pUi->noteEditorBackgroundColorDemoFrame);
    if (!res) {
        return;
    }

    Q_EMIT noteEditorBackgroundColorChanged(color);
}

void PreferencesDialog::onNoteEditorBackgroundColorDialogRequested()
{
    QNDEBUG(QStringLiteral("PreferencesDialog::"
                           "onNoteEditorBackgroundColorDialogRequested"));

    if (!m_pNoteEditorBackgroundColorDialog.isNull()) {
        QNDEBUG(QStringLiteral("Dialog is already opened"));
        return;
    }

    QScopedPointer<QColorDialog> pColorDialog(new QColorDialog(this));
    pColorDialog->setWindowModality(Qt::WindowModal);
    pColorDialog->setCurrentColor(noteEditorBackgroundColor());

    m_pNoteEditorBackgroundColorDialog = pColorDialog.data();

    QObject::connect(pColorDialog.data(),
                     QNSIGNAL(QColorDialog,colorSelected,QColor),
                     this,
                     QNSLOT(PreferencesDialog,onNoteEditorBackgroundColorSelected,
                            QColor));
    Q_UNUSED(pColorDialog->exec())
}

void PreferencesDialog::onNoteEditorBackgroundColorSelected(const QColor & color)
{
    QNDEBUG(QStringLiteral("PreferencesDialog::onNoteEditorBackgroundColorSelected: ")
            << color.name());

    m_pUi->noteEditorBackgroundColorLineEdit->setText(color.name());
    setNoteEditorBackgroundColorToDemoFrame(color);

    QColor previousBackgroundColor = noteEditorBackgroundColor();
    if (previousBackgroundColor.isValid() && color.isValid() &&
        (previousBackgroundColor.name() == color.name()))
    {
        QNDEBUG(QStringLiteral("Note editor background color did not change"));
        return;
    }

    saveNoteEditorBackgroundColor(color);
    Q_EMIT noteEditorBackgroundColorChanged(color);
}

void PreferencesDialog::onNoteEditorHighlightColorCodeEntered()
{
    QString colorCode = m_pUi->noteEditorHighlightColorLineEdit->text();

    QNDEBUG(QStringLiteral("PreferencesDialog::")
            << QStringLiteral("onNoteEditorHighlightColorCodeEntered: ")
            << colorCode);

    QColor prevColor = noteEditorHighlightColor();

    QColor color(colorCode);
    bool res = onNoteEditorColorEnteredImpl(
        color, prevColor,
        NOTE_EDITOR_HIGHLIGHT_COLOR_SETTINGS_KEY,
        *m_pUi->noteEditorHighlightColorLineEdit,
        *m_pUi->noteEditorHighlightColorDemoFrame);
    if (!res) {
        return;
    }

    Q_EMIT noteEditorHighlightColorChanged(color);
}

void PreferencesDialog::onNoteEditorHighlightColorDialogRequested()
{
    QNDEBUG(QStringLiteral("PreferencesDialog::"
                           "onNoteEditorHighlightColorDialogRequested"));

    if (!m_pNoteEditorHighlightColorDialog.isNull()) {
        QNDEBUG(QStringLiteral("Dialog is already opened"));
        return;
    }

    QScopedPointer<QColorDialog> pColorDialog(new QColorDialog(this));
    pColorDialog->setWindowModality(Qt::WindowModal);
    pColorDialog->setCurrentColor(noteEditorHighlightColor());

    m_pNoteEditorHighlightColorDialog = pColorDialog.data();

    QObject::connect(pColorDialog.data(),
                     QNSIGNAL(QColorDialog,colorSelected,QColor),
                     this,
                     QNSLOT(PreferencesDialog,onNoteEditorHighlightColorSelected,
                            QColor));
    Q_UNUSED(pColorDialog->exec())
}

void PreferencesDialog::onNoteEditorHighlightColorSelected(const QColor & color)
{
    QNDEBUG(QStringLiteral("PreferencesDialog::onNoteEditorHighlightColorSelected: ")
            << color.name());

    m_pUi->noteEditorHighlightColorLineEdit->setText(color.name());
    setNoteEditorHighlightColorToDemoFrame(color);

    QColor previousHighlightColor = noteEditorHighlightColor();
    if (previousHighlightColor.isValid() && color.isValid() &&
        (previousHighlightColor.name() == color.name()))
    {
        QNDEBUG(QStringLiteral("Note editor highlight color did not change"));
        return;
    }

    saveNoteEditorHighlightColor(color);
    Q_EMIT noteEditorHighlightColorChanged(color);
}

void PreferencesDialog::onNoteEditorHighlightedTextColorCodeEntered()
{
    QString colorCode = m_pUi->noteEditorHighlightedTextColorLineEdit->text();

    QNDEBUG(QStringLiteral("PreferencesDialog::")
            << QStringLiteral("onNoteEditorHighlightedTextColorCodeEntered: ")
            << colorCode);

    QColor prevColor = noteEditorHighlightedTextColor();

    QColor color(colorCode);
    bool res = onNoteEditorColorEnteredImpl(
        color, prevColor,
        NOTE_EDITOR_HIGHLIGHTED_TEXT_SETTINGS_KEY,
        *m_pUi->noteEditorHighlightedTextColorLineEdit,
        *m_pUi->noteEditorHighlightedTextColorDemoFrame);
    if (!res) {
        return;
    }

    Q_EMIT noteEditorHighlightedTextColorChanged(color);
}

void PreferencesDialog::onNoteEditorHighlightedTextColorDialogRequested()
{
    QNDEBUG(QStringLiteral("PreferencesDialog::"
                           "onNoteEditorHighlightedTextColorDialogRequested"));

    if (!m_pNoteEditorHighlightedTextColorDialog.isNull()) {
        QNDEBUG(QStringLiteral("Dialog is already opened"));
        return;
    }

    QScopedPointer<QColorDialog> pColorDialog(new QColorDialog(this));
    pColorDialog->setWindowModality(Qt::WindowModal);
    pColorDialog->setCurrentColor(noteEditorHighlightedTextColor());

    m_pNoteEditorHighlightedTextColorDialog = pColorDialog.data();

    QObject::connect(pColorDialog.data(),
                     QNSIGNAL(QColorDialog,colorSelected,QColor),
                     this,
                     QNSLOT(PreferencesDialog,
                            onNoteEditorHighlightedTextColorSelected,QColor));
    Q_UNUSED(pColorDialog->exec())
}

void PreferencesDialog::onNoteEditorHighlightedTextColorSelected(
    const QColor & color)
{
    QNDEBUG(QStringLiteral("PreferencesDialog::")
            << QStringLiteral("onNoteEditorHighlightedTextColorSelected: ")
            << color.name());

    m_pUi->noteEditorHighlightedTextColorLineEdit->setText(color.name());
    setNoteEditorHighlightedTextColorToDemoFrame(color);

    QColor previousHighlightedTextColor = noteEditorHighlightedTextColor();
    if (previousHighlightedTextColor.isValid() && color.isValid() &&
        (previousHighlightedTextColor.name() == color.name()))
    {
        QNDEBUG(QStringLiteral("Note editor highlighted text color did not change"));
        return;
    }

    saveNoteEditorHighlightedTextColor(color);
    Q_EMIT noteEditorHighlightedTextColorChanged(color);
}

void PreferencesDialog::onNoteEditorColorsReset()
{
    QNDEBUG(QStringLiteral("PreferencesDialog::onNoteEditorColorsReset"));

    Account currentAccount = m_accountManager.currentAccount();
    ApplicationSettings appSettings(currentAccount, QUENTIER_UI_SETTINGS);

    appSettings.beginGroup(NOTE_EDITOR_SETTINGS_GROUP_NAME);
    appSettings.remove(NOTE_EDITOR_FONT_COLOR_SETTINGS_KEY);
    appSettings.remove(NOTE_EDITOR_BACKGROUND_COLOR_SETTINGS_KEY);
    appSettings.remove(NOTE_EDITOR_HIGHLIGHT_COLOR_SETTINGS_KEY);
    appSettings.remove(NOTE_EDITOR_HIGHLIGHTED_TEXT_SETTINGS_KEY);
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
    m_pUi->noteEditorHighlightedTextColorLineEdit->setText(highlightedTextColor.name());

    Q_EMIT noteEditorColorsReset();
}

void PreferencesDialog::onDownloadNoteThumbnailsCheckboxToggled(bool checked)
{
    QNDEBUG(QStringLiteral("PreferencesDialog::")
            << QStringLiteral("onDownloadNoteThumbnailsCheckboxToggled: ")
            << (checked ? QStringLiteral("checked") : QStringLiteral("unchecked")));

    Account currentAccount = m_accountManager.currentAccount();
    ApplicationSettings appSettings(currentAccount, QUENTIER_SYNC_SETTINGS);
    appSettings.beginGroup(SYNCHRONIZATION_SETTINGS_GROUP_NAME);
    appSettings.setValue(SYNCHRONIZATION_DOWNLOAD_NOTE_THUMBNAILS, checked);
    appSettings.endGroup();

    Q_EMIT synchronizationDownloadNoteThumbnailsOptionChanged(checked);
}

void PreferencesDialog::onDownloadInkNoteImagesCheckboxToggled(bool checked)
{
    QNDEBUG(QStringLiteral("PreferencesDialog::")
            << QStringLiteral("onDownloadInkNoteImagesCheckboxToggled: ")
            << (checked ? QStringLiteral("checked") : QStringLiteral("unchecked")));

    Account currentAccount = m_accountManager.currentAccount();
    ApplicationSettings appSettings(currentAccount, QUENTIER_SYNC_SETTINGS);
    appSettings.beginGroup(SYNCHRONIZATION_SETTINGS_GROUP_NAME);
    appSettings.setValue(SYNCHRONIZATION_DOWNLOAD_INK_NOTE_IMAGES, checked);
    appSettings.endGroup();

    Q_EMIT synchronizationDownloadInkNoteImagesOptionChanged(checked);
}

void PreferencesDialog::onRunSyncPeriodicallyOptionChanged(int index)
{
    QNDEBUG(QStringLiteral("PreferencesDialog::onRunSyncPeriodicallyOptionChanged: ")
            << QStringLiteral("index = ") << index);

    int runSyncEachNumMinutes = 0;
    switch(index)
    {
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

    Account currentAccount = m_accountManager.currentAccount();
    ApplicationSettings syncSettings(currentAccount, QUENTIER_SYNC_SETTINGS);
    syncSettings.beginGroup(SYNCHRONIZATION_SETTINGS_GROUP_NAME);
    syncSettings.setValue(SYNCHRONIZATION_RUN_SYNC_EACH_NUM_MINUTES,
                          runSyncEachNumMinutes);
    syncSettings.endGroup();

    emit runSyncPeriodicallyOptionChanged(runSyncEachNumMinutes);
}

void PreferencesDialog::onNetworkProxyTypeChanged(int type)
{
    QNDEBUG(QStringLiteral("PreferencesDialog::onNetworkProxyTypeChanged: type = ")
            << type);
    checkAndSetNetworkProxy();
}

void PreferencesDialog::onNetworkProxyHostChanged()
{
    QNDEBUG(QStringLiteral("PreferencesDialog::onNetworkProxyHostChanged: ")
            << m_pUi->networkProxyHostLineEdit->text());
    checkAndSetNetworkProxy();
}

void PreferencesDialog::onNetworkProxyPortChanged(int port)
{
    QNDEBUG(QStringLiteral("PreferencesDialog::onNetworkProxyPortChanged: ")
            << port);
    checkAndSetNetworkProxy();
}

void PreferencesDialog::onNetworkProxyUsernameChanged()
{
    QNDEBUG(QStringLiteral("PreferencesDialog::onNetworkProxyUsernameChanged: ")
            << m_pUi->networkProxyUserLineEdit->text());
    checkAndSetNetworkProxy();
}

void PreferencesDialog::onNetworkProxyPasswordChanged()
{
    QNDEBUG(QStringLiteral("PreferencesDialog::onNetworkProxyPasswordChanged: ")
            << m_pUi->networkProxyPasswordLineEdit->text());
    checkAndSetNetworkProxy();
}

void PreferencesDialog::onNetworkProxyPasswordVisibilityToggled(bool checked)
{
    QNDEBUG(QStringLiteral("PreferencesDialog::")
            << QStringLiteral("onNetworkProxyPasswordVisibilityToggled: checked = ")
            << (checked ? QStringLiteral("true") : QStringLiteral("false")));
    m_pUi->networkProxyPasswordLineEdit->setEchoMode(checked
                                                     ? QLineEdit::Normal
                                                     : QLineEdit::Password);
}

bool PreferencesDialog::eventFilter(QObject * pObject, QEvent * pEvent)
{
    if (pObject == m_pUi->noteEditorFontColorDemoFrame)
    {
        if (pEvent && (pEvent->type() == QEvent::MouseButtonDblClick)) {
            onNoteEditorFontColorDialogRequested();
            return true;
        }
    }
    else if (pObject == m_pUi->noteEditorBackgroundColorDemoFrame)
    {
        if (pEvent && (pEvent->type() == QEvent::MouseButtonDblClick)) {
            onNoteEditorBackgroundColorDialogRequested();
            return true;
        }
    }
    else if (pObject == m_pUi->noteEditorHighlightColorDemoFrame)
    {
        if (pEvent && (pEvent->type() == QEvent::MouseButtonDblClick)) {
            onNoteEditorHighlightColorDialogRequested();
            return true;
        }
    }
    else if (pObject == m_pUi->noteEditorHighlightedTextColorDemoFrame)
    {
        if (pEvent && (pEvent->type() == QEvent::MouseButtonDblClick)) {
            onNoteEditorHighlightedTextColorDialogRequested();
            return true;
        }
    }

    return QDialog::eventFilter(pObject, pEvent);
}

void PreferencesDialog::onEnableLogViewerInternalLogsCheckboxToggled(bool checked)
{
    QNDEBUG(QStringLiteral("PreferencesDialog::")
            << QStringLiteral("onEnableLogViewerInternalLogsCheckboxToggled: ")
            << QStringLiteral("checked = ")
            << (checked ? QStringLiteral("true") : QStringLiteral("false")));

    ApplicationSettings globalAppSettings;
    globalAppSettings.beginGroup(LOGGING_SETTINGS_GROUP);
    globalAppSettings.setValue(ENABLE_LOG_VIEWER_INTERNAL_LOGS, checked);
    globalAppSettings.endGroup();
}

void PreferencesDialog::setupCurrentSettingsState(ActionsInfo & actionsInfo,
                                                  ShortcutManager & shortcutManager)
{
    QNDEBUG(QStringLiteral("PreferencesDialog::setupCurrentSettingsState"));

    // 1) System tray tab
    setupSystemTraySettings();

    // 2) Note editor tab
    setupNoteEditorSettingsState();

    // 3) Appearance tab
    Account currentAccount = m_accountManager.currentAccount();
    ApplicationSettings appSettings(currentAccount, QUENTIER_UI_SETTINGS);

    appSettings.beginGroup(LOOK_AND_FEEL_SETTINGS_GROUP_NAME);
    QVariant showThumbnails =
        appSettings.value(SHOW_NOTE_THUMBNAILS_SETTINGS_KEY,
                          QVariant::fromValue(DEFAULT_SHOW_NOTE_THUMBNAILS));
    appSettings.endGroup();

    m_pUi->showNoteThumbnailsCheckBox->setChecked(showThumbnails.toBool());

    setupMainWindowBorderSettings();

    // 4) Behaviour tab
    setupStartAtLoginSettings();

    // 5) Synchronization tab
    if (currentAccount.type() == Account::Type::Local)
    {
        // Remove the synchronization tab entirely
        m_pUi->preferencesTabWidget->removeTab(3);
    }
    else
    {
        ApplicationSettings syncSettings(currentAccount, QUENTIER_SYNC_SETTINGS);

        syncSettings.beginGroup(SYNCHRONIZATION_SETTINGS_GROUP_NAME);

        bool downloadNoteThumbnails = DEFAULT_DOWNLOAD_NOTE_THUMBNAILS;
        if (syncSettings.contains(SYNCHRONIZATION_DOWNLOAD_NOTE_THUMBNAILS)) {
            downloadNoteThumbnails =
                syncSettings.value(SYNCHRONIZATION_DOWNLOAD_NOTE_THUMBNAILS).toBool();
        }

        bool downloadInkNoteImages = DEFAULT_DOWNLOAD_INK_NOTE_IMAGES;
        if (syncSettings.contains(SYNCHRONIZATION_DOWNLOAD_INK_NOTE_IMAGES)) {
            downloadInkNoteImages =
                syncSettings.value(SYNCHRONIZATION_DOWNLOAD_INK_NOTE_IMAGES).toBool();
        }

        int runSyncEachNumMinutes = -1;
        if (syncSettings.contains(SYNCHRONIZATION_RUN_SYNC_EACH_NUM_MINUTES))
        {
            QVariant data =
                syncSettings.value(SYNCHRONIZATION_RUN_SYNC_EACH_NUM_MINUTES);
            bool conversionResult = false;
            runSyncEachNumMinutes = data.toInt(&conversionResult);
            if (Q_UNLIKELY(!conversionResult)) {
                QNDEBUG(QStringLiteral("Failed to convert the number of minutes ")
                        << QStringLiteral("to run sync over to int: ") << data);
                runSyncEachNumMinutes = -1;
            }
        }

        if (runSyncEachNumMinutes < 0) {
            runSyncEachNumMinutes = DEFAULT_RUN_SYNC_EACH_NUM_MINUTES;
        }

        syncSettings.endGroup();

        m_pUi->downloadNoteThumbnailsCheckBox->setChecked(downloadNoteThumbnails);
        m_pUi->downloadInkNoteImagesCheckBox->setChecked(downloadInkNoteImages);

        setupNetworkProxySettingsState();
        setupRunSyncEachNumMinutesComboBox(runSyncEachNumMinutes);

        m_pUi->synchronizationTabStatusLabel->hide();
    }

    // 6) Shortcuts tab
    m_pUi->shortcutSettingsWidget->initialize(currentAccount, actionsInfo,
                                              &shortcutManager);

    // 7) Auxiliary tab
    ApplicationSettings globalAppSettings;
    globalAppSettings.beginGroup(LOGGING_SETTINGS_GROUP);
    QVariant enableLogViewerInternalLogsValue =
        globalAppSettings.value(ENABLE_LOG_VIEWER_INTERNAL_LOGS);
    globalAppSettings.endGroup();

    bool enableLogViewerInternalLogs = false;
    if (enableLogViewerInternalLogsValue.isValid()) {
        enableLogViewerInternalLogs = enableLogViewerInternalLogsValue.toBool();
    }

    m_pUi->enableInternalLogViewerLogsCheckBox->setChecked(
        enableLogViewerInternalLogs);
}

void PreferencesDialog::setupMainWindowBorderSettings()
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 2, 0)
    m_pUi->leftMainWindowBorderColorLineEdit->setClearButtonEnabled(true);
    m_pUi->rightMainWindowBorderColorLineEdit->setClearButtonEnabled(true);
#endif

    m_pUi->leftMainWindowBorderColorLineEdit->setPlaceholderText(tr("hex color code"));
    m_pUi->rightMainWindowBorderColorLineEdit->setPlaceholderText(tr("hex color code"));

    QStringList availableShowBorderOptions;
    availableShowBorderOptions.reserve(3);
    availableShowBorderOptions << tr("Always show");
    availableShowBorderOptions << tr("Never show");
    availableShowBorderOptions << tr("Show only when maximized");
    m_pAvailableMainWindowBorderOptionsModel->setStringList(availableShowBorderOptions);

    Account currentAccount = m_accountManager.currentAccount();
    ApplicationSettings appSettings(currentAccount, QUENTIER_UI_SETTINGS);
    appSettings.beginGroup(LOOK_AND_FEEL_SETTINGS_GROUP_NAME);

    bool conversionResult = false;
    int showLeftMainWindowBorderOption =
        appSettings.value(SHOW_LEFT_MAIN_WINDOW_BORDER_OPTION_KEY)
        .toInt(&conversionResult);
    if (!conversionResult || (showLeftMainWindowBorderOption < 0) ||
        (showLeftMainWindowBorderOption > 2))
    {
        QNDEBUG(QStringLiteral("No valid \"Show left main window border\" option "
                               "was found within the persistent settings, using "
                               "the default option"));
        showLeftMainWindowBorderOption = DEFAULT_SHOW_MAIN_WINDOW_BORDER_OPTION;
    }

    m_pUi->leftMainWindowBorderOptionComboBox->setModel(
        m_pAvailableMainWindowBorderOptionsModel);

    switch(showLeftMainWindowBorderOption)
    {
    case MainWindowSideBorderOption::AlwaysShow:
        m_pUi->leftMainWindowBorderOptionComboBox->setCurrentIndex(0);
        break;
    case MainWindowSideBorderOption::NeverShow:
        m_pUi->leftMainWindowBorderOptionComboBox->setCurrentIndex(1);
        break;
    case MainWindowSideBorderOption::ShowOnlyWhenMaximized:
    default:
        m_pUi->leftMainWindowBorderOptionComboBox->setCurrentIndex(2);
        break;
    }

    QObject::connect(m_pUi->leftMainWindowBorderOptionComboBox,
                     SIGNAL(currentIndexChanged(int)),
                     this,
                     SLOT(onShowMainWindowLeftBorderOptionChanged(int)));

    conversionResult = false;
    int showRightMainWindowBorderOption =
        appSettings.value(SHOW_RIGHT_MAIN_WINDOW_BORDER_OPTION_KEY)
        .toInt(&conversionResult);
    if (!conversionResult || (showRightMainWindowBorderOption < 0) ||
        (showRightMainWindowBorderOption > 2))
    {
        QNDEBUG(QStringLiteral("No valid \"Show right main window border\" option "
                               "was found within the persistent settings, using "
                               "the default option"));
        showRightMainWindowBorderOption = DEFAULT_SHOW_MAIN_WINDOW_BORDER_OPTION;
    }

    m_pUi->rightMainWindowBorderOptionComboBox->setModel(
        m_pAvailableMainWindowBorderOptionsModel);

    switch(showRightMainWindowBorderOption)
    {
    case MainWindowSideBorderOption::AlwaysShow:
        m_pUi->rightMainWindowBorderOptionComboBox->setCurrentIndex(0);
        break;
    case MainWindowSideBorderOption::NeverShow:
        m_pUi->rightMainWindowBorderOptionComboBox->setCurrentIndex(1);
        break;
    case MainWindowSideBorderOption::ShowOnlyWhenMaximized:
    default:
        m_pUi->rightMainWindowBorderOptionComboBox->setCurrentIndex(2);
        break;
    }

    QObject::connect(m_pUi->rightMainWindowBorderOptionComboBox,
                     SIGNAL(currentIndexChanged(int)),
                     this,
                     SLOT(onShowMainWindowRightBorderOptionChanged(int)));

    conversionResult = false;
    int leftMainWindowBorderWidth =
        appSettings.value(LEFT_MAIN_WINDOW_BORDER_WIDTH_KEY)
        .toInt(&conversionResult);
    if (!conversionResult)
    {
        QNDEBUG(QStringLiteral("No valid left main window border's width setting "
                               "was found withiin the persistent settings, using "
                               "the default value"));
        leftMainWindowBorderWidth = DEFAULT_MAIN_WINDOW_BORDER_SIZE;
    }
    else if (leftMainWindowBorderWidth < 0)
    {
        QNDEBUG(QStringLiteral("Found invalid negative left main window border's "
                               "width within the persistent settings, using "
                               "the default value"));
        leftMainWindowBorderWidth = DEFAULT_MAIN_WINDOW_BORDER_SIZE;
    }
    else if (leftMainWindowBorderWidth > MAX_MAIN_WINDOW_BORDER_SIZE)
    {
        QNDEBUG(QStringLiteral("Found invalid too large left main window border's "
                               "width within the persistent settings, using "
                               "the max allowed value instead"));
        leftMainWindowBorderWidth = MAX_MAIN_WINDOW_BORDER_SIZE;
    }

    m_pUi->leftMainWindowBorderWidthSpinBox->setValue(leftMainWindowBorderWidth);
    m_pUi->leftMainWindowBorderWidthSpinBox->setMinimum(0);
    m_pUi->leftMainWindowBorderWidthSpinBox->setMaximum(MAX_MAIN_WINDOW_BORDER_SIZE);

    QObject::connect(m_pUi->leftMainWindowBorderWidthSpinBox,
                     SIGNAL(valueChanged(int)),
                     this,
                     SLOT(onLeftMainWindowBorderWidthChanged(int)));

    conversionResult = false;
    int rightMainWindowBorderWidth =
        appSettings.value(RIGHT_MAIN_WINDOW_BORDER_WIDTH_KEY)
        .toInt(&conversionResult);
    if (!conversionResult)
    {
        QNDEBUG(QStringLiteral("No valid right main window border's width setting "
                               "was found within the persistent settings, using "
                               "the default value"));
        rightMainWindowBorderWidth = DEFAULT_MAIN_WINDOW_BORDER_SIZE;
    }
    else if (rightMainWindowBorderWidth < 0)
    {
        QNDEBUG(QStringLiteral("Found invalid negative right main window border's "
                               "width within the persistent settings, using "
                               "the default value"));
        rightMainWindowBorderWidth = DEFAULT_MAIN_WINDOW_BORDER_SIZE;
    }
    else if (rightMainWindowBorderWidth > MAX_MAIN_WINDOW_BORDER_SIZE)
    {
        QNDEBUG(QStringLiteral("Found invalid too large right main window border's "
                               "width within the persistent settings, using "
                               "the max allowed value instead"));
        rightMainWindowBorderWidth = MAX_MAIN_WINDOW_BORDER_SIZE;
    }

    m_pUi->rightMainWindowBorderWidthSpinBox->setValue(rightMainWindowBorderWidth);
    m_pUi->rightMainWindowBorderWidthSpinBox->setMinimum(0);
    m_pUi->rightMainWindowBorderWidthSpinBox->setMaximum(MAX_MAIN_WINDOW_BORDER_SIZE);

    QObject::connect(m_pUi->rightMainWindowBorderWidthSpinBox,
                     SIGNAL(valueChanged(int)),
                     this,
                     SLOT(onRightMainWindowBorderWidthChanged(int)));

    QString leftMainWindowBorderOverrideColorCode =
        appSettings.value(LEFT_MAIN_WINDOW_BORDER_OVERRIDE_COLOR).toString();
    if (!leftMainWindowBorderOverrideColorCode.isEmpty() &&
        QColor::isValidColor(leftMainWindowBorderOverrideColorCode))
    {
        m_pUi->leftMainWindowBorderColorLineEdit->setText(
            leftMainWindowBorderOverrideColorCode);
    }

    QString rightMainWindowBorderOverrideColorCode =
        appSettings.value(RIGHT_MAIN_WINDOW_BORDER_OVERRIDE_COLOR).toString();
    if (!rightMainWindowBorderOverrideColorCode.isEmpty() &&
        QColor::isValidColor(rightMainWindowBorderOverrideColorCode))
    {
        m_pUi->rightMainWindowBorderColorLineEdit->setText(
            rightMainWindowBorderOverrideColorCode);
    }

    ColorCodeValidator * pColorCodeValidator = new ColorCodeValidator(this);
    m_pUi->leftMainWindowBorderColorLineEdit->setValidator(pColorCodeValidator);
    m_pUi->rightMainWindowBorderColorLineEdit->setValidator(pColorCodeValidator);

    QObject::connect(m_pUi->leftMainWindowBorderColorLineEdit,
                     QNSIGNAL(QLineEdit,textChanged,QString),
                     this,
                     QNSLOT(PreferencesDialog,
                            onLeftMainWindowBorderColorCodeChanged,QString));
    QObject::connect(m_pUi->rightMainWindowBorderColorLineEdit,
                     QNSIGNAL(QLineEdit,textChanged,QString),
                     this,
                     QNSLOT(PreferencesDialog,
                            onRightMainWindowBorderColorCodeChanged,QString));

    QObject::connect(m_pUi->leftMainWindowBorderColorPushButton,
                     QNSIGNAL(QPushButton,clicked),
                     this,
                     QNSLOT(PreferencesDialog,
                            onLeftMainWindowBorderColorPickerRequested));
    QObject::connect(m_pUi->rightMainWindowBorderColorPushButton,
                     QNSIGNAL(QPushButton,clicked),
                     this,
                     QNSLOT(PreferencesDialog,
                            onRightMainWindowBorderColorPickerRequested));

    appSettings.endGroup();
}

void PreferencesDialog::setupSystemTraySettings()
{
#ifdef Q_WS_MAC
    // It makes little sense to minimize to tray on Mac
    // because the minimized app goes to dock
    m_pUi->minimizeToTrayCheckBox->setDisabled(true);
    m_pUi->minimizeToTrayCheckBox->setHidden(true);
#endif

    if (!m_systemTrayIconManager.isSystemTrayAvailable())
    {
        QNDEBUG(QStringLiteral("The system tray is not available, setting up "
                               "the blank system tray settings"));

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

    Account currentAccount = m_accountManager.currentAccount();
    ApplicationSettings appSettings(currentAccount, QUENTIER_UI_SETTINGS);
    appSettings.beginGroup(SYSTEM_TRAY_SETTINGS_GROUP_NAME);

    bool shouldShowSystemTrayIcon = DEFAULT_SHOW_SYSTEM_TRAY_ICON;
    QVariant shouldShowSystemTrayIconData =
        appSettings.value(SHOW_SYSTEM_TRAY_ICON_SETTINGS_KEY);
    if (shouldShowSystemTrayIconData.isValid()) {
        shouldShowSystemTrayIcon = shouldShowSystemTrayIconData.toBool();
    }

    bool shouldCloseToTray = DEFAULT_CLOSE_TO_SYSTEM_TRAY;
    QVariant shouldCloseToTrayData =
        appSettings.value(CLOSE_TO_SYSTEM_TRAY_SETTINGS_KEY);
    if (shouldCloseToTrayData.isValid()) {
        shouldCloseToTray = shouldCloseToTrayData.toBool();
    }

    bool shouldMinimizeToTray = DEFAULT_MINIMIZE_TO_SYSTEM_TRAY;
    QVariant shouldMinimizeToTrayData =
        appSettings.value(MINIMIZE_TO_SYSTEM_TRAY_SETTINGS_KEY);
    if (shouldMinimizeToTrayData.isValid()) {
        shouldMinimizeToTray = shouldMinimizeToTrayData.toBool();
    }

    bool shouldStartMinimizedToTray = DEFAULT_START_MINIMIZED_TO_SYSTEM_TRAY;
    QVariant shouldStartMinimizedToTrayData =
        appSettings.value(START_MINIMIZED_TO_SYSTEM_TRAY_SETTINGS_KEY);
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

    SystemTrayIconManager::TrayAction singleClickTrayAction =
        m_systemTrayIconManager.singleClickTrayAction();
    m_pUi->traySingleClickActionComboBox->setCurrentIndex(singleClickTrayAction);

    SystemTrayIconManager::TrayAction middleClickTrayAction =
        m_systemTrayIconManager.middleClickTrayAction();
    m_pUi->trayMiddleClickActionComboBox->setCurrentIndex(middleClickTrayAction);

    SystemTrayIconManager::TrayAction doubleClickTrayAction =
        m_systemTrayIconManager.doubleClickTrayAction();
    m_pUi->trayDoubleClickActionComboBox->setCurrentIndex(doubleClickTrayAction);

    // If the tray icon is now shown, disable all the tray actions but the one
    // to show/hide the system tray icon
    if (!m_systemTrayIconManager.isShown())
    {
        m_pUi->closeToTrayCheckBox->setDisabled(true);
        m_pUi->minimizeToTrayCheckBox->setDisabled(true);
        m_pUi->startFromTrayCheckBox->setDisabled(true);

        m_pUi->traySingleClickActionComboBox->setDisabled(true);
        m_pUi->trayMiddleClickActionComboBox->setDisabled(true);
        m_pUi->trayDoubleClickActionComboBox->setDisabled(true);
    }
}

void PreferencesDialog::setupStartAtLoginSettings()
{
    QNDEBUG(QStringLiteral("PreferencesDialog::setupStartAtLoginSettings"));

    std::pair<bool, StartQuentierAtLoginOption::type> startAtLoginOptions =
        isQuentierSetToStartAtLogin();

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

void PreferencesDialog::setupRunSyncEachNumMinutesComboBox(int currentNumMinutes)
{
    QNDEBUG(QStringLiteral("PreferencesDialog::setupRunSyncEachNumMinutesComboBox: ")
            << currentNumMinutes);

    QStringList runSyncPeriodicallyOptions;
    runSyncPeriodicallyOptions.reserve(5);
    runSyncPeriodicallyOptions << tr("Manually");
    runSyncPeriodicallyOptions << tr("Every 5 minutes");
    runSyncPeriodicallyOptions << tr("Every 15 minutes");
    runSyncPeriodicallyOptions << tr("Every 30 minutes");
    runSyncPeriodicallyOptions << tr("Every hour");

    QStringListModel * pRunSyncPeriodicallyComboBoxModel = new QStringListModel(this);
    pRunSyncPeriodicallyComboBoxModel->setStringList(runSyncPeriodicallyOptions);
    m_pUi->runSyncPeriodicallyComboBox->setModel(pRunSyncPeriodicallyComboBoxModel);

    int currentIndex = 2;
    switch(currentNumMinutes)
    {
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
            QNDEBUG(QStringLiteral("Unrecognized option, using the default one"));
            break;
        }
    }

    m_pUi->runSyncPeriodicallyComboBox->setCurrentIndex(currentIndex);
    QObject::connect(m_pUi->runSyncPeriodicallyComboBox,
                     SIGNAL(currentIndexChanged(int)),
                     this,
                     SLOT(onRunSyncPeriodicallyOptionChanged(int)));
}

void PreferencesDialog::setupNetworkProxySettingsState()
{
    QNDEBUG(QStringLiteral("PreferencesDialog::setupNetworkProxySettingsState"));

    QNetworkProxy::ProxyType proxyType = QNetworkProxy::DefaultProxy;
    QString proxyHost;
    int proxyPort = 0;
    QString proxyUsername;
    QString proxyPassword;

    parseNetworkProxySettings(m_accountManager.currentAccount(),
                              proxyType, proxyHost, proxyPort,
                              proxyUsername, proxyPassword);

    QStringList networkProxyTypes;
    networkProxyTypes.reserve(3);

    // Allow only "No proxy", "Http proxy" and "Socks5 proxy"
    // If proxy type parsed from the settings is different, fall back to "No proxy"
    // Also treat QNetworkProxy::DefaultProxy as "No proxy" because the default
    // proxy type of QNetworkProxy is QNetworkProxy::NoProxy

    QString noProxyItem = tr("No proxy");
    QString httpProxyItem = tr("Http proxy");
    QString socks5ProxyItem = tr("Socks5 proxy");

    networkProxyTypes << noProxyItem;
    networkProxyTypes << httpProxyItem;
    networkProxyTypes << socks5ProxyItem;

    m_pUi->networkProxyTypeComboBox->setModel(
        new QStringListModel(networkProxyTypes, this));

    if ( (proxyType == QNetworkProxy::NoProxy) ||
         (proxyType == QNetworkProxy::DefaultProxy) )
    {
        m_pUi->networkProxyTypeComboBox->setCurrentIndex(0);
    }
    else if (proxyType == QNetworkProxy::HttpProxy)
    {
        m_pUi->networkProxyTypeComboBox->setCurrentIndex(1);
    }
    else if (proxyType == QNetworkProxy::Socks5Proxy)
    {
        m_pUi->networkProxyTypeComboBox->setCurrentIndex(2);
    }
    else
    {
        QNINFO(QStringLiteral("Detected unsupported proxy type: ") << proxyType
               << QStringLiteral(", falling back to no proxy"));
        proxyType = QNetworkProxy::DefaultProxy;
        m_pUi->networkProxyTypeComboBox->setCurrentIndex(0);
    }

    m_pUi->networkProxyHostLineEdit->setText(proxyHost);
    m_pUi->networkProxyPortSpinBox->setMinimum(0);
    m_pUi->networkProxyPortSpinBox->setMaximum(
        std::max(std::numeric_limits<quint16>::max()-1, 0));
    m_pUi->networkProxyPortSpinBox->setValue(std::max(0, proxyPort));
    m_pUi->networkProxyUserLineEdit->setText(proxyUsername);
    m_pUi->networkProxyPasswordLineEdit->setText(proxyPassword);
}

void PreferencesDialog::setupNoteEditorSettingsState()
{
    QNDEBUG(QStringLiteral("PreferencesDialog::setupNoteEditorSettingsState"));

    Account currentAccount = m_accountManager.currentAccount();
    ApplicationSettings appSettings(currentAccount, QUENTIER_UI_SETTINGS);

    appSettings.beginGroup(NOTE_EDITOR_SETTINGS_GROUP_NAME);
    bool useLimitedFonts = appSettings.value(USE_LIMITED_SET_OF_FONTS).toBool();
    QString fontColorCode = appSettings.value(NOTE_EDITOR_FONT_COLOR_SETTINGS_KEY).toString();
    QString backgroundColorCode = appSettings.value(NOTE_EDITOR_BACKGROUND_COLOR_SETTINGS_KEY).toString();
    QString highlightColorCode = appSettings.value(NOTE_EDITOR_HIGHLIGHT_COLOR_SETTINGS_KEY).toString();
    QString highlightedTextColorCode = appSettings.value(NOTE_EDITOR_HIGHLIGHTED_TEXT_SETTINGS_KEY).toString();
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
        m_pUi->noteEditorHighlightedTextColorLineEdit->setText(highlightedTextColorCode);
    }
    else {
        QColor color = pal.color(QPalette::HighlightedText);
        setNoteEditorHighlightedTextColorToDemoFrame(color);
        m_pUi->noteEditorHighlightedTextColorLineEdit->setText(color.name());
    }
}

void PreferencesDialog::createConnections()
{
    QNDEBUG(QStringLiteral("PreferencesDialog::createConnections"));

    QObject::connect(m_pUi->okPushButton, QNSIGNAL(QPushButton,clicked),
                     this, QNSLOT(PreferencesDialog,accept));

    QObject::connect(m_pUi->showSystemTrayIconCheckBox,
                     QNSIGNAL(QCheckBox,toggled,bool),
                     this,
                     QNSLOT(PreferencesDialog,
                            onShowSystemTrayIconCheckboxToggled,bool));
    QObject::connect(m_pUi->closeToTrayCheckBox,
                     QNSIGNAL(QCheckBox,toggled,bool),
                     this,
                     QNSLOT(PreferencesDialog,
                            onCloseToSystemTrayCheckboxToggled,bool));
    QObject::connect(m_pUi->minimizeToTrayCheckBox,
                     QNSIGNAL(QCheckBox,toggled,bool),
                     this,
                     QNSLOT(PreferencesDialog,
                            onMinimizeToSystemTrayCheckboxToggled,bool));
    QObject::connect(m_pUi->startFromTrayCheckBox,
                     QNSIGNAL(QCheckBox,toggled,bool),
                     this,
                     QNSLOT(PreferencesDialog,
                            onStartMinimizedToSystemTrayCheckboxToggled,bool));

    QObject::connect(m_pUi->startAtLoginCheckBox,
                     QNSIGNAL(QCheckBox,toggled,bool),
                     this,
                     QNSLOT(PreferencesDialog,
                            onStartAtLoginCheckboxToggled,bool));
    QObject::connect(m_pUi->startAtLoginOptionComboBox,
                     SIGNAL(currentIndexChanged(int)),
                     this,
                     SLOT(onStartAtLoginOptionChanged(int)));

    QObject::connect(m_pUi->traySingleClickActionComboBox,
                     SIGNAL(currentIndexChanged(int)),
                     this,
                     SLOT(onSingleClickTrayActionChanged(int)));
    QObject::connect(m_pUi->trayMiddleClickActionComboBox,
                     SIGNAL(currentIndexChanged(int)),
                     this,
                     SLOT(onMiddleClickTrayActionChanged(int)));
    QObject::connect(m_pUi->trayDoubleClickActionComboBox,
                     SIGNAL(currentIndexChanged(int)),
                     this,
                     SLOT(onDoubleClickTrayActionChanged(int)));

    QObject::connect(m_pUi->showNoteThumbnailsCheckBox,
                     QNSIGNAL(QCheckBox,toggled,bool),
                     this,
                     QNSLOT(PreferencesDialog,
                            onShowNoteThumbnailsCheckboxToggled,bool));

    QObject::connect(m_pUi->limitedFontsCheckBox,
                     QNSIGNAL(QCheckBox,toggled,bool),
                     this,
                     QNSLOT(PreferencesDialog,
                            onNoteEditorUseLimitedFontsCheckboxToggled,bool));
    QObject::connect(m_pUi->noteEditorFontColorLineEdit,
                     QNSIGNAL(QLineEdit,editingFinished),
                     this,
                     QNSLOT(PreferencesDialog,onNoteEditorFontColorCodeEntered));
    QObject::connect(m_pUi->noteEditorFontColorPushButton,
                     QNSIGNAL(QPushButton,clicked),
                     this,
                     QNSLOT(PreferencesDialog,onNoteEditorFontColorDialogRequested));
    QObject::connect(m_pUi->noteEditorBackgroundColorLineEdit,
                     QNSIGNAL(QLineEdit,editingFinished),
                     this,
                     QNSLOT(PreferencesDialog,onNoteEditorBackgroundColorCodeEntered));
    QObject::connect(m_pUi->noteEditorBackgroundColorPushButton,
                     QNSIGNAL(QPushButton,clicked),
                     this,
                     QNSLOT(PreferencesDialog,onNoteEditorBackgroundColorDialogRequested));
    QObject::connect(m_pUi->noteEditorHighlightColorLineEdit,
                     QNSIGNAL(QLineEdit,editingFinished),
                     this,
                     QNSLOT(PreferencesDialog,onNoteEditorHighlightColorCodeEntered));
    QObject::connect(m_pUi->noteEditorHighlightColorPushButton,
                     QNSIGNAL(QPushButton,clicked),
                     this,
                     QNSLOT(PreferencesDialog,onNoteEditorHighlightColorDialogRequested));
    QObject::connect(m_pUi->noteEditorHighlightedTextColorLineEdit,
                     QNSIGNAL(QLineEdit,editingFinished),
                     this,
                     QNSLOT(PreferencesDialog,onNoteEditorHighlightedTextColorCodeEntered));
    QObject::connect(m_pUi->noteEditorHighlightedTextColorPushButton,
                     QNSIGNAL(QPushButton,clicked),
                     this,
                     QNSLOT(PreferencesDialog,onNoteEditorHighlightedTextColorDialogRequested));
    QObject::connect(m_pUi->noteEditorResetColorsPushButton,
                     QNSIGNAL(QPushButton,clicked),
                     this,
                     QNSLOT(PreferencesDialog,onNoteEditorColorsReset));

    QObject::connect(m_pUi->downloadNoteThumbnailsCheckBox,
                     QNSIGNAL(QCheckBox,toggled,bool),
                     this,
                     QNSLOT(PreferencesDialog,
                            onDownloadNoteThumbnailsCheckboxToggled,bool));
    QObject::connect(m_pUi->downloadInkNoteImagesCheckBox,
                     QNSIGNAL(QCheckBox,toggled,bool),
                     this,
                     QNSLOT(PreferencesDialog,
                            onDownloadInkNoteImagesCheckboxToggled,bool));

    QObject::connect(m_pUi->networkProxyTypeComboBox,
                     SIGNAL(currentIndexChanged(int)),
                     this,
                     SLOT(onNetworkProxyTypeChanged(int)));
    QObject::connect(m_pUi->networkProxyHostLineEdit,
                     QNSIGNAL(QLineEdit,editingFinished),
                     this,
                     QNSLOT(PreferencesDialog,onNetworkProxyHostChanged));
    QObject::connect(m_pUi->networkProxyPortSpinBox,
                     SIGNAL(valueChanged(int)),
                     this,
                     SLOT(onNetworkProxyPortChanged(int)));
    QObject::connect(m_pUi->networkProxyUserLineEdit,
                     QNSIGNAL(QLineEdit,editingFinished),
                     this,
                     QNSLOT(PreferencesDialog,onNetworkProxyUsernameChanged));
    QObject::connect(m_pUi->networkProxyPasswordLineEdit,
                     QNSIGNAL(QLineEdit,editingFinished),
                     this,
                     QNSLOT(PreferencesDialog,onNetworkProxyPasswordChanged));
    QObject::connect(m_pUi->networkProxyPasswordShowCheckBox,
                     QNSIGNAL(QCheckBox,toggled,bool),
                     this,
                     QNSLOT(PreferencesDialog,
                            onNetworkProxyPasswordVisibilityToggled,bool));

    QObject::connect(m_pUi->enableInternalLogViewerLogsCheckBox,
                     QNSIGNAL(QCheckBox,toggled,bool),
                     this,
                     QNSLOT(PreferencesDialog,
                            onEnableLogViewerInternalLogsCheckboxToggled,bool));
}

void PreferencesDialog::checkAndSetNetworkProxy()
{
    QNDEBUG(QStringLiteral("PreferencesDialog::checkAndSetNetworkProxy"));

    QNetworkProxy proxy;

    int proxyTypeIndex = m_pUi->networkProxyTypeComboBox->currentIndex();
    switch(proxyTypeIndex)
    {
    case 1:
        proxy.setType(QNetworkProxy::HttpProxy);
        break;
    case 2:
        proxy.setType(QNetworkProxy::Socks5Proxy);
        break;
    default:
        {
            proxy.setType(QNetworkProxy::NoProxy);
            persistNetworkProxySettingsForAccount(m_accountManager.currentAccount(),
                                                  proxy);
            break;
        }
    }

    QUrl proxyUrl = m_pUi->networkProxyHostLineEdit->text();
    if (!proxyUrl.isValid())
    {
        m_pUi->synchronizationTabStatusLabel->setText(
            QStringLiteral("<span style=\"color:#ff0000;\">") +
            tr("Network proxy host is not a valid URL") +
            QStringLiteral("</span>"));

        m_pUi->synchronizationTabStatusLabel->show();
        QNDEBUG(QStringLiteral("Invalid network proxy host: ")
                << m_pUi->networkProxyHostLineEdit->text()
                << QStringLiteral(", resetting the application proxy to no proxy"));
        proxy = QNetworkProxy(QNetworkProxy::NoProxy);
        persistNetworkProxySettingsForAccount(m_accountManager.currentAccount(),
                                              proxy);
        QNetworkProxy::setApplicationProxy(proxy);
        return;
    }

    proxy.setHostName(m_pUi->networkProxyHostLineEdit->text());

    int proxyPort = m_pUi->networkProxyPortSpinBox->value();
    if (Q_UNLIKELY((proxyPort < 0) ||
                   (proxyPort >= std::numeric_limits<quint16>::max())))
    {
        m_pUi->synchronizationTabStatusLabel->setText(
            QStringLiteral("<span style=\"color:#ff0000;\">") +
            tr("Network proxy port is outside the allowed range") +
            QStringLiteral("</span>"));

        m_pUi->synchronizationTabStatusLabel->show();
        QNDEBUG(QStringLiteral("Invalid network proxy port: ") << proxyPort
                << QStringLiteral(", resetting the application proxy to no proxy"));
        proxy = QNetworkProxy(QNetworkProxy::NoProxy);
        persistNetworkProxySettingsForAccount(m_accountManager.currentAccount(),
                                              proxy);
        QNetworkProxy::setApplicationProxy(proxy);
        return;
    }

    proxy.setPort(static_cast<quint16>(proxyPort));
    proxy.setUser(m_pUi->networkProxyUserLineEdit->text());
    proxy.setPassword(m_pUi->networkProxyPasswordLineEdit->text());

    QNDEBUG(QStringLiteral("Setting the application proxy: host = ")
            << proxy.hostName() << QStringLiteral(", port = ")
            << proxy.port() << QStringLiteral(", username = ")
            << proxy.user());

    m_pUi->synchronizationTabStatusLabel->clear();
    m_pUi->synchronizationTabStatusLabel->hide();

    persistNetworkProxySettingsForAccount(m_accountManager.currentAccount(),
                                          proxy);
    QNetworkProxy::setApplicationProxy(proxy);
}

bool PreferencesDialog::onNoteEditorColorEnteredImpl(const QColor & color,
                                                     const QColor & prevColor,
                                                     const QString & settingKey,
                                                     QLineEdit & colorLineEdit,
                                                     QFrame & demoFrame)
{
    if (!color.isValid()) {
        colorLineEdit.setText(prevColor.name());
        return false;
    }

    if (prevColor.isValid() && (prevColor.name() == color.name())) {
        QNDEBUG(QStringLiteral("Color did not change"));
        return false;
    }

    setNoteEditorColorToDemoFrameImpl(color, demoFrame);
    saveNoteEditorColorImpl(color, settingKey);
    return true;
}

void PreferencesDialog::setNoteEditorFontColorToDemoFrame(const QColor & color)
{
    setNoteEditorColorToDemoFrameImpl(color, *m_pUi->noteEditorFontColorDemoFrame);
}

void PreferencesDialog::setNoteEditorBackgroundColorToDemoFrame(const QColor & color)
{
    setNoteEditorColorToDemoFrameImpl(color, *m_pUi->noteEditorBackgroundColorDemoFrame);
}

void PreferencesDialog::setNoteEditorHighlightColorToDemoFrame(const QColor & color)
{
    setNoteEditorColorToDemoFrameImpl(color, *m_pUi->noteEditorHighlightColorDemoFrame);
}

void PreferencesDialog::setNoteEditorHighlightedTextColorToDemoFrame(const QColor & color)
{
    setNoteEditorColorToDemoFrameImpl(color, *m_pUi->noteEditorHighlightedTextColorDemoFrame);
}

void PreferencesDialog::setNoteEditorColorToDemoFrameImpl(const QColor & color,
                                                          QFrame & frame)
{
    QPalette pal = frame.palette();
    pal.setColor(QPalette::Background, color);
    frame.setPalette(pal);
}

QColor PreferencesDialog::noteEditorFontColor() const
{
    QColor color = noteEditorColorImpl(NOTE_EDITOR_FONT_COLOR_SETTINGS_KEY);
    if (color.isValid()) {
        return color;
    }

    return palette().color(QPalette::WindowText);
}

QColor PreferencesDialog::noteEditorBackgroundColor() const
{
    QColor color = noteEditorColorImpl(NOTE_EDITOR_BACKGROUND_COLOR_SETTINGS_KEY);
    if (color.isValid()) {
        return color;
    }

    return palette().color(QPalette::Base);
}

QColor PreferencesDialog::noteEditorHighlightColor() const
{
    QColor color = noteEditorColorImpl(NOTE_EDITOR_HIGHLIGHT_COLOR_SETTINGS_KEY);
    if (color.isValid()) {
        return color;
    }

    return palette().color(QPalette::Highlight);
}

QColor PreferencesDialog::noteEditorHighlightedTextColor() const
{
    QColor color = noteEditorColorImpl(NOTE_EDITOR_HIGHLIGHTED_TEXT_SETTINGS_KEY);
    if (color.isValid()) {
        return color;
    }

    return palette().color(QPalette::HighlightedText);
}

QColor PreferencesDialog::noteEditorColorImpl(const QString & settingKey) const
{
    Account currentAccount = m_accountManager.currentAccount();
    ApplicationSettings appSettings(currentAccount, QUENTIER_UI_SETTINGS);

    appSettings.beginGroup(NOTE_EDITOR_SETTINGS_GROUP_NAME);
    QColor color(appSettings.value(settingKey).toString());
    appSettings.endGroup();

    return color;
}

void PreferencesDialog::saveNoteEditorFontColor(const QColor & color)
{
    QNDEBUG(QStringLiteral("PreferencesDialog::saveNoteEditorFontColor: ")
            << color.name());

    saveNoteEditorColorImpl(color, NOTE_EDITOR_FONT_COLOR_SETTINGS_KEY);
}

void PreferencesDialog::saveNoteEditorBackgroundColor(const QColor & color)
{
    QNDEBUG(QStringLiteral("PreferencesDialog::saveNoteEditorBackgroundColor: ")
            << color.name());

    saveNoteEditorColorImpl(color, NOTE_EDITOR_BACKGROUND_COLOR_SETTINGS_KEY);
}

void PreferencesDialog::saveNoteEditorHighlightColor(const QColor & color)
{
    QNDEBUG(QStringLiteral("PreferencesDialog::saveNoteEditorHighlightColor: ")
            << color.name());

    saveNoteEditorColorImpl(color, NOTE_EDITOR_HIGHLIGHT_COLOR_SETTINGS_KEY);
}

void PreferencesDialog::saveNoteEditorHighlightedTextColor(const QColor & color)
{
    QNDEBUG(QStringLiteral("PreferencesDialog::saveNoteEditorHighlightedTextColor: ")
            << color.name());

    saveNoteEditorColorImpl(color, NOTE_EDITOR_HIGHLIGHTED_TEXT_SETTINGS_KEY);
}

void PreferencesDialog::saveNoteEditorColorImpl(const QColor & color,
                                                const QString & settingKey)
{
    Account currentAccount = m_accountManager.currentAccount();
    ApplicationSettings appSettings(currentAccount, QUENTIER_UI_SETTINGS);

    appSettings.beginGroup(NOTE_EDITOR_SETTINGS_GROUP_NAME);
    appSettings.setValue(settingKey, color.name());
    appSettings.endGroup();
}

void PreferencesDialog::workaroundQtBugWithTabWidget()
{
    // For reasons totally unknown to humankind Qt messes up with preferences
    // dialog's tabs. Badly. Some tabs sometimes disappear for unknown reasons.
    // This is the best I was able to do about it.

    m_pUi->preferencesTabWidget->clear();

    m_pUi->preferencesTabWidget->addTab(m_pUi->appearanceTab, tr("Appearance"));
    m_pUi->preferencesTabWidget->addTab(m_pUi->behaviourTab, tr("Behaviour"));
    m_pUi->preferencesTabWidget->addTab(m_pUi->systemTrayTab, tr("System tray"));
    m_pUi->preferencesTabWidget->addTab(m_pUi->noteEditorTab, tr("Note editor"));
    m_pUi->preferencesTabWidget->addTab(m_pUi->syncTab, tr("Synchronization"));
    m_pUi->preferencesTabWidget->addTab(m_pUi->shortcutsTab, tr("Shortcuts"));
    m_pUi->preferencesTabWidget->addTab(m_pUi->auxiliaryTab, tr("Auxiliary"));

    for(int i = 0; i < m_pUi->preferencesTabWidget->count(); ++i) {
        m_pUi->preferencesTabWidget->setTabEnabled(i, true);
    }
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
