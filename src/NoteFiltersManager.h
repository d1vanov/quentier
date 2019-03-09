/*
 * Copyright 2017-2019 Dmitry Ivanov
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

#ifndef QUENTIER_NOTE_FILTERS_MANAGER_H
#define QUENTIER_NOTE_FILTERS_MANAGER_H

#include <quentier/utility/Macros.h>
#include <quentier/local_storage/NoteSearchQuery.h>
#include <quentier/local_storage/LocalStorageManagerAsync.h>
#include <QObject>
#include <QUuid>

QT_FORWARD_DECLARE_CLASS(QLineEdit)

namespace quentier {

QT_FORWARD_DECLARE_CLASS(FilterByTagWidget)
QT_FORWARD_DECLARE_CLASS(FilterByNotebookWidget)
QT_FORWARD_DECLARE_CLASS(FilterBySavedSearchWidget)
QT_FORWARD_DECLARE_CLASS(NoteModel)
QT_FORWARD_DECLARE_CLASS(TagModel)

class NoteFiltersManager: public QObject
{
    Q_OBJECT
public:
    explicit NoteFiltersManager(const Account & account,
                                FilterByTagWidget & filterByTagWidget,
                                FilterByNotebookWidget & filterByNotebookWidget,
                                NoteModel & noteModel,
                                FilterBySavedSearchWidget & filterBySavedSearchWidget,
                                QLineEdit & searchLineEdit,
                                LocalStorageManagerAsync & localStorageManagerAsync,
                                QObject * parent = Q_NULLPTR);

    const QStringList & notebookLocalUidsInFilter() const;
    QStringList tagLocalUidsInFilter() const;
    const QString & savedSearchLocalUidInFilter() const;
    bool isFilterBySearchStringActive() const;

    void clear();
    void resetFilterToNotebookLocalUid(const QString & notebookLocalUid);

    /**
     * @return              True if all filters have already been properly
     *                      initialized, false otherwise
     */
    bool isReady() const;

    static NoteSearchQuery createNoteSearchQuery(const QString & searchString,
                                                 ErrorString & errorDescription);

Q_SIGNALS:
    void notifyError(ErrorString errorDescription);

    void filterChanged();

    /**
     * @brief ready signal is emitted when all filters become properly initialized
     */
    void ready();

    // private signals
    void findNoteLocalUidsForNoteSearchQuery(NoteSearchQuery noteSearchQuery,
                                             QUuid requestId);

private Q_SLOTS:
    // Slots for FilterByTagWidget's signals
    void onAddedTagToFilter(const QString & tagName);
    void onRemovedTagFromFilter(const QString & tagName);
    void onTagsClearedFromFilter();
    void onTagsFilterUpdated();
    void onTagsFilterReady();

    // Slots for FilterByNotebookWidget's signals
    void onAddedNotebookToFilter(const QString & notebookName);
    void onRemovedNotebookFromFilter(const QString & notebookName);
    void onNotebooksClearedFromFilter();
    void onNotebooksFilterUpdated();
    void onNotebooksFilterReady();

    // Slots for saved search combo box
    void onSavedSearchFilterChanged(const QString & savedSearchName);
    void onSavedSearchFilterReady();

    // Slots for the search line edit
    void onSearchStringEdited(const QString & text);
    void onSearchStringChanged();

    // Slots for events from local storage
    void onFindNoteLocalUidsWithSearchQueryCompleted(QStringList noteLocalUids,
                                                     NoteSearchQuery noteSearchQuery,
                                                     QUuid requestId);
    void onFindNoteLocalUidsWithSearchQueryFailed(NoteSearchQuery noteSearchQuery,
                                                  ErrorString errorDescription,
                                                  QUuid requestId);

    // NOTE: don't care of notebook updates because the filtering by notebook
    // is done by its local uid anyway

    void onExpungeNotebookComplete(Notebook notebook, QUuid requestId);

    void onExpungeTagComplete(Tag tag, QStringList expungedChildTagLocalUids,
                              QUuid requestId);

    void onUpdateSavedSearchComplete(SavedSearch search, QUuid requestId);
    void onExpungeSavedSearchComplete(SavedSearch search, QUuid requestId);

private:
    void createConnections();
    void evaluate();

    void persistSearchString();
    void restoreSearchString();

    bool setFilterBySearchString();
    bool setFilterBySavedSearch();
    void setFilterByNotebooks();
    void setFilterByTags();

    void clearFilterWidgetsItems();

    void checkFiltersReadiness();

private:
    Account                         m_account;
    FilterByTagWidget &             m_filterByTagWidget;
    FilterByNotebookWidget &        m_filterByNotebookWidget;
    NoteModel &                     m_noteModel;
    FilterBySavedSearchWidget &     m_filterBySavedSearchWidget;
    QLineEdit &                     m_searchLineEdit;
    LocalStorageManagerAsync &      m_localStorageManagerAsync;

    QString         m_filteredSavedSearchLocalUid;

    QString         m_lastSearchString;

    QUuid           m_findNoteLocalUidsForSearchStringRequestId;
    QUuid           m_findNoteLocalUidsForSavedSearchQueryRequestId;

    bool            m_noteSearchQueryValidated;

    bool            m_isReady;
};

} // namespace quentier

#endif // QUENTIER_NOTE_FILTERS_MANAGER_H
