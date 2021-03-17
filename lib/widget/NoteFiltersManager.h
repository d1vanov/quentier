/*
 * Copyright 2017-2021 Dmitry Ivanov
 *
 * This file is part of Quentier
 *
 * Quentier is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * Quentier is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with Quentier. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef QUENTIER_LIB_WIDGET_NOTE_FILTERS_MANAGER_H
#define QUENTIER_LIB_WIDGET_NOTE_FILTERS_MANAGER_H

#include <quentier/local_storage/LocalStorageManagerAsync.h>
#include <quentier/local_storage/NoteSearchQuery.h>

#include <QObject>
#include <QPointer>
#include <QUuid>

class QLineEdit;

namespace quentier {

class FilterByNotebookWidget;
class FilterBySavedSearchWidget;
class FilterBySearchStringWidget;
class FilterByTagWidget;
class NoteModel;
class TagModel;

class NoteFiltersManager final : public QObject
{
    Q_OBJECT
public:
    explicit NoteFiltersManager(
        Account account, FilterByTagWidget & filterByTagWidget,
        FilterByNotebookWidget & filterByNotebookWidget, NoteModel & noteModel,
        FilterBySavedSearchWidget & filterBySavedSearchWidget,
        FilterBySearchStringWidget & FilterBySearchStringWidget,
        LocalStorageManagerAsync & localStorageManagerAsync,
        QObject * parent = nullptr);

    ~NoteFiltersManager() override;

    [[nodiscard]] QStringList notebookLocalIdsInFilter() const;
    [[nodiscard]] QStringList tagLocalIdsInFilter() const;
    [[nodiscard]] const QString & savedSearchLocalIdInFilter() const noexcept;
    [[nodiscard]] bool isFilterBySearchStringActive() const;

    void clear();

    void setNotebooksToFilter(const QStringList & notebookLocalIds);
    void removeNotebooksFromFilter();

    void setTagsToFilter(const QStringList & tagLocalIds);
    void removeTagsFromFilter();

    void setSavedSearchLocalIdToFilter(const QString & savedSearchLocalId);
    void removeSavedSearchFromFilter();

    void setItemsToFilter(
        const QString & savedSearchLocalId,
        const QStringList & notebookLocalIds,
        const QStringList & tagLocalIds);

    /**
     * @return              True if all filters have already been properly
     *                      initialized, false otherwise
     */
    [[nodiscard]] bool isReady() const noexcept;

    [[nodiscard]] static NoteSearchQuery createNoteSearchQuery(
        const QString & searchString, ErrorString & errorDescription);

Q_SIGNALS:
    void notifyError(ErrorString errorDescription);

    void filterChanged();

    /**
     * @brief ready signal is emitted when all filters become properly
     * initialized
     */
    void ready();

    // private signals
    void findNoteLocalIdsForNoteSearchQuery(
        NoteSearchQuery noteSearchQuery, QUuid requestId);

    void addSavedSearch(qevercloud::SavedSearch savedSearch, QUuid requestId);

    void updateSavedSearch(
        qevercloud::SavedSearch savedSearch, QUuid requestId);

private Q_SLOTS:
    // Slots for FilterByTagWidget's signals
    void onAddedTagToFilter(
        const QString & tagLocalId, const QString & tagName,
        const QString & linkedNotebookGuid,
        const QString & linkedNotebookUsername);

    void onRemovedTagFromFilter(
        const QString & tagLocalId, const QString & tagName,
        const QString & linkedNotebookGuid,
        const QString & linkedNotebookUsername);

    void onTagsClearedFromFilter();
    void onTagsFilterUpdated();
    void onTagsFilterReady();

    // Slots for FilterByNotebookWidget's signals
    void onAddedNotebookToFilter(
        const QString & notebookLocalId, const QString & notebookName,
        const QString & linkedNotebookGuid,
        const QString & linkedNotebookUsername);

    void onRemovedNotebookFromFilter(
        const QString & notebookLocalId, const QString & notebookName,
        const QString & linkedNotebookGuid,
        const QString & linkedNotebookUsername);

    void onNotebooksClearedFromFilter();
    void onNotebooksFilterUpdated();
    void onNotebooksFilterReady();

    // Slots for saved search combo box
    void onSavedSearchFilterChanged(const QString & savedSearchName);
    void onSavedSearchFilterReady();

    // Slots for filter by search string widget
    void onSearchQueryChanged(QString query);
    void onSavedSearchQueryChanged(QString savedSearchLocalId, QString query);
    void onSearchSavingRequested(QString query);

    // Slots for events from local storage
    void onFindNoteLocalIdsWithSearchQueryCompleted(
        QStringList noteLocalIds, NoteSearchQuery noteSearchQuery,
        QUuid requestId);

    void onFindNoteLocalIdsWithSearchQueryFailed(
        NoteSearchQuery noteSearchQuery, ErrorString errorDescription,
        QUuid requestId);

    // Slots which should trigger refresh of note search (if any)
    void onAddNoteComplete(qevercloud::Note note, QUuid requestId);

    void onUpdateNoteComplete(
        qevercloud::Note note, LocalStorageManager::UpdateNoteOptions options,
        QUuid requestId);

    // NOTE: note model will deal with notes expunges on its own

    // NOTE: don't care of notebook updates because the filtering by notebook
    // is done by its local id anyway

    void onExpungeNotebookComplete(
        qevercloud::Notebook notebook, QUuid requestId);

    void onExpungeTagComplete(
        qevercloud::Tag tag, QStringList expungedChildTagLocalIds,
        QUuid requestId);

    void onUpdateSavedSearchComplete(
        qevercloud::SavedSearch search, QUuid requestId);

    void onExpungeSavedSearchComplete(
        qevercloud::SavedSearch search, QUuid requestId);

private:
    void createConnections();
    void evaluate();

    void persistSearchQuery(const QString & query);
    void restoreSearchQuery();

    bool setFilterBySavedSearch();
    bool setFilterBySearchString();
    void setFilterByNotebooks();
    void setFilterByTags();

    void clearFilterWidgetsItems();
    void clearFilterByTagWidgetItems();
    void clearFilterByNotebookWidgetItems();
    void clearFilterBySearchStringWidget();
    void clearFilterBySavedSearchWidget();

    void checkFiltersReadiness();

    void setNotebookToFilterImpl(const QString & notebookLocalId);
    void setNotebooksToFilterImpl(const QStringList & notebookLocalIds);
    void setTagsToFilterImpl(const QStringList & tagLocalIds);
    void setSavedSearchToFilterImpl(const QString & savedSearchLocalId);

    void checkAndRefreshNotesSearchQuery();

    bool setAutomaticFilterByNotebook();

    void persistFilterByNotebookClearedState(bool state);
    [[nodiscard]] bool notebookFilterWasCleared() const;

    void persistFilterByTagClearedState(bool state);
    [[nodiscard]] bool tagFilterWasCleared() const;

    void persistFilterBySavedSearchClearedState(bool state);
    [[nodiscard]] bool savedSearchFilterWasCleared() const;

    void showSearchQueryErrorToolTip(const ErrorString & errorDescription);

private:
    Account m_account;
    FilterByTagWidget & m_filterByTagWidget;
    FilterByNotebookWidget & m_filterByNotebookWidget;
    QPointer<NoteModel> m_pNoteModel;
    FilterBySavedSearchWidget & m_filterBySavedSearchWidget;
    FilterBySearchStringWidget & m_filterBySearchStringWidget;
    LocalStorageManagerAsync & m_localStorageManagerAsync;

    QString m_filteredSavedSearchLocalId;

    QString m_lastSearchString;

    QUuid m_findNoteLocalIdsForSearchStringRequestId;
    QUuid m_findNoteLocalIdsForSavedSearchQueryRequestId;

    bool m_autoFilterNotebookWhenReady = false;
    bool m_noteSearchQueryValidated = false;
    bool m_isReady = false;
};

} // namespace quentier

#endif // QUENTIER_LIB_WIDGET_NOTE_FILTERS_MANAGER_H
