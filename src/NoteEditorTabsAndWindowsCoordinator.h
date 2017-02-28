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

#ifndef QUENTIER_WIDGETS_NOTE_EDITOR_TABS_AND_WINDOWS_COORDINATOR_H
#define QUENTIER_WIDGETS_NOTE_EDITOR_TABS_AND_WINDOWS_COORDINATOR_H

#include "models/NoteCache.h"
#include "models/NotebookCache.h"
#include "models/TagCache.h"
#include <quentier/types/Account.h>
#include <quentier/utility/LRUCache.hpp>
#include <quentier/types/Note.h>
#include <QTabWidget>
#include <QSet>
#include <QMap>
#include <QPointer>
#include <QUuid>

// NOTE: Workaround a bug in Qt4 which may prevent building with some boost versions
#ifndef Q_MOC_RUN
#include <boost/circular_buffer.hpp>
#endif

QT_FORWARD_DECLARE_CLASS(QUndoStack)
QT_FORWARD_DECLARE_CLASS(QThread)
QT_FORWARD_DECLARE_CLASS(QMenu)

namespace quentier {

QT_FORWARD_DECLARE_CLASS(TagModel)
QT_FORWARD_DECLARE_CLASS(LocalStorageManagerThreadWorker)
QT_FORWARD_DECLARE_CLASS(Note)
QT_FORWARD_DECLARE_CLASS(NoteEditorWidget)
QT_FORWARD_DECLARE_CLASS(TabWidget)
QT_FORWARD_DECLARE_CLASS(FileIOThreadWorker)
QT_FORWARD_DECLARE_CLASS(SpellChecker)

class NoteEditorTabsAndWindowsCoordinator: public QObject
{
    Q_OBJECT
public:
    explicit NoteEditorTabsAndWindowsCoordinator(const Account & account, LocalStorageManagerThreadWorker & localStorageWorker,
                                                 NoteCache & noteCache, NotebookCache & notebookCache,
                                                 TagCache & tagCache, TagModel & tagModel,
                                                 TabWidget * tabWidget, QObject * parent = Q_NULLPTR);
    ~NoteEditorTabsAndWindowsCoordinator();

    // Closes all note editor windows and tabs;
    // should be called when the app is about to quit, called automatically when the account is switched
    void clear();

    void switchAccount(const Account & account, quentier::TagModel & tagModel);

    int maxNumNotesInTabs() const { return m_maxNumNotesInTabs; }
    void setMaxNumNotesInTabs(const int maxNumNotesInTabs);

    int numNotesInTabs() const;

    struct NoteEditorMode
    {
        enum type
        {
            Tab = 0,
            Window,
            Any
        };
    };

    void addNote(const QString & noteLocalUid, const NoteEditorMode::type noteEditorMode = NoteEditorMode::Any);

    void createNewNote(const QString & notebookLocalUid, const QString & notebookGuid);

    virtual bool eventFilter(QObject * pWatched, QEvent * pEvent) Q_DECL_OVERRIDE;

Q_SIGNALS:
    void notifyError(ErrorString error);

    void currentNoteChanged(QString noteLocalUid);

    // private signals
    void requestAddNote(Note note, QUuid requestId);

private Q_SLOTS:
    void onNoteEditorWidgetResolved();
    void onNoteEditorWidgetInvalidated();
    void onNoteTitleOrPreviewTextChanged(QString titleOrPreview);
    void onNoteEditorTabCloseRequested(int tabIndex);

    void onNoteLoadedInEditor();
    void onNoteEditorError(ErrorString errorDescription);

    void onAddNoteComplete(Note note, QUuid requestId);
    void onAddNoteFailed(Note note, ErrorString errorDescription, QUuid requestId);

    void onCurrentTabChanged(int currentIndex);

    void onTabContextMenuRequested(const QPoint & pos);
    void onTabContextMenuCloseEditorAction();
    void onTabContextMenuSaveNoteAction();
    void onTabContextMenuMoveToWindowAction();

private:
    void insertNoteEditorWidget(NoteEditorWidget * pNoteEditorWidget, const NoteEditorMode::type noteEditorMode);

    void removeNoteEditorTab(int tabIndex, const bool closeEditor);
    void checkAndCloseOlderNoteEditorTabs();
    void setCurrentNoteEditorWidgetTab(const QString & noteLocalUid);

private:
    void connectToLocalStorage();
    void disconnectFromLocalStorage();

    void setupFileIO();
    void setupSpellChecker();

    QString shortenEditorName(const QString & name, int maxSize = -1) const;

    void moveNoteEditorTabToWindow(const QString & noteLocalUid);
    void moveNoteEditorWindowToTab(NoteEditorWidget * pNoteEditorWidget);

    void persistLocalUidsOfNotesInEditorTabs();
    void persistLocalUidsOfNotesInEditorWindows();
    void persistLastCurrentTabNoteLocalUid();
    void restoreLastOpenNotes();

private:
    Account                             m_currentAccount;
    LocalStorageManagerThreadWorker &   m_localStorageWorker;
    NoteCache &                         m_noteCache;
    NotebookCache &                     m_notebookCache;
    TagCache &                          m_tagCache;
    QPointer<TagModel>                  m_pTagModel;

    TabWidget *                         m_pTabWidget;
    NoteEditorWidget *                  m_pBlankNoteEditor;

    QThread *                           m_pIOThread;
    FileIOThreadWorker *                m_pFileIOThreadWorker;
    SpellChecker *                      m_pSpellChecker;

    int                                 m_maxNumNotesInTabs;

    boost::circular_buffer<QString>     m_localUidsOfNotesInTabbedEditors;
    QString                             m_lastCurrentTabNoteLocalUid;

    typedef QMap<QString, QPointer<NoteEditorWidget> > NoteEditorWindowsByNoteLocalUid;
    NoteEditorWindowsByNoteLocalUid     m_noteEditorWindowsByNoteLocalUid;

    QSet<QUuid>                         m_createNoteRequestIds;

    QMenu *                             m_pTabBarContextMenu;

    bool                                m_trackingCurrentTab;
};

} // namespace quentier

#endif // QUENTIER_WIDGETS_NOTE_EDITOR_TABS_AND_WINDOWS_COORDINATOR_H
