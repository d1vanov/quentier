/*
 * Copyright 2016 Dmitry Ivanov
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

#ifndef QUENTIER_TESTS_MODEL_TEST_FAVORITES_MODEL_TEST_HELPER_H
#define QUENTIER_TESTS_MODEL_TEST_FAVORITES_MODEL_TEST_HELPER_H

#include <quentier/local_storage/LocalStorageManagerAsync.h>

namespace quentier {

QT_FORWARD_DECLARE_CLASS(FavoritesModel)
QT_FORWARD_DECLARE_CLASS(FavoritesModelItem)

class FavoritesModelTestHelper: public QObject
{
    Q_OBJECT
public:
    explicit FavoritesModelTestHelper(LocalStorageManagerAsync * pLocalStorageManagerAsync,
                                      QObject * parent = Q_NULLPTR);

Q_SIGNALS:
    void failure(ErrorString errorDescription);
    void success();

public Q_SLOTS:
    void launchTest();

private Q_SLOTS:
    void onUpdateNoteComplete(Note note, bool updateResources, bool updateTags, QUuid requestId);
    void onUpdateNoteFailed(Note note, bool updateResources, bool updateTags,
                            ErrorString errorDescription, QUuid requestId);
    void onFindNoteFailed(Note note, bool withResourceBinaryData, ErrorString errorDescription, QUuid requestId);
    void onListNotesFailed(LocalStorageManager::ListObjectsOptions flag, bool withResourceBinaryData,
                           size_t limit, size_t offset, LocalStorageManager::ListNotesOrder::type order,
                           LocalStorageManager::OrderDirection::type orderDirection,
                           QString linkedNotebookGuid, ErrorString errorDescription, QUuid requestId);

    void onUpdateNotebookComplete(Notebook notebook, QUuid requestId);
    void onUpdateNotebookFailed(Notebook notebook, ErrorString errorDescription, QUuid requestId);
    void onFindNotebookFailed(Notebook notebook, ErrorString errorDescription, QUuid requestId);
    void onListNotebooksFailed(LocalStorageManager::ListObjectsOptions flag, size_t limit, size_t offset,
                               LocalStorageManager::ListNotebooksOrder::type order,
                               LocalStorageManager::OrderDirection::type orderDirection,
                               QString linkedNotebookGuid, ErrorString errorDescription, QUuid requestId);

    void onUpdateTagComplete(Tag tag, QUuid requestId);
    void onUpdateTagFailed(Tag tag, ErrorString errorDescription, QUuid requestId);
    void onFindTagFailed(Tag tag, ErrorString errorDescription, QUuid requestId);
    void onListTagsFailed(LocalStorageManager::ListObjectsOptions flag,
                          size_t limit, size_t offset,
                          LocalStorageManager::ListTagsOrder::type order,
                          LocalStorageManager::OrderDirection::type orderDirection,
                          QString linkedNotebookGuid,
                          ErrorString errorDescription, QUuid requestId);

    void onUpdateSavedSearchComplete(SavedSearch search, QUuid requestId);
    void onUpdateSavedSearchFailed(SavedSearch search, ErrorString errorDescription, QUuid requestId);
    void onFindSavedSearchFailed(SavedSearch search, ErrorString errorDescription, QUuid requestId);
    void onListSavedSearchesFailed(LocalStorageManager::ListObjectsOptions flag,
                                   size_t limit, size_t offset,
                                   LocalStorageManager::ListSavedSearchesOrder::type order,
                                   LocalStorageManager::OrderDirection::type orderDirection,
                                   ErrorString errorDescription, QUuid requestId);

private:
    void checkSorting(const FavoritesModel & model);
    void notifyFailureWithStackTrace(ErrorString errorDescription);

private:
    struct LessByType
    {
        bool operator()(const FavoritesModelItem & lhs, const FavoritesModelItem & rhs) const;
    };

    struct GreaterByType
    {
        bool operator()(const FavoritesModelItem & lhs, const FavoritesModelItem & rhs) const;
    };

    struct LessByDisplayName
    {
        bool operator()(const FavoritesModelItem & lhs, const FavoritesModelItem & rhs) const;
    };

    struct GreaterByDisplayName
    {
        bool operator()(const FavoritesModelItem & lhs, const FavoritesModelItem & rhs) const;
    };

    struct LessByNumNotesTargeted
    {
        bool operator()(const FavoritesModelItem & lhs, const FavoritesModelItem & rhs) const;
    };

    struct GreaterByNumNotesTargeted
    {
        bool operator()(const FavoritesModelItem & lhs, const FavoritesModelItem & rhs) const;
    };

private:
    LocalStorageManagerAsync *          m_pLocalStorageManagerAsync;
    FavoritesModel *                    m_model;

    Notebook                            m_firstNotebook;
    Notebook                            m_secondNotebook;
    Notebook                            m_thirdNotebook;

    Note                                m_firstNote;
    Note                                m_secondNote;
    Note                                m_thirdNote;
    Note                                m_fourthNote;
    Note                                m_fifthNote;
    Note                                m_sixthNote;

    Tag                                 m_firstTag;
    Tag                                 m_secondTag;
    Tag                                 m_thirdTag;
    Tag                                 m_fourthTag;

    SavedSearch                         m_firstSavedSearch;
    SavedSearch                         m_secondSavedSearch;
    SavedSearch                         m_thirdSavedSearch;
    SavedSearch                         m_fourthSavedSearch;

    bool                                m_expectingNoteUpdateFromLocalStorage;
    bool                                m_expectingNotebookUpdateFromLocalStorage;
    bool                                m_expectingTagUpdateFromLocalStorage;
    bool                                m_expectingSavedSearchUpdateFromLocalStorage;

    bool                                m_expectingNoteUnfavoriteFromLocalStorage;
    bool                                m_expectingNotebookUnfavoriteFromLocalStorage;
    bool                                m_expectingTagUnfavoriteFromLocalStorage;
    bool                                m_expectingSavedSearchUnfavoriteFromLocalStorage;
};

} // namespace quentier

#endif // QUENTIER_TESTS_MODEL_TEST_FAVORITES_MODEL_TEST_HELPER_H
