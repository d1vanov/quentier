/*
 * Copyright 2017-2021 Dmitry Ivanov
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

#include <lib/model/note/NoteCache.h>
#include <lib/model/notebook/NotebookCache.h>
#include <lib/model/tag/TagCache.h>

#include <quentier/local_storage/LocalStorageManager.h>
#include <quentier/types/Account.h>
#include <quentier/utility/LRUCache.hpp>

#include <qevercloud/types/Note.h>

#include <QHash>
#include <QMap>
#include <QPointer>
#include <QSet>
#include <QTabWidget>
#include <QUuid>

#include <boost/bimap.hpp>
#include <boost/circular_buffer.hpp>

class QDebug;
class QMenu;
class QThread;
class QUndoStack;

namespace quentier {

class FileIOProcessorAsync;
class LocalStorageManagerAsync;
class NoteEditorWidget;
class SpellChecker;
class TagModel;
class TabWidget;

class NoteEditorTabsAndWindowsCoordinator final : public QObject
{
    Q_OBJECT
public:
    explicit NoteEditorTabsAndWindowsCoordinator(
        Account account,
        LocalStorageManagerAsync & localStorageManagerAsync,
        NoteCache & noteCache, NotebookCache & notebookCache,
        TagCache & tagCache, TagModel & tagModel, TabWidget * tabWidget,
        QObject * parent = nullptr);

    ~NoteEditorTabsAndWindowsCoordinator() override;

    // Closes all note editor windows and tabs; should be called when the app is
    // about to quit, called automatically when the account is switched
    void clear();

    void switchAccount(const Account & account, quentier::TagModel & tagModel);

    [[nodiscard]] int maxNumNotesInTabs() const
    {
        return m_maxNumNotesInTabs;
    }

    void setMaxNumNotesInTabs(int maxNumNotesInTabs);

    [[nodiscard]] int numNotesInTabs() const;

    NoteEditorWidget * noteEditorWidgetForNoteLocalId(
        const QString & noteLocalId);

    enum class NoteEditorMode
    {
        Tab,
        Window,
        Any
    };

    friend QDebug & operator<<(QDebug & dbg, NoteEditorMode mode);

    void addNote(
        const QString & noteLocalId,
        NoteEditorMode noteEditorMode = NoteEditorMode::Any,
        bool isNewNote = false);

    void createNewNote(
        const QString & notebookLocalId, const QString & notebookGuid,
        NoteEditorMode noteEditorMode);

    void setUseLimitedFonts(bool flag);

    void refreshNoteEditorWidgetsSpecialIcons();

    void saveAllNoteEditorsContents();

    [[nodiscard]] qint64 minIdleTime() const;

Q_SIGNALS:
    void notifyError(ErrorString error);

    void currentNoteChanged(QString noteLocalId);

    // private signals
    void requestAddNote(qevercloud::Note note, QUuid requestId);
    void requestExpungeNote(qevercloud::Note note, QUuid requestId);

    void findNote(
        qevercloud::Note note, LocalStorageManager::GetNoteOptions options,
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

    void onAddNoteComplete(qevercloud::Note note, QUuid requestId);
    void onAddNoteFailed(
        qevercloud::Note note, ErrorString errorDescription, QUuid requestId);

    void onFindNoteComplete(
        qevercloud::Note note, LocalStorageManager::GetNoteOptions options,
        QUuid requestId);

    void onFindNoteFailed(
        qevercloud::Note note, LocalStorageManager::GetNoteOptions options,
        ErrorString errorDescription, QUuid requestId);

    void onExpungeNoteComplete(qevercloud::Note note, QUuid requestId);

    void onExpungeNoteFailed(
        qevercloud::Note note, ErrorString errorDescription, QUuid requestId);

    void onCurrentTabChanged(int currentIndex);

    void onTabContextMenuRequested(const QPoint & pos);
    void onTabContextMenuCloseEditorAction();
    void onTabContextMenuSaveNoteAction();
    void onTabContextMenuMoveToWindowAction();

    void expungeNoteFromLocalStorage();

private:
    [[nodiscard]] bool eventFilter(
        QObject * pWatched, QEvent * pEvent) override;

    void timerEvent(QTimerEvent * pTimerEvent) override;

private:
    void insertNoteEditorWidget(
        NoteEditorWidget * pNoteEditorWidget,
        NoteEditorMode noteEditorMode);

    void removeNoteEditorTab(int tabIndex, bool closeEditor);
    void checkAndCloseOlderNoteEditorTabs();
    void setCurrentNoteEditorWidgetTab(const QString & noteLocalId);

    void scheduleNoteEditorWindowGeometrySave(const QString & noteLocalId);
    void persistNoteEditorWindowGeometry(const QString & noteLocalId);
    void clearPersistedNoteEditorWindowGeometry(const QString & noteLocalId);
    void restoreNoteEditorWindowGeometry(const QString & noteLocalId);

private:
    void connectToLocalStorage();
    void disconnectFromLocalStorage();

    void connectNoteEditorWidgetToColorChangeSignals(NoteEditorWidget & widget);

    void setupFileIO();
    void setupSpellChecker();

    [[nodiscard]] QString shortenEditorName(
        const QString & name, int maxSize = -1) const;

    void moveNoteEditorTabToWindow(const QString & noteLocalId);
    void moveNoteEditorWindowToTab(NoteEditorWidget * pNoteEditorWidget);

    void persistLocalIdsOfNotesInEditorTabs();
    void persistLocalIdsOfNotesInEditorWindows();
    void persistLastCurrentTabNoteLocalId();
    void restoreLastOpenNotes();

    [[nodiscard]] bool shouldExpungeNote(
        const NoteEditorWidget & noteEditorWidget) const;

    void expungeNoteSynchronously(const QString & noteLocalId);

    void checkPendingRequestsAndDisconnectFromLocalStorage();

private:
    Account m_currentAccount;
    LocalStorageManagerAsync & m_localStorageManagerAsync;
    NoteCache & m_noteCache;
    NotebookCache & m_notebookCache;
    TagCache & m_tagCache;
    QPointer<TagModel> m_pTagModel;

    TabWidget * m_pTabWidget;
    NoteEditorWidget * m_pBlankNoteEditor = nullptr;

    QThread * m_pIOThread = nullptr;
    FileIOProcessorAsync * m_pFileIOProcessorAsync = nullptr;
    SpellChecker * m_pSpellChecker = nullptr;

    bool m_connectedToLocalStorage = false;

    int m_maxNumNotesInTabs = -1;

    boost::circular_buffer<QString> m_localIdsOfNotesInTabbedEditors;
    QString m_lastCurrentTabNoteLocalId;

    using NoteEditorWindowsByNoteLocalId =
        QMap<QString, QPointer<NoteEditorWidget>>;

    NoteEditorWindowsByNoteLocalId m_noteEditorWindowsByNoteLocalId;

    using NoteLocalIdToTimerIdBimap = boost::bimap<QString, int>;

    NoteLocalIdToTimerIdBimap
        m_saveNoteEditorWindowGeometryPostponeTimerIdToNoteLocalIdBimap;

    QHash<QUuid, NoteEditorMode> m_noteEditorModeByCreateNoteRequestIds;
    QSet<QUuid> m_expungeNoteRequestIds;
    QSet<QUuid> m_inAppNoteLinkFindNoteRequestIds;

    QMenu * m_pTabBarContextMenu = nullptr;

    QString m_localIdOfNoteToBeExpunged;
    QTimer * m_pExpungeNoteDeadlineTimer = nullptr;

    bool m_trackingCurrentTab = true;
};

} // namespace quentier

#endif // QUENTIER_LIB_WIDGET_NOTE_EDITOR_TABS_AND_WINDOWS_COORDINATOR_H
