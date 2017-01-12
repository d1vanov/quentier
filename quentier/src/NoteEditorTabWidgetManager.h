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

#ifndef QUENTIER_WIDGETS_NOTE_EDITOR_TAB_WIDGET_MANAGER_H
#define QUENTIER_WIDGETS_NOTE_EDITOR_TAB_WIDGET_MANAGER_H

#include "models/NoteCache.h"
#include "models/NotebookCache.h"
#include "models/TagCache.h"
#include <quentier/types/Account.h>
#include <quentier/utility/LRUCache.hpp>
#include <quentier/types/Note.h>
#include <QTabWidget>
#include <QSet>
#include <QUuid>

// NOTE: Workaround a bug in Qt4 which may prevent building with some boost versions
#ifndef Q_MOC_RUN
#include <boost/circular_buffer.hpp>
#endif

QT_FORWARD_DECLARE_CLASS(QUndoStack)

namespace quentier {

QT_FORWARD_DECLARE_CLASS(TagModel)
QT_FORWARD_DECLARE_CLASS(LocalStorageManagerThreadWorker)
QT_FORWARD_DECLARE_CLASS(Note)
QT_FORWARD_DECLARE_CLASS(NoteEditorWidget)
QT_FORWARD_DECLARE_CLASS(TabWidget)

class NoteEditorTabWidgetManager: public QObject
{
    Q_OBJECT
public:
    explicit NoteEditorTabWidgetManager(const Account & account, LocalStorageManagerThreadWorker & localStorageWorker,
                                        NoteCache & noteCache, NotebookCache & notebookCache,
                                        TagCache & tagCache, TagModel & tagModel,
                                        TabWidget * tabWidget, QObject * parent = Q_NULLPTR);

    int maxNumNotes() const { return m_maxNumNotes; }
    void setMaxNumNotes(const int maxNumNotes);

    int numNotes() const;

    void addNote(const QString & noteLocalUid);

    void createNewNote(const QString & notebookLocalUid, const QString & notebookGuid);

Q_SIGNALS:
    void notifyError(QNLocalizedString error);

    // private signals
    void requestAddNote(Note note, QUuid requestId);

private Q_SLOTS:
    void onNoteEditorWidgetResolved();
    void onNoteTitleOrPreviewTextChanged(QString titleOrPreview);
    void onNoteEditorTabCloseRequested(int tabIndex);

    void onNoteLoadedInEditor();

    void onAddNoteComplete(Note note, QUuid requestId);
    void onAddNoteFailed(Note note, QNLocalizedString errorDescription, QUuid requestId);

private:
    void insertNoteEditorWidget(NoteEditorWidget * pNoteEditorWidget);
    void checkAndCloseOlderNoteEditors();

private:
    virtual bool eventFilter(QObject * pWatched, QEvent * pEvent) Q_DECL_OVERRIDE;

    void connectToLocalStorage();
    void disconnectFromLocalStorage();

    QString shortenTabName(const QString & tabName) const;

private:
    Account                             m_currentAccount;
    LocalStorageManagerThreadWorker &   m_localStorageWorker;
    NoteCache &                         m_noteCache;
    NotebookCache &                     m_notebookCache;
    TagCache &                          m_tagCache;
    TagModel &                          m_tagModel;

    TabWidget *                         m_pTabWidget;
    NoteEditorWidget *                  m_pBlankNoteEditor;

    int                                 m_maxNumNotes;

    boost::circular_buffer<QString>     m_shownNoteLocalUids;

    QSet<QUuid>                         m_createNoteRequestIds;
};

} // namespace quentier

#endif // QUENTIER_WIDGETS_NOTE_EDITOR_TAB_WIDGET_MANAGER_H
