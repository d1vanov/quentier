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

#include "MainWindow.h"

#include <lib/delegate/NotebookItemDelegate.h>
#include <lib/delegate/SynchronizableColumnDelegate.h>
#include <lib/delegate/DirtyColumnDelegate.h>
#include <lib/delegate/FavoriteItemDelegate.h>
#include <lib/delegate/FromLinkedNotebookColumnDelegate.h>
#include <lib/delegate/NoteItemDelegate.h>
#include <lib/delegate/TagItemDelegate.h>
#include <lib/delegate/DeletedNoteItemDelegate.h>
#include <lib/dialog/EditNoteDialog.h>
#include <lib/dialog/EditNoteDialogsManager.h>
#include <lib/dialog/AddOrEditNotebookDialog.h>
#include <lib/dialog/AddOrEditTagDialog.h>
#include <lib/dialog/AddOrEditSavedSearchDialog.h>
#include <lib/dialog/FirstShutdownDialog.h>
#include <lib/dialog/LocalStorageUpgradeDialog.h>
#include <lib/dialog/LocalStorageVersionTooHighDialog.h>
#include <lib/dialog/WelcomeToQuentierDialog.h>
#include <lib/enex/EnexExportDialog.h>
#include <lib/enex/EnexImportDialog.h>
#include <lib/enex/EnexExporter.h>
#include <lib/enex/EnexImporter.h>
#include <lib/exception/LocalStorageVersionTooHighException.h>
#include <lib/initialization/DefaultAccountFirstNotebookAndNoteCreator.h>
#include <lib/model/ColumnChangeRerouter.h>
#include <lib/network/NetworkProxySettingsHelpers.h>
#include <lib/preferences/DefaultDisableNativeMenuBar.h>
#include <lib/preferences/SettingsNames.h>
#include <lib/preferences/DefaultSettings.h>
#include <lib/preferences/PreferencesDialog.h>
#include <lib/tray/SystemTrayIconManager.h>
#include <lib/utility/AsyncFileWriter.h>
#include <lib/utility/ActionsInfo.h>
#include <lib/utility/QObjectThreadMover.h>
#include <lib/view/ItemView.h>
#include <lib/view/DeletedNoteItemView.h>
#include <lib/view/NotebookItemView.h>
#include <lib/view/NoteListView.h>
#include <lib/view/TagItemView.h>
#include <lib/view/SavedSearchItemView.h>
#include <lib/view/FavoriteItemView.h>
#include <lib/widget/color-picker-tool-button/ColorPickerToolButton.h>
#include <lib/widget/insert-table-tool-button/InsertTableToolButton.h>
#include <lib/widget/insert-table-tool-button/TableSettingsDialog.h>
#include <lib/widget/NoteFiltersManager.h>
#include <lib/widget/NoteCountLabelController.h>
#include <lib/widget/FindAndReplaceWidget.h>
#include <lib/widget/PanelWidget.h>
using quentier::PanelWidget;

#include <lib/widget/TabWidget.h>
using quentier::TabWidget;

#include <lib/widget/FilterByNotebookWidget.h>
using quentier::FilterByNotebookWidget;

#include <lib/widget/FilterByTagWidget.h>
using quentier::FilterByTagWidget;

#include <lib/widget/FilterBySavedSearchWidget.h>
using quentier::FilterBySavedSearchWidget;

#include <lib/widget/LogViewerWidget.h>
using quentier::LogViewerWidget;

#include <lib/widget/NotebookModelItemInfoWidget.h>
#include <lib/widget/TagModelItemInfoWidget.h>
#include <lib/widget/SavedSearchModelItemInfoWidget.h>
#include <lib/widget/AboutQuentierWidget.h>

#include <quentier/note_editor/NoteEditor.h>

#include "ui_MainWindow.h"

#include <quentier/types/Note.h>
#include <quentier/types/Notebook.h>
#include <quentier/types/Resource.h>
#include <quentier/utility/QuentierCheckPtr.h>
#include <quentier/utility/ApplicationSettings.h>
#include <quentier/utility/MessageBox.h>
#include <quentier/utility/StandardPaths.h>
#include <quentier/utility/Utility.h>
#include <quentier/local_storage/NoteSearchQuery.h>
#include <quentier/logging/QuentierLogger.h>

#include <qt5qevercloud/QEverCloud.h>

#include <QPushButton>
#include <QIcon>
#include <QLabel>
#include <QTextEdit>
#include <QTextCursor>
#include <QTextList>
#include <QColorDialog>
#include <QFile>
#include <QScopedPointer>
#include <QMessageBox>
#include <QtDebug>
#include <QFontDatabase>
#include <QKeySequence>
#include <QUndoStack>
#include <QCryptographicHash>
#include <QXmlStreamWriter>
#include <QDateTime>
#include <QCheckBox>
#include <QPalette>
#include <QToolTip>
#include <QResizeEvent>
#include <QTimerEvent>
#include <QFocusEvent>
#include <QMenu>
#include <QThreadPool>
#include <QDir>
#include <QClipboard>
#include <QNetworkAccessManager>

#include <cmath>
#include <algorithm>

#define NOTIFY_ERROR(error)                                                    \
    QNWARNING(QString::fromUtf8(error));                                       \
    onSetStatusBarText(QString::fromUtf8(error), SEC_TO_MSEC(30))              \
// NOTIFY_ERROR

#define NOTIFY_DEBUG(message)                                                  \
    QNDEBUG(QString::fromUtf8(message));                                       \
    onSetStatusBarText(QString::fromUtf8(message), SEC_TO_MSEC(30))            \
// NOTIFY_DEBUG

#define FILTERS_VIEW_STATUS_KEY QStringLiteral("ViewExpandedStatus")
#define NOTE_SORTING_MODE_KEY QStringLiteral("NoteSortingMode")

#define MAIN_WINDOW_GEOMETRY_KEY QStringLiteral("Geometry")
#define MAIN_WINDOW_STATE_KEY QStringLiteral("State")

#define MAIN_WINDOW_SIDE_PANEL_WIDTH_KEY QStringLiteral("SidePanelWidth")
#define MAIN_WINDOW_NOTE_LIST_WIDTH_KEY QStringLiteral("NoteListWidth")

#define MAIN_WINDOW_FAVORITES_VIEW_HEIGHT QStringLiteral("FavoritesViewHeight")
#define MAIN_WINDOW_NOTEBOOKS_VIEW_HEIGHT QStringLiteral("NotebooksViewHeight")
#define MAIN_WINDOW_TAGS_VIEW_HEIGHT QStringLiteral("TagsViewHeight")

#define MAIN_WINDOW_SAVED_SEARCHES_VIEW_HEIGHT                                 \
    QStringLiteral("SavedSearchesViewHeight")                                  \
// MAIN_WINDOW_SAVED_SEARCHES_VIEW_HEIGHT

#define MAIN_WINDOW_DELETED_NOTES_VIEW_HEIGHT                                  \
    QStringLiteral("DeletedNotesViewHeight")                                   \
// MAIN_WINDOW_DELETED_NOTES_VIEW_HEIGHT

#define PERSIST_GEOMETRY_AND_STATE_DELAY (500)
#define RESTORE_SPLITTER_SIZES_DELAY (200)
#define CREATE_SIDE_BORDERS_CONTROLLER_DELAY (200)
#define NOTIFY_SIDE_BORDERS_CONTROLLER_DELAY (200)

using namespace quentier;

MainWindow::MainWindow(QWidget * pParentWidget) :
    QMainWindow(pParentWidget),
    m_pUI(new Ui::MainWindow),
    m_currentStatusBarChildWidget(nullptr),
    m_lastNoteEditorHtml(),
    m_nativeIconThemeName(),
    m_pAvailableAccountsActionGroup(new QActionGroup(this)),
    m_pAvailableAccountsSubMenu(nullptr),
    m_pAccountManager(new AccountManager(this)),
    m_pAccount(),
    m_pSystemTrayIconManager(nullptr),
    m_pLocalStorageManagerThread(nullptr),
    m_pLocalStorageManagerAsync(nullptr),
    m_lastLocalStorageSwitchUserRequest(),
    m_pSynchronizationManagerThread(nullptr),
    m_pAuthenticationManager(nullptr),
    m_pSynchronizationManager(nullptr),
    m_synchronizationManagerHost(),
    m_applicationProxyBeforeNewEvernoteAccountAuthenticationRequest(),
    m_pendingNewEvernoteAccountAuthentication(false),
    m_pendingCurrentEvernoteAccountAuthentication(false),
    m_authenticatedCurrentEvernoteAccount(false),
    m_pendingSwitchToNewEvernoteAccount(false),
    m_syncInProgress(false),
    m_syncApiRateLimitExceeded(false),
    m_animatedSyncButtonIcon(QStringLiteral(":/sync/sync.gif")),
    m_runSyncPeriodicallyTimerId(0),
    m_notebookCache(),
    m_tagCache(),
    m_savedSearchCache(),
    m_noteCache(),
    m_pNotebookModel(nullptr),
    m_pTagModel(nullptr),
    m_pSavedSearchModel(nullptr),
    m_pNoteModel(nullptr),
    m_pNoteCountLabelController(nullptr),
    m_pNotebookModelColumnChangeRerouter(
        new ColumnChangeRerouter(NotebookModel::Columns::NumNotesPerNotebook,
                                 NotebookModel::Columns::Name, this)),
    m_pTagModelColumnChangeRerouter(
        new ColumnChangeRerouter(TagModel::Columns::NumNotesPerTag,
                                 TagModel::Columns::Name, this)),
    m_pNoteModelColumnChangeRerouter(
        new ColumnChangeRerouter(NoteModel::Columns::PreviewText,
                                 NoteModel::Columns::Title, this)),
    m_pFavoritesModelColumnChangeRerouter(
        new ColumnChangeRerouter(FavoritesModel::Columns::NumNotesTargeted,
                                 FavoritesModel::Columns::DisplayName, this)),
    m_pDeletedNotesModel(nullptr),
    m_pFavoritesModel(nullptr),
    m_blankModel(),
    m_pNoteFiltersManager(nullptr),
    m_setDefaultAccountsFirstNoteAsCurrentDelayTimerId(0),
    m_defaultAccountFirstNoteLocalUid(),
    m_pNoteEditorTabsAndWindowsCoordinator(nullptr),
    m_pEditNoteDialogsManager(nullptr),
    m_pUndoStack(new QUndoStack(this)),
    m_shortcutManager(this),
    m_pendingGreeterDialog(false),
    m_filtersViewExpanded(false),
    m_onceSetupNoteSortingModeComboBox(false),
    m_geometryRestored(false),
    m_stateRestored(false),
    m_shown(false),
    m_geometryAndStatePersistingDelayTimerId(0),
    m_splitterSizesRestorationDelayTimerId(0)
{
    QNTRACE("MainWindow constructor");

    setupAccountManager();

    auto accountSource = AccountManager::AccountSource::LastUsed;

    m_pAccount.reset(
        new Account(m_pAccountManager->currentAccount(&accountSource)));

    bool createdDefaultAccount =
        (accountSource == AccountManager::AccountSource::NewDefault);

    if (createdDefaultAccount && !onceDisplayedGreeterScreen()) {
        m_pendingGreeterDialog = true;
        setOnceDisplayedGreeterScreen();
    }

    restoreNetworkProxySettingsForAccount(*m_pAccount);

    m_pSystemTrayIconManager =
        new SystemTrayIconManager(*m_pAccountManager, this);

    QObject::connect(this,
                     QNSIGNAL(MainWindow,shown),
                     m_pSystemTrayIconManager,
                     QNSLOT(SystemTrayIconManager,onMainWindowShown));
    QObject::connect(this,
                     QNSIGNAL(MainWindow,hidden),
                     m_pSystemTrayIconManager,
                     QNSLOT(SystemTrayIconManager,onMainWindowHidden));

    m_pendingFirstShutdownDialog =
        m_pendingGreeterDialog &&
        m_pSystemTrayIconManager->isSystemTrayAvailable();

    setupThemeIcons();

    m_pUI->setupUi(this);
    setupAccountSpecificUiElements();

    if (m_nativeIconThemeName.isEmpty()) {
        m_pUI->ActionIconsNative->setVisible(false);
        m_pUI->ActionIconsNative->setDisabled(true);
    }

    setupGenericPanelStyleControllers();
    setupSidePanelStyleControllers();
    restorePanelColors();

    m_pAvailableAccountsActionGroup->setExclusive(true);
    m_pUI->searchQueryLineEdit->setClearButtonEnabled(true);

    setWindowTitleForAccount(*m_pAccount);

    setupLocalStorageManager();
    setupModels();
    setupViews();

    if (createdDefaultAccount) {
        setupDefaultAccount();
    }

    setupNoteEditorTabWidgetsCoordinator();

    setupShowHideStartupSettings();

    setupDefaultShortcuts();
    setupUserShortcuts();
    startListeningForShortcutChanges();

    addMenuActionsToMainWindow();

    if (m_pAccount->type() == Account::Type::Evernote) {
        setupSynchronizationManager(SetAccountOption::Set);
    }

    connectActionsToSlots();
    connectViewButtonsToSlots();
    connectNoteSearchActionsToSlots();
    connectToolbarButtonsToSlots();
    connectSystemTrayIconManagerSignalsToSlots();

    restoreGeometryAndState();
    startListeningForSplitterMoves();

    // this will emit event with initial thumbnail show/hide state
    onShowNoteThumbnailsPreferenceChanged();
}

MainWindow::~MainWindow()
{
    clearSynchronizationManager();

    if (m_pLocalStorageManagerThread) {
        m_pLocalStorageManagerThread->quit();
    }

    delete m_pUI;
}

void MainWindow::show()
{
    QNDEBUG("MainWindow::show");

    QWidget::show();

    m_shown = true;

    setupDisableNativeMenuBarPreference();

    if (!m_filtersViewExpanded) {
        adjustNoteListAndFiltersSplitterSizes();
    }

    restoreGeometryAndState();

    if (!m_geometryRestored) {
        setupInitialChildWidgetsWidths();
        persistGeometryAndState();
    }

    if (Q_UNLIKELY(m_pendingGreeterDialog))
    {
        m_pendingGreeterDialog = false;

        QScopedPointer<WelcomeToQuentierDialog> pDialog(
            new WelcomeToQuentierDialog(this));

        pDialog->setWindowModality(Qt::WindowModal);
        centerDialog(*pDialog);
        if (pDialog->exec() == QDialog::Accepted)
        {
            QNDEBUG("Log in to Evernote account option was "
                    << "chosen on the greeter screen");

            onEvernoteAccountAuthenticationRequested(
                pDialog->evernoteServer(),
                QNetworkProxy(QNetworkProxy::NoProxy));
        }
    }
}

const SystemTrayIconManager & MainWindow::systemTrayIconManager() const
{
    return *m_pSystemTrayIconManager;
}

void MainWindow::connectActionsToSlots()
{
    QNDEBUG("MainWindow::connectActionsToSlots");

    // File menu actions
    QObject::connect(m_pUI->ActionNewNote, QNSIGNAL(QAction,triggered),
                     this, QNSLOT(MainWindow,onNewNoteCreationRequested));
    QObject::connect(m_pUI->ActionNewNotebook, QNSIGNAL(QAction,triggered),
                     this, QNSLOT(MainWindow,onNewNotebookCreationRequested));
    QObject::connect(m_pUI->ActionNewTag, QNSIGNAL(QAction,triggered),
                     this, QNSLOT(MainWindow,onNewTagCreationRequested));
    QObject::connect(m_pUI->ActionNewSavedSearch, QNSIGNAL(QAction,triggered),
                     this, QNSLOT(MainWindow,onNewSavedSearchCreationRequested));
    QObject::connect(m_pUI->ActionImportENEX, QNSIGNAL(QAction,triggered),
                     this, QNSLOT(MainWindow,onImportEnexAction));
    QObject::connect(m_pUI->ActionPrint, QNSIGNAL(QAction,triggered),
                     this, QNSLOT(MainWindow,onCurrentNotePrintRequested));
    QObject::connect(m_pUI->ActionQuit, QNSIGNAL(QAction,triggered),
                     this, QNSLOT(MainWindow,onQuitAction));

    // Edit menu actions
    QObject::connect(m_pUI->ActionFindInsideNote, QNSIGNAL(QAction,triggered),
                     this, QNSLOT(MainWindow,onFindInsideNoteAction));
    QObject::connect(m_pUI->ActionFindNext, QNSIGNAL(QAction,triggered),
                     this, QNSLOT(MainWindow,onFindInsideNoteAction));
    QObject::connect(m_pUI->ActionFindPrevious, QNSIGNAL(QAction,triggered),
                     this, QNSLOT(MainWindow,onFindPreviousInsideNoteAction));
    QObject::connect(m_pUI->ActionReplaceInNote, QNSIGNAL(QAction,triggered),
                     this, QNSLOT(MainWindow,onReplaceInsideNoteAction));
    QObject::connect(m_pUI->ActionPreferences, QNSIGNAL(QAction,triggered),
                     this, QNSLOT(MainWindow,onShowPreferencesDialogAction));

    // Undo/redo actions
    QObject::connect(m_pUI->ActionUndo, QNSIGNAL(QAction,triggered),
                     this, QNSLOT(MainWindow,onUndoAction));
    QObject::connect(m_pUI->ActionRedo, QNSIGNAL(QAction,triggered),
                     this, QNSLOT(MainWindow,onRedoAction));
    // Copy/cut/paste actions
    QObject::connect(m_pUI->ActionCopy, QNSIGNAL(QAction,triggered),
                     this, QNSLOT(MainWindow,onCopyAction));
    QObject::connect(m_pUI->ActionCut, QNSIGNAL(QAction,triggered),
                     this, QNSLOT(MainWindow,onCutAction));
    QObject::connect(m_pUI->ActionPaste, QNSIGNAL(QAction,triggered),
                     this, QNSLOT(MainWindow,onPasteAction));
    // Select all action
    QObject::connect(m_pUI->ActionSelectAll, QNSIGNAL(QAction,triggered),
                     this, QNSLOT(MainWindow,onNoteTextSelectAllToggled));
    // Font actions
    QObject::connect(m_pUI->ActionFontBold, QNSIGNAL(QAction,triggered),
                     this, QNSLOT(MainWindow,onNoteTextBoldToggled));
    QObject::connect(m_pUI->ActionFontItalic, QNSIGNAL(QAction,triggered),
                     this, QNSLOT(MainWindow,onNoteTextItalicToggled));
    QObject::connect(m_pUI->ActionFontUnderlined, QNSIGNAL(QAction,triggered),
                     this, QNSLOT(MainWindow,onNoteTextUnderlineToggled));
    QObject::connect(m_pUI->ActionFontStrikethrough, QNSIGNAL(QAction,triggered),
                     this, QNSLOT(MainWindow,onNoteTextStrikethroughToggled));
    QObject::connect(m_pUI->ActionIncreaseFontSize, QNSIGNAL(QAction,triggered),
                     this, QNSLOT(MainWindow,onNoteTextIncreaseFontSizeAction));
    QObject::connect(m_pUI->ActionDecreaseFontSize, QNSIGNAL(QAction,triggered),
                     this, QNSLOT(MainWindow,onNoteTextDecreaseFontSizeAction));
    QObject::connect(m_pUI->ActionFontHighlight, QNSIGNAL(QAction,triggered),
                     this, QNSLOT(MainWindow,onNoteTextHighlightAction));
    // Spell checking
    QObject::connect(m_pUI->ActionSpellCheck, QNSIGNAL(QAction,triggered),
                     this, QNSLOT(MainWindow,onNoteTextSpellCheckToggled));
    // Text format actions
    QObject::connect(m_pUI->ActionAlignLeft, QNSIGNAL(QAction,triggered),
                     this, QNSLOT(MainWindow,onNoteTextAlignLeftAction));
    QObject::connect(m_pUI->ActionAlignCenter, QNSIGNAL(QAction,triggered),
                     this, QNSLOT(MainWindow,onNoteTextAlignCenterAction));
    QObject::connect(m_pUI->ActionAlignRight, QNSIGNAL(QAction,triggered),
                     this, QNSLOT(MainWindow,onNoteTextAlignRightAction));
    QObject::connect(m_pUI->ActionAlignFull, QNSIGNAL(QAction,triggered),
                     this, QNSLOT(MainWindow,onNoteTextAlignFullAction));
    QObject::connect(m_pUI->ActionInsertHorizontalLine, QNSIGNAL(QAction,triggered),
                     this, QNSLOT(MainWindow,onNoteTextAddHorizontalLineAction));
    QObject::connect(m_pUI->ActionIncreaseIndentation, QNSIGNAL(QAction,triggered),
                     this, QNSLOT(MainWindow,onNoteTextIncreaseIndentationAction));
    QObject::connect(m_pUI->ActionDecreaseIndentation, QNSIGNAL(QAction,triggered),
                     this, QNSLOT(MainWindow,onNoteTextDecreaseIndentationAction));
    QObject::connect(m_pUI->ActionInsertBulletedList, QNSIGNAL(QAction,triggered),
                     this, QNSLOT(MainWindow,onNoteTextInsertUnorderedListAction));
    QObject::connect(m_pUI->ActionInsertNumberedList, QNSIGNAL(QAction,triggered),
                     this, QNSLOT(MainWindow,onNoteTextInsertOrderedListAction));
    QObject::connect(m_pUI->ActionInsertToDo, QNSIGNAL(QAction,triggered),
                     this, QNSLOT(MainWindow,onNoteTextInsertToDoAction));
    QObject::connect(m_pUI->ActionInsertTable, QNSIGNAL(QAction,triggered),
                     this, QNSLOT(MainWindow,onNoteTextInsertTableDialogAction));
    QObject::connect(m_pUI->ActionEditHyperlink, QNSIGNAL(QAction,triggered),
                     this, QNSLOT(MainWindow,onNoteTextEditHyperlinkAction));
    QObject::connect(m_pUI->ActionCopyHyperlink, QNSIGNAL(QAction,triggered),
                     this, QNSLOT(MainWindow,onNoteTextCopyHyperlinkAction));
    QObject::connect(m_pUI->ActionRemoveHyperlink, QNSIGNAL(QAction,triggered),
                     this, QNSLOT(MainWindow,onNoteTextRemoveHyperlinkAction));
    QObject::connect(m_pUI->ActionSaveNote, QNSIGNAL(QAction,triggered),
                     this, QNSLOT(MainWindow,onSaveNoteAction));
    // Toggle view actions
    QObject::connect(m_pUI->ActionShowSidePanel, QNSIGNAL(QAction,toggled,bool),
                     this, QNSLOT(MainWindow,onShowSidePanelActionToggled,bool));
    QObject::connect(m_pUI->ActionShowFavorites, QNSIGNAL(QAction,toggled,bool),
                     this, QNSLOT(MainWindow,onShowFavoritesActionToggled,bool));
    QObject::connect(m_pUI->ActionShowNotebooks, QNSIGNAL(QAction,toggled,bool),
                     this, QNSLOT(MainWindow,onShowNotebooksActionToggled,bool));
    QObject::connect(m_pUI->ActionShowTags, QNSIGNAL(QAction,toggled,bool),
                     this, QNSLOT(MainWindow,onShowTagsActionToggled,bool));
    QObject::connect(m_pUI->ActionShowSavedSearches, QNSIGNAL(QAction,toggled,bool),
                     this, QNSLOT(MainWindow,onShowSavedSearchesActionToggled,bool));
    QObject::connect(m_pUI->ActionShowDeletedNotes, QNSIGNAL(QAction,toggled,bool),
                     this, QNSLOT(MainWindow,onShowDeletedNotesActionToggled,bool));
    QObject::connect(m_pUI->ActionShowNotesList, QNSIGNAL(QAction,toggled,bool),
                     this, QNSLOT(MainWindow,onShowNoteListActionToggled,bool));
    QObject::connect(m_pUI->ActionShowToolbar, QNSIGNAL(QAction,toggled,bool),
                     this, QNSLOT(MainWindow,onShowToolbarActionToggled,bool));
    QObject::connect(m_pUI->ActionShowStatusBar, QNSIGNAL(QAction,toggled,bool),
                     this, QNSLOT(MainWindow,onShowStatusBarActionToggled,bool));
    // Look and feel actions
    QObject::connect(m_pUI->ActionIconsNative, QNSIGNAL(QAction,triggered),
                     this, QNSLOT(MainWindow,onSwitchIconThemeToNativeAction));
    QObject::connect(m_pUI->ActionIconsOxygen, QNSIGNAL(QAction,triggered),
                     this, QNSLOT(MainWindow,onSwitchIconThemeToOxygenAction));
    QObject::connect(m_pUI->ActionIconsTango, QNSIGNAL(QAction,triggered),
                     this, QNSLOT(MainWindow,onSwitchIconThemeToTangoAction));
    QObject::connect(m_pUI->ActionIconsBreeze, QNSIGNAL(QAction,triggered),
                     this, QNSLOT(MainWindow,onSwitchIconThemeToBreezeAction));
    QObject::connect(m_pUI->ActionIconsBreezeDark, QNSIGNAL(QAction,triggered),
                     this, QNSLOT(MainWindow,onSwitchIconThemeToBreezeDarkAction));
    // Service menu actions
    QObject::connect(m_pUI->ActionSynchronize, QNSIGNAL(QAction,triggered),
                     this, QNSLOT(MainWindow,onSyncButtonPressed));
    // Help menu actions
    QObject::connect(m_pUI->ActionShowNoteSource, QNSIGNAL(QAction,triggered),
                     this, QNSLOT(MainWindow,onShowNoteSource));
    QObject::connect(m_pUI->ActionViewLogs, QNSIGNAL(QAction,triggered),
                     this, QNSLOT(MainWindow,onViewLogsActionTriggered));
    QObject::connect(m_pUI->ActionAbout, QNSIGNAL(QAction,triggered),
                     this,
                     QNSLOT(MainWindow,onShowInfoAboutQuentierActionTriggered));
}

void MainWindow::connectViewButtonsToSlots()
{
    QNDEBUG("MainWindow::connectViewButtonsToSlots");

    QObject::connect(m_pUI->addNotebookButton, QNSIGNAL(QPushButton,clicked),
                     this, QNSLOT(MainWindow,onNewNotebookCreationRequested));
    QObject::connect(m_pUI->removeNotebookButton, QNSIGNAL(QPushButton,clicked),
                     this, QNSLOT(MainWindow,onRemoveNotebookButtonPressed));
    QObject::connect(m_pUI->notebookInfoButton, QNSIGNAL(QPushButton,clicked),
                     this, QNSLOT(MainWindow,onNotebookInfoButtonPressed));

    QObject::connect(m_pUI->addTagButton, QNSIGNAL(QPushButton,clicked),
                     this, QNSLOT(MainWindow,onNewTagCreationRequested));
    QObject::connect(m_pUI->removeTagButton, QNSIGNAL(QPushButton,clicked),
                     this, QNSLOT(MainWindow,onRemoveTagButtonPressed));
    QObject::connect(m_pUI->tagInfoButton, QNSIGNAL(QPushButton,clicked),
                     this, QNSLOT(MainWindow,onTagInfoButtonPressed));

    QObject::connect(m_pUI->addSavedSearchButton, QNSIGNAL(QPushButton,clicked),
                     this, QNSLOT(MainWindow,onNewSavedSearchCreationRequested));
    QObject::connect(m_pUI->removeSavedSearchButton, QNSIGNAL(QPushButton,clicked),
                     this, QNSLOT(MainWindow,onRemoveSavedSearchButtonPressed));
    QObject::connect(m_pUI->savedSearchInfoButton, QNSIGNAL(QPushButton,clicked),
                     this, QNSLOT(MainWindow,onSavedSearchInfoButtonPressed));

    QObject::connect(m_pUI->unfavoritePushButton, QNSIGNAL(QPushButton,clicked),
                     this, QNSLOT(MainWindow,onUnfavoriteItemButtonPressed));
    QObject::connect(m_pUI->favoriteInfoButton, QNSIGNAL(QPushButton,clicked),
                     this, QNSLOT(MainWindow,onFavoritedItemInfoButtonPressed));

    QObject::connect(m_pUI->restoreDeletedNoteButton, QNSIGNAL(QPushButton,clicked),
                     this, QNSLOT(MainWindow,onRestoreDeletedNoteButtonPressed));
    QObject::connect(m_pUI->eraseDeletedNoteButton, QNSIGNAL(QPushButton,clicked),
                     this, QNSLOT(MainWindow,onDeleteNotePermanentlyButtonPressed));
    QObject::connect(m_pUI->deletedNoteInfoButton, QNSIGNAL(QPushButton,clicked),
                     this, QNSLOT(MainWindow,onDeletedNoteInfoButtonPressed));

    QObject::connect(m_pUI->filtersViewTogglePushButton,
                     QNSIGNAL(QPushButton,clicked),
                     this,
                     QNSLOT(MainWindow,onFiltersViewTogglePushButtonPressed));
}

void MainWindow::connectNoteSearchActionsToSlots()
{
    QNDEBUG("MainWindow::connectNoteSearchActionsToSlots");

    QObject::connect(m_pUI->saveSearchPushButton, QNSIGNAL(QPushButton,clicked),
                     this, QNSLOT(MainWindow,onSaveNoteSearchQueryButtonPressed));
}

void MainWindow::connectToolbarButtonsToSlots()
{
    QNDEBUG("MainWindow::connectToolbarButtonsToSlots");

    QObject::connect(m_pUI->addNotePushButton, QNSIGNAL(QPushButton,clicked),
                     this, QNSLOT(MainWindow,onNewNoteCreationRequested));
    QObject::connect(m_pUI->deleteNotePushButton, QNSIGNAL(QPushButton,clicked),
                     this, QNSLOT(MainWindow,onDeleteCurrentNoteButtonPressed));
    QObject::connect(m_pUI->infoButton, QNSIGNAL(QPushButton,clicked),
                     this, QNSLOT(MainWindow,onCurrentNoteInfoRequested));
    QObject::connect(m_pUI->printNotePushButton, QNSIGNAL(QPushButton,clicked),
                     this, QNSLOT(MainWindow,onCurrentNotePrintRequested));
    QObject::connect(m_pUI->exportNoteToPdfPushButton, QNSIGNAL(QPushButton,clicked),
                     this, QNSLOT(MainWindow,onCurrentNotePdfExportRequested));
    QObject::connect(m_pUI->syncPushButton, QNSIGNAL(QPushButton,clicked),
                     this, QNSLOT(MainWindow,onSyncButtonPressed));
}

void MainWindow::connectSystemTrayIconManagerSignalsToSlots()
{
    QNDEBUG("MainWindow::connectSystemTrayIconManagerSignalsToSlots");

    QObject::connect(m_pSystemTrayIconManager,
                     QNSIGNAL(SystemTrayIconManager,notifyError,ErrorString),
                     this,
                     QNSLOT(MainWindow,onSystemTrayIconManagerError,ErrorString));
    QObject::connect(m_pSystemTrayIconManager,
                     QNSIGNAL(SystemTrayIconManager,newTextNoteAdditionRequested),
                     this,
                     QNSLOT(MainWindow,onNewNoteRequestedFromSystemTrayIcon));
    QObject::connect(m_pSystemTrayIconManager,
                     QNSIGNAL(SystemTrayIconManager,quitRequested),
                     this,
                     QNSLOT(MainWindow,onQuitRequestedFromSystemTrayIcon));
    QObject::connect(m_pSystemTrayIconManager,
                     QNSIGNAL(SystemTrayIconManager,accountSwitchRequested,Account),
                     this,
                     QNSLOT(MainWindow,onAccountSwitchRequested,Account));
    QObject::connect(m_pSystemTrayIconManager,
                     QNSIGNAL(SystemTrayIconManager,showRequested),
                     this,
                     QNSLOT(MainWindow,onShowRequestedFromTrayIcon));
    QObject::connect(m_pSystemTrayIconManager,
                     QNSIGNAL(SystemTrayIconManager,hideRequested),
                     this,
                     QNSLOT(MainWindow,onHideRequestedFromTrayIcon));
}

void MainWindow::connectToPreferencesDialogSignals(PreferencesDialog & dialog)
{
    QObject::connect(
        &dialog,
        &PreferencesDialog::noteEditorUseLimitedFontsOptionChanged,
        this,
        &MainWindow::onUseLimitedFontsPreferenceChanged);

    QObject::connect(
        &dialog,
        &PreferencesDialog::iconThemeChanged,
        this,
        &MainWindow::onSwitchIconTheme);

    QObject::connect(
        &dialog,
        &PreferencesDialog::synchronizationDownloadNoteThumbnailsOptionChanged,
        this,
        &MainWindow::synchronizationDownloadNoteThumbnailsOptionChanged);

    QObject::connect(
        &dialog,
        &PreferencesDialog::synchronizationDownloadInkNoteImagesOptionChanged,
        this,
        &MainWindow::synchronizationDownloadInkNoteImagesOptionChanged);

    QObject::connect(
        &dialog,
        &PreferencesDialog::showNoteThumbnailsOptionChanged,
        this,
        &MainWindow::onShowNoteThumbnailsPreferenceChanged);

    QObject::connect(
        &dialog,
        &PreferencesDialog::disableNativeMenuBarOptionChanged,
        this,
        &MainWindow::onDisableNativeMenuBarPreferenceChanged);

    QObject::connect(
        &dialog,
        &PreferencesDialog::runSyncPeriodicallyOptionChanged,
        this,
        &MainWindow::onRunSyncEachNumMinitesPreferenceChanged);

    QObject::connect(
        &dialog,
        &PreferencesDialog::noteEditorFontColorChanged,
        m_pNoteEditorTabsAndWindowsCoordinator,
        &NoteEditorTabsAndWindowsCoordinator::noteEditorFontColorChanged);

    QObject::connect(
        &dialog,
        &PreferencesDialog::noteEditorBackgroundColorChanged,
        m_pNoteEditorTabsAndWindowsCoordinator,
        &NoteEditorTabsAndWindowsCoordinator::noteEditorBackgroundColorChanged);

    QObject::connect(
        &dialog,
        &PreferencesDialog::noteEditorHighlightColorChanged,
        m_pNoteEditorTabsAndWindowsCoordinator,
        &NoteEditorTabsAndWindowsCoordinator::noteEditorHighlightColorChanged);

    QObject::connect(
        &dialog,
        &PreferencesDialog::noteEditorHighlightedTextColorChanged,
        m_pNoteEditorTabsAndWindowsCoordinator,
        &NoteEditorTabsAndWindowsCoordinator::noteEditorHighlightedTextColorChanged);

    QObject::connect(
        &dialog,
        &PreferencesDialog::noteEditorColorsReset,
        m_pNoteEditorTabsAndWindowsCoordinator,
        &NoteEditorTabsAndWindowsCoordinator::noteEditorColorsReset);

    QObject::connect(
        &dialog,
        &PreferencesDialog::panelFontColorChanged,
        this,
        &MainWindow::onPanelFontColorChanged);

    QObject::connect(
        &dialog,
        &PreferencesDialog::panelBackgroundColorChanged,
        this,
        &MainWindow::onPanelBackgroundColorChanged);

    QObject::connect(
        &dialog,
        &PreferencesDialog::panelUseBackgroundGradientSettingChanged,
        this,
        &MainWindow::onPanelUseBackgroundGradientSettingChanged);

    QObject::connect(
        &dialog,
        &PreferencesDialog::panelBackgroundLinearGradientChanged,
        this,
        &MainWindow::onPanelBackgroundLinearGradientChanged);
}

void MainWindow::addMenuActionsToMainWindow()
{
    QNDEBUG("MainWindow::addMenuActionsToMainWindow");

    // NOTE: adding the actions from the menu bar's menus is required for getting
    // the shortcuts of these actions to work properly; action shortcuts only
    // fire when the menu is shown which is not really the purpose behind those
    // shortcuts
    QList<QMenu*> menus = m_pUI->menuBar->findChildren<QMenu*>();
    const int numMenus = menus.size();
    for(int i = 0; i < numMenus; ++i)
    {
        QMenu * menu = menus[i];

        QList<QAction*> actions = menu->actions();
        const int numActions = actions.size();
        for(int j = 0; j < numActions; ++j) {
            QAction * action = actions[j];
            addAction(action);
        }
    }

    m_pAvailableAccountsSubMenu = new QMenu(tr("Switch account"));
    QAction * separatorAction =
        m_pUI->menuFile->insertSeparator(m_pUI->ActionQuit);

    QAction * switchAccountSubMenuAction =
        m_pUI->menuFile->insertMenu(separatorAction, m_pAvailableAccountsSubMenu);
    Q_UNUSED(m_pUI->menuFile->insertSeparator(switchAccountSubMenuAction));

    updateSubMenuWithAvailableAccounts();
}

void MainWindow::updateSubMenuWithAvailableAccounts()
{
    QNDEBUG("MainWindow::updateSubMenuWithAvailableAccounts");

    if (Q_UNLIKELY(!m_pAvailableAccountsSubMenu)) {
        QNDEBUG("No available accounts sub-menu");
        return;
    }

    delete m_pAvailableAccountsActionGroup;
    m_pAvailableAccountsActionGroup = new QActionGroup(this);
    m_pAvailableAccountsActionGroup->setExclusive(true);

    m_pAvailableAccountsSubMenu->clear();

    const QVector<Account> & availableAccounts =
        m_pAccountManager->availableAccounts();

    int numAvailableAccounts = availableAccounts.size();
    for(int i = 0; i < numAvailableAccounts; ++i)
    {
        const Account & availableAccount = availableAccounts[i];
        QNTRACE("Examining the available account: " << availableAccount);

        QString availableAccountRepresentationName = availableAccount.name();
        if (availableAccount.type() == Account::Type::Local)
        {
            availableAccountRepresentationName += QStringLiteral(" (");
            availableAccountRepresentationName += tr("local");
            availableAccountRepresentationName += QStringLiteral(")");
        }
        else if (availableAccount.type() == Account::Type::Evernote)
        {
            QString host = availableAccount.evernoteHost();
            if (host != QStringLiteral("www.evernote.com"))
            {
                availableAccountRepresentationName += QStringLiteral(" (");

                if (host == QStringLiteral("sandbox.evernote.com"))
                {
                    availableAccountRepresentationName +=
                        QStringLiteral("sandbox");
                }
                else if (host == QStringLiteral("app.yinxiang.com"))
                {
                    availableAccountRepresentationName +=
                        QStringLiteral("Yinxiang Biji");
                }
                else
                {
                    availableAccountRepresentationName += host;
                }

                availableAccountRepresentationName += QStringLiteral(")");
            }
        }

        QAction * pAccountAction =
            new QAction(availableAccountRepresentationName, nullptr);
        m_pAvailableAccountsSubMenu->addAction(pAccountAction);

        pAccountAction->setData(i);
        pAccountAction->setCheckable(true);

        if (!m_pAccount.isNull() && (*m_pAccount == availableAccount)) {
            pAccountAction->setChecked(true);
        }

        addAction(pAccountAction);
        QObject::connect(pAccountAction,
                         QNSIGNAL(QAction,toggled,bool),
                         this,
                         QNSLOT(MainWindow,onSwitchAccountActionToggled,bool));

        m_pAvailableAccountsActionGroup->addAction(pAccountAction);
    }

    if (Q_LIKELY(numAvailableAccounts != 0)) {
        Q_UNUSED(m_pAvailableAccountsSubMenu->addSeparator())
    }

    QAction * pAddAccountAction =
        m_pAvailableAccountsSubMenu->addAction(tr("Add account"));
    addAction(pAddAccountAction);
    QObject::connect(pAddAccountAction, QNSIGNAL(QAction,triggered,bool),
                     this, QNSLOT(MainWindow,onAddAccountActionTriggered,bool));

    QAction * pManageAccountsAction =
        m_pAvailableAccountsSubMenu->addAction(tr("Manage accounts"));
    addAction(pManageAccountsAction);
    QObject::connect(pManageAccountsAction, QNSIGNAL(QAction,triggered,bool),
                     this,
                     QNSLOT(MainWindow,onManageAccountsActionTriggered,bool));
}

void MainWindow::setupInitialChildWidgetsWidths()
{
    QNDEBUG("MainWindow::setupInitialChildWidgetsWidths");

    int totalWidth = width();

    // 1/3 - for side view, 2/3 - for note list view, 3/3 - for the note editor
    int partWidth = totalWidth / 5;

    QNTRACE("Total width = " << totalWidth
            << ", part width = " << partWidth);

    QList<int> splitterSizes = m_pUI->splitter->sizes();
    int splitterSizesCount = splitterSizes.count();
    if (Q_UNLIKELY(splitterSizesCount != 3))
    {
        ErrorString error(QT_TR_NOOP("Internal error: can't setup the proper "
                                     "initial widths for side panel, note list "
                                     "view and note editors view: wrong number "
                                     "of sizes within the splitter"));
        QNWARNING(error << ", sizes count: " << splitterSizesCount);
        onSetStatusBarText(error.localizedString(), SEC_TO_MSEC(30));
        return;
    }

    splitterSizes[0] = partWidth;
    splitterSizes[1] = partWidth;
    splitterSizes[2] = totalWidth - 2 * partWidth;

    m_pUI->splitter->setSizes(splitterSizes);
}

void MainWindow::setWindowTitleForAccount(const Account & account)
{
    QNDEBUG("MainWindow::setWindowTitleForAccount: " << account.name());

    bool nonStandardPersistencePath = false;
    Q_UNUSED(applicationPersistentStoragePath(&nonStandardPersistencePath))

    QString username = account.name();
    QString displayName = account.displayName();

    QString title = qApp->applicationName() + QStringLiteral(": ");
    if (!displayName.isEmpty())
    {
        title += displayName;
        title += QStringLiteral(" (");
        title += username;
        if (nonStandardPersistencePath)
        {
            title += QStringLiteral(", ");
            title += QDir::toNativeSeparators(accountPersistentStoragePath(account));
        }
        title += QStringLiteral(")");
    }
    else
    {
        title += username;

        if (nonStandardPersistencePath) {
            title += QStringLiteral(" (");
            title += QDir::toNativeSeparators(accountPersistentStoragePath(account));
            title += QStringLiteral(")");
        }
    }

    setWindowTitle(title);
}

NoteEditorWidget * MainWindow::currentNoteEditorTab()
{
    QNDEBUG("MainWindow::currentNoteEditorTab");

    if (Q_UNLIKELY(m_pUI->noteEditorsTabWidget->count() == 0)) {
        QNTRACE("No open note editors");
        return nullptr;
    }

    int currentIndex = m_pUI->noteEditorsTabWidget->currentIndex();
    if (Q_UNLIKELY(currentIndex < 0)) {
        QNTRACE("No current note editor");
        return nullptr;
    }

    QWidget * currentWidget = m_pUI->noteEditorsTabWidget->widget(currentIndex);
    if (Q_UNLIKELY(!currentWidget)) {
        QNTRACE("No current widget");
        return nullptr;
    }

    NoteEditorWidget * noteEditorWidget =
        qobject_cast<NoteEditorWidget*>(currentWidget);
    if (Q_UNLIKELY(!noteEditorWidget)) {
        QNWARNING("Can't cast current note tag widget's widget "
                  "to note editor widget");
        return nullptr;
    }

    return noteEditorWidget;
}

void MainWindow::createNewNote(
    NoteEditorTabsAndWindowsCoordinator::NoteEditorMode::type noteEditorMode)
{
    QNDEBUG("MainWindow::createNewNote: note editor mode = " << noteEditorMode);

    if (Q_UNLIKELY(!m_pNoteEditorTabsAndWindowsCoordinator)) {
        QNDEBUG("No note editor tabs and windows coordinator, "
                "probably the button was pressed too quickly on "
                "startup, skipping");
        return;
    }

    if (Q_UNLIKELY(!m_pNoteModel)) {
        Q_UNUSED(internalErrorMessageBox(this, tr("Can't create a new note: note "
                                                  "model is unexpectedly null")))
        return;
    }

    if (Q_UNLIKELY(!m_pNotebookModel)) {
        Q_UNUSED(internalErrorMessageBox(this, tr("Can't create a new note: "
                                                  "notebook model is unexpectedly "
                                                  "null")))
        return;
    }

    QModelIndex currentNotebookIndex =
        m_pUI->notebooksTreeView->currentlySelectedItemIndex();
    if (Q_UNLIKELY(!currentNotebookIndex.isValid())) {
        Q_UNUSED(informationMessageBox(this, tr("No notebook is selected"),
                                       tr("Please select the notebook in which "
                                          "you want to create the note; if you "
                                          "don't have any notebooks yet, create "
                                          "one")))
        return;
    }

    const NotebookModelItem * pNotebookModelItem =
        m_pNotebookModel->itemForIndex(currentNotebookIndex);
    if (Q_UNLIKELY(!pNotebookModelItem)) {
        Q_UNUSED(internalErrorMessageBox(this, tr("Can't create a new note: can't "
                                                  "find the notebook model item "
                                                  "corresponding to the currently "
                                                  "selected notebook")))
        return;
    }

    if (Q_UNLIKELY(pNotebookModelItem->type() != NotebookModelItem::Type::Notebook))
    {
        Q_UNUSED(informationMessageBox(this, tr("No notebook is selected"),
                                       tr("Please select the notebook in which "
                                          "you want to create the note (currently "
                                          "the notebook stack seems to be selected)")))
        return;
    }

    const NotebookItem * pNotebookItem = pNotebookModelItem->notebookItem();
    if (Q_UNLIKELY(!pNotebookItem))
    {
        Q_UNUSED(internalErrorMessageBox(this, tr("Can't create a new note: "
                                                  "the notebook model item has "
                                                  "notebook type but null pointer "
                                                  "to the actual notebook item")))
        return;
    }

    m_pNoteEditorTabsAndWindowsCoordinator->createNewNote(pNotebookItem->localUid(),
                                                          pNotebookItem->guid(),
                                                          noteEditorMode);
}

void MainWindow::connectSynchronizationManager()
{
    QNDEBUG("MainWindow::connectSynchronizationManager");

    if (Q_UNLIKELY(!m_pSynchronizationManager)) {
        QNDEBUG("No synchronization manager");
        return;
    }

    // Connect local signals to SynchronizationManager slots
    QObject::connect(this, QNSIGNAL(MainWindow,authenticate),
                     m_pSynchronizationManager,
                     QNSLOT(SynchronizationManager,authenticate),
                     Qt::ConnectionType(Qt::QueuedConnection | Qt::UniqueConnection));
    QObject::connect(this, QNSIGNAL(MainWindow,authenticateCurrentAccount),
                     m_pSynchronizationManager,
                     QNSLOT(SynchronizationManager,authenticateCurrentAccount),
                     Qt::ConnectionType(Qt::QueuedConnection | Qt::UniqueConnection));
    QObject::connect(this, QNSIGNAL(MainWindow,revokeAuthentication,qevercloud::UserID),
                     m_pSynchronizationManager,
                     QNSLOT(SynchronizationManager,revokeAuthentication,qevercloud::UserID));
    QObject::connect(this, QNSIGNAL(MainWindow,synchronize),
                     m_pSynchronizationManager,
                     QNSLOT(SynchronizationManager,synchronize),
                     Qt::ConnectionType(Qt::QueuedConnection | Qt::UniqueConnection));
    QObject::connect(this, QNSIGNAL(MainWindow,stopSynchronization),
                     m_pSynchronizationManager,
                     QNSLOT(SynchronizationManager,stop),
                     Qt::ConnectionType(Qt::QueuedConnection | Qt::UniqueConnection));

    // Connect SynchronizationManager signals to local slots
    QObject::connect(m_pSynchronizationManager,
                     QNSIGNAL(SynchronizationManager,started),
                     this, QNSLOT(MainWindow,onSynchronizationStarted),
                     Qt::ConnectionType(Qt::QueuedConnection | Qt::UniqueConnection));
    QObject::connect(m_pSynchronizationManager,
                     QNSIGNAL(SynchronizationManager,stopped),
                     this, QNSLOT(MainWindow,onSynchronizationStopped),
                     Qt::ConnectionType(Qt::QueuedConnection | Qt::UniqueConnection));
    QObject::connect(m_pSynchronizationManager,
                     QNSIGNAL(SynchronizationManager,failed,ErrorString),
                     this, QNSLOT(MainWindow,onSynchronizationManagerFailure,ErrorString),
                     Qt::ConnectionType(Qt::QueuedConnection | Qt::UniqueConnection));
    QObject::connect(m_pSynchronizationManager,
                     QNSIGNAL(SynchronizationManager,finished,Account,bool,bool),
                     this,
                     QNSLOT(MainWindow,onSynchronizationFinished,Account,bool,bool),
                     Qt::ConnectionType(Qt::QueuedConnection | Qt::UniqueConnection));
    QObject::connect(m_pSynchronizationManager,
                     QNSIGNAL(SynchronizationManager,authenticationFinished,
                              bool,ErrorString,Account),
                     this,
                     QNSLOT(MainWindow,onAuthenticationFinished,
                            bool,ErrorString,Account),
                     Qt::ConnectionType(Qt::QueuedConnection | Qt::UniqueConnection));
    QObject::connect(m_pSynchronizationManager,
                     QNSIGNAL(SynchronizationManager,authenticationRevoked,
                              bool,ErrorString,qevercloud::UserID),
                     this,
                     QNSLOT(MainWindow,onAuthenticationRevoked,
                            bool,ErrorString,qevercloud::UserID),
                     Qt::ConnectionType(Qt::QueuedConnection | Qt::UniqueConnection));
    QObject::connect(m_pSynchronizationManager,
                     QNSIGNAL(SynchronizationManager,remoteToLocalSyncStopped),
                     this,
                     QNSLOT(MainWindow,onRemoteToLocalSyncStopped),
                     Qt::ConnectionType(Qt::QueuedConnection | Qt::UniqueConnection));
    QObject::connect(m_pSynchronizationManager,
                     QNSIGNAL(SynchronizationManager,sendLocalChangesStopped),
                     this,
                     QNSLOT(MainWindow,onSendLocalChangesStopped),
                     Qt::ConnectionType(Qt::QueuedConnection | Qt::UniqueConnection));
    QObject::connect(m_pSynchronizationManager,
                     QNSIGNAL(SynchronizationManager,rateLimitExceeded,qint32),
                     this,
                     QNSLOT(MainWindow,onRateLimitExceeded,qint32),
                     Qt::ConnectionType(Qt::QueuedConnection | Qt::UniqueConnection));
    QObject::connect(m_pSynchronizationManager,
                     QNSIGNAL(SynchronizationManager,remoteToLocalSyncDone,bool),
                     this,
                     QNSLOT(MainWindow,onRemoteToLocalSyncDone,bool),
                     Qt::ConnectionType(Qt::QueuedConnection | Qt::UniqueConnection));
    QObject::connect(m_pSynchronizationManager,
                     QNSIGNAL(SynchronizationManager,syncChunksDownloadProgress,
                              qint32,qint32,qint32),
                     this,
                     QNSLOT(MainWindow,onSyncChunksDownloadProgress,
                            qint32,qint32,qint32),
                     Qt::ConnectionType(Qt::QueuedConnection | Qt::UniqueConnection));
    QObject::connect(m_pSynchronizationManager,
                     QNSIGNAL(SynchronizationManager,syncChunksDownloaded),
                     this,
                     QNSLOT(MainWindow,onSyncChunksDownloaded),
                     Qt::ConnectionType(Qt::QueuedConnection | Qt::UniqueConnection));
    QObject::connect(m_pSynchronizationManager,
                     QNSIGNAL(SynchronizationManager,notesDownloadProgress,
                              quint32,quint32),
                     this,
                     QNSLOT(MainWindow,onNotesDownloadProgress,quint32,quint32),
                     Qt::ConnectionType(Qt::QueuedConnection | Qt::UniqueConnection));
    QObject::connect(m_pSynchronizationManager,
                     QNSIGNAL(SynchronizationManager,resourcesDownloadProgress,
                              quint32,quint32),
                     this,
                     QNSLOT(MainWindow,onResourcesDownloadProgress,
                            quint32,quint32),
                     Qt::ConnectionType(Qt::QueuedConnection | Qt::UniqueConnection));
    QObject::connect(m_pSynchronizationManager,
                     QNSIGNAL(SynchronizationManager,
                              linkedNotebookSyncChunksDownloadProgress,
                              qint32,qint32,qint32,LinkedNotebook),
                     this,
                     QNSLOT(MainWindow,onLinkedNotebookSyncChunksDownloadProgress,
                            qint32,qint32,qint32,LinkedNotebook),
                     Qt::ConnectionType(Qt::QueuedConnection | Qt::UniqueConnection));
    QObject::connect(m_pSynchronizationManager,
                     QNSIGNAL(SynchronizationManager,
                              linkedNotebooksSyncChunksDownloaded),
                     this,
                     QNSLOT(MainWindow,onLinkedNotebooksSyncChunksDownloaded),
                     Qt::ConnectionType(Qt::QueuedConnection | Qt::UniqueConnection));
    QObject::connect(m_pSynchronizationManager,
                     QNSIGNAL(SynchronizationManager,
                              linkedNotebooksNotesDownloadProgress,
                              quint32,quint32),
                     this,
                     QNSLOT(MainWindow,onLinkedNotebooksNotesDownloadProgress,
                            quint32,quint32),
                     Qt::ConnectionType(Qt::QueuedConnection | Qt::UniqueConnection));
}

void MainWindow::disconnectSynchronizationManager()
{
    QNDEBUG("MainWindow::disconnectSynchronizationManager");

    if (Q_UNLIKELY(!m_pSynchronizationManager)) {
        QNDEBUG("No synchronization manager");
        return;
    }

    // Disconnect local signals from SynchronizationManager slots
    QObject::disconnect(this,
                        QNSIGNAL(MainWindow,authenticate),
                        m_pSynchronizationManager,
                        QNSLOT(SynchronizationManager,authenticate));
    QObject::disconnect(this,
                        QNSIGNAL(MainWindow,authenticateCurrentAccount),
                        m_pSynchronizationManager,
                        QNSLOT(SynchronizationManager,authenticateCurrentAccount));
    QObject::disconnect(this,
                        QNSIGNAL(MainWindow,synchronize),
                        m_pSynchronizationManager,
                        QNSLOT(SynchronizationManager,synchronize));
    QObject::disconnect(this,
                        QNSIGNAL(MainWindow,stopSynchronization),
                        m_pSynchronizationManager,
                        QNSLOT(SynchronizationManager,stop));

    // Disconnect SynchronizationManager signals from local slots
    QObject::disconnect(m_pSynchronizationManager,
                        QNSIGNAL(SynchronizationManager,started),
                        this,
                        QNSLOT(MainWindow,onSynchronizationStarted));
    QObject::disconnect(m_pSynchronizationManager,
                        QNSIGNAL(SynchronizationManager,stopped),
                        this,
                        QNSLOT(MainWindow,onSynchronizationStopped));
    QObject::disconnect(m_pSynchronizationManager,
                        QNSIGNAL(SynchronizationManager,failed,ErrorString),
                        this,
                        QNSLOT(MainWindow,onSynchronizationManagerFailure,
                               ErrorString));
    QObject::disconnect(m_pSynchronizationManager,
                        QNSIGNAL(SynchronizationManager,finished,
                                 Account,bool,bool),
                        this,
                        QNSLOT(MainWindow,onSynchronizationFinished,
                               Account,bool,bool));
    QObject::disconnect(m_pSynchronizationManager,
                        QNSIGNAL(SynchronizationManager,authenticationFinished,
                                 bool,ErrorString,Account),
                        this,
                        QNSLOT(MainWindow,onAuthenticationFinished,
                               bool,ErrorString,Account));
    QObject::disconnect(m_pSynchronizationManager,
                        QNSIGNAL(SynchronizationManager,authenticationRevoked,
                                 bool,ErrorString,qevercloud::UserID),
                        this,
                        QNSLOT(MainWindow,onAuthenticationRevoked,
                               bool,ErrorString,qevercloud::UserID));
    QObject::disconnect(m_pSynchronizationManager,
                        QNSIGNAL(SynchronizationManager,remoteToLocalSyncStopped),
                        this,
                        QNSLOT(MainWindow,onRemoteToLocalSyncStopped));
    QObject::disconnect(m_pSynchronizationManager,
                        QNSIGNAL(SynchronizationManager,sendLocalChangesStopped),
                        this,
                        QNSLOT(MainWindow,onSendLocalChangesStopped));
    QObject::disconnect(m_pSynchronizationManager,
                        QNSIGNAL(SynchronizationManager,rateLimitExceeded,qint32),
                        this,
                        QNSLOT(MainWindow,onRateLimitExceeded,qint32));
    QObject::disconnect(m_pSynchronizationManager,
                        QNSIGNAL(SynchronizationManager,remoteToLocalSyncDone,
                                 bool),
                        this,
                        QNSLOT(MainWindow,onRemoteToLocalSyncDone,bool));
    QObject::disconnect(m_pSynchronizationManager,
                        QNSIGNAL(SynchronizationManager,
                                 syncChunksDownloadProgress,qint32,qint32,qint32),
                        this,
                        QNSLOT(MainWindow,onSyncChunksDownloadProgress,
                               qint32,qint32,qint32));
    QObject::disconnect(m_pSynchronizationManager,
                        QNSIGNAL(SynchronizationManager,syncChunksDownloaded),
                        this,
                        QNSLOT(MainWindow,onSyncChunksDownloaded));
    QObject::disconnect(m_pSynchronizationManager,
                        QNSIGNAL(SynchronizationManager,notesDownloadProgress,
                                 quint32,quint32),
                        this,
                        QNSLOT(MainWindow,onNotesDownloadProgress,
                               quint32,quint32));
    QObject::disconnect(m_pSynchronizationManager,
                        QNSIGNAL(SynchronizationManager,
                                 resourcesDownloadProgress,quint32,quint32),
                        this,
                        QNSLOT(MainWindow,onResourcesDownloadProgress,
                               quint32,quint32));
    QObject::disconnect(m_pSynchronizationManager,
                        QNSIGNAL(SynchronizationManager,
                                 linkedNotebookSyncChunksDownloadProgress,
                                 qint32,qint32,qint32,LinkedNotebook),
                        this,
                        QNSLOT(MainWindow,
                               onLinkedNotebookSyncChunksDownloadProgress,
                               qint32,qint32,qint32,LinkedNotebook));
    QObject::disconnect(m_pSynchronizationManager,
                        QNSIGNAL(SynchronizationManager,
                                 linkedNotebooksSyncChunksDownloaded),
                        this,
                        QNSLOT(MainWindow,onLinkedNotebooksSyncChunksDownloaded));
    QObject::disconnect(m_pSynchronizationManager,
                        QNSIGNAL(SynchronizationManager,
                                 linkedNotebooksNotesDownloadProgress,
                                 quint32,quint32),
                        this,
                        QNSLOT(MainWindow,onLinkedNotebooksNotesDownloadProgress,
                               quint32,quint32));
}

void MainWindow::startSyncButtonAnimation()
{
    QNDEBUG("MainWindow::startSyncButtonAnimation");

    QObject::disconnect(&m_animatedSyncButtonIcon,
                        QNSIGNAL(QMovie,frameChanged,int),
                        this,
                        QNSLOT(MainWindow,
                               onAnimatedSyncIconFrameChangedPendingFinish,int));

    QObject::connect(&m_animatedSyncButtonIcon,
                     QNSIGNAL(QMovie,frameChanged,int),
                     this,
                     QNSLOT(MainWindow,onAnimatedSyncIconFrameChanged,int));

    // If the animation doesn't run forever, make it so
    if (m_animatedSyncButtonIcon.loopCount() != -1) {
        QObject::connect(&m_animatedSyncButtonIcon, QNSIGNAL(QMovie,finished),
                         &m_animatedSyncButtonIcon, QNSLOT(QMovie,start));
    }

    m_animatedSyncButtonIcon.start();
}

void MainWindow::stopSyncButtonAnimation()
{
    QNDEBUG("MainWindow::stopSyncButtonAnimation");

    if (m_animatedSyncButtonIcon.loopCount() != -1) {
        QObject::disconnect(&m_animatedSyncButtonIcon, QNSIGNAL(QMovie,finished),
                            &m_animatedSyncButtonIcon, QNSLOT(QMovie,start));
    }

    QObject::disconnect(&m_animatedSyncButtonIcon, QNSIGNAL(QMovie,finished),
                        this, QNSLOT(MainWindow,onSyncIconAnimationFinished));
    QObject::disconnect(&m_animatedSyncButtonIcon,
                        QNSIGNAL(QMovie,frameChanged,int),
                        this,
                        QNSLOT(MainWindow,onAnimatedSyncIconFrameChanged,int));

    m_animatedSyncButtonIcon.stop();
    m_pUI->syncPushButton->setIcon(QIcon(QStringLiteral(":/sync/sync.png")));
}

void MainWindow::scheduleSyncButtonAnimationStop()
{
    QNDEBUG("MainWindow::scheduleSyncButtonAnimationStop");

    if (m_animatedSyncButtonIcon.state() != QMovie::Running) {
        stopSyncButtonAnimation();
        return;
    }

    // NOTE: either finished signal should stop the sync animation or,
    // if the movie is naturally looped, the frame count coming to max value
    // (or zero after max value) would serve as a sign that full animation loop
    // has finished
    QObject::connect(&m_animatedSyncButtonIcon, QNSIGNAL(QMovie,finished),
                     this, QNSLOT(MainWindow,onSyncIconAnimationFinished),
                     Qt::ConnectionType(Qt::UniqueConnection | Qt::QueuedConnection));
    QObject::connect(&m_animatedSyncButtonIcon,
                     QNSIGNAL(QMovie,frameChanged,int),
                     this,
                     QNSLOT(MainWindow,
                            onAnimatedSyncIconFrameChangedPendingFinish,int),
                     Qt::ConnectionType(Qt::UniqueConnection | Qt::QueuedConnection));
}

void MainWindow::startListeningForSplitterMoves()
{
    QNDEBUG("MainWindow::startListeningForSplitterMoves");

    QObject::connect(m_pUI->splitter,
                     QNSIGNAL(QSplitter,splitterMoved,int,int),
                     this,
                     QNSLOT(MainWindow,onSplitterHandleMoved,int,int),
                     Qt::UniqueConnection);
    QObject::connect(m_pUI->sidePanelSplitter,
                     QNSIGNAL(QSplitter,splitterMoved,int,int),
                     this,
                     QNSLOT(MainWindow,onSidePanelSplittedHandleMoved,int,int),
                     Qt::UniqueConnection);
}

void MainWindow::stopListeningForSplitterMoves()
{
    QNDEBUG("MainWindow::stopListeningForSplitterMoves");

    QObject::disconnect(m_pUI->splitter,
                        QNSIGNAL(QSplitter,splitterMoved,int,int),
                        this,
                        QNSLOT(MainWindow,onSplitterHandleMoved,int,int));
    QObject::disconnect(m_pUI->sidePanelSplitter,
                        QNSIGNAL(QSplitter,splitterMoved,int,int),
                        this,
                        QNSLOT(MainWindow,onSidePanelSplittedHandleMoved,int,int));
}

void MainWindow::persistChosenIconTheme(const QString & iconThemeName)
{
    QNDEBUG("MainWindow::persistChosenIconTheme: " << iconThemeName);

    ApplicationSettings appSettings(*m_pAccount, QUENTIER_UI_SETTINGS);
    appSettings.beginGroup(LOOK_AND_FEEL_SETTINGS_GROUP_NAME);
    appSettings.setValue(ICON_THEME_SETTINGS_KEY, iconThemeName);
    appSettings.endGroup();
}

void MainWindow::refreshChildWidgetsThemeIcons()
{
    QNDEBUG("MainWindow::refreshChildWidgetsThemeIcons");

    refreshThemeIcons<QAction>();
    refreshThemeIcons<QPushButton>();
    refreshThemeIcons<QCheckBox>();
    refreshThemeIcons<ColorPickerToolButton>();
    refreshThemeIcons<InsertTableToolButton>();

    refreshNoteEditorWidgetsSpecialIcons();
}

void MainWindow::refreshNoteEditorWidgetsSpecialIcons()
{
    QNDEBUG("MainWindow::refreshNoteEditorWidgetsSpecialIcons");

    if (m_pNoteEditorTabsAndWindowsCoordinator) {
        m_pNoteEditorTabsAndWindowsCoordinator->refreshNoteEditorWidgetsSpecialIcons();
    }
}

void MainWindow::showHideViewColumnsForAccountType(
    const Account::Type::type accountType)
{
    QNDEBUG("MainWindow::showHideViewColumnsForAccountType: " << accountType);

    bool isLocal = (accountType == Account::Type::Local);

    NotebookItemView * notebooksTreeView = m_pUI->notebooksTreeView;
    notebooksTreeView->setColumnHidden(NotebookModel::Columns::Published, isLocal);
    notebooksTreeView->setColumnHidden(NotebookModel::Columns::Dirty, isLocal);

    TagItemView * tagsTreeView = m_pUI->tagsTreeView;
    tagsTreeView->setColumnHidden(TagModel::Columns::Dirty, isLocal);

    SavedSearchItemView * savedSearchesTableView = m_pUI->savedSearchesTableView;
    savedSearchesTableView->setColumnHidden(SavedSearchModel::Columns::Dirty,
                                            isLocal);

    DeletedNoteItemView * deletedNotesTableView = m_pUI->deletedNotesTableView;
    deletedNotesTableView->setColumnHidden(NoteModel::Columns::Dirty, isLocal);
}

void MainWindow::expandFiltersView()
{
    QNDEBUG("MainWindow::expandFiltersView");

    m_pUI->filtersViewTogglePushButton->setIcon(
        QIcon::fromTheme(QStringLiteral("go-down")));
    m_pUI->filterBodyFrame->show();
    m_pUI->filterFrameBottomBoundary->hide();
    m_pUI->filterFrame->adjustSize();
}

void MainWindow::foldFiltersView()
{
    QNDEBUG("MainWindow::foldFiltersView");

    m_pUI->filtersViewTogglePushButton->setIcon(
        QIcon::fromTheme(QStringLiteral("go-next")));
    m_pUI->filterBodyFrame->hide();
    m_pUI->filterFrameBottomBoundary->show();

    if (m_shown) {
        adjustNoteListAndFiltersSplitterSizes();
    }
}

void MainWindow::adjustNoteListAndFiltersSplitterSizes()
{
    QNDEBUG("MainWindow::adjustNoteListAndFiltersSplitterSizes");

    QList<int> splitterSizes = m_pUI->noteListAndFiltersSplitter->sizes();
    int count = splitterSizes.count();
    if (Q_UNLIKELY(count != 2))
    {
        ErrorString error(QT_TR_NOOP("Internal error: can't properly resize "
                                     "the splitter after folding the filter view: "
                                     "wrong number of sizes within the splitter"));
        QNWARNING(error << "Sizes count: " << count);
        onSetStatusBarText(error.localizedString(), SEC_TO_MSEC(30));
        return;
    }

    int filtersPanelHeight = m_pUI->noteFiltersGenericPanel->height();
    int heightDiff = std::max(splitterSizes[0] - filtersPanelHeight, 0);
    splitterSizes[0] = filtersPanelHeight;
    splitterSizes[1] = splitterSizes[1] + heightDiff;
    m_pUI->noteListAndFiltersSplitter->setSizes(splitterSizes);

    // Need to schedule the repaint because otherwise the actions above
    // seem to have no effect
    m_pUI->noteListAndFiltersSplitter->update();
}

void MainWindow::restorePanelColors()
{
    if (!m_pAccount) {
        return;
    }

    ApplicationSettings settings(*m_pAccount, QUENTIER_UI_SETTINGS);
    settings.beginGroup(PANEL_COLORS_SETTINGS_GROUP_NAME);
    ApplicationSettings::GroupCloser groupCloser(settings);

    QString fontColorName = settings.value(
        PANEL_COLORS_FONT_COLOR_SETTINGS_KEY).toString();
    QColor fontColor(fontColorName);
    if (!fontColor.isValid()) {
        fontColor = QColor(Qt::white);
    }

    QString backgroundColorName = settings.value(
        PANEL_COLORS_BACKGROUND_COLOR_SETTINGS_KEY).toString();
    QColor backgroundColor(backgroundColorName);
    if (!backgroundColor.isValid()) {
        backgroundColor = QColor(Qt::darkGray);
    }

    QLinearGradient gradient(0, 0, 0, 1);
    int rowCount = settings.beginReadArray(
        PANEL_COLORS_BACKGROUND_GRADIENT_LINES_SETTINGS_KEY);
    for(int i = 0; i < rowCount; ++i)
    {
        settings.setArrayIndex(i);

        bool conversionResult = false;
        double value = settings.value(
            PANEL_COLORS_BACKGROUND_GRADIENT_LINE_VALUE_SETTINGS_KEY).toDouble(
                &conversionResult);
        if (Q_UNLIKELY(!conversionResult)) {
            QNWARNING("Failed to convert panel background gradient row value "
                << "to double");
            gradient = QLinearGradient(0, 0, 0, 1);
            break;
        }

        QString colorName = settings.value(
            PANEL_COLORS_BACKGROUND_GRADIENT_LINE_COLOR_SETTTINGS_KEY).toString();
        QColor color(colorName);
        if (!color.isValid()) {
            QNWARNING("Failed to convert panel background gradient row color "
                << "name to valid color: " << colorName);
            gradient = QLinearGradient(0, 0, 0, 1);
            break;
        }

        gradient.setColorAt(value, color);
    }
    settings.endArray();

    bool useBackgroundGradient = settings.value(
        PANEL_COLORS_USE_BACKGROUND_GRADIENT_SETTINGS_KEY).toBool();

    for(auto & pPanelStyleController: m_genericPanelStyleControllers)
    {
        if (useBackgroundGradient) {
            pPanelStyleController->setOverrideColors(fontColor, gradient);
        }
        else {
            pPanelStyleController->setOverrideColors(fontColor, backgroundColor);
        }
    }

    for(auto & pPanelStyleController: m_sidePanelStyleControllers)
    {
        if (useBackgroundGradient) {
            pPanelStyleController->setOverrideColors(fontColor, gradient);
        }
        else {
            pPanelStyleController->setOverrideColors(fontColor, backgroundColor);
        }
    }
}

void MainWindow::setupGenericPanelStyleControllers()
{
    auto panels = findChildren<PanelWidget*>(
        QRegularExpression(QStringLiteral("(.*)GenericPanel")));

    m_genericPanelStyleControllers.clear();
    m_genericPanelStyleControllers.reserve(
        static_cast<size_t>(std::max(panels.size(), 0)));

    QString extraStyleSheet;
    for(auto * pPanel: panels)
    {
        if (pPanel->objectName().startsWith(QStringLiteral("upperBar"))) {
            QTextStream strm(&extraStyleSheet);
            strm << "#upperBarGenericPanel {\n"
                << "border-bottom: 1px solid black;\n"
                << "}\n";
        }
        else {
            extraStyleSheet.clear();
        }

        m_genericPanelStyleControllers.emplace_back(
            std::make_unique<PanelStyleController>(pPanel, extraStyleSheet));
    }
}

void MainWindow::setupSidePanelStyleControllers()
{
    auto panels = findChildren<PanelWidget*>(
        QRegularExpression(QStringLiteral("(.*)SidePanel")));

    m_sidePanelStyleControllers.clear();
    m_sidePanelStyleControllers.reserve(
        static_cast<size_t>(std::max(panels.size(), 0)));

    for(auto * pPanel: panels) {
        m_sidePanelStyleControllers.emplace_back(
            std::make_unique<SidePanelStyleController>(pPanel));
    }
}

void MainWindow::onSetStatusBarText(QString message, const int duration)
{
    QStatusBar * pStatusBar = m_pUI->statusBar;

    pStatusBar->clearMessage();

    if (m_currentStatusBarChildWidget != nullptr) {
        pStatusBar->removeWidget(m_currentStatusBarChildWidget);
        m_currentStatusBarChildWidget = nullptr;
    }

    if (duration == 0) {
        m_currentStatusBarChildWidget = new QLabel(message);
        pStatusBar->addWidget(m_currentStatusBarChildWidget);
    }
    else {
        pStatusBar->showMessage(message, duration);
    }
}

#define DISPATCH_TO_NOTE_EDITOR(MainWindowMethod, NoteEditorMethod)            \
    void MainWindow::MainWindowMethod()                                        \
    {                                                                          \
        QNDEBUG("MainWindow::" #MainWindowMethod);                             \
        NoteEditorWidget * noteEditorWidget = currentNoteEditorTab();          \
        if (!noteEditorWidget) {                                               \
            return;                                                            \
        }                                                                      \
        noteEditorWidget->NoteEditorMethod();                                  \
    }                                                                          \
// DISPATCH_TO_NOTE_EDITOR

DISPATCH_TO_NOTE_EDITOR(onUndoAction, onUndoAction)
DISPATCH_TO_NOTE_EDITOR(onRedoAction, onRedoAction)
DISPATCH_TO_NOTE_EDITOR(onCopyAction, onCopyAction)
DISPATCH_TO_NOTE_EDITOR(onCutAction, onCutAction)
DISPATCH_TO_NOTE_EDITOR(onPasteAction, onPasteAction)
DISPATCH_TO_NOTE_EDITOR(onNoteTextSelectAllToggled, onSelectAllAction)
DISPATCH_TO_NOTE_EDITOR(onNoteTextBoldToggled, onEditorTextBoldToggled)
DISPATCH_TO_NOTE_EDITOR(onNoteTextItalicToggled, onEditorTextItalicToggled)
DISPATCH_TO_NOTE_EDITOR(onNoteTextUnderlineToggled, onEditorTextUnderlineToggled)
DISPATCH_TO_NOTE_EDITOR(onNoteTextStrikethroughToggled,
                        onEditorTextStrikethroughToggled)
DISPATCH_TO_NOTE_EDITOR(onNoteTextAlignLeftAction, onEditorTextAlignLeftAction)
DISPATCH_TO_NOTE_EDITOR(onNoteTextAlignCenterAction,
                        onEditorTextAlignCenterAction)
DISPATCH_TO_NOTE_EDITOR(onNoteTextAlignRightAction, onEditorTextAlignRightAction)
DISPATCH_TO_NOTE_EDITOR(onNoteTextAlignFullAction, onEditorTextAlignFullAction)
DISPATCH_TO_NOTE_EDITOR(onNoteTextAddHorizontalLineAction,
                        onEditorTextAddHorizontalLineAction)
DISPATCH_TO_NOTE_EDITOR(onNoteTextIncreaseFontSizeAction,
                        onEditorTextIncreaseFontSizeAction)
DISPATCH_TO_NOTE_EDITOR(onNoteTextDecreaseFontSizeAction,
                        onEditorTextDecreaseFontSizeAction)
DISPATCH_TO_NOTE_EDITOR(onNoteTextHighlightAction, onEditorTextHighlightAction)
DISPATCH_TO_NOTE_EDITOR(onNoteTextIncreaseIndentationAction,
                        onEditorTextIncreaseIndentationAction)
DISPATCH_TO_NOTE_EDITOR(onNoteTextDecreaseIndentationAction,
                        onEditorTextDecreaseIndentationAction)
DISPATCH_TO_NOTE_EDITOR(onNoteTextInsertUnorderedListAction,
                        onEditorTextInsertUnorderedListAction)
DISPATCH_TO_NOTE_EDITOR(onNoteTextInsertOrderedListAction,
                        onEditorTextInsertOrderedListAction)
DISPATCH_TO_NOTE_EDITOR(onNoteTextInsertToDoAction, onEditorTextInsertToDoAction)
DISPATCH_TO_NOTE_EDITOR(onNoteTextInsertTableDialogAction,
                        onEditorTextInsertTableDialogRequested)
DISPATCH_TO_NOTE_EDITOR(onNoteTextEditHyperlinkAction,
                        onEditorTextEditHyperlinkAction)
DISPATCH_TO_NOTE_EDITOR(onNoteTextCopyHyperlinkAction,
                        onEditorTextCopyHyperlinkAction)
DISPATCH_TO_NOTE_EDITOR(onNoteTextRemoveHyperlinkAction,
                        onEditorTextRemoveHyperlinkAction)
DISPATCH_TO_NOTE_EDITOR(onFindInsideNoteAction, onFindInsideNoteAction)
DISPATCH_TO_NOTE_EDITOR(onFindPreviousInsideNoteAction,
                        onFindPreviousInsideNoteAction)
DISPATCH_TO_NOTE_EDITOR(onReplaceInsideNoteAction, onReplaceInsideNoteAction)

#undef DISPATCH_TO_NOTE_EDITOR

void MainWindow::onImportEnexAction()
{
    QNDEBUG("MainWindow::onImportEnexAction");

    if (Q_UNLIKELY(!m_pAccount)) {
        QNDEBUG("No current account, skipping");
        return;
    }

    if (Q_UNLIKELY(!m_pLocalStorageManagerAsync)) {
        QNDEBUG("No local storage manager, skipping");
        return;
    }

    if (Q_UNLIKELY(!m_pTagModel)) {
        QNDEBUG("No tag model, skipping");
        return;
    }

    if (Q_UNLIKELY(!m_pNotebookModel)) {
        QNDEBUG("No notebook model, skipping");
        return;
    }

    QScopedPointer<EnexImportDialog> pEnexImportDialog(
        new EnexImportDialog(*m_pAccount, *m_pNotebookModel, this));
    pEnexImportDialog->setWindowModality(Qt::WindowModal);
    centerDialog(*pEnexImportDialog);
    if (pEnexImportDialog->exec() != QDialog::Accepted) {
        QNDEBUG("The import of ENEX was cancelled");
        return;
    }

    ErrorString errorDescription;

    QString enexFilePath = pEnexImportDialog->importEnexFilePath(&errorDescription);
    if (enexFilePath.isEmpty())
    {
        if (errorDescription.isEmpty()) {
            errorDescription.setBase(QT_TR_NOOP("Can't import ENEX: internal "
                                                "error, can't retrieve ENEX "
                                                "file path"));
        }

        QNDEBUG("Bad ENEX file path: " << errorDescription);
        onSetStatusBarText(errorDescription.localizedString(), SEC_TO_MSEC(30));
        return;
    }

    QString notebookName = pEnexImportDialog->notebookName(&errorDescription);
    if (notebookName.isEmpty())
    {
        if (errorDescription.isEmpty()) {
            errorDescription.setBase(QT_TR_NOOP("Can't import ENEX: internal "
                                                "error, can't retrieve notebook "
                                                "name"));
        }

        QNDEBUG("Bad notebook name: " << errorDescription);
        onSetStatusBarText(errorDescription.localizedString(), SEC_TO_MSEC(30));
        return;
    }

    EnexImporter * pImporter = new EnexImporter(enexFilePath, notebookName,
                                                *m_pLocalStorageManagerAsync,
                                                *m_pTagModel, *m_pNotebookModel,
                                                this);
    QObject::connect(pImporter,
                     QNSIGNAL(EnexImporter,enexImportedSuccessfully,QString),
                     this,
                     QNSLOT(MainWindow,onEnexImportCompletedSuccessfully,QString));
    QObject::connect(pImporter,
                     QNSIGNAL(EnexImporter,enexImportFailed,ErrorString),
                     this,
                     QNSLOT(MainWindow,onEnexImportFailed,ErrorString));
    pImporter->start();
}

void MainWindow::onSynchronizationStarted()
{
    QNDEBUG("MainWindow::onSynchronizationStarted");

    onSetStatusBarText(tr("Starting the synchronization") + QStringLiteral("..."));
    m_syncApiRateLimitExceeded = false;
    m_syncInProgress = true;
    startSyncButtonAnimation();
}

void MainWindow::onSynchronizationStopped()
{
    QNINFO("MainWindow::onSynchronizationStopped");

    onSetStatusBarText(tr("Synchronization was stopped"), SEC_TO_MSEC(30));
    m_syncApiRateLimitExceeded = false;
}

void MainWindow::onSynchronizationManagerFailure(ErrorString errorDescription)
{
    QNERROR("MainWindow::onSynchronizationManagerFailure: " << errorDescription);
    onSetStatusBarText(errorDescription.localizedString(), SEC_TO_MSEC(60));
    Q_EMIT stopSynchronization();
}

void MainWindow::onSynchronizationFinished(
    Account account, bool somethingDownloaded, bool somethingSent)
{
    QNINFO("MainWindow::onSynchronizationFinished: " << account.name());

    if (somethingDownloaded || somethingSent) {
        onSetStatusBarText(tr("Synchronization finished!"), SEC_TO_MSEC(5));
    }
    else {
        onSetStatusBarText(QString());
    }

    m_syncInProgress = false;
    scheduleSyncButtonAnimationStop();

    setupRunSyncPeriodicallyTimer();

    QNINFO("Synchronization finished for user " << account.name()
           << ", id " << account.id() << ", something downloaded = "
           << (somethingDownloaded ? "true" : "false")
           << ", something sent = " << (somethingSent ? "true" : "false"));
}

void MainWindow::onAuthenticationFinished(
    bool success, ErrorString errorDescription, Account account)
{
    QNINFO("MainWindow::onAuthenticationFinished: success = "
           << (success ? "true" : "false")
           << ", error description = " << errorDescription
           << ", account = " << account.name());

    bool wasPendingNewEvernoteAccountAuthentication =
        m_pendingNewEvernoteAccountAuthentication;
    m_pendingNewEvernoteAccountAuthentication = false;

    bool wasPendingCurrentEvernoteAccountAuthentication =
        m_pendingCurrentEvernoteAccountAuthentication;
    m_pendingCurrentEvernoteAccountAuthentication = false;

    if (!success)
    {
        // Restore the network proxy which was active before the authentication
        QNetworkProxy::setApplicationProxy(
            m_applicationProxyBeforeNewEvernoteAccountAuthenticationRequest);
        m_applicationProxyBeforeNewEvernoteAccountAuthenticationRequest =
            QNetworkProxy(QNetworkProxy::NoProxy);

        onSetStatusBarText(tr("Couldn't authenticate the Evernote user") +
                           QStringLiteral(": ") + errorDescription.localizedString(),
                           SEC_TO_MSEC(30));
        return;
    }

    QNetworkProxy currentProxy = QNetworkProxy::applicationProxy();
    persistNetworkProxySettingsForAccount(account, currentProxy);

    m_applicationProxyBeforeNewEvernoteAccountAuthenticationRequest =
        QNetworkProxy(QNetworkProxy::NoProxy);

    if (wasPendingCurrentEvernoteAccountAuthentication) {
        setupSynchronizationManagerThread();
        m_authenticatedCurrentEvernoteAccount = true;
        launchSynchronization();
        return;
    }

    if (wasPendingNewEvernoteAccountAuthentication) {
        m_pendingSwitchToNewEvernoteAccount = true;
        m_pAccountManager->switchAccount(account);
    }
}

void MainWindow::onAuthenticationRevoked(
    bool success, ErrorString errorDescription, qevercloud::UserID userId)
{
    QNINFO("MainWindow::onAuthenticationRevoked: success = "
           << (success ? "true" : "false")
           << ", error description = " << errorDescription
           << ", user id = " << userId);

    if (!success) {
        onSetStatusBarText(tr("Couldn't revoke the authentication") +
                           QStringLiteral(": ") + errorDescription.localizedString(),
                           SEC_TO_MSEC(30));
        return;
    }

    QNINFO("Revoked authentication for user with id " << userId);
}

void MainWindow::onRateLimitExceeded(qint32 secondsToWait)
{
    QNINFO("MainWindow::onRateLimitExceeded: seconds to wait = "
           << secondsToWait);

    qint64 currentTimestamp = QDateTime::currentMSecsSinceEpoch();
    qint64 futureTimestamp = currentTimestamp + secondsToWait * 1000;
    QDateTime futureDateTime;
    futureDateTime.setMSecsSinceEpoch(futureTimestamp);

    QDateTime today = QDateTime::currentDateTime();
    bool includeDate = (today.date() != futureDateTime.date());

    QString dateTimeToShow =
        (includeDate
         ? futureDateTime.toString(QStringLiteral("dd.MM.yyyy hh:mm:ss"))
         : futureDateTime.toString(QStringLiteral("hh:mm:ss")));

    onSetStatusBarText(tr("The synchronization has reached the Evernote API rate "
                          "limit, it will continue automatically at approximately") +
                       QStringLiteral(" ") + dateTimeToShow,
                       SEC_TO_MSEC(60));

    m_animatedSyncButtonIcon.setPaused(true);

    QNINFO("Evernote API rate limit exceeded, need to wait for "
           << secondsToWait
           << " seconds, the synchronization will continue at "
           << dateTimeToShow);

    m_syncApiRateLimitExceeded = true;
}

void MainWindow::onRemoteToLocalSyncDone(bool somethingDownloaded)
{
    QNTRACE("MainWindow::onRemoteToLocalSyncDone");

    QNINFO("Remote to local sync done: "
           << (somethingDownloaded
               ? "received all updates from Evernote"
               : "no updates found on Evernote side"));

    if (somethingDownloaded) {
        onSetStatusBarText(tr("Received all updates from Evernote servers, "
                              "sending local changes"));
    }
    else {
        onSetStatusBarText(tr("No updates found on Evernote servers, sending "
                              "local changes"));
    }
}

void MainWindow::onSyncChunksDownloadProgress(
    qint32 highestDownloadedUsn, qint32 highestServerUsn,
    qint32 lastPreviousUsn)
{
    QNINFO("MainWindow::onSyncChunksDownloadProgress: "
           << "highest downloaded USN = "
           << highestDownloadedUsn << ", highest server USN = "
           << highestServerUsn << ", last previous USN = "
           << lastPreviousUsn);

    if (Q_UNLIKELY((highestServerUsn <= lastPreviousUsn) ||
                   (highestDownloadedUsn <= lastPreviousUsn)))
    {
        QNWARNING("Received incorrect sync chunks download progress "
                  << "state: highest downloaded USN = "
                  << highestDownloadedUsn << ", highest server USN = "
                  << highestServerUsn << ", last previous USN = "
                  << lastPreviousUsn);
        return;
    }

    double numerator = highestDownloadedUsn - lastPreviousUsn;
    double denominator = highestServerUsn - lastPreviousUsn;

    double percentage = numerator / denominator * 100.0;
    percentage = std::min(percentage, 100.0);

    QString statusBarText;
    QTextStream strm(&statusBarText);
    strm << tr("Downloading sync chunks") << ": "
        << QString::number(percentage, 'f', 2) << "%";

    onSetStatusBarText(statusBarText);
}

void MainWindow::onSyncChunksDownloaded()
{
    QNDEBUG("MainWindow::onSyncChunksDownloaded");
    onSetStatusBarText(tr("Downloaded sync chunks, parsing tags, notebooks and "
                          "saved searches from them") + QStringLiteral("..."));
}

void MainWindow::onNotesDownloadProgress(
    quint32 notesDownloaded, quint32 totalNotesToDownload)
{
    QNDEBUG("MainWindow::onNotesDownloadProgress: notes downloaded = "
            << notesDownloaded << ", total notes to download = "
            << totalNotesToDownload);

    onSetStatusBarText(tr("Downloading notes") + QStringLiteral(": ") +
                       QString::number(notesDownloaded) + QStringLiteral(" ") +
                       tr("of") + QStringLiteral(" ") +
                       QString::number(totalNotesToDownload));
}

void MainWindow::onResourcesDownloadProgress(
    quint32 resourcesDownloaded, quint32 totalResourcesToDownload)
{
    QNDEBUG("MainWindow::onResourcesDownloadProgress: "
            << "resources downloaded = " << resourcesDownloaded
            << ", total resources to download = " << totalResourcesToDownload);

    onSetStatusBarText(tr("Downloading attachments") + QStringLiteral(": ") +
                       QString::number(resourcesDownloaded) + QStringLiteral(" ") +
                       tr("of") + QStringLiteral(" ") +
                       QString::number(totalResourcesToDownload));
}

void MainWindow::onLinkedNotebookSyncChunksDownloadProgress(
    qint32 highestDownloadedUsn, qint32 highestServerUsn,
    qint32 lastPreviousUsn, LinkedNotebook linkedNotebook)
{
    QNDEBUG("MainWindow::onLinkedNotebookSyncChunksDownloadProgress: "
            << "highest downloaded USN = " << highestDownloadedUsn
            << ", highest server USN = " << highestServerUsn
            << ", last previous USN = " << lastPreviousUsn
            << ", linked notebook = " << linkedNotebook);

    if (Q_UNLIKELY((highestServerUsn <= lastPreviousUsn) ||
                   (highestDownloadedUsn <= lastPreviousUsn)))
    {
        QNWARNING("Received incorrect sync chunks download progress "
                  << "state: highest downloaded USN = "
                  << highestDownloadedUsn << ", highest server USN = "
                  << highestServerUsn << ", last previous USN = "
                  << lastPreviousUsn << ", linked notebook: "
                  << linkedNotebook);
        return;
    }

    double percentage = (highestDownloadedUsn - lastPreviousUsn) /
                        (highestServerUsn - lastPreviousUsn) * 100.0;
    percentage = std::min(percentage, 100.0);

    QString message = tr("Downloading sync chunks from linked notebook");

    if (linkedNotebook.hasShareName()) {
        message += QStringLiteral(": ") + linkedNotebook.shareName();
    }

    if (linkedNotebook.hasUsername()) {
        message += QStringLiteral(" (") + linkedNotebook.username() +
                   QStringLiteral(")");
    }

    message += QStringLiteral(": ") + QString::number(percentage) +
               QStringLiteral("%");
    onSetStatusBarText(message);
}

void MainWindow::onLinkedNotebooksSyncChunksDownloaded()
{
    QNDEBUG("MainWindow::onLinkedNotebooksSyncChunksDownloaded");
    onSetStatusBarText(tr("Downloaded the sync chunks from linked notebooks"));
}

void MainWindow::onLinkedNotebooksNotesDownloadProgress(
    quint32 notesDownloaded, quint32 totalNotesToDownload)
{
    QNDEBUG("MainWindow::onLinkedNotebooksNotesDownloadProgress: "
            << "notes downloaded = " << notesDownloaded
            << ", total notes to download = " << totalNotesToDownload);

    onSetStatusBarText(tr("Downloading notes from linked notebooks") +
                       QStringLiteral(": ") + QString::number(notesDownloaded) +
                       QStringLiteral(" ") + tr("of") + QStringLiteral(" ") +
                       QString::number(totalNotesToDownload));
}

void MainWindow::onRemoteToLocalSyncStopped()
{
    QNDEBUG("MainWindow::onRemoteToLocalSyncStopped");
    onSynchronizationStopped();
}

void MainWindow::onSendLocalChangesStopped()
{
    QNDEBUG("MainWindow::onSendLocalChangesStopped");
    onSynchronizationStopped();
}

void MainWindow::onEvernoteAccountAuthenticationRequested(
    QString host, QNetworkProxy proxy)
{
    QNDEBUG("MainWindow::onEvernoteAccountAuthenticationRequested: host = "
            << host << ", proxy type = " << proxy.type() << ", proxy host = "
            << proxy.hostName() << ", proxy port = " << proxy.port()
            << ", proxy user = " << proxy.user());

    m_synchronizationManagerHost = host;
    setupSynchronizationManager();

    // Set the proxy specified within the slot argument but remember the previous
    // application proxy so that it can be restored in case of authentication
    // failure
    m_applicationProxyBeforeNewEvernoteAccountAuthenticationRequest =
        QNetworkProxy::applicationProxy();
    QNetworkProxy::setApplicationProxy(proxy);

    m_pendingNewEvernoteAccountAuthentication = true;
    Q_EMIT authenticate();
}

void MainWindow::onNoteTextSpellCheckToggled()
{
    QNDEBUG("MainWindow::onNoteTextSpellCheckToggled");

    NoteEditorWidget * noteEditorWidget = currentNoteEditorTab();
    if (!noteEditorWidget) {
        return;
    }

    if (noteEditorWidget->isSpellCheckEnabled()) {
        noteEditorWidget->onEditorSpellCheckStateChanged(Qt::Unchecked);
    }
    else {
        noteEditorWidget->onEditorSpellCheckStateChanged(Qt::Checked);
    }
}

void MainWindow::onShowNoteSource()
{
    QNDEBUG("MainWindow::onShowNoteSource");

    NoteEditorWidget * pNoteEditorWidget = currentNoteEditorTab();
    if (!pNoteEditorWidget) {
        return;
    }

    if (!pNoteEditorWidget->isNoteSourceShown()) {
        pNoteEditorWidget->showNoteSource();
    }
    else {
        pNoteEditorWidget->hideNoteSource();
    }
}

void MainWindow::onSaveNoteAction()
{
    QNDEBUG("MainWindow::onSaveNoteAction");

    NoteEditorWidget * pNoteEditorWidget = currentNoteEditorTab();
    if (!pNoteEditorWidget) {
        return;
    }

    pNoteEditorWidget->onSaveNoteAction();
}

void MainWindow::onNewNotebookCreationRequested()
{
    QNDEBUG("MainWindow::onNewNotebookCreationRequested");

    if (Q_UNLIKELY(!m_pNotebookModel)) {
        ErrorString error(QT_TR_NOOP("Can't create a new notebook: no notebook "
                                     "model is set up"));
        QNWARNING(error);
        onSetStatusBarText(error.localizedString(), SEC_TO_MSEC(30));
        return;
    }

    QScopedPointer<AddOrEditNotebookDialog> pAddNotebookDialog(
        new AddOrEditNotebookDialog(m_pNotebookModel, this));
    pAddNotebookDialog->setWindowModality(Qt::WindowModal);
    centerDialog(*pAddNotebookDialog);
    Q_UNUSED(pAddNotebookDialog->exec())
}

void MainWindow::onRemoveNotebookButtonPressed()
{
    QNDEBUG("MainWindow::onRemoveNotebookButtonPressed");
    m_pUI->notebooksTreeView->deleteSelectedItem();
}

void MainWindow::onNotebookInfoButtonPressed()
{
    QNDEBUG("MainWindow::onNotebookInfoButtonPressed");

    QModelIndex index = m_pUI->notebooksTreeView->currentlySelectedItemIndex();
    NotebookModelItemInfoWidget * pNotebookModelItemInfoWidget =
        new NotebookModelItemInfoWidget(index, this);
    showInfoWidget(pNotebookModelItemInfoWidget);
}

void MainWindow::onNewTagCreationRequested()
{
    QNDEBUG("MainWindow::onNewTagCreationRequested");

    if (Q_UNLIKELY(!m_pTagModel)) {
        ErrorString error(QT_TR_NOOP("Can't create a new tag: no tag model is set up"));
        QNWARNING(error);
        onSetStatusBarText(error.localizedString(), SEC_TO_MSEC(30));
        return;
    }

    QScopedPointer<AddOrEditTagDialog> pAddTagDialog(
        new AddOrEditTagDialog(m_pTagModel, this));
    pAddTagDialog->setWindowModality(Qt::WindowModal);
    centerDialog(*pAddTagDialog);
    Q_UNUSED(pAddTagDialog->exec())
}

void MainWindow::onRemoveTagButtonPressed()
{
    QNDEBUG("MainWindow::onRemoveTagButtonPressed");
    m_pUI->tagsTreeView->deleteSelectedItem();
}

void MainWindow::onTagInfoButtonPressed()
{
    QNDEBUG("MainWindow::onTagInfoButtonPressed");

    QModelIndex index = m_pUI->tagsTreeView->currentlySelectedItemIndex();
    TagModelItemInfoWidget * pTagModelItemInfoWidget =
        new TagModelItemInfoWidget(index, this);
    showInfoWidget(pTagModelItemInfoWidget);
}

void MainWindow::onNewSavedSearchCreationRequested()
{
    QNDEBUG("MainWindow::onNewSavedSearchCreationRequested");

    if (Q_UNLIKELY(!m_pSavedSearchModel)) {
        ErrorString error(QT_TR_NOOP("Can't create a new saved search: no saved "
                                     "search model is set up"));
        QNWARNING(error);
        onSetStatusBarText(error.localizedString(), SEC_TO_MSEC(30));
        return;
    }

    QScopedPointer<AddOrEditSavedSearchDialog> pAddSavedSearchDialog(
        new AddOrEditSavedSearchDialog(m_pSavedSearchModel, this));
    pAddSavedSearchDialog->setWindowModality(Qt::WindowModal);
    centerDialog(*pAddSavedSearchDialog);
    Q_UNUSED(pAddSavedSearchDialog->exec())
}

void MainWindow::onRemoveSavedSearchButtonPressed()
{
    QNDEBUG("MainWindow::onRemoveSavedSearchButtonPressed");
    m_pUI->savedSearchesTableView->deleteSelectedItem();
}

void MainWindow::onSavedSearchInfoButtonPressed()
{
    QNDEBUG("MainWindow::onSavedSearchInfoButtonPressed");

    QModelIndex index = m_pUI->savedSearchesTableView->currentlySelectedItemIndex();
    SavedSearchModelItemInfoWidget * pSavedSearchModelItemInfoWidget =
        new SavedSearchModelItemInfoWidget(index, this);
    showInfoWidget(pSavedSearchModelItemInfoWidget);
}

void MainWindow::onUnfavoriteItemButtonPressed()
{
    QNDEBUG("MainWindow::onUnfavoriteItemButtonPressed");
    m_pUI->favoritesTableView->deleteSelectedItems();
}

void MainWindow::onFavoritedItemInfoButtonPressed()
{
    QNDEBUG("MainWindow::onFavoritedItemInfoButtonPressed");
    QModelIndex index = m_pUI->favoritesTableView->currentlySelectedItemIndex();
    if (!index.isValid()) {
        Q_UNUSED(informationMessageBox(this,
                                       tr("Not exactly one favorited item is "
                                          "selected"),
                                       tr("Please select the only one favorited "
                                          "item to see its detailed info")));
        return;
    }

    FavoritesModel * pFavoritesModel =
        qobject_cast<FavoritesModel*>(m_pUI->favoritesTableView->model());
    if (Q_UNLIKELY(!pFavoritesModel)) {
        Q_UNUSED(internalErrorMessageBox(this,
                                         tr("Failed to cast the favorited table "
                                            "view's model to favorites model")))
        return;
    }

    const FavoritesModelItem * pItem = pFavoritesModel->itemAtRow(index.row());
    if (Q_UNLIKELY(!pItem)) {
        Q_UNUSED(internalErrorMessageBox(this,
                                         tr("Favorites model returned null pointer "
                                            "to favorited item for the selected "
                                            "index")))
        return;
    }

    switch(pItem->type())
    {
    case FavoritesModelItem::Type::Note:
        Q_EMIT noteInfoDialogRequested(pItem->localUid());
        break;
    case FavoritesModelItem::Type::Notebook:
        {
            if (Q_LIKELY(m_pNotebookModel))
            {
                QModelIndex notebookIndex =
                    m_pNotebookModel->indexForLocalUid(pItem->localUid());
                NotebookModelItemInfoWidget * pNotebookItemInfoWidget =
                    new NotebookModelItemInfoWidget(notebookIndex, this);
                showInfoWidget(pNotebookItemInfoWidget);
            }
            else
            {
                Q_UNUSED(internalErrorMessageBox(this,
                                                 tr("No notebook model exists "
                                                    "at the moment")))
            }
            break;
        }
    case FavoritesModelItem::Type::SavedSearch:
        {
            if (Q_LIKELY(m_pSavedSearchModel))
            {
                QModelIndex savedSearchIndex =
                    m_pSavedSearchModel->indexForLocalUid(pItem->localUid());
                SavedSearchModelItemInfoWidget * pSavedSearchItemInfoWidget =
                    new SavedSearchModelItemInfoWidget(savedSearchIndex, this);
                showInfoWidget(pSavedSearchItemInfoWidget);
            }
            else
            {
                Q_UNUSED(internalErrorMessageBox(this, tr("No saved search model "
                                                          "exists at the moment"));)
            }
            break;
        }
    case FavoritesModelItem::Type::Tag:
        {
            if (Q_LIKELY(m_pTagModel))
            {
                QModelIndex tagIndex =
                    m_pTagModel->indexForLocalUid(pItem->localUid());
                TagModelItemInfoWidget * pTagItemInfoWidget =
                    new TagModelItemInfoWidget(tagIndex, this);
                showInfoWidget(pTagItemInfoWidget);
            }
            else
            {
                Q_UNUSED(internalErrorMessageBox(this,
                                                 tr("No tag model exists "
                                                    "at the moment"));)
            }
            break;
        }
    default:
        Q_UNUSED(internalErrorMessageBox(this,
                                         tr("Incorrect favorited item type") +
                                         QStringLiteral(": ") +
                                         QString::number(pItem->type())))
        break;
    }
}

void MainWindow::onRestoreDeletedNoteButtonPressed()
{
    QNDEBUG("MainWindow::onRestoreDeletedNoteButtonPressed");
    m_pUI->deletedNotesTableView->restoreCurrentlySelectedNote();
}

void MainWindow::onDeleteNotePermanentlyButtonPressed()
{
    QNDEBUG("MainWindow::onDeleteNotePermanentlyButtonPressed");
    m_pUI->deletedNotesTableView->deleteCurrentlySelectedNotePermanently();
}

void MainWindow::onDeletedNoteInfoButtonPressed()
{
    QNDEBUG("MainWindow::onDeletedNoteInfoButtonPressed");
    m_pUI->deletedNotesTableView->showCurrentlySelectedNoteInfo();
}

void MainWindow::showInfoWidget(QWidget * pWidget)
{
    pWidget->setAttribute(Qt::WA_DeleteOnClose);
    pWidget->setWindowModality(Qt::WindowModal);
    pWidget->adjustSize();
#ifndef Q_OS_MAC
    centerWidget(*pWidget);
#endif
    pWidget->show();
}

void MainWindow::onFiltersViewTogglePushButtonPressed()
{
    QNDEBUG("MainWindow::onFiltersViewTogglePushButtonPressed");

    m_filtersViewExpanded = !m_filtersViewExpanded;

    if (m_filtersViewExpanded) {
        expandFiltersView();
    }
    else {
        foldFiltersView();
    }

    ApplicationSettings appSettings(*m_pAccount, QUENTIER_UI_SETTINGS);
    appSettings.beginGroup(QStringLiteral("FiltersView"));
    appSettings.setValue(FILTERS_VIEW_STATUS_KEY, QVariant(m_filtersViewExpanded));
    appSettings.endGroup();
}

void MainWindow::onShowPreferencesDialogAction()
{
    QNDEBUG("MainWindow::onShowPreferencesDialogAction");

    PreferencesDialog * pExistingPreferencesDialog =
        findChild<PreferencesDialog*>();
    if (pExistingPreferencesDialog) {
        QNDEBUG("Preferences dialog already exists, won't show another one");
        return;
    }

    QList<QMenu*> menus = m_pUI->menuBar->findChildren<QMenu*>();
    ActionsInfo actionsInfo(menus);

    auto pPreferencesDialog = std::make_unique<PreferencesDialog>(
        *m_pAccountManager,
        m_shortcutManager,
        *m_pSystemTrayIconManager,
        actionsInfo,
        this);

    pPreferencesDialog->setWindowModality(Qt::WindowModal);
    centerDialog(*pPreferencesDialog);
    connectToPreferencesDialogSignals(*pPreferencesDialog);
    Q_UNUSED(pPreferencesDialog->exec())
}

void MainWindow::onNoteSortingModeChanged(int index)
{
    QNDEBUG("MainWindow::onNoteSortingModeChanged: index = " << index);

    persistChosenNoteSortingMode(index);

    if (Q_UNLIKELY(!m_pNoteModel)) {
        QNDEBUG("No note model, ignoring the change");
        return;
    }

    switch(index)
    {
    case NoteModel::NoteSortingMode::CreatedAscending:
        m_pNoteModel->sort(NoteModel::Columns::CreationTimestamp,
                           Qt::AscendingOrder);
        break;
    case NoteModel::NoteSortingMode::CreatedDescending:
        m_pNoteModel->sort(NoteModel::Columns::CreationTimestamp,
                           Qt::DescendingOrder);
        break;
    case NoteModel::NoteSortingMode::ModifiedAscending:
        m_pNoteModel->sort(NoteModel::Columns::ModificationTimestamp,
                           Qt::AscendingOrder);
        break;
    case NoteModel::NoteSortingMode::ModifiedDescending:
        m_pNoteModel->sort(NoteModel::Columns::ModificationTimestamp,
                           Qt::DescendingOrder);
        break;
    case NoteModel::NoteSortingMode::TitleAscending:
        m_pNoteModel->sort(NoteModel::Columns::Title, Qt::AscendingOrder);
        break;
    case NoteModel::NoteSortingMode::TitleDescending:
        m_pNoteModel->sort(NoteModel::Columns::Title, Qt::DescendingOrder);
        break;
    case NoteModel::NoteSortingMode::SizeAscending:
        m_pNoteModel->sort(NoteModel::Columns::Size, Qt::AscendingOrder);
        break;
    case NoteModel::NoteSortingMode::SizeDescending:
        m_pNoteModel->sort(NoteModel::Columns::Size, Qt::DescendingOrder);
        break;
    default:
        {
            ErrorString error(QT_TR_NOOP("Internal error: got unknown note "
                                         "sorting order, fallback to the default"));
            QNWARNING(error << ", sorting mode index = " << index);
            onSetStatusBarText(error.localizedString(), SEC_TO_MSEC(30));

            m_pNoteModel->sort(NoteModel::Columns::CreationTimestamp,
                               Qt::AscendingOrder);
            break;
        }
    }
}

void MainWindow::onNewNoteCreationRequested()
{
    QNDEBUG("MainWindow::onNewNoteCreationRequested");
    createNewNote(NoteEditorTabsAndWindowsCoordinator::NoteEditorMode::Any);
}

void MainWindow::onToggleThumbnailsPreference(QString noteLocalUid)
{
    QNDEBUG("MainWindow::onToggleThumbnailsPreference: note local uid = "
            << noteLocalUid);
    bool toggleForAllNotes = noteLocalUid.isEmpty();
    if (toggleForAllNotes) {
        toggleShowNoteThumbnails();
    }
    else {
        toggleHideNoteThumbnailFor(noteLocalUid);
    }

    onShowNoteThumbnailsPreferenceChanged();
}

void MainWindow::onCopyInAppLinkNoteRequested(
    QString noteLocalUid, QString noteGuid)
{
    QNDEBUG("MainWindow::onCopyInAppLinkNoteRequested: "
            << "note local uid = " << noteLocalUid
            << ", note guid = " << noteGuid);

    if (noteGuid.isEmpty()) {
        QNDEBUG("Can't copy the in-app note link: note guid is empty");
        return;
    }

    if (Q_UNLIKELY(m_pAccount.isNull())) {
        QNDEBUG("Can't copy the in-app note link: no current account");
        return;
    }

    if (Q_UNLIKELY(m_pAccount->type() != Account::Type::Evernote)) {
        QNDEBUG("Can't copy the in-app note link: the current "
                "account is not of Evernote type");
        return;
    }

    qevercloud::UserID id = m_pAccount->id();
    if (Q_UNLIKELY(id < 0)) {
        QNDEBUG("Can't copy the in-app note link: the current "
                "account's id is negative");
        return;
    }

    QString shardId = m_pAccount->shardId();
    if (shardId.isEmpty()) {
        QNDEBUG("Can't copy the in-app note link: the current "
                "account's shard id is empty");
        return;
    }

    QString urlString = QStringLiteral("evernote:///view/") + QString::number(id) +
                        QStringLiteral("/") + shardId + QStringLiteral("/") +
                        noteGuid + QStringLiteral("/") + noteGuid;

    QClipboard * pClipboard = QApplication::clipboard();
    if (pClipboard) {
        QNTRACE("Setting the composed in-app note URL to the clipboard: "
                << urlString);
        pClipboard->setText(urlString);
    }
}

void MainWindow::onCurrentNoteInListChanged(QString noteLocalUid)
{
    QNDEBUG("MainWindow::onCurrentNoteInListChanged: " << noteLocalUid);
    m_pNoteEditorTabsAndWindowsCoordinator->addNote(noteLocalUid);
}

void MainWindow::onOpenNoteInSeparateWindow(QString noteLocalUid)
{
    QNDEBUG("MainWindow::onOpenNoteInSeparateWindow: " << noteLocalUid);
    m_pNoteEditorTabsAndWindowsCoordinator->addNote(
        noteLocalUid,
        NoteEditorTabsAndWindowsCoordinator::NoteEditorMode::Window);
}

void MainWindow::onDeleteCurrentNoteButtonPressed()
{
    QNDEBUG("MainWindow::onDeleteCurrentNoteButtonPressed");

    if (Q_UNLIKELY(!m_pNoteModel)) {
        ErrorString errorDescription(QT_TR_NOOP("Can't delete the current note: "
                                                "internal error, no note model"));
        QNDEBUG(errorDescription);
        onSetStatusBarText(errorDescription.localizedString(), SEC_TO_MSEC(30));
        return;
    }

    NoteEditorWidget * pNoteEditorWidget = currentNoteEditorTab();
    if (!pNoteEditorWidget) {
        ErrorString errorDescription(QT_TR_NOOP("Can't delete the current note: "
                                                "no note editor tabs"));
        QNDEBUG(errorDescription);
        onSetStatusBarText(errorDescription.localizedString(), SEC_TO_MSEC(30));
        return;
    }

    ErrorString error;
    bool res = m_pNoteModel->deleteNote(pNoteEditorWidget->noteLocalUid(), error);
    if (Q_UNLIKELY(!res)) {
        ErrorString errorDescription(QT_TR_NOOP("Can't delete the current note"));
        errorDescription.appendBase(error.base());
        errorDescription.appendBase(error.additionalBases());
        errorDescription.details() = error.details();
        QNDEBUG(errorDescription);
        onSetStatusBarText(errorDescription.localizedString(), SEC_TO_MSEC(30));
        return;
    }
}

void MainWindow::onCurrentNoteInfoRequested()
{
    QNDEBUG("MainWindow::onCurrentNoteInfoRequested");

    NoteEditorWidget * pNoteEditorWidget = currentNoteEditorTab();
    if (!pNoteEditorWidget) {
        ErrorString errorDescription(QT_TR_NOOP("Can't show note info: no note "
                                                "editor tabs"));
        QNDEBUG(errorDescription);
        onSetStatusBarText(errorDescription.localizedString(), SEC_TO_MSEC(30));
        return;
    }

    Q_EMIT noteInfoDialogRequested(pNoteEditorWidget->noteLocalUid());
}

void MainWindow::onCurrentNotePrintRequested()
{
    QNDEBUG("MainWindow::onCurrentNotePrintRequested");

    NoteEditorWidget * pNoteEditorWidget = currentNoteEditorTab();
    if (!pNoteEditorWidget) {
        ErrorString errorDescription(QT_TR_NOOP("Can't print note: no note "
                                                "editor tabs"));
        QNDEBUG(errorDescription);
        onSetStatusBarText(errorDescription.localizedString(), SEC_TO_MSEC(30));
        return;
    }

    ErrorString errorDescription;
    bool res = pNoteEditorWidget->printNote(errorDescription);
    if (!res)
    {
        if (errorDescription.isEmpty()) {
            return;
        }

        QNDEBUG(errorDescription);
        onSetStatusBarText(errorDescription.localizedString(), SEC_TO_MSEC(30));
    }
}

void MainWindow::onCurrentNotePdfExportRequested()
{
    QNDEBUG("MainWindow::onCurrentNotePdfExportRequested");

    NoteEditorWidget * pNoteEditorWidget = currentNoteEditorTab();
    if (!pNoteEditorWidget) {
        ErrorString errorDescription(QT_TR_NOOP("Can't export note to pdf: "
                                                "no note editor tabs"));
        QNDEBUG(errorDescription);
        onSetStatusBarText(errorDescription.localizedString(), SEC_TO_MSEC(30));
        return;
    }

    ErrorString errorDescription;
    bool res = pNoteEditorWidget->exportNoteToPdf(errorDescription);
    if (!res)
    {
        if (errorDescription.isEmpty()) {
            return;
        }

        QNDEBUG(errorDescription);
        onSetStatusBarText(errorDescription.localizedString(), SEC_TO_MSEC(30));
    }
}

void MainWindow::onExportNotesToEnexRequested(QStringList noteLocalUids)
{
    QNDEBUG("MainWindow::onExportNotesToEnexRequested: "
            << noteLocalUids.join(QStringLiteral(", ")));

    if (Q_UNLIKELY(noteLocalUids.isEmpty())) {
        QNDEBUG("The list of note local uids to export is empty");
        return;
    }

    if (Q_UNLIKELY(!m_pAccount)) {
        QNDEBUG("No current account, skipping");
        return;
    }

    if (Q_UNLIKELY(!m_pLocalStorageManagerAsync)) {
        QNDEBUG("No local storage manager, skipping");
        return;
    }

    if (Q_UNLIKELY(!m_pNoteEditorTabsAndWindowsCoordinator)) {
        QNDEBUG("No note editor tabs and windows coordinator, skipping");
        return;
    }

    if (Q_UNLIKELY(!m_pTagModel)) {
        QNDEBUG("No tag model, skipping");
        return;
    }

    ApplicationSettings appSettings(*m_pAccount, QUENTIER_UI_SETTINGS);
    appSettings.beginGroup(NOTE_EDITOR_SETTINGS_GROUP_NAME);
    QString lastExportNoteToEnexPath =
        appSettings.value(LAST_EXPORT_NOTE_TO_ENEX_PATH_SETTINGS_KEY).toString();
    appSettings.endGroup();

    if (lastExportNoteToEnexPath.isEmpty()) {
        lastExportNoteToEnexPath = documentsPath();
    }

    QScopedPointer<EnexExportDialog> pExportEnexDialog(
        new EnexExportDialog(*m_pAccount, this));
    pExportEnexDialog->setWindowModality(Qt::WindowModal);
    centerDialog(*pExportEnexDialog);
    if (pExportEnexDialog->exec() != QDialog::Accepted) {
        QNDEBUG("Enex export was not confirmed");
        return;
    }

    QString enexFilePath = pExportEnexDialog->exportEnexFilePath();

    QFileInfo enexFileInfo(enexFilePath);
    if (enexFileInfo.exists())
    {
        if (!enexFileInfo.isWritable()) {
            QNINFO("Chosen ENEX export file is not writable: " << enexFilePath);
            onSetStatusBarText(tr("The file selected for ENEX export is not writable") +
                               QStringLiteral(": ") + enexFilePath);
            return;
        }
    }
    else
    {
        QDir enexFileDir = enexFileInfo.absoluteDir();
        if (!enexFileDir.exists())
        {
            bool res = enexFileDir.mkpath(enexFileInfo.absolutePath());
            if (!res) {
                QNDEBUG("Failed to create folder for the selected ENEX file");
                onSetStatusBarText(tr("Could not create the folder for "
                                      "the selected ENEX file") +
                                   QStringLiteral(": ") + enexFilePath,
                                   SEC_TO_MSEC(30));
                return;
            }
        }
    }

    EnexExporter * pExporter = new EnexExporter(
        *m_pLocalStorageManagerAsync, *m_pNoteEditorTabsAndWindowsCoordinator,
        *m_pTagModel, this);
    pExporter->setTargetEnexFilePath(enexFilePath);
    pExporter->setIncludeTags(pExportEnexDialog->exportTags());
    pExporter->setNoteLocalUids(noteLocalUids);

    QObject::connect(pExporter,
                     QNSIGNAL(EnexExporter,notesExportedToEnex,QString),
                     this,
                     QNSLOT(MainWindow,onExportedNotesToEnex,QString));
    QObject::connect(pExporter,
                     QNSIGNAL(EnexExporter,failedToExportNotesToEnex,ErrorString),
                     this,
                     QNSLOT(MainWindow,onExportNotesToEnexFailed,ErrorString));
    pExporter->start();
}

void MainWindow::onExportedNotesToEnex(QString enex)
{
    QNDEBUG("MainWindow::onExportedNotesToEnex");

    EnexExporter * pExporter = qobject_cast<EnexExporter*>(sender());
    if (Q_UNLIKELY(!pExporter)) {
        ErrorString error(QT_TR_NOOP("Can't export notes to ENEX: internal error, "
                                     "can't cast the slot invoker to EnexExporter"));
        QNWARNING(error);
        onSetStatusBarText(error.localizedString(), SEC_TO_MSEC(30));
        return;
    }

    QString enexFilePath = pExporter->targetEnexFilePath();
    if (Q_UNLIKELY(enexFilePath.isEmpty())) {
        ErrorString error(QT_TR_NOOP("Can't export notes to ENEX: internal error, "
                                     "the selected ENEX file path was lost"));
        QNWARNING(error);
        onSetStatusBarText(error.localizedString(), SEC_TO_MSEC(30));
        return;
    }

    QByteArray enexRawData = enex.toUtf8();

    AsyncFileWriter * pAsyncFileWriter =
        new AsyncFileWriter(enexFilePath, enexRawData);
    QObject::connect(pAsyncFileWriter,
                     QNSIGNAL(AsyncFileWriter,fileSuccessfullyWritten,QString),
                     this,
                     QNSLOT(MainWindow,onEnexFileWrittenSuccessfully,QString));
    QObject::connect(pAsyncFileWriter,
                     QNSIGNAL(AsyncFileWriter,fileWriteFailed,ErrorString),
                     this,
                     QNSLOT(MainWindow,onEnexFileWriteFailed));
    QObject::connect(pAsyncFileWriter,
                     QNSIGNAL(AsyncFileWriter,fileWriteIncomplete,qint64,qint64),
                     this,
                     QNSLOT(MainWindow,onEnexFileWriteIncomplete,qint64,qint64));
    QThreadPool::globalInstance()->start(pAsyncFileWriter);
}

void MainWindow::onExportNotesToEnexFailed(ErrorString errorDescription)
{
    QNDEBUG("MainWindow::onExportNotesToEnexFailed: " << errorDescription);

    EnexExporter * pExporter = qobject_cast<EnexExporter*>(sender());
    if (pExporter) {
        pExporter->clear();
        pExporter->deleteLater();
    }

    onSetStatusBarText(errorDescription.localizedString(), SEC_TO_MSEC(30));
}

void MainWindow::onEnexFileWrittenSuccessfully(QString filePath)
{
    QNDEBUG("MainWindow::onEnexFileWrittenSuccessfully: " << filePath);
    onSetStatusBarText(tr("Successfully exported note(s) to ENEX: ") +
                       QDir::toNativeSeparators(filePath), SEC_TO_MSEC(5));
}

void MainWindow::onEnexFileWriteFailed(ErrorString errorDescription)
{
    QNDEBUG("MainWindow::onEnexFileWriteFailed: " << errorDescription);
    onSetStatusBarText(tr("Can't export note(s) to ENEX, failed to write "
                          "the ENEX to file") +
                       QStringLiteral(": ") + errorDescription.localizedString(),
                       SEC_TO_MSEC(30));
}

void MainWindow::onEnexFileWriteIncomplete(
    qint64 bytesWritten, qint64 bytesTotal)
{
    QNDEBUG("MainWindow::onEnexFileWriteIncomplete: bytes written = "
            << bytesWritten << ", bytes total = " << bytesTotal);

    if (bytesWritten == 0) {
        onSetStatusBarText(tr("Can't export note(s) to ENEX, failed to write "
                              "the ENEX to file"),
                           SEC_TO_MSEC(30));
    }
    else {
        onSetStatusBarText(tr("Can't export note(s) to ENEX, failed to write "
                              "the ENEX to file, only a portion of data has "
                              "been written") +
                           QStringLiteral(": ") + QString::number(bytesWritten) +
                           QStringLiteral("/") + QString::number(bytesTotal),
                           SEC_TO_MSEC(30));
    }
}

void MainWindow::onEnexImportCompletedSuccessfully(QString enexFilePath)
{
    QNDEBUG("MainWindow::onEnexImportCompletedSuccessfully: " << enexFilePath);

    onSetStatusBarText(tr("Successfully imported note(s) from ENEX file") +
                       QStringLiteral(": ") + QDir::toNativeSeparators(enexFilePath),
                       SEC_TO_MSEC(5));

    EnexImporter * pImporter = qobject_cast<EnexImporter*>(sender());
    if (pImporter) {
        pImporter->clear();
        pImporter->deleteLater();
    }
}

void MainWindow::onEnexImportFailed(ErrorString errorDescription)
{
    QNDEBUG("MainWindow::onEnexImportFailed: " << errorDescription);

    onSetStatusBarText(errorDescription.localizedString(), SEC_TO_MSEC(30));

    EnexImporter * pImporter = qobject_cast<EnexImporter*>(sender());
    if (pImporter) {
        pImporter->clear();
        pImporter->deleteLater();
    }
}

void MainWindow::onUseLimitedFontsPreferenceChanged(bool flag)
{
    QNDEBUG("MainWindow::onUseLimitedFontsPreferenceChanged: flag = "
            << (flag ? "enabled" : "disabled"));

    if (m_pNoteEditorTabsAndWindowsCoordinator) {
        m_pNoteEditorTabsAndWindowsCoordinator->setUseLimitedFonts(flag);
    }
}

void MainWindow::onShowNoteThumbnailsPreferenceChanged()
{
    QNDEBUG("MainWindow::onShowNoteThumbnailsPreferenceChanged");

    bool showNoteThumbnails = getShowNoteThumbnailsPreference();
    Q_EMIT showNoteThumbnailsStateChanged(showNoteThumbnails,
                                          getHideNoteThumbnailsFor());

    NoteItemDelegate * pNoteItemDelegate =
        qobject_cast<NoteItemDelegate*>(m_pUI->noteListView->itemDelegate());
    if (Q_UNLIKELY(!pNoteItemDelegate)) {
        QNDEBUG("No NoteItemDelegate");
        return;
    }

    m_pUI->noteListView->update();
}

void MainWindow::onDisableNativeMenuBarPreferenceChanged()
{
    QNDEBUG("MainWindow::onDisableNativeMenuBarPreferenceChanged");
    setupDisableNativeMenuBarPreference();
}

void MainWindow::onRunSyncEachNumMinitesPreferenceChanged(
    int runSyncEachNumMinutes)
{
    QNDEBUG("MainWindow::onRunSyncEachNumMinitesPreferenceChanged: "
            << runSyncEachNumMinutes);

    if (m_runSyncPeriodicallyTimerId != 0) {
        killTimer(m_runSyncPeriodicallyTimerId);
        m_runSyncPeriodicallyTimerId = 0;
    }

    if (runSyncEachNumMinutes <= 0) {
        return;
    }

    if (Q_UNLIKELY(!m_pAccount ||
                   (m_pAccount->type() != Account::Type::Evernote)))
    {
        return;
    }

    m_runSyncPeriodicallyTimerId =
        startTimer(SEC_TO_MSEC(runSyncEachNumMinutes * 60));
}

void MainWindow::onPanelFontColorChanged(QColor color)
{
    QNDEBUG("MainWindow::onPanelFontColorChanged: " << color.name());

    for(auto & pPanelStyleController: m_genericPanelStyleControllers) {
        pPanelStyleController->setOverrideFontColor(color);
    }

    for(auto & pPanelStyleController: m_sidePanelStyleControllers) {
        pPanelStyleController->setOverrideFontColor(color);
    }
}

void MainWindow::onPanelBackgroundColorChanged(QColor color)
{
    QNDEBUG("MainWindow::onPanelBackgroundColorChanged: " << color.name());

    if (Q_UNLIKELY(!m_pAccount)) {
        QNDEBUG("No current account");
        return;
    }

    ApplicationSettings settings(*m_pAccount, QUENTIER_UI_SETTINGS);
    settings.beginGroup(PANEL_COLORS_SETTINGS_GROUP_NAME);
    bool useBackgroundGradient =
        settings.value(PANEL_COLORS_USE_BACKGROUND_GRADIENT_SETTINGS_KEY).toBool();
    settings.endGroup();

    if (useBackgroundGradient) {
        QNDEBUG("Background gradient is used instead of solid color");
        return;
    }

    for(auto & pPanelStyleController: m_genericPanelStyleControllers) {
        pPanelStyleController->setOverrideBackgroundColor(color);
    }

    for(auto & pPanelStyleController: m_sidePanelStyleControllers) {
        pPanelStyleController->setOverrideBackgroundColor(color);
    }
}

void MainWindow::onPanelUseBackgroundGradientSettingChanged(
    bool useBackgroundGradient)
{
    QNDEBUG("MainWindow::onPanelUseBackgroundGradientSettingChanged: "
        << (useBackgroundGradient ? "true" : "false"));

    restorePanelColors();
}

void MainWindow::onPanelBackgroundLinearGradientChanged(QLinearGradient gradient)
{
    QNDEBUG("MainWindow::onPanelBackgroundLinearGradientChanged");

    if (Q_UNLIKELY(!m_pAccount)) {
        QNDEBUG("No current account");
        return;
    }

    ApplicationSettings settings(*m_pAccount, QUENTIER_UI_SETTINGS);
    settings.beginGroup(PANEL_COLORS_SETTINGS_GROUP_NAME);
    bool useBackgroundGradient =
        settings.value(PANEL_COLORS_USE_BACKGROUND_GRADIENT_SETTINGS_KEY).toBool();
    settings.endGroup();

    if (!useBackgroundGradient) {
        QNDEBUG("Background color is used instead of gradient");
        return;
    }

    for(auto & pPanelStyleController: m_genericPanelStyleControllers) {
        pPanelStyleController->setOverrideBackgroundGradient(gradient);
    }

    for(auto & pPanelStyleController: m_sidePanelStyleControllers) {
        pPanelStyleController->setOverrideBackgroundGradient(gradient);
    }
}

void MainWindow::onSaveNoteSearchQueryButtonPressed()
{
    QString searchString = m_pUI->searchQueryLineEdit->text();
    QNDEBUG("MainWindow::onSaveNoteSearchQueryButtonPressed, "
            << "search string = " << searchString);

    QScopedPointer<AddOrEditSavedSearchDialog> pAddSavedSearchDialog(
        new AddOrEditSavedSearchDialog(m_pSavedSearchModel, this));
    pAddSavedSearchDialog->setWindowModality(Qt::WindowModal);
    centerDialog(*pAddSavedSearchDialog);
    pAddSavedSearchDialog->setQuery(searchString);
    Q_UNUSED(pAddSavedSearchDialog->exec())
}

void MainWindow::onNewNoteRequestedFromSystemTrayIcon()
{
    QNDEBUG("MainWindow::onNewNoteRequestedFromSystemTrayIcon");

    Qt::WindowStates state = windowState();
    bool isMinimized = (state & Qt::WindowMinimized);

    bool shown = !isMinimized && !isHidden();

    createNewNote(shown
                  ? NoteEditorTabsAndWindowsCoordinator::NoteEditorMode::Any
                  : NoteEditorTabsAndWindowsCoordinator::NoteEditorMode::Window);
}

void MainWindow::onQuitRequestedFromSystemTrayIcon()
{
    QNINFO("MainWindow::onQuitRequestedFromSystemTrayIcon");
    onQuitAction();
}

void MainWindow::onAccountSwitchRequested(Account account)
{
    QNDEBUG("MainWindow::onAccountSwitchRequested: " << account.name());

    stopListeningForSplitterMoves();
    m_pAccountManager->switchAccount(account);
}

void MainWindow::onSystemTrayIconManagerError(ErrorString errorDescription)
{
    QNDEBUG("MainWindow::onSystemTrayIconManagerError: " << errorDescription);
    onSetStatusBarText(errorDescription.localizedString(), SEC_TO_MSEC(30));
}

void MainWindow::onShowRequestedFromTrayIcon()
{
    QNDEBUG("MainWindow::onShowRequestedFromTrayIcon");
    show();
}

void MainWindow::onHideRequestedFromTrayIcon()
{
    QNDEBUG("MainWindow::onHideRequestedFromTrayIcon");
    hide();
}

void MainWindow::onViewLogsActionTriggered()
{
    LogViewerWidget * pLogViewerWidget = findChild<LogViewerWidget*>();
    if (pLogViewerWidget) {
        return;
    }

    pLogViewerWidget = new LogViewerWidget(this);
    pLogViewerWidget->setAttribute(Qt::WA_DeleteOnClose);
    pLogViewerWidget->show();
}

void MainWindow::onShowInfoAboutQuentierActionTriggered()
{
    QNDEBUG("MainWindow::onShowInfoAboutQuentierActionTriggered");

    AboutQuentierWidget * pWidget = findChild<AboutQuentierWidget*>();
    if (pWidget) {
        pWidget->show();
        pWidget->raise();
        pWidget->setFocus();
        return;
    }

    pWidget = new AboutQuentierWidget(this);
    pWidget->setAttribute(Qt::WA_DeleteOnClose);
    centerWidget(*pWidget);
    pWidget->adjustSize();
    pWidget->show();
}

void MainWindow::onNoteEditorError(ErrorString error)
{
    QNINFO("MainWindow::onNoteEditorError: " << error);
    onSetStatusBarText(error.localizedString(), SEC_TO_MSEC(30));
}

void MainWindow::onModelViewError(ErrorString error)
{
    QNINFO("MainWindow::onModelViewError: " << error);
    onSetStatusBarText(error.localizedString(), SEC_TO_MSEC(30));
}

void MainWindow::onNoteEditorSpellCheckerNotReady()
{
    QNDEBUG("MainWindow::onNoteEditorSpellCheckerNotReady");

    NoteEditorWidget * noteEditor = qobject_cast<NoteEditorWidget*>(sender());
    if (!noteEditor) {
        QNTRACE("Can't cast caller to note editor widget, skipping");
        return;
    }

    NoteEditorWidget * currentEditor = currentNoteEditorTab();
    if (!currentEditor || (currentEditor != noteEditor)) {
        QNTRACE("Not an update from current note editor, skipping");
        return;
    }

    onSetStatusBarText(tr("Spell checker is loading dictionaries, please wait"));
}

void MainWindow::onNoteEditorSpellCheckerReady()
{
    QNDEBUG("MainWindow::onNoteEditorSpellCheckerReady");

    NoteEditorWidget * noteEditor = qobject_cast<NoteEditorWidget*>(sender());
    if (!noteEditor) {
        QNTRACE("Can't cast caller to note editor widget, skipping");
        return;
    }

    NoteEditorWidget * currentEditor = currentNoteEditorTab();
    if (!currentEditor || (currentEditor != noteEditor)) {
        QNTRACE("Not an update from current note editor, skipping");
        return;
    }

    onSetStatusBarText(QString());
}

void MainWindow::onAddAccountActionTriggered(bool checked)
{
    QNDEBUG("MainWindow::onAddAccountActionTriggered");
    Q_UNUSED(checked)
    onNewAccountCreationRequested();
}

void MainWindow::onManageAccountsActionTriggered(bool checked)
{
    QNDEBUG("MainWindow::onManageAccountsActionTriggered");

    Q_UNUSED(checked)
    Q_UNUSED(m_pAccountManager->execManageAccountsDialog());
}

void MainWindow::onSwitchAccountActionToggled(bool checked)
{
    QNDEBUG("MainWindow::onSwitchAccountActionToggled: checked = "
            << (checked ? "true" : "false"));

    if (!checked) {
        QNTRACE("Ignoring the unchecking of account");
        return;
    }

    QAction * action = qobject_cast<QAction*>(sender());
    if (Q_UNLIKELY(!action)) {
        NOTIFY_ERROR(QT_TR_NOOP("Internal error: account switching action is "
                                "unexpectedly null"));
        return;
    }

    QVariant indexData = action->data();
    bool conversionResult = false;
    int index = indexData.toInt(&conversionResult);
    if (Q_UNLIKELY(!conversionResult)) {
        NOTIFY_ERROR(QT_TR_NOOP("Internal error: can't get identification data "
                                "from the account switching action"));
        return;
    }

    const QVector<Account> & availableAccounts =
        m_pAccountManager->availableAccounts();
    const int numAvailableAccounts = availableAccounts.size();

    if ((index < 0) || (index >= numAvailableAccounts)) {
        NOTIFY_ERROR(QT_TR_NOOP("Internal error: wrong index into available "
                                "accounts in account switching action"));
        return;
    }

    if (m_syncInProgress) {
        NOTIFY_DEBUG(QT_TR_NOOP("Can't switch account while the synchronization "
                                "is in progress"));
        return;
    }

    if (m_pendingNewEvernoteAccountAuthentication) {
        NOTIFY_DEBUG(QT_TR_NOOP("Can't switch account while the authentication "
                                "of new Evernote accont is in progress"));
        return;
    }

    if (m_pendingCurrentEvernoteAccountAuthentication) {
        NOTIFY_DEBUG(QT_TR_NOOP("Can't switch account while the authentication "
                                "of Evernote account is in progress"));
        return;
    }

    if (m_pendingSwitchToNewEvernoteAccount) {
        NOTIFY_DEBUG(QT_TR_NOOP("Can't switch account: account switching "
                                "operation is already in progress"));
        return;
    }

    const Account & availableAccount = availableAccounts[index];

    stopListeningForSplitterMoves();
    m_pAccountManager->switchAccount(availableAccount);
    // The continuation is in onAccountSwitched slot connected to AccountManager's
    // switchedAccount signal
}

void MainWindow::onAccountSwitched(Account account)
{
    QNDEBUG("MainWindow::onAccountSwitched: " << account.name());

    if (Q_UNLIKELY(!m_pLocalStorageManagerThread))
    {
        ErrorString errorDescription(QT_TR_NOOP("internal error: no local storage "
                                                "manager thread exists"));
        QNWARNING(errorDescription);
        onSetStatusBarText(tr("Could not switch account: ") + QStringLiteral(": ") +
                           errorDescription.localizedString(),
                           SEC_TO_MSEC(30));
        return;
    }

    if (Q_UNLIKELY(!m_pLocalStorageManagerAsync))
    {
        ErrorString errorDescription(QT_TR_NOOP("internal error: no local storage "
                                                "manager exists"));
        QNWARNING(errorDescription);
        onSetStatusBarText(tr("Could not switch account: ") + QStringLiteral(": ") +
                           errorDescription.localizedString(),
                           SEC_TO_MSEC(30));
        return;
    }

    // This stops SynchronizationManager's thread and moves QEverCloud's
    // QNetworkAccessManager instance to the GUI thread which is required for
    // proper work of OAuth. If the account being switched to is Evernote one,
    // that would be required
    clearSynchronizationManager();

    // Since Qt 5.11 QSqlDatabase opening only works properly from the thread
    // which has loaded the SQL drivers - which is this thread, the GUI one.
    // However, LocalStorageManagerAsync operates in another thread. So need
    // to stop that thread, perform the account switching operation synchronously
    // and then start the stopped thread again. See
    // https://bugreports.qt.io/browse/QTBUG-72545 for reference.

    bool localStorageThreadWasStopped = false;
    if (m_pLocalStorageManagerThread->isRunning())
    {
        QObject::disconnect(m_pLocalStorageManagerThread,
                            QNSIGNAL(QThread,finished),
                            m_pLocalStorageManagerThread,
                            QNSLOT(QThread,deleteLater));
        m_pLocalStorageManagerThread->quit();
        m_pLocalStorageManagerThread->wait();
        localStorageThreadWasStopped = true;
    }

    bool cacheIsUsed =
        (m_pLocalStorageManagerAsync->localStorageCacheManager() != nullptr);
    m_pLocalStorageManagerAsync->setUseCache(false);

    ErrorString errorDescription;
    try {
        m_pLocalStorageManagerAsync->localStorageManager()->switchUser(account);
    }
    catch(const std::exception & e) {
        errorDescription.setBase(QT_TR_NOOP("Can't switch user in the local "
                                            "storage: caught exception"));
        errorDescription.details() = QString::fromUtf8(e.what());
    }

    m_pLocalStorageManagerAsync->setUseCache(cacheIsUsed);

    bool checkRes = true;
    if (errorDescription.isEmpty()) {
        checkRes = checkLocalStorageVersion(account);
    }

    if (localStorageThreadWasStopped) {
        QObject::connect(m_pLocalStorageManagerThread,
                         QNSIGNAL(QThread,finished),
                         m_pLocalStorageManagerThread,
                         QNSLOT(QThread,deleteLater));
        m_pLocalStorageManagerThread->start();
    }

    if (!checkRes) {
        return;
    }

    m_lastLocalStorageSwitchUserRequest = QUuid::createUuid();
    if (errorDescription.isEmpty()) {
        onLocalStorageSwitchUserRequestComplete(
            account, m_lastLocalStorageSwitchUserRequest);
    }
    else {
        onLocalStorageSwitchUserRequestFailed(account, errorDescription,
                                              m_lastLocalStorageSwitchUserRequest);
    }
}

void MainWindow::onAccountUpdated(Account account)
{
    QNDEBUG("MainWindow::onAccountUpdated: " << account.name());

    if (!m_pAccount) {
        QNDEBUG("No account is current at the moment");
        return;
    }

    if (m_pAccount->type() != account.type()) {
        QNDEBUG("Not an update for the current account: it has another type");
        QNTRACE(*m_pAccount);
        return;
    }

    bool isLocal = (m_pAccount->type() == Account::Type::Local);

    if (isLocal && (m_pAccount->name() != account.name())) {
        QNDEBUG("Not an update for the current account: it has another name");
        QNTRACE(*m_pAccount);
        return;
    }

    if (!isLocal &&
        ((m_pAccount->id() != account.id()) || (m_pAccount->name() != account.name())))
    {
        QNDEBUG("Not an update for the current account: either "
                "id or name don't match");
        QNTRACE(*m_pAccount);
        return;
    }

    *m_pAccount = account;
    setWindowTitleForAccount(account);
}

void MainWindow::onAccountAdded(Account account)
{
    QNDEBUG("MainWindow::onAccountAdded: " << account.name());
    updateSubMenuWithAvailableAccounts();
}

void MainWindow::onAccountRemoved(Account account)
{
    QNDEBUG("MainWindow::onAccountRemoved: " << account);
    updateSubMenuWithAvailableAccounts();
}

void MainWindow::onAccountManagerError(ErrorString errorDescription)
{
    QNDEBUG("MainWindow::onAccountManagerError: " << errorDescription);
    onSetStatusBarText(errorDescription.localizedString(), SEC_TO_MSEC(30));
}

void MainWindow::onRevokeAuthentication(qevercloud::UserID userId)
{
    QNDEBUG("MainWindow::onRevokeAuthentication: user id = " << userId);
    Q_EMIT revokeAuthentication(userId);
}

void MainWindow::onShowSidePanelActionToggled(bool checked)
{
    QNDEBUG("MainWindow::onShowSidePanelActionToggled: checked = "
            << (checked ? "true" : "false"));

    ApplicationSettings appSettings(*m_pAccount, QUENTIER_UI_SETTINGS);
    appSettings.beginGroup(QStringLiteral("MainWindow"));
    appSettings.setValue(QStringLiteral("ShowSidePanel"), checked);
    appSettings.endGroup();

    if (checked) {
        m_pUI->sidePanelSplitter->show();
    }
    else {
        m_pUI->sidePanelSplitter->hide();
    }
}

void MainWindow::onShowFavoritesActionToggled(bool checked)
{
    QNDEBUG("MainWindow::onShowFavoritesActionToggled: checked = "
            << (checked ? "true" : "false"));

    ApplicationSettings appSettings(*m_pAccount, QUENTIER_UI_SETTINGS);
    appSettings.beginGroup(QStringLiteral("MainWindow"));
    appSettings.setValue(QStringLiteral("ShowFavorites"), checked);
    appSettings.endGroup();

    if (checked) {
        m_pUI->favoritesWidget->show();
    }
    else {
        m_pUI->favoritesWidget->hide();
    }
}

void MainWindow::onShowNotebooksActionToggled(bool checked)
{
    QNDEBUG("MainWindow::onShowNotebooksActionToggled: checked = "
            << (checked ? "true" : "false"));

    ApplicationSettings appSettings(*m_pAccount, QUENTIER_UI_SETTINGS);
    appSettings.beginGroup(QStringLiteral("MainWindow"));
    appSettings.setValue(QStringLiteral("ShowNotebooks"), checked);
    appSettings.endGroup();

    if (checked) {
        m_pUI->notebooksWidget->show();
    }
    else {
        m_pUI->notebooksWidget->hide();
    }
}

void MainWindow::onShowTagsActionToggled(bool checked)
{
    QNDEBUG("MainWindow::onShowTagsActionToggled: checked = "
            << (checked ? "true" : "false"));

    ApplicationSettings appSettings(*m_pAccount, QUENTIER_UI_SETTINGS);
    appSettings.beginGroup(QStringLiteral("MainWindow"));
    appSettings.setValue(QStringLiteral("ShowTags"), checked);
    appSettings.endGroup();

    if (checked) {
        m_pUI->tagsWidget->show();
    }
    else {
        m_pUI->tagsWidget->hide();
    }
}

void MainWindow::onShowSavedSearchesActionToggled(bool checked)
{
    QNDEBUG("MainWindow::onShowSavedSearchesActionToggled: checked = "
            << (checked ? "true" : "false"));

    ApplicationSettings appSettings(*m_pAccount, QUENTIER_UI_SETTINGS);
    appSettings.beginGroup(QStringLiteral("MainWindow"));
    appSettings.setValue(QStringLiteral("ShowSavedSearches"), checked);
    appSettings.endGroup();

    if (checked) {
        m_pUI->savedSearchesWidget->show();
    }
    else {
        m_pUI->savedSearchesWidget->hide();
    }
}

void MainWindow::onShowDeletedNotesActionToggled(bool checked)
{
    QNDEBUG("MainWindow::onShowDeletedNotesActionToggled: checked = "
            << (checked ? "true" : "false"));

    ApplicationSettings appSettings(*m_pAccount, QUENTIER_UI_SETTINGS);
    appSettings.beginGroup(QStringLiteral("MainWindow"));
    appSettings.setValue(QStringLiteral("ShowDeletedNotes"), checked);
    appSettings.endGroup();

    if (checked) {
        m_pUI->deletedNotesWidget->show();
    }
    else {
        m_pUI->deletedNotesWidget->hide();
    }
}

void MainWindow::onShowNoteListActionToggled(bool checked)
{
    QNDEBUG("MainWindow::onShowNoteListActionToggled: checked = "
            << (checked ? "true" : "false"));

    ApplicationSettings appSettings(*m_pAccount, QUENTIER_UI_SETTINGS);
    appSettings.beginGroup(QStringLiteral("MainWindow"));
    appSettings.setValue(QStringLiteral("ShowNotesList"), checked);
    appSettings.endGroup();

    if (checked) {
        m_pUI->noteListView->setModel(m_pNoteModel);
        m_pUI->notesListAndFiltersFrame->show();
    }
    else {
        m_pUI->notesListAndFiltersFrame->hide();
        m_pUI->noteListView->setModel(&m_blankModel);
    }
}

void MainWindow::onShowToolbarActionToggled(bool checked)
{
    QNDEBUG("MainWindow::onShowToolbarActionToggled: checked = "
            << (checked ? "true" : "false"));

    ApplicationSettings appSettings(*m_pAccount, QUENTIER_UI_SETTINGS);
    appSettings.beginGroup(QStringLiteral("MainWindow"));
    appSettings.setValue(QStringLiteral("ShowToolbar"), checked);
    appSettings.endGroup();

    if (checked) {
        m_pUI->upperBarGenericPanel->show();
    }
    else {
        m_pUI->upperBarGenericPanel->hide();
    }
}

void MainWindow::onShowStatusBarActionToggled(bool checked)
{
    QNDEBUG("MainWindow::onShowStatusBarActionToggled: checked = "
            << (checked ? "true" : "false"));

    ApplicationSettings appSettings(*m_pAccount, QUENTIER_UI_SETTINGS);
    appSettings.beginGroup(QStringLiteral("MainWindow"));
    appSettings.setValue(QStringLiteral("ShowStatusBar"), checked);
    appSettings.endGroup();

    if (checked) {
        m_pUI->statusBar->show();
    }
    else {
        m_pUI->statusBar->hide();
    }
}

void MainWindow::onSwitchIconTheme(const QString & iconTheme)
{
    QNDEBUG("MainWindow::onSwitchIconTheme: " << iconTheme);

    if (iconTheme == tr("Native")) {
        onSwitchIconThemeToNativeAction();
    }
    else if (iconTheme == QStringLiteral("breeze")) {
        onSwitchIconThemeToBreezeAction();
    }
    else if (iconTheme == QStringLiteral("breeze-dark")) {
        onSwitchIconThemeToBreezeDarkAction();
    }
    else if (iconTheme == QStringLiteral("oxygen")) {
        onSwitchIconThemeToOxygenAction();
    }
    else if (iconTheme == QStringLiteral("tango")) {
        onSwitchIconThemeToTangoAction();
    }
    else {
        ErrorString error(QT_TR_NOOP("Unknown icon theme selected"));
        error.details() = iconTheme;
        QNWARNING(error);
        onSetStatusBarText(error.localizedString(), SEC_TO_MSEC(30));
    }
}

void MainWindow::onSwitchIconThemeToNativeAction()
{
    QNDEBUG("MainWindow::onSwitchIconThemeToNativeAction");

    if (m_nativeIconThemeName.isEmpty()) {
        ErrorString error(QT_TR_NOOP("No native icon theme is available"));
        QNDEBUG(error);
        onSetStatusBarText(error.localizedString(), SEC_TO_MSEC(30));
        return;
    }

    if (QIcon::themeName() == m_nativeIconThemeName) {
        ErrorString error(QT_TR_NOOP("Already using the native icon theme"));
        QNDEBUG(error);
        onSetStatusBarText(error.localizedString(), SEC_TO_MSEC(30));
        return;
    }

    QIcon::setThemeName(m_nativeIconThemeName);
    persistChosenIconTheme(m_nativeIconThemeName);
    refreshChildWidgetsThemeIcons();
}

void MainWindow::onSwitchIconThemeToTangoAction()
{
    QNDEBUG("MainWindow::onSwitchIconThemeToTangoAction");

    QString tango = QStringLiteral("tango");

    if (QIcon::themeName() == tango) {
        ErrorString error(QT_TR_NOOP("Already using tango icon theme"));
        QNDEBUG(error);
        onSetStatusBarText(error.localizedString(), SEC_TO_MSEC(30));
        return;
    }

    QIcon::setThemeName(tango);
    persistChosenIconTheme(tango);
    refreshChildWidgetsThemeIcons();
}

void MainWindow::onSwitchIconThemeToOxygenAction()
{
    QNDEBUG("MainWindow::onSwitchIconThemeToOxygenAction");

    QString oxygen = QStringLiteral("oxygen");

    if (QIcon::themeName() == oxygen) {
        ErrorString error(QT_TR_NOOP("Already using oxygen icon theme"));
        QNDEBUG(error);
        onSetStatusBarText(error.localizedString(), SEC_TO_MSEC(10));
        return;
    }

    QIcon::setThemeName(oxygen);
    persistChosenIconTheme(oxygen);
    refreshChildWidgetsThemeIcons();
}

void MainWindow::onSwitchIconThemeToBreezeAction()
{
    QNDEBUG(QStringLiteral("MainWindow::onSwitchIconThemeToBreezeAction"));

    QString breeze = QStringLiteral("breeze");

    if (QIcon::themeName() == breeze) {
        ErrorString error(QT_TR_NOOP("Already using breeze icon theme"));
        QNDEBUG(error);
        onSetStatusBarText(error.localizedString(), SEC_TO_MSEC(10));
        return;
    }

    QIcon::setThemeName(breeze);
    persistChosenIconTheme(breeze);
    refreshChildWidgetsThemeIcons();
}

void MainWindow::onSwitchIconThemeToBreezeDarkAction()
{
    QNDEBUG(QStringLiteral("MainWindow::onSwitchIconThemeToBreezeDarkAction"));

    QString breezeDark = QStringLiteral("breeze-dark");

    if (QIcon::themeName() == breezeDark) {
        ErrorString error(QT_TR_NOOP("Already using breeze-dark icon theme"));
        QNDEBUG(error);
        onSetStatusBarText(error.localizedString(), SEC_TO_MSEC(10));
        return;
    }

    QIcon::setThemeName(breezeDark);
    persistChosenIconTheme(breezeDark);
    refreshChildWidgetsThemeIcons();
}

void MainWindow::onLocalStorageSwitchUserRequestComplete(
    Account account, QUuid requestId)
{
    QNDEBUG("MainWindow::onLocalStorageSwitchUserRequestComplete: "
            << "account = " << account.name()
            << ", request id = " << requestId);
    QNTRACE(account);

    bool expected = (m_lastLocalStorageSwitchUserRequest == requestId);
    m_lastLocalStorageSwitchUserRequest = QUuid();

    bool wasPendingSwitchToNewEvernoteAccount = m_pendingSwitchToNewEvernoteAccount;
    m_pendingSwitchToNewEvernoteAccount = false;

    if (!expected) {
        NOTIFY_ERROR(QT_TR_NOOP("Local storage user was switched without explicit "
                                "user action"));
        // Trying to undo it
        // This should trigger the switch in local storage as well
        m_pAccountManager->switchAccount(*m_pAccount);
        startListeningForSplitterMoves();
        return;
    }

    m_notebookCache.clear();
    m_tagCache.clear();
    m_savedSearchCache.clear();
    m_noteCache.clear();

    if (m_geometryAndStatePersistingDelayTimerId != 0) {
        killTimer(m_geometryAndStatePersistingDelayTimerId);
    }
    m_geometryAndStatePersistingDelayTimerId = 0;

    if (m_splitterSizesRestorationDelayTimerId != 0) {
        killTimer(m_splitterSizesRestorationDelayTimerId);
    }
    m_splitterSizesRestorationDelayTimerId = 0;

    *m_pAccount = account;
    setWindowTitleForAccount(account);

    stopListeningForShortcutChanges();
    setupDefaultShortcuts();
    setupUserShortcuts();
    startListeningForShortcutChanges();

    restoreNetworkProxySettingsForAccount(*m_pAccount);

    if (m_pAccount->type() == Account::Type::Local) {
        clearSynchronizationManager();
    }
    else {
        m_synchronizationManagerHost.clear();
        setupSynchronizationManager(SetAccountOption::Set);
        setSynchronizationOptions(*m_pAccount);
    }

    setupModels();

    if (m_pNoteEditorTabsAndWindowsCoordinator) {
        m_pNoteEditorTabsAndWindowsCoordinator->switchAccount(*m_pAccount,
                                                              *m_pTagModel);
    }

    m_pUI->filterByNotebooksWidget->switchAccount(*m_pAccount, m_pNotebookModel);
    m_pUI->filterByTagsWidget->switchAccount(*m_pAccount, m_pTagModel);
    m_pUI->filterBySavedSearchComboBox->switchAccount(*m_pAccount,
                                                      m_pSavedSearchModel);

    setupViews();
    setupAccountSpecificUiElements();

    // FIXME: this can be done more lightweight: just set the current account
    // in the already filled list
    updateSubMenuWithAvailableAccounts();

    restoreGeometryAndState();
    restorePanelColors();

    if (m_pAccount->type() != Account::Type::Evernote) {
        QNTRACE("Not an Evernote account, no need to bother setting up sync");
        return;
    }

    // TODO: should also start the sync if the corresponding setting is set
    // to sync stuff when one switches to the Evernote account
    if (!wasPendingSwitchToNewEvernoteAccount) {
        QNTRACE("Not an account switch after authenticating "
                "new Evernote account");
        return;
    }

    // For new Evernote account is is convenient if the first note to be synchronized
    // automatically opens in the note editor
    m_pUI->noteListView->setAutoSelectNoteOnNextAddition();

    if (Q_UNLIKELY(!m_pSynchronizationManager)) {
        QNWARNING("Detected unexpectedly missing "
                  "SynchronizationManager, trying to workaround");
        setupSynchronizationManager();
    }

    setupSynchronizationManagerThread();
    m_authenticatedCurrentEvernoteAccount = true;
    launchSynchronization();
}

void MainWindow::onLocalStorageSwitchUserRequestFailed(
    Account account, ErrorString errorDescription, QUuid requestId)
{
    bool expected = (m_lastLocalStorageSwitchUserRequest == requestId);
    if (!expected) {
        return;
    }

    QNDEBUG("MainWindow::onLocalStorageSwitchUserRequestFailed: "
            << account.name() << "\nError description: "
            << errorDescription << ", request id = " << requestId);
    QNTRACE(account);

    m_lastLocalStorageSwitchUserRequest = QUuid();

    onSetStatusBarText(tr("Could not switch account") + QStringLiteral(": ") +
                       errorDescription.localizedString(),
                       SEC_TO_MSEC(30));

    if (!m_pAccount) {
        // If there was no any account set previously, nothing to do
        return;
    }

    restoreNetworkProxySettingsForAccount(*m_pAccount);
    startListeningForSplitterMoves();

    const QVector<Account> & availableAccounts =
        m_pAccountManager->availableAccounts();
    const int numAvailableAccounts = availableAccounts.size();

    // Trying to restore the previously selected account as the current one in the UI
    QList<QAction*> availableAccountActions =
        m_pAvailableAccountsActionGroup->actions();
    for(auto it = availableAccountActions.constBegin(),
        end = availableAccountActions.constEnd(); it != end; ++it)
    {
        QAction * action = *it;
        if (Q_UNLIKELY(!action)) {
            QNDEBUG("Found null pointer to action within "
                    "the available accounts action group");
            continue;
        }

        QVariant actionData = action->data();
        bool conversionResult = false;
        int index = actionData.toInt(&conversionResult);
        if (Q_UNLIKELY(!conversionResult)) {
            QNDEBUG("Can't convert available account's user data to int: "
                    << actionData);
            continue;
        }

        if (Q_UNLIKELY((index < 0) || (index >= numAvailableAccounts))) {
            QNDEBUG("Available account's index is beyond the range "
                    << "of available accounts: index = "
                    << index << ", num available accounts = "
                    << numAvailableAccounts);
            continue;
        }

        const Account & actionAccount = availableAccounts.at(index);
        if (actionAccount == *m_pAccount) {
            QNDEBUG("Restoring the current account in UI: index = "
                    << index << ", account = " << actionAccount);
            action->setChecked(true);
            return;
        }
    }

    // If we got here, it means we haven't found the proper previous account
    QNDEBUG("Couldn't find the action corresponding to the previous "
            << "available account: " << *m_pAccount);
}

void MainWindow::onSplitterHandleMoved(int pos, int index)
{
    QNDEBUG("MainWindow::onSplitterHandleMoved: pos = " << pos
            << ", index = " << index);

    scheduleGeometryAndStatePersisting();
}

void MainWindow::onSidePanelSplittedHandleMoved(int pos, int index)
{
    QNDEBUG("MainWindow::onSidePanelSplittedHandleMoved: pos = "
            << pos << ", index = " << index);

    scheduleGeometryAndStatePersisting();
}

void MainWindow::onSyncButtonPressed()
{
    QNDEBUG("MainWindow::onSyncButtonPressed");

    if (Q_UNLIKELY(!m_pAccount)) {
        QNDEBUG("Ignoring the sync button click - no account is set");
        return;
    }

    if (Q_UNLIKELY(m_pAccount->type() == Account::Type::Local)) {
        QNDEBUG("The current account is of local type, won't do "
                "anything on attempt to sync it");
        return;
    }

    if (m_pNoteEditorTabsAndWindowsCoordinator) {
        m_pNoteEditorTabsAndWindowsCoordinator->saveAllNoteEditorsContents();
    }

    if (m_syncInProgress) {
        QNDEBUG("The synchronization is in progress, will stop it");
        Q_EMIT stopSynchronization();
    }
    else {
        launchSynchronization();
    }
}

void MainWindow::onAnimatedSyncIconFrameChanged(int frame)
{
    Q_UNUSED(frame)
    m_pUI->syncPushButton->setIcon(QIcon(m_animatedSyncButtonIcon.currentPixmap()));
}

void MainWindow::onAnimatedSyncIconFrameChangedPendingFinish(int frame)
{
    if ((frame == 0) || (frame == (m_animatedSyncButtonIcon.frameCount() - 1))) {
        stopSyncButtonAnimation();
    }
}

void MainWindow::onSyncIconAnimationFinished()
{
    QNDEBUG("MainWindow::onSyncIconAnimationFinished");

    QObject::disconnect(&m_animatedSyncButtonIcon, QNSIGNAL(QMovie,finished),
                        this, QNSLOT(MainWindow,onSyncIconAnimationFinished));
    QObject::disconnect(&m_animatedSyncButtonIcon,
                        QNSIGNAL(QMovie,frameChanged,int),
                        this,
                        QNSLOT(MainWindow,
                               onAnimatedSyncIconFrameChangedPendingFinish,int));

    stopSyncButtonAnimation();
}

void MainWindow::onNewAccountCreationRequested()
{
    QNDEBUG("MainWindow::onNewAccountCreationRequested");

    int res = m_pAccountManager->execAddAccountDialog();
    if (res == QDialog::Accepted) {
        return;
    }

    if (Q_UNLIKELY(!m_pLocalStorageManagerAsync)) {
        QNWARNING("Local storage manager async unexpectedly "
                  "doesn't exist, can't check local storage version");
        return;
    }

    bool localStorageThreadWasStopped = false;
    if (m_pLocalStorageManagerThread &&
        m_pLocalStorageManagerThread->isRunning())
    {
        QObject::disconnect(m_pLocalStorageManagerThread,
                            QNSIGNAL(QThread,finished),
                            m_pLocalStorageManagerThread,
                            QNSLOT(QThread,deleteLater));
        m_pLocalStorageManagerThread->quit();
        m_pLocalStorageManagerThread->wait();
        localStorageThreadWasStopped = true;
    }

    Q_UNUSED(checkLocalStorageVersion(*m_pAccount))

    if (localStorageThreadWasStopped)
    {
        QObject::connect(m_pLocalStorageManagerThread,
                         QNSIGNAL(QThread,finished),
                         m_pLocalStorageManagerThread,
                         QNSLOT(QThread,deleteLater));
        m_pLocalStorageManagerThread->start();
    }
}

void MainWindow::onQuitAction()
{
    QNDEBUG("MainWindow::onQuitAction");

    if (m_pNoteEditorTabsAndWindowsCoordinator) {
        // That would save the modified notes
        m_pNoteEditorTabsAndWindowsCoordinator->clear();
    }

    qApp->quit();
}

void MainWindow::onShortcutChanged(
    int key, QKeySequence shortcut, const Account & account, QString context)
{
    QNDEBUG("MainWindow::onShortcutChanged: key = " << key
            << ", shortcut: " << shortcut.toString(QKeySequence::PortableText)
            << ", context: " << context
            << ", account: " << account.name());

    auto it = m_shortcutKeyToAction.find(key);
    if (it == m_shortcutKeyToAction.end()) {
        QNDEBUG("Haven't found the action corresponding to the shortcut key");
        return;
    }

    QAction * pAction = it.value();
    pAction->setShortcut(shortcut);
    QNDEBUG("Updated shortcut for action " << pAction->text()
            << " (" << pAction->objectName() << ")");
}

void MainWindow::onNonStandardShortcutChanged(
    QString nonStandardKey, QKeySequence shortcut, const Account & account,
    QString context)
{
    QNDEBUG("MainWindow::onNonStandardShortcutChanged: non-standard key = "
            << nonStandardKey << ", shortcut: "
            << shortcut.toString(QKeySequence::PortableText)
            << ", context: " << context << ", account: " << account.name());

    auto it = m_nonStandardShortcutKeyToAction.find(nonStandardKey);
    if (it == m_nonStandardShortcutKeyToAction.end()) {
        QNDEBUG("Haven't found the action corresponding to "
                "the non-standard shortcut key");
        return;
    }

    QAction * pAction = it.value();
    pAction->setShortcut(shortcut);
    QNDEBUG("Updated shortcut for action " << pAction->text()
            << " (" << pAction->objectName() << ")");
}

void MainWindow::onDefaultAccountFirstNotebookAndNoteCreatorFinished(
    QString createdNoteLocalUid)
{
    QNDEBUG("MainWindow::"
            << "onDefaultAccountFirstNotebookAndNoteCreatorFinished: "
            << "created note local uid = " << createdNoteLocalUid);

    DefaultAccountFirstNotebookAndNoteCreator * pDefaultAccountFirstNotebookAndNoteCreator =
        qobject_cast<DefaultAccountFirstNotebookAndNoteCreator*>(sender());
    if (pDefaultAccountFirstNotebookAndNoteCreator) {
        pDefaultAccountFirstNotebookAndNoteCreator->deleteLater();
    }

    bool foundNoteModelItem = false;
    if (m_pNoteModel)
    {
        QModelIndex index = m_pNoteModel->indexForLocalUid(createdNoteLocalUid);
        if (index.isValid()) {
            foundNoteModelItem = true;
        }
    }

    if (foundNoteModelItem) {
        m_pUI->noteListView->setCurrentNoteByLocalUid(createdNoteLocalUid);
        return;
    }

    // If we got here, the just created note hasn't got to the note model yet;
    // in the ideal world should subscribe to note model's insert signal
    // but as a shortcut will just introduce a small delay in the hope the note
    // would have enough time to get from local storage into the model
    m_defaultAccountFirstNoteLocalUid = createdNoteLocalUid;
    m_setDefaultAccountsFirstNoteAsCurrentDelayTimerId = startTimer(100);
}

void MainWindow::onDefaultAccountFirstNotebookAndNoteCreatorError(
    ErrorString errorDescription)
{
    QNDEBUG("MainWindow::"
            << "onDefaultAccountFirstNotebookAndNoteCreatorError: "
            << errorDescription);

    onSetStatusBarText(errorDescription.localizedString());

    DefaultAccountFirstNotebookAndNoteCreator * pDefaultAccountFirstNotebookAndNoteCreator =
        qobject_cast<DefaultAccountFirstNotebookAndNoteCreator*>(sender());
    if (pDefaultAccountFirstNotebookAndNoteCreator) {
        pDefaultAccountFirstNotebookAndNoteCreator->deleteLater();
    }
}

void MainWindow::resizeEvent(QResizeEvent * pEvent)
{
    QMainWindow::resizeEvent(pEvent);

    if (m_pUI->filterBodyFrame->isHidden()) {
        // NOTE: without this the splitter seems to take a wrong guess about
        // the size of note filters header panel and that doesn't look good
        adjustNoteListAndFiltersSplitterSizes();
    }

    scheduleGeometryAndStatePersisting();
}

void MainWindow::closeEvent(QCloseEvent * pEvent)
{
    QNDEBUG("MainWindow::closeEvent");
    if (m_pendingFirstShutdownDialog) {
        QNDEBUG("About to display FirstShutdownDialog");
        m_pendingFirstShutdownDialog = false;

        QScopedPointer<FirstShutdownDialog> pDialog(new FirstShutdownDialog(this));
        pDialog->setWindowModality(Qt::WindowModal);
        centerDialog(*pDialog);
        bool shouldCloseToSystemTray = (pDialog->exec() == QDialog::Accepted);
        m_pSystemTrayIconManager->setPreferenceCloseToSystemTray(shouldCloseToSystemTray);
    }

    if (pEvent && m_pSystemTrayIconManager->shouldCloseToSystemTray()) {
        QNINFO("Hiding to system tray instead of closing");
        pEvent->ignore();
        hide();
        return;
    }

    if (m_pNoteEditorTabsAndWindowsCoordinator) {
        m_pNoteEditorTabsAndWindowsCoordinator->clear();
    }

    persistGeometryAndState();
    QNINFO("Closing application");
    QMainWindow::closeEvent(pEvent);
    onQuitAction();
}

void MainWindow::timerEvent(QTimerEvent * pTimerEvent)
{
    QNDEBUG("MainWindow::timerEvent: timer id = "
            << (pTimerEvent
                ? QString::number(pTimerEvent->timerId())
                : QStringLiteral("<null>")));

    if (Q_UNLIKELY(!pTimerEvent)) {
        return;
    }

    if (pTimerEvent->timerId() == m_geometryAndStatePersistingDelayTimerId)
    {
        persistGeometryAndState();
        killTimer(m_geometryAndStatePersistingDelayTimerId);
        m_geometryAndStatePersistingDelayTimerId = 0;
    }
    else if (pTimerEvent->timerId() == m_splitterSizesRestorationDelayTimerId)
    {
        restoreSplitterSizes();
        startListeningForSplitterMoves();
        killTimer(m_splitterSizesRestorationDelayTimerId);
        m_splitterSizesRestorationDelayTimerId = 0;
    }
    else if (pTimerEvent->timerId() == m_runSyncPeriodicallyTimerId)
    {
        if (Q_UNLIKELY(!m_pAccount ||
                       (m_pAccount->type() != Account::Type::Evernote) ||
                       !m_pSynchronizationManager))
        {
            QNDEBUG("Non-Evernote account is being used: "
                    << (m_pAccount
                        ? m_pAccount->toString()
                        : QStringLiteral("<null>")));
            killTimer(m_runSyncPeriodicallyTimerId);
            m_runSyncPeriodicallyTimerId = 0;
            return;
        }

        if (m_syncInProgress ||
            m_pendingNewEvernoteAccountAuthentication ||
            m_pendingCurrentEvernoteAccountAuthentication ||
            m_pendingSwitchToNewEvernoteAccount ||
            m_syncApiRateLimitExceeded)
        {
            QNDEBUG("Sync in progress = "
                    << (m_syncInProgress ? "true" : "false")
                    << ", pending new Evernote account authentication = "
                    << (m_pendingNewEvernoteAccountAuthentication
                        ? "true"
                        : "false")
                    << ", pending current Evernote account authentication = "
                    << (m_pendingCurrentEvernoteAccountAuthentication
                        ? "true"
                        : "false")
                    << ", pending switch to new Evernote account = "
                    << (m_pendingSwitchToNewEvernoteAccount
                        ? "true"
                        : "false")
                    << ", sync API rate limit exceeded = "
                    << (m_syncApiRateLimitExceeded
                        ? "true"
                        : "false"));
            return;
        }

        QNDEBUG("Starting the periodically run sync");
        launchSynchronization();
    }
    else if (pTimerEvent->timerId() ==
             m_setDefaultAccountsFirstNoteAsCurrentDelayTimerId)
    {
        QNDEBUG("Executing postponed setting of defaut account's "
                "first note as the current note");

        m_pUI->noteListView->setCurrentNoteByLocalUid(
            m_defaultAccountFirstNoteLocalUid);

        m_defaultAccountFirstNoteLocalUid.clear();
        killTimer(m_setDefaultAccountsFirstNoteAsCurrentDelayTimerId);
        m_setDefaultAccountsFirstNoteAsCurrentDelayTimerId = 0;
    }
}

void MainWindow::focusInEvent(QFocusEvent * pFocusEvent)
{
    QNDEBUG("MainWindow::focusInEvent");

    if (Q_UNLIKELY(!pFocusEvent)) {
        return;
    }

    QNDEBUG("Reason = " << pFocusEvent->reason());

    QMainWindow::focusInEvent(pFocusEvent);

    NoteEditorWidget * pCurrentNoteEditorTab = currentNoteEditorTab();
    if (pCurrentNoteEditorTab) {
        pCurrentNoteEditorTab->setFocusToEditor();
    }
}

void MainWindow::focusOutEvent(QFocusEvent * pFocusEvent)
{
    QNDEBUG("MainWindow::focusOutEvent");

    if (Q_UNLIKELY(!pFocusEvent)) {
        return;
    }

    QNDEBUG("Reason = " << pFocusEvent->reason());
    QMainWindow::focusOutEvent(pFocusEvent);
}

void MainWindow::showEvent(QShowEvent * pShowEvent)
{
    QNDEBUG("MainWindow::showEvent");
    QMainWindow::showEvent(pShowEvent);

    Qt::WindowStates state = windowState();
    if (!(state & Qt::WindowMinimized)) {
        Q_EMIT shown();
    }
}

void MainWindow::hideEvent(QHideEvent * pHideEvent)
{
    QNDEBUG("MainWindow::hideEvent");

    QMainWindow::hideEvent(pHideEvent);
    Q_EMIT hidden();
}

void MainWindow::changeEvent(QEvent * pEvent)
{
    QMainWindow::changeEvent(pEvent);

    if (pEvent && (pEvent->type() == QEvent::WindowStateChange))
    {
        Qt::WindowStates state = windowState();
        bool minimized = (state & Qt::WindowMinimized);
        bool maximized = (state & Qt::WindowMaximized);

        QNDEBUG("Change event of window state change type: "
                << "minimized = " << (minimized ? "true" : "false")
                << ", maximized = " << (maximized ? "true" : "false"));

        if (!minimized)
        {
            QNDEBUG("MainWindow is no longer minimized");

            if (isVisible()) {
                Q_EMIT shown();
            }

            scheduleGeometryAndStatePersisting();
        }
        else if (minimized)
        {
            QNDEBUG("MainWindow became minimized");

            if (m_pSystemTrayIconManager->shouldMinimizeToSystemTray())
            {
                QNDEBUG("Should minimize to system tray instead "
                        "of the conventional minimization");

                // 1) Undo the already changed window state
                state = state & (~Qt::WindowMinimized);
                setWindowState(state);

                // 2) hide instead of minimizing
                hide();

                return;
            }

            Q_EMIT hidden();
        }
    }
}

void MainWindow::centerWidget(QWidget & widget)
{
    // Center the widget relative to the main window
    const QRect & geometryRect = geometry();
    const QRect & widgetGeometryRect = widget.geometry();
    if (geometryRect.isValid() && widgetGeometryRect.isValid()) {
        const QPoint center = geometryRect.center();
        int x = center.x() - widgetGeometryRect.width() / 2;
        int y = center.y() - widgetGeometryRect.height() / 2;
        widget.move(x, y);
    }
}

void MainWindow::centerDialog(QDialog & dialog)
{
#ifndef Q_OS_MAC
    centerWidget(dialog);
#else
    Q_UNUSED(dialog)
#endif
}

void MainWindow::setupThemeIcons()
{
    QNTRACE("MainWindow::setupThemeIcons");

    m_nativeIconThemeName = QIcon::themeName();
    QNDEBUG("Native icon theme name: " << m_nativeIconThemeName);

    if (!QIcon::hasThemeIcon(QStringLiteral("document-new"))) {
        QNDEBUG("There seems to be no native icon theme available: "
                "document-new icon is not present within the theme");
        m_nativeIconThemeName.clear();
    }

    ApplicationSettings appSettings(*m_pAccount, QUENTIER_UI_SETTINGS);
    appSettings.beginGroup(LOOK_AND_FEEL_SETTINGS_GROUP_NAME);
    QString lastUsedIconThemeName = appSettings.value(ICON_THEME_SETTINGS_KEY).toString();
    appSettings.endGroup();

    QString iconThemeName;
    if (!lastUsedIconThemeName.isEmpty())
    {
        QNDEBUG("Last chosen icon theme: " << lastUsedIconThemeName);
        iconThemeName = lastUsedIconThemeName;
    }
    else
    {
        QNDEBUG("No last chosen icon theme");

        if (!m_nativeIconThemeName.isEmpty()) {
            QNDEBUG("Using native icon theme: " << m_nativeIconThemeName);
            iconThemeName = m_nativeIconThemeName;
        }
        else {
            iconThemeName = fallbackIconThemeName();
            QNDEBUG("Using fallback icon theme: " << iconThemeName);
        }
    }

    QIcon::setThemeName(iconThemeName);
}

void MainWindow::setupAccountManager()
{
    QNDEBUG("MainWindow::setupAccountManager");

    QObject::connect(m_pAccountManager,
                     QNSIGNAL(AccountManager,
                              evernoteAccountAuthenticationRequested,
                              QString,QNetworkProxy),
                     this,
                     QNSLOT(MainWindow,onEvernoteAccountAuthenticationRequested,
                            QString,QNetworkProxy));
    QObject::connect(m_pAccountManager,
                     QNSIGNAL(AccountManager,switchedAccount,Account),
                     this,
                     QNSLOT(MainWindow,onAccountSwitched,Account));
    QObject::connect(m_pAccountManager,
                     QNSIGNAL(AccountManager,accountUpdated,Account),
                     this,
                     QNSLOT(MainWindow,onAccountUpdated,Account));
    QObject::connect(m_pAccountManager,
                     QNSIGNAL(AccountManager,accountAdded,Account),
                     this,
                     QNSLOT(MainWindow,onAccountAdded,Account));
    QObject::connect(m_pAccountManager,
                     QNSIGNAL(AccountManager,accountRemoved,Account),
                     this,
                     QNSLOT(MainWindow,onAccountRemoved,Account));
    QObject::connect(m_pAccountManager,
                     QNSIGNAL(AccountManager,notifyError,ErrorString),
                     this,
                     QNSLOT(MainWindow,onAccountManagerError,ErrorString));
    QObject::connect(m_pAccountManager,
                     QNSIGNAL(AccountManager,revokeAuthentication,qevercloud::UserID),
                     this,
                     QNSLOT(MainWindow,onRevokeAuthentication,qevercloud::UserID));
}

void MainWindow::setupLocalStorageManager()
{
    QNDEBUG("MainWindow::setupLocalStorageManager");

    m_pLocalStorageManagerThread = new QThread;
    m_pLocalStorageManagerThread->setObjectName(
        QStringLiteral("LocalStorageManagerThread"));
    QObject::connect(m_pLocalStorageManagerThread, QNSIGNAL(QThread,finished),
                     m_pLocalStorageManagerThread, QNSLOT(QThread,deleteLater));
    m_pLocalStorageManagerThread->start();

    m_pLocalStorageManagerAsync = new LocalStorageManagerAsync(
        *m_pAccount,
        LocalStorageManager::StartupOptions(0));

    m_pLocalStorageManagerAsync->init();

    ErrorString errorDescription;
    auto & localStorageManager = *m_pLocalStorageManagerAsync->localStorageManager();
    if (localStorageManager.isLocalStorageVersionTooHigh(errorDescription)) {
        throw quentier::LocalStorageVersionTooHighException(errorDescription);
    }

    QVector<QSharedPointer<ILocalStoragePatch> > localStoragePatches =
        localStorageManager.requiredLocalStoragePatches();
    if (!localStoragePatches.isEmpty())
    {
        QNDEBUG("Local storage requires upgrade: detected "
                << localStoragePatches.size()
                << " pending local storage patches");
        QScopedPointer<LocalStorageUpgradeDialog> pUpgradeDialog(
            new LocalStorageUpgradeDialog(*m_pAccount,
                                          m_pAccountManager->accountModel(),
                                          localStoragePatches,
                                          LocalStorageUpgradeDialog::Options(0),
                                          this));
        QObject::connect(pUpgradeDialog.data(),
                         QNSIGNAL(LocalStorageUpgradeDialog,shouldQuitApp),
                         this,
                         QNSLOT(MainWindow,onQuitAction),
                         Qt::ConnectionType(Qt::UniqueConnection |
                                            Qt::QueuedConnection));
        pUpgradeDialog->adjustSize();
        Q_UNUSED(pUpgradeDialog->exec())
    }

    m_pLocalStorageManagerAsync->moveToThread(m_pLocalStorageManagerThread);

    QObject::connect(this,
                     QNSIGNAL(MainWindow,localStorageSwitchUserRequest,
                              Account,LocalStorageManager::StartupOptions,QUuid),
                     m_pLocalStorageManagerAsync,
                     QNSLOT(LocalStorageManagerAsync,onSwitchUserRequest,
                            Account,LocalStorageManager::StartupOptions,QUuid));
    QObject::connect(m_pLocalStorageManagerAsync,
                     QNSIGNAL(LocalStorageManagerAsync,switchUserComplete,
                              Account,QUuid),
                     this,
                     QNSLOT(MainWindow,onLocalStorageSwitchUserRequestComplete,
                            Account,QUuid));
    QObject::connect(m_pLocalStorageManagerAsync,
                     QNSIGNAL(LocalStorageManagerAsync,switchUserFailed,
                              Account,ErrorString,QUuid),
                     this,
                     QNSLOT(MainWindow,onLocalStorageSwitchUserRequestFailed,
                            Account,ErrorString,QUuid));
}

void MainWindow::setupDisableNativeMenuBarPreference()
{
    QNDEBUG("MainWindow::setupDisableNativeMenuBarPreference");

    bool disableNativeMenuBar = getDisableNativeMenuBarPreference();
    m_pUI->menuBar->setNativeMenuBar(!disableNativeMenuBar);

    if (disableNativeMenuBar) {
        // Without this the menu bar forcefully integrated into the main window
        // looks kinda ugly
        m_pUI->menuBar->setStyleSheet(QString());
    }
}

void MainWindow::setupDefaultAccount()
{
    QNDEBUG("MainWindow::setupDefaultAccount");

    DefaultAccountFirstNotebookAndNoteCreator * pDefaultAccountFirstNotebookAndNoteCreator =
        new DefaultAccountFirstNotebookAndNoteCreator(
            *m_pLocalStorageManagerAsync, *m_pNoteFiltersManager, this);
    QObject::connect(pDefaultAccountFirstNotebookAndNoteCreator,
                     QNSIGNAL(DefaultAccountFirstNotebookAndNoteCreator,
                              finished,QString),
                     this,
                     QNSLOT(MainWindow,
                            onDefaultAccountFirstNotebookAndNoteCreatorFinished,
                            QString));
    QObject::connect(pDefaultAccountFirstNotebookAndNoteCreator,
                     QNSIGNAL(DefaultAccountFirstNotebookAndNoteCreator,
                              notifyError,ErrorString),
                     this,
                     QNSLOT(MainWindow,
                            onDefaultAccountFirstNotebookAndNoteCreatorError,
                            ErrorString));
    pDefaultAccountFirstNotebookAndNoteCreator->start();
}

void MainWindow::setupModels()
{
    QNDEBUG("MainWindow::setupModels");

    clearModels();

    NoteModel::NoteSortingMode::type noteSortingMode = restoreNoteSortingMode();
    if (noteSortingMode == NoteModel::NoteSortingMode::None) {
        noteSortingMode = NoteModel::NoteSortingMode::ModifiedDescending;
    }

    m_pNoteModel = new NoteModel(*m_pAccount, *m_pLocalStorageManagerAsync,
                                 m_noteCache, m_notebookCache, this,
                                 NoteModel::IncludedNotes::NonDeleted,
                                 noteSortingMode);
    m_pFavoritesModel = new FavoritesModel(*m_pAccount,
                                           *m_pLocalStorageManagerAsync,
                                           m_noteCache, m_notebookCache,
                                           m_tagCache, m_savedSearchCache, this);
    m_pNotebookModel = new NotebookModel(*m_pAccount, *m_pLocalStorageManagerAsync,
                                         m_notebookCache, this);
    m_pTagModel = new TagModel(*m_pAccount, *m_pLocalStorageManagerAsync,
                               m_tagCache, this);
    m_pSavedSearchModel = new SavedSearchModel(*m_pAccount,
                                               *m_pLocalStorageManagerAsync,
                                               m_savedSearchCache, this);
    m_pDeletedNotesModel = new NoteModel(*m_pAccount,
                                         *m_pLocalStorageManagerAsync,
                                         m_noteCache, m_notebookCache, this,
                                         NoteModel::IncludedNotes::Deleted);
    m_pDeletedNotesModel->start();

    if (m_pNoteCountLabelController == nullptr) {
        m_pNoteCountLabelController =
            new NoteCountLabelController(*m_pUI->notesCountLabelPanel, this);
    }

    m_pNoteCountLabelController->setNoteModel(*m_pNoteModel);

    setupNoteFilters();

    m_pUI->favoritesTableView->setModel(m_pFavoritesModel);
    m_pUI->notebooksTreeView->setModel(m_pNotebookModel);
    m_pUI->tagsTreeView->setModel(m_pTagModel);
    m_pUI->savedSearchesTableView->setModel(m_pSavedSearchModel);
    m_pUI->deletedNotesTableView->setModel(m_pDeletedNotesModel);
    m_pUI->noteListView->setModel(m_pNoteModel);

    m_pNotebookModelColumnChangeRerouter->setModel(m_pNotebookModel);
    m_pTagModelColumnChangeRerouter->setModel(m_pTagModel);
    m_pNoteModelColumnChangeRerouter->setModel(m_pNoteModel);
    m_pFavoritesModelColumnChangeRerouter->setModel(m_pFavoritesModel);

    if (m_pEditNoteDialogsManager) {
        m_pEditNoteDialogsManager->setNotebookModel(m_pNotebookModel);
    }
}

void MainWindow::clearModels()
{
    QNDEBUG("MainWindow::clearModels");

    clearViews();

    if (m_pNotebookModel) {
        delete m_pNotebookModel;
        m_pNotebookModel = nullptr;
    }

    if (m_pTagModel) {
        delete m_pTagModel;
        m_pTagModel = nullptr;
    }

    if (m_pSavedSearchModel) {
        delete m_pSavedSearchModel;
        m_pSavedSearchModel = nullptr;
    }

    if (m_pNoteModel) {
        m_pNoteModel->stop(IStartable::StopMode::Forced);
        delete m_pNoteModel;
        m_pNoteModel = nullptr;
    }

    if (m_pDeletedNotesModel) {
        m_pDeletedNotesModel->stop(IStartable::StopMode::Forced);
        delete m_pDeletedNotesModel;
        m_pDeletedNotesModel = nullptr;
    }

    if (m_pFavoritesModel) {
        delete m_pFavoritesModel;
        m_pFavoritesModel = nullptr;
    }
}

void MainWindow::setupShowHideStartupSettings()
{
    QNDEBUG("MainWindow::setupShowHideStartupSettings");

    ApplicationSettings appSettings(*m_pAccount, QUENTIER_UI_SETTINGS);
    appSettings.beginGroup(QStringLiteral("MainWindow"));

#define CHECK_AND_SET_SHOW_SETTING(name, action, widget)                       \
    {                                                                          \
        QVariant showSetting = appSettings.value(name);                        \
        if (showSetting.isNull()) {                                            \
            showSetting = m_pUI->Action##action->isChecked();                  \
        }                                                                      \
        if (showSetting.toBool()) {                                            \
            m_pUI->widget->show();                                             \
        }                                                                      \
        else {                                                                 \
            m_pUI->widget->hide();                                             \
        }                                                                      \
        m_pUI->Action##action->setChecked(showSetting.toBool());               \
    }                                                                          \
// CHECK_AND_SET_SHOW_SETTING

    CHECK_AND_SET_SHOW_SETTING(QStringLiteral("ShowSidePanel"),
                               ShowSidePanel, sidePanelSplitter)
    CHECK_AND_SET_SHOW_SETTING(QStringLiteral("ShowFavorites"),
                               ShowFavorites, favoritesWidget)
    CHECK_AND_SET_SHOW_SETTING(QStringLiteral("ShowNotebooks"),
                               ShowNotebooks, notebooksWidget)
    CHECK_AND_SET_SHOW_SETTING(QStringLiteral("ShowTags"),
                               ShowTags, tagsWidget)
    CHECK_AND_SET_SHOW_SETTING(QStringLiteral("ShowSavedSearches"),
                               ShowSavedSearches, savedSearchesWidget)
    CHECK_AND_SET_SHOW_SETTING(QStringLiteral("ShowDeletedNotes"),
                               ShowDeletedNotes, deletedNotesWidget)
    CHECK_AND_SET_SHOW_SETTING(QStringLiteral("ShowNotesList"),
                               ShowNotesList, notesListAndFiltersFrame)
    CHECK_AND_SET_SHOW_SETTING(QStringLiteral("ShowToolbar"),
                               ShowToolbar, upperBarGenericPanel)
    CHECK_AND_SET_SHOW_SETTING(QStringLiteral("ShowStatusBar"),
                               ShowStatusBar, upperBarGenericPanel)

#undef CHECK_AND_SET_SHOW_SETTING

    appSettings.endGroup();
}

void MainWindow::setupViews()
{
    QNDEBUG("MainWindow::setupViews");

    // NOTE: only a few columns would be shown for each view because otherwise
    // there are problems finding space for everything
    // TODO: in future should implement the persistent setting of which columns
    // to show or not to show

    FavoriteItemView * pFavoritesTableView = m_pUI->favoritesTableView;

    QAbstractItemDelegate * pPreviousFavoriteItemDelegate =
        pFavoritesTableView->itemDelegate();
    FavoriteItemDelegate * pFavoriteItemDelegate =
        qobject_cast<FavoriteItemDelegate*>(pPreviousFavoriteItemDelegate);
    if (!pFavoriteItemDelegate)
    {
        pFavoriteItemDelegate = new FavoriteItemDelegate(pFavoritesTableView);
        pFavoritesTableView->setItemDelegate(pFavoriteItemDelegate);

        if (pPreviousFavoriteItemDelegate) {
            pPreviousFavoriteItemDelegate->deleteLater();
            pPreviousFavoriteItemDelegate = nullptr;
        }
    }

    // This column's values would be displayed along with the favorite item's name
    pFavoritesTableView->setColumnHidden(FavoritesModel::Columns::NumNotesTargeted,
                                         true);
    pFavoritesTableView->setColumnWidth(FavoritesModel::Columns::Type,
                                        pFavoriteItemDelegate->sideSize());

    QObject::connect(m_pFavoritesModelColumnChangeRerouter,
                     &ColumnChangeRerouter::dataChanged,
                     pFavoritesTableView,
                     &FavoriteItemView::dataChanged,
                     Qt::UniqueConnection);
    pFavoritesTableView->header()->setSectionResizeMode(QHeaderView::ResizeToContents);

    QObject::connect(pFavoritesTableView,
                     QNSIGNAL(FavoriteItemView,notifyError,ErrorString),
                     this,
                     QNSLOT(MainWindow,onModelViewError,ErrorString),
                     Qt::UniqueConnection);
    QObject::connect(pFavoritesTableView,
                     QNSIGNAL(FavoriteItemView,favoritedItemInfoRequested),
                     this,
                     QNSLOT(MainWindow,onFavoritedItemInfoButtonPressed),
                     Qt::UniqueConnection);

    NotebookItemView * pNotebooksTreeView = m_pUI->notebooksTreeView;
    pNotebooksTreeView->setNoteFiltersManager(*m_pNoteFiltersManager);
    pNotebooksTreeView->setNoteModel(m_pNoteModel);

    QAbstractItemDelegate * pPreviousNotebookItemDelegate =
        pNotebooksTreeView->itemDelegate();
    NotebookItemDelegate * pNotebookItemDelegate =
        qobject_cast<NotebookItemDelegate*>(pPreviousNotebookItemDelegate);
    if (!pNotebookItemDelegate)
    {
        pNotebookItemDelegate = new NotebookItemDelegate(pNotebooksTreeView);
        pNotebooksTreeView->setItemDelegate(pNotebookItemDelegate);

        if (pPreviousNotebookItemDelegate) {
            pPreviousNotebookItemDelegate->deleteLater();
            pPreviousNotebookItemDelegate = nullptr;
        }
    }

    // This column's values would be displayed along with the notebook's name
    pNotebooksTreeView->setColumnHidden(NotebookModel::Columns::NumNotesPerNotebook,
                                        true);
    pNotebooksTreeView->setColumnHidden(NotebookModel::Columns::Synchronizable,
                                        true);
    pNotebooksTreeView->setColumnHidden(NotebookModel::Columns::LastUsed, true);
    pNotebooksTreeView->setColumnHidden(NotebookModel::Columns::FromLinkedNotebook,
                                        true);

    QAbstractItemDelegate * pPreviousNotebookDirtyColumnDelegate =
        pNotebooksTreeView->itemDelegateForColumn(NotebookModel::Columns::Dirty);
    DirtyColumnDelegate * pNotebookDirtyColumnDelegate =
        qobject_cast<DirtyColumnDelegate*>(pPreviousNotebookDirtyColumnDelegate);
    if (!pNotebookDirtyColumnDelegate)
    {
        pNotebookDirtyColumnDelegate = new DirtyColumnDelegate(pNotebooksTreeView);
        pNotebooksTreeView->setItemDelegateForColumn(NotebookModel::Columns::Dirty,
                                                     pNotebookDirtyColumnDelegate);

        if (pPreviousNotebookDirtyColumnDelegate) {
            pPreviousNotebookDirtyColumnDelegate->deleteLater();
            pPreviousNotebookDirtyColumnDelegate = nullptr;
        }
    }

    pNotebooksTreeView->setColumnWidth(NotebookModel::Columns::Dirty,
                                       pNotebookDirtyColumnDelegate->sideSize());
    pNotebooksTreeView->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    QObject::connect(m_pNotebookModelColumnChangeRerouter,
                     QNSIGNAL(ColumnChangeRerouter,dataChanged,
                              const QModelIndex&,const QModelIndex&,
                              const QVector<int>&),
                     pNotebooksTreeView,
                     QNSLOT(NotebookItemView,dataChanged,
                            const QModelIndex&,const QModelIndex&,
                            const QVector<int>&),
                     Qt::UniqueConnection);

    QObject::connect(pNotebooksTreeView,
                     QNSIGNAL(NotebookItemView,newNotebookCreationRequested),
                     this,
                     QNSLOT(MainWindow,onNewNotebookCreationRequested),
                     Qt::UniqueConnection);
    QObject::connect(pNotebooksTreeView,
                     QNSIGNAL(NotebookItemView,notebookInfoRequested),
                     this,
                     QNSLOT(MainWindow,onNotebookInfoButtonPressed),
                     Qt::UniqueConnection);
    QObject::connect(pNotebooksTreeView,
                     QNSIGNAL(NotebookItemView,notifyError,ErrorString),
                     this,
                     QNSLOT(MainWindow,onModelViewError,ErrorString),
                     Qt::UniqueConnection);

    TagItemView * pTagsTreeView = m_pUI->tagsTreeView;
    // This column's values would be displayed along with the notebook's name
    pTagsTreeView->setColumnHidden(TagModel::Columns::NumNotesPerTag, true);
    pTagsTreeView->setColumnHidden(TagModel::Columns::Synchronizable, true);
    pTagsTreeView->setColumnHidden(TagModel::Columns::FromLinkedNotebook, true);

    QAbstractItemDelegate * pPreviousTagDirtyColumnDelegate =
        pTagsTreeView->itemDelegateForColumn(TagModel::Columns::Dirty);
    DirtyColumnDelegate * pTagDirtyColumnDelegate =
        qobject_cast<DirtyColumnDelegate*>(pPreviousTagDirtyColumnDelegate);
    if (!pTagDirtyColumnDelegate)
    {
        pTagDirtyColumnDelegate = new DirtyColumnDelegate(pTagsTreeView);
        pTagsTreeView->setItemDelegateForColumn(TagModel::Columns::Dirty,
                                                pTagDirtyColumnDelegate);

        if (pPreviousTagDirtyColumnDelegate) {
            pPreviousTagDirtyColumnDelegate->deleteLater();
            pPreviousTagDirtyColumnDelegate = nullptr;
        }
    }

    pTagsTreeView->setColumnWidth(TagModel::Columns::Dirty,
                                  pTagDirtyColumnDelegate->sideSize());

    QAbstractItemDelegate * pPreviousTagItemDelegate =
        pTagsTreeView->itemDelegateForColumn(TagModel::Columns::Name);
    TagItemDelegate * pTagItemDelegate =
        qobject_cast<TagItemDelegate*>(pPreviousTagItemDelegate);
    if (!pTagItemDelegate)
    {
        pTagItemDelegate = new TagItemDelegate(pTagsTreeView);
        pTagsTreeView->setItemDelegateForColumn(TagModel::Columns::Name,
                                                pTagItemDelegate);

        if (pPreviousTagItemDelegate) {
            pPreviousTagItemDelegate->deleteLater();
            pPreviousTagItemDelegate = nullptr;
        }
    }

    pTagsTreeView->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    QObject::connect(m_pTagModelColumnChangeRerouter,
                     QNSIGNAL(ColumnChangeRerouter,dataChanged,
                              const QModelIndex&,const QModelIndex&,
                              const QVector<int>&),
                     pTagsTreeView,
                     QNSLOT(TagItemView,dataChanged,
                            const QModelIndex&,const QModelIndex&,
                            const QVector<int>&),
                     Qt::UniqueConnection);

    QObject::connect(pTagsTreeView,
                     QNSIGNAL(TagItemView,newTagCreationRequested),
                     this,
                     QNSLOT(MainWindow,onNewTagCreationRequested),
                     Qt::UniqueConnection);
    QObject::connect(pTagsTreeView,
                     QNSIGNAL(TagItemView,tagInfoRequested),
                     this,
                     QNSLOT(MainWindow,onTagInfoButtonPressed),
                     Qt::UniqueConnection);
    QObject::connect(pTagsTreeView,
                     QNSIGNAL(TagItemView,notifyError,ErrorString),
                     this,
                     QNSLOT(MainWindow,onModelViewError,ErrorString),
                     Qt::UniqueConnection);

    SavedSearchItemView * pSavedSearchesTableView = m_pUI->savedSearchesTableView;
    pSavedSearchesTableView->setColumnHidden(SavedSearchModel::Columns::Query,
                                             true);
    pSavedSearchesTableView->setColumnHidden(SavedSearchModel::Columns::Synchronizable,
                                             true);

    QAbstractItemDelegate * pPreviousSavedSearchDirtyColumnDelegate =
        pSavedSearchesTableView->itemDelegateForColumn(SavedSearchModel::Columns::Dirty);
    DirtyColumnDelegate * pSavedSearchDirtyColumnDelegate =
        qobject_cast<DirtyColumnDelegate*>(pPreviousSavedSearchDirtyColumnDelegate);
    if (!pSavedSearchDirtyColumnDelegate)
    {
        pSavedSearchDirtyColumnDelegate =
            new DirtyColumnDelegate(pSavedSearchesTableView);
        pSavedSearchesTableView->setItemDelegateForColumn(
            SavedSearchModel::Columns::Dirty,
            pSavedSearchDirtyColumnDelegate);

        if (pPreviousSavedSearchDirtyColumnDelegate) {
            pPreviousSavedSearchDirtyColumnDelegate->deleteLater();
            pPreviousSavedSearchDirtyColumnDelegate = nullptr;
        }
    }

    pSavedSearchesTableView->setColumnWidth(
        SavedSearchModel::Columns::Dirty,
        pSavedSearchDirtyColumnDelegate->sideSize());
    pSavedSearchesTableView->header()->setSectionResizeMode(QHeaderView::ResizeToContents);

    QObject::connect(pSavedSearchesTableView,
                     QNSIGNAL(SavedSearchItemView,savedSearchInfoRequested),
                     this,
                     QNSLOT(MainWindow,onSavedSearchInfoButtonPressed),
                     Qt::UniqueConnection);
    QObject::connect(pSavedSearchesTableView,
                     QNSIGNAL(SavedSearchItemView,newSavedSearchCreationRequested),
                     this,
                     QNSLOT(MainWindow,onNewSavedSearchCreationRequested),
                     Qt::UniqueConnection);
    QObject::connect(pSavedSearchesTableView,
                     QNSIGNAL(SavedSearchItemView,notifyError,ErrorString),
                     this,
                     QNSLOT(MainWindow,onModelViewError,ErrorString),
                     Qt::UniqueConnection);

    NoteListView * pNoteListView = m_pUI->noteListView;
    if (m_pAccount) {
        pNoteListView->setCurrentAccount(*m_pAccount);
    }

    QAbstractItemDelegate * pPreviousNoteItemDelegate =
        pNoteListView->itemDelegate();
    NoteItemDelegate * pNoteItemDelegate =
        qobject_cast<NoteItemDelegate*>(pPreviousNoteItemDelegate);
    if (!pNoteItemDelegate)
    {
        pNoteItemDelegate = new NoteItemDelegate(pNoteListView);

        pNoteListView->setModelColumn(NoteModel::Columns::Title);
        pNoteListView->setItemDelegate(pNoteItemDelegate);

        if (pPreviousNoteItemDelegate) {
            pPreviousNoteItemDelegate->deleteLater();
            pPreviousNoteItemDelegate = nullptr;
        }
    }

    pNoteListView->setNotebookItemView(pNotebooksTreeView);
    QObject::connect(m_pNoteModelColumnChangeRerouter,
                     &ColumnChangeRerouter::dataChanged,
                     pNoteListView,
                     &NoteListView::dataChanged,
                     Qt::UniqueConnection);
    QObject::connect(pNoteListView,
                     QNSIGNAL(NoteListView,currentNoteChanged,QString),
                     this,
                     QNSLOT(MainWindow,onCurrentNoteInListChanged,QString),
                     Qt::UniqueConnection);
    QObject::connect(pNoteListView,
                     QNSIGNAL(NoteListView,openNoteInSeparateWindowRequested,
                              QString),
                     this,
                     QNSLOT(MainWindow,onOpenNoteInSeparateWindow,QString),
                     Qt::UniqueConnection);
    QObject::connect(pNoteListView,
                     QNSIGNAL(NoteListView,enexExportRequested,QStringList),
                     this,
                     QNSLOT(MainWindow,onExportNotesToEnexRequested,QStringList),
                     Qt::UniqueConnection);
    QObject::connect(pNoteListView,
                     QNSIGNAL(NoteListView,newNoteCreationRequested),
                     this,
                     QNSLOT(MainWindow,onNewNoteCreationRequested),
                     Qt::UniqueConnection);
    QObject::connect(pNoteListView,
                     QNSIGNAL(NoteListView,copyInAppNoteLinkRequested,
                              QString,QString),
                     this,
                     QNSLOT(MainWindow,onCopyInAppLinkNoteRequested,
                            QString,QString),
                     Qt::UniqueConnection);
    QObject::connect(pNoteListView,
                     QNSIGNAL(NoteListView,toggleThumbnailsPreference,QString),
                     this,
                     QNSLOT(MainWindow,onToggleThumbnailsPreference,QString),
                     Qt::UniqueConnection);
    QObject::connect(this,
                     QNSIGNAL(MainWindow,showNoteThumbnailsStateChanged,
                              bool,QSet<QString>),
                     pNoteListView,
                     QNSLOT(NoteListView,setShowNoteThumbnailsState,
                            bool,QSet<QString>),
                     Qt::UniqueConnection);
    QObject::connect(this,
                     QNSIGNAL(MainWindow,showNoteThumbnailsStateChanged,
                              bool,QSet<QString>),
                     pNoteItemDelegate,
                     QNSLOT(NoteItemDelegate,setShowNoteThumbnailsState,
                            bool,QSet<QString>),
                     Qt::UniqueConnection);


    if (!m_onceSetupNoteSortingModeComboBox)
    {
        QStringList noteSortingModes;
        noteSortingModes.reserve(8);
        noteSortingModes << tr("Created (ascending)");
        noteSortingModes << tr("Created (descending)");
        noteSortingModes << tr("Modified (ascending)");
        noteSortingModes << tr("Modified (descending)");
        noteSortingModes << tr("Title (ascending)");
        noteSortingModes << tr("Title (descending)");
        noteSortingModes << tr("Size (ascending)");
        noteSortingModes << tr("Size (descending)");

        QStringListModel * pNoteSortingModeModel = new QStringListModel(this);
        pNoteSortingModeModel->setStringList(noteSortingModes);

        m_pUI->noteSortingModeComboBox->setModel(pNoteSortingModeModel);
        m_onceSetupNoteSortingModeComboBox = true;
    }

    NoteModel::NoteSortingMode::type noteSortingMode = restoreNoteSortingMode();
    if (noteSortingMode == NoteModel::NoteSortingMode::None) {
        noteSortingMode = NoteModel::NoteSortingMode::ModifiedDescending;
        QNDEBUG("Couldn't restore the note sorting mode, fallback "
                << "to the default one of " << noteSortingMode);
    }

    m_pUI->noteSortingModeComboBox->setCurrentIndex(noteSortingMode);

    QObject::connect(m_pUI->noteSortingModeComboBox,
                     SIGNAL(currentIndexChanged(int)),
                     this,
                     SLOT(onNoteSortingModeChanged(int)),
                     Qt::UniqueConnection);
    onNoteSortingModeChanged(m_pUI->noteSortingModeComboBox->currentIndex());

    DeletedNoteItemView * pDeletedNotesTableView = m_pUI->deletedNotesTableView;
    pDeletedNotesTableView->setColumnHidden(
        NoteModel::Columns::CreationTimestamp, true);
    pDeletedNotesTableView->setColumnHidden(
        NoteModel::Columns::ModificationTimestamp, true);
    pDeletedNotesTableView->setColumnHidden(
        NoteModel::Columns::PreviewText, true);
    pDeletedNotesTableView->setColumnHidden(
        NoteModel::Columns::ThumbnailImage, true);
    pDeletedNotesTableView->setColumnHidden(
        NoteModel::Columns::TagNameList, true);
    pDeletedNotesTableView->setColumnHidden(
        NoteModel::Columns::Size, true);
    pDeletedNotesTableView->setColumnHidden(
        NoteModel::Columns::Synchronizable, true);
    pDeletedNotesTableView->setColumnHidden(
        NoteModel::Columns::NotebookName, true);

    QAbstractItemDelegate * pPreviousDeletedNoteItemDelegate =
        pDeletedNotesTableView->itemDelegate();
    DeletedNoteItemDelegate * pDeletedNoteItemDelegate =
        qobject_cast<DeletedNoteItemDelegate*>(pPreviousDeletedNoteItemDelegate);
    if (!pDeletedNoteItemDelegate)
    {
        pDeletedNoteItemDelegate = new DeletedNoteItemDelegate(pDeletedNotesTableView);
        pDeletedNotesTableView->setItemDelegate(pDeletedNoteItemDelegate);

        if (pPreviousDeletedNoteItemDelegate) {
            pPreviousDeletedNoteItemDelegate->deleteLater();
            pPreviousDeletedNoteItemDelegate = nullptr;
        }
    }

    pDeletedNotesTableView->header()->setSectionResizeMode(QHeaderView::ResizeToContents);

    if (!m_pEditNoteDialogsManager)
    {
        m_pEditNoteDialogsManager =
            new EditNoteDialogsManager(*m_pLocalStorageManagerAsync,
                                       m_noteCache, m_pNotebookModel, this);
        QObject::connect(pNoteListView,
                         QNSIGNAL(NoteListView,editNoteDialogRequested,QString),
                         m_pEditNoteDialogsManager,
                         QNSLOT(EditNoteDialogsManager,onEditNoteDialogRequested,
                                QString),
                         Qt::UniqueConnection);
        QObject::connect(pNoteListView,
                         QNSIGNAL(NoteListView,noteInfoDialogRequested,QString),
                         m_pEditNoteDialogsManager,
                         QNSLOT(EditNoteDialogsManager,onNoteInfoDialogRequested,
                                QString),
                         Qt::UniqueConnection);
        QObject::connect(this,
                         QNSIGNAL(MainWindow,noteInfoDialogRequested,QString),
                         m_pEditNoteDialogsManager,
                         QNSLOT(EditNoteDialogsManager,onNoteInfoDialogRequested,
                                QString),
                         Qt::UniqueConnection);
        QObject::connect(pDeletedNotesTableView,
                         QNSIGNAL(DeletedNoteItemView,deletedNoteInfoRequested,
                                  QString),
                         m_pEditNoteDialogsManager,
                         QNSLOT(EditNoteDialogsManager,onNoteInfoDialogRequested,
                                QString),
                         Qt::UniqueConnection);
    }

    Account::Type::type currentAccountType = Account::Type::Local;
    if (m_pAccount) {
        currentAccountType = m_pAccount->type();
    }

    showHideViewColumnsForAccountType(currentAccountType);
}

bool MainWindow::getShowNoteThumbnailsPreference() const
{
    ApplicationSettings appSettings(*m_pAccount, QUENTIER_UI_SETTINGS);
    appSettings.beginGroup(LOOK_AND_FEEL_SETTINGS_GROUP_NAME);
    QVariant showThumbnails =
        appSettings.value(SHOW_NOTE_THUMBNAILS_SETTINGS_KEY,
                          QVariant::fromValue(DEFAULT_SHOW_NOTE_THUMBNAILS));
    appSettings.endGroup();

    return showThumbnails.toBool();
}

bool MainWindow::getDisableNativeMenuBarPreference() const
{
    ApplicationSettings appSettings(*m_pAccount, QUENTIER_UI_SETTINGS);
    appSettings.beginGroup(LOOK_AND_FEEL_SETTINGS_GROUP_NAME);
    QVariant disableNativeMenuBar =
        appSettings.value(DISABLE_NATIVE_MENU_BAR_SETTINGS_KEY,
                          QVariant::fromValue(defaultDisableNativeMenuBar()));
    appSettings.endGroup();

    return disableNativeMenuBar.toBool();
}

QSet<QString> MainWindow::getHideNoteThumbnailsFor() const
{
    ApplicationSettings appSettings(*m_pAccount, QUENTIER_UI_SETTINGS);
    appSettings.beginGroup(LOOK_AND_FEEL_SETTINGS_GROUP_NAME);
    QVariant hideThumbnailsFor =
        appSettings.value(HIDE_NOTE_THUMBNAILS_FOR_SETTINGS_KEY,
                          QLatin1String(""));
    appSettings.endGroup();

    return QSet<QString>::fromList(hideThumbnailsFor.toStringList());
}

void MainWindow::toggleShowNoteThumbnails() const
{
    bool newValue = !getShowNoteThumbnailsPreference();
    ApplicationSettings appSettings(*m_pAccount, QUENTIER_UI_SETTINGS);
    appSettings.beginGroup(LOOK_AND_FEEL_SETTINGS_GROUP_NAME);
    appSettings.setValue(SHOW_NOTE_THUMBNAILS_SETTINGS_KEY,
                         QVariant::fromValue(newValue));
    appSettings.endGroup();
}

void MainWindow::toggleHideNoteThumbnailFor(QString noteLocalUid) const
{
    QSet<QString> hideThumbnailsForSet = getHideNoteThumbnailsFor();
    if (hideThumbnailsForSet.contains(noteLocalUid)) {
        hideThumbnailsForSet.remove(noteLocalUid);
    }
    else {
        // after max. count is reached we ignore further requests
        if (hideThumbnailsForSet.size() <=
            HIDE_NOTE_THUMBNAILS_FOR_SETTINGS_KEY_MAX_COUNT)
        {
            hideThumbnailsForSet.insert(noteLocalUid);
        }
    }
    ApplicationSettings appSettings(*m_pAccount, QUENTIER_UI_SETTINGS);
    appSettings.beginGroup(LOOK_AND_FEEL_SETTINGS_GROUP_NAME);
    appSettings.setValue(HIDE_NOTE_THUMBNAILS_FOR_SETTINGS_KEY,
                         QStringList(hideThumbnailsForSet.values()));
    appSettings.endGroup();
}

QString MainWindow::fallbackIconThemeName() const
{
    const QPalette & pal = palette();
    const QColor & windowColor = pal.color(QPalette::Window);

    // lightness is the HSL color model value from 0 to 255
    // which can be used to deduce whether light or dark color theme
    // is used
    int lightness = windowColor.lightness();

    if (lightness < 128) {
        // It appears that dark color theme is used
        return QStringLiteral("breeze-dark");
    }

    return QStringLiteral("breeze");
}

void MainWindow::clearViews()
{
    QNDEBUG("MainWindow::clearViews");

    m_pUI->favoritesTableView->setModel(&m_blankModel);
    m_pUI->notebooksTreeView->setModel(&m_blankModel);
    m_pUI->tagsTreeView->setModel(&m_blankModel);
    m_pUI->savedSearchesTableView->setModel(&m_blankModel);

    m_pUI->noteListView->setModel(&m_blankModel);
    // NOTE: without this the note list view doesn't seem to re-render
    // so the items from the previously set model are still displayed
    m_pUI->noteListView->update();

    m_pUI->deletedNotesTableView->setModel(&m_blankModel);
}

void MainWindow::setupAccountSpecificUiElements()
{
    QNDEBUG("MainWindow::setupAccountSpecificUiElements");

    if (Q_UNLIKELY(!m_pAccount)) {
        QNDEBUG("No account");
        return;
    }

    bool isLocal = (m_pAccount->type() == Account::Type::Local);

    m_pUI->removeNotebookButton->setHidden(!isLocal);
    m_pUI->removeNotebookButton->setDisabled(!isLocal);

    m_pUI->removeTagButton->setHidden(!isLocal);
    m_pUI->removeTagButton->setDisabled(!isLocal);

    m_pUI->removeSavedSearchButton->setHidden(!isLocal);
    m_pUI->removeSavedSearchButton->setDisabled(!isLocal);

    m_pUI->eraseDeletedNoteButton->setHidden(!isLocal);
    m_pUI->eraseDeletedNoteButton->setDisabled(!isLocal);

    m_pUI->syncPushButton->setHidden(isLocal);
    m_pUI->syncPushButton->setDisabled(isLocal);

    m_pUI->ActionSynchronize->setVisible(!isLocal);
    m_pUI->menuService->menuAction()->setVisible(!isLocal);
}

void MainWindow::setupNoteFilters()
{
    QNDEBUG("MainWindow::setupNoteFilters");

    m_pUI->filterFrameBottomBoundary->hide();

    m_pUI->filterByNotebooksWidget->setLocalStorageManager(
        *m_pLocalStorageManagerAsync);
    m_pUI->filterByTagsWidget->setLocalStorageManager(*m_pLocalStorageManagerAsync);

    m_pUI->filterByNotebooksWidget->switchAccount(*m_pAccount, m_pNotebookModel);
    m_pUI->filterByTagsWidget->switchAccount(*m_pAccount, m_pTagModel);
    m_pUI->filterBySavedSearchComboBox->switchAccount(*m_pAccount,
                                                      m_pSavedSearchModel);

    m_pUI->filterStatusBarLabel->hide();

    if (m_pNoteFiltersManager) {
        m_pNoteFiltersManager->disconnect();
        m_pNoteFiltersManager->deleteLater();
    }
    m_pNoteFiltersManager =
        new NoteFiltersManager(*m_pAccount, *m_pUI->filterByTagsWidget,
                               *m_pUI->filterByNotebooksWidget,
                               *m_pNoteModel,
                               *m_pUI->filterBySavedSearchComboBox,
                               *m_pUI->searchQueryLineEdit,
                               *m_pLocalStorageManagerAsync, this);
    m_pNoteModel->start();

    ApplicationSettings appSettings(*m_pAccount, QUENTIER_UI_SETTINGS);
    appSettings.beginGroup(QStringLiteral("FiltersView"));
    m_filtersViewExpanded = appSettings.value(FILTERS_VIEW_STATUS_KEY).toBool();
    appSettings.endGroup();

    if (!m_filtersViewExpanded) {
        foldFiltersView();
    }
    else {
        expandFiltersView();
    }
}

void MainWindow::setupNoteEditorTabWidgetsCoordinator()
{
    QNDEBUG("MainWindow::setupNoteEditorTabWidgetsCoordinator");

    delete m_pNoteEditorTabsAndWindowsCoordinator;
    m_pNoteEditorTabsAndWindowsCoordinator =
        new NoteEditorTabsAndWindowsCoordinator(*m_pAccount,
                                                *m_pLocalStorageManagerAsync,
                                                m_noteCache, m_notebookCache,
                                                m_tagCache, *m_pTagModel,
                                                m_pUI->noteEditorsTabWidget, this);
    QObject::connect(m_pNoteEditorTabsAndWindowsCoordinator,
                     QNSIGNAL(NoteEditorTabsAndWindowsCoordinator,notifyError,
                              ErrorString),
                     this,
                     QNSLOT(MainWindow,onNoteEditorError,ErrorString));
    QObject::connect(m_pNoteEditorTabsAndWindowsCoordinator,
                     QNSIGNAL(NoteEditorTabsAndWindowsCoordinator,
                              currentNoteChanged,QString),
                     m_pUI->noteListView,
                     QNSLOT(NoteListView,setCurrentNoteByLocalUid,QString));
}

bool MainWindow::checkLocalStorageVersion(const Account & account)
{
    QNDEBUG("MainWindow::checkLocalStorageVersion: account = " << account);

    ErrorString errorDescription;
    if (m_pLocalStorageManagerAsync->localStorageManager()->isLocalStorageVersionTooHigh(
            errorDescription))
    {
        QNINFO("Detected too high local storage version: "
               << errorDescription << "; account: " << account);

        QScopedPointer<LocalStorageVersionTooHighDialog> pVersionTooHighDialog(
            new LocalStorageVersionTooHighDialog(
                account, m_pAccountManager->accountModel(),
                *(m_pLocalStorageManagerAsync->localStorageManager()), this));
        QObject::connect(pVersionTooHighDialog.data(),
                         QNSIGNAL(LocalStorageVersionTooHighDialog,
                                  shouldSwitchToAccount,Account),
                         this,
                         QNSLOT(MainWindow,onAccountSwitchRequested,Account),
                         Qt::ConnectionType(Qt::UniqueConnection | Qt::QueuedConnection));
        QObject::connect(pVersionTooHighDialog.data(),
                         QNSIGNAL(LocalStorageVersionTooHighDialog,
                                  shouldCreateNewAccount),
                         this,
                         QNSLOT(MainWindow,onNewAccountCreationRequested),
                         Qt::ConnectionType(Qt::UniqueConnection | Qt::QueuedConnection));
        QObject::connect(pVersionTooHighDialog.data(),
                         QNSIGNAL(LocalStorageVersionTooHighDialog,shouldQuitApp),
                         this,
                         QNSLOT(MainWindow,onQuitAction),
                         Qt::ConnectionType(Qt::UniqueConnection | Qt::QueuedConnection));
        Q_UNUSED(pVersionTooHighDialog->exec())
        return false;
    }

    errorDescription.clear();
    QVector<QSharedPointer<ILocalStoragePatch> > localStoragePatches =
        m_pLocalStorageManagerAsync->localStorageManager()->requiredLocalStoragePatches();
    if (!localStoragePatches.isEmpty())
    {
        QNDEBUG("Local storage requires upgrade: detected "
                << localStoragePatches.size()
                << " pending local storage patches");
        LocalStorageUpgradeDialog::Options options(
            LocalStorageUpgradeDialog::Option::AddAccount |
            LocalStorageUpgradeDialog::Option::SwitchToAnotherAccount);
        QScopedPointer<LocalStorageUpgradeDialog> pUpgradeDialog(
            new LocalStorageUpgradeDialog(account, m_pAccountManager->accountModel(),
                                          localStoragePatches, options, this));
        QObject::connect(pUpgradeDialog.data(),
                         QNSIGNAL(LocalStorageUpgradeDialog,shouldSwitchToAccount,
                                  Account),
                         this,
                         QNSLOT(MainWindow,onAccountSwitchRequested,Account),
                         Qt::ConnectionType(Qt::UniqueConnection | Qt::QueuedConnection));
        QObject::connect(pUpgradeDialog.data(),
                         QNSIGNAL(LocalStorageUpgradeDialog,shouldCreateNewAccount),
                         this,
                         QNSLOT(MainWindow,onNewAccountCreationRequested),
                         Qt::ConnectionType(Qt::UniqueConnection | Qt::QueuedConnection));
        QObject::connect(pUpgradeDialog.data(),
                         QNSIGNAL(LocalStorageUpgradeDialog,shouldQuitApp),
                         this,
                         QNSLOT(MainWindow,onQuitAction),
                         Qt::ConnectionType(Qt::UniqueConnection | Qt::QueuedConnection));
        pUpgradeDialog->adjustSize();
        Q_UNUSED(pUpgradeDialog->exec())

        if (!pUpgradeDialog->isUpgradeDone()) {
            return false;
        }
    }

    return true;
}

bool MainWindow::onceDisplayedGreeterScreen() const
{
    ApplicationSettings appSettings;
    appSettings.beginGroup(ACCOUNT_SETTINGS_GROUP);

    bool result = false;
    if (appSettings.contains(ONCE_DISPLAYED_GREETER_SCREEN)) {
        result = appSettings.value(ONCE_DISPLAYED_GREETER_SCREEN).toBool();
    }

    appSettings.endGroup();
    return result;
}

void MainWindow::setOnceDisplayedGreeterScreen()
{
    QNDEBUG("MainWindow::setOnceDisplayedGreeterScreen");

    ApplicationSettings appSettings;
    appSettings.beginGroup(ACCOUNT_SETTINGS_GROUP);
    appSettings.setValue(ONCE_DISPLAYED_GREETER_SCREEN, true);
    appSettings.endGroup();
}

void MainWindow::setupSynchronizationManager(
    const SetAccountOption::type setAccountOption)
{
    QNDEBUG("MainWindow::setupSynchronizationManager: "
            << "set account option = " << setAccountOption);

    clearSynchronizationManager();

    if (m_synchronizationManagerHost.isEmpty())
    {
        if (Q_UNLIKELY(!m_pAccount)) {
            ErrorString error(QT_TR_NOOP("Can't set up the synchronization: "
                                         "no account"));
            QNWARNING(error);
            onSetStatusBarText(error.localizedString(), SEC_TO_MSEC(30));
            return;
        }

        if (Q_UNLIKELY(m_pAccount->type() != Account::Type::Evernote)) {
            ErrorString error(QT_TR_NOOP("Can't set up the synchronization: "
                                         "non-Evernote account is chosen"));
            QNWARNING(error << "; account: " << *m_pAccount);
            onSetStatusBarText(error.localizedString(), SEC_TO_MSEC(30));
            return;
        }

        m_synchronizationManagerHost = m_pAccount->evernoteHost();
        if (Q_UNLIKELY(m_synchronizationManagerHost.isEmpty())) {
            ErrorString error(QT_TR_NOOP("Can't set up the synchronization: "
                                         "no Evernote host within the account"));
            QNWARNING(error << "; account: " << *m_pAccount);
            onSetStatusBarText(error.localizedString(), SEC_TO_MSEC(30));
            return;
        }
    }

    QString consumerKey, consumerSecret;
    setupConsumerKeyAndSecret(consumerKey, consumerSecret);

    if (Q_UNLIKELY(consumerKey.isEmpty())) {
        QNDEBUG("Consumer key is empty");
        return;
    }

    if (Q_UNLIKELY(consumerSecret.isEmpty())) {
        QNDEBUG("Consumer secret is empty");
        return;
    }

    m_pAuthenticationManager =
        new AuthenticationManager(consumerKey, consumerSecret,
                                  m_synchronizationManagerHost);
    m_pSynchronizationManager =
        new SynchronizationManager(m_synchronizationManagerHost,
                                   *m_pLocalStorageManagerAsync,
                                   *m_pAuthenticationManager);

    if (m_pAccount && (setAccountOption == SetAccountOption::Set)) {
        m_pSynchronizationManager->setAccount(*m_pAccount);
    }

    connectSynchronizationManager();
    setupRunSyncPeriodicallyTimer();
}

void MainWindow::clearSynchronizationManager()
{
    QNDEBUG("MainWindow::clearSynchronizationManager");

    disconnectSynchronizationManager();

    if (m_pSynchronizationManager) {
        m_pSynchronizationManager->disconnect(this);
        m_pSynchronizationManager->deleteLater();
        m_pSynchronizationManager = nullptr;
    }

    if (m_pAuthenticationManager) {
        m_pAuthenticationManager->deleteLater();
        m_pAuthenticationManager = nullptr;
    }

    if (m_pSynchronizationManagerThread &&
        m_pSynchronizationManagerThread->isRunning())
    {
        QObject::disconnect(m_pSynchronizationManagerThread,
                            QNSIGNAL(QThread,finished),
                            m_pSynchronizationManagerThread,
                            QNSLOT(QThread,deleteLater));
        m_pSynchronizationManagerThread->quit();
        m_pSynchronizationManagerThread->wait();
        m_pSynchronizationManagerThread->deleteLater();
        m_pSynchronizationManagerThread = nullptr;

    }

    m_syncApiRateLimitExceeded = false;
    scheduleSyncButtonAnimationStop();

    if (m_runSyncPeriodicallyTimerId != 0) {
        killTimer(m_runSyncPeriodicallyTimerId);
        m_runSyncPeriodicallyTimerId = 0;
    }
}

void MainWindow::setSynchronizationOptions(const Account & account)
{
    QNDEBUG("MainWindow::setSynchronizationOptions");

    if (Q_UNLIKELY(!m_pSynchronizationManager)) {
        QNWARNING("Can't set synchronization options: "
                  "no synchronization manager");
        return;
    }

    ApplicationSettings appSettings(account, QUENTIER_SYNC_SETTINGS);
    appSettings.beginGroup(SYNCHRONIZATION_SETTINGS_GROUP_NAME);
    bool downloadNoteThumbnailsOption =
        (appSettings.contains(SYNCHRONIZATION_DOWNLOAD_NOTE_THUMBNAILS)
         ? appSettings.value(SYNCHRONIZATION_DOWNLOAD_NOTE_THUMBNAILS).toBool()
         : DEFAULT_DOWNLOAD_NOTE_THUMBNAILS);
    bool downloadInkNoteImagesOption =
        (appSettings.contains(SYNCHRONIZATION_DOWNLOAD_INK_NOTE_IMAGES)
         ? appSettings.value(SYNCHRONIZATION_DOWNLOAD_INK_NOTE_IMAGES).toBool()
         : DEFAULT_DOWNLOAD_INK_NOTE_IMAGES);
    appSettings.endGroup();

    m_pSynchronizationManager->setDownloadNoteThumbnails(downloadNoteThumbnailsOption);
    m_pSynchronizationManager->setDownloadInkNoteImages(downloadInkNoteImagesOption);

    QString inkNoteImagesStoragePath = accountPersistentStoragePath(account);
    inkNoteImagesStoragePath += QStringLiteral("/NoteEditorPage/inkNoteImages");
    QNTRACE("Ink note images storage path: " << inkNoteImagesStoragePath
            << "; account: " << account.name());

    m_pSynchronizationManager->setInkNoteImagesStoragePath(inkNoteImagesStoragePath);
}

void MainWindow::setupSynchronizationManagerThread()
{
    QNDEBUG("MainWindow::setupSynchronizationManagerThread");

    m_pSynchronizationManagerThread = new QThread;
    m_pSynchronizationManagerThread->setObjectName(
        QStringLiteral("SynchronizationManagerThread"));
    QObject::connect(m_pSynchronizationManagerThread,
                     QNSIGNAL(QThread,finished),
                     m_pSynchronizationManagerThread,
                     QNSLOT(QThread,deleteLater));
    m_pSynchronizationManagerThread->start();

    m_pSynchronizationManager->moveToThread(m_pSynchronizationManagerThread);
    m_pAuthenticationManager->moveToThread(m_pSynchronizationManagerThread);
}

void MainWindow::setupRunSyncPeriodicallyTimer()
{
    QNDEBUG("MainWindow::setupRunSyncPeriodicallyTimer");

    if (Q_UNLIKELY(!m_pAccount)) {
        QNDEBUG("No current account");
        return;
    }

    if (Q_UNLIKELY(m_pAccount->type() != Account::Type::Evernote)) {
        QNDEBUG("Non-Evernote account is used");
        return;
    }

    ApplicationSettings syncSettings(*m_pAccount, QUENTIER_SYNC_SETTINGS);
    syncSettings.beginGroup(SYNCHRONIZATION_SETTINGS_GROUP_NAME);

    int runSyncEachNumMinutes = -1;
    if (syncSettings.contains(SYNCHRONIZATION_RUN_SYNC_EACH_NUM_MINUTES))
    {
        QVariant data = syncSettings.value(SYNCHRONIZATION_RUN_SYNC_EACH_NUM_MINUTES);
        bool conversionResult = false;
        runSyncEachNumMinutes = data.toInt(&conversionResult);
        if (Q_UNLIKELY(!conversionResult)) {
            QNDEBUG("Failed to convert the number of minutes to "
                    << "run sync over to int: " << data);
            runSyncEachNumMinutes = -1;
        }
    }

    if (runSyncEachNumMinutes < 0) {
        runSyncEachNumMinutes = DEFAULT_RUN_SYNC_EACH_NUM_MINUTES;
    }

    syncSettings.endGroup();

    onRunSyncEachNumMinitesPreferenceChanged(runSyncEachNumMinutes);
}

void MainWindow::launchSynchronization()
{
    QNDEBUG("MainWindow::launchSynchronization");

    if (Q_UNLIKELY(!m_pSynchronizationManager)) {
        ErrorString error(QT_TR_NOOP("Can't start synchronization: internal "
                                     "error, no synchronization manager is set up"));
        QNWARNING(error);
        onSetStatusBarText(error.localizedString(), SEC_TO_MSEC(30));
        return;
    }

    if (m_syncInProgress) {
        QNDEBUG("Synchronization is already in progress");
        return;
    }

    if (m_pendingNewEvernoteAccountAuthentication) {
        QNDEBUG("Pending new Evernote account authentication");
        return;
    }

    if (m_pendingCurrentEvernoteAccountAuthentication) {
        QNDEBUG("Pending current Evernote account authentication");
        return;
    }

    if (m_pendingSwitchToNewEvernoteAccount) {
        QNDEBUG("Pending switch to new Evernote account");
        return;
    }

    if (m_authenticatedCurrentEvernoteAccount) {
        QNDEBUG("Emitting synchronize signal");
        Q_EMIT synchronize();
        return;
    }

    m_pendingCurrentEvernoteAccountAuthentication = true;
    QNDEBUG("Emitting authenticate current account signal");
    Q_EMIT authenticateCurrentAccount();
}

void MainWindow::setupDefaultShortcuts()
{
    QNDEBUG("MainWindow::setupDefaultShortcuts");

    using quentier::ShortcutManager;

#define PROCESS_ACTION_SHORTCUT(action, key, context)                          \
    {                                                                          \
        QString contextStr = QString::fromUtf8(context);                       \
        ActionKeyWithContext actionData;                                       \
        actionData.m_key = key;                                                \
        actionData.m_context = contextStr;                                     \
        QVariant data;                                                         \
        data.setValue(actionData);                                             \
        m_pUI->Action##action->setData(data);                                  \
        QKeySequence shortcut = m_pUI->Action##action->shortcut();             \
        if (shortcut.isEmpty()) {                                              \
            QNTRACE("No shortcut was found for action "                        \
                    << m_pUI->Action##action->objectName());                   \
        }                                                                      \
        else {                                                                 \
            m_shortcutManager.setDefaultShortcut(key, shortcut,                \
                                                 *m_pAccount, contextStr);     \
        }                                                                      \
    }                                                                          \
// PROCESS_ACTION_SHORTCUT

#define PROCESS_NON_STANDARD_ACTION_SHORTCUT(action, context)                  \
    {                                                                          \
        QString actionStr = QString::fromUtf8(#action);                        \
        QString contextStr = QString::fromUtf8(context);                       \
        ActionNonStandardKeyWithContext actionData;                            \
        actionData.m_nonStandardActionKey = actionStr;                         \
        actionData.m_context = contextStr;                                     \
        QVariant data;                                                         \
        data.setValue(actionData);                                             \
        m_pUI->Action##action->setData(data);                                  \
        QKeySequence shortcut = m_pUI->Action##action->shortcut();             \
        if (shortcut.isEmpty()) {                                              \
            QNTRACE("No shortcut was found for action "                        \
                    << m_pUI->Action##action->objectName());                   \
        }                                                                      \
        else {                                                                 \
            m_shortcutManager.setNonStandardDefaultShortcut(                   \
                actionStr, shortcut, *m_pAccount, contextStr);                 \
        }                                                                      \
    }                                                                          \
// PROCESS_NON_STANDARD_ACTION_SHORTCUT

#include "ActionShortcuts.inl"

#undef PROCESS_NON_STANDARD_ACTION_SHORTCUT
#undef PROCESS_ACTION_SHORTCUT
}

void MainWindow::setupUserShortcuts()
{
    QNDEBUG("MainWindow::setupUserShortcuts");

#define PROCESS_ACTION_SHORTCUT(action, key, ...)                              \
    {                                                                          \
        QKeySequence shortcut = m_shortcutManager.shortcut(                    \
            key, *m_pAccount, QStringLiteral("" __VA_ARGS__));                 \
        if (shortcut.isEmpty())                                                \
        {                                                                      \
            QNTRACE("No shortcut was found for action "                        \
                    << m_pUI->Action##action->objectName());                   \
            auto it = m_shortcutKeyToAction.find(key);                         \
            if (it != m_shortcutKeyToAction.end()) {                           \
                QAction * pAction = it.value();                                \
                pAction->setShortcut(QKeySequence());                          \
                Q_UNUSED(m_shortcutKeyToAction.erase(it))                      \
            }                                                                  \
        }                                                                      \
        else                                                                   \
        {                                                                      \
            m_pUI->Action##action->setShortcut(shortcut);                      \
            m_pUI->Action##action->setShortcutContext(                         \
                Qt::WidgetWithChildrenShortcut);                               \
            m_shortcutKeyToAction[key] = m_pUI->Action##action;                \
        }                                                                      \
    }                                                                          \
// PROCESS_ACTION_SHORTCUT

#define PROCESS_NON_STANDARD_ACTION_SHORTCUT(action, ...)                      \
    {                                                                          \
        QKeySequence shortcut = m_shortcutManager.shortcut(                    \
            QStringLiteral(#action), *m_pAccount,                              \
            QStringLiteral("" __VA_ARGS__));                                   \
        if (shortcut.isEmpty())                                                \
        {                                                                      \
            QNTRACE("No shortcut was found for action "                        \
                    << m_pUI->Action##action->objectName());                   \
            auto it = m_nonStandardShortcutKeyToAction.find(                   \
                QStringLiteral(#action));                                      \
            if (it != m_nonStandardShortcutKeyToAction.end()) {                \
                QAction * pAction = it.value();                                \
                pAction->setShortcut(QKeySequence());                          \
                Q_UNUSED(m_nonStandardShortcutKeyToAction.erase(it))           \
            }                                                                  \
        }                                                                      \
        else                                                                   \
        {                                                                      \
            m_pUI->Action##action->setShortcut(shortcut);                      \
            m_pUI->Action##action->setShortcutContext(                         \
                Qt::WidgetWithChildrenShortcut);                               \
            m_nonStandardShortcutKeyToAction[QStringLiteral(#action)] =        \
                m_pUI->Action##action;                                         \
        }                                                                      \
    }                                                                          \
// PROCESS_NON_STANDARD_ACTION_SHORTCUT

#include "ActionShortcuts.inl"

#undef PROCESS_NON_STANDARD_ACTION_SHORTCUT
#undef PROCESS_ACTION_SHORTCUT
}

void MainWindow::startListeningForShortcutChanges()
{
    QNDEBUG("MainWindow::startListeningForShortcutChanges");

    QObject::connect(&m_shortcutManager,
                     QNSIGNAL(ShortcutManager,shortcutChanged,
                              int,QKeySequence,Account,QString),
                     this,
                     QNSLOT(MainWindow,onShortcutChanged,
                            int,QKeySequence,Account,QString));
    QObject::connect(&m_shortcutManager,
                     QNSIGNAL(ShortcutManager,nonStandardShortcutChanged,
                              QString,QKeySequence,Account,QString),
                     this,
                     QNSLOT(MainWindow,onNonStandardShortcutChanged,
                            QString,QKeySequence,Account,QString));
}

void MainWindow::stopListeningForShortcutChanges()
{
    QNDEBUG("MainWindow::stopListeningForShortcutChanges");

    QObject::disconnect(&m_shortcutManager,
                        QNSIGNAL(ShortcutManager,shortcutChanged,
                                 int,QKeySequence,Account,QString),
                        this,
                        QNSLOT(MainWindow,onShortcutChanged,
                               int,QKeySequence,Account,QString));
    QObject::disconnect(&m_shortcutManager,
                        QNSIGNAL(ShortcutManager,nonStandardShortcutChanged,
                                 QString,QKeySequence,Account,QString),
                        this,
                        QNSLOT(MainWindow,onNonStandardShortcutChanged,
                               QString,QKeySequence,Account,QString));
}

void MainWindow::setupConsumerKeyAndSecret(
    QString & consumerKey, QString & consumerSecret)
{
    const char key[10] = "e3zA914Ol";

    QByteArray consumerKeyObf =
        QByteArray::fromBase64(QStringLiteral("AR8MO2M8Z14WFFcuLzM1Shob").toUtf8());
    char lastChar = 0;
    int size = consumerKeyObf.size();
    for(int i = 0; i < size; ++i) {
        char currentChar = consumerKeyObf[i];
        consumerKeyObf[i] = consumerKeyObf[i] ^ lastChar ^ key[i % 8];
        lastChar = currentChar;
    }

    consumerKey = QString::fromUtf8(consumerKeyObf.constData(),
                                    consumerKeyObf.size());

    QByteArray consumerSecretObf =
        QByteArray::fromBase64(QStringLiteral("BgFLOzJiZh9KSwRyLS8sAg==").toUtf8());
    lastChar = 0;
    size = consumerSecretObf.size();
    for(int i = 0; i < size; ++i) {
        char currentChar = consumerSecretObf[i];
        consumerSecretObf[i] = consumerSecretObf[i] ^ lastChar ^ key[i % 8];
        lastChar = currentChar;
    }

    consumerSecret = QString::fromUtf8(consumerSecretObf.constData(),
                                       consumerSecretObf.size());
}

void MainWindow::persistChosenNoteSortingMode(int index)
{
    QNDEBUG("MainWindow::persistChosenNoteSortingMode: index = " << index);

    ApplicationSettings appSettings(*m_pAccount, QUENTIER_UI_SETTINGS);
    appSettings.beginGroup(QStringLiteral("NoteListView"));
    appSettings.setValue(NOTE_SORTING_MODE_KEY, index);
    appSettings.endGroup();
}

NoteModel::NoteSortingMode::type MainWindow::restoreNoteSortingMode()
{
    QNDEBUG("MainWindow::restoreNoteSortingMode");

    ApplicationSettings appSettings(*m_pAccount, QUENTIER_UI_SETTINGS);
    appSettings.beginGroup(QStringLiteral("NoteListView"));
    if (!appSettings.contains(NOTE_SORTING_MODE_KEY)) {
        QNDEBUG("No persisted note sorting mode within "
                "the settings, nothing to restore");
        appSettings.endGroup();
        return NoteModel::NoteSortingMode::None;
    }

    QVariant data = appSettings.value(NOTE_SORTING_MODE_KEY);
    appSettings.endGroup();

    if (data.isNull()) {
        QNDEBUG("No persisted note sorting mode, nothing to restore");
        return NoteModel::NoteSortingMode::None;
    }

    bool conversionResult = false;
    int index = data.toInt(&conversionResult);
    if (Q_UNLIKELY(!conversionResult))
    {
        ErrorString error(QT_TR_NOOP("Internal error: can't restore the last "
                                     "used note sorting mode, can't convert "
                                     "the persisted setting to the integer index"));
        QNWARNING(error << ", persisted data: " << data);
        onSetStatusBarText(error.localizedString(), SEC_TO_MSEC(30));
        return NoteModel::NoteSortingMode::None;
    }

    return static_cast<NoteModel::NoteSortingMode::type>(index);
}

void MainWindow::persistGeometryAndState()
{
    QNDEBUG("MainWindow::persistGeometryAndState");

    ApplicationSettings appSettings(*m_pAccount, QUENTIER_UI_SETTINGS);
    appSettings.beginGroup(QStringLiteral("MainWindow"));

    appSettings.setValue(MAIN_WINDOW_GEOMETRY_KEY, saveGeometry());
    appSettings.setValue(MAIN_WINDOW_STATE_KEY, saveState());

    bool showSidePanel = m_pUI->ActionShowSidePanel->isChecked();

    bool showFavoritesView = m_pUI->ActionShowFavorites->isChecked();
    bool showNotebooksView = m_pUI->ActionShowNotebooks->isChecked();
    bool showTagsView = m_pUI->ActionShowTags->isChecked();
    bool showSavedSearches = m_pUI->ActionShowSavedSearches->isChecked();
    bool showDeletedNotes = m_pUI->ActionShowDeletedNotes->isChecked();

    QList<int> splitterSizes = m_pUI->splitter->sizes();
    int splitterSizesCount = splitterSizes.count();
    bool splitterSizesCountOk = (splitterSizesCount == 3);

    QList<int> sidePanelSplitterSizes = m_pUI->sidePanelSplitter->sizes();
    int sidePanelSplitterSizesCount = sidePanelSplitterSizes.count();
    bool sidePanelSplitterSizesCountOk = (sidePanelSplitterSizesCount == 5);

    QNTRACE("Show side panel = "
            << (showSidePanel ? "true" : "false")
            << ", show favorites view = "
            << (showFavoritesView ? "true" : "false")
            << ", show notebooks view = "
            << (showNotebooksView ? "true" : "false")
            << ", show tags view = "
            << (showTagsView ? "true" : "false")
            << ", show saved searches view = "
            << (showSavedSearches ? "true" : "false")
            << ", show deleted notes view = "
            << (showDeletedNotes ? "true" : "false")
            << ", splitter sizes ok = "
            << (splitterSizesCountOk ? "true" : "false")
            << ", side panel splitter sizes ok = "
            << (sidePanelSplitterSizesCountOk ? "true" : "false"));

    if (QuentierIsLogLevelActive(LogLevel::TraceLevel))
    {
        QString str;
        QTextStream strm(&str);

        strm << "Splitter sizes: " ;
        for(auto it = splitterSizes.constBegin(),
            end = splitterSizes.constEnd(); it != end; ++it)
        {
            strm << *it << " ";
        }

        strm << "\n";

        strm << "Side panel splitter sizes: ";
        for(auto it = sidePanelSplitterSizes.constBegin(),
            end = sidePanelSplitterSizes.constEnd(); it != end; ++it)
        {
            strm << *it << " ";
        }

        QNTRACE(str);
    }

    if (splitterSizesCountOk && showSidePanel &&
        (showFavoritesView || showNotebooksView || showTagsView ||
         showSavedSearches || showDeletedNotes))
    {
        appSettings.setValue(MAIN_WINDOW_SIDE_PANEL_WIDTH_KEY, splitterSizes[0]);
    }
    else
    {
        appSettings.setValue(MAIN_WINDOW_SIDE_PANEL_WIDTH_KEY, QVariant());
    }

    bool showNotesList = m_pUI->ActionShowNotesList->isChecked();
    if (splitterSizesCountOk && showNotesList) {
        appSettings.setValue(MAIN_WINDOW_NOTE_LIST_WIDTH_KEY, splitterSizes[1]);
    }
    else {
        appSettings.setValue(MAIN_WINDOW_NOTE_LIST_WIDTH_KEY, QVariant());
    }

    if (sidePanelSplitterSizesCountOk && showFavoritesView) {
        appSettings.setValue(MAIN_WINDOW_FAVORITES_VIEW_HEIGHT,
                             sidePanelSplitterSizes[0]);
    }
    else {
        appSettings.setValue(MAIN_WINDOW_FAVORITES_VIEW_HEIGHT, QVariant());
    }

    if (sidePanelSplitterSizesCountOk && showNotebooksView) {
        appSettings.setValue(MAIN_WINDOW_NOTEBOOKS_VIEW_HEIGHT,
                             sidePanelSplitterSizes[1]);
    }
    else {
        appSettings.setValue(MAIN_WINDOW_NOTEBOOKS_VIEW_HEIGHT, QVariant());
    }

    if (sidePanelSplitterSizesCountOk && showTagsView) {
        appSettings.setValue(MAIN_WINDOW_TAGS_VIEW_HEIGHT,
                             sidePanelSplitterSizes[2]);
    }
    else {
        appSettings.setValue(MAIN_WINDOW_TAGS_VIEW_HEIGHT, QVariant());
    }

    if (sidePanelSplitterSizesCountOk && showSavedSearches) {
        appSettings.setValue(MAIN_WINDOW_SAVED_SEARCHES_VIEW_HEIGHT,
                             sidePanelSplitterSizes[3]);
    }
    else {
        appSettings.setValue(MAIN_WINDOW_SAVED_SEARCHES_VIEW_HEIGHT, QVariant());
    }

    if (sidePanelSplitterSizesCountOk && showDeletedNotes) {
        appSettings.setValue(MAIN_WINDOW_DELETED_NOTES_VIEW_HEIGHT,
                             sidePanelSplitterSizes[4]);
    }
    else {
        appSettings.setValue(MAIN_WINDOW_DELETED_NOTES_VIEW_HEIGHT, QVariant());
    }

    appSettings.endGroup();
}

void MainWindow::restoreGeometryAndState()
{
    QNDEBUG("MainWindow::restoreGeometryAndState");

    ApplicationSettings appSettings(*m_pAccount, QUENTIER_UI_SETTINGS);
    appSettings.beginGroup(QStringLiteral("MainWindow"));
    QByteArray savedGeometry =
        appSettings.value(MAIN_WINDOW_GEOMETRY_KEY).toByteArray();
    QByteArray savedState = appSettings.value(MAIN_WINDOW_STATE_KEY).toByteArray();

    appSettings.endGroup();

    m_geometryRestored = restoreGeometry(savedGeometry);
    m_stateRestored = restoreState(savedState);

    QNTRACE("Geometry restored = " << (m_geometryRestored ? "true" : "false")
            << ", state restored = " << (m_stateRestored ? "true" : "false"));

    scheduleSplitterSizesRestoration();
}

void MainWindow::restoreSplitterSizes()
{
    QNDEBUG("MainWindow::restoreSplitterSizes");

    ApplicationSettings appSettings(*m_pAccount, QUENTIER_UI_SETTINGS);
    appSettings.beginGroup(QStringLiteral("MainWindow"));

    QVariant sidePanelWidth = appSettings.value(MAIN_WINDOW_SIDE_PANEL_WIDTH_KEY);
    QVariant notesListWidth = appSettings.value(MAIN_WINDOW_NOTE_LIST_WIDTH_KEY);

    QVariant favoritesViewHeight =
        appSettings.value(MAIN_WINDOW_FAVORITES_VIEW_HEIGHT);
    QVariant notebooksViewHeight =
        appSettings.value(MAIN_WINDOW_NOTEBOOKS_VIEW_HEIGHT);
    QVariant tagsViewHeight = appSettings.value(MAIN_WINDOW_TAGS_VIEW_HEIGHT);
    QVariant savedSearchesViewHeight =
        appSettings.value(MAIN_WINDOW_SAVED_SEARCHES_VIEW_HEIGHT);
    QVariant deletedNotesViewHeight =
        appSettings.value(MAIN_WINDOW_DELETED_NOTES_VIEW_HEIGHT);

    appSettings.endGroup();

    QNTRACE("Side panel width = " << sidePanelWidth
            << ", notes list width = " << notesListWidth
            << ", favorites view height = " << favoritesViewHeight
            << ", notebooks view height = " << notebooksViewHeight
            << ", tags view height = " << tagsViewHeight
            << ", saved searches view height = " << savedSearchesViewHeight
            << ", deleted notes view height = " << deletedNotesViewHeight);

    bool showSidePanel = m_pUI->ActionShowSidePanel->isChecked();
    bool showNotesList = m_pUI->ActionShowNotesList->isChecked();

    bool showFavoritesView = m_pUI->ActionShowFavorites->isChecked();
    bool showNotebooksView = m_pUI->ActionShowNotebooks->isChecked();
    bool showTagsView = m_pUI->ActionShowTags->isChecked();
    bool showSavedSearches = m_pUI->ActionShowSavedSearches->isChecked();
    bool showDeletedNotes = m_pUI->ActionShowDeletedNotes->isChecked();

    QList<int> splitterSizes = m_pUI->splitter->sizes();
    int splitterSizesCount = splitterSizes.count();
    if (splitterSizesCount == 3)
    {
        int totalWidth = 0;
        for(int i = 0; i < splitterSizesCount; ++i) {
            totalWidth += splitterSizes[i];
        }

        if (sidePanelWidth.isValid() && showSidePanel &&
            (showFavoritesView || showNotebooksView || showTagsView ||
             showSavedSearches || showDeletedNotes))
        {
            bool conversionResult = false;
            int sidePanelWidthInt = sidePanelWidth.toInt(&conversionResult);
            if (conversionResult) {
                splitterSizes[0] = sidePanelWidthInt;
                QNTRACE("Restored side panel width: " << sidePanelWidthInt);
            }
            else {
                QNDEBUG("Can't restore the side panel width: can't "
                        << "convert the persisted width to int: "
                        << sidePanelWidth);
            }
        }

        if (notesListWidth.isValid() && showNotesList)
        {
            bool conversionResult = false;
            int notesListWidthInt = notesListWidth.toInt(&conversionResult);
            if (conversionResult) {
                splitterSizes[1] = notesListWidthInt;
                QNTRACE("Restored notes list panel width: " << notesListWidthInt);
            }
            else {
                QNDEBUG("Can't restore the notes list panel width: "
                        << "can't convert the persisted width to int: "
                        << notesListWidth);
            }
        }

        splitterSizes[2] = totalWidth - splitterSizes[0] - splitterSizes[1];

        if (QuentierIsLogLevelActive(LogLevel::TraceLevel))
        {
            QString str;
            QTextStream strm(&str);

            strm << "Splitter sizes before restoring (total " << totalWidth
                << "): ";
            QList<int> splitterSizesBefore = m_pUI->splitter->sizes();
            for(auto it = splitterSizesBefore.constBegin(),
                end = splitterSizesBefore.constEnd(); it != end; ++it)
            {
                strm << *it << " ";
            }

            QNTRACE(str);
        }

        m_pUI->splitter->setSizes(splitterSizes);

        QWidget * pSidePanel = m_pUI->splitter->widget(0);
        QSizePolicy sidePanelSizePolicy = pSidePanel->sizePolicy();
        sidePanelSizePolicy.setHorizontalPolicy(QSizePolicy::Minimum);
        sidePanelSizePolicy.setHorizontalStretch(0);
        pSidePanel->setSizePolicy(sidePanelSizePolicy);

        QWidget * pNoteListView = m_pUI->splitter->widget(1);
        QSizePolicy noteListViewSizePolicy = pNoteListView->sizePolicy();
        noteListViewSizePolicy.setHorizontalPolicy(QSizePolicy::Minimum);
        noteListViewSizePolicy.setHorizontalStretch(0);
        pNoteListView->setSizePolicy(noteListViewSizePolicy);

        QWidget * pNoteEditor = m_pUI->splitter->widget(2);
        QSizePolicy noteEditorSizePolicy = pNoteEditor->sizePolicy();
        noteEditorSizePolicy.setHorizontalPolicy(QSizePolicy::Expanding);
        noteEditorSizePolicy.setHorizontalStretch(1);
        pNoteEditor->setSizePolicy(noteEditorSizePolicy);

        QNTRACE("Set splitter sizes");

        if (QuentierIsLogLevelActive(LogLevel::TraceLevel))
        {
            QString str;
            QTextStream strm(&str);

            strm << "Splitter sizes after restoring: ";
            QList<int> splitterSizesAfter = m_pUI->splitter->sizes();
            for(auto it = splitterSizesAfter.constBegin(),
                end = splitterSizesAfter.constEnd(); it != end; ++it)
            {
                strm << *it << " ";
            }

            QNTRACE(str);
        }
    }
    else
    {
        ErrorString error(QT_TR_NOOP("Internal error: can't restore the widths "
                                     "for side panel, note list view and note "
                                     "editors view: wrong number of sizes within "
                                     "the splitter"));
        QNWARNING(error << ", sizes count: " << splitterSizesCount);
        onSetStatusBarText(error.localizedString(), SEC_TO_MSEC(30));
    }

    QList<int> sidePanelSplitterSizes = m_pUI->sidePanelSplitter->sizes();
    int sidePanelSplitterSizesCount = sidePanelSplitterSizes.count();
    if (sidePanelSplitterSizesCount == 5)
    {
        int totalHeight = 0;
        for(int i = 0; i < 5; ++i) {
            totalHeight += sidePanelSplitterSizes[i];
        }

        if (QuentierIsLogLevelActive(LogLevel::TraceLevel))
        {
            QString str;
            QTextStream strm(&str);

            strm << "Side panel splitter sizes before restoring (total "
                << totalHeight << "): ";
            for(auto it = sidePanelSplitterSizes.constBegin(),
                end = sidePanelSplitterSizes.constEnd(); it != end; ++it)
            {
                strm << *it << " ";
            }

            QNTRACE(str);
        }

        if (showFavoritesView && favoritesViewHeight.isValid())
        {
            bool conversionResult = false;
            int favoritesViewHeightInt =
                favoritesViewHeight.toInt(&conversionResult);
            if (conversionResult) {
                sidePanelSplitterSizes[0] = favoritesViewHeightInt;
                QNTRACE("Restored favorites view height: "
                        << favoritesViewHeightInt);
            }
            else {
                QNDEBUG("Can't restore the favorites view height: "
                        << "can't convert the persisted height to int: "
                        << favoritesViewHeight);
            }
        }

        if (showNotebooksView && notebooksViewHeight.isValid())
        {
            bool conversionResult = false;
            int notebooksViewHeightInt =
                notebooksViewHeight.toInt(&conversionResult);
            if (conversionResult) {
                sidePanelSplitterSizes[1] = notebooksViewHeightInt;
                QNTRACE("Restored notebooks view height: "
                        << notebooksViewHeightInt);
            }
            else {
                QNDEBUG("Can't restore the notebooks view height: "
                        << "can't convert the persisted height to int: "
                        << notebooksViewHeight);
            }
        }

        if (showTagsView && tagsViewHeight.isValid())
        {
            bool conversionResult = false;
            int tagsViewHeightInt = tagsViewHeight.toInt(&conversionResult);
            if (conversionResult) {
                sidePanelSplitterSizes[2] = tagsViewHeightInt;
                QNTRACE("Restored tags view height: " << tagsViewHeightInt);
            }
            else {
                QNDEBUG("Can't restore the tags view height: can't "
                        << "convert the persisted height to int: "
                        << tagsViewHeight);
            }
        }

        if (showSavedSearches && savedSearchesViewHeight.isValid())
        {
            bool conversionResult = false;
            int savedSearchesViewHeightInt =
                savedSearchesViewHeight.toInt(&conversionResult);
            if (conversionResult) {
                sidePanelSplitterSizes[3] = savedSearchesViewHeightInt;
                QNTRACE("Restored saved searches view height: "
                        << savedSearchesViewHeightInt);
            }
            else {
                QNDEBUG("Can't restore the saved searches view "
                        << "height: can't convert the persisted height to int: "
                        << savedSearchesViewHeight);
            }
        }

        if (showDeletedNotes && deletedNotesViewHeight.isValid())
        {
            bool conversionResult = false;
            int deletedNotesViewHeightInt =
                deletedNotesViewHeight.toInt(&conversionResult);
            if (conversionResult) {
                sidePanelSplitterSizes[4] = deletedNotesViewHeightInt;
                QNTRACE("Restored deleted notes view height: "
                        << deletedNotesViewHeightInt);
            }
            else {
                QNDEBUG("Can't restore the deleted notes view "
                        << "height: can't convert the persisted "
                        << "height to int: " << deletedNotesViewHeight);
            }
        }

        int totalHeightAfterRestore = 0;
        for(int i = 0; i < 5; ++i) {
            totalHeightAfterRestore += sidePanelSplitterSizes[i];
        }

        if (QuentierIsLogLevelActive(LogLevel::TraceLevel))
        {
            QString str;
            QTextStream strm(&str);

            strm << "Side panel splitter sizes after restoring (total "
                << totalHeightAfterRestore << "): ";
            for(auto it = sidePanelSplitterSizes.constBegin(),
                end = sidePanelSplitterSizes.constEnd(); it != end; ++it)
            {
                strm << *it << " ";
            }

            QNTRACE(str);
        }

        m_pUI->sidePanelSplitter->setSizes(sidePanelSplitterSizes);
        QNTRACE("Set side panel splitter sizes");
    }
    else
    {
        ErrorString error(QT_TR_NOOP("Internal error: can't restore the heights "
                                     "of side panel's views: wrong number of "
                                     "sizes within the splitter"));
        QNWARNING(error << ", sizes count: " << splitterSizesCount);
        onSetStatusBarText(error.localizedString(), SEC_TO_MSEC(30));
    }
}

void MainWindow::scheduleSplitterSizesRestoration()
{
    QNDEBUG("MainWindow::scheduleSplitterSizesRestoration");

    if (!m_shown) {
        QNDEBUG("Not shown yet, won't do anything");
        return;
    }

    if (m_splitterSizesRestorationDelayTimerId != 0) {
        QNDEBUG("Splitter sizes restoration already scheduled, "
                << "timer id = "
                << m_splitterSizesRestorationDelayTimerId);
        return;
    }

    m_splitterSizesRestorationDelayTimerId =
        startTimer(RESTORE_SPLITTER_SIZES_DELAY);
    if (Q_UNLIKELY(m_splitterSizesRestorationDelayTimerId == 0)) {
        QNWARNING("Failed to start the timer to delay "
                  "the restoration of splitter sizes");
        return;
    }

    QNDEBUG("Started the timer to delay the restoration of "
            << "splitter sizes: " << m_splitterSizesRestorationDelayTimerId);
}

void MainWindow::scheduleGeometryAndStatePersisting()
{
    QNDEBUG("MainWindow::scheduleGeometryAndStatePersisting");

    if (!m_shown) {
        QNDEBUG("Not shown yet, won't do anything");
        return;
    }

    if (m_geometryAndStatePersistingDelayTimerId != 0) {
        QNDEBUG("Persisting already scheduled, timer id = "
                << m_geometryAndStatePersistingDelayTimerId);
        return;
    }

    m_geometryAndStatePersistingDelayTimerId =
        startTimer(PERSIST_GEOMETRY_AND_STATE_DELAY);
    if (Q_UNLIKELY(m_geometryAndStatePersistingDelayTimerId == 0)) {
        QNWARNING("Failed to start the timer to delay "
                  "the persistence of MainWindow's state and "
                  "geometry");
        return;
    }

    QNDEBUG("Started the timer to delay the persistence of "
            << "MainWindow's state and geometry: timer id = "
            << m_geometryAndStatePersistingDelayTimerId);
}

template <class T>
void MainWindow::refreshThemeIcons()
{
    QList<T*> objects = findChildren<T*>();
    QNDEBUG("Found " << objects.size() << " child objects");

    for(auto it = objects.constBegin(),
        end = objects.constEnd(); it != end; ++it)
    {
        T * object = *it;
        if (Q_UNLIKELY(!object)) {
            continue;
        }

        QIcon icon = object->icon();
        if (icon.isNull()) {
            continue;
        }

        QString iconName = icon.name();
        if (iconName.isEmpty()) {
            continue;
        }

        QIcon newIcon;
        if (QIcon::hasThemeIcon(iconName)) {
            newIcon = QIcon::fromTheme(iconName);
        }
        else {
            newIcon.addFile(QStringLiteral("."), QSize(),
                            QIcon::Normal, QIcon::Off);
        }

        object->setIcon(newIcon);
    }
}
