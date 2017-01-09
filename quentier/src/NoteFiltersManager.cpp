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

#include "NoteFiltersManager.h"
#include "widgets/FilterByTagWidget.h"
#include "widgets/FilterByNotebookWidget.h"
#include "models/NoteFilterModel.h"
#include <quentier/logging/QuentierLogger.h>
#include <quentier/local_storage/LocalStorageManagerThreadWorker.h>
#include <QComboBox>
#include <QLineEdit>

namespace quentier {

NoteFiltersManager::NoteFiltersManager(FilterByTagWidget & filterByTagWidget,
                                       FilterByNotebookWidget & filterByNotebookWidget,
                                       NoteFilterModel & noteFilterModel,
                                       QComboBox & filterBySavedSearchComboBox,
                                       QLineEdit & searchLineEdit,
                                       LocalStorageManagerThreadWorker & localStorageManager,
                                       QObject * parent) :
    QObject(parent),
    m_filterByTagWidget(filterByTagWidget),
    m_filterByNotebookWidget(filterByNotebookWidget),
    m_noteFilterModel(noteFilterModel),
    m_filterBySavedSearchComboBox(filterBySavedSearchComboBox),
    m_searchLineEdit(searchLineEdit),
    m_localStorageManager(localStorageManager)
{
    createConnections();
}

void NoteFiltersManager::onAddedTagToFilter(const QString & tagName)
{
    QNDEBUG(QStringLiteral("NoteFiltersManager::onAddedTagToFilter: ") << tagName);

    // TODO: implement
}

void NoteFiltersManager::onRemovedTagFromFilter(const QString & tagName)
{
    QNDEBUG(QStringLiteral("NoteFiltersManager::onRemovedTagFromFilter: ") << tagName);

    // TODO: implement
}

void NoteFiltersManager::onTagsClearedFromFilter()
{
    QNDEBUG(QStringLiteral("NoteFiltersManager::onTagsClearedFromFilter"));

    // TODO: implement
}

void NoteFiltersManager::onTagsFilterUpdated()
{
    QNDEBUG(QStringLiteral("NoteFiltersManager::onTagsFilterUpdated"));

    // TODO: implement
}

void NoteFiltersManager::onAddedNotebookToFilter(const QString & notebookName)
{
    QNDEBUG(QStringLiteral("NoteFiltersManager::onAddedNotebookToFilter: ") << notebookName);

    // TODO: implement
}

void NoteFiltersManager::onRemovedNotebookFromFilter(const QString & notebookName)
{
    QNDEBUG(QStringLiteral("NoteFiltersManager::onRemovedNotebookFromFilter: ") << notebookName);

    // TODO: implement
}

void NoteFiltersManager::onNotebooksClearedFromFilter()
{
    QNDEBUG(QStringLiteral("NoteFiltersManager::onNotebooksClearedFromFilter"));

    // TODO: implement
}

void NoteFiltersManager::onNotebooksFilterUpdated()
{
    QNDEBUG(QStringLiteral("NoteFiltersManager::onNotebooksFilterUpdated"));

    // TODO: implement
}

void NoteFiltersManager::onSavedSearchFilterChanged(const QString & savedSearchName)
{
    QNDEBUG(QStringLiteral("NoteFiltersManager::onSavedSearchFilterChanged: ") << savedSearchName);

    // TODO: implement
}

void NoteFiltersManager::onSearchStringChanged()
{
    QNDEBUG(QStringLiteral("NoteFiltersManager::onSearchStringChanged"));

    // TODO: implement
}

void NoteFiltersManager::onFindNoteLocalUidsWithSearchQueryCompleted(QStringList noteLocalUids,
                                                                     NoteSearchQuery noteSearchQuery,
                                                                     QUuid requestId)
{
    // TODO: implement
    Q_UNUSED(noteLocalUids)
    Q_UNUSED(noteSearchQuery)
    Q_UNUSED(requestId)
}

void NoteFiltersManager::onFindNoteLocalUidsWithSearchQueryFailed(NoteSearchQuery noteSearchQuery,
                                                                  QNLocalizedString errorDescription,
                                                                  QUuid requestId)
{
    // TODO: implement
    Q_UNUSED(noteSearchQuery)
    Q_UNUSED(errorDescription)
    Q_UNUSED(requestId)
}

void NoteFiltersManager::createConnections()
{
    QNDEBUG(QStringLiteral("NoteFiltersManager::createConnections"));

    QObject::connect(&m_filterByTagWidget, QNSIGNAL(FilterByTagWidget,addedItemToFilter,const QString&),
                     this, QNSLOT(NoteFiltersManager,onAddedTagToFilter,const QString&));
    QObject::connect(&m_filterByTagWidget, QNSIGNAL(FilterByTagWidget,itemRemovedFromFilter,const QString&),
                     this, QNSLOT(NoteFiltersManager,onRemovedTagFromFilter,const QString&));
    QObject::connect(&m_filterByTagWidget, QNSIGNAL(FilterByTagWidget,cleared),
                     this, QNSLOT(NoteFiltersManager,onTagsClearedFromFilter));
    QObject::connect(&m_filterByTagWidget, QNSIGNAL(FilterByTagWidget,updated),
                     this, QNSLOT(NoteFiltersManager,onTagsFilterUpdated));

    QObject::connect(&m_filterByNotebookWidget, QNSIGNAL(FilterByNotebookWidget,addedItemToFilter,const QString&),
                     this, QNSLOT(NoteFiltersManager,onAddedNotebookToFilter,const QString&));
    QObject::connect(&m_filterByNotebookWidget, QNSIGNAL(FilterByNotebookWidget,itemRemovedFromFilter,const QString&),
                     this, QNSLOT(NoteFiltersManager,onRemovedNotebookFromFilter,const QString&));
    QObject::connect(&m_filterByNotebookWidget, QNSIGNAL(FilterByNotebookWidget,cleared),
                     this, QNSLOT(NoteFiltersManager,onNotebooksClearedFromFilter));
    QObject::connect(&m_filterByNotebookWidget, QNSIGNAL(FilterByNotebookWidget,updated),
                     this, QNSLOT(NoteFiltersManager,onNotebooksFilterUpdated));

    QObject::connect(&m_filterBySavedSearchComboBox, SIGNAL(currentIndexChanged(QString)),
                     this, SLOT(onSavedSearchFilterChanged(QString)));

    QObject::connect(&m_searchLineEdit, QNSIGNAL(QLineEdit,editingFinished),
                     this, QNSLOT(NoteFiltersManager,onSearchStringChanged));

    QObject::connect(this, QNSIGNAL(NoteFiltersManager,findNoteLocalUidsForNoteSearchQuery,NoteSearchQuery,QUuid),
                     &m_localStorageManager,
                     QNSLOT(LocalStorageManagerThreadWorker,onFindNoteLocalUidsWithSearchQuery,NoteSearchQuery,QUuid));
    QObject::connect(&m_localStorageManager,
                     QNSIGNAL(LocalStorageManagerThreadWorker,findNoteLocalUidsWithSearchQueryComplete,QStringList,NoteSearchQuery,QUuid),
                     this, QNSLOT(NoteFiltersManager,onFindNoteLocalUidsWithSearchQueryCompleted,QStringList,NoteSearchQuery,QUuid));
    QObject::connect(&m_localStorageManager,
                     QNSIGNAL(LocalStorageManagerThreadWorker,findNoteLocalUidsWithSearchQueryFailed,NoteSearhQuery,QNLocalizedString,QUuid),
                     this, QNSLOT(NoteFiltersManager,onFindNoteLocalUidsWithSearchQueryFailed,NoteSearhQuery,QNLocalizedString,QUuid));
}

} // namespace quentier
