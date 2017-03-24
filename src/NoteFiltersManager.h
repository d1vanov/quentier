/*
 * Copyright 2017 Dmitry Ivanov
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
#include <quentier/types/ErrorString.h>
#include <quentier/types/Tag.h>
#include <quentier/types/Notebook.h>
#include <quentier/types/SavedSearch.h>
#include <quentier/types/Note.h>
#include <quentier/local_storage/NoteSearchQuery.h>
#include <QObject>
#include <QUuid>

QT_FORWARD_DECLARE_CLASS(QLineEdit)

namespace quentier {

QT_FORWARD_DECLARE_CLASS(FilterByTagWidget)
QT_FORWARD_DECLARE_CLASS(FilterByNotebookWidget)
QT_FORWARD_DECLARE_CLASS(FilterBySavedSearchWidget)
QT_FORWARD_DECLARE_CLASS(NoteFilterModel)
QT_FORWARD_DECLARE_CLASS(TagModel)
QT_FORWARD_DECLARE_CLASS(LocalStorageManagerThreadWorker)

class NoteFiltersManager: public QObject
{
    Q_OBJECT
public:
    explicit NoteFiltersManager(FilterByTagWidget & filterByTagWidget,
                                FilterByNotebookWidget & filterByNotebookWidget,
                                NoteFilterModel & noteFilterModel,
                                FilterBySavedSearchWidget & filterBySavedSearchWidget,
                                QLineEdit & searchLineEdit,
                                LocalStorageManagerThreadWorker & localStorageManager,
                                QObject * parent = Q_NULLPTR);

Q_SIGNALS:
    void notifyError(ErrorString errorDescription);

    // private signals
    void findNoteLocalUidsForNoteSearchQuery(NoteSearchQuery noteSearchQuery, QUuid requestId);

private Q_SLOTS:
    // Slots for FilterByTagWidget's signals
    void onAddedTagToFilter(const QString & tagName);
    void onRemovedTagFromFilter(const QString & tagName);
    void onTagsClearedFromFilter();
    void onTagsFilterUpdated();

    // Slots for FilterByNotebookWidget's signals
    void onAddedNotebookToFilter(const QString & notebookName);
    void onRemovedNotebookFromFilter(const QString & notebookName);
    void onNotebooksClearedFromFilter();
    void onNotebooksFilterUpdated();

    // Slots for saved search combo box
    void onSavedSearchFilterChanged(const QString & savedSearchName);

    // Slots for the search line edit
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

    void onUpdateTagComplete(Tag tag, QUuid requestId);
    void onExpungeTagComplete(Tag tag, QUuid requestId);

    void onUpdateSavedSearchComplete(SavedSearch search, QUuid requestId);
    void onExpungeSavedSearchComplete(SavedSearch search, QUuid requestId);

    void onAddNoteComplete(Note note, QUuid requestId);
    void onUpdateNoteComplete(Note note, bool updateResources, bool updateTags, QUuid requestId);
    void onExpungeNoteComplete(Note note, QUuid requestId);

private:
    void createConnections();
    void evaluate();

    bool setFilterBySearchString();
    bool setFilterBySavedSearch();
    void setFilterByNotebooks();
    void setFilterByTags();

    QStringList tagNamesFromLocalUids(const TagModel & tagModel) const;

private:
    FilterByTagWidget &                 m_filterByTagWidget;
    FilterByNotebookWidget &            m_filterByNotebookWidget;
    NoteFilterModel &                   m_noteFilterModel;
    FilterBySavedSearchWidget &         m_filterBySavedSearchWidget;
    QLineEdit &                         m_searchLineEdit;
    LocalStorageManagerThreadWorker &   m_localStorageManager;

    QSet<QString>                       m_filteredTagLocalUids;
    QString                             m_filteredSavedSearchLocalUid;

    QUuid                               m_findNoteLocalUidsForSearchStringRequestId;
    QUuid                               m_findNoteLocalUidsForSavedSearchQueryRequestId;
};

} // namespace quentier

#endif // QUENTIER_NOTE_FILTERS_MANAGER_H
