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

#ifndef QUENTIER_LIB_MODEL_TESTS_FAVORITES_MODEL_TEST_HELPER_H
#define QUENTIER_LIB_MODEL_TESTS_FAVORITES_MODEL_TEST_HELPER_H

#include <quentier/local_storage/LocalStorageManagerAsync.h>

namespace quentier {

QT_FORWARD_DECLARE_CLASS(FavoritesModel)
QT_FORWARD_DECLARE_CLASS(FavoritesModelItem)

class FavoritesModelTestHelper : public QObject
{
    Q_OBJECT
public:
    explicit FavoritesModelTestHelper(
        LocalStorageManagerAsync * pLocalStorageManagerAsync,
        QObject * parent = nullptr);

    virtual ~FavoritesModelTestHelper() override;

Q_SIGNALS:
    void failure(ErrorString errorDescription);
    void success();

public Q_SLOTS:
    void test();

private Q_SLOTS:
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
        LocalStorageManager::GetNoteOptions options, size_t limit,
        size_t offset, LocalStorageManager::ListNotesOrder order,
        LocalStorageManager::OrderDirection orderDirection,
        QString linkedNotebookGuid, ErrorString errorDescription,
        QUuid requestId);

    void onUpdateNotebookComplete(Notebook notebook, QUuid requestId);

    void onUpdateNotebookFailed(
        Notebook notebook, ErrorString errorDescription, QUuid requestId);

    void onFindNotebookFailed(
        Notebook notebook, ErrorString errorDescription, QUuid requestId);

    void onListNotebooksFailed(
        LocalStorageManager::ListObjectsOptions flag, size_t limit,
        size_t offset, LocalStorageManager::ListNotebooksOrder order,
        LocalStorageManager::OrderDirection orderDirection,
        QString linkedNotebookGuid, ErrorString errorDescription,
        QUuid requestId);

    void onUpdateTagComplete(Tag tag, QUuid requestId);

    void onUpdateTagFailed(
        Tag tag, ErrorString errorDescription, QUuid requestId);

    void onFindTagFailed(
        Tag tag, ErrorString errorDescription, QUuid requestId);

    void onListTagsFailed(
        LocalStorageManager::ListObjectsOptions flag, size_t limit,
        size_t offset, LocalStorageManager::ListTagsOrder order,
        LocalStorageManager::OrderDirection orderDirection,
        QString linkedNotebookGuid, ErrorString errorDescription,
        QUuid requestId);

    void onUpdateSavedSearchComplete(SavedSearch search, QUuid requestId);

    void onUpdateSavedSearchFailed(
        SavedSearch search, ErrorString errorDescription, QUuid requestId);

    void onFindSavedSearchFailed(
        SavedSearch search, ErrorString errorDescription, QUuid requestId);

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
        bool operator()(
            const FavoritesModelItem & lhs,
            const FavoritesModelItem & rhs) const;
    };

    struct GreaterByType
    {
        bool operator()(
            const FavoritesModelItem & lhs,
            const FavoritesModelItem & rhs) const;
    };

    struct LessByDisplayName
    {
        bool operator()(
            const FavoritesModelItem & lhs,
            const FavoritesModelItem & rhs) const;
    };

    struct GreaterByDisplayName
    {
        bool operator()(
            const FavoritesModelItem & lhs,
            const FavoritesModelItem & rhs) const;
    };

    struct LessByNoteCount
    {
        bool operator()(
            const FavoritesModelItem & lhs,
            const FavoritesModelItem & rhs) const;
    };

    struct GreaterByNoteCount
    {
        bool operator()(
            const FavoritesModelItem & lhs,
            const FavoritesModelItem & rhs) const;
    };

private:
    LocalStorageManagerAsync * m_pLocalStorageManagerAsync;
    FavoritesModel * m_model = nullptr;

    Notebook m_firstNotebook;
    Notebook m_secondNotebook;
    Notebook m_thirdNotebook;

    Note m_firstNote;
    Note m_secondNote;
    Note m_thirdNote;
    Note m_fourthNote;
    Note m_fifthNote;
    Note m_sixthNote;

    Tag m_firstTag;
    Tag m_secondTag;
    Tag m_thirdTag;
    Tag m_fourthTag;

    SavedSearch m_firstSavedSearch;
    SavedSearch m_secondSavedSearch;
    SavedSearch m_thirdSavedSearch;
    SavedSearch m_fourthSavedSearch;

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
