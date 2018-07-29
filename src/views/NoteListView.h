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

#ifndef QUENTIER_VIEWS_NOTE_LIST_VIEW_H
#define QUENTIER_VIEWS_NOTE_LIST_VIEW_H

#include <quentier/utility/Macros.h>
#include <quentier/types/ErrorString.h>
#include <quentier/types/Account.h>
#include <QListView>

QT_FORWARD_DECLARE_CLASS(QMenu)

namespace quentier {

QT_FORWARD_DECLARE_CLASS(NotebookItemView)
QT_FORWARD_DECLARE_CLASS(NotebookItem)
QT_FORWARD_DECLARE_CLASS(NoteModel)
QT_FORWARD_DECLARE_CLASS(NoteFilterModel)

/**
 * @brief The NoteListView is a simple subclass of QListView which adds some bits of functionality specific to note list
 * on top of it
 */
class NoteListView: public QListView
{
    Q_OBJECT
public:
    explicit NoteListView(QWidget * parent = Q_NULLPTR);

    void setNotebookItemView(NotebookItemView * pNotebookItemView);

    /**
     * After this method is called, NoteListView would automatically select the first note of the model
     * after the next event of note item addition. It would happen only once.
     *
     * This method is handy to run before starting the synchronization of a fresh Evernote account
     */
    void setAutoSelectNoteOnNextAddition();

    /**
     * @return local uids of selected notes
     */
    QStringList selectedNotesLocalUids() const;

    /**
     * @return the local uid of the current note
     */
    QString currentNoteLocalUid() const;

    /**
     * @return the current account
     */
    const Account & currentAccount() const;

    void setCurrentAccount(const Account & account);

Q_SIGNALS:
    void notifyError(ErrorString errorDescription);
    void currentNoteChanged(QString noteLocalUid);

    void newNoteCreationRequested();
    void editNoteDialogRequested(QString noteLocalUid);
    void noteInfoDialogRequested(QString noteLocalUid);
    void openNoteInSeparateWindowRequested(QString noteLocalUid);
    void copyInAppNoteLinkRequested(QString noteLocalUid, QString noteGuid);

    void enexExportRequested(QStringList noteLocalUids);

    void toggleThumbnailsPreference(QString noteLocalUid);

public Q_SLOTS:
    /**
     * The slot which can watch for external changes of current note
     * and reflect the change of the current item in the view
     */
    void setCurrentNoteByLocalUid(QString noteLocalUid);

    /**
     * The slot which can watch for external changes in thumbnail display state.
     * @param showThumbnailsForAllNotes Global flag for all notes.
     * @param hideThumbnailsLocalUids  Map with local uids where the thumbails was manually hidden.
     */
    void setShowNoteThumbnailsState(bool showThumbnailsForAllNotes, const QSet<QString> & hideThumbnailsLocalUids);


    /**
     * The slot which can watch for external changes of selected notes
     * and reflect that change in the view
     */
    void selectNotesByLocalUids(const QStringList & noteLocalUids);

    /**
     * @brief The dataChanged method is redefined in NoteListView for the sole reason of being a public slot
     * instead of protected; it calls the implementation of QListView's dataChanged protected slot
     */
    virtual void dataChanged(const QModelIndex & topLeft, const QModelIndex & bottomRight
#if QT_VERSION < 0x050000
                            )
#else
                            , const QVector<int> & roles = QVector<int>())
#endif
                            Q_DECL_OVERRIDE;

    virtual void rowsAboutToBeRemoved(const QModelIndex & parent, int start, int end) Q_DECL_OVERRIDE;
    virtual void rowsInserted(const QModelIndex & parent, int start, int end) Q_DECL_OVERRIDE;

protected Q_SLOTS:
    void onCreateNewNoteAction();
    void onDeleteNoteAction();
    void onEditNoteAction();
    void onMoveToOtherNotebookAction();
    void onOpenNoteInSeparateWindowAction();

    void onUnfavoriteAction();
    void onFavoriteAction();

    void onShowNoteInfoAction();
    void onCopyInAppNoteLinkAction();

    /**
     * Toggle thumbnail preference on all notes (when passed data is empty)
     * or one given note (passed in data).
     */
    void onToggleThumbnailPreference();

    void onExportSingleNoteToEnexAction();
    void onExportSeveralNotesToEnexAction();

    void onSelectFirstNoteEvent();
    void onTrySetLastCurrentNoteByLocalUidEvent();

    virtual void contextMenuEvent(QContextMenuEvent * pEvent) Q_DECL_OVERRIDE;

protected:
    virtual void currentChanged(const QModelIndex & current,
                                const QModelIndex & previous) Q_DECL_OVERRIDE;
    virtual void mousePressEvent(QMouseEvent * pEvent) Q_DECL_OVERRIDE;

    const NotebookItem * currentNotebookItem();

protected:
    void showContextMenuAtPoint(const QPoint & pos, const QPoint & globalPos);
    void showSingleNoteContextMenu(const QPoint & pos, const QPoint & globalPos,
                                   const NoteFilterModel & noteFilterModel, const NoteModel & noteModel);
    void showMultipleNotesContextMenu(const QPoint & globalPos, const QStringList & noteLocalUids);

private:
    /**
     * Return note filter model for given model.
     */
    NoteFilterModel * noteFilterModel(QAbstractItemModel * model) const;

    /**
     * Return note filter model for current model.
     */
    NoteFilterModel * noteFilterModel() const;

    /**
     * Return note model for given filter model.
     */
    NoteModel * noteModel(NoteFilterModel * pNoteFilterModel) const;

    /**
     * Convenience method called in slots invoked by QAction's signals.
     * @return variant data from QAction sender's data (invalid variant in case of error).
     */
    QVariant actionData();

    /**
     * Convenience method called in slots invoked by QAction's signals.
     * @return string data from QAction sender's data (empty string in case of error).
     */
    QString actionDataString();

    /**
     * Convenience method called in slots invoked by QAction's signals.
     * @return string list data from QAction sender's data (empty list in case of error).
     */
    QStringList actionDataStringList();

protected:
    QMenu *             m_pNoteItemContextMenu;
    NotebookItemView *  m_pNotebookItemView;
    bool                m_shouldSelectFirstNoteOnNextNoteAddition;

    Account             m_currentAccount;

    QString             m_lastCurrentNoteLocalUid;

    /**
     * Current value of "shown thumbnails for all notes".
     */
    bool                m_showThumbnailsForAllNotes;
    /**
     * Set with local uids of notes where thumbnail was manually hidden.
     */
    QSet<QString>       m_hideThumbnailsLocalUids;
};

} // namespace quentier

#endif // QUENTIER_VIEWS_NOTE_LIST_VIEW_H
