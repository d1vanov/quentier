/*
 * Copyright 2017-2024 Dmitry Ivanov
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

#include <lib/model/note/NoteCache.h>
#include <lib/model/notebook/NotebookCache.h>
#include <lib/model/tag/TagCache.h>

#include <quentier/local_storage/Fwd.h>
#include <quentier/types/Account.h>
#include <quentier/types/ErrorString.h>
#include <quentier/utility/LRUCache.hpp>
#include <quentier/utility/SuppressWarnings.h>
#include <quentier/utility/cancelers/Fwd.h>

#include <QHash>
#include <QMap>
#include <QPointer>
#include <QSet>
#include <QTabWidget>

SAVE_WARNINGS

MSVC_SUPPRESS_WARNING(4834)

#include <boost/bimap.hpp>
#include <boost/circular_buffer.hpp>

RESTORE_WARNINGS

class QDebug;
class QMenu;
class QThread;
class QUndoStack;

namespace quentier {

class FileIOProcessorAsync;
class NoteEditorWidget;
class SpellChecker;
class TagModel;
class TabWidget;

class NoteEditorTabsAndWindowsCoordinator : public QObject
{
    Q_OBJECT
public:
    explicit NoteEditorTabsAndWindowsCoordinator(
        Account account, local_storage::ILocalStoragePtr localStorage,
        NoteCache & noteCache, NotebookCache & notebookCache,
        TagCache & tagCache, TagModel & tagModel, TabWidget * tabWidget,
        QObject * parent = nullptr);

    ~NoteEditorTabsAndWindowsCoordinator() override;

    // Closes all note editor windows and tabs; should be called when the app is
    // about to quit, called automatically when the account is switched
    void clear();

    void switchAccount(const Account & account, quentier::TagModel & tagModel);

    [[nodiscard]] int maxNumNotesInTabs() const noexcept
    {
        return m_maxNumNotesInTabs;
    }

    void setMaxNumNotesInTabs(const int maxNumNotesInTabs);

    [[nodiscard]] int numNotesInTabs() const;

    [[nodiscard]] NoteEditorWidget * noteEditorWidgetForNoteLocalId(
        const QString & noteLocalId);

    enum class NoteEditorMode
    {
        Tab = 0,
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

    void onCurrentTabChanged(int currentIndex);

    void onTabContextMenuRequested(const QPoint & pos);
    void onTabContextMenuCloseEditorAction();
    void onTabContextMenuSaveNoteAction();
    void onTabContextMenuMoveToWindowAction();

    void expungeNoteFromLocalStorage();

private:
    bool eventFilter(QObject * watched, QEvent * event) override;
    void timerEvent(QTimerEvent * timerEvent) override;

private:
    void insertNoteEditorWidget(
        NoteEditorWidget * noteEditorWidget, NoteEditorMode noteEditorMode);

    void removeNoteEditorTab(int tabIndex, bool closeEditor);
    void checkAndCloseOlderNoteEditorTabs();
    void setCurrentNoteEditorWidgetTab(const QString & noteLocalId);

    void scheduleNoteEditorWindowGeometrySave(const QString & noteLocalId);
    void persistNoteEditorWindowGeometry(const QString & noteLocalId);
    void clearPersistedNoteEditorWindowGeometry(const QString & noteLocalId);
    void restoreNoteEditorWindowGeometry(const QString & noteLocalId);

private:
    void connectNoteEditorWidgetToColorChangeSignals(NoteEditorWidget & widget);

    void setupFileIO();
    void setupSpellChecker();

    QString shortenEditorName(const QString & name, int maxSize = -1) const;

    void moveNoteEditorTabToWindow(const QString & noteLocalId);
    void moveNoteEditorWindowToTab(NoteEditorWidget * noteEditorWidget);

    void persistLocalIdsOfNotesInEditorTabs();
    void persistLocalIdsOfNotesInEditorWindows();
    void persistLastCurrentTabNoteLocalId();
    void restoreLastOpenNotes();

    bool shouldExpungeNote(const NoteEditorWidget & noteEditorWidget) const;
    void expungeNoteSynchronously(const QString & noteLocalId);

    [[nodiscard]] utility::cancelers::ICancelerPtr setupCanceler();

private:
    local_storage::ILocalStoragePtr m_localStorage;
    Account m_currentAccount;

    NoteCache & m_noteCache;
    NotebookCache & m_notebookCache;
    TagCache & m_tagCache;
    QPointer<TagModel> m_tagModel;

    TabWidget * m_tabWidget;
    NoteEditorWidget * m_blankNoteEditor = nullptr;

    QThread * m_ioThread = nullptr;
    FileIOProcessorAsync * m_fileIOProcessorAsync = nullptr;
    SpellChecker * m_spellChecker = nullptr;

    utility::cancelers::ManualCancelerPtr m_canceler;

    int m_maxNumNotesInTabs = -1;

    boost::circular_buffer<QString> m_localIdsOfNotesInTabbedEditors;
    QString m_lastCurrentTabNoteLocalId;

    using NoteEditorWindowsByNoteLocalId =
        QMap<QString, QPointer<NoteEditorWidget>>;

    NoteEditorWindowsByNoteLocalId m_noteEditorWindowsByNoteLocalId;

    using NoteLocalIdToTimerIdBimap = boost::bimap<QString, int>;

    NoteLocalIdToTimerIdBimap
        m_saveNoteEditorWindowGeometryPostponeTimerIdToNoteLocalIdBimap;

    QMenu * m_tabBarContextMenu = nullptr;

    QString m_localIdOfNoteToBeExpunged;
    QTimer * m_expungeNoteDeadlineTimer = nullptr;

    bool m_trackingCurrentTab = true;
};

} // namespace quentier
