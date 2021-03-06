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

#ifndef QUENTIER_LIB_VIEW_NOTE_LIST_VIEW_H
#define QUENTIER_LIB_VIEW_NOTE_LIST_VIEW_H

#include <quentier/types/Account.h>
#include <quentier/types/ErrorString.h>

#include <QListView>

QT_FORWARD_DECLARE_CLASS(QMenu)

namespace quentier {

QT_FORWARD_DECLARE_CLASS(NotebookItem)
QT_FORWARD_DECLARE_CLASS(NotebookItemView)
QT_FORWARD_DECLARE_CLASS(NoteModel)

/**
 * @brief The NoteListView is a simple subclass of QListView which adds some
 * bits of functionality specific to note list on top of it
 */
class NoteListView final : public QListView
{
    Q_OBJECT
public:
    explicit NoteListView(QWidget * parent = nullptr);

    void setNotebookItemView(NotebookItemView * pNotebookItemView);

    /**
     * After this method is called, NoteListView would automatically select
     * the first note of the model after the next event of note item addition.
     * It would happen only once.
     *
     * This method is handy to run before starting the synchronization of
     * a fresh Evernote account
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
     * @param showThumbnailsForAllNotes     Global flag for all notes.
     * @param hideThumbnailsLocalUids       Map with local uids where
     *                                      the thumbails was manually hidden.
     */
    void setShowNoteThumbnailsState(
        bool showThumbnailsForAllNotes,
        const QSet<QString> & hideThumbnailsLocalUids);

    /**
     * The slot which can watch for external changes of selected notes
     * and reflect that change in the view
     */
    void selectNotesByLocalUids(const QStringList & noteLocalUids);

    /**
     * @brief The dataChanged method is redefined in NoteListView for the sole
     * reason of being a public slot instead of protected; it calls
     * the implementation of QListView's dataChanged protected slot
     */
    virtual void dataChanged(
        const QModelIndex & topLeft, const QModelIndex & bottomRight,
        const QVector<int> & roles = QVector<int>()) override;

    virtual void rowsAboutToBeRemoved(
        const QModelIndex & parent, int start, int end) override;

    virtual void rowsInserted(
        const QModelIndex & parent, int start, int end) override;

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

    virtual void contextMenuEvent(QContextMenuEvent * pEvent) override;

protected:
    virtual void currentChanged(
        const QModelIndex & current, const QModelIndex & previous) override;

    virtual void mousePressEvent(QMouseEvent * pEvent) override;

    const NotebookItem * currentNotebookItem();

protected:
    void showContextMenuAtPoint(const QPoint & pos, const QPoint & globalPos);

    void showSingleNoteContextMenu(
        const QPoint & pos, const QPoint & globalPos,
        const NoteModel & noteModel);

    void showMultipleNotesContextMenu(
        const QPoint & globalPos, const QStringList & noteLocalUids);

private:
    /**
     * @return current model as note filter model.
     */
    NoteModel * noteModel() const;

    /**
     * Convenience method called in slots invoked by QAction's signals.
     * @return      Variant data from QAction sender's data (invalid variant in
     *              case of error).
     */
    QVariant actionData();

    /**
     * Convenience method called in slots invoked by QAction's signals.
     * @return      String data from QAction sender's data (empty string in case
     *              of error).
     */
    QString actionDataString();

    /**
     * Convenience method called in slots invoked by QAction's signals.
     * @return      String list data from QAction sender's data (empty list in
     *              case of error).
     */
    QStringList actionDataStringList();

protected:
    QMenu * m_pNoteItemContextMenu = nullptr;
    NotebookItemView * m_pNotebookItemView = nullptr;
    bool m_shouldSelectFirstNoteOnNextNoteAddition = false;

    Account m_currentAccount;

    QString m_lastCurrentNoteLocalUid;

    /**
     * Current value of "show thumbnails for all notes".
     */
    bool m_showThumbnailsForAllNotes = true;

    /**
     * Set with local uids of notes where thumbnail was manually hidden.
     */
    QSet<QString> m_hideThumbnailsLocalUids;
};

} // namespace quentier

#endif // QUENTIER_LIB_VIEW_NOTE_LIST_VIEW_H
