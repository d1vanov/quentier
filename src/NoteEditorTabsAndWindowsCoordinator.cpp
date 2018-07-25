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

#include "NoteEditorTabsAndWindowsCoordinator.h"
#include "SettingsNames.h"
#include "DefaultSettings.h"
#include "models/TagModel.h"
#include "widgets/NoteEditorWidget.h"
#include "widgets/TabWidget.h"
#include <quentier/logging/QuentierLogger.h>
#include <quentier/utility/ApplicationSettings.h>
#include <quentier/utility/MessageBox.h>
#include <quentier/utility/FileIOProcessorAsync.h>
#include <quentier/utility/EventLoopWithExitStatus.h>
#include <quentier/note_editor/SpellChecker.h>
#include <quentier/note_editor/NoteEditor.h>
#include <QUndoStack>
#include <QTabBar>
#include <QCloseEvent>
#include <QThread>
#include <QMenu>
#include <QContextMenuEvent>
#include <QKeyEvent>
#include <QEvent>
#include <QTimerEvent>
#include <QTimer>
#include <QApplication>
#include <algorithm>

#define DEFAULT_MAX_NUM_NOTES_IN_TABS (5)
#define MIN_NUM_NOTES_IN_TABS (1)

#define BLANK_NOTE_KEY QStringLiteral("BlankNoteId")
#define MAX_TAB_NAME_SIZE (10)
#define MAX_WINDOW_NAME_SIZE (120)

#define OPEN_NOTES_LOCAL_UIDS_IN_TABS_SETTINGS_KEY QStringLiteral("LocalUidsOfNotesLastOpenInNoteEditorTabs")
#define OPEN_NOTES_LOCAL_UIDS_IN_WINDOWS_SETTINGS_KEY QStringLiteral("LocalUidsOfNotesLastOpenInNoteEditorWindows")
#define LAST_CURRENT_TAB_NOTE_LOCAL_UID QStringLiteral("LastCurrentTabNoteLocalUid")

#define NOTE_EDITOR_WINDOW_GEOMETRY_KEY_PREFIX QStringLiteral("NoteEditorWindowGeometry_")

#define PERSIST_NOTE_EDITOR_WINDOW_GEOMETRY_DELAY (3000)

namespace quentier {

NoteEditorTabsAndWindowsCoordinator::NoteEditorTabsAndWindowsCoordinator(const Account & account, LocalStorageManagerAsync & localStorageManagerAsync,
                                                                         NoteCache & noteCache, NotebookCache & notebookCache,
                                                                         TagCache & tagCache, TagModel & tagModel,
                                                                         TabWidget * tabWidget, QObject * parent) :
    QObject(parent),
    m_currentAccount(account),
    m_localStorageManagerAsync(localStorageManagerAsync),
    m_noteCache(noteCache),
    m_notebookCache(notebookCache),
    m_tagCache(tagCache),
    m_pTagModel(&tagModel),
    m_pTabWidget(tabWidget),
    m_pBlankNoteEditor(Q_NULLPTR),
    m_pIOThread(Q_NULLPTR),
    m_pFileIOProcessorAsync(Q_NULLPTR),
    m_pSpellChecker(Q_NULLPTR),
    m_connectedToLocalStorage(false),
    m_maxNumNotesInTabs(-1),
    m_localUidsOfNotesInTabbedEditors(),
    m_lastCurrentTabNoteLocalUid(),
    m_noteEditorWindowsByNoteLocalUid(),
    m_saveNoteEditorWindowGeometryPostponeTimerIdToNoteLocalUidBimap(),
    m_noteEditorModeByCreateNoteRequestIds(),
    m_expungeNoteRequestIds(),
    m_inAppNoteLinkFindNoteRequestIds(),
    m_pTabBarContextMenu(Q_NULLPTR),
    m_localUidOfNoteToBeExpunged(),
    m_pExpungeNoteDeadlineTimer(Q_NULLPTR),
    m_trackingCurrentTab(true)
{
    ApplicationSettings appSettings(m_currentAccount, QUENTIER_UI_SETTINGS);
    appSettings.beginGroup(NOTE_EDITOR_SETTINGS_GROUP_NAME);
    QVariant maxNumNoteTabsData = appSettings.value(QStringLiteral("MaxNumNoteTabs"));
    appSettings.endGroup();

    bool conversionResult = false;
    int maxNumNoteTabs = maxNumNoteTabsData.toInt(&conversionResult);
    if (!conversionResult) {
        QNDEBUG(QStringLiteral("NoteEditorTabsAndWindowsCoordinator: no persisted max num note tabs setting, "
                               "fallback to the default value of ") << DEFAULT_MAX_NUM_NOTES_IN_TABS);
        m_maxNumNotesInTabs = DEFAULT_MAX_NUM_NOTES_IN_TABS;
    }
    else {
        QNDEBUG(QStringLiteral("NoteEditorTabsAndWindowsCoordinator: max num note tabs: ") << maxNumNoteTabs);
        m_maxNumNotesInTabs = maxNumNoteTabs;
    }

    m_localUidsOfNotesInTabbedEditors.set_capacity(static_cast<size_t>(std::max(m_maxNumNotesInTabs, MIN_NUM_NOTES_IN_TABS)));
    QNTRACE(QStringLiteral("Tabbed note local uids circular buffer capacity: ") << m_localUidsOfNotesInTabbedEditors.capacity());

    setupFileIO();
    setupSpellChecker();

    QUndoStack * pUndoStack = new QUndoStack;
    m_pBlankNoteEditor = new NoteEditorWidget(m_currentAccount, m_localStorageManagerAsync,
                                              *m_pFileIOProcessorAsync, *m_pSpellChecker,
                                              m_noteCache, m_notebookCache, m_tagCache,
                                              *m_pTagModel, pUndoStack, m_pTabWidget);
    pUndoStack->setParent(m_pBlankNoteEditor);

    Q_UNUSED(m_pTabWidget->addTab(m_pBlankNoteEditor, BLANK_NOTE_KEY))

    QTabBar * pTabBar = m_pTabWidget->tabBar();
    pTabBar->hide();
    pTabBar->setContextMenuPolicy(Qt::CustomContextMenu);
    QObject::connect(pTabBar, QNSIGNAL(QTabBar,customContextMenuRequested,QPoint),
                     this, QNSLOT(NoteEditorTabsAndWindowsCoordinator,onTabContextMenuRequested,QPoint));

    restoreLastOpenNotes();

    QObject::connect(m_pTabWidget, QNSIGNAL(TabWidget,tabCloseRequested,int),
                     this, QNSLOT(NoteEditorTabsAndWindowsCoordinator,onNoteEditorTabCloseRequested,int));
    QObject::connect(m_pTabWidget, QNSIGNAL(TabWidget,currentChanged,int),
                     this, QNSLOT(NoteEditorTabsAndWindowsCoordinator,onCurrentTabChanged,int));

    NoteEditorWidget * pCurrentEditor = qobject_cast<NoteEditorWidget*>(m_pTabWidget->currentWidget());
    if (pCurrentEditor)
    {
        if (pCurrentEditor->hasFocus()) {
            QNDEBUG(QStringLiteral("The tab widget's current tab widget already has focus"));
            return;
        }

        QNDEBUG(QStringLiteral("Setting the focus to the current tab widget"));
        pCurrentEditor->setFocusToEditor();
    }
}

NoteEditorTabsAndWindowsCoordinator::~NoteEditorTabsAndWindowsCoordinator()
{
    if (m_pIOThread) {
        m_pIOThread->quit();
    }
}

void NoteEditorTabsAndWindowsCoordinator::clear()
{
    QNDEBUG(QStringLiteral("NoteEditorTabsAndWindowsCoordinator::clear: num tabs = ")
            << m_pTabWidget->count() << QStringLiteral(", num windows = ")
            << m_noteEditorWindowsByNoteLocalUid.size());

    m_pBlankNoteEditor = Q_NULLPTR;

    // Prevent currentChanged signal from tabs removal inside this method to mess with last current tab note local uid
    QObject::disconnect(m_pTabWidget, QNSIGNAL(TabWidget,currentChanged,int),
                        this, QNSLOT(NoteEditorTabsAndWindowsCoordinator,onCurrentTabChanged,int));

    while(m_pTabWidget->count() != 0)
    {
        NoteEditorWidget * pNoteEditorWidget = qobject_cast<NoteEditorWidget*>(m_pTabWidget->widget(0));
        if (Q_UNLIKELY(!pNoteEditorWidget))
        {
            QNWARNING(QStringLiteral("Detected some widget other than NoteEditorWidget within the tab widget"));
            QWidget * pWidget = m_pTabWidget->widget(0);
            m_pTabWidget->removeTab(0);
            if (pWidget) {
                pWidget->hide();
                pWidget->deleteLater();
            }

            continue;
        }

        QString noteLocalUid = pNoteEditorWidget->noteLocalUid();
        QNTRACE(QStringLiteral("Safely closing note editor tab: ") << noteLocalUid);

        ErrorString errorDescription;
        NoteEditorWidget::NoteSaveStatus::type res = pNoteEditorWidget->checkAndSaveModifiedNote(errorDescription);
        if (Q_UNLIKELY(res != NoteEditorWidget::NoteSaveStatus::Ok)) {
            QNINFO(QStringLiteral("Could not save note: ") << pNoteEditorWidget->noteLocalUid()
                   << QStringLiteral(", status: ") << res << QStringLiteral(", error: ") << errorDescription);
        }

        m_pTabWidget->removeTab(0);

        pNoteEditorWidget->removeEventFilter(this);
        pNoteEditorWidget->hide();
        pNoteEditorWidget->deleteLater();

        QNTRACE(QStringLiteral("Removed note editor tab: ") << noteLocalUid);
    }

    while(!m_noteEditorWindowsByNoteLocalUid.isEmpty())
    {
        auto it = m_noteEditorWindowsByNoteLocalUid.begin();
        if (Q_UNLIKELY(it.value().isNull())) {
            Q_UNUSED(m_noteEditorWindowsByNoteLocalUid.erase(it))
            continue;
        }

        NoteEditorWidget * pNoteEditorWidget = it.value().data();

        QString noteLocalUid = pNoteEditorWidget->noteLocalUid();
        QNTRACE(QStringLiteral("Safely closing note editor window: ") << noteLocalUid);

        ErrorString errorDescription;
        NoteEditorWidget::NoteSaveStatus::type res = pNoteEditorWidget->checkAndSaveModifiedNote(errorDescription);
        if (Q_UNLIKELY(res != NoteEditorWidget::NoteSaveStatus::Ok)) {
            QNINFO(QStringLiteral("Could not save note: ") << pNoteEditorWidget->noteLocalUid()
                   << QStringLiteral(", status: ") << res << QStringLiteral(", error: ") << errorDescription);
        }

        pNoteEditorWidget->removeEventFilter(this);
        pNoteEditorWidget->hide();
        pNoteEditorWidget->deleteLater();
        Q_UNUSED(m_noteEditorWindowsByNoteLocalUid.erase(it))

        QNTRACE(QStringLiteral("Closed note editor window: ") << noteLocalUid);
    }

    m_localUidsOfNotesInTabbedEditors.clear();
    m_lastCurrentTabNoteLocalUid.clear();
    m_noteEditorWindowsByNoteLocalUid.clear();

    for(auto it = m_saveNoteEditorWindowGeometryPostponeTimerIdToNoteLocalUidBimap.left.begin(),
        end = m_saveNoteEditorWindowGeometryPostponeTimerIdToNoteLocalUidBimap.left.end(); it != end; ++it)
    {
        killTimer(it->second);
    }

    m_saveNoteEditorWindowGeometryPostponeTimerIdToNoteLocalUidBimap.clear();

    m_noteEditorModeByCreateNoteRequestIds.clear();
    m_expungeNoteRequestIds.clear();
    m_inAppNoteLinkFindNoteRequestIds.clear();

    m_localUidOfNoteToBeExpunged.clear();
}

void NoteEditorTabsAndWindowsCoordinator::switchAccount(const Account & account, TagModel & tagModel)
{
    QNDEBUG(QStringLiteral("NoteEditorTabsAndWindowsCoordinator::switchAccount: ") << account);

    if (account == m_currentAccount) {
        QNDEBUG(QStringLiteral("The account has not changed, nothing to do"));
        return;
    }

    clear();

    m_pTagModel = QPointer<TagModel>(&tagModel);
    m_currentAccount = account;

    restoreLastOpenNotes();
}

void NoteEditorTabsAndWindowsCoordinator::setMaxNumNotesInTabs(const int maxNumNotesInTabs)
{
    QNDEBUG(QStringLiteral("NoteEditorTabsAndWindowsCoordinator::setMaxNumNotesInTabs: ") << maxNumNotesInTabs);

    if (m_maxNumNotesInTabs == maxNumNotesInTabs) {
        QNDEBUG(QStringLiteral("Max number of notes in tabs hasn't changed"));
        return;
    }

    if (m_maxNumNotesInTabs < maxNumNotesInTabs) {
        m_maxNumNotesInTabs = maxNumNotesInTabs;
        QNDEBUG(QStringLiteral("Max number of notes in tabs has been increased to ") << maxNumNotesInTabs);
        return;
    }

    int currentNumNotesInTabs = numNotesInTabs();

    m_maxNumNotesInTabs = maxNumNotesInTabs;
    QNDEBUG(QStringLiteral("Max number of notes in tabs has been decreased to ") << maxNumNotesInTabs);

    if (currentNumNotesInTabs <= maxNumNotesInTabs) {
        return;
    }

    m_localUidsOfNotesInTabbedEditors.set_capacity(static_cast<size_t>(std::max(maxNumNotesInTabs, MIN_NUM_NOTES_IN_TABS)));
    QNTRACE(QStringLiteral("Tabbed note local uids circular buffer capacity: ") << m_localUidsOfNotesInTabbedEditors.capacity());
    checkAndCloseOlderNoteEditorTabs();
}

int NoteEditorTabsAndWindowsCoordinator::numNotesInTabs() const
{
    if (Q_UNLIKELY(!m_pTabWidget)) {
        return 0;
    }

    if (m_pBlankNoteEditor) {
        return 0;
    }

    return m_pTabWidget->count();
}

NoteEditorWidget * NoteEditorTabsAndWindowsCoordinator::noteEditorWidgetForNoteLocalUid(const QString & noteLocalUid)
{
    QNDEBUG(QStringLiteral("NoteEditorTabsAndWindowsCoordinator::noteEditorWidgetForNoteLocalUid: ") << noteLocalUid);

    for(int i = 0; i < m_pTabWidget->count(); ++i)
    {
        NoteEditorWidget * pNoteEditorWidget = qobject_cast<NoteEditorWidget*>(m_pTabWidget->widget(i));
        if (Q_UNLIKELY(!pNoteEditorWidget)) {
            continue;
        }

        if (pNoteEditorWidget->noteLocalUid() != noteLocalUid) {
            continue;
        }

        QNDEBUG(QStringLiteral("Found existing editor tab"));
        return pNoteEditorWidget;
    }

    for(auto it = m_noteEditorWindowsByNoteLocalUid.begin(), end = m_noteEditorWindowsByNoteLocalUid.end(); it != end; ++it)
    {
        QNTRACE(QStringLiteral("Examining window editor's note local uid = ") << it.key());

        const QPointer<NoteEditorWidget> & pNoteEditorWidget = it.value();
        if (Q_UNLIKELY(pNoteEditorWidget.isNull())) {
            QNTRACE(QStringLiteral("The note editor widget is gone, skipping"));
            continue;
        }

        const QString & existingNoteLocalUid = it.key();
        if (existingNoteLocalUid != noteLocalUid) {
            continue;
        }

        QNDEBUG(QStringLiteral("Found existing editor window"));
        return pNoteEditorWidget.data();
    }

    return Q_NULLPTR;
}

void NoteEditorTabsAndWindowsCoordinator::addNote(const QString & noteLocalUid, const NoteEditorMode::type noteEditorMode,
                                                  const bool isNewNote)
{
    QNDEBUG(QStringLiteral("NoteEditorTabsAndWindowsCoordinator::addNote: ") << noteLocalUid
            << QStringLiteral(", note editor mode = ") << noteEditorMode << QStringLiteral(", is new note = ")
            << (isNewNote ? QStringLiteral("true") : QStringLiteral("false")));

    // First, check if this note is already open in some existing tab/window

    for(auto it = m_localUidsOfNotesInTabbedEditors.begin(), end = m_localUidsOfNotesInTabbedEditors.end(); it != end; ++it)
    {
        QNTRACE(QStringLiteral("Examining tab editor's note local uid = ") << *it);

        const QString & existingNoteLocalUid = *it;
        if (Q_UNLIKELY(existingNoteLocalUid == noteLocalUid))
        {
            QNDEBUG(QStringLiteral("The requested note is already open within one of note editor tabs"));

            if (noteEditorMode == NoteEditorMode::Window) {
                moveNoteEditorTabToWindow(noteLocalUid);
            }
            else {
                setCurrentNoteEditorWidgetTab(noteLocalUid);
            }

            return;
        }
    }

    for(auto it = m_noteEditorWindowsByNoteLocalUid.begin(), end = m_noteEditorWindowsByNoteLocalUid.end(); it != end; ++it)
    {
        QNTRACE(QStringLiteral("Examining window editor's note local uid = ") << it.key());

        const QPointer<NoteEditorWidget> & pNoteEditorWidget = it.value();
        if (pNoteEditorWidget.isNull()) {
            QNTRACE(QStringLiteral("The note editor widget is gone, skipping"));
            continue;
        }

        const QString & existingNoteLocalUid = it.key();
        if (Q_UNLIKELY(existingNoteLocalUid == noteLocalUid))
        {
            QNDEBUG(QStringLiteral("The requested note is already open within one of note editor windows"));

            if (noteEditorMode == NoteEditorMode::Tab) {
                moveNoteEditorWindowToTab(pNoteEditorWidget.data());
                setCurrentNoteEditorWidgetTab(noteLocalUid);
            }

            return;
        }
    }

    // If we got here, the note with specified local uid was not found within already open windows or tabs

    if ((noteEditorMode != NoteEditorMode::Window) && m_pBlankNoteEditor)
    {
        QNDEBUG(QStringLiteral("Currently only the blank note tab is displayed, "
                               "inserting the new note into the blank tab"));
        NoteEditorWidget * pNoteEditorWidget = m_pBlankNoteEditor;
        m_pBlankNoteEditor = Q_NULLPTR;

        pNoteEditorWidget->setNoteLocalUid(noteLocalUid, isNewNote);
        insertNoteEditorWidget(pNoteEditorWidget, NoteEditorMode::Tab);

        if (m_trackingCurrentTab)
        {
            m_lastCurrentTabNoteLocalUid = noteLocalUid;
            persistLastCurrentTabNoteLocalUid();

            QNTRACE(QStringLiteral("Emitting the update of last current tab note local uid to ")
                    << m_lastCurrentTabNoteLocalUid);
            Q_EMIT currentNoteChanged(m_lastCurrentTabNoteLocalUid);
        }

        return;
    }

    QUndoStack * pUndoStack = new QUndoStack;
    NoteEditorWidget * pNoteEditorWidget = new NoteEditorWidget(m_currentAccount, m_localStorageManagerAsync,
                                                                *m_pFileIOProcessorAsync, *m_pSpellChecker,
                                                                m_noteCache, m_notebookCache, m_tagCache,
                                                                *m_pTagModel, pUndoStack, m_pTabWidget);
    pUndoStack->setParent(pNoteEditorWidget);
    pNoteEditorWidget->setNoteLocalUid(noteLocalUid, isNewNote);
    insertNoteEditorWidget(pNoteEditorWidget, noteEditorMode);
}

void NoteEditorTabsAndWindowsCoordinator::createNewNote(const QString & notebookLocalUid,
                                                        const QString & notebookGuid,
                                                        const NoteEditorMode::type noteEditorMode)
{
    QNDEBUG(QStringLiteral("NoteEditorTabsAndWindowsCoordinator::createNewNote: notebook local uid = ")
            << notebookLocalUid << QStringLiteral(", notebook guid = ") << notebookGuid
            << QStringLiteral(", node editor mode = ") << noteEditorMode);

    Note newNote;
    newNote.setNotebookLocalUid(notebookLocalUid);
    newNote.setLocal(m_currentAccount.type() == Account::Type::Local);
    newNote.setDirty(true);
    newNote.setContent(QStringLiteral("<?xml version=\"1.0\" encoding=\"UTF-8\"?><!DOCTYPE en-note SYSTEM \"http://xml.evernote.com/pub/enml2.dtd\"><en-note><div></div></en-note>"));

    qint64 timestamp = QDateTime::currentMSecsSinceEpoch();
    newNote.setCreationTimestamp(timestamp);
    newNote.setModificationTimestamp(timestamp);

    QString sourceApplicationName = QApplication::applicationName();
    int sourceApplicationNameSize = sourceApplicationName.size();
    if ((sourceApplicationNameSize >= qevercloud::EDAM_ATTRIBUTE_LEN_MIN) &&
        (sourceApplicationNameSize <= qevercloud::EDAM_ATTRIBUTE_LEN_MAX))
    {
        qevercloud::NoteAttributes & noteAttributes = newNote.noteAttributes();
        noteAttributes.sourceApplication = sourceApplicationName;
    }

    if (!notebookGuid.isEmpty()) {
        newNote.setNotebookGuid(notebookGuid);
    }

    connectToLocalStorage();

    QUuid requestId = QUuid::createUuid();
    m_noteEditorModeByCreateNoteRequestIds[requestId] = noteEditorMode;

    QNTRACE(QStringLiteral("Emitting the request to add a new note to the local storage: request id = ")
            << requestId << QStringLiteral(", note editor mode: ") << noteEditorMode
            << QStringLiteral(", note = ") << newNote);
    Q_EMIT requestAddNote(newNote, requestId);
}

void NoteEditorTabsAndWindowsCoordinator::setUseLimitedFonts(const bool flag)
{
    QNDEBUG(QStringLiteral("NoteEditorTabsAndWindowsCoordinator::setUseLimitedFonts: ")
            << (flag ? QStringLiteral("true") : QStringLiteral("false")));

    for(int i = 0; i < m_pTabWidget->count(); ++i)
    {
        NoteEditorWidget * pNoteEditorWidget = qobject_cast<NoteEditorWidget*>(m_pTabWidget->widget(i));
        if (Q_UNLIKELY(!pNoteEditorWidget)) {
            continue;
        }

        pNoteEditorWidget->onSetUseLimitedFonts(flag);
    }

    for(auto it = m_noteEditorWindowsByNoteLocalUid.begin(), end = m_noteEditorWindowsByNoteLocalUid.end(); it != end; ++it)
    {
        QNTRACE(QStringLiteral("Examining window editor's note local uid = ") << it.key());

        const QPointer<NoteEditorWidget> & pNoteEditorWidget = it.value();
        if (Q_UNLIKELY(pNoteEditorWidget.isNull())) {
            QNTRACE(QStringLiteral("The note editor widget is gone, skipping"));
            continue;
        }

        pNoteEditorWidget->onSetUseLimitedFonts(flag);
    }
}

void NoteEditorTabsAndWindowsCoordinator::refreshNoteEditorWidgetsSpecialIcons()
{
    QNDEBUG(QStringLiteral("NoteEditorTabsAndWindowsCoordinator::refreshNoteEditorWidgetsSpecialIcons"));

    for(int i = 0; i < m_pTabWidget->count(); ++i)
    {
        NoteEditorWidget * pNoteEditorWidget = qobject_cast<NoteEditorWidget*>(m_pTabWidget->widget(i));
        if (Q_UNLIKELY(!pNoteEditorWidget)) {
            continue;
        }

        pNoteEditorWidget->refreshSpecialIcons();
    }

    for(auto it = m_noteEditorWindowsByNoteLocalUid.begin(), end = m_noteEditorWindowsByNoteLocalUid.end(); it != end; ++it)
    {
        QNTRACE(QStringLiteral("Examining window editor's note local uid = ") << it.key());

        const QPointer<NoteEditorWidget> & pNoteEditorWidget = it.value();
        if (Q_UNLIKELY(pNoteEditorWidget.isNull())) {
            QNTRACE(QStringLiteral("The note editor widget is gone, skipping"));
            continue;
        }

        pNoteEditorWidget->refreshSpecialIcons();
    }
}

void NoteEditorTabsAndWindowsCoordinator::saveAllNoteEditorsContents()
{
    QNDEBUG(QStringLiteral("NoteEditorTabsAndWindowsCoordinator::saveAllNoteEditorsContents"));

    ErrorString errorDescription;
    QList<NoteEditorWidget*> noteEditorWidgets = m_pTabWidget->findChildren<NoteEditorWidget*>();
    for(auto it = noteEditorWidgets.begin(), end = noteEditorWidgets.end(); it != end; ++it)
    {
        NoteEditorWidget * pNoteEditorWidget = *it;
        if (pNoteEditorWidget->noteLocalUid().isEmpty()) {
            continue;
        }

        NoteEditorWidget::NoteSaveStatus::type status = pNoteEditorWidget->checkAndSaveModifiedNote(errorDescription);
        if (status != NoteEditorWidget::NoteSaveStatus::Ok) {
            Q_EMIT notifyError(errorDescription);
        }
    }
}

bool NoteEditorTabsAndWindowsCoordinator::eventFilter(QObject * pWatched, QEvent * pEvent)
{
    if (Q_UNLIKELY(!pEvent)) {
        return true;
    }

    if (pEvent && ((pEvent->type() == QEvent::Close) || (pEvent->type() == QEvent::Destroy)))
    {
        NoteEditorWidget * pNoteEditorWidget = qobject_cast<NoteEditorWidget*>(pWatched);
        if (pNoteEditorWidget)
        {
            QString noteLocalUid = pNoteEditorWidget->noteLocalUid();

            if (pNoteEditorWidget->isSeparateWindow()) {
                clearPersistedNoteEditorWindowGeometry(noteLocalUid);
            }

            auto it = m_noteEditorWindowsByNoteLocalUid.find(noteLocalUid);
            if (it != m_noteEditorWindowsByNoteLocalUid.end())
            {
                QNTRACE(QStringLiteral("Intercepted close of note editor window, note local uid = ") << noteLocalUid);

                bool expungeFlag = false;

                if (pNoteEditorWidget->isModified())
                {
                    ErrorString errorDescription;
                    NoteEditorWidget::NoteSaveStatus::type status = pNoteEditorWidget->checkAndSaveModifiedNote(errorDescription);
                    QNDEBUG(QStringLiteral("Check and save modified note, status: ") << status
                            << QStringLiteral(", error description: ") << errorDescription);
                }
                else
                {
                    expungeFlag = shouldExpungeNote(*pNoteEditorWidget);
                }

                Q_UNUSED(m_noteEditorWindowsByNoteLocalUid.erase(it))
                persistLocalUidsOfNotesInEditorWindows();

                if (expungeFlag)
                {
                    pNoteEditorWidget->setNoteLocalUid(QString());
                    pNoteEditorWidget->hide();
                    pNoteEditorWidget->deleteLater();

                    expungeNoteSynchronously(noteLocalUid);
                }
            }
        }
    }
    else if (pEvent && (pEvent->type() == QEvent::Resize))
    {
        NoteEditorWidget * pNoteEditorWidget = qobject_cast<NoteEditorWidget*>(pWatched);
        if (pNoteEditorWidget && pNoteEditorWidget->isSeparateWindow()) {
            QString noteLocalUid = pNoteEditorWidget->noteLocalUid();
            scheduleNoteEditorWindowGeometrySave(noteLocalUid);
        }
    }
    else if (pEvent && (pEvent->type() == QEvent::FocusOut))
    {
        NoteEditorWidget * pNoteEditorWidget = qobject_cast<NoteEditorWidget*>(pWatched);
        if (pNoteEditorWidget)
        {
            QString noteLocalUid = pNoteEditorWidget->noteLocalUid();
            if (!noteLocalUid.isEmpty())
            {
                ErrorString errorDescription;
                NoteEditorWidget::NoteSaveStatus::type noteSaveStatus = pNoteEditorWidget->checkAndSaveModifiedNote(errorDescription);
                if (noteSaveStatus != NoteEditorWidget::NoteSaveStatus::Ok) {
                    Q_EMIT notifyError(errorDescription);
                }
            }
        }
    }

    return QObject::eventFilter(pWatched, pEvent);
}

void NoteEditorTabsAndWindowsCoordinator::onNoteEditorWidgetResolved()
{
    QNDEBUG(QStringLiteral("NoteEditorTabsAndWindowsCoordinator::onNoteEditorWidgetResolved"));

    NoteEditorWidget * pNoteEditorWidget = qobject_cast<NoteEditorWidget*>(sender());
    if (Q_UNLIKELY(!pNoteEditorWidget)) {
        ErrorString error(QT_TR_NOOP("Internal error: can't resolve the added note editor, cast failed"));
        QNWARNING(error);
        Q_EMIT notifyError(error);
        return;
    }

    QObject::disconnect(pNoteEditorWidget, QNSIGNAL(NoteEditorWidget,resolved),
                        this, QNSLOT(NoteEditorTabsAndWindowsCoordinator,onNoteEditorWidgetResolved));

    QString noteLocalUid = pNoteEditorWidget->noteLocalUid();

    bool foundTab = false;
    for(int i = 0, count = m_pTabWidget->count(); i < count; ++i)
    {
        NoteEditorWidget * pTabNoteEditorWidget = qobject_cast<NoteEditorWidget*>(m_pTabWidget->widget(i));
        if (Q_UNLIKELY(!pTabNoteEditorWidget)) {
            continue;
        }

        if (pTabNoteEditorWidget->noteLocalUid() != noteLocalUid) {
            continue;
        }

        QString displayName = shortenEditorName(pNoteEditorWidget->titleOrPreview());
        m_pTabWidget->setTabText(i, displayName);
        QNTRACE(QStringLiteral("Updated tab text for note editor with note ") << noteLocalUid
                << QStringLiteral(": ") << displayName);
        foundTab = true;

        if (i == m_pTabWidget->currentIndex()) {
            QNTRACE(QStringLiteral("Setting the focus to the current tab's note editor"));
            pNoteEditorWidget->setFocusToEditor();
        }

        break;
    }

    if (foundTab) {
        return;
    }

    auto it = m_noteEditorWindowsByNoteLocalUid.find(noteLocalUid);
    if (it == m_noteEditorWindowsByNoteLocalUid.end()) {
        QNTRACE(QStringLiteral("Couldn't find the note editor window corresponding to note local uid ")
                << noteLocalUid);
        return;
    }

    if (Q_UNLIKELY(it.value().isNull())) {
        QNTRACE(QStringLiteral("The note editor corresponding to note local uid ") << noteLocalUid
                << QStringLiteral(" is gone already"));
        return;
    }

    NoteEditorWidget * pWindowNoteEditor = it.value().data();
    QString displayName = shortenEditorName(pWindowNoteEditor->titleOrPreview(), MAX_WINDOW_NAME_SIZE);
    pWindowNoteEditor->setWindowTitle(displayName);
    QNTRACE(QStringLiteral("Updated window title for note editor with note ") << noteLocalUid
            << QStringLiteral(": ") << displayName);
}

void NoteEditorTabsAndWindowsCoordinator::onNoteEditorWidgetInvalidated()
{
    QNDEBUG(QStringLiteral("NoteEditorTabsAndWindowsCoordinator::onNoteEditorWidgetInvalidated"));

    NoteEditorWidget * pNoteEditorWidget = qobject_cast<NoteEditorWidget*>(sender());
    if (Q_UNLIKELY(!pNoteEditorWidget)) {
        ErrorString error(QT_TR_NOOP("Internal error: can't invalidate the note editor, cast failed"));
        QNWARNING(error);
        Q_EMIT notifyError(error);
        return;
    }

    for(int i = 0, numTabs = m_pTabWidget->count(); i < numTabs; ++i)
    {
        if (pNoteEditorWidget != m_pTabWidget->widget(i)) {
            continue;
        }

        onNoteEditorTabCloseRequested(i);
        break;
    }
}

void NoteEditorTabsAndWindowsCoordinator::onNoteTitleOrPreviewTextChanged(QString titleOrPreview)
{
    QNDEBUG(QStringLiteral("NoteEditorTabsAndWindowsCoordinator::onNoteTitleOrPreviewTextChanged: ") << titleOrPreview);

    NoteEditorWidget * pNoteEditorWidget = qobject_cast<NoteEditorWidget*>(sender());
    if (Q_UNLIKELY(!pNoteEditorWidget)) {
        ErrorString error(QT_TR_NOOP("Internal error: can't update the note editor's tab name, cast failed"));
        QNWARNING(error);
        Q_EMIT notifyError(error);
        return;
    }

    for(int i = 0, numTabs = m_pTabWidget->count(); i < numTabs; ++i)
    {
        if (pNoteEditorWidget != m_pTabWidget->widget(i)) {
            continue;
        }

        QString tabName = shortenEditorName(titleOrPreview);
        m_pTabWidget->setTabText(i, tabName);
        return;
    }

    for(auto it = m_noteEditorWindowsByNoteLocalUid.begin(),
        end = m_noteEditorWindowsByNoteLocalUid.end(); it != end; ++it)
    {
        if (it.value().isNull()) {
            continue;
        }

        NoteEditorWidget * pWindowNoteEditor = it.value().data();
        if (pWindowNoteEditor != pNoteEditorWidget) {
            continue;
        }

        QString windowName = shortenEditorName(titleOrPreview, MAX_WINDOW_NAME_SIZE);
        pWindowNoteEditor->setWindowTitle(windowName);
        return;
    }

    ErrorString error(QT_TR_NOOP("Internal error: can't find the note editor which has sent "
                                 "the title or preview text update"));
    QNWARNING(error);
    Q_EMIT notifyError(error);
}

void NoteEditorTabsAndWindowsCoordinator::onNoteEditorTabCloseRequested(int tabIndex)
{
    QNDEBUG(QStringLiteral("NoteEditorTabsAndWindowsCoordinator::onNoteEditorTabCloseRequested: ") << tabIndex);
    removeNoteEditorTab(tabIndex, /* close editor = */ true);
}

void NoteEditorTabsAndWindowsCoordinator::onInAppNoteLinkClicked(QString userId, QString shardId, QString noteGuid)
{
    QNDEBUG(QStringLiteral("NoteEditorTabsAndWindowsCoordinator::onInAppNoteLinkClicked: user id = ")
            << userId << QStringLiteral(", shard id = ") << shardId << QStringLiteral(", note guid = ") << noteGuid);

    connectToLocalStorage();

    Note dummyNote;
    dummyNote.unsetLocalUid();
    dummyNote.setGuid(noteGuid);
    QUuid requestId = QUuid::createUuid();
    Q_UNUSED(m_inAppNoteLinkFindNoteRequestIds.insert(requestId))
    QNTRACE(QStringLiteral("Emitting the request to find note by guid for opening it inside a new note editor tab: request id = ")
            << requestId << QStringLiteral(", note: ") << dummyNote);
    Q_EMIT findNote(dummyNote, /* with resource binary data = */ false, requestId);
}

void NoteEditorTabsAndWindowsCoordinator::onNoteLoadedInEditor()
{
    QNDEBUG(QStringLiteral("NoteEditorTabsAndWindowsCoordinator::onNoteLoadedInEditor"));
}

void NoteEditorTabsAndWindowsCoordinator::onNoteEditorError(ErrorString errorDescription)
{
    QNDEBUG(QStringLiteral("NoteEditorTabsAndWindowsCoordinator::onNoteEditorError: ") << errorDescription);

    NoteEditorWidget * pNoteEditorWidget = qobject_cast<NoteEditorWidget*>(sender());
    if (Q_UNLIKELY(!pNoteEditorWidget)) {
        QNWARNING(QStringLiteral("Received error from note editor but can't cast the sender to NoteEditorWidget; error: ")
                  << errorDescription);
        Q_EMIT notifyError(errorDescription);
        return;
    }

    ErrorString error(QT_TR_NOOP("Note editor error"));
    error.appendBase(errorDescription.base());
    error.appendBase(errorDescription.additionalBases());
    error.details() = errorDescription.details();

    QString titleOrPreview = pNoteEditorWidget->titleOrPreview();
    if (Q_UNLIKELY(titleOrPreview.isEmpty())) {
        error.details() += QStringLiteral(", note local uid ");
        error.details() += pNoteEditorWidget->noteLocalUid();
    }
    else {
        error.details() = QStringLiteral("note \"");
        error.details() += titleOrPreview;
        error.details() += QStringLiteral("\"");
    }

    Q_EMIT notifyError(error);
}

void NoteEditorTabsAndWindowsCoordinator::onAddNoteComplete(Note note, QUuid requestId)
{
    auto it = m_noteEditorModeByCreateNoteRequestIds.find(requestId);
    if (it == m_noteEditorModeByCreateNoteRequestIds.end()) {
        return;
    }

    QNDEBUG(QStringLiteral("NoteEditorTabsAndWindowsCoordinator::onAddNoteComplete: request id = ")
            << requestId << QStringLiteral(", note: ") << note);

    NoteEditorMode::type noteEditorMode = it.value();
    Q_UNUSED(m_noteEditorModeByCreateNoteRequestIds.erase(it))
    checkPendingRequestsAndDisconnectFromLocalStorage();

    m_noteCache.put(note.localUid(), note);
    addNote(note.localUid(), noteEditorMode, /* is new note = */ true);
}

void NoteEditorTabsAndWindowsCoordinator::onAddNoteFailed(Note note, ErrorString errorDescription, QUuid requestId)
{
    auto it = m_noteEditorModeByCreateNoteRequestIds.find(requestId);
    if (it == m_noteEditorModeByCreateNoteRequestIds.end()) {
        return;
    }

    QNWARNING(QStringLiteral("NoteEditorTabsAndWindowsCoordinator::onAddNoteFailed: request id = ")
              << requestId << QStringLiteral(", note: ") << note << QStringLiteral("\nError description: ")
              << errorDescription);

    Q_UNUSED(m_noteEditorModeByCreateNoteRequestIds.erase(it))
    checkPendingRequestsAndDisconnectFromLocalStorage();

    Q_UNUSED(internalErrorMessageBox(m_pTabWidget,
                                     tr("Note creation in local storage has failed") +
                                     QStringLiteral(": ") + errorDescription.localizedString()))
}

void NoteEditorTabsAndWindowsCoordinator::onFindNoteComplete(Note note, bool withResourceBinaryData, QUuid requestId)
{
    auto it = m_inAppNoteLinkFindNoteRequestIds.find(requestId);
    if (it == m_inAppNoteLinkFindNoteRequestIds.end()) {
        return;
    }

    QNDEBUG(QStringLiteral("NoteEditorTabsAndWindowsCoordinator::onFindNoteComplete: request id = ")
            << requestId << QStringLiteral(", with resource binary data = ")
            << (withResourceBinaryData ? QStringLiteral("true") : QStringLiteral("false"))
            << QStringLiteral(", note: ") << note);

    Q_UNUSED(m_inAppNoteLinkFindNoteRequestIds.erase(it))
    checkPendingRequestsAndDisconnectFromLocalStorage();

    addNote(note.localUid());
}

void NoteEditorTabsAndWindowsCoordinator::onFindNoteFailed(Note note, bool withResourceBinaryData,
                                                           ErrorString errorDescription, QUuid requestId)
{
    auto it = m_inAppNoteLinkFindNoteRequestIds.find(requestId);
    if (it == m_inAppNoteLinkFindNoteRequestIds.end()) {
        return;
    }

    QNDEBUG(QStringLiteral("NoteEditorTabsAndWindowsCoordinator::onFindNoteFailed: request id = ")
            << requestId << QStringLiteral(", with resource binary data = ")
            << (withResourceBinaryData ? QStringLiteral("true") : QStringLiteral("false"))
            << QStringLiteral(", error description = ") << errorDescription
            << QStringLiteral(", note: ") << note);

    Q_UNUSED(m_inAppNoteLinkFindNoteRequestIds.erase(it))
    checkPendingRequestsAndDisconnectFromLocalStorage();

    Q_EMIT notifyError(errorDescription);
}

void NoteEditorTabsAndWindowsCoordinator::onExpungeNoteComplete(Note note, QUuid requestId)
{
    auto it = m_expungeNoteRequestIds.find(requestId);
    if (it == m_expungeNoteRequestIds.end()) {
        return;
    }

    QNDEBUG(QStringLiteral("NoteEditorTabsAndWindowsCoordinator::onExpungeNoteComplete: request id = ")
            << requestId << QStringLiteral(", note: ") << note);

    Q_UNUSED(m_expungeNoteRequestIds.erase(it))
    checkPendingRequestsAndDisconnectFromLocalStorage();

    Q_EMIT noteExpungedFromLocalStorage();
}

void NoteEditorTabsAndWindowsCoordinator::onExpungeNoteFailed(Note note,
                                                              ErrorString errorDescription,
                                                              QUuid requestId)
{
    auto it = m_expungeNoteRequestIds.find(requestId);
    if (it == m_expungeNoteRequestIds.end()) {
        return;
    }

    QNWARNING(QStringLiteral("NoteEditorTabsAndWindowsCoordinator::onExpungeNoteFailed: request id = ")
              << requestId << QStringLiteral(", error description = ") << errorDescription
              << QStringLiteral(", note: ") << note);

    Q_UNUSED(m_expungeNoteRequestIds.erase(it))
    checkPendingRequestsAndDisconnectFromLocalStorage();

    Q_EMIT noteExpungeFromLocalStorageFailed();
}

void NoteEditorTabsAndWindowsCoordinator::onCurrentTabChanged(int currentIndex)
{
    QNDEBUG(QStringLiteral("NoteEditorTabsAndWindowsCoordinator::onCurrentTabChanged: ") << currentIndex);

    if (currentIndex < 0)
    {
        if (!m_lastCurrentTabNoteLocalUid.isEmpty())
        {
            m_lastCurrentTabNoteLocalUid.clear();
            persistLastCurrentTabNoteLocalUid();

            QNTRACE(QStringLiteral("Emitting last current tab note local uid update to empty"));
            Q_EMIT currentNoteChanged(QString());
        }

        return;
    }

    NoteEditorWidget * pNoteEditorWidget = qobject_cast<NoteEditorWidget*>(m_pTabWidget->widget(currentIndex));
    if (Q_UNLIKELY(!pNoteEditorWidget))
    {
        QNWARNING(QStringLiteral("Detected current tab change in the note editor tab widget "
                                 "but can't cast the tab widget's tab to note editor"));

        if (!m_lastCurrentTabNoteLocalUid.isEmpty())
        {
            m_lastCurrentTabNoteLocalUid.clear();
            persistLastCurrentTabNoteLocalUid();

            QNTRACE(QStringLiteral("Emitting last current tab note local uid update to empty"));
            Q_EMIT currentNoteChanged(QString());
        }

        return;
    }

    pNoteEditorWidget->setFocusToEditor();

    if (pNoteEditorWidget == m_pBlankNoteEditor)
    {
        QNTRACE(QStringLiteral("Switched to blank note editor"));

        if (!m_lastCurrentTabNoteLocalUid.isEmpty())
        {
            m_lastCurrentTabNoteLocalUid.clear();
            persistLastCurrentTabNoteLocalUid();

            QNTRACE(QStringLiteral("Emitting last current tab note local uid update to empty"));
            Q_EMIT currentNoteChanged(QString());
        }

        return;
    }

    if (!m_trackingCurrentTab) {
        return;
    }

    QString currentNoteLocalUid = pNoteEditorWidget->noteLocalUid();
    if (m_lastCurrentTabNoteLocalUid != currentNoteLocalUid)
    {
        m_lastCurrentTabNoteLocalUid = currentNoteLocalUid;
        persistLastCurrentTabNoteLocalUid();

        QNTRACE(QStringLiteral("Emitting last current tab note local uid update to ") << m_lastCurrentTabNoteLocalUid);
        Q_EMIT currentNoteChanged(currentNoteLocalUid);
    }
}

void NoteEditorTabsAndWindowsCoordinator::onTabContextMenuRequested(const QPoint & pos)
{
    QNDEBUG(QStringLiteral("NoteEditorTabsAndWindowsCoordinator::onTabContextMenuRequested, pos: x = ")
            << pos.x() << QStringLiteral(", y = ") << pos.y());

    int tabIndex = m_pTabWidget->tabBar()->tabAt(pos);
    if (Q_UNLIKELY(tabIndex < 0)) {
        ErrorString error(QT_TR_NOOP("Can't show the tab context menu: can't find the tab index "
                                     "of the target note editor"));
        QNWARNING(error);
        Q_EMIT notifyError(error);
        return;
    }

    NoteEditorWidget * pNoteEditorWidget = qobject_cast<NoteEditorWidget*>(m_pTabWidget->widget(tabIndex));
    if (Q_UNLIKELY(!pNoteEditorWidget)) {
        ErrorString error(QT_TR_NOOP("Can't show the tab context menu: can't cast the widget "
                                     "at the clicked tab to note editor"));
        QNWARNING(error << QStringLiteral(", tab index = ") << tabIndex);
        Q_EMIT notifyError(error);
        return;
    }

    delete m_pTabBarContextMenu;
    m_pTabBarContextMenu = new QMenu(m_pTabWidget);

#define ADD_CONTEXT_MENU_ACTION(name, menu, slot, data, enabled) \
    { \
        QAction * pAction = new QAction(name, menu); \
        pAction->setData(data); \
        pAction->setEnabled(enabled); \
        QObject::connect(pAction, QNSIGNAL(QAction,triggered), this, QNSLOT(NoteEditorTabsAndWindowsCoordinator,slot)); \
        menu->addAction(pAction); \
    }

    if (pNoteEditorWidget->isModified()) {
        ADD_CONTEXT_MENU_ACTION(tr("Save"), m_pTabBarContextMenu, onTabContextMenuSaveNoteAction,
                                pNoteEditorWidget->noteLocalUid(), true);
    }

    if (m_pTabWidget->count() != 1) {
        ADD_CONTEXT_MENU_ACTION(tr("Open in separate window"), m_pTabBarContextMenu, onTabContextMenuMoveToWindowAction,
                                pNoteEditorWidget->noteLocalUid(), true);
    }

    ADD_CONTEXT_MENU_ACTION(tr("Close"), m_pTabBarContextMenu, onTabContextMenuCloseEditorAction,
                            pNoteEditorWidget->noteLocalUid(), true);

    m_pTabBarContextMenu->show();
    m_pTabBarContextMenu->exec(m_pTabWidget->tabBar()->mapToGlobal(pos));
}

void NoteEditorTabsAndWindowsCoordinator::onTabContextMenuCloseEditorAction()
{
    QNDEBUG(QStringLiteral("NoteEditorTabsAndWindowsCoordinator::onTabContextMenuCloseEditorAction"));

    QAction * pAction = qobject_cast<QAction*>(sender());
    if (Q_UNLIKELY(!pAction)) {
        ErrorString error(QT_TR_NOOP("Internal error: can't close the chosen note editor, "
                                     "can't cast the slot invoker to QAction"));
        QNWARNING(error);
        Q_EMIT notifyError(error);
        return;
    }

    QString noteLocalUid = pAction->data().toString();
    if (Q_UNLIKELY(noteLocalUid.isEmpty())) {
        ErrorString error(QT_TR_NOOP("Internal error: can't close the chosen note editor, "
                                     "can't get the note local uid corresponding to the editor"));
        QNWARNING(error);
        Q_EMIT notifyError(error);
        return;
    }

    for(int i = 0; i < m_pTabWidget->count(); ++i)
    {
        NoteEditorWidget * pNoteEditorWidget = qobject_cast<NoteEditorWidget*>(m_pTabWidget->widget(i));
        if (Q_UNLIKELY(!pNoteEditorWidget)) {
            continue;
        }

        if (pNoteEditorWidget->noteLocalUid() == noteLocalUid) {
            onNoteEditorTabCloseRequested(i);
            return;
        }
    }

    // If we got here, no target note editor widget was found
    ErrorString error(QT_TR_NOOP("Internal error: can't close the chosen note editor, "
                                 "can't find the editor to be closed by note local uid"));
    QNWARNING(error << QStringLiteral(", note local uid = ") << noteLocalUid);
    Q_EMIT notifyError(error);
    return;
}

void NoteEditorTabsAndWindowsCoordinator::onTabContextMenuSaveNoteAction()
{
    QNDEBUG(QStringLiteral("NoteEditorTabsAndWindowsCoordinator::onTabContextMenuSaveNoteAction"));

    QAction * pAction = qobject_cast<QAction*>(sender());
    if (Q_UNLIKELY(!pAction)) {
        ErrorString error(QT_TR_NOOP("Internal error: can't save the note within the chosen note editor, "
                                     "can't cast the slot invoker to QAction"));
        QNWARNING(error);
        Q_EMIT notifyError(error);
        return;
    }

    QString noteLocalUid = pAction->data().toString();
    if (Q_UNLIKELY(noteLocalUid.isEmpty())) {
        ErrorString error(QT_TR_NOOP("Internal error: can't save the note within the chosen note editor, "
                                     "can't get the note local uid corresponding to the editor"));
        QNWARNING(error);
        Q_EMIT notifyError(error);
        return;
    }

    for(int i = 0; i < m_pTabWidget->count(); ++i)
    {
        NoteEditorWidget * pNoteEditorWidget = qobject_cast<NoteEditorWidget*>(m_pTabWidget->widget(i));
        if (Q_UNLIKELY(!pNoteEditorWidget)) {
            continue;
        }

        if (pNoteEditorWidget->noteLocalUid() != noteLocalUid) {
            continue;
        }

        if (Q_UNLIKELY(!pNoteEditorWidget->isModified())) {
            QNINFO(QStringLiteral("The note editor widget doesn't appear to contain a note that needs saving"));
            return;
        }

        ErrorString errorDescription;
        NoteEditorWidget::NoteSaveStatus::type res = pNoteEditorWidget->checkAndSaveModifiedNote(errorDescription);
        if (Q_UNLIKELY(res != NoteEditorWidget::NoteSaveStatus::Ok))
        {
            ErrorString error(QT_TR_NOOP("Couldn't save the note"));
            error.appendBase(errorDescription.base());
            error.appendBase(errorDescription.additionalBases());
            error.details() = errorDescription.details();
            QNWARNING(error << QStringLiteral(", note local uid = ") << noteLocalUid);
            Q_EMIT notifyError(error);
        }

        return;
    }

    // If we got here, no target note editor widget was found
    ErrorString error(QT_TR_NOOP("Internal error: can't save the note within the chosen note editor, "
                                 "can't find the editor to be closed by note local uid"));
    QNWARNING(error << QStringLiteral(", note local uid = ") << noteLocalUid);
    Q_EMIT notifyError(error);
    return;
}

void NoteEditorTabsAndWindowsCoordinator::onTabContextMenuMoveToWindowAction()
{
    QNDEBUG(QStringLiteral("NoteEditorTabsAndWindowsCoordinator::onTabContextMenuMoveToWindowAction"));

    QAction * pAction = qobject_cast<QAction*>(sender());
    if (Q_UNLIKELY(!pAction)) {
        ErrorString error(QT_TR_NOOP("Internal error: can't move the note editor tab to window, "
                                     "can't cast the slot invoker to QAction"));
        QNWARNING(error);
        Q_EMIT notifyError(error);
        return;
    }

    QString noteLocalUid = pAction->data().toString();
    if (Q_UNLIKELY(noteLocalUid.isEmpty())) {
        ErrorString error(QT_TR_NOOP("Internal error: can't move the note editor tab to window, "
                                     "can't get the note local uid corresponding to the editor"));
        QNWARNING(error);
        Q_EMIT notifyError(error);
        return;
    }

    moveNoteEditorTabToWindow(noteLocalUid);
}

void NoteEditorTabsAndWindowsCoordinator::expungeNoteFromLocalStorage()
{
    QNDEBUG(QStringLiteral("NoteEditorTabsAndWindowsCoordinator::expungeNoteFromLocalStorage"));

    if (Q_UNLIKELY(m_localUidOfNoteToBeExpunged.isEmpty())) {
        QNWARNING(QStringLiteral("The local uid of note to be expunged is empty"));
        Q_EMIT noteExpungeFromLocalStorageFailed();
        return;
    }

    connectToLocalStorage();

    Note dummyNote;
    dummyNote.setLocalUid(m_localUidOfNoteToBeExpunged);

    QUuid requestId = QUuid::createUuid();
    Q_UNUSED(m_expungeNoteRequestIds.insert(requestId))
    QNTRACE(QStringLiteral("Emitting the request to expunge note: request id = ")
            << requestId << QStringLiteral(", note local uid = ") << m_localUidOfNoteToBeExpunged);
    Q_EMIT requestExpungeNote(dummyNote, requestId);

    m_localUidOfNoteToBeExpunged.clear();
}

void NoteEditorTabsAndWindowsCoordinator::timerEvent(QTimerEvent * pTimerEvent)
{
    QNDEBUG(QStringLiteral("NoteEditorTabsAndWindowsCoordinator::timerEvent: id = ")
            << (pTimerEvent ? QString::number(pTimerEvent->timerId()) : QStringLiteral("<null>")));

    if (Q_UNLIKELY(!pTimerEvent)) {
        return;
    }

    int timerId = pTimerEvent->timerId();
    auto it = m_saveNoteEditorWindowGeometryPostponeTimerIdToNoteLocalUidBimap.right.find(timerId);
    if (it != m_saveNoteEditorWindowGeometryPostponeTimerIdToNoteLocalUidBimap.right.end())
    {
        QString noteLocalUid = it->second;
        Q_UNUSED(m_saveNoteEditorWindowGeometryPostponeTimerIdToNoteLocalUidBimap.right.erase(it))
        killTimer(timerId);
        persistNoteEditorWindowGeometry(noteLocalUid);
    }
}

void NoteEditorTabsAndWindowsCoordinator::insertNoteEditorWidget(NoteEditorWidget * pNoteEditorWidget, const NoteEditorMode::type noteEditorMode)
{
    QNDEBUG(QStringLiteral("NoteEditorTabsAndWindowsCoordinator::insertNoteEditorWidget: ") << pNoteEditorWidget->noteLocalUid()
            << QStringLiteral(", note editor mode = ") << noteEditorMode);

    QObject::connect(pNoteEditorWidget, QNSIGNAL(NoteEditorWidget,titleOrPreviewChanged,QString),
                     this, QNSLOT(NoteEditorTabsAndWindowsCoordinator,onNoteTitleOrPreviewTextChanged,QString));
    QObject::connect(pNoteEditorWidget, QNSIGNAL(NoteEditorWidget,resolved),
                     this, QNSLOT(NoteEditorTabsAndWindowsCoordinator,onNoteEditorWidgetResolved));
    QObject::connect(pNoteEditorWidget, QNSIGNAL(NoteEditorWidget,invalidated),
                     this, QNSLOT(NoteEditorTabsAndWindowsCoordinator,onNoteEditorWidgetInvalidated));
    QObject::connect(pNoteEditorWidget, QNSIGNAL(NoteEditorWidget,noteLoaded),
                     this, QNSLOT(NoteEditorTabsAndWindowsCoordinator,onNoteLoadedInEditor));
    QObject::connect(pNoteEditorWidget, QNSIGNAL(NoteEditorWidget,inAppNoteLinkClicked,QString,QString,QString),
                     this, QNSLOT(NoteEditorTabsAndWindowsCoordinator,onInAppNoteLinkClicked,QString,QString,QString));
    QObject::connect(pNoteEditorWidget, QNSIGNAL(NoteEditorWidget,notifyError,ErrorString),
                     this, QNSLOT(NoteEditorTabsAndWindowsCoordinator,onNoteEditorError,ErrorString));

    if (noteEditorMode == NoteEditorMode::Window)
    {
        QString noteLocalUid = pNoteEditorWidget->noteLocalUid();

        Q_UNUSED(pNoteEditorWidget->makeSeparateWindow())

        QString displayName = shortenEditorName(pNoteEditorWidget->titleOrPreview(), MAX_WINDOW_NAME_SIZE);
        pNoteEditorWidget->setWindowTitle(displayName);
        pNoteEditorWidget->setAttribute(Qt::WA_DeleteOnClose, true);
        pNoteEditorWidget->installEventFilter(this);

        if (!noteLocalUid.isEmpty()) {
            m_noteEditorWindowsByNoteLocalUid[noteLocalUid] = QPointer<NoteEditorWidget>(pNoteEditorWidget);
            persistLocalUidsOfNotesInEditorWindows();
        }

        pNoteEditorWidget->show();
        return;
    }

    // If we got here, will insert the note editor as a tab

    m_localUidsOfNotesInTabbedEditors.push_back(pNoteEditorWidget->noteLocalUid());
    QNTRACE(QStringLiteral("Added tabbed note local uid: ") << pNoteEditorWidget->noteLocalUid()
            << QStringLiteral(", the number of tabbed note local uids = ") << m_localUidsOfNotesInTabbedEditors.size());
    persistLocalUidsOfNotesInEditorTabs();

    QString displayName = shortenEditorName(pNoteEditorWidget->titleOrPreview());

    int tabIndex = m_pTabWidget->indexOf(pNoteEditorWidget);
    if (tabIndex < 0) {
        tabIndex = m_pTabWidget->addTab(pNoteEditorWidget, displayName);
    }
    else {
        m_pTabWidget->setTabText(tabIndex, displayName);
    }

    m_pTabWidget->setCurrentIndex(tabIndex);

    pNoteEditorWidget->installEventFilter(this);

    int currentNumNotesInTabs = numNotesInTabs();

    if (currentNumNotesInTabs > 1) {
        m_pTabWidget->tabBar()->show();
        m_pTabWidget->setTabsClosable(true);
    }
    else {
        m_pTabWidget->tabBar()->hide();
        m_pTabWidget->setTabsClosable(false);
    }

    if (currentNumNotesInTabs <= m_maxNumNotesInTabs) {
        QNDEBUG(QStringLiteral("The addition of note ") << pNoteEditorWidget->noteLocalUid()
                << QStringLiteral(" doesn't cause the overflow of max allowed number of note editor tabs"));
        return;
    }

    checkAndCloseOlderNoteEditorTabs();
}

void NoteEditorTabsAndWindowsCoordinator::removeNoteEditorTab(int tabIndex, const bool closeEditor)
{
    QNDEBUG(QStringLiteral("NoteEditorTabsAndWindowsCoordinator::removeNoteEditorTab: tab index = ")
            << tabIndex << QStringLiteral(", close editor = ") << (closeEditor ? QStringLiteral("true") : QStringLiteral("false")));

    NoteEditorWidget * pNoteEditorWidget = qobject_cast<NoteEditorWidget*>(m_pTabWidget->widget(tabIndex));
    if (Q_UNLIKELY(!pNoteEditorWidget)) {
        QNWARNING(QStringLiteral("Detected attempt to close the note editor tab but can't cast the tab widget's tab to note editor"));
        return;
    }

    if (pNoteEditorWidget == m_pBlankNoteEditor) {
        QNDEBUG(QStringLiteral("Silently refusing to remove the blank note editor tab"));
        return;
    }

    bool expungeFlag = false;

    if (pNoteEditorWidget->isModified())
    {
        ErrorString errorDescription;
        NoteEditorWidget::NoteSaveStatus::type status = pNoteEditorWidget->checkAndSaveModifiedNote(errorDescription);
        QNDEBUG(QStringLiteral("Check and save modified note, status: ") << status
                << QStringLiteral(", error description: ") << errorDescription);
    }
    else
    {
        expungeFlag = shouldExpungeNote(*pNoteEditorWidget);
    }

    QString noteLocalUid = pNoteEditorWidget->noteLocalUid();

    auto it = std::find(m_localUidsOfNotesInTabbedEditors.begin(), m_localUidsOfNotesInTabbedEditors.end(), noteLocalUid);
    if (it != m_localUidsOfNotesInTabbedEditors.end()) {
        Q_UNUSED(m_localUidsOfNotesInTabbedEditors.erase(it))
        QNTRACE(QStringLiteral("Removed note local uid ") << pNoteEditorWidget->noteLocalUid());
        persistLocalUidsOfNotesInEditorTabs();
    }

    QStringLiteral("Remaining tabbed note local uids: ");
    for(auto sit = m_localUidsOfNotesInTabbedEditors.begin(), end = m_localUidsOfNotesInTabbedEditors.end(); sit != end; ++sit) {
        QNTRACE(*sit);
    }

    if (m_lastCurrentTabNoteLocalUid == noteLocalUid)
    {
        m_lastCurrentTabNoteLocalUid.clear();
        persistLastCurrentTabNoteLocalUid();

        QNTRACE(QStringLiteral("Emitting last current tab note local uid update to empty"));
        Q_EMIT currentNoteChanged(QString());
    }

    if (m_pTabWidget->count() == 1)
    {
        // That should remove the note from the editor (if any)
        pNoteEditorWidget->setNoteLocalUid(QString());
        m_pBlankNoteEditor = pNoteEditorWidget;

        m_pTabWidget->setTabText(0, BLANK_NOTE_KEY);
        m_pTabWidget->tabBar()->hide();
        m_pTabWidget->setTabsClosable(false);

        if (expungeFlag) {
            expungeNoteSynchronously(noteLocalUid);
        }

        return;
    }

    m_pTabWidget->removeTab(tabIndex);

    if (closeEditor) {
        pNoteEditorWidget->hide();
        pNoteEditorWidget->deleteLater();
        pNoteEditorWidget = Q_NULLPTR;
    }

    if (m_pTabWidget->count() == 1) {
        m_pTabWidget->tabBar()->hide();
        m_pTabWidget->setTabsClosable(false);
    }

    if (expungeFlag) {
        expungeNoteSynchronously(noteLocalUid);
    }
}

void NoteEditorTabsAndWindowsCoordinator::checkAndCloseOlderNoteEditorTabs()
{
    for(int i = 0; i < m_pTabWidget->count(); ++i)
    {
        NoteEditorWidget * pNoteEditorWidget = qobject_cast<NoteEditorWidget*>(m_pTabWidget->widget(i));
        if (Q_UNLIKELY(!pNoteEditorWidget)) {
            continue;
        }

        if (Q_UNLIKELY(pNoteEditorWidget == m_pBlankNoteEditor)) {
            continue;
        }

        const QString & noteLocalUid = pNoteEditorWidget->noteLocalUid();
        auto it = std::find(m_localUidsOfNotesInTabbedEditors.begin(), m_localUidsOfNotesInTabbedEditors.end(), noteLocalUid);
        if (it == m_localUidsOfNotesInTabbedEditors.end()) {
            m_pTabWidget->removeTab(i);
            pNoteEditorWidget->hide();
            pNoteEditorWidget->deleteLater();
        }
    }

    if (m_pTabWidget->count() <= 1) {
        m_pTabWidget->tabBar()->hide();
        m_pTabWidget->setTabsClosable(false);
    }
    else {
        m_pTabWidget->tabBar()->show();
        m_pTabWidget->setTabsClosable(true);
    }
}

void NoteEditorTabsAndWindowsCoordinator::setCurrentNoteEditorWidgetTab(const QString & noteLocalUid)
{
    QNDEBUG(QStringLiteral("NoteEditorTabsAndWindowsCoordinator::setCurrentNoteEditorWidgetTab: ") << noteLocalUid);

    for(int i = 0; i < m_pTabWidget->count(); ++i)
    {
        NoteEditorWidget * pNoteEditorWidget = qobject_cast<NoteEditorWidget*>(m_pTabWidget->widget(i));
        if (Q_UNLIKELY(!pNoteEditorWidget)) {
            continue;
        }

        if (Q_UNLIKELY(pNoteEditorWidget == m_pBlankNoteEditor)) {
            continue;
        }

        if (pNoteEditorWidget->noteLocalUid() == noteLocalUid) {
            m_pTabWidget->setCurrentIndex(i);
            break;
        }
    }
}

void NoteEditorTabsAndWindowsCoordinator::scheduleNoteEditorWindowGeometrySave(const QString & noteLocalUid)
{
    QNDEBUG(QStringLiteral("NoteEditorTabsAndWindowsCoordinator::scheduleNoteEditorWindowGeometrySave: ")
            << noteLocalUid);

    if (Q_UNLIKELY(noteLocalUid.isEmpty())) {
        return;
    }

    auto it = m_saveNoteEditorWindowGeometryPostponeTimerIdToNoteLocalUidBimap.left.find(noteLocalUid);
    if (it != m_saveNoteEditorWindowGeometryPostponeTimerIdToNoteLocalUidBimap.left.end()) {
        QNDEBUG(QStringLiteral("The timer delaying the save of this note editor window's geometry "
                               "has already been started: timer id = ") << it->second);
        return;
    }

    int timerId = startTimer(PERSIST_NOTE_EDITOR_WINDOW_GEOMETRY_DELAY);
    if (Q_UNLIKELY(timerId == 0)) {
        QNWARNING(QStringLiteral("Failed to start timer to postpone saving the note editor window's geometry"));
        return;
    }

    QNDEBUG(QStringLiteral("Started the timer to postpone saving the note editor window's geometry: timer id = ")
            << timerId << QStringLiteral(", note local uid = ") << noteLocalUid);

    Q_UNUSED(m_saveNoteEditorWindowGeometryPostponeTimerIdToNoteLocalUidBimap.insert(NoteLocalUidToTimerIdBimap::value_type(noteLocalUid, timerId)))
}

void NoteEditorTabsAndWindowsCoordinator::persistNoteEditorWindowGeometry(const QString & noteLocalUid)
{
    QNDEBUG(QStringLiteral("NoteEditorTabsAndWindowsCoordinator::persistNoteEditorWindowGeometry: ")
            << noteLocalUid);

    if (Q_UNLIKELY(noteLocalUid.isEmpty())) {
        return;
    }

    auto it = m_noteEditorWindowsByNoteLocalUid.find(noteLocalUid);
    if (Q_UNLIKELY(it == m_noteEditorWindowsByNoteLocalUid.end())) {
        QNDEBUG(QStringLiteral("Can't persist note editor window geometry: could not find the note editor window "
                               "for note local uid ") << noteLocalUid);
        return;
    }

    if (it.value().isNull()) {
        QNDEBUG(QStringLiteral("Can't persist the note editor window geometry: the note editor window is gone "
                               "for note local uid ") << noteLocalUid);
        Q_UNUSED(m_noteEditorWindowsByNoteLocalUid.erase(it))
        return;
    }

    ApplicationSettings appSettings(m_currentAccount, QUENTIER_UI_SETTINGS);
    appSettings.beginGroup(NOTE_EDITOR_SETTINGS_GROUP_NAME);
    appSettings.setValue(NOTE_EDITOR_WINDOW_GEOMETRY_KEY_PREFIX + noteLocalUid,
                         it.value().data()->saveGeometry());
    appSettings.endGroup();
}

void NoteEditorTabsAndWindowsCoordinator::clearPersistedNoteEditorWindowGeometry(const QString & noteLocalUid)
{
    QNDEBUG(QStringLiteral("NoteEditorTabsAndWindowsCoordinator::clearPersistedNoteEditorWindowGeometry: ")
            << noteLocalUid);

    if (Q_UNLIKELY(noteLocalUid.isEmpty())) {
        return;
    }

    ApplicationSettings appSettings(m_currentAccount, QUENTIER_UI_SETTINGS);
    appSettings.beginGroup(NOTE_EDITOR_SETTINGS_GROUP_NAME);
    appSettings.remove(NOTE_EDITOR_WINDOW_GEOMETRY_KEY_PREFIX + noteLocalUid);
    appSettings.endGroup();
}

void NoteEditorTabsAndWindowsCoordinator::restoreNoteEditorWindowGeometry(const QString & noteLocalUid)
{
    QNDEBUG(QStringLiteral("NoteEditorTabsAndWindowsCoordinator::restoreNoteEditorWindowGeometry: ")
            << noteLocalUid);

    if (Q_UNLIKELY(noteLocalUid.isEmpty())) {
        return;
    }

    auto it = m_noteEditorWindowsByNoteLocalUid.find(noteLocalUid);
    if (Q_UNLIKELY(it == m_noteEditorWindowsByNoteLocalUid.end())) {
        QNDEBUG(QStringLiteral("Can't restore note editor window's geometry: could not find the note editor window "
                               "for note local uid ") << noteLocalUid);
        return;
    }

    if (it.value().isNull()) {
        QNDEBUG(QStringLiteral("Can't restore the note editor window geometry: the note editor window is gone "
                               "for note local uid ") << noteLocalUid);
        Q_UNUSED(m_noteEditorWindowsByNoteLocalUid.erase(it))
        clearPersistedNoteEditorWindowGeometry(noteLocalUid);
        return;
    }

    ApplicationSettings appSettings(m_currentAccount, QUENTIER_UI_SETTINGS);
    appSettings.beginGroup(NOTE_EDITOR_SETTINGS_GROUP_NAME);
    QByteArray noteEditorWindowGeometry = appSettings.value(NOTE_EDITOR_WINDOW_GEOMETRY_KEY_PREFIX + noteLocalUid).toByteArray();
    appSettings.endGroup();

    bool res = it.value().data()->restoreGeometry(noteEditorWindowGeometry);
    if (!res) {
        QNDEBUG(QStringLiteral("Could not restore the geometry for note editor window with note local uid ")
                << noteLocalUid);
        return;
    }

    QNTRACE(QStringLiteral("Restored the geometry for note editor window with note local uid ") << noteLocalUid);
}

void NoteEditorTabsAndWindowsCoordinator::connectToLocalStorage()
{
    QNDEBUG(QStringLiteral("NoteEditorTabsAndWindowsCoordinator::connectToLocalStorage"));

    if (m_connectedToLocalStorage) {
        QNDEBUG(QStringLiteral("Already connected to the local storage"));
        return;
    }

    QObject::connect(this, QNSIGNAL(NoteEditorTabsAndWindowsCoordinator,requestAddNote,Note,QUuid),
                     &m_localStorageManagerAsync, QNSLOT(LocalStorageManagerAsync,onAddNoteRequest,Note,QUuid));
    QObject::connect(this, QNSIGNAL(NoteEditorTabsAndWindowsCoordinator,requestExpungeNote,Note,QUuid),
                     &m_localStorageManagerAsync, QNSLOT(LocalStorageManagerAsync,onExpungeNoteRequest,Note,QUuid));
    QObject::connect(this, QNSIGNAL(NoteEditorTabsAndWindowsCoordinator,findNote,Note,bool,QUuid),
                     &m_localStorageManagerAsync, QNSLOT(LocalStorageManagerAsync,onFindNoteRequest,Note,bool,QUuid));

    QObject::connect(&m_localStorageManagerAsync, QNSIGNAL(LocalStorageManagerAsync,addNoteComplete,Note,QUuid),
                     this, QNSLOT(NoteEditorTabsAndWindowsCoordinator,onAddNoteComplete,Note,QUuid));
    QObject::connect(&m_localStorageManagerAsync, QNSIGNAL(LocalStorageManagerAsync,addNoteFailed,Note,ErrorString,QUuid),
                     this, QNSLOT(NoteEditorTabsAndWindowsCoordinator,onAddNoteFailed,Note,ErrorString,QUuid));
    QObject::connect(&m_localStorageManagerAsync, QNSIGNAL(LocalStorageManagerAsync,findNoteComplete,Note,bool,QUuid),
                     this, QNSLOT(NoteEditorTabsAndWindowsCoordinator,onFindNoteComplete,Note,bool,QUuid));
    QObject::connect(&m_localStorageManagerAsync, QNSIGNAL(LocalStorageManagerAsync,findNoteFailed,Note,bool,ErrorString,QUuid),
                     this, QNSLOT(NoteEditorTabsAndWindowsCoordinator,onFindNoteFailed,Note,bool,ErrorString,QUuid));
    QObject::connect(&m_localStorageManagerAsync, QNSIGNAL(LocalStorageManagerAsync,expungeNoteComplete,Note,QUuid),
                     this, QNSLOT(NoteEditorTabsAndWindowsCoordinator,onExpungeNoteComplete,Note,QUuid));
    QObject::connect(&m_localStorageManagerAsync, QNSIGNAL(LocalStorageManagerAsync,expungeNoteFailed,Note,ErrorString,QUuid),
                     this, QNSLOT(NoteEditorTabsAndWindowsCoordinator,onExpungeNoteFailed,Note,ErrorString,QUuid));

    m_connectedToLocalStorage = true;
}

void NoteEditorTabsAndWindowsCoordinator::disconnectFromLocalStorage()
{
    QNDEBUG(QStringLiteral("NoteEditorTabsAndWindowsCoordinator::disconnectFromLocalStorage"));

    if (!m_connectedToLocalStorage) {
        QNDEBUG(QStringLiteral("Not connected to local storage at the moment"));
        return;
    }

    QObject::disconnect(this, QNSIGNAL(NoteEditorTabsAndWindowsCoordinator,requestAddNote,Note,QUuid),
                        &m_localStorageManagerAsync, QNSLOT(LocalStorageManagerAsync,onAddNoteRequest,Note,QUuid));
    QObject::disconnect(this, QNSIGNAL(NoteEditorTabsAndWindowsCoordinator,requestExpungeNote,Note,QUuid),
                        &m_localStorageManagerAsync, QNSLOT(LocalStorageManagerAsync,onExpungeNoteRequest,Note,QUuid));
    QObject::disconnect(this, QNSIGNAL(NoteEditorTabsAndWindowsCoordinator,findNote,Note,bool,QUuid),
                        &m_localStorageManagerAsync, QNSLOT(LocalStorageManagerAsync,onFindNoteRequest,Note,bool,QUuid));

    QObject::disconnect(&m_localStorageManagerAsync, QNSIGNAL(LocalStorageManagerAsync,addNoteComplete,Note,QUuid),
                        this, QNSLOT(NoteEditorTabsAndWindowsCoordinator,onAddNoteComplete,Note,QUuid));
    QObject::disconnect(&m_localStorageManagerAsync, QNSIGNAL(LocalStorageManagerAsync,addNoteFailed,Note,ErrorString,QUuid),
                        this, QNSLOT(NoteEditorTabsAndWindowsCoordinator,onAddNoteFailed,Note,ErrorString,QUuid));
    QObject::disconnect(&m_localStorageManagerAsync, QNSIGNAL(LocalStorageManagerAsync,findNoteComplete,Note,bool,QUuid),
                        this, QNSLOT(NoteEditorTabsAndWindowsCoordinator,onFindNoteComplete,Note,bool,QUuid));
    QObject::disconnect(&m_localStorageManagerAsync, QNSIGNAL(LocalStorageManagerAsync,findNoteFailed,Note,bool,ErrorString,QUuid),
                        this, QNSLOT(NoteEditorTabsAndWindowsCoordinator,onFindNoteFailed,Note,bool,ErrorString,QUuid));
    QObject::disconnect(&m_localStorageManagerAsync, QNSIGNAL(LocalStorageManagerAsync,expungeNoteComplete,Note,QUuid),
                        this, QNSLOT(NoteEditorTabsAndWindowsCoordinator,onExpungeNoteComplete,Note,QUuid));
    QObject::disconnect(&m_localStorageManagerAsync, QNSIGNAL(LocalStorageManagerAsync,expungeNoteFailed,Note,ErrorString,QUuid),
                        this, QNSLOT(NoteEditorTabsAndWindowsCoordinator,onExpungeNoteFailed,Note,ErrorString,QUuid));

    m_connectedToLocalStorage = false;
}

void NoteEditorTabsAndWindowsCoordinator::setupFileIO()
{
    QNDEBUG(QStringLiteral("NoteEditorTabsAndWindowsCoordinator::setupFileIO"));

    if (!m_pIOThread) {
        m_pIOThread = new QThread;
        QObject::connect(m_pIOThread, QNSIGNAL(QThread,finished), m_pIOThread, QNSLOT(QThread,deleteLater));
        m_pIOThread->start(QThread::LowPriority);
    }

    if (!m_pFileIOProcessorAsync) {
        m_pFileIOProcessorAsync = new FileIOProcessorAsync;
        m_pFileIOProcessorAsync->moveToThread(m_pIOThread);
    }
}

void NoteEditorTabsAndWindowsCoordinator::setupSpellChecker()
{
    QNDEBUG(QStringLiteral("NoteEditorTabsAndWindowsCoordinator::setupSpellChecker"));

    if (!m_pSpellChecker) {
        m_pSpellChecker = new SpellChecker(m_pFileIOProcessorAsync, m_currentAccount, this);
    }
}

QString NoteEditorTabsAndWindowsCoordinator::shortenEditorName(const QString & name, int maxSize) const
{
    if (maxSize < 0) {
        maxSize = MAX_TAB_NAME_SIZE;
    }

    if (name.size() <= maxSize) {
        return name;
    }

    QString result = name.simplified();
    result.truncate(std::max(maxSize - 3, 0));
    result += QStringLiteral("...");
    return result;
}

void NoteEditorTabsAndWindowsCoordinator::moveNoteEditorTabToWindow(const QString & noteLocalUid)
{
    QNDEBUG(QStringLiteral("NoteEditorTabsAndWindowsCoordinator::moveNoteEditorTabToWindow: note local uid = ")
            << noteLocalUid);

    for(int i = 0; i < m_pTabWidget->count(); ++i)
    {
        NoteEditorWidget * pNoteEditorWidget = qobject_cast<NoteEditorWidget*>(m_pTabWidget->widget(i));
        if (Q_UNLIKELY(!pNoteEditorWidget)) {
            continue;
        }

        if (pNoteEditorWidget->noteLocalUid() != noteLocalUid) {
            continue;
        }

        removeNoteEditorTab(i, /* close editor = */ false);
        insertNoteEditorWidget(pNoteEditorWidget, NoteEditorMode::Window);
        return;
    }

    ErrorString error(QT_TR_NOOP("Could not find the note editor widget to be moved from tab to window"));
    error.details() = noteLocalUid;
    QNWARNING(error);
    Q_EMIT notifyError(error);
}

void NoteEditorTabsAndWindowsCoordinator::moveNoteEditorWindowToTab(NoteEditorWidget * pNoteEditorWidget)
{
    QNDEBUG(QStringLiteral("NoteEditorTabsAndWindowsCoordinator::moveNoteEditorWindowToTab: note local uid = ")
            << (pNoteEditorWidget ? pNoteEditorWidget->noteLocalUid() : QStringLiteral("<null>")));

    if (Q_UNLIKELY(!pNoteEditorWidget)) {
        return;
    }

    pNoteEditorWidget->setAttribute(Qt::WA_DeleteOnClose, false);
    pNoteEditorWidget->makeNonWindow();

    insertNoteEditorWidget(pNoteEditorWidget, NoteEditorMode::Tab);
    pNoteEditorWidget->show();
}

void NoteEditorTabsAndWindowsCoordinator::persistLocalUidsOfNotesInEditorTabs()
{
    QNDEBUG("NoteEditorTabsAndWindowsCoordinator::persistLocalUidsOfNotesInEditorTabs");

    QStringList openNotesLocalUids;
    size_t size = m_localUidsOfNotesInTabbedEditors.size();
    openNotesLocalUids.reserve(static_cast<int>(size));

    for(auto it = m_localUidsOfNotesInTabbedEditors.begin(),
        end = m_localUidsOfNotesInTabbedEditors.end(); it != end; ++it)
    {
        openNotesLocalUids << *it;
    }

    ApplicationSettings appSettings(m_currentAccount, QUENTIER_UI_SETTINGS);
    appSettings.beginGroup(NOTE_EDITOR_SETTINGS_GROUP_NAME);
    appSettings.setValue(OPEN_NOTES_LOCAL_UIDS_IN_TABS_SETTINGS_KEY, openNotesLocalUids);
    appSettings.endGroup();
}

void NoteEditorTabsAndWindowsCoordinator::persistLocalUidsOfNotesInEditorWindows()
{
    QNDEBUG("NoteEditorTabsAndWindowsCoordinator::persistLocalUidsOfNotesInEditorWindows");

    QStringList openNotesLocalUids;
    openNotesLocalUids.reserve(m_noteEditorWindowsByNoteLocalUid.size());

    for(auto it = m_noteEditorWindowsByNoteLocalUid.constBegin(),
        end = m_noteEditorWindowsByNoteLocalUid.constEnd(); it != end; ++it)
    {
        if (Q_UNLIKELY(it.value().isNull())) {
            continue;
        }

        openNotesLocalUids << it.key();
    }

    ApplicationSettings appSettings(m_currentAccount, QUENTIER_UI_SETTINGS);
    appSettings.beginGroup(NOTE_EDITOR_SETTINGS_GROUP_NAME);
    appSettings.setValue(OPEN_NOTES_LOCAL_UIDS_IN_WINDOWS_SETTINGS_KEY, openNotesLocalUids);
    appSettings.endGroup();
}

void NoteEditorTabsAndWindowsCoordinator::persistLastCurrentTabNoteLocalUid()
{
    QNDEBUG(QStringLiteral("NoteEditorTabsAndWindowsCoordinator::persistLastCurrentTabNoteLocalUid: ")
            << m_lastCurrentTabNoteLocalUid);

    ApplicationSettings appSettings(m_currentAccount, QUENTIER_UI_SETTINGS);
    appSettings.beginGroup(NOTE_EDITOR_SETTINGS_GROUP_NAME);
    appSettings.setValue(LAST_CURRENT_TAB_NOTE_LOCAL_UID, m_lastCurrentTabNoteLocalUid);
    appSettings.endGroup();
}

void NoteEditorTabsAndWindowsCoordinator::restoreLastOpenNotes()
{
    QNDEBUG(QStringLiteral("NoteEditorTabsAndWindowsCoordinator::restoreLastOpenNotes"));

    ApplicationSettings appSettings(m_currentAccount, QUENTIER_UI_SETTINGS);
    appSettings.beginGroup(NOTE_EDITOR_SETTINGS_GROUP_NAME);

    QStringList localUidsOfLastNotesInTabs =
            appSettings.value(OPEN_NOTES_LOCAL_UIDS_IN_TABS_SETTINGS_KEY).toStringList();
    QStringList localUidsOfLastNotesInWindows =
            appSettings.value(OPEN_NOTES_LOCAL_UIDS_IN_WINDOWS_SETTINGS_KEY).toStringList();

    m_lastCurrentTabNoteLocalUid = appSettings.value(LAST_CURRENT_TAB_NOTE_LOCAL_UID).toString();
    QNDEBUG(QStringLiteral("Last current tab note local uid: ") << m_lastCurrentTabNoteLocalUid);

    appSettings.endGroup();

    m_trackingCurrentTab = false;

    for(auto it = localUidsOfLastNotesInTabs.constBegin(), end = localUidsOfLastNotesInTabs.constEnd(); it != end; ++it) {
        addNote(*it, NoteEditorMode::Tab);
    }

    for(auto it = localUidsOfLastNotesInWindows.constBegin(), end = localUidsOfLastNotesInWindows.constEnd(); it != end; ++it) {
        addNote(*it, NoteEditorMode::Window);
        restoreNoteEditorWindowGeometry(*it);
    }

    m_trackingCurrentTab = true;

    if (m_lastCurrentTabNoteLocalUid.isEmpty())
    {
        NoteEditorWidget * pCurrentNoteEditorWidget = qobject_cast<NoteEditorWidget*>(m_pTabWidget->currentWidget());
        if (pCurrentNoteEditorWidget && (pCurrentNoteEditorWidget != m_pBlankNoteEditor)) {
            m_lastCurrentTabNoteLocalUid = pCurrentNoteEditorWidget->noteLocalUid();
            persistLastCurrentTabNoteLocalUid();
        }
    }
    else
    {
        for(int i = 0, count = m_pTabWidget->count(); i < count; ++i)
        {
            NoteEditorWidget * pNoteEditorWidget = qobject_cast<NoteEditorWidget*>(m_pTabWidget->widget(i));
            if (Q_UNLIKELY(!pNoteEditorWidget)) {
                QNWARNING(QStringLiteral("Found a widget not being NoteEditorWidget in one of note editor tabs"));
                continue;
            }

            QString noteLocalUid = pNoteEditorWidget->noteLocalUid();
            if (noteLocalUid == m_lastCurrentTabNoteLocalUid) {
                m_pTabWidget->setCurrentIndex(i);
                break;
            }
        }
    }

    return;
}

bool NoteEditorTabsAndWindowsCoordinator::shouldExpungeNote(const NoteEditorWidget & noteEditorWidget) const
{
    QNDEBUG(QStringLiteral("NoteEditorTabsAndWindowsCoordinator::shouldExpungeNote"));

    if (!noteEditorWidget.isNewNote()) {
        QNDEBUG(QStringLiteral("The note within the editor was not a new one, should not expunge it"));
        return false;
    }

    if (noteEditorWidget.hasBeenModified()) {
        QNDEBUG(QStringLiteral("The note within the editor was modified while "
                               "having been loaded into the editor"));
        return false;
    }

    const Note * pNote = noteEditorWidget.currentNote();
    if (!pNote) {
        QNDEBUG(QStringLiteral("There is no note within the editor"));
        return false;
    }

    ApplicationSettings appSettings(m_currentAccount, QUENTIER_UI_SETTINGS);
    appSettings.beginGroup(NOTE_EDITOR_SETTINGS_GROUP_NAME);
    QVariant removeEmptyNotesData = appSettings.value(REMOVE_EMPTY_UNEDITED_NOTES_SETTINGS_KEY);
    appSettings.endGroup();

    bool removeEmptyNotes = DEFAULT_REMOVE_EMPTY_UNEDITED_NOTES;
    if (removeEmptyNotesData.isValid()) {
        removeEmptyNotes = removeEmptyNotesData.toBool();
        QNDEBUG(QStringLiteral("Remove empty notes setting: ")
                << (removeEmptyNotes ? QStringLiteral("true") : QStringLiteral("false")));
    }

    if (removeEmptyNotes) {
        QNDEBUG(QStringLiteral("Will remove empty unedited note with local uid ")
                << pNote->localUid());
        return true;
    }

    QNDEBUG(QStringLiteral("Won't remove the empty note due to setting"));
    return false;
}

void NoteEditorTabsAndWindowsCoordinator::expungeNoteSynchronously(const QString & noteLocalUid)
{
    QNDEBUG(QStringLiteral("NoteEditorTabsAndWindowsCoordinator::expungeNoteSynchronously: ")
            << noteLocalUid);

    ApplicationSettings appSettings(m_currentAccount, QUENTIER_UI_SETTINGS);
    appSettings.beginGroup(NOTE_EDITOR_SETTINGS_GROUP_NAME);
    QVariant expungeNoteTimeoutData = appSettings.value(EXPUNGE_NOTE_TIMEOUT_SETTINGS_KEY);
    appSettings.endGroup();

    bool conversionResult = false;
    int expungeNoteTimeout = expungeNoteTimeoutData.toInt(&conversionResult);
    if (Q_UNLIKELY(!conversionResult)) {
        QNDEBUG(QStringLiteral("Can't read the timeout for note expunging from local storage, "
                               "fallback to the default value of ") << DEFAULT_EXPUNGE_NOTE_TIMEOUT
                << QStringLiteral(" milliseconds"));
        expungeNoteTimeout = DEFAULT_EXPUNGE_NOTE_TIMEOUT;
    }
    else {
        expungeNoteTimeout = std::max(expungeNoteTimeout, 100);
    }

    if (m_pExpungeNoteDeadlineTimer) {
        m_pExpungeNoteDeadlineTimer->deleteLater();
    }

    m_pExpungeNoteDeadlineTimer = new QTimer(this);
    m_pExpungeNoteDeadlineTimer->setSingleShot(true);

    EventLoopWithExitStatus eventLoop;
    QObject::connect(m_pExpungeNoteDeadlineTimer, QNSIGNAL(QTimer,timeout),
                     &eventLoop, QNSLOT(EventLoopWithExitStatus,exitAsTimeout));
    QObject::connect(this, QNSIGNAL(NoteEditorTabsAndWindowsCoordinator,noteExpungedFromLocalStorage),
                     &eventLoop, QNSLOT(EventLoopWithExitStatus,exitAsSuccess));
    QObject::connect(this, QNSIGNAL(NoteEditorTabsAndWindowsCoordinator,noteExpungeFromLocalStorageFailed),
                     &eventLoop, QNSLOT(EventLoopWithExitStatus,exitAsFailure));

    m_pExpungeNoteDeadlineTimer->start(expungeNoteTimeout);

    m_localUidOfNoteToBeExpunged = noteLocalUid;
    QTimer::singleShot(0, this, SLOT(expungeNoteFromLocalStorage()));

    int result = eventLoop.exec(QEventLoop::ExcludeUserInputEvents);
    if (result == EventLoopWithExitStatus::ExitStatus::Failure) {
        QNWARNING(QStringLiteral("Failed to expunge the empty unedited note from local storage"));
    }
    else if (result == EventLoopWithExitStatus::ExitStatus::Timeout) {
        QNWARNING(QStringLiteral("The expunging of note from local storage took too much time to finish"));
    }
    else {
        QNDEBUG(QStringLiteral("Successfully expunged the note from local storage"));
    }
}

void NoteEditorTabsAndWindowsCoordinator::checkPendingRequestsAndDisconnectFromLocalStorage()
{
    QNDEBUG(QStringLiteral("NoteEditorTabsAndWindowsCoordinator::checkPendingRequestsAndDisconnectFromLocalStorage"));

    if (m_noteEditorModeByCreateNoteRequestIds.empty() &&
        m_expungeNoteRequestIds.isEmpty() &&
        m_inAppNoteLinkFindNoteRequestIds.isEmpty())
    {
        disconnectFromLocalStorage();
    }
}

} // namespace quentier
