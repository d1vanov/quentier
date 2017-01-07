/*
 * Copyright 2017 Dmitry Ivanov
 *
 * This file is part of quentier
 *
 * libquentier is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * libquentier is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with libquentier. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef QUENTIER_NOTE_FILTERS_MANAGER_H
#define QUENTIER_NOTE_FILTERS_MANAGER_H

#include <quentier/utility/Qt4Helper.h>
#include <quentier/utility/QNLocalizedString.h>
#include <quentier/local_storage/NoteSearchQuery.h>
#include <QObject>
#include <QUuid>

QT_FORWARD_DECLARE_CLASS(QComboBox)
QT_FORWARD_DECLARE_CLASS(QLineEdit)

namespace quentier {

QT_FORWARD_DECLARE_CLASS(FilterByTagWidget)
QT_FORWARD_DECLARE_CLASS(FilterByNotebookWidget)
QT_FORWARD_DECLARE_CLASS(NoteFilterModel)
QT_FORWARD_DECLARE_CLASS(LocalStorageManagerThreadWorker)

class NoteFiltersManager: public QObject
{
    Q_OBJECT
public:
    explicit NoteFiltersManager(FilterByTagWidget & filterByTagWidget,
                                FilterByNotebookWidget & filterByNotebookWidget,
                                NoteFilterModel & noteFilterModel,
                                QComboBox & filterBySavedSearchComboBox,
                                QLineEdit & searchLineEdit,
                                LocalStorageManagerThreadWorker & localStorageManager,
                                QObject * parent = Q_NULLPTR);

Q_SIGNALS:
    void notifyError(QNLocalizedString errorDescription);

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
                                                  QNLocalizedString errorDescription,
                                                  QUuid requestId);

private:
    void createConnections();

private:
    FilterByTagWidget &                 m_filterByTagWidget;
    FilterByNotebookWidget &            m_filterByNotebookWidget;
    NoteFilterModel &                   m_noteFilterModel;
    QComboBox &                         m_filterBySavedSearchComboBox;
    QLineEdit &                         m_searchLineEdit;
    LocalStorageManagerThreadWorker &   m_localStorageManager;
};

} // namespace quentier

#endif // QUENTIER_NOTE_FILTERS_MANAGER_H
