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

#ifndef QUENTIER_LIB_MODEL_TESTS_FAVORITES_MODEL_TEST_HELPER_H
#define QUENTIER_LIB_MODEL_TESTS_FAVORITES_MODEL_TEST_HELPER_H

#include <quentier/local_storage/LocalStorageManagerAsync.h>

namespace quentier {

class FavoritesModel;
class FavoritesModelItem;

class FavoritesModelTestHelper final: public QObject
{
    Q_OBJECT
public:
    explicit FavoritesModelTestHelper(
        LocalStorageManagerAsync * pLocalStorageManagerAsync,
        QObject * parent = nullptr);

    ~FavoritesModelTestHelper() override;

Q_SIGNALS:
    void failure(ErrorString errorDescription);
    void success();

public Q_SLOTS:
    void test();

private Q_SLOTS:
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

    void onUpdateNotebookComplete(
        qevercloud::Notebook notebook, QUuid requestId);

    void onUpdateNotebookFailed(
        qevercloud::Notebook notebook, ErrorString errorDescription,
        QUuid requestId);

    void onFindNotebookFailed(
        qevercloud::Notebook notebook, ErrorString errorDescription,
        QUuid requestId);

    void onListNotebooksFailed(
        LocalStorageManager::ListObjectsOptions flag, size_t limit,
        size_t offset, LocalStorageManager::ListNotebooksOrder order,
        LocalStorageManager::OrderDirection orderDirection,
        QString linkedNotebookGuid, ErrorString errorDescription,
        QUuid requestId);

    void onUpdateTagComplete(qevercloud::Tag tag, QUuid requestId);

    void onUpdateTagFailed(
        qevercloud::Tag tag, ErrorString errorDescription, QUuid requestId);

    void onFindTagFailed(
        qevercloud::Tag tag, ErrorString errorDescription, QUuid requestId);

    void onListTagsFailed(
        LocalStorageManager::ListObjectsOptions flag, size_t limit,
        size_t offset, LocalStorageManager::ListTagsOrder order,
        LocalStorageManager::OrderDirection orderDirection,
        QString linkedNotebookGuid, ErrorString errorDescription,
        QUuid requestId);

    void onUpdateSavedSearchComplete(
        qevercloud::SavedSearch search, QUuid requestId);

    void onUpdateSavedSearchFailed(
        qevercloud::SavedSearch search, ErrorString errorDescription,
        QUuid requestId);

    void onFindSavedSearchFailed(
        qevercloud::SavedSearch search, ErrorString errorDescription,
        QUuid requestId);

    void onListSavedSearchesFailed(
        LocalStorageManager::ListObjectsOptions flag, size_t limit,
        size_t offset, LocalStorageManager::ListSavedSearchesOrder order,
        LocalStorageManager::OrderDirection orderDirection,
        ErrorString errorDescription, QUuid requestId);

private:
    void checkSorting(const FavoritesModel & model);
    void notifyFailureWithStackTrace(ErrorString errorDescription);

private:
    struct LessByType
    {
        [[nodiscard]] bool operator()(
            const FavoritesModelItem & lhs,
            const FavoritesModelItem & rhs) const noexcept;
    };

    struct GreaterByType
    {
        [[nodiscard]] bool operator()(
            const FavoritesModelItem & lhs,
            const FavoritesModelItem & rhs) const noexcept;
    };

    struct LessByDisplayName
    {
        [[nodiscard]] bool operator()(
            const FavoritesModelItem & lhs,
            const FavoritesModelItem & rhs) const noexcept;
    };

    struct GreaterByDisplayName
    {
        [[nodiscard]] bool operator()(
            const FavoritesModelItem & lhs,
            const FavoritesModelItem & rhs) const noexcept;
    };

    struct LessByNoteCount
    {
        [[nodiscard]] bool operator()(
            const FavoritesModelItem & lhs,
            const FavoritesModelItem & rhs) const noexcept;
    };

    struct GreaterByNoteCount
    {
        [[nodiscard]] bool operator()(
            const FavoritesModelItem & lhs,
            const FavoritesModelItem & rhs) const noexcept;
    };

private:
    LocalStorageManagerAsync * m_pLocalStorageManagerAsync;
    FavoritesModel * m_model = nullptr;

    qevercloud::Notebook m_firstNotebook;
    qevercloud::Notebook m_secondNotebook;
    qevercloud::Notebook m_thirdNotebook;

    qevercloud::Note m_firstNote;
    qevercloud::Note m_secondNote;
    qevercloud::Note m_thirdNote;
    qevercloud::Note m_fourthNote;
    qevercloud::Note m_fifthNote;
    qevercloud::Note m_sixthNote;

    qevercloud::Tag m_firstTag;
    qevercloud::Tag m_secondTag;
    qevercloud::Tag m_thirdTag;
    qevercloud::Tag m_fourthTag;

    qevercloud::SavedSearch m_firstSavedSearch;
    qevercloud::SavedSearch m_secondSavedSearch;
    qevercloud::SavedSearch m_thirdSavedSearch;
    qevercloud::SavedSearch m_fourthSavedSearch;

    bool m_expectingNoteUpdateFromLocalStorage = false;
    bool m_expectingNotebookUpdateFromLocalStorage = false;
    bool m_expectingTagUpdateFromLocalStorage = false;
    bool m_expectingSavedSearchUpdateFromLocalStorage = false;

    bool m_expectingNoteUnfavoriteFromLocalStorage = false;
    bool m_expectingNotebookUnfavoriteFromLocalStorage = false;
    bool m_expectingTagUnfavoriteFromLocalStorage = false;
    bool m_expectingSavedSearchUnfavoriteFromLocalStorage = false;
};

} // namespace quentier

#endif // QUENTIER_LIB_MODEL_TESTS_FAVORITES_MODEL_TEST_HELPER_H
