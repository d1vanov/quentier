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

#pragma once

#ifdef WITH_UPDATE_MANAGER
#include "UpdateSettings.h"
#endif

#include <QColor>
#include <QDialog>
#include <QPointer>

#include <string_view>

namespace Ui {
class PreferencesDialog;
} // namespace Ui

class QColorDialog;
class QFrame;
class QLineEdit;
class QStringListModel;

namespace quentier {

class AccountManager;
class ActionsInfo;
class ShortcutManager;
class SystemTrayIconManager;

class PreferencesDialog final : public QDialog
{
    Q_OBJECT
public:
    explicit PreferencesDialog(
        AccountManager & accountManager, ShortcutManager & shortcutManager,
        SystemTrayIconManager & systemTrayIconManager,
        ActionsInfo & actionsInfo, QWidget * parent = nullptr);

    ~PreferencesDialog() override;

Q_SIGNALS:
    void noteEditorUseLimitedFontsOptionChanged(bool enabled);
    void noteEditorFontColorChanged(QColor color);
    void noteEditorBackgroundColorChanged(QColor color);
    void noteEditorHighlightColorChanged(QColor color);
    void noteEditorHighlightedTextColorChanged(QColor color);
    void noteEditorColorsReset();

    void synchronizationDownloadNoteThumbnailsOptionChanged(bool enabled);
    void synchronizationDownloadInkNoteImagesOptionChanged(bool enabled);
    void showNoteThumbnailsOptionChanged();
    void disableNativeMenuBarOptionChanged();
    void runSyncPeriodicallyOptionChanged(int runSyncEachNumMinutes);
    void runSyncOnStartupChanged(bool enabled);

    void iconThemeChanged(QString iconThemeName);

    void panelFontColorChanged(QColor color);
    void panelBackgroundColorChanged(QColor color);
    void panelUseBackgroundGradientSettingChanged(bool useBackgroundGradient);
    void panelBackgroundLinearGradientChanged(QLinearGradient gradient);

#if WITH_UPDATE_MANAGER
    void checkForUpdatesOptionChanged(bool enabled);
    void checkForUpdatesOnStartupOptionChanged(bool enabled);
    void useContinuousUpdateChannelOptionChanged(bool enabled);
    void checkForUpdatesIntervalChanged(qint64 intervalMsec);
    void updateChannelChanged(QString channel);
    void updateProviderChanged(UpdateProvider provider);

    void checkForUpdatesRequested();
#endif

    void filterByNotebookOptionChanged(bool enabled);
    void filterByTagOptionChanged(bool enabled);
    void filterBySavedSearchOptionChanged(bool enabled);
    void filterByFavoritedItemsOptionChanged(bool enabled);

private Q_SLOTS:
    // System tray tab
    void onShowSystemTrayIconCheckboxToggled(bool checked);
    void onCloseToSystemTrayCheckboxToggled(bool checked);
    void onMinimizeToSystemTrayCheckboxToggled(bool checked);
    void onStartMinimizedToSystemTrayCheckboxToggled(bool checked);

    void onSingleClickTrayActionChanged(int action);
    void onMiddleClickTrayActionChanged(int action);
    void onDoubleClickTrayActionChanged(int action);

    // Appearance tab
    void onShowNoteThumbnailsCheckboxToggled(bool checked);
    void onDisableNativeMenuBarCheckboxToggled(bool checked);
    void onPanelColorWidgetUserError(QString errorMessage);
    void onIconThemeChanged(int iconThemeIndex);

    // Behaviour tab
    void onStartAtLoginCheckboxToggled(bool checked);
    void onStartAtLoginOptionChanged(int option);

#ifdef WITH_UPDATE_MANAGER
    void onCheckForUpdatesCheckboxToggled(bool checked);
    void onCheckForUpdatesOnStartupCheckboxToggled(bool checked);
    void onUseContinuousUpdateChannelCheckboxToggled(bool checked);
    void onCheckForUpdatesIntervalChanged(int option);
    void onUpdateChannelChanged(int index);
    void onUpdateProviderChanged(int index);
#endif

    void onFilterByNotebookCheckboxToggled(bool checked);
    void onFilterByTagCheckboxToggled(bool checked);
    void onFilterBySavedSearchCheckboxToggled(bool checked);
    void onFilterByFavoritedItemsCheckboxToggled(bool checked);

    // Note editor tab
    void onNoteEditorUseLimitedFontsCheckboxToggled(bool checked);

    void onNoteEditorFontColorCodeEntered();
    void onNoteEditorFontColorDialogRequested();
    void onNoteEditorFontColorSelected(const QColor & color);

    void onNoteEditorBackgroundColorCodeEntered();
    void onNoteEditorBackgroundColorDialogRequested();
    void onNoteEditorBackgroundColorSelected(const QColor & color);

    void onNoteEditorHighlightColorCodeEntered();
    void onNoteEditorHighlightColorDialogRequested();
    void onNoteEditorHighlightColorSelected(const QColor & color);

    void onNoteEditorHighlightedTextColorCodeEntered();
    void onNoteEditorHighlightedTextColorDialogRequested();
    void onNoteEditorHighlightedTextColorSelected(const QColor & color);

    void onNoteEditorColorsReset();

    // Synchronization tab
    void onDownloadNoteThumbnailsCheckboxToggled(bool checked);
    void onDownloadInkNoteImagesCheckboxToggled(bool checked);
    void onRunSyncPeriodicallyOptionChanged(int index);
    void onRunSyncOnStartupOptionChanged(bool checked);

    void onNetworkProxyTypeChanged(int type);
    void onNetworkProxyHostChanged();
    void onNetworkProxyPortChanged(int port);
    void onNetworkProxyUsernameChanged();
    void onNetworkProxyPasswordChanged();
    void onNetworkProxyPasswordVisibilityToggled(bool checked);

    // Auxiliary tab
    void onEnableLogViewerInternalLogsCheckboxToggled(bool checked);

private:
    bool eventFilter(QObject * pObject, QEvent * pEvent) override;
    void timerEvent(QTimerEvent * pEvent) override;

private:
    void setupInitialPreferencesState(
        ActionsInfo & actionsInfo, ShortcutManager & shortcutManager);

    void setupSystemTrayPreferences();
    void setupStartAtLoginPreferences();
    void setupCheckForUpdatesPreferences();
    void setupFilteringPreferences();
    void setupRunSyncPeriodicallyComboBox(int currentNumMinutes);
    void setupAppearancePreferences(const ActionsInfo & actionsInfo);
    void setupNetworkProxyPreferences();
    void setupNoteEditorPreferences();

    void createConnections();
    void installEventFilters();

    void checkAndSetNetworkProxy();

    bool onNoteEditorColorEnteredImpl(
        const QColor & color, const QColor & prevColor, std::string_view key,
        QLineEdit & colorLineEdit, QFrame & demoFrame);

    void setNoteEditorFontColorToDemoFrame(const QColor & color);
    void setNoteEditorBackgroundColorToDemoFrame(const QColor & color);
    void setNoteEditorHighlightColorToDemoFrame(const QColor & color);
    void setNoteEditorHighlightedTextColorToDemoFrame(const QColor & color);
    void setNoteEditorColorToDemoFrameImpl(
        const QColor & color, QFrame & frame);

    [[nodiscard]] QColor noteEditorFontColor() const;
    [[nodiscard]] QColor noteEditorBackgroundColor() const;
    [[nodiscard]] QColor noteEditorHighlightColor() const;
    [[nodiscard]] QColor noteEditorHighlightedTextColor() const;
    [[nodiscard]] QColor noteEditorColorImpl(std::string_view key) const;

    void saveNoteEditorFontColor(const QColor & color);
    void saveNoteEditorBackgroundColor(const QColor & color);
    void saveNoteEditorHighlightColor(const QColor & color);
    void saveNoteEditorHighlightedTextColor(const QColor & color);
    void saveNoteEditorColorImpl(const QColor & color, std::string_view key);

private:
    Ui::PreferencesDialog * m_ui;
    AccountManager & m_accountManager;
    SystemTrayIconManager & m_systemTrayIconManager;
    QStringListModel * m_trayActionsModel;
    QStringListModel * m_networkProxyTypesModel;
    QStringListModel * m_startAtLoginOptionsModel;

    QPointer<QColorDialog> m_noteEditorFontColorDialog;
    QPointer<QColorDialog> m_noteEditorBackgroundColorDialog;
    QPointer<QColorDialog> m_noteEditorHighlightColorDialog;
    QPointer<QColorDialog> m_noteEditorHighlightedTextColorDialog;

    int m_clearAndHideStatusBarTimerId = 0;
};

} // namespace quentier
