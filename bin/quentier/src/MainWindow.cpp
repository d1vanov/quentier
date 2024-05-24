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
#include <lib/exception/Utils.h>
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
#include <lib/synchronization/SyncEventsTracker.h>
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
#include <lib/widget/FilterByNotebookWidget.h>
#include <lib/widget/FilterBySavedSearchWidget.h>
#include <lib/widget/FilterBySearchStringWidget.h>
#include <lib/widget/FilterByTagWidget.h>
#include <lib/widget/FindAndReplaceWidget.h>
#include <lib/widget/LogViewerWidget.h>
#include <lib/widget/NoteCountLabelController.h>
#include <lib/widget/NoteFiltersManager.h>
#include <lib/widget/PanelWidget.h>
#include <lib/widget/TabWidget.h>
#include <lib/widget/color-picker-tool-button/ColorPickerToolButton.h>
#include <lib/widget/insert-table-tool-button/InsertTableToolButton.h>
#include <lib/widget/insert-table-tool-button/TableSettingsDialog.h>

#include <lib/widget/AboutQuentierWidget.h>
#include <lib/widget/NotebookModelItemInfoWidget.h>
#include <lib/widget/SavedSearchModelItemInfoWidget.h>
#include <lib/widget/TagModelItemInfoWidget.h>

#include <quentier/note_editor/NoteEditor.h>

using quentier::DeletedNoteItemView;
using quentier::FavoriteItemView;
using quentier::FilterByNotebookWidget;
using quentier::FilterBySavedSearchWidget;
using quentier::FilterBySearchStringWidget;
using quentier::FilterByTagWidget;
using quentier::LogViewerWidget;
using quentier::NotebookItemView;
using quentier::NoteListView;
using quentier::PanelWidget;
using quentier::SavedSearchItemView;
using quentier::TabWidget;
using quentier::TagItemView;

#include "ui_MainWindow.h"

#include <quentier/enml/Factory.h>
#include <quentier/local_storage/Factory.h>
#include <quentier/local_storage/ILocalStorage.h>
#include <quentier/local_storage/NoteSearchQuery.h>
#include <quentier/logging/QuentierLogger.h>
#include <quentier/synchronization/Factory.h>
#include <quentier/synchronization/ISyncEventsNotifier.h>
#include <quentier/synchronization/ISynchronizer.h>
#include <quentier/synchronization/types/ISyncChunksDataCounters.h>
#include <quentier/synchronization/types/ISyncOptionsBuilder.h>
#include <quentier/synchronization/types/ISyncResult.h>
#include <quentier/threading/Future.h>
#include <quentier/utility/ApplicationSettings.h>
#include <quentier/utility/DateTime.h>
#include <quentier/utility/MessageBox.h>
#include <quentier/utility/StandardPaths.h>
#include <quentier/utility/cancelers/ManualCanceler.h>

#include <qevercloud/types/Note.h>
#include <qevercloud/types/Notebook.h>
#include <qevercloud/types/Resource.h>

#include <QActionGroup>
#include <QCheckBox>
#include <QClipboard>
#include <QColorDialog>
#include <QCryptographicHash>
#include <QDateTime>
#include <QDebug>
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
#include <QTextStream>
#include <QThreadPool>
#include <QTimer>
#include <QTimerEvent>
#include <QToolTip>
#include <QXmlStreamWriter>

#include <algorithm>
#include <cstddef>
#include <limits>
#include <string_view>
#include <utility>

#define NOTIFY_ERROR(error)                                                    \
    QNWARNING("quentier::MainWindow", QString::fromUtf8(error));               \
    onSetStatusBarText(                                                        \
        QCoreApplication::translate("quentier::MainWindow", error),            \
        secondsToMilliseconds(30))

#define NOTIFY_DEBUG(message)                                                  \
    QNDEBUG("quentier::MainWindow", QString::fromUtf8(message));               \
    onSetStatusBarText(                                                        \
        QCoreApplication::translate("quentier::MainWindow", message),          \
        secondsToMilliseconds(30))

namespace quentier {

using namespace std::string_view_literals;

namespace {

constexpr auto gFiltersViewStatusKey = "ViewExpandedStatus"sv;
constexpr auto gMainWindowDeletedNotesViewHeightKey =
    "DeletedNotesViewHeight"sv;

constexpr auto gMainWindowFavoritesViewHeightKey = "FavoritesViewHeight"sv;
constexpr auto gMainWindowGeometryKey = "Geometry"sv;
constexpr auto gMainWindowNoteListWidthKey = "NoteListWidth"sv;
constexpr auto gMainWindowNotebooksViewHeightKey = "NotebooksViewHeight"sv;
constexpr auto gMainWindowSavedSearchesViewHeightKey =
    "SavedSearchesViewHeight"sv;

constexpr auto gMainWindowSidePanelWidthKey = "SidePanelWidth"sv;
constexpr auto gMainWindowStateKey = "State"sv;
constexpr auto gMainWindowTagsViewHeightKey = "TagsViewHeight"sv;
constexpr auto gNoteSortingModeKey = "NoteSortingMode"sv;

#ifdef WITH_UPDATE_MANAGER
class UpdateManagerIdleInfoProvider final :
    public UpdateManager::IIdleStateInfoProvider
{
public:
    UpdateManagerIdleInfoProvider(NoteEditorTabsAndWindowsCoordinator & c) :
        m_coordinator{c}
    {}

    ~UpdateManagerIdleInfoProvider() override = default;

    [[nodiscard]] qint64 idleTime() noexcept override
    {
        return m_coordinator.minIdleTime();
    }

private:
    NoteEditorTabsAndWindowsCoordinator & m_coordinator;
};
#endif

} // namespace

MainWindow::MainWindow(QWidget * parentWidget) :
    QMainWindow{parentWidget}, m_ui{new Ui::MainWindow},
    m_availableAccountsActionGroup{new QActionGroup{this}},
    m_accountManager{new AccountManager{this}},
    m_animatedSyncButtonIcon{QStringLiteral(":/sync/sync.gif")},
    m_notebookModelColumnChangeRerouter{new ColumnChangeRerouter{
        static_cast<int>(NotebookModel::Column::NoteCount),
        static_cast<int>(NotebookModel::Column::Name), this}},
    m_tagModelColumnChangeRerouter{new ColumnChangeRerouter{
        static_cast<int>(TagModel::Column::NoteCount),
        static_cast<int>(TagModel::Column::Name), this}},
    m_noteModelColumnChangeRerouter{new ColumnChangeRerouter{
        static_cast<int>(NoteModel::Column::PreviewText),
        static_cast<int>(NoteModel::Column::Title), this}},
    m_favoritesModelColumnChangeRerouter{new ColumnChangeRerouter{
        static_cast<int>(FavoritesModel::Column::NoteCount),
        static_cast<int>(FavoritesModel::Column::DisplayName), this}},
    m_shortcutManager{this}
{
    QNTRACE("quentier::MainWindow", "MainWindow constructor");

    setupAccountManager();
    auto accountSource = AccountManager::AccountSource::LastUsed;
    m_account.emplace(m_accountManager->currentAccount(&accountSource));

    const bool createdDefaultAccount =
        (accountSource == AccountManager::AccountSource::NewDefault);
    if (createdDefaultAccount && !onceDisplayedGreeterScreen()) {
        m_pendingGreeterDialog = true;
        setOnceDisplayedGreeterScreen();
    }

    restoreNetworkProxySettingsForAccount(*m_account);

    m_systemTrayIconManager =
        new SystemTrayIconManager{*m_accountManager, this};

    QObject::connect(
        this, &MainWindow::shown, m_systemTrayIconManager,
        &SystemTrayIconManager::onMainWindowShown);

    QObject::connect(
        this, &MainWindow::hidden, m_systemTrayIconManager,
        &SystemTrayIconManager::onMainWindowHidden);

    m_pendingFirstShutdownDialog = m_pendingGreeterDialog &&
        m_systemTrayIconManager->isSystemTrayAvailable();

    setupThemeIcons();

    m_ui->setupUi(this);
    setupAccountSpecificUiElements();

    if (m_nativeIconThemeName.isEmpty()) {
        m_ui->ActionIconsNative->setVisible(false);
        m_ui->ActionIconsNative->setDisabled(true);
    }

    setupGenericPanelStyleControllers();
    setupSidePanelStyleControllers();
    restorePanelColors();

    m_availableAccountsActionGroup->setExclusive(true);

    setWindowTitleForAccount(*m_account);

    setupLocalStorage();
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

    if (m_account->type() == Account::Type::Evernote) {
        setupSynchronizer(SetAccountOption::Set);
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
    m_ui->ActionCheckForUpdates->setVisible(false);
#endif

    if (shouldRunSyncOnStartup()) {
        QTimer::singleShot(0, this, &MainWindow::onSyncButtonPressed);
    }
}

MainWindow::~MainWindow()
{
    QNDEBUG("quentier::MainWindow", "MainWindow dtor");
    delete m_ui;
}

void MainWindow::show()
{
    QNDEBUG("quentier::MainWindow", "MainWindow::show");

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

        auto dialog = std::make_unique<WelcomeToQuentierDialog>(this);
        dialog->setWindowModality(Qt::WindowModal);
        centerDialog(*dialog);

        if (dialog->exec() == QDialog::Accepted) {
            QNDEBUG(
                "quentier::MainWindow",
                "Log in to Evernote account option was chosen on the greeter "
                "screen");

            onEvernoteAccountAuthenticationRequested(
                dialog->evernoteServer(),
                QNetworkProxy{QNetworkProxy::NoProxy});
        }
    }
}

const SystemTrayIconManager & MainWindow::systemTrayIconManager() const noexcept
{
    return *m_systemTrayIconManager;
}

void MainWindow::connectActionsToSlots()
{
    QNDEBUG("quentier::MainWindow", "MainWindow::connectActionsToSlots");

    // File menu actions
    QObject::connect(
        m_ui->ActionNewNote, &QAction::triggered, this,
        &MainWindow::onNewNoteCreationRequested);

    QObject::connect(
        m_ui->ActionNewNotebook, &QAction::triggered, this,
        &MainWindow::onNewNotebookCreationRequested);

    QObject::connect(
        m_ui->ActionNewTag, &QAction::triggered, this,
        &MainWindow::onNewTagCreationRequested);

    QObject::connect(
        m_ui->ActionNewSavedSearch, &QAction::triggered, this,
        &MainWindow::onNewSavedSearchCreationRequested);

    QObject::connect(
        m_ui->ActionImportENEX, &QAction::triggered, this,
        &MainWindow::onImportEnexAction);

    QObject::connect(
        m_ui->ActionPrint, &QAction::triggered, this,
        &MainWindow::onCurrentNotePrintRequested);

    QObject::connect(
        m_ui->ActionQuit, &QAction::triggered, this, &MainWindow::onQuitAction);

    // Edit menu actions
    QObject::connect(
        m_ui->ActionFindInsideNote, &QAction::triggered, this,
        &MainWindow::onFindInsideNoteAction);

    QObject::connect(
        m_ui->ActionFindNext, &QAction::triggered, this,
        &MainWindow::onFindInsideNoteAction);

    QObject::connect(
        m_ui->ActionFindPrevious, &QAction::triggered, this,
        &MainWindow::onFindPreviousInsideNoteAction);

    QObject::connect(
        m_ui->ActionReplaceInNote, &QAction::triggered, this,
        &MainWindow::onReplaceInsideNoteAction);

    QObject::connect(
        m_ui->ActionPreferences, &QAction::triggered, this,
        &MainWindow::onShowPreferencesDialogAction);

    // Undo/redo actions
    QObject::connect(
        m_ui->ActionUndo, &QAction::triggered, this, &MainWindow::onUndoAction);

    QObject::connect(
        m_ui->ActionRedo, &QAction::triggered, this, &MainWindow::onRedoAction);

    // Copy/cut/paste actions
    QObject::connect(
        m_ui->ActionCopy, &QAction::triggered, this, &MainWindow::onCopyAction);

    QObject::connect(
        m_ui->ActionCut, &QAction::triggered, this, &MainWindow::onCutAction);

    QObject::connect(
        m_ui->ActionPaste, &QAction::triggered, this,
        &MainWindow::onPasteAction);

    // Select all action
    QObject::connect(
        m_ui->ActionSelectAll, &QAction::triggered, this,
        &MainWindow::onNoteTextSelectAllToggled);

    // Font actions
    QObject::connect(
        m_ui->ActionFontBold, &QAction::triggered, this,
        &MainWindow::onNoteTextBoldToggled);

    QObject::connect(
        m_ui->ActionFontItalic, &QAction::triggered, this,
        &MainWindow::onNoteTextItalicToggled);

    QObject::connect(
        m_ui->ActionFontUnderlined, &QAction::triggered, this,
        &MainWindow::onNoteTextUnderlineToggled);

    QObject::connect(
        m_ui->ActionFontStrikethrough, &QAction::triggered, this,
        &MainWindow::onNoteTextStrikethroughToggled);

    QObject::connect(
        m_ui->ActionIncreaseFontSize, &QAction::triggered, this,
        &MainWindow::onNoteTextIncreaseFontSizeAction);

    QObject::connect(
        m_ui->ActionDecreaseFontSize, &QAction::triggered, this,
        &MainWindow::onNoteTextDecreaseFontSizeAction);

    QObject::connect(
        m_ui->ActionFontHighlight, &QAction::triggered, this,
        &MainWindow::onNoteTextHighlightAction);

    // Spell checking
    QObject::connect(
        m_ui->ActionSpellCheck, &QAction::triggered, this,
        &MainWindow::onNoteTextSpellCheckToggled);

    // Text format actions
    QObject::connect(
        m_ui->ActionAlignLeft, &QAction::triggered, this,
        &MainWindow::onNoteTextAlignLeftAction);

    QObject::connect(
        m_ui->ActionAlignCenter, &QAction::triggered, this,
        &MainWindow::onNoteTextAlignCenterAction);

    QObject::connect(
        m_ui->ActionAlignRight, &QAction::triggered, this,
        &MainWindow::onNoteTextAlignRightAction);

    QObject::connect(
        m_ui->ActionAlignFull, &QAction::triggered, this,
        &MainWindow::onNoteTextAlignFullAction);

    QObject::connect(
        m_ui->ActionInsertHorizontalLine, &QAction::triggered, this,
        &MainWindow::onNoteTextAddHorizontalLineAction);

    QObject::connect(
        m_ui->ActionIncreaseIndentation, &QAction::triggered, this,
        &MainWindow::onNoteTextIncreaseIndentationAction);

    QObject::connect(
        m_ui->ActionDecreaseIndentation, &QAction::triggered, this,
        &MainWindow::onNoteTextDecreaseIndentationAction);

    QObject::connect(
        m_ui->ActionInsertBulletedList, &QAction::triggered, this,
        &MainWindow::onNoteTextInsertUnorderedListAction);

    QObject::connect(
        m_ui->ActionInsertNumberedList, &QAction::triggered, this,
        &MainWindow::onNoteTextInsertOrderedListAction);

    QObject::connect(
        m_ui->ActionInsertToDo, &QAction::triggered, this,
        &MainWindow::onNoteTextInsertToDoAction);

    QObject::connect(
        m_ui->ActionInsertTable, &QAction::triggered, this,
        &MainWindow::onNoteTextInsertTableDialogAction);

    QObject::connect(
        m_ui->ActionEditHyperlink, &QAction::triggered, this,
        &MainWindow::onNoteTextEditHyperlinkAction);

    QObject::connect(
        m_ui->ActionCopyHyperlink, &QAction::triggered, this,
        &MainWindow::onNoteTextCopyHyperlinkAction);

    QObject::connect(
        m_ui->ActionRemoveHyperlink, &QAction::triggered, this,
        &MainWindow::onNoteTextRemoveHyperlinkAction);

    QObject::connect(
        m_ui->ActionSaveNote, &QAction::triggered, this,
        &MainWindow::onSaveNoteAction);

    // Toggle view actions
    QObject::connect(
        m_ui->ActionShowSidePanel, &QAction::toggled, this,
        &MainWindow::onShowSidePanelActionToggled);

    QObject::connect(
        m_ui->ActionShowFavorites, &QAction::toggled, this,
        &MainWindow::onShowFavoritesActionToggled);

    QObject::connect(
        m_ui->ActionShowNotebooks, &QAction::toggled, this,
        &MainWindow::onShowNotebooksActionToggled);

    QObject::connect(
        m_ui->ActionShowTags, &QAction::toggled, this,
        &MainWindow::onShowTagsActionToggled);

    QObject::connect(
        m_ui->ActionShowSavedSearches, &QAction::toggled, this,
        &MainWindow::onShowSavedSearchesActionToggled);

    QObject::connect(
        m_ui->ActionShowDeletedNotes, &QAction::toggled, this,
        &MainWindow::onShowDeletedNotesActionToggled);

    QObject::connect(
        m_ui->ActionShowNotesList, &QAction::toggled, this,
        &MainWindow::onShowNoteListActionToggled);

    QObject::connect(
        m_ui->ActionShowToolbar, &QAction::toggled, this,
        &MainWindow::onShowToolbarActionToggled);

    QObject::connect(
        m_ui->ActionShowStatusBar, &QAction::toggled, this,
        &MainWindow::onShowStatusBarActionToggled);

    // Look and feel actions
    QObject::connect(
        m_ui->ActionIconsNative, &QAction::triggered, this,
        &MainWindow::onSwitchIconThemeToNativeAction);

    QObject::connect(
        m_ui->ActionIconsOxygen, &QAction::triggered, this,
        &MainWindow::onSwitchIconThemeToOxygenAction);

    QObject::connect(
        m_ui->ActionIconsTango, &QAction::triggered, this,
        &MainWindow::onSwitchIconThemeToTangoAction);

    QObject::connect(
        m_ui->ActionIconsBreeze, &QAction::triggered, this,
        &MainWindow::onSwitchIconThemeToBreezeAction);

    QObject::connect(
        m_ui->ActionIconsBreezeDark, &QAction::triggered, this,
        &MainWindow::onSwitchIconThemeToBreezeDarkAction);

    // Service menu actions
    QObject::connect(
        m_ui->ActionSynchronize, &QAction::triggered, this,
        &MainWindow::onSyncButtonPressed);

    // Help menu actions
    QObject::connect(
        m_ui->ActionShowNoteSource, &QAction::triggered, this,
        &MainWindow::onShowNoteSource);

    QObject::connect(
        m_ui->ActionViewLogs, &QAction::triggered, this,
        &MainWindow::onViewLogsActionTriggered);

    QObject::connect(
        m_ui->ActionAbout, &QAction::triggered, this,
        &MainWindow::onShowInfoAboutQuentierActionTriggered);

#ifdef WITH_UPDATE_MANAGER
    QObject::connect(
        m_ui->ActionCheckForUpdates, &QAction::triggered, this,
        &MainWindow::onCheckForUpdatesActionTriggered);
#endif
}

void MainWindow::connectViewButtonsToSlots()
{
    QNDEBUG("quentier::MainWindow", "MainWindow::connectViewButtonsToSlots");

    QObject::connect(
        m_ui->addNotebookButton, &QPushButton::clicked, this,
        &MainWindow::onNewNotebookCreationRequested);

    QObject::connect(
        m_ui->removeNotebookButton, &QPushButton::clicked, this,
        &MainWindow::onRemoveNotebookButtonPressed);

    QObject::connect(
        m_ui->notebookInfoButton, &QPushButton::clicked, this,
        &MainWindow::onNotebookInfoButtonPressed);

    QObject::connect(
        m_ui->addTagButton, &QPushButton::clicked, this,
        &MainWindow::onNewTagCreationRequested);

    QObject::connect(
        m_ui->removeTagButton, &QPushButton::clicked, this,
        &MainWindow::onRemoveTagButtonPressed);

    QObject::connect(
        m_ui->tagInfoButton, &QPushButton::clicked, this,
        &MainWindow::onTagInfoButtonPressed);

    QObject::connect(
        m_ui->addSavedSearchButton, &QPushButton::clicked, this,
        &MainWindow::onNewSavedSearchCreationRequested);

    QObject::connect(
        m_ui->removeSavedSearchButton, &QPushButton::clicked, this,
        &MainWindow::onRemoveSavedSearchButtonPressed);

    QObject::connect(
        m_ui->savedSearchInfoButton, &QPushButton::clicked, this,
        &MainWindow::onSavedSearchInfoButtonPressed);

    QObject::connect(
        m_ui->unfavoritePushButton, &QPushButton::clicked, this,
        &MainWindow::onUnfavoriteItemButtonPressed);

    QObject::connect(
        m_ui->favoriteInfoButton, &QPushButton::clicked, this,
        &MainWindow::onFavoritedItemInfoButtonPressed);

    QObject::connect(
        m_ui->restoreDeletedNoteButton, &QPushButton::clicked, this,
        &MainWindow::onRestoreDeletedNoteButtonPressed);

    QObject::connect(
        m_ui->eraseDeletedNoteButton, &QPushButton::clicked, this,
        &MainWindow::onDeleteNotePermanentlyButtonPressed);

    QObject::connect(
        m_ui->deletedNoteInfoButton, &QPushButton::clicked, this,
        &MainWindow::onDeletedNoteInfoButtonPressed);

    QObject::connect(
        m_ui->filtersViewTogglePushButton, &QPushButton::clicked, this,
        &MainWindow::onFiltersViewTogglePushButtonPressed);
}

void MainWindow::connectToolbarButtonsToSlots()
{
    QNDEBUG("quentier::MainWindow", "MainWindow::connectToolbarButtonsToSlots");

    QObject::connect(
        m_ui->addNotePushButton, &QPushButton::clicked, this,
        &MainWindow::onNewNoteCreationRequested);

    QObject::connect(
        m_ui->deleteNotePushButton, &QPushButton::clicked, this,
        &MainWindow::onDeleteCurrentNoteButtonPressed);

    QObject::connect(
        m_ui->infoButton, &QPushButton::clicked, this,
        &MainWindow::onCurrentNoteInfoRequested);

    QObject::connect(
        m_ui->printNotePushButton, &QPushButton::clicked, this,
        &MainWindow::onCurrentNotePrintRequested);

    QObject::connect(
        m_ui->exportNoteToPdfPushButton, &QPushButton::clicked, this,
        &MainWindow::onCurrentNotePdfExportRequested);

    QObject::connect(
        m_ui->syncPushButton, &QPushButton::clicked, this,
        &MainWindow::onSyncButtonPressed);
}

void MainWindow::connectSystemTrayIconManagerSignalsToSlots()
{
    QNDEBUG(
        "quentier::MainWindow",
        "MainWindow::connectSystemTrayIconManagerSignalsToSlots");

    QObject::connect(
        m_systemTrayIconManager, &SystemTrayIconManager::notifyError, this,
        &MainWindow::onSystemTrayIconManagerError);

    QObject::connect(
        m_systemTrayIconManager,
        &SystemTrayIconManager::newTextNoteAdditionRequested, this,
        &MainWindow::onNewNoteRequestedFromSystemTrayIcon);

    QObject::connect(
        m_systemTrayIconManager, &SystemTrayIconManager::quitRequested, this,
        &MainWindow::onQuitRequestedFromSystemTrayIcon);

    QObject::connect(
        m_systemTrayIconManager, &SystemTrayIconManager::accountSwitchRequested,
        this, &MainWindow::onAccountSwitchRequested);

    QObject::connect(
        m_systemTrayIconManager, &SystemTrayIconManager::showRequested, this,
        &MainWindow::onShowRequestedFromTrayIcon);

    QObject::connect(
        m_systemTrayIconManager, &SystemTrayIconManager::hideRequested, this,
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
        m_noteEditorTabsAndWindowsCoordinator,
        &NoteEditorTabsAndWindowsCoordinator::noteEditorFontColorChanged);

    QObject::connect(
        &dialog, &PreferencesDialog::noteEditorBackgroundColorChanged,
        m_noteEditorTabsAndWindowsCoordinator,
        &NoteEditorTabsAndWindowsCoordinator::noteEditorBackgroundColorChanged);

    QObject::connect(
        &dialog, &PreferencesDialog::noteEditorHighlightColorChanged,
        m_noteEditorTabsAndWindowsCoordinator,
        &NoteEditorTabsAndWindowsCoordinator::noteEditorHighlightColorChanged);

    QObject::connect(
        &dialog, &PreferencesDialog::noteEditorHighlightedTextColorChanged,
        m_noteEditorTabsAndWindowsCoordinator,
        &NoteEditorTabsAndWindowsCoordinator::
            noteEditorHighlightedTextColorChanged);

    QObject::connect(
        &dialog, &PreferencesDialog::noteEditorColorsReset,
        m_noteEditorTabsAndWindowsCoordinator,
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
        [this](bool enabled) { this->m_updateManager->setEnabled(enabled); });

    QObject::connect(
        &dialog, &PreferencesDialog::checkForUpdatesOnStartupOptionChanged,
        this, [this](bool enabled) {
            this->m_updateManager->setShouldCheckForUpdatesOnStartup(enabled);
        });

    QObject::connect(
        &dialog, &PreferencesDialog::useContinuousUpdateChannelOptionChanged,
        this, [this](bool enabled) {
            this->m_updateManager->setUseContinuousUpdateChannel(enabled);
        });

    QObject::connect(
        &dialog, &PreferencesDialog::checkForUpdatesIntervalChanged, this,
        [this](qint64 intervalMsec) {
            this->m_updateManager->setCheckForUpdatesIntervalMsec(intervalMsec);
        });

    QObject::connect(
        &dialog, &PreferencesDialog::updateChannelChanged, this,
        [this](QString channel) {
            this->m_updateManager->setUpdateChannel(std::move(channel));
        });

    QObject::connect(
        &dialog, &PreferencesDialog::updateProviderChanged, this,
        [this](UpdateProvider provider) {
            this->m_updateManager->setUpdateProvider(provider);
        });
#endif // WITH_UPDATE_MANAGER
}

void MainWindow::addMenuActionsToMainWindow()
{
    QNDEBUG("quentier::MainWindow", "MainWindow::addMenuActionsToMainWindow");

    // NOTE: adding the actions from the menu bar's menus is required for
    // getting the shortcuts of these actions to work properly; action shortcuts
    // only fire when the menu is shown which is not really the purpose behind
    // those shortcuts
    const auto menus = m_ui->menuBar->findChildren<QMenu *>();
    for (auto * menu: std::as_const(menus)) {
        auto actions = menu->actions();
        for (auto * action: std::as_const(actions)) {
            addAction(action);
        }
    }

    m_availableAccountsSubMenu = new QMenu{tr("Switch account")};

    auto * separatorAction = m_ui->menuFile->insertSeparator(m_ui->ActionQuit);

    auto * switchAccountSubMenuAction =
        m_ui->menuFile->insertMenu(separatorAction, m_availableAccountsSubMenu);

    m_ui->menuFile->insertSeparator(switchAccountSubMenuAction);
    updateSubMenuWithAvailableAccounts();
}

void MainWindow::updateSubMenuWithAvailableAccounts()
{
    QNDEBUG(
        "quentier::MainWindow",
        "MainWindow::updateSubMenuWithAvailableAccounts");

    if (Q_UNLIKELY(!m_availableAccountsSubMenu)) {
        QNDEBUG("quentier::MainWindow", "No available accounts sub-menu");
        return;
    }

    delete m_availableAccountsActionGroup;
    m_availableAccountsActionGroup = new QActionGroup{this};
    m_availableAccountsActionGroup->setExclusive(true);

    m_availableAccountsSubMenu->clear();

    const auto & availableAccounts = m_accountManager->availableAccounts();

    Q_ASSERT(availableAccounts.size() <= std::numeric_limits<int>::max());
    const int numAvailableAccounts = static_cast<int>(availableAccounts.size());
    for (int i = 0; i < numAvailableAccounts; ++i) {
        const Account & availableAccount = availableAccounts[i];

        QNTRACE(
            "quentier::MainWindow",
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

        auto * accountAction =
            new QAction{availableAccountRepresentationName, nullptr};

        m_availableAccountsSubMenu->addAction(accountAction);

        accountAction->setData(i);
        accountAction->setCheckable(true);

        if (m_account == availableAccount) {
            accountAction->setChecked(true);
        }

        addAction(accountAction);

        QObject::connect(
            accountAction, &QAction::toggled, this,
            &MainWindow::onSwitchAccountActionToggled);

        m_availableAccountsActionGroup->addAction(accountAction);
    }

    if (Q_LIKELY(numAvailableAccounts != 0)) {
        Q_UNUSED(m_availableAccountsSubMenu->addSeparator())
    }

    auto * addAccountAction =
        m_availableAccountsSubMenu->addAction(tr("Add account"));

    addAction(addAccountAction);

    QObject::connect(
        addAccountAction, &QAction::triggered, this,
        &MainWindow::onAddAccountActionTriggered);

    auto * manageAccountsAction =
        m_availableAccountsSubMenu->addAction(tr("Manage accounts"));

    addAction(manageAccountsAction);

    QObject::connect(
        manageAccountsAction, &QAction::triggered, this,
        &MainWindow::onManageAccountsActionTriggered);
}

void MainWindow::setupInitialChildWidgetsWidths()
{
    QNDEBUG(
        "quentier::MainWindow", "MainWindow::setupInitialChildWidgetsWidths");

    const int totalWidth = width();

    // 1/3 - for side view, 2/3 - for note list view, 3/3 - for the note editor
    const int partWidth = totalWidth / 5;

    QNTRACE(
        "quentier::MainWindow",
        "Total width = " << totalWidth << ", part width = " << partWidth);

    auto splitterSizes = m_ui->splitter->sizes();

    Q_ASSERT(splitterSizes.count() <= std::numeric_limits<int>::max());
    const int splitterSizesCount = static_cast<int>(splitterSizes.count());
    if (Q_UNLIKELY(splitterSizesCount != 3)) {
        ErrorString error{
            QT_TR_NOOP("Internal error: can't setup the proper initial widths "
                       "for side panel, note list view and note editors view: "
                       "wrong number of sizes within the splitter")};

        QNWARNING(
            "quentier::MainWindow",
            error << ", sizes count: " << splitterSizesCount);

        onSetStatusBarText(error.localizedString(), secondsToMilliseconds(30));
        return;
    }

    splitterSizes[0] = partWidth;
    splitterSizes[1] = partWidth;
    splitterSizes[2] = totalWidth - 2 * partWidth;

    m_ui->splitter->setSizes(splitterSizes);
}

void MainWindow::setWindowTitleForAccount(const Account & account)
{
    QNDEBUG(
        "quentier::MainWindow",
        "MainWindow::setWindowTitleForAccount: " << account.name());

    bool nonStandardPersistencePath = false;
    Q_UNUSED(applicationPersistentStoragePath(&nonStandardPersistencePath))

    const QString username = account.name();
    const QString displayName = account.displayName();

    QString title;
    QTextStream strm{&title};

    strm << qApp->applicationName() << ": ";
    if (!displayName.isEmpty()) {
        strm << displayName;
        strm << " (";
        strm << username;
        if (nonStandardPersistencePath) {
            strm << ", ";
            strm << QDir::toNativeSeparators(
                accountPersistentStoragePath(account));
        }
        strm << ")";
    }
    else {
        strm << username;

        if (nonStandardPersistencePath) {
            strm << " (";
            strm << QDir::toNativeSeparators(
                accountPersistentStoragePath(account));

            strm << ")";
        }
    }

    strm.flush();
    setWindowTitle(title);
}

NoteEditorWidget * MainWindow::currentNoteEditorTab()
{
    QNDEBUG("quentier::MainWindow", "MainWindow::currentNoteEditorTab");

    if (Q_UNLIKELY(m_ui->noteEditorsTabWidget->count() == 0)) {
        QNTRACE("quentier::MainWindow", "No open note editors");
        return nullptr;
    }

    const int currentIndex = m_ui->noteEditorsTabWidget->currentIndex();
    if (Q_UNLIKELY(currentIndex < 0)) {
        QNTRACE("quentier::MainWindow", "No current note editor");
        return nullptr;
    }

    auto * currentWidget = m_ui->noteEditorsTabWidget->widget(currentIndex);
    if (Q_UNLIKELY(!currentWidget)) {
        QNTRACE("quentier::MainWindow", "No current widget");
        return nullptr;
    }

    auto * noteEditorWidget = qobject_cast<NoteEditorWidget *>(currentWidget);
    if (Q_UNLIKELY(!noteEditorWidget)) {
        QNWARNING(
            "quentier::MainWindow",
            "Can't cast current note tag widget's widget to note editor "
            "widget");
        return nullptr;
    }

    return noteEditorWidget;
}

void MainWindow::createNewNote(
    NoteEditorTabsAndWindowsCoordinator::NoteEditorMode noteEditorMode)
{
    QNDEBUG(
        "quentier::MainWindow",
        "MainWindow::createNewNote: note editor mode = " << noteEditorMode);

    if (Q_UNLIKELY(!m_noteEditorTabsAndWindowsCoordinator)) {
        QNDEBUG(
            "quentier::MainWindow",
            "No note editor tabs and windows coordinator, probably the button "
            "was pressed too quickly on startup, skipping");
        return;
    }

    if (Q_UNLIKELY(!m_noteModel)) {
        internalErrorMessageBox(
            this,
            tr("Can't create a new note: note model is unexpectedly null"));
        return;
    }

    if (Q_UNLIKELY(!m_notebookModel)) {
        internalErrorMessageBox(
            this,
            tr("Can't create a new note: notebook model is unexpectedly null"));
        return;
    }

    auto currentNotebookIndex =
        m_ui->notebooksTreeView->currentlySelectedItemIndex();

    if (Q_UNLIKELY(!currentNotebookIndex.isValid())) {
        informationMessageBox(
            this, tr("No notebook is selected"),
            tr("Please select the notebook in which you want to create "
               "the note; if you don't have any notebooks yet, create one"));
        return;
    }

    const auto * notebookModelItem =
        m_notebookModel->itemForIndex(currentNotebookIndex);

    if (Q_UNLIKELY(!notebookModelItem)) {
        internalErrorMessageBox(
            this,
            tr("Can't create a new note: can't find the notebook model item "
               "corresponding to the currently selected notebook"));
        return;
    }

    if (Q_UNLIKELY(
            notebookModelItem->type() != INotebookModelItem::Type::Notebook))
    {
        Q_UNUSED(informationMessageBox(
            this, tr("No notebook is selected"),
            tr("Please select the notebook in which you want to create "
               "the note (currently the notebook stack seems to be selected)")))
        return;
    }

    const auto * notebookItem = notebookModelItem->cast<NotebookItem>();
    if (Q_UNLIKELY(!notebookItem)) {
        Q_UNUSED(internalErrorMessageBox(
            this,
            tr("Can't create a new note: the notebook model item has notebook "
               "type but null pointer to the actual notebook item")))
        return;
    }

    m_noteEditorTabsAndWindowsCoordinator->createNewNote(
        notebookItem->localId(), notebookItem->guid(), noteEditorMode);
}

void MainWindow::startSyncButtonAnimation()
{
    QNDEBUG("quentier::MainWindow", "MainWindow::startSyncButtonAnimation");

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
    QNDEBUG("quentier::MainWindow", "MainWindow::stopSyncButtonAnimation");

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
    m_ui->syncPushButton->setIcon(QIcon(QStringLiteral(":/sync/sync.png")));
}

void MainWindow::scheduleSyncButtonAnimationStop()
{
    QNDEBUG(
        "quentier::MainWindow", "MainWindow::scheduleSyncButtonAnimationStop");

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
        "quentier::MainWindow", "MainWindow::startListeningForSplitterMoves");

    QObject::connect(
        m_ui->splitter, &QSplitter::splitterMoved, this,
        &MainWindow::onSplitterHandleMoved, Qt::UniqueConnection);

    QObject::connect(
        m_ui->sidePanelSplitter, &QSplitter::splitterMoved, this,
        &MainWindow::onSidePanelSplittedHandleMoved, Qt::UniqueConnection);
}

void MainWindow::stopListeningForSplitterMoves()
{
    QNDEBUG(
        "quentier::MainWindow", "MainWindow::stopListeningForSplitterMoves");

    QObject::disconnect(
        m_ui->splitter, &QSplitter::splitterMoved, this,
        &MainWindow::onSplitterHandleMoved);

    QObject::disconnect(
        m_ui->sidePanelSplitter, &QSplitter::splitterMoved, this,
        &MainWindow::onSidePanelSplittedHandleMoved);
}

void MainWindow::persistChosenIconTheme(const QString & iconThemeName)
{
    QNDEBUG(
        "quentier::MainWindow",
        "MainWindow::persistChosenIconTheme: " << iconThemeName);

    ApplicationSettings appSettings{
        *m_account, preferences::keys::files::userInterface};

    appSettings.beginGroup(preferences::keys::appearanceGroup);
    appSettings.setValue(preferences::keys::iconTheme, iconThemeName);
    appSettings.endGroup();
}

void MainWindow::refreshChildWidgetsThemeIcons()
{
    QNDEBUG(
        "quentier::MainWindow", "MainWindow::refreshChildWidgetsThemeIcons");

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
        "quentier::MainWindow",
        "MainWindow::refreshNoteEditorWidgetsSpecialIcons");

    if (m_noteEditorTabsAndWindowsCoordinator) {
        m_noteEditorTabsAndWindowsCoordinator
            ->refreshNoteEditorWidgetsSpecialIcons();
    }
}

void MainWindow::showHideViewColumnsForAccountType(
    const Account::Type accountType)
{
    QNDEBUG(
        "quentier::MainWindow",
        "MainWindow::showHideViewColumnsForAccountType: " << accountType);

    const bool isLocal = (accountType == Account::Type::Local);

    auto * notebooksTreeView = m_ui->notebooksTreeView;
    notebooksTreeView->setColumnHidden(
        static_cast<int>(NotebookModel::Column::Published), isLocal);

    notebooksTreeView->setColumnHidden(
        static_cast<int>(NotebookModel::Column::Dirty), isLocal);

    auto * tagsTreeView = m_ui->tagsTreeView;
    tagsTreeView->setColumnHidden(
        static_cast<int>(TagModel::Column::Dirty), isLocal);

    auto * savedSearchesItemView = m_ui->savedSearchesItemView;
    savedSearchesItemView->setColumnHidden(
        static_cast<int>(SavedSearchModel::Column::Dirty), isLocal);

    auto * deletedNotesTableView = m_ui->deletedNotesTableView;
    deletedNotesTableView->setColumnHidden(
        static_cast<int>(NoteModel::Column::Dirty), isLocal);
}

void MainWindow::expandFiltersView()
{
    QNDEBUG("quentier::MainWindow", "MainWindow::expandFiltersView");

    m_ui->filtersViewTogglePushButton->setIcon(
        QIcon::fromTheme(QStringLiteral("go-down")));

    m_ui->filterBodyFrame->show();
    m_ui->filterFrameBottomBoundary->hide();
    m_ui->filterFrame->adjustSize();
}

void MainWindow::foldFiltersView()
{
    QNDEBUG("quentier::MainWindow", "MainWindow::foldFiltersView");

    m_ui->filtersViewTogglePushButton->setIcon(
        QIcon::fromTheme(QStringLiteral("go-next")));

    m_ui->filterBodyFrame->hide();
    m_ui->filterFrameBottomBoundary->show();

    if (m_shown) {
        adjustNoteListAndFiltersSplitterSizes();
    }
}

void MainWindow::adjustNoteListAndFiltersSplitterSizes()
{
    QNDEBUG(
        "quentier::MainWindow",
        "MainWindow::adjustNoteListAndFiltersSplitterSizes");

    auto splitterSizes = m_ui->noteListAndFiltersSplitter->sizes();
    Q_ASSERT(splitterSizes.count() <= std::numeric_limits<int>::max());
    const int count = static_cast<int>(splitterSizes.count());
    if (Q_UNLIKELY(count != 2)) {
        ErrorString error{
            QT_TR_NOOP("Internal error: can't properly resize the splitter "
                       "after folding the filter view: wrong number of sizes "
                       "within the splitter")};

        QNWARNING("quentier::MainWindow", error << "Sizes count: " << count);
        onSetStatusBarText(error.localizedString(), secondsToMilliseconds(30));
        return;
    }

    const int filtersPanelHeight = m_ui->noteFiltersGenericPanel->height();
    const int heightDiff = std::max(splitterSizes[0] - filtersPanelHeight, 0);
    splitterSizes[0] = filtersPanelHeight;
    splitterSizes[1] = splitterSizes[1] + heightDiff;
    m_ui->noteListAndFiltersSplitter->setSizes(splitterSizes);

    // Need to schedule the repaint because otherwise the actions above
    // seem to have no effect
    m_ui->noteListAndFiltersSplitter->update();
}

void MainWindow::restorePanelColors()
{
    if (!m_account) {
        return;
    }

    ApplicationSettings settings{
        *m_account, preferences::keys::files::userInterface};

    settings.beginGroup(preferences::keys::panelColorsGroup);
    ApplicationSettings::GroupCloser groupCloser{settings};

    const QString fontColorName =
        settings.value(preferences::keys::panelFontColor).toString();

    QColor fontColor{fontColorName};
    if (!fontColor.isValid()) {
        fontColor = QColor{Qt::white};
    }

    const QString backgroundColorName =
        settings.value(preferences::keys::panelBackgroundColor).toString();

    QColor backgroundColor{backgroundColorName};
    if (!backgroundColor.isValid()) {
        backgroundColor = QColor{Qt::darkGray};
    }

    QLinearGradient gradient{0, 0, 0, 1};

    const int rowCount = settings.beginReadArray(
        preferences::keys::panelBackgroundGradientLineCount);

    for (int i = 0; i < rowCount; ++i) {
        settings.setArrayIndex(i);
        bool conversionResult = false;

        const double value =
            settings.value(preferences::keys::panelBackgroundGradientLineSize)
                .toDouble(&conversionResult);

        if (Q_UNLIKELY(!conversionResult)) {
            QNWARNING(
                "quentier::MainWindow",
                "Failed to convert panel background gradient row value to "
                    << "double");

            gradient = QLinearGradient{0, 0, 0, 1};
            break;
        }

        const QString colorName =
            settings.value(preferences::keys::panelBackgroundGradientLineColor)
                .toString();

        QColor color{colorName};
        if (!color.isValid()) {
            QNWARNING(
                "quentier::MainWindow",
                "Failed to convert panel background gradient row color name to "
                    << "valid color: " << colorName);

            gradient = QLinearGradient{0, 0, 0, 1};
            break;
        }

        gradient.setColorAt(value, color);
    }
    settings.endArray();

    const bool useBackgroundGradient =
        settings.value(preferences::keys::panelUseBackgroundGradient).toBool();

    for (auto & panelStyleController: m_genericPanelStyleControllers) {
        if (useBackgroundGradient) {
            panelStyleController->setOverrideColors(fontColor, gradient);
        }
        else {
            panelStyleController->setOverrideColors(fontColor, backgroundColor);
        }
    }

    for (auto & panelStyleController: m_sidePanelStyleControllers) {
        if (useBackgroundGradient) {
            panelStyleController->setOverrideColors(fontColor, gradient);
        }
        else {
            panelStyleController->setOverrideColors(fontColor, backgroundColor);
        }
    }
}

void MainWindow::setupGenericPanelStyleControllers()
{
    const auto panels = findChildren<PanelWidget *>(
        QRegularExpression{QStringLiteral("(.*)GenericPanel")});

    m_genericPanelStyleControllers.clear();
    m_genericPanelStyleControllers.reserve(static_cast<std::size_t>(
        std::max<decltype(panels.size())>(panels.size(), 0)));

    QString extraStyleSheet;
    for (auto * panel: std::as_const(panels)) {
        if (panel->objectName().startsWith(QStringLiteral("upperBar"))) {
            QTextStream strm{&extraStyleSheet};
            strm << "#upperBarGenericPanel {\n"
                 << "border-bottom: 1px solid black;\n"
                 << "}\n";
        }
        else {
            extraStyleSheet.clear();
        }

        m_genericPanelStyleControllers.emplace_back(
            std::make_unique<PanelStyleController>(panel, extraStyleSheet));
    }
}

void MainWindow::setupSidePanelStyleControllers()
{
    auto panels = findChildren<PanelWidget *>(
        QRegularExpression{QStringLiteral("(.*)SidePanel")});

    m_sidePanelStyleControllers.clear();
    m_sidePanelStyleControllers.reserve(static_cast<std::size_t>(
        std::max<decltype(panels.size())>(panels.size(), 0)));

    for (auto * panel: std::as_const(panels)) {
        m_sidePanelStyleControllers.emplace_back(
            std::make_unique<SidePanelStyleController>(panel));
    }
}

void MainWindow::onSetStatusBarText(QString message, const int durationMsec)
{
    auto * statusBar = m_ui->statusBar;
    statusBar->clearMessage();

    if (m_currentStatusBarChildWidget) {
        statusBar->removeWidget(m_currentStatusBarChildWidget);
        m_currentStatusBarChildWidget = nullptr;
    }

    if (durationMsec == 0) {
        m_currentStatusBarChildWidget = new QLabel{message};
        statusBar->addWidget(m_currentStatusBarChildWidget);
    }
    else {
        statusBar->showMessage(message, durationMsec);
    }
}

#define DISPATCH_TO_NOTE_EDITOR(MainWindowMethod, NoteEditorMethod)            \
    void MainWindow::MainWindowMethod()                                        \
    {                                                                          \
        QNDEBUG("quentier::MainWindow", "MainWindow::" #MainWindowMethod);     \
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
    QNDEBUG("quentier::MainWindow", "MainWindow::onImportEnexAction");

    if (Q_UNLIKELY(!m_account)) {
        QNDEBUG("quentier::MainWindow", "No current account, skipping");
        return;
    }

    if (Q_UNLIKELY(!m_localStorage)) {
        QNDEBUG("quentier::MainWindow", "No local storage, skipping");
        return;
    }

    if (Q_UNLIKELY(!m_tagModel)) {
        QNDEBUG("quentier::MainWindow", "No tag model, skipping");
        return;
    }

    if (Q_UNLIKELY(!m_notebookModel)) {
        QNDEBUG("quentier::MainWindow", "No notebook model, skipping");
        return;
    }

    auto enexImportDialog =
        std::make_unique<EnexImportDialog>(*m_account, *m_notebookModel, this);

    enexImportDialog->setWindowModality(Qt::WindowModal);
    centerDialog(*enexImportDialog);
    if (enexImportDialog->exec() != QDialog::Accepted) {
        QNDEBUG("quentier::MainWindow", "The import of ENEX was cancelled");
        return;
    }

    ErrorString errorDescription;

    const QString enexFilePath =
        enexImportDialog->importEnexFilePath(&errorDescription);

    if (enexFilePath.isEmpty()) {
        if (errorDescription.isEmpty()) {
            errorDescription.setBase(
                QT_TR_NOOP("Can't import ENEX: internal error, can't retrieve "
                           "ENEX file path"));
        }

        QNDEBUG(
            "quentier::MainWindow", "Bad ENEX file path: " << errorDescription);

        onSetStatusBarText(
            errorDescription.localizedString(), secondsToMilliseconds(30));
        return;
    }

    const QString notebookName =
        enexImportDialog->notebookName(&errorDescription);
    if (notebookName.isEmpty()) {
        if (errorDescription.isEmpty()) {
            errorDescription.setBase(
                QT_TR_NOOP("Can't import ENEX: internal error, can't retrieve "
                           "notebook name"));
        }

        QNDEBUG(
            "quentier::MainWindow", "Bad notebook name: " << errorDescription);

        onSetStatusBarText(
            errorDescription.localizedString(), secondsToMilliseconds(30));
        return;
    }

    auto enmlConverter = enml::createConverter();
    auto * enexImporter = new EnexImporter{
        enexFilePath,
        notebookName,
        m_localStorage,
        std::move(enmlConverter),
        *m_tagModel,
        *m_notebookModel,
        this};

    QObject::connect(
        enexImporter, &EnexImporter::enexImportedSuccessfully, this,
        &MainWindow::onEnexImportCompletedSuccessfully);

    QObject::connect(
        enexImporter, &EnexImporter::enexImportFailed, this,
        &MainWindow::onEnexImportFailed);

    enexImporter->start();
}

void MainWindow::onAuthenticationFinished(
    const bool success, ErrorString errorDescription, Account account)
{
    QNINFO(
        "quentier::MainWindow",
        "MainWindow::onAuthenticationFinished: "
            << "success = " << (success ? "true" : "false")
            << ", error description = " << errorDescription
            << ", account = " << account.name());

    const bool wasPendingNewEvernoteAccountAuthentication =
        m_pendingNewEvernoteAccountAuthentication;

    m_pendingNewEvernoteAccountAuthentication = false;

    const bool wasPendingCurrentEvernoteAccountAuthentication =
        m_pendingCurrentEvernoteAccountAuthentication;

    m_pendingCurrentEvernoteAccountAuthentication = false;

    if (!success) {
        // Restore the network proxy which was active before the authentication
        QNetworkProxy::setApplicationProxy(
            m_applicationProxyBeforeNewEvernoteAccountAuthenticationRequest);

        m_applicationProxyBeforeNewEvernoteAccountAuthenticationRequest =
            QNetworkProxy{QNetworkProxy::NoProxy};

        onSetStatusBarText(
            tr("Couldn't authenticate the Evernote user") +
                QStringLiteral(": ") + errorDescription.localizedString(),
            secondsToMilliseconds(30));

        return;
    }

    QNetworkProxy currentProxy = QNetworkProxy::applicationProxy();
    persistNetworkProxySettingsForAccount(account, currentProxy);

    m_applicationProxyBeforeNewEvernoteAccountAuthenticationRequest =
        QNetworkProxy{QNetworkProxy::NoProxy};

    if (wasPendingCurrentEvernoteAccountAuthentication) {
        m_authenticatedCurrentEvernoteAccount = true;
        launchSynchronization();
        return;
    }

    if (wasPendingNewEvernoteAccountAuthentication) {
        m_pendingSwitchToNewEvernoteAccount = true;
        m_accountManager->switchAccount(account);
    }
}

void MainWindow::onRateLimitExceeded(const std::optional<qint32> secondsToWait)
{
    QNINFO(
        "quentier::MainWindow",
        "MainWindow::onRateLimitExceeded: seconds to wait = "
            << (secondsToWait ? QString::number(*secondsToWait)
                              : QStringLiteral("<none>")));

    m_syncApiRateLimitExceeded = true;
    stopSynchronization(StopSynchronizationMode::Quiet);

    if (!secondsToWait) {
        onSetStatusBarText(
            tr("Synchronization was stopped: Evernote API rate limit reached, "
               "please restart sync later"),
            secondsToMilliseconds(60));
        return;
    }

    const qint64 currentTimestamp = QDateTime::currentMSecsSinceEpoch();
    const qint64 futureTimestamp = currentTimestamp + *secondsToWait * 1000;
    QDateTime futureDateTime;
    futureDateTime.setMSecsSinceEpoch(futureTimestamp);

    const QDateTime today = QDateTime::currentDateTime();
    const bool includeDate = (today.date() != futureDateTime.date());

    const QString dateTimeToShow =
        (includeDate
             ? futureDateTime.toString(QStringLiteral("dd.MM.yyyy hh:mm:ss"))
             : futureDateTime.toString(QStringLiteral("hh:mm:ss")));

    onSetStatusBarText(
        tr("The synchronization has reached Evernote API rate "
           "limit, it will continue automatically at approximately") +
            QStringLiteral(" ") + dateTimeToShow,
        secondsToMilliseconds(60));

    QNINFO(
        "quentier::MainWindow",
        "Evernote API rate limit exceeded, need to wait for "
            << *secondsToWait
            << " seconds, the synchronization will continue at "
            << dateTimeToShow);
}

void MainWindow::onEvernoteAccountAuthenticationRequested(
    QString host, QNetworkProxy proxy)
{
    QNDEBUG(
        "quentier::MainWindow",
        "MainWindow::onEvernoteAccountAuthenticationRequested: host = "
            << host << ", proxy type = " << proxy.type() << ", proxy host = "
            << proxy.hostName() << ", proxy port = " << proxy.port()
            << ", proxy user = " << proxy.user());

    m_synchronizationRemoteHost = host;
    setupSynchronizer();

    // Set the proxy specified within the slot argument but remember the
    // previous application proxy so that it can be restored in case of
    // authentication failure
    m_applicationProxyBeforeNewEvernoteAccountAuthenticationRequest =
        QNetworkProxy::applicationProxy();

    QNetworkProxy::setApplicationProxy(proxy);

    m_pendingNewEvernoteAccountAuthentication = true;

    Q_ASSERT(m_synchronizer);

    auto canceler = setupSyncCanceler();
    Q_ASSERT(canceler);

    auto authenticationFuture = m_synchronizer->authenticateNewAccount();
    auto authenticationThenFuture = threading::then(
        std::move(authenticationFuture), this,
        [this, canceler](std::pair<Account, synchronization::IAuthenticationInfoPtr>
                   result) {
            if (canceler->isCanceled()) {
                return;
            }

            onAuthenticationFinished(
                true, ErrorString{}, std::move(result.first));
        });

    threading::onFailed(
        std::move(authenticationThenFuture), this,
        [this, canceler = std::move(canceler)](const QException & e) {
            if (canceler->isCanceled()) {
                return;
            }

            auto message = exceptionMessage(e);
            onAuthenticationFinished(false, std::move(message), Account{});
        });
}

void MainWindow::onNoteTextSpellCheckToggled()
{
    QNDEBUG("quentier::MainWindow", "MainWindow::onNoteTextSpellCheckToggled");

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
    QNDEBUG("quentier::MainWindow", "MainWindow::onShowNoteSource");

    auto * noteEditorWidget = currentNoteEditorTab();
    if (!noteEditorWidget) {
        return;
    }

    if (!noteEditorWidget->isNoteSourceShown()) {
        noteEditorWidget->showNoteSource();
    }
    else {
        noteEditorWidget->hideNoteSource();
    }
}

void MainWindow::onSaveNoteAction()
{
    QNDEBUG("quentier::MainWindow", "MainWindow::onSaveNoteAction");

    auto * noteEditorWidget = currentNoteEditorTab();
    if (!noteEditorWidget) {
        return;
    }

    noteEditorWidget->onSaveNoteAction();
}

void MainWindow::onNewNotebookCreationRequested()
{
    QNDEBUG(
        "quentier::MainWindow", "MainWindow::onNewNotebookCreationRequested");

    if (Q_UNLIKELY(!m_notebookModel)) {
        ErrorString error{
            QT_TR_NOOP("Can't create a new notebook: no notebook model is set "
                       "up")};
        QNWARNING("quentier::MainWindow", error);
        onSetStatusBarText(error.localizedString(), secondsToMilliseconds(30));
        return;
    }

    const auto addNotebookDialog =
        std::make_unique<AddOrEditNotebookDialog>(m_notebookModel, this);

    addNotebookDialog->setWindowModality(Qt::WindowModal);
    centerDialog(*addNotebookDialog);
    addNotebookDialog->exec();
}

void MainWindow::onRemoveNotebookButtonPressed()
{
    QNDEBUG(
        "quentier::MainWindow", "MainWindow::onRemoveNotebookButtonPressed");

    m_ui->notebooksTreeView->deleteSelectedItem();
}

void MainWindow::onNotebookInfoButtonPressed()
{
    QNDEBUG("quentier::MainWindow", "MainWindow::onNotebookInfoButtonPressed");

    const auto index = m_ui->notebooksTreeView->currentlySelectedItemIndex();

    auto * notebookModelItemInfoWidget =
        new NotebookModelItemInfoWidget{index, this};

    showInfoWidget(notebookModelItemInfoWidget);
}

void MainWindow::onNewTagCreationRequested()
{
    QNDEBUG("quentier::MainWindow", "MainWindow::onNewTagCreationRequested");

    if (Q_UNLIKELY(!m_tagModel)) {
        ErrorString error{
            QT_TR_NOOP("Can't create a new tag: no tag model is set up")};
        QNWARNING("quentier::MainWindow", error);
        onSetStatusBarText(error.localizedString(), secondsToMilliseconds(30));
        return;
    }

    const auto addTagDialog =
        std::make_unique<AddOrEditTagDialog>(m_tagModel, this);

    addTagDialog->setWindowModality(Qt::WindowModal);
    centerDialog(*addTagDialog);
    addTagDialog->exec();
}

void MainWindow::onRemoveTagButtonPressed()
{
    QNDEBUG("quentier::MainWindow", "MainWindow::onRemoveTagButtonPressed");
    m_ui->tagsTreeView->deleteSelectedItem();
}

void MainWindow::onTagInfoButtonPressed()
{
    QNDEBUG("quentier::MainWindow", "MainWindow::onTagInfoButtonPressed");

    const auto index = m_ui->tagsTreeView->currentlySelectedItemIndex();
    auto * tagModelItemInfoWidget = new TagModelItemInfoWidget{index, this};
    showInfoWidget(tagModelItemInfoWidget);
}

void MainWindow::onNewSavedSearchCreationRequested()
{
    QNDEBUG(
        "quentier::MainWindow",
        "MainWindow::onNewSavedSearchCreationRequested");

    if (Q_UNLIKELY(!m_savedSearchModel)) {
        ErrorString error{
            QT_TR_NOOP("Can't create a new saved search: no saved search model "
                       "is set up")};
        QNWARNING("quentier::MainWindow", error);
        onSetStatusBarText(error.localizedString(), secondsToMilliseconds(30));
        return;
    }

    const auto addSavedSearchDialog =
        std::make_unique<AddOrEditSavedSearchDialog>(m_savedSearchModel, this);

    addSavedSearchDialog->setWindowModality(Qt::WindowModal);
    centerDialog(*addSavedSearchDialog);
    addSavedSearchDialog->exec();
}

void MainWindow::onRemoveSavedSearchButtonPressed()
{
    QNDEBUG(
        "quentier::MainWindow", "MainWindow::onRemoveSavedSearchButtonPressed");

    m_ui->savedSearchesItemView->deleteSelectedItem();
}

void MainWindow::onSavedSearchInfoButtonPressed()
{
    QNDEBUG(
        "quentier::MainWindow", "MainWindow::onSavedSearchInfoButtonPressed");

    const auto index =
        m_ui->savedSearchesItemView->currentlySelectedItemIndex();

    auto * savedSearchModelItemInfoWidget =
        new SavedSearchModelItemInfoWidget{index, this};

    showInfoWidget(savedSearchModelItemInfoWidget);
}

void MainWindow::onUnfavoriteItemButtonPressed()
{
    QNDEBUG(
        "quentier::MainWindow", "MainWindow::onUnfavoriteItemButtonPressed");

    m_ui->favoritesTableView->unfavoriteSelectedItems();
}

void MainWindow::onFavoritedItemInfoButtonPressed()
{
    QNDEBUG(
        "quentier::MainWindow", "MainWindow::onFavoritedItemInfoButtonPressed");

    const auto index = m_ui->favoritesTableView->currentlySelectedItemIndex();
    if (!index.isValid()) {
        informationMessageBox(
            this, tr("Not exactly one favorited item is selected"),
            tr("Please select the only one favorited item to see its detailed "
               "info"));
        return;
    }

    auto * favoritesModel =
        qobject_cast<FavoritesModel *>(m_ui->favoritesTableView->model());

    if (Q_UNLIKELY(!favoritesModel)) {
        internalErrorMessageBox(
            this,
            tr("Failed to cast the favorited table view's model to favorites "
               "model"));
        return;
    }

    const auto * item = favoritesModel->itemAtRow(index.row());
    if (Q_UNLIKELY(!item)) {
        internalErrorMessageBox(
            this,
            tr("Favorites model returned null pointer to favorited item for "
               "the selected index"));
        return;
    }

    switch (item->type()) {
    case FavoritesModelItem::Type::Note:
        Q_EMIT noteInfoDialogRequested(item->localId());
        break;
    case FavoritesModelItem::Type::Notebook:
    {
        if (Q_LIKELY(m_notebookModel)) {
            const auto notebookIndex =
                m_notebookModel->indexForLocalId(item->localId());

            auto * notebookItemInfoWidget =
                new NotebookModelItemInfoWidget{notebookIndex, this};

            showInfoWidget(notebookItemInfoWidget);
        }
        else {
            Q_UNUSED(internalErrorMessageBox(
                this, tr("No notebook model exists at the moment")))
        }
        break;
    }
    case FavoritesModelItem::Type::SavedSearch:
    {
        if (Q_LIKELY(m_savedSearchModel)) {
            const auto savedSearchIndex =
                m_savedSearchModel->indexForLocalId(item->localId());

            auto * savedSearchItemInfoWidget =
                new SavedSearchModelItemInfoWidget(savedSearchIndex, this);

            showInfoWidget(savedSearchItemInfoWidget);
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
        if (Q_LIKELY(m_tagModel)) {
            const auto tagIndex = m_tagModel->indexForLocalId(item->localId());

            auto * tagItemInfoWidget =
                new TagModelItemInfoWidget(tagIndex, this);

            showInfoWidget(tagItemInfoWidget);
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
        QDebug dbg{&type};
        dbg << item->type();

        internalErrorMessageBox(
            this,
            tr("Incorrect favorited item type") + QStringLiteral(": ") + type);
    } break;
    }
}

void MainWindow::onRestoreDeletedNoteButtonPressed()
{
    QNDEBUG(
        "quentier::MainWindow",
        "MainWindow::onRestoreDeletedNoteButtonPressed");

    m_ui->deletedNotesTableView->restoreCurrentlySelectedNote();
}

void MainWindow::onDeleteNotePermanentlyButtonPressed()
{
    QNDEBUG(
        "quentier::MainWindow",
        "MainWindow::onDeleteNotePermanentlyButtonPressed");

    m_ui->deletedNotesTableView->deleteCurrentlySelectedNotePermanently();
}

void MainWindow::onDeletedNoteInfoButtonPressed()
{
    QNDEBUG(
        "quentier::MainWindow", "MainWindow::onDeletedNoteInfoButtonPressed");

    m_ui->deletedNotesTableView->showCurrentlySelectedNoteInfo();
}

void MainWindow::showInfoWidget(QWidget * widget)
{
    widget->setAttribute(Qt::WA_DeleteOnClose);
    widget->setWindowModality(Qt::WindowModal);
    widget->adjustSize();

#ifndef Q_OS_MAC
    centerWidget(*widget);
#endif

    widget->show();
}

void MainWindow::onFiltersViewTogglePushButtonPressed()
{
    QNDEBUG(
        "quentier::MainWindow",
        "MainWindow::onFiltersViewTogglePushButtonPressed");

    m_filtersViewExpanded = !m_filtersViewExpanded;

    if (m_filtersViewExpanded) {
        expandFiltersView();
    }
    else {
        foldFiltersView();
    }

    ApplicationSettings appSettings{
        *m_account, preferences::keys::files::userInterface};

    appSettings.beginGroup(QStringLiteral("FiltersView"));

    appSettings.setValue(
        gFiltersViewStatusKey, QVariant{m_filtersViewExpanded});

    appSettings.endGroup();
}

void MainWindow::onShowPreferencesDialogAction()
{
    QNDEBUG(
        "quentier::MainWindow", "MainWindow::onShowPreferencesDialogAction");

    auto * existingPreferencesDialog = findChild<PreferencesDialog *>();
    if (existingPreferencesDialog) {
        QNDEBUG(
            "quentier::MainWindow",
            "Preferences dialog already exists, won't show another one");
        return;
    }

    auto menus = m_ui->menuBar->findChildren<QMenu *>();
    ActionsInfo actionsInfo{menus};

    auto preferencesDialog = std::make_unique<PreferencesDialog>(
        *m_accountManager, m_shortcutManager, *m_systemTrayIconManager,
        actionsInfo, this);

    preferencesDialog->setWindowModality(Qt::WindowModal);
    centerDialog(*preferencesDialog);
    connectToPreferencesDialogSignals(*preferencesDialog);
    preferencesDialog->exec();
}

void MainWindow::onNoteSortingModeChanged(const int index)
{
    QNDEBUG(
        "quentier::MainWindow",
        "MainWindow::onNoteSortingModeChanged: index = " << index);

    persistChosenNoteSortingMode(index);

    if (Q_UNLIKELY(!m_noteModel)) {
        QNDEBUG("quentier::MainWindow", "No note model, ignoring the change");
        return;
    }

    switch (static_cast<NoteModel::NoteSortingMode>(index)) {
    case NoteModel::NoteSortingMode::CreatedAscending:
        m_noteModel->sort(
            static_cast<int>(NoteModel::Column::CreationTimestamp),
            Qt::AscendingOrder);
        break;
    case NoteModel::NoteSortingMode::CreatedDescending:
        m_noteModel->sort(
            static_cast<int>(NoteModel::Column::CreationTimestamp),
            Qt::DescendingOrder);
        break;
    case NoteModel::NoteSortingMode::ModifiedAscending:
        m_noteModel->sort(
            static_cast<int>(NoteModel::Column::ModificationTimestamp),
            Qt::AscendingOrder);
        break;
    case NoteModel::NoteSortingMode::ModifiedDescending:
        m_noteModel->sort(
            static_cast<int>(NoteModel::Column::ModificationTimestamp),
            Qt::DescendingOrder);
        break;
    case NoteModel::NoteSortingMode::TitleAscending:
        m_noteModel->sort(
            static_cast<int>(NoteModel::Column::Title), Qt::AscendingOrder);
        break;
    case NoteModel::NoteSortingMode::TitleDescending:
        m_noteModel->sort(
            static_cast<int>(NoteModel::Column::Title), Qt::DescendingOrder);
        break;
    case NoteModel::NoteSortingMode::SizeAscending:
        m_noteModel->sort(
            static_cast<int>(NoteModel::Column::Size), Qt::AscendingOrder);
        break;
    case NoteModel::NoteSortingMode::SizeDescending:
        m_noteModel->sort(
            static_cast<int>(NoteModel::Column::Size), Qt::DescendingOrder);
        break;
    default:
    {
        const ErrorString error{
            QT_TR_NOOP("Internal error: got unknown note sorting order, "
                       "fallback to the default")};

        QNWARNING(
            "quentier::MainWindow",
            error << ", sorting mode index = " << index);

        onSetStatusBarText(error.localizedString(), secondsToMilliseconds(30));

        m_noteModel->sort(
            static_cast<int>(NoteModel::Column::CreationTimestamp),
            Qt::AscendingOrder);

        break;
    }
    }
}

void MainWindow::onNewNoteCreationRequested()
{
    QNDEBUG("quentier::MainWindow", "MainWindow::onNewNoteCreationRequested");
    createNewNote(NoteEditorTabsAndWindowsCoordinator::NoteEditorMode::Any);
}

void MainWindow::onToggleThumbnailsPreference(const QString & noteLocalId)
{
    QNDEBUG(
        "quentier::MainWindow",
        "MainWindow::onToggleThumbnailsPreference: note local id = "
            << noteLocalId);

    const bool toggleForAllNotes = noteLocalId.isEmpty();
    if (toggleForAllNotes) {
        toggleShowNoteThumbnails();
    }
    else {
        toggleHideNoteThumbnail(noteLocalId);
    }

    onShowNoteThumbnailsPreferenceChanged();
}

void MainWindow::onCopyInAppLinkNoteRequested(
    const QString & noteLocalId, const QString & noteGuid)
{
    QNDEBUG(
        "quentier::MainWindow",
        "MainWindow::onCopyInAppLinkNoteRequested: note local id = "
            << noteLocalId << ", note guid = " << noteGuid);

    if (noteGuid.isEmpty()) {
        QNDEBUG(
            "quentier::MainWindow",
            "Can't copy the in-app note link: note guid is empty");
        return;
    }

    if (Q_UNLIKELY(!m_account)) {
        QNDEBUG(
            "quentier::MainWindow",
            "Can't copy the in-app note link: no current account");
        return;
    }

    if (Q_UNLIKELY(m_account->type() != Account::Type::Evernote)) {
        QNDEBUG(
            "quentier::MainWindow",
            "Can't copy the in-app note link: the current account is not of "
            "Evernote type");
        return;
    }

    const auto id = m_account->id();
    if (Q_UNLIKELY(id < 0)) {
        QNDEBUG(
            "quentier::MainWindow",
            "Can't copy the in-app note link: the current account's id is "
            "negative");
        return;
    }

    const QString shardId = m_account->shardId();
    if (shardId.isEmpty()) {
        QNDEBUG(
            "quentier::MainWindow",
            "Can't copy the in-app note link: the current account's shard id "
            "is empty");
        return;
    }

    const QString urlString = QStringLiteral("evernote:///view/") +
        QString::number(id) + QStringLiteral("/") + shardId +
        QStringLiteral("/") + noteGuid + QStringLiteral("/") + noteGuid;

    auto * clipboard = QApplication::clipboard();
    if (clipboard) {
        QNTRACE(
            "quentier::MainWindow",
            "Setting the composed in-app note URL to the clipboard: "
                << urlString);
        clipboard->setText(urlString);
    }
}

void MainWindow::onFavoritedNoteSelected(const QString & noteLocalId)
{
    QNDEBUG(
        "quentier::MainWindow",
        "MainWindow::onFavoritedNoteSelected: " << noteLocalId);

    if (Q_UNLIKELY(!m_noteEditorTabsAndWindowsCoordinator)) {
        QNDEBUG(
            "quentier::MainWindow",
            "No note editor tabs and windows coordinator, skipping");
        return;
    }

    m_noteEditorTabsAndWindowsCoordinator->addNote(noteLocalId);
}

void MainWindow::onCurrentNoteInListChanged(const QString & noteLocalId)
{
    QNDEBUG(
        "quentier::MainWindow",
        "MainWindow::onCurrentNoteInListChanged: " << noteLocalId);

    m_noteEditorTabsAndWindowsCoordinator->addNote(noteLocalId);
}

void MainWindow::onOpenNoteInSeparateWindow(const QString & noteLocalId)
{
    QNDEBUG(
        "quentier::MainWindow",
        "MainWindow::onOpenNoteInSeparateWindow: " << noteLocalId);

    m_noteEditorTabsAndWindowsCoordinator->addNote(
        noteLocalId,
        NoteEditorTabsAndWindowsCoordinator::NoteEditorMode::Window);
}

void MainWindow::onDeleteCurrentNoteButtonPressed()
{
    QNDEBUG(
        "quentier::MainWindow", "MainWindow::onDeleteCurrentNoteButtonPressed");

    if (Q_UNLIKELY(!m_noteModel)) {
        const ErrorString errorDescription{
            QT_TR_NOOP("Can't delete the current note: internal error, no note "
                       "model")};

        QNDEBUG("quentier::MainWindow", errorDescription);
        onSetStatusBarText(
            errorDescription.localizedString(), secondsToMilliseconds(30));
        return;
    }

    auto * noteEditorWidget = currentNoteEditorTab();
    if (!noteEditorWidget) {
        const ErrorString errorDescription{
            QT_TR_NOOP("Can't delete the current note: no note editor tabs")};
        QNDEBUG("quentier::MainWindow", errorDescription);
        onSetStatusBarText(
            errorDescription.localizedString(), secondsToMilliseconds(30));
        return;
    }

    ErrorString error;
    if (Q_UNLIKELY(
            !m_noteModel->deleteNote(noteEditorWidget->noteLocalId(), error)))
    {
        ErrorString errorDescription{
            QT_TR_NOOP("Can't delete the current note")};
        errorDescription.appendBase(error.base());
        errorDescription.appendBase(error.additionalBases());
        errorDescription.details() = error.details();
        QNDEBUG("quentier::MainWindow", errorDescription);
        onSetStatusBarText(
            errorDescription.localizedString(), secondsToMilliseconds(30));
        return;
    }
}

void MainWindow::onCurrentNoteInfoRequested()
{
    QNDEBUG("quentier::MainWindow", "MainWindow::onCurrentNoteInfoRequested");

    auto * noteEditorWidget = currentNoteEditorTab();
    if (!noteEditorWidget) {
        const ErrorString errorDescription{
            QT_TR_NOOP("Can't show note info: no note editor tabs")};
        QNDEBUG("quentier::MainWindow", errorDescription);
        onSetStatusBarText(
            errorDescription.localizedString(), secondsToMilliseconds(30));
        return;
    }

    Q_EMIT noteInfoDialogRequested(noteEditorWidget->noteLocalId());
}

void MainWindow::onCurrentNotePrintRequested()
{
    QNDEBUG("quentier::MainWindow", "MainWindow::onCurrentNotePrintRequested");

    auto * noteEditorWidget = currentNoteEditorTab();
    if (!noteEditorWidget) {
        const ErrorString errorDescription{
            QT_TR_NOOP("Can't print note: no note editor tabs")};
        QNDEBUG("quentier::MainWindow", errorDescription);
        onSetStatusBarText(
            errorDescription.localizedString(), secondsToMilliseconds(30));
        return;
    }

    ErrorString errorDescription;
    if (!noteEditorWidget->printNote(errorDescription)) {
        if (errorDescription.isEmpty()) {
            return;
        }

        QNDEBUG("quentier::MainWindow", errorDescription);
        onSetStatusBarText(
            errorDescription.localizedString(), secondsToMilliseconds(30));
    }
}

void MainWindow::onCurrentNotePdfExportRequested()
{
    QNDEBUG(
        "quentier::MainWindow", "MainWindow::onCurrentNotePdfExportRequested");

    auto * noteEditorWidget = currentNoteEditorTab();
    if (!noteEditorWidget) {
        const ErrorString errorDescription{
            QT_TR_NOOP("Can't export note to pdf: no note editor tabs")};
        QNDEBUG("quentier::MainWindow", errorDescription);
        onSetStatusBarText(
            errorDescription.localizedString(), secondsToMilliseconds(30));
        return;
    }

    ErrorString errorDescription;
    if (!noteEditorWidget->exportNoteToPdf(errorDescription)) {
        if (errorDescription.isEmpty()) {
            return;
        }

        QNDEBUG("quentier::MainWindow", errorDescription);
        onSetStatusBarText(
            errorDescription.localizedString(), secondsToMilliseconds(30));
    }
}

void MainWindow::onExportNotesToEnexRequested(QStringList noteLocalIds)
{
    QNDEBUG(
        "quentier::MainWindow",
        "MainWindow::onExportNotesToEnexRequested: "
            << noteLocalIds.join(QStringLiteral(", ")));

    if (Q_UNLIKELY(noteLocalIds.isEmpty())) {
        QNDEBUG(
            "quentier::MainWindow",
            "The list of note local uids to export is empty");
        return;
    }

    if (Q_UNLIKELY(!m_account)) {
        QNDEBUG("quentier::MainWindow", "No current account, skipping");
        return;
    }

    if (Q_UNLIKELY(!m_localStorage)) {
        QNDEBUG("quentier::MainWindow", "No local storage, skipping");
        return;
    }

    if (Q_UNLIKELY(!m_noteEditorTabsAndWindowsCoordinator)) {
        QNDEBUG(
            "quentier::MainWindow",
            "No note editor tabs and windows coordinator, skipping");
        return;
    }

    if (Q_UNLIKELY(!m_tagModel)) {
        QNDEBUG("quentier::MainWindow", "No tag model, skipping");
        return;
    }

    ApplicationSettings appSettings{
        *m_account, preferences::keys::files::userInterface};

    appSettings.beginGroup(preferences::keys::noteEditorGroup);

    QString lastExportNoteToEnexPath =
        appSettings.value(preferences::keys::lastExportNotesToEnexPath)
            .toString();

    appSettings.endGroup();

    if (lastExportNoteToEnexPath.isEmpty()) {
        lastExportNoteToEnexPath = documentsPath();
    }

    auto exportEnexDialog =
        std::make_unique<EnexExportDialog>(*m_account, this);

    exportEnexDialog->setWindowModality(Qt::WindowModal);
    centerDialog(*exportEnexDialog);
    if (exportEnexDialog->exec() != QDialog::Accepted) {
        QNDEBUG("quentier::MainWindow", "Enex export was not confirmed");
        return;
    }

    const QString enexFilePath = exportEnexDialog->exportEnexFilePath();

    const QFileInfo enexFileInfo{enexFilePath};
    if (enexFileInfo.exists()) {
        if (!enexFileInfo.isWritable()) {
            QNINFO(
                "quentier::MainWindow",
                "Chosen ENEX export file is not writable: " << enexFilePath);

            onSetStatusBarText(
                tr("The file selected for ENEX export is not writable") +
                QStringLiteral(": ") + enexFilePath);

            return;
        }
    }
    else {
        const QDir enexFileDir = enexFileInfo.absoluteDir();
        if (!enexFileDir.exists() &&
            !enexFileDir.mkpath(enexFileInfo.absolutePath()))
        {
            QNDEBUG(
                "quentier::MainWindow",
                "Failed to create folder for the selected ENEX file");

            onSetStatusBarText(
                tr("Could not create the folder for the selected ENEX "
                   "file") +
                    QStringLiteral(": ") + enexFilePath,
                secondsToMilliseconds(30));

            return;
        }
    }

    auto * exporter = new EnexExporter{
        m_localStorage, *m_noteEditorTabsAndWindowsCoordinator, *m_tagModel,
        this};

    exporter->setTargetEnexFilePath(enexFilePath);
    exporter->setIncludeTags(exportEnexDialog->exportTags());
    exporter->setNoteLocalIds(std::move(noteLocalIds));

    QObject::connect(
        exporter, &EnexExporter::notesExportedToEnex, this,
        &MainWindow::onExportedNotesToEnex);

    QObject::connect(
        exporter, &EnexExporter::failedToExportNotesToEnex, this,
        &MainWindow::onExportNotesToEnexFailed);

    exporter->start();
}

void MainWindow::onExportedNotesToEnex(const QString & enex)
{
    QNDEBUG("quentier::MainWindow", "MainWindow::onExportedNotesToEnex");

    auto * exporter = qobject_cast<EnexExporter *>(sender());
    if (Q_UNLIKELY(!exporter)) {
        const ErrorString error{
            QT_TR_NOOP("Can't export notes to ENEX: internal error, "
                       "can't cast the slot invoker to EnexExporter")};
        QNWARNING("quentier::MainWindow", error);
        onSetStatusBarText(error.localizedString(), secondsToMilliseconds(30));
        return;
    }

    const QString enexFilePath = exporter->targetEnexFilePath();
    if (Q_UNLIKELY(enexFilePath.isEmpty())) {
        const ErrorString error{
            QT_TR_NOOP("Can't export notes to ENEX: internal error, "
                       "the selected ENEX file path was lost")};
        QNWARNING("quentier::MainWindow", error);
        onSetStatusBarText(error.localizedString(), secondsToMilliseconds(30));
        return;
    }

    QByteArray enexRawData = enex.toUtf8();
    auto * asyncFileWriter = new AsyncFileWriter{enexFilePath, enexRawData};

    QObject::connect(
        asyncFileWriter, &AsyncFileWriter::fileSuccessfullyWritten, this,
        &MainWindow::onEnexFileWrittenSuccessfully);

    QObject::connect(
        asyncFileWriter, &AsyncFileWriter::fileWriteFailed, this,
        &MainWindow::onEnexFileWriteFailed);

    QObject::connect(
        asyncFileWriter, &AsyncFileWriter::fileWriteIncomplete, this,
        &MainWindow::onEnexFileWriteIncomplete);

    QThreadPool::globalInstance()->start(asyncFileWriter);
}

void MainWindow::onExportNotesToEnexFailed(const ErrorString & errorDescription)
{
    QNDEBUG(
        "quentier::MainWindow",
        "MainWindow::onExportNotesToEnexFailed: " << errorDescription);

    auto * exporter = qobject_cast<EnexExporter *>(sender());
    if (exporter) {
        exporter->clear();
        exporter->deleteLater();
    }

    onSetStatusBarText(
        errorDescription.localizedString(), secondsToMilliseconds(30));
}

void MainWindow::onEnexFileWrittenSuccessfully(const QString & filePath)
{
    QNDEBUG(
        "quentier::MainWindow",
        "MainWindow::onEnexFileWrittenSuccessfully: " << filePath);

    onSetStatusBarText(
        tr("Successfully exported note(s) to ENEX: ") +
            QDir::toNativeSeparators(filePath),
        secondsToMilliseconds(5));
}

void MainWindow::onEnexFileWriteFailed(const ErrorString & errorDescription)
{
    QNDEBUG(
        "quentier::MainWindow",
        "MainWindow::onEnexFileWriteFailed: " << errorDescription);

    onSetStatusBarText(
        tr("Can't export note(s) to ENEX, failed to write the ENEX to file") +
            QStringLiteral(": ") + errorDescription.localizedString(),
        secondsToMilliseconds(30));
}

void MainWindow::onEnexFileWriteIncomplete(
    const qint64 bytesWritten, const qint64 bytesTotal)
{
    QNDEBUG(
        "quentier::MainWindow",
        "MainWindow::onEnexFileWriteIncomplete: bytes written = "
            << bytesWritten << ", bytes total = " << bytesTotal);

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

void MainWindow::onEnexImportCompletedSuccessfully(const QString & enexFilePath)
{
    QNDEBUG(
        "quentier::MainWindow",
        "MainWindow::onEnexImportCompletedSuccessfully: " << enexFilePath);

    onSetStatusBarText(
        tr("Successfully imported note(s) from ENEX file") +
            QStringLiteral(": ") + QDir::toNativeSeparators(enexFilePath),
        secondsToMilliseconds(5));

    auto * enexImporter = qobject_cast<EnexImporter *>(sender());
    if (enexImporter) {
        enexImporter->clear();
        enexImporter->deleteLater();
    }
}

void MainWindow::onEnexImportFailed(const ErrorString & errorDescription)
{
    QNDEBUG(
        "quentier::MainWindow",
        "MainWindow::onEnexImportFailed: " << errorDescription);

    onSetStatusBarText(
        errorDescription.localizedString(), secondsToMilliseconds(30));

    auto * enexImporter = qobject_cast<EnexImporter *>(sender());
    if (enexImporter) {
        enexImporter->clear();
        enexImporter->deleteLater();
    }
}

void MainWindow::onUseLimitedFontsPreferenceChanged(const bool flag)
{
    QNDEBUG(
        "quentier::MainWindow",
        "MainWindow::onUseLimitedFontsPreferenceChanged: flag = "
            << (flag ? "enabled" : "disabled"));

    if (m_noteEditorTabsAndWindowsCoordinator) {
        m_noteEditorTabsAndWindowsCoordinator->setUseLimitedFonts(flag);
    }
}

void MainWindow::onShowNoteThumbnailsPreferenceChanged()
{
    QNDEBUG(
        "quentier::MainWindow",
        "MainWindow::onShowNoteThumbnailsPreferenceChanged");

    const bool showNoteThumbnails = getShowNoteThumbnailsPreference();

    Q_EMIT showNoteThumbnailsStateChanged(
        showNoteThumbnails, notesWithHiddenThumbnails());

    auto * noteItemDelegate =
        qobject_cast<NoteItemDelegate *>(m_ui->noteListView->itemDelegate());

    if (Q_UNLIKELY(!noteItemDelegate)) {
        QNDEBUG("quentier::MainWindow", "No NoteItemDelegate");
        return;
    }

    m_ui->noteListView->update();
}

void MainWindow::onDisableNativeMenuBarPreferenceChanged()
{
    QNDEBUG(
        "quentier::MainWindow",
        "MainWindow::onDisableNativeMenuBarPreferenceChanged");

    setupDisableNativeMenuBarPreference();
}

void MainWindow::onRunSyncEachNumMinitesPreferenceChanged(
    const int runSyncEachNumMinutes)
{
    QNDEBUG(
        "quentier::MainWindow",
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
            !m_account || (m_account->type() != Account::Type::Evernote)))
    {
        return;
    }

    m_runSyncPeriodicallyTimerId =
        startTimer(secondsToMilliseconds(runSyncEachNumMinutes * 60));
}

void MainWindow::onPanelFontColorChanged(QColor color)
{
    QNDEBUG(
        "quentier::MainWindow",
        "MainWindow::onPanelFontColorChanged: " << color.name());

    for (const auto & panelStyleController:
         std::as_const(m_genericPanelStyleControllers))
    {
        panelStyleController->setOverrideFontColor(color);
    }

    for (const auto & panelStyleController:
         std::as_const(m_sidePanelStyleControllers))
    {
        panelStyleController->setOverrideFontColor(color);
    }
}

void MainWindow::onPanelBackgroundColorChanged(QColor color)
{
    QNDEBUG(
        "quentier::MainWindow",
        "MainWindow::onPanelBackgroundColorChanged: " << color.name());

    if (Q_UNLIKELY(!m_account)) {
        QNDEBUG("quentier::MainWindow", "No current account");
        return;
    }

    ApplicationSettings settings{
        *m_account, preferences::keys::files::userInterface};

    settings.beginGroup(preferences::keys::panelColorsGroup);

    const bool useBackgroundGradient =
        settings.value(preferences::keys::panelUseBackgroundGradient).toBool();

    settings.endGroup();

    if (useBackgroundGradient) {
        QNDEBUG(
            "quentier::MainWindow",
            "Background gradient is used instead of solid color");
        return;
    }

    for (const auto & panelStyleController:
         std::as_const(m_genericPanelStyleControllers))
    {
        panelStyleController->setOverrideBackgroundColor(color);
    }

    for (const auto & panelStyleController:
         std::as_const(m_sidePanelStyleControllers))
    {
        panelStyleController->setOverrideBackgroundColor(color);
    }
}

void MainWindow::onPanelUseBackgroundGradientSettingChanged(
    const bool useBackgroundGradient)
{
    QNDEBUG(
        "quentier::MainWindow",
        "MainWindow::onPanelUseBackgroundGradientSettingChanged: "
            << (useBackgroundGradient ? "true" : "false"));

    restorePanelColors();
}

void MainWindow::onPanelBackgroundLinearGradientChanged(
    const QLinearGradient & gradient)
{
    QNDEBUG(
        "quentier::MainWindow",
        "MainWindow::onPanelBackgroundLinearGradientChanged");

    if (Q_UNLIKELY(!m_account)) {
        QNDEBUG("quentier::MainWindow", "No current account");
        return;
    }

    ApplicationSettings settings{
        *m_account, preferences::keys::files::userInterface};

    settings.beginGroup(preferences::keys::panelColorsGroup);

    const bool useBackgroundGradient =
        settings.value(preferences::keys::panelUseBackgroundGradient).toBool();

    settings.endGroup();

    if (!useBackgroundGradient) {
        QNDEBUG(
            "quentier::MainWindow",
            "Background color is used instead of " << "gradient");
        return;
    }

    for (const auto & panelStyleController:
         std::as_const(m_genericPanelStyleControllers))
    {
        panelStyleController->setOverrideBackgroundGradient(gradient);
    }

    for (const auto & panelStyleController:
         std::as_const(m_sidePanelStyleControllers))
    {
        panelStyleController->setOverrideBackgroundGradient(gradient);
    }
}

void MainWindow::onNewNoteRequestedFromSystemTrayIcon()
{
    QNDEBUG(
        "quentier::MainWindow",
        "MainWindow::onNewNoteRequestedFromSystemTrayIcon");

    const Qt::WindowStates state = windowState();
    const bool isMinimized = (state & Qt::WindowMinimized);
    const bool shown = !isMinimized && !isHidden();

    createNewNote(
        shown ? NoteEditorTabsAndWindowsCoordinator::NoteEditorMode::Any
              : NoteEditorTabsAndWindowsCoordinator::NoteEditorMode::Window);
}

void MainWindow::onQuitRequestedFromSystemTrayIcon()
{
    QNINFO(
        "quentier::MainWindow",
        "MainWindow::onQuitRequestedFromSystemTrayIcon");

    quitApp();
}

void MainWindow::onAccountSwitchRequested(const Account & account)
{
    QNDEBUG(
        "quentier::MainWindow",
        "MainWindow::onAccountSwitchRequested: " << account.name());

    stopListeningForSplitterMoves();
    m_accountManager->switchAccount(account);
}

void MainWindow::onSystemTrayIconManagerError(
    const ErrorString & errorDescription)
{
    QNDEBUG(
        "quentier::MainWindow",
        "MainWindow::onSystemTrayIconManagerError: " << errorDescription);

    onSetStatusBarText(
        errorDescription.localizedString(), secondsToMilliseconds(30));
}

void MainWindow::onShowRequestedFromTrayIcon()
{
    QNDEBUG("quentier::MainWindow", "MainWindow::onShowRequestedFromTrayIcon");
    show();
}

void MainWindow::onHideRequestedFromTrayIcon()
{
    QNDEBUG("quentier::MainWindow", "MainWindow::onHideRequestedFromTrayIcon");
    hide();
}

void MainWindow::onViewLogsActionTriggered()
{
    auto * logViewerWidget = findChild<LogViewerWidget *>();
    if (logViewerWidget) {
        return;
    }

    logViewerWidget = new LogViewerWidget{this};
    logViewerWidget->setAttribute(Qt::WA_DeleteOnClose);
    logViewerWidget->show();
}

void MainWindow::onShowInfoAboutQuentierActionTriggered()
{
    QNDEBUG(
        "quentier::MainWindow",
        "MainWindow::onShowInfoAboutQuentierActionTriggered");

    auto * widget = findChild<AboutQuentierWidget *>();
    if (widget) {
        widget->show();
        widget->raise();
        widget->setFocus();
        return;
    }

    widget = new AboutQuentierWidget{this};
    widget->setAttribute(Qt::WA_DeleteOnClose);
    centerWidget(*widget);
    widget->adjustSize();
    widget->show();
}

void MainWindow::onNoteEditorError(const ErrorString & error)
{
    QNINFO("quentier::MainWindow", "MainWindow::onNoteEditorError: " << error);
    onSetStatusBarText(error.localizedString(), secondsToMilliseconds(30));
}

void MainWindow::onModelViewError(const ErrorString & error)
{
    QNINFO("quentier::MainWindow", "MainWindow::onModelViewError: " << error);
    onSetStatusBarText(error.localizedString(), secondsToMilliseconds(30));
}

void MainWindow::onNoteEditorSpellCheckerNotReady()
{
    QNDEBUG(
        "quentier::MainWindow", "MainWindow::onNoteEditorSpellCheckerNotReady");

    auto * noteEditor = qobject_cast<NoteEditorWidget *>(sender());
    if (!noteEditor) {
        QNTRACE(
            "quentier::MainWindow",
            "Can't cast caller to note editor widget, skipping");
        return;
    }

    auto * currentEditor = currentNoteEditorTab();
    if (!currentEditor || (currentEditor != noteEditor)) {
        QNTRACE(
            "quentier::MainWindow",
            "Not an update from current note editor, skipping");
        return;
    }

    onSetStatusBarText(
        tr("Spell checker is loading dictionaries, please wait"));
}

void MainWindow::onNoteEditorSpellCheckerReady()
{
    QNDEBUG(
        "quentier::MainWindow", "MainWindow::onNoteEditorSpellCheckerReady");

    auto * noteEditor = qobject_cast<NoteEditorWidget *>(sender());
    if (!noteEditor) {
        QNTRACE(
            "quentier::MainWindow",
            "Can't cast caller to note editor widget, skipping");
        return;
    }

    auto * currentEditor = currentNoteEditorTab();
    if (!currentEditor || (currentEditor != noteEditor)) {
        QNTRACE(
            "quentier::MainWindow",
            "Not an update from current note editor, skipping");
        return;
    }

    onSetStatusBarText(QString());
}

void MainWindow::onAddAccountActionTriggered(const bool checked)
{
    QNDEBUG("quentier::MainWindow", "MainWindow::onAddAccountActionTriggered");
    Q_UNUSED(checked)
    onNewAccountCreationRequested();
}

void MainWindow::onManageAccountsActionTriggered(const bool checked)
{
    QNDEBUG(
        "quentier::MainWindow", "MainWindow::onManageAccountsActionTriggered");

    Q_UNUSED(checked)
    m_accountManager->execManageAccountsDialog();
}

void MainWindow::onSwitchAccountActionToggled(const bool checked)
{
    QNDEBUG(
        "quentier::MainWindow",
        "MainWindow::onSwitchAccountActionToggled: checked = "
            << (checked ? "true" : "false"));

    if (!checked) {
        QNTRACE("quentier::MainWindow", "Ignoring the unchecking of account");
        return;
    }

    auto * action = qobject_cast<QAction *>(sender());
    if (Q_UNLIKELY(!action)) {
        NOTIFY_ERROR(
            QT_TR_NOOP("Internal error: account switching action is "
                       "unexpectedly null"));
        return;
    }

    const auto indexData = action->data();
    bool conversionResult = false;
    const int index = indexData.toInt(&conversionResult);
    if (Q_UNLIKELY(!conversionResult)) {
        NOTIFY_ERROR(
            QT_TR_NOOP("Internal error: can't get identification data from "
                       "the account switching action"));
        return;
    }

    const auto & availableAccounts = m_accountManager->availableAccounts();
    Q_ASSERT(availableAccounts.size() <= std::numeric_limits<int>::max());
    const int numAvailableAccounts = static_cast<int>(availableAccounts.size());
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
    m_accountManager->switchAccount(availableAccount);

    // The continuation is in onAccountSwitched slot connected to
    // AccountManager's switchedAccount signal
}

void MainWindow::onAccountSwitched(Account account)
{
    QNDEBUG(
        "quentier::MainWindow",
        "MainWindow::onAccountSwitched: " << account.name());

    if (m_account && m_account->type() == Account::Type::Evernote) {
        clearSynchronizer();
    }

    clearModels();

    m_account = std::move(account);

    const auto localStoragePath = accountPersistentStoragePath(*m_account);
    m_localStorage =
        local_storage::createSqliteLocalStorage(*m_account, localStoragePath);
    Q_ASSERT(m_localStorage);

    ErrorString errorDescription;
    if (!checkLocalStorageVersion(*m_account, errorDescription)) {
        QNWARNING(
            "quentier::MainWindow",
            "Cannot switch account: " << errorDescription);
        quitApp(-1);
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

    setWindowTitleForAccount(*m_account);

    stopListeningForShortcutChanges();
    setupDefaultShortcuts();
    setupUserShortcuts();
    startListeningForShortcutChanges();

    restoreNetworkProxySettingsForAccount(*m_account);

    if (m_account->type() == Account::Type::Evernote) {
        m_synchronizationRemoteHost.clear();
        setupSynchronizer(SetAccountOption::Set);
    }

    setupModels();

    if (m_noteEditorTabsAndWindowsCoordinator) {
        m_noteEditorTabsAndWindowsCoordinator->switchAccount(
            *m_account, *m_tagModel);
    }

    m_ui->filterByNotebooksWidget->switchAccount(
        *m_account, m_notebookModel);

    m_ui->filterByTagsWidget->switchAccount(*m_account, m_tagModel);

    m_ui->filterBySavedSearchComboBox->switchAccount(
        *m_account, m_savedSearchModel);

    setupViews();
    setupAccountSpecificUiElements();

    // FIXME: this can be done more lightweight: just set the current account
    // in the already filled list
    updateSubMenuWithAvailableAccounts();

    restoreGeometryAndState();
    restorePanelColors();

    const bool wasPendingSwitchToNewEvernoteAccount =
        m_pendingSwitchToNewEvernoteAccount;

    m_pendingSwitchToNewEvernoteAccount = false;

    if (m_account->type() != Account::Type::Evernote) {
        QNDEBUG(
            "quentier::MainWindow",
            "Not an Evernote account, no need to bother setting up sync");
        return;
    }

    m_authenticatedCurrentEvernoteAccount = true;
    setupSynchronizer();

    if (wasPendingSwitchToNewEvernoteAccount) {
        // For new Evernote account is is convenient if the first note to be
        // synchronized automatically opens in the note editor
        m_ui->noteListView->setAutoSelectNoteOnNextAddition();

        launchSynchronization();
    }
}

void MainWindow::onAccountUpdated(Account account)
{
    QNDEBUG(
        "quentier::MainWindow",
        "MainWindow::onAccountUpdated: " << account.name());

    if (!m_account) {
        QNDEBUG("quentier::MainWindow", "No account is current at the moment");
        return;
    }

    if (m_account->type() != account.type()) {
        QNDEBUG(
            "quentier::MainWindow",
            "Not an update for the current account: it has another type");
        QNTRACE("quentier::MainWindow", *m_account);
        return;
    }

    const bool isLocal = (m_account->type() == Account::Type::Local);
    if (isLocal && (m_account->name() != account.name())) {
        QNDEBUG(
            "quentier::MainWindow",
            "Not an update for the current account: it has another name");
        QNTRACE("quentier::MainWindow", *m_account);
        return;
    }

    if (!isLocal &&
        (m_account->id() != account.id() ||
         m_account->name() != account.name()))
    {
        QNDEBUG(
            "quentier::MainWindow",
            "Not an update for the current account: either id or name don't "
            "match");
        QNTRACE("quentier::MainWindow", *m_account);
        return;
    }

    m_account = std::move(account);
    setWindowTitleForAccount(*m_account);
}

void MainWindow::onAccountAdded(const Account & account)
{
    QNDEBUG(
        "quentier::MainWindow",
        "MainWindow::onAccountAdded: " << account.name());

    updateSubMenuWithAvailableAccounts();
}

void MainWindow::onAccountRemoved(const Account & account)
{
    QNDEBUG(
        "quentier::MainWindow", "MainWindow::onAccountRemoved: " << account);

    updateSubMenuWithAvailableAccounts();
}

void MainWindow::onAccountManagerError(const ErrorString & errorDescription)
{
    QNDEBUG(
        "quentier::MainWindow",
        "MainWindow::onAccountManagerError: " << errorDescription);

    onSetStatusBarText(
        errorDescription.localizedString(), secondsToMilliseconds(30));
}

void MainWindow::onShowSidePanelActionToggled(const bool checked)
{
    QNDEBUG(
        "quentier::MainWindow",
        "MainWindow::onShowSidePanelActionToggled: checked = "
            << (checked ? "true" : "false"));

    Q_ASSERT(m_account);

    ApplicationSettings appSettings{
        *m_account, preferences::keys::files::userInterface};

    appSettings.beginGroup(QStringLiteral("MainWindow"));
    appSettings.setValue(QStringLiteral("ShowSidePanel"), checked);
    appSettings.endGroup();

    if (checked) {
        m_ui->sidePanelSplitter->show();
    }
    else {
        m_ui->sidePanelSplitter->hide();
    }
}

void MainWindow::onShowFavoritesActionToggled(const bool checked)
{
    QNDEBUG(
        "quentier::MainWindow",
        "MainWindow::onShowFavoritesActionToggled: checked = "
            << (checked ? "true" : "false"));

    Q_ASSERT(m_account);

    ApplicationSettings appSettings{
        *m_account, preferences::keys::files::userInterface};

    appSettings.beginGroup(QStringLiteral("MainWindow"));
    appSettings.setValue(QStringLiteral("ShowFavorites"), checked);
    appSettings.endGroup();

    if (checked) {
        m_ui->favoritesWidget->show();
    }
    else {
        m_ui->favoritesWidget->hide();
    }
}

void MainWindow::onShowNotebooksActionToggled(const bool checked)
{
    QNDEBUG(
        "quentier::MainWindow",
        "MainWindow::onShowNotebooksActionToggled: checked = "
            << (checked ? "true" : "false"));

    Q_ASSERT(m_account);

    ApplicationSettings appSettings{
        *m_account, preferences::keys::files::userInterface};

    appSettings.beginGroup(QStringLiteral("MainWindow"));
    appSettings.setValue(QStringLiteral("ShowNotebooks"), checked);
    appSettings.endGroup();

    if (checked) {
        m_ui->notebooksWidget->show();
    }
    else {
        m_ui->notebooksWidget->hide();
    }
}

void MainWindow::onShowTagsActionToggled(const bool checked)
{
    QNDEBUG(
        "quentier::MainWindow",
        "MainWindow::onShowTagsActionToggled: checked = "
            << (checked ? "true" : "false"));

    Q_ASSERT(m_account);

    ApplicationSettings appSettings{
        *m_account, preferences::keys::files::userInterface};

    appSettings.beginGroup(QStringLiteral("MainWindow"));
    appSettings.setValue(QStringLiteral("ShowTags"), checked);
    appSettings.endGroup();

    if (checked) {
        m_ui->tagsWidget->show();
    }
    else {
        m_ui->tagsWidget->hide();
    }
}

void MainWindow::onShowSavedSearchesActionToggled(const bool checked)
{
    QNDEBUG(
        "quentier::MainWindow",
        "MainWindow::onShowSavedSearchesActionToggled: checked = "
            << (checked ? "true" : "false"));

    Q_ASSERT(m_account);

    ApplicationSettings appSettings{
        *m_account, preferences::keys::files::userInterface};

    appSettings.beginGroup(QStringLiteral("MainWindow"));
    appSettings.setValue(QStringLiteral("ShowSavedSearches"), checked);
    appSettings.endGroup();

    if (checked) {
        m_ui->savedSearchesWidget->show();
    }
    else {
        m_ui->savedSearchesWidget->hide();
    }
}

void MainWindow::onShowDeletedNotesActionToggled(const bool checked)
{
    QNDEBUG(
        "quentier::MainWindow",
        "MainWindow::onShowDeletedNotesActionToggled: checked = "
            << (checked ? "true" : "false"));

    Q_ASSERT(m_account);

    ApplicationSettings appSettings{
        *m_account, preferences::keys::files::userInterface};

    appSettings.beginGroup(QStringLiteral("MainWindow"));
    appSettings.setValue(QStringLiteral("ShowDeletedNotes"), checked);
    appSettings.endGroup();

    if (checked) {
        m_ui->deletedNotesWidget->show();
    }
    else {
        m_ui->deletedNotesWidget->hide();
    }
}

void MainWindow::onShowNoteListActionToggled(const bool checked)
{
    QNDEBUG(
        "quentier::MainWindow",
        "MainWindow::onShowNoteListActionToggled: checked = "
            << (checked ? "true" : "false"));

    Q_ASSERT(m_account);

    ApplicationSettings appSettings{
        *m_account, preferences::keys::files::userInterface};

    appSettings.beginGroup(QStringLiteral("MainWindow"));
    appSettings.setValue(QStringLiteral("ShowNotesList"), checked);
    appSettings.endGroup();

    if (checked) {
        m_ui->noteListView->setModel(m_noteModel);
        m_ui->notesListAndFiltersFrame->show();
    }
    else {
        m_ui->notesListAndFiltersFrame->hide();
        m_ui->noteListView->setModel(&m_blankModel);
    }
}

void MainWindow::onShowToolbarActionToggled(const bool checked)
{
    QNDEBUG(
        "quentier::MainWindow",
        "MainWindow::onShowToolbarActionToggled: checked = "
            << (checked ? "true" : "false"));

    Q_ASSERT(m_account);

    ApplicationSettings appSettings{
        *m_account, preferences::keys::files::userInterface};

    appSettings.beginGroup(QStringLiteral("MainWindow"));
    appSettings.setValue(QStringLiteral("ShowToolbar"), checked);
    appSettings.endGroup();

    if (checked) {
        m_ui->upperBarGenericPanel->show();
    }
    else {
        m_ui->upperBarGenericPanel->hide();
    }
}

void MainWindow::onShowStatusBarActionToggled(const bool checked)
{
    QNDEBUG(
        "quentier::MainWindow",
        "MainWindow::onShowStatusBarActionToggled: checked = "
            << (checked ? "true" : "false"));

    ApplicationSettings appSettings(
        *m_account, preferences::keys::files::userInterface);

    appSettings.beginGroup(QStringLiteral("MainWindow"));
    appSettings.setValue(QStringLiteral("ShowStatusBar"), checked);
    appSettings.endGroup();

    if (checked) {
        m_ui->statusBar->show();
    }
    else {
        m_ui->statusBar->hide();
    }
}

void MainWindow::onSwitchIconTheme(const QString & iconTheme)
{
    QNDEBUG(
        "quentier::MainWindow", "MainWindow::onSwitchIconTheme: " << iconTheme);

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
        ErrorString error{QT_TR_NOOP("Unknown icon theme selected")};
        error.details() = iconTheme;
        QNWARNING("quentier::MainWindow", error);
        onSetStatusBarText(error.localizedString(), secondsToMilliseconds(30));
    }
}

void MainWindow::onSwitchIconThemeToNativeAction()
{
    QNDEBUG(
        "quentier::MainWindow", "MainWindow::onSwitchIconThemeToNativeAction");

    if (m_nativeIconThemeName.isEmpty()) {
        const ErrorString error{
            QT_TR_NOOP("No native icon theme is available")};
        QNDEBUG("quentier::MainWindow", error);
        onSetStatusBarText(error.localizedString(), secondsToMilliseconds(30));
        return;
    }

    if (QIcon::themeName() == m_nativeIconThemeName) {
        const ErrorString error{
            QT_TR_NOOP("Already using the native icon theme")};
        QNDEBUG("quentier::MainWindow", error);
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
        "quentier::MainWindow", "MainWindow::onSwitchIconThemeToTangoAction");

    const QString tango = QStringLiteral("tango");

    if (QIcon::themeName() == tango) {
        const ErrorString error{QT_TR_NOOP("Already using tango icon theme")};
        QNDEBUG("quentier::MainWindow", error);
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
        "quentier::MainWindow", "MainWindow::onSwitchIconThemeToOxygenAction");

    const QString oxygen = QStringLiteral("oxygen");

    if (QIcon::themeName() == oxygen) {
        const ErrorString error{QT_TR_NOOP("Already using oxygen icon theme")};
        QNDEBUG("quentier::MainWindow", error);
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
        "quentier::MainWindow", "MainWindow::onSwitchIconThemeToBreezeAction");

    const QString breeze = QStringLiteral("breeze");

    if (QIcon::themeName() == breeze) {
        const ErrorString error{QT_TR_NOOP("Already using breeze icon theme")};
        QNDEBUG("quentier::MainWindow", error);
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
        "quentier::MainWindow",
        "MainWindow::onSwitchIconThemeToBreezeDarkAction");

    const QString breezeDark = QStringLiteral("breeze-dark");

    if (QIcon::themeName() == breezeDark) {
        const ErrorString error{
            QT_TR_NOOP("Already using breeze-dark icon theme")};
        QNDEBUG("quentier::MainWindow", error);
        onSetStatusBarText(error.localizedString(), secondsToMilliseconds(10));
        return;
    }

    QIcon::setThemeName(breezeDark);
    persistChosenIconTheme(breezeDark);
    refreshChildWidgetsThemeIcons();
}

void MainWindow::onSplitterHandleMoved(const int pos, const int index)
{
    QNDEBUG(
        "quentier::MainWindow",
        "MainWindow::onSplitterHandleMoved: pos = " << pos
                                                    << ", index = " << index);

    scheduleGeometryAndStatePersisting();
}

void MainWindow::onSidePanelSplittedHandleMoved(const int pos, const int index)
{
    QNDEBUG(
        "quentier::MainWindow",
        "MainWindow::onSidePanelSplittedHandleMoved: pos = "
            << pos << ", index = " << index);

    scheduleGeometryAndStatePersisting();
}

void MainWindow::onSyncButtonPressed()
{
    QNDEBUG("quentier::MainWindow", "MainWindow::onSyncButtonPressed");

    if (Q_UNLIKELY(!m_account)) {
        QNDEBUG(
            "quentier::MainWindow",
            "Ignoring the sync button click - no account is set");
        return;
    }

    if (Q_UNLIKELY(m_account->type() == Account::Type::Local)) {
        QNDEBUG(
            "quentier::MainWindow",
            "The current account is of local type, won't do anything on "
            "attempt to sync it");
        return;
    }

    if (m_noteEditorTabsAndWindowsCoordinator) {
        m_noteEditorTabsAndWindowsCoordinator->saveAllNoteEditorsContents();
    }

    if (m_syncInProgress) {
        stopSynchronization();
        setupRunSyncPeriodicallyTimer();
    }
    else {
        launchSynchronization();
    }
}

void MainWindow::onAnimatedSyncIconFrameChanged(const int frame)
{
    Q_UNUSED(frame)

    m_ui->syncPushButton->setIcon(
        QIcon{m_animatedSyncButtonIcon.currentPixmap()});
}

void MainWindow::onAnimatedSyncIconFrameChangedPendingFinish(const int frame)
{
    if (frame == 0 || frame == (m_animatedSyncButtonIcon.frameCount() - 1)) {
        stopSyncButtonAnimation();
    }
}

void MainWindow::onSyncIconAnimationFinished()
{
    QNDEBUG("quentier::MainWindow", "MainWindow::onSyncIconAnimationFinished");

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
        "quentier::MainWindow", "MainWindow::onNewAccountCreationRequested");

    const int res = m_accountManager->execAddAccountDialog();
    if (res == QDialog::Accepted) {
        return;
    }

    if (Q_UNLIKELY(!m_localStorage)) {
        QNWARNING(
            "quentier::MainWindow",
            "Local storage is unexpectedly null, can't check local storage "
            "version");
        return;
    }

    ErrorString errorDescription;
    if (checkLocalStorageVersion(*m_account, errorDescription)) {
        QNWARNING(
            "quentier::MainWindow",
            errorDescription);
        onSetStatusBarText(errorDescription.localizedString(), 30);
        return;
    }
}

void MainWindow::onQuitAction()
{
    QNDEBUG("quentier::MainWindow", "MainWindow::onQuitAction");
    quitApp();
}

void MainWindow::onShortcutChanged(
    const int key, const QKeySequence & shortcut, const Account & account,
    const QString & context)
{
    QNDEBUG(
        "quentier::MainWindow",
        "MainWindow::onShortcutChanged: key = "
            << key
            << ", shortcut: " << shortcut.toString(QKeySequence::PortableText)
            << ", context: " << context << ", account: " << account.name());

    const auto it = m_shortcutKeyToAction.find(key);
    if (it == m_shortcutKeyToAction.end()) {
        QNDEBUG(
            "quentier::MainWindow",
            "Haven't found the action corresponding to the shortcut key");
        return;
    }

    auto * action = it.value();
    action->setShortcut(shortcut);

    QNDEBUG(
        "quentier::MainWindow",
        "Updated shortcut for action " << action->text() << " ("
                                       << action->objectName() << ")");
}

void MainWindow::onNonStandardShortcutChanged(
    const QString & nonStandardKey, const QKeySequence & shortcut,
    const Account & account, const QString & context)
{
    QNDEBUG(
        "quentier::MainWindow",
        "MainWindow::onNonStandardShortcutChanged: "
            << "non-standard key = " << nonStandardKey
            << ", shortcut: " << shortcut.toString(QKeySequence::PortableText)
            << ", context: " << context << ", account: " << account.name());

    const auto it = m_nonStandardShortcutKeyToAction.find(nonStandardKey);
    if (it == m_nonStandardShortcutKeyToAction.end()) {
        QNDEBUG(
            "quentier::MainWindow",
            "Haven't found the action "
                << "corresponding to the non-standard shortcut key");
        return;
    }

    auto * action = it.value();
    action->setShortcut(shortcut);

    QNDEBUG(
        "quentier::MainWindow",
        "Updated shortcut for action " << action->text() << " ("
                                       << action->objectName() << ")");
}

void MainWindow::onDefaultAccountFirstNotebookAndNoteCreatorFinished(
    const QString & createdNoteLocalId)
{
    QNDEBUG(
        "quentier::MainWindow",
        "MainWindow::onDefaultAccountFirstNotebookAndNoteCreatorFinished: "
            << "created note local id = " << createdNoteLocalId);

    auto * defaultAccountFirstNotebookAndNoteCreator =
        qobject_cast<DefaultAccountFirstNotebookAndNoteCreator *>(sender());

    if (defaultAccountFirstNotebookAndNoteCreator) {
        defaultAccountFirstNotebookAndNoteCreator->deleteLater();
    }

    bool foundNoteModelItem = false;
    if (m_noteModel) {
        const auto index = m_noteModel->indexForLocalId(createdNoteLocalId);
        if (index.isValid()) {
            foundNoteModelItem = true;
        }
    }

    if (foundNoteModelItem) {
        m_ui->noteListView->setCurrentNoteByLocalId(createdNoteLocalId);
        return;
    }

    // If we got here, the just created note hasn't got to the note model yet;
    // in the ideal world should subscribe to note model's insert signal
    // but as a shortcut will just introduce a small delay in the hope the note
    // would have enough time to get from local storage into the model
    m_defaultAccountFirstNoteLocalId = createdNoteLocalId;
    m_setDefaultAccountsFirstNoteAsCurrentDelayTimerId = startTimer(100);
}

void MainWindow::onDefaultAccountFirstNotebookAndNoteCreatorError(
    const ErrorString & errorDescription)
{
    QNDEBUG(
        "quentier::MainWindow",
        "MainWindow::onDefaultAccountFirstNotebookAndNoteCreatorError: "
            << errorDescription);

    onSetStatusBarText(errorDescription.localizedString());

    auto * defaultAccountFirstNotebookAndNoteCreator =
        qobject_cast<DefaultAccountFirstNotebookAndNoteCreator *>(sender());

    if (defaultAccountFirstNotebookAndNoteCreator) {
        defaultAccountFirstNotebookAndNoteCreator->deleteLater();
    }
}

#ifdef WITH_UPDATE_MANAGER
void MainWindow::onCheckForUpdatesActionTriggered()
{
    QNDEBUG(
        "quentier::MainWindow", "MainWindow::onCheckForUpdatesActionTriggered");

    m_updateManager->checkForUpdates();
}

void MainWindow::onUpdateManagerError(const ErrorString & errorDescription)
{
    QNDEBUG(
        "quentier::MainWindow",
        "MainWindow::onUpdateManagerError: " << errorDescription);

    onSetStatusBarText(errorDescription.localizedString());
}

void MainWindow::onUpdateManagerRequestsRestart()
{
    QNDEBUG(
        "quentier::MainWindow", "MainWindow::onUpdateManagerRequestsRestart");

    quitApp(gRestartExitCode);
}
#endif // WITH_UPDATE_MANAGER

void MainWindow::resizeEvent(QResizeEvent * event)
{
    QMainWindow::resizeEvent(event);

    if (m_ui->filterBodyFrame->isHidden()) {
        // NOTE: without this the splitter seems to take a wrong guess about
        // the size of note filters header panel and that doesn't look good
        adjustNoteListAndFiltersSplitterSizes();
    }

    scheduleGeometryAndStatePersisting();
}

void MainWindow::closeEvent(QCloseEvent * event)
{
    QNDEBUG("quentier::MainWindow", "MainWindow::closeEvent");

    if (m_pendingFirstShutdownDialog) {
        QNDEBUG("quentier::MainWindow", "About to display FirstShutdownDialog");
        m_pendingFirstShutdownDialog = false;

        auto dialog = std::make_unique<FirstShutdownDialog>(this);
        dialog->setWindowModality(Qt::WindowModal);
        centerDialog(*dialog);
        const bool shouldCloseToSystemTray =
            (dialog->exec() == QDialog::Accepted);

        m_systemTrayIconManager->setPreferenceCloseToSystemTray(
            shouldCloseToSystemTray);
    }

    if (event && m_systemTrayIconManager->shouldCloseToSystemTray()) {
        QNINFO(
            "quentier::MainWindow", "Hiding to system tray instead of closing");
        event->ignore();
        hide();
        return;
    }

    if (m_noteEditorTabsAndWindowsCoordinator) {
        m_noteEditorTabsAndWindowsCoordinator->clear();
    }

    persistGeometryAndState();
    QNINFO("quentier::MainWindow", "Closing application");
    QMainWindow::closeEvent(event);
    quitApp();
}

void MainWindow::timerEvent(QTimerEvent * timerEvent)
{
    QNDEBUG(
        "quentier::MainWindow",
        "MainWindow::timerEvent: timer id = "
            << (timerEvent ? QString::number(timerEvent->timerId())
                           : QStringLiteral("<null>")));

    if (Q_UNLIKELY(!timerEvent)) {
        return;
    }

    if (timerEvent->timerId() == m_geometryAndStatePersistingDelayTimerId) {
        persistGeometryAndState();
        killTimer(m_geometryAndStatePersistingDelayTimerId);
        m_geometryAndStatePersistingDelayTimerId = 0;
    }
    else if (timerEvent->timerId() == m_splitterSizesRestorationDelayTimerId) {
        restoreSplitterSizes();
        startListeningForSplitterMoves();
        killTimer(m_splitterSizesRestorationDelayTimerId);
        m_splitterSizesRestorationDelayTimerId = 0;
    }
    else if (timerEvent->timerId() == m_runSyncPeriodicallyTimerId) {
        if (Q_UNLIKELY(
                !m_account || (m_account->type() != Account::Type::Evernote) ||
                !m_synchronizer))
        {
            QNDEBUG(
                "quentier::MainWindow",
                "Non-Evernote account is being used: "
                    << (m_account ? m_account->toString()
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
                "quentier::MainWindow",
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

        QNDEBUG("quentier::MainWindow", "Starting the periodically run sync");
        launchSynchronization();
    }
    else if (
        timerEvent->timerId() ==
        m_setDefaultAccountsFirstNoteAsCurrentDelayTimerId)
    {
        QNDEBUG(
            "quentier::MainWindow",
            "Executing postponed setting of defaut account's first note as the "
            "current note");

        m_ui->noteListView->setCurrentNoteByLocalId(
            m_defaultAccountFirstNoteLocalId);

        m_defaultAccountFirstNoteLocalId.clear();
        killTimer(m_setDefaultAccountsFirstNoteAsCurrentDelayTimerId);
        m_setDefaultAccountsFirstNoteAsCurrentDelayTimerId = 0;
    }
}

void MainWindow::focusInEvent(QFocusEvent * focusEvent)
{
    QNDEBUG("quentier::MainWindow", "MainWindow::focusInEvent");

    if (Q_UNLIKELY(!focusEvent)) {
        return;
    }

    QNDEBUG("quentier::MainWindow", "Reason = " << focusEvent->reason());

    QMainWindow::focusInEvent(focusEvent);

    NoteEditorWidget * noteEditorTab = currentNoteEditorTab();
    if (noteEditorTab) {
        noteEditorTab->setFocusToEditor();
    }
}

void MainWindow::focusOutEvent(QFocusEvent * focusEvent)
{
    QNDEBUG("quentier::MainWindow", "MainWindow::focusOutEvent");

    if (Q_UNLIKELY(!focusEvent)) {
        return;
    }

    QNDEBUG("quentier::MainWindow", "Reason = " << focusEvent->reason());
    QMainWindow::focusOutEvent(focusEvent);
}

void MainWindow::showEvent(QShowEvent * showEvent)
{
    QNDEBUG("quentier::MainWindow", "MainWindow::showEvent");
    QMainWindow::showEvent(showEvent);

    const Qt::WindowStates state = windowState();
    if (!(state & Qt::WindowMinimized)) {
        Q_EMIT shown();
    }
}

void MainWindow::hideEvent(QHideEvent * hideEvent)
{
    QNDEBUG("quentier::MainWindow", "MainWindow::hideEvent");

    QMainWindow::hideEvent(hideEvent);
    Q_EMIT hidden();
}

void MainWindow::changeEvent(QEvent * event)
{
    QMainWindow::changeEvent(event);

    if (event && (event->type() == QEvent::WindowStateChange)) {
        Qt::WindowStates state = windowState();
        bool minimized = (state & Qt::WindowMinimized);
        bool maximized = (state & Qt::WindowMaximized);

        QNDEBUG(
            "quentier::MainWindow",
            "Change event of window state change type: "
                << "minimized = " << (minimized ? "true" : "false")
                << ", maximized = " << (maximized ? "true" : "false"));

        if (!minimized) {
            QNDEBUG(
                "quentier::MainWindow", "MainWindow is no longer minimized");

            if (isVisible()) {
                Q_EMIT shown();
            }

            scheduleGeometryAndStatePersisting();
        }
        else if (minimized) {
            QNDEBUG("quentier::MainWindow", "MainWindow became minimized");

            if (m_systemTrayIconManager->shouldMinimizeToSystemTray()) {
                QNDEBUG(
                    "quentier::MainWindow",
                    "Should minimize to system tray instead of the "
                    "conventional minimization");

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
        const int x = center.x() - widgetGeometryRect.width() / 2;
        const int y = center.y() - widgetGeometryRect.height() / 2;
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
    QNTRACE("quentier::MainWindow", "MainWindow::setupThemeIcons");

    m_nativeIconThemeName = QIcon::themeName();

    QNDEBUG(
        "quentier::MainWindow",
        "Native icon theme name: " << m_nativeIconThemeName);

    if (!QIcon::hasThemeIcon(QStringLiteral("document-new"))) {
        QNDEBUG(
            "quentier::MainWindow",
            "There seems to be no native icon theme available: document-new "
            "icon is not present within the theme");
        m_nativeIconThemeName.clear();
    }

    ApplicationSettings appSettings{
        *m_account, preferences::keys::files::userInterface};

    appSettings.beginGroup(preferences::keys::appearanceGroup);

    const QString lastUsedIconThemeName =
        appSettings.value(preferences::keys::iconTheme).toString();

    appSettings.endGroup();

    QString iconThemeName;
    if (!lastUsedIconThemeName.isEmpty()) {
        QNDEBUG(
            "quentier::MainWindow",
            "Last chosen icon theme: " << lastUsedIconThemeName);

        iconThemeName = lastUsedIconThemeName;
    }
    else {
        QNDEBUG("quentier::MainWindow", "No last chosen icon theme");

        if (!m_nativeIconThemeName.isEmpty()) {
            QNDEBUG(
                "quentier::MainWindow",
                "Using native icon theme: " << m_nativeIconThemeName);
            iconThemeName = m_nativeIconThemeName;
        }
        else {
            iconThemeName = fallbackIconThemeName();
            QNDEBUG(
                "quentier::MainWindow",
                "Using fallback icon theme: " << iconThemeName);
        }
    }

    QIcon::setThemeName(iconThemeName);
}

void MainWindow::setupAccountManager()
{
    QNDEBUG("quentier::MainWindow", "MainWindow::setupAccountManager");

    QObject::connect(
        m_accountManager,
        &AccountManager::evernoteAccountAuthenticationRequested, this,
        &MainWindow::onEvernoteAccountAuthenticationRequested);

    QObject::connect(
        m_accountManager, &AccountManager::switchedAccount, this,
        &MainWindow::onAccountSwitched);

    QObject::connect(
        m_accountManager, &AccountManager::accountUpdated, this,
        &MainWindow::onAccountUpdated);

    QObject::connect(
        m_accountManager, &AccountManager::accountAdded, this,
        &MainWindow::onAccountAdded);

    QObject::connect(
        m_accountManager, &AccountManager::accountRemoved, this,
        &MainWindow::onAccountRemoved);

    QObject::connect(
        m_accountManager, &AccountManager::notifyError, this,
        &MainWindow::onAccountManagerError);
}

void MainWindow::setupLocalStorage()
{
    QNDEBUG("quentier::MainWindow", "MainWindow::setupLocalStorage");

    Q_ASSERT(m_account);

    const auto localStoragePath = accountPersistentStoragePath(*m_account);
    m_localStorage =
        local_storage::createSqliteLocalStorage(*m_account, localStoragePath);
    Q_ASSERT(m_localStorage);

    QNDEBUG(
        "quentier::MainWindow",
        "Checking local storage version for account " << *m_account);

    auto isVersionTooHighFuture = m_localStorage->isVersionTooHigh();
    isVersionTooHighFuture.waitForFinished();
    const bool isVersionTooHigh = isVersionTooHighFuture.result();

    if (isVersionTooHigh) {
        throw LocalStorageVersionTooHighException(ErrorString{QT_TR_NOOP(
            "Local storage was created by newer version of Quentier")});
    }

    QNDEBUG(
        "quentier::MainWindow",
        "Checking if any patches are required for local storage of account "
            << *m_account);

    auto patchesFuture = m_localStorage->requiredPatches();
    patchesFuture.waitForFinished();

    const auto patches = patchesFuture.result();
    if (!patches.isEmpty()) {
        QNINFO(
            "quentier::MainWindow",
            "Local storage requires upgrade: found " << patches.size()
                << " required patches");

        auto upgradeDialog = std::make_unique<LocalStorageUpgradeDialog>(
            *m_account, m_accountManager->accountModel(), std::move(patches),
            LocalStorageUpgradeDialog::Options{}, this);

        QObject::connect(
            upgradeDialog.get(), &LocalStorageUpgradeDialog::shouldQuitApp,
            this, &MainWindow::onQuitAction,
            Qt::ConnectionType(Qt::UniqueConnection | Qt::QueuedConnection));

        upgradeDialog->adjustSize();
        upgradeDialog->exec();
    }
}

void MainWindow::setupDisableNativeMenuBarPreference()
{
    QNDEBUG(
        "quentier::MainWindow",
        "MainWindow::setupDisableNativeMenuBarPreference");

    const bool disableNativeMenuBar = getDisableNativeMenuBarPreference();
    m_ui->menuBar->setNativeMenuBar(!disableNativeMenuBar);

    if (disableNativeMenuBar) {
        // Without this the menu bar forcefully integrated into the main window
        // looks kinda ugly
        m_ui->menuBar->setStyleSheet(QString());
    }
}

void MainWindow::setupDefaultAccount()
{
    QNDEBUG("quentier::MainWindow", "MainWindow::setupDefaultAccount");

    auto * defaultAccountFirstNotebookAndNoteCreator =
        new DefaultAccountFirstNotebookAndNoteCreator(
            m_localStorage, *m_noteFiltersManager, this);

    QObject::connect(
        defaultAccountFirstNotebookAndNoteCreator,
        &DefaultAccountFirstNotebookAndNoteCreator::finished, this,
        &MainWindow::onDefaultAccountFirstNotebookAndNoteCreatorFinished);

    QObject::connect(
        defaultAccountFirstNotebookAndNoteCreator,
        &DefaultAccountFirstNotebookAndNoteCreator::notifyError, this,
        &MainWindow::onDefaultAccountFirstNotebookAndNoteCreatorError);

    defaultAccountFirstNotebookAndNoteCreator->start();
}

void MainWindow::setupModels()
{
    QNDEBUG("quentier::MainWindow", "MainWindow::setupModels");

    clearModels();

    auto noteSortingMode = restoreNoteSortingMode();
    if (noteSortingMode == NoteModel::NoteSortingMode::None) {
        noteSortingMode = NoteModel::NoteSortingMode::ModifiedDescending;
    }

    m_noteModel = new NoteModel(
        *m_account, m_localStorage, m_noteCache, m_notebookCache, this,
        NoteModel::IncludedNotes::NonDeleted, noteSortingMode);

    m_favoritesModel = new FavoritesModel(
        *m_account, m_localStorage, m_noteCache, m_notebookCache, m_tagCache,
        m_savedSearchCache, this);

    m_notebookModel =
        new NotebookModel(*m_account, m_localStorage, m_notebookCache, this);

    m_tagModel = new TagModel(*m_account, m_localStorage, m_tagCache, this);

    m_savedSearchModel = new SavedSearchModel(
        *m_account, m_localStorage, m_savedSearchCache, this);

    m_deletedNotesModel = new NoteModel(
        *m_account, m_localStorage, m_noteCache, m_notebookCache, this,
        NoteModel::IncludedNotes::Deleted);

    m_deletedNotesModel->start();

    if (m_noteCountLabelController == nullptr) {
        m_noteCountLabelController =
            new NoteCountLabelController{*m_ui->notesCountLabelPanel, this};
    }

    m_noteCountLabelController->setNoteModel(*m_noteModel);

    setupNoteFilters();

    m_ui->favoritesTableView->setModel(m_favoritesModel);
    m_ui->notebooksTreeView->setModel(m_notebookModel);
    m_ui->tagsTreeView->setModel(m_tagModel);
    m_ui->savedSearchesItemView->setModel(m_savedSearchModel);
    m_ui->deletedNotesTableView->setModel(m_deletedNotesModel);
    m_ui->noteListView->setModel(m_noteModel);

    m_notebookModelColumnChangeRerouter->setModel(m_notebookModel);
    m_tagModelColumnChangeRerouter->setModel(m_tagModel);
    m_noteModelColumnChangeRerouter->setModel(m_noteModel);
    m_favoritesModelColumnChangeRerouter->setModel(m_favoritesModel);

    if (m_editNoteDialogsManager) {
        m_editNoteDialogsManager->setNotebookModel(m_notebookModel);
    }
}

void MainWindow::clearModels()
{
    QNDEBUG("quentier::MainWindow", "MainWindow::clearModels");

    clearViews();

    if (m_notebookModel) {
        m_notebookModel->stop(IStartable::StopMode::Forced);
        delete m_notebookModel;
        m_notebookModel = nullptr;
    }

    if (m_tagModel) {
        m_tagModel->stop(IStartable::StopMode::Forced);
        delete m_tagModel;
        m_tagModel = nullptr;
    }

    if (m_savedSearchModel) {
        m_savedSearchModel->stop(IStartable::StopMode::Forced);
        delete m_savedSearchModel;
        m_savedSearchModel = nullptr;
    }

    if (m_noteModel) {
        m_noteModel->stop(IStartable::StopMode::Forced);
        delete m_noteModel;
        m_noteModel = nullptr;
    }

    if (m_deletedNotesModel) {
        m_deletedNotesModel->stop(IStartable::StopMode::Forced);
        delete m_deletedNotesModel;
        m_deletedNotesModel = nullptr;
    }

    if (m_favoritesModel) {
        m_favoritesModel->stop(IStartable::StopMode::Forced);
        delete m_favoritesModel;
        m_favoritesModel = nullptr;
    }
}

void MainWindow::setupShowHideStartupSettings()
{
    QNDEBUG("quentier::MainWindow", "MainWindow::setupShowHideStartupSettings");

    ApplicationSettings appSettings{
        *m_account, preferences::keys::files::userInterface};

    appSettings.beginGroup(QStringLiteral("MainWindow"));
    ApplicationSettings::GroupCloser groupCloser{appSettings};

    const auto checkAndSetShowSetting = [&](const std::string_view settingName,
                                            QAction & action,
                                            QWidget & widget) {
        auto showSetting = appSettings.value(settingName);
        if (showSetting.isNull()) {
            showSetting = action.isChecked();
        }

        const bool shouldShow = showSetting.toBool();
        if (shouldShow) {
            widget.show();
        }
        else {
            widget.hide();
        }

        action.setChecked(shouldShow);
    };

    checkAndSetShowSetting(
        "ShowSidePanel"sv, *m_ui->ActionShowSidePanel,
        *m_ui->sidePanelSplitter);

    checkAndSetShowSetting(
        "ShowFavorites"sv, *m_ui->ActionShowFavorites, *m_ui->favoritesWidget);

    checkAndSetShowSetting(
        "ShowNotebooks"sv, *m_ui->ActionShowNotebooks, *m_ui->notebooksWidget);

    checkAndSetShowSetting(
        "ShowTags"sv, *m_ui->ActionShowTags, *m_ui->tagsWidget);

    checkAndSetShowSetting(
        "ShowSavedSearches"sv, *m_ui->ActionShowSavedSearches,
        *m_ui->savedSearchesWidget);

    checkAndSetShowSetting(
        "ShowDeletedNotes"sv, *m_ui->ActionShowDeletedNotes,
        *m_ui->deletedNotesWidget);

    checkAndSetShowSetting(
        "ShowNotesList"sv, *m_ui->ActionShowNotesList,
        *m_ui->notesListAndFiltersFrame);

    checkAndSetShowSetting(
        "ShowToolbar"sv, *m_ui->ActionShowToolbar, *m_ui->upperBarGenericPanel);

    checkAndSetShowSetting(
        "ShowStatusBar"sv, *m_ui->ActionShowStatusBar,
        *m_ui->upperBarGenericPanel);
}

void MainWindow::setupViews()
{
    QNDEBUG("quentier::MainWindow", "MainWindow::setupViews");

    // NOTE: only a few columns would be shown for each view because otherwise
    // there are problems finding space for everything
    // TODO: in future should implement persistent setting of which columns
    // to show or not to show

    auto * favoritesTableView = m_ui->favoritesTableView;
    favoritesTableView->setNoteFiltersManager(*m_noteFiltersManager);

    auto * previousFavoriteItemDelegate = favoritesTableView->itemDelegate();

    auto * favoriteItemDelegate =
        qobject_cast<FavoriteItemDelegate *>(previousFavoriteItemDelegate);

    if (!favoriteItemDelegate) {
        favoriteItemDelegate = new FavoriteItemDelegate(favoritesTableView);
        favoritesTableView->setItemDelegate(favoriteItemDelegate);

        if (previousFavoriteItemDelegate) {
            previousFavoriteItemDelegate->deleteLater();
            previousFavoriteItemDelegate = nullptr;
        }
    }

    // This column's values would be displayed along with the favorite item's
    // name
    favoritesTableView->setColumnHidden(
        static_cast<int>(FavoritesModel::Column::NoteCount), true);

    favoritesTableView->setColumnWidth(
        static_cast<int>(FavoritesModel::Column::Type),
        favoriteItemDelegate->sideSize());

    QObject::connect(
        m_favoritesModelColumnChangeRerouter,
        &ColumnChangeRerouter::dataChanged, favoritesTableView,
        &FavoriteItemView::dataChanged, Qt::UniqueConnection);

    favoritesTableView->header()->setSectionResizeMode(
        QHeaderView::ResizeToContents);

    QObject::connect(
        favoritesTableView, &FavoriteItemView::notifyError, this,
        &MainWindow::onModelViewError, Qt::UniqueConnection);

    QObject::connect(
        favoritesTableView, &FavoriteItemView::favoritedItemInfoRequested, this,
        &MainWindow::onFavoritedItemInfoButtonPressed, Qt::UniqueConnection);

    QObject::connect(
        favoritesTableView, &FavoriteItemView::favoritedNoteSelected, this,
        &MainWindow::onFavoritedNoteSelected, Qt::UniqueConnection);

    auto * notebooksTreeView = m_ui->notebooksTreeView;
    notebooksTreeView->setNoteFiltersManager(*m_noteFiltersManager);
    notebooksTreeView->setNoteModel(m_noteModel);

    auto * previousNotebookItemDelegate = notebooksTreeView->itemDelegate();

    auto * notebookItemDelegate =
        qobject_cast<NotebookItemDelegate *>(previousNotebookItemDelegate);

    if (!notebookItemDelegate) {
        notebookItemDelegate = new NotebookItemDelegate(notebooksTreeView);
        notebooksTreeView->setItemDelegate(notebookItemDelegate);

        if (previousNotebookItemDelegate) {
            previousNotebookItemDelegate->deleteLater();
            previousNotebookItemDelegate = nullptr;
        }
    }

    // This column's values would be displayed along with the notebook's name
    notebooksTreeView->setColumnHidden(
        static_cast<int>(NotebookModel::Column::NoteCount), true);

    notebooksTreeView->setColumnHidden(
        static_cast<int>(NotebookModel::Column::Synchronizable), true);

    notebooksTreeView->setColumnHidden(
        static_cast<int>(NotebookModel::Column::FromLinkedNotebook), true);

    auto * previousNotebookDirtyColumnDelegate =
        notebooksTreeView->itemDelegateForColumn(
            static_cast<int>(NotebookModel::Column::Dirty));

    auto * notebookDirtyColumnDelegate = qobject_cast<DirtyColumnDelegate *>(
        previousNotebookDirtyColumnDelegate);

    if (!notebookDirtyColumnDelegate) {
        notebookDirtyColumnDelegate =
            new DirtyColumnDelegate(notebooksTreeView);

        notebooksTreeView->setItemDelegateForColumn(
            static_cast<int>(NotebookModel::Column::Dirty),
            notebookDirtyColumnDelegate);

        if (previousNotebookDirtyColumnDelegate) {
            previousNotebookDirtyColumnDelegate->deleteLater();
            previousNotebookDirtyColumnDelegate = nullptr;
        }
    }

    notebooksTreeView->setColumnWidth(
        static_cast<int>(NotebookModel::Column::Dirty),
        notebookDirtyColumnDelegate->sideSize());

    notebooksTreeView->header()->setSectionResizeMode(
        QHeaderView::ResizeToContents);

    QObject::connect(
        m_notebookModelColumnChangeRerouter, &ColumnChangeRerouter::dataChanged,
        notebooksTreeView, &NotebookItemView::dataChanged,
        Qt::UniqueConnection);

    QObject::connect(
        notebooksTreeView, &NotebookItemView::newNotebookCreationRequested,
        this, &MainWindow::onNewNotebookCreationRequested,
        Qt::UniqueConnection);

    QObject::connect(
        notebooksTreeView, &NotebookItemView::notebookInfoRequested, this,
        &MainWindow::onNotebookInfoButtonPressed, Qt::UniqueConnection);

    QObject::connect(
        notebooksTreeView, &NotebookItemView::notifyError, this,
        &MainWindow::onModelViewError, Qt::UniqueConnection);

    auto * tagsTreeView = m_ui->tagsTreeView;
    tagsTreeView->setNoteFiltersManager(*m_noteFiltersManager);

    // These columns' values would be displayed along with the tag's name
    tagsTreeView->setColumnHidden(
        static_cast<int>(TagModel::Column::NoteCount), true);

    tagsTreeView->setColumnHidden(
        static_cast<int>(TagModel::Column::Synchronizable), true);

    tagsTreeView->setColumnHidden(
        static_cast<int>(TagModel::Column::FromLinkedNotebook), true);

    auto * previousTagDirtyColumnDelegate = tagsTreeView->itemDelegateForColumn(
        static_cast<int>(TagModel::Column::Dirty));

    auto * tagDirtyColumnDelegate =
        qobject_cast<DirtyColumnDelegate *>(previousTagDirtyColumnDelegate);

    if (!tagDirtyColumnDelegate) {
        tagDirtyColumnDelegate = new DirtyColumnDelegate(tagsTreeView);

        tagsTreeView->setItemDelegateForColumn(
            static_cast<int>(TagModel::Column::Dirty), tagDirtyColumnDelegate);

        if (previousTagDirtyColumnDelegate) {
            previousTagDirtyColumnDelegate->deleteLater();
            previousTagDirtyColumnDelegate = nullptr;
        }
    }

    tagsTreeView->setColumnWidth(
        static_cast<int>(TagModel::Column::Dirty),
        tagDirtyColumnDelegate->sideSize());

    auto * previousTagItemDelegate = tagsTreeView->itemDelegateForColumn(
        static_cast<int>(TagModel::Column::Name));

    auto * tagItemDelegate =
        qobject_cast<TagItemDelegate *>(previousTagItemDelegate);

    if (!tagItemDelegate) {
        tagItemDelegate = new TagItemDelegate(tagsTreeView);

        tagsTreeView->setItemDelegateForColumn(
            static_cast<int>(TagModel::Column::Name), tagItemDelegate);

        if (previousTagItemDelegate) {
            previousTagItemDelegate->deleteLater();
            previousTagItemDelegate = nullptr;
        }
    }

    tagsTreeView->header()->setSectionResizeMode(QHeaderView::ResizeToContents);

    QObject::connect(
        m_tagModelColumnChangeRerouter, &ColumnChangeRerouter::dataChanged,
        tagsTreeView, &TagItemView::dataChanged, Qt::UniqueConnection);

    QObject::connect(
        tagsTreeView, &TagItemView::newTagCreationRequested, this,
        &MainWindow::onNewTagCreationRequested, Qt::UniqueConnection);

    QObject::connect(
        tagsTreeView, &TagItemView::tagInfoRequested, this,
        &MainWindow::onTagInfoButtonPressed, Qt::UniqueConnection);

    QObject::connect(
        tagsTreeView, &TagItemView::notifyError, this,
        &MainWindow::onModelViewError, Qt::UniqueConnection);

    auto * savedSearchesItemView = m_ui->savedSearchesItemView;
    savedSearchesItemView->setNoteFiltersManager(*m_noteFiltersManager);

    savedSearchesItemView->setColumnHidden(
        static_cast<int>(SavedSearchModel::Column::Query), true);

    savedSearchesItemView->setColumnHidden(
        static_cast<int>(SavedSearchModel::Column::Synchronizable), true);

    auto * previousSavedSearchDirtyColumnDelegate =
        savedSearchesItemView->itemDelegateForColumn(
            static_cast<int>(SavedSearchModel::Column::Dirty));

    auto * savedSearchDirtyColumnDelegate = qobject_cast<DirtyColumnDelegate *>(
        previousSavedSearchDirtyColumnDelegate);

    if (!savedSearchDirtyColumnDelegate) {
        savedSearchDirtyColumnDelegate =
            new DirtyColumnDelegate(savedSearchesItemView);

        savedSearchesItemView->setItemDelegateForColumn(
            static_cast<int>(SavedSearchModel::Column::Dirty),
            savedSearchDirtyColumnDelegate);

        if (previousSavedSearchDirtyColumnDelegate) {
            previousSavedSearchDirtyColumnDelegate->deleteLater();
            previousSavedSearchDirtyColumnDelegate = nullptr;
        }
    }

    savedSearchesItemView->setColumnWidth(
        static_cast<int>(SavedSearchModel::Column::Dirty),
        savedSearchDirtyColumnDelegate->sideSize());

    savedSearchesItemView->header()->setSectionResizeMode(
        QHeaderView::ResizeToContents);

    QObject::connect(
        savedSearchesItemView, &SavedSearchItemView::savedSearchInfoRequested,
        this, &MainWindow::onSavedSearchInfoButtonPressed,
        Qt::UniqueConnection);

    QObject::connect(
        savedSearchesItemView,
        &SavedSearchItemView::newSavedSearchCreationRequested, this,
        &MainWindow::onNewSavedSearchCreationRequested, Qt::UniqueConnection);

    QObject::connect(
        savedSearchesItemView, &SavedSearchItemView::notifyError, this,
        &MainWindow::onModelViewError, Qt::UniqueConnection);

    auto * noteListView = m_ui->noteListView;
    if (m_account) {
        noteListView->setCurrentAccount(*m_account);
    }

    auto * previousNoteItemDelegate = noteListView->itemDelegate();

    auto * noteItemDelegate =
        qobject_cast<NoteItemDelegate *>(previousNoteItemDelegate);

    if (!noteItemDelegate) {
        noteItemDelegate = new NoteItemDelegate{noteListView};
        noteListView->setModelColumn(
            static_cast<int>(NoteModel::Column::Title));
        noteListView->setItemDelegate(noteItemDelegate);

        if (previousNoteItemDelegate) {
            previousNoteItemDelegate->deleteLater();
            previousNoteItemDelegate = nullptr;
        }
    }

    noteListView->setNotebookItemView(notebooksTreeView);

    QObject::connect(
        m_noteModelColumnChangeRerouter, &ColumnChangeRerouter::dataChanged,
        noteListView, &NoteListView::dataChanged, Qt::UniqueConnection);

    QObject::connect(
        noteListView, &NoteListView::currentNoteChanged, this,
        &MainWindow::onCurrentNoteInListChanged, Qt::UniqueConnection);

    QObject::connect(
        noteListView, &NoteListView::openNoteInSeparateWindowRequested, this,
        &MainWindow::onOpenNoteInSeparateWindow, Qt::UniqueConnection);

    QObject::connect(
        noteListView, &NoteListView::enexExportRequested, this,
        &MainWindow::onExportNotesToEnexRequested, Qt::UniqueConnection);

    QObject::connect(
        noteListView, &NoteListView::newNoteCreationRequested, this,
        &MainWindow::onNewNoteCreationRequested, Qt::UniqueConnection);

    QObject::connect(
        noteListView, &NoteListView::copyInAppNoteLinkRequested, this,
        &MainWindow::onCopyInAppLinkNoteRequested, Qt::UniqueConnection);

    QObject::connect(
        noteListView, &NoteListView::toggleThumbnailsPreference, this,
        &MainWindow::onToggleThumbnailsPreference, Qt::UniqueConnection);

    QObject::connect(
        this, &MainWindow::showNoteThumbnailsStateChanged, noteListView,
        &NoteListView::setShowNoteThumbnailsState, Qt::UniqueConnection);

    QObject::connect(
        this, &MainWindow::showNoteThumbnailsStateChanged, noteItemDelegate,
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

        auto * noteSortingModeModel = new QStringListModel{this};
        noteSortingModeModel->setStringList(noteSortingModes);

        m_ui->noteSortingModeComboBox->setModel(noteSortingModeModel);
        m_onceSetupNoteSortingModeComboBox = true;
    }

    auto noteSortingMode = restoreNoteSortingMode();
    if (noteSortingMode == NoteModel::NoteSortingMode::None) {
        noteSortingMode = NoteModel::NoteSortingMode::ModifiedDescending;
        QNDEBUG(
            "quentier::MainWindow",
            "Couldn't restore the note sorting mode, fallback to the default "
                << "one of " << noteSortingMode);
    }

    m_ui->noteSortingModeComboBox->setCurrentIndex(
        static_cast<int>(noteSortingMode));

    QObject::connect(
        m_ui->noteSortingModeComboBox,
        qOverload<int>(&QComboBox::currentIndexChanged), this,
        &MainWindow::onNoteSortingModeChanged, Qt::UniqueConnection);

    onNoteSortingModeChanged(m_ui->noteSortingModeComboBox->currentIndex());

    auto * deletedNotesTableView = m_ui->deletedNotesTableView;

    deletedNotesTableView->setColumnHidden(
        static_cast<int>(NoteModel::Column::CreationTimestamp), true);

    deletedNotesTableView->setColumnHidden(
        static_cast<int>(NoteModel::Column::ModificationTimestamp), true);

    deletedNotesTableView->setColumnHidden(
        static_cast<int>(NoteModel::Column::PreviewText), true);

    deletedNotesTableView->setColumnHidden(
        static_cast<int>(NoteModel::Column::ThumbnailImage), true);

    deletedNotesTableView->setColumnHidden(
        static_cast<int>(NoteModel::Column::TagNameList), true);

    deletedNotesTableView->setColumnHidden(
        static_cast<int>(NoteModel::Column::Size), true);

    deletedNotesTableView->setColumnHidden(
        static_cast<int>(NoteModel::Column::Synchronizable), true);

    deletedNotesTableView->setColumnHidden(
        static_cast<int>(NoteModel::Column::NotebookName), true);

    auto * previousDeletedNoteItemDelegate =
        deletedNotesTableView->itemDelegate();

    auto * deletedNoteItemDelegate = qobject_cast<DeletedNoteItemDelegate *>(
        previousDeletedNoteItemDelegate);

    if (!deletedNoteItemDelegate) {
        deletedNoteItemDelegate =
            new DeletedNoteItemDelegate{deletedNotesTableView};

        deletedNotesTableView->setItemDelegate(deletedNoteItemDelegate);

        if (previousDeletedNoteItemDelegate) {
            previousDeletedNoteItemDelegate->deleteLater();
            previousDeletedNoteItemDelegate = nullptr;
        }
    }

    deletedNotesTableView->header()->setSectionResizeMode(
        QHeaderView::ResizeToContents);

    if (!m_editNoteDialogsManager) {
        m_editNoteDialogsManager = new EditNoteDialogsManager{
            m_localStorage, m_noteCache, m_notebookModel, this};

        QObject::connect(
            noteListView, &NoteListView::editNoteDialogRequested,
            m_editNoteDialogsManager,
            &EditNoteDialogsManager::onEditNoteDialogRequested,
            Qt::UniqueConnection);

        QObject::connect(
            noteListView, &NoteListView::noteInfoDialogRequested,
            m_editNoteDialogsManager,
            &EditNoteDialogsManager::onNoteInfoDialogRequested,
            Qt::UniqueConnection);

        QObject::connect(
            this, &MainWindow::noteInfoDialogRequested,
            m_editNoteDialogsManager,
            &EditNoteDialogsManager::onNoteInfoDialogRequested,
            Qt::UniqueConnection);

        QObject::connect(
            deletedNotesTableView,
            &DeletedNoteItemView::deletedNoteInfoRequested,
            m_editNoteDialogsManager,
            &EditNoteDialogsManager::onNoteInfoDialogRequested,
            Qt::UniqueConnection);
    }

    auto currentAccountType = Account::Type::Local;
    if (m_account) {
        currentAccountType = m_account->type();
    }

    showHideViewColumnsForAccountType(currentAccountType);
}

bool MainWindow::getShowNoteThumbnailsPreference() const
{
    ApplicationSettings appSettings{
        *m_account, preferences::keys::files::userInterface};

    appSettings.beginGroup(preferences::keys::appearanceGroup);

    const QVariant showThumbnails = appSettings.value(
        preferences::keys::showNoteThumbnails,
        QVariant::fromValue(preferences::defaults::showNoteThumbnails));

    appSettings.endGroup();

    return showThumbnails.toBool();
}

bool MainWindow::getDisableNativeMenuBarPreference() const
{
    ApplicationSettings appSettings{
        *m_account, preferences::keys::files::userInterface};

    appSettings.beginGroup(preferences::keys::appearanceGroup);

    const QVariant disableNativeMenuBar = appSettings.value(
        preferences::keys::disableNativeMenuBar,
        QVariant::fromValue(preferences::defaults::disableNativeMenuBar()));

    appSettings.endGroup();

    return disableNativeMenuBar.toBool();
}

QSet<QString> MainWindow::notesWithHiddenThumbnails() const
{
    ApplicationSettings appSettings{
        *m_account, preferences::keys::files::userInterface};

    appSettings.beginGroup(preferences::keys::appearanceGroup);

    const QVariant hideThumbnailsFor = appSettings.value(
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
    const bool newValue = !getShowNoteThumbnailsPreference();

    ApplicationSettings appSettings{
        *m_account, preferences::keys::files::userInterface};

    appSettings.beginGroup(preferences::keys::appearanceGroup);

    appSettings.setValue(
        preferences::keys::showNoteThumbnails, QVariant::fromValue(newValue));

    appSettings.endGroup();
}

void MainWindow::toggleHideNoteThumbnail(const QString & noteLocalId)
{
    auto noteLocalIds = notesWithHiddenThumbnails();
    if (const auto it = noteLocalIds.find(noteLocalId);
        it != noteLocalIds.end())
    {
        noteLocalIds.erase(it);
    }
    else if (
        noteLocalIds.size() <= preferences::keys::maxNotesWithHiddenThumbnails)
    {
        noteLocalIds.insert(noteLocalId);
    }
    else {
        informationMessageBox(
            this, tr("Cannot disable thumbnail for note"),
            tr("Too many notes with hidden thumbnails"),
            tr("There are too many notes for which thumbnails are hidden "
               "already. Consider disabling note thumbnails globally if "
               "you don't want to see them."));
        return;
    }

    ApplicationSettings appSettings{
        *m_account, preferences::keys::files::userInterface};

    appSettings.beginGroup(preferences::keys::appearanceGroup);

    appSettings.setValue(
        preferences::keys::notesWithHiddenThumbnails,
        QStringList(noteLocalIds.values()));

    appSettings.endGroup();
}

QString MainWindow::fallbackIconThemeName() const
{
    const auto & pal = palette();
    const auto & windowColor = pal.color(QPalette::Window);

    // lightness is the HSL color model value from 0 to 255
    // which can be used to deduce whether light or dark color theme
    // is used
    const int lightness = windowColor.lightness();

    if (lightness < 128) {
        // It appears that dark color theme is used
        return QStringLiteral("breeze-dark");
    }

    return QStringLiteral("breeze");
}

void MainWindow::quitApp(int exitCode)
{
    if (m_noteEditorTabsAndWindowsCoordinator) {
        // That would save the modified notes
        m_noteEditorTabsAndWindowsCoordinator->clear();
    }

    qApp->exit(exitCode);
}

utility::cancelers::ICancelerPtr MainWindow::setupSyncCanceler()
{
    if (!m_synchronizationCanceler) {
        m_synchronizationCanceler =
            std::make_shared<utility::cancelers::ManualCanceler>();
    }
    return m_synchronizationCanceler;
}

void MainWindow::clearViews()
{
    QNDEBUG("quentier::MainWindow", "MainWindow::clearViews");

    m_ui->favoritesTableView->setModel(&m_blankModel);
    m_ui->notebooksTreeView->setModel(&m_blankModel);
    m_ui->tagsTreeView->setModel(&m_blankModel);
    m_ui->savedSearchesItemView->setModel(&m_blankModel);

    m_ui->noteListView->setModel(&m_blankModel);
    // NOTE: without this the note list view doesn't seem to re-render
    // so the items from the previously set model are still displayed
    m_ui->noteListView->update();

    m_ui->deletedNotesTableView->setModel(&m_blankModel);
}

void MainWindow::setupAccountSpecificUiElements()
{
    QNDEBUG(
        "quentier::MainWindow", "MainWindow::setupAccountSpecificUiElements");

    if (Q_UNLIKELY(!m_account)) {
        QNDEBUG("quentier::MainWindow", "No account");
        return;
    }

    const bool isLocal = (m_account->type() == Account::Type::Local);

    m_ui->removeNotebookButton->setHidden(!isLocal);
    m_ui->removeNotebookButton->setDisabled(!isLocal);

    m_ui->removeTagButton->setHidden(!isLocal);
    m_ui->removeTagButton->setDisabled(!isLocal);

    m_ui->removeSavedSearchButton->setHidden(!isLocal);
    m_ui->removeSavedSearchButton->setDisabled(!isLocal);

    m_ui->syncPushButton->setHidden(isLocal);
    m_ui->syncPushButton->setDisabled(isLocal);

    m_ui->ActionSynchronize->setVisible(!isLocal);
    m_ui->menuService->menuAction()->setVisible(!isLocal);
}

void MainWindow::setupNoteFilters()
{
    QNDEBUG("quentier::MainWindow", "MainWindow::setupNoteFilters");

    Q_ASSERT(m_localStorage);

    m_ui->filterFrameBottomBoundary->hide();

    m_ui->filterByNotebooksWidget->setLocalStorage(*m_localStorage);
    m_ui->filterByTagsWidget->setLocalStorage(*m_localStorage);

    m_ui->filterByNotebooksWidget->switchAccount(*m_account, m_notebookModel);

    m_ui->filterByTagsWidget->switchAccount(*m_account, m_tagModel);

    m_ui->filterBySavedSearchComboBox->switchAccount(
        *m_account, m_savedSearchModel);

    m_ui->filterStatusBarLabel->hide();

    if (m_noteFiltersManager) {
        m_noteFiltersManager->disconnect();
        m_noteFiltersManager->deleteLater();
    }

    m_noteFiltersManager = new NoteFiltersManager{
        *m_account,
        *m_ui->filterByTagsWidget,
        *m_ui->filterByNotebooksWidget,
        *m_noteModel,
        *m_ui->filterBySavedSearchComboBox,
        *m_ui->filterBySearchStringWidget,
        m_localStorage,
        this};

    m_noteModel->start();

    ApplicationSettings appSettings{
        *m_account, preferences::keys::files::userInterface};

    appSettings.beginGroup("FiltersView"sv);
    m_filtersViewExpanded = appSettings.value(gFiltersViewStatusKey).toBool();
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
        "quentier::MainWindow",
        "MainWindow::setupNoteEditorTabWidgetsCoordinator");

    delete m_noteEditorTabsAndWindowsCoordinator;

    m_noteEditorTabsAndWindowsCoordinator =
        new NoteEditorTabsAndWindowsCoordinator{
            *m_account,
            m_localStorage,
            m_noteCache,
            m_notebookCache,
            m_tagCache,
            *m_tagModel,
            m_ui->noteEditorsTabWidget,
            this};

    QObject::connect(
        m_noteEditorTabsAndWindowsCoordinator,
        &NoteEditorTabsAndWindowsCoordinator::notifyError, this,
        &MainWindow::onNoteEditorError);

    QObject::connect(
        m_noteEditorTabsAndWindowsCoordinator,
        &NoteEditorTabsAndWindowsCoordinator::currentNoteChanged,
        m_ui->noteListView, &NoteListView::setCurrentNoteByLocalId);
}

#ifdef WITH_UPDATE_MANAGER
void MainWindow::setupUpdateManager()
{
    QNDEBUG("quentier::MainWindow", "MainWindow::setupUpdateManager");

    Q_ASSERT(m_noteEditorTabsAndWindowsCoordinator);

    m_updateManagerIdleInfoProvider.reset(new UpdateManagerIdleInfoProvider{
        *m_noteEditorTabsAndWindowsCoordinator});

    m_updateManager = new UpdateManager{m_updateManagerIdleInfoProvider, this};

    QObject::connect(
        m_updateManager, &UpdateManager::notifyError, this,
        &MainWindow::onUpdateManagerError);

    QObject::connect(
        m_updateManager, &UpdateManager::restartAfterUpdateRequested, this,
        &MainWindow::onUpdateManagerRequestsRestart);
}
#endif

bool MainWindow::checkLocalStorageVersion(
    const Account & account, ErrorString & errorDescription)
{
    QNDEBUG(
        "quentier::MainWindow",
        "MainWindow::checkLocalStorageVersion: account = " << account);

    Q_ASSERT(m_localStorage);

    bool isVersionTooHigh = false;
    auto isVersionTooHighFuture = m_localStorage->isVersionTooHigh();
    try {
        isVersionTooHighFuture.waitForFinished();
        isVersionTooHigh = isVersionTooHighFuture.result();
    }
    catch (const IQuentierException & e) {
        errorDescription = e.errorMessage();
        return false;
    }
    catch (const QException & e) {
        errorDescription = ErrorString{QString::fromUtf8(e.what())};
        return false;
    }

    if (isVersionTooHigh) {
        QNINFO(
            "quentier::MainWindow",
            "Detected too high local storage version: "
                << errorDescription << "; account: " << account);

        auto versionTooHighDialog =
            std::make_unique<LocalStorageVersionTooHighDialog>(
                account, m_accountManager->accountModel(), m_localStorage,
                this);

        QObject::connect(
            versionTooHighDialog.get(),
            &LocalStorageVersionTooHighDialog::shouldSwitchToAccount, this,
            &MainWindow::onAccountSwitchRequested,
            Qt::ConnectionType(Qt::UniqueConnection | Qt::QueuedConnection));

        QObject::connect(
            versionTooHighDialog.get(),
            &LocalStorageVersionTooHighDialog::shouldCreateNewAccount, this,
            &MainWindow::onNewAccountCreationRequested,
            Qt::ConnectionType(Qt::UniqueConnection | Qt::QueuedConnection));

        QObject::connect(
            versionTooHighDialog.get(),
            &LocalStorageVersionTooHighDialog::shouldQuitApp, this,
            &MainWindow::onQuitAction,
            Qt::ConnectionType(Qt::UniqueConnection | Qt::QueuedConnection));

        versionTooHighDialog->exec();
        return false;
    }

    QNDEBUG(
        "quentier::MainWindow",
        "Checking if any patches are required for local storage of account "
            << account);

    QList<local_storage::IPatchPtr> patches;
    auto patchesFuture = m_localStorage->requiredPatches();
    try {
        patchesFuture.waitForFinished();
        patches = patchesFuture.result();
    }
    catch (const IQuentierException & e) {
        errorDescription = e.errorMessage();
        return false;
    }
    catch (const QException & e) {
        errorDescription = ErrorString{QString::fromUtf8(e.what())};
        return false;
    }

    if (!patches.isEmpty()) {
        QNINFO(
            "quentier::MainWindow",
            "Local storage requires upgrade: found " << patches.size()
                                                     << " required patches");

        const auto options = LocalStorageUpgradeDialog::Options{} |
            LocalStorageUpgradeDialog::Option::AddAccount |
            LocalStorageUpgradeDialog::Option::SwitchToAnotherAccount;

        auto upgradeDialog = std::make_unique<LocalStorageUpgradeDialog>(
            account, m_accountManager->accountModel(), std::move(patches),
            options, this);

        QObject::connect(
            upgradeDialog.get(),
            &LocalStorageUpgradeDialog::shouldSwitchToAccount, this,
            &MainWindow::onAccountSwitchRequested,
            Qt::ConnectionType(Qt::UniqueConnection | Qt::QueuedConnection));

        QObject::connect(
            upgradeDialog.get(),
            &LocalStorageUpgradeDialog::shouldCreateNewAccount, this,
            &MainWindow::onNewAccountCreationRequested,
            Qt::ConnectionType(Qt::UniqueConnection | Qt::QueuedConnection));

        QObject::connect(
            upgradeDialog.get(), &LocalStorageUpgradeDialog::shouldQuitApp,
            this, &MainWindow::onQuitAction,
            Qt::ConnectionType(Qt::UniqueConnection | Qt::QueuedConnection));

        upgradeDialog->adjustSize();
        upgradeDialog->exec();

        if (!upgradeDialog->isUpgradeDone()) {
            errorDescription =
                ErrorString{QT_TR_NOOP("Could not upgrade local storage")};
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
        "quentier::MainWindow", "MainWindow::setOnceDisplayedGreeterScreen");

    ApplicationSettings appSettings;
    appSettings.beginGroup(preferences::keys::accountGroup);
    appSettings.setValue(preferences::keys::onceDisplayedWelcomeDialog, true);
    appSettings.endGroup();
}

void MainWindow::setupSynchronizer(const SetAccountOption setAccountOption)
{
    QNDEBUG(
        "quentier::MainWindow",
        "MainWindow::setupSynchronizer: set account option = "
            << setAccountOption);

    clearSynchronizer();

    if (m_synchronizationRemoteHost.isEmpty()) {
        if (Q_UNLIKELY(!m_account)) {
            const ErrorString error{
                QT_TR_NOOP("Can't set up the synchronization: no account")};
            QNWARNING("quentier::MainWindow", error);
            onSetStatusBarText(
                error.localizedString(), secondsToMilliseconds(30));
            return;
        }

        if (Q_UNLIKELY(m_account->type() != Account::Type::Evernote)) {
            const ErrorString error{
                QT_TR_NOOP("Can't set up the synchronization: non-Evernote "
                           "account is chosen")};

            QNWARNING(
                "quentier::MainWindow", error << "; account: " << *m_account);

            onSetStatusBarText(
                error.localizedString(), secondsToMilliseconds(30));
            return;
        }

        m_synchronizationRemoteHost = m_account->evernoteHost();
        if (Q_UNLIKELY(m_synchronizationRemoteHost.isEmpty())) {
            const ErrorString error{
                QT_TR_NOOP("Can't set up the synchronization: "
                           "no Evernote host within the account")};
            QNWARNING(
                "quentier::MainWindow", error << "; account: " << *m_account);
            onSetStatusBarText(
                error.localizedString(), secondsToMilliseconds(30));
            return;
        }
    }

    QString consumerKey, consumerSecret;
    setupConsumerKeyAndSecret(consumerKey, consumerSecret);

    if (Q_UNLIKELY(consumerKey.isEmpty())) {
        const ErrorString error{QT_TR_NOOP(
            "Can't set up the synchronization: consumer key is empty")};
        QNWARNING("quentier::MainWindow", error);
        onSetStatusBarText(
            error.localizedString(), secondsToMilliseconds(30));
        return;
    }

    if (Q_UNLIKELY(consumerSecret.isEmpty())) {
        const ErrorString error{QT_TR_NOOP(
            "Can't set up the synchronization: consumer secret is empty")};
        QNWARNING("quentier::MainWindow", error);
        onSetStatusBarText(
            error.localizedString(), secondsToMilliseconds(30));
        return;
    }

    QUrl evernoteServerUrl{
        QStringLiteral("https://") + m_synchronizationRemoteHost};

    if (Q_UNLIKELY(!evernoteServerUrl.isValid())) {
        ErrorString error{QT_TR_NOOP(
            "Can't set up the synchronization: failed to parse Evernote server "
            "URL")};
        error.details() = m_synchronizationRemoteHost;
        QNWARNING("quentier::MainWindow", error);
        onSetStatusBarText(
            error.localizedString(), secondsToMilliseconds(30));
        return;
    }

    QDir synchronizationPersistenceDir{
        accountPersistentStoragePath(*m_account) +
        QStringLiteral("/sync_data")};
    if (!synchronizationPersistenceDir.exists() &&
        !synchronizationPersistenceDir.mkpath(
            synchronizationPersistenceDir.absolutePath()))
    {
        ErrorString error{QT_TR_NOOP(
            "Can't set up the synchronization: cannot create dir for "
            "synchronization data persistence")};
        error.details() = synchronizationPersistenceDir.absolutePath();
        QNWARNING("quentier::MainWindow", error);
        onSetStatusBarText(
            error.localizedString(), secondsToMilliseconds(30));
        return;
    }

    m_authenticator = synchronization::createQEverCloudAuthenticator(
        std::move(consumerKey), std::move(consumerSecret), evernoteServerUrl,
        threading::QThreadPtr{QThread::currentThread(), [](const void *) {}},
        this);

    evernoteServerUrl.setPath(QStringLiteral("/edam/user"));

    m_synchronizer = synchronization::createSynchronizer(
        evernoteServerUrl, synchronizationPersistenceDir, m_authenticator);

    setupRunSyncPeriodicallyTimer();
}

void MainWindow::startSynchronization()
{
    QNINFO("quentier::MainWindow", "MainWindow::startSynchronization");

    Q_ASSERT(m_account);
    Q_ASSERT(m_localStorage);
    Q_ASSERT(m_synchronizer);

    ApplicationSettings appSettings{
        *m_account, preferences::keys::files::synchronization};

    appSettings.beginGroup(preferences::keys::synchronizationGroup);
    auto groupCloser =
        std::optional{ApplicationSettings::GroupCloser{appSettings}};

    const bool downloadNoteThumbnailsOption =
        (appSettings.contains(preferences::keys::downloadNoteThumbnails)
             ? appSettings.value(preferences::keys::downloadNoteThumbnails)
                   .toBool()
             : preferences::defaults::downloadNoteThumbnails);

    const bool downloadInkNoteImagesOption =
        (appSettings.contains(preferences::keys::downloadInkNoteImages)
             ? appSettings.value(preferences::keys::downloadInkNoteImages)
                   .toBool()
             : preferences::defaults::downloadInkNoteImages);

    groupCloser.reset();

    auto syncOptionsBuilder = synchronization::createSyncOptionsBuilder();
    syncOptionsBuilder->setDownloadNoteThumbnails(downloadNoteThumbnailsOption);

    if (downloadInkNoteImagesOption) {
        QString inkNoteImagesStoragePath = accountPersistentStoragePath(*m_account);
        inkNoteImagesStoragePath += QStringLiteral("/NoteEditorPage/inkNoteImages");

        QDir inkNoteImagesStorageDir{inkNoteImagesStoragePath};
        if (!inkNoteImagesStorageDir.exists() &&
            !inkNoteImagesStorageDir.mkpath(inkNoteImagesStoragePath))
        {
            QNWARNING(
                "quentier::MainWindow",
                "Cannot create directory for ink note images, will not "
                    << "download them; path = "
                    << QDir::toNativeSeparators(inkNoteImagesStoragePath));
        }
        else
        {
            syncOptionsBuilder->setInkNoteImagesStorageDir(
                inkNoteImagesStorageDir);
        }
    }

    auto syncCanceler = setupSyncCanceler();
    Q_ASSERT(syncCanceler);

    auto syncResult = m_synchronizer->synchronizeAccount(
        *m_account, m_localStorage, syncCanceler, syncOptionsBuilder->build());

    auto syncResultFuture = syncResult.first;
    if (!syncResultFuture.isFinished())
    {
        m_syncEventsNotifier = syncResult.second;
        Q_ASSERT(m_syncEventsNotifier);

        if (!m_syncEventsTracker) {
            m_syncEventsTracker = new SyncEventsTracker{this};
        }

        m_syncEventsTracker->start(m_syncEventsNotifier);

        m_syncApiRateLimitExceeded = false;
        m_syncInProgress = true;
        startSyncButtonAnimation();
        onSetStatusBarText(tr("Starting synchronization..."));
    }

    auto syncResultThenFuture = threading::then(
        std::move(syncResultFuture), this,
        [this, canceler = syncCanceler](
            const synchronization::ISyncResultPtr & syncResult) {
            if (canceler->isCanceled()) {
                return;
            }

            Q_ASSERT(syncResult);
            onSyncFinished(*syncResult);
        });

    threading::onFailed(
        std::move(syncResultThenFuture), this,
        [this, canceler = std::move(syncCanceler)](const QException & e) {
            if (canceler->isCanceled()) {
                return;
            }

            const auto error = exceptionMessage(e);
            QNWARNING(
                "quentier::MainWindow",
                "Synchronization failed: " << error);

            onSetStatusBarText(
                error.localizedString(), secondsToMilliseconds(60));
        });
}

void MainWindow::stopSynchronization(const StopSynchronizationMode mode)
{
    QNDEBUG(
        "quentier::MainWindow",
        "MainWindow::stopSynchronization: mode = " << mode);

    if (!m_syncInProgress) {
        QNDEBUG(
            "quentier::MainWindow",
            "Synchronization is not in progress, nothing to do");
        return;
    }

    QNINFO("quentier::MainWindow", "Stopping synchronization...");

    if (m_synchronizationCanceler) {
        m_synchronizationCanceler->cancel();
        m_synchronizationCanceler.reset();
    }

    m_syncInProgress = false;
    m_syncApiRateLimitExceeded = false;

    scheduleSyncButtonAnimationStop();

    if (m_syncEventsTracker) {
        m_syncEventsTracker->stop();
    }

    if (m_syncEventsNotifier) {
        // m_syncEventsNotifier is owned by m_synchronizer, so MainWindow is not
        // responsible for its disposal
        m_syncEventsNotifier = nullptr;
    }

    if (mode != StopSynchronizationMode::Quiet) {
        onSetStatusBarText(
            tr("Synchronization was stopped"), secondsToMilliseconds(30));
    }
}

void MainWindow::clearSynchronizer()
{
    QNDEBUG("quentier::MainWindow", "MainWindow::clearSynchronizer");

    stopSynchronization();

    m_synchronizer.reset();
    m_authenticator.reset();
    m_synchronizationRemoteHost.clear();

    if (m_runSyncPeriodicallyTimerId != 0) {
        killTimer(m_runSyncPeriodicallyTimerId);
        m_runSyncPeriodicallyTimerId = 0;
    }
}

void MainWindow::setupRunSyncPeriodicallyTimer()
{
    QNDEBUG(
        "quentier::MainWindow", "MainWindow::setupRunSyncPeriodicallyTimer");

    if (Q_UNLIKELY(!m_account)) {
        QNDEBUG("quentier::MainWindow", "No current account");
        return;
    }

    if (Q_UNLIKELY(m_account->type() != Account::Type::Evernote)) {
        QNDEBUG("quentier::MainWindow", "Non-Evernote account is used");
        return;
    }

    ApplicationSettings syncSettings{
        *m_account, preferences::keys::files::synchronization};

    syncSettings.beginGroup(preferences::keys::synchronizationGroup);
    auto groupCloser =
        std::optional<ApplicationSettings::GroupCloser>{syncSettings};

    int runSyncEachNumMinutes = -1;
    if (syncSettings.contains(preferences::keys::runSyncPeriodMinutes)) {
        const auto data =
            syncSettings.value(preferences::keys::runSyncPeriodMinutes);

        bool conversionResult = false;
        runSyncEachNumMinutes = data.toInt(&conversionResult);
        if (Q_UNLIKELY(!conversionResult)) {
            QNDEBUG(
                "quentier::MainWindow",
                "Failed to convert the number of minutes to run sync over to "
                    << "int: " << data);
            runSyncEachNumMinutes = -1;
        }
    }

    if (runSyncEachNumMinutes < 0) {
        runSyncEachNumMinutes = preferences::defaults::runSyncPeriodMinutes;
    }

    groupCloser.reset();
    onRunSyncEachNumMinitesPreferenceChanged(runSyncEachNumMinutes);
}

void MainWindow::launchSynchronization()
{
    QNDEBUG("quentier::MainWindow", "MainWindow::launchSynchronization");

    if (Q_UNLIKELY(!m_synchronizer)) {
        const ErrorString error{
            QT_TR_NOOP("Can't start synchronization: internal "
                       "error, no synchronization manager is set up")};
        QNWARNING("quentier::MainWindow", error);
        onSetStatusBarText(error.localizedString(), secondsToMilliseconds(30));
        return;
    }

    if (Q_UNLIKELY(!m_account)) {
        const ErrorString error{
            QT_TR_NOOP("Can't start synchronization: internal "
                       "error, no current account")};
        QNWARNING("quentier::MainWindow", error);
        onSetStatusBarText(error.localizedString(), secondsToMilliseconds(30));
        return;
    }

    if (m_syncInProgress) {
        QNDEBUG(
            "quentier::MainWindow", "Synchronization is already in progress");
        return;
    }

    if (m_pendingNewEvernoteAccountAuthentication) {
        QNDEBUG(
            "quentier::MainWindow",
            "Pending new Evernote account authentication");
        return;
    }

    if (m_pendingCurrentEvernoteAccountAuthentication) {
        QNDEBUG(
            "quentier::MainWindow",
            "Pending current Evernote account authentication");
        return;
    }

    if (m_pendingSwitchToNewEvernoteAccount) {
        QNDEBUG(
            "quentier::MainWindow", "Pending switch to new Evernote account");
        return;
    }

    if (m_authenticatedCurrentEvernoteAccount) {
        startSynchronization();
        return;
    }

    m_pendingCurrentEvernoteAccountAuthentication = true;

    QNDEBUG("quentier::MainWindow", "Authenticating current account");

    auto canceler = setupSyncCanceler();
    Q_ASSERT(canceler);

    auto authenticationFuture = m_synchronizer->authenticateAccount(*m_account);
    auto authenticationThenFuture = threading::then(
        std::move(authenticationFuture), this,
        [this, canceler](
            const synchronization::IAuthenticationInfoPtr &) {
            if (canceler->isCanceled()) {
                return;
            }

            if (Q_UNLIKELY(!m_account)) {
                onAuthenticationFinished(
                    false,
                    ErrorString{QT_TR_NOOP("Cannot authenticate current "
                                           "account: no current account")},
                    Account{});
                return;
            }

            onAuthenticationFinished(true, ErrorString{}, *m_account);
        });

    threading::onFailed(
        std::move(authenticationThenFuture), this,
        [this, canceler = std::move(canceler)](const QException & e) {
            if (canceler->isCanceled()) {
                return;
            }

            auto message = exceptionMessage(e);
            onAuthenticationFinished(false, std::move(message), Account{});
        });
}

void MainWindow::onSyncFinished(const synchronization::ISyncResult & syncResult)
{
    const auto synchronizationError = syncResult.stopSynchronizationError();
    if (const auto * rateLimitReachedError =
            std::get_if<synchronization::RateLimitReachedError>(
                &synchronizationError))
    {
        onRateLimitExceeded(rateLimitReachedError->rateLimitDurationSec);
        return;
    }

    if (std::holds_alternative<synchronization::AuthenticationExpiredError>(
            synchronizationError))
    {
        stopSynchronization(StopSynchronizationMode::Quiet);
        onSetStatusBarText(
            tr("Synchronization was stopped: authentication expired"),
            secondsToMilliseconds(30));
        return;
    }

    QNINFO(
        "quentier::MainWindow",
        "MainWindow::onSyncFinished: " << syncResult);

    onSetStatusBarText(
        tr("Synchronization finished!"), secondsToMilliseconds(5));

    stopSynchronization(StopSynchronizationMode::Quiet);
    setupRunSyncPeriodicallyTimer();
}

bool MainWindow::shouldRunSyncOnStartup() const
{
    if (Q_UNLIKELY(!m_account)) {
        QNWARNING(
            "quentier::MainWindow",
            "MainWindow::shouldRunSyncOnStartup: no account");
        return false;
    }

    if (m_account->type() != Account::Type::Evernote) {
        return false;
    }

    ApplicationSettings syncSettings{
        *m_account, preferences::keys::files::synchronization};

    syncSettings.beginGroup(preferences::keys::synchronizationGroup);
    const ApplicationSettings::GroupCloser groupCloser{syncSettings};

    const auto runSyncOnStartupValue =
        syncSettings.value(preferences::keys::runSyncOnStartup);

    if (runSyncOnStartupValue.isValid() &&
        runSyncOnStartupValue.canConvert<bool>())
    {
        return runSyncOnStartupValue.toBool();
    }

    return preferences::defaults::runSyncOnStartup;
}

void MainWindow::setupDefaultShortcuts()
{
    QNDEBUG("quentier::MainWindow", "MainWindow::setupDefaultShortcuts");

    using quentier::ShortcutManager;

#define PROCESS_ACTION_SHORTCUT(action, key, context)                          \
    {                                                                          \
        QString contextStr = QString::fromUtf8(context);                       \
        ActionKeyWithContext actionData;                                       \
        actionData.m_key = key;                                                \
        actionData.m_context = contextStr;                                     \
        QVariant data;                                                         \
        data.setValue(actionData);                                             \
        m_ui->Action##action->setData(data);                                   \
        QKeySequence shortcut = m_ui->Action##action->shortcut();              \
        if (shortcut.isEmpty()) {                                              \
            QNTRACE(                                                           \
                "quentier::MainWindow",                                        \
                "No shortcut was found for action "                            \
                    << m_ui->Action##action->objectName());                    \
        }                                                                      \
        else {                                                                 \
            m_shortcutManager.setDefaultShortcut(                              \
                key, shortcut, *m_account, contextStr);                        \
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
        m_ui->Action##action->setData(data);                                   \
        QKeySequence shortcut = m_ui->Action##action->shortcut();              \
        if (shortcut.isEmpty()) {                                              \
            QNTRACE(                                                           \
                "quentier::MainWindow",                                        \
                "No shortcut was found for action "                            \
                    << m_ui->Action##action->objectName());                    \
        }                                                                      \
        else {                                                                 \
            m_shortcutManager.setNonStandardDefaultShortcut(                   \
                actionStr, shortcut, *m_account, contextStr);                  \
        }                                                                      \
    }

#include "ActionShortcuts.inl"

#undef PROCESS_NON_STANDARD_ACTION_SHORTCUT
#undef PROCESS_ACTION_SHORTCUT
}

void MainWindow::setupUserShortcuts()
{
    QNDEBUG("quentier::MainWindow", "MainWindow::setupUserShortcuts");

#define PROCESS_ACTION_SHORTCUT(action, key, ...)                              \
    {                                                                          \
        QKeySequence shortcut = m_shortcutManager.shortcut(                    \
            key, *m_account, QStringLiteral("" __VA_ARGS__));                  \
        if (shortcut.isEmpty()) {                                              \
            QNTRACE(                                                           \
                "quentier::MainWindow",                                        \
                "No shortcut was found for action "                            \
                    << m_ui->Action##action->objectName());                    \
            auto it = m_shortcutKeyToAction.find(key);                         \
            if (it != m_shortcutKeyToAction.end()) {                           \
                auto * pAction = it.value();                                   \
                pAction->setShortcut(QKeySequence());                          \
                Q_UNUSED(m_shortcutKeyToAction.erase(it))                      \
            }                                                                  \
        }                                                                      \
        else {                                                                 \
            m_ui->Action##action->setShortcut(shortcut);                       \
            m_ui->Action##action->setShortcutContext(                          \
                Qt::WidgetWithChildrenShortcut);                               \
            m_shortcutKeyToAction[key] = m_ui->Action##action;                 \
        }                                                                      \
    }

#define PROCESS_NON_STANDARD_ACTION_SHORTCUT(action, ...)                      \
    {                                                                          \
        QKeySequence shortcut = m_shortcutManager.shortcut(                    \
            QStringLiteral(#action), *m_account,                               \
            QStringLiteral("" __VA_ARGS__));                                   \
        if (shortcut.isEmpty()) {                                              \
            QNTRACE(                                                           \
                "quentier::MainWindow",                                        \
                "No shortcut was found for action "                            \
                    << m_ui->Action##action->objectName());                    \
            auto it = m_nonStandardShortcutKeyToAction.find(                   \
                QStringLiteral(#action));                                      \
            if (it != m_nonStandardShortcutKeyToAction.end()) {                \
                auto * pAction = it.value();                                   \
                pAction->setShortcut(QKeySequence());                          \
                Q_UNUSED(m_nonStandardShortcutKeyToAction.erase(it))           \
            }                                                                  \
        }                                                                      \
        else {                                                                 \
            m_ui->Action##action->setShortcut(shortcut);                       \
            m_ui->Action##action->setShortcutContext(                          \
                Qt::WidgetWithChildrenShortcut);                               \
            m_nonStandardShortcutKeyToAction[QStringLiteral(#action)] =        \
                m_ui->Action##action;                                          \
        }                                                                      \
    }

#include "ActionShortcuts.inl"

#undef PROCESS_NON_STANDARD_ACTION_SHORTCUT
#undef PROCESS_ACTION_SHORTCUT
}

void MainWindow::startListeningForShortcutChanges()
{
    QNDEBUG(
        "quentier::MainWindow", "MainWindow::startListeningForShortcutChanges");

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
        "quentier::MainWindow", "MainWindow::stopListeningForShortcutChanges");

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

    Q_ASSERT(consumerKeyObf.size() <= std::numeric_limits<int>::max());

    char lastChar = 0;
    int size = static_cast<int>(consumerKeyObf.size());
    for (int i = 0; i < size; ++i) {
        char currentChar = consumerKeyObf[i];
        consumerKeyObf[i] = consumerKeyObf[i] ^ lastChar ^ key[i % 8];
        lastChar = currentChar;
    }

    consumerKey =
        QString::fromUtf8(consumerKeyObf.constData(), consumerKeyObf.size());

    QByteArray consumerSecretObf = QByteArray::fromBase64(
        QStringLiteral("BgFLOzJiZh9KSwRyLS8sAg==").toUtf8());

    Q_ASSERT(consumerSecretObf.size() <= std::numeric_limits<int>::max());

    lastChar = 0;
    size = static_cast<int>(consumerSecretObf.size());
    for (int i = 0; i < size; ++i) {
        char currentChar = consumerSecretObf[i];
        consumerSecretObf[i] = consumerSecretObf[i] ^ lastChar ^ key[i % 8];
        lastChar = currentChar;
    }

    consumerSecret = QString::fromUtf8(
        consumerSecretObf.constData(), consumerSecretObf.size());
}

void MainWindow::persistChosenNoteSortingMode(const int index)
{
    QNDEBUG(
        "quentier::MainWindow",
        "MainWindow::persistChosenNoteSortingMode: index = " << index);

    ApplicationSettings appSettings{
        *m_account, preferences::keys::files::userInterface};

    appSettings.beginGroup(QStringLiteral("NoteListView"));
    appSettings.setValue(gNoteSortingModeKey, index);
    appSettings.endGroup();
}

NoteModel::NoteSortingMode MainWindow::restoreNoteSortingMode()
{
    QNDEBUG("quentier::MainWindow", "MainWindow::restoreNoteSortingMode");

    ApplicationSettings appSettings{
        *m_account, preferences::keys::files::userInterface};

    appSettings.beginGroup(QStringLiteral("NoteListView"));
    auto groupCloser =
        std::optional<ApplicationSettings::GroupCloser>{appSettings};

    if (!appSettings.contains(gNoteSortingModeKey)) {
        QNDEBUG(
            "quentier::MainWindow",
            "No persisted note sorting mode within the settings, nothing to "
            "restore");
        return NoteModel::NoteSortingMode::None;
    }

    const auto data = appSettings.value(gNoteSortingModeKey);
    groupCloser.reset();

    if (data.isNull()) {
        QNDEBUG(
            "quentier::MainWindow",
            "No persisted note sorting mode, nothing to restore");
        return NoteModel::NoteSortingMode::None;
    }

    bool conversionResult = false;
    const int index = data.toInt(&conversionResult);
    if (Q_UNLIKELY(!conversionResult)) {
        const ErrorString error{
            QT_TR_NOOP("Internal error: can't restore the last used note "
                       "sorting mode, can't convert the persisted setting to "
                       "the integer index")};

        QNWARNING(
            "quentier::MainWindow", error << ", persisted data: " << data);

        onSetStatusBarText(error.localizedString(), secondsToMilliseconds(30));
        return NoteModel::NoteSortingMode::None;
    }

    if (index < 0 || index > 8) {
        const ErrorString error{
            QT_TR_NOOP("Internal error: can't restore the last used note "
                       "sorting mode, can't convert the persisted setting to "
                       "note soeting mode")};

        QNWARNING(
            "quentier::MainWindow", error << ", persisted data: " << data);

        onSetStatusBarText(error.localizedString(), secondsToMilliseconds(30));
        return NoteModel::NoteSortingMode::None;
    }

    return static_cast<NoteModel::NoteSortingMode>(index);
}

void MainWindow::persistGeometryAndState()
{
    QNDEBUG("quentier::MainWindow", "MainWindow::persistGeometryAndState");

    ApplicationSettings appSettings{
        *m_account, preferences::keys::files::userInterface};

    appSettings.beginGroup(QStringLiteral("MainWindow"));
    ApplicationSettings::GroupCloser groupCloser{appSettings};

    appSettings.setValue(gMainWindowGeometryKey, saveGeometry());
    appSettings.setValue(gMainWindowStateKey, saveState());

    const bool showSidePanel = m_ui->ActionShowSidePanel->isChecked();
    const bool showFavoritesView = m_ui->ActionShowFavorites->isChecked();
    const bool showNotebooksView = m_ui->ActionShowNotebooks->isChecked();
    const bool showTagsView = m_ui->ActionShowTags->isChecked();
    const bool showSavedSearches = m_ui->ActionShowSavedSearches->isChecked();
    const bool showDeletedNotes = m_ui->ActionShowDeletedNotes->isChecked();

    const auto splitterSizes = m_ui->splitter->sizes();
    Q_ASSERT(splitterSizes.count() <= std::numeric_limits<int>::max());
    const int splitterSizesCount = static_cast<int>(splitterSizes.count());
    const bool splitterSizesCountOk = (splitterSizesCount == 3);

    const auto sidePanelSplitterSizes = m_ui->sidePanelSplitter->sizes();
    Q_ASSERT(sidePanelSplitterSizes.count() <= std::numeric_limits<int>::max());
    const int sidePanelSplitterSizesCount =
        static_cast<int>(sidePanelSplitterSizes.count());

    const bool sidePanelSplitterSizesCountOk =
        (sidePanelSplitterSizesCount == 5);

    QNTRACE(
        "quentier::MainWindow",
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
        QTextStream strm{&str};

        strm << "Splitter sizes: ";
        for (const auto size: std::as_const(splitterSizes)) {
            strm << size << " ";
        }

        strm << "\n";

        strm << "Side panel splitter sizes: ";
        for (const auto size: std::as_const(sidePanelSplitterSizes)) {
            strm << size << " ";
        }

        QNTRACE("quentier::MainWindow", str);
    }

    if (splitterSizesCountOk && showSidePanel &&
        (showFavoritesView || showNotebooksView || showTagsView ||
         showSavedSearches || showDeletedNotes))
    {
        appSettings.setValue(gMainWindowSidePanelWidthKey, splitterSizes[0]);
    }
    else {
        appSettings.setValue(gMainWindowSidePanelWidthKey, QVariant{});
    }

    const bool showNotesList = m_ui->ActionShowNotesList->isChecked();
    if (splitterSizesCountOk && showNotesList) {
        appSettings.setValue(gMainWindowNoteListWidthKey, splitterSizes[1]);
    }
    else {
        appSettings.setValue(gMainWindowNoteListWidthKey, QVariant{});
    }

    if (sidePanelSplitterSizesCountOk && showFavoritesView) {
        appSettings.setValue(
            gMainWindowFavoritesViewHeightKey, sidePanelSplitterSizes[0]);
    }
    else {
        appSettings.setValue(gMainWindowFavoritesViewHeightKey, QVariant{});
    }

    if (sidePanelSplitterSizesCountOk && showNotebooksView) {
        appSettings.setValue(
            gMainWindowNotebooksViewHeightKey, sidePanelSplitterSizes[1]);
    }
    else {
        appSettings.setValue(gMainWindowNotebooksViewHeightKey, QVariant{});
    }

    if (sidePanelSplitterSizesCountOk && showTagsView) {
        appSettings.setValue(
            gMainWindowTagsViewHeightKey, sidePanelSplitterSizes[2]);
    }
    else {
        appSettings.setValue(gMainWindowTagsViewHeightKey, QVariant{});
    }

    if (sidePanelSplitterSizesCountOk && showSavedSearches) {
        appSettings.setValue(
            gMainWindowSavedSearchesViewHeightKey, sidePanelSplitterSizes[3]);
    }
    else {
        appSettings.setValue(gMainWindowSavedSearchesViewHeightKey, QVariant{});
    }

    if (sidePanelSplitterSizesCountOk && showDeletedNotes) {
        appSettings.setValue(
            gMainWindowDeletedNotesViewHeightKey, sidePanelSplitterSizes[4]);
    }
    else {
        appSettings.setValue(gMainWindowDeletedNotesViewHeightKey, QVariant{});
    }
}

void MainWindow::restoreGeometryAndState()
{
    QNDEBUG("quentier::MainWindow", "MainWindow::restoreGeometryAndState");

    ApplicationSettings appSettings{
        *m_account, preferences::keys::files::userInterface};

    appSettings.beginGroup(QStringLiteral("MainWindow"));

    const QByteArray savedGeometry =
        appSettings.value(gMainWindowGeometryKey).toByteArray();

    const QByteArray savedState =
        appSettings.value(gMainWindowStateKey).toByteArray();

    appSettings.endGroup();

    m_geometryRestored = restoreGeometry(savedGeometry);
    m_stateRestored = restoreState(savedState);

    QNTRACE(
        "quentier::MainWindow",
        "Geometry restored = " << (m_geometryRestored ? "true" : "false")
                               << ", state restored = "
                               << (m_stateRestored ? "true" : "false"));

    scheduleSplitterSizesRestoration();
}

void MainWindow::restoreSplitterSizes()
{
    QNDEBUG("quentier::MainWindow", "MainWindow::restoreSplitterSizes");

    ApplicationSettings appSettings{
        *m_account, preferences::keys::files::userInterface};

    appSettings.beginGroup(QStringLiteral("MainWindow"));
    auto groupCloser =
        std::optional<ApplicationSettings::GroupCloser>{appSettings};

    const QVariant sidePanelWidth =
        appSettings.value(gMainWindowSidePanelWidthKey);

    const QVariant notesListWidth =
        appSettings.value(gMainWindowNoteListWidthKey);

    const QVariant favoritesViewHeight =
        appSettings.value(gMainWindowFavoritesViewHeightKey);

    const QVariant notebooksViewHeight =
        appSettings.value(gMainWindowNotebooksViewHeightKey);

    const QVariant tagsViewHeight =
        appSettings.value(gMainWindowTagsViewHeightKey);

    const QVariant savedSearchesViewHeight =
        appSettings.value(gMainWindowSavedSearchesViewHeightKey);

    const QVariant deletedNotesViewHeight =
        appSettings.value(gMainWindowDeletedNotesViewHeightKey);

    groupCloser.reset();

    QNTRACE(
        "quentier::MainWindow",
        "Side panel width = "
            << sidePanelWidth << ", notes list width = " << notesListWidth
            << ", favorites view height = " << favoritesViewHeight
            << ", notebooks view height = " << notebooksViewHeight
            << ", tags view height = " << tagsViewHeight
            << ", saved searches view height = " << savedSearchesViewHeight
            << ", deleted notes view height = " << deletedNotesViewHeight);

    const bool showSidePanel = m_ui->ActionShowSidePanel->isChecked();
    const bool showNotesList = m_ui->ActionShowNotesList->isChecked();

    const bool showFavoritesView = m_ui->ActionShowFavorites->isChecked();
    const bool showNotebooksView = m_ui->ActionShowNotebooks->isChecked();
    const bool showTagsView = m_ui->ActionShowTags->isChecked();
    const bool showSavedSearches = m_ui->ActionShowSavedSearches->isChecked();
    const bool showDeletedNotes = m_ui->ActionShowDeletedNotes->isChecked();

    auto splitterSizes = m_ui->splitter->sizes();
    Q_ASSERT(splitterSizes.count() <= std::numeric_limits<int>::max());
    const int splitterSizesCount = static_cast<int>(splitterSizes.count());
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
            const int sidePanelWidthInt =
                sidePanelWidth.toInt(&conversionResult);
            if (conversionResult) {
                splitterSizes[0] = sidePanelWidthInt;
                QNTRACE(
                    "quentier::MainWindow",
                    "Restored side panel width: " << sidePanelWidthInt);
            }
            else {
                QNDEBUG(
                    "quentier::MainWindow",
                    "Can't restore the side panel width: can't "
                        << "convert the persisted width to int: "
                        << sidePanelWidth);
            }
        }

        if (notesListWidth.isValid() && showNotesList) {
            bool conversionResult = false;
            const int notesListWidthInt =
                notesListWidth.toInt(&conversionResult);
            if (conversionResult) {
                splitterSizes[1] = notesListWidthInt;
                QNTRACE(
                    "quentier::MainWindow",
                    "Restored notes list panel width: " << notesListWidthInt);
            }
            else {
                QNDEBUG(
                    "quentier::MainWindow",
                    "Can't restore the notes list panel width: can't convert "
                        << "persisted width to int: " << notesListWidth);
            }
        }

        splitterSizes[2] = totalWidth - splitterSizes[0] - splitterSizes[1];

        if (QuentierIsLogLevelActive(LogLevel::Trace)) {
            QString str;
            QTextStream strm{&str};

            strm << "Splitter sizes before restoring (total " << totalWidth
                 << "): ";

            const auto splitterSizesBefore = m_ui->splitter->sizes();
            for (const auto size: std::as_const(splitterSizesBefore)) {
                strm << size << " ";
            }

            QNTRACE("quentier::MainWindow", str);
        }

        m_ui->splitter->setSizes(splitterSizes);

        auto * sidePanel = m_ui->splitter->widget(0);
        auto sidePanelSizePolicy = sidePanel->sizePolicy();
        sidePanelSizePolicy.setHorizontalPolicy(QSizePolicy::Minimum);
        sidePanelSizePolicy.setHorizontalStretch(0);
        sidePanel->setSizePolicy(sidePanelSizePolicy);

        auto * noteListView = m_ui->splitter->widget(1);
        auto noteListViewSizePolicy = noteListView->sizePolicy();
        noteListViewSizePolicy.setHorizontalPolicy(QSizePolicy::Minimum);
        noteListViewSizePolicy.setHorizontalStretch(0);
        noteListView->setSizePolicy(noteListViewSizePolicy);

        auto * noteEditor = m_ui->splitter->widget(2);
        auto noteEditorSizePolicy = noteEditor->sizePolicy();
        noteEditorSizePolicy.setHorizontalPolicy(QSizePolicy::Expanding);
        noteEditorSizePolicy.setHorizontalStretch(1);
        noteEditor->setSizePolicy(noteEditorSizePolicy);

        QNTRACE("quentier::MainWindow", "Set splitter sizes");

        if (QuentierIsLogLevelActive(LogLevel::Trace)) {
            QString str;
            QTextStream strm{&str};

            strm << "Splitter sizes after restoring: ";
            auto splitterSizesAfter = m_ui->splitter->sizes();
            for (const auto size: std::as_const(splitterSizesAfter)) {
                strm << size << " ";
            }

            QNTRACE("quentier::MainWindow", str);
        }
    }
    else {
        const ErrorString error{
            QT_TR_NOOP("Internal error: can't restore the widths for side "
                       "panel, note list view and note editors view: wrong "
                       "number of sizes within the splitter")};

        QNWARNING(
            "quentier::MainWindow",
            error << ", sizes count: " << splitterSizesCount);

        onSetStatusBarText(error.localizedString(), secondsToMilliseconds(30));
    }

    auto sidePanelSplitterSizes = m_ui->sidePanelSplitter->sizes();
    Q_ASSERT(sidePanelSplitterSizes.count() <= std::numeric_limits<int>::max());
    const int sidePanelSplitterSizesCount =
        static_cast<int>(sidePanelSplitterSizes.count());

    if (sidePanelSplitterSizesCount == 5) {
        int totalHeight = 0;
        for (int i = 0; i < 5; ++i) {
            totalHeight += sidePanelSplitterSizes[i];
        }

        if (QuentierIsLogLevelActive(LogLevel::Trace)) {
            QString str;
            QTextStream strm{&str};

            strm << "Side panel splitter sizes before restoring (total "
                 << totalHeight << "): ";

            for (const auto size: std::as_const(sidePanelSplitterSizes)) {
                strm << size << " ";
            }

            QNTRACE("quentier::MainWindow", str);
        }

        if (showFavoritesView && favoritesViewHeight.isValid()) {
            bool conversionResult = false;

            const int favoritesViewHeightInt =
                favoritesViewHeight.toInt(&conversionResult);

            if (conversionResult) {
                sidePanelSplitterSizes[0] = favoritesViewHeightInt;
                QNTRACE(
                    "quentier::MainWindow",
                    "Restored favorites view height: "
                        << favoritesViewHeightInt);
            }
            else {
                QNDEBUG(
                    "quentier::MainWindow",
                    "Can't restore the favorites view height: can't convert "
                        << "persisted height to int: " << favoritesViewHeight);
            }
        }

        if (showNotebooksView && notebooksViewHeight.isValid()) {
            bool conversionResult = false;

            const int notebooksViewHeightInt =
                notebooksViewHeight.toInt(&conversionResult);

            if (conversionResult) {
                sidePanelSplitterSizes[1] = notebooksViewHeightInt;
                QNTRACE(
                    "quentier::MainWindow",
                    "Restored notebooks view height: "
                        << notebooksViewHeightInt);
            }
            else {
                QNDEBUG(
                    "quentier::MainWindow",
                    "Can't restore the notebooks view height: can't convert "
                        << "persisted height to int: " << notebooksViewHeight);
            }
        }

        if (showTagsView && tagsViewHeight.isValid()) {
            bool conversionResult = false;
            const int tagsViewHeightInt =
                tagsViewHeight.toInt(&conversionResult);
            if (conversionResult) {
                sidePanelSplitterSizes[2] = tagsViewHeightInt;
                QNTRACE(
                    "quentier::MainWindow",
                    "Restored tags view height: " << tagsViewHeightInt);
            }
            else {
                QNDEBUG(
                    "quentier::MainWindow",
                    "Can't restore the tags view height: can't convert "
                        << "persisted height to int: " << tagsViewHeight);
            }
        }

        if (showSavedSearches && savedSearchesViewHeight.isValid()) {
            bool conversionResult = false;

            const int savedSearchesViewHeightInt =
                savedSearchesViewHeight.toInt(&conversionResult);

            if (conversionResult) {
                sidePanelSplitterSizes[3] = savedSearchesViewHeightInt;
                QNTRACE(
                    "quentier::MainWindow",
                    "Restored saved searches view height: "
                        << savedSearchesViewHeightInt);
            }
            else {
                QNDEBUG(
                    "quentier::MainWindow",
                    "Can't restore the saved searches view height: can't "
                        << "convert persisted height to int: "
                        << savedSearchesViewHeight);
            }
        }

        if (showDeletedNotes && deletedNotesViewHeight.isValid()) {
            bool conversionResult = false;

            const int deletedNotesViewHeightInt =
                deletedNotesViewHeight.toInt(&conversionResult);

            if (conversionResult) {
                sidePanelSplitterSizes[4] = deletedNotesViewHeightInt;
                QNTRACE(
                    "quentier::MainWindow",
                    "Restored deleted notes view height: "
                        << deletedNotesViewHeightInt);
            }
            else {
                QNDEBUG(
                    "quentier::MainWindow",
                    "Can't restore the deleted notes view height: can't "
                        << "convert persisted height to int: "
                        << deletedNotesViewHeight);
            }
        }

        int totalHeightAfterRestore = 0;
        for (int i = 0; i < 5; ++i) {
            totalHeightAfterRestore += sidePanelSplitterSizes[i];
        }

        if (QuentierIsLogLevelActive(LogLevel::Trace)) {
            QString str;
            QTextStream strm{&str};

            strm << "Side panel splitter sizes after restoring (total "
                 << totalHeightAfterRestore << "): ";

            for (const auto size: std::as_const(sidePanelSplitterSizes)) {
                strm << size << " ";
            }

            QNTRACE("quentier::MainWindow", str);
        }

        m_ui->sidePanelSplitter->setSizes(sidePanelSplitterSizes);
        QNTRACE("quentier::MainWindow", "Set side panel splitter sizes");
    }
    else {
        const ErrorString error{
            QT_TR_NOOP("Internal error: can't restore the heights "
                       "of side panel's views: wrong number of "
                       "sizes within the splitter")};

        QNWARNING(
            "quentier::MainWindow",
            error << ", sizes count: " << splitterSizesCount);

        onSetStatusBarText(error.localizedString(), secondsToMilliseconds(30));
    }
}

void MainWindow::scheduleSplitterSizesRestoration()
{
    QNDEBUG(
        "quentier::MainWindow", "MainWindow::scheduleSplitterSizesRestoration");

    if (!m_shown) {
        QNDEBUG("quentier::MainWindow", "Not shown yet, won't do anything");
        return;
    }

    if (m_splitterSizesRestorationDelayTimerId != 0) {
        QNDEBUG(
            "quentier::MainWindow",
            "Splitter sizes restoration already scheduled, timer id = "
                << m_splitterSizesRestorationDelayTimerId);
        return;
    }

    m_splitterSizesRestorationDelayTimerId = startTimer(200);

    if (Q_UNLIKELY(m_splitterSizesRestorationDelayTimerId == 0)) {
        QNWARNING(
            "quentier::MainWindow",
            "Failed to start the timer to delay the restoration of splitter "
            "sizes");
        return;
    }

    QNDEBUG(
        "quentier::MainWindow",
        "Started the timer to delay the restoration of splitter sizes: "
            << m_splitterSizesRestorationDelayTimerId);
}

void MainWindow::scheduleGeometryAndStatePersisting()
{
    QNDEBUG(
        "quentier::MainWindow",
        "MainWindow::scheduleGeometryAndStatePersisting");

    if (!m_shown) {
        QNDEBUG("quentier::MainWindow", "Not shown yet, won't do anything");
        return;
    }

    if (m_geometryAndStatePersistingDelayTimerId != 0) {
        QNDEBUG(
            "quentier::MainWindow",
            "Persisting already scheduled, timer id = "
                << m_geometryAndStatePersistingDelayTimerId);
        return;
    }

    m_geometryAndStatePersistingDelayTimerId = startTimer(500);

    if (Q_UNLIKELY(m_geometryAndStatePersistingDelayTimerId == 0)) {
        QNWARNING(
            "quentier::MainWindow",
            "Failed to start the timer to delay the persistence of "
            "MainWindow's state and geometry");
        return;
    }

    QNDEBUG(
        "quentier::MainWindow",
        "Started the timer to delay the persistence of MainWindow's state and "
            << "geometry: timer id = "
            << m_geometryAndStatePersistingDelayTimerId);
}

template <class T>
void MainWindow::refreshThemeIcons()
{
    auto objects = findChildren<T *>();
    QNDEBUG(
        "quentier::MainWindow", "Found " << objects.size() << " child objects");

    for (auto * object: std::as_const(objects)) {
        if (Q_UNLIKELY(!object)) {
            continue;
        }

        const auto icon = object->icon();
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

        object->setIcon(newIcon);
    }
}

QDebug & operator<<(QDebug & dbg, const MainWindow::SetAccountOption option)
{
    switch (option) {
    case MainWindow::SetAccountOption::Set:
        dbg << "Set";
        break;
    case MainWindow::SetAccountOption::DontSet:
        dbg << "Don't set";
        break;
    }

    return dbg;
}

QDebug & operator<<(
    QDebug & dbg, const MainWindow::StopSynchronizationMode mode)
{
    switch (mode) {
    case MainWindow::StopSynchronizationMode::Quiet:
        dbg << "Quiet";
        break;
    case MainWindow::StopSynchronizationMode::Verbose:
        dbg << "Verbose";
        break;
    }

    return dbg;
}

} // namespace quentier
