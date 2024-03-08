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

#include <quentier/types/Account.h>
#include <quentier/types/ErrorString.h>

#include <QListView>

class QMenu;

namespace quentier {

class NotebookItem;
class NotebookItemView;
class NoteModel;

/**
 * @brief The NoteListView is a simple subclass of QListView which adds some
 * bits of functionality specific to note list on top of it
 */
class NoteListView final : public QListView
{
    Q_OBJECT
public:
    explicit NoteListView(QWidget * parent = nullptr);

    void setNotebookItemView(NotebookItemView * notebookItemView);

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
     * @return local ids of selected notes
     */
    [[nodiscard]] QStringList selectedNotesLocalIds() const;

    /**
     * @return the local id of the current note
     */
    [[nodiscard]] QString currentNoteLocalId() const;

    /**
     * @return the current account
     */
    [[nodiscard]] const Account & currentAccount() const noexcept;

    void setCurrentAccount(Account account);

Q_SIGNALS:
    void notifyError(ErrorString errorDescription);
    void currentNoteChanged(QString noteLocalId);

    void newNoteCreationRequested();
    void editNoteDialogRequested(QString noteLocalId);
    void noteInfoDialogRequested(QString noteLocalId);
    void openNoteInSeparateWindowRequested(QString noteLocalId);
    void copyInAppNoteLinkRequested(QString noteLocalId, QString noteGuid);

    void enexExportRequested(QStringList noteLocalIds);

    void toggleThumbnailsPreference(QString noteLocalId);

public Q_SLOTS:
    /**
     * The slot which can watch for external changes of current note
     * and reflect the change of the current item in the view
     */
    void setCurrentNoteByLocalId(QString noteLocalId);

    /**
     * The slot which can watch for external changes in thumbnail display state.
     * @param showThumbnailsForAllNotes     Global flag for all notes.
     * @param hideThumbnailsLocalIds        Map with local ids where
     *                                      the thumbails was manually hidden.
     */
    void setShowNoteThumbnailsState(
        bool showThumbnailsForAllNotes, QSet<QString> hideThumbnailsLocalIds);

    /**
     * The slot which can watch for external changes of selected notes
     * and reflect that change in the view
     */
    void selectNotesByLocalIds(const QStringList & noteLocalIds);

    /**
     * @brief The dataChanged method is redefined in NoteListView for the sole
     * reason of being a public slot instead of protected; it calls
     * the implementation of QListView's dataChanged protected slot
     */
    void dataChanged(
        const QModelIndex & topLeft, const QModelIndex & bottomRight,
        const QVector<int> & roles = QVector<int>()) override;

    void rowsAboutToBeRemoved(
        const QModelIndex & parent, int start, int end) override;

    void rowsInserted(const QModelIndex & parent, int start, int end) override;

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
    void onTrySetLastCurrentNoteByLocalIdEvent();

    void contextMenuEvent(QContextMenuEvent * event) override;

protected:
    void currentChanged(
        const QModelIndex & current, const QModelIndex & previous) override;

    void mousePressEvent(QMouseEvent * pEvent) override;

    [[nodiscard]] const NotebookItem * currentNotebookItem();

protected:
    void showContextMenuAtPoint(const QPoint & pos, const QPoint & globalPos);

    void showSingleNoteContextMenu(
        const QPoint & pos, const QPoint & globalPos,
        const NoteModel & noteModel);

    void showMultipleNotesContextMenu(
        const QPoint & globalPos, const QStringList & noteLocalIds);

private:
    /**
     * @return current model as note filter model.
     */
    [[nodiscard]] NoteModel * noteModel() const;

    /**
     * Convenience method called in slots invoked by QAction's signals.
     * @return      Variant data from QAction sender's data (invalid variant in
     *              case of error).
     */
    [[nodiscard]] QVariant actionData();

    /**
     * Convenience method called in slots invoked by QAction's signals.
     * @return      String data from QAction sender's data (empty string in case
     *              of error).
     */
    [[nodiscard]] QString actionDataString();

    /**
     * Convenience method called in slots invoked by QAction's signals.
     * @return      String list data from QAction sender's data (empty list in
     *              case of error).
     */
    [[nodiscard]] QStringList actionDataStringList();

protected:
    QMenu * m_noteItemContextMenu = nullptr;
    NotebookItemView * m_notebookItemView = nullptr;
    bool m_shouldSelectFirstNoteOnNextNoteAddition = false;

    Account m_currentAccount;

    QString m_lastCurrentNoteLocalId;

    /**
     * Current value of "show thumbnails for all notes".
     */
    bool m_showThumbnailsForAllNotes = true;

    /**
     * Set with local ids of notes where thumbnail was manually hidden.
     */
    QSet<QString> m_hideThumbnailsLocalIds;
};

} // namespace quentier
