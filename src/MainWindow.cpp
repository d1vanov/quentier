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

#include "MainWindow.h"
#include "SettingsNames.h"
#include "DefaultSettings.h"
#include "AsyncFileWriter.h"
#include "SystemTrayIconManager.h"
#include "ActionsInfo.h"
#include "EditNoteDialogsManager.h"
#include "NoteFiltersManager.h"
#include "EnexExporter.h"
#include "EnexImporter.h"
#include "NetworkProxySettingsHelpers.h"
#include "models/NoteFilterModel.h"
#include "color-picker-tool-button/ColorPickerToolButton.h"
#include "insert-table-tool-button/InsertTableToolButton.h"
#include "insert-table-tool-button/TableSettingsDialog.h"
#include "widgets/FindAndReplaceWidget.h"
#include "delegates/NotebookItemDelegate.h"
#include "delegates/SynchronizableColumnDelegate.h"
#include "delegates/DirtyColumnDelegate.h"
#include "delegates/FavoriteItemDelegate.h"
#include "delegates/FromLinkedNotebookColumnDelegate.h"
#include "delegates/NoteItemDelegate.h"
#include "delegates/TagItemDelegate.h"
#include "delegates/DeletedNoteItemDelegate.h"
#include "dialogs/AddOrEditNotebookDialog.h"
#include "dialogs/AddOrEditTagDialog.h"
#include "dialogs/AddOrEditSavedSearchDialog.h"
#include "dialogs/EnexExportDialog.h"
#include "dialogs/EnexImportDialog.h"
#include "dialogs/LocalStorageVersionTooHighDialog.h"
#include "dialogs/PreferencesDialog.h"
#include "dialogs/WelcomeToQuentierDialog.h"
#include "initialization/DefaultAccountFirstNotebookAndNoteCreator.h"
#include "models/ColumnChangeRerouter.h"
#include "views/ItemView.h"
#include "views/DeletedNoteItemView.h"
#include "views/NotebookItemView.h"
#include "views/NoteListView.h"
#include "views/TagItemView.h"
#include "views/SavedSearchItemView.h"
#include "views/FavoriteItemView.h"

#include "widgets/TabWidget.h"
using quentier::TabWidget;

#include "widgets/FilterByNotebookWidget.h"
using quentier::FilterByNotebookWidget;

#include "widgets/FilterByTagWidget.h"
using quentier::FilterByTagWidget;

#include "widgets/FilterBySavedSearchWidget.h"
using quentier::FilterBySavedSearchWidget;

#include "widgets/LogViewerWidget.h"
using quentier::LogViewerWidget;

#include "widgets/NotebookModelItemInfoWidget.h"
#include "widgets/TagModelItemInfoWidget.h"
#include "widgets/SavedSearchModelItemInfoWidget.h"
#include "widgets/AboutQuentierWidget.h"
#include "dialogs/EditNoteDialog.h"

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
#include <cmath>
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
#include <cmath>
#include <algorithm>

#define NOTIFY_ERROR(error) \
    QNWARNING(error); \
    onSetStatusBarText(error, SEC_TO_MSEC(30))

#define FILTERS_VIEW_STATUS_KEY QStringLiteral("ViewExpandedStatus")
#define NOTE_SORTING_MODE_KEY QStringLiteral("NoteSortingMode")

#define MAIN_WINDOW_GEOMETRY_KEY QStringLiteral("Geometry")
#define MAIN_WINDOW_STATE_KEY QStringLiteral("State")

#define MAIN_WINDOW_SIDE_PANEL_WIDTH_KEY QStringLiteral("SidePanelWidth")
#define MAIN_WINDOW_NOTE_LIST_WIDTH_KEY QStringLiteral("NoteListWidth")

#define MAIN_WINDOW_FAVORITES_VIEW_HEIGHT QStringLiteral("FavoritesViewHeight")
#define MAIN_WINDOW_NOTEBOOKS_VIEW_HEIGHT QStringLiteral("NotebooksViewHeight")
#define MAIN_WINDOW_TAGS_VIEW_HEIGHT QStringLiteral("TagsViewHeight")
#define MAIN_WINDOW_SAVED_SEARCHES_VIEW_HEIGHT QStringLiteral("SavedSearchesViewHeight")
#define MAIN_WINDOW_DELETED_NOTES_VIEW_HEIGHT QStringLiteral("DeletedNotesViewHeight")

#define DARKER_PANEL_STYLE_NAME QStringLiteral("Darker")
#define LIGHTER_PANEL_STYLE_NAME QStringLiteral("Lighter")

#define PERSIST_GEOMETRY_AND_STATE_DELAY (500)
#define RESTORE_SPLITTER_SIZES_DELAY (200)

using namespace quentier;

MainWindow::MainWindow(QWidget * pParentWidget) :
    QMainWindow(pParentWidget),
    m_pUI(new Ui::MainWindow),
    m_currentStatusBarChildWidget(Q_NULLPTR),
    m_lastNoteEditorHtml(),
    m_nativeIconThemeName(),
    m_pAvailableAccountsActionGroup(new QActionGroup(this)),
    m_pAvailableAccountsSubMenu(Q_NULLPTR),
    m_pAccountManager(new AccountManager(this)),
    m_pAccount(),
    m_pSystemTrayIconManager(Q_NULLPTR),
    m_pLocalStorageManagerThread(Q_NULLPTR),
    m_pLocalStorageManagerAsync(Q_NULLPTR),
    m_lastLocalStorageSwitchUserRequest(),
    m_pSynchronizationManagerThread(Q_NULLPTR),
    m_pAuthenticationManager(Q_NULLPTR),
    m_pSynchronizationManager(Q_NULLPTR),
    m_synchronizationManagerHost(),
    m_applicationProxyBeforeNewEvernoteAccountAuthenticationRequest(),
    m_pendingNewEvernoteAccountAuthentication(false),
    m_pendingSwitchToNewEvernoteAccount(false),
    m_syncInProgress(false),
    m_pendingSynchronizationManagerSetAccount(false),
    m_pendingSynchronizationManagerSetDownloadNoteThumbnailsOption(false),
    m_pendingSynchronizationManagerSetDownloadInkNoteImagesOption(false),
    m_pendingSynchronizationManagerSetInkNoteImagesStoragePath(false),
    m_pendingSynchronizationManagerResponseToStartSync(false),
    m_syncApiRateLimitExceeded(false),
    m_animatedSyncButtonIcon(QStringLiteral(":/sync/sync.gif")),
    m_runSyncPeriodicallyTimerId(0),
    m_notebookCache(),
    m_tagCache(),
    m_savedSearchCache(),
    m_noteCache(),
    m_pNotebookModel(Q_NULLPTR),
    m_pTagModel(Q_NULLPTR),
    m_pSavedSearchModel(Q_NULLPTR),
    m_pNoteModel(Q_NULLPTR),
    m_pNotebookModelColumnChangeRerouter(new ColumnChangeRerouter(NotebookModel::Columns::NumNotesPerNotebook,
                                                                  NotebookModel::Columns::Name, this)),
    m_pTagModelColumnChangeRerouter(new ColumnChangeRerouter(TagModel::Columns::NumNotesPerTag,
                                                             TagModel::Columns::Name, this)),
    m_pNoteModelColumnChangeRerouter(new ColumnChangeRerouter(NoteModel::Columns::PreviewText,
                                                              NoteModel::Columns::Title, this)),
    m_pFavoritesModelColumnChangeRerouter(new ColumnChangeRerouter(FavoritesModel::Columns::NumNotesTargeted,
                                                                   FavoritesModel::Columns::DisplayName, this)),
    m_pDeletedNotesModel(Q_NULLPTR),
    m_pFavoritesModel(Q_NULLPTR),
    m_blankModel(),
    m_pNoteFilterModel(Q_NULLPTR),
    m_pNoteFiltersManager(Q_NULLPTR),
    m_setDefaultAccountsFirstNoteAsCurrentDelayTimerId(0),
    m_defaultAccountFirstNoteLocalUid(),
    m_pNoteEditorTabsAndWindowsCoordinator(Q_NULLPTR),
    m_pEditNoteDialogsManager(Q_NULLPTR),
    m_pUndoStack(new QUndoStack(this)),
    m_noteSearchQueryValidated(false),
    m_styleSheetInfo(),
    m_currentPanelStyle(),
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
    QNTRACE(QStringLiteral("MainWindow constructor"));

    setupAccountManager();

    bool createdDefaultAccount = false;
    m_pAccount.reset(new Account(m_pAccountManager->currentAccount(&createdDefaultAccount)));
    if (createdDefaultAccount && !onceDisplayedGreeterScreen()) {
        m_pendingGreeterDialog = true;
        setOnceDisplayedGreeterScreen();
    }

    restoreNetworkProxySettingsForAccount(*m_pAccount);

    m_pSystemTrayIconManager = new SystemTrayIconManager(*m_pAccountManager, this);

    setupThemeIcons();

    m_pUI->setupUi(this);
    setupAccountSpecificUiElements();

    if (m_nativeIconThemeName.isEmpty()) {
        m_pUI->ActionIconsNative->setVisible(false);
        m_pUI->ActionIconsNative->setDisabled(true);
    }

    collectBaseStyleSheets();
    setupPanelOverlayStyleSheets();

    m_pAvailableAccountsActionGroup->setExclusive(true);

#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
    fixupQt4StyleSheets();
#endif

#if QT_VERSION >= QT_VERSION_CHECK(5, 2, 0)
    m_pUI->searchQueryLineEdit->setClearButtonEnabled(true);
#endif

    setWindowTitleForAccount(*m_pAccount);

    setupLocalStorageManager();
    setupModels();
    setupViews();
    setupNoteFilters();

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

    Q_UNUSED(checkLocalStorageVersion(*m_pAccount))
}

MainWindow::~MainWindow()
{
    if (m_pLocalStorageManagerThread) {
        m_pLocalStorageManagerThread->quit();
    }

    delete m_pUI;
}

void MainWindow::show()
{
    QNDEBUG(QStringLiteral("MainWindow::show"));

    QWidget::show();

    m_shown = true;

    // sadly, can't get it to work any other way :(
    if (!m_filtersViewExpanded) {
        foldFiltersView();
    }
    else {
        expandFiltersView();
    }

    restoreGeometryAndState();

    if (!m_geometryRestored) {
        setupInitialChildWidgetsWidths();
        persistGeometryAndState();
    }

    if (Q_UNLIKELY(m_pendingGreeterDialog))
    {
        m_pendingGreeterDialog = false;

        QScopedPointer<WelcomeToQuentierDialog> pDialog(new WelcomeToQuentierDialog(this));
        pDialog->setWindowModality(Qt::WindowModal);
        if (pDialog->exec() == QDialog::Accepted) {
            QNDEBUG(QStringLiteral("Log in to Evernote account option was chosen on the greeter screen"));
            onEvernoteAccountAuthenticationRequested(QStringLiteral("www.evernote.com"), QNetworkProxy(QNetworkProxy::NoProxy));
        }
    }
}

const SystemTrayIconManager & MainWindow::systemTrayIconManager() const
{
    return *m_pSystemTrayIconManager;
}

void MainWindow::connectActionsToSlots()
{
    QNDEBUG(QStringLiteral("MainWindow::connectActionsToSlots"));

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
                     this, QNSLOT(MainWindow,onShowSettingsDialogAction));

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
                     this, QNSLOT(MainWindow,onSwitchIconsToNativeAction));
    QObject::connect(m_pUI->ActionIconsOxygen, QNSIGNAL(QAction,triggered),
                     this, QNSLOT(MainWindow,onSwitchIconsToOxygenAction));
    QObject::connect(m_pUI->ActionIconsTango, QNSIGNAL(QAction,triggered),
                     this, QNSLOT(MainWindow,onSwitchIconsToTangoAction));
    QObject::connect(m_pUI->ActionPanelStyleBuiltIn, QNSIGNAL(QAction,triggered),
                     this, QNSLOT(MainWindow,onSwitchPanelStyleToBuiltIn));
    QObject::connect(m_pUI->ActionPanelStyleLighter, QNSIGNAL(QAction,triggered),
                     this, QNSLOT(MainWindow,onSwitchPanelStyleToLighter));
    QObject::connect(m_pUI->ActionPanelStyleDarker, QNSIGNAL(QAction,triggered),
                     this, QNSLOT(MainWindow,onSwitchPanelStyleToDarker));
    // Help menu actions
    QObject::connect(m_pUI->ActionShowNoteSource, QNSIGNAL(QAction,triggered),
                     this, QNSLOT(MainWindow,onShowNoteSource));
    QObject::connect(m_pUI->ActionViewLogs, QNSIGNAL(QAction,triggered),
                     this, QNSLOT(MainWindow,onViewLogsActionTriggered));
    QObject::connect(m_pUI->ActionAbout, QNSIGNAL(QAction,triggered),
                     this, QNSLOT(MainWindow,onShowInfoAboutQuentierActionTriggered));
}

void MainWindow::connectViewButtonsToSlots()
{
    QNDEBUG(QStringLiteral("MainWindow::connectViewButtonsToSlots"));

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

    QObject::connect(m_pUI->filtersViewTogglePushButton, QNSIGNAL(QPushButton,clicked),
                     this, QNSLOT(MainWindow,onFiltersViewTogglePushButtonPressed));
}

void MainWindow::connectNoteSearchActionsToSlots()
{
    QNDEBUG(QStringLiteral("MainWindow::connectNoteSearchActionsToSlots"));

    QObject::connect(m_pUI->searchQueryLineEdit, QNSIGNAL(QLineEdit,textChanged,const QString&),
                     this, QNSLOT(MainWindow,onNoteSearchQueryChanged,const QString&));
    QObject::connect(m_pUI->searchQueryLineEdit, QNSIGNAL(QLineEdit,returnPressed),
                     this, QNSLOT(MainWindow,onNoteSearchQueryReady));
    QObject::connect(m_pUI->saveSearchPushButton, QNSIGNAL(QPushButton,clicked),
                     this, QNSLOT(MainWindow,onSaveNoteSearchQueryButtonPressed));
}

void MainWindow::connectToolbarButtonsToSlots()
{
    QNDEBUG(QStringLiteral("MainWindow::connectToolbarButtonsToSlots"));

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
    QNDEBUG(QStringLiteral("MainWindow::connectSystemTrayIconManagerSignalsToSlots"));

    QObject::connect(m_pSystemTrayIconManager, QNSIGNAL(SystemTrayIconManager,notifyError,ErrorString),
                     this, QNSLOT(MainWindow,onSystemTrayIconManagerError,ErrorString));
    QObject::connect(m_pSystemTrayIconManager, QNSIGNAL(SystemTrayIconManager,newTextNoteAdditionRequested),
                     this, QNSLOT(MainWindow,onNewNoteRequestedFromSystemTrayIcon));
    QObject::connect(m_pSystemTrayIconManager, QNSIGNAL(SystemTrayIconManager,quitRequested),
                     this, QNSLOT(MainWindow,onQuitRequestedFromSystemTrayIcon));
    QObject::connect(m_pSystemTrayIconManager, QNSIGNAL(SystemTrayIconManager,accountSwitchRequested,Account),
                     this, QNSLOT(MainWindow,onAccountSwitchRequested,Account));
}

void MainWindow::addMenuActionsToMainWindow()
{
    QNDEBUG(QStringLiteral("MainWindow::addMenuActionsToMainWindow"));

    // NOTE: adding the actions from the menu bar's menus is required for getting the shortcuts
    // of these actions to work properly; action shortcuts only fire when the menu is shown
    // which is not really the purpose behind those shortcuts
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
    QAction * separatorAction = m_pUI->menuFile->insertSeparator(m_pUI->ActionQuit);

    QAction * switchAccountSubMenuAction =
            m_pUI->menuFile->insertMenu(separatorAction, m_pAvailableAccountsSubMenu);
    Q_UNUSED(m_pUI->menuFile->insertSeparator(switchAccountSubMenuAction));

    updateSubMenuWithAvailableAccounts();
}

void MainWindow::updateSubMenuWithAvailableAccounts()
{
    QNDEBUG(QStringLiteral("MainWindow::updateSubMenuWithAvailableAccounts"));

    if (Q_UNLIKELY(!m_pAvailableAccountsSubMenu)) {
        QNDEBUG(QStringLiteral("No available accounts sub-menu"));
        return;
    }

    delete m_pAvailableAccountsActionGroup;
    m_pAvailableAccountsActionGroup = new QActionGroup(this);
    m_pAvailableAccountsActionGroup->setExclusive(true);

    m_pAvailableAccountsSubMenu->clear();

    const QVector<Account> & availableAccounts = m_pAccountManager->availableAccounts();

    int numAvailableAccounts = availableAccounts.size();
    for(int i = 0; i < numAvailableAccounts; ++i)
    {
        const Account & availableAccount = availableAccounts[i];
        QNTRACE(QStringLiteral("Examining the available account: ") << availableAccount);

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

                if (host == QStringLiteral("sandbox.evernote.com")) {
                    availableAccountRepresentationName += QStringLiteral("sandbox");
                }
                else if (host == QStringLiteral("app.yinxiang.com")) {
                    availableAccountRepresentationName += QStringLiteral("Yinxiang Biji");
                }
                else {
                    availableAccountRepresentationName += host;
                }

                availableAccountRepresentationName += QStringLiteral(")");
            }
        }

        QAction * pAccountAction = new QAction(availableAccountRepresentationName, Q_NULLPTR);
        m_pAvailableAccountsSubMenu->addAction(pAccountAction);

        pAccountAction->setData(i);
        pAccountAction->setCheckable(true);

        if (!m_pAccount.isNull() && (*m_pAccount == availableAccount)) {
            pAccountAction->setChecked(true);
        }

        addAction(pAccountAction);
        QObject::connect(pAccountAction, QNSIGNAL(QAction,toggled,bool),
                         this, QNSLOT(MainWindow,onSwitchAccountActionToggled,bool));

        m_pAvailableAccountsActionGroup->addAction(pAccountAction);
    }

    if (Q_LIKELY(numAvailableAccounts != 0)) {
        Q_UNUSED(m_pAvailableAccountsSubMenu->addSeparator())
    }

    QAction * pAddAccountAction = m_pAvailableAccountsSubMenu->addAction(tr("Add account"));
    addAction(pAddAccountAction);
    QObject::connect(pAddAccountAction, QNSIGNAL(QAction,triggered,bool),
                     this, QNSLOT(MainWindow,onAddAccountActionTriggered,bool));

    QAction * pManageAccountsAction = m_pAvailableAccountsSubMenu->addAction(tr("Manage accounts"));
    addAction(pManageAccountsAction);
    QObject::connect(pManageAccountsAction, QNSIGNAL(QAction,triggered,bool),
                     this, QNSLOT(MainWindow,onManageAccountsActionTriggered,bool));
}

void MainWindow::setupInitialChildWidgetsWidths()
{
    QNDEBUG(QStringLiteral("MainWindow::setupInitialChildWidgetsWidths"));

    int totalWidth = width();

    // 1/5 - for side view, 1/5 - for note list view, 3/5 - for the note editor
    int partWidth = totalWidth / 5;

    QNTRACE(QStringLiteral("Total width = ") << totalWidth << QStringLiteral(", part width = ") << partWidth);

    QList<int> splitterSizes = m_pUI->splitter->sizes();
    int splitterSizesCount = splitterSizes.count();
    if (Q_UNLIKELY(splitterSizesCount != 3)) {
        ErrorString error(QT_TR_NOOP("Internal error: can't setup the proper initial widths "
                                     "for side panel, note list view and note editors view: "
                                     "wrong number of sizes within the splitter"));
        QNWARNING(error << QStringLiteral(", sizes count: ") << splitterSizesCount);
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
    QNDEBUG(QStringLiteral("MainWindow::setWindowTitleForAccount: ") << account);

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
    QNDEBUG(QStringLiteral("MainWindow::currentNoteEditorTab"));

    if (Q_UNLIKELY(m_pUI->noteEditorsTabWidget->count() == 0)) {
        QNTRACE(QStringLiteral("No open note editors"));
        return Q_NULLPTR;
    }

    int currentIndex = m_pUI->noteEditorsTabWidget->currentIndex();
    if (Q_UNLIKELY(currentIndex < 0)) {
        QNTRACE(QStringLiteral("No current note editor"));
        return Q_NULLPTR;
    }

    QWidget * currentWidget = m_pUI->noteEditorsTabWidget->widget(currentIndex);
    if (Q_UNLIKELY(!currentWidget)) {
        QNTRACE(QStringLiteral("No current widget"));
        return Q_NULLPTR;
    }

    NoteEditorWidget * noteEditorWidget = qobject_cast<NoteEditorWidget*>(currentWidget);
    if (Q_UNLIKELY(!noteEditorWidget)) {
        QNWARNING(QStringLiteral("Can't cast current note tag widget's widget to note editor widget"));
        return Q_NULLPTR;
    }

    return noteEditorWidget;
}

void MainWindow::createNewNote(NoteEditorTabsAndWindowsCoordinator::NoteEditorMode::type noteEditorMode)
{
    QNDEBUG(QStringLiteral("MainWindow::createNewNote: note editor mode = ") << noteEditorMode);

    if (Q_UNLIKELY(!m_pNoteEditorTabsAndWindowsCoordinator)) {
        QNDEBUG(QStringLiteral("No note editor tabs and windows coordinator, probably "
                               "the button was pressed too quickly on startup, skipping"));
        return;
    }

    if (Q_UNLIKELY(!m_pNoteModel)) {
        Q_UNUSED(internalErrorMessageBox(this, tr("Can't create a new note: note model is unexpectedly null")))
        return;
    }

    if (Q_UNLIKELY(!m_pNotebookModel)) {
        Q_UNUSED(internalErrorMessageBox(this, tr("Can't create a new note: notebook model is unexpectedly null")))
        return;
    }

    QModelIndex currentNotebookIndex = m_pUI->notebooksTreeView->currentlySelectedItemIndex();
    if (Q_UNLIKELY(!currentNotebookIndex.isValid())) {
        Q_UNUSED(informationMessageBox(this, tr("No notebook is selected"),
                                       tr("Please select the notebook in which you want to create the note; "
                                          "if you don't have any notebooks yet, create one")))
        return;
    }

    const NotebookModelItem * pNotebookModelItem = m_pNotebookModel->itemForIndex(currentNotebookIndex);
    if (Q_UNLIKELY(!pNotebookModelItem)) {
        Q_UNUSED(internalErrorMessageBox(this, tr("Can't create a new note: can't find the notebook model item "
                                                  "corresponding to the currently selected notebook")))
        return;
    }

    if (Q_UNLIKELY(pNotebookModelItem->type() != NotebookModelItem::Type::Notebook)) {
        Q_UNUSED(informationMessageBox(this, tr("No notebook is selected"),
                                       tr("Please select the notebook in which you want to create the note (currently "
                                          "the notebook stack seems to be selected)")))
        return;
    }

    const NotebookItem * pNotebookItem = pNotebookModelItem->notebookItem();
    if (Q_UNLIKELY(!pNotebookItem)) {
        Q_UNUSED(internalErrorMessageBox(this, tr("Can't create a new note: the notebook model item has notebook type "
                                                  "but null pointer to the actual notebook item")))
        return;
    }

    m_pNoteEditorTabsAndWindowsCoordinator->createNewNote(pNotebookItem->localUid(),
                                                          pNotebookItem->guid(),
                                                          noteEditorMode);
}

void MainWindow::connectSynchronizationManager()
{
    QNDEBUG(QStringLiteral("MainWindow::connectSynchronizationManager"));

    if (Q_UNLIKELY(!m_pSynchronizationManager)) {
        QNDEBUG(QStringLiteral("No synchronization manager"));
        return;
    }

    // Connect local signals to SynchronizationManager slots
    QObject::connect(this, QNSIGNAL(MainWindow,authenticate),
                     m_pSynchronizationManager, QNSLOT(SynchronizationManager,authenticate));
    QObject::connect(this, QNSIGNAL(MainWindow,synchronize),
                     m_pSynchronizationManager, QNSLOT(SynchronizationManager,synchronize));
    QObject::connect(this, QNSIGNAL(MainWindow,stopSynchronization),
                     m_pSynchronizationManager, QNSLOT(SynchronizationManager,stop));
    QObject::connect(this, QNSIGNAL(MainWindow,synchronizationSetAccount,Account),
                     m_pSynchronizationManager, QNSLOT(SynchronizationManager,setAccount,Account));
    QObject::connect(this, QNSIGNAL(MainWindow,synchronizationDownloadNoteThumbnailsOptionChanged,bool),
                     m_pSynchronizationManager, QNSLOT(SynchronizationManager,setDownloadNoteThumbnails,bool));
    QObject::connect(this, QNSIGNAL(MainWindow,synchronizationDownloadInkNoteImagesOptionChanged,bool),
                     m_pSynchronizationManager, QNSLOT(SynchronizationManager,setDownloadInkNoteImages,bool));
    QObject::connect(this, QNSIGNAL(MainWindow,synchronizationSetInkNoteImagesStoragePath,QString),
                     m_pSynchronizationManager, QNSLOT(SynchronizationManager,setInkNoteImagesStoragePath,QString));

    // Connect SynchronizationManager signals to local slots
    QObject::connect(m_pSynchronizationManager, QNSIGNAL(SynchronizationManager,started),
                     this, QNSLOT(MainWindow,onSynchronizationStarted));
    QObject::connect(m_pSynchronizationManager, QNSIGNAL(SynchronizationManager,stopped),
                     this, QNSLOT(MainWindow,onSynchronizationStopped));
    QObject::connect(m_pSynchronizationManager, QNSIGNAL(SynchronizationManager,failed,ErrorString),
                     this, QNSLOT(MainWindow,onSynchronizationManagerFailure,ErrorString));
    QObject::connect(m_pSynchronizationManager, QNSIGNAL(SynchronizationManager,finished,Account),
                     this, QNSLOT(MainWindow,onSynchronizationFinished,Account));
    QObject::connect(m_pSynchronizationManager, QNSIGNAL(SynchronizationManager,authenticationFinished,bool,ErrorString,Account),
                     this, QNSLOT(MainWindow,onAuthenticationFinished,bool,ErrorString,Account));
    QObject::connect(m_pSynchronizationManager, QNSIGNAL(SynchronizationManager,authenticationRevoked,bool,ErrorString,qevercloud::UserID),
                     this, QNSLOT(MainWindow,onAuthenticationRevoked,bool,ErrorString,qevercloud::UserID));
    QObject::connect(m_pSynchronizationManager, QNSIGNAL(SynchronizationManager,remoteToLocalSyncStopped),
                     this, QNSLOT(MainWindow,onRemoteToLocalSyncStopped));
    QObject::connect(m_pSynchronizationManager, QNSIGNAL(SynchronizationManager,sendLocalChangesStopped),
                     this, QNSLOT(MainWindow,onSendLocalChangesStopped));
    QObject::connect(m_pSynchronizationManager, QNSIGNAL(SynchronizationManager,rateLimitExceeded,qint32),
                     this, QNSLOT(MainWindow,onRateLimitExceeded,qint32));
    QObject::connect(m_pSynchronizationManager, QNSIGNAL(SynchronizationManager,remoteToLocalSyncDone),
                     this, QNSLOT(MainWindow,onRemoteToLocalSyncDone));
    QObject::connect(m_pSynchronizationManager, QNSIGNAL(SynchronizationManager,syncChunksDownloadProgress,qint32,qint32,qint32),
                     this, QNSLOT(MainWindow,onSyncChunksDownloadProgress,qint32,qint32,qint32));
    QObject::connect(m_pSynchronizationManager, QNSIGNAL(SynchronizationManager,syncChunksDownloaded),
                     this, QNSLOT(MainWindow,onSyncChunksDownloaded));
    QObject::connect(m_pSynchronizationManager, QNSIGNAL(SynchronizationManager,notesDownloadProgress,quint32,quint32),
                     this, QNSLOT(MainWindow,onNotesDownloadProgress,quint32,quint32));
    QObject::connect(m_pSynchronizationManager, QNSIGNAL(SynchronizationManager,resourcesDownloadProgress,quint32,quint32),
                     this, QNSLOT(MainWindow,onResourcesDownloadProgress,quint32,quint32));
    QObject::connect(m_pSynchronizationManager,
                     QNSIGNAL(SynchronizationManager,linkedNotebookSyncChunksDownloadProgress,qint32,qint32,qint32,LinkedNotebook),
                     this,
                     QNSLOT(MainWindow,onLinkedNotebookSyncChunksDownloadProgress,qint32,qint32,qint32,LinkedNotebook));
    QObject::connect(m_pSynchronizationManager, QNSIGNAL(SynchronizationManager,linkedNotebooksSyncChunksDownloaded),
                     this, QNSLOT(MainWindow,onLinkedNotebooksSyncChunksDownloaded));
    QObject::connect(m_pSynchronizationManager, QNSIGNAL(SynchronizationManager,linkedNotebooksNotesDownloadProgress,quint32,quint32),
                     this, QNSLOT(MainWindow,onLinkedNotebooksNotesDownloadProgress,quint32,quint32));
}

void MainWindow::disconnectSynchronizationManager()
{
    QNDEBUG(QStringLiteral("MainWindow::disconnectSynchronizationManager"));

    if (Q_UNLIKELY(!m_pSynchronizationManager)) {
        QNDEBUG(QStringLiteral("No synchronization manager"));
        return;
    }

    // Disconnect local signals from SynchronizationManager slots
    QObject::disconnect(this, QNSIGNAL(MainWindow,authenticate),
                        m_pSynchronizationManager, QNSLOT(SynchronizationManager,authenticate));
    QObject::disconnect(this, QNSIGNAL(MainWindow,synchronize),
                        m_pSynchronizationManager, QNSLOT(SynchronizationManager,synchronize));
    QObject::disconnect(this, QNSIGNAL(MainWindow,stopSynchronization),
                        m_pSynchronizationManager, QNSLOT(SynchronizationManager,stop));
    QObject::disconnect(this, QNSIGNAL(MainWindow,synchronizationSetAccount,Account),
                        m_pSynchronizationManager, QNSLOT(SynchronizationManager,setAccount,Account));
    QObject::disconnect(this, QNSIGNAL(MainWindow,synchronizationDownloadNoteThumbnailsOptionChanged,bool),
                        m_pSynchronizationManager, QNSLOT(SynchronizationManager,setDownloadNoteThumbnails,bool));
    QObject::disconnect(this, QNSIGNAL(MainWindow,synchronizationDownloadInkNoteImagesOptionChanged,bool),
                        m_pSynchronizationManager, QNSLOT(SynchronizationManager,setDownloadInkNoteImages,bool));
    QObject::disconnect(this, QNSIGNAL(MainWindow,synchronizationSetInkNoteImagesStoragePath,QString),
                        m_pSynchronizationManager, QNSLOT(SynchronizationManager,setInkNoteImagesStoragePath,QString));

    // Disconnect SynchronizationManager signals from local slots
    QObject::disconnect(m_pSynchronizationManager, QNSIGNAL(SynchronizationManager,started),
                        this, QNSLOT(MainWindow,onSynchronizationStarted));
    QObject::disconnect(m_pSynchronizationManager, QNSIGNAL(SynchronizationManager,stopped),
                        this, QNSLOT(MainWindow,onSynchronizationStopped));
    QObject::disconnect(m_pSynchronizationManager, QNSIGNAL(SynchronizationManager,failed,ErrorString),
                        this, QNSLOT(MainWindow,onSynchronizationManagerFailure,ErrorString));
    QObject::disconnect(m_pSynchronizationManager, QNSIGNAL(SynchronizationManager,finished,Account),
                        this, QNSLOT(MainWindow,onSynchronizationFinished,Account));
    QObject::disconnect(m_pSynchronizationManager, QNSIGNAL(SynchronizationManager,authenticationFinished,bool,ErrorString,Account),
                        this, QNSLOT(MainWindow,onAuthenticationFinished,bool,ErrorString,Account));
    QObject::disconnect(m_pSynchronizationManager, QNSIGNAL(SynchronizationManager,authenticationRevoked,bool,ErrorString,qevercloud::UserID),
                        this, QNSLOT(MainWindow,onAuthenticationRevoked,bool,ErrorString,qevercloud::UserID));
    QObject::disconnect(m_pSynchronizationManager, QNSIGNAL(SynchronizationManager,remoteToLocalSyncStopped),
                        this, QNSLOT(MainWindow,onRemoteToLocalSyncStopped));
    QObject::disconnect(m_pSynchronizationManager, QNSIGNAL(SynchronizationManager,sendLocalChangesStopped),
                        this, QNSLOT(MainWindow,onSendLocalChangesStopped));
    QObject::disconnect(m_pSynchronizationManager, QNSIGNAL(SynchronizationManager,rateLimitExceeded,qint32),
                        this, QNSLOT(MainWindow,onRateLimitExceeded,qint32));
    QObject::disconnect(m_pSynchronizationManager, QNSIGNAL(SynchronizationManager,remoteToLocalSyncDone),
                        this, QNSLOT(MainWindow,onRemoteToLocalSyncDone));
    QObject::disconnect(m_pSynchronizationManager, QNSIGNAL(SynchronizationManager,syncChunksDownloadProgress,qint32,qint32,qint32),
                        this, QNSLOT(MainWindow,onSyncChunksDownloadProgress,qint32,qint32,qint32));
    QObject::disconnect(m_pSynchronizationManager, QNSIGNAL(SynchronizationManager,syncChunksDownloaded),
                        this, QNSLOT(MainWindow,onSyncChunksDownloaded));
    QObject::disconnect(m_pSynchronizationManager, QNSIGNAL(SynchronizationManager,notesDownloadProgress,quint32,quint32),
                        this, QNSLOT(MainWindow,onNotesDownloadProgress,quint32,quint32));
    QObject::disconnect(m_pSynchronizationManager, QNSIGNAL(SynchronizationManager,resourcesDownloadProgress,quint32,quint32),
                        this, QNSLOT(MainWindow,onResourcesDownloadProgress,quint32,quint32));
    QObject::disconnect(m_pSynchronizationManager,
                        QNSIGNAL(SynchronizationManager,linkedNotebookSyncChunksDownloadProgress,qint32,qint32,qint32,LinkedNotebook),
                        this,
                        QNSLOT(MainWindow,onLinkedNotebookSyncChunksDownloadProgress,qint32,qint32,qint32,LinkedNotebook));
    QObject::disconnect(m_pSynchronizationManager, QNSIGNAL(SynchronizationManager,linkedNotebooksSyncChunksDownloaded),
                        this, QNSLOT(MainWindow,onLinkedNotebooksSyncChunksDownloaded));
    QObject::disconnect(m_pSynchronizationManager, QNSIGNAL(SynchronizationManager,linkedNotebooksNotesDownloadProgress,quint32,quint32),
                        this, QNSLOT(MainWindow,onLinkedNotebooksNotesDownloadProgress,quint32,quint32));
}

void MainWindow::startSyncButtonAnimation()
{
    QNDEBUG(QStringLiteral("MainWindow::startSyncButtonAnimation"));

    QObject::disconnect(&m_animatedSyncButtonIcon, QNSIGNAL(QMovie,frameChanged,int),
                        this, QNSLOT(MainWindow,onAnimatedSyncIconFrameChangedPendingFinish,int));

    QObject::connect(&m_animatedSyncButtonIcon, QNSIGNAL(QMovie,frameChanged,int),
                     this, QNSLOT(MainWindow,onAnimatedSyncIconFrameChanged,int));

    // If the animation doesn't run forever, make it so
    if (m_animatedSyncButtonIcon.loopCount() != -1) {
        QObject::connect(&m_animatedSyncButtonIcon, QNSIGNAL(QMovie,finished),
                         &m_animatedSyncButtonIcon, QNSLOT(QMovie,start));
    }

    m_animatedSyncButtonIcon.start();
}

void MainWindow::stopSyncButtonAnimation()
{
    QNDEBUG(QStringLiteral("MainWindow::stopSyncButtonAnimation"));

    if (m_animatedSyncButtonIcon.loopCount() != -1) {
        QObject::disconnect(&m_animatedSyncButtonIcon, QNSIGNAL(QMovie,finished),
                            &m_animatedSyncButtonIcon, QNSLOT(QMovie,start));
    }

    QObject::disconnect(&m_animatedSyncButtonIcon, QNSIGNAL(QMovie,finished),
                        this, QNSLOT(MainWindow,onSyncIconAnimationFinished));
    QObject::disconnect(&m_animatedSyncButtonIcon, QNSIGNAL(QMovie,frameChanged,int),
                        this, QNSLOT(MainWindow,onAnimatedSyncIconFrameChanged,int));

    m_animatedSyncButtonIcon.stop();
    m_pUI->syncPushButton->setIcon(QIcon(QStringLiteral(":/sync/sync.png")));
}

void MainWindow::scheduleSyncButtonAnimationStop()
{
    QNDEBUG(QStringLiteral("MainWindow::scheduleSyncButtonAnimationStop"));

    if (m_animatedSyncButtonIcon.state() != QMovie::Running) {
        stopSyncButtonAnimation();
        return;
    }

    // NOTE: either finished signal should stop the sync animation or, if the movie
    // is naturally looped, the frame count coming to max value (or zero after max value)
    // would serve as a sign that full animation loop has finished
    QObject::connect(&m_animatedSyncButtonIcon, QNSIGNAL(QMovie,finished),
                     this, QNSLOT(MainWindow,onSyncIconAnimationFinished));
    QObject::connect(&m_animatedSyncButtonIcon, QNSIGNAL(QMovie,frameChanged,int),
                     this, QNSLOT(MainWindow,onAnimatedSyncIconFrameChangedPendingFinish,int));
}

bool MainWindow::checkNoteSearchQuery(const QString & noteSearchQuery)
{
    QNDEBUG(QStringLiteral("MainWindow::checkNoteSearchQuery: ") << noteSearchQuery);

    if (m_noteSearchQueryValidated) {
        QNTRACE(QStringLiteral("Note search query validated"));
        return true;
    }

    NoteSearchQuery query;
    ErrorString errorDescription;
    bool res = query.setQueryString(noteSearchQuery, errorDescription);
    if (!res) {
        QToolTip::showText(m_pUI->searchQueryLineEdit->mapToGlobal(QPoint(0, m_pUI->searchQueryLineEdit->height())),
                           errorDescription.localizedString(), m_pUI->searchQueryLineEdit);
        m_noteSearchQueryValidated = false;
        return false;
    }

    m_noteSearchQueryValidated = true;
    return true;
}

void MainWindow::startListeningForSplitterMoves()
{
    QNDEBUG(QStringLiteral("MainWindow::startListeningForSplitterMoves"));

    QObject::connect(m_pUI->splitter, QNSIGNAL(QSplitter,splitterMoved,int,int),
                     this, QNSLOT(MainWindow,onSplitterHandleMoved,int,int),
                     Qt::UniqueConnection);
    QObject::connect(m_pUI->sidePanelSplitter, QNSIGNAL(QSplitter,splitterMoved,int,int),
                     this, QNSLOT(MainWindow,onSidePanelSplittedHandleMoved,int,int),
                     Qt::UniqueConnection);
}

void MainWindow::stopListeningForSplitterMoves()
{
    QNDEBUG(QStringLiteral("MainWindow::stopListeningForSplitterMoves"));

    QObject::disconnect(m_pUI->splitter, QNSIGNAL(QSplitter,splitterMoved,int,int),
                        this, QNSLOT(MainWindow,onSplitterHandleMoved,int,int));
    QObject::disconnect(m_pUI->sidePanelSplitter, QNSIGNAL(QSplitter,splitterMoved,int,int),
                        this, QNSLOT(MainWindow,onSidePanelSplittedHandleMoved,int,int));
}

void MainWindow::persistChosenIconTheme(const QString & iconThemeName)
{
    QNDEBUG(QStringLiteral("MainWindow::persistChosenIconTheme: ") << iconThemeName);

    ApplicationSettings appSettings(*m_pAccount, QUENTIER_UI_SETTINGS);
    appSettings.beginGroup(LOOK_AND_FEEL_SETTINGS_GROUP_NAME);
    appSettings.setValue(ICON_THEME_SETTINGS_KEY, iconThemeName);
    appSettings.endGroup();
}

void MainWindow::refreshChildWidgetsThemeIcons()
{
    QNDEBUG(QStringLiteral("MainWindow::refreshChildWidgetsThemeIcons"));

    refreshThemeIcons<QAction>();
    refreshThemeIcons<QPushButton>();
    refreshThemeIcons<QCheckBox>();
    refreshThemeIcons<ColorPickerToolButton>();
    refreshThemeIcons<InsertTableToolButton>();

    refreshNoteEditorWidgetsSpecialIcons();
}

void MainWindow::refreshNoteEditorWidgetsSpecialIcons()
{
    QNDEBUG(QStringLiteral("MainWindow::refreshNoteEditorWidgetsSpecialIcons"));

    if (m_pNoteEditorTabsAndWindowsCoordinator) {
        m_pNoteEditorTabsAndWindowsCoordinator->refreshNoteEditorWidgetsSpecialIcons();
    }
}

void MainWindow::showHideViewColumnsForAccountType(const Account::Type::type accountType)
{
    QNDEBUG(QStringLiteral("MainWindow::showHideViewColumnsForAccountType: ") << accountType);

    bool isLocal = (accountType == Account::Type::Local);

    NotebookItemView * notebooksTreeView = m_pUI->notebooksTreeView;
    notebooksTreeView->setColumnHidden(NotebookModel::Columns::Published, isLocal);
    notebooksTreeView->setColumnHidden(NotebookModel::Columns::Dirty, isLocal);

    TagItemView * tagsTreeView = m_pUI->tagsTreeView;
    tagsTreeView->setColumnHidden(TagModel::Columns::Dirty, isLocal);

    SavedSearchItemView * savedSearchesTableView = m_pUI->savedSearchesTableView;
    savedSearchesTableView->setColumnHidden(SavedSearchModel::Columns::Dirty, isLocal);

    DeletedNoteItemView * deletedNotesTableView = m_pUI->deletedNotesTableView;
    deletedNotesTableView->setColumnHidden(NoteModel::Columns::Dirty, isLocal);
}

void MainWindow::expandFiltersView()
{
    QNDEBUG(QStringLiteral("MainWindow::expandFiltersView"));

    m_pUI->filtersViewTogglePushButton->setIcon(QIcon::fromTheme(QStringLiteral("go-down")));
    m_pUI->filterBodyFrame->show();
    m_pUI->filterFrameBottomBoundary->hide();
    m_pUI->filterFrame->adjustSize();
}

void MainWindow::foldFiltersView()
{
    QNDEBUG(QStringLiteral("MainWindow::foldFiltersView"));

    m_pUI->filtersViewTogglePushButton->setIcon(QIcon::fromTheme(QStringLiteral("go-next")));
    m_pUI->filterBodyFrame->hide();
    m_pUI->filterFrameBottomBoundary->show();

    adjustNoteListAndFiltersSplitterSizes();
}

void MainWindow::adjustNoteListAndFiltersSplitterSizes()
{
    QNDEBUG(QStringLiteral("MainWindow::adjustNoteListAndFiltersSplitterSizes"));

    QList<int> splitterSizes = m_pUI->noteListAndFiltersSplitter->sizes();
    int count = splitterSizes.count();
    if (Q_UNLIKELY(count != 2)) {
        ErrorString error(QT_TR_NOOP("Internal error: can't properly resize "
                                     "the splitter after folding the filter view: "
                                     "wrong number of sizes within the splitter"));
        QNWARNING(error << QStringLiteral("Sizes count: ") << count);
        onSetStatusBarText(error.localizedString(), SEC_TO_MSEC(30));
        return;
    }

    int filterHeaderPanelHeight = m_pUI->noteFilterHeaderPanel->height();
    int heightDiff = std::max(splitterSizes[0] - filterHeaderPanelHeight, 0);
    splitterSizes[0] = filterHeaderPanelHeight;
    splitterSizes[1] = splitterSizes[1] + heightDiff;
    m_pUI->noteListAndFiltersSplitter->setSizes(splitterSizes);

    // Need to schedule the repaint because otherwise the actions above
    // seem to have no effect
    m_pUI->noteListAndFiltersSplitter->update();
}

void MainWindow::clearDir(const QString & path)
{
    QDir newDir(path);

    QStringList files = newDir.entryList(QDir::NoDotAndDotDot | QDir::Files);
    for(auto it = files.constBegin(), end = files.constEnd(); it != end; ++it) {
        Q_UNUSED(newDir.remove(*it))
    }

    QStringList dirs = newDir.entryList(QDir::NoDotAndDotDot | QDir::Dirs);
    for(auto it = dirs.constBegin(), end = dirs.constEnd(); it != end; ++it) {
        clearDir(*it);
        Q_UNUSED(newDir.remove(*it))
    }
}

void MainWindow::collectBaseStyleSheets()
{
    QNDEBUG(QStringLiteral("MainWindow::collectBaseStyleSheets"));

    QList<QWidget*> childWidgets = findChildren<QWidget*>();
    for(auto it = childWidgets.constBegin(), end = childWidgets.constEnd(); it != end; ++it)
    {
        QWidget * widget = *it;
        if (Q_UNLIKELY(!widget)) {
            continue;
        }

        QString styleSheet = widget->styleSheet();
        if (styleSheet.isEmpty()) {
            continue;
        }

        StyleSheetInfo & info = m_styleSheetInfo[widget];
        info.m_baseStyleSheet = styleSheet;
        info.m_targetWidget = QPointer<QWidget>(widget);
    }
}

void MainWindow::setupPanelOverlayStyleSheets()
{
    QNDEBUG(QStringLiteral("MainWindow::setupPanelOverlayStyleSheets"));

    ApplicationSettings appSettings(*m_pAccount, QUENTIER_UI_SETTINGS);
    appSettings.beginGroup(LOOK_AND_FEEL_SETTINGS_GROUP_NAME);
    QString panelStyle = appSettings.value(PANELS_STYLE_SETTINGS_KEY).toString();
    appSettings.endGroup();

    if (panelStyle.isEmpty()) {
        QNDEBUG(QStringLiteral("No last chosen panel style"));
        return;
    }

    StyleSheetProperties properties;
    getPanelStyleSheetProperties(panelStyle, properties);
    if (Q_UNLIKELY(properties.isEmpty())) {
        return;
    }

    m_currentPanelStyle = panelStyle;

    setPanelsOverlayStyleSheet(properties);
}

void MainWindow::getPanelStyleSheetProperties(const QString & panelStyleOption,
                                              StyleSheetProperties & properties) const
{
    properties.clear();
    properties.reserve(4);

    QString backgroundColorName, colorName, backgroundColorButtonHoverName,
            backgroundColorButtonPressedName;

    if (panelStyleOption == LIGHTER_PANEL_STYLE_NAME)
    {
        QColor backgroundColor = palette().color(QPalette::Light);
        backgroundColorName = backgroundColor.name();

        QColor color = palette().color(QPalette::WindowText).name();
        colorName = color.name();

        QColor backgroundColorButtonHover = backgroundColor.darker(200);
        backgroundColorButtonHoverName = backgroundColorButtonHover.name();

        QColor backgroundColorButtonPressed = backgroundColor.lighter(150);
        backgroundColorButtonPressedName = backgroundColorButtonPressed.name();
    }
    else if (panelStyleOption == DARKER_PANEL_STYLE_NAME)
    {
        QColor backgroundColor = palette().color(QPalette::Dark);
        backgroundColorName = backgroundColor.name();

        QColor color = palette().color(QPalette::BrightText);
        colorName = color.name();

        QColor backgroundColorButtonHover = backgroundColor.lighter(150);
        backgroundColorButtonHoverName = backgroundColorButtonHover.name();

        QColor backgroundColorButtonPressed = backgroundColor.darker(200);
        backgroundColorButtonPressedName = backgroundColorButtonPressed.name();
    }
    else
    {
        QNDEBUG(QStringLiteral("Unidentified panel style name: ") << panelStyleOption);
        return;
    }

    properties.push_back(StyleSheetProperty(StyleSheetProperty::Target::None,
                                            "background-color", backgroundColorName));
    properties.push_back(StyleSheetProperty(StyleSheetProperty::Target::None,
                                            "color", colorName));
    properties.push_back(StyleSheetProperty(StyleSheetProperty::Target::ButtonHover,
                                            "background-color", backgroundColorButtonHoverName));
    properties.push_back(StyleSheetProperty(StyleSheetProperty::Target::ButtonPressed,
                                            "background-color", backgroundColorButtonPressedName));

    if (QuentierIsLogLevelActive(LogLevel::TraceLevel))
    {
        QNTRACE("Computed stylesheet properties: ");
        for(auto it = properties.constBegin(), end = properties.constEnd(); it != end; ++it)
        {
            const StyleSheetProperty & property = *it;
            QNTRACE("Property: target = " << property.m_targetType << ", property name = " << property.m_name
                    << ", property value = " << property.m_value);
        }
    }

    return;
}

void MainWindow::setPanelsOverlayStyleSheet(const StyleSheetProperties & properties)
{
    for(auto it = m_styleSheetInfo.begin(); it != m_styleSheetInfo.end(); )
    {
        StyleSheetInfo & info = it.value();

        const QPointer<QWidget> & widget = info.m_targetWidget;
        if (Q_UNLIKELY(widget.isNull())) {
            it = m_styleSheetInfo.erase(it);
            continue;
        }

        ++it;

        QString widgetName = widget->objectName();
        if (!widgetName.contains(QStringLiteral("Panel"))) {
            continue;
        }

        if (properties.isEmpty()) {
            widget->setStyleSheet(info.m_baseStyleSheet);
            continue;
        }

        QString overlayStyleSheet = alterStyleSheet(info.m_baseStyleSheet, properties);
        if (Q_UNLIKELY(overlayStyleSheet.isEmpty())) {
            widget->setStyleSheet(info.m_baseStyleSheet);
            continue;
        }

        widget->setStyleSheet(overlayStyleSheet);
    }
}

QString MainWindow::alterStyleSheet(const QString & originalStyleSheet,
                                    const StyleSheetProperties & properties)
{
    QNDEBUG(QStringLiteral("MainWindow::alterStyleSheet: original stylesheet = ")
            << originalStyleSheet);

    QString result = originalStyleSheet;

#define REPORT_ERROR() \
    ErrorString errorDescription(QT_TR_NOOP("Can't alter the stylesheet: stylesheet parsing failed")); \
    QNINFO(errorDescription << QStringLiteral(", original stylesheet: ") \
           << originalStyleSheet << QStringLiteral(", stylesheet modified so far: ") \
           << result << QStringLiteral(", property index = ") \
           << propertyIndex); \
    onSetStatusBarText(errorDescription.localizedString(), SEC_TO_MSEC(30))

    for(auto it = properties.constBegin(), end = properties.constEnd(); it != end; ++it)
    {
        const StyleSheetProperty & property = *it;

        QString propertyName = QString::fromUtf8(property.m_name);

        int propertyIndex = -1;
        while(true)
        {
            if (propertyIndex >= result.size()) {
                break;
            }

            propertyIndex = result.indexOf(propertyName, propertyIndex + 1);
            if (propertyIndex < 0) {
                break;
            }

            if (propertyIndex > 0)
            {
                // Check that what we've found is the start of a word
                QChar previousChar = result[propertyIndex - 1];
                QString previousCharString(previousChar);
                if (!previousCharString.trimmed().isEmpty()) {
                    continue;
                }
            }

            bool error = false;
            bool insideHover = isInsideStyleBlock(result, QStringLiteral(":hover:!pressed"), propertyIndex, error);
            if (Q_UNLIKELY(error)) {
                REPORT_ERROR();
                return QString();
            }

            error = false;
            bool insidePressed = isInsideStyleBlock(result, QStringLiteral(":pressed"), propertyIndex, error);
            if (Q_UNLIKELY(error)) {
                REPORT_ERROR();
                return QString();
            }

            error = false;
            bool insidePseudoButtonHover = isInsideStyleBlock(result, QStringLiteral("pseudoPushButton:hover:!pressed"),
                                                              propertyIndex, error);
            if (Q_UNLIKELY(error)) {
                REPORT_ERROR();
                return QString();
            }

            error = false;
            bool insidePseudoButtonPressed = isInsideStyleBlock(result, QStringLiteral("pseudoPushButton:hover:pressed"),
                                                                propertyIndex, error);
            if (Q_UNLIKELY(error)) {
                REPORT_ERROR();
                return QString();
            }

            int propertyEndIndex = result.indexOf(QStringLiteral(";"), propertyIndex + 1);
            if (Q_UNLIKELY(propertyEndIndex < 0)) {
                REPORT_ERROR();
                return QString();
            }

            QString replacement = propertyName + QStringLiteral(": ") + property.m_value;

#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
#define REPLACE() \
            QNDEBUG(QStringLiteral("Replacing substring ") \
                    << result.midRef(propertyIndex, propertyEndIndex - propertyIndex) \
                    << QStringLiteral(" with ") << replacement); \
            result.replace(propertyIndex, propertyEndIndex - propertyIndex, replacement); \
            QNDEBUG(QStringLiteral("Stylesheet after the replacement: ") << result)
#else
#define REPLACE() \
            QNDEBUG(QStringLiteral("Replacing substring ") \
                    << result.mid(propertyIndex, propertyEndIndex - propertyIndex) \
                    << QStringLiteral(" with ") << replacement); \
            result.replace(propertyIndex, propertyEndIndex - propertyIndex, replacement); \
            QNDEBUG(QStringLiteral("Stylesheet after the replacement: ") << result)
#endif

            if (property.m_targetType == StyleSheetProperty::Target::None)
            {
                if (insideHover && !insidePseudoButtonHover) {
                    continue;
                }

                if (insidePressed && !insidePseudoButtonPressed) {
                    continue;
                }

                REPLACE();
            }
            else if (property.m_targetType == StyleSheetProperty::Target::ButtonHover)
            {
                if (insidePressed) {
                    continue;
                }

                if (!insideHover || insidePseudoButtonHover) {
                    continue;
                }

                REPLACE();
            }
            else if (property.m_targetType == StyleSheetProperty::Target::ButtonPressed)
            {
                if (!insidePressed || insidePseudoButtonPressed) {
                    continue;
                }

                REPLACE();
            }
        }
    }

    QNDEBUG(QStringLiteral("Altered stylesheet: ") << result);
    return result;
}

bool MainWindow::isInsideStyleBlock(const QString & styleSheet, const QString & styleBlockStartSearchString,
                                    const int currentIndex, bool & error) const
{
    error = false;

    int blockStartIndex = styleSheet.lastIndexOf(styleBlockStartSearchString, currentIndex);
    if (blockStartIndex < 0) {
        // No block start before the current index => not inside the style block of this kind
        return false;
    }

    int blockEndIndex = styleSheet.lastIndexOf(QStringLiteral("}"), currentIndex);
    if (blockEndIndex > blockStartIndex) {
        // Block end was found after the last block start before the current index => the current index is not within the style block
        return false;
    }

    // There's no block end between block start and the current index, need to look for the block end after the current index
    blockEndIndex = styleSheet.indexOf(QStringLiteral("}"), currentIndex + 1);
    if (Q_UNLIKELY(blockEndIndex < 0)) {
        QNDEBUG(QStringLiteral("Can't parse stylesheet: can't find block end for style block search string ")
                << styleBlockStartSearchString);
        error = true;
        return false;
    }

    // Found first style block end after the current index while the block start is before the current index =>
    // the current index is inside the style block
    return true;
}

#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
void MainWindow::fixupQt4StyleSheets()
{
    QNDEBUG(QStringLiteral("MainWindow::fixupQt4StyleSheets"));

    QString alternateCentralWidgetStylesheet = QString::fromUtf8(
                "QSplitter::handle {"
                "   background-color: black;"
                "}"
                "QSplitter::handle:horizontal {"
                "   width: 1px;"
                "}"
                "QSplitter::handle:vertical {"
                "   height: 1px;"
                "}"
                "#noteEditorVerticalSplitter::handle {"
                "   background: none;"
                "}");
    m_pUI->centralWidget->setStyleSheet(alternateCentralWidgetStylesheet);

    QString alternateNoteListFrameStylesheet = QString::fromUtf8(
                "#notesListAndFiltersFrame {"
                "   border: none;"
                "}");
    m_pUI->notesListAndFiltersFrame->setStyleSheet(alternateNoteListFrameStylesheet);

    QString alternateFilterBodyFrameStylesheet = QString::fromUtf8(
                "#filterBodyFrame {"
                "padding: 0px;"
                "margin: 0px;"
                "border: none;"
                "border-top: 1px solid black;"
                "}");
    m_pUI->filterBodyFrame->setStyleSheet(alternateFilterBodyFrameStylesheet);

    QString alternateViewStylesheetBase = QString::fromUtf8(
                "{"
                "   border: none;"
                "   margin-top: 1px;"
                "   background-color: transparent;"
                "}");

    QString alternateFavoritesTableViewStylesheet = QString::fromUtf8("#favoritesTableView ") +
                                                    alternateViewStylesheetBase;
    m_pUI->favoritesTableView->setStyleSheet(alternateFavoritesTableViewStylesheet);

    QString alternateNotebooksTreeViewStylesheet = QString::fromUtf8("#notebooksTreeView ") +
                                                    alternateViewStylesheetBase;
    m_pUI->notebooksTreeView->setStyleSheet(alternateNotebooksTreeViewStylesheet);

    QString alternateTagsTreeViewStylesheet = QString::fromUtf8("#tagsTreeView ") +
                                                   alternateViewStylesheetBase;
    m_pUI->tagsTreeView->setStyleSheet(alternateTagsTreeViewStylesheet);

    QString alternateSavedSearchTableViewStylesheet = QString::fromUtf8("#savedSearchesTableView ") +
                                                      alternateViewStylesheetBase;
    m_pUI->savedSearchesTableView->setStyleSheet(alternateSavedSearchTableViewStylesheet);

    QString alternateDeletedNotesTableViewStylesheet = QString::fromUtf8("#deletedNotesTableView ") +
                                                       alternateViewStylesheetBase;
    m_pUI->deletedNotesTableView->setStyleSheet(alternateDeletedNotesTableViewStylesheet);

    QString alternateSidePanelSplitterStylesheet = QString::fromUtf8(
                "QFrame {"
                "   border: none;"
                "}"
                "#sidePanelSplitter {"
                "   border-bottom: 1px solid black;"
                "}");
    m_pUI->sidePanelSplitter->setStyleSheet(alternateSidePanelSplitterStylesheet);

    QString alternateNoteListWidgetHeaderPanelStylesheet = m_pUI->noteListWidgetHeaderPanel->styleSheet();
    int index = alternateNoteListWidgetHeaderPanelStylesheet.indexOf(QStringLiteral("QLabel"));
    if (Q_UNLIKELY(index < 0)) {
        QNDEBUG(QStringLiteral("Can't fixup the stylesheet of note list widget header panel: "
                               "no QLabel within the stylesheet"));
        return;
    }

    index = alternateNoteListWidgetHeaderPanelStylesheet.indexOf(QStringLiteral("border-right"), index);
    if (Q_UNLIKELY(index < 0)) {
        QNDEBUG(QStringLiteral("Can't fixup the stylesheet of note list widget header panel: "
                               "no border-right property for QLabel within the stylesheet"));
        return;
    }

    int propertyEndIndex = alternateNoteListWidgetHeaderPanelStylesheet.indexOf(QStringLiteral(";"), index);
    if (Q_UNLIKELY(propertyEndIndex < 0)) {
        QNDEBUG(QStringLiteral("Can't fixup the stylesheet of note list widget header panel: "
                               "no closing \";\" for border-right property for QLabel "
                               "within the stylesheet"));
        return;
    }

    alternateNoteListWidgetHeaderPanelStylesheet.remove(index, propertyEndIndex - index + 1);
    QNDEBUG(QStringLiteral("alternateNoteListWidgetHeaderPanelStylesheet: ")
            << alternateNoteListWidgetHeaderPanelStylesheet);
    m_pUI->noteListWidgetHeaderPanel->setStyleSheet(alternateNoteListWidgetHeaderPanelStylesheet);

    // Add bottom border to the header panel widgets
    QString alternativeFavoritesHeaderPanelStylesheet = m_pUI->favoritesHeaderPanel->styleSheet();
    index = alternativeFavoritesHeaderPanelStylesheet.indexOf(QStringLiteral("}"));
    if (Q_UNLIKELY(index < 0)) {
        QNDEBUG(QStringLiteral("Can't fixup the stylesheet of favorites header panel: no first closing curly brace "
                               "found within the stylesheet"));
        return;
    }
    alternativeFavoritesHeaderPanelStylesheet.insert(index, QStringLiteral("border-bottom: 1px solid black;"));
    m_pUI->favoritesHeaderPanel->setStyleSheet(alternativeFavoritesHeaderPanelStylesheet);
    m_pUI->favoritesHeaderPanel->setMinimumHeight(23);
    m_pUI->favoritesHeaderPanel->setMaximumHeight(23);

    QString alternativeNotebooksHeaderPanelStylesheet = m_pUI->notebooksHeaderPanel->styleSheet();
    index = alternativeNotebooksHeaderPanelStylesheet.indexOf(QStringLiteral("}"));
    if (Q_UNLIKELY(index < 0)) {
        QNDEBUG(QStringLiteral("Can't fixup the stylesheet of notebooks header panel: no first closing curly brace "
                               "found within the stylesheet"));
        return;
    }
    alternativeNotebooksHeaderPanelStylesheet.insert(index, QStringLiteral("border-bottom: 1px solid black;"));
    m_pUI->notebooksHeaderPanel->setStyleSheet(alternativeNotebooksHeaderPanelStylesheet);
    m_pUI->notebooksHeaderPanel->setMinimumHeight(23);
    m_pUI->notebooksHeaderPanel->setMaximumHeight(23);

    QString alternativeTagsHeaderPanelStylesheet = m_pUI->tagsHeaderPanel->styleSheet();
    index = alternativeTagsHeaderPanelStylesheet.indexOf(QStringLiteral("}"));
    if (Q_UNLIKELY(index < 0)) {
        QNDEBUG(QStringLiteral("Can't fixup the stylesheet of tags header panel: no first closing curly brace "
                               "found within the stylesheet"));
        return;
    }
    alternativeTagsHeaderPanelStylesheet.insert(index, QStringLiteral("border-bottom: 1px solid black;"));
    m_pUI->tagsHeaderPanel->setStyleSheet(alternativeTagsHeaderPanelStylesheet);
    m_pUI->tagsHeaderPanel->setMinimumHeight(23);
    m_pUI->tagsHeaderPanel->setMaximumHeight(23);

    QString alternativeSavedSearchesHeaderPanelStylesheet = m_pUI->savedSearchesHeaderPanel->styleSheet();
    index = alternativeSavedSearchesHeaderPanelStylesheet.indexOf(QStringLiteral("}"));
    if (Q_UNLIKELY(index < 0)) {
        QNDEBUG(QStringLiteral("Can't fixup the stylesheet of saved searches header panel: no first closing curly brace "
                               "found within the stylesheet"));
        return;
    }
    alternativeSavedSearchesHeaderPanelStylesheet.insert(index, QStringLiteral("border-bottom: 1px solid black;"));
    m_pUI->savedSearchesHeaderPanel->setStyleSheet(alternativeSavedSearchesHeaderPanelStylesheet);
    m_pUI->savedSearchesHeaderPanel->setMinimumHeight(23);
    m_pUI->savedSearchesHeaderPanel->setMaximumHeight(23);

    QString alternativeDeletedNotesHeaderPanelStylesheet = m_pUI->deletedNotesHeaderPanel->styleSheet();
    index = alternativeDeletedNotesHeaderPanelStylesheet.indexOf(QStringLiteral("}"));
    if (Q_UNLIKELY(index < 0)) {
        QNDEBUG(QStringLiteral("Can't fixup the stylesheet of deleted notes header panel: no first closing curly brace "
                               "found within the stylesheet"));
        return;
    }
    alternativeDeletedNotesHeaderPanelStylesheet.insert(index, QStringLiteral("border-bottom: 1px solid black;"));
    m_pUI->deletedNotesHeaderPanel->setStyleSheet(alternativeDeletedNotesHeaderPanelStylesheet);
    m_pUI->deletedNotesHeaderPanel->setMinimumHeight(23);
    m_pUI->deletedNotesHeaderPanel->setMaximumHeight(23);
}
#endif

void MainWindow::onSetStatusBarText(QString message, const int duration)
{
    QStatusBar * pStatusBar = m_pUI->statusBar;

    pStatusBar->clearMessage();

    if (m_currentStatusBarChildWidget != Q_NULLPTR) {
        pStatusBar->removeWidget(m_currentStatusBarChildWidget);
        m_currentStatusBarChildWidget = Q_NULLPTR;
    }

    if (duration == 0) {
        m_currentStatusBarChildWidget = new QLabel(message);
        pStatusBar->addWidget(m_currentStatusBarChildWidget);
    }
    else {
        pStatusBar->showMessage(message, duration);
    }
}

#define DISPATCH_TO_NOTE_EDITOR(MainWindowMethod, NoteEditorMethod) \
    void MainWindow::MainWindowMethod() \
    { \
        QNDEBUG(QStringLiteral("MainWindow::" #MainWindowMethod)); \
        \
        NoteEditorWidget * noteEditorWidget = currentNoteEditorTab(); \
        if (!noteEditorWidget) { \
            return; \
        } \
        \
        noteEditorWidget->NoteEditorMethod(); \
    }

DISPATCH_TO_NOTE_EDITOR(onUndoAction, onUndoAction)
DISPATCH_TO_NOTE_EDITOR(onRedoAction, onRedoAction)
DISPATCH_TO_NOTE_EDITOR(onCopyAction, onCopyAction)
DISPATCH_TO_NOTE_EDITOR(onCutAction, onCutAction)
DISPATCH_TO_NOTE_EDITOR(onPasteAction, onPasteAction)
DISPATCH_TO_NOTE_EDITOR(onNoteTextSelectAllToggled, onSelectAllAction)
DISPATCH_TO_NOTE_EDITOR(onNoteTextBoldToggled, onEditorTextBoldToggled)
DISPATCH_TO_NOTE_EDITOR(onNoteTextItalicToggled, onEditorTextItalicToggled)
DISPATCH_TO_NOTE_EDITOR(onNoteTextUnderlineToggled, onEditorTextUnderlineToggled)
DISPATCH_TO_NOTE_EDITOR(onNoteTextStrikethroughToggled, onEditorTextStrikethroughToggled)
DISPATCH_TO_NOTE_EDITOR(onNoteTextAlignLeftAction, onEditorTextAlignLeftAction)
DISPATCH_TO_NOTE_EDITOR(onNoteTextAlignCenterAction, onEditorTextAlignCenterAction)
DISPATCH_TO_NOTE_EDITOR(onNoteTextAlignRightAction, onEditorTextAlignRightAction)
DISPATCH_TO_NOTE_EDITOR(onNoteTextAlignFullAction, onEditorTextAlignFullAction)
DISPATCH_TO_NOTE_EDITOR(onNoteTextAddHorizontalLineAction, onEditorTextAddHorizontalLineAction)
DISPATCH_TO_NOTE_EDITOR(onNoteTextIncreaseFontSizeAction, onEditorTextIncreaseFontSizeAction)
DISPATCH_TO_NOTE_EDITOR(onNoteTextDecreaseFontSizeAction, onEditorTextDecreaseFontSizeAction)
DISPATCH_TO_NOTE_EDITOR(onNoteTextHighlightAction, onEditorTextHighlightAction)
DISPATCH_TO_NOTE_EDITOR(onNoteTextIncreaseIndentationAction, onEditorTextIncreaseIndentationAction)
DISPATCH_TO_NOTE_EDITOR(onNoteTextDecreaseIndentationAction, onEditorTextDecreaseIndentationAction)
DISPATCH_TO_NOTE_EDITOR(onNoteTextInsertUnorderedListAction, onEditorTextInsertUnorderedListAction)
DISPATCH_TO_NOTE_EDITOR(onNoteTextInsertOrderedListAction, onEditorTextInsertOrderedListAction)
DISPATCH_TO_NOTE_EDITOR(onNoteTextInsertToDoAction, onEditorTextInsertToDoAction)
DISPATCH_TO_NOTE_EDITOR(onNoteTextInsertTableDialogAction, onEditorTextInsertTableDialogRequested)
DISPATCH_TO_NOTE_EDITOR(onNoteTextEditHyperlinkAction, onEditorTextEditHyperlinkAction)
DISPATCH_TO_NOTE_EDITOR(onNoteTextCopyHyperlinkAction, onEditorTextCopyHyperlinkAction)
DISPATCH_TO_NOTE_EDITOR(onNoteTextRemoveHyperlinkAction, onEditorTextRemoveHyperlinkAction)
DISPATCH_TO_NOTE_EDITOR(onFindInsideNoteAction, onFindInsideNoteAction)
DISPATCH_TO_NOTE_EDITOR(onFindPreviousInsideNoteAction, onFindPreviousInsideNoteAction)
DISPATCH_TO_NOTE_EDITOR(onReplaceInsideNoteAction, onReplaceInsideNoteAction)

#undef DISPATCH_TO_NOTE_EDITOR

void MainWindow::onImportEnexAction()
{
    QNDEBUG(QStringLiteral("MainWindow::onImportEnexAction"));

    if (Q_UNLIKELY(!m_pAccount)) {
        QNDEBUG(QStringLiteral("No current account, skipping"));
        return;
    }

    if (Q_UNLIKELY(!m_pLocalStorageManagerAsync)) {
        QNDEBUG(QStringLiteral("No local storage manager, skipping"));
        return;
    }

    if (Q_UNLIKELY(!m_pTagModel)) {
        QNDEBUG(QStringLiteral("No tag model, skipping"));
        return;
    }

    if (Q_UNLIKELY(!m_pNotebookModel)) {
        QNDEBUG(QStringLiteral("No notebook model, skipping"));
        return;
    }

    QScopedPointer<EnexImportDialog> pEnexImportDialog(new EnexImportDialog(*m_pAccount, *m_pNotebookModel, this));
    pEnexImportDialog->setWindowModality(Qt::WindowModal);
    if (pEnexImportDialog->exec() != QDialog::Accepted) {
        QNDEBUG(QStringLiteral("The import of ENEX was cancelled"));
        return;
    }

    ErrorString errorDescription;

    QString enexFilePath = pEnexImportDialog->importEnexFilePath(&errorDescription);
    if (enexFilePath.isEmpty())
    {
        if (errorDescription.isEmpty()) {
            errorDescription.setBase(QT_TR_NOOP("Can't import ENEX: internal error, can't retrieve ENEX file path"));
        }

        QNDEBUG(QStringLiteral("Bad ENEX file path: ") << errorDescription);
        onSetStatusBarText(errorDescription.localizedString(), SEC_TO_MSEC(30));
        return;
    }

    QString notebookName = pEnexImportDialog->notebookName(&errorDescription);
    if (notebookName.isEmpty())
    {
        if (errorDescription.isEmpty()) {
            errorDescription.setBase(QT_TR_NOOP("Can't import ENEX: internal error, can't retrieve notebook name"));
        }

        QNDEBUG(QStringLiteral("Bad notebook name: ") << errorDescription);
        onSetStatusBarText(errorDescription.localizedString(), SEC_TO_MSEC(30));
        return;
    }

    EnexImporter * pImporter = new EnexImporter(enexFilePath, notebookName, *m_pLocalStorageManagerAsync,
                                                *m_pTagModel, *m_pNotebookModel, this);
    QObject::connect(pImporter, QNSIGNAL(EnexImporter,enexImportedSuccessfully,QString),
                     this, QNSLOT(MainWindow,onEnexImportCompletedSuccessfully,QString));
    QObject::connect(pImporter, QNSIGNAL(EnexImporter,enexImportFailed,ErrorString),
                     this, QNSLOT(MainWindow,onEnexImportFailed,ErrorString));
    pImporter->start();
}

void MainWindow::onSynchronizationStarted()
{
    QNDEBUG(QStringLiteral("MainWindow::onSynchronizationStarted"));

    onSetStatusBarText(tr("Starting the synchronization") + QStringLiteral("..."));
    m_syncApiRateLimitExceeded = false;
    m_syncInProgress = true;
    startSyncButtonAnimation();
}

void MainWindow::onSynchronizationStopped()
{
    QNDEBUG(QStringLiteral("MainWindow::onSynchronizationStopped"));

    onSetStatusBarText(tr("Synchronization was stopped"), SEC_TO_MSEC(30));
    m_syncApiRateLimitExceeded = false;
    m_syncInProgress = false;
    scheduleSyncButtonAnimationStop();
}

void MainWindow::onSynchronizationManagerFailure(ErrorString errorDescription)
{
    QNDEBUG(QStringLiteral("MainWindow::onSynchronizationManagerFailure: ") << errorDescription);
    onSetStatusBarText(errorDescription.localizedString(), SEC_TO_MSEC(60));
    Q_EMIT stopSynchronization();
}

void MainWindow::onSynchronizationFinished(Account account)
{
    QNDEBUG(QStringLiteral("MainWindow::onSynchronizationFinished: ") << account);

    onSetStatusBarText(tr("Synchronization finished!"), SEC_TO_MSEC(5));
    m_syncInProgress = false;
    scheduleSyncButtonAnimationStop();

    setupRunSyncPeriodicallyTimer();

    QNINFO(QStringLiteral("Synchronization finished for user ") << account.name()
           << QStringLiteral(", id ") << account.id());
}

void MainWindow::onAuthenticationFinished(bool success, ErrorString errorDescription, Account account)
{
    QNDEBUG(QStringLiteral("MainWindow::onAuthenticationFinished: success = ")
            << (success ? QStringLiteral("true") : QStringLiteral("false"))
            << QStringLiteral(", error description = ") << errorDescription
            << QStringLiteral(", account = ") << account);

    bool wasPendingNewEvernoteAccountAuthentication = m_pendingNewEvernoteAccountAuthentication;
    m_pendingNewEvernoteAccountAuthentication = false;

    if (!success)
    {
        // Restore the network proxy which was active before the authentication
        QNetworkProxy::setApplicationProxy(m_applicationProxyBeforeNewEvernoteAccountAuthenticationRequest);
        m_applicationProxyBeforeNewEvernoteAccountAuthenticationRequest = QNetworkProxy(QNetworkProxy::NoProxy);

        onSetStatusBarText(tr("Couldn't authenticate the Evernote user") + QStringLiteral(": ") +
                           errorDescription.localizedString(), SEC_TO_MSEC(30));
        return;
    }

    QNetworkProxy currentProxy = QNetworkProxy::applicationProxy();
    persistNetworkProxySettingsForAccount(account, currentProxy);

    m_applicationProxyBeforeNewEvernoteAccountAuthenticationRequest = QNetworkProxy(QNetworkProxy::NoProxy);

    if (wasPendingNewEvernoteAccountAuthentication) {
        m_pendingSwitchToNewEvernoteAccount = true;
    }

    m_pAccountManager->switchAccount(account);
}

void MainWindow::onAuthenticationRevoked(bool success, ErrorString errorDescription,
                                         qevercloud::UserID userId)
{
    QNDEBUG(QStringLiteral("MainWindow::onAuthenticationRevoked: success = ")
            << (success ? QStringLiteral("true") : QStringLiteral("false"))
            << QStringLiteral(", error description = ") << errorDescription
            << QStringLiteral(", user id = ") << userId);

    if (!success) {
        onSetStatusBarText(tr("Couldn't revoke the authentication") + QStringLiteral(": ") +
                           errorDescription.localizedString(), SEC_TO_MSEC(30));
        return;
    }

    QNINFO(QStringLiteral("Revoked authentication for user with id ") << userId);
}

void MainWindow::onRateLimitExceeded(qint32 secondsToWait)
{
    QNDEBUG(QStringLiteral("MainWindow::onRateLimitExceeded: seconds to wait = ")
            << secondsToWait);

    qint64 currentTimestamp = QDateTime::currentMSecsSinceEpoch();
    qint64 futureTimestamp = currentTimestamp + secondsToWait * 1000;
    QDateTime futureDateTime;
    futureDateTime.setMSecsSinceEpoch(futureTimestamp);

    QDateTime today = QDateTime::currentDateTime();
    bool includeDate = (today.date() != futureDateTime.date());

    QString dateTimeToShow = (includeDate
                              ? futureDateTime.toString(QStringLiteral("dd.MM.yyyy hh:mm:ss"))
                              : futureDateTime.toString(QStringLiteral("hh:mm:ss")));

    onSetStatusBarText(tr("The synchronization has reached the Evernote API rate limit, "
                          "it will continue automatically at approximately") + QStringLiteral(" ") +
                       dateTimeToShow, SEC_TO_MSEC(60));

    m_animatedSyncButtonIcon.setPaused(true);

    QNINFO(QStringLiteral("Evernote API rate limit exceeded, need to wait for ")
           << secondsToWait << QStringLiteral(" seconds, the synchronization will continue at ")
           << dateTimeToShow);

    m_syncApiRateLimitExceeded = true;
}

void MainWindow::onRemoteToLocalSyncDone()
{
    QNDEBUG(QStringLiteral("MainWindow::onRemoteToLocalSyncDone"));

    QNINFO(QStringLiteral("Remote to local sync done"));
    onSetStatusBarText(tr("Received all updates from Evernote servers, sending local changes"));
}

void MainWindow::onSyncChunksDownloadProgress(qint32 highestDownloadedUsn, qint32 highestServerUsn, qint32 lastPreviousUsn)
{
    QNDEBUG(QStringLiteral("MainWindow::onSyncChunksDownloadProgress: highest downloaded USN = ")
            << highestDownloadedUsn << QStringLiteral(", highest server USN = ")
            << highestServerUsn << QStringLiteral(", last previous USN = ")
            << lastPreviousUsn);

    if (Q_UNLIKELY((highestServerUsn <= lastPreviousUsn) || (highestDownloadedUsn <= lastPreviousUsn))) {
        QNWARNING(QStringLiteral("Received incorrect sync chunks download progress state: highest downloaded USN = ")
                  << highestDownloadedUsn << QStringLiteral(", highest server USN = ")
                  << highestServerUsn << QStringLiteral(", last previous USN = ")
                  << lastPreviousUsn);
        return;
    }

    double numerator = highestDownloadedUsn - lastPreviousUsn;
    double denominator = highestServerUsn - lastPreviousUsn;

    double percentage = numerator / denominator * 100.0;
    percentage = std::min(percentage, 100.0);

    onSetStatusBarText(tr("Downloading sync chunks") + QStringLiteral(": ") +
                       QString::number(percentage) + QStringLiteral("%"));
}

void MainWindow::onSyncChunksDownloaded()
{
    QNDEBUG(QStringLiteral("MainWindow::onSyncChunksDownloaded"));
    onSetStatusBarText(tr("Downloaded sync chunks, parsing tags, notebooks and saved searches from them") +
                       QStringLiteral("..."));
}

void MainWindow::onNotesDownloadProgress(quint32 notesDownloaded, quint32 totalNotesToDownload)
{
    QNDEBUG(QStringLiteral("MainWindow::onNotesDownloadProgress: notes downloaded = ")
            << notesDownloaded << QStringLiteral(", total notes to download = ")
            << totalNotesToDownload);

    onSetStatusBarText(tr("Downloading notes") + QStringLiteral(": ") + QString::number(notesDownloaded) +
                       QStringLiteral(" ") + tr("of") + QStringLiteral(" ") + QString::number(totalNotesToDownload));
}

void MainWindow::onResourcesDownloadProgress(quint32 resourcesDownloaded, quint32 totalResourcesToDownload)
{
    QNDEBUG(QStringLiteral("MainWindow::onResourcesDownloadProgress: resources downloaded = ")
            << resourcesDownloaded << QStringLiteral(", total resources to download = ")
            << totalResourcesToDownload);

    onSetStatusBarText(tr("Downloading attachments") + QStringLiteral(": ") + QString::number(resourcesDownloaded) +
                       QStringLiteral(" ") + tr("of") + QStringLiteral(" ") + QString::number(totalResourcesToDownload));
}

void MainWindow::onLinkedNotebookSyncChunksDownloadProgress(qint32 highestDownloadedUsn, qint32 highestServerUsn,
                                                            qint32 lastPreviousUsn, LinkedNotebook linkedNotebook)
{
    QNDEBUG(QStringLiteral("MainWindow::onLinkedNotebookSyncChunksDownloadProgress: highest downloaded USN = ")
            << highestDownloadedUsn << QStringLiteral(", highest server USN = ") << highestServerUsn
            << QStringLiteral(", last previous USN = ") << lastPreviousUsn << QStringLiteral(", linked notebook = ")
            << linkedNotebook);

    if (Q_UNLIKELY((highestServerUsn <= lastPreviousUsn) || (highestDownloadedUsn <= lastPreviousUsn))) {
        QNWARNING(QStringLiteral("Received incorrect sync chunks download progress state: highest downloaded USN = ")
                  << highestDownloadedUsn << QStringLiteral(", highest server USN = ")
                  << highestServerUsn << QStringLiteral(", last previous USN = ")
                  << lastPreviousUsn << QStringLiteral(", linked notebook: ") << linkedNotebook);
        return;
    }

    double percentage = (highestDownloadedUsn - lastPreviousUsn) / (highestServerUsn - lastPreviousUsn) * 100.0;
    percentage = std::min(percentage, 100.0);

    QString message = tr("Downloading sync chunks from linked notebook");

    if (linkedNotebook.hasShareName()) {
        message += QStringLiteral(": ") + linkedNotebook.shareName();
    }

    if (linkedNotebook.hasUsername()) {
        message += QStringLiteral(" (") + linkedNotebook.username() + QStringLiteral(")");
    }

    message += QStringLiteral(": ") + QString::number(percentage) + QStringLiteral("%");
    onSetStatusBarText(message);
}

void MainWindow::onLinkedNotebooksSyncChunksDownloaded()
{
    QNDEBUG(QStringLiteral("MainWindow::onLinkedNotebooksSyncChunksDownloaded"));
    onSetStatusBarText(tr("Downloaded the sync chunks from linked notebooks"));
}

void MainWindow::onLinkedNotebooksNotesDownloadProgress(quint32 notesDownloaded, quint32 totalNotesToDownload)
{
    QNDEBUG(QStringLiteral("MainWindow::onLinkedNotebooksNotesDownloadProgress: notes downloaded = ")
            << notesDownloaded << QStringLiteral(", total notes to download = ")
            << totalNotesToDownload);

    onSetStatusBarText(tr("Downloading notes from linked notebooks") + QStringLiteral(": ") + QString::number(notesDownloaded) +
                       QStringLiteral(" ") + tr("of") + QStringLiteral(" ") + QString::number(totalNotesToDownload));
}

void MainWindow::onRemoteToLocalSyncStopped()
{
    QNDEBUG(QStringLiteral("MainWindow::onRemoteToLocalSyncStopped"));
    onSynchronizationStopped();
}

void MainWindow::onSendLocalChangesStopped()
{
    QNDEBUG(QStringLiteral("MainWindow::onSendLocalChangesStopped"));
    onSynchronizationStopped();
}

void MainWindow::onEvernoteAccountAuthenticationRequested(QString host, QNetworkProxy proxy)
{
    QNDEBUG(QStringLiteral("MainWindow::onEvernoteAccountAuthenticationRequested: host = ")
            << host << QStringLiteral(", proxy type = ") << proxy.type() << QStringLiteral(", proxy host = ")
            << proxy.hostName() << QStringLiteral(", proxy port = ") << proxy.port()
            << QStringLiteral(", proxy user = ") << proxy.user());

    if ((host != m_synchronizationManagerHost) || !m_pSynchronizationManager) {
        m_synchronizationManagerHost = host;
        setupSynchronizationManager();
    }

    // Set the proxy specified within the slot argument but remember the previous application proxy
    // so that it can be restored in case of authentication failure
    m_applicationProxyBeforeNewEvernoteAccountAuthenticationRequest = QNetworkProxy::applicationProxy();
    QNetworkProxy::setApplicationProxy(proxy);

    m_pendingNewEvernoteAccountAuthentication = true;
    Q_EMIT authenticate();
}

void MainWindow::onNoteTextSpellCheckToggled()
{
    QNDEBUG(QStringLiteral("MainWindow::onNoteTextSpellCheckToggled"));

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
    QNDEBUG(QStringLiteral("MainWindow::onShowNoteSource"));

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
    QNDEBUG(QStringLiteral("MainWindow::onSaveNoteAction"));

    NoteEditorWidget * pNoteEditorWidget = currentNoteEditorTab();
    if (!pNoteEditorWidget) {
        return;
    }

    pNoteEditorWidget->onSaveNoteAction();
}

void MainWindow::onNewNotebookCreationRequested()
{
    QNDEBUG(QStringLiteral("MainWindow::onNewNotebookCreationRequested"));

    if (Q_UNLIKELY(!m_pNotebookModel)) {
        ErrorString error(QT_TR_NOOP("Can't create a new notebook: no notebook model is set up"));
        QNWARNING(error);
        onSetStatusBarText(error.localizedString(), SEC_TO_MSEC(30));
        return;
    }

    QScopedPointer<AddOrEditNotebookDialog> pAddNotebookDialog(new AddOrEditNotebookDialog(m_pNotebookModel, this));
    pAddNotebookDialog->setWindowModality(Qt::WindowModal);
    Q_UNUSED(pAddNotebookDialog->exec())
}

void MainWindow::onRemoveNotebookButtonPressed()
{
    QNDEBUG(QStringLiteral("MainWindow::onRemoveNotebookButtonPressed"));
    m_pUI->notebooksTreeView->deleteSelectedItem();
}

void MainWindow::onNotebookInfoButtonPressed()
{
    QNDEBUG(QStringLiteral("MainWindow::onNotebookInfoButtonPressed"));

    QModelIndex index = m_pUI->notebooksTreeView->currentlySelectedItemIndex();
    NotebookModelItemInfoWidget * pNotebookModelItemInfoWidget = new NotebookModelItemInfoWidget(index, this);
    showInfoWidget(pNotebookModelItemInfoWidget);
}

void MainWindow::onNewTagCreationRequested()
{
    QNDEBUG(QStringLiteral("MainWindow::onNewTagCreationRequested"));

    if (Q_UNLIKELY(!m_pTagModel)) {
        ErrorString error(QT_TR_NOOP("Can't create a new tag: no tag model is set up"));
        QNWARNING(error);
        onSetStatusBarText(error.localizedString(), SEC_TO_MSEC(30));
        return;
    }

    QScopedPointer<AddOrEditTagDialog> pAddTagDialog(new AddOrEditTagDialog(m_pTagModel, this));
    pAddTagDialog->setWindowModality(Qt::WindowModal);
    Q_UNUSED(pAddTagDialog->exec())
}

void MainWindow::onRemoveTagButtonPressed()
{
    QNDEBUG(QStringLiteral("MainWindow::onRemoveTagButtonPressed"));
    m_pUI->tagsTreeView->deleteSelectedItem();
}

void MainWindow::onTagInfoButtonPressed()
{
    QNDEBUG(QStringLiteral("MainWindow::onTagInfoButtonPressed"));

    QModelIndex index = m_pUI->tagsTreeView->currentlySelectedItemIndex();
    TagModelItemInfoWidget * pTagModelItemInfoWidget = new TagModelItemInfoWidget(index, this);
    showInfoWidget(pTagModelItemInfoWidget);
}

void MainWindow::onNewSavedSearchCreationRequested()
{
    QNDEBUG(QStringLiteral("MainWindow::onNewSavedSearchCreationRequested"));

    if (Q_UNLIKELY(!m_pSavedSearchModel)) {
        ErrorString error(QT_TR_NOOP("Can't create a new saved search: no saved search model is set up"));
        QNWARNING(error);
        onSetStatusBarText(error.localizedString(), SEC_TO_MSEC(30));
        return;
    }

    QScopedPointer<AddOrEditSavedSearchDialog> pAddSavedSearchDialog(new AddOrEditSavedSearchDialog(m_pSavedSearchModel, this));
    pAddSavedSearchDialog->setWindowModality(Qt::WindowModal);
    Q_UNUSED(pAddSavedSearchDialog->exec())
}

void MainWindow::onRemoveSavedSearchButtonPressed()
{
    QNDEBUG(QStringLiteral("MainWindow::onRemoveSavedSearchButtonPressed"));
    m_pUI->savedSearchesTableView->deleteSelectedItem();
}

void MainWindow::onSavedSearchInfoButtonPressed()
{
    QNDEBUG(QStringLiteral("MainWindow::onSavedSearchInfoButtonPressed"));

    QModelIndex index = m_pUI->savedSearchesTableView->currentlySelectedItemIndex();
    SavedSearchModelItemInfoWidget * pSavedSearchModelItemInfoWidget = new SavedSearchModelItemInfoWidget(index, this);
    showInfoWidget(pSavedSearchModelItemInfoWidget);
}

void MainWindow::onUnfavoriteItemButtonPressed()
{
    QNDEBUG(QStringLiteral("MainWindow::onUnfavoriteItemButtonPressed"));
    m_pUI->favoritesTableView->deleteSelectedItems();
}

void MainWindow::onFavoritedItemInfoButtonPressed()
{
    QNDEBUG(QStringLiteral("MainWindow::onFavoritedItemInfoButtonPressed"));
    QModelIndex index = m_pUI->favoritesTableView->currentlySelectedItemIndex();
    if (!index.isValid()) {
        Q_UNUSED(informationMessageBox(this, tr("Not exactly one favorited item is selected"),
                                       tr("Please select the only one favorited item to "
                                          "see its detailed info")));
        return;
    }

    FavoritesModel * pFavoritesModel = qobject_cast<FavoritesModel*>(m_pUI->favoritesTableView->model());
    if (Q_UNLIKELY(!pFavoritesModel)) {
        Q_UNUSED(internalErrorMessageBox(this, tr("Failed to cast the favorited table "
                                                  "view's model to favorites model")))
        return;
    }

    const FavoritesModelItem * pItem = pFavoritesModel->itemAtRow(index.row());
    if (Q_UNLIKELY(!pItem)) {
        Q_UNUSED(internalErrorMessageBox(this, tr("Favorites model returned null pointer "
                                                  "to favorited item for the selected index")))
        return;
    }

    switch(pItem->type())
    {
    case FavoritesModelItem::Type::Note:
        Q_EMIT noteInfoDialogRequested(pItem->localUid());
        break;
    case FavoritesModelItem::Type::Notebook:
        {
            if (Q_LIKELY(m_pNotebookModel)) {
                QModelIndex notebookIndex = m_pNotebookModel->indexForLocalUid(pItem->localUid());
                NotebookModelItemInfoWidget * pNotebookItemInfoWidget = new NotebookModelItemInfoWidget(notebookIndex, this);
                showInfoWidget(pNotebookItemInfoWidget);
            }
            else {
                Q_UNUSED(internalErrorMessageBox(this, tr("No notebook model exists at the moment")))
            }
            break;
        }
    case FavoritesModelItem::Type::SavedSearch:
        {
            if (Q_LIKELY(m_pSavedSearchModel)) {
                QModelIndex savedSearchIndex = m_pSavedSearchModel->indexForLocalUid(pItem->localUid());
                SavedSearchModelItemInfoWidget * pSavedSearchItemInfoWidget = new SavedSearchModelItemInfoWidget(savedSearchIndex, this);
                showInfoWidget(pSavedSearchItemInfoWidget);
            }
            else {
                Q_UNUSED(internalErrorMessageBox(this, tr("No saved search model exists at the moment"));)
            }
            break;
        }
    case FavoritesModelItem::Type::Tag:
        {
            if (Q_LIKELY(m_pTagModel)) {
                QModelIndex tagIndex = m_pTagModel->indexForLocalUid(pItem->localUid());
                TagModelItemInfoWidget * pTagItemInfoWidget = new TagModelItemInfoWidget(tagIndex, this);
                showInfoWidget(pTagItemInfoWidget);
            }
            else {
                Q_UNUSED(internalErrorMessageBox(this, tr("No tag model exists at the moment"));)
            }
            break;
        }
    default:
        Q_UNUSED(internalErrorMessageBox(this, tr("Incorrect favorited item type") +
                                         QStringLiteral(": ") + QString::number(pItem->type())))
        break;
    }
}

void MainWindow::onRestoreDeletedNoteButtonPressed()
{
    QNDEBUG(QStringLiteral("MainWindow::onRestoreDeletedNoteButtonPressed"));
    m_pUI->deletedNotesTableView->restoreCurrentlySelectedNote();
}

void MainWindow::onDeleteNotePermanentlyButtonPressed()
{
    QNDEBUG(QStringLiteral("MainWindow::onDeleteNotePermanentlyButtonPressed"));
    m_pUI->deletedNotesTableView->deleteCurrentlySelectedNotePermanently();
}

void MainWindow::onDeletedNoteInfoButtonPressed()
{
    QNDEBUG(QStringLiteral("MainWindow::onDeletedNoteInfoButtonPressed"));
    m_pUI->deletedNotesTableView->showCurrentlySelectedNoteInfo();
}

void MainWindow::showInfoWidget(QWidget * pWidget)
{
    pWidget->setAttribute(Qt::WA_DeleteOnClose, true);
    pWidget->setWindowModality(Qt::WindowModal);
    pWidget->adjustSize();
#ifndef Q_OS_MAC
    centerWidget(*pWidget);
#endif
    pWidget->show();
}

void MainWindow::onFiltersViewTogglePushButtonPressed()
{
    QNDEBUG(QStringLiteral("MainWindow::onFiltersViewTogglePushButtonPressed"));

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

void MainWindow::onShowSettingsDialogAction()
{
    QNDEBUG(QStringLiteral("MainWindow::onShowSettingsDialogAction"));

    QList<QMenu*> menus = m_pUI->menuBar->findChildren<QMenu*>();
    ActionsInfo actionsInfo(menus);

    QScopedPointer<PreferencesDialog> pPreferencesDialog(new PreferencesDialog(*m_pAccountManager,
                                                                               m_shortcutManager,
                                                                               *m_pSystemTrayIconManager,
                                                                               actionsInfo, this));
    pPreferencesDialog->setWindowModality(Qt::WindowModal);

    QObject::connect(pPreferencesDialog.data(), QNSIGNAL(PreferencesDialog,noteEditorUseLimitedFontsOptionChanged,bool),
                     this, QNSLOT(MainWindow,onUseLimitedFontsPreferenceChanged,bool));
    QObject::connect(pPreferencesDialog.data(), QNSIGNAL(PreferencesDialog,synchronizationDownloadNoteThumbnailsOptionChanged,bool),
                     this, QNSIGNAL(MainWindow,synchronizationDownloadNoteThumbnailsOptionChanged,bool));
    QObject::connect(pPreferencesDialog.data(), QNSIGNAL(PreferencesDialog,synchronizationDownloadInkNoteImagesOptionChanged,bool),
                     this, QNSIGNAL(MainWindow,synchronizationDownloadInkNoteImagesOptionChanged,bool));
    QObject::connect(pPreferencesDialog.data(), QNSIGNAL(PreferencesDialog,showNoteThumbnailsOptionChanged,bool),
                     this, QNSLOT(MainWindow,onShowNoteThumbnailsPreferenceChanged,bool));
    QObject::connect(pPreferencesDialog.data(), QNSIGNAL(PreferencesDialog,runSyncPeriodicallyOptionChanged,int),
                     this, QNSLOT(MainWindow,onRunSyncEachNumMinitesPreferenceChanged,int));

    Q_UNUSED(pPreferencesDialog->exec());
}

void MainWindow::onNoteSortingModeChanged(int index)
{
    QNDEBUG(QStringLiteral("MainWindow::onNoteSortingModeChanged: index = ") << index);

    persistChosenNoteSortingMode(index);

    if (Q_UNLIKELY(!m_pNoteModel)) {
        QNDEBUG(QStringLiteral("No note model, ignoring the change"));
        return;
    }

    switch(index)
    {
    case NoteSortingModes::CreatedAscending:
        m_pNoteModel->sort(NoteModel::Columns::CreationTimestamp, Qt::AscendingOrder);
        break;
    case NoteSortingModes::CreatedDescending:
        m_pNoteModel->sort(NoteModel::Columns::CreationTimestamp, Qt::DescendingOrder);
        break;
    case NoteSortingModes::ModifiedAscending:
        m_pNoteModel->sort(NoteModel::Columns::ModificationTimestamp, Qt::AscendingOrder);
        break;
    case NoteSortingModes::ModifiedDescending:
        m_pNoteModel->sort(NoteModel::Columns::ModificationTimestamp, Qt::DescendingOrder);
        break;
    case NoteSortingModes::TitleAscending:
        m_pNoteModel->sort(NoteModel::Columns::Title, Qt::AscendingOrder);
        break;
    case NoteSortingModes::TitleDescending:
        m_pNoteModel->sort(NoteModel::Columns::Title, Qt::DescendingOrder);
        break;
    case NoteSortingModes::SizeAscending:
        m_pNoteModel->sort(NoteModel::Columns::Size, Qt::AscendingOrder);
        break;
    case NoteSortingModes::SizeDescending:
        m_pNoteModel->sort(NoteModel::Columns::Size, Qt::DescendingOrder);
        break;
    default:
        {
            ErrorString error(QT_TR_NOOP("Internal error: got unknown note sorting order, fallback to the default"));
            QNWARNING(error << QStringLiteral(", sorting mode index = ") << index);
            onSetStatusBarText(error.localizedString(), SEC_TO_MSEC(30));

            m_pNoteModel->sort(NoteModel::Columns::CreationTimestamp, Qt::AscendingOrder);
            break;
        }
    }
}

void MainWindow::onNewNoteCreationRequested()
{
    QNDEBUG(QStringLiteral("MainWindow::onNewNoteCreationRequested"));
    createNewNote(NoteEditorTabsAndWindowsCoordinator::NoteEditorMode::Any);
}

void MainWindow::onCopyInAppLinkNoteRequested(QString noteLocalUid, QString noteGuid)
{
    QNDEBUG(QStringLiteral("MainWindow::onCopyInAppLinkNoteRequested: note local uid = ")
            << noteLocalUid << QStringLiteral(", note guid = ") << noteGuid);

    if (noteGuid.isEmpty()) {
        QNDEBUG(QStringLiteral("Can't copy the in-app note link: note guid is empty"));
        return;
    }

    if (Q_UNLIKELY(m_pAccount.isNull())) {
        QNDEBUG(QStringLiteral("Can't copy the in-app note link: no current account"));
        return;
    }

    if (Q_UNLIKELY(m_pAccount->type() != Account::Type::Evernote)) {
        QNDEBUG(QStringLiteral("Can't copy the in-app note link: the current account is not of Evernote type"));
        return;
    }

    qevercloud::UserID id = m_pAccount->id();
    if (Q_UNLIKELY(id < 0)) {
        QNDEBUG(QStringLiteral("Can't copy the in-app note link: the current account's id is negative"));
        return;
    }

    QString shardId = m_pAccount->shardId();
    if (shardId.isEmpty()) {
        QNDEBUG(QStringLiteral("Can't copy the in-app note link: the current account's shard id is empty"));
        return;
    }

    QString urlString = QStringLiteral("evernote:///view/") + QString::number(id) + QStringLiteral("/") + shardId +
                        QStringLiteral("/") + noteGuid + QStringLiteral("/") + noteGuid;

    QClipboard * pClipboard = QApplication::clipboard();
    if (pClipboard) {
        QNTRACE(QStringLiteral("Setting the composed in-app note URL to the clipboard: ") << urlString);
        pClipboard->setText(urlString);
    }
}

void MainWindow::onNoteModelAllNotesListed()
{
    QNDEBUG(QStringLiteral("MainWindow::onNoteModelAllNotesListed"));

    // NOTE: the task of this slot is to ensure that the note selected within the note list view
    // is the same as the one in the current note editor tab

    QObject::disconnect(m_pNoteModel, QNSIGNAL(NoteModel,notifyAllNotesListed),
                        this, QNSLOT(MainWindow,onNoteModelAllNotesListed));

    NoteEditorWidget * pCurrentNoteEditorTab = currentNoteEditorTab();
    if (!pCurrentNoteEditorTab) {
        QNDEBUG(QStringLiteral("No current note editor tab"));
        return;
    }

    QString noteLocalUid = pCurrentNoteEditorTab->noteLocalUid();
    m_pUI->noteListView->setCurrentNoteByLocalUid(noteLocalUid);
}

void MainWindow::onCurrentNoteInListChanged(QString noteLocalUid)
{
    QNDEBUG(QStringLiteral("MainWindow::onCurrentNoteInListChanged: ") << noteLocalUid);
    m_pNoteEditorTabsAndWindowsCoordinator->addNote(noteLocalUid);
}

void MainWindow::onOpenNoteInSeparateWindow(QString noteLocalUid)
{
    QNDEBUG(QStringLiteral("MainWindow::onOpenNoteInSeparateWindow: ") << noteLocalUid);
    m_pNoteEditorTabsAndWindowsCoordinator->addNote(noteLocalUid, NoteEditorTabsAndWindowsCoordinator::NoteEditorMode::Window);
}

void MainWindow::onDeleteCurrentNoteButtonPressed()
{
    QNDEBUG(QStringLiteral("MainWindow::onDeleteCurrentNoteButtonPressed"));

    if (Q_UNLIKELY(!m_pNoteModel)) {
        ErrorString errorDescription(QT_TR_NOOP("Can't delete the current note: internal error, no note model"));
        QNDEBUG(errorDescription);
        onSetStatusBarText(errorDescription.localizedString(), SEC_TO_MSEC(30));
        return;
    }

    NoteEditorWidget * pNoteEditorWidget = currentNoteEditorTab();
    if (!pNoteEditorWidget) {
        ErrorString errorDescription(QT_TR_NOOP("Can't delete the current note: no note editor tabs"));
        QNDEBUG(errorDescription);
        onSetStatusBarText(errorDescription.localizedString(), SEC_TO_MSEC(30));
        return;
    }

    bool res = m_pNoteModel->deleteNote(pNoteEditorWidget->noteLocalUid());
    if (Q_UNLIKELY(!res)) {
        ErrorString errorDescription(QT_TR_NOOP("Can't delete the current note: can't find the note to be deleted"));
        QNDEBUG(errorDescription);
        onSetStatusBarText(errorDescription.localizedString(), SEC_TO_MSEC(30));
        return;
    }
}

void MainWindow::onCurrentNoteInfoRequested()
{
    QNDEBUG(QStringLiteral("MainWindow::onCurrentNoteInfoRequested"));

    NoteEditorWidget * pNoteEditorWidget = currentNoteEditorTab();
    if (!pNoteEditorWidget) {
        ErrorString errorDescription(QT_TR_NOOP("Can't show note info: no note editor tabs"));
        QNDEBUG(errorDescription);
        onSetStatusBarText(errorDescription.localizedString(), SEC_TO_MSEC(30));
        return;
    }

    Q_EMIT noteInfoDialogRequested(pNoteEditorWidget->noteLocalUid());
}

void MainWindow::onCurrentNotePrintRequested()
{
    QNDEBUG(QStringLiteral("MainWindow::onCurrentNotePrintRequested"));

    NoteEditorWidget * pNoteEditorWidget = currentNoteEditorTab();
    if (!pNoteEditorWidget) {
        ErrorString errorDescription(QT_TR_NOOP("Can't print note: no note editor tabs"));
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
    QNDEBUG(QStringLiteral("MainWindow::onCurrentNotePdfExportRequested"));

    NoteEditorWidget * pNoteEditorWidget = currentNoteEditorTab();
    if (!pNoteEditorWidget) {
        ErrorString errorDescription(QT_TR_NOOP("Can't export note to pdf: no note editor tabs"));
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
    QNDEBUG(QStringLiteral("MainWindow::onExportNotesToEnexRequested: ")
            << noteLocalUids.join(QStringLiteral(", ")));

    if (Q_UNLIKELY(noteLocalUids.isEmpty())) {
        QNDEBUG(QStringLiteral("The list of note local uids to export is empty"));
        return;
    }

    if (Q_UNLIKELY(!m_pAccount)) {
        QNDEBUG(QStringLiteral("No current account, skipping"));
        return;
    }

    if (Q_UNLIKELY(!m_pLocalStorageManagerAsync)) {
        QNDEBUG(QStringLiteral("No local storage manager, skipping"));
        return;
    }

    if (Q_UNLIKELY(!m_pNoteEditorTabsAndWindowsCoordinator)) {
        QNDEBUG(QStringLiteral("No note editor tabs and windows coordinator, skipping"));
        return;
    }

    if (Q_UNLIKELY(!m_pTagModel)) {
        QNDEBUG(QStringLiteral("No tag model, skipping"));
        return;
    }

    ApplicationSettings appSettings(*m_pAccount, QUENTIER_UI_SETTINGS);
    appSettings.beginGroup(NOTE_EDITOR_SETTINGS_GROUP_NAME);
    QString lastExportNoteToEnexPath = appSettings.value(LAST_EXPORT_NOTE_TO_ENEX_PATH_SETTINGS_KEY).toString();
    appSettings.endGroup();

    if (lastExportNoteToEnexPath.isEmpty()) {
        lastExportNoteToEnexPath = documentsPath();
    }

    QScopedPointer<EnexExportDialog> pExportEnexDialog(new EnexExportDialog(*m_pAccount, this));
    pExportEnexDialog->setWindowModality(Qt::WindowModal);
    if (pExportEnexDialog->exec() != QDialog::Accepted) {
        QNDEBUG(QStringLiteral("Enex export was not confirmed"));
        return;
    }

    QString enexFilePath = pExportEnexDialog->exportEnexFilePath();

    QFileInfo enexFileInfo(enexFilePath);
    if (enexFileInfo.exists())
    {
        if (!enexFileInfo.isWritable()) {
            QNINFO(QStringLiteral("Chosen ENEX export file is not writable: ") << enexFilePath);
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
                QNDEBUG(QStringLiteral("Failed to create the folder for the selected ENEX file"));
                onSetStatusBarText(tr("Could not create the folder for the selected ENEX file") +
                                   QStringLiteral(": ") + enexFilePath, SEC_TO_MSEC(30));
                return;
            }
        }
    }

    EnexExporter * pExporter = new EnexExporter(*m_pLocalStorageManagerAsync,
                                                *m_pNoteEditorTabsAndWindowsCoordinator,
                                                *m_pTagModel, this);
    pExporter->setTargetEnexFilePath(enexFilePath);
    pExporter->setIncludeTags(pExportEnexDialog->exportTags());
    pExporter->setNoteLocalUids(noteLocalUids);

    QObject::connect(pExporter, QNSIGNAL(EnexExporter,notesExportedToEnex,QString),
                     this, QNSLOT(MainWindow,onExportedNotesToEnex,QString));
    QObject::connect(pExporter, QNSIGNAL(EnexExporter,failedToExportNotesToEnex,ErrorString),
                     this, QNSLOT(MainWindow,onExportNotesToEnexFailed,ErrorString));
    pExporter->start();
}

void MainWindow::onExportedNotesToEnex(QString enex)
{
    QNDEBUG(QStringLiteral("MainWindow::onExportedNotesToEnex"));

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

    AsyncFileWriter * pAsyncFileWriter = new AsyncFileWriter(enexFilePath, enexRawData);
    QObject::connect(pAsyncFileWriter, QNSIGNAL(AsyncFileWriter,fileSuccessfullyWritten,QString),
                     this, QNSLOT(MainWindow,onEnexFileWrittenSuccessfully,QString));
    QObject::connect(pAsyncFileWriter, QNSIGNAL(AsyncFileWriter,fileWriteFailed,ErrorString),
                     this, QNSLOT(MainWindow,onEnexFileWriteFailed));
    QObject::connect(pAsyncFileWriter, QNSIGNAL(AsyncFileWriter,fileWriteIncomplete,qint64,qint64),
                     this, QNSLOT(MainWindow,onEnexFileWriteIncomplete,qint64,qint64));
    QThreadPool::globalInstance()->start(pAsyncFileWriter);
}

void MainWindow::onExportNotesToEnexFailed(ErrorString errorDescription)
{
    QNDEBUG(QStringLiteral("MainWindow::onExportNotesToEnexFailed: ") << errorDescription);

    EnexExporter * pExporter = qobject_cast<EnexExporter*>(sender());
    if (pExporter) {
        pExporter->clear();
        pExporter->deleteLater();
    }

    onSetStatusBarText(errorDescription.localizedString(), SEC_TO_MSEC(30));
}

void MainWindow::onEnexFileWrittenSuccessfully(QString filePath)
{
    QNDEBUG(QStringLiteral("MainWindow::onEnexFileWrittenSuccessfully: ") << filePath);
    onSetStatusBarText(tr("Successfully exported note(s) to ENEX: ") + QDir::toNativeSeparators(filePath), SEC_TO_MSEC(5));
}

void MainWindow::onEnexFileWriteFailed(ErrorString errorDescription)
{
    QNDEBUG(QStringLiteral("MainWindow::onEnexFileWriteFailed: ") << errorDescription);
    onSetStatusBarText(tr("Can't export note(s) to ENEX, failed to write the ENEX to file") +
                       QStringLiteral(": ") + errorDescription.localizedString(), SEC_TO_MSEC(30));
}

void MainWindow::onEnexFileWriteIncomplete(qint64 bytesWritten, qint64 bytesTotal)
{
    QNDEBUG(QStringLiteral("MainWindow::onEnexFileWriteIncomplete: bytes written = ")
            << bytesWritten << QStringLiteral(", bytes total = ") << bytesTotal);


    if (bytesWritten == 0) {
        onSetStatusBarText(tr("Can't export note(s) to ENEX, failed to write the ENEX to file"), SEC_TO_MSEC(30));
    }
    else {
        onSetStatusBarText(tr("Can't export note(s) to ENEX, failed to write the ENEX to file, "
                              "only a portion of data has been written") + QStringLiteral(": ") +
                           QString::number(bytesWritten) + QStringLiteral("/") + QString::number(bytesTotal), SEC_TO_MSEC(30));
    }
}

void MainWindow::onEnexImportCompletedSuccessfully(QString enexFilePath)
{
    QNDEBUG(QStringLiteral("MainWindow::onEnexImportCompletedSuccessfully: ") << enexFilePath);

    onSetStatusBarText(tr("Successfully imported note(s) from ENEX file") +
                       QStringLiteral(": ") + QDir::toNativeSeparators(enexFilePath), SEC_TO_MSEC(5));

    EnexImporter * pImporter = qobject_cast<EnexImporter*>(sender());
    if (pImporter) {
        pImporter->clear();
        pImporter->deleteLater();
    }
}

void MainWindow::onEnexImportFailed(ErrorString errorDescription)
{
    QNDEBUG(QStringLiteral("MainWindow::onEnexImportFailed: ") << errorDescription);

    onSetStatusBarText(errorDescription.localizedString(), SEC_TO_MSEC(30));

    EnexImporter * pImporter = qobject_cast<EnexImporter*>(sender());
    if (pImporter) {
        pImporter->clear();
        pImporter->deleteLater();
    }
}

void MainWindow::onUseLimitedFontsPreferenceChanged(bool flag)
{
    QNDEBUG(QStringLiteral("MainWindow::onUseLimitedFontsPreferenceChanged: flag = ")
            << (flag ? QStringLiteral("enabled") : QStringLiteral("disabled")));

    if (m_pNoteEditorTabsAndWindowsCoordinator) {
        m_pNoteEditorTabsAndWindowsCoordinator->setUseLimitedFonts(flag);
    }
}

void MainWindow::onShowNoteThumbnailsPreferenceChanged(bool flag)
{
    QNDEBUG(QStringLiteral("MainWindow::onShowNoteThumbnailsPreferenceChanged: ")
            << (flag ? QStringLiteral("enabled") : QStringLiteral("disabled")));

    NoteItemDelegate * pNoteItemDelegate = qobject_cast<NoteItemDelegate*>(m_pUI->noteListView->itemDelegate());
    if (Q_UNLIKELY(!pNoteItemDelegate)) {
        QNDEBUG(QStringLiteral("No NoteItemDelegate"));
        return;
    }

    pNoteItemDelegate->setShowNoteThumbnails(flag);
    m_pUI->noteListView->update();
}

void MainWindow::onRunSyncEachNumMinitesPreferenceChanged(int runSyncEachNumMinutes)
{
    QNDEBUG(QStringLiteral("MainWindow::onRunSyncEachNumMinitesPreferenceChanged: ")
            << runSyncEachNumMinutes);

    if (m_runSyncPeriodicallyTimerId != 0) {
        killTimer(m_runSyncPeriodicallyTimerId);
        m_runSyncPeriodicallyTimerId = 0;
    }

    if (runSyncEachNumMinutes <= 0) {
        return;
    }

    if (Q_UNLIKELY(!m_pAccount || (m_pAccount->type() != Account::Type::Evernote))) {
        return;
    }

    m_runSyncPeriodicallyTimerId = startTimer(SEC_TO_MSEC(runSyncEachNumMinutes * 60));
}

void MainWindow::onNoteSearchQueryChanged(const QString & query)
{
    QNDEBUG(QStringLiteral("MainWindow::onNoteSearchQueryChanged: ") << query);
    m_noteSearchQueryValidated = false;
}

void MainWindow::onNoteSearchQueryReady()
{
    QString noteSearchQuery = m_pUI->searchQueryLineEdit->text();
    QNDEBUG(QStringLiteral("MainWindow::onNoteSearchQueryReady: query = ") << noteSearchQuery);

    if (noteSearchQuery.isEmpty()) {
        // TODO: remove search query from NoteFilterModel
        return;
    }

    bool res = checkNoteSearchQuery(noteSearchQuery);
    if (!res) {
        // TODO: remove search query from NoteFilterModel
        return;
    }

    // TODO: set the search query to the NoteFilterModel
}

void MainWindow::onSaveNoteSearchQueryButtonPressed()
{
    QString noteSearchQuery = m_pUI->searchQueryLineEdit->text();
    QNDEBUG(QStringLiteral("MainWindow::onSaveNoteSearchQueryButtonPressed, query = ")
            << noteSearchQuery);

    bool res = checkNoteSearchQuery(noteSearchQuery);
    if (!res) {
        // TODO: remove search query from NoteFilterModel
        return;
    }

    QScopedPointer<AddOrEditSavedSearchDialog> pAddSavedSearchDialog(new AddOrEditSavedSearchDialog(m_pSavedSearchModel, this));
    pAddSavedSearchDialog->setWindowModality(Qt::WindowModal);
    pAddSavedSearchDialog->setQuery(noteSearchQuery);
    Q_UNUSED(pAddSavedSearchDialog->exec())
}

void MainWindow::onNewNoteRequestedFromSystemTrayIcon()
{
    QNDEBUG(QStringLiteral("MainWindow::onNewNoteRequestedFromSystemTrayIcon"));

    Qt::WindowStates state = windowState();
    bool isMinimized = (state & Qt::WindowMinimized);

    bool shown = !isMinimized && !isHidden();

    createNewNote(shown
                  ? NoteEditorTabsAndWindowsCoordinator::NoteEditorMode::Any
                  : NoteEditorTabsAndWindowsCoordinator::NoteEditorMode::Window);
}

void MainWindow::onQuitRequestedFromSystemTrayIcon()
{
    QNINFO(QStringLiteral("MainWindow::onQuitRequestedFromSystemTrayIcon"));
    onQuitAction();
}

void MainWindow::onAccountSwitchRequested(Account account)
{
    QNDEBUG(QStringLiteral("MainWindow::onAccountSwitchRequested: ") << account);

    stopListeningForSplitterMoves();
    m_pAccountManager->switchAccount(account);
}

void MainWindow::onSystemTrayIconManagerError(ErrorString errorDescription)
{
    QNDEBUG(QStringLiteral("MainWindow::onSystemTrayIconManagerError: ") << errorDescription);
    onSetStatusBarText(errorDescription.localizedString(), SEC_TO_MSEC(30));
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
    QNDEBUG(QStringLiteral("MainWindow::onShowInfoAboutQuentierActionTriggered"));

    AboutQuentierWidget * pWidget = findChild<AboutQuentierWidget*>();
    if (pWidget) {
        pWidget->show();
        pWidget->raise();
        pWidget->setFocus();
        return;
    }

    pWidget = new AboutQuentierWidget(this);
    pWidget->setAttribute(Qt::WA_DeleteOnClose, true);
    centerWidget(*pWidget);
    pWidget->adjustSize();
    pWidget->show();
}

void MainWindow::onNoteEditorError(ErrorString error)
{
    QNINFO(QStringLiteral("MainWindow::onNoteEditorError: ") << error);
    onSetStatusBarText(error.localizedString(), SEC_TO_MSEC(30));
}

void MainWindow::onModelViewError(ErrorString error)
{
    QNINFO(QStringLiteral("MainWindow::onModelViewError: ") << error);
    onSetStatusBarText(error.localizedString(), SEC_TO_MSEC(30));
}

void MainWindow::onNoteEditorSpellCheckerNotReady()
{
    QNDEBUG(QStringLiteral("MainWindow::onNoteEditorSpellCheckerNotReady"));

    NoteEditorWidget * noteEditor = qobject_cast<NoteEditorWidget*>(sender());
    if (!noteEditor) {
        QNTRACE(QStringLiteral("Can't cast caller to note editor widget, skipping"));
        return;
    }

    NoteEditorWidget * currentEditor = currentNoteEditorTab();
    if (!currentEditor || (currentEditor != noteEditor)) {
        QNTRACE(QStringLiteral("Not an update from current note editor, skipping"));
        return;
    }

    onSetStatusBarText(tr("Spell checker is loading dictionaries, please wait"));
}

void MainWindow::onNoteEditorSpellCheckerReady()
{
    QNDEBUG(QStringLiteral("MainWindow::onNoteEditorSpellCheckerReady"));

    NoteEditorWidget * noteEditor = qobject_cast<NoteEditorWidget*>(sender());
    if (!noteEditor) {
        QNTRACE(QStringLiteral("Can't cast caller to note editor widget, skipping"));
        return;
    }

    NoteEditorWidget * currentEditor = currentNoteEditorTab();
    if (!currentEditor || (currentEditor != noteEditor)) {
        QNTRACE(QStringLiteral("Not an update from current note editor, skipping"));
        return;
    }

    onSetStatusBarText(QString());
}

void MainWindow::onAddAccountActionTriggered(bool checked)
{
    QNDEBUG(QStringLiteral("MainWindow::onAddAccountActionTriggered"));

    Q_UNUSED(checked)

    m_pAccountManager->raiseAddAccountDialog();
}

void MainWindow::onManageAccountsActionTriggered(bool checked)
{
    QNDEBUG(QStringLiteral("MainWindow::onManageAccountsActionTriggered"));

    Q_UNUSED(checked)

    m_pAccountManager->raiseManageAccountsDialog();
}

void MainWindow::onSwitchAccountActionToggled(bool checked)
{
    QNDEBUG(QStringLiteral("MainWindow::onSwitchAccountActionToggled: checked = ")
            << (checked ? QStringLiteral("true") : QStringLiteral("false")));

    if (!checked) {
        QNTRACE(QStringLiteral("Ignoring the unchecking of account"));
        return;
    }

    QAction * action = qobject_cast<QAction*>(sender());
    if (Q_UNLIKELY(!action)) {
        NOTIFY_ERROR(QString::fromUtf8(QT_TR_NOOP("Internal error: account switching action is unexpectedly null")));
        return;
    }

    QVariant indexData = action->data();
    bool conversionResult = false;
    int index = indexData.toInt(&conversionResult);
    if (Q_UNLIKELY(!conversionResult)) {
        NOTIFY_ERROR(QString::fromUtf8(QT_TR_NOOP("Internal error: can't get identification data "
                                                  "from the account switching action")));
        return;
    }

    const QVector<Account> & availableAccounts = m_pAccountManager->availableAccounts();
    const int numAvailableAccounts = availableAccounts.size();

    if ((index < 0) || (index >= numAvailableAccounts)) {
        NOTIFY_ERROR(QString::fromUtf8(QT_TR_NOOP("Internal error: wrong index into available accounts "
                                                  "in account switching action")));
        return;
    }

    const Account & availableAccount = availableAccounts[index];

    stopListeningForSplitterMoves();
    m_pAccountManager->switchAccount(availableAccount);
    // Will continue in slot connected to AccountManager's switchedAccount signal
}

void MainWindow::onAccountSwitched(Account account)
{
    QNDEBUG(QStringLiteral("MainWindow::onAccountSwitched: ") << account);

    if (Q_UNLIKELY(!m_pLocalStorageManagerThread)) {
        ErrorString errorDescription(QT_TR_NOOP("internal error: no local storage manager thread exists"));
        QNWARNING(errorDescription);
        onSetStatusBarText(tr("Could not switch account: ") + QStringLiteral(": ") + errorDescription.localizedString(), SEC_TO_MSEC(30));
        return;
    }

    if (Q_UNLIKELY(!m_pLocalStorageManagerAsync)) {
        ErrorString errorDescription(QT_TR_NOOP("internal error: no local storage manager exists"));
        QNWARNING(errorDescription);
        onSetStatusBarText(tr("Could not switch account: ") + QStringLiteral(": ") + errorDescription.localizedString(), SEC_TO_MSEC(30));
        return;
    }

    // Since Qt 5.11 QSqlDatabase opening only works properly from the thread which has loaded the SQL drivers -
    // which is this thread, the GUI one. However, LocalStorageManagerAsync operates in another thread. So need to stop
    // that thread, perform the operation synchronously and then start the stopped thread again

    if (m_pLocalStorageManagerThread->isRunning()) {
        QObject::disconnect(m_pLocalStorageManagerThread, QNSIGNAL(QThread,finished),
                            m_pLocalStorageManagerThread, QNSLOT(QThread,deleteLater));
        m_pLocalStorageManagerThread->quit();
        m_pLocalStorageManagerThread->wait();
    }

    ErrorString errorDescription;
    try {
        m_pLocalStorageManagerAsync->localStorageManager()->switchUser(account);
    }
    catch(const std::exception & e) {
        errorDescription.setBase(QT_TR_NOOP("Can't switch user in the local storage: caught exception"));
        errorDescription.details() = QString::fromUtf8(e.what());
    }

    if (!checkLocalStorageVersion(account)) {
        return;
    }

    QObject::connect(m_pLocalStorageManagerThread, QNSIGNAL(QThread,finished),
                     m_pLocalStorageManagerThread, QNSLOT(QThread,deleteLater));
    m_pLocalStorageManagerThread->start();

    m_lastLocalStorageSwitchUserRequest = QUuid::createUuid();
    if (errorDescription.isEmpty()) {
        onLocalStorageSwitchUserRequestComplete(account, m_lastLocalStorageSwitchUserRequest);
    }
    else {
        onLocalStorageSwitchUserRequestFailed(account, errorDescription, m_lastLocalStorageSwitchUserRequest);
    }
}

void MainWindow::onAccountUpdated(Account account)
{
    QNDEBUG(QStringLiteral("MainWindow::onAccountUpdated: ") << account);

    if (!m_pAccount) {
        QNDEBUG(QStringLiteral("No account is current at the moment"));
        return;
    }

    if (m_pAccount->type() != account.type()) {
        QNDEBUG(QStringLiteral("Not an update for the current account: it has another type"));
        QNTRACE(*m_pAccount);
        return;
    }

    bool isLocal = (m_pAccount->type() == Account::Type::Local);

    if (isLocal && (m_pAccount->name() != account.name())) {
        QNDEBUG(QStringLiteral("Not an update for the current account: it has another name"));
        QNTRACE(*m_pAccount);
        return;
    }

    if (!isLocal &&
        ((m_pAccount->id() != account.id()) || (m_pAccount->name() != account.name())))
    {
        QNDEBUG(QStringLiteral("Not an update for the current account: either id or name don't match"));
        QNTRACE(*m_pAccount);
        return;
    }

    *m_pAccount = account;
    setWindowTitleForAccount(account);
}

void MainWindow::onAccountAdded(Account account)
{
    QNDEBUG(QStringLiteral("MainWindow::onAccountAdded: ") << account);
    updateSubMenuWithAvailableAccounts();
}

void MainWindow::onAccountManagerError(ErrorString errorDescription)
{
    QNDEBUG(QStringLiteral("MainWindow::onAccountManagerError: ") << errorDescription);
    onSetStatusBarText(errorDescription.localizedString(), SEC_TO_MSEC(30));
}

void MainWindow::onShowSidePanelActionToggled(bool checked)
{
    QNDEBUG(QStringLiteral("MainWindow::onShowSidePanelActionToggled: checked = ")
            << (checked ? QStringLiteral("true") : QStringLiteral("false")));

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
    QNDEBUG(QStringLiteral("MainWindow::onShowFavoritesActionToggled: checked = ")
            << (checked ? QStringLiteral("true") : QStringLiteral("false")));

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
    QNDEBUG(QStringLiteral("MainWindow::onShowNotebooksActionToggled: checked = ")
            << (checked ? QStringLiteral("true") : QStringLiteral("false")));

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
    QNDEBUG(QStringLiteral("MainWindow::onShowTagsActionToggled: checked = ")
            << (checked ? QStringLiteral("true") : QStringLiteral("false")));

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
    QNDEBUG(QStringLiteral("MainWindow::onShowSavedSearchesActionToggled: checked = ")
            << (checked ? QStringLiteral("true") : QStringLiteral("false")));

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
    QNDEBUG(QStringLiteral("MainWindow::onShowDeletedNotesActionToggled: checked = ")
            << (checked ? QStringLiteral("true") : QStringLiteral("false")));

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
    QNDEBUG(QStringLiteral("MainWindow::onShowNoteListActionToggled: checked = ")
            << (checked ? QStringLiteral("true") : QStringLiteral("false")));

    ApplicationSettings appSettings(*m_pAccount, QUENTIER_UI_SETTINGS);
    appSettings.beginGroup(QStringLiteral("MainWindow"));
    appSettings.setValue(QStringLiteral("ShowNotesList"), checked);
    appSettings.endGroup();

    if (checked) {
        m_pUI->noteListView->setModel(m_pNoteFilterModel);
        m_pUI->notesListAndFiltersFrame->show();
    }
    else {
        m_pUI->notesListAndFiltersFrame->hide();
        m_pUI->noteListView->setModel(&m_blankModel);
    }
}

void MainWindow::onShowToolbarActionToggled(bool checked)
{
    QNDEBUG(QStringLiteral("MainWindow::onShowToolbarActionToggled: checked = ")
            << (checked ? QStringLiteral("true") : QStringLiteral("false")));

    ApplicationSettings appSettings(*m_pAccount, QUENTIER_UI_SETTINGS);
    appSettings.beginGroup(QStringLiteral("MainWindow"));
    appSettings.setValue(QStringLiteral("ShowToolbar"), checked);
    appSettings.endGroup();

    if (checked) {
        m_pUI->upperBarPanel->show();
    }
    else {
        m_pUI->upperBarPanel->hide();
    }
}

void MainWindow::onShowStatusBarActionToggled(bool checked)
{
    QNDEBUG(QStringLiteral("MainWindow::onShowStatusBarActionToggled: checked = ")
            << (checked ? QStringLiteral("true") : QStringLiteral("false")));

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

void MainWindow::onSwitchIconsToNativeAction()
{
    QNDEBUG(QStringLiteral("MainWindow::onSwitchIconsToNativeAction"));

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

void MainWindow::onSwitchIconsToTangoAction()
{
    QNDEBUG(QStringLiteral("MainWindow::onSwitchIconsToTangoAction"));

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

void MainWindow::onSwitchIconsToOxygenAction()
{
    QNDEBUG(QStringLiteral("MainWindow::onSwitchIconsToOxygenAction"));

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

void MainWindow::onSwitchPanelStyleToBuiltIn()
{
    QNDEBUG(QStringLiteral("MainWindow::onSwitchPanelStyleToBuiltIn"));

    if (m_currentPanelStyle.isEmpty()) {
        ErrorString error(QT_TR_NOOP("Already using the built-in panel style"));
        QNDEBUG(error);
        onSetStatusBarText(error.localizedString(), SEC_TO_MSEC(10));
        return;
    }

    m_currentPanelStyle.clear();

    ApplicationSettings appSettings(*m_pAccount, QUENTIER_UI_SETTINGS);
    appSettings.beginGroup(LOOK_AND_FEEL_SETTINGS_GROUP_NAME);
    appSettings.remove(PANELS_STYLE_SETTINGS_KEY);
    appSettings.endGroup();

    setPanelsOverlayStyleSheet(StyleSheetProperties());
}

void MainWindow::onSwitchPanelStyleToLighter()
{
    QNDEBUG(QStringLiteral("MainWindow::onSwitchPanelStyleToLighter"));

    if (m_currentPanelStyle == LIGHTER_PANEL_STYLE_NAME) {
        ErrorString error(QT_TR_NOOP("Already using the lighter panel style"));
        QNDEBUG(error);
        onSetStatusBarText(error.localizedString(), SEC_TO_MSEC(10));
        return;
    }

    m_currentPanelStyle = LIGHTER_PANEL_STYLE_NAME;

    ApplicationSettings appSettings(*m_pAccount, QUENTIER_UI_SETTINGS);
    appSettings.beginGroup(LOOK_AND_FEEL_SETTINGS_GROUP_NAME);
    appSettings.setValue(PANELS_STYLE_SETTINGS_KEY, m_currentPanelStyle);
    appSettings.endGroup();

    StyleSheetProperties properties;
    getPanelStyleSheetProperties(m_currentPanelStyle, properties);
    setPanelsOverlayStyleSheet(properties);
}

void MainWindow::onSwitchPanelStyleToDarker()
{
    QNDEBUG(QStringLiteral("MainWindow::onSwitchPanelStyleToDarker"));

    if (m_currentPanelStyle == DARKER_PANEL_STYLE_NAME) {
        ErrorString error(QT_TR_NOOP("Already using the darker panel style"));
        QNDEBUG(error);
        onSetStatusBarText(error.localizedString(), SEC_TO_MSEC(10));
        return;
    }

    m_currentPanelStyle = DARKER_PANEL_STYLE_NAME;

    ApplicationSettings appSettings(*m_pAccount, QUENTIER_UI_SETTINGS);
    appSettings.beginGroup(LOOK_AND_FEEL_SETTINGS_GROUP_NAME);
    appSettings.setValue(PANELS_STYLE_SETTINGS_KEY, m_currentPanelStyle);
    appSettings.endGroup();

    StyleSheetProperties properties;
    getPanelStyleSheetProperties(m_currentPanelStyle, properties);
    setPanelsOverlayStyleSheet(properties);
}

void MainWindow::onLocalStorageSwitchUserRequestComplete(Account account, QUuid requestId)
{
    QNDEBUG(QStringLiteral("MainWindow::onLocalStorageSwitchUserRequestComplete: account = ")
            << account << QStringLiteral(", request id = ") << requestId);

    bool expected = (m_lastLocalStorageSwitchUserRequest == requestId);
    m_lastLocalStorageSwitchUserRequest = QUuid();

    bool wasPendingSwitchToNewEvernoteAccount = m_pendingSwitchToNewEvernoteAccount;
    m_pendingSwitchToNewEvernoteAccount = false;

    if (!expected) {
        NOTIFY_ERROR(QString::fromUtf8(QT_TR_NOOP("Local storage user was switched without explicit user action")));
        // Trying to undo it
        m_pAccountManager->switchAccount(*m_pAccount); // This should trigger the switch in local storage as well
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
    setupUserShortcuts();
    startListeningForShortcutChanges();

    restoreNetworkProxySettingsForAccount(*m_pAccount);

    if (m_pAccount->type() == Account::Type::Local)
    {
        clearSynchronizationManager();
    }
    else
    {
        if (m_synchronizationManagerHost == m_pAccount->evernoteHost())
        {
            if (!m_pSynchronizationManager) {
                setupSynchronizationManager(SetAccountOption::Set);
            }
            else {
                setAccountToSyncManager(*m_pAccount);
            }
        }
        else
        {
            m_synchronizationManagerHost = m_pAccount->evernoteHost();
            setupSynchronizationManager(SetAccountOption::Set);
        }

        setSynchronizationOptions(*m_pAccount);
    }

    setupModels();

    if (m_pNoteEditorTabsAndWindowsCoordinator) {
        m_pNoteEditorTabsAndWindowsCoordinator->switchAccount(*m_pAccount, *m_pTagModel);
    }

    m_pUI->filterByNotebooksWidget->switchAccount(*m_pAccount, m_pNotebookModel);
    m_pUI->filterByTagsWidget->switchAccount(*m_pAccount, m_pTagModel);
    m_pUI->filterBySavedSearchComboBox->switchAccount(*m_pAccount, m_pSavedSearchModel);

    setupViews();
    setupAccountSpecificUiElements();

    // FIXME: this can be done more lightweight: just set the current account in the already filled list
    updateSubMenuWithAvailableAccounts();

    restoreGeometryAndState();
    startListeningForSplitterMoves();

    if (m_pAccount->type() == Account::Type::Evernote)
    {
        // TODO: should also start the sync if the corresponding setting is set
        // to sync stuff when one switches to the Evernote account
        if (wasPendingSwitchToNewEvernoteAccount)
        {
            // For new Evernote account is is convenient if the first note to be synchronized
            // automatically opens in the note editor
            m_pUI->noteListView->setAutoSelectNoteOnNextAddition();

            m_pendingSynchronizationManagerResponseToStartSync = true;
            checkAndLaunchPendingSync();
        }
    }
}

void MainWindow::onLocalStorageSwitchUserRequestFailed(Account account, ErrorString errorDescription, QUuid requestId)
{
    bool expected = (m_lastLocalStorageSwitchUserRequest == requestId);
    if (!expected) {
        return;
    }

    QNDEBUG(QStringLiteral("MainWindow::onLocalStorageSwitchUserRequestFailed: ") << account << QStringLiteral("\nError description: ")
            << errorDescription << QStringLiteral(", request id = ") << requestId);

    m_lastLocalStorageSwitchUserRequest = QUuid();

    onSetStatusBarText(tr("Could not switch account") + QStringLiteral(": ") + errorDescription.localizedString(), SEC_TO_MSEC(30));

    if (!m_pAccount) {
        // If there was no any account set previously, nothing to do
        return;
    }

    restoreNetworkProxySettingsForAccount(*m_pAccount);
    startListeningForSplitterMoves();

    const QVector<Account> & availableAccounts = m_pAccountManager->availableAccounts();
    const int numAvailableAccounts = availableAccounts.size();

    // Trying to restore the previously selected account as the current one in the UI
    QList<QAction*> availableAccountActions = m_pAvailableAccountsActionGroup->actions();
    for(auto it = availableAccountActions.constBegin(), end = availableAccountActions.constEnd(); it != end; ++it)
    {
        QAction * action = *it;
        if (Q_UNLIKELY(!action)) {
            QNDEBUG(QStringLiteral("Found null pointer to action within the available accounts action group"));
            continue;
        }

        QVariant actionData = action->data();
        bool conversionResult = false;
        int index = actionData.toInt(&conversionResult);
        if (Q_UNLIKELY(!conversionResult)) {
            QNDEBUG(QStringLiteral("Can't convert available account's user data to int: ") << actionData);
            continue;
        }

        if (Q_UNLIKELY((index < 0) || (index >= numAvailableAccounts))) {
            QNDEBUG(QStringLiteral("Available account's index is beyond the range of available accounts: index = ")
                    << index << QStringLiteral(", num available accounts = ") << numAvailableAccounts);
            continue;
        }

        const Account & actionAccount = availableAccounts.at(index);
        if (actionAccount == *m_pAccount) {
            QNDEBUG(QStringLiteral("Restoring the current account in UI: index = ") << index << QStringLiteral(", account = ")
                    << actionAccount);
            action->setChecked(true);
            return;
        }
    }

    // If we got here, it means we haven't found the proper previous account
    QNDEBUG(QStringLiteral("Couldn't find the action corresponding to the previous available account: ") << *m_pAccount);
}

void MainWindow::onSplitterHandleMoved(int pos, int index)
{
    QNDEBUG(QStringLiteral("MainWindow::onSplitterHandleMoved: pos = ") << pos
            << QStringLiteral(", index = ") << index);

    scheduleGeometryAndStatePersisting();
}

void MainWindow::onSidePanelSplittedHandleMoved(int pos, int index)
{
    QNDEBUG(QStringLiteral("MainWindow::onSidePanelSplittedHandleMoved: pos = ") << pos
            << QStringLiteral(", index = ") << index);

    scheduleGeometryAndStatePersisting();
}

void MainWindow::onSyncButtonPressed()
{
    QNDEBUG(QStringLiteral("MainWindow::onSyncButtonPressed"));

    if (Q_UNLIKELY(!m_pAccount)) {
        QNDEBUG(QStringLiteral("Ignoring the sync button click - no account is set"));
        return;
    }

    if (Q_UNLIKELY(m_pAccount->type() == Account::Type::Local)) {
        QNDEBUG(QStringLiteral("The current account is of local type, won't do anything on attempt to sync it"));
        return;
    }

    if (m_syncInProgress) {
        QNDEBUG(QStringLiteral("The synchronization is in progress, will stop it"));
        Q_EMIT stopSynchronization();
    }
    else {
        Q_EMIT synchronize();
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
    QNDEBUG(QStringLiteral("MainWindow::onSyncIconAnimationFinished"));

    QObject::disconnect(&m_animatedSyncButtonIcon, QNSIGNAL(QMovie,finished),
                        this, QNSLOT(MainWindow,onSyncIconAnimationFinished));
    QObject::disconnect(&m_animatedSyncButtonIcon, QNSIGNAL(QMovie,frameChanged,int),
                        this, QNSLOT(MainWindow,onAnimatedSyncIconFrameChangedPendingFinish,int));

    stopSyncButtonAnimation();
}

void MainWindow::onSynchronizationManagerSetAccountDone(Account account)
{
    QNDEBUG(QStringLiteral("MainWindow::onSynchronizationManagerSetAccountDone: ") << account);

    QObject::disconnect(m_pSynchronizationManager, QNSIGNAL(SynchronizationManager,setAccountDone,Account),
                        this, QNSLOT(MainWindow,onSynchronizationManagerSetAccountDone,Account));

    m_pendingSynchronizationManagerSetAccount = false;
    checkAndLaunchPendingSync();
}

void MainWindow::onSynchronizationManagerSetDownloadNoteThumbnailsDone(bool flag)
{
    QNDEBUG(QStringLiteral("MainWindow::onSynchronizationManagerSetDownloadNoteThumbnailsDone: ")
            << (flag ? QStringLiteral("true") : QStringLiteral("false")));

    QObject::disconnect(m_pSynchronizationManager, QNSIGNAL(SynchronizationManager,setDownloadNoteThumbnailsDone,bool),
                        this, QNSLOT(MainWindow,onSynchronizationManagerSetDownloadNoteThumbnailsDone,bool));

    m_pendingSynchronizationManagerSetDownloadNoteThumbnailsOption = false;
    checkAndLaunchPendingSync();
}

void MainWindow::onSynchronizationManagerSetDownloadInkNoteImagesDone(bool flag)
{
    QNDEBUG(QStringLiteral("MainWindow::onSynchronizationManagerSetDownloadInkNoteImagesDone: ")
            << (flag ? QStringLiteral("true") : QStringLiteral("false")));

    QObject::disconnect(m_pSynchronizationManager, QNSIGNAL(SynchronizationManager,setDownloadInkNoteImagesDone,bool),
                        this, QNSLOT(MainWindow,onSynchronizationManagerSetDownloadInkNoteImagesDone,bool));

    m_pendingSynchronizationManagerSetDownloadInkNoteImagesOption = false;
    checkAndLaunchPendingSync();
}

void MainWindow::onSynchronizationManagerSetInkNoteImagesStoragePathDone(QString path)
{
    QNDEBUG(QStringLiteral("MainWindow::onSynchronizationManagerSetInkNoteImagesStoragePathDone: ") << path);

    QObject::disconnect(m_pSynchronizationManager, QNSIGNAL(SynchronizationManager,setInkNoteImagesStoragePathDone,QString),
                        this, QNSLOT(MainWindow,onSynchronizationManagerSetInkNoteImagesStoragePathDone,QString));

    m_pendingSynchronizationManagerSetInkNoteImagesStoragePath = false;
    checkAndLaunchPendingSync();
}

void MainWindow::onNewAccountCreationRequested()
{
    QNDEBUG(QStringLiteral("MainWindow::onNewAccountCreationRequested"));
    m_pAccountManager->raiseAddAccountDialog();
}

void MainWindow::onQuitAction()
{
    QNDEBUG(QStringLiteral("MainWindow::onQuitAction"));

    if (m_pNoteEditorTabsAndWindowsCoordinator) {
        // That would save the modified notes
        m_pNoteEditorTabsAndWindowsCoordinator->clear();
    }

    qApp->quit();
}

void MainWindow::onShortcutChanged(int key, QKeySequence shortcut, const Account & account, QString context)
{
    QNDEBUG(QStringLiteral("MainWindow::onShortcutChanged: key = ") << key << QStringLiteral(", shortcut: ")
            << shortcut.toString(QKeySequence::PortableText) << QStringLiteral(", context: ")
            << context << QStringLiteral(", account: ") << account);

    auto it = m_shortcutKeyToAction.find(key);
    if (it == m_shortcutKeyToAction.end()) {
        QNDEBUG(QStringLiteral("Haven't found the action corresponding to the shortcut key"));
        return;
    }

    QAction * pAction = it.value();
    pAction->setShortcut(shortcut);
    QNDEBUG(QStringLiteral("Updated shortcut for action ") << pAction->text()
            << QStringLiteral(" (") << pAction->objectName() << QStringLiteral(")"));
}

void MainWindow::onNonStandardShortcutChanged(QString nonStandardKey, QKeySequence shortcut,
                                              const Account & account, QString context)
{
    QNDEBUG(QStringLiteral("MainWindow::onNonStandardShortcutChanged: non-standard key = ")
            << nonStandardKey << QStringLiteral(", shortcut: ") << shortcut.toString(QKeySequence::PortableText)
            << QStringLiteral(", context: ") << context << QStringLiteral(", account: ") << account);

    auto it = m_nonStandardShortcutKeyToAction.find(nonStandardKey);
    if (it == m_nonStandardShortcutKeyToAction.end()) {
        QNDEBUG(QStringLiteral("Haven't found the action corresponding to the non-standard shortcut key"));
        return;
    }

    QAction * pAction = it.value();
    pAction->setShortcut(shortcut);
    QNDEBUG(QStringLiteral("Updated shortcut for action ") << pAction->text()
            << QStringLiteral(" (") << pAction->objectName() << QStringLiteral(")"));
}

void MainWindow::onDefaultAccountFirstNotebookAndNoteCreatorFinished(QString createdNoteLocalUid)
{
    QNDEBUG(QStringLiteral("MainWindow::onDefaultAccountFirstNotebookAndNoteCreatorFinished: created note local uid = ")
            << createdNoteLocalUid);

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

void MainWindow::onDefaultAccountFirstNotebookAndNoteCreatorError(ErrorString errorDescription)
{
    QNDEBUG(QStringLiteral("MainWindow::onDefaultAccountFirstNotebookAndNoteCreatorError: ") << errorDescription);

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
        // NOTE: without this the splitter seems to take a wrong guess
        // about the size of note filters header panel and that doesn't look good
        adjustNoteListAndFiltersSplitterSizes();
    }

    scheduleGeometryAndStatePersisting();
}

void MainWindow::closeEvent(QCloseEvent * pEvent)
{
    QNDEBUG(QStringLiteral("MainWindow::closeEvent"));

    if (pEvent && m_pSystemTrayIconManager->shouldCloseToSystemTray()) {
        QNDEBUG(QStringLiteral("Hiding to system tray instead of closing"));
        pEvent->ignore();
        hide();
        return;
    }

    if (m_pNoteEditorTabsAndWindowsCoordinator) {
        m_pNoteEditorTabsAndWindowsCoordinator->clear();
    }

    persistGeometryAndState();
    QMainWindow::closeEvent(pEvent);
}

void MainWindow::timerEvent(QTimerEvent * pTimerEvent)
{
    QNDEBUG(QStringLiteral("MainWindow::timerEvent: timer id = ")
            << (pTimerEvent ? QString::number(pTimerEvent->timerId()) : QStringLiteral("<null>")));

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
        killTimer(m_splitterSizesRestorationDelayTimerId);
        m_splitterSizesRestorationDelayTimerId = 0;
    }
    else if (pTimerEvent->timerId() == m_runSyncPeriodicallyTimerId)
    {
        if (Q_UNLIKELY(!m_pAccount ||
                       (m_pAccount->type() != Account::Type::Evernote) ||
                       !m_pSynchronizationManager))
        {
            QNDEBUG(QStringLiteral("Non-Evernote account is being used: ")
                    << (m_pAccount ? m_pAccount->toString() : QStringLiteral("<null>")));
            killTimer(m_runSyncPeriodicallyTimerId);
            m_runSyncPeriodicallyTimerId = 0;
            return;
        }

        if (m_syncInProgress ||
            m_pendingNewEvernoteAccountAuthentication ||
            m_pendingSwitchToNewEvernoteAccount ||
            m_syncApiRateLimitExceeded)
        {
            QNDEBUG(QStringLiteral("Sync in progress = ")
                    << (m_syncInProgress ? QStringLiteral("true") : QStringLiteral("false"))
                    << QStringLiteral(", pending new Evernote account authentication = ")
                    << (m_pendingNewEvernoteAccountAuthentication ? QStringLiteral("true") : QStringLiteral("false"))
                    << QStringLiteral(", pending switch to new Evernote account = ")
                    << (m_pendingSwitchToNewEvernoteAccount ? QStringLiteral("true") : QStringLiteral("false"))
                    << QStringLiteral(", sync API rate limit exceeded = ")
                    << (m_syncApiRateLimitExceeded ? QStringLiteral("true") : QStringLiteral("false")));
            return;
        }

        QNDEBUG(QStringLiteral("Starting the periodically run sync"));
        Q_EMIT synchronize();
    }
    else if (pTimerEvent->timerId() == m_setDefaultAccountsFirstNoteAsCurrentDelayTimerId)
    {
        QNDEBUG(QStringLiteral("Executing postponed setting of defaut account's first note as the current note"));

        m_pUI->noteListView->setCurrentNoteByLocalUid(m_defaultAccountFirstNoteLocalUid);
        m_defaultAccountFirstNoteLocalUid.clear();
        killTimer(m_setDefaultAccountsFirstNoteAsCurrentDelayTimerId);
        m_setDefaultAccountsFirstNoteAsCurrentDelayTimerId = 0;
    }
}

void MainWindow::focusInEvent(QFocusEvent * pFocusEvent)
{
    QNDEBUG(QStringLiteral("MainWindow::focusInEvent"));

    if (Q_UNLIKELY(!pFocusEvent)) {
        return;
    }

    QNDEBUG(QStringLiteral("Reason = ") << pFocusEvent->reason());

    QMainWindow::focusInEvent(pFocusEvent);

    NoteEditorWidget * pCurrentNoteEditorTab = currentNoteEditorTab();
    if (pCurrentNoteEditorTab) {
        pCurrentNoteEditorTab->setFocusToEditor();
    }
}

void MainWindow::focusOutEvent(QFocusEvent * pFocusEvent)
{
    QNDEBUG(QStringLiteral("MainWindow::focusOutEvent"));

    if (Q_UNLIKELY(!pFocusEvent)) {
        return;
    }

    QNDEBUG(QStringLiteral("Reason = ") << pFocusEvent->reason());
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

        QNDEBUG(QStringLiteral("Change event of window state change type: minimized = ")
                << (minimized ? QStringLiteral("true") : QStringLiteral("false")));

        if (!minimized)
        {
            QNDEBUG(QStringLiteral("MainWindow is no longer minimized"));

            if (isVisible()) {
                Q_EMIT shown();
            }

            scheduleGeometryAndStatePersisting();
        }
        else if (minimized)
        {
            QNDEBUG(QStringLiteral("MainWindow became minimized"));

            if (m_pSystemTrayIconManager->shouldMinimizeToSystemTray())
            {
                QNDEBUG(QStringLiteral("Should minimize to system tray instead of the conventional minimization"));

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

void MainWindow::setupThemeIcons()
{
    QNTRACE(QStringLiteral("MainWindow::setupThemeIcons"));

    m_nativeIconThemeName = QIcon::themeName();
    QNDEBUG(QStringLiteral("Native icon theme name: ") << m_nativeIconThemeName);

    if (!QIcon::hasThemeIcon(QStringLiteral("document-open"))) {
        QNDEBUG(QStringLiteral("There seems to be no native icon theme available: "
                               "document-open icon is not present within the theme"));
        m_nativeIconThemeName.clear();
    }

    ApplicationSettings appSettings(*m_pAccount, QUENTIER_UI_SETTINGS);
    appSettings.beginGroup(LOOK_AND_FEEL_SETTINGS_GROUP_NAME);
    QString iconThemeName = appSettings.value(ICON_THEME_SETTINGS_KEY).toString();
    appSettings.endGroup();

    if (!iconThemeName.isEmpty()) {
        QNDEBUG(QStringLiteral("Last chosen icon theme: ") << iconThemeName);
    }
    else {
        QNDEBUG(QStringLiteral("No last chosen icon theme, fallback to oxygen"));
        iconThemeName = QStringLiteral("oxygen");
    }

    QIcon::setThemeName(iconThemeName);
}

void MainWindow::setupAccountManager()
{
    QNDEBUG(QStringLiteral("MainWindow::setupAccountManager"));

    QObject::connect(m_pAccountManager, QNSIGNAL(AccountManager,evernoteAccountAuthenticationRequested,QString,QNetworkProxy),
                     this, QNSLOT(MainWindow,onEvernoteAccountAuthenticationRequested,QString,QNetworkProxy));
    QObject::connect(m_pAccountManager, QNSIGNAL(AccountManager,switchedAccount,Account),
                     this, QNSLOT(MainWindow,onAccountSwitched,Account));
    QObject::connect(m_pAccountManager, QNSIGNAL(AccountManager,accountUpdated,Account),
                     this, QNSLOT(MainWindow,onAccountUpdated,Account));
    QObject::connect(m_pAccountManager, QNSIGNAL(AccountManager,accountAdded,Account),
                     this, QNSLOT(MainWindow,onAccountAdded,Account));
    QObject::connect(m_pAccountManager, QNSIGNAL(AccountManager,notifyError,ErrorString),
                     this, QNSLOT(MainWindow,onAccountManagerError,ErrorString));
}

void MainWindow::setupLocalStorageManager()
{
    QNDEBUG(QStringLiteral("MainWindow::setupLocalStorageManager"));

    m_pLocalStorageManagerThread = new QThread;
    QObject::connect(m_pLocalStorageManagerThread, QNSIGNAL(QThread,finished),
                     m_pLocalStorageManagerThread, QNSLOT(QThread,deleteLater));
    m_pLocalStorageManagerThread->start();

    m_pLocalStorageManagerAsync = new LocalStorageManagerAsync(*m_pAccount, /* start from scratch = */ false,
                                                               /* override lock = */ false);
    m_pLocalStorageManagerAsync->init();
    m_pLocalStorageManagerAsync->moveToThread(m_pLocalStorageManagerThread);

    QObject::connect(this, QNSIGNAL(MainWindow,localStorageSwitchUserRequest,Account,bool,QUuid),
                     m_pLocalStorageManagerAsync, QNSLOT(LocalStorageManagerAsync,onSwitchUserRequest,Account,bool,QUuid));
    QObject::connect(m_pLocalStorageManagerAsync, QNSIGNAL(LocalStorageManagerAsync,switchUserComplete,Account,QUuid),
                     this, QNSLOT(MainWindow,onLocalStorageSwitchUserRequestComplete,Account,QUuid));
    QObject::connect(m_pLocalStorageManagerAsync, QNSIGNAL(LocalStorageManagerAsync,switchUserFailed,Account,ErrorString,QUuid),
                     this, QNSLOT(MainWindow,onLocalStorageSwitchUserRequestFailed,Account,ErrorString,QUuid));
}

void MainWindow::setupDefaultAccount()
{
    QNDEBUG(QStringLiteral("MainWindow::setupDefaultAccount"));

    DefaultAccountFirstNotebookAndNoteCreator * pDefaultAccountFirstNotebookAndNoteCreator =
        new DefaultAccountFirstNotebookAndNoteCreator(*m_pLocalStorageManagerAsync, this);
    QObject::connect(pDefaultAccountFirstNotebookAndNoteCreator, QNSIGNAL(DefaultAccountFirstNotebookAndNoteCreator,finished,QString),
                     this, QNSLOT(MainWindow,onDefaultAccountFirstNotebookAndNoteCreatorFinished,QString));
    QObject::connect(pDefaultAccountFirstNotebookAndNoteCreator, QNSIGNAL(DefaultAccountFirstNotebookAndNoteCreator,notifyError,ErrorString),
                     this, QNSLOT(MainWindow,onDefaultAccountFirstNotebookAndNoteCreatorError,ErrorString));
    pDefaultAccountFirstNotebookAndNoteCreator->start();
}

void MainWindow::setupModels()
{
    QNDEBUG(QStringLiteral("MainWindow::setupModels"));

    clearModels();

    m_pNoteModel = new NoteModel(*m_pAccount, *m_pLocalStorageManagerAsync, m_noteCache,
                                 m_notebookCache, this, NoteModel::IncludedNotes::NonDeleted);
    m_pFavoritesModel = new FavoritesModel(*m_pAccount, *m_pNoteModel, *m_pLocalStorageManagerAsync, m_noteCache,
                                           m_notebookCache, m_tagCache, m_savedSearchCache, this);
    m_pNotebookModel = new NotebookModel(*m_pAccount, *m_pNoteModel, *m_pLocalStorageManagerAsync,
                                         m_notebookCache, this);
    m_pTagModel = new TagModel(*m_pAccount, *m_pNoteModel, *m_pLocalStorageManagerAsync, m_tagCache, this);
    m_pSavedSearchModel = new SavedSearchModel(*m_pAccount, *m_pLocalStorageManagerAsync,
                                               m_savedSearchCache, this);
    m_pDeletedNotesModel = new NoteModel(*m_pAccount, *m_pLocalStorageManagerAsync, m_noteCache,
                                         m_notebookCache, this, NoteModel::IncludedNotes::Deleted);

    m_pNoteFilterModel = new NoteFilterModel(this);
    m_pNoteFilterModel->setSourceModel(m_pNoteModel);

    QObject::connect(m_pNoteModel, QNSIGNAL(NoteModel,notifyAllNotesListed),
                     m_pNoteFilterModel, QNSLOT(NoteFilterModel,invalidate));
    QObject::connect(m_pNoteModel, QNSIGNAL(NoteModel,notifyAllNotesListed),
                     this, QNSLOT(MainWindow,onNoteModelAllNotesListed));

    m_pNoteFiltersManager = new NoteFiltersManager(*m_pUI->filterByTagsWidget,
                                                   *m_pUI->filterByNotebooksWidget,
                                                   *m_pNoteFilterModel,
                                                   *m_pUI->filterBySavedSearchComboBox,
                                                   *m_pUI->searchQueryLineEdit,
                                                   *m_pLocalStorageManagerAsync, this);

    m_pUI->favoritesTableView->setModel(m_pFavoritesModel);
    m_pUI->notebooksTreeView->setModel(m_pNotebookModel);
    m_pUI->tagsTreeView->setModel(m_pTagModel);
    m_pUI->savedSearchesTableView->setModel(m_pSavedSearchModel);
    m_pUI->noteListView->setModel(m_pNoteFilterModel);
    m_pUI->deletedNotesTableView->setModel(m_pDeletedNotesModel);

    m_pNotebookModelColumnChangeRerouter->setModel(m_pNotebookModel);
    m_pTagModelColumnChangeRerouter->setModel(m_pTagModel);
    m_pNoteModelColumnChangeRerouter->setModel(m_pNoteFilterModel);
    m_pFavoritesModelColumnChangeRerouter->setModel(m_pFavoritesModel);

    if (m_pEditNoteDialogsManager) {
        m_pEditNoteDialogsManager->setNotebookModel(m_pNotebookModel);
    }
}

void MainWindow::clearModels()
{
    QNDEBUG(QStringLiteral("MainWindow::clearModels"));

    clearViews();

    if (m_pNotebookModel) {
        delete m_pNotebookModel;
        m_pNotebookModel = Q_NULLPTR;
    }

    if (m_pTagModel) {
        delete m_pTagModel;
        m_pTagModel = Q_NULLPTR;
    }

    if (m_pSavedSearchModel) {
        delete m_pSavedSearchModel;
        m_pSavedSearchModel = Q_NULLPTR;
    }

    if (m_pNoteModel) {
        delete m_pNoteModel;
        m_pNoteModel = Q_NULLPTR;
    }

    if (m_pDeletedNotesModel) {
        delete m_pDeletedNotesModel;
        m_pDeletedNotesModel = Q_NULLPTR;
    }

    if (m_pFavoritesModel) {
        delete m_pFavoritesModel;
        m_pFavoritesModel = Q_NULLPTR;
    }
}

void MainWindow::setupShowHideStartupSettings()
{
    QNDEBUG(QStringLiteral("MainWindow::setupShowHideStartupSettings"));

    ApplicationSettings appSettings(*m_pAccount, QUENTIER_UI_SETTINGS);
    appSettings.beginGroup(QStringLiteral("MainWindow"));

#define CHECK_AND_SET_SHOW_SETTING(name, action, widget) \
    { \
        QVariant showSetting = appSettings.value(name); \
        if (showSetting.isNull()) { \
            showSetting = m_pUI->Action##action->isChecked(); \
        } \
        if (showSetting.toBool()) {\
            m_pUI->widget->show(); \
        } \
        else { \
            m_pUI->widget->hide(); \
        } \
        m_pUI->Action##action->setChecked(showSetting.toBool()); \
    }

    CHECK_AND_SET_SHOW_SETTING(QStringLiteral("ShowSidePanel"), ShowSidePanel, sidePanelSplitter)
    CHECK_AND_SET_SHOW_SETTING(QStringLiteral("ShowFavorites"), ShowFavorites, favoritesWidget)
    CHECK_AND_SET_SHOW_SETTING(QStringLiteral("ShowNotebooks"), ShowNotebooks, notebooksWidget)
    CHECK_AND_SET_SHOW_SETTING(QStringLiteral("ShowTags"), ShowTags, tagsWidget)
    CHECK_AND_SET_SHOW_SETTING(QStringLiteral("ShowSavedSearches"), ShowSavedSearches, savedSearchesWidget)
    CHECK_AND_SET_SHOW_SETTING(QStringLiteral("ShowDeletedNotes"), ShowDeletedNotes, deletedNotesWidget)
    CHECK_AND_SET_SHOW_SETTING(QStringLiteral("ShowNotesList"), ShowNotesList, notesListAndFiltersFrame)
    CHECK_AND_SET_SHOW_SETTING(QStringLiteral("ShowToolbar"), ShowToolbar, upperBarPanel)
    CHECK_AND_SET_SHOW_SETTING(QStringLiteral("ShowStatusBar"), ShowStatusBar, upperBarPanel)

#undef CHECK_AND_SET_SHOW_SETTING

    appSettings.endGroup();
}

void MainWindow::setupViews()
{
    QNDEBUG(QStringLiteral("MainWindow::setupViews"));

    // NOTE: only a few columns would be shown for each view because otherwise there are problems finding space for everything
    // TODO: in future should implement the persistent setting of which columns to show or not to show

    FavoriteItemView * pFavoritesTableView = m_pUI->favoritesTableView;

    QAbstractItemDelegate * pPreviousFavoriteItemDelegate = pFavoritesTableView->itemDelegate();
    FavoriteItemDelegate * pFavoriteItemDelegate = qobject_cast<FavoriteItemDelegate*>(pPreviousFavoriteItemDelegate);
    if (!pFavoriteItemDelegate)
    {
        pFavoriteItemDelegate = new FavoriteItemDelegate(pFavoritesTableView);
        pFavoritesTableView->setItemDelegate(pFavoriteItemDelegate);

        if (pPreviousFavoriteItemDelegate) {
            pPreviousFavoriteItemDelegate->deleteLater();
            pPreviousFavoriteItemDelegate = Q_NULLPTR;
        }
    }

    pFavoritesTableView->setColumnHidden(FavoritesModel::Columns::NumNotesTargeted, true);   // This column's values would be displayed along with the favorite item's name
    pFavoritesTableView->setColumnWidth(FavoritesModel::Columns::Type, pFavoriteItemDelegate->sideSize());

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    QObject::connect(m_pFavoritesModelColumnChangeRerouter, &ColumnChangeRerouter::dataChanged,
                     pFavoritesTableView, &FavoriteItemView::dataChanged, Qt::UniqueConnection);
    pFavoritesTableView->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
#else
    QObject::connect(m_pFavoritesModelColumnChangeRerouter, SIGNAL(dataChanged(QModelIndex,QModelIndex)),
                     pFavoritesTableView, SLOT(dataChanged(QModelIndex,QModelIndex)), Qt::UniqueConnection);
    pFavoritesTableView->header()->setResizeMode(QHeaderView::ResizeToContents);
#endif

    QObject::connect(pFavoritesTableView, QNSIGNAL(FavoriteItemView,notifyError,ErrorString),
                     this, QNSLOT(MainWindow,onModelViewError,ErrorString), Qt::UniqueConnection);
    QObject::connect(pFavoritesTableView, QNSIGNAL(FavoriteItemView,favoritedItemInfoRequested),
                     this, QNSLOT(MainWindow,onFavoritedItemInfoButtonPressed), Qt::UniqueConnection);

    NotebookItemView * pNotebooksTreeView = m_pUI->notebooksTreeView;
    pNotebooksTreeView->setNoteFiltersManager(*m_pNoteFiltersManager);

    QAbstractItemDelegate * pPreviousNotebookItemDelegate = pNotebooksTreeView->itemDelegate();
    NotebookItemDelegate * pNotebookItemDelegate = qobject_cast<NotebookItemDelegate*>(pPreviousNotebookItemDelegate);
    if (!pNotebookItemDelegate)
    {
        pNotebookItemDelegate = new NotebookItemDelegate(pNotebooksTreeView);
        pNotebooksTreeView->setItemDelegate(pNotebookItemDelegate);

        if (pPreviousNotebookItemDelegate) {
            pPreviousNotebookItemDelegate->deleteLater();
            pPreviousNotebookItemDelegate = Q_NULLPTR;
        }
    }

    pNotebooksTreeView->setColumnHidden(NotebookModel::Columns::NumNotesPerNotebook, true);    // This column's values would be displayed along with the notebook's name
    pNotebooksTreeView->setColumnHidden(NotebookModel::Columns::Synchronizable, true);
    pNotebooksTreeView->setColumnHidden(NotebookModel::Columns::LastUsed, true);
    pNotebooksTreeView->setColumnHidden(NotebookModel::Columns::FromLinkedNotebook, true);

    QAbstractItemDelegate * pPreviousNotebookDirtyColumnDelegate = pNotebooksTreeView->itemDelegateForColumn(NotebookModel::Columns::Dirty);
    DirtyColumnDelegate * pNotebookDirtyColumnDelegate = qobject_cast<DirtyColumnDelegate*>(pPreviousNotebookDirtyColumnDelegate);
    if (!pNotebookDirtyColumnDelegate)
    {
        pNotebookDirtyColumnDelegate = new DirtyColumnDelegate(pNotebooksTreeView);
        pNotebooksTreeView->setItemDelegateForColumn(NotebookModel::Columns::Dirty,
                                                     pNotebookDirtyColumnDelegate);

        if (pPreviousNotebookDirtyColumnDelegate) {
            pPreviousNotebookDirtyColumnDelegate->deleteLater();
            pPreviousNotebookDirtyColumnDelegate = Q_NULLPTR;
        }
    }

    pNotebooksTreeView->setColumnWidth(NotebookModel::Columns::Dirty,
                                       pNotebookDirtyColumnDelegate->sideSize());
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    pNotebooksTreeView->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    QObject::connect(m_pNotebookModelColumnChangeRerouter, QNSIGNAL(ColumnChangeRerouter,dataChanged,const QModelIndex&,const QModelIndex&,const QVector<int>&),
                     pNotebooksTreeView, QNSLOT(NotebookItemView,dataChanged,const QModelIndex&,const QModelIndex&,const QVector<int>&),
                     Qt::UniqueConnection);
#else
    pNotebooksTreeView->header()->setResizeMode(QHeaderView::ResizeToContents);
    QObject::connect(m_pNotebookModelColumnChangeRerouter, QNSIGNAL(ColumnChangeRerouter,dataChanged,const QModelIndex&,const QModelIndex&),
                     pNotebooksTreeView, QNSLOT(NotebookItemView,dataChanged,const QModelIndex&,const QModelIndex&),
                     Qt::UniqueConnection);
#endif

    QObject::connect(pNotebooksTreeView, QNSIGNAL(NotebookItemView,newNotebookCreationRequested),
                     this, QNSLOT(MainWindow,onNewNotebookCreationRequested),
                     Qt::UniqueConnection);
    QObject::connect(pNotebooksTreeView, QNSIGNAL(NotebookItemView,notebookInfoRequested),
                     this, QNSLOT(MainWindow,onNotebookInfoButtonPressed),
                     Qt::UniqueConnection);
    QObject::connect(pNotebooksTreeView, QNSIGNAL(NotebookItemView,notifyError,ErrorString),
                     this, QNSLOT(MainWindow,onModelViewError,ErrorString),
                     Qt::UniqueConnection);

    TagItemView * pTagsTreeView = m_pUI->tagsTreeView;
    pTagsTreeView->setColumnHidden(TagModel::Columns::NumNotesPerTag, true); // This column's values would be displayed along with the notebook's name
    pTagsTreeView->setColumnHidden(TagModel::Columns::Synchronizable, true);
    pTagsTreeView->setColumnHidden(TagModel::Columns::FromLinkedNotebook, true);

    QAbstractItemDelegate * pPreviousTagDirtyColumnDelegate = pTagsTreeView->itemDelegateForColumn(TagModel::Columns::Dirty);
    DirtyColumnDelegate * pTagDirtyColumnDelegate = qobject_cast<DirtyColumnDelegate*>(pPreviousTagDirtyColumnDelegate);
    if (!pTagDirtyColumnDelegate)
    {
        pTagDirtyColumnDelegate = new DirtyColumnDelegate(pTagsTreeView);
        pTagsTreeView->setItemDelegateForColumn(TagModel::Columns::Dirty,
                                                pTagDirtyColumnDelegate);

        if (pPreviousTagDirtyColumnDelegate) {
            pPreviousTagDirtyColumnDelegate->deleteLater();
            pPreviousTagDirtyColumnDelegate = Q_NULLPTR;
        }
    }

    pTagsTreeView->setColumnWidth(TagModel::Columns::Dirty,
                                  pTagDirtyColumnDelegate->sideSize());

    QAbstractItemDelegate * pPreviousTagItemDelegate = pTagsTreeView->itemDelegateForColumn(TagModel::Columns::Name);
    TagItemDelegate * pTagItemDelegate = qobject_cast<TagItemDelegate*>(pPreviousTagItemDelegate);
    if (!pTagItemDelegate)
    {
        pTagItemDelegate = new TagItemDelegate(pTagsTreeView);
        pTagsTreeView->setItemDelegateForColumn(TagModel::Columns::Name, pTagItemDelegate);

        if (pPreviousTagItemDelegate) {
            pPreviousTagItemDelegate->deleteLater();
            pPreviousTagItemDelegate = Q_NULLPTR;
        }
    }

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    pTagsTreeView->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    QObject::connect(m_pTagModelColumnChangeRerouter, QNSIGNAL(ColumnChangeRerouter,dataChanged,const QModelIndex&,const QModelIndex&,const QVector<int>&),
                     pTagsTreeView, QNSLOT(TagItemView,dataChanged,const QModelIndex&,const QModelIndex&,const QVector<int>&),
                     Qt::UniqueConnection);
#else
    pTagsTreeView->header()->setResizeMode(QHeaderView::ResizeToContents);
    QObject::connect(m_pTagModelColumnChangeRerouter, QNSIGNAL(ColumnChangeRerouter,dataChanged,const QModelIndex&,const QModelIndex&),
                     pTagsTreeView, QNSLOT(TagItemView,dataChanged,const QModelIndex&,const QModelIndex&),
                     Qt::UniqueConnection);
#endif

    QObject::connect(pTagsTreeView, QNSIGNAL(TagItemView,newTagCreationRequested),
                     this, QNSLOT(MainWindow,onNewTagCreationRequested), Qt::UniqueConnection);
    QObject::connect(pTagsTreeView, QNSIGNAL(TagItemView,tagInfoRequested),
                     this, QNSLOT(MainWindow,onTagInfoButtonPressed), Qt::UniqueConnection);
    QObject::connect(pTagsTreeView, QNSIGNAL(TagItemView,notifyError,ErrorString),
                     this, QNSLOT(MainWindow,onModelViewError,ErrorString), Qt::UniqueConnection);

    SavedSearchItemView * pSavedSearchesTableView = m_pUI->savedSearchesTableView;
    pSavedSearchesTableView->setColumnHidden(SavedSearchModel::Columns::Query, true);
    pSavedSearchesTableView->setColumnHidden(SavedSearchModel::Columns::Synchronizable, true);

    QAbstractItemDelegate * pPreviousSavedSearchDirtyColumnDelegate = pSavedSearchesTableView->itemDelegateForColumn(SavedSearchModel::Columns::Dirty);
    DirtyColumnDelegate * pSavedSearchDirtyColumnDelegate = qobject_cast<DirtyColumnDelegate*>(pPreviousSavedSearchDirtyColumnDelegate);
    if (!pSavedSearchDirtyColumnDelegate)
    {
        pSavedSearchDirtyColumnDelegate = new DirtyColumnDelegate(pSavedSearchesTableView);
        pSavedSearchesTableView->setItemDelegateForColumn(SavedSearchModel::Columns::Dirty,
                                                          pSavedSearchDirtyColumnDelegate);

        if (pPreviousSavedSearchDirtyColumnDelegate) {
            pPreviousSavedSearchDirtyColumnDelegate->deleteLater();
            pPreviousSavedSearchDirtyColumnDelegate = Q_NULLPTR;
        }
    }

    pSavedSearchesTableView->setColumnWidth(SavedSearchModel::Columns::Dirty,
                                            pSavedSearchDirtyColumnDelegate->sideSize());
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    pSavedSearchesTableView->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
#else
    pSavedSearchesTableView->header()->setResizeMode(QHeaderView::ResizeToContents);
#endif

    QObject::connect(pSavedSearchesTableView, QNSIGNAL(SavedSearchItemView,savedSearchInfoRequested),
                     this, QNSLOT(MainWindow,onSavedSearchInfoButtonPressed), Qt::UniqueConnection);
    QObject::connect(pSavedSearchesTableView, QNSIGNAL(SavedSearchItemView,newSavedSearchCreationRequested),
                     this, QNSLOT(MainWindow,onNewSavedSearchCreationRequested), Qt::UniqueConnection);
    QObject::connect(pSavedSearchesTableView, QNSIGNAL(SavedSearchItemView,notifyError,ErrorString),
                     this, QNSLOT(MainWindow,onModelViewError,ErrorString), Qt::UniqueConnection);

    NoteListView * pNoteListView = m_pUI->noteListView;
    if (m_pAccount) {
        pNoteListView->setCurrentAccount(*m_pAccount);
    }

    QAbstractItemDelegate * pPreviousNoteItemDelegate = pNoteListView->itemDelegate();
    NoteItemDelegate * pNoteItemDelegate = qobject_cast<NoteItemDelegate*>(pPreviousNoteItemDelegate);
    if (!pNoteItemDelegate)
    {
        pNoteItemDelegate = new NoteItemDelegate(pNoteListView);

        if (m_pAccount)
        {
            ApplicationSettings appSettings(*m_pAccount, QUENTIER_UI_SETTINGS);
            appSettings.beginGroup(LOOK_AND_FEEL_SETTINGS_GROUP_NAME);
            if (appSettings.contains(SHOW_NOTE_THUMBNAILS_SETTINGS_KEY)) {
                bool showNoteThumbnails = appSettings.value(SHOW_NOTE_THUMBNAILS_SETTINGS_KEY).toBool();
                pNoteItemDelegate->setShowNoteThumbnails(showNoteThumbnails);
            }
            appSettings.endGroup();
        }

        pNoteListView->setModelColumn(NoteModel::Columns::Title);
        pNoteListView->setItemDelegate(pNoteItemDelegate);

        if (pPreviousNoteItemDelegate) {
            pPreviousNoteItemDelegate->deleteLater();
            pPreviousNoteItemDelegate = Q_NULLPTR;
        }
    }

    pNoteListView->setNotebookItemView(pNotebooksTreeView);
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    QObject::connect(m_pNoteModelColumnChangeRerouter, &ColumnChangeRerouter::dataChanged,
                     pNoteListView, &NoteListView::dataChanged, Qt::UniqueConnection);
#else
    QObject::connect(m_pNoteModelColumnChangeRerouter, SIGNAL(dataChanged(QModelIndex,QModelIndex)),
                     pNoteListView, SLOT(dataChanged(QModelIndex,QModelIndex)), Qt::UniqueConnection);
#endif
    QObject::connect(pNoteListView, QNSIGNAL(NoteListView,currentNoteChanged,QString),
                     this, QNSLOT(MainWindow,onCurrentNoteInListChanged,QString), Qt::UniqueConnection);
    QObject::connect(pNoteListView, QNSIGNAL(NoteListView,openNoteInSeparateWindowRequested,QString),
                     this, QNSLOT(MainWindow,onOpenNoteInSeparateWindow,QString), Qt::UniqueConnection);
    QObject::connect(pNoteListView, QNSIGNAL(NoteListView,enexExportRequested,QStringList),
                     this, QNSLOT(MainWindow,onExportNotesToEnexRequested,QStringList), Qt::UniqueConnection);
    QObject::connect(pNoteListView, QNSIGNAL(NoteListView,newNoteCreationRequested),
                     this, QNSLOT(MainWindow,onNewNoteCreationRequested), Qt::UniqueConnection);
    QObject::connect(pNoteListView, QNSIGNAL(NoteListView,copyInAppNoteLinkRequested,QString,QString),
                     this, QNSLOT(MainWindow,onCopyInAppLinkNoteRequested,QString,QString), Qt::UniqueConnection);

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

    if (!restoreNoteSortingMode()) {
        m_pUI->noteSortingModeComboBox->setCurrentIndex(NoteSortingModes::ModifiedDescending);
        QNDEBUG(QStringLiteral("Couldn't restore the note sorting mode, fallback to the default one of ")
                << m_pUI->noteSortingModeComboBox->currentIndex());
    }

    QObject::connect(m_pUI->noteSortingModeComboBox, SIGNAL(currentIndexChanged(int)),
                     this, SLOT(onNoteSortingModeChanged(int)), Qt::UniqueConnection);
    onNoteSortingModeChanged(m_pUI->noteSortingModeComboBox->currentIndex());

    DeletedNoteItemView * pDeletedNotesTableView = m_pUI->deletedNotesTableView;
    pDeletedNotesTableView->setColumnHidden(NoteModel::Columns::CreationTimestamp, true);
    pDeletedNotesTableView->setColumnHidden(NoteModel::Columns::ModificationTimestamp, true);
    pDeletedNotesTableView->setColumnHidden(NoteModel::Columns::PreviewText, true);
    pDeletedNotesTableView->setColumnHidden(NoteModel::Columns::ThumbnailImage, true);
    pDeletedNotesTableView->setColumnHidden(NoteModel::Columns::TagNameList, true);
    pDeletedNotesTableView->setColumnHidden(NoteModel::Columns::Size, true);
    pDeletedNotesTableView->setColumnHidden(NoteModel::Columns::Synchronizable, true);
    pDeletedNotesTableView->setColumnHidden(NoteModel::Columns::NotebookName, true);

    QAbstractItemDelegate * pPreviousDeletedNoteItemDelegate = pDeletedNotesTableView->itemDelegate();
    DeletedNoteItemDelegate * pDeletedNoteItemDelegate = qobject_cast<DeletedNoteItemDelegate*>(pPreviousDeletedNoteItemDelegate);
    if (!pDeletedNoteItemDelegate)
    {
        pDeletedNoteItemDelegate = new DeletedNoteItemDelegate(pDeletedNotesTableView);
        pDeletedNotesTableView->setItemDelegate(pDeletedNoteItemDelegate);

        if (pPreviousDeletedNoteItemDelegate) {
            pPreviousDeletedNoteItemDelegate->deleteLater();
            pPreviousDeletedNoteItemDelegate = Q_NULLPTR;
        }
    }

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    pDeletedNotesTableView->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
#else
    pDeletedNotesTableView->header()->setResizeMode(QHeaderView::ResizeToContents);
#endif

    if (!m_pEditNoteDialogsManager)
    {
        m_pEditNoteDialogsManager = new EditNoteDialogsManager(*m_pLocalStorageManagerAsync, m_noteCache, m_pNotebookModel, this);
        QObject::connect(pNoteListView, QNSIGNAL(NoteListView,editNoteDialogRequested,QString),
                         m_pEditNoteDialogsManager, QNSLOT(EditNoteDialogsManager,onEditNoteDialogRequested,QString),
                         Qt::UniqueConnection);
        QObject::connect(pNoteListView, QNSIGNAL(NoteListView,noteInfoDialogRequested,QString),
                         m_pEditNoteDialogsManager, QNSLOT(EditNoteDialogsManager,onNoteInfoDialogRequested,QString),
                         Qt::UniqueConnection);
        QObject::connect(this, QNSIGNAL(MainWindow,noteInfoDialogRequested,QString),
                         m_pEditNoteDialogsManager, QNSLOT(EditNoteDialogsManager,onNoteInfoDialogRequested,QString),
                         Qt::UniqueConnection);
        QObject::connect(pDeletedNotesTableView, QNSIGNAL(DeletedNoteItemView,deletedNoteInfoRequested,QString),
                         m_pEditNoteDialogsManager, QNSLOT(EditNoteDialogsManager,onNoteInfoDialogRequested,QString),
                         Qt::UniqueConnection);
    }

    Account::Type::type currentAccountType = Account::Type::Local;
    if (m_pAccount) {
        currentAccountType = m_pAccount->type();
    }

    showHideViewColumnsForAccountType(currentAccountType);
}

void MainWindow::clearViews()
{
    QNDEBUG(QStringLiteral("MainWindow::clearViews"));

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
    QNDEBUG(QStringLiteral("MainWindow::setupAccountSpecificUiElements"));

    if (Q_UNLIKELY(!m_pAccount)) {
        QNDEBUG(QStringLiteral("No account"));
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
}

void MainWindow::setupNoteFilters()
{
    QNDEBUG(QStringLiteral("MainWindow::setupNoteFilters"));

    m_pUI->filterFrameBottomBoundary->hide();

    m_pUI->filterByNotebooksWidget->setLocalStorageManager(*m_pLocalStorageManagerAsync);
    m_pUI->filterByTagsWidget->setLocalStorageManager(*m_pLocalStorageManagerAsync);

    m_pUI->filterByNotebooksWidget->switchAccount(*m_pAccount, m_pNotebookModel);
    m_pUI->filterByTagsWidget->switchAccount(*m_pAccount, m_pTagModel);
    m_pUI->filterBySavedSearchComboBox->switchAccount(*m_pAccount, m_pSavedSearchModel);

    m_pUI->filterStatusBarLabel->hide();

    // TODO: set up the object which would watch for changes in these widgets and
    // set stuff to the NoteFilterModel accordingly

    ApplicationSettings appSettings(*m_pAccount, QUENTIER_UI_SETTINGS);
    appSettings.beginGroup(QStringLiteral("FiltersView"));
    m_filtersViewExpanded = appSettings.value(FILTERS_VIEW_STATUS_KEY).toBool();
    appSettings.endGroup();

    // NOTE: won't apply this setting immediately: if m_filtersViewExpanded is true,
    // the default settings from the loaded UI are just what's needed. Otherwise
    // the necessary size adjustments would be done in MainWindow::show() method.
    // Why so? Because it seems trying to adjust the sizes of QSplitter before
    // the widget is shown just silently fail for unknown reason.
}

void MainWindow::setupNoteEditorTabWidgetsCoordinator()
{
    QNDEBUG(QStringLiteral("MainWindow::setupNoteEditorTabWidgetsCoordinator"));

    delete m_pNoteEditorTabsAndWindowsCoordinator;
    m_pNoteEditorTabsAndWindowsCoordinator = new NoteEditorTabsAndWindowsCoordinator(*m_pAccount, *m_pLocalStorageManagerAsync,
                                                                                     m_noteCache, m_notebookCache,
                                                                                     m_tagCache, *m_pTagModel,
                                                                                     m_pUI->noteEditorsTabWidget, this);
    QObject::connect(m_pNoteEditorTabsAndWindowsCoordinator, QNSIGNAL(NoteEditorTabsAndWindowsCoordinator,notifyError,ErrorString),
                     this, QNSLOT(MainWindow,onNoteEditorError,ErrorString));
    QObject::connect(m_pNoteEditorTabsAndWindowsCoordinator, QNSIGNAL(NoteEditorTabsAndWindowsCoordinator,currentNoteChanged,QString),
                     m_pUI->noteListView, QNSLOT(NoteListView,setCurrentNoteByLocalUid,QString));
}

bool MainWindow::checkLocalStorageVersion(const Account & account)
{
    QNDEBUG(QStringLiteral("MainWindow::checkLocalStorageVersion: account = ") << account);

    ErrorString versionErrorDescription;
    if (m_pLocalStorageManagerAsync->localStorageManager()->isLocalStorageVersionTooHigh(versionErrorDescription))
    {
        QNINFO(QStringLiteral("Detected too high local storage version: ") << versionErrorDescription
               << QStringLiteral("; account: ") << account);

        QScopedPointer<LocalStorageVersionTooHighDialog> pVersionTooHighDialog(new LocalStorageVersionTooHighDialog(account,
                                                                                                                    m_pAccountManager->accountModel(),
                                                                                                                    *(m_pLocalStorageManagerAsync->localStorageManager()),
                                                                                                                    this));

        QObject::connect(pVersionTooHighDialog.data(), QNSIGNAL(LocalStorageVersionTooHighDialog,shouldSwitchToAccount,Account),
                         this, QNSLOT(MainWindow,onAccountSwitchRequested,Account),
                         Qt::ConnectionType(Qt::UniqueConnection | Qt::QueuedConnection));
        QObject::connect(pVersionTooHighDialog.data(), QNSIGNAL(LocalStorageVersionTooHighDialog,shouldCreateNewAccount),
                         this, QNSLOT(MainWindow,onNewAccountCreationRequested),
                         Qt::ConnectionType(Qt::UniqueConnection | Qt::QueuedConnection));
        QObject::connect(pVersionTooHighDialog.data(), QNSIGNAL(LocalStorageVersionTooHighDialog,shouldQuitApp),
                         this, QNSLOT(MainWindow,onQuitAction),
                         Qt::ConnectionType(Qt::UniqueConnection | Qt::QueuedConnection));
        Q_UNUSED(pVersionTooHighDialog->exec())
        return false;
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
    QNDEBUG(QStringLiteral("MainWindow::setOnceDisplayedGreeterScreen"));

    ApplicationSettings appSettings;
    appSettings.beginGroup(ACCOUNT_SETTINGS_GROUP);
    appSettings.setValue(ONCE_DISPLAYED_GREETER_SCREEN, true);
    appSettings.endGroup();
}

void MainWindow::setupSynchronizationManager(const SetAccountOption::type setAccountOption)
{
    QNDEBUG(QStringLiteral("MainWindow::setupSynchronizationManager: set account option = ")
            << setAccountOption);

    clearSynchronizationManager();

    if (m_synchronizationManagerHost.isEmpty())
    {
        if (Q_UNLIKELY(!m_pAccount)) {
            ErrorString error(QT_TR_NOOP("Can't set up the synchronization: no account"));
            QNWARNING(error);
            onSetStatusBarText(error.localizedString(), SEC_TO_MSEC(30));
            return;
        }

        if (Q_UNLIKELY(m_pAccount->type() != Account::Type::Evernote)) {
            ErrorString error(QT_TR_NOOP("Can't set up the synchronization: non-Evernote account is chosen"));
            QNWARNING(error << QStringLiteral("; account: ") << *m_pAccount);
            onSetStatusBarText(error.localizedString(), SEC_TO_MSEC(30));
            return;
        }

        m_synchronizationManagerHost = m_pAccount->evernoteHost();
        if (Q_UNLIKELY(m_synchronizationManagerHost.isEmpty())) {
            ErrorString error(QT_TR_NOOP("Can't set up the synchronization: no Evernote host within the account"));
            QNWARNING(error << QStringLiteral("; account: ") << *m_pAccount);
            onSetStatusBarText(error.localizedString(), SEC_TO_MSEC(30));
            return;
        }
    }

    QString consumerKey, consumerSecret;
    setupConsumerKeyAndSecret(consumerKey, consumerSecret);

    if (Q_UNLIKELY(consumerKey.isEmpty())) {
        QNDEBUG(QStringLiteral("Consumer key is empty"));
        return;
    }

    if (Q_UNLIKELY(consumerSecret.isEmpty())) {
        QNDEBUG(QStringLiteral("Consumer secret is empty"));
        return;
    }

    m_pSynchronizationManagerThread = new QThread;
    QObject::connect(m_pSynchronizationManagerThread, QNSIGNAL(QThread,finished),
                     m_pSynchronizationManagerThread, QNSLOT(QThread,deleteLater));
    m_pSynchronizationManagerThread->start();

    m_pAuthenticationManager = new AuthenticationManager(consumerKey, consumerSecret,
                                                         m_synchronizationManagerHost, this);
    m_pSynchronizationManager = new SynchronizationManager(consumerKey, consumerSecret,
                                                           m_synchronizationManagerHost,
                                                           *m_pLocalStorageManagerAsync,
                                                           *m_pAuthenticationManager);

    if (m_pAccount && (setAccountOption == SetAccountOption::Set)) {
        m_pSynchronizationManager->setAccount(*m_pAccount);
    }

    m_pSynchronizationManager->moveToThread(m_pSynchronizationManagerThread);
    connectSynchronizationManager();

    setupRunSyncPeriodicallyTimer();
}

void MainWindow::clearSynchronizationManager()
{
    QNDEBUG(QStringLiteral("MainWindow::clearSynchronizationManager"));

    disconnectSynchronizationManager();

    if (m_pSynchronizationManager) {
        m_pSynchronizationManager->deleteLater();
        m_pSynchronizationManager = Q_NULLPTR;
    }

    if (m_pSynchronizationManagerThread) {
        m_pSynchronizationManagerThread->quit();    // The thread would delete itself after it's finished
        m_pSynchronizationManagerThread = Q_NULLPTR;
    }

    if (m_pAuthenticationManager) {
        m_pAuthenticationManager->deleteLater();
        m_pAuthenticationManager = Q_NULLPTR;
    }

    m_pendingSynchronizationManagerSetAccount = false;
    m_pendingSynchronizationManagerSetDownloadNoteThumbnailsOption = false;
    m_pendingSynchronizationManagerSetDownloadInkNoteImagesOption = false;
    m_pendingSynchronizationManagerSetInkNoteImagesStoragePath = false;
    m_pendingSynchronizationManagerResponseToStartSync = false;

    m_syncApiRateLimitExceeded = false;
    scheduleSyncButtonAnimationStop();

    if (m_runSyncPeriodicallyTimerId != 0) {
        killTimer(m_runSyncPeriodicallyTimerId);
        m_runSyncPeriodicallyTimerId = 0;
    }
}

void MainWindow::setAccountToSyncManager(const Account & account)
{
    QNDEBUG(QStringLiteral("MainWindow::setAccountToSyncManager"));

    QObject::connect(m_pSynchronizationManager, QNSIGNAL(SynchronizationManager,setAccountDone,Account),
                     this, QNSLOT(MainWindow,onSynchronizationManagerSetAccountDone,Account));
    Q_EMIT synchronizationSetAccount(account);
}

void MainWindow::setSynchronizationOptions(const Account & account)
{
    QNDEBUG(QStringLiteral("MainWindow::setSynchronizationOptions"));

    QObject::connect(m_pSynchronizationManager, QNSIGNAL(SynchronizationManager,setDownloadNoteThumbnailsDone,bool),
                     this, QNSLOT(MainWindow,onSynchronizationManagerSetDownloadNoteThumbnailsDone,bool));
    QObject::connect(m_pSynchronizationManager, QNSIGNAL(SynchronizationManager,setDownloadInkNoteImagesDone,bool),
                     this, QNSLOT(MainWindow,onSynchronizationManagerSetDownloadInkNoteImagesDone,bool));
    QObject::connect(m_pSynchronizationManager, QNSIGNAL(SynchronizationManager,setInkNoteImagesStoragePathDone,QString),
                     this, QNSLOT(MainWindow,onSynchronizationManagerSetInkNoteImagesStoragePathDone,QString));

    ApplicationSettings appSettings(account, QUENTIER_SYNC_SETTINGS);
    appSettings.beginGroup(SYNCHRONIZATION_SETTINGS_GROUP_NAME);
    bool downloadNoteThumbnailsOption = (appSettings.contains(SYNCHRONIZATION_DOWNLOAD_NOTE_THUMBNAILS)
                                         ? appSettings.value(SYNCHRONIZATION_DOWNLOAD_NOTE_THUMBNAILS).toBool()
                                         : DEFAULT_DOWNLOAD_NOTE_THUMBNAILS);
    bool downloadInkNoteImagesOption = (appSettings.contains(SYNCHRONIZATION_DOWNLOAD_INK_NOTE_IMAGES)
                                        ? appSettings.value(SYNCHRONIZATION_DOWNLOAD_INK_NOTE_IMAGES).toBool()
                                        : DEFAULT_DOWNLOAD_INK_NOTE_IMAGES);
    appSettings.endGroup();

    m_pendingSynchronizationManagerSetDownloadNoteThumbnailsOption = true;
    Q_EMIT synchronizationDownloadNoteThumbnailsOptionChanged(downloadNoteThumbnailsOption);

    m_pendingSynchronizationManagerSetDownloadInkNoteImagesOption = true;
    Q_EMIT synchronizationDownloadInkNoteImagesOptionChanged(downloadInkNoteImagesOption);

    QString inkNoteImagesStoragePath = accountPersistentStoragePath(account);
    inkNoteImagesStoragePath += QStringLiteral("/NoteEditorPage/inkNoteImages");
    QNTRACE(QStringLiteral("Ink note images storage path: ") << inkNoteImagesStoragePath
            << QStringLiteral("; account: ") << account);

    m_pendingSynchronizationManagerSetInkNoteImagesStoragePath = true;
    Q_EMIT synchronizationSetInkNoteImagesStoragePath(inkNoteImagesStoragePath);
}

void MainWindow::checkAndLaunchPendingSync()
{
    QNDEBUG(QStringLiteral("MainWindow::checkAndLaunchPendingSync"));

    if (!m_pendingSynchronizationManagerResponseToStartSync) {
        QNDEBUG(QStringLiteral("No sync is pending"));
        return;
    }

    if (m_pendingSynchronizationManagerSetAccount) {
        QNDEBUG(QStringLiteral("Pending the response to setAccount from SynchronizationManager"));
        return;
    }

    if (m_pendingSynchronizationManagerSetDownloadNoteThumbnailsOption) {
        QNDEBUG(QStringLiteral("Pending the response to setDownloadNoteThumbnails from SynchronizationManager"));
        return;
    }

    if (m_pendingSynchronizationManagerSetDownloadInkNoteImagesOption) {
        QNDEBUG(QStringLiteral("Pending the response to setDownloadInkNoteImages from SynchronizationManager"));
        return;
    }

    if (m_pendingSynchronizationManagerSetInkNoteImagesStoragePath) {
        QNDEBUG(QStringLiteral("Pending the response to setInkNoteImagesStoragePath from SynchronizationManager"));
        return;
    }

    QNDEBUG(QStringLiteral("Everything is ready, starting the sync"));
    m_pendingSynchronizationManagerResponseToStartSync = false;
    Q_EMIT synchronize();
}

void MainWindow::setupRunSyncPeriodicallyTimer()
{
    QNDEBUG(QStringLiteral("MainWindow::setupRunSyncPeriodicallyTimer"));

    if (Q_UNLIKELY(!m_pAccount)) {
        QNDEBUG(QStringLiteral("No current account"));
        return;
    }

    if (Q_UNLIKELY(m_pAccount->type() != Account::Type::Evernote)) {
        QNDEBUG(QStringLiteral("Non-Evernote account is used"));
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
            QNDEBUG(QStringLiteral("Failed to convert the number of minutes to run sync over to int: ") << data);
            runSyncEachNumMinutes = -1;
        }
    }

    if (runSyncEachNumMinutes < 0) {
        runSyncEachNumMinutes = DEFAULT_RUN_SYNC_EACH_NUM_MINUTES;
    }

    syncSettings.endGroup();

    onRunSyncEachNumMinitesPreferenceChanged(runSyncEachNumMinutes);
}

void MainWindow::setupDefaultShortcuts()
{
    QNDEBUG(QStringLiteral("MainWindow::setupDefaultShortcuts"));

    using quentier::ShortcutManager;

#define PROCESS_ACTION_SHORTCUT(action, key, context) \
    { \
        QString contextStr = QString::fromUtf8(context); \
        ActionKeyWithContext actionData; \
        actionData.m_key = key; \
        actionData.m_context = contextStr; \
        QVariant data; \
        data.setValue(actionData); \
        m_pUI->Action##action->setData(data); \
        QKeySequence shortcut = m_pUI->Action##action->shortcut(); \
        if (shortcut.isEmpty()) { \
            QNTRACE(QStringLiteral("No shortcut was found for action ") << m_pUI->Action##action->objectName()); \
        } \
        else { \
            m_shortcutManager.setDefaultShortcut(key, shortcut, *m_pAccount, contextStr); \
        } \
    }

#define PROCESS_NON_STANDARD_ACTION_SHORTCUT(action, context) \
    { \
        QString actionStr = QString::fromUtf8(#action); \
        QString contextStr = QString::fromUtf8(context); \
        ActionNonStandardKeyWithContext actionData; \
        actionData.m_nonStandardActionKey = actionStr; \
        actionData.m_context = contextStr; \
        QVariant data; \
        data.setValue(actionData); \
        m_pUI->Action##action->setData(data); \
        QKeySequence shortcut = m_pUI->Action##action->shortcut(); \
        if (shortcut.isEmpty()) { \
            QNTRACE(QStringLiteral("No shortcut was found for action ") << m_pUI->Action##action->objectName()); \
        } \
        else { \
            m_shortcutManager.setNonStandardDefaultShortcut(actionStr, shortcut, *m_pAccount, contextStr); \
        } \
    }

#include "ActionShortcuts.inl"

#undef PROCESS_NON_STANDARD_ACTION_SHORTCUT
#undef PROCESS_ACTION_SHORTCUT
}

void MainWindow::setupUserShortcuts()
{
    QNDEBUG(QStringLiteral("MainWindow::setupUserShortcuts"));

#define PROCESS_ACTION_SHORTCUT(action, key, ...) \
    { \
        QKeySequence shortcut = m_shortcutManager.shortcut(key, *m_pAccount, QStringLiteral("" __VA_ARGS__)); \
        if (shortcut.isEmpty()) \
        { \
            QNTRACE(QStringLiteral("No shortcut was found for action ") << m_pUI->Action##action->objectName()); \
            auto it = m_shortcutKeyToAction.find(key); \
            if (it != m_shortcutKeyToAction.end()) { \
                QAction * pAction = it.value(); \
                pAction->setShortcut(QKeySequence()); \
                Q_UNUSED(m_shortcutKeyToAction.erase(it)) \
            } \
        } \
        else \
        { \
            m_pUI->Action##action->setShortcut(shortcut); \
            m_pUI->Action##action->setShortcutContext(Qt::WidgetWithChildrenShortcut); \
            m_shortcutKeyToAction[key] = m_pUI->Action##action; \
        } \
    }

#define PROCESS_NON_STANDARD_ACTION_SHORTCUT(action, ...) \
    { \
        QKeySequence shortcut = m_shortcutManager.shortcut(QStringLiteral(#action), *m_pAccount, QStringLiteral("" __VA_ARGS__)); \
        if (shortcut.isEmpty()) \
        { \
            QNTRACE(QStringLiteral("No shortcut was found for action ") << m_pUI->Action##action->objectName()); \
            auto it = m_nonStandardShortcutKeyToAction.find(QStringLiteral(#action)); \
            if (it != m_nonStandardShortcutKeyToAction.end()) { \
                QAction * pAction = it.value(); \
                pAction->setShortcut(QKeySequence()); \
                Q_UNUSED(m_nonStandardShortcutKeyToAction.erase(it)) \
            } \
        } \
        else \
        { \
            m_pUI->Action##action->setShortcut(shortcut); \
            m_pUI->Action##action->setShortcutContext(Qt::WidgetWithChildrenShortcut); \
            m_nonStandardShortcutKeyToAction[QStringLiteral(#action)] = m_pUI->Action##action; \
        } \
    }

#include "ActionShortcuts.inl"

#undef PROCESS_NON_STANDARD_ACTION_SHORTCUT
#undef PROCESS_ACTION_SHORTCUT
}

void MainWindow::startListeningForShortcutChanges()
{
    QNDEBUG(QStringLiteral("MainWindow::startListeningForShortcutChanges"));

    QObject::connect(&m_shortcutManager, QNSIGNAL(ShortcutManager,shortcutChanged,int,QKeySequence,Account,QString),
                     this, QNSLOT(MainWindow,onShortcutChanged,int,QKeySequence,Account,QString));
    QObject::connect(&m_shortcutManager, QNSIGNAL(ShortcutManager,nonStandardShortcutChanged,QString,QKeySequence,Account,QString),
                     this, QNSLOT(MainWindow,onNonStandardShortcutChanged,QString,QKeySequence,Account,QString));
}

void MainWindow::stopListeningForShortcutChanges()
{
    QNDEBUG(QStringLiteral("MainWindow::stopListeningForShortcutChanges"));

    QObject::disconnect(&m_shortcutManager, QNSIGNAL(ShortcutManager,shortcutChanged,int,QKeySequence,Account,QString),
                        this, QNSLOT(MainWindow,onShortcutChanged,int,QKeySequence,Account,QString));
    QObject::disconnect(&m_shortcutManager, QNSIGNAL(ShortcutManager,nonStandardShortcutChanged,QString,QKeySequence,Account,QString),
                        this, QNSLOT(MainWindow,onNonStandardShortcutChanged,QString,QKeySequence,Account,QString));
}

void MainWindow::setupConsumerKeyAndSecret(QString & consumerKey, QString & consumerSecret)
{
    const char key[10] = "e3zA914Ol";

    QByteArray consumerKeyObf = QByteArray::fromBase64(QStringLiteral("AR8MO2M8Z14WFFcuLzM1Shob").toUtf8());
    char lastChar = 0;
    int size = consumerKeyObf.size();
    for(int i = 0; i < size; ++i) {
        char currentChar = consumerKeyObf[i];
        consumerKeyObf[i] = consumerKeyObf[i] ^ lastChar ^ key[i % 8];
        lastChar = currentChar;
    }

    consumerKey = QString::fromUtf8(consumerKeyObf.constData(), consumerKeyObf.size());

    QByteArray consumerSecretObf = QByteArray::fromBase64(QStringLiteral("BgFLOzJiZh9KSwRyLS8sAg==").toUtf8());
    lastChar = 0;
    size = consumerSecretObf.size();
    for(int i = 0; i < size; ++i) {
        char currentChar = consumerSecretObf[i];
        consumerSecretObf[i] = consumerSecretObf[i] ^ lastChar ^ key[i % 8];
        lastChar = currentChar;
    }

    consumerSecret = QString::fromUtf8(consumerSecretObf.constData(), consumerSecretObf.size());
}

void MainWindow::persistChosenNoteSortingMode(int index)
{
    QNDEBUG(QStringLiteral("MainWindow::persistChosenNoteSortingMode: index = ") << index);

    ApplicationSettings appSettings(*m_pAccount, QUENTIER_UI_SETTINGS);
    appSettings.beginGroup(QStringLiteral("NoteListView"));
    appSettings.setValue(NOTE_SORTING_MODE_KEY, index);
    appSettings.endGroup();
}

bool MainWindow::restoreNoteSortingMode()
{
    QNDEBUG(QStringLiteral("MainWindow::restoreNoteSortingMode"));

    ApplicationSettings appSettings(*m_pAccount, QUENTIER_UI_SETTINGS);
    appSettings.beginGroup(QStringLiteral("NoteListView"));
    if (!appSettings.contains(NOTE_SORTING_MODE_KEY)) {
        QNDEBUG(QStringLiteral("No persisted note sorting mode within the settings, nothing to restore"));
        return false;
    }

    QVariant data = appSettings.value(NOTE_SORTING_MODE_KEY);
    appSettings.endGroup();

    if (data.isNull()) {
        QNDEBUG(QStringLiteral("No persisted note sorting mode, nothing to restore"));
        return false;
    }

    bool conversionResult = false;
    int index = data.toInt(&conversionResult);
    if (Q_UNLIKELY(!conversionResult)) {
        ErrorString error(QT_TR_NOOP("Internal error: can't restore the last used note sorting mode, "
                                     "can't convert the persisted setting to the integer index"));
        QNWARNING(error << QStringLiteral(", persisted data: ") << data);
        onSetStatusBarText(error.localizedString(), SEC_TO_MSEC(30));
        return false;
    }

    m_pUI->noteSortingModeComboBox->setCurrentIndex(index);
    QNDEBUG(QStringLiteral("Restored the current note sorting mode combo box index: ") << index);
    return true;
}

void MainWindow::persistGeometryAndState()
{
    QNDEBUG(QStringLiteral("MainWindow::persistGeometryAndState"));

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
    bool sidePanelSplitterSizesOk = (sidePanelSplitterSizesCount == 5);

    QNTRACE(QStringLiteral("Show side panel = ") << (showSidePanel ? QStringLiteral("true") : QStringLiteral("false"))
            << QStringLiteral(", show favorites view = ") << (showFavoritesView ? QStringLiteral("true") : QStringLiteral("false"))
            << QStringLiteral(", show notebooks view = ") << (showNotebooksView ? QStringLiteral("true") : QStringLiteral("false"))
            << QStringLiteral(", show tags view = ") << (showTagsView ? QStringLiteral("true") : QStringLiteral("false"))
            << QStringLiteral(", show saved searches view = ") << (showSavedSearches ? QStringLiteral("true") : QStringLiteral("false"))
            << QStringLiteral(", show deleted notes view = ") << (showDeletedNotes ? QStringLiteral("true") : QStringLiteral("false"))
            << QStringLiteral(", splitter sizes ok = ") << (showDeletedNotes ? QStringLiteral("true") : QStringLiteral("false"))
            << QStringLiteral(", side panel splitter sizes ok = ") << (showDeletedNotes ? QStringLiteral("true") : QStringLiteral("false")));

    if (QuentierIsLogLevelActive(LogLevel::TraceLevel))
    {
        QNTRACE(QStringLiteral("Splitter sizes: " ));
        for(auto it = splitterSizes.constBegin(), end = splitterSizes.constEnd(); it != end; ++it) {
            QNTRACE(*it);
        }

        QNTRACE(QStringLiteral("Side panel splitter sizes: "));
        for(auto it = sidePanelSplitterSizes.constBegin(), end = sidePanelSplitterSizes.constEnd(); it != end; ++it) {
            QNTRACE(*it);
        }
    }

    if (splitterSizesCountOk && showSidePanel &&
        (showFavoritesView || showNotebooksView || showTagsView || showSavedSearches || showDeletedNotes))
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

    if (sidePanelSplitterSizesOk && showFavoritesView) {
        appSettings.setValue(MAIN_WINDOW_FAVORITES_VIEW_HEIGHT, sidePanelSplitterSizes[0]);
    }
    else {
        appSettings.setValue(MAIN_WINDOW_FAVORITES_VIEW_HEIGHT, QVariant());
    }

    if (sidePanelSplitterSizesOk && showNotebooksView) {
        appSettings.setValue(MAIN_WINDOW_NOTEBOOKS_VIEW_HEIGHT, sidePanelSplitterSizes[1]);
    }
    else {
        appSettings.setValue(MAIN_WINDOW_NOTEBOOKS_VIEW_HEIGHT, QVariant());
    }

    if (sidePanelSplitterSizesOk && showTagsView) {
        appSettings.setValue(MAIN_WINDOW_TAGS_VIEW_HEIGHT, sidePanelSplitterSizes[2]);
    }
    else {
        appSettings.setValue(MAIN_WINDOW_TAGS_VIEW_HEIGHT, QVariant());
    }

    if (sidePanelSplitterSizesOk && showSavedSearches) {
        appSettings.setValue(MAIN_WINDOW_SAVED_SEARCHES_VIEW_HEIGHT, sidePanelSplitterSizes[3]);
    }
    else {
        appSettings.setValue(MAIN_WINDOW_SAVED_SEARCHES_VIEW_HEIGHT, QVariant());
    }

    if (sidePanelSplitterSizesOk && showDeletedNotes) {
        appSettings.setValue(MAIN_WINDOW_DELETED_NOTES_VIEW_HEIGHT, sidePanelSplitterSizes[4]);
    }
    else {
        appSettings.setValue(MAIN_WINDOW_DELETED_NOTES_VIEW_HEIGHT, QVariant());
    }

    appSettings.endGroup();
}

void MainWindow::restoreGeometryAndState()
{
    QNDEBUG(QStringLiteral("MainWindow::restoreGeometryAndState"));

    ApplicationSettings appSettings(*m_pAccount, QUENTIER_UI_SETTINGS);
    appSettings.beginGroup(QStringLiteral("MainWindow"));
    QByteArray savedGeometry = appSettings.value(MAIN_WINDOW_GEOMETRY_KEY).toByteArray();
    QByteArray savedState = appSettings.value(MAIN_WINDOW_STATE_KEY).toByteArray();

    appSettings.endGroup();

    m_geometryRestored = restoreGeometry(savedGeometry);
    m_stateRestored = restoreState(savedState);

    QNTRACE(QStringLiteral("Geometry restored = ") << (m_geometryRestored ? QStringLiteral("true") : QStringLiteral("false"))
            << QStringLiteral(", state restored = )") << (m_stateRestored ? QStringLiteral("true") : QStringLiteral("false")));

    scheduleSplitterSizesRestoration();
}

void MainWindow::restoreSplitterSizes()
{
    QNDEBUG(QStringLiteral("MainWindow::restoreSplitterSizes"));

    ApplicationSettings appSettings(*m_pAccount, QUENTIER_UI_SETTINGS);
    appSettings.beginGroup(QStringLiteral("MainWindow"));

    QVariant sidePanelWidth = appSettings.value(MAIN_WINDOW_SIDE_PANEL_WIDTH_KEY);
    QVariant notesListWidth = appSettings.value(MAIN_WINDOW_NOTE_LIST_WIDTH_KEY);

    QVariant favoritesViewHeight = appSettings.value(MAIN_WINDOW_FAVORITES_VIEW_HEIGHT);
    QVariant notebooksViewHeight = appSettings.value(MAIN_WINDOW_NOTEBOOKS_VIEW_HEIGHT);
    QVariant tagsViewHeight = appSettings.value(MAIN_WINDOW_TAGS_VIEW_HEIGHT);
    QVariant savedSearchesViewHeight = appSettings.value(MAIN_WINDOW_SAVED_SEARCHES_VIEW_HEIGHT);
    QVariant deletedNotesViewHeight = appSettings.value(MAIN_WINDOW_DELETED_NOTES_VIEW_HEIGHT);

    appSettings.endGroup();

    QNTRACE(QStringLiteral("Side panel width = ") << sidePanelWidth << QStringLiteral(", notes list width = ")
            << notesListWidth << QStringLiteral(", favorites view height = ") << favoritesViewHeight
            << QStringLiteral(", notebooks view height = ") << notebooksViewHeight << QStringLiteral(", tags view height = ")
            << tagsViewHeight << QStringLiteral(", saved searches view height = ") << savedSearchesViewHeight
            << QStringLiteral(", deleted notes view height = ") << deletedNotesViewHeight);

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
        int totalWidth = splitterSizes[0] + splitterSizes[1] + splitterSizes[2];

        if (sidePanelWidth.isValid() && showSidePanel &&
            (showFavoritesView || showNotebooksView || showTagsView || showSavedSearches || showDeletedNotes))
        {
            bool conversionResult = false;
            int sidePanelWidthInt = sidePanelWidth.toInt(&conversionResult);
            if (conversionResult) {
                splitterSizes[0] = sidePanelWidthInt;
                QNTRACE(QStringLiteral("Restored side panel width: ") << sidePanelWidthInt);
            }
            else {
                QNDEBUG(QStringLiteral("Can't restore the side panel width: can't convert the persisted width to int: ")
                        << sidePanelWidth);
            }
        }

        if (notesListWidth.isValid() && showNotesList)
        {
            bool conversionResult = false;
            int notesListWidthInt = notesListWidth.toInt(&conversionResult);
            if (conversionResult) {
                splitterSizes[1] = notesListWidthInt;
                QNTRACE(QStringLiteral("Restored notes list panel width: ") << notesListWidthInt);
            }
            else {
                QNDEBUG(QStringLiteral("Can't restore the notes list panel width: can't convert the persisted width to int: ")
                        << notesListWidth);
            }
        }

        splitterSizes[2] = totalWidth - splitterSizes[0] - splitterSizes[1];
        m_pUI->splitter->setSizes(splitterSizes);
        QNTRACE(QStringLiteral("Set splitter sizes"));
    }
    else
    {
        ErrorString error(QT_TR_NOOP("Internal error: can't restore the widths for side panel, note list view "
                                     "and note editors view: wrong number of sizes within the splitter"));
        QNWARNING(error << QStringLiteral(", sizes count: ") << splitterSizesCount);
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
            QNTRACE(QStringLiteral("Side panel splitter sizes before restoring (total")
                    << totalHeight << QStringLiteral(": "));
            for(auto it = sidePanelSplitterSizes.constBegin(), end = sidePanelSplitterSizes.constEnd(); it != end; ++it) {
                QNTRACE(*it);
            }
        }

        if (showFavoritesView && favoritesViewHeight.isValid())
        {
            bool conversionResult = false;
            int favoritesViewHeightInt = favoritesViewHeight.toInt(&conversionResult);
            if (conversionResult) {
                sidePanelSplitterSizes[0] = favoritesViewHeightInt;
                QNTRACE(QStringLiteral("Restored favorites view height: ") << favoritesViewHeightInt);
            }
            else {
                QNDEBUG(QStringLiteral("Can't restore the favorites view height: can't convert the persisted height to int: ")
                        << favoritesViewHeight);
            }
        }

        if (showNotebooksView && notebooksViewHeight.isValid())
        {
            bool conversionResult = false;
            int notebooksViewHeightInt = notebooksViewHeight.toInt(&conversionResult);
            if (conversionResult) {
                sidePanelSplitterSizes[1] = notebooksViewHeightInt;
                QNTRACE(QStringLiteral("Restored notebooks view height: ") << notebooksViewHeightInt);
            }
            else {
                QNDEBUG(QStringLiteral("Can't restore the notebooks view height: can't convert the persisted height to int: ")
                        << notebooksViewHeight);
            }
        }

        if (showTagsView && tagsViewHeight.isValid())
        {
            bool conversionResult = false;
            int tagsViewHeightInt = tagsViewHeight.toInt(&conversionResult);
            if (conversionResult) {
                sidePanelSplitterSizes[2] = tagsViewHeightInt;
                QNTRACE(QStringLiteral("Restored tags view height: ") << tagsViewHeightInt);
            }
            else {
                QNDEBUG(QStringLiteral("Can't restore the tags view height: can't convert the persisted height to int: ")
                        << tagsViewHeight);
            }
        }

        if (showSavedSearches && savedSearchesViewHeight.isValid())
        {
            bool conversionResult = false;
            int savedSearchesViewHeightInt = savedSearchesViewHeight.toInt(&conversionResult);
            if (conversionResult) {
                sidePanelSplitterSizes[3] = savedSearchesViewHeightInt;
                QNTRACE(QStringLiteral("Restored saved searches view height: ") << savedSearchesViewHeightInt);
            }
            else {
                QNDEBUG(QStringLiteral("Can't restore the saved searches view height: can't convert the persisted height to int: ")
                        << savedSearchesViewHeight);
            }
        }

        if (showDeletedNotes && deletedNotesViewHeight.isValid())
        {
            bool conversionResult = false;
            int deletedNotesViewHeightInt = deletedNotesViewHeight.toInt(&conversionResult);
            if (conversionResult) {
                sidePanelSplitterSizes[4] = deletedNotesViewHeightInt;
                QNTRACE(QStringLiteral("Restored deleted notes view height: ") << deletedNotesViewHeightInt);
            }
            else {
                QNDEBUG(QStringLiteral("Can't restore the deleted notes view height: can't convert the persisted height to int: ")
                        << deletedNotesViewHeight);
            }
        }

        int totalHeightAfterRestore = 0;
        for(int i = 0; i < 5; ++i) {
            totalHeightAfterRestore += sidePanelSplitterSizes[i];
        }

        if (QuentierIsLogLevelActive(LogLevel::TraceLevel))
        {
            QNTRACE(QStringLiteral("Side panel splitter sizes after restoring (total")
                    << totalHeightAfterRestore << QStringLiteral(": "));
            for(auto it = sidePanelSplitterSizes.constBegin(), end = sidePanelSplitterSizes.constEnd(); it != end; ++it) {
                QNTRACE(*it);
            }
        }

        m_pUI->sidePanelSplitter->setSizes(sidePanelSplitterSizes);
        QNTRACE(QStringLiteral("Set side panel splitter sizes"));
    }
    else
    {
        ErrorString error(QT_TR_NOOP("Internal error: can't restore the heights of side panel's views: "
                                     "wrong number of sizes within the splitter"));
        QNWARNING(error << QStringLiteral(", sizes count: ") << splitterSizesCount);
        onSetStatusBarText(error.localizedString(), SEC_TO_MSEC(30));
    }
}

void MainWindow::scheduleSplitterSizesRestoration()
{
    QNDEBUG(QStringLiteral("MainWindow::scheduleSplitterSizesRestoration"));

    if (!m_shown) {
        QNDEBUG(QStringLiteral("Not shown yet, won't do anything"));
        return;
    }

    if (m_splitterSizesRestorationDelayTimerId != 0) {
        QNDEBUG(QStringLiteral("Splitter sizes restoration already scheduled, timer id = ")
                << m_splitterSizesRestorationDelayTimerId);
        return;
    }

    m_splitterSizesRestorationDelayTimerId = startTimer(RESTORE_SPLITTER_SIZES_DELAY);
    if (Q_UNLIKELY(m_splitterSizesRestorationDelayTimerId == 0)) {
        QNWARNING(QStringLiteral("Failed to start the timer to delay the restoration of splitter sizes"));
        return;
    }

    QNDEBUG(QStringLiteral("Started the timer to delay the restoration of splitter sizes: ")
            << m_splitterSizesRestorationDelayTimerId);
}

void MainWindow::scheduleGeometryAndStatePersisting()
{
    QNDEBUG(QStringLiteral("MainWindow::scheduleGeometryAndStatePersisting"));

    if (!m_shown) {
        QNDEBUG(QStringLiteral("Not shown yet, won't do anything"));
        return;
    }

    if (m_geometryAndStatePersistingDelayTimerId != 0) {
        QNDEBUG(QStringLiteral("Persisting already scheduled, timer id = ") << m_geometryAndStatePersistingDelayTimerId);
        return;
    }

    m_geometryAndStatePersistingDelayTimerId = startTimer(PERSIST_GEOMETRY_AND_STATE_DELAY);
    if (Q_UNLIKELY(m_geometryAndStatePersistingDelayTimerId == 0)) {
        QNWARNING(QStringLiteral("Failed to start the timer to delay the persistence of MainWindow's state and geometry"));
        return;
    }

    QNDEBUG(QStringLiteral("Started the timer to delay the persistence of MainWindow's state and geometry: timer id = ")
            << m_geometryAndStatePersistingDelayTimerId);
}

template <class T>
void MainWindow::refreshThemeIcons()
{
    QList<T*> objects = findChildren<T*>();
    QNDEBUG(QStringLiteral("Found ") << objects.size() << QStringLiteral(" child objects"));

    for(auto it = objects.constBegin(), end = objects.constEnd(); it != end; ++it)
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
            newIcon.addFile(QStringLiteral("."), QSize(), QIcon::Normal, QIcon::Off);
        }

        object->setIcon(newIcon);
    }
}
