/*
 * Copyright 2017-2020 Dmitry Ivanov
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

#ifndef QUENTIER_LIB_WIDGET_NOTE_EDITOR_TABS_AND_WINDOWS_COORDINATOR_H
#define QUENTIER_LIB_WIDGET_NOTE_EDITOR_TABS_AND_WINDOWS_COORDINATOR_H

#include <lib/model/NotebookCache.h>
#include <lib/model/NoteCache.h>
#include <lib/model/TagCache.h>

#include <quentier/local_storage/LocalStorageManager.h>
#include <quentier/types/Account.h>
#include <quentier/types/Note.h>
#include <quentier/utility/LRUCache.hpp>
#include <quentier/utility/SuppressWarnings.h>

#include <QHash>
#include <QMap>
#include <QPointer>
#include <QSet>
#include <QTabWidget>
#include <QUuid>

SAVE_WARNINGS
GCC_SUPPRESS_WARNING(-Wdeprecated-declarations)

#include <boost/bimap.hpp>
#include <boost/circular_buffer.hpp>

RESTORE_WARNINGS

QT_FORWARD_DECLARE_CLASS(QMenu)
QT_FORWARD_DECLARE_CLASS(QThread)
QT_FORWARD_DECLARE_CLASS(QUndoStack)

namespace quentier {

QT_FORWARD_DECLARE_CLASS(FileIOProcessorAsync)
QT_FORWARD_DECLARE_CLASS(LocalStorageManagerAsync)
QT_FORWARD_DECLARE_CLASS(Note)
QT_FORWARD_DECLARE_CLASS(NoteEditorWidget)
QT_FORWARD_DECLARE_CLASS(SpellChecker)
QT_FORWARD_DECLARE_CLASS(TagModel)
QT_FORWARD_DECLARE_CLASS(TabWidget)

class NoteEditorTabsAndWindowsCoordinator: public QObject
{
    Q_OBJECT
public:
    explicit NoteEditorTabsAndWindowsCoordinator(
        const Account & account,
        LocalStorageManagerAsync & localStorageManagerAsync,
        NoteCache & noteCache, NotebookCache & notebookCache,
        TagCache & tagCache, TagModel & tagModel,
        TabWidget * tabWidget, QObject * parent = nullptr);

    virtual ~NoteEditorTabsAndWindowsCoordinator() override;

    // Closes all note editor windows and tabs; should be called when the app is
    // about to quit, called automatically when the account is switched
    void clear();

    void switchAccount(const Account & account, quentier::TagModel & tagModel);

    int maxNumNotesInTabs() const
    {
        return m_maxNumNotesInTabs;
    }

    void setMaxNumNotesInTabs(const int maxNumNotesInTabs);

    int numNotesInTabs() const;

    NoteEditorWidget * noteEditorWidgetForNoteLocalUid(
        const QString & noteLocalUid);

    struct NoteEditorMode
    {
        enum type
        {
            Tab = 0,
            Window,
            Any
        };
    };

    void addNote(
        const QString & noteLocalUid,
        const NoteEditorMode::type noteEditorMode = NoteEditorMode::Any,
        const bool isNewNote = false);

    void createNewNote(
        const QString & notebookLocalUid,
        const QString & notebookGuid,
        const NoteEditorMode::type noteEditorMode);

    void setUseLimitedFonts(const bool flag);

    void refreshNoteEditorWidgetsSpecialIcons();

    void saveAllNoteEditorsContents();

    qint64 minIdleTime() const;

Q_SIGNALS:
    void notifyError(ErrorString error);

    void currentNoteChanged(QString noteLocalUid);

    // private signals
    void requestAddNote(Note note, QUuid requestId);
    void requestExpungeNote(Note note, QUuid requestId);

    void findNote(
        Note note, LocalStorageManager::GetNoteOptions options,
        QUuid requestId);

    void noteExpungedFromLocalStorage();
    void noteExpungeFromLocalStorageFailed();

    void noteEditorFontColorChanged(QColor color);
    void noteEditorBackgroundColorChanged(QColor color);
    void noteEditorHighlightColorChanged(QColor color);
    void noteEditorHighlightedTextColorChanged(QColor color);
    void noteEditorColorsReset();

private Q_SLOTS:
    void onNoteEditorWidgetResolved();
    void onNoteEditorWidgetInvalidated();
    void onNoteTitleOrPreviewTextChanged(QString titleOrPreview);
    void onNoteEditorTabCloseRequested(int tabIndex);

    void onInAppNoteLinkClicked(
        QString userId, QString shardId, QString noteGuid);

    void onNoteLoadedInEditor();
    void onNoteEditorError(ErrorString errorDescription);

    void onAddNoteComplete(Note note, QUuid requestId);
    void onAddNoteFailed(
        Note note, ErrorString errorDescription, QUuid requestId);

    void onFindNoteComplete(
        Note note, LocalStorageManager::GetNoteOptions options,
        QUuid requestId);

    void onFindNoteFailed(
        Note note, LocalStorageManager::GetNoteOptions options,
        ErrorString errorDescription, QUuid requestId);

    void onExpungeNoteComplete(Note note, QUuid requestId);

    void onExpungeNoteFailed(
        Note note, ErrorString errorDescription, QUuid requestId);

    void onCurrentTabChanged(int currentIndex);

    void onTabContextMenuRequested(const QPoint & pos);
    void onTabContextMenuCloseEditorAction();
    void onTabContextMenuSaveNoteAction();
    void onTabContextMenuMoveToWindowAction();

    void expungeNoteFromLocalStorage();

private:
    virtual bool eventFilter(QObject * pWatched, QEvent * pEvent) override;
    virtual void timerEvent(QTimerEvent * pTimerEvent) override;

private:
    void insertNoteEditorWidget(
        NoteEditorWidget * pNoteEditorWidget,
        const NoteEditorMode::type noteEditorMode);

    void removeNoteEditorTab(int tabIndex, const bool closeEditor);
    void checkAndCloseOlderNoteEditorTabs();
    void setCurrentNoteEditorWidgetTab(const QString & noteLocalUid);

    void scheduleNoteEditorWindowGeometrySave(const QString & noteLocalUid);
    void persistNoteEditorWindowGeometry(const QString & noteLocalUid);
    void clearPersistedNoteEditorWindowGeometry(const QString & noteLocalUid);
    void restoreNoteEditorWindowGeometry(const QString & noteLocalUid);

private:
    void connectToLocalStorage();
    void disconnectFromLocalStorage();

    void connectNoteEditorWidgetToColorChangeSignals(NoteEditorWidget & widget);

    void setupFileIO();
    void setupSpellChecker();

    QString shortenEditorName(const QString & name, int maxSize = -1) const;

    void moveNoteEditorTabToWindow(const QString & noteLocalUid);
    void moveNoteEditorWindowToTab(NoteEditorWidget * pNoteEditorWidget);

    void persistLocalUidsOfNotesInEditorTabs();
    void persistLocalUidsOfNotesInEditorWindows();
    void persistLastCurrentTabNoteLocalUid();
    void restoreLastOpenNotes();

    bool shouldExpungeNote(const NoteEditorWidget & noteEditorWidget) const;
    void expungeNoteSynchronously(const QString & noteLocalUid);

    void checkPendingRequestsAndDisconnectFromLocalStorage();

private:
    Account                             m_currentAccount;
    LocalStorageManagerAsync &          m_localStorageManagerAsync;
    NoteCache &                         m_noteCache;
    NotebookCache &                     m_notebookCache;
    TagCache &                          m_tagCache;
    QPointer<TagModel>                  m_pTagModel;

    TabWidget *                         m_pTabWidget;
    NoteEditorWidget *                  m_pBlankNoteEditor = nullptr;

    QThread *                           m_pIOThread = nullptr;
    FileIOProcessorAsync *              m_pFileIOProcessorAsync = nullptr;
    SpellChecker *                      m_pSpellChecker = nullptr;

    bool                                m_connectedToLocalStorage = false;

    int                                 m_maxNumNotesInTabs = -1;

    boost::circular_buffer<QString>     m_localUidsOfNotesInTabbedEditors;
    QString                             m_lastCurrentTabNoteLocalUid;

    using NoteEditorWindowsByNoteLocalUid =
        QMap<QString, QPointer<NoteEditorWidget>>;

    NoteEditorWindowsByNoteLocalUid     m_noteEditorWindowsByNoteLocalUid;

    using NoteLocalUidToTimerIdBimap = boost::bimap<QString, int>;

    NoteLocalUidToTimerIdBimap          m_saveNoteEditorWindowGeometryPostponeTimerIdToNoteLocalUidBimap;

    QHash<QUuid, NoteEditorMode::type>  m_noteEditorModeByCreateNoteRequestIds;
    QSet<QUuid>                         m_expungeNoteRequestIds;
    QSet<QUuid>                         m_inAppNoteLinkFindNoteRequestIds;

    QMenu *                             m_pTabBarContextMenu = nullptr;

    QString                             m_localUidOfNoteToBeExpunged;
    QTimer *                            m_pExpungeNoteDeadlineTimer = nullptr;

    bool                                m_trackingCurrentTab = true;
};

} // namespace quentier

#endif // QUENTIER_LIB_WIDGET_NOTE_EDITOR_TABS_AND_WINDOWS_COORDINATOR_H
