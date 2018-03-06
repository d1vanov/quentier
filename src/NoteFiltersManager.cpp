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
#include "SettingsNames.h"
#include "widgets/FilterByTagWidget.h"
#include "widgets/FilterByNotebookWidget.h"
#include "widgets/FilterBySavedSearchWidget.h"
#include "models/NoteFilterModel.h"
#include "models/SavedSearchModel.h"
#include "models/NotebookModel.h"
#include "models/TagModel.h"
#include <quentier/logging/QuentierLogger.h>
#include <quentier/local_storage/LocalStorageManagerAsync.h>
#include <quentier/utility/ApplicationSettings.h>
#include <QComboBox>
#include <QLineEdit>
#include <QToolTip>

namespace quentier {

#define NOTE_SEARCH_STRING_GROUP_KEY QStringLiteral("NoteSearchStringFilter")
#define NOTE_SEARCH_STRING_KEY QStringLiteral("SearchString")

NoteFiltersManager::NoteFiltersManager(const Account & account,
                                       FilterByTagWidget & filterByTagWidget,
                                       FilterByNotebookWidget & filterByNotebookWidget,
                                       NoteFilterModel & noteFilterModel,
                                       FilterBySavedSearchWidget & filterBySavedSearchWidget,
                                       QLineEdit & searchLineEdit,
                                       LocalStorageManagerAsync & localStorageManagerAsync,
                                       QObject * parent) :
    QObject(parent),
    m_account(account),
    m_filterByTagWidget(filterByTagWidget),
    m_filterByNotebookWidget(filterByNotebookWidget),
    m_noteFilterModel(noteFilterModel),
    m_filterBySavedSearchWidget(filterBySavedSearchWidget),
    m_searchLineEdit(searchLineEdit),
    m_localStorageManagerAsync(localStorageManagerAsync),
    m_filteredTagLocalUids(),
    m_filteredSavedSearchLocalUid(),
    m_lastSearchString(),
    m_findNoteLocalUidsForSearchStringRequestId(),
    m_findNoteLocalUidsForSavedSearchQueryRequestId(),
    m_noteSearchQueryValidated(false),
    m_isReady(false)
{
    createConnections();
    restoreSearchString();
    checkFiltersReadiness();
}

const QStringList & NoteFiltersManager::notebookLocalUidsInFilter() const
{
    return m_noteFilterModel.notebookLocalUids();
}

QStringList NoteFiltersManager::tagLocalUidsInFilter() const
{
    if (isFilterBySearchStringActive()) {
        return QStringList();
    }

    if (!savedSearchLocalUidInFilter().isEmpty()) {
        return QStringList();
    }

    QStringList filteredTagLocalUids;
    filteredTagLocalUids.reserve(m_filteredTagLocalUids.size());
    for(auto it = m_filteredTagLocalUids.constBegin(),
        end = m_filteredTagLocalUids.constEnd(); it != end; ++it)
    {
        filteredTagLocalUids << *it;
    }

    return filteredTagLocalUids;
}

const QString & NoteFiltersManager::savedSearchLocalUidInFilter() const
{
    return m_filteredSavedSearchLocalUid;
}

bool NoteFiltersManager::isFilterBySearchStringActive() const
{
    return !m_filterByTagWidget.isEnabled() &&
           !m_filterByNotebookWidget.isEnabled() &&
           !m_filterBySavedSearchWidget.isEnabled();
}

void NoteFiltersManager::clear()
{
    QNDEBUG(QStringLiteral("NoteFiltersManager::clear"));

    clearFilterWidgetsItems();
    evaluate();
}

void NoteFiltersManager::resetFilterToNotebookLocalUid(const QString & notebookLocalUid)
{
    QNDEBUG(QStringLiteral("NoteFiltersManager::resetFilterToNotebookLocalUid: ")
            << notebookLocalUid);

    if (notebookLocalUid.isEmpty()) {
        clear();
        return;
    }

    const NotebookModel * pNotebookModel = m_filterByNotebookWidget.notebookModel();
    if (Q_UNLIKELY(!pNotebookModel)) {
        QNDEBUG(QStringLiteral("Notebook model in the filter by notebook widget is null"));
        clear();
        return;
    }

    const QString name = pNotebookModel->itemNameForLocalUid(notebookLocalUid);
    if (name.isEmpty()) {
        QNWARNING(QStringLiteral("Failed to find the notebook name for notebook local uid ")
                  << notebookLocalUid);
        clear();
        return;
    }

    clearFilterWidgetsItems();

    QObject::disconnect(&m_filterByNotebookWidget, QNSIGNAL(FilterByNotebookWidget,addedItemToFilter,QString),
                        this, QNSLOT(NoteFiltersManager,onAddedNotebookToFilter,QString));

    m_filterByNotebookWidget.addItemToFilter(notebookLocalUid, name);

    QObject::connect(&m_filterByNotebookWidget, QNSIGNAL(FilterByNotebookWidget,addedItemToFilter,QString),
                     this, QNSLOT(NoteFiltersManager,onAddedNotebookToFilter,QString),
                     Qt::UniqueConnection);

    evaluate();
}

bool NoteFiltersManager::isReady() const
{
    return m_isReady;
}

void NoteFiltersManager::onAddedTagToFilter(const QString & tagName)
{
    QNDEBUG(QStringLiteral("NoteFiltersManager::onAddedTagToFilter: ") << tagName);
    onTagsFilterUpdated();
}

void NoteFiltersManager::onRemovedTagFromFilter(const QString & tagName)
{
    QNDEBUG(QStringLiteral("NoteFiltersManager::onRemovedTagFromFilter: ") << tagName);
    onTagsFilterUpdated();
}

void NoteFiltersManager::onTagsClearedFromFilter()
{
    QNDEBUG(QStringLiteral("NoteFiltersManager::onTagsClearedFromFilter"));
    onTagsFilterUpdated();
}

void NoteFiltersManager::onTagsFilterUpdated()
{
    QNDEBUG(QStringLiteral("NoteFiltersManager::onTagsFilterUpdated"));

    if (!m_isReady) {
        QNDEBUG(QStringLiteral("Not yet ready to process filter updates"));
        return;
    }

    if (!m_filterByTagWidget.isEnabled()) {
        QNDEBUG(QStringLiteral("Filter by tag widget is not enabled which means that filtering by tags is overridden "
                               "by either search string or filter by saved search"));
        return;
    }

    setFilterByTags();
    Q_EMIT filterChanged();
}

void NoteFiltersManager::onTagsFilterReady()
{
    QNDEBUG(QStringLiteral("NoteFiltersManager::onTagsFilterReady"));
    checkFiltersReadiness();
}

void NoteFiltersManager::onAddedNotebookToFilter(const QString & notebookName)
{
    QNDEBUG(QStringLiteral("NoteFiltersManager::onAddedNotebookToFilter: ") << notebookName);
    onNotebooksFilterUpdated();
}

void NoteFiltersManager::onRemovedNotebookFromFilter(const QString & notebookName)
{
    QNDEBUG(QStringLiteral("NoteFiltersManager::onRemovedNotebookFromFilter: ") << notebookName);
    onNotebooksFilterUpdated();
}

void NoteFiltersManager::onNotebooksClearedFromFilter()
{
    QNDEBUG(QStringLiteral("NoteFiltersManager::onNotebooksClearedFromFilter"));
    onNotebooksFilterUpdated();
}

void NoteFiltersManager::onNotebooksFilterUpdated()
{
    QNDEBUG(QStringLiteral("NoteFiltersManager::onNotebooksFilterUpdated"));

    if (!m_isReady) {
        QNDEBUG(QStringLiteral("Not yet ready to process filter updates"));
        return;
    }

    if (!m_filterByNotebookWidget.isEnabled()) {
        QNDEBUG(QStringLiteral("Filter by notebook widget is not enabled which means that filtering by notebooks is overridden "
                               "by either search string or filter by saved search"));
        return;
    }

    setFilterByNotebooks();
    Q_EMIT filterChanged();
}

void NoteFiltersManager::onNotebooksFilterReady()
{
    QNDEBUG(QStringLiteral("NoteFiltersManager::onNotebooksFilterReady"));
    checkFiltersReadiness();
}

void NoteFiltersManager::onSavedSearchFilterChanged(const QString & savedSearchName)
{
    QNDEBUG(QStringLiteral("NoteFiltersManager::onSavedSearchFilterChanged: ") << savedSearchName);

    if (!m_isReady) {
        QNDEBUG(QStringLiteral("Not yet ready to process filter updates"));
        return;
    }

    if (!m_filterBySavedSearchWidget.isEnabled()) {
        QNDEBUG(QStringLiteral("Filter by saved search widget is not enabled which means that filtering by saved search "
                               "is overridden by search string"));
        return;
    }

    bool res = setFilterBySavedSearch();
    if (res) {
        Q_EMIT filterChanged();
        return;
    }

    // If we got here, the saved search is either empty or invalid
    m_filteredSavedSearchLocalUid.clear();

    m_noteFilterModel.beginUpdateFilter();

    setFilterByTags();
    setFilterByNotebooks();

    m_noteFilterModel.endUpdateFilter();
}

void NoteFiltersManager::onSavedSearchFilterReady()
{
    QNDEBUG(QStringLiteral("NoteFiltersManager::onSavedSearchFilterReady"));
    checkFiltersReadiness();
}

void NoteFiltersManager::onSearchStringEdited(const QString & text)
{
    QNDEBUG(QStringLiteral("NoteFiltersManager::onSearchStringEdited: ") << text);

    bool wasEmpty = m_lastSearchString.isEmpty();
    m_lastSearchString = text;
    if (!wasEmpty && m_lastSearchString.isEmpty()) {
        evaluate();
    }
}

void NoteFiltersManager::onSearchStringChanged()
{
    QNDEBUG(QStringLiteral("NoteFiltersManager::onSearchStringChanged"));

    if (m_lastSearchString.isEmpty() && m_searchLineEdit.text().isEmpty()) {
        QNDEBUG(QStringLiteral("Skipping the evaluation as the search string is empty => evaluation should have already occurred"));
        return;
    }

    persistSearchString();
    evaluate();
}

void NoteFiltersManager::onFindNoteLocalUidsWithSearchQueryCompleted(QStringList noteLocalUids,
                                                                     NoteSearchQuery noteSearchQuery,
                                                                     QUuid requestId)
{
    bool isRequestForSearchString = (requestId == m_findNoteLocalUidsForSearchStringRequestId);
    bool isRequestForSavedSearch = !isRequestForSearchString && (requestId == m_findNoteLocalUidsForSavedSearchQueryRequestId);

    if (!isRequestForSearchString && !isRequestForSavedSearch) {
        return;
    }

    QNDEBUG(QStringLiteral("NoteFiltersManager::onFindNoteLocalUidsWithSearchQueryCompleted: note local uids: ")
            << noteLocalUids.join(QStringLiteral(", ")) << QStringLiteral(", note search query: ")
            << noteSearchQuery << QStringLiteral("\nRequest id = ") << requestId);

    if (Q_UNLIKELY(!isRequestForSearchString && !m_filterBySavedSearchWidget.isEnabled())) {
        QNDEBUG(QStringLiteral("Ignoring the update with note local uids for saved search because the filter "
                               "by saved search widget is disabled which means filtering by saved search is overridden "
                               "by filtering via search string"));
        return;
    }

    m_noteFilterModel.setNoteLocalUids(noteLocalUids);
}

void NoteFiltersManager::onFindNoteLocalUidsWithSearchQueryFailed(NoteSearchQuery noteSearchQuery,
                                                                  ErrorString errorDescription,
                                                                  QUuid requestId)
{
    bool isRequestForSearchString = (requestId == m_findNoteLocalUidsForSearchStringRequestId);
    bool isRequestForSavedSearch = !isRequestForSearchString && (requestId == m_findNoteLocalUidsForSavedSearchQueryRequestId);

    if (!isRequestForSearchString && !isRequestForSavedSearch) {
        return;
    }

    QNWARNING(QStringLiteral("NoteFiltersManager::onFindNoteLocalUidsWithSearchQueryFailed: request id = ")
              << requestId << QStringLiteral(", note search query = ") << noteSearchQuery
              << QStringLiteral("\nError description: ") << errorDescription);

    if (isRequestForSearchString)
    {
        ErrorString error(QT_TR_NOOP("Can't set the search string to note filter"));
        error.appendBase(errorDescription.base());
        error.appendBase(errorDescription.additionalBases());
        error.details() = errorDescription.details();
        QNDEBUG(error);
        Q_EMIT notifyError(error);

        m_filterBySavedSearchWidget.setEnabled(true);

        bool res = setFilterBySavedSearch();
        if (res) {
            Q_EMIT filterChanged();
            return;
        }
    }
    else // isRequestForSavedSearch
    {
        m_filteredSavedSearchLocalUid.clear();

        ErrorString error(QT_TR_NOOP("Can't set the saved search to note filter"));
        error.appendBase(errorDescription.base());
        error.appendBase(errorDescription.additionalBases());
        error.details() = errorDescription.details();
        QNDEBUG(error);
        Q_EMIT notifyError(error);
    }

    m_noteFilterModel.beginUpdateFilter();

    setFilterByNotebooks();
    setFilterByTags();

    m_noteFilterModel.endUpdateFilter();

    Q_EMIT filterChanged();
}

void NoteFiltersManager::onExpungeNotebookComplete(Notebook notebook, QUuid requestId)
{
    QNDEBUG(QStringLiteral("NoteFiltersManager::onExpungeNotebookComplete: notebook = ") << notebook
            << QStringLiteral(", request id = ") << requestId);

    if (!m_filterByNotebookWidget.isEnabled()) {
        QNDEBUG(QStringLiteral("Filter by notebook is overridden by either search string or saved search filter"));
        return;
    }

    QStringList notebookLocalUids = m_noteFilterModel.notebookLocalUids();
    int index = notebookLocalUids.indexOf(notebook.localUid());
    if (index < 0) {
        QNDEBUG(QStringLiteral("The expunged notebook was not used within the filter"));
        return;
    }

    QNDEBUG(QStringLiteral("The expunged notebook was used within the filter"));
    notebookLocalUids.removeAt(index);

    m_noteFilterModel.setNotebookLocalUids(notebookLocalUids);
}

void NoteFiltersManager::onUpdateTagComplete(Tag tag, QUuid requestId)
{
    QNDEBUG(QStringLiteral("NoteFiltersManager::onUpdateTagComplete: tag = ") << tag
            << QStringLiteral("\nRequest id = ") << requestId);

    auto it = m_filteredTagLocalUids.find(tag.localUid());
    if (it != m_filteredTagLocalUids.end())
    {
        QNDEBUG(QStringLiteral("One of tags within the filter was updated"));

        if (!m_filterByTagWidget.isEnabled()) {
            QNDEBUG(QStringLiteral("The filter by tags is overridden by either search string or filter by saved search"));
            return;
        }

        const TagModel * pTagModel = m_filterByTagWidget.tagModel();
        if (Q_UNLIKELY(!pTagModel)) {
            ErrorString error(QT_TR_NOOP("Internal error: can't update the filter by tags: no tag model within FilterByTagWidget"));
            QNWARNING(error);
            Q_EMIT notifyError(error);
            m_noteFilterModel.setTagNames(QStringList());
            return;
        }

        QStringList tagNames = tagNamesFromLocalUids(*pTagModel);

        QNTRACE(QStringLiteral("Tag names to be used for filtering: ")
                << tagNames.join(QStringLiteral(", ")));

        m_noteFilterModel.setTagNames(tagNames);
    }
}

void NoteFiltersManager::onExpungeTagComplete(Tag tag, QStringList expungedChildTagLocalUids, QUuid requestId)
{
    QNDEBUG(QStringLiteral("NoteFiltersManager::onExpungeTagComplete: tag = ") << tag
            << QStringLiteral("\nExpunged child tag local uids: ") << expungedChildTagLocalUids.join(QStringLiteral(", "))
            << QStringLiteral(", request id = ") << requestId);

    QStringList expungedTagLocalUids;
    expungedTagLocalUids << tag.localUid();
    expungedTagLocalUids << expungedChildTagLocalUids;

    bool filteredTagsChanged = false;
    for(auto it = expungedTagLocalUids.constBegin(), end = expungedTagLocalUids.constEnd(); it != end; ++it)
    {
        auto tagIt = m_filteredTagLocalUids.find(*it);
        if (tagIt != m_filteredTagLocalUids.end()) {
            Q_UNUSED(m_filteredTagLocalUids.erase(tagIt))
            filteredTagsChanged = true;
        }
    }

    if (!filteredTagsChanged) {
        QNDEBUG(QStringLiteral("None of expunged tags seem to appear within the list of filtered tags"));
        return;
    }

    if (!m_filterByTagWidget.isEnabled()) {
        QNDEBUG(QStringLiteral("The filter by tags is overridden by either search string or filter by saved search"));
        return;
    }

    const TagModel * pTagModel = m_filterByTagWidget.tagModel();
    if (Q_UNLIKELY(!pTagModel)) {
        ErrorString error(QT_TR_NOOP("Internal error: can't update the filter by tags: no tag model within FilterByTagWidget"));
        QNWARNING(error);
        Q_EMIT notifyError(error);
        m_noteFilterModel.setTagNames(QStringList());
        return;
    }

    QStringList tagNames = tagNamesFromLocalUids(*pTagModel);
    QNTRACE(QStringLiteral("Tag names to be used for filtering: ")
            << tagNames.join(QStringLiteral(", ")));

    m_noteFilterModel.setTagNames(tagNames);
}

void NoteFiltersManager::onUpdateSavedSearchComplete(SavedSearch search, QUuid requestId)
{
    QNDEBUG(QStringLiteral("NoteFiltersManager::onUpdateSavedSearchComplete: search = ") << search
            << QStringLiteral("\nRequest id = ") << requestId);

    QString currentSavedSearchName = m_filterBySavedSearchWidget.currentText();
    if (currentSavedSearchName.isEmpty()) {
        QNDEBUG(QStringLiteral("No saved search name is set to the filter"));
        return;
    }

    if (m_filteredSavedSearchLocalUid == search.localUid())
    {
        QNDEBUG(QStringLiteral("The saved search within the filter was updated"));

        QString searchString = m_searchLineEdit.text();
        if (!searchString.isEmpty() && !m_filterBySavedSearchWidget.isEnabled()) {
            QNDEBUG(QStringLiteral("The search string filter overrides the filter by saved search"));
            return;
        }

        if (Q_UNLIKELY(!search.hasName() || !search.hasQuery())) {
            QNDEBUG(QStringLiteral("The updated search lacks either name or query, will remove it from the filter"));
            m_filteredSavedSearchLocalUid.clear();
            onSavedSearchFilterChanged(QString());  // That would disable the filter by saved search's query i.e. by note local uids
            return;
        }

        onSavedSearchFilterChanged(search.name());
    }
}

void NoteFiltersManager::onExpungeSavedSearchComplete(SavedSearch search, QUuid requestId)
{
    QNDEBUG(QStringLiteral("NoteFiltersManager::onExpungeSavedSearchComplete: search = ") << search
            << QStringLiteral("\nRequest id = ") << requestId);

    QString currentSavedSearchName = m_filterBySavedSearchWidget.currentText();
    if (currentSavedSearchName.isEmpty()) {
        QNDEBUG(QStringLiteral("No saved search name is set to the filter"));
        return;
    }

    if (m_filteredSavedSearchLocalUid == search.localUid())
    {
        QNDEBUG(QStringLiteral("The saved search within the filter was expunged"));

        QString searchString = m_searchLineEdit.text();
        if (!searchString.isEmpty() && !m_filterBySavedSearchWidget.isEnabled()) {
            QNDEBUG(QStringLiteral("The search string filter overrides the filter by saved search"));
            m_filteredSavedSearchLocalUid.clear();
        }
        else {
            onSavedSearchFilterChanged(QString());  // That would disable the filter by saved search's query i.e. by note local uids
        }

        return;
    }
}

void NoteFiltersManager::onAddNoteComplete(Note note, QUuid requestId)
{
    QNDEBUG(QStringLiteral("NoteFiltersManager::onAddNoteComplete: note = ") << note
            << QStringLiteral("\nRequest id = ") << requestId);

    m_noteFilterModel.invalidate();
}

void NoteFiltersManager::onUpdateNoteComplete(Note note, bool updateResources, bool updateTags, QUuid requestId)
{
    QNDEBUG(QStringLiteral("NoteFiltersManager::onUpdateNoteComplete: note = ") << note
            << QStringLiteral("\nUpdate resources = ") << (updateResources ? QStringLiteral("true") : QStringLiteral("false"))
            << QStringLiteral(", update tags = ") << (updateTags ? QStringLiteral("true") : QStringLiteral("false"))
            << QStringLiteral(", request id = ") << requestId);

    m_noteFilterModel.invalidate();
}

void NoteFiltersManager::onExpungeNoteComplete(Note note, QUuid requestId)
{
    QNDEBUG(QStringLiteral("NoteFiltersManager::onExpungeNoteComplete: note = ") << note
            << QStringLiteral("\nRequest id = ") << requestId);

    m_noteFilterModel.invalidate();
}

void NoteFiltersManager::createConnections()
{
    QNDEBUG(QStringLiteral("NoteFiltersManager::createConnections"));

    QObject::connect(&m_filterByTagWidget, QNSIGNAL(FilterByTagWidget,addedItemToFilter,QString),
                     this, QNSLOT(NoteFiltersManager,onAddedTagToFilter,QString),
                     Qt::UniqueConnection);
    QObject::connect(&m_filterByTagWidget, QNSIGNAL(FilterByTagWidget,itemRemovedFromFilter,QString),
                     this, QNSLOT(NoteFiltersManager,onRemovedTagFromFilter,QString),
                     Qt::UniqueConnection);
    QObject::connect(&m_filterByTagWidget, QNSIGNAL(FilterByTagWidget,cleared),
                     this, QNSLOT(NoteFiltersManager,onTagsClearedFromFilter),
                     Qt::UniqueConnection);
    QObject::connect(&m_filterByTagWidget, QNSIGNAL(FilterByTagWidget,updated),
                     this, QNSLOT(NoteFiltersManager,onTagsFilterUpdated),
                     Qt::UniqueConnection);
    QObject::connect(&m_filterByTagWidget, QNSIGNAL(FilterByTagWidget,ready),
                     this, QNSLOT(NoteFiltersManager,onTagsFilterReady),
                     Qt::UniqueConnection);

    QObject::connect(&m_filterByNotebookWidget, QNSIGNAL(FilterByNotebookWidget,addedItemToFilter,QString),
                     this, QNSLOT(NoteFiltersManager,onAddedNotebookToFilter,QString),
                     Qt::UniqueConnection);
    QObject::connect(&m_filterByNotebookWidget, QNSIGNAL(FilterByNotebookWidget,itemRemovedFromFilter,QString),
                     this, QNSLOT(NoteFiltersManager,onRemovedNotebookFromFilter,QString),
                     Qt::UniqueConnection);
    QObject::connect(&m_filterByNotebookWidget, QNSIGNAL(FilterByNotebookWidget,cleared),
                     this, QNSLOT(NoteFiltersManager,onNotebooksClearedFromFilter),
                     Qt::UniqueConnection);
    QObject::connect(&m_filterByNotebookWidget, QNSIGNAL(FilterByNotebookWidget,updated),
                     this, QNSLOT(NoteFiltersManager,onNotebooksFilterUpdated),
                     Qt::UniqueConnection);
    QObject::connect(&m_filterByNotebookWidget, QNSIGNAL(FilterByNotebookWidget,ready),
                     this, QNSLOT(NoteFiltersManager,onNotebooksFilterReady),
                     Qt::UniqueConnection);

    QObject::connect(&m_filterBySavedSearchWidget, SIGNAL(currentIndexChanged(QString)),
                     this, SLOT(onSavedSearchFilterChanged(QString)),
                     Qt::UniqueConnection);
    QObject::connect(&m_filterBySavedSearchWidget, QNSIGNAL(FilterBySavedSearchWidget,ready),
                     this, QNSLOT(NoteFiltersManager,onSavedSearchFilterReady),
                     Qt::UniqueConnection);

    QObject::connect(&m_searchLineEdit, QNSIGNAL(QLineEdit,textEdited,QString),
                     this, QNSLOT(NoteFiltersManager,onSearchStringEdited,QString),
                     Qt::UniqueConnection);
    QObject::connect(&m_searchLineEdit, QNSIGNAL(QLineEdit,editingFinished),
                     this, QNSLOT(NoteFiltersManager,onSearchStringChanged),
                     Qt::UniqueConnection);

    QObject::connect(this, QNSIGNAL(NoteFiltersManager,findNoteLocalUidsForNoteSearchQuery,NoteSearchQuery,QUuid),
                     &m_localStorageManagerAsync,
                     QNSLOT(LocalStorageManagerAsync,onFindNoteLocalUidsWithSearchQuery,NoteSearchQuery,QUuid),
                     Qt::UniqueConnection);
    QObject::connect(&m_localStorageManagerAsync,
                     QNSIGNAL(LocalStorageManagerAsync,findNoteLocalUidsWithSearchQueryComplete,QStringList,NoteSearchQuery,QUuid),
                     this, QNSLOT(NoteFiltersManager,onFindNoteLocalUidsWithSearchQueryCompleted,QStringList,NoteSearchQuery,QUuid),
                     Qt::UniqueConnection);
    QObject::connect(&m_localStorageManagerAsync,
                     QNSIGNAL(LocalStorageManagerAsync,findNoteLocalUidsWithSearchQueryFailed,NoteSearchQuery,ErrorString,QUuid),
                     this, QNSLOT(NoteFiltersManager,onFindNoteLocalUidsWithSearchQueryFailed,NoteSearchQuery,ErrorString,QUuid),
                     Qt::UniqueConnection);

    QObject::connect(&m_localStorageManagerAsync, QNSIGNAL(LocalStorageManagerAsync,expungeNotebookComplete,Notebook,QUuid),
                     this, QNSLOT(NoteFiltersManager,onExpungeNotebookComplete,Notebook,QUuid),
                     Qt::UniqueConnection);
    QObject::connect(&m_localStorageManagerAsync, QNSIGNAL(LocalStorageManagerAsync,updateTagComplete,Tag,QUuid),
                     this, QNSLOT(NoteFiltersManager,onUpdateTagComplete,Tag,QUuid),
                     Qt::UniqueConnection);
    QObject::connect(&m_localStorageManagerAsync, QNSIGNAL(LocalStorageManagerAsync,expungeTagComplete,Tag,QStringList,QUuid),
                     this, QNSLOT(NoteFiltersManager,onExpungeTagComplete,Tag,QStringList,QUuid),
                     Qt::UniqueConnection);
    QObject::connect(&m_localStorageManagerAsync, QNSIGNAL(LocalStorageManagerAsync,updateSavedSearchComplete,SavedSearch,QUuid),
                     this, QNSLOT(NoteFiltersManager,onUpdateSavedSearchComplete,SavedSearch,QUuid),
                     Qt::UniqueConnection);
    QObject::connect(&m_localStorageManagerAsync, QNSIGNAL(LocalStorageManagerAsync,expungeSavedSearchComplete,SavedSearch,QUuid),
                     this, QNSLOT(NoteFiltersManager,onExpungeSavedSearchComplete,SavedSearch,QUuid),
                     Qt::UniqueConnection);
    QObject::connect(&m_localStorageManagerAsync, QNSIGNAL(LocalStorageManagerAsync,addNoteComplete,Note,QUuid),
                     this, QNSLOT(NoteFiltersManager,onAddNoteComplete,Note,QUuid),
                     Qt::UniqueConnection);
    QObject::connect(&m_localStorageManagerAsync, QNSIGNAL(LocalStorageManagerAsync,updateNoteComplete,Note,bool,bool,QUuid),
                     this, QNSLOT(NoteFiltersManager,onUpdateNoteComplete,Note,bool,bool,QUuid),
                     Qt::UniqueConnection);
    QObject::connect(&m_localStorageManagerAsync, QNSIGNAL(LocalStorageManagerAsync,expungeNoteComplete,Note,QUuid),
                     this, QNSLOT(NoteFiltersManager,onExpungeNoteComplete,Note,QUuid),
                     Qt::UniqueConnection);
}

void NoteFiltersManager::evaluate()
{
    QNDEBUG(QStringLiteral("NoteFiltersManager::evaluate"));

    // NOTE: the rules for note filter evaluation are the following:
    // 1) If the search string is not empty *and* is indeed a valid search string, if overrides the filters by notebooks,
    //    tags and saved search. So all three filter widgets are disabled if the search string is not empty and is valid.
    // 2) If the search string is empty or invalid and there's some selected saved search, filtering by saved search overrides
    //    the filters by notebooks and tags, so the filter widgets for notebooks and tags get disabled.
    // 3) If the search string is empty or invalid and the filter by saved search doesn't contain any selected saved search,
    //    the filters by notebooks and tags are effective and the corresponidng widgets are enabled. The filtering
    //    by notebooks and tags is cumulative i.e. only those notes would be accepted which both belong to one of the specified
    //    notebooks and have one of the specified tags. In other words, it's like search string containing notebook and tags
    //    but not containing "any" statement.

    bool res = setFilterBySearchString();
    if (res) {
        Q_EMIT filterChanged();
        return;
    }

    res = setFilterBySavedSearch();
    if (res) {
        Q_EMIT filterChanged();
        return;
    }

    m_noteFilterModel.beginUpdateFilter();

    setFilterByNotebooks();
    setFilterByTags();

    m_noteFilterModel.endUpdateFilter();

    Q_EMIT filterChanged();
}

void NoteFiltersManager::persistSearchString()
{
    QNDEBUG(QStringLiteral("NoteFiltersManager::persistSearchString"));

    ApplicationSettings appSettings(m_account, QUENTIER_UI_SETTINGS);
    appSettings.beginGroup(NOTE_SEARCH_STRING_GROUP_KEY);
    appSettings.setValue(NOTE_SEARCH_STRING_KEY, m_searchLineEdit.text());
    appSettings.endGroup();
}

void NoteFiltersManager::restoreSearchString()
{
    QNDEBUG(QStringLiteral("NoteFiltersManager::restoreSearchString"));

    ApplicationSettings appSettings(m_account, QUENTIER_UI_SETTINGS);
    appSettings.beginGroup(NOTE_SEARCH_STRING_GROUP_KEY);
    m_lastSearchString = appSettings.value(NOTE_SEARCH_STRING_KEY).toString();
    appSettings.endGroup();

    m_searchLineEdit.setText(m_lastSearchString);

    if (m_lastSearchString.isEmpty()) {
        return;
    }

    ErrorString error;
    NoteSearchQuery query = createNoteSearchQuery(m_lastSearchString, error);
    if (query.isEmpty()) {
        QToolTip::showText(m_searchLineEdit.mapToGlobal(QPoint(0, m_searchLineEdit.height())),
                           error.localizedString(), &m_searchLineEdit);
    }
}

bool NoteFiltersManager::setFilterBySearchString()
{
    QNDEBUG(QStringLiteral("NoteFiltersManager::setFilterBySearchString"));

    QString searchString = m_searchLineEdit.text();
    if (searchString.isEmpty()) {
        QNDEBUG(QStringLiteral("The search string is empty"));
        return false;
    }

    ErrorString error;
    NoteSearchQuery query = createNoteSearchQuery(searchString, error);
    if (query.isEmpty()) {
        QToolTip::showText(m_searchLineEdit.mapToGlobal(QPoint(0, m_searchLineEdit.height())),
                           error.localizedString(), &m_searchLineEdit);
        return false;
    }

    // Invalidate the active request to find note local uids per saved search's query (if there was any)
    m_findNoteLocalUidsForSavedSearchQueryRequestId = QUuid();

    m_findNoteLocalUidsForSearchStringRequestId = QUuid::createUuid();
    QNTRACE(QStringLiteral("Emitting the request to find note local uids corresponding to the note search query: request id = ")
            << m_findNoteLocalUidsForSearchStringRequestId << QStringLiteral(", query: ") << query
            << QStringLiteral("\nSearch string: ") << searchString);
    Q_EMIT findNoteLocalUidsForNoteSearchQuery(query, m_findNoteLocalUidsForSearchStringRequestId);

    m_filterByTagWidget.setDisabled(true);
    m_filterByNotebookWidget.setDisabled(true);
    m_filterBySavedSearchWidget.setDisabled(true);

    return true;
}

bool NoteFiltersManager::setFilterBySavedSearch()
{
    QNDEBUG(QStringLiteral("NoteFiltersManager::setFilterBySavedSearch"));

    // If this method got called in the first place, it means the search string doesn't override
    // the filter by saved search so should enable the filter by saved search widget
    m_filterBySavedSearchWidget.setEnabled(true);

    QString currentSavedSearchName = m_filterBySavedSearchWidget.currentText();
    if (currentSavedSearchName.isEmpty()) {
        QNDEBUG(QStringLiteral("No saved search name is set to the filter"));
        m_noteFilterModel.clearNoteLocalUids();
        return false;
    }

    const SavedSearchModel * pSavedSearchModel = m_filterBySavedSearchWidget.savedSearchModel();
    if (Q_UNLIKELY(!pSavedSearchModel)) {
        QNDEBUG(QStringLiteral("Saved search model in the filter by saved search widget is null"));
        m_noteFilterModel.clearNoteLocalUids();
        return false;
    }

    QModelIndex itemIndex = pSavedSearchModel->indexForSavedSearchName(currentSavedSearchName);
    if (Q_UNLIKELY(!itemIndex.isValid())) {
        ErrorString error(QT_TR_NOOP("Internal error: can't set the filter by saved search, the saved search model "
                                     "returned invalid model index for the given saved search name"));
        QNWARNING(error);
        Q_EMIT notifyError(error);
        m_noteFilterModel.clearNoteLocalUids();
        return false;
    }

    const SavedSearchModelItem * pItem = pSavedSearchModel->itemForIndex(itemIndex);
    if (Q_UNLIKELY(!pItem)) {
        ErrorString error(QT_TR_NOOP("Internal error: can't set the filter by saved search, the saved search model "
                                     "returned null item for the given valid model index"));
        QNWARNING(error);
        Q_EMIT notifyError(error);
        m_noteFilterModel.clearNoteLocalUids();
        return false;
    }

    if (Q_UNLIKELY(pItem->m_query.isEmpty())) {
        ErrorString error(QT_TR_NOOP("Can't set the filter by saved search: saved search's query is empty"));
        QNWARNING(error << QStringLiteral(", saved search item: ") << *pItem);
        Q_EMIT notifyError(error);
        m_noteFilterModel.clearNoteLocalUids();
        return false;
    }

    NoteSearchQuery query;
    ErrorString errorDescription;
    bool res = query.setQueryString(pItem->m_query, errorDescription);
    if (Q_UNLIKELY(!res)) {
        ErrorString error(QT_TR_NOOP("Internal error: can't set the filter by saved search: failed to parse "
                                     "the saved search query"));
        error.appendBase(errorDescription.base());
        error.appendBase(errorDescription.additionalBases());
        error.details() = errorDescription.details();
        QNWARNING(error << QStringLiteral(", saved search item: ") << *pItem);
        Q_EMIT notifyError(error);
        m_noteFilterModel.clearNoteLocalUids();
        return false;
    }

    m_filteredSavedSearchLocalUid = pItem->m_localUid;

    m_findNoteLocalUidsForSavedSearchQueryRequestId = QUuid::createUuid();
    QNTRACE(QStringLiteral("Emitting the request to find note local uids corresponding to the saved search: request id = ")
            << m_findNoteLocalUidsForSavedSearchQueryRequestId << QStringLiteral(", query: ") << query
            << QStringLiteral("\nSaved search item: ") << *pItem);
    Q_EMIT findNoteLocalUidsForNoteSearchQuery(query, m_findNoteLocalUidsForSavedSearchQueryRequestId);

    m_filterByTagWidget.setDisabled(true);
    m_filterByNotebookWidget.setDisabled(true);

    return true;
}

void NoteFiltersManager::setFilterByNotebooks()
{
    QNDEBUG(QStringLiteral("NoteFiltersManager::setFilterByNotebooks"));

    // If this method got called in the first place, it means the notebook filter is not overridden by either
    // search string or saved search filters so should enable the filter by notebook widget
    m_filterByNotebookWidget.setEnabled(true);

    const NotebookModel * pNotebookModel = m_filterByNotebookWidget.notebookModel();
    if (Q_UNLIKELY(!pNotebookModel)) {
        QNDEBUG(QStringLiteral("Notebook model in the filter by notebook widget is null"));
        m_noteFilterModel.setNotebookLocalUids(QStringList());
        return;
    }

    QStringList notebookNames = m_filterByNotebookWidget.itemsInFilter();
    if (notebookNames.isEmpty()) {
        QNDEBUG(QStringLiteral("No filtered notebooks"));
        m_noteFilterModel.setNotebookLocalUids(QStringList());
        return;
    }

    QStringList notebookLocalUids;
    notebookLocalUids.reserve(notebookNames.size());
    for(auto it = notebookNames.constBegin(), end = notebookNames.constEnd(); it != end; ++it)
    {
        QString localUid = pNotebookModel->localUidForItemName(*it, /* linked notebook guid = */ QString());
        if (Q_UNLIKELY(localUid.isEmpty())) {
            ErrorString error(QT_TR_NOOP("Internal error: can't set the filter by notebooks: the notebook model "
                                         "returned empty local uid for the notebook name in the filter"));
            QNWARNING(error << QStringLiteral(", notebook name: ") << *it);
            Q_EMIT notifyError(error);
            m_noteFilterModel.setNotebookLocalUids(QStringList());
            return;
        }

        notebookLocalUids << localUid;
    }

    QNTRACE(QStringLiteral("Notebook local uids to be used for filtering: ")
            << (notebookLocalUids.isEmpty() ? QStringLiteral("<empty>") : notebookLocalUids.join(QStringLiteral(", "))));

    m_noteFilterModel.setNotebookLocalUids(notebookLocalUids);
}

void NoteFiltersManager::setFilterByTags()
{
    QNDEBUG(QStringLiteral("NoteFiltersManager::setFilterByTags"));

    const TagModel * pTagModel = m_filterByTagWidget.tagModel();
    if (Q_UNLIKELY(!pTagModel)) {
        ErrorString error(QT_TR_NOOP("Internal error: can't set the filter by tags: no tag model within FilterByTagWidget"));
        QNWARNING(error);
        Q_EMIT notifyError(error);
        m_noteFilterModel.setTagNames(QStringList());
        return;
    }

    QStringList tagNames = m_filterByTagWidget.itemsInFilter();
    QNTRACE(QStringLiteral("Tag names to be used for filtering: ")
            << (tagNames.isEmpty() ? QStringLiteral("<empty>") : tagNames.join(QStringLiteral(", "))));

    QStringList tagLocalUids;
    tagLocalUids.reserve(tagNames.size());

    for(auto it = tagNames.constBegin(), end = tagNames.constEnd(); it != end; ++it)
    {
        QModelIndex itemIndex = pTagModel->indexForTagName(*it);
        if (Q_UNLIKELY(!itemIndex.isValid())) {
            ErrorString error(QT_TR_NOOP("Internal error: can't set the filter by tags: tag model returned "
                                         "invalid index for one of tag names"));
            QNWARNING(error << QStringLiteral(", tag name = ") << *it);
            Q_EMIT notifyError(error);
            m_noteFilterModel.setTagNames(QStringList());
            return;
        }

        const TagModelItem * pItem = pTagModel->itemForIndex(itemIndex);
        if (Q_UNLIKELY(!pItem)) {
            ErrorString error(QT_TR_NOOP("Internal error: can't set the filter by tags: tag model returned "
                                         "null item for a valid index corresponding to one of tag names"));
            QNWARNING(error << QStringLiteral(", tag name = ") << *it);
            Q_EMIT notifyError(error);
            m_noteFilterModel.setTagNames(QStringList());
            return;
        }

        const TagItem * pTagItem = pItem->tagItem();
        if (Q_UNLIKELY(!pTagItem)) {
            continue;
        }

        tagLocalUids << pTagItem->localUid();
    }

    m_filteredTagLocalUids.clear();
    for(auto it = tagLocalUids.constBegin(), end = tagLocalUids.constEnd(); it != end; ++it) {
        Q_UNUSED(m_filteredTagLocalUids.insert(*it))
    }

    QNTRACE(QStringLiteral("Tag local uids to be used for filtering: ")
            << tagLocalUids.join(QStringLiteral(", ")));

    // If this method got called in the first place, it means the tag filter is not overridden by either search string
    // or saved search filter so should enable the filter by tag widget
    m_filterByTagWidget.setEnabled(true);

    m_noteFilterModel.setTagNames(tagNames);
}

QStringList NoteFiltersManager::tagNamesFromLocalUids(const TagModel & tagModel) const
{
    QStringList tagNames;
    tagNames.reserve(m_filteredTagLocalUids.size());

    for(auto it = m_filteredTagLocalUids.constBegin(), end = m_filteredTagLocalUids.constEnd(); it != end; ++it)
    {
        const TagModelItem * pItem = tagModel.itemForLocalUid(*it);
        if (Q_UNLIKELY(!pItem)) {
            QNWARNING(QStringLiteral("Can't find tag model item for local uid ") << *it);
            continue;
        }

        const TagItem * pTagItem = pItem->tagItem();
        if (Q_UNLIKELY(!pTagItem)) {
            continue;
        }

        tagNames << pTagItem->name();
    }

    return tagNames;
}

void NoteFiltersManager::clearFilterWidgetsItems()
{
    QNDEBUG(QStringLiteral("NoteFiltersManager::clearFilterWidgetsItems"));

    // Clear tags

    QObject::disconnect(&m_filterByTagWidget, QNSIGNAL(FilterByTagWidget,cleared),
                        this, QNSLOT(NoteFiltersManager,onTagsClearedFromFilter));

    m_filterByTagWidget.clear();

    QObject::connect(&m_filterByTagWidget, QNSIGNAL(FilterByTagWidget,addedItemToFilter,QString),
                     this, QNSLOT(NoteFiltersManager,onAddedTagToFilter,QString),
                     Qt::UniqueConnection);

    // Clear notebooks

    QObject::disconnect(&m_filterByNotebookWidget, QNSIGNAL(FilterByNotebookWidget,addedItemToFilter,QString),
                        this, QNSLOT(NoteFiltersManager,onAddedNotebookToFilter,QString));

    m_filterByNotebookWidget.clear();

    QObject::connect(&m_filterByNotebookWidget, QNSIGNAL(FilterByNotebookWidget,addedItemToFilter,QString),
                     this, QNSLOT(NoteFiltersManager,onAddedNotebookToFilter,QString),
                     Qt::UniqueConnection);

    // Clear saved search

    QObject::disconnect(&m_filterBySavedSearchWidget, SIGNAL(currentIndexChanged(QString)),
                        this, SLOT(onSavedSearchFilterChanged(QString)));

    m_filterBySavedSearchWidget.setCurrentIndex(0);

    QObject::connect(&m_filterBySavedSearchWidget, SIGNAL(currentIndexChanged(QString)),
                     this, SLOT(onSavedSearchFilterChanged(QString)),
                     Qt::UniqueConnection);

    // Clear search string

    QObject::disconnect(&m_searchLineEdit, QNSIGNAL(QLineEdit,editingFinished),
                        this, QNSLOT(NoteFiltersManager,onSearchStringChanged));

    m_searchLineEdit.setText(QString());

    QObject::connect(&m_searchLineEdit, QNSIGNAL(QLineEdit,editingFinished),
                     this, QNSLOT(NoteFiltersManager,onSearchStringChanged),
                     Qt::UniqueConnection);

    persistSearchString();
}

void NoteFiltersManager::checkFiltersReadiness()
{
    QNDEBUG(QStringLiteral("NoteFiltersManager::checkFiltersReadiness"));

    if (m_isReady) {
        QNDEBUG(QStringLiteral("Already marked the filter as ready once"));
        return;
    }

    if (!m_filterByTagWidget.isReady()) {
        QNDEBUG(QStringLiteral("Still pending the readiness of filter by tags"));
        return;
    }

    if (!m_filterByNotebookWidget.isReady()) {
        QNDEBUG(QStringLiteral("Still pending the readiness of filter by notebooks"));
        return;
    }

    if (!m_filterBySavedSearchWidget.isReady()) {
        QNDEBUG(QStringLiteral("Still pending the readiness of filter by saved search"));
        return;
    }

    QNDEBUG(QStringLiteral("All filters are ready"));
    m_isReady = true;
    evaluate();
    Q_EMIT ready();
}

NoteSearchQuery NoteFiltersManager::createNoteSearchQuery(const QString & searchString,
                                                          ErrorString & errorDescription)
{
    if (searchString.isEmpty()) {
        errorDescription.clear();
        return NoteSearchQuery();
    }

    NoteSearchQuery query;
    bool res = query.setQueryString(searchString, errorDescription);
    if (!res) {
        QNDEBUG(QStringLiteral("The search string is invalid: error: ") << errorDescription
                << QStringLiteral(", search string: ") << searchString);
        query.clear();
    }

    return query;
}

} // namespace quentier
