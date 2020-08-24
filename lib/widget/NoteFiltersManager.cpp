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

#include "NoteFiltersManager.h"

#include "FilterByNotebookWidget.h"
#include "FilterBySavedSearchWidget.h"
#include "FilterByTagWidget.h"

#include <lib/model/NoteModel.h>
#include <lib/model/SavedSearchModel.h>
#include <lib/model/notebook/NotebookModel.h>
#include <lib/model/tag/TagModel.h>
#include <lib/preferences/SettingsNames.h>

#include <quentier/logging/QuentierLogger.h>
#include <quentier/utility/ApplicationSettings.h>

#include <QComboBox>
#include <QLineEdit>
#include <QToolTip>

namespace quentier {

// DEPRECATED: NOTE_FILTERS_GROUP_KEY should be used instead
#define NOTE_SEARCH_STRING_GROUP_KEY QStringLiteral("NoteSearchStringFilter")

#define NOTE_FILTERS_GROUP_KEY  QStringLiteral("NoteFilters")
#define NOTE_SEARCH_STRING_KEY  QStringLiteral("SearchString")
#define NOTEBOOK_FILTER_CLEARED QStringLiteral("NotebookFilterCleared")
#define TAG_FILTER_CLEARED      QStringLiteral("TagFilterCleared")

NoteFiltersManager::NoteFiltersManager(
    const Account & account, FilterByTagWidget & filterByTagWidget,
    FilterByNotebookWidget & filterByNotebookWidget, NoteModel & noteModel,
    FilterBySavedSearchWidget & filterBySavedSearchWidget,
    QLineEdit & searchLineEdit,
    LocalStorageManagerAsync & localStorageManagerAsync, QObject * parent) :
    QObject(parent),
    m_account(account), m_filterByTagWidget(filterByTagWidget),
    m_filterByNotebookWidget(filterByNotebookWidget), m_pNoteModel(&noteModel),
    m_filterBySavedSearchWidget(filterBySavedSearchWidget),
    m_searchLineEdit(searchLineEdit),
    m_localStorageManagerAsync(localStorageManagerAsync)
{
    createConnections();

    restoreSearchString();
    if (setFilterBySearchString()) {
        // As search string overrides all other filters, we are good to go
        Q_EMIT filterChanged();

        QNDEBUG(
            "widget:note_filters",
            "Was able to set the filter by search "
                << "string, considering NoteFiltersManager ready");

        m_isReady = true;
        Q_EMIT ready();
        return;
    }

    QString savedSearchLocalUid =
        m_filterBySavedSearchWidget.filteredSavedSearchLocalUid();

    if (!savedSearchLocalUid.isEmpty()) {
        // As filtered saved search's local uid is not empty, need to delay
        // the moment of filtering evaluatuon until filter by saved search is
        // ready
        QNDEBUG(
            "widget:note_filters",
            "Filtered saved search's local uid is "
                << "not empty, need to properly wait for filter's readiness");

        checkFiltersReadiness();
        return;
    }

    // If we got here, there are no filters by search string and saved search
    // We can set filters by notebooks and tags without waiting for them to be
    // complete since local uids of notebooks and tags within the filter are
    // known even before the filter widgets become ready
    noteModel.beginUpdateFilter();
    setFilterByNotebooks();
    setFilterByTags();
    noteModel.endUpdateFilter();

    if (noteModel.filteredNotebookLocalUids().isEmpty() &&
        noteModel.filteredTagLocalUids().isEmpty())
    {
        if (!setAutomaticFilterByNotebook()) {
            QNDEBUG(
                "widget:note_filters",
                "Will wait for notebook model's "
                    << "readiness");
            m_autoFilterNotebookWhenReady = true;
            return;
        }
    }

    Q_EMIT filterChanged();

    m_isReady = true;
    Q_EMIT ready();
}

NoteFiltersManager::~NoteFiltersManager() = default;

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
    QNDEBUG("widget:note_filters", "NoteFiltersManager::clear");

    clearFilterWidgetsItems();
    evaluate();
}

void NoteFiltersManager::setNotebookToFilter(const QString & notebookLocalUid)
{
    QNDEBUG(
        "widget:note_filters",
        "NoteFiltersManager::setNotebookToFilter: " << notebookLocalUid);

    clearFilterByNotebookWidgetItems();
    setNotebookToFilterImpl(notebookLocalUid);
    evaluate();
}

void NoteFiltersManager::removeNotebooksFromFilter()
{
    QNDEBUG(
        "widget:note_filters", "NoteFiltersManager::removeNotebooksFromFilter");

    persistFilterByNotebookClearedState(true);
    clearFilterByNotebookWidgetItems();
    evaluate();
}

void NoteFiltersManager::setTagsToFilter(const QStringList & tagLocalUids)
{
    QNDEBUG(
        "widget:note_filters",
        "NoteFiltersManager::setTagsToFilter: "
            << tagLocalUids.join(QStringLiteral(", ")));

    clearFilterByTagWidgetItems();
    setTagsToFilterImpl(tagLocalUids);
    evaluate();
}

void NoteFiltersManager::removeTagsFromFilter()
{
    QNDEBUG("widget:note_filters", "NoteFiltersManager::removeTagsFromFilter");

    persistFilterByTagClearedState(true);
    clearFilterByTagWidgetItems();
    evaluate();
}

bool NoteFiltersManager::isReady() const
{
    return m_isReady;
}

void NoteFiltersManager::onAddedTagToFilter(
    const QString & tagLocalUid, const QString & tagName,
    const QString & linkedNotebookGuid, const QString & linkedNotebookUsername)
{
    QNDEBUG(
        "widget:note_filters",
        "NoteFiltersManager::onAddedTagToFilter: "
            << "local uid = " << tagLocalUid << ", name = " << tagName
            << ", linked notebook guid = " << linkedNotebookGuid
            << ", linked notebook username = " << linkedNotebookUsername);

    onTagsFilterUpdated();
}

void NoteFiltersManager::onRemovedTagFromFilter(
    const QString & tagLocalUid, const QString & tagName,
    const QString & linkedNotebookGuid, const QString & linkedNotebookUsername)
{
    QNDEBUG(
        "widget:note_filters",
        "NoteFiltersManager::onRemovedTagFromFilter: local uid = "
            << tagLocalUid << ", name = " << tagName
            << ", linked notebook guid = " << linkedNotebookGuid
            << ", linked notebook username = " << linkedNotebookUsername);

    onTagsFilterUpdated();
}

void NoteFiltersManager::onTagsClearedFromFilter()
{
    QNDEBUG(
        "widget:note_filters", "NoteFiltersManager::onTagsClearedFromFilter");

    onTagsFilterUpdated();
}

void NoteFiltersManager::onTagsFilterUpdated()
{
    QNDEBUG("widget:note_filters", "NoteFiltersManager::onTagsFilterUpdated");

    if (!m_isReady) {
        QNDEBUG(
            "widget:note_filters",
            "Not yet ready to process filter "
                << "updates");
        return;
    }

    if (!m_filterByTagWidget.isEnabled()) {
        QNDEBUG(
            "widget:note_filters",
            "Filter by tag widget is not enabled "
                << "which means that filtering by tags is overridden by either "
                << "search string or filter by saved search");
        return;
    }

    setFilterByTags();
    Q_EMIT filterChanged();
}

void NoteFiltersManager::onTagsFilterReady()
{
    QNDEBUG("widget:note_filters", "NoteFiltersManager::onTagsFilterReady");
    checkFiltersReadiness();
}

void NoteFiltersManager::onAddedNotebookToFilter(
    const QString & notebookLocalUid, const QString & notebookName,
    const QString & linkedNotebookGuid, const QString & linkedNotebookUsername)
{
    QNDEBUG(
        "widget:note_filters",
        "NoteFiltersManager::onAddedNotebookToFilter: local uid = "
            << notebookLocalUid << ", name = " << notebookName
            << ", linked notebook guid = " << linkedNotebookGuid
            << ", linked notebook username = " << linkedNotebookUsername);

    onNotebooksFilterUpdated();
}

void NoteFiltersManager::onRemovedNotebookFromFilter(
    const QString & notebookLocalUid, const QString & notebookName,
    const QString & linkedNotebookGuid, const QString & linkedNotebookUsername)
{
    QNDEBUG(
        "widget:note_filters",
        "NoteFiltersManager::onRemovedNotebookFromFilter: local uid = "
            << notebookLocalUid << ", name = " << notebookName
            << ", linked notebook guid = " << linkedNotebookGuid
            << ", linked notebook username = " << linkedNotebookUsername);

    onNotebooksFilterUpdated();
}

void NoteFiltersManager::onNotebooksClearedFromFilter()
{
    QNDEBUG(
        "widget:note_filters",
        "NoteFiltersManager::onNotebooksClearedFromFilter");

    onNotebooksFilterUpdated();
}

void NoteFiltersManager::onNotebooksFilterUpdated()
{
    QNDEBUG(
        "widget:note_filters", "NoteFiltersManager::onNotebooksFilterUpdated");

    if (!m_isReady) {
        QNDEBUG(
            "widget:note_filters",
            "Not yet ready to process filter "
                << "updates");
        return;
    }

    if (!m_filterByNotebookWidget.isEnabled()) {
        QNDEBUG(
            "widget:note_filters",
            "Filter by notebook widget is not "
                << "enabled which means that filtering by notebooks is "
                   "overridden "
                << "by either search string or filter by saved search");
        return;
    }

    setFilterByNotebooks();
    Q_EMIT filterChanged();
}

void NoteFiltersManager::onNotebooksFilterReady()
{
    QNDEBUG(
        "widget:note_filters", "NoteFiltersManager::onNotebooksFilterReady");

    if (m_autoFilterNotebookWhenReady) {
        m_autoFilterNotebookWhenReady = false;
        Q_UNUSED(setAutomaticFilterByNotebook())
    }

    checkFiltersReadiness();
}

void NoteFiltersManager::onSavedSearchFilterChanged(
    const QString & savedSearchName)
{
    QNDEBUG(
        "widget:note_filters",
        "NoteFiltersManager::onSavedSearchFilterChanged: " << savedSearchName);

    if (!m_isReady) {
        QNDEBUG(
            "widget:note_filters",
            "Not yet ready to process filter "
                << "updates");
        return;
    }

    if (!m_filterBySavedSearchWidget.isEnabled()) {
        QNDEBUG(
            "widget:note_filters",
            "Filter by saved search widget is not "
                << "enabled which means that filtering by saved search "
                << "is overridden by search string");
        return;
    }

    if (m_pNoteModel.isNull()) {
        QNDEBUG("widget:note_filters", "Note model is null");
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
    QNDEBUG(
        "widget:note_filters", "NoteFiltersManager::onSavedSearchFilterReady");

    if (!m_isReady && setFilterBySavedSearch()) {
        // If we were not ready, there was no valid search string; if we managed
        // to set the filter by saved search, we are good to go because filter
        // by saved search overrides filters by notebooks and tags anyway
        QNDEBUG(
            "widget:note_filters",
            "Was able to set the filter by saved "
                << "search, considering NoteFiltersManager ready");

        m_isReady = true;
        Q_EMIT ready();
        return;
    }

    checkFiltersReadiness();
}

void NoteFiltersManager::onSearchStringEdited(const QString & text)
{
    QNDEBUG(
        "widget:note_filters",
        "NoteFiltersManager::onSearchStringEdited: " << text);

    bool wasEmpty = m_lastSearchString.isEmpty();
    m_lastSearchString = text;
    if (!wasEmpty && m_lastSearchString.isEmpty()) {
        persistSearchString();
        evaluate();
    }
}

void NoteFiltersManager::onSearchStringChanged()
{
    QNDEBUG("widget:note_filters", "NoteFiltersManager::onSearchStringChanged");

    if (m_lastSearchString.isEmpty() && m_searchLineEdit.text().isEmpty()) {
        QNDEBUG(
            "widget:note_filters",
            "Skipping the evaluation as the search "
                << "string is empty => evaluation should have already "
                   "occurred");
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

    bool isRequestForSavedSearch = !isRequestForSearchString &&
        (requestId == m_findNoteLocalUidsForSavedSearchQueryRequestId);

    if (!isRequestForSearchString && !isRequestForSavedSearch) {
        return;
    }

    QNDEBUG(
        "widget:note_filters",
        "NoteFiltersManager::onFindNoteLocalUidsWithSearchQueryCompleted: "
            << "note search query: " << noteSearchQuery
            << "\nRequest id = " << requestId);

    QNTRACE(
        "widget:note_filters",
        "Note local uids: " << noteLocalUids.join(QStringLiteral(", ")));

    if (Q_UNLIKELY(
            !isRequestForSearchString &&
            !m_filterBySavedSearchWidget.isEnabled()))
    {
        QNDEBUG(
            "widget:note_filters",
            "Ignoring the update with note local "
                << "uids for saved search because the filter by saved search "
                << "widget is disabled which means filtering by saved "
                << "search is overridden by filtering via search string");
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

    bool isRequestForSavedSearch = !isRequestForSearchString &&
        (requestId == m_findNoteLocalUidsForSavedSearchQueryRequestId);

    if (!isRequestForSearchString && !isRequestForSavedSearch) {
        return;
    }

    QNWARNING(
        "widget:note_filters",
        "NoteFiltersManager::onFindNoteLocalUidsWithSearchQueryFailed: "
            << "request id = " << requestId << ", note search query = "
            << noteSearchQuery << "\nError description: " << errorDescription);

    if (isRequestForSearchString) {
        ErrorString error(
            QT_TR_NOOP("Can't set the search string to note filter"));

        error.appendBase(errorDescription.base());
        error.appendBase(errorDescription.additionalBases());
        error.details() = errorDescription.details();
        QNDEBUG("widget:note_filters", error);
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

        ErrorString error(
            QT_TR_NOOP("Can't set the saved search to note filter"));

        error.appendBase(errorDescription.base());
        error.appendBase(errorDescription.additionalBases());
        error.details() = errorDescription.details();
        QNDEBUG("widget:note_filters", error);
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

    QNDEBUG(
        "widget:note_filters",
        "NoteFiltersManager::onAddNoteComplete: "
            << "request id = " << requestId);

    QNTRACE("widget:note_filters", note);

    checkAndRefreshNotesSearchQuery();
}

void NoteFiltersManager::onUpdateNoteComplete(
    Note note, LocalStorageManager::UpdateNoteOptions options, QUuid requestId)
{
    if (m_pNoteModel.isNull()) {
        return;
    }

    QNDEBUG(
        "widget:note_filters",
        "NoteFiltersManager::onUpdateNoteComplete: "
            << "request id = " << requestId);

    QNTRACE("widget:note_filters", note);

    Q_UNUSED(options);

    checkAndRefreshNotesSearchQuery();
}

void NoteFiltersManager::onExpungeNotebookComplete(
    Notebook notebook, QUuid requestId)
{
    QNDEBUG(
        "widget:note_filters",
        "NoteFiltersManager::onExpungeNotebookComplete: notebook = "
            << notebook << ", request id = " << requestId);

    if (!m_filterByNotebookWidget.isEnabled()) {
        QNDEBUG(
            "widget:note_filters",
            "Filter by notebook is overridden by "
                << "either search string or saved search filter");
        return;
    }

    if (m_pNoteModel.isNull()) {
        QNDEBUG("widget:note_filters", "Note model is null");
        return;
    }

    auto notebookLocalUids = m_pNoteModel->filteredNotebookLocalUids();
    int index = notebookLocalUids.indexOf(notebook.localUid());
    if (index < 0) {
        QNDEBUG(
            "widget:note_filters",
            "The expunged notebook was not used "
                << "within the filter");
        return;
    }

    QNDEBUG(
        "widget:note_filters",
        "The expunged notebook was used within "
            << "the filter");
    notebookLocalUids.removeAt(index);

    m_pNoteModel->setFilteredNotebookLocalUids(notebookLocalUids);
}

void NoteFiltersManager::onExpungeTagComplete(
    Tag tag, QStringList expungedChildTagLocalUids, QUuid requestId)
{
    QNDEBUG(
        "widget:note_filters",
        "NoteFiltersManager::onExpungeTagComplete: "
            << "tag = " << tag << "\nExpunged child tag local uids: "
            << expungedChildTagLocalUids.join(QStringLiteral(", "))
            << ", request id = " << requestId);

    if (!m_filterByTagWidget.isEnabled()) {
        QNDEBUG(
            "widget:note_filters",
            "The filter by tags is overridden by "
                << "either search string or filter by saved search");
        return;
    }

    if (m_pNoteModel.isNull()) {
        QNDEBUG("widget:note_filters", "Note model is null");
        return;
    }

    QStringList expungedTagLocalUids;
    expungedTagLocalUids << tag.localUid();
    expungedTagLocalUids << expungedChildTagLocalUids;

    QStringList tagLocalUids = m_pNoteModel->filteredTagLocalUids();

    bool filteredTagsChanged = false;
    for (const auto & expungedTagLocalUid: qAsConst(expungedTagLocalUids)) {
        int tagIndex = tagLocalUids.indexOf(expungedTagLocalUid);
        if (tagIndex >= 0) {
            tagLocalUids.removeAt(tagIndex);
            filteredTagsChanged = true;
        }
    }

    if (!filteredTagsChanged) {
        QNDEBUG(
            "widget:note_filters",
            "None of expunged tags seem to appear "
                << "within the list of filtered tags");
        return;
    }

    m_pNoteModel->setFilteredTagLocalUids(tagLocalUids);
}

void NoteFiltersManager::onUpdateSavedSearchComplete(
    SavedSearch search, QUuid requestId)
{
    QNDEBUG(
        "widget:note_filters",
        "NoteFiltersManager::onUpdateSavedSearchComplete: search = "
            << search << "\nRequest id = " << requestId);

    QString currentSavedSearchName = m_filterBySavedSearchWidget.currentText();
    if (currentSavedSearchName.isEmpty()) {
        QNDEBUG(
            "widget:note_filters",
            "No saved search name is set to "
                << "the filter");
        return;
    }

    if (m_filteredSavedSearchLocalUid == search.localUid()) {
        QNDEBUG(
            "widget:note_filters",
            "The saved search within the filter was "
                << "updated");

        QString searchString = m_searchLineEdit.text();
        if (!searchString.isEmpty() && !m_filterBySavedSearchWidget.isEnabled())
        {
            QNDEBUG(
                "widget:note_filters",
                "The search string filter overrides "
                    << "the filter by saved search");
            return;
        }

        if (Q_UNLIKELY(!search.hasName() || !search.hasQuery())) {
            QNDEBUG(
                "widget:note_filters",
                "The updated search lacks either "
                    << "name or query, will remove it from the filter");
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
    QNDEBUG(
        "widget:note_filters",
        "NoteFiltersManager::onExpungeSavedSearchComplete: search = "
            << search << "\nRequest id = " << requestId);

    QString currentSavedSearchName = m_filterBySavedSearchWidget.currentText();
    if (currentSavedSearchName.isEmpty()) {
        QNDEBUG(
            "widget:note_filters",
            "No saved search name is set to "
                << "the filter");
        return;
    }

    if (m_filteredSavedSearchLocalUid == search.localUid()) {
        QNDEBUG(
            "widget:note_filters",
            "The saved search within the filter was "
                << "expunged");

        QString searchString = m_searchLineEdit.text();
        if (!searchString.isEmpty() && !m_filterBySavedSearchWidget.isEnabled())
        {
            QNDEBUG(
                "widget:note_filters",
                "The search string filter overrides "
                    << "the filter by saved search");
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
    QNDEBUG("widget:note_filters", "NoteFiltersManager::createConnections");

    QObject::connect(
        &m_filterByTagWidget, &FilterByTagWidget::addedItemToFilter, this,
        &NoteFiltersManager::onAddedTagToFilter, Qt::UniqueConnection);

    QObject::connect(
        &m_filterByTagWidget, &FilterByTagWidget::itemRemovedFromFilter, this,
        &NoteFiltersManager::onRemovedTagFromFilter, Qt::UniqueConnection);

    QObject::connect(
        &m_filterByTagWidget, &FilterByTagWidget::cleared, this,
        &NoteFiltersManager::onTagsClearedFromFilter, Qt::UniqueConnection);

    QObject::connect(
        &m_filterByTagWidget, &FilterByTagWidget::updated, this,
        &NoteFiltersManager::onTagsFilterUpdated, Qt::UniqueConnection);

    QObject::connect(
        &m_filterByTagWidget, &FilterByTagWidget::ready, this,
        &NoteFiltersManager::onTagsFilterReady, Qt::UniqueConnection);

    QObject::connect(
        &m_filterByNotebookWidget, &FilterByNotebookWidget::addedItemToFilter,
        this, &NoteFiltersManager::onAddedNotebookToFilter,
        Qt::UniqueConnection);

    QObject::connect(
        &m_filterByNotebookWidget,
        &FilterByNotebookWidget::itemRemovedFromFilter, this,
        &NoteFiltersManager::onRemovedNotebookFromFilter, Qt::UniqueConnection);

    QObject::connect(
        &m_filterByNotebookWidget, &FilterByNotebookWidget::cleared, this,
        &NoteFiltersManager::onNotebooksClearedFromFilter,
        Qt::UniqueConnection);

    QObject::connect(
        &m_filterByNotebookWidget, &FilterByNotebookWidget::updated, this,
        &NoteFiltersManager::onNotebooksFilterUpdated, Qt::UniqueConnection);

    QObject::connect(
        &m_filterByNotebookWidget, &FilterByNotebookWidget::ready, this,
        &NoteFiltersManager::onNotebooksFilterReady, Qt::UniqueConnection);

    QObject::connect(
        &m_filterBySavedSearchWidget,
        &FilterBySavedSearchWidget::currentSavedSearchNameChanged, this,
        &NoteFiltersManager::onSavedSearchFilterChanged, Qt::UniqueConnection);

    QObject::connect(
        &m_filterBySavedSearchWidget, &FilterBySavedSearchWidget::ready, this,
        &NoteFiltersManager::onSavedSearchFilterReady, Qt::UniqueConnection);

    QObject::connect(
        &m_searchLineEdit, &QLineEdit::textEdited, this,
        &NoteFiltersManager::onSearchStringEdited, Qt::UniqueConnection);

    QObject::connect(
        &m_searchLineEdit, &QLineEdit::editingFinished, this,
        &NoteFiltersManager::onSearchStringChanged, Qt::UniqueConnection);

    QObject::connect(
        this, &NoteFiltersManager::findNoteLocalUidsForNoteSearchQuery,
        &m_localStorageManagerAsync,
        &LocalStorageManagerAsync::onFindNoteLocalUidsWithSearchQuery,
        Qt::UniqueConnection);

    QObject::connect(
        &m_localStorageManagerAsync,
        &LocalStorageManagerAsync::findNoteLocalUidsWithSearchQueryComplete,
        this, &NoteFiltersManager::onFindNoteLocalUidsWithSearchQueryCompleted,
        Qt::UniqueConnection);

    QObject::connect(
        &m_localStorageManagerAsync,
        &LocalStorageManagerAsync::findNoteLocalUidsWithSearchQueryFailed, this,
        &NoteFiltersManager::onFindNoteLocalUidsWithSearchQueryFailed,
        Qt::UniqueConnection);

    QObject::connect(
        &m_localStorageManagerAsync,
        &LocalStorageManagerAsync::expungeNotebookComplete, this,
        &NoteFiltersManager::onExpungeNotebookComplete, Qt::UniqueConnection);

    QObject::connect(
        &m_localStorageManagerAsync,
        &LocalStorageManagerAsync::expungeTagComplete, this,
        &NoteFiltersManager::onExpungeTagComplete, Qt::UniqueConnection);

    QObject::connect(
        &m_localStorageManagerAsync,
        &LocalStorageManagerAsync::updateSavedSearchComplete, this,
        &NoteFiltersManager::onUpdateSavedSearchComplete, Qt::UniqueConnection);

    QObject::connect(
        &m_localStorageManagerAsync,
        &LocalStorageManagerAsync::expungeSavedSearchComplete, this,
        &NoteFiltersManager::onExpungeSavedSearchComplete,
        Qt::UniqueConnection);

    QObject::connect(
        &m_localStorageManagerAsync, &LocalStorageManagerAsync::addNoteComplete,
        this, &NoteFiltersManager::onAddNoteComplete, Qt::UniqueConnection);

    QObject::connect(
        &m_localStorageManagerAsync,
        &LocalStorageManagerAsync::updateNoteComplete, this,
        &NoteFiltersManager::onUpdateNoteComplete, Qt::UniqueConnection);
}

void NoteFiltersManager::evaluate()
{
    QNDEBUG("widget:note_filters", "NoteFiltersManager::evaluate");

    if (m_pNoteModel.isNull()) {
        return;
    }

    // NOTE: the rules for note filter evaluation are the following:
    // 1) If the search string is not empty *and* is indeed a valid search
    //    string, if overrides the filters by notebooks, tags and saved search.
    //    So all three filter widgets are disabled if the search string is not
    //    empty and is valid.
    // 2) If the search string is empty or invalid and there's some selected
    //    saved search, filtering by saved search overrides the filters by
    //    notebooks and tags, so the filter widgets for notebooks and tags get
    //    disabled.
    // 3) If the search string is empty or invalid and the filter by saved
    //    search doesn't contain any selected saved search, the filters by
    //    notebooks and tags are effective and the corresponidng widgets are
    //    enabled. The filtering by notebooks and tags is cumulative i.e. only
    //    those notes would be accepted which both belong to one of
    //    the specified notebooks and have one of the specified tags. In other
    //    words, it's like search string containing notebook and tags but not
    //    containing "any" statement.

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
    QNDEBUG("widget:note_filters", "NoteFiltersManager::persistSearchString");

    ApplicationSettings appSettings(m_account, QUENTIER_UI_SETTINGS);
    appSettings.beginGroup(NOTE_FILTERS_GROUP_KEY);
    appSettings.setValue(NOTE_SEARCH_STRING_KEY, m_searchLineEdit.text());
    appSettings.endGroup();

    // Remove old group where this preference used to reside
    appSettings.remove(NOTE_SEARCH_STRING_GROUP_KEY);
}

void NoteFiltersManager::restoreSearchString()
{
    QNDEBUG("widget:note_filters", "NoteFiltersManager::restoreSearchString");

    ApplicationSettings appSettings(m_account, QUENTIER_UI_SETTINGS);

    appSettings.beginGroup(NOTE_FILTERS_GROUP_KEY);
    auto lastSearchStringValue = appSettings.value(NOTE_SEARCH_STRING_KEY);
    appSettings.endGroup();

    // Backward compatibility: look for preference in old location as a fallback
    if (!lastSearchStringValue.isValid()) {
        appSettings.beginGroup(NOTE_SEARCH_STRING_GROUP_KEY);
        lastSearchStringValue = appSettings.value(NOTE_SEARCH_STRING_KEY);
        appSettings.endGroup();
    }

    m_lastSearchString = lastSearchStringValue.toString();
    m_searchLineEdit.setText(m_lastSearchString);

    if (m_lastSearchString.isEmpty()) {
        return;
    }

    ErrorString error;
    auto query = createNoteSearchQuery(m_lastSearchString, error);
    if (query.isEmpty()) {
        QToolTip::showText(
            m_searchLineEdit.mapToGlobal(QPoint(0, m_searchLineEdit.height())),
            error.localizedString(), &m_searchLineEdit);
    }
}

bool NoteFiltersManager::setFilterBySearchString()
{
    QNDEBUG(
        "widget:note_filters", "NoteFiltersManager::setFilterBySearchString");

    QString searchString = m_searchLineEdit.text();
    if (searchString.isEmpty()) {
        QNDEBUG("widget:note_filters", "The search string is empty");
        return false;
    }

    ErrorString error;
    auto query = createNoteSearchQuery(searchString, error);
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

    QNTRACE(
        "widget:note_filters",
        "Emitting the request to find note local "
            << "uids corresponding to the note search query: request id = "
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
    QNDEBUG(
        "widget:note_filters", "NoteFiltersManager::setFilterBySavedSearch");

    if (m_pNoteModel.isNull()) {
        QNDEBUG("widget:note_filters", "Note model is null");
        return false;
    }

    // If this method got called in the first place, it means the search string
    // doesn't override the filter by saved search so should enable the filter
    // by saved search widget
    m_filterBySavedSearchWidget.setEnabled(true);

    QString currentSavedSearchName = m_filterBySavedSearchWidget.currentText();
    if (currentSavedSearchName.isEmpty()) {
        QNDEBUG(
            "widget:note_filters",
            "No saved search name is set to "
                << "the filter");
        m_pNoteModel->clearFilteredNoteLocalUids();
        return false;
    }

    const auto * pSavedSearchModel =
        m_filterBySavedSearchWidget.savedSearchModel();

    if (Q_UNLIKELY(!pSavedSearchModel)) {
        QNDEBUG(
            "widget:note_filters",
            "Saved search model in the filter by "
                << "saved search widget is null");
        m_pNoteModel->clearFilteredNoteLocalUids();
        return false;
    }

    auto itemIndex =
        pSavedSearchModel->indexForSavedSearchName(currentSavedSearchName);

    if (Q_UNLIKELY(!itemIndex.isValid())) {
        ErrorString error(
            QT_TR_NOOP("Internal error: can't set the filter by saved search, "
                       "the saved search model returned invalid model index "
                       "for the given saved search name"));

        QNWARNING("widget:note_filters", error);
        Q_EMIT notifyError(error);
        m_pNoteModel->clearFilteredNoteLocalUids();
        return false;
    }

    const auto * pItem = pSavedSearchModel->itemForIndex(itemIndex);
    if (Q_UNLIKELY(!pItem)) {
        ErrorString error(
            QT_TR_NOOP("Internal error: can't set the filter by saved search, "
                       "the saved search model returned null item for "
                       "the given valid model index"));

        QNWARNING("widget:note_filters", error);
        Q_EMIT notifyError(error);
        m_pNoteModel->clearFilteredNoteLocalUids();
        return false;
    }

    if (Q_UNLIKELY(pItem->m_query.isEmpty())) {
        ErrorString error(
            QT_TR_NOOP("Can't set the filter by saved search: "
                       "saved search's query is empty"));

        QNWARNING(
            "widget:note_filters", error << ", saved search item: " << *pItem);

        Q_EMIT notifyError(error);
        m_pNoteModel->clearFilteredNoteLocalUids();
        return false;
    }

    NoteSearchQuery query;
    ErrorString errorDescription;
    bool res = query.setQueryString(pItem->m_query, errorDescription);
    if (Q_UNLIKELY(!res)) {
        ErrorString error(
            QT_TR_NOOP("Internal error: can't set the filter by saved search: "
                       "failed to parse the saved search query"));

        error.appendBase(errorDescription.base());
        error.appendBase(errorDescription.additionalBases());
        error.details() = errorDescription.details();

        QNWARNING(
            "widget:note_filters", error << ", saved search item: " << *pItem);

        Q_EMIT notifyError(error);
        m_pNoteModel->clearFilteredNoteLocalUids();
        return false;
    }

    m_filteredSavedSearchLocalUid = pItem->m_localUid;
    m_findNoteLocalUidsForSavedSearchQueryRequestId = QUuid::createUuid();

    QNTRACE(
        "widget:note_filters",
        "Emitting the request to find note local "
            << "uids corresponding to the saved search: request id = "
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
    QNDEBUG("widget:note_filters", "NoteFiltersManager::setFilterByNotebooks");

    if (m_pNoteModel.isNull()) {
        QNDEBUG("widget:note_filters", "Note model is null");
        return;
    }

    // If this method got called in the first place, it means the notebook
    // filter is not overridden by either search string or saved search filters
    // so should enable the filter by notebook widget
    m_filterByNotebookWidget.setEnabled(true);

    auto notebookLocalUids =
        m_filterByNotebookWidget.localUidsOfItemsInFilter();

    QNTRACE(
        "widget:note_filters",
        "Notebook local uids to be used for "
            << "filtering: "
            << (notebookLocalUids.isEmpty()
                    ? QStringLiteral("<empty>")
                    : notebookLocalUids.join(QStringLiteral(", "))));

    m_pNoteModel->setFilteredNotebookLocalUids(notebookLocalUids);
}

void NoteFiltersManager::setFilterByTags()
{
    QNDEBUG("widget:note_filters", "NoteFiltersManager::setFilterByTags");

    if (m_pNoteModel.isNull()) {
        QNDEBUG("widget:note_filters", "Note model is null");
        return;
    }

    // If this method got called in the first place, it means the tag filter is
    // not overridden by either search string or saved search filter so should
    // enable the filter by tag widget
    m_filterByTagWidget.setEnabled(true);

    auto tagLocalUids = m_filterByTagWidget.localUidsOfItemsInFilter();

    QNTRACE(
        "widget:note_filters",
        "Tag local uids to be used for filtering: "
            << tagLocalUids.join(QStringLiteral(", ")));

    m_pNoteModel->setFilteredTagLocalUids(tagLocalUids);
}

void NoteFiltersManager::clearFilterWidgetsItems()
{
    QNDEBUG(
        "widget:note_filters", "NoteFiltersManager::clearFilterWidgetsItems");

    clearFilterByTagWidgetItems();
    clearFilterByNotebookWidgetItems();
    clearFilterBySavedSearchWidget();
    clearSearchString();
}

void NoteFiltersManager::clearFilterByTagWidgetItems()
{
    QNDEBUG(
        "widget:note_filters",
        "NoteFiltersManager::clearFilterByTagWidgetItems");

    QObject::disconnect(
        &m_filterByTagWidget, &FilterByTagWidget::cleared, this,
        &NoteFiltersManager::onTagsClearedFromFilter);

    m_filterByTagWidget.clear();

    QObject::connect(
        &m_filterByTagWidget, &FilterByTagWidget::addedItemToFilter, this,
        &NoteFiltersManager::onAddedTagToFilter, Qt::UniqueConnection);
}

void NoteFiltersManager::clearFilterByNotebookWidgetItems()
{
    QNDEBUG(
        "widget:note_filters",
        "NoteFiltersManager::clearFilterByNotebookWidgetItems");

    QObject::disconnect(
        &m_filterByNotebookWidget, &FilterByNotebookWidget::addedItemToFilter,
        this, &NoteFiltersManager::onAddedNotebookToFilter);

    m_filterByNotebookWidget.clear();

    QObject::connect(
        &m_filterByNotebookWidget, &FilterByNotebookWidget::addedItemToFilter,
        this, &NoteFiltersManager::onAddedNotebookToFilter,
        Qt::UniqueConnection);
}

void NoteFiltersManager::clearFilterBySavedSearchWidget()
{
    QNDEBUG(
        "widget:note_filters",
        "NoteFiltersManager::clearFilterBySavedSearchWidget");

    QObject::disconnect(
        &m_filterBySavedSearchWidget,
        &FilterBySavedSearchWidget::currentSavedSearchNameChanged, this,
        &NoteFiltersManager::onSavedSearchFilterChanged);

    m_filterBySavedSearchWidget.setCurrentIndex(0);

    QObject::connect(
        &m_filterBySavedSearchWidget,
        &FilterBySavedSearchWidget::currentSavedSearchNameChanged, this,
        &NoteFiltersManager::onSavedSearchFilterChanged, Qt::UniqueConnection);
}

void NoteFiltersManager::clearSearchString()
{
    QNDEBUG("widget:note_filters", "NoteFiltersManager::clearSearchString");

    QObject::disconnect(
        &m_searchLineEdit, &QLineEdit::editingFinished, this,
        &NoteFiltersManager::onSearchStringChanged);

    m_searchLineEdit.setText(QString());

    QObject::connect(
        &m_searchLineEdit, &QLineEdit::editingFinished, this,
        &NoteFiltersManager::onSearchStringChanged, Qt::UniqueConnection);

    persistSearchString();
}

void NoteFiltersManager::checkFiltersReadiness()
{
    QNDEBUG("widget:note_filters", "NoteFiltersManager::checkFiltersReadiness");

    if (m_isReady) {
        QNDEBUG(
            "widget:note_filters",
            "Already marked the filter as ready "
                << "once");
        return;
    }

    if (!m_filterByTagWidget.isReady()) {
        QNDEBUG(
            "widget:note_filters",
            "Still pending the readiness of filter "
                << "by tags");
        return;
    }

    if (!m_filterByNotebookWidget.isReady()) {
        QNDEBUG(
            "widget:note_filters",
            "Still pending the readiness of filter "
                << "by notebooks");
        return;
    }

    if (!m_filterBySavedSearchWidget.isReady()) {
        QNDEBUG(
            "widget:note_filters",
            "Still pending the readiness of filter "
                << "by saved search");
        return;
    }

    QNDEBUG("widget:note_filters", "All filters are ready");
    m_isReady = true;
    evaluate();
    Q_EMIT ready();
}

void NoteFiltersManager::setNotebookToFilterImpl(
    const QString & notebookLocalUid)
{
    if (notebookLocalUid.isEmpty()) {
        return;
    }

    const auto * pNotebookModel = m_filterByNotebookWidget.notebookModel();
    if (Q_UNLIKELY(!pNotebookModel)) {
        QNDEBUG(
            "widget:note_filters",
            "Notebook model in the filter by "
                << "notebook widget is null");
        return;
    }

    auto itemInfo = pNotebookModel->itemInfoForLocalUid(notebookLocalUid);
    if (itemInfo.m_localUid.isEmpty()) {
        QNWARNING(
            "widget:note_filters",
            "Failed to find notebook name for "
                << "notebook local uid " << notebookLocalUid);
        return;
    }

    QObject::disconnect(
        &m_filterByNotebookWidget, &FilterByNotebookWidget::addedItemToFilter,
        this, &NoteFiltersManager::onAddedNotebookToFilter);

    persistFilterByNotebookClearedState(false);

    m_filterByNotebookWidget.addItemToFilter(
        notebookLocalUid, itemInfo.m_name, itemInfo.m_linkedNotebookGuid,
        itemInfo.m_linkedNotebookUsername);

    QObject::connect(
        &m_filterByNotebookWidget, &FilterByNotebookWidget::addedItemToFilter,
        this, &NoteFiltersManager::onAddedNotebookToFilter,
        Qt::UniqueConnection);
}

void NoteFiltersManager::setTagsToFilterImpl(const QStringList & tagLocalUids)
{
    if (tagLocalUids.isEmpty()) {
        return;
    }

    const auto * pTagModel = m_filterByTagWidget.tagModel();
    if (Q_UNLIKELY(!pTagModel)) {
        QNDEBUG(
            "widget:note_filters",
            "Tag model in the filter by tag widget is null");
        return;
    }

    QVector<ItemModel::ItemInfo> itemInfos;
    itemInfos.reserve(tagLocalUids.size());

    for(const auto & tagLocalUid: qAsConst(tagLocalUids))
    {
        auto itemInfo = pTagModel->itemInfoForLocalUid(tagLocalUid);
        if (Q_UNLIKELY(itemInfo.m_localUid.isEmpty())) {
            QNWARNING(
                "widget:note_filters",
                "Failed to find info for tag local uid: " << tagLocalUid);
            continue;
        }

        itemInfos << itemInfo;
    }

    QObject::disconnect(
        &m_filterByTagWidget, &FilterByTagWidget::addedItemToFilter, this,
        &NoteFiltersManager::onAddedTagToFilter);

    persistFilterByTagClearedState(false);

    for(const auto & itemInfo: qAsConst(itemInfos)) {
        m_filterByTagWidget.addItemToFilter(
            itemInfo.m_localUid, itemInfo.m_name, itemInfo.m_linkedNotebookGuid,
            itemInfo.m_linkedNotebookUsername);
    }

    QObject::connect(
        &m_filterByTagWidget, &FilterByTagWidget::addedItemToFilter, this,
        &NoteFiltersManager::onAddedTagToFilter, Qt::UniqueConnection);
}

void NoteFiltersManager::checkAndRefreshNotesSearchQuery()
{
    QNDEBUG(
        "widget:note_filters",
        "NoteFiltersManager::checkAndRefreshNotesSearchQuery");

    // Refresh notes filtering if it was done via explicit search query or saved
    // search
    if (!m_filterByTagWidget.isEnabled() &&
        !m_filterByNotebookWidget.isEnabled() &&
        (!m_searchLineEdit.text().isEmpty() ||
         m_filterBySavedSearchWidget.isEnabled()))
    {
        evaluate();
    }
}

bool NoteFiltersManager::setAutomaticFilterByNotebook()
{
    QNDEBUG(
        "widget:note_filters",
        "NoteFiltersManager::setAutomaticFilterByNotebook");

    if (notebookFilterWasCleared()) {
        QNDEBUG(
            "widget:note_filters",
            "Notebook filter was cleared, won't set "
                << "automatic filter by notebook");
        return true;
    }

    const auto * pModel = m_filterByNotebookWidget.notebookModel();
    if (!pModel) {
        QNDEBUG(
            "widget:note_filters",
            "Notebook model is not set to filter by "
                << "notebook widget yet");
        return false;
    }

    if (!pModel->allNotebooksListed()) {
        QNDEBUG(
            "widget:note_filters",
            "Not all notebooks are listed yet by "
                << "the notebook model");
        return false;
    }

    QString autoSelectedNotebookLocalUid;
    auto lastUsedNotebookIndex = pModel->lastUsedNotebookIndex();

    const auto * pLastUsedNotebookModelItem =
        pModel->itemForIndex(lastUsedNotebookIndex);

    if (pLastUsedNotebookModelItem) {
        const auto * pLastUsedNotebookItem =
            pLastUsedNotebookModelItem->cast<NotebookItem>();
        if (pLastUsedNotebookItem) {
            autoSelectedNotebookLocalUid = pLastUsedNotebookItem->localUid();
        }
    }

    if (autoSelectedNotebookLocalUid.isEmpty()) {
        QNDEBUG(
            "widget:note_filters",
            "No last used notebook local uid, "
                << "trying default notebook");

        auto defaultNotebookIndex = pModel->defaultNotebookIndex();

        const auto * pDefaultNotebookModelItem =
            pModel->itemForIndex(defaultNotebookIndex);

        if (pDefaultNotebookModelItem) {
            const auto * pDefaultNotebookItem =
                pDefaultNotebookModelItem->cast<NotebookItem>();

            if (pDefaultNotebookItem) {
                autoSelectedNotebookLocalUid = pDefaultNotebookItem->localUid();
            }
        }
    }

    if (autoSelectedNotebookLocalUid.isEmpty()) {
        QNDEBUG(
            "widget:note_filters",
            "No default notebook local uid, trying "
                << "just any notebook");

        QStringList notebookNames = pModel->itemNames(QString());
        if (Q_UNLIKELY(notebookNames.isEmpty())) {
            QNDEBUG(
                "widget:note_filters",
                "No notebooks within the notebook "
                    << "model");
            // NOTE: returning true because false is only for cases
            // in which filter is waiting for something
            return true;
        }

        const QString & firstNotebookName = notebookNames.at(0);

        autoSelectedNotebookLocalUid =
            pModel->localUidForItemName(firstNotebookName, {});
    }

    if (Q_UNLIKELY(autoSelectedNotebookLocalUid.isEmpty())) {
        QNDEBUG(
            "widget:note_filters",
            "Failed to find any notebook for "
                << "automatic selection");
        // NOTE: returning true because false is only for cases
        // in which filter is waiting for something
        return true;
    }

    auto itemInfo = pModel->itemInfoForLocalUid(autoSelectedNotebookLocalUid);
    if (Q_UNLIKELY(itemInfo.m_localUid.isEmpty())) {
        QNWARNING(
            "widget:note_filters",
            "Failed fo find notebook item for "
                << "auto selected local uid " << autoSelectedNotebookLocalUid);
        // NOTE: returning true because false is only for cases
        // in which filter is waiting for something
        return true;
    }

    QNDEBUG(
        "widget:note_filters",
        "Auto selecting notebook: local uid = "
            << autoSelectedNotebookLocalUid << ", name: " << itemInfo.m_name);

    QObject::disconnect(
        &m_filterByNotebookWidget, &FilterByNotebookWidget::addedItemToFilter,
        this, &NoteFiltersManager::onAddedNotebookToFilter);

    m_filterByNotebookWidget.addItemToFilter(
        autoSelectedNotebookLocalUid, itemInfo.m_name,
        itemInfo.m_linkedNotebookGuid, itemInfo.m_linkedNotebookUsername);

    QObject::connect(
        &m_filterByNotebookWidget, &FilterByNotebookWidget::addedItemToFilter,
        this, &NoteFiltersManager::onAddedNotebookToFilter,
        Qt::UniqueConnection);

    return true;
}

void NoteFiltersManager::persistFilterByNotebookClearedState(const bool state)
{
    QNDEBUG(
        "widget:note_filters",
        "NoteFiltersManager::persistFilterByNotebookClearedState: "
            << (state ? "true" : "false"));

    ApplicationSettings appSettings(m_account, QUENTIER_UI_SETTINGS);

    appSettings.beginGroup(NOTE_FILTERS_GROUP_KEY);
    appSettings.setValue(NOTEBOOK_FILTER_CLEARED, state);
    appSettings.endGroup();
}

bool NoteFiltersManager::notebookFilterWasCleared() const
{
    ApplicationSettings appSettings(m_account, QUENTIER_UI_SETTINGS);

    appSettings.beginGroup(NOTE_FILTERS_GROUP_KEY);
    auto notebookFilterWasCleared = appSettings.value(NOTEBOOK_FILTER_CLEARED);
    appSettings.endGroup();

    if (!notebookFilterWasCleared.isValid()) {
        return false;
    }

    return notebookFilterWasCleared.toBool();
}

void NoteFiltersManager::persistFilterByTagClearedState(const bool state)
{
    QNDEBUG(
        "widget:note_filters",
        "NoteFiltersManager::persistFilterByTagClearedState: "
            << (state ? "true" : "false"));

    ApplicationSettings appSettings(m_account, QUENTIER_UI_SETTINGS);

    appSettings.beginGroup(NOTE_FILTERS_GROUP_KEY);
    appSettings.setValue(TAG_FILTER_CLEARED, state);
    appSettings.endGroup();
}

bool NoteFiltersManager::tagFilterWasCleared() const
{
    ApplicationSettings appSettings(m_account, QUENTIER_UI_SETTINGS);

    appSettings.beginGroup(NOTE_FILTERS_GROUP_KEY);
    auto tagFilterWasCleared = appSettings.value(TAG_FILTER_CLEARED);
    appSettings.endGroup();

    if (!tagFilterWasCleared.isValid()) {
        return false;
    }

    return tagFilterWasCleared.toBool();
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
        QNDEBUG(
            "widget:note_filters",
            "The search string is invalid: error: "
                << errorDescription << ", search string: " << searchString);
        query.clear();
    }

    return query;
}

} // namespace quentier
