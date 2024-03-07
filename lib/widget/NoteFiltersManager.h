/*
 * Copyright 2017-2024 Dmitry Ivanov
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

#pragma once

#include <quentier/local_storage/Fwd.h>
#include <quentier/local_storage/NoteSearchQuery.h>
#include <quentier/types/Account.h>

#include <qevercloud/types/Note.h>
#include <qevercloud/types/SavedSearch.h>

#include <QObject>
#include <QPointer>

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
        const Account & account, FilterByTagWidget & filterByTagWidget,
        FilterByNotebookWidget & filterByNotebookWidget, NoteModel & noteModel,
        FilterBySavedSearchWidget & filterBySavedSearchWidget,
        FilterBySearchStringWidget & FilterBySearchStringWidget,
        local_storage::ILocalStoragePtr localStorage,
        QObject * parent = nullptr);

    ~NoteFiltersManager() override;

    [[nodiscard]] QStringList notebookLocalIdsInFilter() const;
    [[nodiscard]] QStringList tagLocalIdsInFilter() const;
    [[nodiscard]] const QString & savedSearchLocalIdInFilter() const;
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

    [[nodiscard]] static local_storage::NoteSearchQuery createNoteSearchQuery(
        const QString & searchString, ErrorString & errorDescription);

Q_SIGNALS:
    void notifyError(ErrorString errorDescription);

    void filterChanged();

    /**
     * @brief ready signal is emitted when all filters become properly
     * initialized
     */
    void ready();

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

private:
    void createConnections();
    void evaluate();

    void onFindNoteLocalIdsWithSearchQueryCompleted(
        QStringList noteLocalIds,
        local_storage::NoteSearchQuery noteSearchQuery);

    void onFindNoteLocalIdsWithSearchQueryFailed(
        local_storage::NoteSearchQuery noteSearchQuery,
        ErrorString errorDescription);

    void onNotePut(const qevercloud::Note & note);
    void onNotebookExpunged(const QString & notebookLocalId);
    void onTagExpunged(
        const QString & tagLocalId, const QStringList & childTagLocalIds);

    void onSavedSearchPut(const qevercloud::SavedSearch & savedSearch);
    void onSavedSearchExpunged(const QString & localId);

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

    void persistFilterBySavedSearchClearedState(const bool state);
    [[nodiscard]] bool savedSearchFilterWasCleared() const;

    void showSearchQueryErrorToolTip(const ErrorString & errorDescription);

private:
    const Account m_account;
    const local_storage::ILocalStoragePtr m_localStorage;

    FilterByTagWidget & m_filterByTagWidget;
    FilterByNotebookWidget & m_filterByNotebookWidget;
    QPointer<NoteModel> m_pNoteModel;
    FilterBySavedSearchWidget & m_filterBySavedSearchWidget;
    FilterBySearchStringWidget & m_filterBySearchStringWidget;

    QString m_filteredSavedSearchLocalId;
    QString m_lastSearchString;

    bool m_pendingFindNoteLocalIdsForSearchString = false;
    bool m_pendingFindNoteLocalIdsForSavedSearchQuery = false;

    bool m_autoFilterNotebookWhenReady = false;
    bool m_noteSearchQueryValidated = false;
    bool m_isReady = false;
};

} // namespace quentier
