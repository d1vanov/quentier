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

#include "NoteFiltersManager.h"

#include "FilterByNotebookWidget.h"
#include "FilterBySavedSearchWidget.h"
#include "FilterBySearchStringWidget.h"
#include "FilterByTagWidget.h"

#include <lib/dialog/AddOrEditSavedSearchDialog.h>
#include <lib/model/note/NoteModel.h>
#include <lib/model/notebook/NotebookModel.h>
#include <lib/model/saved_search/SavedSearchModel.h>
#include <lib/model/tag/TagModel.h>
#include <lib/preferences/keys/Files.h>

#include <quentier/logging/QuentierLogger.h>
#include <quentier/utility/ApplicationSettings.h>

#include <QComboBox>
#include <QLineEdit>
#include <QToolTip>

#include <memory>

namespace quentier {

// DEPRECATED: NOTE_FILTERS_GROUP_KEY should be used instead
#define NOTE_SEARCH_STRING_GROUP_KEY QStringLiteral("NoteSearchStringFilter")

#define NOTE_FILTERS_GROUP_KEY      QStringLiteral("NoteFilters")
#define NOTE_SEARCH_QUERY_KEY       QStringLiteral("SearchString")
#define NOTEBOOK_FILTER_CLEARED     QStringLiteral("NotebookFilterCleared")
#define TAG_FILTER_CLEARED          QStringLiteral("TagFilterCleared")
#define SAVED_SEARCH_FILTER_CLEARED QStringLiteral("SavedSearchFilterCleared")

NoteFiltersManager::NoteFiltersManager(
    Account account, FilterByTagWidget & filterByTagWidget,
    FilterByNotebookWidget & filterByNotebookWidget, NoteModel & noteModel,
    FilterBySavedSearchWidget & filterBySavedSearchWidget,
    FilterBySearchStringWidget & FilterBySearchStringWidget,
    LocalStorageManagerAsync & localStorageManagerAsync, QObject * parent) :
    QObject(parent),
    m_account(std::move(account)), m_filterByTagWidget(filterByTagWidget),
    m_filterByNotebookWidget(filterByNotebookWidget), m_pNoteModel(&noteModel),
    m_filterBySavedSearchWidget(filterBySavedSearchWidget),
    m_filterBySearchStringWidget(FilterBySearchStringWidget),
    m_localStorageManagerAsync(localStorageManagerAsync)
{
    createConnections();

    QString savedSearchLocalId =
        m_filterBySavedSearchWidget.filteredSavedSearchLocalId();

    if (!savedSearchLocalId.isEmpty()) {
        // As filtered saved search's local id is not empty, need to delay
        // the moment of filtering evaluatuon until filter by saved search is
        // ready
        QNDEBUG(
            "widget:note_filters",
            "Filtered saved search's local id is "
                << "not empty, need to properly wait for filter's readiness");

        checkFiltersReadiness();
        return;
    }

    restoreSearchQuery();
    if (setFilterBySearchString()) {
        Q_EMIT filterChanged();

        QNDEBUG(
            "widget:note_filters",
            "Was able to set the filter by search string, considering "
                << "NoteFiltersManager ready");

        m_isReady = true;
        Q_EMIT ready();
        return;
    }

    // If we got here, there are no filters by saved search or search string.
    // We can set filters by notebooks and tags without waiting for them to be
    // complete since local ids of notebooks and tags within the filter are
    // known even before the filter widgets become ready
    noteModel.beginUpdateFilter();
    setFilterByNotebooks();
    setFilterByTags();
    noteModel.endUpdateFilter();

    if (noteModel.filteredNotebookLocalIds().isEmpty() &&
        noteModel.filteredTagLocalIds().isEmpty())
    {
        if (!setAutomaticFilterByNotebook()) {
            QNDEBUG(
                "widget:note_filters",
                "Will wait for notebook model's readiness");
            m_autoFilterNotebookWhenReady = true;
            return;
        }
    }

    Q_EMIT filterChanged();

    m_isReady = true;
    Q_EMIT ready();
}

NoteFiltersManager::~NoteFiltersManager() = default;

QStringList NoteFiltersManager::notebookLocalIdsInFilter() const
{
    if (!savedSearchLocalIdInFilter().isEmpty()) {
        return {};
    }

    if (isFilterBySearchStringActive()) {
        return {};
    }

    if (Q_UNLIKELY(m_pNoteModel.isNull())) {
        return {};
    }

    return m_pNoteModel->filteredNotebookLocalIds();
}

QStringList NoteFiltersManager::tagLocalIdsInFilter() const
{
    if (!savedSearchLocalIdInFilter().isEmpty()) {
        return {};
    }

    if (isFilterBySearchStringActive()) {
        return {};
    }

    if (Q_UNLIKELY(m_pNoteModel.isNull())) {
        return {};
    }

    return m_pNoteModel->filteredTagLocalIds();
}

const QString & NoteFiltersManager::savedSearchLocalIdInFilter() const noexcept
{
    return m_filteredSavedSearchLocalId;
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

void NoteFiltersManager::setNotebooksToFilter(
    const QStringList & notebookLocalIds)
{
    QNDEBUG(
        "widget:note_filters",
        "NoteFiltersManager::setNotebooksToFilter: "
            << notebookLocalIds.join(QStringLiteral(", ")));

    clearFilterByNotebookWidgetItems();
    setNotebooksToFilterImpl(notebookLocalIds);
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

void NoteFiltersManager::setTagsToFilter(const QStringList & tagLocalIds)
{
    QNDEBUG(
        "widget:note_filters",
        "NoteFiltersManager::setTagsToFilter: "
            << tagLocalIds.join(QStringLiteral(", ")));

    clearFilterByTagWidgetItems();
    setTagsToFilterImpl(tagLocalIds);
    evaluate();
}

void NoteFiltersManager::removeTagsFromFilter()
{
    QNDEBUG("widget:note_filters", "NoteFiltersManager::removeTagsFromFilter");

    persistFilterByTagClearedState(true);
    clearFilterByTagWidgetItems();
    evaluate();
}

void NoteFiltersManager::setSavedSearchLocalIdToFilter(
    const QString & savedSearchLocalId)
{
    QNDEBUG(
        "widget:note_filters",
        "NoteFiltersManager::setSavedSearchLocalIdToFilter: "
            << savedSearchLocalId);

    setSavedSearchToFilterImpl(savedSearchLocalId);
}

void NoteFiltersManager::removeSavedSearchFromFilter()
{
    QNDEBUG(
        "widget:note_filters",
        "NoteFiltersManager::removeSavedSearchFromFilter");

    setSavedSearchToFilterImpl(QString());
}

void NoteFiltersManager::setItemsToFilter(
    const QString & savedSearchLocalId, const QStringList & notebookLocalIds,
    const QStringList & tagLocalIds)
{
    QNDEBUG(
        "widget:note_filters",
        "NoteFiltersManager::setItemsToFilter: saved search local id = "
            << savedSearchLocalId << ", notebook local ids: "
            << notebookLocalIds.join(QStringLiteral(", "))
            << ", tag local ids: " << tagLocalIds.join(QStringLiteral(", ")));

    setSavedSearchToFilterImpl(savedSearchLocalId);

    clearFilterByNotebookWidgetItems();
    setNotebooksToFilterImpl(notebookLocalIds);

    clearFilterByTagWidgetItems();
    setTagsToFilterImpl(tagLocalIds);

    evaluate();
}

bool NoteFiltersManager::isReady() const noexcept
{
    return m_isReady;
}

void NoteFiltersManager::onAddedTagToFilter(
    const QString & tagLocalId, const QString & tagName,
    const QString & linkedNotebookGuid, const QString & linkedNotebookUsername)
{
    QNDEBUG(
        "widget:note_filters",
        "NoteFiltersManager::onAddedTagToFilter: local id = "
            << tagLocalId << ", name = " << tagName
            << ", linked notebook guid = " << linkedNotebookGuid
            << ", linked notebook username = " << linkedNotebookUsername);

    onTagsFilterUpdated();
}

void NoteFiltersManager::onRemovedTagFromFilter(
    const QString & tagLocalId, const QString & tagName,
    const QString & linkedNotebookGuid, const QString & linkedNotebookUsername)
{
    QNDEBUG(
        "widget:note_filters",
        "NoteFiltersManager::onRemovedTagFromFilter: local id = "
            << tagLocalId << ", name = " << tagName
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
            "widget:note_filters", "Not yet ready to process filter updates");
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
    const QString & notebookLocalId, const QString & notebookName,
    const QString & linkedNotebookGuid, const QString & linkedNotebookUsername)
{
    QNDEBUG(
        "widget:note_filters",
        "NoteFiltersManager::onAddedNotebookToFilter: local id = "
            << notebookLocalId << ", name = " << notebookName
            << ", linked notebook guid = " << linkedNotebookGuid
            << ", linked notebook username = " << linkedNotebookUsername);

    onNotebooksFilterUpdated();
}

void NoteFiltersManager::onRemovedNotebookFromFilter(
    const QString & notebookLocalId, const QString & notebookName,
    const QString & linkedNotebookGuid, const QString & linkedNotebookUsername)
{
    QNDEBUG(
        "widget:note_filters",
        "NoteFiltersManager::onRemovedNotebookFromFilter: local id = "
            << notebookLocalId << ", name = " << notebookName
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
            "widget:note_filters", "Not yet ready to process filter updates");
        return;
    }

    if (!m_filterByNotebookWidget.isEnabled()) {
        QNDEBUG(
            "widget:note_filters",
            "Filter by notebook widget is not enabled which means filtering by "
                << "notebooks is overridden by either saved search or search "
                << "string");
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
            "widget:note_filters", "Not yet ready to process filter updates");
        return;
    }

    if (Q_UNLIKELY(m_pNoteModel.isNull())) {
        QNDEBUG("widget:note_filters", "Note model is null");
        return;
    }

    bool res = setFilterBySavedSearch();
    if (res) {
        Q_EMIT filterChanged();
        return;
    }

    // If we got here, the saved search is either empty or invalid
    m_filteredSavedSearchLocalId.clear();

    res = setFilterBySearchString();
    if (res) {
        Q_EMIT filterChanged();
        return;
    }

    m_pNoteModel->beginUpdateFilter();

    setFilterByTags();
    setFilterByNotebooks();

    m_pNoteModel->endUpdateFilter();

    Q_EMIT filterChanged();
}

void NoteFiltersManager::onSavedSearchFilterReady()
{
    QNDEBUG(
        "widget:note_filters", "NoteFiltersManager::onSavedSearchFilterReady");

    if (!m_isReady && setFilterBySavedSearch()) {
        QNDEBUG(
            "widget:note_filters",
            "Was able to set the filter by saved search, considering "
                << "NoteFiltersManager ready");

        m_isReady = true;
        Q_EMIT ready();
        return;
    }

    checkFiltersReadiness();
}

void NoteFiltersManager::onSearchQueryChanged(QString query) // NOLINT
{
    QNDEBUG(
        "widget:note_filters",
        "NoteFiltersManager::onSearchQueryChanged: " << query);

    persistSearchQuery(query);
    evaluate();
}

void NoteFiltersManager::onSavedSearchQueryChanged(
    QString savedSearchLocalId, QString query) // NOLINT
{
    QNDEBUG(
        "widget:note_filters",
        "NoteFiltersManager::onSavedSearchQueryChanged: saved search local id "
            << "= " << savedSearchLocalId << ", query: " << query);

    auto * pSavedSearchModel = m_filterBySavedSearchWidget.savedSearchModel();
    if (Q_UNLIKELY(!pSavedSearchModel)) {
        QNWARNING(
            "widget:note_filters",
            "Cannot update saved search: no saved search model");
        return;
    }

    const QString existingQuery =
        pSavedSearchModel->queryForLocalId(savedSearchLocalId);

    if (existingQuery == query) {
        QNDEBUG("widget:note_filters", "Saved search query did not change");
        return;
    }

    auto pUpdateSavedSearchDialog =
        std::make_unique<AddOrEditSavedSearchDialog>(
            pSavedSearchModel, qobject_cast<QWidget *>(parent()),
            savedSearchLocalId);

    pUpdateSavedSearchDialog->setQuery(query);
    Q_UNUSED(pUpdateSavedSearchDialog->exec())

    // Whatever the outcome, need to update the query in the filter by search
    // string widget: if the dialog was rejected, it would return back
    // the original search query; if the dialog was accepted, the query
    // might have been edited before accepting, so need to account for that
    m_filterBySearchStringWidget.setSavedSearch(
        savedSearchLocalId,
        pSavedSearchModel->queryForLocalId(savedSearchLocalId));
}

void NoteFiltersManager::onSearchSavingRequested(QString query) // NOLINT
{
    QNDEBUG(
        "widget:note_filters",
        "NoteFiltersManager::onSearchSavingRequested: " << query);

    auto * pParentWidget = qobject_cast<QWidget *>(parent());

    auto * pSavedSearchModel = m_filterBySavedSearchWidget.savedSearchModel();

    if (Q_UNLIKELY(!pSavedSearchModel)) {
        QNWARNING(
            "widget:note_filters",
            "Cannot create a new saved search: no saved search model");
        return;
    }

    auto pCreateSavedSearchDialog =
        std::make_unique<AddOrEditSavedSearchDialog>(
            pSavedSearchModel, pParentWidget);

    pCreateSavedSearchDialog->setQuery(query);
    if (pCreateSavedSearchDialog->exec() != QDialog::Accepted) {
        return;
    }

    // The query might have been edited before accepting, in this case need
    // to update it in the widget and also re-evaluate the search
    const QString queryFromDialog = pCreateSavedSearchDialog->query();
    if (queryFromDialog == query) {
        return;
    }

    m_filterBySearchStringWidget.setSearchQuery(queryFromDialog);
    persistSearchQuery(queryFromDialog);
    evaluate();
}

void NoteFiltersManager::onFindNoteLocalIdsWithSearchQueryCompleted(
    QStringList noteLocalIds, NoteSearchQuery noteSearchQuery, // NOLINT
    QUuid requestId)
{
    if (Q_UNLIKELY(m_pNoteModel.isNull())) {
        return;
    }

    bool isRequestForSearchString =
        (requestId == m_findNoteLocalIdsForSearchStringRequestId);

    bool isRequestForSavedSearch = !isRequestForSearchString &&
        (requestId == m_findNoteLocalIdsForSavedSearchQueryRequestId);

    if (!isRequestForSearchString && !isRequestForSavedSearch) {
        return;
    }

    QNDEBUG(
        "widget:note_filters",
        "NoteFiltersManager::onFindNoteLocalIdsWithSearchQueryCompleted: "
            << "note search query: " << noteSearchQuery
            << "\nRequest id = " << requestId);

    QNTRACE(
        "widget:note_filters",
        "Note local ids: " << noteLocalIds.join(QStringLiteral(", ")));

    m_pNoteModel->setFilteredNoteLocalIds(noteLocalIds);
}

void NoteFiltersManager::onFindNoteLocalIdsWithSearchQueryFailed(
    NoteSearchQuery noteSearchQuery, ErrorString errorDescription, // NOLINT
    QUuid requestId)
{
    if (Q_UNLIKELY(m_pNoteModel.isNull())) {
        return;
    }

    bool isRequestForSearchString =
        (requestId == m_findNoteLocalIdsForSearchStringRequestId);

    bool isRequestForSavedSearch = !isRequestForSearchString &&
        (requestId == m_findNoteLocalIdsForSavedSearchQueryRequestId);

    if (!isRequestForSearchString && !isRequestForSavedSearch) {
        return;
    }

    QNWARNING(
        "widget:note_filters",
        "NoteFiltersManager::onFindNoteLocalIdsWithSearchQueryFailed: "
            << "request id = " << requestId << ", note search query = "
            << noteSearchQuery << "\nError description: " << errorDescription);

    ErrorString error;

    if (isRequestForSearchString) {
        error.setBase(QT_TR_NOOP("Can't set the search string to note filter"));
    }
    else // isRequestForSavedSearch
    {
        error.setBase(QT_TR_NOOP("Can't set the saved search to note filter"));
    }

    error.appendBase(errorDescription.base());
    error.appendBase(errorDescription.additionalBases());
    error.details() = errorDescription.details();
    QNDEBUG("widget:note_filters", error);
    Q_EMIT notifyError(error);

    if (isRequestForSavedSearch) {
        m_filteredSavedSearchLocalId.clear();
        m_filterBySavedSearchWidget.setCurrentSavedSearchLocalId({});
        m_filterBySearchStringWidget.clearSavedSearch();

        if (setFilterBySearchString()) {
            Q_EMIT filterChanged();
            return;
        }
    }

    m_pNoteModel->beginUpdateFilter();

    setFilterByNotebooks();
    setFilterByTags();

    m_pNoteModel->endUpdateFilter();

    Q_EMIT filterChanged();
}

void NoteFiltersManager::onAddNoteComplete(
    qevercloud::Note note, QUuid requestId) // NOLINT
{
    if (Q_UNLIKELY(m_pNoteModel.isNull())) {
        return;
    }

    QNDEBUG(
        "widget:note_filters",
        "NoteFiltersManager::onAddNoteComplete: request id = " << requestId);

    QNTRACE("widget:note_filters", note);

    checkAndRefreshNotesSearchQuery();
}

void NoteFiltersManager::onUpdateNoteComplete(
    qevercloud::Note note, // NOLINT
    LocalStorageManager::UpdateNoteOptions options, QUuid requestId)
{
    Q_UNUSED(options);

    if (Q_UNLIKELY(m_pNoteModel.isNull())) {
        return;
    }

    QNDEBUG(
        "widget:note_filters",
        "NoteFiltersManager::onUpdateNoteComplete: "
            << "request id = " << requestId);

    QNTRACE("widget:note_filters", note);

    checkAndRefreshNotesSearchQuery();
}

void NoteFiltersManager::onExpungeNotebookComplete(
    qevercloud::Notebook notebook, QUuid requestId) // NOLINT
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

    if (Q_UNLIKELY(m_pNoteModel.isNull())) {
        QNDEBUG("widget:note_filters", "Note model is null");
        return;
    }

    auto notebookLocalIds = m_pNoteModel->filteredNotebookLocalIds();
    int index = notebookLocalIds.indexOf(notebook.localId());
    if (index < 0) {
        QNDEBUG(
            "widget:note_filters",
            "The expunged notebook was not used "
                << "within the filter");
        return;
    }

    QNDEBUG(
        "widget:note_filters",
        "The expunged notebook was used within the filter");

    notebookLocalIds.removeAt(index);
    m_pNoteModel->setFilteredNotebookLocalIds(notebookLocalIds);
}

void NoteFiltersManager::onExpungeTagComplete(
    qevercloud::Tag tag, QStringList expungedChildTagLocalIds, // NOLINT
    QUuid requestId)
{
    QNDEBUG(
        "widget:note_filters",
        "NoteFiltersManager::onExpungeTagComplete: "
            << "tag = " << tag << "\nExpunged child tag local ids: "
            << expungedChildTagLocalIds.join(QStringLiteral(", "))
            << ", request id = " << requestId);

    if (!m_filterByTagWidget.isEnabled()) {
        QNDEBUG(
            "widget:note_filters",
            "The filter by tags is overridden by either search string or "
                << "filter by saved search");
        return;
    }

    if (Q_UNLIKELY(m_pNoteModel.isNull())) {
        QNDEBUG("widget:note_filters", "Note model is null");
        return;
    }

    QStringList expungedTagLocalIds;
    expungedTagLocalIds.reserve(
        std::max(0, expungedChildTagLocalIds.size() + 1));

    expungedTagLocalIds << tag.localId();
    expungedTagLocalIds << expungedChildTagLocalIds;

    QStringList tagLocalIds = m_pNoteModel->filteredTagLocalIds();

    bool filteredTagsChanged = false;
    for (const auto & expungedTagLocalId: qAsConst(expungedTagLocalIds)) {
        int tagIndex = tagLocalIds.indexOf(expungedTagLocalId);
        if (tagIndex >= 0) {
            tagLocalIds.removeAt(tagIndex);
            filteredTagsChanged = true;
        }
    }

    if (!filteredTagsChanged) {
        QNDEBUG(
            "widget:note_filters",
            "None of expunged tags seem to appear within the list of filtered "
                << "tags");
        return;
    }

    m_pNoteModel->setFilteredTagLocalIds(tagLocalIds);
}

void NoteFiltersManager::onUpdateSavedSearchComplete(
    qevercloud::SavedSearch search, QUuid requestId) // NOLINT
{
    QNDEBUG(
        "widget:note_filters",
        "NoteFiltersManager::onUpdateSavedSearchComplete: search = "
            << search << "\nRequest id = " << requestId);

    if (m_filteredSavedSearchLocalId != search.localId()) {
        return;
    }

    QNDEBUG(
        "widget:note_filters",
        "The saved search within the filter was updated");

    if (search.name() && search.query()) {
        onSavedSearchFilterChanged(*search.name());
        return;
    }

    QNDEBUG(
        "widget:note_filters",
        "The updated saved search lacks either name or query, removing it from "
            << "the filter");

    onSavedSearchFilterChanged(QString());
}

void NoteFiltersManager::onExpungeSavedSearchComplete(
    qevercloud::SavedSearch search, QUuid requestId) // NOLINT
{
    QNDEBUG(
        "widget:note_filters",
        "NoteFiltersManager::onExpungeSavedSearchComplete: search = "
            << search << "\nRequest id = " << requestId);

    if (m_filteredSavedSearchLocalId != search.localId()) {
        return;
    }

    QNDEBUG(
        "widget:note_filters",
        "The saved search within the filter was expunged");

    onSavedSearchFilterChanged({});
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
        &m_filterBySearchStringWidget,
        &FilterBySearchStringWidget::searchQueryChanged, this,
        &NoteFiltersManager::onSearchQueryChanged);

    QObject::connect(
        &m_filterBySearchStringWidget,
        &FilterBySearchStringWidget::searchSavingRequested, this,
        &NoteFiltersManager::onSearchSavingRequested);

    QObject::connect(
        &m_filterBySearchStringWidget,
        &FilterBySearchStringWidget::savedSearchQueryChanged, this,
        &NoteFiltersManager::onSavedSearchQueryChanged);

    QObject::connect(
        this, &NoteFiltersManager::findNoteLocalIdsForNoteSearchQuery,
        &m_localStorageManagerAsync,
        &LocalStorageManagerAsync::onFindNoteLocalIdsWithSearchQuery,
        Qt::UniqueConnection);

    QObject::connect(
        &m_localStorageManagerAsync,
        &LocalStorageManagerAsync::findNoteLocalIdsWithSearchQueryComplete,
        this, &NoteFiltersManager::onFindNoteLocalIdsWithSearchQueryCompleted,
        Qt::UniqueConnection);

    QObject::connect(
        &m_localStorageManagerAsync,
        &LocalStorageManagerAsync::findNoteLocalIdsWithSearchQueryFailed, this,
        &NoteFiltersManager::onFindNoteLocalIdsWithSearchQueryFailed,
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

    if (Q_UNLIKELY(m_pNoteModel.isNull())) {
        QNDEBUG("widget:note_filters", "Note model is null");
        return;
    }

    // NOTE: the rules for note filter evaluation are the following:
    // 1) If some saved search is selected, filtering by saved search overrides
    //    the filters by notebooks and tags as well as the filter by manually
    //    entered search query.
    // 2) If no saved search is selected and the search query is not empty *and*
    //    it is indeed a valid search query string, if overrides filters by
    //    notebook and tags.
    // 3) If no saved search is selected and the search query is empty or
    //    invalid, the filters by notebooks and tags are considered.
    //    Filtering by notebooks and tags is cumulative i.e. only those notes
    //    would be accepted which both belong to one of the specified notebooks
    //    and have at least one of the specified tags. In other words, it's
    //    like a search query string containing a notebook and some tags but
    //    not containing "any" statement.

    bool res = setFilterBySavedSearch();
    if (res) {
        Q_EMIT filterChanged();
        return;
    }

    res = setFilterBySearchString();
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

void NoteFiltersManager::persistSearchQuery(const QString & query)
{
    QNDEBUG("widget:note_filters", "NoteFiltersManager::persistSearchQuery");

    ApplicationSettings appSettings(
        m_account, preferences::keys::files::userInterface);

    appSettings.beginGroup(NOTE_FILTERS_GROUP_KEY);
    appSettings.setValue(NOTE_SEARCH_QUERY_KEY, query);
    appSettings.endGroup();

    // Remove old group where this preference used to reside
    appSettings.remove(NOTE_SEARCH_STRING_GROUP_KEY);
}

void NoteFiltersManager::restoreSearchQuery()
{
    QNDEBUG("widget:note_filters", "NoteFiltersManager::restoreSearchQuery");

    ApplicationSettings appSettings(
        m_account, preferences::keys::files::userInterface);

    appSettings.beginGroup(NOTE_FILTERS_GROUP_KEY);
    auto lastSearchStringValue = appSettings.value(NOTE_SEARCH_QUERY_KEY);
    appSettings.endGroup();

    // Backward compatibility: look for preference in old location as a fallback
    if (!lastSearchStringValue.isValid()) {
        appSettings.beginGroup(NOTE_SEARCH_STRING_GROUP_KEY);
        lastSearchStringValue = appSettings.value(NOTE_SEARCH_QUERY_KEY);
        appSettings.endGroup();
    }

    QString lastSearchString = lastSearchStringValue.toString();
    m_filterBySearchStringWidget.setSearchQuery(lastSearchString);
    if (lastSearchString.isEmpty()) {
        return;
    }

    ErrorString error;
    auto query = createNoteSearchQuery(lastSearchString, error);
    if (query.isEmpty()) {
        showSearchQueryErrorToolTip(error);
    }
}

bool NoteFiltersManager::setFilterBySavedSearch()
{
    QNDEBUG(
        "widget:note_filters", "NoteFiltersManager::setFilterBySavedSearch");

    if (Q_UNLIKELY(m_pNoteModel.isNull())) {
        QNDEBUG("widget:note_filters", "Note model is null");
        return false;
    }

    m_filterBySavedSearchWidget.setEnabled(true);

    QString currentSavedSearchName = m_filterBySavedSearchWidget.currentText();
    if (currentSavedSearchName.isEmpty()) {
        QNDEBUG(
            "widget:note_filters", "No saved search name is set to the filter");
        m_pNoteModel->clearFilteredNoteLocalIds();
        m_filterBySearchStringWidget.clearSavedSearch();
        return false;
    }

    const auto * pSavedSearchModel =
        m_filterBySavedSearchWidget.savedSearchModel();

    if (Q_UNLIKELY(!pSavedSearchModel)) {
        QNDEBUG(
            "widget:note_filters",
            "Saved search model in the filter by saved search widget is null");
        m_pNoteModel->clearFilteredNoteLocalIds();
        m_filterBySearchStringWidget.clearSavedSearch();
        return false;
    }

    auto itemIndex =
        pSavedSearchModel->indexForSavedSearchName(currentSavedSearchName);

    if (Q_UNLIKELY(!itemIndex.isValid())) {
        ErrorString error(
            QT_TR_NOOP("Internal error: can't set the filter by saved search, "
                       "the saved search model returned invalid model index "
                       "for saved search name"));

        QNWARNING("widget:note_filters", error);
        Q_EMIT notifyError(error);
        m_pNoteModel->clearFilteredNoteLocalIds();
        m_filterBySearchStringWidget.clearSavedSearch();
        return false;
    }

    const auto * pItem = pSavedSearchModel->itemForIndex(itemIndex);

    const SavedSearchItem * pSavedSearchItem =
        (pItem ? pItem->cast<SavedSearchItem>() : nullptr);

    if (Q_UNLIKELY(!pSavedSearchItem)) {
        ErrorString error(
            QT_TR_NOOP("Internal error: can't set the filter by saved search, "
                       "the saved search model returned null item for valid "
                       "model index"));

        QNWARNING("widget:note_filters", error);
        Q_EMIT notifyError(error);
        m_pNoteModel->clearFilteredNoteLocalIds();
        m_filterBySearchStringWidget.clearSavedSearch();
        return false;
    }

    if (Q_UNLIKELY(pSavedSearchItem->query().isEmpty())) {
        ErrorString error(
            QT_TR_NOOP("Can't set the filter by saved search: "
                       "saved search's query is empty"));

        QNWARNING(
            "widget:note_filters",
            error << ", saved search item: " << *pSavedSearchItem);

        Q_EMIT notifyError(error);
        m_pNoteModel->clearFilteredNoteLocalIds();
        m_filterBySearchStringWidget.clearSavedSearch();
        return false;
    }

    m_filterBySearchStringWidget.setSavedSearch(
        pSavedSearchItem->localId(), pSavedSearchItem->query());

    NoteSearchQuery query;
    ErrorString errorDescription;

    bool res =
        query.setQueryString(pSavedSearchItem->query(), errorDescription);

    if (Q_UNLIKELY(!res)) {
        ErrorString error(
            QT_TR_NOOP("Internal error: can't set the filter by saved search: "
                       "failed to parse saved search query"));

        error.appendBase(errorDescription.base());
        error.appendBase(errorDescription.additionalBases());
        error.details() = errorDescription.details();

        QNWARNING(
            "widget:note_filters",
            error << ", saved search item: " << *pSavedSearchItem);

        Q_EMIT notifyError(error);
        m_pNoteModel->clearFilteredNoteLocalIds();
        return false;
    }

    m_filteredSavedSearchLocalId = pSavedSearchItem->localId();

    // Invalidate the active request to find note local ids per search query
    // (if there was any)
    m_findNoteLocalIdsForSearchStringRequestId = QUuid();

    m_findNoteLocalIdsForSavedSearchQueryRequestId = QUuid::createUuid();

    QNTRACE(
        "widget:note_filters",
        "Emitting the request to find note local "
            << "uids corresponding to the saved search: request id = "
            << m_findNoteLocalIdsForSavedSearchQueryRequestId
            << ", query: " << query << "\nSaved search item: " << *pItem);

    Q_EMIT findNoteLocalIdsForNoteSearchQuery(
        query, m_findNoteLocalIdsForSavedSearchQueryRequestId);

    m_filterByTagWidget.setDisabled(true);
    m_filterByNotebookWidget.setDisabled(true);

    return true;
}

bool NoteFiltersManager::setFilterBySearchString()
{
    QNDEBUG(
        "widget:note_filters", "NoteFiltersManager::setFilterBySearchString");

    if (m_filterBySearchStringWidget.displaysSavedSearchQuery()) {
        return false;
    }

    QString searchString = m_filterBySearchStringWidget.searchQuery();
    if (searchString.isEmpty()) {
        QNDEBUG("widget:note_filters", "The search string is empty");
        return false;
    }

    ErrorString error;
    auto query = createNoteSearchQuery(searchString, error);
    if (query.isEmpty()) {
        showSearchQueryErrorToolTip(error);
        return false;
    }

    // Invalidate the active request to find note local ids per saved search's
    // query (if there was any)
    m_findNoteLocalIdsForSavedSearchQueryRequestId = QUuid();

    m_findNoteLocalIdsForSearchStringRequestId = QUuid::createUuid();

    QNTRACE(
        "widget:note_filters",
        "Emitting the request to find note local "
            << "uids corresponding to the note search query: request id = "
            << m_findNoteLocalIdsForSearchStringRequestId
            << ", query: " << query << "\nSearch string: " << searchString);

    Q_EMIT findNoteLocalIdsForNoteSearchQuery(
        query, m_findNoteLocalIdsForSearchStringRequestId);

    m_filterByTagWidget.setDisabled(true);
    m_filterByNotebookWidget.setDisabled(true);

    return true;
}

void NoteFiltersManager::setFilterByNotebooks()
{
    QNDEBUG("widget:note_filters", "NoteFiltersManager::setFilterByNotebooks");

    if (Q_UNLIKELY(m_pNoteModel.isNull())) {
        QNDEBUG("widget:note_filters", "Note model is null");
        return;
    }

    // If this method got called in the first place, it means the notebook
    // filter is not overridden by either search string or saved search filters
    // so should enable the filter by notebook widget
    m_filterByNotebookWidget.setEnabled(true);

    auto notebookLocalIds =
        m_filterByNotebookWidget.localIdsOfItemsInFilter();

    QNTRACE(
        "widget:note_filters",
        "Notebook local ids to be used for "
            << "filtering: "
            << (notebookLocalIds.isEmpty()
                    ? QStringLiteral("<empty>")
                    : notebookLocalIds.join(QStringLiteral(", "))));

    m_pNoteModel->setFilteredNotebookLocalIds(notebookLocalIds);
}

void NoteFiltersManager::setFilterByTags()
{
    QNDEBUG("widget:note_filters", "NoteFiltersManager::setFilterByTags");

    if (Q_UNLIKELY(m_pNoteModel.isNull())) {
        QNDEBUG("widget:note_filters", "Note model is null");
        return;
    }

    // If this method got called in the first place, it means the tag filter is
    // not overridden by either search string or saved search filter so should
    // enable the filter by tag widget
    m_filterByTagWidget.setEnabled(true);

    auto tagLocalIds = m_filterByTagWidget.localIdsOfItemsInFilter();

    QNTRACE(
        "widget:note_filters",
        "Tag local ids to be used for filtering: "
            << tagLocalIds.join(QStringLiteral(", ")));

    m_pNoteModel->setFilteredTagLocalIds(tagLocalIds);
}

void NoteFiltersManager::clearFilterWidgetsItems()
{
    QNDEBUG(
        "widget:note_filters", "NoteFiltersManager::clearFilterWidgetsItems");

    clearFilterByTagWidgetItems();
    clearFilterByNotebookWidgetItems();
    clearFilterBySavedSearchWidget();
    clearFilterBySearchStringWidget();
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

void NoteFiltersManager::clearFilterBySearchStringWidget()
{
    QNDEBUG(
        "widget:note_filters",
        "NoteFiltersManager::clearFilterBySearchStringWidget");

    QObject::disconnect(
        &m_filterBySearchStringWidget,
        &FilterBySearchStringWidget::searchQueryChanged, this,
        &NoteFiltersManager::onSearchQueryChanged);

    QObject::disconnect(
        &m_filterBySearchStringWidget,
        &FilterBySearchStringWidget::searchSavingRequested, this,
        &NoteFiltersManager::onSearchSavingRequested);

    QObject::disconnect(
        &m_filterBySearchStringWidget,
        &FilterBySearchStringWidget::savedSearchQueryChanged, this,
        &NoteFiltersManager::onSavedSearchQueryChanged);

    m_filterBySearchStringWidget.clearSavedSearch();
    m_filterBySearchStringWidget.setSearchQuery({});

    QObject::connect(
        &m_filterBySearchStringWidget,
        &FilterBySearchStringWidget::searchQueryChanged, this,
        &NoteFiltersManager::onSearchQueryChanged);

    QObject::connect(
        &m_filterBySearchStringWidget,
        &FilterBySearchStringWidget::searchSavingRequested, this,
        &NoteFiltersManager::onSearchSavingRequested);

    QObject::connect(
        &m_filterBySearchStringWidget,
        &FilterBySearchStringWidget::savedSearchQueryChanged, this,
        &NoteFiltersManager::onSavedSearchQueryChanged);

    persistSearchQuery({});
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

void NoteFiltersManager::checkFiltersReadiness()
{
    QNDEBUG("widget:note_filters", "NoteFiltersManager::checkFiltersReadiness");

    if (m_isReady) {
        QNDEBUG(
            "widget:note_filters", "Already marked the filter as ready once");
        return;
    }

    if (!m_filterByTagWidget.isReady()) {
        QNDEBUG(
            "widget:note_filters",
            "Still pending the readiness of filter by tags");
        return;
    }

    if (!m_filterByNotebookWidget.isReady()) {
        QNDEBUG(
            "widget:note_filters",
            "Still pending the readiness of filter by notebooks");
        return;
    }

    if (!m_filterBySavedSearchWidget.isReady()) {
        QNDEBUG(
            "widget:note_filters",
            "Still pending the readiness of filter by saved search");
        return;
    }

    QNDEBUG("widget:note_filters", "All filters are ready");
    m_isReady = true;
    evaluate();
    Q_EMIT ready();
}

void NoteFiltersManager::setNotebooksToFilterImpl(
    const QStringList & notebookLocalIds)
{
    if (notebookLocalIds.isEmpty()) {
        return;
    }

    const auto * pNotebookModel = m_filterByNotebookWidget.notebookModel();
    if (Q_UNLIKELY(!pNotebookModel)) {
        QNDEBUG(
            "widget:note_filters",
            "Notebook model in the filter by notebook widget is null");
        return;
    }

    QVector<AbstractItemModel::ItemInfo> itemInfos;
    itemInfos.reserve(notebookLocalIds.size());

    for (const auto & notebookLocalId: qAsConst(notebookLocalIds)) {
        auto itemInfo = pNotebookModel->itemInfoForLocalId(notebookLocalId);
        if (itemInfo.m_localId.isEmpty()) {
            QNWARNING(
                "widget:note_filters",
                "Failed to find notebook name for notebook local id "
                    << notebookLocalId);
            continue;
        }

        itemInfos << itemInfo;
    }

    QObject::disconnect(
        &m_filterByNotebookWidget, &FilterByNotebookWidget::addedItemToFilter,
        this, &NoteFiltersManager::onAddedNotebookToFilter);

    persistFilterByNotebookClearedState(false);

    for (const auto & itemInfo: qAsConst(itemInfos)) {
        m_filterByNotebookWidget.addItemToFilter(
            itemInfo.m_localId, itemInfo.m_name, itemInfo.m_linkedNotebookGuid,
            itemInfo.m_linkedNotebookUsername);
    }

    QObject::connect(
        &m_filterByNotebookWidget, &FilterByNotebookWidget::addedItemToFilter,
        this, &NoteFiltersManager::onAddedNotebookToFilter,
        Qt::UniqueConnection);
}

void NoteFiltersManager::setTagsToFilterImpl(const QStringList & tagLocalIds)
{
    if (tagLocalIds.isEmpty()) {
        return;
    }

    const auto * pTagModel = m_filterByTagWidget.tagModel();
    if (Q_UNLIKELY(!pTagModel)) {
        QNDEBUG(
            "widget:note_filters",
            "Tag model in the filter by tag widget is null");
        return;
    }

    QVector<AbstractItemModel::ItemInfo> itemInfos;
    itemInfos.reserve(tagLocalIds.size());

    for (const auto & tagLocalId: qAsConst(tagLocalIds)) {
        auto itemInfo = pTagModel->itemInfoForLocalId(tagLocalId);
        if (Q_UNLIKELY(itemInfo.m_localId.isEmpty())) {
            QNWARNING(
                "widget:note_filters",
                "Failed to find info for tag local id: " << tagLocalId);
            continue;
        }

        itemInfos << itemInfo;
    }

    QObject::disconnect(
        &m_filterByTagWidget, &FilterByTagWidget::addedItemToFilter, this,
        &NoteFiltersManager::onAddedTagToFilter);

    persistFilterByTagClearedState(false);

    for (const auto & itemInfo: qAsConst(itemInfos)) {
        m_filterByTagWidget.addItemToFilter(
            itemInfo.m_localId, itemInfo.m_name, itemInfo.m_linkedNotebookGuid,
            itemInfo.m_linkedNotebookUsername);
    }

    QObject::connect(
        &m_filterByTagWidget, &FilterByTagWidget::addedItemToFilter, this,
        &NoteFiltersManager::onAddedTagToFilter, Qt::UniqueConnection);
}

void NoteFiltersManager::setSavedSearchToFilterImpl(
    const QString & savedSearchLocalId)
{
    const auto * pSavedSearchModel =
        m_filterBySavedSearchWidget.savedSearchModel();

    if (Q_UNLIKELY(!pSavedSearchModel)) {
        QNDEBUG(
            "widget:note_filters",
            "Saved search model in the filter by saved search widget is null");
        return;
    }

    persistFilterBySavedSearchClearedState(savedSearchLocalId.isEmpty());

    m_filteredSavedSearchLocalId = savedSearchLocalId;

    m_filterBySavedSearchWidget.setCurrentSavedSearchLocalId(
        savedSearchLocalId);

    if (savedSearchLocalId.isEmpty()) {
        m_filterBySearchStringWidget.clearSavedSearch();
        return;
    }

    m_filterBySearchStringWidget.setSavedSearch(
        savedSearchLocalId,
        pSavedSearchModel->queryForLocalId(savedSearchLocalId));
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
        (!m_filterBySearchStringWidget.searchQuery().isEmpty() ||
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
        QNDEBUG("widget:note_filters", "Notebook filter was cleared");
        return true;
    }

    const auto * pModel = m_filterByNotebookWidget.notebookModel();
    if (!pModel) {
        QNDEBUG(
            "widget:note_filters",
            "Notebook model is not set to filter by notebook widget yet");
        return false;
    }

    if (!pModel->allNotebooksListed()) {
        QNDEBUG(
            "widget:note_filters",
            "Not all notebooks are listed yet by the notebook model");
        return false;
    }

    QString autoSelectedNotebookLocalId;
    auto lastUsedNotebookIndex = pModel->lastUsedNotebookIndex();

    const auto * pLastUsedNotebookModelItem =
        pModel->itemForIndex(lastUsedNotebookIndex);

    if (pLastUsedNotebookModelItem) {
        const auto * pLastUsedNotebookItem =
            pLastUsedNotebookModelItem->cast<NotebookItem>();

        if (pLastUsedNotebookItem) {
            autoSelectedNotebookLocalId = pLastUsedNotebookItem->localId();
        }
    }

    if (autoSelectedNotebookLocalId.isEmpty()) {
        QNDEBUG(
            "widget:note_filters",
            "No last used notebook local id, "
                << "trying default notebook");

        auto defaultNotebookIndex = pModel->defaultNotebookIndex();

        const auto * pDefaultNotebookModelItem =
            pModel->itemForIndex(defaultNotebookIndex);

        if (pDefaultNotebookModelItem) {
            const auto * pDefaultNotebookItem =
                pDefaultNotebookModelItem->cast<NotebookItem>();

            if (pDefaultNotebookItem) {
                autoSelectedNotebookLocalId = pDefaultNotebookItem->localId();
            }
        }
    }

    if (autoSelectedNotebookLocalId.isEmpty()) {
        QNDEBUG(
            "widget:note_filters",
            "No default notebook local id, trying just any notebook");

        QStringList notebookNames = pModel->itemNames(QString());
        if (Q_UNLIKELY(notebookNames.isEmpty())) {
            QNDEBUG(
                "widget:note_filters",
                "No notebooks within the notebook model");
            // NOTE: returning true because false is only for cases
            // in which the filter is waiting for something
            return true;
        }

        const QString & firstNotebookName = notebookNames.at(0);

        autoSelectedNotebookLocalId =
            pModel->localIdForItemName(firstNotebookName, {});
    }

    if (Q_UNLIKELY(autoSelectedNotebookLocalId.isEmpty())) {
        QNDEBUG(
            "widget:note_filters",
            "Failed to find any notebook for automatic selection");
        // NOTE: returning true because false is only for cases
        // in which the filter is waiting for something
        return true;
    }

    auto itemInfo = pModel->itemInfoForLocalId(autoSelectedNotebookLocalId);
    if (Q_UNLIKELY(itemInfo.m_localId.isEmpty())) {
        QNWARNING(
            "widget:note_filters",
            "Failed fo find notebook item for auto selected local id "
                << autoSelectedNotebookLocalId);

        // NOTE: returning true because false is only for cases
        // in which the filter is waiting for something
        return true;
    }

    QNDEBUG(
        "widget:note_filters",
        "Auto selecting notebook: local id = "
            << autoSelectedNotebookLocalId << ", name: " << itemInfo.m_name);

    QObject::disconnect(
        &m_filterByNotebookWidget, &FilterByNotebookWidget::addedItemToFilter,
        this, &NoteFiltersManager::onAddedNotebookToFilter);

    m_filterByNotebookWidget.addItemToFilter(
        autoSelectedNotebookLocalId, itemInfo.m_name,
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

    ApplicationSettings appSettings(
        m_account, preferences::keys::files::userInterface);

    appSettings.beginGroup(NOTE_FILTERS_GROUP_KEY);
    appSettings.setValue(NOTEBOOK_FILTER_CLEARED, state);
    appSettings.endGroup();
}

bool NoteFiltersManager::notebookFilterWasCleared() const
{
    ApplicationSettings appSettings(
        m_account, preferences::keys::files::userInterface);

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

    ApplicationSettings appSettings(
        m_account, preferences::keys::files::userInterface);

    appSettings.beginGroup(NOTE_FILTERS_GROUP_KEY);
    appSettings.setValue(TAG_FILTER_CLEARED, state);
    appSettings.endGroup();
}

bool NoteFiltersManager::tagFilterWasCleared() const
{
    ApplicationSettings appSettings(
        m_account, preferences::keys::files::userInterface);

    appSettings.beginGroup(NOTE_FILTERS_GROUP_KEY);
    auto tagFilterWasCleared = appSettings.value(TAG_FILTER_CLEARED);
    appSettings.endGroup();

    if (!tagFilterWasCleared.isValid()) {
        return false;
    }

    return tagFilterWasCleared.toBool();
}

void NoteFiltersManager::persistFilterBySavedSearchClearedState(
    const bool state)
{
    QNDEBUG(
        "widget:note_filters",
        "NoteFiltersManager::persistFilterBySavedSearchClearedState: "
            << (state ? "true" : "false"));

    ApplicationSettings appSettings(
        m_account, preferences::keys::files::userInterface);

    appSettings.beginGroup(NOTE_FILTERS_GROUP_KEY);
    appSettings.setValue(SAVED_SEARCH_FILTER_CLEARED, state);
    appSettings.endGroup();
}

bool NoteFiltersManager::savedSearchFilterWasCleared() const
{
    ApplicationSettings appSettings(
        m_account, preferences::keys::files::userInterface);

    appSettings.beginGroup(NOTE_FILTERS_GROUP_KEY);

    auto savedSearchFilterWasCleared =
        appSettings.value(SAVED_SEARCH_FILTER_CLEARED);

    appSettings.endGroup();

    if (!savedSearchFilterWasCleared.isValid()) {
        return false;
    }

    return savedSearchFilterWasCleared.toBool();
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

void NoteFiltersManager::showSearchQueryErrorToolTip(
    const ErrorString & errorDescription)
{
    if (Q_UNLIKELY(errorDescription.isEmpty())) {
        return;
    }

    auto * pLineEdit = m_filterBySearchStringWidget.findChild<QLineEdit *>();
    if (pLineEdit) {
        QToolTip::showText(
            pLineEdit->mapToGlobal(QPoint(0, pLineEdit->height())),
            errorDescription.localizedString(), pLineEdit);
    }
}

} // namespace quentier
