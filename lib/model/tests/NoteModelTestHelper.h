/*
 * Copyright 2016-2021 Dmitry Ivanov
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

class NoteModel;
class NoteModelItem;

class NoteModelTestHelper final: public QObject
{
    Q_OBJECT
public:
    explicit NoteModelTestHelper(
        LocalStorageManagerAsync * pLocalStorageManagerAsync,
        QObject * parent = nullptr);

    ~NoteModelTestHelper() override;

Q_SIGNALS:
    void failure(ErrorString errorDescription);
    void success();

public Q_SLOTS:
    void test();

private Q_SLOTS:
    void onAddNoteComplete(qevercloud::Note note, QUuid requestId);

    void onAddNoteFailed(
        qevercloud::Note note, ErrorString errorDescription, QUuid requestId);

    void onUpdateNoteComplete(
        qevercloud::Note note, LocalStorageManager::UpdateNoteOptions options,
        QUuid requestId);

    void onUpdateNoteFailed(
        qevercloud::Note note, LocalStorageManager::UpdateNoteOptions options,
        ErrorString errorDescription, QUuid requestId);

    void onFindNoteFailed(
        qevercloud::Note note, LocalStorageManager::GetNoteOptions options,
        ErrorString errorDescription, QUuid requestId);

    void onListNotesFailed(
        LocalStorageManager::ListObjectsOptions flag,
        LocalStorageManager::GetNoteOptions options, size_t limit,
        size_t offset, LocalStorageManager::ListNotesOrder order,
        LocalStorageManager::OrderDirection orderDirection,
        QString linkedNotebookGuid, ErrorString errorDescription,
        QUuid requestId);

    void onExpungeNoteComplete(qevercloud::Note note, QUuid requestId);

    void onExpungeNoteFailed(
        qevercloud::Note note, ErrorString errorDescription, QUuid requestId);

    void onAddNotebookFailed(
        qevercloud::Notebook notebook, ErrorString errorDescription,
        QUuid requestId);

    void onUpdateNotebookFailed(
        qevercloud::Notebook notebook, ErrorString errorDescription,
        QUuid requestId);

    void onAddTagFailed(
        qevercloud::Tag tag, ErrorString errorDescription, QUuid requestId);

private:
    void checkSorting(const NoteModel & model);
    void notifyFailureWithStackTrace(ErrorString errorDescription);

private:
    struct LessByCreationTimestamp
    {
        [[nodiscard]] bool operator()(
            const NoteModelItem & lhs,
            const NoteModelItem & rhs) const noexcept;
    };

    struct GreaterByCreationTimestamp
    {
        [[nodiscard]] bool operator()(
            const NoteModelItem & lhs,
            const NoteModelItem & rhs) const noexcept;
    };

    struct LessByModificationTimestamp
    {
        [[nodiscard]] bool operator()(
            const NoteModelItem & lhs,
            const NoteModelItem & rhs) const noexcept;
    };

    struct GreaterByModificationTimestamp
    {
        [[nodiscard]] bool operator()(
            const NoteModelItem & lhs,
            const NoteModelItem & rhs) const noexcept;
    };

    struct LessByDeletionTimestamp
    {
        [[nodiscard]] bool operator()(
            const NoteModelItem & lhs,
            const NoteModelItem & rhs) const noexcept;
    };

    struct GreaterByDeletionTimestamp
    {
        [[nodiscard]] bool operator()(
            const NoteModelItem & lhs,
            const NoteModelItem & rhs) const noexcept;
    };

    struct LessByTitle
    {
        [[nodiscard]] bool operator()(
            const NoteModelItem & lhs,
            const NoteModelItem & rhs) const noexcept;
    };

    struct GreaterByTitle
    {
        [[nodiscard]] bool operator()(
            const NoteModelItem & lhs,
            const NoteModelItem & rhs) const noexcept;
    };

    struct LessByPreviewText
    {
        [[nodiscard]] bool operator()(
            const NoteModelItem & lhs,
            const NoteModelItem & rhs) const noexcept;
    };

    struct GreaterByPreviewText
    {
        [[nodiscard]] bool operator()(
            const NoteModelItem & lhs,
            const NoteModelItem & rhs) const noexcept;
    };

    struct LessByNotebookName
    {
        [[nodiscard]] bool operator()(
            const NoteModelItem & lhs,
            const NoteModelItem & rhs) const noexcept;
    };

    struct GreaterByNotebookName
    {
        [[nodiscard]] bool operator()(
            const NoteModelItem & lhs,
            const NoteModelItem & rhs) const noexcept;
    };

    struct LessBySize
    {
        [[nodiscard]] bool operator()(
            const NoteModelItem & lhs,
            const NoteModelItem & rhs) const noexcept;
    };

    struct GreaterBySize
    {
        [[nodiscard]] bool operator()(
            const NoteModelItem & lhs,
            const NoteModelItem & rhs) const noexcept;
    };

    struct LessBySynchronizable
    {
        [[nodiscard]] bool operator()(
            const NoteModelItem & lhs,
            const NoteModelItem & rhs) const noexcept;
    };

    struct GreaterBySynchronizable
    {
        [[nodiscard]] bool operator()(
            const NoteModelItem & lhs,
            const NoteModelItem & rhs) const noexcept;
    };

    struct LessByDirty
    {
        [[nodiscard]] bool operator()(
            const NoteModelItem & lhs,
            const NoteModelItem & rhs) const noexcept;
    };

    struct GreaterByDirty
    {
        [[nodiscard]] bool operator()(
            const NoteModelItem & lhs,
            const NoteModelItem & rhs) const noexcept;
    };

private:
    LocalStorageManagerAsync * m_pLocalStorageManagerAsync;

    NoteModel * m_model = nullptr;
    qevercloud::Notebook m_firstNotebook;
    QString m_noteToExpungeLocalId;

    bool m_expectingNewNoteFromLocalStorage = false;
    bool m_expectingNoteUpdateFromLocalStorage = false;
    bool m_expectingNoteDeletionFromLocalStorage = false;
    bool m_expectingNoteExpungeFromLocalStorage = false;
};

} // namespace quentier

#endif // QUENTIER_LIB_MODEL_TESTS_NOTE_MODEL_TEST_HELPER_H
