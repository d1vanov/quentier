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

#ifndef QUENTIER_DIALOGS_PREFERENCES_DIALOG_H
#define QUENTIER_DIALOGS_PREFERENCES_DIALOG_H

#include <quentier/utility/Macros.h>
#include <QDialog>
#include <QColor>

namespace Ui {
class PreferencesDialog;
}

QT_FORWARD_DECLARE_CLASS(QStringListModel)

namespace quentier {

QT_FORWARD_DECLARE_CLASS(AccountManager)
QT_FORWARD_DECLARE_CLASS(ShortcutManager)
QT_FORWARD_DECLARE_CLASS(SystemTrayIconManager)
QT_FORWARD_DECLARE_CLASS(ActionsInfo)

class PreferencesDialog: public QDialog
{
    Q_OBJECT
public:
    explicit PreferencesDialog(AccountManager & accountManager,
                               ShortcutManager & shortcutManager,
                               SystemTrayIconManager & systemTrayIconManager,
                               ActionsInfo & actionsInfo,
                               QWidget * parent = Q_NULLPTR);
    virtual ~PreferencesDialog();

Q_SIGNALS:
    void noteEditorUseLimitedFontsOptionChanged(bool enabled);
    void noteEditorFontColorChanged(QColor color);
    void noteEditorBackgroundColorChanged(QColor color);
    void noteEditorHighlightColorChanged(QColor color);
    void noteEditorHighlightedTextColorChanged(QColor color);

    void synchronizationDownloadNoteThumbnailsOptionChanged(bool enabled);
    void synchronizationDownloadInkNoteImagesOptionChanged(bool enabled);
    void showNoteThumbnailsOptionChanged(bool enabled);
    void runSyncPeriodicallyOptionChanged(int runSyncEachNumMinutes);

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

    // Note editor tab
    void onNoteEditorUseLimitedFontsCheckboxToggled(bool checked);

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

private:
    void setupCurrentSettingsState(ActionsInfo & actionsInfo,
                                   ShortcutManager & shortcutManager);
    void setupSystemTraySettings();
    void setupRunSyncEachNumMinutesComboBox(int currentNumMinutes);
    void setupNetworkProxySettingsState();
    void setupNoteEditorSettingsState();
    void createConnections();

    void checkAndSetNetworkProxy();

private:
    Ui::PreferencesDialog *         m_pUi;
    AccountManager &                m_accountManager;
    SystemTrayIconManager &         m_systemTrayIconManager;
    QStringListModel *              m_pTrayActionsModel;
    QStringListModel *              m_pNetworkProxyTypesModel;
};

} // namespace quentier

#endif // QUENTIER_DIALOGS_PREFERENCES_DIALOG_H
