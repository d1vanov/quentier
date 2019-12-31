/*
 * Copyright 2016-2019 Dmitry Ivanov
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

#ifndef QUENTIER_MAINWINDOW_H
#define QUENTIER_MAINWINDOW_H

#include <lib/account/AccountManager.h>
#include <lib/model/NotebookCache.h>
#include <lib/model/TagCache.h>
#include <lib/model/SavedSearchCache.h>
#include <lib/model/NoteCache.h>
#include <lib/model/NotebookModel.h>
#include <lib/model/TagModel.h>
#include <lib/model/SavedSearchModel.h>
#include <lib/model/NoteModel.h>
#include <lib/model/FavoritesModel.h>
#include <lib/widget/NoteEditorWidget.h>
#include <lib/widget/NoteEditorTabsAndWindowsCoordinator.h>
#include <lib/widget/SidePanelController.h>

#include <quentier/utility/ShortcutManager.h>
#include <quentier/local_storage/LocalStorageManagerAsync.h>
#include <quentier/synchronization/SynchronizationManager.h>

#include <quentier/utility/VersionInfo.h>
#if !LIB_QUENTIER_HAS_AUTHENTICATION_MANAGER
#error "Quentier needs libquentier built with authentication manager"
#endif

#include <quentier/synchronization/AuthenticationManager.h>

#include <QtCore>
#include <QtWidgets/QMainWindow>
#include <QTextListFormat>
#include <QMap>
#include <QStandardItemModel>
#include <QStringList>
#include <QMovie>
#include <QNetworkProxy>
#include <QScopedPointer>
#include <QVector>

#include <memory>
#include <vector>

namespace Ui {
class MainWindow;
}

QT_FORWARD_DECLARE_CLASS(QUrl)
QT_FORWARD_DECLARE_CLASS(QUndoStack)
QT_FORWARD_DECLARE_CLASS(QActionGroup)

QT_FORWARD_DECLARE_CLASS(ColumnChangeRerouter)

namespace quentier {
QT_FORWARD_DECLARE_CLASS(NoteEditor)
QT_FORWARD_DECLARE_CLASS(NoteFiltersManager)
QT_FORWARD_DECLARE_CLASS(EditNoteDialogsManager)
QT_FORWARD_DECLARE_CLASS(SystemTrayIconManager)
QT_FORWARD_DECLARE_CLASS(NoteCountLabelController)
}

using namespace quentier;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget * pParentWidget = nullptr);
    virtual ~MainWindow();

    void show();

    const SystemTrayIconManager & systemTrayIconManager() const;

public Q_SLOTS:
    void onSetStatusBarText(QString message, const int duration = 0);

Q_SIGNALS:
    void shown();
    void hidden();

    // private signals
    void localStorageSwitchUserRequest(
        Account account, LocalStorageManager::StartupOptions options,
        QUuid requestId);

    void authenticate();
    void authenticateCurrentAccount();
    void noteInfoDialogRequested(QString noteLocalUid);
    void synchronize();
    void stopSynchronization();

    void synchronizationSetAccount(Account account);
    void synchronizationDownloadNoteThumbnailsOptionChanged(bool enabled);
    void synchronizationDownloadInkNoteImagesOptionChanged(bool enabled);
    void synchronizationSetInkNoteImagesStoragePath(QString path);

    void showNoteThumbnailsStateChanged(
        bool showThumbnailsForAllNotes, QSet<QString> hideThumbnailsLocalUids);

private Q_SLOTS:
    void onUndoAction();
    void onRedoAction();
    void onCopyAction();
    void onCutAction();
    void onPasteAction();

    void onNoteTextSelectAllToggled();
    void onNoteTextBoldToggled();
    void onNoteTextItalicToggled();
    void onNoteTextUnderlineToggled();
    void onNoteTextStrikethroughToggled();
    void onNoteTextAlignLeftAction();
    void onNoteTextAlignCenterAction();
    void onNoteTextAlignRightAction();
    void onNoteTextAlignFullAction();
    void onNoteTextAddHorizontalLineAction();
    void onNoteTextIncreaseFontSizeAction();
    void onNoteTextDecreaseFontSizeAction();
    void onNoteTextHighlightAction();
    void onNoteTextIncreaseIndentationAction();
    void onNoteTextDecreaseIndentationAction();
    void onNoteTextInsertUnorderedListAction();
    void onNoteTextInsertOrderedListAction();
    void onNoteTextInsertToDoAction();
    void onNoteTextInsertTableDialogAction();
    void onNoteTextEditHyperlinkAction();
    void onNoteTextCopyHyperlinkAction();
    void onNoteTextRemoveHyperlinkAction();
    void onNoteTextSpellCheckToggled();
    void onShowNoteSource();
    void onSaveNoteAction();

    void onFindInsideNoteAction();
    void onFindPreviousInsideNoteAction();
    void onReplaceInsideNoteAction();

    void onImportEnexAction();

    // Synchronization manager slots
    void onSynchronizationStarted();
    void onSynchronizationStopped();
    void onSynchronizationManagerFailure(ErrorString errorDescription);

    void onSynchronizationFinished(
        Account account, bool somethingDownloaded, bool somethingSent);

    void onAuthenticationFinished(
        bool success, ErrorString errorDescription, Account account);

    void onAuthenticationRevoked(
        bool success, ErrorString errorDescription, qevercloud::UserID userId);

    void onRateLimitExceeded(qint32 secondsToWait);
    void onRemoteToLocalSyncDone(bool somethingDownloaded);

    void onSyncChunksDownloadProgress(
        qint32 highestDownloadedUsn, qint32 highestServerUsn,
        qint32 lastPreviousUsn);

    void onSyncChunksDownloaded();

    void onNotesDownloadProgress(
        quint32 notesDownloaded, quint32 totalNotesToDownload);

    void onResourcesDownloadProgress(
        quint32 resourcesDownloaded, quint32 totalResourcesToDownload);

    void onLinkedNotebookSyncChunksDownloadProgress(
        qint32 highestDownloadedUsn, qint32 highestServerUsn,
        qint32 lastPreviousUsn, LinkedNotebook linkedNotebook);

    void onLinkedNotebooksSyncChunksDownloaded();

    void onLinkedNotebooksNotesDownloadProgress(
        quint32 notesDownloaded, quint32 totalNotesToDownload);

    void onRemoteToLocalSyncStopped();
    void onSendLocalChangesStopped();

    // AccountManager slots
    void onEvernoteAccountAuthenticationRequested(
        QString host, QNetworkProxy proxy);

    void onAccountSwitched(Account account);
    void onAccountUpdated(Account account);
    void onAccountAdded(Account account);
    void onAccountRemoved(Account account);
    void onAccountManagerError(ErrorString errorDescription);

    // Toggle view slots
    void onShowSidePanelActionToggled(bool checked);
    void onShowFavoritesActionToggled(bool checked);
    void onShowNotebooksActionToggled(bool checked);
    void onShowTagsActionToggled(bool checked);
    void onShowSavedSearchesActionToggled(bool checked);
    void onShowDeletedNotesActionToggled(bool checked);
    void onShowNoteListActionToggled(bool checked);
    void onShowToolbarActionToggled(bool checked);
    void onShowStatusBarActionToggled(bool checked);

    // Look and feel settings slots
    void onSwitchIconTheme(const QString & iconTheme);
    void onSwitchIconThemeToNativeAction();
    void onSwitchIconThemeToTangoAction();
    void onSwitchIconThemeToOxygenAction();
    void onSwitchIconThemeToBreezeAction();
    void onSwitchIconThemeToBreezeDarkAction();

    void onSwitchPanelStyle(const QString & panelStyle);
    void onSwitchPanelStyleToBuiltIn();
    void onSwitchPanelStyleToLighter();
    void onSwitchPanelStyleToDarker();

    // View buttons slots
    void onNewNotebookCreationRequested();
    void onRemoveNotebookButtonPressed();
    void onNotebookInfoButtonPressed();

    void onNewTagCreationRequested();
    void onRemoveTagButtonPressed();
    void onTagInfoButtonPressed();

    void onNewSavedSearchCreationRequested();
    void onRemoveSavedSearchButtonPressed();
    void onSavedSearchInfoButtonPressed();

    void onUnfavoriteItemButtonPressed();
    void onFavoritedItemInfoButtonPressed();

    void onRestoreDeletedNoteButtonPressed();
    void onDeleteNotePermanentlyButtonPressed();
    void onDeletedNoteInfoButtonPressed();

    void showInfoWidget(QWidget * pWidget);

    void onFiltersViewTogglePushButtonPressed();

    void onShowPreferencesDialogAction();

    // Various note-related slots
    void onNoteSortingModeChanged(int index);
    void onNewNoteCreationRequested();
    void onCopyInAppLinkNoteRequested(QString noteLocalUid, QString noteGuid);

    /**
     * Toggle thumbnail preference on all notes (when noteLocalUid is empty)
     * or one given note.
     * @param noteLocalUid Either empty for all notes or local uid.
     */
    void onToggleThumbnailsPreference(QString noteLocalUid);

    void onCurrentNoteInListChanged(QString noteLocalUid);
    void onOpenNoteInSeparateWindow(QString noteLocalUid);

    void onDeleteCurrentNoteButtonPressed();
    void onCurrentNoteInfoRequested();
    void onCurrentNotePrintRequested();
    void onCurrentNotePdfExportRequested();

    void onExportNotesToEnexRequested(QStringList noteLocalUids);
    void onExportedNotesToEnex(QString enex);
    void onExportNotesToEnexFailed(ErrorString errorDescription);

    void onEnexFileWrittenSuccessfully(QString filePath);
    void onEnexFileWriteFailed(ErrorString errorDescription);
    void onEnexFileWriteIncomplete(qint64 bytesWritten, qint64 bytesTotal);

    void onEnexImportCompletedSuccessfully(QString enexFilePath);
    void onEnexImportFailed(ErrorString errorDescription);

    // Preferences dialog slots
    void onUseLimitedFontsPreferenceChanged(bool flag);
    void onShowNoteThumbnailsPreferenceChanged();
    void onDisableNativeMenuBarPreferenceChanged();
    void onRunSyncEachNumMinitesPreferenceChanged(int runSyncEachNumMinutes);

    // Note search-related slots
    void onSaveNoteSearchQueryButtonPressed();

    // SystemTrayIconManager slots
    void onNewNoteRequestedFromSystemTrayIcon();
    void onQuitRequestedFromSystemTrayIcon();
    void onSystemTrayIconManagerError(ErrorString errorDescription);
    void onShowRequestedFromTrayIcon();
    void onHideRequestedFromTrayIcon();

    void onViewLogsActionTriggered();
    void onShowInfoAboutQuentierActionTriggered();

    void onNoteEditorError(ErrorString error);
    void onModelViewError(ErrorString error);

    void onNoteEditorSpellCheckerNotReady();
    void onNoteEditorSpellCheckerReady();

    void onAddAccountActionTriggered(bool checked);
    void onManageAccountsActionTriggered(bool checked);
    void onSwitchAccountActionToggled(bool checked);

    void onLocalStorageSwitchUserRequestComplete(
        Account account, QUuid requestId);

    void onLocalStorageSwitchUserRequestFailed(
        Account account, ErrorString errorDescription, QUuid requestId);

    void onSplitterHandleMoved(int pos, int index);
    void onSidePanelSplittedHandleMoved(int pos, int index);

    void onSyncButtonPressed();
    void onAnimatedSyncIconFrameChanged(int frame);
    void onAnimatedSyncIconFrameChangedPendingFinish(int frame);
    void onSyncIconAnimationFinished();

    void onNewAccountCreationRequested();
    void onAccountSwitchRequested(Account account);
    void onQuitAction();

    void onShortcutChanged(
        int key, QKeySequence shortcut, const Account & account, QString context);

    void onNonStandardShortcutChanged(
        QString nonStandardKey, QKeySequence shortcut,
        const Account & account, QString context);

    void onDefaultAccountFirstNotebookAndNoteCreatorFinished(
        QString createdNoteLocalUid);

    void onDefaultAccountFirstNotebookAndNoteCreatorError(
        ErrorString errorDescription);

private:
    virtual void resizeEvent(QResizeEvent * pEvent) override;
    virtual void closeEvent(QCloseEvent * pEvent) override;
    virtual void timerEvent(QTimerEvent * pEvent) override;
    virtual void focusInEvent(QFocusEvent * pFocusEvent) override;
    virtual void focusOutEvent(QFocusEvent * pFocusEvent) override;
    virtual void showEvent(QShowEvent * pShowEvent) override;
    virtual void hideEvent(QHideEvent * pHideEvent) override;
    virtual void changeEvent(QEvent * pEvent) override;

private:
    void centerWidget(QWidget & widget);
    void centerDialog(QDialog & dialog);

    void setupThemeIcons();
    void setupAccountManager();
    void setupLocalStorageManager();

    void setupDisableNativeMenuBarPreference();
    void setupDefaultAccount();

    void setupModels();
    void clearModels();

    void setupShowHideStartupSettings();
    void setupViews();
    void clearViews();

    void setupAccountSpecificUiElements();
    void setupNoteFilters();
    void setupNoteEditorTabWidgetsCoordinator();

    bool checkLocalStorageVersion(const Account & account);

    bool onceDisplayedGreeterScreen() const;
    void setOnceDisplayedGreeterScreen();

    struct SetAccountOption
    {
        enum type
        {
            Set = 0,
            DontSet
        };
    };

    void setupSynchronizationManager(
        const SetAccountOption::type = SetAccountOption::DontSet);

    void clearSynchronizationManager();
    void setSynchronizationOptions(const Account & account);
    void setupSynchronizationManagerThread();
    void setupRunSyncPeriodicallyTimer();
    void launchSynchronization();

    void setupDefaultShortcuts();
    void setupUserShortcuts();
    void startListeningForShortcutChanges();
    void stopListeningForShortcutChanges();

    void setupConsumerKeyAndSecret(QString & consumerKey,
                                   QString & consumerSecret);

    void connectActionsToSlots();
    void connectViewButtonsToSlots();
    void connectNoteSearchActionsToSlots();
    void connectToolbarButtonsToSlots();
    void connectSystemTrayIconManagerSignalsToSlots();

    void addMenuActionsToMainWindow();
    void updateSubMenuWithAvailableAccounts();

    void setupInitialChildWidgetsWidths();
    void setWindowTitleForAccount(const Account & account);

    NoteEditorWidget * currentNoteEditorTab();

    void createNewNote(
        NoteEditorTabsAndWindowsCoordinator::NoteEditorMode::type noteEditorMode);

    void connectSynchronizationManager();
    void disconnectSynchronizationManager();

    void startSyncButtonAnimation();
    void stopSyncButtonAnimation();
    void scheduleSyncButtonAnimationStop();

    void startListeningForSplitterMoves();
    void stopListeningForSplitterMoves();

    void persistChosenIconTheme(const QString & iconThemeName);
    void refreshChildWidgetsThemeIcons();
    void refreshNoteEditorWidgetsSpecialIcons();

    void persistChosenNoteSortingMode(int index);
    NoteModel::NoteSortingMode::type restoreNoteSortingMode();

    void persistGeometryAndState();
    void restoreGeometryAndState();

    void restoreSplitterSizes();
    void scheduleSplitterSizesRestoration();

    void scheduleGeometryAndStatePersisting();

    template <class T>
    void refreshThemeIcons();

    void showHideViewColumnsForAccountType(const Account::Type::type accountType);

    void expandFiltersView();
    void foldFiltersView();

    void adjustNoteListAndFiltersSplitterSizes();

    void clearDir(const QString & path);

    void setupSidePanelControllers();
    void setPanelStyleToControllers(const QString & panelStyle);

    bool getShowNoteThumbnailsPreference() const;
    bool getDisableNativeMenuBarPreference() const;

    /**
     * Get a set with note local uids.
     */
    QSet<QString> getHideNoteThumbnailsFor() const;

    /**
     * Toggle value of "hide current note thumbnail".
     */
    void toggleHideNoteThumbnailFor(QString noteLocalUid) const;

    /**
     * Toggle value of "show note thumbnails".
     */
    void toggleShowNoteThumbnails() const;

    QString fallbackIconThemeName() const;

private:
    Ui::MainWindow *        m_pUI;
    QWidget *               m_currentStatusBarChildWidget;
    QString                 m_lastNoteEditorHtml;

    QString                 m_nativeIconThemeName;
    QActionGroup *          m_pAvailableAccountsActionGroup;
    QMenu *                 m_pAvailableAccountsSubMenu;

    AccountManager *            m_pAccountManager;
    QScopedPointer<Account>     m_pAccount;

    SystemTrayIconManager *     m_pSystemTrayIconManager;

    QThread *                   m_pLocalStorageManagerThread;
    LocalStorageManagerAsync *  m_pLocalStorageManagerAsync;

    QUuid                       m_lastLocalStorageSwitchUserRequest;

    QThread *                   m_pSynchronizationManagerThread;
    AuthenticationManager *     m_pAuthenticationManager;
    SynchronizationManager *    m_pSynchronizationManager;
    QString                     m_synchronizationManagerHost;
    QNetworkProxy               m_applicationProxyBeforeNewEvernoteAccountAuthenticationRequest;
    bool                        m_pendingNewEvernoteAccountAuthentication;
    bool                        m_pendingCurrentEvernoteAccountAuthentication;
    bool                        m_authenticatedCurrentEvernoteAccount;
    bool                        m_pendingSwitchToNewEvernoteAccount;

    bool                        m_syncInProgress;
    bool                        m_syncApiRateLimitExceeded;

    QMovie                      m_animatedSyncButtonIcon;
    int                         m_runSyncPeriodicallyTimerId;

    NotebookCache           m_notebookCache;
    TagCache                m_tagCache;
    SavedSearchCache        m_savedSearchCache;
    NoteCache               m_noteCache;

    NotebookModel *         m_pNotebookModel;
    TagModel *              m_pTagModel;
    SavedSearchModel *      m_pSavedSearchModel;
    NoteModel *             m_pNoteModel;

    NoteCountLabelController *  m_pNoteCountLabelController;

    ColumnChangeRerouter *  m_pNotebookModelColumnChangeRerouter;
    ColumnChangeRerouter *  m_pTagModelColumnChangeRerouter;
    ColumnChangeRerouter *  m_pNoteModelColumnChangeRerouter;
    ColumnChangeRerouter *  m_pFavoritesModelColumnChangeRerouter;

    NoteModel *             m_pDeletedNotesModel;
    FavoritesModel *        m_pFavoritesModel;

    QStandardItemModel      m_blankModel;

    NoteFiltersManager *    m_pNoteFiltersManager;

    int                     m_setDefaultAccountsFirstNoteAsCurrentDelayTimerId;
    QString                 m_defaultAccountFirstNoteLocalUid;

    NoteEditorTabsAndWindowsCoordinator *   m_pNoteEditorTabsAndWindowsCoordinator;
    EditNoteDialogsManager *                m_pEditNoteDialogsManager;

    QUndoStack *            m_pUndoStack;

    QString                 m_currentPanelStyle;
    std::vector<std::unique_ptr<SidePanelController>>   m_sidePanelControllers;

    quentier::ShortcutManager   m_shortcutManager;
    QHash<int, QAction*>        m_shortcutKeyToAction;
    QHash<QString, QAction*>    m_nonStandardShortcutKeyToAction;

    bool                    m_pendingGreeterDialog;
    bool                    m_pendingFirstShutdownDialog;

    bool                    m_filtersViewExpanded;
    bool                    m_onceSetupNoteSortingModeComboBox;

    bool                    m_geometryRestored;
    bool                    m_stateRestored;
    bool                    m_shown;

    int                     m_geometryAndStatePersistingDelayTimerId;
    int                     m_splitterSizesRestorationDelayTimerId;
};

#endif // QUENTIER_MAINWINDOW_H
