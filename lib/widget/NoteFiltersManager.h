/*
 * Copyright 2017-2020 Dmitry Ivanov
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
#include <quentier/utility/Macros.h>

#include <QObject>
#include <QPointer>
#include <QUuid>

QT_FORWARD_DECLARE_CLASS(QLineEdit)

namespace quentier {

QT_FORWARD_DECLARE_CLASS(FilterByNotebookWidget)
QT_FORWARD_DECLARE_CLASS(FilterBySavedSearchWidget)
QT_FORWARD_DECLARE_CLASS(FilterBySearchStringWidget)
QT_FORWARD_DECLARE_CLASS(FilterByTagWidget)
QT_FORWARD_DECLARE_CLASS(NoteModel)
QT_FORWARD_DECLARE_CLASS(TagModel)

class NoteFiltersManager : public QObject
{
    Q_OBJECT
public:
    explicit NoteFiltersManager(
        const Account & account, FilterByTagWidget & filterByTagWidget,
        FilterByNotebookWidget & filterByNotebookWidget, NoteModel & noteModel,
        FilterBySavedSearchWidget & filterBySavedSearchWidget,
        FilterBySearchStringWidget & FilterBySearchStringWidget,
        LocalStorageManagerAsync & localStorageManagerAsync,
        QObject * parent = nullptr);

    virtual ~NoteFiltersManager() override;

    QStringList notebookLocalUidsInFilter() const;
    QStringList tagLocalUidsInFilter() const;
    const QString & savedSearchLocalUidInFilter() const;
    bool isFilterBySearchStringActive() const;

    void clear();

    void setNotebooksToFilter(const QStringList & notebookLocalUids);
    void removeNotebooksFromFilter();

    void setTagsToFilter(const QStringList & tagLocalUids);
    void removeTagsFromFilter();

    void setSavedSearchLocalUidToFilter(const QString & savedSearchLocalUid);
    void removeSavedSearchFromFilter();

    /**
     * @return              True if all filters have already been properly
     *                      initialized, false otherwise
     */
    bool isReady() const;

    static NoteSearchQuery createNoteSearchQuery(
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
    void findNoteLocalUidsForNoteSearchQuery(
        NoteSearchQuery noteSearchQuery, QUuid requestId);

    void addSavedSearch(SavedSearch savedSearch, QUuid requestId);
    void updateSavedSearch(SavedSearch savedSearch, QUuid requestId);

private Q_SLOTS:
    // Slots for FilterByTagWidget's signals
    void onAddedTagToFilter(
        const QString & tagLocalUid, const QString & tagName,
        const QString & linkedNotebookGuid,
        const QString & linkedNotebookUsername);

    void onRemovedTagFromFilter(
        const QString & tagLocalUid, const QString & tagName,
        const QString & linkedNotebookGuid,
        const QString & linkedNotebookUsername);

    void onTagsClearedFromFilter();
    void onTagsFilterUpdated();
    void onTagsFilterReady();

    // Slots for FilterByNotebookWidget's signals
    void onAddedNotebookToFilter(
        const QString & notebookLocalUid, const QString & notebookName,
        const QString & linkedNotebookGuid,
        const QString & linkedNotebookUsername);

    void onRemovedNotebookFromFilter(
        const QString & notebookLocalUid, const QString & notebookName,
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
    void onSavedSearchQueryChanged(QString savedSearchLocalUid, QString query);

    // Slots for events from local storage
    void onFindNoteLocalUidsWithSearchQueryCompleted(
        QStringList noteLocalUids, NoteSearchQuery noteSearchQuery,
        QUuid requestId);

    void onFindNoteLocalUidsWithSearchQueryFailed(
        NoteSearchQuery noteSearchQuery, ErrorString errorDescription,
        QUuid requestId);

    // Slots which should trigger refresh of note search (if any)
    void onAddNoteComplete(Note note, QUuid requestId);

    void onUpdateNoteComplete(
        Note note, LocalStorageManager::UpdateNoteOptions options,
        QUuid requestId);

    // NOTE: note model will deal with notes expunges on its own

    // NOTE: don't care of notebook updates because the filtering by notebook
    // is done by its local uid anyway

    void onExpungeNotebookComplete(Notebook notebook, QUuid requestId);

    void onExpungeTagComplete(
        Tag tag, QStringList expungedChildTagLocalUids, QUuid requestId);

    void onAddSavedSearchComplete(SavedSearch search, QUuid requestId);
    void onUpdateSavedSearchComplete(SavedSearch search, QUuid requestId);
    void onExpungeSavedSearchComplete(SavedSearch search, QUuid requestId);

private:
    void createConnections();
    void evaluate();

    void persistSearchString();
    void restoreSearchString();

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

    void setNotebookToFilterImpl(const QString & notebookLocalUid);
    void setNotebooksToFilterImpl(const QStringList & notebookLocalUids);
    void setTagsToFilterImpl(const QStringList & tagLocalUids);
    void setSavedSearchToFilterImpl(const QString & savedSearchLocalUid);

    void checkAndRefreshNotesSearchQuery();

    bool setAutomaticFilterByNotebook();

    void persistFilterByNotebookClearedState(const bool state);
    bool notebookFilterWasCleared() const;

    void persistFilterByTagClearedState(const bool state);
    bool tagFilterWasCleared() const;

    void persistFilterBySavedSearchClearedState(const bool state);
    bool savedSearchFilterWasCleared() const;

    void showSearchQueryErrorToolTip(const ErrorString & errorDescription);

private:
    Account m_account;
    FilterByTagWidget & m_filterByTagWidget;
    FilterByNotebookWidget & m_filterByNotebookWidget;
    QPointer<NoteModel> m_pNoteModel;
    FilterBySavedSearchWidget & m_filterBySavedSearchWidget;
    FilterBySearchStringWidget & m_filterBySearchStringWidget;
    LocalStorageManagerAsync & m_localStorageManagerAsync;

    QString m_filteredSavedSearchLocalUid;

    QString m_lastSearchString;

    QUuid m_findNoteLocalUidsForSearchStringRequestId;
    QUuid m_findNoteLocalUidsForSavedSearchQueryRequestId;

    bool m_autoFilterNotebookWhenReady = false;

    bool m_noteSearchQueryValidated = false;

    bool m_isReady = false;
};

} // namespace quentier

#endif // QUENTIER_LIB_WIDGET_NOTE_FILTERS_MANAGER_H
