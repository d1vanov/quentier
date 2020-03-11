/*
 * Copyright 2017-2020 Dmitry Ivanov
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

#ifndef QUENTIER_LIB_PREFERENCES_PREFERENCES_DIALOG_H
#define QUENTIER_LIB_PREFERENCES_PREFERENCES_DIALOG_H

#include <quentier/utility/Macros.h>

#include <QDialog>
#include <QColor>
#include <QPointer>

namespace Ui {
class PreferencesDialog;
}

QT_FORWARD_DECLARE_CLASS(QColorDialog)
QT_FORWARD_DECLARE_CLASS(QFrame)
QT_FORWARD_DECLARE_CLASS(QLineEdit)
QT_FORWARD_DECLARE_CLASS(QStringListModel)

namespace quentier {

QT_FORWARD_DECLARE_CLASS(AccountManager)
QT_FORWARD_DECLARE_CLASS(ActionsInfo)
QT_FORWARD_DECLARE_CLASS(ShortcutManager)
QT_FORWARD_DECLARE_CLASS(SystemTrayIconManager)

class PreferencesDialog: public QDialog
{
    Q_OBJECT
public:
    explicit PreferencesDialog(
        AccountManager & accountManager,
        ShortcutManager & shortcutManager,
        SystemTrayIconManager & systemTrayIconManager,
        ActionsInfo & actionsInfo,
        QWidget * parent = nullptr);

    virtual ~PreferencesDialog() override;

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

    void iconThemeChanged(const QString & iconTheme);

    void panelFontColorChanged(QColor color);
    void panelBackgroundColorChanged(QColor color);
    void panelUseBackgroundGradientSettingChanged(bool useBackgroundGradient);
    void panelBackgroundLinearGradientChanged(QLinearGradient gradient);

    void checkForUpdatesOptionChanged(bool enabled);
    void checkForUpdatesOnStartupOptionChanged(bool enabled);
    void useContinuousUpdateChannelOptionChanged(bool enabled);
    void checkForUpdatesIntervalChanged(qint64 intervalMsec);
    void updateChannelChanged(QString channel);
    void updateProviderChanged(QString providerName);

    void checkForUpdatesRequested();

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

    // Behaviour tab
    void onStartAtLoginCheckboxToggled(bool checked);
    void onStartAtLoginOptionChanged(int option);

    void onCheckForUpdatesCheckboxToggled(bool checked);
    void onCheckForUpdatesOnStartupCheckboxToggled(bool checked);
    void onUseContinuousUpdateChannelCheckboxToggled(bool checked);
    void onCheckForUpdatesIntervalChanged(int option);
    void onUpdateChannelChanged(const QString & channel);
    void onUpdateProviderChanged(const QString & provider);

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

    void onNetworkProxyTypeChanged(int type);
    void onNetworkProxyHostChanged();
    void onNetworkProxyPortChanged(int port);
    void onNetworkProxyUsernameChanged();
    void onNetworkProxyPasswordChanged();
    void onNetworkProxyPasswordVisibilityToggled(bool checked);

    // Auxiliary tab
    void onEnableLogViewerInternalLogsCheckboxToggled(bool checked);

private:
    virtual bool eventFilter(QObject * pObject, QEvent * pEvent) override;
    virtual void timerEvent(QTimerEvent * pEvent) override;

private:
    void setupCurrentSettingsState(
        ActionsInfo & actionsInfo, ShortcutManager & shortcutManager);

    void setupSystemTraySettings();
    void setupStartAtLoginSettings();
    void setupCheckForUpdatesSettings();
    void setupRunSyncEachNumMinutesComboBox(int currentNumMinutes);
    void setupAppearanceSettingsState(const ActionsInfo & actionsInfo);
    void setupNetworkProxySettingsState();
    void setupNoteEditorSettingsState();
    void createConnections();
    void installEventFilters();

    void checkAndSetNetworkProxy();

    bool onNoteEditorColorEnteredImpl(
        const QColor & color, const QColor & prevColor,
        const QString & settingKey, QLineEdit & colorLineEdit,
        QFrame & demoFrame);

    void setNoteEditorFontColorToDemoFrame(const QColor & color);
    void setNoteEditorBackgroundColorToDemoFrame(const QColor & color);
    void setNoteEditorHighlightColorToDemoFrame(const QColor & color);
    void setNoteEditorHighlightedTextColorToDemoFrame(const QColor & color);

    void setNoteEditorColorToDemoFrameImpl(
        const QColor & color, QFrame & frame);

    QColor noteEditorFontColor() const;
    QColor noteEditorBackgroundColor() const;
    QColor noteEditorHighlightColor() const;
    QColor noteEditorHighlightedTextColor() const;
    QColor noteEditorColorImpl(const QString & settingKey) const;

    void saveNoteEditorFontColor(const QColor & color);
    void saveNoteEditorBackgroundColor(const QColor & color);
    void saveNoteEditorHighlightColor(const QColor & color);
    void saveNoteEditorHighlightedTextColor(const QColor & color);

    void saveNoteEditorColorImpl(
        const QColor & color, const QString & settingKey);

private:
    Ui::PreferencesDialog *         m_pUi;
    AccountManager &                m_accountManager;
    SystemTrayIconManager &         m_systemTrayIconManager;
    QStringListModel *              m_pTrayActionsModel;
    QStringListModel *              m_pNetworkProxyTypesModel;
    QStringListModel *              m_pStartAtLoginOptionsModel;

    QPointer<QColorDialog>          m_pNoteEditorFontColorDialog;
    QPointer<QColorDialog>          m_pNoteEditorBackgroundColorDialog;
    QPointer<QColorDialog>          m_pNoteEditorHighlightColorDialog;
    QPointer<QColorDialog>          m_pNoteEditorHighlightedTextColorDialog;

    int     m_clearAndHideStatusBarTimerId = 0;
};

} // namespace quentier

#endif // QUENTIER_LIB_PREFERENCES_PREFERENCES_DIALOG_H
