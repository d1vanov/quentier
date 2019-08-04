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

#include "NoteFiltersManager.h"
#include "FilterByTagWidget.h"
#include "FilterByNotebookWidget.h"
#include "FilterBySavedSearchWidget.h"

#include <lib/preferences/SettingsNames.h>
#include <lib/model/NoteModel.h>
#include <lib/model/SavedSearchModel.h>
#include <lib/model/NotebookModel.h>
#include <lib/model/TagModel.h>

#include <quentier/logging/QuentierLogger.h>
#include <quentier/utility/ApplicationSettings.h>

#include <QComboBox>
#include <QLineEdit>
#include <QToolTip>

namespace quentier {

#define NOTE_SEARCH_STRING_GROUP_KEY QStringLiteral("NoteSearchStringFilter")
#define NOTE_SEARCH_STRING_KEY QStringLiteral("SearchString")

NoteFiltersManager::NoteFiltersManager(
        const Account & account,
        FilterByTagWidget & filterByTagWidget,
        FilterByNotebookWidget & filterByNotebookWidget,
        NoteModel & noteModel,
        FilterBySavedSearchWidget & filterBySavedSearchWidget,
        QLineEdit & searchLineEdit,
        LocalStorageManagerAsync & localStorageManagerAsync,
        QObject * parent) :
    QObject(parent),
    m_account(account),
    m_filterByTagWidget(filterByTagWidget),
    m_filterByNotebookWidget(filterByNotebookWidget),
    m_pNoteModel(&noteModel),
    m_filterBySavedSearchWidget(filterBySavedSearchWidget),
    m_searchLineEdit(searchLineEdit),
    m_localStorageManagerAsync(localStorageManagerAsync),
    m_filteredSavedSearchLocalUid(),
    m_lastSearchString(),
    m_findNoteLocalUidsForSearchStringRequestId(),
    m_findNoteLocalUidsForSavedSearchQueryRequestId(),
    m_noteSearchQueryValidated(false),
    m_isReady(false)
{
    createConnections();

    restoreSearchString();
    if (setFilterBySearchString())
    {
        // As search string overrides all other filters, we are good to go
        Q_EMIT filterChanged();
        QNDEBUG("Was able to set the filter by search string, "
                "considering NoteFiltersManager ready");
        m_isReady = true;
        Q_EMIT ready();
        return;
    }

    QString savedSearchLocalUid =
        m_filterBySavedSearchWidget.filteredSavedSearchLocalUid();
    if (!savedSearchLocalUid.isEmpty())
    {
        // As filtered saved search's local uid is not empty, need to delay
        // the moment of filtering evaluatuon until filter by saved search is ready
        QNDEBUG("Filtered saved search's local uid is not empty, "
                "need to properly wait for filter's readiness");
        checkFiltersReadiness();
        return;
    }

    // If we got here, there are no filters by search string and saved search
    // We can set filters by notebooks and tags without waiting for them to be
    // complete since local uids of notebooks and tags within the filter are known
    // even before the filter widgets become ready
    noteModel.beginUpdateFilter();
    setFilterByNotebooks();
    setFilterByTags();
    noteModel.endUpdateFilter();

    Q_EMIT filterChanged();

    m_isReady = true;
    Q_EMIT ready();
}

QStringList NoteFiltersManager::notebookLocalUidsInFilter() const
{
    if (m_pNoteModel.isNull()) {
        return QStringList();
    }

    return m_pNoteModel->filteredNotebookLocalUids();
}

QStringList NoteFiltersManager::tagLocalUidsInFilter() const
{
    if (isFilterBySearchStringActive()) {
        return QStringList();
    }

    if (!savedSearchLocalUidInFilter().isEmpty()) {
        return QStringList();
    }

    if (m_pNoteModel.isNull()) {
        return QStringList();
    }

    return m_pNoteModel->filteredTagLocalUids();
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
    QNDEBUG("NoteFiltersManager::clear");

    clearFilterWidgetsItems();
    evaluate();
}

void NoteFiltersManager::resetFilterToNotebookLocalUid(
    const QString & notebookLocalUid)
{
    QNDEBUG("NoteFiltersManager::resetFilterToNotebookLocalUid: "
            << notebookLocalUid);

    if (notebookLocalUid.isEmpty()) {
        clear();
        return;
    }

    const NotebookModel * pNotebookModel =
        m_filterByNotebookWidget.notebookModel();
    if (Q_UNLIKELY(!pNotebookModel)) {
        QNDEBUG("Notebook model in the filter by notebook widget is null");
        clear();
        return;
    }

    const QString name = pNotebookModel->itemNameForLocalUid(notebookLocalUid);
    if (name.isEmpty()) {
        QNWARNING("Failed to find the notebook name for notebook local uid "
                  << notebookLocalUid);
        clear();
        return;
    }

    clearFilterWidgetsItems();

    QObject::disconnect(&m_filterByNotebookWidget,
                        QNSIGNAL(FilterByNotebookWidget,addedItemToFilter,QString),
                        this,
                        QNSLOT(NoteFiltersManager,onAddedNotebookToFilter,QString));

    m_filterByNotebookWidget.addItemToFilter(notebookLocalUid, name);

    QObject::connect(&m_filterByNotebookWidget,
                     QNSIGNAL(FilterByNotebookWidget,addedItemToFilter,QString),
                     this,
                     QNSLOT(NoteFiltersManager,onAddedNotebookToFilter,QString),
                     Qt::UniqueConnection);

    evaluate();
}

bool NoteFiltersManager::isReady() const
{
    return m_isReady;
}

void NoteFiltersManager::onAddedTagToFilter(const QString & tagName)
{
    QNDEBUG("NoteFiltersManager::onAddedTagToFilter: " << tagName);
    onTagsFilterUpdated();
}

void NoteFiltersManager::onRemovedTagFromFilter(const QString & tagName)
{
    QNDEBUG("NoteFiltersManager::onRemovedTagFromFilter: " << tagName);
    onTagsFilterUpdated();
}

void NoteFiltersManager::onTagsClearedFromFilter()
{
    QNDEBUG("NoteFiltersManager::onTagsClearedFromFilter");
    onTagsFilterUpdated();
}

void NoteFiltersManager::onTagsFilterUpdated()
{
    QNDEBUG("NoteFiltersManager::onTagsFilterUpdated");

    if (!m_isReady) {
        QNDEBUG("Not yet ready to process filter updates");
        return;
    }

    if (!m_filterByTagWidget.isEnabled()) {
        QNDEBUG("Filter by tag widget is not enabled which means "
                "that filtering by tags is overridden by either "
                "search string or filter by saved search");
        return;
    }

    setFilterByTags();
    Q_EMIT filterChanged();
}

void NoteFiltersManager::onTagsFilterReady()
{
    QNDEBUG("NoteFiltersManager::onTagsFilterReady");
    checkFiltersReadiness();
}

void NoteFiltersManager::onAddedNotebookToFilter(const QString & notebookName)
{
    QNDEBUG("NoteFiltersManager::onAddedNotebookToFilter: " << notebookName);
    onNotebooksFilterUpdated();
}

void NoteFiltersManager::onRemovedNotebookFromFilter(const QString & notebookName)
{
    QNDEBUG("NoteFiltersManager::onRemovedNotebookFromFilter: " << notebookName);
    onNotebooksFilterUpdated();
}

void NoteFiltersManager::onNotebooksClearedFromFilter()
{
    QNDEBUG("NoteFiltersManager::onNotebooksClearedFromFilter");
    onNotebooksFilterUpdated();
}

void NoteFiltersManager::onNotebooksFilterUpdated()
{
    QNDEBUG("NoteFiltersManager::onNotebooksFilterUpdated");

    if (!m_isReady) {
        QNDEBUG("Not yet ready to process filter updates");
        return;
    }

    if (!m_filterByNotebookWidget.isEnabled()) {
        QNDEBUG("Filter by notebook widget is not enabled which "
                "means that filtering by notebooks is overridden "
                "by either search string or filter by saved search");
        return;
    }

    setFilterByNotebooks();
    Q_EMIT filterChanged();
}

void NoteFiltersManager::onNotebooksFilterReady()
{
    QNDEBUG("NoteFiltersManager::onNotebooksFilterReady");
    checkFiltersReadiness();
}

void NoteFiltersManager::onSavedSearchFilterChanged(
    const QString & savedSearchName)
{
    QNDEBUG("NoteFiltersManager::onSavedSearchFilterChanged: "
            << savedSearchName);

    if (!m_isReady) {
        QNDEBUG("Not yet ready to process filter updates");
        return;
    }

    if (!m_filterBySavedSearchWidget.isEnabled()) {
        QNDEBUG("Filter by saved search widget is not enabled "
                "which means that filtering by saved search "
                "is overridden by search string");
        return;
    }

    if (m_pNoteModel.isNull()) {
        QNDEBUG("Note model is null");
        return;
    }

    bool res = setFilterBySavedSearch();
    if (res) {
        Q_EMIT filterChanged();
        return;
    }

    // If we got here, the saved search is either empty or invalid
    m_filteredSavedSearchLocalUid.clear();

    m_pNoteModel->beginUpdateFilter();

    setFilterByTags();
    setFilterByNotebooks();

    m_pNoteModel->endUpdateFilter();
}

void NoteFiltersManager::onSavedSearchFilterReady()
{
    QNDEBUG("NoteFiltersManager::onSavedSearchFilterReady");

    if (!m_isReady && setFilterBySavedSearch()) {
        // If we were not ready, there was no valid search string; if we managed
        // to set the filter by saved search, we are good to go because filter
        // by saved search overrides filters by notebooks and tags anyway
        QNDEBUG("Was able to set the filter by saved search, "
                "considering NoteFiltersManager ready");
        m_isReady = true;
        Q_EMIT ready();
        return;
    }

    checkFiltersReadiness();
}

void NoteFiltersManager::onSearchStringEdited(const QString & text)
{
    QNDEBUG("NoteFiltersManager::onSearchStringEdited: " << text);

    bool wasEmpty = m_lastSearchString.isEmpty();
    m_lastSearchString = text;
    if (!wasEmpty && m_lastSearchString.isEmpty()) {
        persistSearchString();
        evaluate();
    }
}

void NoteFiltersManager::onSearchStringChanged()
{
    QNDEBUG("NoteFiltersManager::onSearchStringChanged");

    if (m_lastSearchString.isEmpty() && m_searchLineEdit.text().isEmpty()) {
        QNDEBUG("Skipping the evaluation as the search string is "
                "empty => evaluation should have already occurred");
        return;
    }

    persistSearchString();
    evaluate();
}

void NoteFiltersManager::onFindNoteLocalUidsWithSearchQueryCompleted(
    QStringList noteLocalUids, NoteSearchQuery noteSearchQuery, QUuid requestId)
{
    if (m_pNoteModel.isNull()) {
        return;
    }

    bool isRequestForSearchString =
        (requestId == m_findNoteLocalUidsForSearchStringRequestId);
    bool isRequestForSavedSearch =
        !isRequestForSearchString &&
        (requestId == m_findNoteLocalUidsForSavedSearchQueryRequestId);

    if (!isRequestForSearchString && !isRequestForSavedSearch) {
        return;
    }

    QNDEBUG("NoteFiltersManager::onFindNoteLocalUidsWithSearchQueryCompleted: "
            << "note local uids: " << noteLocalUids.join(QStringLiteral(", "))
            << ", note search query: " << noteSearchQuery << "\nRequest id = "
            << requestId);

    if (Q_UNLIKELY(!isRequestForSearchString &&
                   !m_filterBySavedSearchWidget.isEnabled()))
    {
        QNDEBUG("Ignoring the update with note local uids for "
                "saved search because the filter by saved search "
                "widget is disabled which means filtering by saved "
                "search is overridden by filtering via search string");
        return;
    }

    m_pNoteModel->setFilteredNoteLocalUids(noteLocalUids);
}

void NoteFiltersManager::onFindNoteLocalUidsWithSearchQueryFailed(
    NoteSearchQuery noteSearchQuery, ErrorString errorDescription,
    QUuid requestId)
{
    if (m_pNoteModel.isNull()) {
        return;
    }

    bool isRequestForSearchString =
        (requestId == m_findNoteLocalUidsForSearchStringRequestId);
    bool isRequestForSavedSearch =
        !isRequestForSearchString &&
        (requestId == m_findNoteLocalUidsForSavedSearchQueryRequestId);

    if (!isRequestForSearchString && !isRequestForSavedSearch) {
        return;
    }

    QNWARNING("NoteFiltersManager::onFindNoteLocalUidsWithSearchQueryFailed: "
              << "request id = " << requestId
              << ", note search query = " << noteSearchQuery
              << "\nError description: " << errorDescription);

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

    m_pNoteModel->beginUpdateFilter();

    setFilterByNotebooks();
    setFilterByTags();

    m_pNoteModel->endUpdateFilter();

    Q_EMIT filterChanged();
}

void NoteFiltersManager::onAddNoteComplete(Note note, QUuid requestId)
{
    if (m_pNoteModel.isNull()) {
        return;
    }

    QNDEBUG("NoteFiltersManager::onAddNoteComplete: request id = " << requestId);
    QNTRACE(note);

    checkAndRefreshNotesSearchQuery();
}

void NoteFiltersManager::onUpdateNoteComplete(
    Note note, LocalStorageManager::UpdateNoteOptions options,
    QUuid requestId)
{
    if (m_pNoteModel.isNull()) {
        return;
    }

    QNDEBUG("NoteFiltersManager::onUpdateNoteComplete: request id = "
            << requestId);
    QNTRACE(note);

    Q_UNUSED(options);

    checkAndRefreshNotesSearchQuery();
}

void NoteFiltersManager::onExpungeNotebookComplete(
    Notebook notebook, QUuid requestId)
{
    QNDEBUG("NoteFiltersManager::onExpungeNotebookComplete: notebook = "
            << notebook << ", request id = " << requestId);

    if (!m_filterByNotebookWidget.isEnabled()) {
        QNDEBUG("Filter by notebook is overridden by either "
                "search string or saved search filter");
        return;
    }

    if (m_pNoteModel.isNull()) {
        QNDEBUG("Note model is null");
        return;
    }

    QStringList notebookLocalUids = m_pNoteModel->filteredNotebookLocalUids();
    int index = notebookLocalUids.indexOf(notebook.localUid());
    if (index < 0) {
        QNDEBUG("The expunged notebook was not used within the filter");
        return;
    }

    QNDEBUG("The expunged notebook was used within the filter");
    notebookLocalUids.removeAt(index);

    m_pNoteModel->setFilteredNotebookLocalUids(notebookLocalUids);
}

void NoteFiltersManager::onExpungeTagComplete(
    Tag tag, QStringList expungedChildTagLocalUids, QUuid requestId)
{
    QNDEBUG("NoteFiltersManager::onExpungeTagComplete: tag = "
            << tag << "\nExpunged child tag local uids: "
            << expungedChildTagLocalUids.join(QStringLiteral(", "))
            << ", request id = " << requestId);

    if (!m_filterByTagWidget.isEnabled()) {
        QNDEBUG("The filter by tags is overridden by either "
                "search string or filter by saved search");
        return;
    }

    if (m_pNoteModel.isNull()) {
        QNDEBUG("Note model is null");
        return;
    }

    QStringList expungedTagLocalUids;
    expungedTagLocalUids << tag.localUid();
    expungedTagLocalUids << expungedChildTagLocalUids;

    QStringList tagLocalUids = m_pNoteModel->filteredTagLocalUids();

    bool filteredTagsChanged = false;
    for(auto it = expungedTagLocalUids.constBegin(),
        end = expungedTagLocalUids.constEnd(); it != end; ++it)
    {
        int tagIndex = tagLocalUids.indexOf(*it);
        if (tagIndex >= 0) {
            tagLocalUids.removeAt(tagIndex);
            filteredTagsChanged = true;
        }
    }

    if (!filteredTagsChanged) {
        QNDEBUG("None of expunged tags seem to appear within "
                "the list of filtered tags");
        return;
    }

    m_pNoteModel->setFilteredTagLocalUids(tagLocalUids);
}

void NoteFiltersManager::onUpdateSavedSearchComplete(
    SavedSearch search, QUuid requestId)
{
    QNDEBUG("NoteFiltersManager::onUpdateSavedSearchComplete: search = "
            << search << "\nRequest id = " << requestId);

    QString currentSavedSearchName = m_filterBySavedSearchWidget.currentText();
    if (currentSavedSearchName.isEmpty()) {
        QNDEBUG("No saved search name is set to the filter");
        return;
    }

    if (m_filteredSavedSearchLocalUid == search.localUid())
    {
        QNDEBUG("The saved search within the filter was updated");

        QString searchString = m_searchLineEdit.text();
        if (!searchString.isEmpty() &&
            !m_filterBySavedSearchWidget.isEnabled())
        {
            QNDEBUG("The search string filter overrides the filter "
                    "by saved search");
            return;
        }

        if (Q_UNLIKELY(!search.hasName() || !search.hasQuery()))
        {
            QNDEBUG("The updated search lacks either name or query, "
                    "will remove it from the filter");
            m_filteredSavedSearchLocalUid.clear();
            // That would disable the filter by saved search's query i.e. by
            // note local uids
            onSavedSearchFilterChanged(QString());
            return;
        }

        onSavedSearchFilterChanged(search.name());
    }
}

void NoteFiltersManager::onExpungeSavedSearchComplete(
    SavedSearch search, QUuid requestId)
{
    QNDEBUG("NoteFiltersManager::onExpungeSavedSearchComplete: search = "
            << search << "\nRequest id = " << requestId);

    QString currentSavedSearchName = m_filterBySavedSearchWidget.currentText();
    if (currentSavedSearchName.isEmpty()) {
        QNDEBUG("No saved search name is set to the filter");
        return;
    }

    if (m_filteredSavedSearchLocalUid == search.localUid())
    {
        QNDEBUG("The saved search within the filter was expunged");

        QString searchString = m_searchLineEdit.text();
        if (!searchString.isEmpty() && !m_filterBySavedSearchWidget.isEnabled())
        {
            QNDEBUG("The search string filter overrides the filter "
                    "by saved search");
            m_filteredSavedSearchLocalUid.clear();
        }
        else {
            // That would disable the filter by saved search's query i.e. by
            // note local uids
            onSavedSearchFilterChanged(QString());
        }

        return;
    }
}

void NoteFiltersManager::createConnections()
{
    QNDEBUG("NoteFiltersManager::createConnections");

    QObject::connect(&m_filterByTagWidget,
                     QNSIGNAL(FilterByTagWidget,addedItemToFilter,QString),
                     this,
                     QNSLOT(NoteFiltersManager,onAddedTagToFilter,QString),
                     Qt::UniqueConnection);
    QObject::connect(&m_filterByTagWidget,
                     QNSIGNAL(FilterByTagWidget,itemRemovedFromFilter,QString),
                     this,
                     QNSLOT(NoteFiltersManager,onRemovedTagFromFilter,QString),
                     Qt::UniqueConnection);
    QObject::connect(&m_filterByTagWidget,
                     QNSIGNAL(FilterByTagWidget,cleared),
                     this,
                     QNSLOT(NoteFiltersManager,onTagsClearedFromFilter),
                     Qt::UniqueConnection);
    QObject::connect(&m_filterByTagWidget,
                     QNSIGNAL(FilterByTagWidget,updated),
                     this,
                     QNSLOT(NoteFiltersManager,onTagsFilterUpdated),
                     Qt::UniqueConnection);
    QObject::connect(&m_filterByTagWidget,
                     QNSIGNAL(FilterByTagWidget,ready),
                     this,
                     QNSLOT(NoteFiltersManager,onTagsFilterReady),
                     Qt::UniqueConnection);

    QObject::connect(&m_filterByNotebookWidget,
                     QNSIGNAL(FilterByNotebookWidget,addedItemToFilter,QString),
                     this,
                     QNSLOT(NoteFiltersManager,onAddedNotebookToFilter,QString),
                     Qt::UniqueConnection);
    QObject::connect(&m_filterByNotebookWidget,
                     QNSIGNAL(FilterByNotebookWidget,itemRemovedFromFilter,
                              QString),
                     this,
                     QNSLOT(NoteFiltersManager,onRemovedNotebookFromFilter,
                            QString),
                     Qt::UniqueConnection);
    QObject::connect(&m_filterByNotebookWidget,
                     QNSIGNAL(FilterByNotebookWidget,cleared),
                     this,
                     QNSLOT(NoteFiltersManager,onNotebooksClearedFromFilter),
                     Qt::UniqueConnection);
    QObject::connect(&m_filterByNotebookWidget,
                     QNSIGNAL(FilterByNotebookWidget,updated),
                     this,
                     QNSLOT(NoteFiltersManager,onNotebooksFilterUpdated),
                     Qt::UniqueConnection);
    QObject::connect(&m_filterByNotebookWidget,
                     QNSIGNAL(FilterByNotebookWidget,ready),
                     this,
                     QNSLOT(NoteFiltersManager,onNotebooksFilterReady),
                     Qt::UniqueConnection);

    QObject::connect(&m_filterBySavedSearchWidget,
                     SIGNAL(currentIndexChanged(QString)),
                     this,
                     SLOT(onSavedSearchFilterChanged(QString)),
                     Qt::UniqueConnection);
    QObject::connect(&m_filterBySavedSearchWidget,
                     QNSIGNAL(FilterBySavedSearchWidget,ready),
                     this,
                     QNSLOT(NoteFiltersManager,onSavedSearchFilterReady),
                     Qt::UniqueConnection);

    QObject::connect(&m_searchLineEdit,
                     QNSIGNAL(QLineEdit,textEdited,QString),
                     this,
                     QNSLOT(NoteFiltersManager,onSearchStringEdited,QString),
                     Qt::UniqueConnection);
    QObject::connect(&m_searchLineEdit, QNSIGNAL(QLineEdit,editingFinished),
                     this, QNSLOT(NoteFiltersManager,onSearchStringChanged),
                     Qt::UniqueConnection);

    QObject::connect(this,
                     QNSIGNAL(NoteFiltersManager,
                              findNoteLocalUidsForNoteSearchQuery,
                              NoteSearchQuery,QUuid),
                     &m_localStorageManagerAsync,
                     QNSLOT(LocalStorageManagerAsync,
                            onFindNoteLocalUidsWithSearchQuery,
                            NoteSearchQuery,QUuid),
                     Qt::UniqueConnection);
    QObject::connect(&m_localStorageManagerAsync,
                     QNSIGNAL(LocalStorageManagerAsync,
                              findNoteLocalUidsWithSearchQueryComplete,
                              QStringList,NoteSearchQuery,QUuid),
                     this,
                     QNSLOT(NoteFiltersManager,
                            onFindNoteLocalUidsWithSearchQueryCompleted,
                            QStringList,NoteSearchQuery,QUuid),
                     Qt::UniqueConnection);
    QObject::connect(&m_localStorageManagerAsync,
                     QNSIGNAL(LocalStorageManagerAsync,
                              findNoteLocalUidsWithSearchQueryFailed,
                              NoteSearchQuery,ErrorString,QUuid),
                     this,
                     QNSLOT(NoteFiltersManager,
                            onFindNoteLocalUidsWithSearchQueryFailed,
                            NoteSearchQuery,ErrorString,QUuid),
                     Qt::UniqueConnection);

    QObject::connect(&m_localStorageManagerAsync,
                     QNSIGNAL(LocalStorageManagerAsync,
                              expungeNotebookComplete,Notebook,QUuid),
                     this,
                     QNSLOT(NoteFiltersManager,onExpungeNotebookComplete,
                            Notebook,QUuid),
                     Qt::UniqueConnection);
    QObject::connect(&m_localStorageManagerAsync,
                     QNSIGNAL(LocalStorageManagerAsync,expungeTagComplete,
                              Tag,QStringList,QUuid),
                     this,
                     QNSLOT(NoteFiltersManager,onExpungeTagComplete,
                            Tag,QStringList,QUuid),
                     Qt::UniqueConnection);
    QObject::connect(&m_localStorageManagerAsync,
                     QNSIGNAL(LocalStorageManagerAsync,updateSavedSearchComplete,
                              SavedSearch,QUuid),
                     this,
                     QNSLOT(NoteFiltersManager,onUpdateSavedSearchComplete,
                            SavedSearch,QUuid),
                     Qt::UniqueConnection);
    QObject::connect(&m_localStorageManagerAsync,
                     QNSIGNAL(LocalStorageManagerAsync,expungeSavedSearchComplete,
                              SavedSearch,QUuid),
                     this,
                     QNSLOT(NoteFiltersManager,onExpungeSavedSearchComplete,
                            SavedSearch,QUuid),
                     Qt::UniqueConnection);
    QObject::connect(&m_localStorageManagerAsync,
                     QNSIGNAL(LocalStorageManagerAsync,addNoteComplete,
                              Note,QUuid),
                     this,
                     QNSLOT(NoteFiltersManager,onAddNoteComplete,Note,QUuid),
                     Qt::UniqueConnection);
    QObject::connect(&m_localStorageManagerAsync,
                     QNSIGNAL(LocalStorageManagerAsync,updateNoteComplete,
                              Note,LocalStorageManager::UpdateNoteOptions,QUuid),
                     this,
                     QNSLOT(NoteFiltersManager,onUpdateNoteComplete,
                            Note,LocalStorageManager::UpdateNoteOptions,QUuid),
                     Qt::UniqueConnection);
}

void NoteFiltersManager::evaluate()
{
    QNDEBUG("NoteFiltersManager::evaluate");

    if (m_pNoteModel.isNull()) {
        return;
    }

    // NOTE: the rules for note filter evaluation are the following:
    // 1) If the search string is not empty *and* is indeed a valid search string,
    //    if overrides the filters by notebooks, tags and saved search. So all
    //    three filter widgets are disabled if the search string is not empty and
    //    is valid.
    // 2) If the search string is empty or invalid and there's some selected saved
    //    search, filtering by saved search overrides the filters by notebooks
    //    and tags, so the filter widgets for notebooks and tags get disabled.
    // 3) If the search string is empty or invalid and the filter by saved search
    //    doesn't contain any selected saved search, the filters by notebooks and
    //    tags are effective and the corresponidng widgets are enabled.
    //    The filtering by notebooks and tags is cumulative i.e. only those notes
    //    would be accepted which both belong to one of the specified notebooks
    //    and have one of the specified tags. In other words, it's like search
    //    string containing notebook and tags but not containing "any" statement.

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

    m_pNoteModel->beginUpdateFilter();

    setFilterByNotebooks();
    setFilterByTags();

    m_pNoteModel->endUpdateFilter();

    Q_EMIT filterChanged();
}

void NoteFiltersManager::persistSearchString()
{
    QNDEBUG("NoteFiltersManager::persistSearchString");

    ApplicationSettings appSettings(m_account, QUENTIER_UI_SETTINGS);
    appSettings.beginGroup(NOTE_SEARCH_STRING_GROUP_KEY);
    appSettings.setValue(NOTE_SEARCH_STRING_KEY, m_searchLineEdit.text());
    appSettings.endGroup();
}

void NoteFiltersManager::restoreSearchString()
{
    QNDEBUG("NoteFiltersManager::restoreSearchString");

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
        QToolTip::showText(
            m_searchLineEdit.mapToGlobal(QPoint(0, m_searchLineEdit.height())),
            error.localizedString(), &m_searchLineEdit);
    }
}

bool NoteFiltersManager::setFilterBySearchString()
{
    QNDEBUG("NoteFiltersManager::setFilterBySearchString");

    QString searchString = m_searchLineEdit.text();
    if (searchString.isEmpty()) {
        QNDEBUG("The search string is empty");
        return false;
    }

    ErrorString error;
    NoteSearchQuery query = createNoteSearchQuery(searchString, error);
    if (query.isEmpty()) {
        QToolTip::showText(
            m_searchLineEdit.mapToGlobal(QPoint(0, m_searchLineEdit.height())),
            error.localizedString(), &m_searchLineEdit);
        return false;
    }

    // Invalidate the active request to find note local uids per saved search's
    // query (if there was any)
    m_findNoteLocalUidsForSavedSearchQueryRequestId = QUuid();

    m_findNoteLocalUidsForSearchStringRequestId = QUuid::createUuid();
    QNTRACE("Emitting the request to find note local uids "
            << "corresponding to the note search query: request id = "
            << m_findNoteLocalUidsForSearchStringRequestId
            << ", query: " << query << "\nSearch string: " << searchString);
    Q_EMIT findNoteLocalUidsForNoteSearchQuery(
        query, m_findNoteLocalUidsForSearchStringRequestId);

    m_filterByTagWidget.setDisabled(true);
    m_filterByNotebookWidget.setDisabled(true);
    m_filterBySavedSearchWidget.setDisabled(true);

    return true;
}

bool NoteFiltersManager::setFilterBySavedSearch()
{
    QNDEBUG("NoteFiltersManager::setFilterBySavedSearch");

    if (m_pNoteModel.isNull()) {
        QNDEBUG("Note model is null");
        return false;
    }

    // If this method got called in the first place, it means the search string
    // doesn't override the filter by saved search so should enable the filter
    // by saved search widget
    m_filterBySavedSearchWidget.setEnabled(true);

    QString currentSavedSearchName = m_filterBySavedSearchWidget.currentText();
    if (currentSavedSearchName.isEmpty()) {
        QNDEBUG("No saved search name is set to the filter");
        m_pNoteModel->clearFilteredNoteLocalUids();
        return false;
    }

    const SavedSearchModel * pSavedSearchModel =
        m_filterBySavedSearchWidget.savedSearchModel();
    if (Q_UNLIKELY(!pSavedSearchModel)) {
        QNDEBUG("Saved search model in the filter by saved search "
                "widget is null");
        m_pNoteModel->clearFilteredNoteLocalUids();
        return false;
    }

    QModelIndex itemIndex =
        pSavedSearchModel->indexForSavedSearchName(currentSavedSearchName);
    if (Q_UNLIKELY(!itemIndex.isValid()))
    {
        ErrorString error(QT_TR_NOOP("Internal error: can't set the filter by "
                                     "saved search, the saved search model "
                                     "returned invalid model index for the given "
                                     "saved search name"));
        QNWARNING(error);
        Q_EMIT notifyError(error);
        m_pNoteModel->clearFilteredNoteLocalUids();
        return false;
    }

    const SavedSearchModelItem * pItem = pSavedSearchModel->itemForIndex(itemIndex);
    if (Q_UNLIKELY(!pItem))
    {
        ErrorString error(QT_TR_NOOP("Internal error: can't set the filter by "
                                     "saved search, the saved search model "
                                     "returned null item for the given valid "
                                     "model index"));
        QNWARNING(error);
        Q_EMIT notifyError(error);
        m_pNoteModel->clearFilteredNoteLocalUids();
        return false;
    }

    if (Q_UNLIKELY(pItem->m_query.isEmpty()))
    {
        ErrorString error(QT_TR_NOOP("Can't set the filter by saved search: "
                                     "saved search's query is empty"));
        QNWARNING(error << ", saved search item: " << *pItem);
        Q_EMIT notifyError(error);
        m_pNoteModel->clearFilteredNoteLocalUids();
        return false;
    }

    NoteSearchQuery query;
    ErrorString errorDescription;
    bool res = query.setQueryString(pItem->m_query, errorDescription);
    if (Q_UNLIKELY(!res)) {
        ErrorString error(QT_TR_NOOP("Internal error: can't set the filter by "
                                     "saved search: failed to parse "
                                     "the saved search query"));
        error.appendBase(errorDescription.base());
        error.appendBase(errorDescription.additionalBases());
        error.details() = errorDescription.details();
        QNWARNING(error << ", saved search item: " << *pItem);
        Q_EMIT notifyError(error);
        m_pNoteModel->clearFilteredNoteLocalUids();
        return false;
    }

    m_filteredSavedSearchLocalUid = pItem->m_localUid;

    m_findNoteLocalUidsForSavedSearchQueryRequestId = QUuid::createUuid();
    QNTRACE("Emitting the request to find note local uids "
            << "corresponding to the saved search: request id = "
            << m_findNoteLocalUidsForSavedSearchQueryRequestId
            << ", query: " << query << "\nSaved search item: " << *pItem);
    Q_EMIT findNoteLocalUidsForNoteSearchQuery(
        query, m_findNoteLocalUidsForSavedSearchQueryRequestId);

    m_filterByTagWidget.setDisabled(true);
    m_filterByNotebookWidget.setDisabled(true);

    return true;
}

void NoteFiltersManager::setFilterByNotebooks()
{
    QNDEBUG("NoteFiltersManager::setFilterByNotebooks");

    if (m_pNoteModel.isNull()) {
        QNDEBUG("Note model is null");
        return;
    }

    // If this method got called in the first place, it means the notebook filter
    // is not overridden by either search string or saved search filters so should
    // enable the filter by notebook widget
    m_filterByNotebookWidget.setEnabled(true);

    QStringList notebookLocalUids = m_filterByNotebookWidget.localUidsOfItemsInFilter();

    QNTRACE("Notebook local uids to be used for filtering: "
            << (notebookLocalUids.isEmpty()
                ? QStringLiteral("<empty>")
                : notebookLocalUids.join(QStringLiteral(", "))));

    m_pNoteModel->setFilteredNotebookLocalUids(notebookLocalUids);
}

void NoteFiltersManager::setFilterByTags()
{
    QNDEBUG("NoteFiltersManager::setFilterByTags");

    if (m_pNoteModel.isNull()) {
        QNDEBUG("Note model is null");
        return;
    }

    // If this method got called in the first place, it means the tag filter is
    // not overridden by either search string or saved search filter so should
    // enable the filter by tag widget
    m_filterByTagWidget.setEnabled(true);

    QStringList tagLocalUids = m_filterByTagWidget.localUidsOfItemsInFilter();

    QNTRACE("Tag local uids to be used for filtering: "
            << tagLocalUids.join(QStringLiteral(", ")));

    m_pNoteModel->setFilteredTagLocalUids(tagLocalUids);
}

void NoteFiltersManager::clearFilterWidgetsItems()
{
    QNDEBUG("NoteFiltersManager::clearFilterWidgetsItems");

    // Clear tags

    QObject::disconnect(&m_filterByTagWidget, QNSIGNAL(FilterByTagWidget,cleared),
                        this, QNSLOT(NoteFiltersManager,onTagsClearedFromFilter));

    m_filterByTagWidget.clear();

    QObject::connect(&m_filterByTagWidget,
                     QNSIGNAL(FilterByTagWidget,addedItemToFilter,QString),
                     this,
                     QNSLOT(NoteFiltersManager,onAddedTagToFilter,QString),
                     Qt::UniqueConnection);

    // Clear notebooks

    QObject::disconnect(&m_filterByNotebookWidget,
                        QNSIGNAL(FilterByNotebookWidget,addedItemToFilter,QString),
                        this,
                        QNSLOT(NoteFiltersManager,onAddedNotebookToFilter,QString));

    m_filterByNotebookWidget.clear();

    QObject::connect(&m_filterByNotebookWidget,
                     QNSIGNAL(FilterByNotebookWidget,addedItemToFilter,QString),
                     this,
                     QNSLOT(NoteFiltersManager,onAddedNotebookToFilter,QString),
                     Qt::UniqueConnection);

    // Clear saved search

    QObject::disconnect(&m_filterBySavedSearchWidget,
                        SIGNAL(currentIndexChanged(QString)),
                        this,
                        SLOT(onSavedSearchFilterChanged(QString)));

    m_filterBySavedSearchWidget.setCurrentIndex(0);

    QObject::connect(&m_filterBySavedSearchWidget,
                     SIGNAL(currentIndexChanged(QString)),
                     this,
                     SLOT(onSavedSearchFilterChanged(QString)),
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
    QNDEBUG("NoteFiltersManager::checkFiltersReadiness");

    if (m_isReady) {
        QNDEBUG("Already marked the filter as ready once");
        return;
    }

    if (!m_filterByTagWidget.isReady()) {
        QNDEBUG("Still pending the readiness of filter by tags");
        return;
    }

    if (!m_filterByNotebookWidget.isReady()) {
        QNDEBUG("Still pending the readiness of filter by notebooks");
        return;
    }

    if (!m_filterBySavedSearchWidget.isReady()) {
        QNDEBUG("Still pending the readiness of filter by saved search");
        return;
    }

    QNDEBUG("All filters are ready");
    m_isReady = true;
    evaluate();
    Q_EMIT ready();
}

void NoteFiltersManager::checkAndRefreshNotesSearchQuery()
{
    QNDEBUG("NoteFiltersManager::checkAndRefreshNotesSearchQuery");

    // Refresh notes filtering if it was done via explicit search query or saved search
    if (!m_filterByTagWidget.isEnabled() &&
        !m_filterByNotebookWidget.isEnabled() &&
        (!m_searchLineEdit.text().isEmpty() ||
         m_filterBySavedSearchWidget.isEnabled()))
    {
        evaluate();
    }
}

NoteSearchQuery NoteFiltersManager::createNoteSearchQuery(
    const QString & searchString, ErrorString & errorDescription)
{
    if (searchString.isEmpty()) {
        errorDescription.clear();
        return NoteSearchQuery();
    }

    NoteSearchQuery query;
    bool res = query.setQueryString(searchString, errorDescription);
    if (!res) {
        QNDEBUG("The search string is invalid: error: " << errorDescription
                << ", search string: " << searchString);
        query.clear();
    }

    return query;
}

} // namespace quentier
