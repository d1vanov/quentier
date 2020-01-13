/*
 * Copyright 2016-2020 Dmitry Ivanov
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

#ifndef QUENTIER_LIB_MODEL_TESTS_NOTE_MODEL_TEST_HELPER_H
#define QUENTIER_LIB_MODEL_TESTS_NOTE_MODEL_TEST_HELPER_H

#include <quentier/local_storage/LocalStorageManagerAsync.h>

namespace quentier {

QT_FORWARD_DECLARE_CLASS(NoteModel)
QT_FORWARD_DECLARE_CLASS(NoteModelItem)

class NoteModelTestHelper: public QObject
{
    Q_OBJECT
public:
    explicit NoteModelTestHelper(
        LocalStorageManagerAsync * pLocalStorageManagerAsync,
        QObject * parent = nullptr);

Q_SIGNALS:
    void failure(ErrorString errorDescription);
    void success();

public Q_SLOTS:
    void launchTest();

private Q_SLOTS:
    void onAddNoteComplete(Note note, QUuid requestId);

    void onAddNoteFailed(
        Note note, ErrorString errorDescription, QUuid requestId);

    void onUpdateNoteComplete(
        Note note, LocalStorageManager::UpdateNoteOptions options,
        QUuid requestId);

    void onUpdateNoteFailed(
        Note note, LocalStorageManager::UpdateNoteOptions options,
        ErrorString errorDescription, QUuid requestId);

    void onFindNoteFailed(
        Note note, LocalStorageManager::GetNoteOptions options,
        ErrorString errorDescription, QUuid requestId);

    void onListNotesFailed(
        LocalStorageManager::ListObjectsOptions flag,
        LocalStorageManager::GetNoteOptions options,
        size_t limit, size_t offset,
        LocalStorageManager::ListNotesOrder order,
        LocalStorageManager::OrderDirection orderDirection,
        QString linkedNotebookGuid, ErrorString errorDescription,
        QUuid requestId);

    void onExpungeNoteComplete(Note note, QUuid requestId);

    void onExpungeNoteFailed(
        Note note, ErrorString errorDescription, QUuid requestId);

    void onAddNotebookFailed(
        Notebook notebook, ErrorString errorDescription, QUuid requestId);

    void onUpdateNotebookFailed(
        Notebook notebook, ErrorString errorDescription, QUuid requestId);

    void onAddTagFailed(Tag tag, ErrorString errorDescription, QUuid requestId);

private:
    void checkSorting(const NoteModel & model);
    void notifyFailureWithStackTrace(ErrorString errorDescription);

private:
    struct LessByCreationTimestamp
    {
        bool operator()(
            const NoteModelItem & lhs, const NoteModelItem & rhs) const;
    };

    struct GreaterByCreationTimestamp
    {
        bool operator()(
            const NoteModelItem & lhs, const NoteModelItem & rhs) const;
    };

    struct LessByModificationTimestamp
    {
        bool operator()(
            const NoteModelItem & lhs, const NoteModelItem & rhs) const;
    };

    struct GreaterByModificationTimestamp
    {
        bool operator()(
            const NoteModelItem & lhs, const NoteModelItem & rhs) const;
    };

    struct LessByDeletionTimestamp
    {
        bool operator()(
            const NoteModelItem & lhs, const NoteModelItem & rhs) const;
    };

    struct GreaterByDeletionTimestamp
    {
        bool operator()(
            const NoteModelItem & lhs, const NoteModelItem & rhs) const;
    };

    struct LessByTitle
    {
        bool operator()(
            const NoteModelItem & lhs, const NoteModelItem & rhs) const;
    };

    struct GreaterByTitle
    {
        bool operator()(
            const NoteModelItem & lhs, const NoteModelItem & rhs) const;
    };

    struct LessByPreviewText
    {
        bool operator()(
            const NoteModelItem & lhs, const NoteModelItem & rhs) const;
    };

    struct GreaterByPreviewText
    {
        bool operator()(
            const NoteModelItem & lhs, const NoteModelItem & rhs) const;
    };

    struct LessByNotebookName
    {
        bool operator()(
            const NoteModelItem & lhs, const NoteModelItem & rhs) const;
    };

    struct GreaterByNotebookName
    {
        bool operator()(
            const NoteModelItem & lhs, const NoteModelItem & rhs) const;
    };

    struct LessBySize
    {
        bool operator()(
            const NoteModelItem & lhs, const NoteModelItem & rhs) const;
    };

    struct GreaterBySize
    {
        bool operator()(
            const NoteModelItem & lhs, const NoteModelItem & rhs) const;
    };

    struct LessBySynchronizable
    {
        bool operator()(
            const NoteModelItem & lhs, const NoteModelItem & rhs) const;
    };

    struct GreaterBySynchronizable
    {
        bool operator()(
            const NoteModelItem & lhs, const NoteModelItem & rhs) const;
    };

    struct LessByDirty
    {
        bool operator()(
            const NoteModelItem & lhs, const NoteModelItem & rhs) const;
    };

    struct GreaterByDirty
    {
        bool operator()(
            const NoteModelItem & lhs, const NoteModelItem & rhs) const;
    };

private:
    LocalStorageManagerAsync *  m_pLocalStorageManagerAsync;
    NoteModel *     m_model;
    Notebook        m_firstNotebook;
    QString         m_noteToExpungeLocalUid;
    bool            m_expectingNewNoteFromLocalStorage;
    bool            m_expectingNoteUpdateFromLocalStorage;
    bool            m_expectingNoteDeletionFromLocalStorage;
    bool            m_expectingNoteExpungeFromLocalStorage;
};

} // namespace quentier

#endif // QUENTIER_LIB_MODEL_TESTS_NOTE_MODEL_TEST_HELPER_H
