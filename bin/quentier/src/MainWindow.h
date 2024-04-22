/*
 * Copyright 2016-2024 Dmitry Ivanov
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

#include <lib/account/AccountManager.h>
#include <lib/model/favorites/FavoritesModel.h>
#include <lib/model/note/NoteCache.h>
#include <lib/model/note/NoteModel.h>
#include <lib/model/notebook/NotebookCache.h>
#include <lib/model/notebook/NotebookModel.h>
#include <lib/model/saved_search/SavedSearchCache.h>
#include <lib/model/saved_search/SavedSearchModel.h>
#include <lib/model/tag/TagCache.h>
#include <lib/model/tag/TagModel.h>

#ifdef WITH_UPDATE_MANAGER
#include <lib/update/UpdateManager.h>
#endif

#include <lib/widget/NoteEditorTabsAndWindowsCoordinator.h>
#include <lib/widget/NoteEditorWidget.h>
#include <lib/widget/panel/SidePanelStyleController.h>

#include <quentier/local_storage/Fwd.h>
#include <quentier/synchronization/Fwd.h>
#include <quentier/utility/ShortcutManager.h>
#include <quentier/utility/cancelers/Fwd.h>

#include <quentier/utility/VersionInfo.h>
#if !LIB_QUENTIER_HAS_AUTHENTICATION_MANAGER
#error "Quentier needs libquentier built with authentication manager"
#endif

#include <QLinearGradient>
#include <QMap>
#include <QMovie>
#include <QNetworkProxy>
#include <QStandardItemModel>
#include <QStringList>
#include <QTextListFormat>
#include <QVector>

#include <QtCore>
#include <QtWidgets/QMainWindow>

#include <memory>
#include <optional>
#include <vector>

namespace Ui {

class MainWindow;

} // namespace Ui

class QActionGroup;
class QUrl;

namespace quentier {

class ColumnChangeRerouter;
class EditNoteDialogsManager;
class NoteCountLabelController;
class NoteEditor;
class NoteFiltersManager;
class PreferencesDialog;
class SystemTrayIconManager;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget * parentWidget = nullptr);
    ~MainWindow() override;

    void show();

    [[nodiscard]] const SystemTrayIconManager & systemTrayIconManager()
        const noexcept;

public Q_SLOTS:
    void onSetStatusBarText(QString message, int durationMsec = 0);

Q_SIGNALS:
    void shown();
    void hidden();

    // FIXME: remove this when it is no longer necessary
    // private signals
    /*
    void localStorageSwitchUserRequest(
        Account account, LocalStorageManager::StartupOptions options,
        QUuid requestId);

    void authenticate();
    void authenticateCurrentAccount();

    void synchronize();
    void stopSynchronization();

    void synchronizationSetAccount(Account account);
    */

    void noteInfoDialogRequested(QString noteLocalId);

    void synchronizationDownloadNoteThumbnailsOptionChanged(bool enabled);
    void synchronizationDownloadInkNoteImagesOptionChanged(bool enabled);
    void synchronizationSetInkNoteImagesStoragePath(QString path);

    void showNoteThumbnailsStateChanged(
        bool showThumbnailsForAllNotes, QSet<QString> hideThumbnailsLocalIds);

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

    // Synchronization slots
    void onSynchronizationFinished(
        Account account, bool somethingDownloaded, bool somethingSent);

    void onAuthenticationFinished(
        bool success, ErrorString errorDescription, Account account);

    void onRateLimitExceeded(qint32 secondsToWait);
    void onRemoteToLocalSyncDone(bool somethingDownloaded);

    void onSyncChunksDownloadProgress(
        qint32 highestDownloadedUsn, qint32 highestServerUsn,
        qint32 lastPreviousUsn);

    void onSyncChunksDownloaded();

    void onSyncChunksDataProcessingProgress(
        const quentier::synchronization::ISyncChunksDataCountersPtr & counters);

    void onNotesDownloadProgress(
        quint32 notesDownloaded, quint32 totalNotesToDownload);

    void onResourcesDownloadProgress(
        quint32 resourcesDownloaded, quint32 totalResourcesToDownload);

    void onLinkedNotebookSyncChunksDownloadProgress(
        qint32 highestDownloadedUsn, qint32 highestServerUsn,
        qint32 lastPreviousUsn, qevercloud::LinkedNotebook linkedNotebook);

    void onLinkedNotebooksSyncChunksDownloaded();

    void onLinkedNotebookSyncChunksDataProcessingProgress(
        const synchronization::ISyncChunksDataCountersPtr & counters);

    void onLinkedNotebooksNotesDownloadProgress(
        quint32 notesDownloaded, quint32 totalNotesToDownload);

    // AccountManager slots
    void onEvernoteAccountAuthenticationRequested(
        QString host, QNetworkProxy proxy);

    void onAccountSwitched(Account account);
    void onAccountUpdated(Account account);
    void onAccountAdded(const Account & account);
    void onAccountRemoved(const Account & account);
    void onAccountManagerError(const ErrorString & errorDescription);

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

    void showInfoWidget(QWidget * widget);

    void onFiltersViewTogglePushButtonPressed();

    void onShowPreferencesDialogAction();

    // Various note-related slots
    void onNoteSortingModeChanged(int index);
    void onNewNoteCreationRequested();
    void onCopyInAppLinkNoteRequested(
        const QString & noteLocalId, const QString & noteGuid);

    void onFavoritedNoteSelected(const QString & noteLocalId);

    /**
     * Toggle thumbnail preference on all notes (when noteLocalId is empty)
     * or one given note.
     * @param noteLocalId Either empty for all notes or local uid.
     */
    void onToggleThumbnailsPreference(const QString & noteLocalId);

    void onCurrentNoteInListChanged(const QString & noteLocalId);
    void onOpenNoteInSeparateWindow(const QString & noteLocalId);

    void onDeleteCurrentNoteButtonPressed();
    void onCurrentNoteInfoRequested();
    void onCurrentNotePrintRequested();
    void onCurrentNotePdfExportRequested();

    void onExportNotesToEnexRequested(QStringList noteLocalIds);
    void onExportedNotesToEnex(const QString & enex);
    void onExportNotesToEnexFailed(const ErrorString & errorDescription);

    void onEnexFileWrittenSuccessfully(const QString & filePath);
    void onEnexFileWriteFailed(const ErrorString & errorDescription);
    void onEnexFileWriteIncomplete(qint64 bytesWritten, qint64 bytesTotal);

    void onEnexImportCompletedSuccessfully(const QString & enexFilePath);
    void onEnexImportFailed(const ErrorString & errorDescription);

    // Preferences dialog slots
    void onUseLimitedFontsPreferenceChanged(bool flag);
    void onShowNoteThumbnailsPreferenceChanged();
    void onDisableNativeMenuBarPreferenceChanged();
    void onRunSyncEachNumMinitesPreferenceChanged(int runSyncEachNumMinutes);

    void onPanelFontColorChanged(QColor color);
    void onPanelBackgroundColorChanged(QColor color);
    void onPanelUseBackgroundGradientSettingChanged(bool useBackgroundGradient);
    void onPanelBackgroundLinearGradientChanged(
        const QLinearGradient & gradient);

    // SystemTrayIconManager slots
    void onNewNoteRequestedFromSystemTrayIcon();
    void onQuitRequestedFromSystemTrayIcon();
    void onSystemTrayIconManagerError(const ErrorString & errorDescription);
    void onShowRequestedFromTrayIcon();
    void onHideRequestedFromTrayIcon();

    void onViewLogsActionTriggered();
    void onShowInfoAboutQuentierActionTriggered();

    void onNoteEditorError(const ErrorString & error);
    void onModelViewError(const ErrorString & error);

    void onNoteEditorSpellCheckerNotReady();
    void onNoteEditorSpellCheckerReady();

    void onAddAccountActionTriggered(bool checked);
    void onManageAccountsActionTriggered(bool checked);
    void onSwitchAccountActionToggled(bool checked);

    void onSplitterHandleMoved(int pos, int index);
    void onSidePanelSplittedHandleMoved(int pos, int index);

    void onSyncButtonPressed();
    void onAnimatedSyncIconFrameChanged(int frame);
    void onAnimatedSyncIconFrameChangedPendingFinish(int frame);
    void onSyncIconAnimationFinished();

    void onNewAccountCreationRequested();
    void onAccountSwitchRequested(const Account & account);
    void onQuitAction();

    void onShortcutChanged(
        int key, const QKeySequence & shortcut, const Account & account,
        const QString & context);

    void onNonStandardShortcutChanged(
        const QString & nonStandardKey, const QKeySequence & shortcut,
        const Account & account, const QString & context);

    void onDefaultAccountFirstNotebookAndNoteCreatorFinished(
        const QString & createdNoteLocalId);

    void onDefaultAccountFirstNotebookAndNoteCreatorError(
        const ErrorString & errorDescription);

#ifdef WITH_UPDATE_MANAGER
    void onCheckForUpdatesActionTriggered();
    void onUpdateManagerError(const ErrorString & errorDescription);
    void onUpdateManagerRequestsRestart();
#endif

private:
    void resizeEvent(QResizeEvent * event) override;
    void closeEvent(QCloseEvent * event) override;
    void timerEvent(QTimerEvent * event) override;
    void focusInEvent(QFocusEvent * focusEvent) override;
    void focusOutEvent(QFocusEvent * focusEvent) override;
    void showEvent(QShowEvent * showEvent) override;
    void hideEvent(QHideEvent * hideEvent) override;
    void changeEvent(QEvent * event) override;

private:
    void centerWidget(QWidget & widget);
    void centerDialog(QDialog & dialog);

    void setupThemeIcons();
    void setupAccountManager();
    void setupLocalStorage();

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

#ifdef WITH_UPDATE_MANAGER
    void setupUpdateManager();
#endif

    [[nodiscard]] bool checkLocalStorageVersion(
        const Account & account, ErrorString & errorDescription);

    [[nodiscard]] bool onceDisplayedGreeterScreen() const;
    void setOnceDisplayedGreeterScreen();

    enum class SetAccountOption
    {
        Set = 0,
        DontSet
    };

    friend QDebug & operator<<(QDebug & dbg, SetAccountOption option);

    void setupSynchronizer(
        const SetAccountOption option = SetAccountOption::DontSet);

    void stopSynchronization();
    void clearSynchronizer();
    void clearSynchronizationCounters();

    void setupRunSyncPeriodicallyTimer();
    void launchSynchronization();

    bool shouldRunSyncOnStartup() const;

    void setupDefaultShortcuts();
    void setupUserShortcuts();
    void startListeningForShortcutChanges();
    void stopListeningForShortcutChanges();

    void setupConsumerKeyAndSecret(
        QString & consumerKey, QString & consumerSecret);

    void connectActionsToSlots();
    void connectViewButtonsToSlots();
    void connectToolbarButtonsToSlots();
    void connectSystemTrayIconManagerSignalsToSlots();
    void connectToPreferencesDialogSignals(PreferencesDialog & dialog);

    void addMenuActionsToMainWindow();
    void updateSubMenuWithAvailableAccounts();

    void setupInitialChildWidgetsWidths();
    void setWindowTitleForAccount(const Account & account);

    [[nodiscard]] NoteEditorWidget * currentNoteEditorTab();

    void createNewNote(
        NoteEditorTabsAndWindowsCoordinator::NoteEditorMode noteEditorMode);

    // FIXME: remove this when it's no longer necessary
    /*
    void connectSynchronizer();
    void disconnectSynchronizer();
    */

    void startSyncButtonAnimation();
    void stopSyncButtonAnimation();
    void scheduleSyncButtonAnimationStop();

    void startListeningForSplitterMoves();
    void stopListeningForSplitterMoves();

    void persistChosenIconTheme(const QString & iconThemeName);
    void refreshChildWidgetsThemeIcons();
    void refreshNoteEditorWidgetsSpecialIcons();

    void persistChosenNoteSortingMode(int index);
    [[nodiscard]] NoteModel::NoteSortingMode restoreNoteSortingMode();

    void persistGeometryAndState();
    void restoreGeometryAndState();

    void restoreSplitterSizes();
    void scheduleSplitterSizesRestoration();

    void scheduleGeometryAndStatePersisting();

    template <class T>
    void refreshThemeIcons();

    void showHideViewColumnsForAccountType(Account::Type accountType);

    void expandFiltersView();
    void foldFiltersView();

    void adjustNoteListAndFiltersSplitterSizes();

    void restorePanelColors();
    void setupGenericPanelStyleControllers();
    void setupSidePanelStyleControllers();

    [[nodiscard]] bool getShowNoteThumbnailsPreference() const;
    [[nodiscard]] bool getDisableNativeMenuBarPreference() const;

    /**
     * Note local ids of notes which thumbnails shouldn't be displayed
     */
    [[nodiscard]] QSet<QString> notesWithHiddenThumbnails() const;

    /**
     * Toggle value of "hide thumbnail" preference for note
     */
    void toggleHideNoteThumbnail(const QString & noteLocalId);

    /**
     * Toggle value of "show note thumbnails".
     */
    void toggleShowNoteThumbnails() const;

    [[nodiscard]] QString fallbackIconThemeName() const;

    void quitApp(int exitCode = 0);

    [[nodiscard]] utility::cancelers::ICancelerPtr setupSyncCanceler();

private:
    Ui::MainWindow * m_ui;
    QWidget * m_currentStatusBarChildWidget = nullptr;
    QString m_lastNoteEditorHtml;

    QString m_nativeIconThemeName;
    QActionGroup * m_availableAccountsActionGroup;
    QMenu * m_availableAccountsSubMenu = nullptr;

    AccountManager * m_accountManager;
    std::optional<Account> m_account;

    SystemTrayIconManager * m_systemTrayIconManager = nullptr;

    local_storage::ILocalStoragePtr m_localStorage;
    synchronization::IAuthenticatorPtr m_authenticator;
    synchronization::ISynchronizerPtr m_synchronizer;
    synchronization::ISyncEventsNotifier * m_syncEventsNotifier = nullptr;
    synchronization::ISyncOptionsPtr m_syncOptions;
    utility::cancelers::ManualCancelerPtr m_synchronizationCanceler;

    QString m_synchronizationRemoteHost;

    QNetworkProxy
        m_applicationProxyBeforeNewEvernoteAccountAuthenticationRequest;

    bool m_pendingNewEvernoteAccountAuthentication = false;
    bool m_pendingCurrentEvernoteAccountAuthentication = false;
    bool m_authenticatedCurrentEvernoteAccount = false;
    bool m_pendingSwitchToNewEvernoteAccount = false;

    bool m_syncInProgress = false;
    bool m_syncApiRateLimitExceeded = false;

    double m_lastSyncNotesDownloadedPercentage = 0.0;
    double m_lastSyncResourcesDownloadedPercentage = 0.0;
    double m_lastSyncLinkedNotebookNotesDownloadedPercentage = 0.0;

    qint64 m_syncChunksDownloadedTimestamp = 0;
    double m_lastSyncChunksDataProcessingProgressPercentage = 0.0;

    qint64 m_linkedNotebookSyncChunksDownloadedTimestamp = 0;
    double m_lastLinkedNotebookSyncChunksDataProcessingProgressPercentage = 0.0;

    QMovie m_animatedSyncButtonIcon;
    int m_runSyncPeriodicallyTimerId = 0;

    NotebookCache m_notebookCache;
    TagCache m_tagCache;
    SavedSearchCache m_savedSearchCache;
    NoteCache m_noteCache;

    NotebookModel * m_notebookModel = nullptr;
    TagModel * m_tagModel = nullptr;
    SavedSearchModel * m_savedSearchModel = nullptr;
    NoteModel * m_noteModel = nullptr;

    NoteCountLabelController * m_noteCountLabelController = nullptr;

    ColumnChangeRerouter * m_notebookModelColumnChangeRerouter;
    ColumnChangeRerouter * m_tagModelColumnChangeRerouter;
    ColumnChangeRerouter * m_noteModelColumnChangeRerouter;
    ColumnChangeRerouter * m_favoritesModelColumnChangeRerouter;

    NoteModel * m_deletedNotesModel = nullptr;
    FavoritesModel * m_favoritesModel = nullptr;

    QStandardItemModel m_blankModel;

    NoteFiltersManager * m_noteFiltersManager = nullptr;

    int m_setDefaultAccountsFirstNoteAsCurrentDelayTimerId = 0;
    QString m_defaultAccountFirstNoteLocalId;

    NoteEditorTabsAndWindowsCoordinator *
        m_noteEditorTabsAndWindowsCoordinator = nullptr;
    EditNoteDialogsManager * m_editNoteDialogsManager = nullptr;

    QColor m_overridePanelFontColor;
    QColor m_overridePanelBackgroundColor;
    QLinearGradient m_overridePanelBackgroundGradient;
    bool m_panelUseBackgroundGradient = false;
    std::vector<std::unique_ptr<PanelStyleController>>
        m_genericPanelStyleControllers;
    std::vector<std::unique_ptr<SidePanelStyleController>>
        m_sidePanelStyleControllers;

    quentier::ShortcutManager m_shortcutManager;
    QHash<int, QAction *> m_shortcutKeyToAction;
    QHash<QString, QAction *> m_nonStandardShortcutKeyToAction;

#ifdef WITH_UPDATE_MANAGER
    std::shared_ptr<UpdateManager::IIdleStateInfoProvider>
        m_updateManagerIdleInfoProvider;
    UpdateManager * m_updateManager = nullptr;
#endif

    bool m_pendingGreeterDialog = false;
    bool m_pendingFirstShutdownDialog = false;

    bool m_filtersViewExpanded = false;
    bool m_onceSetupNoteSortingModeComboBox = false;

    bool m_geometryRestored = false;
    bool m_stateRestored = false;
    bool m_shown = false;

    int m_geometryAndStatePersistingDelayTimerId = 0;
    int m_splitterSizesRestorationDelayTimerId = 0;
};

} // namespace quentier
