/*
 * Copyright 2017-2025 Dmitry Ivanov
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

#include "NoteEditorTabsAndWindowsCoordinator.h"

#include "NoteEditorWidget.h"
#include "TabWidget.h"

#include <lib/exception/Utils.h>
#include <lib/model/tag/TagModel.h>
#include <lib/preferences/defaults/NoteEditor.h>
#include <lib/preferences/keys/Files.h>
#include <lib/preferences/keys/NoteEditor.h>
#include <lib/view/Utils.h>

#include <quentier/enml/Factory.h>
#include <quentier/enml/IDecryptedTextCache.h>
#include <quentier/exception/InvalidArgument.h>
#include <quentier/local_storage/ILocalStorage.h>
#include <quentier/local_storage/ILocalStorageNotifier.h>
#include <quentier/logging/QuentierLogger.h>
#include <quentier/note_editor/NoteEditor.h>
#include <quentier/note_editor/SpellChecker.h>
#include <quentier/threading/Future.h>
#include <quentier/utility/ApplicationSettings.h>
#include <quentier/utility/EventLoopWithExitStatus.h>
#include <quentier/utility/FileIOProcessorAsync.h>
#include <quentier/utility/MessageBox.h>
#include <quentier/utility/cancelers/ManualCanceler.h>

#include <qevercloud/types/Note.h>
#include <qevercloud/types/builders/NoteAttributesBuilder.h>

#include <QApplication>
#include <QCloseEvent>
#include <QContextMenuEvent>
#include <QEvent>
#include <QKeyEvent>
#include <QMenu>
#include <QTabBar>
#include <QThread>
#include <QTimer>
#include <QTimerEvent>
#include <QUndoStack>

#include <algorithm>
#include <string_view>
#include <utility>

namespace quentier {

using namespace std::string_view_literals;

namespace {

constexpr int gMinNumNotesInTabs = 1;
constexpr int gMaxWindowNameSize = 120;
constexpr auto gBlankNoteKey = "BlankNoteId"sv;

constexpr auto gOpenNotesLocalIdsInTabsKey =
    "LocalUidsOfNotesLastOpenInNoteEditorTabs"sv;

constexpr auto gOpenNotesLocalIdsInWindowsKey =
    "LocalUidsOfNotesLastOpenInNoteEditorWindows"sv;

constexpr auto gLastCurrentTabLocalId = "LastCurrentTabNoteLocalUid"sv;

constexpr auto gNoteEditorWindowGeometryKeyPrefix =
    "NoteEditorWindowGeometry_"sv;

} // namespace

NoteEditorTabsAndWindowsCoordinator::NoteEditorTabsAndWindowsCoordinator(
    Account account, local_storage::ILocalStoragePtr localStorage,
    NoteCache & noteCache, NotebookCache & notebookCache, TagCache & tagCache,
    TagModel & tagModel, TabWidget * tabWidget, QObject * parent) :
    QObject{parent}, m_localStorage{std::move(localStorage)},
    m_currentAccount{std::move(account)}, m_noteCache{noteCache},
    m_notebookCache{notebookCache}, m_tagCache{tagCache}, m_tagModel{&tagModel},
    m_tabWidget{tabWidget}
{
    if (Q_UNLIKELY(!m_localStorage)) {
        throw InvalidArgument{ErrorString{
            "NoteEditorTabsAndWindowsCoordinator ctor: local storage is null"}};
    }

    ApplicationSettings appSettings{
        m_currentAccount, preferences::keys::files::userInterface};

    appSettings.beginGroup(preferences::keys::noteEditorGroup);

    const auto maxNumNoteTabsData =
        appSettings.value(QStringLiteral("MaxNumNoteTabs"));

    appSettings.endGroup();

    bool conversionResult = false;
    const int maxNumNoteTabs = maxNumNoteTabsData.toInt(&conversionResult);
    if (!conversionResult) {
        constexpr int defaultMaxNumNotesInTabs = 5;
        QNDEBUG(
            "widget::NoteEditorTabsAndWindowsCoordinator",
            "NoteEditorTabsAndWindowsCoordinator: no persisted max num "
                << "note tabs setting, fallback to the default value of "
                << defaultMaxNumNotesInTabs);

        m_maxNumNotesInTabs = defaultMaxNumNotesInTabs;
    }
    else {
        QNDEBUG(
            "widget::NoteEditorTabsAndWindowsCoordinator",
            "NoteEditorTabsAndWindowsCoordinator: max num note tabs: "
                << maxNumNoteTabs);

        m_maxNumNotesInTabs = maxNumNoteTabs;
    }

    m_localIdsOfNotesInTabbedEditors.set_capacity(static_cast<std::size_t>(
        std::max(m_maxNumNotesInTabs, gMinNumNotesInTabs)));

    QNTRACE(
        "widget::NoteEditorTabsAndWindowsCoordinator",
        "Tabbed note local ids circular buffer capacity: "
            << m_localIdsOfNotesInTabbedEditors.capacity());

    setupFileIO();
    setupSpellChecker();

    auto * undoStack = new QUndoStack;

    m_blankNoteEditor = new NoteEditorWidget(
        m_currentAccount, m_localStorage, nullptr, *m_spellChecker, m_ioThread,
        m_noteCache, m_notebookCache, m_tagCache, *m_tagModel, undoStack,
        m_tabWidget);

    undoStack->setParent(m_blankNoteEditor);

    connectNoteEditorWidgetToColorChangeSignals(*m_blankNoteEditor);

    m_tabWidget->addTab(
        m_blankNoteEditor,
        QString::fromUtf8(gBlankNoteKey.data(), gBlankNoteKey.size()));

    auto * tabBar = m_tabWidget->tabBar();
    tabBar->hide();
    tabBar->setContextMenuPolicy(Qt::CustomContextMenu);

    QObject::connect(
        tabBar, &QTabBar::customContextMenuRequested, this,
        &NoteEditorTabsAndWindowsCoordinator::onTabContextMenuRequested);

    restoreLastOpenNotes();

    QObject::connect(
        m_tabWidget, &TabWidget::tabCloseRequested, this,
        &NoteEditorTabsAndWindowsCoordinator::onNoteEditorTabCloseRequested);

    QObject::connect(
        m_tabWidget, &TabWidget::currentChanged, this,
        &NoteEditorTabsAndWindowsCoordinator::onCurrentTabChanged);

    auto * currentEditor =
        qobject_cast<NoteEditorWidget *>(m_tabWidget->currentWidget());

    if (currentEditor) {
        if (currentEditor->hasFocus()) {
            QNDEBUG(
                "widget::NoteEditorTabsAndWindowsCoordinator",
                "The tab widget's current tab widget already has focus");
            return;
        }

        QNDEBUG(
            "widget::NoteEditorTabsAndWindowsCoordinator",
            "Setting the focus to the current tab widget");

        currentEditor->setFocusToEditor();
    }
}

NoteEditorTabsAndWindowsCoordinator::~NoteEditorTabsAndWindowsCoordinator()
{
    if (m_ioThread) {
        m_ioThread->quit();
    }
}

void NoteEditorTabsAndWindowsCoordinator::clear()
{
    QNDEBUG(
        "widget::NoteEditorTabsAndWindowsCoordinator",
        "NoteEditorTabsAndWindowsCoordinator::clear: num tabs = "
            << m_tabWidget->count()
            << ", num windows = " << m_noteEditorWindowsByNoteLocalId.size());

    m_blankNoteEditor = nullptr;

    // Prevent currentChanged signal from tabs removal inside this method to
    // mess with last current tab note local id
    QObject::disconnect(
        m_tabWidget, &TabWidget::currentChanged, this,
        &NoteEditorTabsAndWindowsCoordinator::onCurrentTabChanged);

    while (m_tabWidget->count() != 0) {
        auto * noteEditorWidget =
            qobject_cast<NoteEditorWidget *>(m_tabWidget->widget(0));

        if (Q_UNLIKELY(!noteEditorWidget)) {
            QNWARNING(
                "widget::NoteEditorTabsAndWindowsCoordinator",
                "Detected some widget other than NoteEditorWidget within the "
                "tab widget");

            auto * widget = m_tabWidget->widget(0);
            m_tabWidget->removeTab(0);
            if (widget) {
                widget->hide();
                widget->deleteLater();
            }

            continue;
        }

        const QString noteLocalId = noteEditorWidget->noteLocalId();

        QNTRACE(
            "widget::NoteEditorTabsAndWindowsCoordinator",
            "Safely closing note editor tab: " << noteLocalId);

        ErrorString errorDescription;

        const auto res =
            noteEditorWidget->checkAndSaveModifiedNote(errorDescription);

        if (Q_UNLIKELY(res != NoteEditorWidget::NoteSaveStatus::Ok)) {
            QNINFO(
                "widget::NoteEditorTabsAndWindowsCoordinator",
                "Could not save note: " << noteEditorWidget->noteLocalId()
                                        << ", status: " << res
                                        << ", error: " << errorDescription);
        }

        m_tabWidget->removeTab(0);

        noteEditorWidget->removeEventFilter(this);
        noteEditorWidget->hide();
        noteEditorWidget->deleteLater();

        QNTRACE(
            "widget::NoteEditorTabsAndWindowsCoordinator",
            "Removed note editor tab: " << noteLocalId);
    }

    while (!m_noteEditorWindowsByNoteLocalId.isEmpty()) {
        const auto it = m_noteEditorWindowsByNoteLocalId.begin();
        if (Q_UNLIKELY(it.value().isNull())) {
            Q_UNUSED(m_noteEditorWindowsByNoteLocalId.erase(it))
            continue;
        }

        auto * noteEditorWidget = it.value().data();

        const QString noteLocalId = noteEditorWidget->noteLocalId();
        QNTRACE(
            "widget::NoteEditorTabsAndWindowsCoordinator",
            "Safely closing note editor " << "window: " << noteLocalId);

        ErrorString errorDescription;

        const auto res =
            noteEditorWidget->checkAndSaveModifiedNote(errorDescription);

        if (Q_UNLIKELY(res != NoteEditorWidget::NoteSaveStatus::Ok)) {
            QNINFO(
                "widget::NoteEditorTabsAndWindowsCoordinator",
                "Could not save note: " << noteEditorWidget->noteLocalId()
                                        << ", status: " << res
                                        << ", error: " << errorDescription);
        }

        noteEditorWidget->removeEventFilter(this);
        noteEditorWidget->hide();
        noteEditorWidget->deleteLater();
        Q_UNUSED(m_noteEditorWindowsByNoteLocalId.erase(it))

        QNTRACE(
            "widget::NoteEditorTabsAndWindowsCoordinator",
            "Closed note editor window: " << noteLocalId);
    }

    m_localIdsOfNotesInTabbedEditors.clear();
    m_lastCurrentTabNoteLocalId.clear();
    m_noteEditorWindowsByNoteLocalId.clear();
    m_decryptedTextCachesByNoteLocalId.clear();

    for (auto
             it =
                 m_saveNoteEditorWindowGeometryPostponeTimerIdToNoteLocalIdBimap
                     .left.begin(),
             end =
                 m_saveNoteEditorWindowGeometryPostponeTimerIdToNoteLocalIdBimap
                     .left.end();
         it != end; ++it)
    {
        killTimer(it->second);
    }

    m_saveNoteEditorWindowGeometryPostponeTimerIdToNoteLocalIdBimap.clear();

    if (m_canceler) {
        m_canceler->cancel();
        m_canceler.reset();
    }

    m_localIdOfNoteToBeExpunged.clear();
}

void NoteEditorTabsAndWindowsCoordinator::switchAccount(
    Account account, local_storage::ILocalStoragePtr localStorage,
    TagModel & tagModel)
{
    QNDEBUG(
        "widget::NoteEditorTabsAndWindowsCoordinator",
        "NoteEditorTabsAndWindowsCoordinator::switchAccount: " << account);

    Q_ASSERT(localStorage);

    if (account == m_currentAccount) {
        QNDEBUG(
            "widget::NoteEditorTabsAndWindowsCoordinator",
            "The account has not changed, nothing to do");
        return;
    }

    clear();

    m_tagModel = QPointer<TagModel>(&tagModel);
    m_currentAccount = std::move(account);
    m_localStorage = std::move(localStorage);

    restoreLastOpenNotes();
}

void NoteEditorTabsAndWindowsCoordinator::setMaxNumNotesInTabs(
    const int maxNumNotesInTabs)
{
    QNDEBUG(
        "widget::NoteEditorTabsAndWindowsCoordinator",
        "NoteEditorTabsAndWindowsCoordinator::setMaxNumNotesInTabs: "
            << maxNumNotesInTabs);

    if (m_maxNumNotesInTabs == maxNumNotesInTabs) {
        QNDEBUG(
            "widget::NoteEditorTabsAndWindowsCoordinator",
            "Max number of notes in tabs hasn't changed");
        return;
    }

    if (m_maxNumNotesInTabs < maxNumNotesInTabs) {
        m_maxNumNotesInTabs = maxNumNotesInTabs;
        QNDEBUG(
            "widget::NoteEditorTabsAndWindowsCoordinator",
            "Max number of notes in tabs has been increased to "
                << maxNumNotesInTabs);
        return;
    }

    const int currentNumNotesInTabs = numNotesInTabs();

    m_maxNumNotesInTabs = maxNumNotesInTabs;
    QNDEBUG(
        "widget::NoteEditorTabsAndWindowsCoordinator",
        "Max number of notes in tabs has been decreased to "
            << maxNumNotesInTabs);

    if (currentNumNotesInTabs <= maxNumNotesInTabs) {
        return;
    }

    m_localIdsOfNotesInTabbedEditors.set_capacity(static_cast<std::size_t>(
        std::max(maxNumNotesInTabs, gMinNumNotesInTabs)));

    QNTRACE(
        "widget::NoteEditorTabsAndWindowsCoordinator",
        "Tabbed note local ids circular buffer capacity: "
            << m_localIdsOfNotesInTabbedEditors.capacity());

    checkAndCloseOlderNoteEditorTabs();
}

int NoteEditorTabsAndWindowsCoordinator::numNotesInTabs() const
{
    if (Q_UNLIKELY(!m_tabWidget)) {
        return 0;
    }

    if (m_blankNoteEditor) {
        return 0;
    }

    return m_tabWidget->count();
}

NoteEditorWidget *
NoteEditorTabsAndWindowsCoordinator::noteEditorWidgetForNoteLocalId(
    const QString & noteLocalId)
{
    QNDEBUG(
        "widget::NoteEditorTabsAndWindowsCoordinator",
        "NoteEditorTabsAndWindowsCoordinator::noteEditorWidgetForNoteLocalId: "
            << noteLocalId);

    for (int i = 0; i < m_tabWidget->count(); ++i) {
        auto * noteEditorWidget =
            qobject_cast<NoteEditorWidget *>(m_tabWidget->widget(i));

        if (Q_UNLIKELY(!noteEditorWidget)) {
            continue;
        }

        if (noteEditorWidget->noteLocalId() != noteLocalId) {
            continue;
        }

        QNDEBUG(
            "widget::NoteEditorTabsAndWindowsCoordinator",
            "Found existing editor tab");
        return noteEditorWidget;
    }

    for (auto it = m_noteEditorWindowsByNoteLocalId.begin(),
              end = m_noteEditorWindowsByNoteLocalId.end();
         it != end; ++it)
    {
        QNTRACE(
            "widget::NoteEditorTabsAndWindowsCoordinator",
            "Examining window editor's note local id = " << it.key());

        const auto & noteEditorWidget = it.value();
        if (Q_UNLIKELY(noteEditorWidget.isNull())) {
            QNTRACE(
                "widget::NoteEditorTabsAndWindowsCoordinator",
                "The note editor widget is " << "gone, skipping");
            continue;
        }

        const QString & existingNoteLocalId = it.key();
        if (existingNoteLocalId != noteLocalId) {
            continue;
        }

        QNDEBUG(
            "widget::NoteEditorTabsAndWindowsCoordinator",
            "Found existing editor window");
        return noteEditorWidget.data();
    }

    return nullptr;
}

void NoteEditorTabsAndWindowsCoordinator::addNote(
    const QString & noteLocalId, const NoteEditorMode noteEditorMode,
    const bool isNewNote)
{
    QNDEBUG(
        "widget::NoteEditorTabsAndWindowsCoordinator",
        "NoteEditorTabsAndWindowsCoordinator::addNote: "
            << noteLocalId << ", note editor mode = " << noteEditorMode
            << ", is new note = " << (isNewNote ? "true" : "false"));

    // First, check if this note is already open in some existing tab/window
    for (auto it = m_localIdsOfNotesInTabbedEditors.begin(),
              end = m_localIdsOfNotesInTabbedEditors.end();
         it != end; ++it)
    {
        QNTRACE(
            "widget::NoteEditorTabsAndWindowsCoordinator",
            "Examining tab editor's note local id = " << *it);

        const QString & existingNoteLocalId = *it;
        if (Q_UNLIKELY(existingNoteLocalId == noteLocalId)) {
            QNDEBUG(
                "widget::NoteEditorTabsAndWindowsCoordinator",
                "The requested note is already open within one of note editor "
                "tabs");

            if (noteEditorMode == NoteEditorMode::Window) {
                moveNoteEditorTabToWindow(noteLocalId);
            }
            else {
                setCurrentNoteEditorWidgetTab(noteLocalId);
            }

            return;
        }
    }

    for (auto it = m_noteEditorWindowsByNoteLocalId.begin(),
              end = m_noteEditorWindowsByNoteLocalId.end();
         it != end; ++it)
    {
        QNTRACE(
            "widget::NoteEditorTabsAndWindowsCoordinator",
            "Examining window editor's note local id = " << it.key());

        const auto & noteEditorWidget = it.value();
        if (noteEditorWidget.isNull()) {
            QNTRACE(
                "widget::NoteEditorTabsAndWindowsCoordinator",
                "The note editor widget is gone, skipping");
            continue;
        }

        const QString & existingNoteLocalId = it.key();
        if (Q_UNLIKELY(existingNoteLocalId == noteLocalId)) {
            QNDEBUG(
                "widget::NoteEditorTabsAndWindowsCoordinator",
                "The requested note is already open within one of note editor "
                "windows");

            if (noteEditorMode == NoteEditorMode::Tab) {
                moveNoteEditorWindowToTab(noteEditorWidget.data());
                setCurrentNoteEditorWidgetTab(noteLocalId);
            }

            return;
        }
    }

    // If we got here, the note with specified local id was not found within
    // already open windows or tabs

    if ((noteEditorMode != NoteEditorMode::Window) && m_blankNoteEditor) {
        QNDEBUG(
            "widget::NoteEditorTabsAndWindowsCoordinator",
            "Currently only the blank note tab is displayed, inserting the new "
            "note into the blank tab");

        auto * noteEditorWidget = m_blankNoteEditor;
        m_blankNoteEditor = nullptr;

        noteEditorWidget->setNoteLocalId(noteLocalId, isNewNote);
        insertNoteEditorWidget(noteEditorWidget, NoteEditorMode::Tab);

        if (m_trackingCurrentTab) {
            m_lastCurrentTabNoteLocalId = noteLocalId;
            persistLastCurrentTabNoteLocalId();

            QNTRACE(
                "widget::NoteEditorTabsAndWindowsCoordinator",
                "Emitting the update of last current tab note local id to "
                    << m_lastCurrentTabNoteLocalId);

            Q_EMIT currentNoteChanged(m_lastCurrentTabNoteLocalId);
        }

        return;
    }

    auto * undoStack = new QUndoStack;

    enml::IDecryptedTextCachePtr decryptedTextCache = [this, &noteLocalId] {
        if (const auto it =
                m_decryptedTextCachesByNoteLocalId.constFind(noteLocalId);
            it != m_decryptedTextCachesByNoteLocalId.constEnd())
        {
            return it.value();
        }

        auto decryptedTextCache = enml::createDecryptedTextCache();
        m_decryptedTextCachesByNoteLocalId.insert(
            noteLocalId, decryptedTextCache);
        return decryptedTextCache;
    }();

    auto * noteEditorWidget = new NoteEditorWidget(
        m_currentAccount, m_localStorage, std::move(decryptedTextCache),
        *m_spellChecker, m_ioThread, m_noteCache, m_notebookCache, m_tagCache,
        *m_tagModel, undoStack, m_tabWidget);

    undoStack->setParent(noteEditorWidget);

    connectNoteEditorWidgetToColorChangeSignals(*noteEditorWidget);

    noteEditorWidget->setNoteLocalId(noteLocalId, isNewNote);
    insertNoteEditorWidget(noteEditorWidget, noteEditorMode);
}

void NoteEditorTabsAndWindowsCoordinator::createNewNote(
    const QString & notebookLocalId, const QString & notebookGuid,
    const NoteEditorMode noteEditorMode)
{
    QNDEBUG(
        "widget::NoteEditorTabsAndWindowsCoordinator",
        "NoteEditorTabsAndWindowsCoordinator::createNewNote: "
            << "notebook local id = " << notebookLocalId << ", notebook guid = "
            << notebookGuid << ", node editor mode = " << noteEditorMode);

    qevercloud::Note newNote;
    newNote.setNotebookLocalId(notebookLocalId);
    newNote.setLocalOnly(m_currentAccount.type() == Account::Type::Local);
    newNote.setLocallyModified(true);
    newNote.setContent(
        QStringLiteral("<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
                       "<!DOCTYPE en-note SYSTEM "
                       "\"http://xml.evernote.com/pub/enml2.dtd\">"
                       "<en-note><div></div></en-note>"));

    qint64 timestamp = QDateTime::currentMSecsSinceEpoch();
    newNote.setCreated(timestamp);
    newNote.setUpdated(timestamp);

    QString sourceApplicationName = QApplication::applicationName();
    auto sourceApplicationNameSize = sourceApplicationName.size();
    if ((sourceApplicationNameSize >= qevercloud::EDAM_ATTRIBUTE_LEN_MIN) &&
        (sourceApplicationNameSize <= qevercloud::EDAM_ATTRIBUTE_LEN_MAX))
    {
        newNote.setAttributes(
            qevercloud::NoteAttributesBuilder{}
                .setSourceApplication(std::move(sourceApplicationName))
                .build());
    }

    if (!notebookGuid.isEmpty()) {
        newNote.setNotebookGuid(notebookGuid);
    }

    QNDEBUG(
        "widget::NoteEditorTabsAndWindowsCoordinator",
        "Putting new note into local storage: " << newNote);

    auto canceler = setupCanceler();
    Q_ASSERT(canceler);

    auto putNoteFuture = m_localStorage->putNote(newNote);

    auto putNoteThenFuture = threading::then(
        std::move(putNoteFuture), this,
        [this, newNote, noteEditorMode, canceler] {
            if (canceler->isCanceled()) {
                return;
            }

            QNDEBUG(
                "widget::NoteEditorTabsAndWindowsCoordinator",
                "Successfully put new note into local storage: " << newNote);

            m_noteCache.put(newNote.localId(), newNote);
            addNote(
                newNote.localId(), noteEditorMode, /* is new note = */ true);
        });

    threading::onFailed(
        std::move(putNoteThenFuture), this,
        [this, newNote, canceler = std::move(canceler)](const QException & e) {
            if (canceler->isCanceled()) {
                return;
            }

            auto message = exceptionMessage(e);

            QNWARNING(
                "widget::NoteEditorTabsAndWindowsCoordinator",
                "Failed to put new note into local storage: " << message);

            Q_UNUSED(internalErrorMessageBox(
                m_tabWidget,
                tr("Note creation in local storage has failed") +
                    QStringLiteral(": ") + message.localizedString()))
        });
}

void NoteEditorTabsAndWindowsCoordinator::setUseLimitedFonts(const bool flag)
{
    QNDEBUG(
        "widget::NoteEditorTabsAndWindowsCoordinator",
        "NoteEditorTabsAndWindowsCoordinator::setUseLimitedFonts: "
            << (flag ? "true" : "false"));

    for (int i = 0; i < m_tabWidget->count(); ++i) {
        auto * noteEditorWidget =
            qobject_cast<NoteEditorWidget *>(m_tabWidget->widget(i));

        if (Q_UNLIKELY(!noteEditorWidget)) {
            continue;
        }

        noteEditorWidget->onSetUseLimitedFonts(flag);
    }

    for (auto it = m_noteEditorWindowsByNoteLocalId.begin(),
              end = m_noteEditorWindowsByNoteLocalId.end();
         it != end; ++it)
    {
        QNTRACE(
            "widget::NoteEditorTabsAndWindowsCoordinator",
            "Examining window editor's note local id = " << it.key());

        const auto & noteEditorWidget = it.value();
        if (Q_UNLIKELY(noteEditorWidget.isNull())) {
            QNTRACE(
                "widget::NoteEditorTabsAndWindowsCoordinator",
                "The note editor widget is gone, skipping");
            continue;
        }

        noteEditorWidget->onSetUseLimitedFonts(flag);
    }
}

void NoteEditorTabsAndWindowsCoordinator::refreshNoteEditorWidgetsSpecialIcons()
{
    QNDEBUG(
        "widget::NoteEditorTabsAndWindowsCoordinator",
        "NoteEditorTabsAndWindowsCoordinator::"
        "refreshNoteEditorWidgetsSpecialIcons");

    for (int i = 0; i < m_tabWidget->count(); ++i) {
        auto * noteEditorWidget =
            qobject_cast<NoteEditorWidget *>(m_tabWidget->widget(i));
        if (Q_UNLIKELY(!noteEditorWidget)) {
            continue;
        }

        noteEditorWidget->refreshSpecialIcons();
    }

    for (auto it = m_noteEditorWindowsByNoteLocalId.begin(),
              end = m_noteEditorWindowsByNoteLocalId.end();
         it != end; ++it)
    {
        QNTRACE(
            "widget::NoteEditorTabsAndWindowsCoordinator",
            "Examining window editor's note local id = " << it.key());

        const auto & noteEditorWidget = it.value();
        if (Q_UNLIKELY(noteEditorWidget.isNull())) {
            QNTRACE(
                "widget::NoteEditorTabsAndWindowsCoordinator",
                "The note editor widget is gone, skipping");
            continue;
        }

        noteEditorWidget->refreshSpecialIcons();
    }
}

void NoteEditorTabsAndWindowsCoordinator::saveAllNoteEditorsContents()
{
    QNDEBUG(
        "widget::NoteEditorTabsAndWindowsCoordinator",
        "NoteEditorTabsAndWindowsCoordinator::saveAllNoteEditorsContents");

    ErrorString errorDescription;
    const auto noteEditorWidgets =
        m_tabWidget->findChildren<NoteEditorWidget *>();
    for (auto it = noteEditorWidgets.begin(), end = noteEditorWidgets.end();
         it != end; ++it)
    {
        auto * noteEditorWidget = *it;
        if (noteEditorWidget->noteLocalId().isEmpty()) {
            continue;
        }

        const auto status =
            noteEditorWidget->checkAndSaveModifiedNote(errorDescription);
        if (status != NoteEditorWidget::NoteSaveStatus::Ok) {
            Q_EMIT notifyError(errorDescription);
        }
    }
}

qint64 NoteEditorTabsAndWindowsCoordinator::minIdleTime() const
{
    qint64 minIdleTime = -1;
    const auto noteEditorWidgets =
        m_tabWidget->findChildren<NoteEditorWidget *>();
    for (const auto * noteEditorWidget: std::as_const(noteEditorWidgets)) {
        const qint64 idleTime = noteEditorWidget->idleTime();
        if (idleTime < 0) {
            continue;
        }

        if (minIdleTime < 0 || minIdleTime > idleTime) {
            minIdleTime = idleTime;
        }
    }

    return minIdleTime;
}

bool NoteEditorTabsAndWindowsCoordinator::eventFilter(
    QObject * watched, QEvent * event)
{
    if (Q_UNLIKELY(!event)) {
        return true;
    }

    if (event &&
        (event->type() == QEvent::Close || event->type() == QEvent::Destroy))
    {
        auto * noteEditorWidget = qobject_cast<NoteEditorWidget *>(watched);
        if (noteEditorWidget) {
            const QString noteLocalId = noteEditorWidget->noteLocalId();
            if (noteEditorWidget->isSeparateWindow()) {
                clearPersistedNoteEditorWindowGeometry(noteLocalId);
            }

            if (const auto it =
                    m_noteEditorWindowsByNoteLocalId.find(noteLocalId);
                it != m_noteEditorWindowsByNoteLocalId.end())
            {
                QNTRACE(
                    "widget::NoteEditorTabsAndWindowsCoordinator",
                    "Intercepted close of note editor window, note local id = "
                        << noteLocalId);

                bool expungeFlag = false;

                if (noteEditorWidget->isModified()) {
                    ErrorString errorDescription;
                    const auto status =
                        noteEditorWidget->checkAndSaveModifiedNote(
                            errorDescription);

                    QNDEBUG(
                        "widget::NoteEditorTabsAndWindowsCoordinator",
                        "Check and save modified note, status: "
                            << status
                            << ", error description: " << errorDescription);
                }
                else {
                    expungeFlag = shouldExpungeNote(*noteEditorWidget);
                }

                // If there were no decrypted text entries with "remember for
                // session" flag set, there's no need to keep the cache of
                // decrypted text for this note around. Otherwise will keep
                // the cache so that if this note is opened again, the cache
                // would contain the entries meant to be remembered for session.
                if (const auto cit =
                        m_decryptedTextCachesByNoteLocalId.find(noteLocalId);
                    cit != m_decryptedTextCachesByNoteLocalId.end())
                {
                    Q_ASSERT(cit.value());
                    if (!cit.value()->containsRememberedForSessionEntries()) {
                        m_decryptedTextCachesByNoteLocalId.erase(cit);
                    }
                }

                m_noteEditorWindowsByNoteLocalId.erase(it);
                persistLocalIdsOfNotesInEditorWindows();

                if (expungeFlag) {
                    noteEditorWidget->setNoteLocalId(QString{});
                    noteEditorWidget->hide();
                    noteEditorWidget->deleteLater();

                    expungeNoteSynchronously(noteLocalId);
                }
            }
        }
    }
    else if (event && event->type() == QEvent::Resize) {
        auto * noteEditorWidget = qobject_cast<NoteEditorWidget *>(watched);
        if (noteEditorWidget && noteEditorWidget->isSeparateWindow()) {
            QString noteLocalId = noteEditorWidget->noteLocalId();
            scheduleNoteEditorWindowGeometrySave(noteLocalId);
        }
    }
    else if (event && event->type() == QEvent::FocusOut) {
        auto * noteEditorWidget = qobject_cast<NoteEditorWidget *>(watched);
        if (noteEditorWidget) {
            QString noteLocalId = noteEditorWidget->noteLocalId();
            if (!noteLocalId.isEmpty()) {
                ErrorString errorDescription;
                const auto noteSaveStatus =
                    noteEditorWidget->checkAndSaveModifiedNote(
                        errorDescription);
                if (noteSaveStatus != NoteEditorWidget::NoteSaveStatus::Ok) {
                    Q_EMIT notifyError(errorDescription);
                }
            }
        }
    }

    return QObject::eventFilter(watched, event);
}

void NoteEditorTabsAndWindowsCoordinator::onNoteEditorWidgetResolved()
{
    QNDEBUG(
        "widget::NoteEditorTabsAndWindowsCoordinator",
        "NoteEditorTabsAndWindowsCoordinator::onNoteEditorWidgetResolved");

    auto * noteEditorWidget = qobject_cast<NoteEditorWidget *>(sender());
    if (Q_UNLIKELY(!noteEditorWidget)) {
        ErrorString error{
            QT_TR_NOOP("Internal error: can't resolve the added "
                       "note editor, cast failed")};

        QNWARNING("widget::NoteEditorTabsAndWindowsCoordinator", error);
        Q_EMIT notifyError(std::move(error));
        return;
    }

    QObject::disconnect(
        noteEditorWidget, &NoteEditorWidget::resolved, this,
        &NoteEditorTabsAndWindowsCoordinator::onNoteEditorWidgetResolved);

    QString noteLocalId = noteEditorWidget->noteLocalId();

    bool foundTab = false;
    for (int i = 0, count = m_tabWidget->count(); i < count; ++i) {
        auto * tabNoteEditorWidget =
            qobject_cast<NoteEditorWidget *>(m_tabWidget->widget(i));

        if (Q_UNLIKELY(!tabNoteEditorWidget)) {
            continue;
        }

        if (tabNoteEditorWidget->noteLocalId() != noteLocalId) {
            continue;
        }

        const QString displayName =
            shortenEditorName(noteEditorWidget->titleOrPreview());
        m_tabWidget->setTabText(i, displayName);

        QNTRACE(
            "widget::NoteEditorTabsAndWindowsCoordinator",
            "Updated tab text for note editor with note " << noteLocalId << ": "
                                                          << displayName);

        foundTab = true;

        if (i == m_tabWidget->currentIndex()) {
            QNTRACE(
                "widget::NoteEditorTabsAndWindowsCoordinator",
                "Setting the focus to the current tab's note editor");
            noteEditorWidget->setFocusToEditor();
        }

        break;
    }

    if (foundTab) {
        return;
    }

    const auto it = m_noteEditorWindowsByNoteLocalId.find(noteLocalId);
    if (it == m_noteEditorWindowsByNoteLocalId.end()) {
        QNTRACE(
            "widget::NoteEditorTabsAndWindowsCoordinator",
            "Couldn't find the note editor window corresponding to note local "
                << "id " << noteLocalId);
        return;
    }

    if (Q_UNLIKELY(it.value().isNull())) {
        QNTRACE(
            "widget::NoteEditorTabsAndWindowsCoordinator",
            "The note editor corresponding to note local id "
                << noteLocalId << " is gone already");
        return;
    }

    auto * windowNoteEditor = it.value().data();

    const QString displayName = shortenEditorName(
        windowNoteEditor->titleOrPreview(), gMaxWindowNameSize);

    windowNoteEditor->setWindowTitle(displayName);

    QNTRACE(
        "widget::NoteEditorTabsAndWindowsCoordinator",
        "Updated window title for note editor with note " << noteLocalId << ": "
                                                          << displayName);
}

void NoteEditorTabsAndWindowsCoordinator::onNoteEditorWidgetInvalidated()
{
    QNDEBUG(
        "widget::NoteEditorTabsAndWindowsCoordinator",
        "NoteEditorTabsAndWindowsCoordinator::onNoteEditorWidgetInvalidated");

    auto * noteEditorWidget = qobject_cast<NoteEditorWidget *>(sender());
    if (Q_UNLIKELY(!noteEditorWidget)) {
        ErrorString error{
            QT_TR_NOOP("Internal error: can't invalidate the note "
                       "editor, cast failed")};

        QNWARNING("widget::NoteEditorTabsAndWindowsCoordinator", error);
        Q_EMIT notifyError(std::move(error));
        return;
    }

    for (int i = 0, numTabs = m_tabWidget->count(); i < numTabs; ++i) {
        if (noteEditorWidget != m_tabWidget->widget(i)) {
            continue;
        }

        onNoteEditorTabCloseRequested(i);
        break;
    }
}

void NoteEditorTabsAndWindowsCoordinator::onNoteTitleOrPreviewTextChanged(
    QString titleOrPreview)
{
    QNDEBUG(
        "widget::NoteEditorTabsAndWindowsCoordinator",
        "NoteEditorTabsAndWindowsCoordinator::onNoteTitleOrPreviewTextChanged: "
            << titleOrPreview);

    auto * noteEditorWidget = qobject_cast<NoteEditorWidget *>(sender());
    if (Q_UNLIKELY(!noteEditorWidget)) {
        ErrorString error{
            QT_TR_NOOP("Internal error: can't update the note "
                       "editor's tab name, cast failed")};

        QNWARNING("widget::NoteEditorTabsAndWindowsCoordinator", error);
        Q_EMIT notifyError(std::move(error));
        return;
    }

    for (int i = 0, numTabs = m_tabWidget->count(); i < numTabs; ++i) {
        if (noteEditorWidget != m_tabWidget->widget(i)) {
            continue;
        }

        const QString tabName = shortenEditorName(titleOrPreview);
        m_tabWidget->setTabText(i, tabName);
        return;
    }

    for (auto it = m_noteEditorWindowsByNoteLocalId.begin(),
              end = m_noteEditorWindowsByNoteLocalId.end();
         it != end; ++it)
    {
        if (it.value().isNull()) {
            continue;
        }

        auto * windowNoteEditor = it.value().data();
        if (windowNoteEditor != noteEditorWidget) {
            continue;
        }

        const QString windowName =
            shortenEditorName(titleOrPreview, gMaxWindowNameSize);

        windowNoteEditor->setWindowTitle(windowName);
        return;
    }

    ErrorString error{
        QT_TR_NOOP("Internal error: can't find the note editor which "
                   "has sent the title or preview text update")};

    QNWARNING("widget::NoteEditorTabsAndWindowsCoordinator", error);
    Q_EMIT notifyError(std::move(error));
}

void NoteEditorTabsAndWindowsCoordinator::onNoteEditorTabCloseRequested(
    int tabIndex)
{
    QNDEBUG(
        "widget::NoteEditorTabsAndWindowsCoordinator",
        "NoteEditorTabsAndWindowsCoordinator::onNoteEditorTabCloseRequested: "
            << tabIndex);

    removeNoteEditorTab(tabIndex, /* close editor = */ true);
}

void NoteEditorTabsAndWindowsCoordinator::onInAppNoteLinkClicked(
    QString userId, QString shardId, QString noteGuid)
{
    QNDEBUG(
        "widget::NoteEditorTabsAndWindowsCoordinator",
        "NoteEditorTabsAndWindowsCoordinator::onInAppNoteLinkClicked: "
            << "user id = " << userId << ", shard id = " << shardId
            << ", note guid = " << noteGuid);

    QNDEBUG(
        "widget::NoteEditorTabsAndWindowsCoordinator",
        "Trying to find note in local storage by guid: " << noteGuid);

    auto canceler = setupCanceler();
    Q_ASSERT(canceler);

    auto findNoteFuture = m_localStorage->findNoteByGuid(
        noteGuid, local_storage::ILocalStorage::FetchNoteOptions{});

    auto findNoteThenFuture = threading::then(
        std::move(findNoteFuture), this,
        [this, noteGuid,
         canceler](const std::optional<qevercloud::Note> & note) {
            if (canceler->isCanceled()) {
                return;
            }

            if (!note) {
                ErrorString errorDescription{
                    QT_TR_NOOP("Failed to find note corresponding to link: "
                               "note not found")};
                QNWARNING(
                    "widget::NoteEditorTabsAndWindowsCoordinator",
                    errorDescription << ", note guid = " << noteGuid);
                return;
            }

            QNDEBUG(
                "widget::NoteEditorTabsAndWindowsCoordinator",
                "Found note targeted by in app note link: note guid = "
                    << noteGuid);

            m_noteCache.put(note->localId(), *note);
            addNote(note->localId());
        });

    threading::onFailed(
        std::move(findNoteThenFuture), this,
        [this, noteGuid, canceler = std::move(canceler)](const QException & e) {
            if (canceler->isCanceled()) {
                return;
            }

            auto message = exceptionMessage(e);

            ErrorString errorDescription{
                QT_TR_NOOP("Failed to find note corresponding to link")};
            errorDescription.appendBase(message.base());
            errorDescription.appendBase(message.additionalBases());
            errorDescription.details() = message.details();

            QNWARNING(
                "widget::NoteEditorTabsAndWindowsCoordinator",
                errorDescription << ", note guid = " << noteGuid);

            Q_EMIT notifyError(std::move(errorDescription));
        });
}

void NoteEditorTabsAndWindowsCoordinator::onNoteLoadedInEditor()
{
    QNDEBUG(
        "widget::NoteEditorTabsAndWindowsCoordinator",
        "NoteEditorTabsAndWindowsCoordinator::onNoteLoadedInEditor");
}

void NoteEditorTabsAndWindowsCoordinator::onNoteEditorError(
    ErrorString errorDescription)
{
    QNDEBUG(
        "widget::NoteEditorTabsAndWindowsCoordinator",
        "NoteEditorTabsAndWindowsCoordinator::onNoteEditorError: "
            << errorDescription);

    auto * noteEditorWidget = qobject_cast<NoteEditorWidget *>(sender());
    if (Q_UNLIKELY(!noteEditorWidget)) {
        QNWARNING(
            "widget::NoteEditorTabsAndWindowsCoordinator",
            "Received error from note editor "
                << "but can't cast the sender to NoteEditorWidget; error: "
                << errorDescription);
        Q_EMIT notifyError(errorDescription);
        return;
    }

    ErrorString error{QT_TR_NOOP("Note editor error")};
    error.appendBase(errorDescription.base());
    error.appendBase(errorDescription.additionalBases());
    error.details() = errorDescription.details();

    QString titleOrPreview = noteEditorWidget->titleOrPreview();
    if (Q_UNLIKELY(titleOrPreview.isEmpty())) {
        error.details() += QStringLiteral(", note local id ");
        error.details() += noteEditorWidget->noteLocalId();
    }
    else {
        error.details() = QStringLiteral("note \"");
        error.details() += std::move(titleOrPreview);
        error.details() += QStringLiteral("\"");
    }

    Q_EMIT notifyError(error);
}

void NoteEditorTabsAndWindowsCoordinator::onCurrentTabChanged(
    const int currentIndex)
{
    QNDEBUG(
        "widget::NoteEditorTabsAndWindowsCoordinator",
        "NoteEditorTabsAndWindowsCoordinator::onCurrentTabChanged: "
            << currentIndex);

    if (currentIndex < 0) {
        if (!m_lastCurrentTabNoteLocalId.isEmpty()) {
            m_lastCurrentTabNoteLocalId.clear();
            persistLastCurrentTabNoteLocalId();

            QNTRACE(
                "widget::NoteEditorTabsAndWindowsCoordinator",
                "Emitting last current tab note local id update to empty");
            Q_EMIT currentNoteChanged(QString());
        }

        return;
    }

    auto * noteEditorWidget =
        qobject_cast<NoteEditorWidget *>(m_tabWidget->widget(currentIndex));

    if (Q_UNLIKELY(!noteEditorWidget)) {
        QNWARNING(
            "widget::NoteEditorTabsAndWindowsCoordinator",
            "Detected current tab change in the note editor tab widget but "
            "can't cast the tab widget's to note editor");

        if (!m_lastCurrentTabNoteLocalId.isEmpty()) {
            m_lastCurrentTabNoteLocalId.clear();
            persistLastCurrentTabNoteLocalId();

            QNTRACE(
                "widget::NoteEditorTabsAndWindowsCoordinator",
                "Emitting last current tab note local id update to empty");
            Q_EMIT currentNoteChanged(QString());
        }

        return;
    }

    noteEditorWidget->setFocusToEditor();

    if (noteEditorWidget == m_blankNoteEditor) {
        QNTRACE(
            "widget::NoteEditorTabsAndWindowsCoordinator",
            "Switched to blank note editor");

        if (!m_lastCurrentTabNoteLocalId.isEmpty()) {
            m_lastCurrentTabNoteLocalId.clear();
            persistLastCurrentTabNoteLocalId();

            QNTRACE(
                "widget::NoteEditorTabsAndWindowsCoordinator",
                "Emitting last current tab note local id update to empty");
            Q_EMIT currentNoteChanged(QString());
        }

        return;
    }

    if (!m_trackingCurrentTab) {
        return;
    }

    QString currentNoteLocalId = noteEditorWidget->noteLocalId();
    if (m_lastCurrentTabNoteLocalId != currentNoteLocalId) {
        m_lastCurrentTabNoteLocalId = currentNoteLocalId;
        persistLastCurrentTabNoteLocalId();

        QNTRACE(
            "widget::NoteEditorTabsAndWindowsCoordinator",
            "Emitting last current tab note local id update to "
                << m_lastCurrentTabNoteLocalId);
        Q_EMIT currentNoteChanged(std::move(currentNoteLocalId));
    }
}

void NoteEditorTabsAndWindowsCoordinator::onTabContextMenuRequested(
    const QPoint & pos)
{
    QNDEBUG(
        "widget::NoteEditorTabsAndWindowsCoordinator",
        "NoteEditorTabsAndWindowsCoordinator::onTabContextMenuRequested, pos: "
            << "x = " << pos.x() << ", y = " << pos.y());

    const int tabIndex = m_tabWidget->tabBar()->tabAt(pos);
    if (Q_UNLIKELY(tabIndex < 0)) {
        return;
    }

    auto * noteEditorWidget =
        qobject_cast<NoteEditorWidget *>(m_tabWidget->widget(tabIndex));

    if (Q_UNLIKELY(!noteEditorWidget)) {
        ErrorString error{
            QT_TR_NOOP("Can't show the tab context menu: can't cast "
                       "the widget at the clicked tab to note editor")};

        QNWARNING(
            "widget::NoteEditorTabsAndWindowsCoordinator",
            error << ", tab index = " << tabIndex);
        Q_EMIT notifyError(std::move(error));
        return;
    }

    delete m_tabBarContextMenu;
    m_tabBarContextMenu = new QMenu(m_tabWidget);

    if (noteEditorWidget->isModified()) {
        addContextMenuAction(
            tr("Save"), *m_tabBarContextMenu, this,
            [this] { onTabContextMenuSaveNoteAction(); },
            noteEditorWidget->noteLocalId(), ActionState::Enabled);
    }

    if (m_tabWidget->count() != 1) {
        addContextMenuAction(
            tr("Open in separate window"), *m_tabBarContextMenu, this,
            [this] { onTabContextMenuMoveToWindowAction(); },
            noteEditorWidget->noteLocalId(), ActionState::Enabled);
    }

    addContextMenuAction(
        tr("Close"), *m_tabBarContextMenu, this,
        [this] { onTabContextMenuCloseEditorAction(); },
        noteEditorWidget->noteLocalId(), ActionState::Enabled);

    m_tabBarContextMenu->show();
    m_tabBarContextMenu->exec(m_tabWidget->tabBar()->mapToGlobal(pos));
}

void NoteEditorTabsAndWindowsCoordinator::onTabContextMenuCloseEditorAction()
{
    QNDEBUG(
        "widget::NoteEditorTabsAndWindowsCoordinator",
        "NoteEditorTabsAndWindowsCoordinator::"
        "onTabContextMenuCloseEditorAction");

    auto * action = qobject_cast<QAction *>(sender());
    if (Q_UNLIKELY(!action)) {
        ErrorString error(
            QT_TR_NOOP("Internal error: can't close the chosen note "
                       "editor, can't cast the slot invoker to QAction"));

        QNWARNING("widget::NoteEditorTabsAndWindowsCoordinator", error);
        Q_EMIT notifyError(error);
        return;
    }

    const QString noteLocalId = action->data().toString();
    if (Q_UNLIKELY(noteLocalId.isEmpty())) {
        ErrorString error{
            QT_TR_NOOP("Internal error: can't close the chosen "
                       "note editor, can't get the note local id "
                       "corresponding to the editor")};

        QNWARNING("widget::NoteEditorTabsAndWindowsCoordinator", error);
        Q_EMIT notifyError(std::move(error));
        return;
    }

    for (int i = 0; i < m_tabWidget->count(); ++i) {
        auto * noteEditorWidget =
            qobject_cast<NoteEditorWidget *>(m_tabWidget->widget(i));

        if (Q_UNLIKELY(!noteEditorWidget)) {
            continue;
        }

        if (noteEditorWidget->noteLocalId() == noteLocalId) {
            onNoteEditorTabCloseRequested(i);
            return;
        }
    }

    // If we got here, no target note editor widget was found
    ErrorString error{
        QT_TR_NOOP("Internal error: can't close the chosen note editor, "
                   "can't find the editor to be closed by note local id")};

    QNWARNING(
        "widget::NoteEditorTabsAndWindowsCoordinator",
        error << ", note local id = " << noteLocalId);

    Q_EMIT notifyError(std::move(error));
    return;
}

void NoteEditorTabsAndWindowsCoordinator::onTabContextMenuSaveNoteAction()
{
    QNDEBUG(
        "widget::NoteEditorTabsAndWindowsCoordinator",
        "NoteEditorTabsAndWindowsCoordinator::onTabContextMenuSaveNoteAction");

    auto * action = qobject_cast<QAction *>(sender());
    if (Q_UNLIKELY(!action)) {
        ErrorString error{
            QT_TR_NOOP("Internal error: can't save the note within "
                       "the chosen note editor, can't cast the slot "
                       "invoker to QAction")};

        QNWARNING("widget::NoteEditorTabsAndWindowsCoordinator", error);
        Q_EMIT notifyError(std::move(error));
        return;
    }

    const QString noteLocalId = action->data().toString();
    if (Q_UNLIKELY(noteLocalId.isEmpty())) {
        ErrorString error{
            QT_TR_NOOP("Internal error: can't save the note within "
                       "the chosen note editor, can't get the note "
                       "local id corresponding to the editor")};

        QNWARNING("widget::NoteEditorTabsAndWindowsCoordinator", error);
        Q_EMIT notifyError(std::move(error));
        return;
    }

    for (int i = 0; i < m_tabWidget->count(); ++i) {
        auto * noteEditorWidget =
            qobject_cast<NoteEditorWidget *>(m_tabWidget->widget(i));

        if (Q_UNLIKELY(!noteEditorWidget)) {
            continue;
        }

        if (noteEditorWidget->noteLocalId() != noteLocalId) {
            continue;
        }

        if (Q_UNLIKELY(!noteEditorWidget->isModified())) {
            QNINFO(
                "widget::NoteEditorTabsAndWindowsCoordinator",
                "The note editor widget doesn't appear to contain a note that "
                "needs saving");
            return;
        }

        ErrorString errorDescription;
        const auto res =
            noteEditorWidget->checkAndSaveModifiedNote(errorDescription);

        if (Q_UNLIKELY(res != NoteEditorWidget::NoteSaveStatus::Ok)) {
            ErrorString error{QT_TR_NOOP("Couldn't save the note")};
            error.appendBase(errorDescription.base());
            error.appendBase(errorDescription.additionalBases());
            error.details() = errorDescription.details();

            QNWARNING(
                "widget::NoteEditorTabsAndWindowsCoordinator",
                error << ", note local id = " << noteLocalId);

            Q_EMIT notifyError(std::move(error));
        }

        return;
    }

    // If we got here, no target note editor widget was found
    ErrorString error{
        QT_TR_NOOP("Internal error: can't save the note within "
                   "the chosen note editor, can't find the editor "
                   "to be closed by note local id")};

    QNWARNING(
        "widget::NoteEditorTabsAndWindowsCoordinator",
        error << ", note local id = " << noteLocalId);

    Q_EMIT notifyError(std::move(error));
    return;
}

void NoteEditorTabsAndWindowsCoordinator::onTabContextMenuMoveToWindowAction()
{
    QNDEBUG(
        "widget::NoteEditorTabsAndWindowsCoordinator",
        "NoteEditorTabsAndWindowsCoordinator::"
        "onTabContextMenuMoveToWindowAction");

    auto * action = qobject_cast<QAction *>(sender());
    if (Q_UNLIKELY(!action)) {
        ErrorString error{
            QT_TR_NOOP("Internal error: can't move the note editor tab to "
                       "window, can't cast the slot invoker to QAction")};

        QNWARNING("widget::NoteEditorTabsAndWindowsCoordinator", error);
        Q_EMIT notifyError(std::move(error));
        return;
    }

    const QString noteLocalId = action->data().toString();
    if (Q_UNLIKELY(noteLocalId.isEmpty())) {
        ErrorString error{
            QT_TR_NOOP("Internal error: can't move the note editor tab to "
                       "window, can't get the note local id corresponding to "
                       "the editor")};

        QNWARNING("widget::NoteEditorTabsAndWindowsCoordinator", error);
        Q_EMIT notifyError(error);
        return;
    }

    moveNoteEditorTabToWindow(noteLocalId);
}

void NoteEditorTabsAndWindowsCoordinator::expungeNoteFromLocalStorage()
{
    QNDEBUG(
        "widget::NoteEditorTabsAndWindowsCoordinator",
        "NoteEditorTabsAndWindowsCoordinator::expungeNoteFromLocalStorage");

    if (Q_UNLIKELY(m_localIdOfNoteToBeExpunged.isEmpty())) {
        QNWARNING(
            "widget::NoteEditorTabsAndWindowsCoordinator",
            "The local id of note to be expunged is empty");
        Q_EMIT noteExpungeFromLocalStorageFailed();
        return;
    }

    auto localId = m_localIdOfNoteToBeExpunged;
    m_localIdOfNoteToBeExpunged.clear();

    QNDEBUG(
        "widget::NoteEditorTabsAndWindowsCoordinator",
        "Expunging note: local id = " << localId);

    auto canceler = setupCanceler();
    Q_ASSERT(canceler);

    auto expungeNoteFuture = m_localStorage->expungeNoteByLocalId(localId);

    auto expungeNoteThenFuture = threading::then(
        std::move(expungeNoteFuture), this, [this, localId, canceler] {
            if (canceler->isCanceled()) {
                return;
            }

            QNDEBUG(
                "widget::NoteEditorTabsAndWindowsCoordinator",
                "Successfully expunged note from local storage: local id = "
                    << localId);

            Q_EMIT noteExpungedFromLocalStorage();
        });

    threading::onFailed(
        std::move(expungeNoteThenFuture), this,
        [this, localId = std::move(localId),
         canceler = std::move(canceler)](const QException & e) {
            if (canceler->isCanceled()) {
                return;
            }

            auto message = exceptionMessage(e);

            ErrorString errorDescription{
                QT_TR_NOOP("Failed to expunge note from local storage")};
            errorDescription.appendBase(message.base());
            errorDescription.appendBase(message.additionalBases());
            errorDescription.details() = message.details();
            QNWARNING(
                "widget::NoteEditorTabsAndWindowsCoordinator",
                errorDescription << ", note local id = " << localId);

            Q_EMIT noteExpungeFromLocalStorageFailed();
        });
}

void NoteEditorTabsAndWindowsCoordinator::timerEvent(QTimerEvent * timerEvent)
{
    QNDEBUG(
        "widget::NoteEditorTabsAndWindowsCoordinator",
        "NoteEditorTabsAndWindowsCoordinator::timerEvent: id = "
            << (timerEvent ? QString::number(timerEvent->timerId())
                           : QStringLiteral("<null>")));

    if (Q_UNLIKELY(!timerEvent)) {
        return;
    }

    const int timerId = timerEvent->timerId();
    const auto it =
        m_saveNoteEditorWindowGeometryPostponeTimerIdToNoteLocalIdBimap.right
            .find(timerId);
    if (it !=
        m_saveNoteEditorWindowGeometryPostponeTimerIdToNoteLocalIdBimap.right
            .end())
    {
        const QString noteLocalId = it->second;
        Q_UNUSED(m_saveNoteEditorWindowGeometryPostponeTimerIdToNoteLocalIdBimap
                     .right.erase(it))
        killTimer(timerId);
        persistNoteEditorWindowGeometry(noteLocalId);
    }
}

void NoteEditorTabsAndWindowsCoordinator::insertNoteEditorWidget(
    NoteEditorWidget * noteEditorWidget, const NoteEditorMode noteEditorMode)
{
    QNDEBUG(
        "widget::NoteEditorTabsAndWindowsCoordinator",
        "NoteEditorTabsAndWindowsCoordinator::insertNoteEditorWidget: "
            << noteEditorWidget->noteLocalId()
            << ", note editor mode = " << noteEditorMode);

    QObject::connect(
        noteEditorWidget, &NoteEditorWidget::titleOrPreviewChanged, this,
        &NoteEditorTabsAndWindowsCoordinator::onNoteTitleOrPreviewTextChanged);

    QObject::connect(
        noteEditorWidget, &NoteEditorWidget::resolved, this,
        &NoteEditorTabsAndWindowsCoordinator::onNoteEditorWidgetResolved);

    QObject::connect(
        noteEditorWidget, &NoteEditorWidget::invalidated, this,
        &NoteEditorTabsAndWindowsCoordinator::onNoteEditorWidgetInvalidated);

    QObject::connect(
        noteEditorWidget, &NoteEditorWidget::noteLoaded, this,
        &NoteEditorTabsAndWindowsCoordinator::onNoteLoadedInEditor);

    QObject::connect(
        noteEditorWidget, &NoteEditorWidget::inAppNoteLinkClicked, this,
        &NoteEditorTabsAndWindowsCoordinator::onInAppNoteLinkClicked);

    QObject::connect(
        noteEditorWidget, &NoteEditorWidget::notifyError, this,
        &NoteEditorTabsAndWindowsCoordinator::onNoteEditorError);

    if (noteEditorMode == NoteEditorMode::Window) {
        const QString noteLocalId = noteEditorWidget->noteLocalId();

        Q_UNUSED(noteEditorWidget->makeSeparateWindow())

        const QString displayName = shortenEditorName(
            noteEditorWidget->titleOrPreview(), gMaxWindowNameSize);

        noteEditorWidget->setWindowTitle(displayName);
        noteEditorWidget->setAttribute(Qt::WA_DeleteOnClose, true);
        noteEditorWidget->installEventFilter(this);

        if (!noteLocalId.isEmpty()) {
            m_noteEditorWindowsByNoteLocalId[noteLocalId] =
                QPointer<NoteEditorWidget>(noteEditorWidget);
            persistLocalIdsOfNotesInEditorWindows();
        }

        noteEditorWidget->show();
        return;
    }

    // If we got here, will insert the note editor as a tab

    m_localIdsOfNotesInTabbedEditors.push_back(noteEditorWidget->noteLocalId());

    QNTRACE(
        "widget::NoteEditorTabsAndWindowsCoordinator",
        "Added tabbed note local id: "
            << noteEditorWidget->noteLocalId()
            << ", the number of tabbed note local ids = "
            << m_localIdsOfNotesInTabbedEditors.size());

    persistLocalIdsOfNotesInEditorTabs();

    const QString displayName =
        shortenEditorName(noteEditorWidget->titleOrPreview());

    int tabIndex = m_tabWidget->indexOf(noteEditorWidget);
    if (tabIndex < 0) {
        tabIndex = m_tabWidget->addTab(noteEditorWidget, displayName);
    }
    else {
        m_tabWidget->setTabText(tabIndex, displayName);
    }

    m_tabWidget->setCurrentIndex(tabIndex);

    noteEditorWidget->installEventFilter(this);

    const int currentNumNotesInTabs = numNotesInTabs();
    if (currentNumNotesInTabs > 1) {
        m_tabWidget->tabBar()->show();
        m_tabWidget->setTabsClosable(true);
    }
    else {
        m_tabWidget->tabBar()->hide();
        m_tabWidget->setTabsClosable(false);
    }

    if (currentNumNotesInTabs <= m_maxNumNotesInTabs) {
        QNDEBUG(
            "widget::NoteEditorTabsAndWindowsCoordinator",
            "The addition of note "
                << noteEditorWidget->noteLocalId()
                << " doesn't cause the overflow of max allowed "
                << "number of note editor tabs");
        return;
    }

    checkAndCloseOlderNoteEditorTabs();
}

void NoteEditorTabsAndWindowsCoordinator::removeNoteEditorTab(
    int tabIndex, const bool closeEditor)
{
    QNDEBUG(
        "widget::NoteEditorTabsAndWindowsCoordinator",
        "NoteEditorTabsAndWindowsCoordinator::removeNoteEditorTab: "
            << "tab index = " << tabIndex
            << ", close editor = " << (closeEditor ? "true" : "false"));

    auto * noteEditorWidget =
        qobject_cast<NoteEditorWidget *>(m_tabWidget->widget(tabIndex));

    if (Q_UNLIKELY(!noteEditorWidget)) {
        QNWARNING(
            "widget::NoteEditorTabsAndWindowsCoordinator",
            "Detected attempt to close the note editor tab but can't cast the "
            "tab widget's tab to note editor");
        return;
    }

    if (noteEditorWidget == m_blankNoteEditor) {
        QNDEBUG(
            "widget::NoteEditorTabsAndWindowsCoordinator",
            "Silently refusing to remove the blank note editor tab");
        return;
    }

    bool expungeFlag = false;
    if (noteEditorWidget->isModified()) {
        ErrorString errorDescription;
        const auto status =
            noteEditorWidget->checkAndSaveModifiedNote(errorDescription);

        QNDEBUG(
            "widget::NoteEditorTabsAndWindowsCoordinator",
            "Check and save modified note, "
                << "status: " << status
                << ", error description: " << errorDescription);
    }
    else {
        expungeFlag = shouldExpungeNote(*noteEditorWidget);
    }

    const QString noteLocalId = noteEditorWidget->noteLocalId();

    const auto it = std::find(
        m_localIdsOfNotesInTabbedEditors.begin(),
        m_localIdsOfNotesInTabbedEditors.end(), noteLocalId);

    if (it != m_localIdsOfNotesInTabbedEditors.end()) {
        m_localIdsOfNotesInTabbedEditors.erase(it);
        QNTRACE(
            "widget::NoteEditorTabsAndWindowsCoordinator",
            "Removed note local id " << noteEditorWidget->noteLocalId());
        persistLocalIdsOfNotesInEditorTabs();
    }

    if (QuentierIsLogLevelActive(LogLevel::Trace)) {
        QString str;
        QTextStream strm{&str};
        strm << "Remaining tabbed note local ids:\n";

        for (const auto & noteLocalId:
             std::as_const(m_localIdsOfNotesInTabbedEditors))
        {
            strm << noteLocalId << "\n";
        }

        strm.flush();
        QNTRACE("widget::NoteEditorTabsAndWindowsCoordinator", str);
    }

    if (m_lastCurrentTabNoteLocalId == noteLocalId) {
        m_lastCurrentTabNoteLocalId.clear();
        persistLastCurrentTabNoteLocalId();

        QNTRACE(
            "widget::NoteEditorTabsAndWindowsCoordinator",
            "Emitting last current tab note local id update to empty");

        Q_EMIT currentNoteChanged(QString{});
    }

    if (m_tabWidget->count() == 1) {
        // That should remove the note from the editor (if any)
        noteEditorWidget->setNoteLocalId(QString{});
        m_blankNoteEditor = noteEditorWidget;

        m_tabWidget->setTabText(
            0, QString::fromUtf8(gBlankNoteKey.data(), gBlankNoteKey.size()));
        m_tabWidget->tabBar()->hide();
        m_tabWidget->setTabsClosable(false);

        if (expungeFlag) {
            expungeNoteSynchronously(noteLocalId);
        }

        return;
    }

    m_tabWidget->removeTab(tabIndex);

    if (closeEditor) {
        noteEditorWidget->hide();
        noteEditorWidget->deleteLater();
        noteEditorWidget = nullptr;
    }

    if (m_tabWidget->count() == 1) {
        m_tabWidget->tabBar()->hide();
        m_tabWidget->setTabsClosable(false);
    }

    if (expungeFlag) {
        expungeNoteSynchronously(noteLocalId);
    }
}

void NoteEditorTabsAndWindowsCoordinator::checkAndCloseOlderNoteEditorTabs()
{
    for (int i = 0; i < m_tabWidget->count(); ++i) {
        auto * noteEditorWidget =
            qobject_cast<NoteEditorWidget *>(m_tabWidget->widget(i));

        if (Q_UNLIKELY(!noteEditorWidget)) {
            continue;
        }

        if (Q_UNLIKELY(noteEditorWidget == m_blankNoteEditor)) {
            continue;
        }

        const QString & noteLocalId = noteEditorWidget->noteLocalId();

        const auto it = std::find(
            m_localIdsOfNotesInTabbedEditors.begin(),
            m_localIdsOfNotesInTabbedEditors.end(), noteLocalId);
        if (it == m_localIdsOfNotesInTabbedEditors.end()) {
            m_tabWidget->removeTab(i);
            noteEditorWidget->hide();
            noteEditorWidget->deleteLater();
        }
    }

    if (m_tabWidget->count() <= 1) {
        m_tabWidget->tabBar()->hide();
        m_tabWidget->setTabsClosable(false);
    }
    else {
        m_tabWidget->tabBar()->show();
        m_tabWidget->setTabsClosable(true);
    }
}

void NoteEditorTabsAndWindowsCoordinator::setCurrentNoteEditorWidgetTab(
    const QString & noteLocalId)
{
    QNDEBUG(
        "widget::NoteEditorTabsAndWindowsCoordinator",
        "NoteEditorTabsAndWindowsCoordinator::setCurrentNoteEditorWidgetTab: "
            << noteLocalId);

    for (int i = 0; i < m_tabWidget->count(); ++i) {
        auto * noteEditorWidget =
            qobject_cast<NoteEditorWidget *>(m_tabWidget->widget(i));

        if (Q_UNLIKELY(!noteEditorWidget)) {
            continue;
        }

        if (Q_UNLIKELY(noteEditorWidget == m_blankNoteEditor)) {
            continue;
        }

        if (noteEditorWidget->noteLocalId() == noteLocalId) {
            m_tabWidget->setCurrentIndex(i);
            break;
        }
    }
}

void NoteEditorTabsAndWindowsCoordinator::scheduleNoteEditorWindowGeometrySave(
    const QString & noteLocalId)
{
    QNDEBUG(
        "widget::NoteEditorTabsAndWindowsCoordinator",
        "NoteEditorTabsAndWindowsCoordinator::"
        "scheduleNoteEditorWindowGeometrySave: "
            << noteLocalId);

    if (Q_UNLIKELY(noteLocalId.isEmpty())) {
        return;
    }

    const auto it =
        m_saveNoteEditorWindowGeometryPostponeTimerIdToNoteLocalIdBimap.left
            .find(noteLocalId);
    if (it !=
        m_saveNoteEditorWindowGeometryPostponeTimerIdToNoteLocalIdBimap.left
            .end())
    {
        QNDEBUG(
            "widget::NoteEditorTabsAndWindowsCoordinator",
            "The timer delaying the save of this note editor window's geometry "
                << "has already been started: timer id = " << it->second);
        return;
    }

    const int timerId = startTimer(3000);
    if (Q_UNLIKELY(timerId == 0)) {
        QNWARNING(
            "widget::NoteEditorTabsAndWindowsCoordinator",
            "Failed to start timer to postpone saving the note editor window's "
            "geometry");
        return;
    }

    QNDEBUG(
        "widget::NoteEditorTabsAndWindowsCoordinator",
        "Started the timer to postpone saving the note editor window's "
            << "geometry: timer id = " << timerId
            << ", note local id = " << noteLocalId);

    m_saveNoteEditorWindowGeometryPostponeTimerIdToNoteLocalIdBimap.insert(
        NoteLocalIdToTimerIdBimap::value_type{noteLocalId, timerId});
}

void NoteEditorTabsAndWindowsCoordinator::persistNoteEditorWindowGeometry(
    const QString & noteLocalId)
{
    QNDEBUG(
        "widget::NoteEditorTabsAndWindowsCoordinator",
        "NoteEditorTabsAndWindowsCoordinator::persistNoteEditorWindowGeometry: "
            << noteLocalId);

    if (Q_UNLIKELY(noteLocalId.isEmpty())) {
        return;
    }

    const auto it = m_noteEditorWindowsByNoteLocalId.find(noteLocalId);
    if (Q_UNLIKELY(it == m_noteEditorWindowsByNoteLocalId.end())) {
        QNDEBUG(
            "widget::NoteEditorTabsAndWindowsCoordinator",
            "Can't persist note editor window geometry: could not find the "
                << "note editor window for note local id " << noteLocalId);
        return;
    }

    if (it.value().isNull()) {
        QNDEBUG(
            "widget::NoteEditorTabsAndWindowsCoordinator",
            "Can't persist the note editor window geometry: the note editor "
                << "window is gone for note local id " << noteLocalId);
        m_noteEditorWindowsByNoteLocalId.erase(it);
        return;
    }

    ApplicationSettings appSettings{
        m_currentAccount, preferences::keys::files::userInterface};

    appSettings.beginGroup(preferences::keys::noteEditorGroup);

    appSettings.setValue(
        QString::fromUtf8(
            gNoteEditorWindowGeometryKeyPrefix.data(),
            gNoteEditorWindowGeometryKeyPrefix.size()) +
            noteLocalId,
        it.value().data()->saveGeometry());

    appSettings.endGroup();
}

void NoteEditorTabsAndWindowsCoordinator::
    clearPersistedNoteEditorWindowGeometry(const QString & noteLocalId)
{
    QNDEBUG(
        "widget::NoteEditorTabsAndWindowsCoordinator",
        "NoteEditorTabsAndWindowsCoordinator::"
        "clearPersistedNoteEditorWindowGeometry: "
            << noteLocalId);

    if (Q_UNLIKELY(noteLocalId.isEmpty())) {
        return;
    }

    ApplicationSettings appSettings{
        m_currentAccount, preferences::keys::files::userInterface};

    appSettings.beginGroup(preferences::keys::noteEditorGroup);
    appSettings.remove(
        QString::fromUtf8(
            gNoteEditorWindowGeometryKeyPrefix.data(),
            gNoteEditorWindowGeometryKeyPrefix.size()) +
        noteLocalId);
    appSettings.endGroup();
}

void NoteEditorTabsAndWindowsCoordinator::restoreNoteEditorWindowGeometry(
    const QString & noteLocalId)
{
    QNDEBUG(
        "widget::NoteEditorTabsAndWindowsCoordinator",
        "NoteEditorTabsAndWindowsCoordinator::restoreNoteEditorWindowGeometry: "
            << noteLocalId);

    if (Q_UNLIKELY(noteLocalId.isEmpty())) {
        return;
    }

    const auto it = m_noteEditorWindowsByNoteLocalId.find(noteLocalId);
    if (Q_UNLIKELY(it == m_noteEditorWindowsByNoteLocalId.end())) {
        QNDEBUG(
            "widget::NoteEditorTabsAndWindowsCoordinator",
            "Can't restore note editor window's geometry: could not find the "
                << "note editor window for note local id " << noteLocalId);
        return;
    }

    if (it.value().isNull()) {
        QNDEBUG(
            "widget::NoteEditorTabsAndWindowsCoordinator",
            "Can't restore the note editor window geometry: the note editor "
                << "window is gone for note local id " << noteLocalId);
        m_noteEditorWindowsByNoteLocalId.erase(it);
        clearPersistedNoteEditorWindowGeometry(noteLocalId);
        return;
    }

    ApplicationSettings appSettings{
        m_currentAccount, preferences::keys::files::userInterface};

    appSettings.beginGroup(preferences::keys::noteEditorGroup);

    QByteArray noteEditorWindowGeometry =
        appSettings
            .value(
                QString::fromUtf8(
                    gNoteEditorWindowGeometryKeyPrefix.data(),
                    gNoteEditorWindowGeometryKeyPrefix.size()) +
                noteLocalId)
            .toByteArray();

    appSettings.endGroup();

    if (!it.value().data()->restoreGeometry(noteEditorWindowGeometry)) {
        QNDEBUG(
            "widget::NoteEditorTabsAndWindowsCoordinator",
            "Could not restore the geometry for note editor window with note "
                << "local id " << noteLocalId);
        return;
    }

    QNTRACE(
        "widget::NoteEditorTabsAndWindowsCoordinator",
        "Restored the geometry for note editor window with note local id "
            << noteLocalId);
}

void NoteEditorTabsAndWindowsCoordinator::
    connectNoteEditorWidgetToColorChangeSignals(NoteEditorWidget & widget)
{
    QObject::connect(
        this, &NoteEditorTabsAndWindowsCoordinator::noteEditorFontColorChanged,
        &widget, &NoteEditorWidget::onNoteEditorFontColorChanged,
        Qt::ConnectionType(Qt::UniqueConnection | Qt::QueuedConnection));

    QObject::connect(
        this,
        &NoteEditorTabsAndWindowsCoordinator::noteEditorBackgroundColorChanged,
        &widget, &NoteEditorWidget::onNoteEditorBackgroundColorChanged,
        Qt::ConnectionType(Qt::UniqueConnection | Qt::QueuedConnection));

    QObject::connect(
        this,
        &NoteEditorTabsAndWindowsCoordinator::noteEditorHighlightColorChanged,
        &widget, &NoteEditorWidget::onNoteEditorHighlightColorChanged,
        Qt::ConnectionType(Qt::UniqueConnection | Qt::QueuedConnection));

    QObject::connect(
        this,
        &NoteEditorTabsAndWindowsCoordinator::
            noteEditorHighlightedTextColorChanged,
        &widget, &NoteEditorWidget::onNoteEditorHighlightedTextColorChanged,
        Qt::ConnectionType(Qt::UniqueConnection | Qt::QueuedConnection));

    QObject::connect(
        this, &NoteEditorTabsAndWindowsCoordinator::noteEditorColorsReset,
        &widget, &NoteEditorWidget::onNoteEditorColorsReset,
        Qt::ConnectionType(Qt::UniqueConnection | Qt::QueuedConnection));
}

void NoteEditorTabsAndWindowsCoordinator::setupFileIO()
{
    QNDEBUG(
        "widget::NoteEditorTabsAndWindowsCoordinator",
        "NoteEditorTabsAndWindowsCoordinator::setupFileIO");

    if (!m_ioThread) {
        m_ioThread = new QThread;
        m_ioThread->setObjectName(QStringLiteral("NoteEditorsIOThread"));

        QObject::connect(
            m_ioThread, &QThread::finished, m_ioThread, &QThread::deleteLater);

        m_ioThread->start(QThread::LowPriority);

        QNDEBUG(
            "widget::NoteEditorTabsAndWindowsCoordinator",
            "Started background thread for note editors' IO: "
                << m_ioThread->objectName());
    }

    if (!m_fileIOProcessorAsync) {
        m_fileIOProcessorAsync = new FileIOProcessorAsync;
        m_fileIOProcessorAsync->moveToThread(m_ioThread);
    }
}

void NoteEditorTabsAndWindowsCoordinator::setupSpellChecker()
{
    QNDEBUG(
        "widget::NoteEditorTabsAndWindowsCoordinator",
        "NoteEditorTabsAndWindowsCoordinator::setupSpellChecker");

    if (!m_spellChecker) {
        m_spellChecker =
            new SpellChecker{m_fileIOProcessorAsync, m_currentAccount, this};
    }
}

QString NoteEditorTabsAndWindowsCoordinator::shortenEditorName(
    const QString & name, int maxSize) const
{
    if (maxSize < 0) {
        constexpr int maxTabNameSize = 10;
        maxSize = maxTabNameSize;
    }

    if (name.size() <= maxSize) {
        return name;
    }

    QString result = name.simplified();
    result.truncate(std::max(maxSize - 3, 0));
    result += QStringLiteral("...");
    return result;
}

void NoteEditorTabsAndWindowsCoordinator::moveNoteEditorTabToWindow(
    const QString & noteLocalId)
{
    QNDEBUG(
        "widget::NoteEditorTabsAndWindowsCoordinator",
        "NoteEditorTabsAndWindowsCoordinator::moveNoteEditorTabToWindow: "
            << "note local id = " << noteLocalId);

    for (int i = 0; i < m_tabWidget->count(); ++i) {
        auto * noteEditorWidget =
            qobject_cast<NoteEditorWidget *>(m_tabWidget->widget(i));

        if (Q_UNLIKELY(!noteEditorWidget)) {
            continue;
        }

        if (noteEditorWidget->noteLocalId() != noteLocalId) {
            continue;
        }

        removeNoteEditorTab(i, /* close editor = */ false);
        insertNoteEditorWidget(noteEditorWidget, NoteEditorMode::Window);
        return;
    }

    ErrorString error{
        QT_TR_NOOP("Could not find the note editor widget to be "
                   "moved from tab to window")};

    error.details() = noteLocalId;
    QNWARNING("widget::NoteEditorTabsAndWindowsCoordinator", error);
    Q_EMIT notifyError(std::move(error));
}

void NoteEditorTabsAndWindowsCoordinator::moveNoteEditorWindowToTab(
    NoteEditorWidget * noteEditorWidget)
{
    QNDEBUG(
        "widget::NoteEditorTabsAndWindowsCoordinator",
        "NoteEditorTabsAndWindowsCoordinator::moveNoteEditorWindowToTab: "
            << "note local id = "
            << (noteEditorWidget ? noteEditorWidget->noteLocalId()
                                 : QStringLiteral("<null>")));

    if (Q_UNLIKELY(!noteEditorWidget)) {
        return;
    }

    noteEditorWidget->setAttribute(Qt::WA_DeleteOnClose, false);
    Q_UNUSED(noteEditorWidget->makeNonWindow())

    insertNoteEditorWidget(noteEditorWidget, NoteEditorMode::Tab);
    noteEditorWidget->show();
}

void NoteEditorTabsAndWindowsCoordinator::persistLocalIdsOfNotesInEditorTabs()
{
    QNDEBUG(
        "widget::NoteEditorTabsAndWindowsCoordinator",
        "NoteEditorTabsAndWindowsCoordinator::"
        "persistLocalIdsOfNotesInEditorTabs");

    QStringList openNotesLocalIds;
    openNotesLocalIds.reserve(static_cast<QStringList::size_type>(
        m_localIdsOfNotesInTabbedEditors.size()));
    for (const auto & noteLocalId:
         std::as_const(m_localIdsOfNotesInTabbedEditors))
    {
        openNotesLocalIds << noteLocalId;
    }

    ApplicationSettings appSettings{
        m_currentAccount, preferences::keys::files::userInterface};

    appSettings.beginGroup(preferences::keys::noteEditorGroup);
    appSettings.setValue(gOpenNotesLocalIdsInTabsKey, openNotesLocalIds);
    appSettings.endGroup();
}

void NoteEditorTabsAndWindowsCoordinator::
    persistLocalIdsOfNotesInEditorWindows()
{
    QNDEBUG(
        "widget::NoteEditorTabsAndWindowsCoordinator",
        "NoteEditorTabsAndWindowsCoordinator::"
        "persistLocalIdsOfNotesInEditorWindows");

    QStringList openNotesLocalIds;
    openNotesLocalIds.reserve(m_noteEditorWindowsByNoteLocalId.size());

    for (auto it = m_noteEditorWindowsByNoteLocalId.constBegin(),
              end = m_noteEditorWindowsByNoteLocalId.constEnd();
         it != end; ++it)
    {
        if (Q_UNLIKELY(it.value().isNull())) {
            continue;
        }

        openNotesLocalIds << it.key();
    }

    ApplicationSettings appSettings{
        m_currentAccount, preferences::keys::files::userInterface};

    appSettings.beginGroup(preferences::keys::noteEditorGroup);
    appSettings.setValue(gOpenNotesLocalIdsInWindowsKey, openNotesLocalIds);
    appSettings.endGroup();
}

void NoteEditorTabsAndWindowsCoordinator::persistLastCurrentTabNoteLocalId()
{
    QNDEBUG(
        "widget::NoteEditorTabsAndWindowsCoordinator",
        "NoteEditorTabsAndWindowsCoordinator::"
        "persistLastCurrentTabNoteLocalId: "
            << m_lastCurrentTabNoteLocalId);

    ApplicationSettings appSettings{
        m_currentAccount, preferences::keys::files::userInterface};

    appSettings.beginGroup(preferences::keys::noteEditorGroup);
    appSettings.setValue(gLastCurrentTabLocalId, m_lastCurrentTabNoteLocalId);
    appSettings.endGroup();
}

void NoteEditorTabsAndWindowsCoordinator::restoreLastOpenNotes()
{
    QNDEBUG(
        "widget::NoteEditorTabsAndWindowsCoordinator",
        "NoteEditorTabsAndWindowsCoordinator::restoreLastOpenNotes");

    ApplicationSettings appSettings{
        m_currentAccount, preferences::keys::files::userInterface};

    appSettings.beginGroup(preferences::keys::noteEditorGroup);

    QStringList localIdsOfLastNotesInTabs =
        appSettings.value(gOpenNotesLocalIdsInTabsKey).toStringList();

    QStringList localIdsOfLastNotesInWindows =
        appSettings.value(gOpenNotesLocalIdsInWindowsKey).toStringList();

    m_lastCurrentTabNoteLocalId =
        appSettings.value(gLastCurrentTabLocalId).toString();

    QNDEBUG(
        "widget::NoteEditorTabsAndWindowsCoordinator",
        "Last current tab note local id: " << m_lastCurrentTabNoteLocalId);

    appSettings.endGroup();

    m_trackingCurrentTab = false;

    for (const auto & noteLocalId: std::as_const(localIdsOfLastNotesInTabs)) {
        addNote(noteLocalId, NoteEditorMode::Tab);
    }

    for (const auto & noteLocalId: std::as_const(localIdsOfLastNotesInWindows))
    {
        addNote(noteLocalId, NoteEditorMode::Window);
        restoreNoteEditorWindowGeometry(noteLocalId);
    }

    m_trackingCurrentTab = true;

    if (m_lastCurrentTabNoteLocalId.isEmpty()) {
        auto * currentNoteEditorWidget =
            qobject_cast<NoteEditorWidget *>(m_tabWidget->currentWidget());

        if (currentNoteEditorWidget &&
            currentNoteEditorWidget != m_blankNoteEditor)
        {
            m_lastCurrentTabNoteLocalId =
                currentNoteEditorWidget->noteLocalId();

            persistLastCurrentTabNoteLocalId();
        }
    }
    else {
        for (int i = 0, count = m_tabWidget->count(); i < count; ++i) {
            auto * noteEditorWidget =
                qobject_cast<NoteEditorWidget *>(m_tabWidget->widget(i));

            if (Q_UNLIKELY(!noteEditorWidget)) {
                QNWARNING(
                    "widget::NoteEditorTabsAndWindowsCoordinator",
                    "Found a widget not being NoteEditorWidget in one of note "
                    "editor tabs");
                continue;
            }

            const QString noteLocalId = noteEditorWidget->noteLocalId();
            if (noteLocalId == m_lastCurrentTabNoteLocalId) {
                m_tabWidget->setCurrentIndex(i);
                break;
            }
        }
    }

    return;
}

bool NoteEditorTabsAndWindowsCoordinator::shouldExpungeNote(
    const NoteEditorWidget & noteEditorWidget) const
{
    QNDEBUG(
        "widget::NoteEditorTabsAndWindowsCoordinator",
        "NoteEditorTabsAndWindowsCoordinator::shouldExpungeNote");

    if (!noteEditorWidget.isNewNote()) {
        QNDEBUG(
            "widget::NoteEditorTabsAndWindowsCoordinator",
            "The note within the editor was not a new one, should not expunge "
            "it");
        return false;
    }

    if (noteEditorWidget.hasBeenModified()) {
        QNDEBUG(
            "widget::NoteEditorTabsAndWindowsCoordinator",
            "The note within the editor was modified while having been loaded "
            "into the editor");
        return false;
    }

    const auto & note = noteEditorWidget.currentNote();
    if (!note) {
        QNDEBUG(
            "widget::NoteEditorTabsAndWindowsCoordinator",
            "There is no note within the editor");
        return false;
    }

    ApplicationSettings appSettings{
        m_currentAccount, preferences::keys::files::userInterface};

    appSettings.beginGroup(preferences::keys::noteEditorGroup);

    QVariant removeEmptyNotesData =
        appSettings.value(preferences::keys::removeEmptyUneditedNotes);

    appSettings.endGroup();

    bool removeEmptyNotes = preferences::defaults::removeEmptyUneditedNotes;

    if (removeEmptyNotesData.isValid()) {
        removeEmptyNotes = removeEmptyNotesData.toBool();
        QNDEBUG(
            "widget::NoteEditorTabsAndWindowsCoordinator",
            "Remove empty notes setting: "
                << (removeEmptyNotes ? "true" : "false"));
    }

    if (removeEmptyNotes) {
        QNDEBUG(
            "widget::NoteEditorTabsAndWindowsCoordinator",
            "Will remove empty unedited note with local id "
                << note->localId());
        return true;
    }

    QNDEBUG(
        "widget::NoteEditorTabsAndWindowsCoordinator",
        "Won't remove the empty note due to setting");
    return false;
}

void NoteEditorTabsAndWindowsCoordinator::expungeNoteSynchronously(
    const QString & noteLocalId)
{
    QNDEBUG(
        "widget::NoteEditorTabsAndWindowsCoordinator",
        "NoteEditorTabsAndWindowsCoordinator::expungeNoteSynchronously: "
            << noteLocalId);

    ApplicationSettings appSettings{
        m_currentAccount, preferences::keys::files::userInterface};

    appSettings.beginGroup(preferences::keys::noteEditorGroup);

    const QVariant expungeNoteTimeoutData =
        appSettings.value(preferences::keys::noteEditorExpungeNoteTimeout);

    appSettings.endGroup();

    bool conversionResult = false;
    int expungeNoteTimeout = expungeNoteTimeoutData.toInt(&conversionResult);
    if (Q_UNLIKELY(!conversionResult)) {
        QNDEBUG(
            "widget::NoteEditorTabsAndWindowsCoordinator",
            "Can't read the timeout for note expunging from the local storage, "
                << "fallback to the default value of "
                << preferences::defaults::expungeNoteTimeout
                << " milliseconds");
        expungeNoteTimeout = preferences::defaults::expungeNoteTimeout;
    }
    else {
        expungeNoteTimeout = std::max(expungeNoteTimeout, 100);
    }

    if (m_expungeNoteDeadlineTimer) {
        m_expungeNoteDeadlineTimer->deleteLater();
    }

    m_expungeNoteDeadlineTimer = new QTimer{this};
    m_expungeNoteDeadlineTimer->setSingleShot(true);

    EventLoopWithExitStatus eventLoop;

    QObject::connect(
        m_expungeNoteDeadlineTimer, &QTimer::timeout, &eventLoop,
        &EventLoopWithExitStatus::exitAsTimeout);

    QObject::connect(
        this,
        &NoteEditorTabsAndWindowsCoordinator::noteExpungedFromLocalStorage,
        &eventLoop, &EventLoopWithExitStatus::exitAsSuccess);

    QObject::connect(
        this,
        &NoteEditorTabsAndWindowsCoordinator::noteExpungeFromLocalStorageFailed,
        &eventLoop, &EventLoopWithExitStatus::exitAsFailure);

    m_expungeNoteDeadlineTimer->start(expungeNoteTimeout);

    m_localIdOfNoteToBeExpunged = noteLocalId;
    QTimer::singleShot(0, this, SLOT(expungeNoteFromLocalStorage()));

    Q_UNUSED(eventLoop.exec(QEventLoop::ExcludeUserInputEvents))
    auto status = eventLoop.exitStatus();

    if (status == EventLoopWithExitStatus::ExitStatus::Failure) {
        QNWARNING(
            "widget::NoteEditorTabsAndWindowsCoordinator",
            "Failed to expunge the empty unedited note from the local storage");
    }
    else if (status == EventLoopWithExitStatus::ExitStatus::Timeout) {
        QNWARNING(
            "widget::NoteEditorTabsAndWindowsCoordinator",
            "The expunging of note from local storage took too much time to "
            "finish");
    }
    else {
        QNDEBUG(
            "widget::NoteEditorTabsAndWindowsCoordinator",
            "Successfully expunged the note from the local storage");
    }
}

utility::cancelers::ICancelerPtr
NoteEditorTabsAndWindowsCoordinator::setupCanceler()
{
    if (!m_canceler) {
        m_canceler = std::make_shared<utility::cancelers::ManualCanceler>();
    }

    return m_canceler;
}

QDebug & operator<<(
    QDebug & dbg,
    const NoteEditorTabsAndWindowsCoordinator::NoteEditorMode mode)
{
    using NoteEditorMode = NoteEditorTabsAndWindowsCoordinator::NoteEditorMode;

    switch (mode) {
    case NoteEditorMode::Tab:
        dbg << "Tab";
        break;
    case NoteEditorMode::Window:
        dbg << "Window";
        break;
    case NoteEditorMode::Any:
        dbg << "Any";
        break;
    }

    return dbg;
}

} // namespace quentier
