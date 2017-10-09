/*
 * Copyright 2016 Dmitry Ivanov
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

#include "AccountManager.h"
#include "NoteEditorTabsAndWindowsCoordinator.h"
#include "models/NotebookCache.h"
#include "models/TagCache.h"
#include "models/SavedSearchCache.h"
#include "models/NoteCache.h"
#include "models/NotebookModel.h"
#include "models/TagModel.h"
#include "models/SavedSearchModel.h"
#include "models/NoteModel.h"
#include "models/FavoritesModel.h"
#include "widgets/NoteEditorWidget.h"
#include <quentier/utility/ShortcutManager.h>
#include <quentier/local_storage/LocalStorageManagerAsync.h>
#include <quentier/synchronization/SynchronizationManager.h>

#include <quentier/utility/VersionInfo.h>
#if !LIB_QUENTIER_HAS_AUTHENTICATION_MANAGER
#error "Quentier needs libquentier built with authentication manager"
#endif

#include <quentier/synchronization/AuthenticationManager.h>

#include <QtCore>

#if QT_VERSION >= 0x050000
#include <QtWidgets/QMainWindow>
#else
#include <QMainWindow>
#endif

#include <QTextListFormat>
#include <QMap>
#include <QStandardItemModel>
#include <QStringList>
#include <QMovie>
#include <QNetworkProxy>

namespace Ui {
class MainWindow;
}

QT_FORWARD_DECLARE_CLASS(QUrl)
QT_FORWARD_DECLARE_CLASS(QUndoStack)
QT_FORWARD_DECLARE_CLASS(QActionGroup)

QT_FORWARD_DECLARE_CLASS(ColumnChangeRerouter)

namespace quentier {
QT_FORWARD_DECLARE_CLASS(NoteEditor)
QT_FORWARD_DECLARE_CLASS(NoteFilterModel)
QT_FORWARD_DECLARE_CLASS(NoteFiltersManager)
QT_FORWARD_DECLARE_CLASS(EditNoteDialogsManager)
QT_FORWARD_DECLARE_CLASS(SystemTrayIconManager)
}

using namespace quentier;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget * pParentWidget = Q_NULLPTR);
    virtual ~MainWindow();

    void show();

    const SystemTrayIconManager & systemTrayIconManager() const;

public Q_SLOTS:
    void onSetStatusBarText(QString message, const int duration = 0);

Q_SIGNALS:
    void shown();
    void hidden();

    // private signals
    void localStorageSwitchUserRequest(Account account, bool startFromScratch, QUuid requestId);
    void authenticate();
    void noteInfoDialogRequested(QString noteLocalUid);
    void synchronize();
    void stopSynchronization();

    void synchronizationSetAccount(Account account);
    void synchronizationDownloadNoteThumbnailsOptionChanged(bool enabled);
    void synchronizationDownloadInkNoteImagesOptionChanged(bool enabled);
    void synchronizationSetInkNoteImagesStoragePath(QString path);

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
    void onSynchronizationFinished(Account account);
    void onAuthenticationFinished(bool success, ErrorString errorDescription,
                                  Account account);
    void onAuthenticationRevoked(bool success, ErrorString errorDescription,
                                 qevercloud::UserID userId);
    void onRateLimitExceeded(qint32 secondsToWait);
    void onRemoteToLocalSyncDone();
    void onSyncChunksDownloaded();
    void onNotesDownloadProgress(quint32 notesDownloaded, quint32 totalNotesToDownload);
    void onLinkedNotebooksSyncChunksDownloaded();
    void onLinkedNotebooksNotesDownloadProgress(quint32 notesDownloaded, quint32 totalNotesToDownload);

    void onRemoteToLocalSyncStopped();
    void onSendLocalChangesStopped();

    // AccountManager slots
    void onEvernoteAccountAuthenticationRequested(QString host, QNetworkProxy proxy);
    void onAccountSwitched(Account account);
    void onAccountUpdated(Account account);
    void onAccountAdded(Account account);
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
    void onSwitchIconsToNativeAction();
    void onSwitchIconsToTangoAction();
    void onSwitchIconsToOxygenAction();

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

    void onShowSettingsDialogAction();

    // Various note-related slots
    void onNoteSortingModeChanged(int index);
    void onNewNoteCreationRequested();
    void onCopyInAppLinkNoteRequested(QString noteLocalUid, QString noteGuid);

    void onNoteModelAllNotesListed();

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
    void onShowNoteThumbnailsPreferenceChanged(bool flag);

    // Note search-related slots
    void onNoteSearchQueryChanged(const QString & query);
    void onNoteSearchQueryReady();
    void onSaveNoteSearchQueryButtonPressed();

    // SystemTrayIconManager slots
    void onNewNoteRequestedFromSystemTrayIcon();
    void onQuitRequestedFromSystemTrayIcon();
    void onAccountSwitchRequestedFromSystemTrayIcon(Account account);
    void onSystemTrayIconManagerError(ErrorString errorDescription);

    void onViewLogsActionTriggered();

    void onNoteEditorError(ErrorString error);
    void onModelViewError(ErrorString error);

    void onNoteEditorSpellCheckerNotReady();
    void onNoteEditorSpellCheckerReady();

    void onAddAccountActionTriggered(bool checked);
    void onManageAccountsActionTriggered(bool checked);
    void onSwitchAccountActionToggled(bool checked);

    void onLocalStorageSwitchUserRequestComplete(Account account, QUuid requestId);
    void onLocalStorageSwitchUserRequestFailed(Account account, ErrorString errorDescription, QUuid requestId);

    void onSplitterHandleMoved(int pos, int index);
    void onSidePanelSplittedHandleMoved(int pos, int index);

    void onSyncButtonPressed();
    void onAnimatedSyncIconFrameChanged(int frame);
    void onAnimatedSyncIconFrameChangedPendingFinish(int frame);
    void onSyncIconAnimationFinished();

    void onSynchronizationManagerSetAccountDone(Account account);
    void onSynchronizationManagerSetDownloadNoteThumbnailsDone(bool flag);
    void onSynchronizationManagerSetDownloadInkNoteImagesDone(bool flag);
    void onSynchronizationManagerSetInkNoteImagesStoragePathDone(QString path);

    void onQuitAction();

    void onShortcutChanged(int key, QKeySequence shortcut, const Account & account, QString context);
    void onNonStandardShortcutChanged(QString nonStandardKey, QKeySequence shortcut,
                                      const Account & account, QString context);

private:
    virtual void resizeEvent(QResizeEvent * pEvent) Q_DECL_OVERRIDE;
    virtual void closeEvent(QCloseEvent * pEvent) Q_DECL_OVERRIDE;
    virtual void timerEvent(QTimerEvent * pEvent) Q_DECL_OVERRIDE;
    virtual void focusInEvent(QFocusEvent * pFocusEvent) Q_DECL_OVERRIDE;
    virtual void focusOutEvent(QFocusEvent * pFocusEvent) Q_DECL_OVERRIDE;
    virtual void showEvent(QShowEvent * pShowEvent) Q_DECL_OVERRIDE;
    virtual void hideEvent(QHideEvent * pHideEvent) Q_DECL_OVERRIDE;
    virtual void changeEvent(QEvent * pEvent) Q_DECL_OVERRIDE;

private:
    void setupThemeIcons();
    void setupAccountManager();
    void setupLocalStorageManager();

    void setupModels();
    void clearModels();

    void setupShowHideStartupSettings();
    void setupViews();
    void clearViews();

    void setupAccountSpecificUiElements();
    void setupNoteFilters();
    void setupNoteEditorTabWidgetsCoordinator();

    struct SetAccountOption
    {
        enum type
        {
            Set = 0,
            DontSet
        };
    };

    void setupSynchronizationManager(const SetAccountOption::type = SetAccountOption::DontSet);
    void clearSynchronizationManager();
    void setAccountToSyncManager(const Account & account);
    void setSynchronizationOptions(const Account & account);
    void checkAndLaunchPendingSync();

    void setupDefaultShortcuts();
    void setupUserShortcuts();
    void startListeningForShortcutChanges();
    void stopListeningForShortcutChanges();

    void setupConsumerKeyAndSecret(QString & consumerKey, QString & consumerSecret);

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
    void createNewNote(NoteEditorTabsAndWindowsCoordinator::NoteEditorMode::type noteEditorMode);

    void connectSynchronizationManager();
    void disconnectSynchronizationManager();

    void startSyncButtonAnimation();
    void stopSyncButtonAnimation();
    void scheduleSyncButtonAnimationStop();

    bool checkNoteSearchQuery(const QString & noteSearchQuery);

    void persistChosenIconTheme(const QString & iconThemeName);
    void refreshChildWidgetsThemeIcons();
    void refreshNoteEditorWidgetsSpecialIcons();

    void persistChosenNoteSortingMode(int index);
    bool restoreNoteSortingMode();

    void persistGeometryAndState();
    void restoreGeometryAndState();

    void scheduleGeometryAndStatePersisting();

    template <class T>
    void refreshThemeIcons();

    void showHideViewColumnsForAccountType(const Account::Type::type accountType);

    void expandFiltersView();
    void foldFiltersView();

    void adjustNoteListAndFiltersSplitterSizes();

    void clearDir(const QString & path);

    class StyleSheetProperty
    {
    public:
        struct Target
        {
            enum type {
                None = 0,
                ButtonHover,
                ButtonPressed
            };
        };

        StyleSheetProperty(const Target::type targetType = Target::None,
                           const char * name = Q_NULLPTR,
                           const QString & value = QString()) :
            m_targetType(targetType),
            m_name(name),
            m_value(value)
        {}

        Target::type    m_targetType;
        const char *    m_name;
        QString         m_value;
    };

    typedef QVector<StyleSheetProperty> StyleSheetProperties;

    void collectBaseStyleSheets();
    void setupPanelOverlayStyleSheets();
    void getPanelStyleSheetProperties(const QString & panelStyleOption, StyleSheetProperties & properties) const;
    void setPanelsOverlayStyleSheet(const StyleSheetProperties & properties);

    // This method performs a nasty hack - it searches for some properties within
    // the passed in stylesheet and alters some of these; the whole workflow is based
    // on weak assumptions about the structure of the stylesheet so once it is sufficiently
    // altered, this method would stop working. Don't program like this, kids.
    QString alterStyleSheet(const QString & originalStyleSheet,
                            const StyleSheetProperties & properties);

    bool isInsideStyleBlock(const QString & styleSheet, const QString & styleBlockStartSearchString,
                            const int currentIndex, bool & error) const;

#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
    // Qt4 has a problem with zero-size QSplitter handles - they don't work that way.
    // Hence, need to set the alternate stylesheet for Qt4 version, with non-zero-size
    // QSplitter handles and some other elements' boundaries removed
    void fixupQt4StyleSheets();
#endif

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
    bool                        m_pendingSwitchToNewEvernoteAccount;

    bool                        m_syncInProgress;
    bool                        m_pendingSynchronizationManagerSetAccount;
    bool                        m_pendingSynchronizationManagerSetDownloadNoteThumbnailsOption;
    bool                        m_pendingSynchronizationManagerSetDownloadInkNoteImagesOption;
    bool                        m_pendingSynchronizationManagerSetInkNoteImagesStoragePath;
    bool                        m_pendingSynchronizationManagerResponseToStartSync;
    QMovie                      m_animatedSyncButtonIcon;
    QString                     m_noteThumbnailsStoragePath;

    NotebookCache           m_notebookCache;
    TagCache                m_tagCache;
    SavedSearchCache        m_savedSearchCache;
    NoteCache               m_noteCache;

    NotebookModel *         m_pNotebookModel;
    TagModel *              m_pTagModel;
    SavedSearchModel *      m_pSavedSearchModel;
    NoteModel *             m_pNoteModel;

    ColumnChangeRerouter *  m_pNotebookModelColumnChangeRerouter;
    ColumnChangeRerouter *  m_pTagModelColumnChangeRerouter;
    ColumnChangeRerouter *  m_pNoteModelColumnChangeRerouter;
    ColumnChangeRerouter *  m_pFavoritesModelColumnChangeRerouter;

    NoteModel *             m_pDeletedNotesModel;
    FavoritesModel *        m_pFavoritesModel;

    QStandardItemModel      m_blankModel;

    NoteFilterModel *       m_pNoteFilterModel;
    NoteFiltersManager *    m_pNoteFiltersManager;

    struct NoteSortingModes
    {
        // This enum defines the order in which the items are stored in the combobox determining the notes sorting order
        enum type
        {
            CreatedAscending = 0,
            CreatedDescending,
            ModifiedAscending,
            ModifiedDescending,
            TitleAscending,
            TitleDescending,
            SizeAscending,
            SizeDescending
        };
    };

    NoteEditorTabsAndWindowsCoordinator *   m_pNoteEditorTabsAndWindowsCoordinator;
    EditNoteDialogsManager *                m_pEditNoteDialogsManager;

    QUndoStack *            m_pUndoStack;

    bool                    m_noteSearchQueryValidated;

    struct StyleSheetInfo
    {
        QPointer<QWidget>   m_targetWidget;
        QString             m_baseStyleSheet;
        QString             m_overlayStyleSheet;
    };

    QHash<QWidget*, StyleSheetInfo>    m_styleSheetInfo;

    QString                     m_currentPanelStyle;

    quentier::ShortcutManager   m_shortcutManager;
    QHash<int, QAction*>        m_shortcutKeyToAction;
    QHash<QString, QAction*>    m_nonStandardShortcutKeyToAction;

    bool                    m_filtersViewExpanded;
    bool                    m_onceSetupNoteSortingModeComboBox;

    bool                    m_geometryRestored;
    bool                    m_stateRestored;
    bool                    m_shown;

    int                     m_geometryAndStatePersistingDelayTimerId;
};

#endif // QUENTIER_MAINWINDOW_H
