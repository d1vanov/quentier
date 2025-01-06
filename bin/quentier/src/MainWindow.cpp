/*
 * Copyright 2016-2020 Dmitry Ivanov
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

#include <lib/delegate/DeletedNoteItemDelegate.h>
#include <lib/delegate/DirtyColumnDelegate.h>
#include <lib/delegate/FavoriteItemDelegate.h>
#include <lib/delegate/FromLinkedNotebookColumnDelegate.h>
#include <lib/delegate/NoteItemDelegate.h>
#include <lib/delegate/NotebookItemDelegate.h>
#include <lib/delegate/SynchronizableColumnDelegate.h>
#include <lib/delegate/TagItemDelegate.h>
#include <lib/dialog/AddOrEditNotebookDialog.h>
#include <lib/dialog/AddOrEditSavedSearchDialog.h>
#include <lib/dialog/AddOrEditTagDialog.h>
#include <lib/dialog/EditNoteDialog.h>
#include <lib/dialog/EditNoteDialogsManager.h>
#include <lib/dialog/FirstShutdownDialog.h>
#include <lib/dialog/LocalStorageUpgradeDialog.h>
#include <lib/dialog/LocalStorageVersionTooHighDialog.h>
#include <lib/dialog/WelcomeToQuentierDialog.h>
#include <lib/enex/EnexExportDialog.h>
#include <lib/enex/EnexExporter.h>
#include <lib/enex/EnexImportDialog.h>
#include <lib/enex/EnexImporter.h>
#include <lib/exception/LocalStorageVersionTooHighException.h>
#include <lib/initialization/DefaultAccountFirstNotebookAndNoteCreator.h>
#include <lib/model/common/ColumnChangeRerouter.h>
#include <lib/network/NetworkProxySettingsHelpers.h>
#include <lib/preferences/PreferencesDialog.h>
#include <lib/preferences/UpdateSettings.h>
#include <lib/preferences/defaults/Appearance.h>
#include <lib/preferences/defaults/Synchronization.h>
#include <lib/preferences/keys/Account.h>
#include <lib/preferences/keys/Appearance.h>
#include <lib/preferences/keys/Enex.h>
#include <lib/preferences/keys/Files.h>
#include <lib/preferences/keys/NoteEditor.h>
#include <lib/preferences/keys/PanelColors.h>
#include <lib/preferences/keys/Synchronization.h>
#include <lib/tray/SystemTrayIconManager.h>
#include <lib/utility/ActionsInfo.h>
#include <lib/utility/AsyncFileWriter.h>
#include <lib/utility/ExitCodes.h>
#include <lib/utility/Keychain.h>
#include <lib/utility/QObjectThreadMover.h>
#include <lib/view/DeletedNoteItemView.h>
#include <lib/view/FavoriteItemView.h>
#include <lib/view/NoteListView.h>
#include <lib/view/NotebookItemView.h>
#include <lib/view/SavedSearchItemView.h>
#include <lib/view/TagItemView.h>
#include <lib/widget/FindAndReplaceWidget.h>
#include <lib/widget/NoteCountLabelController.h>
#include <lib/widget/NoteFiltersManager.h>
#include <lib/widget/PanelWidget.h>
#include <lib/widget/color-picker-tool-button/ColorPickerToolButton.h>
#include <lib/widget/insert-table-tool-button/InsertTableToolButton.h>
#include <lib/widget/insert-table-tool-button/TableSettingsDialog.h>
using quentier::PanelWidget;

#include <lib/widget/TabWidget.h>
using quentier::TabWidget;

#include <lib/widget/FilterByNotebookWidget.h>
using quentier::FilterByNotebookWidget;

#include <lib/widget/FilterByTagWidget.h>
using quentier::FilterByTagWidget;

#include <lib/widget/FilterBySavedSearchWidget.h>
using quentier::FilterBySavedSearchWidget;

#include <lib/widget/FilterBySearchStringWidget.h>
using quentier::FilterBySearchStringWidget;

#include <lib/widget/LogViewerWidget.h>
using quentier::LogViewerWidget;

#include <lib/widget/AboutQuentierWidget.h>
#include <lib/widget/NotebookModelItemInfoWidget.h>
#include <lib/widget/SavedSearchModelItemInfoWidget.h>
#include <lib/widget/TagModelItemInfoWidget.h>

#include <quentier/note_editor/NoteEditor.h>

#include "ui_MainWindow.h"

#include <quentier/local_storage/NoteSearchQuery.h>
#include <quentier/logging/QuentierLogger.h>
#include <quentier/synchronization/ISyncChunksDataCounters.h>
#include <quentier/types/Note.h>
#include <quentier/types/Notebook.h>
#include <quentier/types/Resource.h>
#include <quentier/utility/ApplicationSettings.h>
#include <quentier/utility/Compat.h>
#include <quentier/utility/DateTime.h>
#include <quentier/utility/MessageBox.h>
#include <quentier/utility/QuentierCheckPtr.h>
#include <quentier/utility/StandardPaths.h>

#include <qt5qevercloud/QEverCloud.h>

#include <QCheckBox>
#include <QClipboard>
#include <QColorDialog>
#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFocusEvent>
#include <QFontDatabase>
#include <QIcon>
#include <QKeySequence>
#include <QLabel>
#include <QMenu>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QPalette>
#include <QPushButton>
#include <QResizeEvent>
#include <QTextCursor>
#include <QTextEdit>
#include <QTextList>
#include <QThreadPool>
#include <QTimer>
#include <QTimerEvent>
#include <QToolTip>
#include <QXmlStreamWriter>

#include <QtDebug>

#include <algorithm>
#include <cmath>

#define NOTIFY_ERROR(error)                                                    \
    QNWARNING("quentier:main_window", QString::fromUtf8(error));               \
    onSetStatusBarText(QString::fromUtf8(error), secondsToMilliseconds(30))

#define NOTIFY_DEBUG(message)                                                  \
    QNDEBUG("quentier:main_window", QString::fromUtf8(message));               \
    onSetStatusBarText(QString::fromUtf8(message), secondsToMilliseconds(30))

#define FILTERS_VIEW_STATUS_KEY QStringLiteral("ViewExpandedStatus")
#define NOTE_SORTING_MODE_KEY   QStringLiteral("NoteSortingMode")

#define MAIN_WINDOW_GEOMETRY_KEY QStringLiteral("Geometry")
#define MAIN_WINDOW_STATE_KEY    QStringLiteral("State")

#define MAIN_WINDOW_SIDE_PANEL_WIDTH_KEY QStringLiteral("SidePanelWidth")
#define MAIN_WINDOW_NOTE_LIST_WIDTH_KEY  QStringLiteral("NoteListWidth")

#define MAIN_WINDOW_FAVORITES_VIEW_HEIGHT QStringLiteral("FavoritesViewHeight")
#define MAIN_WINDOW_NOTEBOOKS_VIEW_HEIGHT QStringLiteral("NotebooksViewHeight")
#define MAIN_WINDOW_TAGS_VIEW_HEIGHT      QStringLiteral("TagsViewHeight")

#define MAIN_WINDOW_SAVED_SEARCHES_VIEW_HEIGHT                                 \
    QStringLiteral("SavedSearchesViewHeight")

#define MAIN_WINDOW_DELETED_NOTES_VIEW_HEIGHT                                  \
    QStringLiteral("DeletedNotesViewHeight")

#define PERSIST_GEOMETRY_AND_STATE_DELAY     (500)
#define RESTORE_SPLITTER_SIZES_DELAY         (200)
#define CREATE_SIDE_BORDERS_CONTROLLER_DELAY (200)
#define NOTIFY_SIDE_BORDERS_CONTROLLER_DELAY (200)

using namespace quentier;

namespace {

#ifdef WITH_UPDATE_MANAGER
class UpdateManagerIdleInfoProvider final :
    public UpdateManager::IIdleStateInfoProvider
{
public:
    UpdateManagerIdleInfoProvider(NoteEditorTabsAndWindowsCoordinator & c) :
        m_coordinator(c)
    {}

    virtual ~UpdateManagerIdleInfoProvider() override = default;

    virtual qint64 idleTime() override
    {
        return m_coordinator.minIdleTime();
    }

private:
    NoteEditorTabsAndWindowsCoordinator & m_coordinator;
};
#endif

[[nodiscard]] double computeSyncChunkDataProcessingProgress(
    const ISyncChunksDataCounters & counters)
{
    double total = 0.0;
    double processed = 0.0;

    const auto totalSavedSearches = counters.totalSavedSearches();
    if (totalSavedSearches != 0) {
        total += static_cast<double>(totalSavedSearches);
        processed += static_cast<double>(counters.addedSavedSearches());
        processed += static_cast<double>(counters.updatedSavedSearches());
    }

    const auto totalExpungedSavedSearches =
        counters.totalExpungedSavedSearches();
    if (totalExpungedSavedSearches != 0) {
        total += static_cast<double>(totalExpungedSavedSearches);
        processed += static_cast<double>(counters.expungedSavedSearches());
    }

    const auto totalTags = counters.totalTags();
    if (totalTags != 0) {
        total += static_cast<double>(totalTags);
        processed += static_cast<double>(counters.addedTags());
        processed += static_cast<double>(counters.updatedTags());
    }

    const auto totalExpungedTags = counters.totalExpungedTags();
    if (totalExpungedTags != 0) {
        total += static_cast<double>(totalExpungedTags);
        processed += static_cast<double>(counters.expungedTags());
    }

    const auto totalNotebooks = counters.totalNotebooks();
    if (totalNotebooks != 0) {
        total += static_cast<double>(totalNotebooks);
        processed += static_cast<double>(counters.addedNotebooks());
        processed += static_cast<double>(counters.updatedNotebooks());
    }

    const auto totalExpungedNotebooks = counters.totalExpungedNotebooks();
    if (totalExpungedNotebooks != 0) {
        total += static_cast<double>(totalExpungedNotebooks);
        processed += static_cast<double>(counters.expungedNotebooks());
    }

    const auto totalLinkedNotebooks = counters.totalLinkedNotebooks();
    if (totalLinkedNotebooks != 0) {
        total += static_cast<double>(totalLinkedNotebooks);
        processed += static_cast<double>(counters.addedLinkedNotebooks());
        processed += static_cast<double>(counters.updatedLinkedNotebooks());
    }

    const auto totalExpungedLinkedNotebooks =
        counters.totalExpungedLinkedNotebooks();
    if (totalExpungedLinkedNotebooks != 0) {
        total += static_cast<double>(totalExpungedLinkedNotebooks);
        processed += static_cast<double>(counters.expungedLinkedNotebooks());
    }

    if (Q_UNLIKELY(processed > total)) {
        QNWARNING(
            "quentier:main_window",
            "Invalid sync chunk data counters: " << counters);
    }

    if (total == 0.0) {
        return 0.0;
    }

    double percentage = std::min(processed / total, 1.0);
    percentage = std::max(percentage, 0.0);
    return percentage * 100.0;
}

} // namespace

MainWindow::MainWindow(QWidget * pParentWidget) :
    QMainWindow(pParentWidget), m_pUi(new Ui::MainWindow),
    m_pAvailableAccountsActionGroup(new QActionGroup(this)),
    m_pAccountManager(new AccountManager(this)),
    m_animatedSyncButtonIcon(QStringLiteral(":/sync/sync.gif")),
    m_pNotebookModelColumnChangeRerouter(new ColumnChangeRerouter(
        static_cast<int>(NotebookModel::Column::NoteCount),
        static_cast<int>(NotebookModel::Column::Name), this)),
    m_pTagModelColumnChangeRerouter(new ColumnChangeRerouter(
        static_cast<int>(TagModel::Column::NoteCount),
        static_cast<int>(TagModel::Column::Name), this)),
    m_pNoteModelColumnChangeRerouter(new ColumnChangeRerouter(
        NoteModel::Columns::PreviewText, NoteModel::Columns::Title, this)),
    m_pFavoritesModelColumnChangeRerouter(new ColumnChangeRerouter(
        static_cast<int>(FavoritesModel::Column::NoteCount),
        static_cast<int>(FavoritesModel::Column::DisplayName), this)),
    m_shortcutManager(this)
{
    QNTRACE("quentier:main_window", "MainWindow constructor");

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

    QObject::connect(
        this, &MainWindow::shown, m_pSystemTrayIconManager,
        &SystemTrayIconManager::onMainWindowShown);

    QObject::connect(
        this, &MainWindow::hidden, m_pSystemTrayIconManager,
        &SystemTrayIconManager::onMainWindowHidden);

    m_pendingFirstShutdownDialog = m_pendingGreeterDialog &&
        m_pSystemTrayIconManager->isSystemTrayAvailable();

    setupThemeIcons();

    m_pUi->setupUi(this);
    setupAccountSpecificUiElements();

    if (m_nativeIconThemeName.isEmpty()) {
        m_pUi->ActionIconsNative->setVisible(false);
        m_pUi->ActionIconsNative->setDisabled(true);
    }

    setupGenericPanelStyleControllers();
    setupSidePanelStyleControllers();
    restorePanelColors();

    m_pAvailableAccountsActionGroup->setExclusive(true);

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
    connectToolbarButtonsToSlots();
    connectSystemTrayIconManagerSignalsToSlots();

    restoreGeometryAndState();
    startListeningForSplitterMoves();

    // this will emit event with initial thumbnail show/hide state
    onShowNoteThumbnailsPreferenceChanged();

#ifdef WITH_UPDATE_MANAGER
    setupUpdateManager();
#else
    m_pUi->ActionCheckForUpdates->setVisible(false);
#endif

    if (shouldRunSyncOnStartup()) {
        QTimer::singleShot(0, this, &MainWindow::onSyncButtonPressed);
    }
}

MainWindow::~MainWindow()
{
    QNDEBUG("quentier:main_window", "MainWindow::~MainWindow");

    clearSynchronizationManager();

    if (m_pLocalStorageManagerThread) {
        QNDEBUG(
            "quentier:main_window",
            "Local storage manager thread is "
                << "active, stopping it");

        QObject::disconnect(
            m_pLocalStorageManagerThread, &QThread::finished,
            m_pLocalStorageManagerThread, &QThread::deleteLater);

        m_pLocalStorageManagerThread->quit();
        m_pLocalStorageManagerThread->wait();

        QNDEBUG("quentier:main_window", "Deleting LocalStorageManagerAsync");
        delete m_pLocalStorageManagerAsync;
        m_pLocalStorageManagerAsync = nullptr;
    }

    delete m_pUi;
}

void MainWindow::show()
{
    QNDEBUG("quentier:main_window", "MainWindow::show");

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

    if (Q_UNLIKELY(m_pendingGreeterDialog)) {
        m_pendingGreeterDialog = false;

        auto pDialog = std::make_unique<WelcomeToQuentierDialog>(this);
        pDialog->setWindowModality(Qt::WindowModal);
        centerDialog(*pDialog);
        if (pDialog->exec() == QDialog::Accepted) {
            QNDEBUG(
                "quentier:main_window",
                "Log in to Evernote account option "
                    << "was chosen on the greeter screen");

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
    QNDEBUG("quentier:main_window", "MainWindow::connectActionsToSlots");

    // File menu actions
    QObject::connect(
        m_pUi->ActionNewNote, &QAction::triggered, this,
        &MainWindow::onNewNoteCreationRequested);

    QObject::connect(
        m_pUi->ActionNewNotebook, &QAction::triggered, this,
        &MainWindow::onNewNotebookCreationRequested);

    QObject::connect(
        m_pUi->ActionNewTag, &QAction::triggered, this,
        &MainWindow::onNewTagCreationRequested);

    QObject::connect(
        m_pUi->ActionNewSavedSearch, &QAction::triggered, this,
        &MainWindow::onNewSavedSearchCreationRequested);

    QObject::connect(
        m_pUi->ActionImportENEX, &QAction::triggered, this,
        &MainWindow::onImportEnexAction);

    QObject::connect(
        m_pUi->ActionPrint, &QAction::triggered, this,
        &MainWindow::onCurrentNotePrintRequested);

    QObject::connect(
        m_pUi->ActionQuit, &QAction::triggered, this,
        &MainWindow::onQuitAction);

    // Edit menu actions
    QObject::connect(
        m_pUi->ActionFindInsideNote, &QAction::triggered, this,
        &MainWindow::onFindInsideNoteAction);

    QObject::connect(
        m_pUi->ActionFindNext, &QAction::triggered, this,
        &MainWindow::onFindInsideNoteAction);

    QObject::connect(
        m_pUi->ActionFindPrevious, &QAction::triggered, this,
        &MainWindow::onFindPreviousInsideNoteAction);

    QObject::connect(
        m_pUi->ActionReplaceInNote, &QAction::triggered, this,
        &MainWindow::onReplaceInsideNoteAction);

    QObject::connect(
        m_pUi->ActionPreferences, &QAction::triggered, this,
        &MainWindow::onShowPreferencesDialogAction);

    // Undo/redo actions
    QObject::connect(
        m_pUi->ActionUndo, &QAction::triggered, this,
        &MainWindow::onUndoAction);

    QObject::connect(
        m_pUi->ActionRedo, &QAction::triggered, this,
        &MainWindow::onRedoAction);

    // Copy/cut/paste actions
    QObject::connect(
        m_pUi->ActionCopy, &QAction::triggered, this,
        &MainWindow::onCopyAction);

    QObject::connect(
        m_pUi->ActionCut, &QAction::triggered, this, &MainWindow::onCutAction);

    QObject::connect(
        m_pUi->ActionPaste, &QAction::triggered, this,
        &MainWindow::onPasteAction);

    // Select all action
    QObject::connect(
        m_pUi->ActionSelectAll, &QAction::triggered, this,
        &MainWindow::onNoteTextSelectAllToggled);

    // Font actions
    QObject::connect(
        m_pUi->ActionFontBold, &QAction::triggered, this,
        &MainWindow::onNoteTextBoldToggled);

    QObject::connect(
        m_pUi->ActionFontItalic, &QAction::triggered, this,
        &MainWindow::onNoteTextItalicToggled);

    QObject::connect(
        m_pUi->ActionFontUnderlined, &QAction::triggered, this,
        &MainWindow::onNoteTextUnderlineToggled);

    QObject::connect(
        m_pUi->ActionFontStrikethrough, &QAction::triggered, this,
        &MainWindow::onNoteTextStrikethroughToggled);

    QObject::connect(
        m_pUi->ActionIncreaseFontSize, &QAction::triggered, this,
        &MainWindow::onNoteTextIncreaseFontSizeAction);

    QObject::connect(
        m_pUi->ActionDecreaseFontSize, &QAction::triggered, this,
        &MainWindow::onNoteTextDecreaseFontSizeAction);

    QObject::connect(
        m_pUi->ActionFontHighlight, &QAction::triggered, this,
        &MainWindow::onNoteTextHighlightAction);

    // Spell checking
    QObject::connect(
        m_pUi->ActionSpellCheck, &QAction::triggered, this,
        &MainWindow::onNoteTextSpellCheckToggled);

    // Text format actions
    QObject::connect(
        m_pUi->ActionAlignLeft, &QAction::triggered, this,
        &MainWindow::onNoteTextAlignLeftAction);

    QObject::connect(
        m_pUi->ActionAlignCenter, &QAction::triggered, this,
        &MainWindow::onNoteTextAlignCenterAction);

    QObject::connect(
        m_pUi->ActionAlignRight, &QAction::triggered, this,
        &MainWindow::onNoteTextAlignRightAction);

    QObject::connect(
        m_pUi->ActionAlignFull, &QAction::triggered, this,
        &MainWindow::onNoteTextAlignFullAction);

    QObject::connect(
        m_pUi->ActionInsertHorizontalLine, &QAction::triggered, this,
        &MainWindow::onNoteTextAddHorizontalLineAction);

    QObject::connect(
        m_pUi->ActionIncreaseIndentation, &QAction::triggered, this,
        &MainWindow::onNoteTextIncreaseIndentationAction);

    QObject::connect(
        m_pUi->ActionDecreaseIndentation, &QAction::triggered, this,
        &MainWindow::onNoteTextDecreaseIndentationAction);

    QObject::connect(
        m_pUi->ActionInsertBulletedList, &QAction::triggered, this,
        &MainWindow::onNoteTextInsertUnorderedListAction);

    QObject::connect(
        m_pUi->ActionInsertNumberedList, &QAction::triggered, this,
        &MainWindow::onNoteTextInsertOrderedListAction);

    QObject::connect(
        m_pUi->ActionInsertToDo, &QAction::triggered, this,
        &MainWindow::onNoteTextInsertToDoAction);

    QObject::connect(
        m_pUi->ActionInsertTable, &QAction::triggered, this,
        &MainWindow::onNoteTextInsertTableDialogAction);

    QObject::connect(
        m_pUi->ActionEditHyperlink, &QAction::triggered, this,
        &MainWindow::onNoteTextEditHyperlinkAction);

    QObject::connect(
        m_pUi->ActionCopyHyperlink, &QAction::triggered, this,
        &MainWindow::onNoteTextCopyHyperlinkAction);

    QObject::connect(
        m_pUi->ActionRemoveHyperlink, &QAction::triggered, this,
        &MainWindow::onNoteTextRemoveHyperlinkAction);

    QObject::connect(
        m_pUi->ActionSaveNote, &QAction::triggered, this,
        &MainWindow::onSaveNoteAction);

    // Toggle view actions
    QObject::connect(
        m_pUi->ActionShowSidePanel, &QAction::toggled, this,
        &MainWindow::onShowSidePanelActionToggled);

    QObject::connect(
        m_pUi->ActionShowFavorites, &QAction::toggled, this,
        &MainWindow::onShowFavoritesActionToggled);

    QObject::connect(
        m_pUi->ActionShowNotebooks, &QAction::toggled, this,
        &MainWindow::onShowNotebooksActionToggled);

    QObject::connect(
        m_pUi->ActionShowTags, &QAction::toggled, this,
        &MainWindow::onShowTagsActionToggled);

    QObject::connect(
        m_pUi->ActionShowSavedSearches, &QAction::toggled, this,
        &MainWindow::onShowSavedSearchesActionToggled);

    QObject::connect(
        m_pUi->ActionShowDeletedNotes, &QAction::toggled, this,
        &MainWindow::onShowDeletedNotesActionToggled);

    QObject::connect(
        m_pUi->ActionShowNotesList, &QAction::toggled, this,
        &MainWindow::onShowNoteListActionToggled);

    QObject::connect(
        m_pUi->ActionShowToolbar, &QAction::toggled, this,
        &MainWindow::onShowToolbarActionToggled);

    QObject::connect(
        m_pUi->ActionShowStatusBar, &QAction::toggled, this,
        &MainWindow::onShowStatusBarActionToggled);

    // Look and feel actions
    QObject::connect(
        m_pUi->ActionIconsNative, &QAction::triggered, this,
        &MainWindow::onSwitchIconThemeToNativeAction);

    QObject::connect(
        m_pUi->ActionIconsOxygen, &QAction::triggered, this,
        &MainWindow::onSwitchIconThemeToOxygenAction);

    QObject::connect(
        m_pUi->ActionIconsTango, &QAction::triggered, this,
        &MainWindow::onSwitchIconThemeToTangoAction);

    QObject::connect(
        m_pUi->ActionIconsBreeze, &QAction::triggered, this,
        &MainWindow::onSwitchIconThemeToBreezeAction);

    QObject::connect(
        m_pUi->ActionIconsBreezeDark, &QAction::triggered, this,
        &MainWindow::onSwitchIconThemeToBreezeDarkAction);

    // Service menu actions
    QObject::connect(
        m_pUi->ActionSynchronize, &QAction::triggered, this,
        &MainWindow::onSyncButtonPressed);

    // Help menu actions
    QObject::connect(
        m_pUi->ActionShowNoteSource, &QAction::triggered, this,
        &MainWindow::onShowNoteSource);

    QObject::connect(
        m_pUi->ActionViewLogs, &QAction::triggered, this,
        &MainWindow::onViewLogsActionTriggered);

    QObject::connect(
        m_pUi->ActionAbout, &QAction::triggered, this,
        &MainWindow::onShowInfoAboutQuentierActionTriggered);

#ifdef WITH_UPDATE_MANAGER
    QObject::connect(
        m_pUi->ActionCheckForUpdates, &QAction::triggered, this,
        &MainWindow::onCheckForUpdatesActionTriggered);
#endif
}

void MainWindow::connectViewButtonsToSlots()
{
    QNDEBUG("quentier:main_window", "MainWindow::connectViewButtonsToSlots");

    QObject::connect(
        m_pUi->addNotebookButton, &QPushButton::clicked, this,
        &MainWindow::onNewNotebookCreationRequested);

    QObject::connect(
        m_pUi->removeNotebookButton, &QPushButton::clicked, this,
        &MainWindow::onRemoveNotebookButtonPressed);

    QObject::connect(
        m_pUi->notebookInfoButton, &QPushButton::clicked, this,
        &MainWindow::onNotebookInfoButtonPressed);

    QObject::connect(
        m_pUi->addTagButton, &QPushButton::clicked, this,
        &MainWindow::onNewTagCreationRequested);

    QObject::connect(
        m_pUi->removeTagButton, &QPushButton::clicked, this,
        &MainWindow::onRemoveTagButtonPressed);

    QObject::connect(
        m_pUi->tagInfoButton, &QPushButton::clicked, this,
        &MainWindow::onTagInfoButtonPressed);

    QObject::connect(
        m_pUi->addSavedSearchButton, &QPushButton::clicked, this,
        &MainWindow::onNewSavedSearchCreationRequested);

    QObject::connect(
        m_pUi->removeSavedSearchButton, &QPushButton::clicked, this,
        &MainWindow::onRemoveSavedSearchButtonPressed);

    QObject::connect(
        m_pUi->savedSearchInfoButton, &QPushButton::clicked, this,
        &MainWindow::onSavedSearchInfoButtonPressed);

    QObject::connect(
        m_pUi->unfavoritePushButton, &QPushButton::clicked, this,
        &MainWindow::onUnfavoriteItemButtonPressed);

    QObject::connect(
        m_pUi->favoriteInfoButton, &QPushButton::clicked, this,
        &MainWindow::onFavoritedItemInfoButtonPressed);

    QObject::connect(
        m_pUi->restoreDeletedNoteButton, &QPushButton::clicked, this,
        &MainWindow::onRestoreDeletedNoteButtonPressed);

    QObject::connect(
        m_pUi->eraseDeletedNoteButton, &QPushButton::clicked, this,
        &MainWindow::onDeleteNotePermanentlyButtonPressed);

    QObject::connect(
        m_pUi->deletedNoteInfoButton, &QPushButton::clicked, this,
        &MainWindow::onDeletedNoteInfoButtonPressed);

    QObject::connect(
        m_pUi->filtersViewTogglePushButton, &QPushButton::clicked, this,
        &MainWindow::onFiltersViewTogglePushButtonPressed);
}

void MainWindow::connectToolbarButtonsToSlots()
{
    QNDEBUG("quentier:main_window", "MainWindow::connectToolbarButtonsToSlots");

    QObject::connect(
        m_pUi->addNotePushButton, &QPushButton::clicked, this,
        &MainWindow::onNewNoteCreationRequested);

    QObject::connect(
        m_pUi->deleteNotePushButton, &QPushButton::clicked, this,
        &MainWindow::onDeleteCurrentNoteButtonPressed);

    QObject::connect(
        m_pUi->infoButton, &QPushButton::clicked, this,
        &MainWindow::onCurrentNoteInfoRequested);

    QObject::connect(
        m_pUi->printNotePushButton, &QPushButton::clicked, this,
        &MainWindow::onCurrentNotePrintRequested);

    QObject::connect(
        m_pUi->exportNoteToPdfPushButton, &QPushButton::clicked, this,
        &MainWindow::onCurrentNotePdfExportRequested);

    QObject::connect(
        m_pUi->syncPushButton, &QPushButton::clicked, this,
        &MainWindow::onSyncButtonPressed);
}

void MainWindow::connectSystemTrayIconManagerSignalsToSlots()
{
    QNDEBUG(
        "quentier:main_window",
        "MainWindow::connectSystemTrayIconManagerSignalsToSlots");

    QObject::connect(
        m_pSystemTrayIconManager, &SystemTrayIconManager::notifyError, this,
        &MainWindow::onSystemTrayIconManagerError);

    QObject::connect(
        m_pSystemTrayIconManager,
        &SystemTrayIconManager::newTextNoteAdditionRequested, this,
        &MainWindow::onNewNoteRequestedFromSystemTrayIcon);

    QObject::connect(
        m_pSystemTrayIconManager, &SystemTrayIconManager::quitRequested, this,
        &MainWindow::onQuitRequestedFromSystemTrayIcon);

    QObject::connect(
        m_pSystemTrayIconManager,
        &SystemTrayIconManager::accountSwitchRequested, this,
        &MainWindow::onAccountSwitchRequested);

    QObject::connect(
        m_pSystemTrayIconManager, &SystemTrayIconManager::showRequested, this,
        &MainWindow::onShowRequestedFromTrayIcon);

    QObject::connect(
        m_pSystemTrayIconManager, &SystemTrayIconManager::hideRequested, this,
        &MainWindow::onHideRequestedFromTrayIcon);
}

void MainWindow::connectToPreferencesDialogSignals(PreferencesDialog & dialog)
{
    QObject::connect(
        &dialog, &PreferencesDialog::noteEditorUseLimitedFontsOptionChanged,
        this, &MainWindow::onUseLimitedFontsPreferenceChanged);

    QObject::connect(
        &dialog, &PreferencesDialog::iconThemeChanged, this,
        &MainWindow::onSwitchIconTheme);

    QObject::connect(
        &dialog,
        &PreferencesDialog::synchronizationDownloadNoteThumbnailsOptionChanged,
        this, &MainWindow::synchronizationDownloadNoteThumbnailsOptionChanged);

    QObject::connect(
        &dialog,
        &PreferencesDialog::synchronizationDownloadInkNoteImagesOptionChanged,
        this, &MainWindow::synchronizationDownloadInkNoteImagesOptionChanged);

    QObject::connect(
        &dialog, &PreferencesDialog::showNoteThumbnailsOptionChanged, this,
        &MainWindow::onShowNoteThumbnailsPreferenceChanged);

    QObject::connect(
        &dialog, &PreferencesDialog::disableNativeMenuBarOptionChanged, this,
        &MainWindow::onDisableNativeMenuBarPreferenceChanged);

    QObject::connect(
        &dialog, &PreferencesDialog::runSyncPeriodicallyOptionChanged, this,
        &MainWindow::onRunSyncEachNumMinitesPreferenceChanged);

    QObject::connect(
        &dialog, &PreferencesDialog::noteEditorFontColorChanged,
        m_pNoteEditorTabsAndWindowsCoordinator,
        &NoteEditorTabsAndWindowsCoordinator::noteEditorFontColorChanged);

    QObject::connect(
        &dialog, &PreferencesDialog::noteEditorBackgroundColorChanged,
        m_pNoteEditorTabsAndWindowsCoordinator,
        &NoteEditorTabsAndWindowsCoordinator::noteEditorBackgroundColorChanged);

    QObject::connect(
        &dialog, &PreferencesDialog::noteEditorHighlightColorChanged,
        m_pNoteEditorTabsAndWindowsCoordinator,
        &NoteEditorTabsAndWindowsCoordinator::noteEditorHighlightColorChanged);

    QObject::connect(
        &dialog, &PreferencesDialog::noteEditorHighlightedTextColorChanged,
        m_pNoteEditorTabsAndWindowsCoordinator,
        &NoteEditorTabsAndWindowsCoordinator::
            noteEditorHighlightedTextColorChanged);

    QObject::connect(
        &dialog, &PreferencesDialog::noteEditorColorsReset,
        m_pNoteEditorTabsAndWindowsCoordinator,
        &NoteEditorTabsAndWindowsCoordinator::noteEditorColorsReset);

    QObject::connect(
        &dialog, &PreferencesDialog::panelFontColorChanged, this,
        &MainWindow::onPanelFontColorChanged);

    QObject::connect(
        &dialog, &PreferencesDialog::panelBackgroundColorChanged, this,
        &MainWindow::onPanelBackgroundColorChanged);

    QObject::connect(
        &dialog, &PreferencesDialog::panelUseBackgroundGradientSettingChanged,
        this, &MainWindow::onPanelUseBackgroundGradientSettingChanged);

    QObject::connect(
        &dialog, &PreferencesDialog::panelBackgroundLinearGradientChanged, this,
        &MainWindow::onPanelBackgroundLinearGradientChanged);

#if WITH_UPDATE_MANAGER
    QObject::connect(
        &dialog, &PreferencesDialog::checkForUpdatesRequested, this,
        &MainWindow::onCheckForUpdatesActionTriggered);

    QObject::connect(
        &dialog, &PreferencesDialog::checkForUpdatesOptionChanged, this,
        [this](bool enabled) { this->m_pUpdateManager->setEnabled(enabled); });

    QObject::connect(
        &dialog, &PreferencesDialog::checkForUpdatesOnStartupOptionChanged,
        this, [this](bool enabled) {
            this->m_pUpdateManager->setShouldCheckForUpdatesOnStartup(enabled);
        });

    QObject::connect(
        &dialog, &PreferencesDialog::useContinuousUpdateChannelOptionChanged,
        this, [this](bool enabled) {
            this->m_pUpdateManager->setUseContinuousUpdateChannel(enabled);
        });

    QObject::connect(
        &dialog, &PreferencesDialog::checkForUpdatesIntervalChanged, this,
        [this](qint64 intervalMsec) {
            this->m_pUpdateManager->setCheckForUpdatesIntervalMsec(
                intervalMsec);
        });

    QObject::connect(
        &dialog, &PreferencesDialog::updateChannelChanged, this,
        [this](QString channel) {
            this->m_pUpdateManager->setUpdateChannel(std::move(channel));
        });

    QObject::connect(
        &dialog, &PreferencesDialog::updateProviderChanged, this,
        [this](UpdateProvider provider) {
            this->m_pUpdateManager->setUpdateProvider(provider);
        });
#endif
}

void MainWindow::addMenuActionsToMainWindow()
{
    QNDEBUG("quentier:main_window", "MainWindow::addMenuActionsToMainWindow");

    // NOTE: adding the actions from the menu bar's menus is required for
    // getting the shortcuts of these actions to work properly; action shortcuts
    // only fire when the menu is shown which is not really the purpose behind
    // those shortcuts
    auto menus = m_pUi->menuBar->findChildren<QMenu *>();
    const int numMenus = menus.size();
    for (int i = 0; i < numMenus; ++i) {
        auto * menu = menus[i];
        auto actions = menu->actions();
        for (auto * action: qAsConst(actions)) {
            addAction(action);
        }
    }

    m_pAvailableAccountsSubMenu = new QMenu(tr("Switch account"));

    auto * separatorAction =
        m_pUi->menuFile->insertSeparator(m_pUi->ActionQuit);

    auto * switchAccountSubMenuAction = m_pUi->menuFile->insertMenu(
        separatorAction, m_pAvailableAccountsSubMenu);

    Q_UNUSED(m_pUi->menuFile->insertSeparator(switchAccountSubMenuAction));
    updateSubMenuWithAvailableAccounts();
}

void MainWindow::updateSubMenuWithAvailableAccounts()
{
    QNDEBUG(
        "quentier:main_window",
        "MainWindow::updateSubMenuWithAvailableAccounts");

    if (Q_UNLIKELY(!m_pAvailableAccountsSubMenu)) {
        QNDEBUG("quentier:main_window", "No available accounts sub-menu");
        return;
    }

    delete m_pAvailableAccountsActionGroup;
    m_pAvailableAccountsActionGroup = new QActionGroup(this);
    m_pAvailableAccountsActionGroup->setExclusive(true);

    m_pAvailableAccountsSubMenu->clear();

    const auto & availableAccounts = m_pAccountManager->availableAccounts();
    int numAvailableAccounts = availableAccounts.size();
    for (int i = 0; i < numAvailableAccounts; ++i) {
        const Account & availableAccount = availableAccounts[i];

        QNTRACE(
            "quentier:main_window",
            "Examining the available account: " << availableAccount);

        QString availableAccountRepresentationName = availableAccount.name();
        if (availableAccount.type() == Account::Type::Local) {
            availableAccountRepresentationName += QStringLiteral(" (");
            availableAccountRepresentationName += tr("local");
            availableAccountRepresentationName += QStringLiteral(")");
        }
        else if (availableAccount.type() == Account::Type::Evernote) {
            QString host = availableAccount.evernoteHost();
            if (host != QStringLiteral("www.evernote.com")) {
                availableAccountRepresentationName += QStringLiteral(" (");

                if (host == QStringLiteral("app.yinxiang.com")) {
                    availableAccountRepresentationName +=
                        QStringLiteral("Yinxiang Biji");
                }
                else {
                    availableAccountRepresentationName += host;
                }

                availableAccountRepresentationName += QStringLiteral(")");
            }
        }

        auto * pAccountAction =
            new QAction(availableAccountRepresentationName, nullptr);

        m_pAvailableAccountsSubMenu->addAction(pAccountAction);

        pAccountAction->setData(i);
        pAccountAction->setCheckable(true);

        if (!m_pAccount.isNull() && (*m_pAccount == availableAccount)) {
            pAccountAction->setChecked(true);
        }

        addAction(pAccountAction);

        QObject::connect(
            pAccountAction, &QAction::toggled, this,
            &MainWindow::onSwitchAccountActionToggled);

        m_pAvailableAccountsActionGroup->addAction(pAccountAction);
    }

    if (Q_LIKELY(numAvailableAccounts != 0)) {
        Q_UNUSED(m_pAvailableAccountsSubMenu->addSeparator())
    }

    auto * pAddAccountAction =
        m_pAvailableAccountsSubMenu->addAction(tr("Add account"));

    addAction(pAddAccountAction);

    QObject::connect(
        pAddAccountAction, &QAction::triggered, this,
        &MainWindow::onAddAccountActionTriggered);

    auto * pManageAccountsAction =
        m_pAvailableAccountsSubMenu->addAction(tr("Manage accounts"));

    addAction(pManageAccountsAction);

    QObject::connect(
        pManageAccountsAction, &QAction::triggered, this,
        &MainWindow::onManageAccountsActionTriggered);
}

void MainWindow::setupInitialChildWidgetsWidths()
{
    QNDEBUG(
        "quentier:main_window", "MainWindow::setupInitialChildWidgetsWidths");

    int totalWidth = width();

    // 1/3 - for side view, 2/3 - for note list view, 3/3 - for the note editor
    int partWidth = totalWidth / 5;

    QNTRACE(
        "quentier:main_window",
        "Total width = " << totalWidth << ", part width = " << partWidth);

    auto splitterSizes = m_pUi->splitter->sizes();
    int splitterSizesCount = splitterSizes.count();
    if (Q_UNLIKELY(splitterSizesCount != 3)) {
        ErrorString error(
            QT_TR_NOOP("Internal error: can't setup the proper initial widths "
                       "for side panel, note list view and note editors view: "
                       "wrong number of sizes within the splitter"));

        QNWARNING(
            "quentier:main_window",
            error << ", sizes count: " << splitterSizesCount);

        onSetStatusBarText(error.localizedString(), secondsToMilliseconds(30));
        return;
    }

    splitterSizes[0] = partWidth;
    splitterSizes[1] = partWidth;
    splitterSizes[2] = totalWidth - 2 * partWidth;

    m_pUi->splitter->setSizes(splitterSizes);
}

void MainWindow::setWindowTitleForAccount(const Account & account)
{
    QNDEBUG(
        "quentier:main_window",
        "MainWindow::setWindowTitleForAccount: " << account.name());

    bool nonStandardPersistencePath = false;
    Q_UNUSED(applicationPersistentStoragePath(&nonStandardPersistencePath))

    QString username = account.name();
    QString displayName = account.displayName();

    QString title = qApp->applicationName() + QStringLiteral(": ");
    if (!displayName.isEmpty()) {
        title += displayName;
        title += QStringLiteral(" (");
        title += username;
        if (nonStandardPersistencePath) {
            title += QStringLiteral(", ");

            title +=
                QDir::toNativeSeparators(accountPersistentStoragePath(account));
        }
        title += QStringLiteral(")");
    }
    else {
        title += username;

        if (nonStandardPersistencePath) {
            title += QStringLiteral(" (");

            title +=
                QDir::toNativeSeparators(accountPersistentStoragePath(account));

            title += QStringLiteral(")");
        }
    }

    setWindowTitle(title);
}

NoteEditorWidget * MainWindow::currentNoteEditorTab()
{
    QNDEBUG("quentier:main_window", "MainWindow::currentNoteEditorTab");

    if (Q_UNLIKELY(m_pUi->noteEditorsTabWidget->count() == 0)) {
        QNTRACE("quentier:main_window", "No open note editors");
        return nullptr;
    }

    int currentIndex = m_pUi->noteEditorsTabWidget->currentIndex();
    if (Q_UNLIKELY(currentIndex < 0)) {
        QNTRACE("quentier:main_window", "No current note editor");
        return nullptr;
    }

    auto * currentWidget = m_pUi->noteEditorsTabWidget->widget(currentIndex);
    if (Q_UNLIKELY(!currentWidget)) {
        QNTRACE("quentier:main_window", "No current widget");
        return nullptr;
    }

    auto * noteEditorWidget = qobject_cast<NoteEditorWidget *>(currentWidget);
    if (Q_UNLIKELY(!noteEditorWidget)) {
        QNWARNING(
            "quentier:main_window",
            "Can't cast current note tag "
                << "widget's widget to note editor widget");
        return nullptr;
    }

    return noteEditorWidget;
}

void MainWindow::createNewNote(
    NoteEditorTabsAndWindowsCoordinator::NoteEditorMode::type noteEditorMode)
{
    QNDEBUG(
        "quentier:main_window",
        "MainWindow::createNewNote: "
            << "note editor mode = " << noteEditorMode);

    if (Q_UNLIKELY(!m_pNoteEditorTabsAndWindowsCoordinator)) {
        QNDEBUG(
            "quentier:main_window",
            "No note editor tabs and windows "
                << "coordinator, probably the button was pressed too quickly "
                << "on startup, skipping");
        return;
    }

    if (Q_UNLIKELY(!m_pNoteModel)) {
        Q_UNUSED(internalErrorMessageBox(
            this,
            tr("Can't create a new note: note model is unexpectedly null")))
        return;
    }

    if (Q_UNLIKELY(!m_pNotebookModel)) {
        Q_UNUSED(internalErrorMessageBox(
            this,
            tr("Can't create a new note: notebook model is unexpectedly null")))
        return;
    }

    auto currentNotebookIndex =
        m_pUi->notebooksTreeView->currentlySelectedItemIndex();

    if (Q_UNLIKELY(!currentNotebookIndex.isValid())) {
        Q_UNUSED(informationMessageBox(
            this, tr("No notebook is selected"),
            tr("Please select the notebook in which you want to create "
               "the note; if you don't have any notebooks yet, create one")))
        return;
    }

    const auto * pNotebookModelItem =
        m_pNotebookModel->itemForIndex(currentNotebookIndex);

    if (Q_UNLIKELY(!pNotebookModelItem)) {
        Q_UNUSED(internalErrorMessageBox(
            this,
            tr("Can't create a new note: can't find the notebook model item "
               "corresponding to the currently selected notebook")))
        return;
    }

    if (Q_UNLIKELY(
            pNotebookModelItem->type() != INotebookModelItem::Type::Notebook))
    {
        Q_UNUSED(informationMessageBox(
            this, tr("No notebook is selected"),
            tr("Please select the notebook in which you want to create "
               "the note (currently the notebook stack seems to be selected)")))
        return;
    }

    const auto * pNotebookItem = pNotebookModelItem->cast<NotebookItem>();
    if (Q_UNLIKELY(!pNotebookItem)) {
        Q_UNUSED(internalErrorMessageBox(
            this,
            tr("Can't create a new note: the notebook model item has notebook "
               "type but null pointer to the actual notebook item")))
        return;
    }

    m_pNoteEditorTabsAndWindowsCoordinator->createNewNote(
        pNotebookItem->localUid(), pNotebookItem->guid(), noteEditorMode);
}

void MainWindow::connectSynchronizationManager()
{
    QNDEBUG(
        "quentier:main_window", "MainWindow::connectSynchronizationManager");

    if (Q_UNLIKELY(!m_pSynchronizationManager)) {
        QNDEBUG("quentier:main_window", "No synchronization manager");
        return;
    }

    // Connect local signals to SynchronizationManager slots
    QObject::connect(
        this, &MainWindow::authenticate, m_pSynchronizationManager,
        &SynchronizationManager::authenticate,
        Qt::ConnectionType(Qt::QueuedConnection | Qt::UniqueConnection));

    QObject::connect(
        this, &MainWindow::authenticateCurrentAccount,
        m_pSynchronizationManager,
        &SynchronizationManager::authenticateCurrentAccount,
        Qt::ConnectionType(Qt::QueuedConnection | Qt::UniqueConnection));

    QObject::connect(
        m_pAccountManager, &AccountManager::revokeAuthenticationRequested,
        m_pSynchronizationManager,
        &SynchronizationManager::revokeAuthentication);

    QObject::connect(
        this, &MainWindow::synchronize, m_pSynchronizationManager,
        &SynchronizationManager::synchronize,
        Qt::ConnectionType(Qt::QueuedConnection | Qt::UniqueConnection));

    QObject::connect(
        this, &MainWindow::stopSynchronization, m_pSynchronizationManager,
        &SynchronizationManager::stop,
        Qt::ConnectionType(Qt::QueuedConnection | Qt::UniqueConnection));

    // Connect SynchronizationManager signals to local slots
    QObject::connect(
        m_pSynchronizationManager, &SynchronizationManager::started, this,
        &MainWindow::onSynchronizationStarted,
        Qt::ConnectionType(Qt::QueuedConnection | Qt::UniqueConnection));

    QObject::connect(
        m_pSynchronizationManager, &SynchronizationManager::stopped, this,
        &MainWindow::onSynchronizationStopped,
        Qt::ConnectionType(Qt::QueuedConnection | Qt::UniqueConnection));

    QObject::connect(
        m_pSynchronizationManager, &SynchronizationManager::failed, this,
        &MainWindow::onSynchronizationManagerFailure,
        Qt::ConnectionType(Qt::QueuedConnection | Qt::UniqueConnection));

    QObject::connect(
        m_pSynchronizationManager, &SynchronizationManager::finished, this,
        &MainWindow::onSynchronizationFinished,
        Qt::ConnectionType(Qt::QueuedConnection | Qt::UniqueConnection));

    QObject::connect(
        m_pSynchronizationManager,
        &SynchronizationManager::authenticationFinished, this,
        &MainWindow::onAuthenticationFinished,
        Qt::ConnectionType(Qt::QueuedConnection | Qt::UniqueConnection));

    QObject::connect(
        m_pSynchronizationManager,
        &SynchronizationManager::authenticationRevoked, m_pAccountManager,
        &AccountManager::onAuthenticationRevoked,
        Qt::ConnectionType(Qt::QueuedConnection | Qt::UniqueConnection));

    QObject::connect(
        m_pSynchronizationManager,
        &SynchronizationManager::remoteToLocalSyncStopped, this,
        &MainWindow::onRemoteToLocalSyncStopped,
        Qt::ConnectionType(Qt::QueuedConnection | Qt::UniqueConnection));

    QObject::connect(
        m_pSynchronizationManager,
        &SynchronizationManager::sendLocalChangesStopped, this,
        &MainWindow::onSendLocalChangesStopped,
        Qt::ConnectionType(Qt::QueuedConnection | Qt::UniqueConnection));

    QObject::connect(
        m_pSynchronizationManager, &SynchronizationManager::rateLimitExceeded,
        this, &MainWindow::onRateLimitExceeded,
        Qt::ConnectionType(Qt::QueuedConnection | Qt::UniqueConnection));

    QObject::connect(
        m_pSynchronizationManager,
        &SynchronizationManager::remoteToLocalSyncDone, this,
        &MainWindow::onRemoteToLocalSyncDone,
        Qt::ConnectionType(Qt::QueuedConnection | Qt::UniqueConnection));

    QObject::connect(
        m_pSynchronizationManager,
        &SynchronizationManager::syncChunksDownloadProgress, this,
        &MainWindow::onSyncChunksDownloadProgress,
        Qt::ConnectionType(Qt::QueuedConnection | Qt::UniqueConnection));

    QObject::connect(
        m_pSynchronizationManager,
        &SynchronizationManager::syncChunksDownloaded, this,
        &MainWindow::onSyncChunksDownloaded,
        Qt::ConnectionType(Qt::QueuedConnection | Qt::UniqueConnection));

    QObject::connect(
        m_pSynchronizationManager,
        &SynchronizationManager::syncChunksDataProcessingProgress, this,
        &MainWindow::onSyncChunksDataProcessingProgress,
        Qt::ConnectionType(Qt::QueuedConnection | Qt::UniqueConnection));

    QObject::connect(
        m_pSynchronizationManager,
        &SynchronizationManager::notesDownloadProgress, this,
        &MainWindow::onNotesDownloadProgress,
        Qt::ConnectionType(Qt::QueuedConnection | Qt::UniqueConnection));

    QObject::connect(
        m_pSynchronizationManager,
        &SynchronizationManager::resourcesDownloadProgress, this,
        &MainWindow::onResourcesDownloadProgress,
        Qt::ConnectionType(Qt::QueuedConnection | Qt::UniqueConnection));

    QObject::connect(
        m_pSynchronizationManager,
        &SynchronizationManager::linkedNotebookSyncChunksDownloadProgress, this,
        &MainWindow::onLinkedNotebookSyncChunksDownloadProgress,
        Qt::ConnectionType(Qt::QueuedConnection | Qt::UniqueConnection));

    QObject::connect(
        m_pSynchronizationManager,
        &SynchronizationManager::linkedNotebooksSyncChunksDownloaded, this,
        &MainWindow::onLinkedNotebooksSyncChunksDownloaded,
        Qt::ConnectionType(Qt::QueuedConnection | Qt::UniqueConnection));

    QObject::connect(
        m_pSynchronizationManager,
        &SynchronizationManager::linkedNotebookSyncChunksDataProcessingProgress,
        this, &MainWindow::onLinkedNotebookSyncChunksDataProcessingProgress,
        Qt::ConnectionType(Qt::QueuedConnection | Qt::UniqueConnection));

    QObject::connect(
        m_pSynchronizationManager,
        &SynchronizationManager::linkedNotebooksNotesDownloadProgress, this,
        &MainWindow::onLinkedNotebooksNotesDownloadProgress,
        Qt::ConnectionType(Qt::QueuedConnection | Qt::UniqueConnection));
}

void MainWindow::disconnectSynchronizationManager()
{
    QNDEBUG(
        "quentier:main_window", "MainWindow::disconnectSynchronizationManager");

    if (Q_UNLIKELY(!m_pSynchronizationManager)) {
        QNDEBUG("quentier:main_window", "No synchronization manager");
        return;
    }

    // Disconnect local signals from SynchronizationManager slots
    QObject::disconnect(
        this, &MainWindow::authenticate, m_pSynchronizationManager,
        &SynchronizationManager::authenticate);

    QObject::disconnect(
        this, &MainWindow::authenticateCurrentAccount,
        m_pSynchronizationManager,
        &SynchronizationManager::authenticateCurrentAccount);

    QObject::disconnect(
        this, &MainWindow::synchronize, m_pSynchronizationManager,
        &SynchronizationManager::synchronize);

    QObject::disconnect(
        this, &MainWindow::stopSynchronization, m_pSynchronizationManager,
        &SynchronizationManager::stop);

    // Disconnect SynchronizationManager signals from local slots
    QObject::disconnect(
        m_pSynchronizationManager, &SynchronizationManager::started, this,
        &MainWindow::onSynchronizationStarted);

    QObject::disconnect(
        m_pSynchronizationManager, &SynchronizationManager::stopped, this,
        &MainWindow::onSynchronizationStopped);

    QObject::disconnect(
        m_pSynchronizationManager, &SynchronizationManager::failed, this,
        &MainWindow::onSynchronizationManagerFailure);

    QObject::disconnect(
        m_pSynchronizationManager, &SynchronizationManager::finished, this,
        &MainWindow::onSynchronizationFinished);

    QObject::disconnect(
        m_pSynchronizationManager,
        &SynchronizationManager::authenticationFinished, this,
        &MainWindow::onAuthenticationFinished);

    QObject::disconnect(
        m_pSynchronizationManager,
        &SynchronizationManager::authenticationRevoked, m_pAccountManager,
        &AccountManager::onAuthenticationRevoked);

    QObject::disconnect(
        m_pSynchronizationManager,
        &SynchronizationManager::remoteToLocalSyncStopped, this,
        &MainWindow::onRemoteToLocalSyncStopped);

    QObject::disconnect(
        m_pSynchronizationManager,
        &SynchronizationManager::sendLocalChangesStopped, this,
        &MainWindow::onSendLocalChangesStopped);

    QObject::disconnect(
        m_pSynchronizationManager, &SynchronizationManager::rateLimitExceeded,
        this, &MainWindow::onRateLimitExceeded);

    QObject::disconnect(
        m_pSynchronizationManager,
        &SynchronizationManager::remoteToLocalSyncDone, this,
        &MainWindow::onRemoteToLocalSyncDone);

    QObject::disconnect(
        m_pSynchronizationManager,
        &SynchronizationManager::syncChunksDownloadProgress, this,
        &MainWindow::onSyncChunksDownloadProgress);

    QObject::disconnect(
        m_pSynchronizationManager,
        &SynchronizationManager::syncChunksDownloaded, this,
        &MainWindow::onSyncChunksDownloaded);

    QObject::disconnect(
        m_pSynchronizationManager,
        &SynchronizationManager::syncChunksDataProcessingProgress, this,
        &MainWindow::onSyncChunksDataProcessingProgress);

    QObject::disconnect(
        m_pSynchronizationManager,
        &SynchronizationManager::notesDownloadProgress, this,
        &MainWindow::onNotesDownloadProgress);

    QObject::disconnect(
        m_pSynchronizationManager,
        &SynchronizationManager::resourcesDownloadProgress, this,
        &MainWindow::onResourcesDownloadProgress);

    QObject::disconnect(
        m_pSynchronizationManager,
        &SynchronizationManager::linkedNotebookSyncChunksDownloadProgress, this,
        &MainWindow::onLinkedNotebookSyncChunksDownloadProgress);

    QObject::disconnect(
        m_pSynchronizationManager,
        &SynchronizationManager::linkedNotebooksSyncChunksDownloaded, this,
        &MainWindow::onLinkedNotebooksSyncChunksDownloaded);

    QObject::disconnect(
        m_pSynchronizationManager,
        &SynchronizationManager::linkedNotebookSyncChunksDataProcessingProgress,
        this, &MainWindow::onLinkedNotebookSyncChunksDataProcessingProgress);

    QObject::disconnect(
        m_pSynchronizationManager,
        &SynchronizationManager::linkedNotebooksNotesDownloadProgress, this,
        &MainWindow::onLinkedNotebooksNotesDownloadProgress);
}

void MainWindow::startSyncButtonAnimation()
{
    QNDEBUG("quentier:main_window", "MainWindow::startSyncButtonAnimation");

    QObject::disconnect(
        &m_animatedSyncButtonIcon, &QMovie::frameChanged, this,
        &MainWindow::onAnimatedSyncIconFrameChangedPendingFinish);

    QObject::connect(
        &m_animatedSyncButtonIcon, &QMovie::frameChanged, this,
        &MainWindow::onAnimatedSyncIconFrameChanged);

    // If the animation doesn't run forever, make it so
    if (m_animatedSyncButtonIcon.loopCount() != -1) {
        QObject::connect(
            &m_animatedSyncButtonIcon, &QMovie::finished,
            &m_animatedSyncButtonIcon, &QMovie::start);
    }

    m_animatedSyncButtonIcon.start();
}

void MainWindow::stopSyncButtonAnimation()
{
    QNDEBUG("quentier:main_window", "MainWindow::stopSyncButtonAnimation");

    if (m_animatedSyncButtonIcon.loopCount() != -1) {
        QObject::disconnect(
            &m_animatedSyncButtonIcon, &QMovie::finished,
            &m_animatedSyncButtonIcon, &QMovie::start);
    }

    QObject::disconnect(
        &m_animatedSyncButtonIcon, &QMovie::finished, this,
        &MainWindow::onSyncIconAnimationFinished);

    QObject::disconnect(
        &m_animatedSyncButtonIcon, &QMovie::frameChanged, this,
        &MainWindow::onAnimatedSyncIconFrameChanged);

    m_animatedSyncButtonIcon.stop();
    m_pUi->syncPushButton->setIcon(QIcon(QStringLiteral(":/sync/sync.png")));
}

void MainWindow::scheduleSyncButtonAnimationStop()
{
    QNDEBUG(
        "quentier:main_window", "MainWindow::scheduleSyncButtonAnimationStop");

    if (m_animatedSyncButtonIcon.state() != QMovie::Running) {
        stopSyncButtonAnimation();
        return;
    }

    // NOTE: either finished signal should stop the sync animation or,
    // if the movie is naturally looped, the frame count coming to max value
    // (or zero after max value) would serve as a sign that full animation loop
    // has finished

    QObject::connect(
        &m_animatedSyncButtonIcon, &QMovie::finished, this,
        &MainWindow::onSyncIconAnimationFinished,
        Qt::ConnectionType(Qt::UniqueConnection | Qt::QueuedConnection));

    QObject::connect(
        &m_animatedSyncButtonIcon, &QMovie::frameChanged, this,
        &MainWindow::onAnimatedSyncIconFrameChangedPendingFinish,
        Qt::ConnectionType(Qt::UniqueConnection | Qt::QueuedConnection));
}

void MainWindow::startListeningForSplitterMoves()
{
    QNDEBUG(
        "quentier:main_window", "MainWindow::startListeningForSplitterMoves");

    QObject::connect(
        m_pUi->splitter, &QSplitter::splitterMoved, this,
        &MainWindow::onSplitterHandleMoved, Qt::UniqueConnection);

    QObject::connect(
        m_pUi->sidePanelSplitter, &QSplitter::splitterMoved, this,
        &MainWindow::onSidePanelSplittedHandleMoved, Qt::UniqueConnection);
}

void MainWindow::stopListeningForSplitterMoves()
{
    QNDEBUG(
        "quentier:main_window", "MainWindow::stopListeningForSplitterMoves");

    QObject::disconnect(
        m_pUi->splitter, &QSplitter::splitterMoved, this,
        &MainWindow::onSplitterHandleMoved);

    QObject::disconnect(
        m_pUi->sidePanelSplitter, &QSplitter::splitterMoved, this,
        &MainWindow::onSidePanelSplittedHandleMoved);
}

void MainWindow::persistChosenIconTheme(const QString & iconThemeName)
{
    QNDEBUG(
        "quentier:main_window",
        "MainWindow::persistChosenIconTheme: " << iconThemeName);

    ApplicationSettings appSettings(
        *m_pAccount, preferences::keys::files::userInterface);

    appSettings.beginGroup(preferences::keys::appearanceGroup);
    appSettings.setValue(preferences::keys::iconTheme, iconThemeName);
    appSettings.endGroup();
}

void MainWindow::refreshChildWidgetsThemeIcons()
{
    QNDEBUG(
        "quentier:main_window", "MainWindow::refreshChildWidgetsThemeIcons");

    refreshThemeIcons<QAction>();
    refreshThemeIcons<QPushButton>();
    refreshThemeIcons<QCheckBox>();
    refreshThemeIcons<ColorPickerToolButton>();
    refreshThemeIcons<InsertTableToolButton>();

    refreshNoteEditorWidgetsSpecialIcons();
}

void MainWindow::refreshNoteEditorWidgetsSpecialIcons()
{
    QNDEBUG(
        "quentier:main_window",
        "MainWindow::refreshNoteEditorWidgetsSpecialIcons");

    if (m_pNoteEditorTabsAndWindowsCoordinator) {
        m_pNoteEditorTabsAndWindowsCoordinator
            ->refreshNoteEditorWidgetsSpecialIcons();
    }
}

void MainWindow::showHideViewColumnsForAccountType(
    const Account::Type accountType)
{
    QNDEBUG(
        "quentier:main_window",
        "MainWindow::showHideViewColumnsForAccountType: " << accountType);

    bool isLocal = (accountType == Account::Type::Local);

    auto * notebooksTreeView = m_pUi->notebooksTreeView;

    notebooksTreeView->setColumnHidden(
        static_cast<int>(NotebookModel::Column::Published), isLocal);

    notebooksTreeView->setColumnHidden(
        static_cast<int>(NotebookModel::Column::Dirty), isLocal);

    auto * tagsTreeView = m_pUi->tagsTreeView;

    tagsTreeView->setColumnHidden(
        static_cast<int>(TagModel::Column::Dirty), isLocal);

    auto * savedSearchesItemView = m_pUi->savedSearchesItemView;

    savedSearchesItemView->setColumnHidden(
        static_cast<int>(SavedSearchModel::Column::Dirty), isLocal);

    auto * deletedNotesTableView = m_pUi->deletedNotesTableView;
    deletedNotesTableView->setColumnHidden(NoteModel::Columns::Dirty, isLocal);
}

void MainWindow::expandFiltersView()
{
    QNDEBUG("quentier:main_window", "MainWindow::expandFiltersView");

    m_pUi->filtersViewTogglePushButton->setIcon(
        QIcon::fromTheme(QStringLiteral("go-down")));

    m_pUi->filterBodyFrame->show();
    m_pUi->filterFrameBottomBoundary->hide();
    m_pUi->filterFrame->adjustSize();
}

void MainWindow::foldFiltersView()
{
    QNDEBUG("quentier:main_window", "MainWindow::foldFiltersView");

    m_pUi->filtersViewTogglePushButton->setIcon(
        QIcon::fromTheme(QStringLiteral("go-next")));

    m_pUi->filterBodyFrame->hide();
    m_pUi->filterFrameBottomBoundary->show();

    if (m_shown) {
        adjustNoteListAndFiltersSplitterSizes();
    }
}

void MainWindow::adjustNoteListAndFiltersSplitterSizes()
{
    QNDEBUG(
        "quentier:main_window",
        "MainWindow::adjustNoteListAndFiltersSplitterSizes");

    auto splitterSizes = m_pUi->noteListAndFiltersSplitter->sizes();
    int count = splitterSizes.count();
    if (Q_UNLIKELY(count != 2)) {
        ErrorString error(
            QT_TR_NOOP("Internal error: can't properly resize the splitter "
                       "after folding the filter view: wrong number of sizes "
                       "within the splitter"));

        QNWARNING("quentier:main_window", error << "Sizes count: " << count);
        onSetStatusBarText(error.localizedString(), secondsToMilliseconds(30));
        return;
    }

    int filtersPanelHeight = m_pUi->noteFiltersGenericPanel->height();
    int heightDiff = std::max(splitterSizes[0] - filtersPanelHeight, 0);
    splitterSizes[0] = filtersPanelHeight;
    splitterSizes[1] = splitterSizes[1] + heightDiff;
    m_pUi->noteListAndFiltersSplitter->setSizes(splitterSizes);

    // Need to schedule the repaint because otherwise the actions above
    // seem to have no effect
    m_pUi->noteListAndFiltersSplitter->update();
}

void MainWindow::restorePanelColors()
{
    if (!m_pAccount) {
        return;
    }

    ApplicationSettings settings(
        *m_pAccount, preferences::keys::files::userInterface);

    settings.beginGroup(preferences::keys::panelColorsGroup);
    ApplicationSettings::GroupCloser groupCloser(settings);

    QString fontColorName =
        settings.value(preferences::keys::panelFontColor).toString();

    QColor fontColor(fontColorName);
    if (!fontColor.isValid()) {
        fontColor = QColor(Qt::white);
    }

    QString backgroundColorName =
        settings.value(preferences::keys::panelBackgroundColor).toString();

    QColor backgroundColor(backgroundColorName);
    if (!backgroundColor.isValid()) {
        backgroundColor = QColor(Qt::darkGray);
    }

    QLinearGradient gradient(0, 0, 0, 1);

    int rowCount = settings.beginReadArray(
        preferences::keys::panelBackgroundGradientLineCount);

    for (int i = 0; i < rowCount; ++i) {
        settings.setArrayIndex(i);
        bool conversionResult = false;

        double value =
            settings.value(preferences::keys::panelBackgroundGradientLineSize)
                .toDouble(&conversionResult);

        if (Q_UNLIKELY(!conversionResult)) {
            QNWARNING(
                "quentier:main_window",
                "Failed to convert panel background gradient row value to "
                    << "double");

            gradient = QLinearGradient(0, 0, 0, 1);
            break;
        }

        QString colorName =
            settings.value(preferences::keys::panelBackgroundGradientLineColor)
                .toString();

        QColor color(colorName);
        if (!color.isValid()) {
            QNWARNING(
                "quentier:main_window",
                "Failed to convert panel background gradient row color name to "
                    << "valid color: " << colorName);

            gradient = QLinearGradient(0, 0, 0, 1);
            break;
        }

        gradient.setColorAt(value, color);
    }
    settings.endArray();

    bool useBackgroundGradient =
        settings.value(preferences::keys::panelUseBackgroundGradient).toBool();

    for (auto & pPanelStyleController: m_genericPanelStyleControllers) {
        if (useBackgroundGradient) {
            pPanelStyleController->setOverrideColors(fontColor, gradient);
        }
        else {
            pPanelStyleController->setOverrideColors(
                fontColor, backgroundColor);
        }
    }

    for (auto & pPanelStyleController: m_sidePanelStyleControllers) {
        if (useBackgroundGradient) {
            pPanelStyleController->setOverrideColors(fontColor, gradient);
        }
        else {
            pPanelStyleController->setOverrideColors(
                fontColor, backgroundColor);
        }
    }
}

void MainWindow::setupGenericPanelStyleControllers()
{
    auto panels = findChildren<PanelWidget *>(
        QRegularExpression(QStringLiteral("(.*)GenericPanel")));

    m_genericPanelStyleControllers.clear();

    m_genericPanelStyleControllers.reserve(
        static_cast<size_t>(std::max(panels.size(), 0)));

    QString extraStyleSheet;
    for (auto * pPanel: qAsConst(panels)) {
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
    auto panels = findChildren<PanelWidget *>(
        QRegularExpression(QStringLiteral("(.*)SidePanel")));

    m_sidePanelStyleControllers.clear();

    m_sidePanelStyleControllers.reserve(
        static_cast<size_t>(std::max(panels.size(), 0)));

    for (auto * pPanel: qAsConst(panels)) {
        m_sidePanelStyleControllers.emplace_back(
            std::make_unique<SidePanelStyleController>(pPanel));
    }
}

void MainWindow::onSetStatusBarText(QString message, const int durationMsec)
{
    auto * pStatusBar = m_pUi->statusBar;
    pStatusBar->clearMessage();

    if (m_currentStatusBarChildWidget != nullptr) {
        pStatusBar->removeWidget(m_currentStatusBarChildWidget);
        m_currentStatusBarChildWidget = nullptr;
    }

    if (durationMsec == 0) {
        m_currentStatusBarChildWidget = new QLabel(message);
        pStatusBar->addWidget(m_currentStatusBarChildWidget);
    }
    else {
        pStatusBar->showMessage(message, durationMsec);
    }
}

#define DISPATCH_TO_NOTE_EDITOR(MainWindowMethod, NoteEditorMethod)            \
    void MainWindow::MainWindowMethod()                                        \
    {                                                                          \
        QNDEBUG("quentier:main_window", "MainWindow::" #MainWindowMethod);     \
        auto * noteEditorWidget = currentNoteEditorTab();                      \
        if (!noteEditorWidget) {                                               \
            return;                                                            \
        }                                                                      \
        noteEditorWidget->NoteEditorMethod();                                  \
    }

DISPATCH_TO_NOTE_EDITOR(onUndoAction, onUndoAction)
DISPATCH_TO_NOTE_EDITOR(onRedoAction, onRedoAction)
DISPATCH_TO_NOTE_EDITOR(onCopyAction, onCopyAction)
DISPATCH_TO_NOTE_EDITOR(onCutAction, onCutAction)
DISPATCH_TO_NOTE_EDITOR(onPasteAction, onPasteAction)
DISPATCH_TO_NOTE_EDITOR(onNoteTextSelectAllToggled, onSelectAllAction)
DISPATCH_TO_NOTE_EDITOR(onNoteTextBoldToggled, onEditorTextBoldToggled)
DISPATCH_TO_NOTE_EDITOR(onNoteTextItalicToggled, onEditorTextItalicToggled)

DISPATCH_TO_NOTE_EDITOR(
    onNoteTextUnderlineToggled, onEditorTextUnderlineToggled)

DISPATCH_TO_NOTE_EDITOR(
    onNoteTextStrikethroughToggled, onEditorTextStrikethroughToggled)

DISPATCH_TO_NOTE_EDITOR(onNoteTextAlignLeftAction, onEditorTextAlignLeftAction)

DISPATCH_TO_NOTE_EDITOR(
    onNoteTextAlignCenterAction, onEditorTextAlignCenterAction)

DISPATCH_TO_NOTE_EDITOR(
    onNoteTextAlignRightAction, onEditorTextAlignRightAction)

DISPATCH_TO_NOTE_EDITOR(onNoteTextAlignFullAction, onEditorTextAlignFullAction)

DISPATCH_TO_NOTE_EDITOR(
    onNoteTextAddHorizontalLineAction, onEditorTextAddHorizontalLineAction)

DISPATCH_TO_NOTE_EDITOR(
    onNoteTextIncreaseFontSizeAction, onEditorTextIncreaseFontSizeAction)

DISPATCH_TO_NOTE_EDITOR(
    onNoteTextDecreaseFontSizeAction, onEditorTextDecreaseFontSizeAction)

DISPATCH_TO_NOTE_EDITOR(onNoteTextHighlightAction, onEditorTextHighlightAction)

DISPATCH_TO_NOTE_EDITOR(
    onNoteTextIncreaseIndentationAction, onEditorTextIncreaseIndentationAction)

DISPATCH_TO_NOTE_EDITOR(
    onNoteTextDecreaseIndentationAction, onEditorTextDecreaseIndentationAction)

DISPATCH_TO_NOTE_EDITOR(
    onNoteTextInsertUnorderedListAction, onEditorTextInsertUnorderedListAction)

DISPATCH_TO_NOTE_EDITOR(
    onNoteTextInsertOrderedListAction, onEditorTextInsertOrderedListAction)

DISPATCH_TO_NOTE_EDITOR(
    onNoteTextInsertToDoAction, onEditorTextInsertToDoAction)

DISPATCH_TO_NOTE_EDITOR(
    onNoteTextInsertTableDialogAction, onEditorTextInsertTableDialogRequested)

DISPATCH_TO_NOTE_EDITOR(
    onNoteTextEditHyperlinkAction, onEditorTextEditHyperlinkAction)

DISPATCH_TO_NOTE_EDITOR(
    onNoteTextCopyHyperlinkAction, onEditorTextCopyHyperlinkAction)

DISPATCH_TO_NOTE_EDITOR(
    onNoteTextRemoveHyperlinkAction, onEditorTextRemoveHyperlinkAction)

DISPATCH_TO_NOTE_EDITOR(onFindInsideNoteAction, onFindInsideNoteAction)

DISPATCH_TO_NOTE_EDITOR(
    onFindPreviousInsideNoteAction, onFindPreviousInsideNoteAction)

DISPATCH_TO_NOTE_EDITOR(onReplaceInsideNoteAction, onReplaceInsideNoteAction)

#undef DISPATCH_TO_NOTE_EDITOR

void MainWindow::onImportEnexAction()
{
    QNDEBUG("quentier:main_window", "MainWindow::onImportEnexAction");

    if (Q_UNLIKELY(!m_pAccount)) {
        QNDEBUG("quentier:main_window", "No current account, skipping");
        return;
    }

    if (Q_UNLIKELY(!m_pLocalStorageManagerAsync)) {
        QNDEBUG("quentier:main_window", "No local storage manager, skipping");
        return;
    }

    if (Q_UNLIKELY(!m_pTagModel)) {
        QNDEBUG("quentier:main_window", "No tag model, skipping");
        return;
    }

    if (Q_UNLIKELY(!m_pNotebookModel)) {
        QNDEBUG("quentier:main_window", "No notebook model, skipping");
        return;
    }

    auto pEnexImportDialog = std::make_unique<EnexImportDialog>(
        *m_pAccount, *m_pNotebookModel, this);

    pEnexImportDialog->setWindowModality(Qt::WindowModal);
    centerDialog(*pEnexImportDialog);
    if (pEnexImportDialog->exec() != QDialog::Accepted) {
        QNDEBUG("quentier:main_window", "The import of ENEX was cancelled");
        return;
    }

    ErrorString errorDescription;

    QString enexFilePath =
        pEnexImportDialog->importEnexFilePath(&errorDescription);

    if (enexFilePath.isEmpty()) {
        if (errorDescription.isEmpty()) {
            errorDescription.setBase(
                QT_TR_NOOP("Can't import ENEX: internal error, can't retrieve "
                           "ENEX file path"));
        }

        QNDEBUG(
            "quentier:main_window", "Bad ENEX file path: " << errorDescription);

        onSetStatusBarText(
            errorDescription.localizedString(), secondsToMilliseconds(30));
        return;
    }

    QString notebookName = pEnexImportDialog->notebookName(&errorDescription);
    if (notebookName.isEmpty()) {
        if (errorDescription.isEmpty()) {
            errorDescription.setBase(
                QT_TR_NOOP("Can't import ENEX: internal error, can't retrieve "
                           "notebook name"));
        }

        QNDEBUG(
            "quentier:main_window", "Bad notebook name: " << errorDescription);

        onSetStatusBarText(
            errorDescription.localizedString(), secondsToMilliseconds(30));
        return;
    }

    auto * pImporter = new EnexImporter(
        enexFilePath, notebookName, *m_pLocalStorageManagerAsync, *m_pTagModel,
        *m_pNotebookModel, this);

    QObject::connect(
        pImporter, &EnexImporter::enexImportedSuccessfully, this,
        &MainWindow::onEnexImportCompletedSuccessfully);

    QObject::connect(
        pImporter, &EnexImporter::enexImportFailed, this,
        &MainWindow::onEnexImportFailed);

    pImporter->start();
}

void MainWindow::onSynchronizationStarted()
{
    QNINFO("quentier:main_window", "MainWindow::onSynchronizationStarted");

    onSetStatusBarText(tr("Starting the synchronization..."));
    m_syncApiRateLimitExceeded = false;
    m_syncInProgress = true;
    clearSynchronizationCounters();
    startSyncButtonAnimation();
}

void MainWindow::onSynchronizationStopped()
{
    QNINFO("quentier:main_window", "MainWindow::onSynchronizationStopped");

    if (m_syncInProgress) {
        m_syncInProgress = false;
        clearSynchronizationCounters();
        onSetStatusBarText(
            tr("Synchronization was stopped"), secondsToMilliseconds(30));
        scheduleSyncButtonAnimationStop();
        setupRunSyncPeriodicallyTimer();
    }
    // Otherwise sync was stopped after SynchronizationManager failure

    m_syncApiRateLimitExceeded = false;

    m_syncInProgress = false;
    scheduleSyncButtonAnimationStop();
}

void MainWindow::onSynchronizationManagerFailure(ErrorString errorDescription)
{
    QNERROR(
        "quentier:main_window",
        "MainWindow::onSynchronizationManagerFailure: " << errorDescription);

    onSetStatusBarText(
        errorDescription.localizedString(), secondsToMilliseconds(60));

    m_syncInProgress = false;
    clearSynchronizationCounters();
    scheduleSyncButtonAnimationStop();

    setupRunSyncPeriodicallyTimer();

    Q_EMIT stopSynchronization();
}

void MainWindow::onSynchronizationFinished(
    Account account, bool somethingDownloaded, bool somethingSent)
{
    QNINFO(
        "quentier:main_window",
        "MainWindow::onSynchronizationFinished: " << account.name());

    if (somethingDownloaded || somethingSent) {
        onSetStatusBarText(
            tr("Synchronization finished!"), secondsToMilliseconds(5));
    }
    else {
        onSetStatusBarText(
            tr("The account is already in sync with Evernote service"),
            secondsToMilliseconds(5));
    }

    m_syncInProgress = false;
    clearSynchronizationCounters();
    scheduleSyncButtonAnimationStop();

    setupRunSyncPeriodicallyTimer();

    QNINFO(
        "quentier:main_window",
        "Synchronization finished for user "
            << account.name() << ", id " << account.id()
            << ", something downloaded = "
            << (somethingDownloaded ? "true" : "false")
            << ", something sent = " << (somethingSent ? "true" : "false"));
}

void MainWindow::onAuthenticationFinished(
    bool success, ErrorString errorDescription, Account account)
{
    QNINFO(
        "quentier:main_window",
        "MainWindow::onAuthenticationFinished: "
            << "success = " << (success ? "true" : "false")
            << ", error description = " << errorDescription
            << ", account = " << account.name());

    bool wasPendingNewEvernoteAccountAuthentication =
        m_pendingNewEvernoteAccountAuthentication;

    m_pendingNewEvernoteAccountAuthentication = false;

    bool wasPendingCurrentEvernoteAccountAuthentication =
        m_pendingCurrentEvernoteAccountAuthentication;

    m_pendingCurrentEvernoteAccountAuthentication = false;

    if (!success) {
        // Restore the network proxy which was active before the authentication
        QNetworkProxy::setApplicationProxy(
            m_applicationProxyBeforeNewEvernoteAccountAuthenticationRequest);

        m_applicationProxyBeforeNewEvernoteAccountAuthenticationRequest =
            QNetworkProxy(QNetworkProxy::NoProxy);

        onSetStatusBarText(
            tr("Couldn't authenticate the Evernote user") +
                QStringLiteral(": ") + errorDescription.localizedString(),
            secondsToMilliseconds(30));

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

void MainWindow::onRateLimitExceeded(qint32 secondsToWait)
{
    QNINFO(
        "quentier:main_window",
        "MainWindow::onRateLimitExceeded: "
            << "seconds to wait = " << secondsToWait);

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

    onSetStatusBarText(
        tr("The synchronization has reached Evernote API rate "
           "limit, it will continue automatically at approximately") +
            QStringLiteral(" ") + dateTimeToShow,
        secondsToMilliseconds(60));

    m_animatedSyncButtonIcon.setPaused(true);

    QNINFO(
        "quentier:main_window",
        "Evernote API rate limit exceeded, need to wait for "
            << secondsToWait
            << " seconds, the synchronization will continue at "
            << dateTimeToShow);

    m_syncApiRateLimitExceeded = true;
}

void MainWindow::onRemoteToLocalSyncDone(bool somethingDownloaded)
{
    QNTRACE("quentier:main_window", "MainWindow::onRemoteToLocalSyncDone");

    QNINFO(
        "quentier:main_window",
        "Remote to local sync done: "
            << (somethingDownloaded ? "received all updates from Evernote"
                                    : "no updates found on Evernote side"));

    if (somethingDownloaded) {
        onSetStatusBarText(
            tr("Received all updates from Evernote servers, "
               "sending local changes"));
    }
    else {
        onSetStatusBarText(
            tr("No updates found on Evernote servers, sending "
               "local changes"));
    }
}

void MainWindow::onSyncChunksDownloadProgress(
    qint32 highestDownloadedUsn, qint32 highestServerUsn,
    qint32 lastPreviousUsn)
{
    QNINFO(
        "quentier:main_window",
        "MainWindow::onSyncChunksDownloadProgress: "
            << "highest downloaded USN = " << highestDownloadedUsn
            << ", highest server USN = " << highestServerUsn
            << ", last previous USN = " << lastPreviousUsn);

    if (Q_UNLIKELY(
            (highestServerUsn <= lastPreviousUsn) ||
            (highestDownloadedUsn <= lastPreviousUsn)))
    {
        QNWARNING(
            "quentier:main_window",
            "Received incorrect sync chunks "
                << "download progress state: highest downloaded USN = "
                << highestDownloadedUsn
                << ", highest server USN = " << highestServerUsn
                << ", last previous USN = " << lastPreviousUsn);
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
    QNINFO("quentier:main_window", "MainWindow::onSyncChunksDownloaded");

    onSetStatusBarText(
        tr("Downloaded sync chunks, parsing tags, notebooks and "
           "saved searches from them...") +
        QStringLiteral("..."));

    m_syncChunksDownloadedTimestamp = QDateTime::currentMSecsSinceEpoch();
}

void MainWindow::onSyncChunksDataProcessingProgress(
    const ISyncChunksDataCountersPtr counters)
{
    Q_ASSERT(counters);

    const double percentage = computeSyncChunkDataProcessingProgress(*counters);

    // On small accounts stuff from sync chunks is processed rather quickly.
    // So for the first few seconds won't show progress notifications
    // to prevent unreadable text blinking
    const auto now = QDateTime::currentMSecsSinceEpoch();
    if (!m_syncChunksDownloadedTimestamp ||
        now - m_syncChunksDownloadedTimestamp < 2000)
    {
        QNDEBUG(
            "quentier:main_window",
            "MainWindow::onSyncChunksDataProcessingProgress: "
                << *counters << "\nPercentage = " << percentage);
        return;
    }

    // It's useful to log this event at INFO level but not too often
    // or it would clutter the log and slow the app down
    if (percentage - m_lastSyncChunksDataProcessingProgressPercentage > 10.0) {
        QNINFO(
            "quentier:main_window",
            "MainWindow::onSyncChunksDataProcessingProgress: "
                << *counters << "\nPercentage = " << percentage);

        m_lastSyncChunksDataProcessingProgressPercentage = percentage;
    }
    else {
        QNDEBUG(
            "quentier:main_window",
            "MainWindow::onSyncChunksDataProcessingProgress: "
                << *counters << "\nPercentage = " << percentage);
    }

    onSetStatusBarText(
        tr("Processing data from sync chunks") + QStringLiteral(": ") +
        QString::number(percentage, 'f', 2));
}

void MainWindow::onNotesDownloadProgress(
    quint32 notesDownloaded, quint32 totalNotesToDownload)
{
    double percentage = static_cast<double>(notesDownloaded) /
        static_cast<double>(totalNotesToDownload) * 100.0;

    percentage = std::min(percentage, 100.0);

    // It's useful to log this event at INFO level but not too often
    // or it would clutter the log and slow the app down
    if (percentage - m_lastSyncNotesDownloadedPercentage > 10.0) {
        QNINFO(
            "quentier:main_window",
            "MainWindow::onNotesDownloadProgress: "
                << "notes downloaded = " << notesDownloaded
                << ", total notes to download = " << totalNotesToDownload);

        m_lastSyncNotesDownloadedPercentage = percentage;
    }
    else {
        QNDEBUG(
            "quentier:main_window",
            "MainWindow::onNotesDownloadProgress: "
                << "notes downloaded = " << notesDownloaded
                << ", total notes to download = " << totalNotesToDownload);
    }

    onSetStatusBarText(
        tr("Downloading notes") + QStringLiteral(": ") +
        QString::number(notesDownloaded) + QStringLiteral(" ") + tr("of") +
        QStringLiteral(" ") + QString::number(totalNotesToDownload));
}

void MainWindow::onResourcesDownloadProgress(
    quint32 resourcesDownloaded, quint32 totalResourcesToDownload)
{
    double percentage = static_cast<double>(resourcesDownloaded) /
        static_cast<double>(totalResourcesToDownload) * 100.0;

    percentage = std::min(percentage, 100.0);

    // It's useful to log this event at INFO level but not too often
    // or it would clutter the log and slow the app down
    if (percentage - m_lastSyncResourcesDownloadedPercentage > 10.0) {
        QNINFO(
            "quentier:main_window",
            "MainWindow::onResourcesDownloadProgress: "
                << "resources downloaded = " << resourcesDownloaded
                << ", total resources to download = "
                << totalResourcesToDownload);

        m_lastSyncResourcesDownloadedPercentage = percentage;
    }
    else {
        QNDEBUG(
            "quentier:main_window",
            "MainWindow::onResourcesDownloadProgress: "
                << "resources downloaded = " << resourcesDownloaded
                << ", total resources to download = "
                << totalResourcesToDownload);
    }

    onSetStatusBarText(
        tr("Downloading attachments") + QStringLiteral(": ") +
        QString::number(resourcesDownloaded) + QStringLiteral(" ") + tr("of") +
        QStringLiteral(" ") + QString::number(totalResourcesToDownload));
}

void MainWindow::onLinkedNotebookSyncChunksDownloadProgress(
    qint32 highestDownloadedUsn, qint32 highestServerUsn,
    qint32 lastPreviousUsn, LinkedNotebook linkedNotebook)
{
    QNINFO(
        "quentier:main_window",
        "MainWindow::onLinkedNotebookSyncChunksDownloadProgress: "
            << "highest downloaded USN = " << highestDownloadedUsn
            << ", highest server USN = " << highestServerUsn
            << ", last previous USN = " << lastPreviousUsn
            << ", linked notebook = " << linkedNotebook);

    if (Q_UNLIKELY(
            (highestServerUsn <= lastPreviousUsn) ||
            (highestDownloadedUsn <= lastPreviousUsn)))
    {
        QNWARNING(
            "quentier:main_window",
            "Received incorrect sync chunks "
                << "download progress state: highest downloaded USN = "
                << highestDownloadedUsn
                << ", highest server USN = " << highestServerUsn
                << ", last previous USN = " << lastPreviousUsn
                << ", linked notebook: " << linkedNotebook);
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
    QNINFO(
        "quentier:main_window",
        "MainWindow::onLinkedNotebooksSyncChunksDownloaded");

    onSetStatusBarText(tr("Downloaded the sync chunks from linked notebooks"));

    m_linkedNotebookSyncChunksDownloadedTimestamp =
        QDateTime::currentMSecsSinceEpoch();
}

void MainWindow::onLinkedNotebookSyncChunksDataProcessingProgress(
    ISyncChunksDataCountersPtr counters)
{
    Q_ASSERT(counters);

    const double percentage = computeSyncChunkDataProcessingProgress(*counters);

    // On small accounts stuff from sync chunks is processed rather quickly.
    // So for the first few seconds won't show progress notifications
    // to prevent unreadable text blinking
    const auto now = QDateTime::currentMSecsSinceEpoch();
    if (!m_linkedNotebookSyncChunksDownloadedTimestamp ||
        now - m_linkedNotebookSyncChunksDownloadedTimestamp < 2000)
    {
        QNDEBUG(
            "quentier:main_window",
            "MainWindow::onLinkedNotebookSyncChunksDataProcessingProgress: "
                << *counters << "\nPercentage = " << percentage);
        return;
    }

    // It's useful to log this event at INFO level but not too often
    // or it would clutter the log and slow the app down
    if (percentage -
            m_lastLinkedNotebookSyncChunksDataProcessingProgressPercentage >
        10.0)
    {
        QNINFO(
            "quentier:main_window",
            "MainWindow::onLinkedNotebookSyncChunksDataProcessingProgress: "
                << *counters << "\nPercentage = " << percentage);

        m_lastLinkedNotebookSyncChunksDataProcessingProgressPercentage =
            percentage;
    }
    else {
        QNDEBUG(
            "quentier:main_window",
            "MainWindow::onLinkedNotebookSyncChunksDataProcessingProgress: "
                << *counters << "\nPercentage = " << percentage);
    }
}

void MainWindow::onLinkedNotebooksNotesDownloadProgress(
    quint32 notesDownloaded, quint32 totalNotesToDownload)
{
    double percentage = static_cast<double>(notesDownloaded) /
        static_cast<double>(totalNotesToDownload) * 100.0;

    percentage = std::min(percentage, 100.0);

    // It's useful to log this event at INFO level but not too often
    // or it would clutter the log and slow the app down
    if (percentage - m_lastSyncLinkedNotebookNotesDownloadedPercentage > 0.1) {
        QNINFO(
            "quentier:main_window",
            "MainWindow::onLinkedNotebooksNotesDownloadProgress: "
                << "notes downloaded = " << notesDownloaded
                << ", total notes to download = " << totalNotesToDownload);

        m_lastSyncLinkedNotebookNotesDownloadedPercentage = percentage;
    }
    else {
        QNDEBUG(
            "quentier:main_window",
            "MainWindow::onLinkedNotebooksNotesDownloadProgress: "
                << "notes downloaded = " << notesDownloaded
                << ", total notes to download = " << totalNotesToDownload);
    }

    onSetStatusBarText(
        tr("Downloading notes from linked notebooks") + QStringLiteral(": ") +
        QString::number(notesDownloaded) + QStringLiteral(" ") + tr("of") +
        QStringLiteral(" ") + QString::number(totalNotesToDownload));
}

void MainWindow::onRemoteToLocalSyncStopped()
{
    QNDEBUG("quentier:main_window", "MainWindow::onRemoteToLocalSyncStopped");
    onSynchronizationStopped();
}

void MainWindow::onSendLocalChangesStopped()
{
    QNDEBUG("quentier:main_window", "MainWindow::onSendLocalChangesStopped");
    onSynchronizationStopped();
}

void MainWindow::onEvernoteAccountAuthenticationRequested(
    QString host, QNetworkProxy proxy)
{
    QNDEBUG(
        "quentier:main_window",
        "MainWindow::onEvernoteAccountAuthenticationRequested: host = "
            << host << ", proxy type = " << proxy.type() << ", proxy host = "
            << proxy.hostName() << ", proxy port = " << proxy.port()
            << ", proxy user = " << proxy.user());

    m_synchronizationManagerHost = host;
    setupSynchronizationManager();

    // Set the proxy specified within the slot argument but remember the
    // previous application proxy so that it can be restored in case of
    // authentication failure
    m_applicationProxyBeforeNewEvernoteAccountAuthenticationRequest =
        QNetworkProxy::applicationProxy();

    QNetworkProxy::setApplicationProxy(proxy);

    m_pendingNewEvernoteAccountAuthentication = true;
    Q_EMIT authenticate();
}

void MainWindow::onNoteTextSpellCheckToggled()
{
    QNDEBUG("quentier:main_window", "MainWindow::onNoteTextSpellCheckToggled");

    auto * noteEditorWidget = currentNoteEditorTab();
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
    QNDEBUG("quentier:main_window", "MainWindow::onShowNoteSource");

    auto * pNoteEditorWidget = currentNoteEditorTab();
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
    QNDEBUG("quentier:main_window", "MainWindow::onSaveNoteAction");

    auto * pNoteEditorWidget = currentNoteEditorTab();
    if (!pNoteEditorWidget) {
        return;
    }

    pNoteEditorWidget->onSaveNoteAction();
}

void MainWindow::onNewNotebookCreationRequested()
{
    QNDEBUG(
        "quentier:main_window", "MainWindow::onNewNotebookCreationRequested");

    if (Q_UNLIKELY(!m_pNotebookModel)) {
        ErrorString error(
            QT_TR_NOOP("Can't create a new notebook: no notebook model is set "
                       "up"));
        QNWARNING("quentier:main_window", error);
        onSetStatusBarText(error.localizedString(), secondsToMilliseconds(30));
        return;
    }

    auto pAddNotebookDialog =
        std::make_unique<AddOrEditNotebookDialog>(m_pNotebookModel, this);

    pAddNotebookDialog->setWindowModality(Qt::WindowModal);
    centerDialog(*pAddNotebookDialog);
    Q_UNUSED(pAddNotebookDialog->exec())
}

void MainWindow::onRemoveNotebookButtonPressed()
{
    QNDEBUG(
        "quentier:main_window", "MainWindow::onRemoveNotebookButtonPressed");

    m_pUi->notebooksTreeView->deleteSelectedItem();
}

void MainWindow::onNotebookInfoButtonPressed()
{
    QNDEBUG("quentier:main_window", "MainWindow::onNotebookInfoButtonPressed");

    auto index = m_pUi->notebooksTreeView->currentlySelectedItemIndex();

    auto * pNotebookModelItemInfoWidget =
        new NotebookModelItemInfoWidget(index, this);

    showInfoWidget(pNotebookModelItemInfoWidget);
}

void MainWindow::onNewTagCreationRequested()
{
    QNDEBUG("quentier:main_window", "MainWindow::onNewTagCreationRequested");

    if (Q_UNLIKELY(!m_pTagModel)) {
        ErrorString error(
            QT_TR_NOOP("Can't create a new tag: no tag model is set up"));
        QNWARNING("quentier:main_window", error);
        onSetStatusBarText(error.localizedString(), secondsToMilliseconds(30));
        return;
    }

    auto pAddTagDialog =
        std::make_unique<AddOrEditTagDialog>(m_pTagModel, this);

    pAddTagDialog->setWindowModality(Qt::WindowModal);
    centerDialog(*pAddTagDialog);
    Q_UNUSED(pAddTagDialog->exec())
}

void MainWindow::onRemoveTagButtonPressed()
{
    QNDEBUG("quentier:main_window", "MainWindow::onRemoveTagButtonPressed");
    m_pUi->tagsTreeView->deleteSelectedItem();
}

void MainWindow::onTagInfoButtonPressed()
{
    QNDEBUG("quentier:main_window", "MainWindow::onTagInfoButtonPressed");

    auto index = m_pUi->tagsTreeView->currentlySelectedItemIndex();
    auto * pTagModelItemInfoWidget = new TagModelItemInfoWidget(index, this);
    showInfoWidget(pTagModelItemInfoWidget);
}

void MainWindow::onNewSavedSearchCreationRequested()
{
    QNDEBUG(
        "quentier:main_window",
        "MainWindow::onNewSavedSearchCreationRequested");

    if (Q_UNLIKELY(!m_pSavedSearchModel)) {
        ErrorString error(
            QT_TR_NOOP("Can't create a new saved search: no saved "
                       "search model is set up"));
        QNWARNING("quentier:main_window", error);
        onSetStatusBarText(error.localizedString(), secondsToMilliseconds(30));
        return;
    }

    auto pAddSavedSearchDialog =
        std::make_unique<AddOrEditSavedSearchDialog>(m_pSavedSearchModel, this);

    pAddSavedSearchDialog->setWindowModality(Qt::WindowModal);
    centerDialog(*pAddSavedSearchDialog);
    Q_UNUSED(pAddSavedSearchDialog->exec())
}

void MainWindow::onRemoveSavedSearchButtonPressed()
{
    QNDEBUG(
        "quentier:main_window", "MainWindow::onRemoveSavedSearchButtonPressed");

    m_pUi->savedSearchesItemView->deleteSelectedItem();
}

void MainWindow::onSavedSearchInfoButtonPressed()
{
    QNDEBUG(
        "quentier:main_window", "MainWindow::onSavedSearchInfoButtonPressed");

    auto index = m_pUi->savedSearchesItemView->currentlySelectedItemIndex();

    auto * pSavedSearchModelItemInfoWidget =
        new SavedSearchModelItemInfoWidget(index, this);

    showInfoWidget(pSavedSearchModelItemInfoWidget);
}

void MainWindow::onUnfavoriteItemButtonPressed()
{
    QNDEBUG(
        "quentier:main_window", "MainWindow::onUnfavoriteItemButtonPressed");

    m_pUi->favoritesTableView->unfavoriteSelectedItems();
}

void MainWindow::onFavoritedItemInfoButtonPressed()
{
    QNDEBUG(
        "quentier:main_window", "MainWindow::onFavoritedItemInfoButtonPressed");

    auto index = m_pUi->favoritesTableView->currentlySelectedItemIndex();
    if (!index.isValid()) {
        Q_UNUSED(informationMessageBox(
            this, tr("Not exactly one favorited item is selected"),
            tr("Please select the only one favorited item to see its detailed "
               "info")));
        return;
    }

    auto * pFavoritesModel =
        qobject_cast<FavoritesModel *>(m_pUi->favoritesTableView->model());

    if (Q_UNLIKELY(!pFavoritesModel)) {
        Q_UNUSED(internalErrorMessageBox(
            this,
            tr("Failed to cast the favorited table view's model to favorites "
               "model")))
        return;
    }

    const auto * pItem = pFavoritesModel->itemAtRow(index.row());
    if (Q_UNLIKELY(!pItem)) {
        Q_UNUSED(internalErrorMessageBox(
            this,
            tr("Favorites model returned null pointer to favorited item for "
               "the selected index")))
        return;
    }

    switch (pItem->type()) {
    case FavoritesModelItem::Type::Note:
        Q_EMIT noteInfoDialogRequested(pItem->localUid());
        break;
    case FavoritesModelItem::Type::Notebook:
    {
        if (Q_LIKELY(m_pNotebookModel)) {
            auto notebookIndex =
                m_pNotebookModel->indexForLocalUid(pItem->localUid());

            auto * pNotebookItemInfoWidget =
                new NotebookModelItemInfoWidget(notebookIndex, this);

            showInfoWidget(pNotebookItemInfoWidget);
        }
        else {
            Q_UNUSED(internalErrorMessageBox(
                this, tr("No notebook model exists at the moment")))
        }
        break;
    }
    case FavoritesModelItem::Type::SavedSearch:
    {
        if (Q_LIKELY(m_pSavedSearchModel)) {
            auto savedSearchIndex =
                m_pSavedSearchModel->indexForLocalUid(pItem->localUid());

            auto * pSavedSearchItemInfoWidget =
                new SavedSearchModelItemInfoWidget(savedSearchIndex, this);

            showInfoWidget(pSavedSearchItemInfoWidget);
        }
        else {
            Q_UNUSED(
                internalErrorMessageBox(
                    this, tr("No saved search model exists at the moment"));)
        }
        break;
    }
    case FavoritesModelItem::Type::Tag:
    {
        if (Q_LIKELY(m_pTagModel)) {
            auto tagIndex = m_pTagModel->indexForLocalUid(pItem->localUid());

            auto * pTagItemInfoWidget =
                new TagModelItemInfoWidget(tagIndex, this);

            showInfoWidget(pTagItemInfoWidget);
        }
        else {
            Q_UNUSED(internalErrorMessageBox(
                         this, tr("No tag model exists at the moment"));)
        }
        break;
    }
    default:
    {
        QString type;
        QDebug dbg(&type);
        dbg << pItem->type();

        Q_UNUSED(internalErrorMessageBox(
            this,
            tr("Incorrect favorited item type") + QStringLiteral(": ") + type))
    } break;
    }
}

void MainWindow::onRestoreDeletedNoteButtonPressed()
{
    QNDEBUG(
        "quentier:main_window",
        "MainWindow::onRestoreDeletedNoteButtonPressed");

    m_pUi->deletedNotesTableView->restoreCurrentlySelectedNote();
}

void MainWindow::onDeleteNotePermanentlyButtonPressed()
{
    QNDEBUG(
        "quentier:main_window",
        "MainWindow::onDeleteNotePermanentlyButtonPressed");

    m_pUi->deletedNotesTableView->deleteCurrentlySelectedNotePermanently();
}

void MainWindow::onDeletedNoteInfoButtonPressed()
{
    QNDEBUG(
        "quentier:main_window", "MainWindow::onDeletedNoteInfoButtonPressed");

    m_pUi->deletedNotesTableView->showCurrentlySelectedNoteInfo();
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
    QNDEBUG(
        "quentier:main_window",
        "MainWindow::onFiltersViewTogglePushButtonPressed");

    m_filtersViewExpanded = !m_filtersViewExpanded;

    if (m_filtersViewExpanded) {
        expandFiltersView();
    }
    else {
        foldFiltersView();
    }

    ApplicationSettings appSettings(
        *m_pAccount, preferences::keys::files::userInterface);

    appSettings.beginGroup(QStringLiteral("FiltersView"));

    appSettings.setValue(
        FILTERS_VIEW_STATUS_KEY, QVariant(m_filtersViewExpanded));

    appSettings.endGroup();
}

void MainWindow::onShowPreferencesDialogAction()
{
    QNDEBUG(
        "quentier:main_window", "MainWindow::onShowPreferencesDialogAction");

    auto * pExistingPreferencesDialog = findChild<PreferencesDialog *>();
    if (pExistingPreferencesDialog) {
        QNDEBUG(
            "quentier:main_window",
            "Preferences dialog already exists, "
                << "won't show another one");
        return;
    }

    auto menus = m_pUi->menuBar->findChildren<QMenu *>();
    ActionsInfo actionsInfo(menus);

    auto pPreferencesDialog = std::make_unique<PreferencesDialog>(
        *m_pAccountManager, m_shortcutManager, *m_pSystemTrayIconManager,
        actionsInfo, this);

    pPreferencesDialog->setWindowModality(Qt::WindowModal);
    centerDialog(*pPreferencesDialog);
    connectToPreferencesDialogSignals(*pPreferencesDialog);
    Q_UNUSED(pPreferencesDialog->exec())
}

void MainWindow::onNoteSortingModeChanged(int index)
{
    QNDEBUG(
        "quentier:main_window",
        "MainWindow::onNoteSortingModeChanged: "
            << "index = " << index);

    persistChosenNoteSortingMode(index);

    if (Q_UNLIKELY(!m_pNoteModel)) {
        QNDEBUG("quentier:main_window", "No note model, ignoring the change");
        return;
    }

    switch (index) {
    case NoteModel::NoteSortingMode::CreatedAscending:
        m_pNoteModel->sort(
            NoteModel::Columns::CreationTimestamp, Qt::AscendingOrder);
        break;
    case NoteModel::NoteSortingMode::CreatedDescending:
        m_pNoteModel->sort(
            NoteModel::Columns::CreationTimestamp, Qt::DescendingOrder);
        break;
    case NoteModel::NoteSortingMode::ModifiedAscending:
        m_pNoteModel->sort(
            NoteModel::Columns::ModificationTimestamp, Qt::AscendingOrder);
        break;
    case NoteModel::NoteSortingMode::ModifiedDescending:
        m_pNoteModel->sort(
            NoteModel::Columns::ModificationTimestamp, Qt::DescendingOrder);
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
        ErrorString error(
            QT_TR_NOOP("Internal error: got unknown note sorting order, "
                       "fallback to the default"));

        QNWARNING(
            "quentier:main_window",
            error << ", sorting mode index = " << index);

        onSetStatusBarText(error.localizedString(), secondsToMilliseconds(30));

        m_pNoteModel->sort(
            NoteModel::Columns::CreationTimestamp, Qt::AscendingOrder);

        break;
    }
    }
}

void MainWindow::onNewNoteCreationRequested()
{
    QNDEBUG("quentier:main_window", "MainWindow::onNewNoteCreationRequested");
    createNewNote(NoteEditorTabsAndWindowsCoordinator::NoteEditorMode::Any);
}

void MainWindow::onToggleThumbnailsPreference(QString noteLocalUid)
{
    QNDEBUG(
        "quentier:main_window",
        "MainWindow::onToggleThumbnailsPreference: "
            << "note local uid = " << noteLocalUid);

    bool toggleForAllNotes = noteLocalUid.isEmpty();
    if (toggleForAllNotes) {
        toggleShowNoteThumbnails();
    }
    else {
        toggleHideNoteThumbnail(noteLocalUid);
    }

    onShowNoteThumbnailsPreferenceChanged();
}

void MainWindow::onCopyInAppLinkNoteRequested(
    QString noteLocalUid, QString noteGuid)
{
    QNDEBUG(
        "quentier:main_window",
        "MainWindow::onCopyInAppLinkNoteRequested: "
            << "note local uid = " << noteLocalUid
            << ", note guid = " << noteGuid);

    if (noteGuid.isEmpty()) {
        QNDEBUG(
            "quentier:main_window",
            "Can't copy the in-app note link: note "
                << "guid is empty");
        return;
    }

    if (Q_UNLIKELY(m_pAccount.isNull())) {
        QNDEBUG(
            "quentier:main_window",
            "Can't copy the in-app note link: no "
                << "current account");
        return;
    }

    if (Q_UNLIKELY(m_pAccount->type() != Account::Type::Evernote)) {
        QNDEBUG(
            "quentier:main_window",
            "Can't copy the in-app note link: "
                << "the current account is not of Evernote type");
        return;
    }

    auto id = m_pAccount->id();
    if (Q_UNLIKELY(id < 0)) {
        QNDEBUG(
            "quentier:main_window",
            "Can't copy the in-app note link: "
                << "the current account's id is negative");
        return;
    }

    QString shardId = m_pAccount->shardId();
    if (shardId.isEmpty()) {
        QNDEBUG(
            "quentier:main_window",
            "Can't copy the in-app note link: "
                << "the current account's shard id is empty");
        return;
    }

    QString urlString = QStringLiteral("evernote:///view/") +
        QString::number(id) + QStringLiteral("/") + shardId +
        QStringLiteral("/") + noteGuid + QStringLiteral("/") + noteGuid;

    auto * pClipboard = QApplication::clipboard();
    if (pClipboard) {
        QNTRACE(
            "quentier:main_window",
            "Setting the composed in-app note URL "
                << "to the clipboard: " << urlString);
        pClipboard->setText(urlString);
    }
}

void MainWindow::onFavoritedNoteSelected(QString noteLocalUid)
{
    QNDEBUG(
        "quentier:main_window",
        "MainWindow::onFavoritedNoteSelected: " << noteLocalUid);

    if (Q_UNLIKELY(!m_pNoteEditorTabsAndWindowsCoordinator)) {
        QNDEBUG(
            "quentier:main_window",
            "No note editor tabs and windows coordinator, skipping");
        return;
    }

    m_pNoteEditorTabsAndWindowsCoordinator->addNote(noteLocalUid);
}

void MainWindow::onCurrentNoteInListChanged(QString noteLocalUid)
{
    QNDEBUG(
        "quentier:main_window",
        "MainWindow::onCurrentNoteInListChanged: " << noteLocalUid);

    m_pNoteEditorTabsAndWindowsCoordinator->addNote(noteLocalUid);
}

void MainWindow::onOpenNoteInSeparateWindow(QString noteLocalUid)
{
    QNDEBUG(
        "quentier:main_window",
        "MainWindow::onOpenNoteInSeparateWindow: " << noteLocalUid);

    m_pNoteEditorTabsAndWindowsCoordinator->addNote(
        noteLocalUid,
        NoteEditorTabsAndWindowsCoordinator::NoteEditorMode::Window);
}

void MainWindow::onDeleteCurrentNoteButtonPressed()
{
    QNDEBUG(
        "quentier:main_window", "MainWindow::onDeleteCurrentNoteButtonPressed");

    if (Q_UNLIKELY(!m_pNoteModel)) {
        ErrorString errorDescription(
            QT_TR_NOOP("Can't delete the current note: internal error, no note "
                       "model"));

        QNDEBUG("quentier:main_window", errorDescription);
        onSetStatusBarText(
            errorDescription.localizedString(), secondsToMilliseconds(30));
        return;
    }

    auto * pNoteEditorWidget = currentNoteEditorTab();
    if (!pNoteEditorWidget) {
        ErrorString errorDescription(
            QT_TR_NOOP("Can't delete the current note: no note editor tabs"));
        QNDEBUG("quentier:main_window", errorDescription);
        onSetStatusBarText(
            errorDescription.localizedString(), secondsToMilliseconds(30));
        return;
    }

    ErrorString error;

    bool res =
        m_pNoteModel->deleteNote(pNoteEditorWidget->noteLocalUid(), error);

    if (Q_UNLIKELY(!res)) {
        ErrorString errorDescription(
            QT_TR_NOOP("Can't delete the current note"));
        errorDescription.appendBase(error.base());
        errorDescription.appendBase(error.additionalBases());
        errorDescription.details() = error.details();
        QNDEBUG("quentier:main_window", errorDescription);
        onSetStatusBarText(
            errorDescription.localizedString(), secondsToMilliseconds(30));
        return;
    }
}

void MainWindow::onCurrentNoteInfoRequested()
{
    QNDEBUG("quentier:main_window", "MainWindow::onCurrentNoteInfoRequested");

    auto * pNoteEditorWidget = currentNoteEditorTab();
    if (!pNoteEditorWidget) {
        ErrorString errorDescription(
            QT_TR_NOOP("Can't show note info: no note editor tabs"));
        QNDEBUG("quentier:main_window", errorDescription);
        onSetStatusBarText(
            errorDescription.localizedString(), secondsToMilliseconds(30));
        return;
    }

    Q_EMIT noteInfoDialogRequested(pNoteEditorWidget->noteLocalUid());
}

void MainWindow::onCurrentNotePrintRequested()
{
    QNDEBUG("quentier:main_window", "MainWindow::onCurrentNotePrintRequested");

    auto * pNoteEditorWidget = currentNoteEditorTab();
    if (!pNoteEditorWidget) {
        ErrorString errorDescription(
            QT_TR_NOOP("Can't print note: no note editor tabs"));
        QNDEBUG("quentier:main_window", errorDescription);
        onSetStatusBarText(
            errorDescription.localizedString(), secondsToMilliseconds(30));
        return;
    }

    ErrorString errorDescription;
    bool res = pNoteEditorWidget->printNote(errorDescription);
    if (!res) {
        if (errorDescription.isEmpty()) {
            return;
        }

        QNDEBUG("quentier:main_window", errorDescription);
        onSetStatusBarText(
            errorDescription.localizedString(), secondsToMilliseconds(30));
    }
}

void MainWindow::onCurrentNotePdfExportRequested()
{
    QNDEBUG(
        "quentier:main_window", "MainWindow::onCurrentNotePdfExportRequested");

    auto * pNoteEditorWidget = currentNoteEditorTab();
    if (!pNoteEditorWidget) {
        ErrorString errorDescription(
            QT_TR_NOOP("Can't export note to pdf: no note editor tabs"));
        QNDEBUG("quentier:main_window", errorDescription);
        onSetStatusBarText(
            errorDescription.localizedString(), secondsToMilliseconds(30));
        return;
    }

    ErrorString errorDescription;
    bool res = pNoteEditorWidget->exportNoteToPdf(errorDescription);
    if (!res) {
        if (errorDescription.isEmpty()) {
            return;
        }

        QNDEBUG("quentier:main_window", errorDescription);
        onSetStatusBarText(
            errorDescription.localizedString(), secondsToMilliseconds(30));
    }
}

void MainWindow::onExportNotesToEnexRequested(QStringList noteLocalUids)
{
    QNDEBUG(
        "quentier:main_window",
        "MainWindow::onExportNotesToEnexRequested: "
            << noteLocalUids.join(QStringLiteral(", ")));

    if (Q_UNLIKELY(noteLocalUids.isEmpty())) {
        QNDEBUG(
            "quentier:main_window",
            "The list of note local uids to export "
                << "is empty");
        return;
    }

    if (Q_UNLIKELY(!m_pAccount)) {
        QNDEBUG("quentier:main_window", "No current account, skipping");
        return;
    }

    if (Q_UNLIKELY(!m_pLocalStorageManagerAsync)) {
        QNDEBUG("quentier:main_window", "No local storage manager, skipping");
        return;
    }

    if (Q_UNLIKELY(!m_pNoteEditorTabsAndWindowsCoordinator)) {
        QNDEBUG(
            "quentier:main_window",
            "No note editor tabs and windows "
                << "coordinator, skipping");
        return;
    }

    if (Q_UNLIKELY(!m_pTagModel)) {
        QNDEBUG("quentier:main_window", "No tag model, skipping");
        return;
    }

    ApplicationSettings appSettings(
        *m_pAccount, preferences::keys::files::userInterface);

    appSettings.beginGroup(preferences::keys::noteEditorGroup);

    QString lastExportNoteToEnexPath =
        appSettings.value(preferences::keys::lastExportNotesToEnexPath)
            .toString();

    appSettings.endGroup();

    if (lastExportNoteToEnexPath.isEmpty()) {
        lastExportNoteToEnexPath = documentsPath();
    }

    auto pExportEnexDialog =
        std::make_unique<EnexExportDialog>(*m_pAccount, this);

    pExportEnexDialog->setWindowModality(Qt::WindowModal);
    centerDialog(*pExportEnexDialog);
    if (pExportEnexDialog->exec() != QDialog::Accepted) {
        QNDEBUG("quentier:main_window", "Enex export was not confirmed");
        return;
    }

    QString enexFilePath = pExportEnexDialog->exportEnexFilePath();

    QFileInfo enexFileInfo(enexFilePath);
    if (enexFileInfo.exists()) {
        if (!enexFileInfo.isWritable()) {
            QNINFO(
                "quentier:main_window",
                "Chosen ENEX export file is not "
                    << "writable: " << enexFilePath);

            onSetStatusBarText(
                tr("The file selected for ENEX export is not writable") +
                QStringLiteral(": ") + enexFilePath);

            return;
        }
    }
    else {
        QDir enexFileDir = enexFileInfo.absoluteDir();
        if (!enexFileDir.exists()) {
            bool res = enexFileDir.mkpath(enexFileInfo.absolutePath());
            if (!res) {
                QNDEBUG(
                    "quentier:main_window",
                    "Failed to create folder for "
                        << "the selected ENEX file");

                onSetStatusBarText(
                    tr("Could not create the folder for the selected ENEX "
                       "file") +
                        QStringLiteral(": ") + enexFilePath,
                    secondsToMilliseconds(30));

                return;
            }
        }
    }

    auto * pExporter = new EnexExporter(
        *m_pLocalStorageManagerAsync, *m_pNoteEditorTabsAndWindowsCoordinator,
        *m_pTagModel, this);

    pExporter->setTargetEnexFilePath(enexFilePath);
    pExporter->setIncludeTags(pExportEnexDialog->exportTags());
    pExporter->setNoteLocalUids(noteLocalUids);

    QObject::connect(
        pExporter, &EnexExporter::notesExportedToEnex, this,
        &MainWindow::onExportedNotesToEnex);

    QObject::connect(
        pExporter, &EnexExporter::failedToExportNotesToEnex, this,
        &MainWindow::onExportNotesToEnexFailed);

    pExporter->start();
}

void MainWindow::onExportedNotesToEnex(QString enex)
{
    QNDEBUG("quentier:main_window", "MainWindow::onExportedNotesToEnex");

    auto * pExporter = qobject_cast<EnexExporter *>(sender());
    if (Q_UNLIKELY(!pExporter)) {
        ErrorString error(
            QT_TR_NOOP("Can't export notes to ENEX: internal error, "
                       "can't cast the slot invoker to EnexExporter"));
        QNWARNING("quentier:main_window", error);
        onSetStatusBarText(error.localizedString(), secondsToMilliseconds(30));
        return;
    }

    QString enexFilePath = pExporter->targetEnexFilePath();
    if (Q_UNLIKELY(enexFilePath.isEmpty())) {
        ErrorString error(
            QT_TR_NOOP("Can't export notes to ENEX: internal error, "
                       "the selected ENEX file path was lost"));
        QNWARNING("quentier:main_window", error);
        onSetStatusBarText(error.localizedString(), secondsToMilliseconds(30));
        return;
    }

    QByteArray enexRawData = enex.toUtf8();
    auto * pAsyncFileWriter = new AsyncFileWriter(enexFilePath, enexRawData);

    QObject::connect(
        pAsyncFileWriter, &AsyncFileWriter::fileSuccessfullyWritten, this,
        &MainWindow::onEnexFileWrittenSuccessfully);

    QObject::connect(
        pAsyncFileWriter, &AsyncFileWriter::fileWriteFailed, this,
        &MainWindow::onEnexFileWriteFailed);

    QObject::connect(
        pAsyncFileWriter, &AsyncFileWriter::fileWriteIncomplete, this,
        &MainWindow::onEnexFileWriteIncomplete);

    QThreadPool::globalInstance()->start(pAsyncFileWriter);
}

void MainWindow::onExportNotesToEnexFailed(ErrorString errorDescription)
{
    QNDEBUG(
        "quentier:main_window",
        "MainWindow::onExportNotesToEnexFailed: " << errorDescription);

    auto * pExporter = qobject_cast<EnexExporter *>(sender());
    if (pExporter) {
        pExporter->clear();
        pExporter->deleteLater();
    }

    onSetStatusBarText(
        errorDescription.localizedString(), secondsToMilliseconds(30));
}

void MainWindow::onEnexFileWrittenSuccessfully(QString filePath)
{
    QNDEBUG(
        "quentier:main_window",
        "MainWindow::onEnexFileWrittenSuccessfully: " << filePath);

    onSetStatusBarText(
        tr("Successfully exported note(s) to ENEX: ") +
            QDir::toNativeSeparators(filePath),
        secondsToMilliseconds(5));
}

void MainWindow::onEnexFileWriteFailed(ErrorString errorDescription)
{
    QNDEBUG(
        "quentier:main_window",
        "MainWindow::onEnexFileWriteFailed: " << errorDescription);

    onSetStatusBarText(
        tr("Can't export note(s) to ENEX, failed to write the ENEX to file") +
            QStringLiteral(": ") + errorDescription.localizedString(),
        secondsToMilliseconds(30));
}

void MainWindow::onEnexFileWriteIncomplete(
    qint64 bytesWritten, qint64 bytesTotal)
{
    QNDEBUG(
        "quentier:main_window",
        "MainWindow::onEnexFileWriteIncomplete: "
            << "bytes written = " << bytesWritten
            << ", bytes total = " << bytesTotal);

    if (bytesWritten == 0) {
        onSetStatusBarText(
            tr("Can't export note(s) to ENEX, failed to write the ENEX to "
               "file"),
            secondsToMilliseconds(30));
    }
    else {
        onSetStatusBarText(
            tr("Can't export note(s) to ENEX, failed to write the ENEX to "
               "file, only a portion of data has been written") +
                QStringLiteral(": ") + QString::number(bytesWritten) +
                QStringLiteral("/") + QString::number(bytesTotal),
            secondsToMilliseconds(30));
    }
}

void MainWindow::onEnexImportCompletedSuccessfully(QString enexFilePath)
{
    QNDEBUG(
        "quentier:main_window",
        "MainWindow::onEnexImportCompletedSuccessfully: " << enexFilePath);

    onSetStatusBarText(
        tr("Successfully imported note(s) from ENEX file") +
            QStringLiteral(": ") + QDir::toNativeSeparators(enexFilePath),
        secondsToMilliseconds(5));

    auto * pImporter = qobject_cast<EnexImporter *>(sender());
    if (pImporter) {
        pImporter->clear();
        pImporter->deleteLater();
    }
}

void MainWindow::onEnexImportFailed(ErrorString errorDescription)
{
    QNDEBUG(
        "quentier:main_window",
        "MainWindow::onEnexImportFailed: " << errorDescription);

    onSetStatusBarText(
        errorDescription.localizedString(), secondsToMilliseconds(30));

    auto * pImporter = qobject_cast<EnexImporter *>(sender());
    if (pImporter) {
        pImporter->clear();
        pImporter->deleteLater();
    }
}

void MainWindow::onUseLimitedFontsPreferenceChanged(bool flag)
{
    QNDEBUG(
        "quentier:main_window",
        "MainWindow::onUseLimitedFontsPreferenceChanged: flag = "
            << (flag ? "enabled" : "disabled"));

    if (m_pNoteEditorTabsAndWindowsCoordinator) {
        m_pNoteEditorTabsAndWindowsCoordinator->setUseLimitedFonts(flag);
    }
}

void MainWindow::onShowNoteThumbnailsPreferenceChanged()
{
    QNDEBUG(
        "quentier:main_window",
        "MainWindow::onShowNoteThumbnailsPreferenceChanged");

    bool showNoteThumbnails = getShowNoteThumbnailsPreference();

    Q_EMIT showNoteThumbnailsStateChanged(
        showNoteThumbnails, notesWithHiddenThumbnails());

    auto * pNoteItemDelegate =
        qobject_cast<NoteItemDelegate *>(m_pUi->noteListView->itemDelegate());

    if (Q_UNLIKELY(!pNoteItemDelegate)) {
        QNDEBUG("quentier:main_window", "No NoteItemDelegate");
        return;
    }

    m_pUi->noteListView->update();
}

void MainWindow::onDisableNativeMenuBarPreferenceChanged()
{
    QNDEBUG(
        "quentier:main_window",
        "MainWindow::onDisableNativeMenuBarPreferenceChanged");

    setupDisableNativeMenuBarPreference();
}

void MainWindow::onRunSyncEachNumMinitesPreferenceChanged(
    int runSyncEachNumMinutes)
{
    QNDEBUG(
        "quentier:main_window",
        "MainWindow::onRunSyncEachNumMinitesPreferenceChanged: "
            << runSyncEachNumMinutes);

    if (m_runSyncPeriodicallyTimerId != 0) {
        killTimer(m_runSyncPeriodicallyTimerId);
        m_runSyncPeriodicallyTimerId = 0;
    }

    if (runSyncEachNumMinutes <= 0) {
        return;
    }

    if (Q_UNLIKELY(
            !m_pAccount || (m_pAccount->type() != Account::Type::Evernote))) {
        return;
    }

    m_runSyncPeriodicallyTimerId =
        startTimer(secondsToMilliseconds(runSyncEachNumMinutes * 60));
}

void MainWindow::onPanelFontColorChanged(QColor color)
{
    QNDEBUG(
        "quentier:main_window",
        "MainWindow::onPanelFontColorChanged: " << color.name());

    for (auto & pPanelStyleController: m_genericPanelStyleControllers) {
        pPanelStyleController->setOverrideFontColor(color);
    }

    for (auto & pPanelStyleController: m_sidePanelStyleControllers) {
        pPanelStyleController->setOverrideFontColor(color);
    }
}

void MainWindow::onPanelBackgroundColorChanged(QColor color)
{
    QNDEBUG(
        "quentier:main_window",
        "MainWindow::onPanelBackgroundColorChanged: " << color.name());

    if (Q_UNLIKELY(!m_pAccount)) {
        QNDEBUG("quentier:main_window", "No current account");
        return;
    }

    ApplicationSettings settings(
        *m_pAccount, preferences::keys::files::userInterface);

    settings.beginGroup(preferences::keys::panelColorsGroup);

    bool useBackgroundGradient =
        settings.value(preferences::keys::panelUseBackgroundGradient).toBool();

    settings.endGroup();

    if (useBackgroundGradient) {
        QNDEBUG(
            "quentier:main_window",
            "Background gradient is used instead "
                << "of solid color");
        return;
    }

    for (auto & pPanelStyleController: m_genericPanelStyleControllers) {
        pPanelStyleController->setOverrideBackgroundColor(color);
    }

    for (auto & pPanelStyleController: m_sidePanelStyleControllers) {
        pPanelStyleController->setOverrideBackgroundColor(color);
    }
}

void MainWindow::onPanelUseBackgroundGradientSettingChanged(
    bool useBackgroundGradient)
{
    QNDEBUG(
        "quentier:main_window",
        "MainWindow::onPanelUseBackgroundGradientSettingChanged: "
            << (useBackgroundGradient ? "true" : "false"));

    restorePanelColors();
}

void MainWindow::onPanelBackgroundLinearGradientChanged(
    QLinearGradient gradient)
{
    QNDEBUG(
        "quentier:main_window",
        "MainWindow::onPanelBackgroundLinearGradientChanged");

    if (Q_UNLIKELY(!m_pAccount)) {
        QNDEBUG("quentier:main_window", "No current account");
        return;
    }

    ApplicationSettings settings(
        *m_pAccount, preferences::keys::files::userInterface);

    settings.beginGroup(preferences::keys::panelColorsGroup);

    bool useBackgroundGradient =
        settings.value(preferences::keys::panelUseBackgroundGradient).toBool();

    settings.endGroup();

    if (!useBackgroundGradient) {
        QNDEBUG(
            "quentier:main_window",
            "Background color is used instead of "
                << "gradient");
        return;
    }

    for (auto & pPanelStyleController: m_genericPanelStyleControllers) {
        pPanelStyleController->setOverrideBackgroundGradient(gradient);
    }

    for (auto & pPanelStyleController: m_sidePanelStyleControllers) {
        pPanelStyleController->setOverrideBackgroundGradient(gradient);
    }
}

void MainWindow::onNewNoteRequestedFromSystemTrayIcon()
{
    QNDEBUG(
        "quentier:main_window",
        "MainWindow::onNewNoteRequestedFromSystemTrayIcon");

    Qt::WindowStates state = windowState();
    bool isMinimized = (state & Qt::WindowMinimized);

    bool shown = !isMinimized && !isHidden();

    createNewNote(
        shown ? NoteEditorTabsAndWindowsCoordinator::NoteEditorMode::Any
              : NoteEditorTabsAndWindowsCoordinator::NoteEditorMode::Window);
}

void MainWindow::onQuitRequestedFromSystemTrayIcon()
{
    QNINFO(
        "quentier:main_window",
        "MainWindow::onQuitRequestedFromSystemTrayIcon");

    quitApp();
}

void MainWindow::onAccountSwitchRequested(Account account)
{
    QNDEBUG(
        "quentier:main_window",
        "MainWindow::onAccountSwitchRequested: " << account.name());

    stopListeningForSplitterMoves();
    m_pAccountManager->switchAccount(account);
}

void MainWindow::onSystemTrayIconManagerError(ErrorString errorDescription)
{
    QNDEBUG(
        "quentier:main_window",
        "MainWindow::onSystemTrayIconManagerError: " << errorDescription);

    onSetStatusBarText(
        errorDescription.localizedString(), secondsToMilliseconds(30));
}

void MainWindow::onShowRequestedFromTrayIcon()
{
    QNDEBUG("quentier:main_window", "MainWindow::onShowRequestedFromTrayIcon");
    show();
}

void MainWindow::onHideRequestedFromTrayIcon()
{
    QNDEBUG("quentier:main_window", "MainWindow::onHideRequestedFromTrayIcon");
    hide();
}

void MainWindow::onViewLogsActionTriggered()
{
    auto * pLogViewerWidget = findChild<LogViewerWidget *>();
    if (pLogViewerWidget) {
        return;
    }

    pLogViewerWidget = new LogViewerWidget(this);
    pLogViewerWidget->setAttribute(Qt::WA_DeleteOnClose);
    pLogViewerWidget->show();
}

void MainWindow::onShowInfoAboutQuentierActionTriggered()
{
    QNDEBUG(
        "quentier:main_window",
        "MainWindow::onShowInfoAboutQuentierActionTriggered");

    auto * pWidget = findChild<AboutQuentierWidget *>();
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
    QNINFO("quentier:main_window", "MainWindow::onNoteEditorError: " << error);
    onSetStatusBarText(error.localizedString(), secondsToMilliseconds(30));
}

void MainWindow::onModelViewError(ErrorString error)
{
    QNINFO("quentier:main_window", "MainWindow::onModelViewError: " << error);
    onSetStatusBarText(error.localizedString(), secondsToMilliseconds(30));
}

void MainWindow::onNoteEditorSpellCheckerNotReady()
{
    QNDEBUG(
        "quentier:main_window", "MainWindow::onNoteEditorSpellCheckerNotReady");

    auto * noteEditor = qobject_cast<NoteEditorWidget *>(sender());
    if (!noteEditor) {
        QNTRACE(
            "quentier:main_window",
            "Can't cast caller to note editor "
                << "widget, skipping");
        return;
    }

    auto * currentEditor = currentNoteEditorTab();
    if (!currentEditor || (currentEditor != noteEditor)) {
        QNTRACE(
            "quentier:main_window",
            "Not an update from current note "
                << "editor, skipping");
        return;
    }

    onSetStatusBarText(
        tr("Spell checker is loading dictionaries, please wait"));
}

void MainWindow::onNoteEditorSpellCheckerReady()
{
    QNDEBUG(
        "quentier:main_window", "MainWindow::onNoteEditorSpellCheckerReady");

    auto * noteEditor = qobject_cast<NoteEditorWidget *>(sender());
    if (!noteEditor) {
        QNTRACE(
            "quentier:main_window",
            "Can't cast caller to note editor "
                << "widget, skipping");
        return;
    }

    auto * currentEditor = currentNoteEditorTab();
    if (!currentEditor || (currentEditor != noteEditor)) {
        QNTRACE(
            "quentier:main_window",
            "Not an update from current note "
                << "editor, skipping");
        return;
    }

    onSetStatusBarText(QString());
}

void MainWindow::onAddAccountActionTriggered(bool checked)
{
    QNDEBUG("quentier:main_window", "MainWindow::onAddAccountActionTriggered");
    Q_UNUSED(checked)
    onNewAccountCreationRequested();
}

void MainWindow::onManageAccountsActionTriggered(bool checked)
{
    QNDEBUG(
        "quentier:main_window", "MainWindow::onManageAccountsActionTriggered");

    Q_UNUSED(checked)
    Q_UNUSED(m_pAccountManager->execManageAccountsDialog());
}

void MainWindow::onSwitchAccountActionToggled(bool checked)
{
    QNDEBUG(
        "quentier:main_window",
        "MainWindow::onSwitchAccountActionToggled: "
            << "checked = " << (checked ? "true" : "false"));

    if (!checked) {
        QNTRACE("quentier:main_window", "Ignoring the unchecking of account");
        return;
    }

    auto * action = qobject_cast<QAction *>(sender());
    if (Q_UNLIKELY(!action)) {
        NOTIFY_ERROR(
            QT_TR_NOOP("Internal error: account switching action is "
                       "unexpectedly null"));
        return;
    }

    auto indexData = action->data();
    bool conversionResult = false;
    int index = indexData.toInt(&conversionResult);
    if (Q_UNLIKELY(!conversionResult)) {
        NOTIFY_ERROR(
            QT_TR_NOOP("Internal error: can't get identification data from "
                       "the account switching action"));
        return;
    }

    const auto & availableAccounts = m_pAccountManager->availableAccounts();
    const int numAvailableAccounts = availableAccounts.size();

    if ((index < 0) || (index >= numAvailableAccounts)) {
        NOTIFY_ERROR(
            QT_TR_NOOP("Internal error: wrong index into available "
                       "accounts in account switching action"));
        return;
    }

    if (m_syncInProgress) {
        NOTIFY_DEBUG(
            QT_TR_NOOP("Can't switch account while the synchronization "
                       "is in progress"));
        return;
    }

    if (m_pendingNewEvernoteAccountAuthentication) {
        NOTIFY_DEBUG(
            QT_TR_NOOP("Can't switch account while the authentication "
                       "of new Evernote accont is in progress"));
        return;
    }

    if (m_pendingCurrentEvernoteAccountAuthentication) {
        NOTIFY_DEBUG(
            QT_TR_NOOP("Can't switch account while the authentication "
                       "of Evernote account is in progress"));
        return;
    }

    if (m_pendingSwitchToNewEvernoteAccount) {
        NOTIFY_DEBUG(
            QT_TR_NOOP("Can't switch account: account switching "
                       "operation is already in progress"));
        return;
    }

    const Account & availableAccount = availableAccounts[index];

    stopListeningForSplitterMoves();
    m_pAccountManager->switchAccount(availableAccount);

    // The continuation is in onAccountSwitched slot connected to
    // AccountManager's switchedAccount signal
}

void MainWindow::onAccountSwitched(Account account)
{
    QNDEBUG(
        "quentier:main_window",
        "MainWindow::onAccountSwitched: " << account.name());

    if (Q_UNLIKELY(!m_pLocalStorageManagerThread)) {
        ErrorString errorDescription(
            QT_TR_NOOP("internal error: no local storage manager thread "
                       "exists"));

        QNWARNING("quentier:main_window", errorDescription);

        onSetStatusBarText(
            tr("Could not switch account: ") + QStringLiteral(": ") +
                errorDescription.localizedString(),
            secondsToMilliseconds(30));
        return;
    }

    if (Q_UNLIKELY(!m_pLocalStorageManagerAsync)) {
        ErrorString errorDescription(
            QT_TR_NOOP("internal error: no local storage manager exists"));
        QNWARNING("quentier:main_window", errorDescription);

        onSetStatusBarText(
            tr("Could not switch account: ") + QStringLiteral(": ") +
                errorDescription.localizedString(),
            secondsToMilliseconds(30));
        return;
    }

    clearSynchronizationManager();

    // Since Qt 5.11 QSqlDatabase opening only works properly from the thread
    // which has loaded the SQL drivers - which is this thread, the GUI one.
    // However, LocalStorageManagerAsync operates in another thread. So need
    // to stop that thread, perform the account switching operation
    // synchronously and then start the stopped thread again. See
    // https://bugreports.qt.io/browse/QTBUG-72545 for reference.

    bool localStorageThreadWasStopped = false;
    if (m_pLocalStorageManagerThread->isRunning()) {
        QObject::disconnect(
            m_pLocalStorageManagerThread, &QThread::finished,
            m_pLocalStorageManagerThread, &QThread::deleteLater);

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
    catch (const std::exception & e) {
        errorDescription.setBase(
            QT_TR_NOOP("Can't switch user in the local storage: caught "
                       "exception"));
        errorDescription.details() = QString::fromUtf8(e.what());
    }

    m_pLocalStorageManagerAsync->setUseCache(cacheIsUsed);

    bool checkRes = true;
    if (errorDescription.isEmpty()) {
        checkRes = checkLocalStorageVersion(account);
    }

    if (localStorageThreadWasStopped) {
        QObject::connect(
            m_pLocalStorageManagerThread, &QThread::finished,
            m_pLocalStorageManagerThread, &QThread::deleteLater);

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
        onLocalStorageSwitchUserRequestFailed(
            account, errorDescription, m_lastLocalStorageSwitchUserRequest);
    }
}

void MainWindow::onAccountUpdated(Account account)
{
    QNDEBUG(
        "quentier:main_window",
        "MainWindow::onAccountUpdated: " << account.name());

    if (!m_pAccount) {
        QNDEBUG("quentier:main_window", "No account is current at the moment");
        return;
    }

    if (m_pAccount->type() != account.type()) {
        QNDEBUG(
            "quentier:main_window",
            "Not an update for the current "
                << "account: it has another type");
        QNTRACE("quentier:main_window", *m_pAccount);
        return;
    }

    bool isLocal = (m_pAccount->type() == Account::Type::Local);

    if (isLocal && (m_pAccount->name() != account.name())) {
        QNDEBUG(
            "quentier:main_window",
            "Not an update for the current "
                << "account: it has another name");
        QNTRACE("quentier:main_window", *m_pAccount);
        return;
    }

    if (!isLocal &&
        ((m_pAccount->id() != account.id()) ||
         (m_pAccount->name() != account.name())))
    {
        QNDEBUG(
            "quentier:main_window",
            "Not an update for the current "
                << "account: either id or name don't match");
        QNTRACE("quentier:main_window", *m_pAccount);
        return;
    }

    *m_pAccount = account;
    setWindowTitleForAccount(account);
}

void MainWindow::onAccountAdded(Account account)
{
    QNDEBUG(
        "quentier:main_window",
        "MainWindow::onAccountAdded: " << account.name());

    updateSubMenuWithAvailableAccounts();
}

void MainWindow::onAccountRemoved(Account account)
{
    QNDEBUG(
        "quentier:main_window", "MainWindow::onAccountRemoved: " << account);

    updateSubMenuWithAvailableAccounts();
}

void MainWindow::onAccountManagerError(ErrorString errorDescription)
{
    QNDEBUG(
        "quentier:main_window",
        "MainWindow::onAccountManagerError: " << errorDescription);

    onSetStatusBarText(
        errorDescription.localizedString(), secondsToMilliseconds(30));
}

void MainWindow::onShowSidePanelActionToggled(bool checked)
{
    QNDEBUG(
        "quentier:main_window",
        "MainWindow::onShowSidePanelActionToggled: "
            << "checked = " << (checked ? "true" : "false"));

    ApplicationSettings appSettings(
        *m_pAccount, preferences::keys::files::userInterface);

    appSettings.beginGroup(QStringLiteral("MainWindow"));
    appSettings.setValue(QStringLiteral("ShowSidePanel"), checked);
    appSettings.endGroup();

    if (checked) {
        m_pUi->sidePanelSplitter->show();
    }
    else {
        m_pUi->sidePanelSplitter->hide();
    }
}

void MainWindow::onShowFavoritesActionToggled(bool checked)
{
    QNDEBUG(
        "quentier:main_window",
        "MainWindow::onShowFavoritesActionToggled: "
            << "checked = " << (checked ? "true" : "false"));

    ApplicationSettings appSettings(
        *m_pAccount, preferences::keys::files::userInterface);

    appSettings.beginGroup(QStringLiteral("MainWindow"));
    appSettings.setValue(QStringLiteral("ShowFavorites"), checked);
    appSettings.endGroup();

    if (checked) {
        m_pUi->favoritesWidget->show();
    }
    else {
        m_pUi->favoritesWidget->hide();
    }
}

void MainWindow::onShowNotebooksActionToggled(bool checked)
{
    QNDEBUG(
        "quentier:main_window",
        "MainWindow::onShowNotebooksActionToggled: "
            << "checked = " << (checked ? "true" : "false"));

    ApplicationSettings appSettings(
        *m_pAccount, preferences::keys::files::userInterface);

    appSettings.beginGroup(QStringLiteral("MainWindow"));
    appSettings.setValue(QStringLiteral("ShowNotebooks"), checked);
    appSettings.endGroup();

    if (checked) {
        m_pUi->notebooksWidget->show();
    }
    else {
        m_pUi->notebooksWidget->hide();
    }
}

void MainWindow::onShowTagsActionToggled(bool checked)
{
    QNDEBUG(
        "quentier:main_window",
        "MainWindow::onShowTagsActionToggled: "
            << "checked = " << (checked ? "true" : "false"));

    ApplicationSettings appSettings(
        *m_pAccount, preferences::keys::files::userInterface);

    appSettings.beginGroup(QStringLiteral("MainWindow"));
    appSettings.setValue(QStringLiteral("ShowTags"), checked);
    appSettings.endGroup();

    if (checked) {
        m_pUi->tagsWidget->show();
    }
    else {
        m_pUi->tagsWidget->hide();
    }
}

void MainWindow::onShowSavedSearchesActionToggled(bool checked)
{
    QNDEBUG(
        "quentier:main_window",
        "MainWindow::onShowSavedSearchesActionToggled: checked = "
            << (checked ? "true" : "false"));

    ApplicationSettings appSettings(
        *m_pAccount, preferences::keys::files::userInterface);

    appSettings.beginGroup(QStringLiteral("MainWindow"));
    appSettings.setValue(QStringLiteral("ShowSavedSearches"), checked);
    appSettings.endGroup();

    if (checked) {
        m_pUi->savedSearchesWidget->show();
    }
    else {
        m_pUi->savedSearchesWidget->hide();
    }
}

void MainWindow::onShowDeletedNotesActionToggled(bool checked)
{
    QNDEBUG(
        "quentier:main_window",
        "MainWindow::onShowDeletedNotesActionToggled: checked = "
            << (checked ? "true" : "false"));

    ApplicationSettings appSettings(
        *m_pAccount, preferences::keys::files::userInterface);

    appSettings.beginGroup(QStringLiteral("MainWindow"));
    appSettings.setValue(QStringLiteral("ShowDeletedNotes"), checked);
    appSettings.endGroup();

    if (checked) {
        m_pUi->deletedNotesWidget->show();
    }
    else {
        m_pUi->deletedNotesWidget->hide();
    }
}

void MainWindow::onShowNoteListActionToggled(bool checked)
{
    QNDEBUG(
        "quentier:main_window",
        "MainWindow::onShowNoteListActionToggled: "
            << "checked = " << (checked ? "true" : "false"));

    ApplicationSettings appSettings(
        *m_pAccount, preferences::keys::files::userInterface);

    appSettings.beginGroup(QStringLiteral("MainWindow"));
    appSettings.setValue(QStringLiteral("ShowNotesList"), checked);
    appSettings.endGroup();

    if (checked) {
        m_pUi->noteListView->setModel(m_pNoteModel);
        m_pUi->notesListAndFiltersFrame->show();
    }
    else {
        m_pUi->notesListAndFiltersFrame->hide();
        m_pUi->noteListView->setModel(&m_blankModel);
    }
}

void MainWindow::onShowToolbarActionToggled(bool checked)
{
    QNDEBUG(
        "quentier:main_window",
        "MainWindow::onShowToolbarActionToggled: "
            << "checked = " << (checked ? "true" : "false"));

    ApplicationSettings appSettings(
        *m_pAccount, preferences::keys::files::userInterface);

    appSettings.beginGroup(QStringLiteral("MainWindow"));
    appSettings.setValue(QStringLiteral("ShowToolbar"), checked);
    appSettings.endGroup();

    if (checked) {
        m_pUi->upperBarGenericPanel->show();
    }
    else {
        m_pUi->upperBarGenericPanel->hide();
    }
}

void MainWindow::onShowStatusBarActionToggled(bool checked)
{
    QNDEBUG(
        "quentier:main_window",
        "MainWindow::onShowStatusBarActionToggled: "
            << "checked = " << (checked ? "true" : "false"));

    ApplicationSettings appSettings(
        *m_pAccount, preferences::keys::files::userInterface);

    appSettings.beginGroup(QStringLiteral("MainWindow"));
    appSettings.setValue(QStringLiteral("ShowStatusBar"), checked);
    appSettings.endGroup();

    if (checked) {
        m_pUi->statusBar->show();
    }
    else {
        m_pUi->statusBar->hide();
    }
}

void MainWindow::onSwitchIconTheme(const QString & iconTheme)
{
    QNDEBUG(
        "quentier:main_window", "MainWindow::onSwitchIconTheme: " << iconTheme);

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
        QNWARNING("quentier:main_window", error);
        onSetStatusBarText(error.localizedString(), secondsToMilliseconds(30));
    }
}

void MainWindow::onSwitchIconThemeToNativeAction()
{
    QNDEBUG(
        "quentier:main_window", "MainWindow::onSwitchIconThemeToNativeAction");

    if (m_nativeIconThemeName.isEmpty()) {
        ErrorString error(QT_TR_NOOP("No native icon theme is available"));
        QNDEBUG("quentier:main_window", error);
        onSetStatusBarText(error.localizedString(), secondsToMilliseconds(30));
        return;
    }

    if (QIcon::themeName() == m_nativeIconThemeName) {
        ErrorString error(QT_TR_NOOP("Already using the native icon theme"));
        QNDEBUG("quentier:main_window", error);
        onSetStatusBarText(error.localizedString(), secondsToMilliseconds(30));
        return;
    }

    QIcon::setThemeName(m_nativeIconThemeName);
    persistChosenIconTheme(m_nativeIconThemeName);
    refreshChildWidgetsThemeIcons();
}

void MainWindow::onSwitchIconThemeToTangoAction()
{
    QNDEBUG(
        "quentier:main_window", "MainWindow::onSwitchIconThemeToTangoAction");

    QString tango = QStringLiteral("tango");

    if (QIcon::themeName() == tango) {
        ErrorString error(QT_TR_NOOP("Already using tango icon theme"));
        QNDEBUG("quentier:main_window", error);
        onSetStatusBarText(error.localizedString(), secondsToMilliseconds(30));
        return;
    }

    QIcon::setThemeName(tango);
    persistChosenIconTheme(tango);
    refreshChildWidgetsThemeIcons();
}

void MainWindow::onSwitchIconThemeToOxygenAction()
{
    QNDEBUG(
        "quentier:main_window", "MainWindow::onSwitchIconThemeToOxygenAction");

    QString oxygen = QStringLiteral("oxygen");

    if (QIcon::themeName() == oxygen) {
        ErrorString error(QT_TR_NOOP("Already using oxygen icon theme"));
        QNDEBUG("quentier:main_window", error);
        onSetStatusBarText(error.localizedString(), secondsToMilliseconds(10));
        return;
    }

    QIcon::setThemeName(oxygen);
    persistChosenIconTheme(oxygen);
    refreshChildWidgetsThemeIcons();
}

void MainWindow::onSwitchIconThemeToBreezeAction()
{
    QNDEBUG(
        "quentier:main_window", "MainWindow::onSwitchIconThemeToBreezeAction");

    QString breeze = QStringLiteral("breeze");

    if (QIcon::themeName() == breeze) {
        ErrorString error(QT_TR_NOOP("Already using breeze icon theme"));
        QNDEBUG("quentier:main_window", error);
        onSetStatusBarText(error.localizedString(), secondsToMilliseconds(10));
        return;
    }

    QIcon::setThemeName(breeze);
    persistChosenIconTheme(breeze);
    refreshChildWidgetsThemeIcons();
}

void MainWindow::onSwitchIconThemeToBreezeDarkAction()
{
    QNDEBUG(
        "quentier:main_window",
        "MainWindow::onSwitchIconThemeToBreezeDarkAction");

    QString breezeDark = QStringLiteral("breeze-dark");

    if (QIcon::themeName() == breezeDark) {
        ErrorString error(QT_TR_NOOP("Already using breeze-dark icon theme"));
        QNDEBUG("quentier:main_window", error);
        onSetStatusBarText(error.localizedString(), secondsToMilliseconds(10));
        return;
    }

    QIcon::setThemeName(breezeDark);
    persistChosenIconTheme(breezeDark);
    refreshChildWidgetsThemeIcons();
}

void MainWindow::onLocalStorageSwitchUserRequestComplete(
    Account account, QUuid requestId)
{
    QNDEBUG(
        "quentier:main_window",
        "MainWindow::onLocalStorageSwitchUserRequestComplete: "
            << "account = " << account.name()
            << ", request id = " << requestId);

    QNTRACE("quentier:main_window", account);

    bool expected = (m_lastLocalStorageSwitchUserRequest == requestId);
    m_lastLocalStorageSwitchUserRequest = QUuid();

    bool wasPendingSwitchToNewEvernoteAccount =
        m_pendingSwitchToNewEvernoteAccount;

    m_pendingSwitchToNewEvernoteAccount = false;

    if (!expected) {
        NOTIFY_ERROR(
            QT_TR_NOOP("Local storage user was switched without explicit "
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
        m_pNoteEditorTabsAndWindowsCoordinator->switchAccount(
            *m_pAccount, *m_pTagModel);
    }

    m_pUi->filterByNotebooksWidget->switchAccount(
        *m_pAccount, m_pNotebookModel);

    m_pUi->filterByTagsWidget->switchAccount(*m_pAccount, m_pTagModel);

    m_pUi->filterBySavedSearchComboBox->switchAccount(
        *m_pAccount, m_pSavedSearchModel);

    setupViews();
    setupAccountSpecificUiElements();

    // FIXME: this can be done more lightweight: just set the current account
    // in the already filled list
    updateSubMenuWithAvailableAccounts();

    restoreGeometryAndState();
    restorePanelColors();

    if (m_pAccount->type() != Account::Type::Evernote) {
        QNTRACE(
            "quentier:main_window",
            "Not an Evernote account, no need to "
                << "bother setting up sync");
        return;
    }

    // TODO: should also start the sync if the corresponding setting is set
    // to sync stuff when one switches to the Evernote account
    if (!wasPendingSwitchToNewEvernoteAccount) {
        QNTRACE(
            "quentier:main_window",
            "Not an account switch after "
                << "authenticating new Evernote account");
        return;
    }

    // For new Evernote account is is convenient if the first note to be
    // synchronized automatically opens in the note editor
    m_pUi->noteListView->setAutoSelectNoteOnNextAddition();

    if (Q_UNLIKELY(!m_pSynchronizationManager)) {
        QNWARNING(
            "quentier:main_window",
            "Detected unexpectedly missing "
                << "SynchronizationManager, trying to workaround");

        setupSynchronizationManager();

        if (Q_UNLIKELY(!m_pSynchronizationManager)) {
            // Wasn't able to set up the synchronization manager
            return;
        }
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

    QNDEBUG(
        "quentier:main_window",
        "MainWindow::onLocalStorageSwitchUserRequestFailed: "
            << account.name() << "\nError description: " << errorDescription
            << ", request id = " << requestId);

    QNTRACE("quentier:main_window", account);

    m_lastLocalStorageSwitchUserRequest = QUuid();

    onSetStatusBarText(
        tr("Could not switch account") + QStringLiteral(": ") +
            errorDescription.localizedString(),
        secondsToMilliseconds(30));

    if (!m_pAccount) {
        // If there was no any account set previously, nothing to do
        return;
    }

    restoreNetworkProxySettingsForAccount(*m_pAccount);
    startListeningForSplitterMoves();

    const auto & availableAccounts = m_pAccountManager->availableAccounts();
    const int numAvailableAccounts = availableAccounts.size();

    // Trying to restore the previously selected account as the current one in
    // the UI
    auto availableAccountActions = m_pAvailableAccountsActionGroup->actions();
    for (auto * pAction: qAsConst(availableAccountActions)) {
        if (Q_UNLIKELY(!pAction)) {
            QNDEBUG(
                "quentier:main_window",
                "Found null pointer to action "
                    << "within the available accounts action group");
            continue;
        }

        auto actionData = pAction->data();
        bool conversionResult = false;
        int index = actionData.toInt(&conversionResult);
        if (Q_UNLIKELY(!conversionResult)) {
            QNDEBUG(
                "quentier:main_window",
                "Can't convert available account's "
                    << "user data to int: " << actionData);
            continue;
        }

        if (Q_UNLIKELY((index < 0) || (index >= numAvailableAccounts))) {
            QNDEBUG(
                "quentier:main_window",
                "Available account's index is "
                    << "beyond the range of available accounts: index = "
                    << index
                    << ", num available accounts = " << numAvailableAccounts);
            continue;
        }

        const auto & actionAccount = availableAccounts.at(index);
        if (actionAccount == *m_pAccount) {
            QNDEBUG(
                "quentier:main_window",
                "Restoring the current account in "
                    << "UI: index = " << index
                    << ", account = " << actionAccount);
            pAction->setChecked(true);
            return;
        }
    }

    // If we got here, it means we haven't found the proper previous account
    QNDEBUG(
        "quentier:main_window",
        "Couldn't find the action corresponding to "
            << "the previous available account: " << *m_pAccount);
}

void MainWindow::onSplitterHandleMoved(int pos, int index)
{
    QNDEBUG(
        "quentier:main_window",
        "MainWindow::onSplitterHandleMoved: pos = " << pos
                                                    << ", index = " << index);

    scheduleGeometryAndStatePersisting();
}

void MainWindow::onSidePanelSplittedHandleMoved(int pos, int index)
{
    QNDEBUG(
        "quentier:main_window",
        "MainWindow::onSidePanelSplittedHandleMoved: pos = "
            << pos << ", index = " << index);

    scheduleGeometryAndStatePersisting();
}

void MainWindow::onSyncButtonPressed()
{
    QNDEBUG("quentier:main_window", "MainWindow::onSyncButtonPressed");

    if (Q_UNLIKELY(!m_pAccount)) {
        QNDEBUG(
            "quentier:main_window",
            "Ignoring the sync button click - no "
                << "account is set");
        return;
    }

    if (Q_UNLIKELY(m_pAccount->type() == Account::Type::Local)) {
        QNDEBUG(
            "quentier:main_window",
            "The current account is of local type, "
                << "won't do anything on attempt to sync it");
        return;
    }

    if (m_pNoteEditorTabsAndWindowsCoordinator) {
        m_pNoteEditorTabsAndWindowsCoordinator->saveAllNoteEditorsContents();
    }

    if (m_syncInProgress) {
        QNDEBUG(
            "quentier:main_window",
            "The synchronization is in progress, will stop it");
        Q_EMIT stopSynchronization();
    }
    else {
        launchSynchronization();
    }
}

void MainWindow::onAnimatedSyncIconFrameChanged(int frame)
{
    Q_UNUSED(frame)

    m_pUi->syncPushButton->setIcon(
        QIcon(m_animatedSyncButtonIcon.currentPixmap()));
}

void MainWindow::onAnimatedSyncIconFrameChangedPendingFinish(int frame)
{
    if ((frame == 0) || (frame == (m_animatedSyncButtonIcon.frameCount() - 1)))
    {
        stopSyncButtonAnimation();
    }
}

void MainWindow::onSyncIconAnimationFinished()
{
    QNDEBUG("quentier:main_window", "MainWindow::onSyncIconAnimationFinished");

    QObject::disconnect(
        &m_animatedSyncButtonIcon, &QMovie::finished, this,
        &MainWindow::onSyncIconAnimationFinished);

    QObject::disconnect(
        &m_animatedSyncButtonIcon, &QMovie::frameChanged, this,
        &MainWindow::onAnimatedSyncIconFrameChangedPendingFinish);

    stopSyncButtonAnimation();
}

void MainWindow::onNewAccountCreationRequested()
{
    QNDEBUG(
        "quentier:main_window", "MainWindow::onNewAccountCreationRequested");

    int res = m_pAccountManager->execAddAccountDialog();
    if (res == QDialog::Accepted) {
        return;
    }

    if (Q_UNLIKELY(!m_pLocalStorageManagerAsync)) {
        QNWARNING(
            "quentier:main_window",
            "Local storage manager async "
                << "unexpectedly doesn't exist, can't check local storage "
                   "version");
        return;
    }

    bool localStorageThreadWasStopped = false;
    if (m_pLocalStorageManagerThread &&
        m_pLocalStorageManagerThread->isRunning()) {
        QObject::disconnect(
            m_pLocalStorageManagerThread, &QThread::finished,
            m_pLocalStorageManagerThread, &QThread::deleteLater);

        m_pLocalStorageManagerThread->quit();
        m_pLocalStorageManagerThread->wait();
        localStorageThreadWasStopped = true;
    }

    Q_UNUSED(checkLocalStorageVersion(*m_pAccount))

    if (localStorageThreadWasStopped) {
        QObject::connect(
            m_pLocalStorageManagerThread, &QThread::finished,
            m_pLocalStorageManagerThread, &QThread::deleteLater);

        m_pLocalStorageManagerThread->start();
    }
}

void MainWindow::onQuitAction()
{
    QNDEBUG("quentier:main_window", "MainWindow::onQuitAction");
    quitApp();
}

void MainWindow::onShortcutChanged(
    int key, QKeySequence shortcut, const Account & account, QString context)
{
    QNDEBUG(
        "quentier:main_window",
        "MainWindow::onShortcutChanged: key = "
            << key
            << ", shortcut: " << shortcut.toString(QKeySequence::PortableText)
            << ", context: " << context << ", account: " << account.name());

    auto it = m_shortcutKeyToAction.find(key);
    if (it == m_shortcutKeyToAction.end()) {
        QNDEBUG(
            "quentier:main_window",
            "Haven't found the action "
                << "corresponding to the shortcut key");
        return;
    }

    auto * pAction = it.value();
    pAction->setShortcut(shortcut);

    QNDEBUG(
        "quentier:main_window",
        "Updated shortcut for action " << pAction->text() << " ("
                                       << pAction->objectName() << ")");
}

void MainWindow::onNonStandardShortcutChanged(
    QString nonStandardKey, QKeySequence shortcut, const Account & account,
    QString context)
{
    QNDEBUG(
        "quentier:main_window",
        "MainWindow::onNonStandardShortcutChanged: "
            << "non-standard key = " << nonStandardKey
            << ", shortcut: " << shortcut.toString(QKeySequence::PortableText)
            << ", context: " << context << ", account: " << account.name());

    auto it = m_nonStandardShortcutKeyToAction.find(nonStandardKey);
    if (it == m_nonStandardShortcutKeyToAction.end()) {
        QNDEBUG(
            "quentier:main_window",
            "Haven't found the action "
                << "corresponding to the non-standard shortcut key");
        return;
    }

    auto * pAction = it.value();
    pAction->setShortcut(shortcut);

    QNDEBUG(
        "quentier:main_window",
        "Updated shortcut for action " << pAction->text() << " ("
                                       << pAction->objectName() << ")");
}

void MainWindow::onDefaultAccountFirstNotebookAndNoteCreatorFinished(
    QString createdNoteLocalUid)
{
    QNDEBUG(
        "quentier:main_window",
        "MainWindow::onDefaultAccountFirstNotebookAndNoteCreatorFinished: "
            << "created note local uid = " << createdNoteLocalUid);

    auto * pDefaultAccountFirstNotebookAndNoteCreator =
        qobject_cast<DefaultAccountFirstNotebookAndNoteCreator *>(sender());

    if (pDefaultAccountFirstNotebookAndNoteCreator) {
        pDefaultAccountFirstNotebookAndNoteCreator->deleteLater();
    }

    bool foundNoteModelItem = false;
    if (m_pNoteModel) {
        auto index = m_pNoteModel->indexForLocalUid(createdNoteLocalUid);
        if (index.isValid()) {
            foundNoteModelItem = true;
        }
    }

    if (foundNoteModelItem) {
        m_pUi->noteListView->setCurrentNoteByLocalUid(createdNoteLocalUid);
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
    QNDEBUG(
        "quentier:main_window",
        "MainWindow::onDefaultAccountFirstNotebookAndNoteCreatorError: "
            << errorDescription);

    onSetStatusBarText(errorDescription.localizedString());

    auto * pDefaultAccountFirstNotebookAndNoteCreator =
        qobject_cast<DefaultAccountFirstNotebookAndNoteCreator *>(sender());

    if (pDefaultAccountFirstNotebookAndNoteCreator) {
        pDefaultAccountFirstNotebookAndNoteCreator->deleteLater();
    }
}

#ifdef WITH_UPDATE_MANAGER
void MainWindow::onCheckForUpdatesActionTriggered()
{
    QNDEBUG(
        "quentier:main_window", "MainWindow::onCheckForUpdatesActionTriggered");

    m_pUpdateManager->checkForUpdates();
}

void MainWindow::onUpdateManagerError(ErrorString errorDescription)
{
    QNDEBUG(
        "quentier:main_window",
        "MainWindow::onUpdateManagerError: " << errorDescription);

    onSetStatusBarText(errorDescription.localizedString());
}

void MainWindow::onUpdateManagerRequestsRestart()
{
    QNDEBUG(
        "quentier:main_window", "MainWindow::onUpdateManagerRequestsRestart");

    quitApp(RESTART_EXIT_CODE);
}
#endif // WITH_UPDATE_MANAGER

void MainWindow::resizeEvent(QResizeEvent * pEvent)
{
    QMainWindow::resizeEvent(pEvent);

    if (m_pUi->filterBodyFrame->isHidden()) {
        // NOTE: without this the splitter seems to take a wrong guess about
        // the size of note filters header panel and that doesn't look good
        adjustNoteListAndFiltersSplitterSizes();
    }

    scheduleGeometryAndStatePersisting();
}

void MainWindow::closeEvent(QCloseEvent * pEvent)
{
    QNDEBUG("quentier:main_window", "MainWindow::closeEvent");

    if (m_pendingFirstShutdownDialog) {
        QNDEBUG("quentier:main_window", "About to display FirstShutdownDialog");
        m_pendingFirstShutdownDialog = false;

        auto pDialog = std::make_unique<FirstShutdownDialog>(this);
        pDialog->setWindowModality(Qt::WindowModal);
        centerDialog(*pDialog);
        bool shouldCloseToSystemTray = (pDialog->exec() == QDialog::Accepted);

        m_pSystemTrayIconManager->setPreferenceCloseToSystemTray(
            shouldCloseToSystemTray);
    }

    if (pEvent && m_pSystemTrayIconManager->shouldCloseToSystemTray()) {
        QNINFO(
            "quentier:main_window",
            "Hiding to system tray instead of "
                << "closing");
        pEvent->ignore();
        hide();
        return;
    }

    if (m_pNoteEditorTabsAndWindowsCoordinator) {
        m_pNoteEditorTabsAndWindowsCoordinator->clear();
    }

    persistGeometryAndState();
    QNINFO("quentier:main_window", "Closing application");
    QMainWindow::closeEvent(pEvent);
    quitApp();
}

void MainWindow::timerEvent(QTimerEvent * pTimerEvent)
{
    QNDEBUG(
        "quentier:main_window",
        "MainWindow::timerEvent: timer id = "
            << (pTimerEvent ? QString::number(pTimerEvent->timerId())
                            : QStringLiteral("<null>")));

    if (Q_UNLIKELY(!pTimerEvent)) {
        return;
    }

    if (pTimerEvent->timerId() == m_geometryAndStatePersistingDelayTimerId) {
        persistGeometryAndState();
        killTimer(m_geometryAndStatePersistingDelayTimerId);
        m_geometryAndStatePersistingDelayTimerId = 0;
    }
    else if (pTimerEvent->timerId() == m_splitterSizesRestorationDelayTimerId) {
        restoreSplitterSizes();
        startListeningForSplitterMoves();
        killTimer(m_splitterSizesRestorationDelayTimerId);
        m_splitterSizesRestorationDelayTimerId = 0;
    }
    else if (pTimerEvent->timerId() == m_runSyncPeriodicallyTimerId) {
        if (Q_UNLIKELY(
                !m_pAccount ||
                (m_pAccount->type() != Account::Type::Evernote) ||
                !m_pSynchronizationManager))
        {
            QNDEBUG(
                "quentier:main_window",
                "Non-Evernote account is being "
                    << "used: "
                    << (m_pAccount ? m_pAccount->toString()
                                   : QStringLiteral("<null>")));

            killTimer(m_runSyncPeriodicallyTimerId);
            m_runSyncPeriodicallyTimerId = 0;
            return;
        }

        if (m_syncInProgress || m_pendingNewEvernoteAccountAuthentication ||
            m_pendingCurrentEvernoteAccountAuthentication ||
            m_pendingSwitchToNewEvernoteAccount || m_syncApiRateLimitExceeded)
        {
            QNDEBUG(
                "quentier:main_window",
                "Sync in progress = "
                    << (m_syncInProgress ? "true" : "false")
                    << ", pending new Evernote account authentication = "
                    << (m_pendingNewEvernoteAccountAuthentication ? "true"
                                                                  : "false")
                    << ", pending current Evernote account authentication = "
                    << (m_pendingCurrentEvernoteAccountAuthentication ? "true"
                                                                      : "false")
                    << ", pending switch to new Evernote account = "
                    << (m_pendingSwitchToNewEvernoteAccount ? "true" : "false")
                    << ", sync API rate limit exceeded = "
                    << (m_syncApiRateLimitExceeded ? "true" : "false"));
            return;
        }

        QNDEBUG("quentier:main_window", "Starting the periodically run sync");
        launchSynchronization();
    }
    else if (
        pTimerEvent->timerId() ==
        m_setDefaultAccountsFirstNoteAsCurrentDelayTimerId)
    {
        QNDEBUG(
            "quentier:main_window",
            "Executing postponed setting of defaut "
                << "account's first note as the current note");

        m_pUi->noteListView->setCurrentNoteByLocalUid(
            m_defaultAccountFirstNoteLocalUid);

        m_defaultAccountFirstNoteLocalUid.clear();
        killTimer(m_setDefaultAccountsFirstNoteAsCurrentDelayTimerId);
        m_setDefaultAccountsFirstNoteAsCurrentDelayTimerId = 0;
    }
}

void MainWindow::focusInEvent(QFocusEvent * pFocusEvent)
{
    QNDEBUG("quentier:main_window", "MainWindow::focusInEvent");

    if (Q_UNLIKELY(!pFocusEvent)) {
        return;
    }

    QNDEBUG("quentier:main_window", "Reason = " << pFocusEvent->reason());

    QMainWindow::focusInEvent(pFocusEvent);

    NoteEditorWidget * pCurrentNoteEditorTab = currentNoteEditorTab();
    if (pCurrentNoteEditorTab) {
        pCurrentNoteEditorTab->setFocusToEditor();
    }
}

void MainWindow::focusOutEvent(QFocusEvent * pFocusEvent)
{
    QNDEBUG("quentier:main_window", "MainWindow::focusOutEvent");

    if (Q_UNLIKELY(!pFocusEvent)) {
        return;
    }

    QNDEBUG("quentier:main_window", "Reason = " << pFocusEvent->reason());
    QMainWindow::focusOutEvent(pFocusEvent);
}

void MainWindow::showEvent(QShowEvent * pShowEvent)
{
    QNDEBUG("quentier:main_window", "MainWindow::showEvent");
    QMainWindow::showEvent(pShowEvent);

    Qt::WindowStates state = windowState();
    if (!(state & Qt::WindowMinimized)) {
        Q_EMIT shown();
    }
}

void MainWindow::hideEvent(QHideEvent * pHideEvent)
{
    QNDEBUG("quentier:main_window", "MainWindow::hideEvent");

    QMainWindow::hideEvent(pHideEvent);
    Q_EMIT hidden();
}

void MainWindow::changeEvent(QEvent * pEvent)
{
    QMainWindow::changeEvent(pEvent);

    if (pEvent && (pEvent->type() == QEvent::WindowStateChange)) {
        Qt::WindowStates state = windowState();
        bool minimized = (state & Qt::WindowMinimized);
        bool maximized = (state & Qt::WindowMaximized);

        QNDEBUG(
            "quentier:main_window",
            "Change event of window state change "
                << "type: "
                << "minimized = " << (minimized ? "true" : "false")
                << ", maximized = " << (maximized ? "true" : "false"));

        if (!minimized) {
            QNDEBUG(
                "quentier:main_window",
                "MainWindow is no longer "
                    << "minimized");

            if (isVisible()) {
                Q_EMIT shown();
            }

            scheduleGeometryAndStatePersisting();
        }
        else if (minimized) {
            QNDEBUG("quentier:main_window", "MainWindow became minimized");

            if (m_pSystemTrayIconManager->shouldMinimizeToSystemTray()) {
                QNDEBUG(
                    "quentier:main_window",
                    "Should minimize to system "
                        << "tray instead of the conventional minimization");

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
    QNTRACE("quentier:main_window", "MainWindow::setupThemeIcons");

    m_nativeIconThemeName = QIcon::themeName();

    QNDEBUG(
        "quentier:main_window",
        "Native icon theme name: " << m_nativeIconThemeName);

    if (!QIcon::hasThemeIcon(QStringLiteral("document-new"))) {
        QNDEBUG(
            "quentier:main_window",
            "There seems to be no native icon "
                << "theme available: document-new icon is not present within "
                << "the theme");
        m_nativeIconThemeName.clear();
    }

    ApplicationSettings appSettings(
        *m_pAccount, preferences::keys::files::userInterface);

    appSettings.beginGroup(preferences::keys::appearanceGroup);

    QString lastUsedIconThemeName =
        appSettings.value(preferences::keys::iconTheme).toString();

    appSettings.endGroup();

    QString iconThemeName;
    if (!lastUsedIconThemeName.isEmpty()) {
        QNDEBUG(
            "quentier:main_window",
            "Last chosen icon theme: " << lastUsedIconThemeName);

        iconThemeName = lastUsedIconThemeName;
    }
    else {
        QNDEBUG("quentier:main_window", "No last chosen icon theme");

        if (!m_nativeIconThemeName.isEmpty()) {
            QNDEBUG(
                "quentier:main_window",
                "Using native icon theme: " << m_nativeIconThemeName);
            iconThemeName = m_nativeIconThemeName;
        }
        else {
            iconThemeName = fallbackIconThemeName();
            QNDEBUG(
                "quentier:main_window",
                "Using fallback icon theme: " << iconThemeName);
        }
    }

    QIcon::setThemeName(iconThemeName);
}

void MainWindow::setupAccountManager()
{
    QNDEBUG("quentier:main_window", "MainWindow::setupAccountManager");

    QObject::connect(
        m_pAccountManager,
        &AccountManager::evernoteAccountAuthenticationRequested, this,
        &MainWindow::onEvernoteAccountAuthenticationRequested);

    QObject::connect(
        m_pAccountManager, &AccountManager::switchedAccount, this,
        &MainWindow::onAccountSwitched);

    QObject::connect(
        m_pAccountManager, &AccountManager::accountUpdated, this,
        &MainWindow::onAccountUpdated);

    QObject::connect(
        m_pAccountManager, &AccountManager::accountAdded, this,
        &MainWindow::onAccountAdded);

    QObject::connect(
        m_pAccountManager, &AccountManager::accountRemoved, this,
        &MainWindow::onAccountRemoved);

    QObject::connect(
        m_pAccountManager, &AccountManager::notifyError, this,
        &MainWindow::onAccountManagerError);
}

void MainWindow::setupLocalStorageManager()
{
    QNDEBUG("quentier:main_window", "MainWindow::setupLocalStorageManager");

    m_pLocalStorageManagerThread = new QThread;

    m_pLocalStorageManagerThread->setObjectName(
        QStringLiteral("LocalStorageManagerThread"));

    QObject::connect(
        m_pLocalStorageManagerThread, &QThread::finished,
        m_pLocalStorageManagerThread, &QThread::deleteLater);

    m_pLocalStorageManagerThread->start();

    m_pLocalStorageManagerAsync = new LocalStorageManagerAsync(
        *m_pAccount,
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
        LocalStorageManager::StartupOptions());
#else
        LocalStorageManager::StartupOptions(0));
#endif

    m_pLocalStorageManagerAsync->init();

    ErrorString errorDescription;

    auto & localStorageManager =
        *m_pLocalStorageManagerAsync->localStorageManager();

    if (localStorageManager.isLocalStorageVersionTooHigh(errorDescription)) {
        throw quentier::LocalStorageVersionTooHighException(errorDescription);
    }

    auto localStoragePatches =
        localStorageManager.requiredLocalStoragePatches();

    if (!localStoragePatches.isEmpty()) {
        QNDEBUG(
            "quentier:main_window",
            "Local storage requires upgrade: "
                << "detected " << localStoragePatches.size()
                << " pending local storage patches");

        auto pUpgradeDialog = std::make_unique<LocalStorageUpgradeDialog>(
            *m_pAccount, m_pAccountManager->accountModel(), localStoragePatches,
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
            LocalStorageUpgradeDialog::Options(),
#else
            LocalStorageUpgradeDialog::Options(0),
#endif
            this);

        QObject::connect(
            pUpgradeDialog.get(), &LocalStorageUpgradeDialog::shouldQuitApp,
            this, &MainWindow::onQuitAction,
            Qt::ConnectionType(Qt::UniqueConnection | Qt::QueuedConnection));

        pUpgradeDialog->adjustSize();
        Q_UNUSED(pUpgradeDialog->exec())
    }

    m_pLocalStorageManagerAsync->moveToThread(m_pLocalStorageManagerThread);

    QObject::connect(
        this, &MainWindow::localStorageSwitchUserRequest,
        m_pLocalStorageManagerAsync,
        &LocalStorageManagerAsync::onSwitchUserRequest);

    QObject::connect(
        m_pLocalStorageManagerAsync,
        &LocalStorageManagerAsync::switchUserComplete, this,
        &MainWindow::onLocalStorageSwitchUserRequestComplete);

    QObject::connect(
        m_pLocalStorageManagerAsync,
        &LocalStorageManagerAsync::switchUserFailed, this,
        &MainWindow::onLocalStorageSwitchUserRequestFailed);
}

void MainWindow::setupDisableNativeMenuBarPreference()
{
    QNDEBUG(
        "quentier:main_window",
        "MainWindow::setupDisableNativeMenuBarPreference");

    bool disableNativeMenuBar = getDisableNativeMenuBarPreference();
    m_pUi->menuBar->setNativeMenuBar(!disableNativeMenuBar);

    if (disableNativeMenuBar) {
        // Without this the menu bar forcefully integrated into the main window
        // looks kinda ugly
        m_pUi->menuBar->setStyleSheet(QString());
    }
}

void MainWindow::setupDefaultAccount()
{
    QNDEBUG("quentier:main_window", "MainWindow::setupDefaultAccount");

    auto * pDefaultAccountFirstNotebookAndNoteCreator =
        new DefaultAccountFirstNotebookAndNoteCreator(
            *m_pLocalStorageManagerAsync, *m_pNoteFiltersManager, this);

    QObject::connect(
        pDefaultAccountFirstNotebookAndNoteCreator,
        &DefaultAccountFirstNotebookAndNoteCreator::finished, this,
        &MainWindow::onDefaultAccountFirstNotebookAndNoteCreatorFinished);

    QObject::connect(
        pDefaultAccountFirstNotebookAndNoteCreator,
        &DefaultAccountFirstNotebookAndNoteCreator::notifyError, this,
        &MainWindow::onDefaultAccountFirstNotebookAndNoteCreatorError);

    pDefaultAccountFirstNotebookAndNoteCreator->start();
}

void MainWindow::setupModels()
{
    QNDEBUG("quentier:main_window", "MainWindow::setupModels");

    clearModels();

    auto noteSortingMode = restoreNoteSortingMode();
    if (noteSortingMode == NoteModel::NoteSortingMode::None) {
        noteSortingMode = NoteModel::NoteSortingMode::ModifiedDescending;
    }

    m_pNoteModel = new NoteModel(
        *m_pAccount, *m_pLocalStorageManagerAsync, m_noteCache, m_notebookCache,
        this, NoteModel::IncludedNotes::NonDeleted, noteSortingMode);

    m_pFavoritesModel = new FavoritesModel(
        *m_pAccount, *m_pLocalStorageManagerAsync, m_noteCache, m_notebookCache,
        m_tagCache, m_savedSearchCache, this);

    m_pNotebookModel = new NotebookModel(
        *m_pAccount, *m_pLocalStorageManagerAsync, m_notebookCache, this);

    m_pTagModel = new TagModel(
        *m_pAccount, *m_pLocalStorageManagerAsync, m_tagCache, this);

    m_pSavedSearchModel = new SavedSearchModel(
        *m_pAccount, *m_pLocalStorageManagerAsync, m_savedSearchCache, this);

    m_pDeletedNotesModel = new NoteModel(
        *m_pAccount, *m_pLocalStorageManagerAsync, m_noteCache, m_notebookCache,
        this, NoteModel::IncludedNotes::Deleted);

    m_pDeletedNotesModel->start();

    if (m_pNoteCountLabelController == nullptr) {
        m_pNoteCountLabelController =
            new NoteCountLabelController(*m_pUi->notesCountLabelPanel, this);
    }

    m_pNoteCountLabelController->setNoteModel(*m_pNoteModel);

    setupNoteFilters();

    m_pUi->favoritesTableView->setModel(m_pFavoritesModel);
    m_pUi->notebooksTreeView->setModel(m_pNotebookModel);
    m_pUi->tagsTreeView->setModel(m_pTagModel);
    m_pUi->savedSearchesItemView->setModel(m_pSavedSearchModel);
    m_pUi->deletedNotesTableView->setModel(m_pDeletedNotesModel);
    m_pUi->noteListView->setModel(m_pNoteModel);

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
    QNDEBUG("quentier:main_window", "MainWindow::clearModels");

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
    QNDEBUG("quentier:main_window", "MainWindow::setupShowHideStartupSettings");

    ApplicationSettings appSettings(
        *m_pAccount, preferences::keys::files::userInterface);

    appSettings.beginGroup(QStringLiteral("MainWindow"));

#define CHECK_AND_SET_SHOW_SETTING(name, action, widget)                       \
    {                                                                          \
        auto showSetting = appSettings.value(name);                            \
        if (showSetting.isNull()) {                                            \
            showSetting = m_pUi->Action##action->isChecked();                  \
        }                                                                      \
        if (showSetting.toBool()) {                                            \
            m_pUi->widget->show();                                             \
        }                                                                      \
        else {                                                                 \
            m_pUi->widget->hide();                                             \
        }                                                                      \
        m_pUi->Action##action->setChecked(showSetting.toBool());               \
    }

    CHECK_AND_SET_SHOW_SETTING(
        QStringLiteral("ShowSidePanel"), ShowSidePanel, sidePanelSplitter)

    CHECK_AND_SET_SHOW_SETTING(
        QStringLiteral("ShowFavorites"), ShowFavorites, favoritesWidget)

    CHECK_AND_SET_SHOW_SETTING(
        QStringLiteral("ShowNotebooks"), ShowNotebooks, notebooksWidget)

    CHECK_AND_SET_SHOW_SETTING(QStringLiteral("ShowTags"), ShowTags, tagsWidget)

    CHECK_AND_SET_SHOW_SETTING(
        QStringLiteral("ShowSavedSearches"), ShowSavedSearches,
        savedSearchesWidget)

    CHECK_AND_SET_SHOW_SETTING(
        QStringLiteral("ShowDeletedNotes"), ShowDeletedNotes,
        deletedNotesWidget)

    CHECK_AND_SET_SHOW_SETTING(
        QStringLiteral("ShowNotesList"), ShowNotesList,
        notesListAndFiltersFrame)

    CHECK_AND_SET_SHOW_SETTING(
        QStringLiteral("ShowToolbar"), ShowToolbar, upperBarGenericPanel)

    CHECK_AND_SET_SHOW_SETTING(
        QStringLiteral("ShowStatusBar"), ShowStatusBar, upperBarGenericPanel)

#undef CHECK_AND_SET_SHOW_SETTING

    appSettings.endGroup();
}

void MainWindow::setupViews()
{
    QNDEBUG("quentier:main_window", "MainWindow::setupViews");

    // NOTE: only a few columns would be shown for each view because otherwise
    // there are problems finding space for everything
    // TODO: in future should implement the persistent setting of which columns
    // to show or not to show

    auto * pFavoritesTableView = m_pUi->favoritesTableView;
    pFavoritesTableView->setNoteFiltersManager(*m_pNoteFiltersManager);

    auto * pPreviousFavoriteItemDelegate = pFavoritesTableView->itemDelegate();

    auto * pFavoriteItemDelegate =
        qobject_cast<FavoriteItemDelegate *>(pPreviousFavoriteItemDelegate);

    if (!pFavoriteItemDelegate) {
        pFavoriteItemDelegate = new FavoriteItemDelegate(pFavoritesTableView);
        pFavoritesTableView->setItemDelegate(pFavoriteItemDelegate);

        if (pPreviousFavoriteItemDelegate) {
            pPreviousFavoriteItemDelegate->deleteLater();
            pPreviousFavoriteItemDelegate = nullptr;
        }
    }

    // This column's values would be displayed along with the favorite item's
    // name
    pFavoritesTableView->setColumnHidden(
        static_cast<int>(FavoritesModel::Column::NoteCount), true);

    pFavoritesTableView->setColumnWidth(
        static_cast<int>(FavoritesModel::Column::Type),
        pFavoriteItemDelegate->sideSize());

    QObject::connect(
        m_pFavoritesModelColumnChangeRerouter,
        &ColumnChangeRerouter::dataChanged, pFavoritesTableView,
        &FavoriteItemView::dataChanged, Qt::UniqueConnection);

    pFavoritesTableView->header()->setSectionResizeMode(
        QHeaderView::ResizeToContents);

    QObject::connect(
        pFavoritesTableView, &FavoriteItemView::notifyError, this,
        &MainWindow::onModelViewError, Qt::UniqueConnection);

    QObject::connect(
        pFavoritesTableView, &FavoriteItemView::favoritedItemInfoRequested,
        this, &MainWindow::onFavoritedItemInfoButtonPressed,
        Qt::UniqueConnection);

    QObject::connect(
        pFavoritesTableView, &FavoriteItemView::favoritedNoteSelected, this,
        &MainWindow::onFavoritedNoteSelected, Qt::UniqueConnection);

    auto * pNotebooksTreeView = m_pUi->notebooksTreeView;
    pNotebooksTreeView->setNoteFiltersManager(*m_pNoteFiltersManager);
    pNotebooksTreeView->setNoteModel(m_pNoteModel);

    auto * pPreviousNotebookItemDelegate = pNotebooksTreeView->itemDelegate();

    auto * pNotebookItemDelegate =
        qobject_cast<NotebookItemDelegate *>(pPreviousNotebookItemDelegate);

    if (!pNotebookItemDelegate) {
        pNotebookItemDelegate = new NotebookItemDelegate(pNotebooksTreeView);
        pNotebooksTreeView->setItemDelegate(pNotebookItemDelegate);

        if (pPreviousNotebookItemDelegate) {
            pPreviousNotebookItemDelegate->deleteLater();
            pPreviousNotebookItemDelegate = nullptr;
        }
    }

    // This column's values would be displayed along with the notebook's name
    pNotebooksTreeView->setColumnHidden(
        static_cast<int>(NotebookModel::Column::NoteCount), true);

    pNotebooksTreeView->setColumnHidden(
        static_cast<int>(NotebookModel::Column::Synchronizable), true);

    pNotebooksTreeView->setColumnHidden(
        static_cast<int>(NotebookModel::Column::LastUsed), true);

    pNotebooksTreeView->setColumnHidden(
        static_cast<int>(NotebookModel::Column::FromLinkedNotebook), true);

    auto * pPreviousNotebookDirtyColumnDelegate =
        pNotebooksTreeView->itemDelegateForColumn(
            static_cast<int>(NotebookModel::Column::Dirty));

    auto * pNotebookDirtyColumnDelegate = qobject_cast<DirtyColumnDelegate *>(
        pPreviousNotebookDirtyColumnDelegate);

    if (!pNotebookDirtyColumnDelegate) {
        pNotebookDirtyColumnDelegate =
            new DirtyColumnDelegate(pNotebooksTreeView);

        pNotebooksTreeView->setItemDelegateForColumn(
            static_cast<int>(NotebookModel::Column::Dirty),
            pNotebookDirtyColumnDelegate);

        if (pPreviousNotebookDirtyColumnDelegate) {
            pPreviousNotebookDirtyColumnDelegate->deleteLater();
            pPreviousNotebookDirtyColumnDelegate = nullptr;
        }
    }

    pNotebooksTreeView->setColumnWidth(
        static_cast<int>(NotebookModel::Column::Dirty),
        pNotebookDirtyColumnDelegate->sideSize());

    pNotebooksTreeView->header()->setSectionResizeMode(
        QHeaderView::ResizeToContents);

    QObject::connect(
        m_pNotebookModelColumnChangeRerouter,
        &ColumnChangeRerouter::dataChanged, pNotebooksTreeView,
        &NotebookItemView::dataChanged, Qt::UniqueConnection);

    QObject::connect(
        pNotebooksTreeView, &NotebookItemView::newNotebookCreationRequested,
        this, &MainWindow::onNewNotebookCreationRequested,
        Qt::UniqueConnection);

    QObject::connect(
        pNotebooksTreeView, &NotebookItemView::notebookInfoRequested, this,
        &MainWindow::onNotebookInfoButtonPressed, Qt::UniqueConnection);

    QObject::connect(
        pNotebooksTreeView, &NotebookItemView::notifyError, this,
        &MainWindow::onModelViewError, Qt::UniqueConnection);

    auto * pTagsTreeView = m_pUi->tagsTreeView;
    pTagsTreeView->setNoteFiltersManager(*m_pNoteFiltersManager);

    // These columns' values would be displayed along with the tag's name
    pTagsTreeView->setColumnHidden(
        static_cast<int>(TagModel::Column::NoteCount), true);

    pTagsTreeView->setColumnHidden(
        static_cast<int>(TagModel::Column::Synchronizable), true);

    pTagsTreeView->setColumnHidden(
        static_cast<int>(TagModel::Column::FromLinkedNotebook), true);

    auto * pPreviousTagDirtyColumnDelegate =
        pTagsTreeView->itemDelegateForColumn(
            static_cast<int>(TagModel::Column::Dirty));

    auto * pTagDirtyColumnDelegate =
        qobject_cast<DirtyColumnDelegate *>(pPreviousTagDirtyColumnDelegate);

    if (!pTagDirtyColumnDelegate) {
        pTagDirtyColumnDelegate = new DirtyColumnDelegate(pTagsTreeView);

        pTagsTreeView->setItemDelegateForColumn(
            static_cast<int>(TagModel::Column::Dirty), pTagDirtyColumnDelegate);

        if (pPreviousTagDirtyColumnDelegate) {
            pPreviousTagDirtyColumnDelegate->deleteLater();
            pPreviousTagDirtyColumnDelegate = nullptr;
        }
    }

    pTagsTreeView->setColumnWidth(
        static_cast<int>(TagModel::Column::Dirty),
        pTagDirtyColumnDelegate->sideSize());

    auto * pPreviousTagItemDelegate = pTagsTreeView->itemDelegateForColumn(
        static_cast<int>(TagModel::Column::Name));

    auto * pTagItemDelegate =
        qobject_cast<TagItemDelegate *>(pPreviousTagItemDelegate);

    if (!pTagItemDelegate) {
        pTagItemDelegate = new TagItemDelegate(pTagsTreeView);

        pTagsTreeView->setItemDelegateForColumn(
            static_cast<int>(TagModel::Column::Name), pTagItemDelegate);

        if (pPreviousTagItemDelegate) {
            pPreviousTagItemDelegate->deleteLater();
            pPreviousTagItemDelegate = nullptr;
        }
    }

    pTagsTreeView->header()->setSectionResizeMode(
        QHeaderView::ResizeToContents);

    QObject::connect(
        m_pTagModelColumnChangeRerouter, &ColumnChangeRerouter::dataChanged,
        pTagsTreeView, &TagItemView::dataChanged, Qt::UniqueConnection);

    QObject::connect(
        pTagsTreeView, &TagItemView::newTagCreationRequested, this,
        &MainWindow::onNewTagCreationRequested, Qt::UniqueConnection);

    QObject::connect(
        pTagsTreeView, &TagItemView::tagInfoRequested, this,
        &MainWindow::onTagInfoButtonPressed, Qt::UniqueConnection);

    QObject::connect(
        pTagsTreeView, &TagItemView::notifyError, this,
        &MainWindow::onModelViewError, Qt::UniqueConnection);

    auto * pSavedSearchesItemView = m_pUi->savedSearchesItemView;
    pSavedSearchesItemView->setNoteFiltersManager(*m_pNoteFiltersManager);

    pSavedSearchesItemView->setColumnHidden(
        static_cast<int>(SavedSearchModel::Column::Query), true);

    pSavedSearchesItemView->setColumnHidden(
        static_cast<int>(SavedSearchModel::Column::Synchronizable), true);

    auto * pPreviousSavedSearchDirtyColumnDelegate =
        pSavedSearchesItemView->itemDelegateForColumn(
            static_cast<int>(SavedSearchModel::Column::Dirty));

    auto * pSavedSearchDirtyColumnDelegate =
        qobject_cast<DirtyColumnDelegate *>(
            pPreviousSavedSearchDirtyColumnDelegate);

    if (!pSavedSearchDirtyColumnDelegate) {
        pSavedSearchDirtyColumnDelegate =
            new DirtyColumnDelegate(pSavedSearchesItemView);

        pSavedSearchesItemView->setItemDelegateForColumn(
            static_cast<int>(SavedSearchModel::Column::Dirty),
            pSavedSearchDirtyColumnDelegate);

        if (pPreviousSavedSearchDirtyColumnDelegate) {
            pPreviousSavedSearchDirtyColumnDelegate->deleteLater();
            pPreviousSavedSearchDirtyColumnDelegate = nullptr;
        }
    }

    pSavedSearchesItemView->setColumnWidth(
        static_cast<int>(SavedSearchModel::Column::Dirty),
        pSavedSearchDirtyColumnDelegate->sideSize());

    pSavedSearchesItemView->header()->setSectionResizeMode(
        QHeaderView::ResizeToContents);

    QObject::connect(
        pSavedSearchesItemView, &SavedSearchItemView::savedSearchInfoRequested,
        this, &MainWindow::onSavedSearchInfoButtonPressed,
        Qt::UniqueConnection);

    QObject::connect(
        pSavedSearchesItemView,
        &SavedSearchItemView::newSavedSearchCreationRequested, this,
        &MainWindow::onNewSavedSearchCreationRequested, Qt::UniqueConnection);

    QObject::connect(
        pSavedSearchesItemView, &SavedSearchItemView::notifyError, this,
        &MainWindow::onModelViewError, Qt::UniqueConnection);

    auto * pNoteListView = m_pUi->noteListView;
    if (m_pAccount) {
        pNoteListView->setCurrentAccount(*m_pAccount);
    }

    auto * pPreviousNoteItemDelegate = pNoteListView->itemDelegate();

    auto * pNoteItemDelegate =
        qobject_cast<NoteItemDelegate *>(pPreviousNoteItemDelegate);

    if (!pNoteItemDelegate) {
        pNoteItemDelegate = new NoteItemDelegate(pNoteListView);
        pNoteListView->setModelColumn(NoteModel::Columns::Title);
        pNoteListView->setItemDelegate(pNoteItemDelegate);

        if (pPreviousNoteItemDelegate) {
            pPreviousNoteItemDelegate->deleteLater();
            pPreviousNoteItemDelegate = nullptr;
        }
    }

    pNoteListView->setNotebookItemView(pNotebooksTreeView);

    QObject::connect(
        m_pNoteModelColumnChangeRerouter, &ColumnChangeRerouter::dataChanged,
        pNoteListView, &NoteListView::dataChanged, Qt::UniqueConnection);

    QObject::connect(
        pNoteListView, &NoteListView::currentNoteChanged, this,
        &MainWindow::onCurrentNoteInListChanged, Qt::UniqueConnection);

    QObject::connect(
        pNoteListView, &NoteListView::openNoteInSeparateWindowRequested, this,
        &MainWindow::onOpenNoteInSeparateWindow, Qt::UniqueConnection);

    QObject::connect(
        pNoteListView, &NoteListView::enexExportRequested, this,
        &MainWindow::onExportNotesToEnexRequested, Qt::UniqueConnection);

    QObject::connect(
        pNoteListView, &NoteListView::newNoteCreationRequested, this,
        &MainWindow::onNewNoteCreationRequested, Qt::UniqueConnection);

    QObject::connect(
        pNoteListView, &NoteListView::copyInAppNoteLinkRequested, this,
        &MainWindow::onCopyInAppLinkNoteRequested, Qt::UniqueConnection);

    QObject::connect(
        pNoteListView, &NoteListView::toggleThumbnailsPreference, this,
        &MainWindow::onToggleThumbnailsPreference, Qt::UniqueConnection);

    QObject::connect(
        this, &MainWindow::showNoteThumbnailsStateChanged, pNoteListView,
        &NoteListView::setShowNoteThumbnailsState, Qt::UniqueConnection);

    QObject::connect(
        this, &MainWindow::showNoteThumbnailsStateChanged, pNoteItemDelegate,
        &NoteItemDelegate::setShowNoteThumbnailsState, Qt::UniqueConnection);

    if (!m_onceSetupNoteSortingModeComboBox) {
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

        auto * pNoteSortingModeModel = new QStringListModel(this);
        pNoteSortingModeModel->setStringList(noteSortingModes);

        m_pUi->noteSortingModeComboBox->setModel(pNoteSortingModeModel);
        m_onceSetupNoteSortingModeComboBox = true;
    }

    auto noteSortingMode = restoreNoteSortingMode();
    if (noteSortingMode == NoteModel::NoteSortingMode::None) {
        noteSortingMode = NoteModel::NoteSortingMode::ModifiedDescending;
        QNDEBUG(
            "quentier:main_window",
            "Couldn't restore the note sorting "
                << "mode, fallback to the default one of " << noteSortingMode);
    }

    m_pUi->noteSortingModeComboBox->setCurrentIndex(noteSortingMode);

#if QT_VERSION >= QT_VERSION_CHECK(5, 7, 0)
    QObject::connect(
        m_pUi->noteSortingModeComboBox,
        qOverload<int>(&QComboBox::currentIndexChanged), this,
        &MainWindow::onNoteSortingModeChanged, Qt::UniqueConnection);
#else
    QObject::connect(
        m_pUi->noteSortingModeComboBox, SIGNAL(currentIndexChanged(int)), this,
        SLOT(onNoteSortingModeChanged(int)), Qt::UniqueConnection);
#endif

    onNoteSortingModeChanged(m_pUi->noteSortingModeComboBox->currentIndex());

    auto * pDeletedNotesTableView = m_pUi->deletedNotesTableView;

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

    pDeletedNotesTableView->setColumnHidden(NoteModel::Columns::Size, true);

    pDeletedNotesTableView->setColumnHidden(
        NoteModel::Columns::Synchronizable, true);

    pDeletedNotesTableView->setColumnHidden(
        NoteModel::Columns::NotebookName, true);

    auto * pPreviousDeletedNoteItemDelegate =
        pDeletedNotesTableView->itemDelegate();

    auto * pDeletedNoteItemDelegate = qobject_cast<DeletedNoteItemDelegate *>(
        pPreviousDeletedNoteItemDelegate);

    if (!pDeletedNoteItemDelegate) {
        pDeletedNoteItemDelegate =
            new DeletedNoteItemDelegate(pDeletedNotesTableView);

        pDeletedNotesTableView->setItemDelegate(pDeletedNoteItemDelegate);

        if (pPreviousDeletedNoteItemDelegate) {
            pPreviousDeletedNoteItemDelegate->deleteLater();
            pPreviousDeletedNoteItemDelegate = nullptr;
        }
    }

    pDeletedNotesTableView->header()->setSectionResizeMode(
        QHeaderView::ResizeToContents);

    if (!m_pEditNoteDialogsManager) {
        m_pEditNoteDialogsManager = new EditNoteDialogsManager(
            *m_pLocalStorageManagerAsync, m_noteCache, m_pNotebookModel, this);

        QObject::connect(
            pNoteListView, &NoteListView::editNoteDialogRequested,
            m_pEditNoteDialogsManager,
            &EditNoteDialogsManager::onEditNoteDialogRequested,
            Qt::UniqueConnection);

        QObject::connect(
            pNoteListView, &NoteListView::noteInfoDialogRequested,
            m_pEditNoteDialogsManager,
            &EditNoteDialogsManager::onNoteInfoDialogRequested,
            Qt::UniqueConnection);

        QObject::connect(
            this, &MainWindow::noteInfoDialogRequested,
            m_pEditNoteDialogsManager,
            &EditNoteDialogsManager::onNoteInfoDialogRequested,
            Qt::UniqueConnection);

        QObject::connect(
            pDeletedNotesTableView,
            &DeletedNoteItemView::deletedNoteInfoRequested,
            m_pEditNoteDialogsManager,
            &EditNoteDialogsManager::onNoteInfoDialogRequested,
            Qt::UniqueConnection);
    }

    auto currentAccountType = Account::Type::Local;
    if (m_pAccount) {
        currentAccountType = m_pAccount->type();
    }

    showHideViewColumnsForAccountType(currentAccountType);
}

bool MainWindow::getShowNoteThumbnailsPreference() const
{
    ApplicationSettings appSettings(
        *m_pAccount, preferences::keys::files::userInterface);

    appSettings.beginGroup(preferences::keys::appearanceGroup);

    QVariant showThumbnails = appSettings.value(
        preferences::keys::showNoteThumbnails,
        QVariant::fromValue(preferences::defaults::showNoteThumbnails));

    appSettings.endGroup();

    return showThumbnails.toBool();
}

bool MainWindow::getDisableNativeMenuBarPreference() const
{
    ApplicationSettings appSettings(
        *m_pAccount, preferences::keys::files::userInterface);

    appSettings.beginGroup(preferences::keys::appearanceGroup);

    QVariant disableNativeMenuBar = appSettings.value(
        preferences::keys::disableNativeMenuBar,
        QVariant::fromValue(preferences::defaults::disableNativeMenuBar()));

    appSettings.endGroup();

    return disableNativeMenuBar.toBool();
}

QSet<QString> MainWindow::notesWithHiddenThumbnails() const
{
    ApplicationSettings appSettings(
        *m_pAccount, preferences::keys::files::userInterface);

    appSettings.beginGroup(preferences::keys::appearanceGroup);

    QVariant hideThumbnailsFor = appSettings.value(
        preferences::keys::notesWithHiddenThumbnails, QLatin1String(""));

    appSettings.endGroup();

    auto hideThumbnailsForList = hideThumbnailsFor.toStringList();

#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    return QSet<QString>(
        hideThumbnailsForList.begin(), hideThumbnailsForList.end());
#else
    return QSet<QString>::fromList(hideThumbnailsForList);
#endif
}

void MainWindow::toggleShowNoteThumbnails() const
{
    bool newValue = !getShowNoteThumbnailsPreference();

    ApplicationSettings appSettings(
        *m_pAccount, preferences::keys::files::userInterface);

    appSettings.beginGroup(preferences::keys::appearanceGroup);

    appSettings.setValue(
        preferences::keys::showNoteThumbnails, QVariant::fromValue(newValue));

    appSettings.endGroup();
}

void MainWindow::toggleHideNoteThumbnail(const QString & noteLocalUid)
{
    auto noteLocalUids = notesWithHiddenThumbnails();
    auto it = noteLocalUids.find(noteLocalUid);
    if (it != noteLocalUids.end()) {
        noteLocalUids.erase(it);
    }
    else {
        if (noteLocalUids.size() <=
            preferences::keys::maxNotesWithHiddenThumbnails) {
            noteLocalUids.insert(noteLocalUid);
        }
        else {
            Q_UNUSED(informationMessageBox(
                this, tr("Cannot disable thumbnail for note"),
                tr("Too many notes with hidden thumbnails"),
                tr("There are too many notes for which thumbnails are hidden "
                   "already. Consider disabling note thumbnails globally if "
                   "you don't want to see them.")))
            return;
        }
    }

    ApplicationSettings appSettings(
        *m_pAccount, preferences::keys::files::userInterface);

    appSettings.beginGroup(preferences::keys::appearanceGroup);

    appSettings.setValue(
        preferences::keys::notesWithHiddenThumbnails,
        QStringList(noteLocalUids.values()));

    appSettings.endGroup();
}

QString MainWindow::fallbackIconThemeName() const
{
    const auto & pal = palette();
    const auto & windowColor = pal.color(QPalette::Window);

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

void MainWindow::quitApp(int exitCode)
{
    if (m_pNoteEditorTabsAndWindowsCoordinator) {
        // That would save the modified notes
        m_pNoteEditorTabsAndWindowsCoordinator->clear();
    }

    qApp->exit(exitCode);
}

void MainWindow::clearViews()
{
    QNDEBUG("quentier:main_window", "MainWindow::clearViews");

    m_pUi->favoritesTableView->setModel(&m_blankModel);
    m_pUi->notebooksTreeView->setModel(&m_blankModel);
    m_pUi->tagsTreeView->setModel(&m_blankModel);
    m_pUi->savedSearchesItemView->setModel(&m_blankModel);

    m_pUi->noteListView->setModel(&m_blankModel);
    // NOTE: without this the note list view doesn't seem to re-render
    // so the items from the previously set model are still displayed
    m_pUi->noteListView->update();

    m_pUi->deletedNotesTableView->setModel(&m_blankModel);
}

void MainWindow::setupAccountSpecificUiElements()
{
    QNDEBUG(
        "quentier:main_window", "MainWindow::setupAccountSpecificUiElements");

    if (Q_UNLIKELY(!m_pAccount)) {
        QNDEBUG("quentier:main_window", "No account");
        return;
    }

    bool isLocal = (m_pAccount->type() == Account::Type::Local);

    m_pUi->removeNotebookButton->setHidden(!isLocal);
    m_pUi->removeNotebookButton->setDisabled(!isLocal);

    m_pUi->removeTagButton->setHidden(!isLocal);
    m_pUi->removeTagButton->setDisabled(!isLocal);

    m_pUi->removeSavedSearchButton->setHidden(!isLocal);
    m_pUi->removeSavedSearchButton->setDisabled(!isLocal);

    m_pUi->eraseDeletedNoteButton->setHidden(!isLocal);
    m_pUi->eraseDeletedNoteButton->setDisabled(!isLocal);

    m_pUi->syncPushButton->setHidden(isLocal);
    m_pUi->syncPushButton->setDisabled(isLocal);

    m_pUi->ActionSynchronize->setVisible(!isLocal);
    m_pUi->menuService->menuAction()->setVisible(!isLocal);
}

void MainWindow::setupNoteFilters()
{
    QNDEBUG("quentier:main_window", "MainWindow::setupNoteFilters");

    m_pUi->filterFrameBottomBoundary->hide();

    m_pUi->filterByNotebooksWidget->setLocalStorageManager(
        *m_pLocalStorageManagerAsync);

    m_pUi->filterByTagsWidget->setLocalStorageManager(
        *m_pLocalStorageManagerAsync);

    m_pUi->filterByNotebooksWidget->switchAccount(
        *m_pAccount, m_pNotebookModel);

    m_pUi->filterByTagsWidget->switchAccount(*m_pAccount, m_pTagModel);

    m_pUi->filterBySavedSearchComboBox->switchAccount(
        *m_pAccount, m_pSavedSearchModel);

    m_pUi->filterStatusBarLabel->hide();

    if (m_pNoteFiltersManager) {
        m_pNoteFiltersManager->disconnect();
        m_pNoteFiltersManager->deleteLater();
    }

    m_pNoteFiltersManager = new NoteFiltersManager(
        *m_pAccount, *m_pUi->filterByTagsWidget,
        *m_pUi->filterByNotebooksWidget, *m_pNoteModel,
        *m_pUi->filterBySavedSearchComboBox, *m_pUi->filterBySearchStringWidget,
        *m_pLocalStorageManagerAsync, this);

    m_pNoteModel->start();

    ApplicationSettings appSettings(
        *m_pAccount, preferences::keys::files::userInterface);

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
    QNDEBUG(
        "quentier:main_window",
        "MainWindow::setupNoteEditorTabWidgetsCoordinator");

    delete m_pNoteEditorTabsAndWindowsCoordinator;

    m_pNoteEditorTabsAndWindowsCoordinator =
        new NoteEditorTabsAndWindowsCoordinator(
            *m_pAccount, *m_pLocalStorageManagerAsync, m_noteCache,
            m_notebookCache, m_tagCache, *m_pTagModel,
            m_pUi->noteEditorsTabWidget, this);

    QObject::connect(
        m_pNoteEditorTabsAndWindowsCoordinator,
        &NoteEditorTabsAndWindowsCoordinator::notifyError, this,
        &MainWindow::onNoteEditorError);

    QObject::connect(
        m_pNoteEditorTabsAndWindowsCoordinator,
        &NoteEditorTabsAndWindowsCoordinator::currentNoteChanged,
        m_pUi->noteListView, &NoteListView::setCurrentNoteByLocalUid);
}

#ifdef WITH_UPDATE_MANAGER
void MainWindow::setupUpdateManager()
{
    QNDEBUG("quentier:main_window", "MainWindow::setupUpdateManager");

    Q_ASSERT(m_pNoteEditorTabsAndWindowsCoordinator);

    m_pUpdateManagerIdleInfoProvider.reset(new UpdateManagerIdleInfoProvider(
        *m_pNoteEditorTabsAndWindowsCoordinator));

    m_pUpdateManager =
        new UpdateManager(m_pUpdateManagerIdleInfoProvider, this);

    QObject::connect(
        m_pUpdateManager, &UpdateManager::notifyError, this,
        &MainWindow::onUpdateManagerError);

    QObject::connect(
        m_pUpdateManager, &UpdateManager::restartAfterUpdateRequested, this,
        &MainWindow::onUpdateManagerRequestsRestart);
}
#endif

bool MainWindow::checkLocalStorageVersion(const Account & account)
{
    QNDEBUG(
        "quentier:main_window",
        "MainWindow::checkLocalStorageVersion: "
            << "account = " << account);

    auto * pLocalStorageManager =
        m_pLocalStorageManagerAsync->localStorageManager();

    ErrorString errorDescription;

    if (pLocalStorageManager->isLocalStorageVersionTooHigh(errorDescription)) {
        QNINFO(
            "quentier:main_window",
            "Detected too high local storage "
                << "version: " << errorDescription << "; account: " << account);

        auto pVersionTooHighDialog =
            std::make_unique<LocalStorageVersionTooHighDialog>(
                account, m_pAccountManager->accountModel(),
                *pLocalStorageManager, this);

        QObject::connect(
            pVersionTooHighDialog.get(),
            &LocalStorageVersionTooHighDialog::shouldSwitchToAccount, this,
            &MainWindow::onAccountSwitchRequested,
            Qt::ConnectionType(Qt::UniqueConnection | Qt::QueuedConnection));

        QObject::connect(
            pVersionTooHighDialog.get(),
            &LocalStorageVersionTooHighDialog::shouldCreateNewAccount, this,
            &MainWindow::onNewAccountCreationRequested,
            Qt::ConnectionType(Qt::UniqueConnection | Qt::QueuedConnection));

        QObject::connect(
            pVersionTooHighDialog.get(),
            &LocalStorageVersionTooHighDialog::shouldQuitApp, this,
            &MainWindow::onQuitAction,
            Qt::ConnectionType(Qt::UniqueConnection | Qt::QueuedConnection));

        Q_UNUSED(pVersionTooHighDialog->exec())
        return false;
    }

    errorDescription.clear();

    auto localStoragePatches =
        pLocalStorageManager->requiredLocalStoragePatches();

    if (!localStoragePatches.isEmpty()) {
        QNDEBUG(
            "quentier:main_window",
            "Local storage requires upgrade: "
                << "detected " << localStoragePatches.size()
                << " pending local storage patches");

        LocalStorageUpgradeDialog::Options options(
            LocalStorageUpgradeDialog::Option::AddAccount |
            LocalStorageUpgradeDialog::Option::SwitchToAnotherAccount);

        auto pUpgradeDialog = std::make_unique<LocalStorageUpgradeDialog>(
            account, m_pAccountManager->accountModel(), localStoragePatches,
            options, this);

        QObject::connect(
            pUpgradeDialog.get(),
            &LocalStorageUpgradeDialog::shouldSwitchToAccount, this,
            &MainWindow::onAccountSwitchRequested,
            Qt::ConnectionType(Qt::UniqueConnection | Qt::QueuedConnection));

        QObject::connect(
            pUpgradeDialog.get(),
            &LocalStorageUpgradeDialog::shouldCreateNewAccount, this,
            &MainWindow::onNewAccountCreationRequested,
            Qt::ConnectionType(Qt::UniqueConnection | Qt::QueuedConnection));

        QObject::connect(
            pUpgradeDialog.get(), &LocalStorageUpgradeDialog::shouldQuitApp,
            this, &MainWindow::onQuitAction,
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
    appSettings.beginGroup(preferences::keys::accountGroup);

    bool result = false;
    if (appSettings.contains(preferences::keys::onceDisplayedWelcomeDialog)) {
        result =
            appSettings.value(preferences::keys::onceDisplayedWelcomeDialog)
                .toBool();
    }

    appSettings.endGroup();
    return result;
}

void MainWindow::setOnceDisplayedGreeterScreen()
{
    QNDEBUG(
        "quentier:main_window", "MainWindow::setOnceDisplayedGreeterScreen");

    ApplicationSettings appSettings;
    appSettings.beginGroup(preferences::keys::accountGroup);
    appSettings.setValue(preferences::keys::onceDisplayedWelcomeDialog, true);
    appSettings.endGroup();
}

void MainWindow::setupSynchronizationManager(
    const SetAccountOption::type setAccountOption)
{
    QNDEBUG(
        "quentier:main_window",
        "MainWindow::setupSynchronizationManager: "
            << "set account option = " << setAccountOption);

    clearSynchronizationManager();

    if (m_synchronizationManagerHost.isEmpty()) {
        if (Q_UNLIKELY(!m_pAccount)) {
            ErrorString error(
                QT_TR_NOOP("Can't set up the synchronization: no account"));
            QNWARNING("quentier:main_window", error);
            onSetStatusBarText(
                error.localizedString(), secondsToMilliseconds(30));
            return;
        }

        if (Q_UNLIKELY(m_pAccount->type() != Account::Type::Evernote)) {
            ErrorString error(
                QT_TR_NOOP("Can't set up the synchronization: non-Evernote "
                           "account is chosen"));

            QNWARNING(
                "quentier:main_window", error << "; account: " << *m_pAccount);

            onSetStatusBarText(
                error.localizedString(), secondsToMilliseconds(30));
            return;
        }

        m_synchronizationManagerHost = m_pAccount->evernoteHost();
        if (Q_UNLIKELY(m_synchronizationManagerHost.isEmpty())) {
            ErrorString error(
                QT_TR_NOOP("Can't set up the synchronization: "
                           "no Evernote host within the account"));

            QNWARNING(
                "quentier:main_window", error << "; account: " << *m_pAccount);

            onSetStatusBarText(
                error.localizedString(), secondsToMilliseconds(30));
            return;
        }
    }

    QString consumerKey, consumerSecret;
    setupConsumerKeyAndSecret(consumerKey, consumerSecret);

    if (Q_UNLIKELY(consumerKey.isEmpty())) {
        QNDEBUG("quentier:main_window", "Consumer key is empty");
        return;
    }

    if (Q_UNLIKELY(consumerSecret.isEmpty())) {
        QNDEBUG("quentier:main_window", "Consumer secret is empty");
        return;
    }

    m_pAuthenticationManager = new AuthenticationManager(
        consumerKey, consumerSecret, m_synchronizationManagerHost, this);

    m_pSynchronizationManager = new SynchronizationManager(
        m_synchronizationManagerHost, *m_pLocalStorageManagerAsync,
        *m_pAuthenticationManager, nullptr, nullptr, nullptr, newKeychain());

    if (m_pAccount && (setAccountOption == SetAccountOption::Set)) {
        m_pSynchronizationManager->setAccount(*m_pAccount);
    }

    connectSynchronizationManager();
    setupRunSyncPeriodicallyTimer();
}

void MainWindow::clearSynchronizationManager()
{
    QNDEBUG("quentier:main_window", "MainWindow::clearSynchronizationManager");

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
        QObject::disconnect(
            m_pSynchronizationManagerThread, &QThread::finished,
            m_pSynchronizationManagerThread, &QThread::deleteLater);

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

void MainWindow::clearSynchronizationCounters()
{
    QNDEBUG("quentier:main_window", "MainWindow::clearSynchronizationCounters");

    m_lastSyncNotesDownloadedPercentage = 0.0;
    m_lastSyncResourcesDownloadedPercentage = 0.0;
    m_lastSyncLinkedNotebookNotesDownloadedPercentage = 0.0;
    m_syncChunksDownloadedTimestamp = 0;
    m_lastSyncChunksDataProcessingProgressPercentage = 0.0;
    m_linkedNotebookSyncChunksDownloadedTimestamp = 0;
    m_lastLinkedNotebookSyncChunksDataProcessingProgressPercentage = 0.0;
}

void MainWindow::setSynchronizationOptions(const Account & account)
{
    QNDEBUG("quentier:main_window", "MainWindow::setSynchronizationOptions");

    if (Q_UNLIKELY(!m_pSynchronizationManager)) {
        QNWARNING(
            "quentier:main_window",
            "Can't set synchronization options: "
                << "no synchronization manager");
        return;
    }

    ApplicationSettings appSettings(
        account, preferences::keys::files::synchronization);

    appSettings.beginGroup(preferences::keys::synchronizationGroup);

    bool downloadNoteThumbnailsOption =
        (appSettings.contains(preferences::keys::downloadNoteThumbnails)
             ? appSettings.value(preferences::keys::downloadNoteThumbnails)
                   .toBool()
             : preferences::defaults::downloadNoteThumbnails);

    bool downloadInkNoteImagesOption =
        (appSettings.contains(preferences::keys::downloadInkNoteImages)
             ? appSettings.value(preferences::keys::downloadInkNoteImages)
                   .toBool()
             : preferences::defaults::downloadInkNoteImages);

    appSettings.endGroup();

    m_pSynchronizationManager->setDownloadNoteThumbnails(
        downloadNoteThumbnailsOption);

    m_pSynchronizationManager->setDownloadInkNoteImages(
        downloadInkNoteImagesOption);

    QString inkNoteImagesStoragePath = accountPersistentStoragePath(account);
    inkNoteImagesStoragePath += QStringLiteral("/NoteEditorPage/inkNoteImages");

    QNTRACE(
        "quentier:main_window",
        "Ink note images storage path: " << inkNoteImagesStoragePath
                                         << "; account: " << account.name());

    m_pSynchronizationManager->setInkNoteImagesStoragePath(
        inkNoteImagesStoragePath);
}

void MainWindow::setupSynchronizationManagerThread()
{
    QNDEBUG(
        "quentier:main_window",
        "MainWindow::setupSynchronizationManagerThread");

    m_pSynchronizationManagerThread = new QThread;

    m_pSynchronizationManagerThread->setObjectName(
        QStringLiteral("SynchronizationManagerThread"));

    QObject::connect(
        m_pSynchronizationManagerThread, &QThread::finished,
        m_pSynchronizationManagerThread, &QThread::deleteLater);

    m_pSynchronizationManagerThread->start();
    m_pSynchronizationManager->moveToThread(m_pSynchronizationManagerThread);

    // NOTE: m_pAuthenticationManager should NOT be moved to synchronization
    // manager's thread but should reside in the GUI thread because for OAuth
    // it needs to show a widget to the user. In most cases widget shown from
    // outside the GUI thread actually works but some Qt styles on Linux distros
    // may be confused by this behaviour and it might even lead to a crash.
}

void MainWindow::setupRunSyncPeriodicallyTimer()
{
    QNDEBUG(
        "quentier:main_window", "MainWindow::setupRunSyncPeriodicallyTimer");

    if (Q_UNLIKELY(!m_pAccount)) {
        QNDEBUG("quentier:main_window", "No current account");
        return;
    }

    if (Q_UNLIKELY(m_pAccount->type() != Account::Type::Evernote)) {
        QNDEBUG("quentier:main_window", "Non-Evernote account is used");
        return;
    }

    ApplicationSettings syncSettings(
        *m_pAccount, preferences::keys::files::synchronization);

    syncSettings.beginGroup(preferences::keys::synchronizationGroup);

    int runSyncEachNumMinutes = -1;
    if (syncSettings.contains(preferences::keys::runSyncPeriodMinutes)) {
        auto data = syncSettings.value(preferences::keys::runSyncPeriodMinutes);

        bool conversionResult = false;
        runSyncEachNumMinutes = data.toInt(&conversionResult);
        if (Q_UNLIKELY(!conversionResult)) {
            QNDEBUG(
                "quentier:main_window",
                "Failed to convert the number of "
                    << "minutes to run sync over to int: " << data);
            runSyncEachNumMinutes = -1;
        }
    }

    if (runSyncEachNumMinutes < 0) {
        runSyncEachNumMinutes = preferences::defaults::runSyncPeriodMinutes;
    }

    syncSettings.endGroup();

    onRunSyncEachNumMinitesPreferenceChanged(runSyncEachNumMinutes);
}

void MainWindow::launchSynchronization()
{
    QNDEBUG("quentier:main_window", "MainWindow::launchSynchronization");

    if (Q_UNLIKELY(!m_pSynchronizationManager)) {
        ErrorString error(
            QT_TR_NOOP("Can't start synchronization: internal "
                       "error, no synchronization manager is set up"));
        QNWARNING("quentier:main_window", error);
        onSetStatusBarText(error.localizedString(), secondsToMilliseconds(30));
        return;
    }

    if (m_syncInProgress) {
        QNDEBUG(
            "quentier:main_window", "Synchronization is already in progress");
        return;
    }

    if (m_pendingNewEvernoteAccountAuthentication) {
        QNDEBUG(
            "quentier:main_window",
            "Pending new Evernote account "
                << "authentication");
        return;
    }

    if (m_pendingCurrentEvernoteAccountAuthentication) {
        QNDEBUG(
            "quentier:main_window",
            "Pending current Evernote account "
                << "authentication");
        return;
    }

    if (m_pendingSwitchToNewEvernoteAccount) {
        QNDEBUG(
            "quentier:main_window",
            "Pending switch to new Evernote "
                << "account");
        return;
    }

    if (m_authenticatedCurrentEvernoteAccount) {
        QNDEBUG("quentier:main_window", "Emitting synchronize signal");
        Q_EMIT synchronize();
        return;
    }

    m_pendingCurrentEvernoteAccountAuthentication = true;

    QNDEBUG(
        "quentier:main_window",
        "Emitting authenticate current account "
            << "signal");

    Q_EMIT authenticateCurrentAccount();
}

bool MainWindow::shouldRunSyncOnStartup() const
{
    if (Q_UNLIKELY(!m_pAccount)) {
        QNWARNING(
            "quentier:main_window",
            "MainWindow::shouldRunSyncOnStartup: no account");
        return false;
    }

    if (m_pAccount->type() != Account::Type::Evernote) {
        return false;
    }

    ApplicationSettings syncSettings(
        *m_pAccount, preferences::keys::files::synchronization);

    syncSettings.beginGroup(preferences::keys::synchronizationGroup);

    const ApplicationSettings::GroupCloser groupCloser{syncSettings};

    const auto runSyncOnStartupValue =
        syncSettings.value(preferences::keys::runSyncOnStartup);

    if (runSyncOnStartupValue.isValid() &&
        runSyncOnStartupValue.canConvert<bool>()) {
        return runSyncOnStartupValue.toBool();
    }

    return preferences::defaults::runSyncOnStartup;
}

void MainWindow::setupDefaultShortcuts()
{
    QNDEBUG("quentier:main_window", "MainWindow::setupDefaultShortcuts");

    using quentier::ShortcutManager;

#define PROCESS_ACTION_SHORTCUT(action, key, context)                          \
    {                                                                          \
        QString contextStr = QString::fromUtf8(context);                       \
        ActionKeyWithContext actionData;                                       \
        actionData.m_key = key;                                                \
        actionData.m_context = contextStr;                                     \
        QVariant data;                                                         \
        data.setValue(actionData);                                             \
        m_pUi->Action##action->setData(data);                                  \
        QKeySequence shortcut = m_pUi->Action##action->shortcut();             \
        if (shortcut.isEmpty()) {                                              \
            QNTRACE(                                                           \
                "quentier:main_window",                                        \
                "No shortcut was found for action "                            \
                    << m_pUi->Action##action->objectName());                   \
        }                                                                      \
        else {                                                                 \
            m_shortcutManager.setDefaultShortcut(                              \
                key, shortcut, *m_pAccount, contextStr);                       \
        }                                                                      \
    }

#define PROCESS_NON_STANDARD_ACTION_SHORTCUT(action, context)                  \
    {                                                                          \
        QString actionStr = QString::fromUtf8(#action);                        \
        QString contextStr = QString::fromUtf8(context);                       \
        ActionNonStandardKeyWithContext actionData;                            \
        actionData.m_nonStandardActionKey = actionStr;                         \
        actionData.m_context = contextStr;                                     \
        QVariant data;                                                         \
        data.setValue(actionData);                                             \
        m_pUi->Action##action->setData(data);                                  \
        QKeySequence shortcut = m_pUi->Action##action->shortcut();             \
        if (shortcut.isEmpty()) {                                              \
            QNTRACE(                                                           \
                "quentier:main_window",                                        \
                "No shortcut was found for action "                            \
                    << m_pUi->Action##action->objectName());                   \
        }                                                                      \
        else {                                                                 \
            m_shortcutManager.setNonStandardDefaultShortcut(                   \
                actionStr, shortcut, *m_pAccount, contextStr);                 \
        }                                                                      \
    }

#include "ActionShortcuts.inl"

#undef PROCESS_NON_STANDARD_ACTION_SHORTCUT
#undef PROCESS_ACTION_SHORTCUT
}

void MainWindow::setupUserShortcuts()
{
    QNDEBUG("quentier:main_window", "MainWindow::setupUserShortcuts");

#define PROCESS_ACTION_SHORTCUT(action, key, ...)                              \
    {                                                                          \
        QKeySequence shortcut = m_shortcutManager.shortcut(                    \
            key, *m_pAccount, QStringLiteral("" __VA_ARGS__));                 \
        if (shortcut.isEmpty()) {                                              \
            QNTRACE(                                                           \
                "quentier:main_window",                                        \
                "No shortcut was found for action "                            \
                    << m_pUi->Action##action->objectName());                   \
            auto it = m_shortcutKeyToAction.find(key);                         \
            if (it != m_shortcutKeyToAction.end()) {                           \
                auto * pAction = it.value();                                   \
                pAction->setShortcut(QKeySequence());                          \
                Q_UNUSED(m_shortcutKeyToAction.erase(it))                      \
            }                                                                  \
        }                                                                      \
        else {                                                                 \
            m_pUi->Action##action->setShortcut(shortcut);                      \
            m_pUi->Action##action->setShortcutContext(                         \
                Qt::WidgetWithChildrenShortcut);                               \
            m_shortcutKeyToAction[key] = m_pUi->Action##action;                \
        }                                                                      \
    }

#define PROCESS_NON_STANDARD_ACTION_SHORTCUT(action, ...)                      \
    {                                                                          \
        QKeySequence shortcut = m_shortcutManager.shortcut(                    \
            QStringLiteral(#action), *m_pAccount,                              \
            QStringLiteral("" __VA_ARGS__));                                   \
        if (shortcut.isEmpty()) {                                              \
            QNTRACE(                                                           \
                "quentier:main_window",                                        \
                "No shortcut was found for action "                            \
                    << m_pUi->Action##action->objectName());                   \
            auto it = m_nonStandardShortcutKeyToAction.find(                   \
                QStringLiteral(#action));                                      \
            if (it != m_nonStandardShortcutKeyToAction.end()) {                \
                auto * pAction = it.value();                                   \
                pAction->setShortcut(QKeySequence());                          \
                Q_UNUSED(m_nonStandardShortcutKeyToAction.erase(it))           \
            }                                                                  \
        }                                                                      \
        else {                                                                 \
            m_pUi->Action##action->setShortcut(shortcut);                      \
            m_pUi->Action##action->setShortcutContext(                         \
                Qt::WidgetWithChildrenShortcut);                               \
            m_nonStandardShortcutKeyToAction[QStringLiteral(#action)] =        \
                m_pUi->Action##action;                                         \
        }                                                                      \
    }

#include "ActionShortcuts.inl"

#undef PROCESS_NON_STANDARD_ACTION_SHORTCUT
#undef PROCESS_ACTION_SHORTCUT
}

void MainWindow::startListeningForShortcutChanges()
{
    QNDEBUG(
        "quentier:main_window", "MainWindow::startListeningForShortcutChanges");

    QObject::connect(
        &m_shortcutManager, &ShortcutManager::shortcutChanged, this,
        &MainWindow::onShortcutChanged);

    QObject::connect(
        &m_shortcutManager, &ShortcutManager::nonStandardShortcutChanged, this,
        &MainWindow::onNonStandardShortcutChanged);
}

void MainWindow::stopListeningForShortcutChanges()
{
    QNDEBUG(
        "quentier:main_window", "MainWindow::stopListeningForShortcutChanges");

    QObject::disconnect(
        &m_shortcutManager, &ShortcutManager::shortcutChanged, this,
        &MainWindow::onShortcutChanged);

    QObject::disconnect(
        &m_shortcutManager, &ShortcutManager::nonStandardShortcutChanged, this,
        &MainWindow::onNonStandardShortcutChanged);
}

void MainWindow::setupConsumerKeyAndSecret(
    QString & consumerKey, QString & consumerSecret)
{
    const char key[10] = "e3zA914Ol";

    QByteArray consumerKeyObf = QByteArray::fromBase64(
        QStringLiteral("AR8MO2M8Z14WFFcuLzM1Shob").toUtf8());

    char lastChar = 0;
    int size = consumerKeyObf.size();
    for (int i = 0; i < size; ++i) {
        char currentChar = consumerKeyObf[i];
        consumerKeyObf[i] = consumerKeyObf[i] ^ lastChar ^ key[i % 8];
        lastChar = currentChar;
    }

    consumerKey =
        QString::fromUtf8(consumerKeyObf.constData(), consumerKeyObf.size());

    QByteArray consumerSecretObf = QByteArray::fromBase64(
        QStringLiteral("BgFLOzJiZh9KSwRyLS8sAg==").toUtf8());

    lastChar = 0;
    size = consumerSecretObf.size();
    for (int i = 0; i < size; ++i) {
        char currentChar = consumerSecretObf[i];
        consumerSecretObf[i] = consumerSecretObf[i] ^ lastChar ^ key[i % 8];
        lastChar = currentChar;
    }

    consumerSecret = QString::fromUtf8(
        consumerSecretObf.constData(), consumerSecretObf.size());
}

void MainWindow::persistChosenNoteSortingMode(int index)
{
    QNDEBUG(
        "quentier:main_window",
        "MainWindow::persistChosenNoteSortingMode: "
            << "index = " << index);

    ApplicationSettings appSettings(
        *m_pAccount, preferences::keys::files::userInterface);

    appSettings.beginGroup(QStringLiteral("NoteListView"));
    appSettings.setValue(NOTE_SORTING_MODE_KEY, index);
    appSettings.endGroup();
}

NoteModel::NoteSortingMode::type MainWindow::restoreNoteSortingMode()
{
    QNDEBUG("quentier:main_window", "MainWindow::restoreNoteSortingMode");

    ApplicationSettings appSettings(
        *m_pAccount, preferences::keys::files::userInterface);

    appSettings.beginGroup(QStringLiteral("NoteListView"));
    if (!appSettings.contains(NOTE_SORTING_MODE_KEY)) {
        QNDEBUG(
            "quentier:main_window",
            "No persisted note sorting mode within "
                << "the settings, nothing to restore");
        appSettings.endGroup();
        return NoteModel::NoteSortingMode::None;
    }

    auto data = appSettings.value(NOTE_SORTING_MODE_KEY);
    appSettings.endGroup();

    if (data.isNull()) {
        QNDEBUG(
            "quentier:main_window",
            "No persisted note sorting mode, "
                << "nothing to restore");
        return NoteModel::NoteSortingMode::None;
    }

    bool conversionResult = false;
    int index = data.toInt(&conversionResult);
    if (Q_UNLIKELY(!conversionResult)) {
        ErrorString error(
            QT_TR_NOOP("Internal error: can't restore the last used note "
                       "sorting mode, can't convert the persisted setting to "
                       "the integer index"));

        QNWARNING(
            "quentier:main_window", error << ", persisted data: " << data);

        onSetStatusBarText(error.localizedString(), secondsToMilliseconds(30));
        return NoteModel::NoteSortingMode::None;
    }

    return static_cast<NoteModel::NoteSortingMode::type>(index);
}

void MainWindow::persistGeometryAndState()
{
    QNDEBUG("quentier:main_window", "MainWindow::persistGeometryAndState");

    ApplicationSettings appSettings(
        *m_pAccount, preferences::keys::files::userInterface);

    appSettings.beginGroup(QStringLiteral("MainWindow"));

    appSettings.setValue(MAIN_WINDOW_GEOMETRY_KEY, saveGeometry());
    appSettings.setValue(MAIN_WINDOW_STATE_KEY, saveState());

    bool showSidePanel = m_pUi->ActionShowSidePanel->isChecked();

    bool showFavoritesView = m_pUi->ActionShowFavorites->isChecked();
    bool showNotebooksView = m_pUi->ActionShowNotebooks->isChecked();
    bool showTagsView = m_pUi->ActionShowTags->isChecked();
    bool showSavedSearches = m_pUi->ActionShowSavedSearches->isChecked();
    bool showDeletedNotes = m_pUi->ActionShowDeletedNotes->isChecked();

    auto splitterSizes = m_pUi->splitter->sizes();
    int splitterSizesCount = splitterSizes.count();
    bool splitterSizesCountOk = (splitterSizesCount == 3);

    auto sidePanelSplitterSizes = m_pUi->sidePanelSplitter->sizes();
    int sidePanelSplitterSizesCount = sidePanelSplitterSizes.count();
    bool sidePanelSplitterSizesCountOk = (sidePanelSplitterSizesCount == 5);

    QNTRACE(
        "quentier:main_window",
        "Show side panel = "
            << (showSidePanel ? "true" : "false") << ", show favorites view = "
            << (showFavoritesView ? "true" : "false")
            << ", show notebooks view = "
            << (showNotebooksView ? "true" : "false")
            << ", show tags view = " << (showTagsView ? "true" : "false")
            << ", show saved searches view = "
            << (showSavedSearches ? "true" : "false")
            << ", show deleted notes view = "
            << (showDeletedNotes ? "true" : "false") << ", splitter sizes ok = "
            << (splitterSizesCountOk ? "true" : "false")
            << ", side panel splitter sizes ok = "
            << (sidePanelSplitterSizesCountOk ? "true" : "false"));

    if (QuentierIsLogLevelActive(LogLevel::Trace)) {
        QString str;
        QTextStream strm(&str);

        strm << "Splitter sizes: ";
        for (const auto size: qAsConst(splitterSizes)) {
            strm << size << " ";
        }

        strm << "\n";

        strm << "Side panel splitter sizes: ";
        for (const auto size: qAsConst(sidePanelSplitterSizes)) {
            strm << size << " ";
        }

        QNTRACE("quentier:main_window", str);
    }

    if (splitterSizesCountOk && showSidePanel &&
        (showFavoritesView || showNotebooksView || showTagsView ||
         showSavedSearches || showDeletedNotes))
    {
        appSettings.setValue(
            MAIN_WINDOW_SIDE_PANEL_WIDTH_KEY, splitterSizes[0]);
    }
    else {
        appSettings.setValue(MAIN_WINDOW_SIDE_PANEL_WIDTH_KEY, QVariant());
    }

    bool showNotesList = m_pUi->ActionShowNotesList->isChecked();
    if (splitterSizesCountOk && showNotesList) {
        appSettings.setValue(MAIN_WINDOW_NOTE_LIST_WIDTH_KEY, splitterSizes[1]);
    }
    else {
        appSettings.setValue(MAIN_WINDOW_NOTE_LIST_WIDTH_KEY, QVariant());
    }

    if (sidePanelSplitterSizesCountOk && showFavoritesView) {
        appSettings.setValue(
            MAIN_WINDOW_FAVORITES_VIEW_HEIGHT, sidePanelSplitterSizes[0]);
    }
    else {
        appSettings.setValue(MAIN_WINDOW_FAVORITES_VIEW_HEIGHT, QVariant());
    }

    if (sidePanelSplitterSizesCountOk && showNotebooksView) {
        appSettings.setValue(
            MAIN_WINDOW_NOTEBOOKS_VIEW_HEIGHT, sidePanelSplitterSizes[1]);
    }
    else {
        appSettings.setValue(MAIN_WINDOW_NOTEBOOKS_VIEW_HEIGHT, QVariant());
    }

    if (sidePanelSplitterSizesCountOk && showTagsView) {
        appSettings.setValue(
            MAIN_WINDOW_TAGS_VIEW_HEIGHT, sidePanelSplitterSizes[2]);
    }
    else {
        appSettings.setValue(MAIN_WINDOW_TAGS_VIEW_HEIGHT, QVariant());
    }

    if (sidePanelSplitterSizesCountOk && showSavedSearches) {
        appSettings.setValue(
            MAIN_WINDOW_SAVED_SEARCHES_VIEW_HEIGHT, sidePanelSplitterSizes[3]);
    }
    else {
        appSettings.setValue(
            MAIN_WINDOW_SAVED_SEARCHES_VIEW_HEIGHT, QVariant());
    }

    if (sidePanelSplitterSizesCountOk && showDeletedNotes) {
        appSettings.setValue(
            MAIN_WINDOW_DELETED_NOTES_VIEW_HEIGHT, sidePanelSplitterSizes[4]);
    }
    else {
        appSettings.setValue(MAIN_WINDOW_DELETED_NOTES_VIEW_HEIGHT, QVariant());
    }

    appSettings.endGroup();
}

void MainWindow::restoreGeometryAndState()
{
    QNDEBUG("quentier:main_window", "MainWindow::restoreGeometryAndState");

    ApplicationSettings appSettings(
        *m_pAccount, preferences::keys::files::userInterface);

    appSettings.beginGroup(QStringLiteral("MainWindow"));

    QByteArray savedGeometry =
        appSettings.value(MAIN_WINDOW_GEOMETRY_KEY).toByteArray();

    QByteArray savedState =
        appSettings.value(MAIN_WINDOW_STATE_KEY).toByteArray();

    appSettings.endGroup();

    m_geometryRestored = restoreGeometry(savedGeometry);
    m_stateRestored = restoreState(savedState);

    QNTRACE(
        "quentier:main_window",
        "Geometry restored = " << (m_geometryRestored ? "true" : "false")
                               << ", state restored = "
                               << (m_stateRestored ? "true" : "false"));

    scheduleSplitterSizesRestoration();
}

void MainWindow::restoreSplitterSizes()
{
    QNDEBUG("quentier:main_window", "MainWindow::restoreSplitterSizes");

    ApplicationSettings appSettings(
        *m_pAccount, preferences::keys::files::userInterface);

    appSettings.beginGroup(QStringLiteral("MainWindow"));

    QVariant sidePanelWidth =
        appSettings.value(MAIN_WINDOW_SIDE_PANEL_WIDTH_KEY);

    QVariant notesListWidth =
        appSettings.value(MAIN_WINDOW_NOTE_LIST_WIDTH_KEY);

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

    QNTRACE(
        "quentier:main_window",
        "Side panel width = "
            << sidePanelWidth << ", notes list width = " << notesListWidth
            << ", favorites view height = " << favoritesViewHeight
            << ", notebooks view height = " << notebooksViewHeight
            << ", tags view height = " << tagsViewHeight
            << ", saved searches view height = " << savedSearchesViewHeight
            << ", deleted notes view height = " << deletedNotesViewHeight);

    bool showSidePanel = m_pUi->ActionShowSidePanel->isChecked();
    bool showNotesList = m_pUi->ActionShowNotesList->isChecked();

    bool showFavoritesView = m_pUi->ActionShowFavorites->isChecked();
    bool showNotebooksView = m_pUi->ActionShowNotebooks->isChecked();
    bool showTagsView = m_pUi->ActionShowTags->isChecked();
    bool showSavedSearches = m_pUi->ActionShowSavedSearches->isChecked();
    bool showDeletedNotes = m_pUi->ActionShowDeletedNotes->isChecked();

    auto splitterSizes = m_pUi->splitter->sizes();
    int splitterSizesCount = splitterSizes.count();
    if (splitterSizesCount == 3) {
        int totalWidth = 0;
        for (int i = 0; i < splitterSizesCount; ++i) {
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
                QNTRACE(
                    "quentier:main_window",
                    "Restored side panel width: " << sidePanelWidthInt);
            }
            else {
                QNDEBUG(
                    "quentier:main_window",
                    "Can't restore the side panel "
                        << "width: can't "
                        << "convert the persisted width to int: "
                        << sidePanelWidth);
            }
        }

        if (notesListWidth.isValid() && showNotesList) {
            bool conversionResult = false;
            int notesListWidthInt = notesListWidth.toInt(&conversionResult);
            if (conversionResult) {
                splitterSizes[1] = notesListWidthInt;
                QNTRACE(
                    "quentier:main_window",
                    "Restored notes list panel "
                        << "width: " << notesListWidthInt);
            }
            else {
                QNDEBUG(
                    "quentier:main_window",
                    "Can't restore the notes list "
                        << "panel width: can't convert the persisted width to "
                           "int: "
                        << notesListWidth);
            }
        }

        splitterSizes[2] = totalWidth - splitterSizes[0] - splitterSizes[1];

        if (QuentierIsLogLevelActive(LogLevel::Trace)) {
            QString str;
            QTextStream strm(&str);

            strm << "Splitter sizes before restoring (total " << totalWidth
                 << "): ";

            auto splitterSizesBefore = m_pUi->splitter->sizes();
            for (const auto size: qAsConst(splitterSizesBefore)) {
                strm << size << " ";
            }

            QNTRACE("quentier:main_window", str);
        }

        m_pUi->splitter->setSizes(splitterSizes);

        auto * pSidePanel = m_pUi->splitter->widget(0);
        auto sidePanelSizePolicy = pSidePanel->sizePolicy();
        sidePanelSizePolicy.setHorizontalPolicy(QSizePolicy::Minimum);
        sidePanelSizePolicy.setHorizontalStretch(0);
        pSidePanel->setSizePolicy(sidePanelSizePolicy);

        auto * pNoteListView = m_pUi->splitter->widget(1);
        auto noteListViewSizePolicy = pNoteListView->sizePolicy();
        noteListViewSizePolicy.setHorizontalPolicy(QSizePolicy::Minimum);
        noteListViewSizePolicy.setHorizontalStretch(0);
        pNoteListView->setSizePolicy(noteListViewSizePolicy);

        auto * pNoteEditor = m_pUi->splitter->widget(2);
        auto noteEditorSizePolicy = pNoteEditor->sizePolicy();
        noteEditorSizePolicy.setHorizontalPolicy(QSizePolicy::Expanding);
        noteEditorSizePolicy.setHorizontalStretch(1);
        pNoteEditor->setSizePolicy(noteEditorSizePolicy);

        QNTRACE("quentier:main_window", "Set splitter sizes");

        if (QuentierIsLogLevelActive(LogLevel::Trace)) {
            QString str;
            QTextStream strm(&str);

            strm << "Splitter sizes after restoring: ";
            auto splitterSizesAfter = m_pUi->splitter->sizes();
            for (const auto size: qAsConst(splitterSizesAfter)) {
                strm << size << " ";
            }

            QNTRACE("quentier:main_window", str);
        }
    }
    else {
        ErrorString error(
            QT_TR_NOOP("Internal error: can't restore the widths for side "
                       "panel, note list view and note editors view: wrong "
                       "number of sizes within the splitter"));

        QNWARNING(
            "quentier:main_window",
            error << ", sizes count: " << splitterSizesCount);

        onSetStatusBarText(error.localizedString(), secondsToMilliseconds(30));
    }

    auto sidePanelSplitterSizes = m_pUi->sidePanelSplitter->sizes();
    int sidePanelSplitterSizesCount = sidePanelSplitterSizes.count();
    if (sidePanelSplitterSizesCount == 5) {
        int totalHeight = 0;
        for (int i = 0; i < 5; ++i) {
            totalHeight += sidePanelSplitterSizes[i];
        }

        if (QuentierIsLogLevelActive(LogLevel::Trace)) {
            QString str;
            QTextStream strm(&str);

            strm << "Side panel splitter sizes before restoring (total "
                 << totalHeight << "): ";

            for (const auto size: qAsConst(sidePanelSplitterSizes)) {
                strm << size << " ";
            }

            QNTRACE("quentier:main_window", str);
        }

        if (showFavoritesView && favoritesViewHeight.isValid()) {
            bool conversionResult = false;

            int favoritesViewHeightInt =
                favoritesViewHeight.toInt(&conversionResult);

            if (conversionResult) {
                sidePanelSplitterSizes[0] = favoritesViewHeightInt;
                QNTRACE(
                    "quentier:main_window",
                    "Restored favorites view "
                        << "height: " << favoritesViewHeightInt);
            }
            else {
                QNDEBUG(
                    "quentier:main_window",
                    "Can't restore the favorites "
                        << "view height: can't convert the persisted height to "
                        << "int: " << favoritesViewHeight);
            }
        }

        if (showNotebooksView && notebooksViewHeight.isValid()) {
            bool conversionResult = false;

            int notebooksViewHeightInt =
                notebooksViewHeight.toInt(&conversionResult);

            if (conversionResult) {
                sidePanelSplitterSizes[1] = notebooksViewHeightInt;
                QNTRACE(
                    "quentier:main_window",
                    "Restored notebooks view "
                        << "height: " << notebooksViewHeightInt);
            }
            else {
                QNDEBUG(
                    "quentier:main_window",
                    "Can't restore the notebooks "
                        << "view height: can't convert the persisted height to "
                        << "int: " << notebooksViewHeight);
            }
        }

        if (showTagsView && tagsViewHeight.isValid()) {
            bool conversionResult = false;
            int tagsViewHeightInt = tagsViewHeight.toInt(&conversionResult);
            if (conversionResult) {
                sidePanelSplitterSizes[2] = tagsViewHeightInt;
                QNTRACE(
                    "quentier:main_window",
                    "Restored tags view height: " << tagsViewHeightInt);
            }
            else {
                QNDEBUG(
                    "quentier:main_window",
                    "Can't restore the tags view "
                        << "height: can't convert the persisted height to int: "
                        << tagsViewHeight);
            }
        }

        if (showSavedSearches && savedSearchesViewHeight.isValid()) {
            bool conversionResult = false;

            int savedSearchesViewHeightInt =
                savedSearchesViewHeight.toInt(&conversionResult);

            if (conversionResult) {
                sidePanelSplitterSizes[3] = savedSearchesViewHeightInt;
                QNTRACE(
                    "quentier:main_window",
                    "Restored saved searches view "
                        << "height: " << savedSearchesViewHeightInt);
            }
            else {
                QNDEBUG(
                    "quentier:main_window",
                    "Can't restore the saved "
                        << "searches view height: can't convert the persisted "
                        << "height to int: " << savedSearchesViewHeight);
            }
        }

        if (showDeletedNotes && deletedNotesViewHeight.isValid()) {
            bool conversionResult = false;

            int deletedNotesViewHeightInt =
                deletedNotesViewHeight.toInt(&conversionResult);

            if (conversionResult) {
                sidePanelSplitterSizes[4] = deletedNotesViewHeightInt;
                QNTRACE(
                    "quentier:main_window",
                    "Restored deleted notes view "
                        << "height: " << deletedNotesViewHeightInt);
            }
            else {
                QNDEBUG(
                    "quentier:main_window",
                    "Can't restore the deleted "
                        << "notes view height: can't convert the persisted "
                        << "height to int: " << deletedNotesViewHeight);
            }
        }

        int totalHeightAfterRestore = 0;
        for (int i = 0; i < 5; ++i) {
            totalHeightAfterRestore += sidePanelSplitterSizes[i];
        }

        if (QuentierIsLogLevelActive(LogLevel::Trace)) {
            QString str;
            QTextStream strm(&str);

            strm << "Side panel splitter sizes after restoring (total "
                 << totalHeightAfterRestore << "): ";

            for (const auto size: qAsConst(sidePanelSplitterSizes)) {
                strm << size << " ";
            }

            QNTRACE("quentier:main_window", str);
        }

        m_pUi->sidePanelSplitter->setSizes(sidePanelSplitterSizes);
        QNTRACE("quentier:main_window", "Set side panel splitter sizes");
    }
    else {
        ErrorString error(
            QT_TR_NOOP("Internal error: can't restore the heights "
                       "of side panel's views: wrong number of "
                       "sizes within the splitter"));

        QNWARNING(
            "quentier:main_window",
            error << ", sizes count: " << splitterSizesCount);

        onSetStatusBarText(error.localizedString(), secondsToMilliseconds(30));
    }
}

void MainWindow::scheduleSplitterSizesRestoration()
{
    QNDEBUG(
        "quentier:main_window", "MainWindow::scheduleSplitterSizesRestoration");

    if (!m_shown) {
        QNDEBUG("quentier:main_window", "Not shown yet, won't do anything");
        return;
    }

    if (m_splitterSizesRestorationDelayTimerId != 0) {
        QNDEBUG(
            "quentier:main_window",
            "Splitter sizes restoration already "
                << "scheduled, timer id = "
                << m_splitterSizesRestorationDelayTimerId);
        return;
    }

    m_splitterSizesRestorationDelayTimerId =
        startTimer(RESTORE_SPLITTER_SIZES_DELAY);

    if (Q_UNLIKELY(m_splitterSizesRestorationDelayTimerId == 0)) {
        QNWARNING(
            "quentier:main_window",
            "Failed to start the timer to delay "
                << "the restoration of splitter sizes");
        return;
    }

    QNDEBUG(
        "quentier:main_window",
        "Started the timer to delay "
            << "the restoration of splitter sizes: "
            << m_splitterSizesRestorationDelayTimerId);
}

void MainWindow::scheduleGeometryAndStatePersisting()
{
    QNDEBUG(
        "quentier:main_window",
        "MainWindow::scheduleGeometryAndStatePersisting");

    if (!m_shown) {
        QNDEBUG("quentier:main_window", "Not shown yet, won't do anything");
        return;
    }

    if (m_geometryAndStatePersistingDelayTimerId != 0) {
        QNDEBUG(
            "quentier:main_window",
            "Persisting already scheduled, "
                << "timer id = " << m_geometryAndStatePersistingDelayTimerId);
        return;
    }

    m_geometryAndStatePersistingDelayTimerId =
        startTimer(PERSIST_GEOMETRY_AND_STATE_DELAY);

    if (Q_UNLIKELY(m_geometryAndStatePersistingDelayTimerId == 0)) {
        QNWARNING(
            "quentier:main_window",
            "Failed to start the timer to delay "
                << "the persistence of MainWindow's state and geometry");
        return;
    }

    QNDEBUG(
        "quentier:main_window",
        "Started the timer to delay "
            << "the persistence of MainWindow's state and geometry: timer id = "
            << m_geometryAndStatePersistingDelayTimerId);
}

template <class T>
void MainWindow::refreshThemeIcons()
{
    auto objects = findChildren<T *>();
    QNDEBUG(
        "quentier:main_window", "Found " << objects.size() << " child objects");

    for (auto * pObject: qAsConst(objects)) {
        if (Q_UNLIKELY(!pObject)) {
            continue;
        }

        auto icon = pObject->icon();
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
            newIcon.addFile(
                QStringLiteral("."), QSize(), QIcon::Normal, QIcon::Off);
        }

        pObject->setIcon(newIcon);
    }
}
