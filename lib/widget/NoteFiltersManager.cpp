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

#include "NoteFiltersManager.h"

#include "FilterByNotebookWidget.h"
#include "FilterBySavedSearchWidget.h"
#include "FilterBySearchStringWidget.h"
#include "FilterByTagWidget.h"

#include <lib/exception/Utils.h>
#include <lib/dialog/AddOrEditSavedSearchDialog.h>
#include <lib/model/note/NoteModel.h>
#include <lib/model/notebook/NotebookModel.h>
#include <lib/model/saved_search/SavedSearchModel.h>
#include <lib/model/tag/TagModel.h>
#include <lib/preferences/keys/Files.h>

#include <quentier/exception/InvalidArgument.h>
#include <quentier/local_storage/ILocalStorage.h>
#include <quentier/local_storage/ILocalStorageNotifier.h>
#include <quentier/local_storage/NoteSearchQuery.h>
#include <quentier/logging/QuentierLogger.h>
#include <quentier/threading/Future.h>
#include <quentier/utility/ApplicationSettings.h>
#include <quentier/utility/cancelers/ManualCanceler.h>

#include <QComboBox>
#include <QLineEdit>
#include <QToolTip>

#include <memory>
#include <string_view>

namespace quentier {

using namespace std::string_view_literals;

namespace {

// Deprecated setting: gNoteFiltersGroupKey is used instead on new installations
constexpr auto gNoteSearchStringGroupKey = "NoteSearchStringFilter"sv;
constexpr auto gNoteFiltersGroupKey = "NoteFilters"sv;
constexpr auto gNoteSearchQueryKey = "SearchString"sv;
constexpr auto gNotebookFilterClearedKey = "NotebookFilterCleared"sv;
constexpr auto gTagFilterClearedKey = "TagFilterCleared"sv;
constexpr auto gSavedSearchFilterClearedKey = "SavedSearchFilterCleared"sv;

} // namespace

NoteFiltersManager::NoteFiltersManager(
    Account account, FilterByTagWidget & filterByTagWidget,
    FilterByNotebookWidget & filterByNotebookWidget, NoteModel & noteModel,
    FilterBySavedSearchWidget & filterBySavedSearchWidget,
    FilterBySearchStringWidget & FilterBySearchStringWidget,
    local_storage::ILocalStoragePtr localStorage, QObject * parent) :
    QObject{parent},
    m_account{std::move(account)}, m_localStorage{std::move(localStorage)},
    m_filterByTagWidget{filterByTagWidget},
    m_filterByNotebookWidget{filterByNotebookWidget}, m_noteModel{&noteModel},
    m_filterBySavedSearchWidget{filterBySavedSearchWidget},
    m_filterBySearchStringWidget{FilterBySearchStringWidget}
{
    if (Q_UNLIKELY(!m_localStorage)) {
        throw InvalidArgument{ErrorString{
            "NoteFiltersManager ctor: local storage is null"}};
    }

    createConnections();

    const QString savedSearchLocalId =
        m_filterBySavedSearchWidget.filteredSavedSearchLocalId();

    if (!savedSearchLocalId.isEmpty()) {
        // As filtered saved search's local id is not empty, need to delay
        // the moment of filtering evaluatuon until filter by saved search is
        // ready
        QNDEBUG(
            "widget::NoteFiltersManager",
            "Filtered saved search's local id is not empty, need to wait for "
            "filter's readiness");

        checkFiltersReadiness();
        return;
    }

    restoreSearchQuery();
    if (setFilterBySearchString()) {
        Q_EMIT filterChanged();

        QNDEBUG(
            "widget::NoteFiltersManager",
            "Was able to set the filter by search string, considering "
            "NoteFiltersManager ready");

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
                "widget::NoteFiltersManager",
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

    if (Q_UNLIKELY(m_noteModel.isNull())) {
        return {};
    }

    return m_noteModel->filteredNotebookLocalIds();
}

QStringList NoteFiltersManager::tagLocalIdsInFilter() const
{
    if (!savedSearchLocalIdInFilter().isEmpty()) {
        return {};
    }

    if (isFilterBySearchStringActive()) {
        return {};
    }

    if (Q_UNLIKELY(m_noteModel.isNull())) {
        return {};
    }

    return m_noteModel->filteredTagLocalIds();
}

const QString & NoteFiltersManager::savedSearchLocalIdInFilter() const
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
    QNDEBUG("widget::NoteFiltersManager", "NoteFiltersManager::clear");

    clearFilterWidgetsItems();
    evaluate();
}

void NoteFiltersManager::setNotebooksToFilter(
    const QStringList & notebookLocalIds)
{
    QNDEBUG(
        "widget::NoteFiltersManager",
        "NoteFiltersManager::setNotebooksToFilter: "
            << notebookLocalIds.join(QStringLiteral(", ")));

    clearFilterByNotebookWidgetItems();
    setNotebooksToFilterImpl(notebookLocalIds);
    evaluate();
}

void NoteFiltersManager::removeNotebooksFromFilter()
{
    QNDEBUG(
        "widget::NoteFiltersManager",
        "NoteFiltersManager::removeNotebooksFromFilter");

    persistFilterByNotebookClearedState(true);
    clearFilterByNotebookWidgetItems();
    evaluate();
}

void NoteFiltersManager::setTagsToFilter(const QStringList & tagLocalIds)
{
    QNDEBUG(
        "widget::NoteFiltersManager",
        "NoteFiltersManager::setTagsToFilter: "
            << tagLocalIds.join(QStringLiteral(", ")));

    clearFilterByTagWidgetItems();
    setTagsToFilterImpl(tagLocalIds);
    evaluate();
}

void NoteFiltersManager::removeTagsFromFilter()
{
    QNDEBUG(
        "widget::NoteFiltersManager",
        "NoteFiltersManager::removeTagsFromFilter");

    persistFilterByTagClearedState(true);
    clearFilterByTagWidgetItems();
    evaluate();
}

void NoteFiltersManager::setSavedSearchLocalIdToFilter(
    const QString & savedSearchLocalId)
{
    QNDEBUG(
        "widget::NoteFiltersManager",
        "NoteFiltersManager::setSavedSearchLocalIdToFilter: "
            << savedSearchLocalId);

    setSavedSearchToFilterImpl(savedSearchLocalId);
}

void NoteFiltersManager::removeSavedSearchFromFilter()
{
    QNDEBUG(
        "widget::NoteFiltersManager",
        "NoteFiltersManager::removeSavedSearchFromFilter");

    setSavedSearchToFilterImpl(QString{});
}

void NoteFiltersManager::setItemsToFilter(
    const QString & savedSearchLocalId, const QStringList & notebookLocalIds,
    const QStringList & tagLocalIds)
{
    QNDEBUG(
        "widget::NoteFiltersManager",
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
        "widget::NoteFiltersManager",
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
        "widget::NoteFiltersManager",
        "NoteFiltersManager::onRemovedTagFromFilter: local id = "
            << tagLocalId << ", name = " << tagName
            << ", linked notebook guid = " << linkedNotebookGuid
            << ", linked notebook username = " << linkedNotebookUsername);

    onTagsFilterUpdated();
}

void NoteFiltersManager::onTagsClearedFromFilter()
{
    QNDEBUG(
        "widget::NoteFiltersManager",
        "NoteFiltersManager::onTagsClearedFromFilter");

    onTagsFilterUpdated();
}

void NoteFiltersManager::onTagsFilterUpdated()
{
    QNDEBUG(
        "widget::NoteFiltersManager",
        "NoteFiltersManager::onTagsFilterUpdated");

    if (!m_isReady) {
        QNDEBUG(
            "widget::NoteFiltersManager",
            "Not yet ready to process filter updates");
        return;
    }

    if (!m_filterByTagWidget.isEnabled()) {
        QNDEBUG(
            "widget::NoteFiltersManager",
            "Filter by tag widget is not enabled which means that filtering by "
            "tags is overridden by either search string or filter by saved "
            "search");
        return;
    }

    setFilterByTags();
    Q_EMIT filterChanged();
}

void NoteFiltersManager::onTagsFilterReady()
{
    QNDEBUG(
        "widget::NoteFiltersManager", "NoteFiltersManager::onTagsFilterReady");

    checkFiltersReadiness();
}

void NoteFiltersManager::onAddedNotebookToFilter(
    const QString & notebookLocalId, const QString & notebookName,
    const QString & linkedNotebookGuid, const QString & linkedNotebookUsername)
{
    QNDEBUG(
        "widget::NoteFiltersManager",
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
        "widget::NoteFiltersManager",
        "NoteFiltersManager::onRemovedNotebookFromFilter: local id = "
            << notebookLocalId << ", name = " << notebookName
            << ", linked notebook guid = " << linkedNotebookGuid
            << ", linked notebook username = " << linkedNotebookUsername);

    onNotebooksFilterUpdated();
}

void NoteFiltersManager::onNotebooksClearedFromFilter()
{
    QNDEBUG(
        "widget::NoteFiltersManager",
        "NoteFiltersManager::onNotebooksClearedFromFilter");

    onNotebooksFilterUpdated();
}

void NoteFiltersManager::onNotebooksFilterUpdated()
{
    QNDEBUG(
        "widget::NoteFiltersManager",
        "NoteFiltersManager::onNotebooksFilterUpdated");

    if (!m_isReady) {
        QNDEBUG(
            "widget::NoteFiltersManager",
            "Not yet ready to process filter updates");
        return;
    }

    if (!m_filterByNotebookWidget.isEnabled()) {
        QNDEBUG(
            "widget::NoteFiltersManager",
            "Filter by notebook widget is not enabled which means filtering by "
            "notebooks is overridden by either saved search or search string");
        return;
    }

    setFilterByNotebooks();
    Q_EMIT filterChanged();
}

void NoteFiltersManager::onNotebooksFilterReady()
{
    QNDEBUG(
        "widget::NoteFiltersManager",
        "NoteFiltersManager::onNotebooksFilterReady");

    if (m_autoFilterNotebookWhenReady) {
        m_autoFilterNotebookWhenReady = false;
        setAutomaticFilterByNotebook();
    }

    checkFiltersReadiness();
}

void NoteFiltersManager::onSavedSearchFilterChanged(
    const QString & savedSearchName)
{
    QNDEBUG(
        "widget::NoteFiltersManager",
        "NoteFiltersManager::onSavedSearchFilterChanged: " << savedSearchName);

    if (!m_isReady) {
        QNDEBUG(
            "widget::NoteFiltersManager",
            "Not yet ready to process filter updates");
        return;
    }

    if (Q_UNLIKELY(m_noteModel.isNull())) {
        QNDEBUG("widget::NoteFiltersManager", "Note model is null");
        return;
    }

    if (setFilterBySavedSearch()) {
        Q_EMIT filterChanged();
        return;
    }

    // If we got here, the saved search is either empty or invalid
    m_filteredSavedSearchLocalId.clear();

    if (setFilterBySearchString()) {
        Q_EMIT filterChanged();
        return;
    }

    m_noteModel->beginUpdateFilter();

    setFilterByTags();
    setFilterByNotebooks();

    m_noteModel->endUpdateFilter();

    Q_EMIT filterChanged();
}

void NoteFiltersManager::onSavedSearchFilterReady()
{
    QNDEBUG(
        "widget::NoteFiltersManager",
        "NoteFiltersManager::onSavedSearchFilterReady");

    if (!m_isReady && setFilterBySavedSearch()) {
        QNDEBUG(
            "widget::NoteFiltersManager",
            "Was able to set the filter by saved search, considering "
            "NoteFiltersManager ready");

        m_isReady = true;
        Q_EMIT ready();
        return;
    }

    checkFiltersReadiness();
}

void NoteFiltersManager::onSearchQueryChanged(const QString & query)
{
    QNDEBUG(
        "widget::NoteFiltersManager",
        "NoteFiltersManager::onSearchQueryChanged: " << query);

    persistSearchQuery(query);
    evaluate();
}

void NoteFiltersManager::onSavedSearchQueryChanged(
    const QString & savedSearchLocalId, const QString & query)
{
    QNDEBUG(
        "widget::NoteFiltersManager",
        "NoteFiltersManager::onSavedSearchQueryChanged: saved search local id "
            << "= " << savedSearchLocalId << ", query: " << query);

    auto * savedSearchModel = m_filterBySavedSearchWidget.savedSearchModel();
    if (Q_UNLIKELY(!savedSearchModel)) {
        QNWARNING(
            "widget::NoteFiltersManager",
            "Cannot update saved search: no saved search model");
        return;
    }

    const QString existingQuery =
        savedSearchModel->queryForLocalId(savedSearchLocalId);

    if (existingQuery == query) {
        QNDEBUG(
            "widget::NoteFiltersManager", "Saved search query did not change");
        return;
    }

    auto updateSavedSearchDialog =
        std::make_unique<AddOrEditSavedSearchDialog>(
            savedSearchModel, qobject_cast<QWidget *>(parent()),
            savedSearchLocalId);

    updateSavedSearchDialog->setQuery(query);
    updateSavedSearchDialog->exec();

    // Whatever the outcome, need to update the query in the filter by search
    // string widget: if the dialog was rejected, it would return back
    // the original search query; if the dialog was accepted, the query
    // might have been edited before accepting, so need to account for that
    m_filterBySearchStringWidget.setSavedSearch(
        savedSearchLocalId,
        savedSearchModel->queryForLocalId(savedSearchLocalId));
}

void NoteFiltersManager::onSearchSavingRequested(const QString & query)
{
    QNDEBUG(
        "widget::NoteFiltersManager",
        "NoteFiltersManager::onSearchSavingRequested: " << query);

    auto * parentWidget = qobject_cast<QWidget *>(parent());

    auto * savedSearchModel = m_filterBySavedSearchWidget.savedSearchModel();
    if (Q_UNLIKELY(!savedSearchModel)) {
        QNWARNING(
            "widget::NoteFiltersManager",
            "Cannot create a new saved search: no saved search model");
        return;
    }

    auto createSavedSearchDialog =
        std::make_unique<AddOrEditSavedSearchDialog>(
            savedSearchModel, parentWidget);

    createSavedSearchDialog->setQuery(query);
    if (createSavedSearchDialog->exec() != QDialog::Accepted) {
        return;
    }

    // The query might have been edited before accepting, in this case need
    // to update it in the widget and also re-evaluate the search
    const QString queryFromDialog = createSavedSearchDialog->query();
    if (queryFromDialog == query) {
        return;
    }

    m_filterBySearchStringWidget.setSearchQuery(queryFromDialog);
    persistSearchQuery(queryFromDialog);
    evaluate();
}

void NoteFiltersManager::onFindNoteLocalIdsWithSearchQueryCompleted(
    const QStringList & noteLocalIds,
    const local_storage::NoteSearchQuery & noteSearchQuery)
{
    if (Q_UNLIKELY(m_noteModel.isNull())) {
        return;
    }

    QNDEBUG(
        "widget::NoteFiltersManager",
        "NoteFiltersManager::onFindNoteLocalIdsWithSearchQueryCompleted: "
            << "note search query: " << noteSearchQuery);

    QNTRACE(
        "widget::NoteFiltersManager",
        "Note local ids: " << noteLocalIds.join(QStringLiteral(", ")));

    m_noteModel->setFilteredNoteLocalIds(noteLocalIds);
}

void NoteFiltersManager::onNotebookExpunged(
    const QString & notebookLocalId)
{
    QNDEBUG(
        "widget::NoteFiltersManager",
        "NoteFiltersManager::onNotebookExpunged: local id = "
            << notebookLocalId);

    if (!m_filterByNotebookWidget.isEnabled()) {
        QNDEBUG(
            "widget::NoteFiltersManager",
            "Filter by notebook is overridden by either search string or saved "
            "search filter");
        return;
    }

    if (Q_UNLIKELY(m_noteModel.isNull())) {
        QNDEBUG("widget::NoteFiltersManager", "Note model is null");
        return;
    }

    auto notebookLocalIds = m_noteModel->filteredNotebookLocalIds();
    const auto index = notebookLocalIds.indexOf(notebookLocalId);
    if (index < 0) {
        QNDEBUG(
            "widget::NoteFiltersManager",
            "The expunged notebook was not used within the filter");
        return;
    }

    QNDEBUG(
        "widget::NoteFiltersManager",
        "The expunged notebook was used within the filter");

    notebookLocalIds.removeAt(index);
    m_noteModel->setFilteredNotebookLocalIds(notebookLocalIds);
}

void NoteFiltersManager::onTagExpunged(
    const QString & tagLocalId, const QStringList & childTagLocalIds)
{
    QNDEBUG(
        "widget::NoteFiltersManager",
        "NoteFiltersManager::onTagExpunged: "
            << "local id = " << tagLocalId << "\nExpunged child tag local ids: "
            << childTagLocalIds.join(QStringLiteral(", ")));

    if (!m_filterByTagWidget.isEnabled()) {
        QNDEBUG(
            "widget::NoteFiltersManager",
            "The filter by tags is overridden by either search string or "
            "filter by saved search");
        return;
    }

    if (Q_UNLIKELY(m_noteModel.isNull())) {
        QNDEBUG("widget::NoteFiltersManager", "Note model is null");
        return;
    }

    QStringList expungedTagLocalIds;
    expungedTagLocalIds.reserve(std::max<decltype(childTagLocalIds.size())>(
        0, childTagLocalIds.size() + 1));
    expungedTagLocalIds << tagLocalId;
    expungedTagLocalIds << childTagLocalIds;

    QStringList tagLocalIds = m_noteModel->filteredTagLocalIds();

    bool filteredTagsChanged = false;
    for (const auto & expungedTagLocalId: std::as_const(expungedTagLocalIds)) {
        const auto tagIndex = tagLocalIds.indexOf(expungedTagLocalId);
        if (tagIndex >= 0) {
            tagLocalIds.removeAt(tagIndex);
            filteredTagsChanged = true;
        }
    }

    if (!filteredTagsChanged) {
        QNDEBUG(
            "widget::NoteFiltersManager",
            "None of expunged tags seem to appear within the list of filtered "
            "tags");
        return;
    }

    m_noteModel->setFilteredTagLocalIds(tagLocalIds);
}

void NoteFiltersManager::onSavedSearchPut(
    const qevercloud::SavedSearch & search)
{
    QNDEBUG(
        "widget::NoteFiltersManager",
        "NoteFiltersManager::onSavedSearchPut: search = " << search);

    if (m_filteredSavedSearchLocalId != search.localId()) {
        return;
    }

    QNDEBUG(
        "widget::NoteFiltersManager",
        "The saved search within the filter was updated");

    if (search.name() && search.query()) {
        onSavedSearchFilterChanged(*search.name());
        return;
    }

    QNDEBUG(
        "widget::NoteFiltersManager",
        "The updated saved search lacks either name or query, removing it from "
        "the filter");

    onSavedSearchFilterChanged(QString{});
}

void NoteFiltersManager::onSavedSearchExpunged(
    const QString & savedSearchLocalId)
{
    QNDEBUG(
        "widget::NoteFiltersManager",
        "NoteFiltersManager::onSavedSearchExpunged: local id = "
            << savedSearchLocalId);

    if (m_filteredSavedSearchLocalId != savedSearchLocalId) {
        return;
    }

    QNDEBUG(
        "widget::NoteFiltersManager",
        "The saved search within the filter was expunged");

    onSavedSearchFilterChanged({});
}

void NoteFiltersManager::onNotePut(const qevercloud::Note & note)
{
    if (Q_UNLIKELY(m_noteModel.isNull())) {
        return;
    }

    QNDEBUG(
        "widget::NoteFiltersManager",
        "NoteFiltersManager::onNotePut: note local id = " << note.localId());

    QNTRACE("widget::NoteFiltersManager", note);

    checkAndRefreshNotesSearchQuery();
}

void NoteFiltersManager::createConnections()
{
    QObject::connect(
        &m_filterByTagWidget, &FilterByTagWidget::addedItemToFilter, this,
        &NoteFiltersManager::onAddedTagToFilter);

    QObject::connect(
        &m_filterByTagWidget, &FilterByTagWidget::itemRemovedFromFilter, this,
        &NoteFiltersManager::onRemovedTagFromFilter);

    QObject::connect(
        &m_filterByTagWidget, &FilterByTagWidget::cleared, this,
        &NoteFiltersManager::onTagsClearedFromFilter);

    QObject::connect(
        &m_filterByTagWidget, &FilterByTagWidget::updated, this,
        &NoteFiltersManager::onTagsFilterUpdated);

    QObject::connect(
        &m_filterByTagWidget, &FilterByTagWidget::ready, this,
        &NoteFiltersManager::onTagsFilterReady);

    QObject::connect(
        &m_filterByNotebookWidget, &FilterByNotebookWidget::addedItemToFilter,
        this, &NoteFiltersManager::onAddedNotebookToFilter);

    QObject::connect(
        &m_filterByNotebookWidget,
        &FilterByNotebookWidget::itemRemovedFromFilter, this,
        &NoteFiltersManager::onRemovedNotebookFromFilter);

    QObject::connect(
        &m_filterByNotebookWidget, &FilterByNotebookWidget::cleared, this,
        &NoteFiltersManager::onNotebooksClearedFromFilter);

    QObject::connect(
        &m_filterByNotebookWidget, &FilterByNotebookWidget::updated, this,
        &NoteFiltersManager::onNotebooksFilterUpdated);

    QObject::connect(
        &m_filterByNotebookWidget, &FilterByNotebookWidget::ready, this,
        &NoteFiltersManager::onNotebooksFilterReady);

    QObject::connect(
        &m_filterBySavedSearchWidget,
        &FilterBySavedSearchWidget::currentSavedSearchNameChanged, this,
        &NoteFiltersManager::onSavedSearchFilterChanged);

    QObject::connect(
        &m_filterBySavedSearchWidget, &FilterBySavedSearchWidget::ready, this,
        &NoteFiltersManager::onSavedSearchFilterReady);

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

    auto * notifier = m_localStorage->notifier();

    QObject::connect(
        notifier,
        &local_storage::ILocalStorageNotifier::notebookExpunged, this,
        &NoteFiltersManager::onNotebookExpunged);

    QObject::connect(
        notifier,
        &local_storage::ILocalStorageNotifier::tagExpunged, this,
        &NoteFiltersManager::onTagExpunged);

    QObject::connect(
        notifier,
        &local_storage::ILocalStorageNotifier::savedSearchPut, this,
        &NoteFiltersManager::onSavedSearchPut);

    QObject::connect(
        notifier, &local_storage::ILocalStorageNotifier::savedSearchExpunged,
        this, &NoteFiltersManager::onSavedSearchExpunged);

    QObject::connect(
        notifier, &local_storage::ILocalStorageNotifier::notePut, this,
        &NoteFiltersManager::onNotePut);

    QObject::connect(
        notifier, &local_storage::ILocalStorageNotifier::noteUpdated, this,
        [this](
            const qevercloud::Note & note,
            const local_storage::ILocalStorage::UpdateNoteOptions options) {
            Q_UNUSED(options)
            onNotePut(note);
        });
}

void NoteFiltersManager::evaluate()
{
    QNDEBUG("widget::NoteFiltersManager", "NoteFiltersManager::evaluate");

    if (Q_UNLIKELY(m_noteModel.isNull())) {
        QNDEBUG("widget::NoteFiltersManager", "Note model is null");
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
    if (setFilterBySavedSearch()) {
        Q_EMIT filterChanged();
        return;
    }

    if (setFilterBySearchString()) {
        Q_EMIT filterChanged();
        return;
    }

    m_noteModel->beginUpdateFilter();

    setFilterByNotebooks();
    setFilterByTags();

    m_noteModel->endUpdateFilter();
    Q_EMIT filterChanged();
}

void NoteFiltersManager::persistSearchQuery(const QString & query)
{
    QNDEBUG(
        "widget::NoteFiltersManager", "NoteFiltersManager::persistSearchQuery");

    ApplicationSettings appSettings{
        m_account, preferences::keys::files::userInterface};

    appSettings.beginGroup(gNoteFiltersGroupKey);
    appSettings.setValue(gNoteSearchQueryKey, query);
    appSettings.endGroup();

    // Remove old group where this preference used to reside
    appSettings.remove(gNoteSearchStringGroupKey);
}

void NoteFiltersManager::restoreSearchQuery()
{
    QNDEBUG(
        "widget::NoteFiltersManager", "NoteFiltersManager::restoreSearchQuery");

    ApplicationSettings appSettings{
        m_account, preferences::keys::files::userInterface};

    appSettings.beginGroup(gNoteFiltersGroupKey);
    auto lastSearchStringValue = appSettings.value(gNoteSearchQueryKey);
    appSettings.endGroup();

    // Backward compatibility: look for preference in old location as a fallback
    if (!lastSearchStringValue.isValid()) {
        appSettings.beginGroup(gNoteSearchStringGroupKey);
        lastSearchStringValue = appSettings.value(gNoteSearchQueryKey);
        appSettings.endGroup();
    }

    const QString lastSearchString = lastSearchStringValue.toString();
    m_filterBySearchStringWidget.setSearchQuery(lastSearchString);
    if (lastSearchString.isEmpty()) {
        return;
    }

    ErrorString error;
    const auto query = createNoteSearchQuery(lastSearchString, error);
    if (query.isEmpty()) {
        showSearchQueryErrorToolTip(error);
    }
}

bool NoteFiltersManager::setFilterBySavedSearch()
{
    QNDEBUG(
        "widget::NoteFiltersManager",
        "NoteFiltersManager::setFilterBySavedSearch");

    if (Q_UNLIKELY(m_noteModel.isNull())) {
        QNDEBUG("widget::NoteFiltersManager", "Note model is null");
        return false;
    }

    m_filterBySavedSearchWidget.setEnabled(true);

    const QString currentSavedSearchName =
        m_filterBySavedSearchWidget.currentText();
    if (currentSavedSearchName.isEmpty()) {
        QNDEBUG(
            "widget::NoteFiltersManager",
            "No saved search name is set to the filter");
        m_noteModel->clearFilteredNoteLocalIds();
        m_filterBySearchStringWidget.clearSavedSearch();
        return false;
    }

    const auto * savedSearchModel =
        m_filterBySavedSearchWidget.savedSearchModel();

    if (Q_UNLIKELY(!savedSearchModel)) {
        QNDEBUG(
            "widget::NoteFiltersManager",
            "Saved search model in the filter by saved search widget is null");
        m_noteModel->clearFilteredNoteLocalIds();
        m_filterBySearchStringWidget.clearSavedSearch();
        return false;
    }

    const auto itemIndex =
        savedSearchModel->indexForSavedSearchName(currentSavedSearchName);

    if (Q_UNLIKELY(!itemIndex.isValid())) {
        ErrorString error{
            QT_TR_NOOP("Internal error: can't set the filter by saved search, "
                       "the saved search model returned invalid model index "
                       "for saved search name")};

        QNWARNING("widget::NoteFiltersManager", error);
        Q_EMIT notifyError(std::move(error));
        m_noteModel->clearFilteredNoteLocalIds();
        m_filterBySearchStringWidget.clearSavedSearch();
        return false;
    }

    const auto * item = savedSearchModel->itemForIndex(itemIndex);

    const SavedSearchItem * savedSearchItem =
        (item ? item->cast<SavedSearchItem>() : nullptr);

    if (Q_UNLIKELY(!savedSearchItem)) {
        ErrorString error{
            QT_TR_NOOP("Internal error: can't set the filter by saved search, "
                       "the saved search model returned null item for valid "
                       "model index")};

        QNWARNING("widget::NoteFiltersManager", error);
        Q_EMIT notifyError(std::move(error));
        m_noteModel->clearFilteredNoteLocalIds();
        m_filterBySearchStringWidget.clearSavedSearch();
        return false;
    }

    if (Q_UNLIKELY(savedSearchItem->query().isEmpty())) {
        ErrorString error{
            QT_TR_NOOP("Can't set the filter by saved search: "
                       "saved search's query is empty")};

        QNWARNING(
            "widget::NoteFiltersManager",
            error << ", saved search item: " << *savedSearchItem);

        Q_EMIT notifyError(std::move(error));
        m_noteModel->clearFilteredNoteLocalIds();
        m_filterBySearchStringWidget.clearSavedSearch();
        return false;
    }

    m_filterBySearchStringWidget.setSavedSearch(
        savedSearchItem->localId(), savedSearchItem->query());

    local_storage::NoteSearchQuery query;
    ErrorString errorDescription;
    if (Q_UNLIKELY(
            !query.setQueryString(savedSearchItem->query(), errorDescription)))
    {
        ErrorString error{
            QT_TR_NOOP("Internal error: can't set the filter by saved search: "
                       "failed to parse saved search query")};

        error.appendBase(errorDescription.base());
        error.appendBase(errorDescription.additionalBases());
        error.details() = errorDescription.details();

        QNWARNING(
            "widget::NoteFiltersManager",
            error << ", saved search item: " << *savedSearchItem);

        Q_EMIT notifyError(error);
        m_noteModel->clearFilteredNoteLocalIds();
        return false;
    }

    m_filteredSavedSearchLocalId = savedSearchItem->localId();

    // Invalidate active request to find note local ids per search query
    // (if there was any)
    if (m_canceler) {
        m_canceler->cancel();
        m_canceler.reset();
    }

    auto canceler = setupCanceler();
    Q_ASSERT(canceler);

    QNTRACE(
        "widget::NoteFiltersManager",
        "Trying to find note local ids corresponding to the saved search: "
            << "query: " << query << "\nSaved search item: " << *item);

    auto findNoteLocalIdsFuture = m_localStorage->queryNoteLocalIds(query);

    auto findNoteLocalIdsThenFuture = threading::then(
        std::move(findNoteLocalIdsFuture), this,
        [this, query, canceler](const QStringList & noteLocalIds) {
            if (canceler->isCanceled()) {
                return;
            }

            onFindNoteLocalIdsWithSearchQueryCompleted(noteLocalIds, query);
        });

    threading::onFailed(
        std::move(findNoteLocalIdsThenFuture), this,
        [this, query = std::move(query),
         canceler = std::move(canceler)](const QException & e) {
            if (canceler->isCanceled()) {
                return;
            }

            if (Q_UNLIKELY(m_noteModel.isNull())) {
                return;
            }

            auto message = exceptionMessage(e);

            QNWARNING(
                "widget::NoteFiltersManager",
                "Could not find note local ids for saved search query: "
                << message << ", note search query = " << query);

            ErrorString error{QT_TR_NOOP(
                "Can't set saved search to note filter")};
            error.appendBase(message.base());
            error.appendBase(message.additionalBases());
            error.details() = std::move(message.details());
            Q_EMIT notifyError(std::move(error));

            m_filteredSavedSearchLocalId.clear();
            m_filterBySavedSearchWidget.setCurrentSavedSearchLocalId({});
            m_filterBySearchStringWidget.clearSavedSearch();

            if (setFilterBySearchString()) {
                Q_EMIT filterChanged();
                return;
            }

            m_noteModel->beginUpdateFilter();

            setFilterByNotebooks();
            setFilterByTags();

            m_noteModel->endUpdateFilter();

            Q_EMIT filterChanged();
        });

    m_filterByTagWidget.setDisabled(true);
    m_filterByNotebookWidget.setDisabled(true);

    return true;
}

bool NoteFiltersManager::setFilterBySearchString()
{
    QNDEBUG(
        "widget::NoteFiltersManager",
        "NoteFiltersManager::setFilterBySearchString");

    if (m_filterBySearchStringWidget.displaysSavedSearchQuery()) {
        return false;
    }

    const QString searchString = m_filterBySearchStringWidget.searchQuery();
    if (searchString.isEmpty()) {
        QNDEBUG("widget::NoteFiltersManager", "The search string is empty");
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
    if (m_canceler) {
        m_canceler->cancel();
        m_canceler.reset();
    }

    auto canceler = setupCanceler();
    Q_ASSERT(canceler);

    QNTRACE(
        "widget::NoteFiltersManager",
        "Trying to find note local ids corresponding to note search query: "
            << ", query: " << query << "\nSearch string: " << searchString);

    auto findNoteLocalIdsFuture = m_localStorage->queryNoteLocalIds(query);

    auto findNoteLocalIdsThenFuture = threading::then(
        std::move(findNoteLocalIdsFuture), this,
        [this, query, canceler](const QStringList & noteLocalIds) {
            if (canceler->isCanceled()) {
                return;
            }

            onFindNoteLocalIdsWithSearchQueryCompleted(noteLocalIds, query);
        });

    threading::onFailed(
        std::move(findNoteLocalIdsThenFuture), this,
        [this, query = std::move(query),
         canceler = std::move(canceler)](const QException & e) {
            if (canceler->isCanceled()) {
                return;
            }

            if (Q_UNLIKELY(m_noteModel.isNull())) {
                return;
            }

            auto message = exceptionMessage(e);

            QNWARNING(
                "widget::NoteFiltersManager",
                "Could not find note local ids for saved search query: "
                << message << ", note search query = " << query);

            ErrorString error{QT_TR_NOOP(
                "Can't set search string to note filter")};
            error.appendBase(message.base());
            error.appendBase(message.additionalBases());
            error.details() = std::move(message.details());
            Q_EMIT notifyError(std::move(error));

            m_noteModel->beginUpdateFilter();

            setFilterByNotebooks();
            setFilterByTags();

            m_noteModel->endUpdateFilter();

            Q_EMIT filterChanged();
        });

    m_filterByTagWidget.setDisabled(true);
    m_filterByNotebookWidget.setDisabled(true);

    return true;
}

void NoteFiltersManager::setFilterByNotebooks()
{
    QNDEBUG(
        "widget::NoteFiltersManager",
        "NoteFiltersManager::setFilterByNotebooks");

    if (Q_UNLIKELY(m_noteModel.isNull())) {
        QNDEBUG("widget::NoteFiltersManager", "Note model is null");
        return;
    }

    // If this method got called in the first place, it means the notebook
    // filter is not overridden by either search string or saved search filters
    // so should enable the filter by notebook widget
    m_filterByNotebookWidget.setEnabled(true);

    auto notebookLocalIds =
        m_filterByNotebookWidget.localIdsOfItemsInFilter();

    QNTRACE(
        "widget::NoteFiltersManager",
        "Notebook local ids to be used for "
            << "filtering: "
            << (notebookLocalIds.isEmpty()
                    ? QStringLiteral("<empty>")
                    : notebookLocalIds.join(QStringLiteral(", "))));

    m_noteModel->setFilteredNotebookLocalIds(std::move(notebookLocalIds));
}

void NoteFiltersManager::setFilterByTags()
{
    QNDEBUG(
        "widget::NoteFiltersManager", "NoteFiltersManager::setFilterByTags");

    if (Q_UNLIKELY(m_noteModel.isNull())) {
        QNDEBUG("widget::NoteFiltersManager", "Note model is null");
        return;
    }

    // If this method got called in the first place, it means the tag filter is
    // not overridden by either search string or saved search filter so should
    // enable the filter by tag widget
    m_filterByTagWidget.setEnabled(true);

    auto tagLocalIds = m_filterByTagWidget.localIdsOfItemsInFilter();

    QNTRACE(
        "widget::NoteFiltersManager",
        "Tag local ids to be used for filtering: "
            << tagLocalIds.join(QStringLiteral(", ")));

    m_noteModel->setFilteredTagLocalIds(std::move(tagLocalIds));
}

void NoteFiltersManager::clearFilterWidgetsItems()
{
    QNDEBUG(
        "widget::NoteFiltersManager",
        "NoteFiltersManager::clearFilterWidgetsItems");

    clearFilterByTagWidgetItems();
    clearFilterByNotebookWidgetItems();
    clearFilterBySavedSearchWidget();
    clearFilterBySearchStringWidget();
}

void NoteFiltersManager::clearFilterByTagWidgetItems()
{
    QNDEBUG(
        "widget::NoteFiltersManager",
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
        "widget::NoteFiltersManager",
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
        "widget::NoteFiltersManager",
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
        "widget::NoteFiltersManager",
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
    QNDEBUG(
        "widget::NoteFiltersManager",
        "NoteFiltersManager::checkFiltersReadiness");

    if (m_isReady) {
        QNDEBUG(
            "widget::NoteFiltersManager",
            "Already marked the filter as ready once");
        return;
    }

    if (!m_filterByTagWidget.isReady()) {
        QNDEBUG(
            "widget::NoteFiltersManager",
            "Still pending the readiness of filter by tags");
        return;
    }

    if (!m_filterByNotebookWidget.isReady()) {
        QNDEBUG(
            "widget::NoteFiltersManager",
            "Still pending the readiness of filter by notebooks");
        return;
    }

    if (!m_filterBySavedSearchWidget.isReady()) {
        QNDEBUG(
            "widget::NoteFiltersManager",
            "Still pending the readiness of filter by saved search");
        return;
    }

    QNDEBUG("widget::NoteFiltersManager", "All filters are ready");

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

    const auto * notebookModel = m_filterByNotebookWidget.notebookModel();
    if (Q_UNLIKELY(!notebookModel)) {
        QNDEBUG(
            "widget::NoteFiltersManager",
            "Notebook model in the filter by notebook widget is null");
        return;
    }

    QVector<AbstractItemModel::ItemInfo> itemInfos;
    itemInfos.reserve(notebookLocalIds.size());

    for (const auto & notebookLocalId: std::as_const(notebookLocalIds)) {
        auto itemInfo = notebookModel->itemInfoForLocalId(notebookLocalId);
        if (itemInfo.m_localId.isEmpty()) {
            QNWARNING(
                "widget::NoteFiltersManager",
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

    for (const auto & itemInfo: std::as_const(itemInfos)) {
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

    const auto * tagModel = m_filterByTagWidget.tagModel();
    if (Q_UNLIKELY(!tagModel)) {
        QNDEBUG(
            "widget::NoteFiltersManager",
            "Tag model in the filter by tag widget is null");
        return;
    }

    QVector<AbstractItemModel::ItemInfo> itemInfos;
    itemInfos.reserve(tagLocalIds.size());

    for (const auto & tagLocalId: qAsConst(tagLocalIds)) {
        auto itemInfo = tagModel->itemInfoForLocalId(tagLocalId);
        if (Q_UNLIKELY(itemInfo.m_localId.isEmpty())) {
            QNWARNING(
                "widget::NoteFiltersManager",
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
    const auto * savedSearchModel =
        m_filterBySavedSearchWidget.savedSearchModel();

    if (Q_UNLIKELY(!savedSearchModel)) {
        QNDEBUG(
            "widget::NoteFiltersManager",
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
        savedSearchModel->queryForLocalId(savedSearchLocalId));
}

void NoteFiltersManager::checkAndRefreshNotesSearchQuery()
{
    QNDEBUG(
        "widget::NoteFiltersManager",
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
        "widget::NoteFiltersManager",
        "NoteFiltersManager::setAutomaticFilterByNotebook");

    if (notebookFilterWasCleared()) {
        QNDEBUG("widget::NoteFiltersManager", "Notebook filter was cleared");
        return true;
    }

    const auto * model = m_filterByNotebookWidget.notebookModel();
    if (!model) {
        QNDEBUG(
            "widget::NoteFiltersManager",
            "Notebook model is not set to filter by notebook widget yet");
        return false;
    }

    if (!model->allNotebooksListed()) {
        QNDEBUG(
            "widget::NoteFiltersManager",
            "Not all notebooks are listed yet by the notebook model");
        return false;
    }

    QString autoSelectedNotebookLocalId;
    const auto defaultNotebookIndex = model->defaultNotebookIndex();

    const auto * defaultNotebookModelItem =
        model->itemForIndex(defaultNotebookIndex);

    if (defaultNotebookModelItem) {
        const auto * defaultNotebookItem =
            defaultNotebookModelItem->cast<NotebookItem>();

        if (defaultNotebookItem) {
            autoSelectedNotebookLocalId = defaultNotebookItem->localId();
        }
    }

    if (autoSelectedNotebookLocalId.isEmpty()) {
        QNDEBUG(
            "widget::NoteFiltersManager",
            "No default notebook local id, trying just any notebook");

        const QStringList notebookNames = model->itemNames(QString());
        if (Q_UNLIKELY(notebookNames.isEmpty())) {
            QNDEBUG(
                "widget::NoteFiltersManager",
                "No notebooks within the notebook model");
            // NOTE: returning true because false is only for cases
            // in which the filter is waiting for something
            return true;
        }

        const QString & firstNotebookName = notebookNames.at(0);

        autoSelectedNotebookLocalId =
            model->localIdForItemName(firstNotebookName, {});
    }

    if (Q_UNLIKELY(autoSelectedNotebookLocalId.isEmpty())) {
        QNDEBUG(
            "widget::NoteFiltersManager",
            "Failed to find any notebook for automatic selection");
        // NOTE: returning true because false is only for cases
        // in which the filter is waiting for something
        return true;
    }

    const auto itemInfo =
        model->itemInfoForLocalId(autoSelectedNotebookLocalId);
    if (Q_UNLIKELY(itemInfo.m_localId.isEmpty())) {
        QNWARNING(
            "widget::NoteFiltersManager",
            "Failed fo find notebook item for auto selected local id "
                << autoSelectedNotebookLocalId);

        // NOTE: returning true because false is only for cases
        // in which the filter is waiting for something
        return true;
    }

    QNDEBUG(
        "widget::NoteFiltersManager",
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
        "widget::NoteFiltersManager",
        "NoteFiltersManager::persistFilterByNotebookClearedState: "
            << (state ? "true" : "false"));

    ApplicationSettings appSettings{
        m_account, preferences::keys::files::userInterface};

    appSettings.beginGroup(gNoteFiltersGroupKey);
    appSettings.setValue(gNotebookFilterClearedKey, state);
    appSettings.endGroup();
}

bool NoteFiltersManager::notebookFilterWasCleared() const
{
    ApplicationSettings appSettings{
        m_account, preferences::keys::files::userInterface};

    appSettings.beginGroup(gNoteFiltersGroupKey);
    const auto notebookFilterWasCleared =
        appSettings.value(gNotebookFilterClearedKey);
    appSettings.endGroup();

    if (!notebookFilterWasCleared.isValid()) {
        return false;
    }

    return notebookFilterWasCleared.toBool();
}

void NoteFiltersManager::persistFilterByTagClearedState(const bool state)
{
    QNDEBUG(
        "widget::NoteFiltersManager",
        "NoteFiltersManager::persistFilterByTagClearedState: "
            << (state ? "true" : "false"));

    ApplicationSettings appSettings{
        m_account, preferences::keys::files::userInterface};

    appSettings.beginGroup(gNoteFiltersGroupKey);
    appSettings.setValue(gTagFilterClearedKey, state);
    appSettings.endGroup();
}

bool NoteFiltersManager::tagFilterWasCleared() const
{
    ApplicationSettings appSettings{
        m_account, preferences::keys::files::userInterface};

    appSettings.beginGroup(gNoteFiltersGroupKey);
    const auto tagFilterWasCleared = appSettings.value(gTagFilterClearedKey);
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
        "widget::NoteFiltersManager",
        "NoteFiltersManager::persistFilterBySavedSearchClearedState: "
            << (state ? "true" : "false"));

    ApplicationSettings appSettings{
        m_account, preferences::keys::files::userInterface};

    appSettings.beginGroup(gNoteFiltersGroupKey);
    appSettings.setValue(gSavedSearchFilterClearedKey, state);
    appSettings.endGroup();
}

bool NoteFiltersManager::savedSearchFilterWasCleared() const
{
    ApplicationSettings appSettings{
        m_account, preferences::keys::files::userInterface};

    appSettings.beginGroup(gNoteFiltersGroupKey);

    const auto savedSearchFilterWasCleared =
        appSettings.value(gSavedSearchFilterClearedKey);

    appSettings.endGroup();

    if (!savedSearchFilterWasCleared.isValid()) {
        return false;
    }

    return savedSearchFilterWasCleared.toBool();
}

local_storage::NoteSearchQuery NoteFiltersManager::createNoteSearchQuery(
    const QString & searchString, ErrorString & errorDescription)
{
    if (searchString.isEmpty()) {
        errorDescription.clear();
        return local_storage::NoteSearchQuery{};
    }

    local_storage::NoteSearchQuery query;
    if (!query.setQueryString(searchString, errorDescription)) {
        QNDEBUG(
            "widget::NoteFiltersManager",
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

    auto * lineEdit = m_filterBySearchStringWidget.findChild<QLineEdit *>();
    if (lineEdit) {
        QToolTip::showText(
            lineEdit->mapToGlobal(QPoint(0, lineEdit->height())),
            errorDescription.localizedString(), lineEdit);
    }
}

utility::cancelers::ICancelerPtr NoteFiltersManager::setupCanceler()
{
    if (!m_canceler) {
        m_canceler = std::make_shared<utility::cancelers::ManualCanceler>();
    }

    return m_canceler;
}

} // namespace quentier
