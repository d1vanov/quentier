/*
 * Copyright 2016-2024 Dmitry Ivanov
 *
 * This file is part of Quentier.
 *
 * Quentier is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * Quentier is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Quentier. If not, see <http://www.gnu.org/licenses/>.
 */

#include "FavoritesModel.h"

#include <lib/exception/Utils.h>

#include <quentier/exception/InvalidArgument.h>
#include <quentier/local_storage/ILocalStorage.h>
#include <quentier/local_storage/ILocalStorageNotifier.h>
#include <quentier/logging/QuentierLogger.h>
#include <quentier/threading/Future.h>
#include <quentier/types/Validation.h>
#include <quentier/utility/Compat.h>

#include <algorithm>
#include <cstddef>
#include <utility>

// Limit for the queries to the local storage
#define NOTE_LIST_LIMIT         (40)
#define NOTEBOOK_LIST_LIMIT     (40)
#define TAG_LIST_LIMIT          (40)
#define SAVED_SEARCH_LIST_LIMIT (40)

namespace quentier {

namespace {

constexpr int gFavoritesModelColumnCount = 3;

} // namespace

FavoritesModel::FavoritesModel(
    Account account, local_storage::ILocalStoragePtr localStorage,
    NoteCache & noteCache, NotebookCache & notebookCache, TagCache & tagCache,
    SavedSearchCache & savedSearchCache, QObject * parent) :
    AbstractItemModel{account, parent},
    m_localStorage{std::move(localStorage)}, m_noteCache{noteCache},
    m_notebookCache{notebookCache}, m_tagCache{tagCache},
    m_savedSearchCache{savedSearchCache}
{
    if (Q_UNLIKELY(!m_localStorage)) {
        throw InvalidArgument{ErrorString{
            "FavoritesModel ctor: local storage is null"}};
    }

    connectToLocalStorageEvents(m_localStorage->notifier());

    requestNotebooksList();
    requestTagsList();
    requestNotesList();
    requestSavedSearchesList();
}

FavoritesModel::~FavoritesModel() {}

const FavoritesModelItem * FavoritesModel::itemForLocalId(
    const QString & localId) const
{
    const auto & localIdIndex = m_data.get<ByLocalId>();
    const auto it = localIdIndex.find(localId);
    if (Q_UNLIKELY(it == localIdIndex.end())) {
        QNDEBUG(
            "model::FavoritesModel",
            "Can't find favorites model item by local id: " << localId);
        return nullptr;
    }

    return &(*it);
}

const FavoritesModelItem * FavoritesModel::itemAtRow(const int row) const
{
    const auto & rowIndex = m_data.get<ByIndex>();
    if (Q_UNLIKELY((row < 0) || (rowIndex.size() <= static_cast<size_t>(row))))
    {
        QNDEBUG(
            "model::FavoritesModel",
            "Detected attempt to get the favorites model item for non-existing "
                << "row");
        return nullptr;
    }

    return &(rowIndex[static_cast<std::size_t>(row)]);
}

QModelIndex FavoritesModel::indexForLocalId(const QString & localId) const
{
    const auto & localIdIndex = m_data.get<ByLocalId>();
    const auto it = localIdIndex.find(localId);
    if (Q_UNLIKELY(it == localIdIndex.end())) {
        QNDEBUG(
            "model::FavoritesModel",
            "Can't find favorites model item by local id: " << localId);
        return {};
    }

    const auto & rowIndex = m_data.get<ByIndex>();
    const auto indexIt = m_data.project<ByIndex>(it);
    if (Q_UNLIKELY(indexIt == rowIndex.end())) {
        QNWARNING(
            "model::FavoritesModel",
            "Can't find indexed reference to the favorites model item: "
                << *it);
        return {};
    }

    const int row = static_cast<int>(std::distance(rowIndex.begin(), indexIt));
    return createIndex(row, static_cast<int>(Column::DisplayName));
}

QString FavoritesModel::itemNameForLocalId(const QString & localId) const
{
    const auto * item = itemForLocalId(localId);
    if (!item) {
        return {};
    }

    return item->displayName();
}

AbstractItemModel::ItemInfo FavoritesModel::itemInfoForLocalId(
    const QString & localId) const
{
    const auto * item = itemForLocalId(localId);
    if (!item) {
        return {};
    }

    ItemInfo info;
    info.m_localId = item->localId();
    info.m_name = item->localId();
    return info;
}

QString FavoritesModel::localIdForItemIndex(const QModelIndex & index) const
{
    if (!index.isValid()) {
        return {};
    }

    const int row = index.row();
    const int column = index.column();

    if ((row < 0) || (row >= static_cast<int>(m_data.size())) || (column < 0) ||
        (column >= gFavoritesModelColumnCount))
    {
        return {};
    }

    const auto & rowIndex = m_data.get<ByIndex>();
    const auto & item = rowIndex[static_cast<size_t>(index.row())];
    return item.localId();
}

Qt::ItemFlags FavoritesModel::flags(const QModelIndex & index) const
{
    Qt::ItemFlags indexFlags = QAbstractItemModel::flags(index);
    if (!index.isValid()) {
        return indexFlags;
    }

    if ((index.row() < 0) || (index.row() >= static_cast<int>(m_data.size())) ||
        (index.column() < 0) || (index.column() >= gFavoritesModelColumnCount))
    {
        return indexFlags;
    }

    indexFlags |= Qt::ItemIsSelectable;
    indexFlags |= Qt::ItemIsEnabled;

    const auto & rowIndex = m_data.get<ByIndex>();
    const auto & item = rowIndex.at(static_cast<std::size_t>(index.row()));

    switch (item.type()) {
    case FavoritesModelItem::Type::Notebook:
        if (canUpdateNotebook(item.localId())) {
            indexFlags |= Qt::ItemIsEditable;
        }
        break;
    case FavoritesModelItem::Type::Note:
        if (canUpdateNote(item.localId())) {
            indexFlags |= Qt::ItemIsEditable;
        }
        break;
    case FavoritesModelItem::Type::Tag:
        if (canUpdateTag(item.localId())) {
            indexFlags |= Qt::ItemIsEditable;
        }
        break;
    default:
        indexFlags |= Qt::ItemIsEditable;
        break;
    }

    return indexFlags;
}

QVariant FavoritesModel::data(const QModelIndex & index, int role) const
{
    if (!index.isValid()) {
        return {};
    }

    if ((index.row() < 0) || (index.row() >= static_cast<int>(m_data.size())) ||
        (index.column() < 0) || (index.column() >= gFavoritesModelColumnCount))
    {
        return {};
    }

    Column column;
    switch (static_cast<Column>(index.column())) {
    case Column::Type:
    case Column::DisplayName:
    case Column::NoteCount:
        column = static_cast<Column>(index.column());
        break;
    default:
        return {};
    }

    switch (role) {
    case Qt::DisplayRole:
    case Qt::EditRole:
    case Qt::ToolTipRole:
        return dataImpl(index.row(), column);
    case Qt::AccessibleTextRole:
    case Qt::AccessibleDescriptionRole:
        return dataAccessibleText(index.row(), column);
    default:
        return {};
    }
}

QVariant FavoritesModel::headerData(
    int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole) {
        return {};
    }

    if (orientation == Qt::Vertical) {
        return QVariant{section + 1};
    }

    switch (static_cast<Column>(section)) {
    case Column::Type:
        // TRANSLATOR: the type of the favorites model item - note, notebook,
        // TRANSLATOR: tag or saved search
        return QVariant(tr("Type"));
    case Column::DisplayName:
        // TRANSLATOR: the displayed name of the favorites model item - the name
        // TRANSLATOR: of a notebook or tag or saved search of the note's title
        return QVariant(tr("Name"));
    case Column::NoteCount:
        // TRANSLATOR: the number of items targeted by the favorites model item,
        // TRANSLATOR: in particular, the number of notes targeted by
        // the notebook TRANSLATOR: or tag
        return QVariant(tr("N items"));
    default:
        return {};
    }
}

int FavoritesModel::rowCount(const QModelIndex & parent) const
{
    if (parent.isValid()) {
        return 0;
    }

    return static_cast<int>(m_data.size());
}

int FavoritesModel::columnCount(const QModelIndex & parent) const
{
    if (parent.isValid()) {
        return 0;
    }

    return gFavoritesModelColumnCount;
}

QModelIndex FavoritesModel::index(
    int row, int column, const QModelIndex & parent) const
{
    if (parent.isValid()) {
        return {};
    }

    if ((row < 0) || (row >= static_cast<int>(m_data.size())) || (column < 0) ||
        (column >= gFavoritesModelColumnCount))
    {
        return {};
    }

    return createIndex(row, column);
}

QModelIndex FavoritesModel::parent(const QModelIndex & index) const
{
    Q_UNUSED(index)
    return {};
}

bool FavoritesModel::setHeaderData(
    int section, Qt::Orientation orientation, const QVariant & value, int role)
{
    Q_UNUSED(section)
    Q_UNUSED(orientation)
    Q_UNUSED(value)
    Q_UNUSED(role)
    return false;
}

bool FavoritesModel::setData(
    const QModelIndex & index, const QVariant & value, int role)
{
    if (!index.isValid()) {
        return false;
    }

    if (role != Qt::EditRole) {
        return false;
    }

    if ((index.row() < 0) || (index.row() >= static_cast<int>(m_data.size())) ||
        (index.column() < 0) || (index.column() >= gFavoritesModelColumnCount))
    {
        return false;
    }

    auto & rowIndex = m_data.get<ByIndex>();
    auto item = rowIndex.at(static_cast<std::size_t>(index.row()));

    switch (item.type()) {
    case FavoritesModelItem::Type::Notebook:
        if (!canUpdateNotebook(item.localId())) {
            return false;
        }
        break;
    case FavoritesModelItem::Type::Note:
        if (!canUpdateNote(item.localId())) {
            return false;
        }
        break;
    case FavoritesModelItem::Type::Tag:
        if (!canUpdateTag(item.localId())) {
            return false;
        }
        break;
    default:
        break;
    }

    switch (static_cast<Column>(index.column())) {
    case Column::DisplayName:
    {
        const QString newDisplayName = value.toString().trimmed();
        const bool changed = (item.displayName() != newDisplayName);
        if (!changed) {
            return true;
        }

        switch (item.type()) {
        case FavoritesModelItem::Type::Notebook:
        {
            auto it = m_lowerCaseNotebookNames.find(newDisplayName.toLower());
            if (it != m_lowerCaseNotebookNames.end()) {
                ErrorString error{
                    QT_TR_NOOP("Can't rename the notebook: no two "
                               "notebooks within the account are "
                               "allowed to have the same name in "
                               "case-insensitive manner")};
                error.details() = newDisplayName;
                QNINFO("model::FavoritesModel", error);
                Q_EMIT notifyError(std::move(error));
                return false;
            }

            ErrorString errorDescription;

            if (!validateNotebookName(newDisplayName, &errorDescription)) {
                ErrorString error{QT_TR_NOOP("Can't rename the notebook")};
                error.appendBase(errorDescription.base());
                error.appendBase(errorDescription.additionalBases());
                error.details() = errorDescription.details();

                QNINFO(
                    "model::FavoritesModel",
                    error << ", suggested new "
                          << "name = " << newDisplayName);

                Q_EMIT notifyError(std::move(error));
                return false;
            }

            // Removing the previous name and inserting the new one
            it = m_lowerCaseNotebookNames.find(item.displayName().toLower());

            if (Q_LIKELY(it != m_lowerCaseNotebookNames.end())) {
                m_lowerCaseNotebookNames.erase(it);
            }

            m_lowerCaseNotebookNames.insert(newDisplayName.toLower());
            break;
        }
        case FavoritesModelItem::Type::Tag:
        {
            auto it = m_lowerCaseTagNames.find(newDisplayName.toLower());
            if (it != m_lowerCaseTagNames.end()) {
                ErrorString error{
                    QT_TR_NOOP("Can't rename the tag: no two tags "
                               "within the account are allowed to have "
                               "the same name in case-insensitive "
                               "manner")};
                QNINFO("model::FavoritesModel", error);
                Q_EMIT notifyError(std::move(error));
                return false;
            }

            ErrorString errorDescription;
            if (!validateTagName(newDisplayName, &errorDescription)) {
                ErrorString error{QT_TR_NOOP("Can't rename the tag")};
                error.appendBase(errorDescription.base());
                error.appendBase(errorDescription.additionalBases());
                error.details() = errorDescription.details();

                QNINFO(
                    "model::FavoritesModel",
                    error << ", suggested new "
                          << "name = " << newDisplayName);
                Q_EMIT notifyError(std::move(error));
                return false;
            }

            // Removing the previous name and inserting the new one
            it = m_lowerCaseTagNames.find(item.displayName().toLower());
            if (Q_LIKELY(it != m_lowerCaseTagNames.end())) {
                m_lowerCaseTagNames.erase(it);
            }

            m_lowerCaseTagNames.insert(newDisplayName.toLower());
            break;
        }
        case FavoritesModelItem::Type::SavedSearch:
        {
            auto it =
                m_lowerCaseSavedSearchNames.find(newDisplayName.toLower());

            if (it != m_lowerCaseSavedSearchNames.end()) {
                ErrorString error{
                    QT_TR_NOOP("Can't rename the saved search: no two saved "
                               "searches within the account are allowed to "
                               "have the same name in case-insensitive "
                               "manner")};

                QNINFO("model::FavoritesModel", error);
                Q_EMIT notifyError(std::move(error));
                return false;
            }

            ErrorString errorDescription;
            if (!validateSavedSearchName(newDisplayName, &errorDescription)) {
                ErrorString error{QT_TR_NOOP("Can't rename the saved search")};
                error.appendBase(errorDescription.base());
                error.appendBase(errorDescription.additionalBases());
                error.details() = errorDescription.details();

                QNINFO(
                    "model::FavoritesModel",
                    error << ", suggested new "
                          << "name = " << newDisplayName);

                Q_EMIT notifyError(std::move(error));
                return false;
            }

            // Removing the previous name and inserting the new one
            it = m_lowerCaseSavedSearchNames.find(item.displayName().toLower());

            if (Q_LIKELY(it != m_lowerCaseSavedSearchNames.end())) {
                m_lowerCaseSavedSearchNames.erase(it);
            }

            m_lowerCaseSavedSearchNames.insert(newDisplayName.toLower());
            break;
        }
        default:
            break;
        }

        item.setDisplayName(newDisplayName);
        break;
    }
    default:
        return false;
    }

    rowIndex.replace(rowIndex.begin() + index.row(), item);
    Q_EMIT dataChanged(index, index);

    updateItemRowWithRespectToSorting(item);
    updateItemInLocalStorage(item);
    return true;
}

bool FavoritesModel::insertRows(int row, int count, const QModelIndex & parent)
{
    // Not implemented for this model
    Q_UNUSED(row)
    Q_UNUSED(count)
    Q_UNUSED(parent)
    return false;
}

bool FavoritesModel::removeRows(int row, int count, const QModelIndex & parent)
{
    if (Q_UNLIKELY(parent.isValid())) {
        QNDEBUG(
            "model::FavoritesModel",
            "Ignoring the attempt to remove rows from "
                << "favorites model for valid parent model index");
        return false;
    }

    if (Q_UNLIKELY((row + count - 1) >= static_cast<int>(m_data.size()))) {
        ErrorString error{
            QT_TR_NOOP("Detected attempt to remove more rows than "
                       "the favorites model contains")};

        QNINFO(
            "model::FavoritesModel",
            error << ", row = " << row << ", count = " << count
                  << ", number of favorites model items = " << m_data.size());

        Q_EMIT notifyError(std::move(error));
        return false;
    }

    auto & rowIndex = m_data.get<ByIndex>();

    beginRemoveRows(QModelIndex(), row, row + count - 1);

    QStringList notebookLocalIdsToRemove;
    QStringList noteLocalIdsToRemove;
    QStringList tagLocalIdsToRemove;
    QStringList savedSearchLocalIdsToRemove;

    // NOTE: just a wild guess about how many items of each kind we can meet
    notebookLocalIdsToRemove.reserve(count / 4);
    noteLocalIdsToRemove.reserve(count / 4);
    tagLocalIdsToRemove.reserve(count / 4);
    savedSearchLocalIdsToRemove.reserve(count / 4);

    for (int i = 0; i < count; ++i) {
        auto it = rowIndex.begin() + row + i;
        const FavoritesModelItem & item = *it;

        switch (item.type()) {
        case FavoritesModelItem::Type::Notebook:
            notebookLocalIdsToRemove << item.localId();
            break;
        case FavoritesModelItem::Type::Note:
            noteLocalIdsToRemove << item.localId();
            break;
        case FavoritesModelItem::Type::Tag:
            tagLocalIdsToRemove << item.localId();
            break;
        case FavoritesModelItem::Type::SavedSearch:
            savedSearchLocalIdsToRemove << item.localId();
            break;
        default:
        {
            QNDEBUG(
                "model::FavoritesModel",
                "Favorites model item with unknown type detected: " << item);
            break;
        }
        }
    }

    rowIndex.erase(rowIndex.begin() + row, rowIndex.begin() + row + count);

    for (const auto & localId: std::as_const(notebookLocalIdsToRemove)) {
        unfavoriteNotebook(localId);
    }

    for (const auto & localId: std::as_const(noteLocalIdsToRemove)) {
        unfavoriteNote(localId);
    }

    for (const auto & localId: std::as_const(tagLocalIdsToRemove)) {
        unfavoriteTag(localId);
    }

    for (const auto & localId: std::as_const(savedSearchLocalIdsToRemove)) {
        unfavoriteSavedSearch(localId);
    }

    endRemoveRows();
    return true;
}

void FavoritesModel::sort(int column, Qt::SortOrder order)
{
    QNDEBUG(
        "model::FavoritesModel",
        "FavoritesModel::sort: column = "
            << column << ", order = " << order << " ("
            << (order == Qt::AscendingOrder ? "ascending" : "descending")
            << ")");

    if (Q_UNLIKELY((column < 0) || (column >= gFavoritesModelColumnCount))) {
        return;
    }

    FavoritesDataByIndex & rowIndex = m_data.get<ByIndex>();

    if (column == static_cast<int>(m_sortedColumn)) {
        if (order == m_sortOrder) {
            QNDEBUG(
                "model::FavoritesModel",
                "Neither sorted column nor sort order have changed, nothing to "
                    << "do");
            return;
        }

        m_sortOrder = order;

        QNDEBUG(
            "model::FavoritesModel",
            "Only the sort order has changed, reversing the index");

        Q_EMIT layoutAboutToBeChanged();
        rowIndex.reverse();
        Q_EMIT layoutChanged();
        return;
    }

    m_sortedColumn = static_cast<Column>(column);
    m_sortOrder = order;

    Q_EMIT layoutAboutToBeChanged();

    auto persistentIndices = persistentIndexList();
    QList<std::pair<QString, int>> localIdsToUpdateWithColumns;
    localIdsToUpdateWithColumns.reserve(persistentIndices.size());

    QStringList localIdsToUpdate;
    for (const auto & modelIndex: std::as_const(persistentIndices)) {
        const int column = modelIndex.column();
        if (!modelIndex.isValid()) {
            localIdsToUpdateWithColumns
                << std::pair<QString, int>(QString{}, column);
            continue;
        }

        const int row = modelIndex.row();
        if ((row < 0) || (row >= static_cast<int>(m_data.size())) ||
            (column < 0) || (column >= gFavoritesModelColumnCount))
        {
            localIdsToUpdateWithColumns
                << std::pair<QString, int>(QString{}, column);
        }

        const auto itemIt = rowIndex.begin() + row;
        const auto & item = *itemIt;
        localIdsToUpdateWithColumns << std::make_pair(item.localId(), column);
    }

    std::vector<boost::reference_wrapper<const FavoritesModelItem>> items(
        rowIndex.begin(), rowIndex.end());

    std::sort(
        items.begin(), items.end(), Comparator(m_sortedColumn, m_sortOrder));

    rowIndex.rearrange(items.begin());

    QModelIndexList replacementIndices;

    replacementIndices.reserve(
        std::max(localIdsToUpdateWithColumns.size(), 0));

    for (const auto & pair: std::as_const(localIdsToUpdateWithColumns)) {
        const QString & localId = pair.first;
        const int column = pair.second;

        if (localId.isEmpty()) {
            replacementIndices << QModelIndex();
            continue;
        }

        const auto newIndex = indexForLocalId(localId);
        if (!newIndex.isValid()) {
            replacementIndices << QModelIndex();
            continue;
        }

        const auto newIndexWithColumn = createIndex(newIndex.row(), column);
        replacementIndices << newIndexWithColumn;
    }

    changePersistentIndexList(persistentIndices, replacementIndices);

    Q_EMIT layoutChanged();
}

// FIXME: remove these methods when they are no longer necessary
/*
void FavoritesModel::onFindNoteComplete(
    Note note, LocalStorageManager::GetNoteOptions options, QUuid requestId)
{
    auto restoreUpdateIt =
        m_findNoteToRestoreFailedUpdateRequestIds.find(requestId);

    auto performUpdateIt = m_findNoteToPerformUpdateRequestIds.find(requestId);
    auto unfavoriteIt = m_findNoteToUnfavoriteRequestIds.find(requestId);

    if ((restoreUpdateIt == m_findNoteToRestoreFailedUpdateRequestIds.end()) &&
        (performUpdateIt == m_findNoteToPerformUpdateRequestIds.end()) &&
        (unfavoriteIt == m_findNoteToUnfavoriteRequestIds.end()))
    {
        return;
    }

    QNDEBUG(
        "model::FavoritesModel",
        "FavoritesModel::onFindNoteComplete: note = "
            << note << ", with resource metadata = "
            << ((options &
                 LocalStorageManager::GetNoteOption::WithResourceMetadata)
                    ? "true"
                    : "false")
            << ", with resource binary data = "
            << ((options &
                 LocalStorageManager::GetNoteOption::WithResourceBinaryData)
                    ? "true"
                    : "false")
            << ", request id = " << requestId);

    if (restoreUpdateIt != m_findNoteToRestoreFailedUpdateRequestIds.end()) {
        Q_UNUSED(
            m_findNoteToRestoreFailedUpdateRequestIds.erase(restoreUpdateIt))

        onNoteAddedOrUpdated(note);
    }
    else if (performUpdateIt != m_findNoteToPerformUpdateRequestIds.end()) {
        Q_UNUSED(m_findNoteToPerformUpdateRequestIds.erase(performUpdateIt))
        m_noteCache.put(note.localId(), note);

        FavoritesDataByLocalId & localIdIndex = m_data.get<ByLocalId>();
        auto it = localIdIndex.find(note.localId());
        if (it != localIdIndex.end()) {
            updateItemInLocalStorage(*it);
        }
    }
    else if (unfavoriteIt != m_findNoteToUnfavoriteRequestIds.end()) {
        Q_UNUSED(m_findNoteToUnfavoriteRequestIds.erase(unfavoriteIt))
        m_noteCache.put(note.localId(), note);
        unfavoriteNote(note.localId());
    }
}

void FavoritesModel::onFindNoteFailed(
    Note note, LocalStorageManager::GetNoteOptions options,
    ErrorString errorDescription, QUuid requestId)
{
    auto restoreUpdateIt =
        m_findNoteToRestoreFailedUpdateRequestIds.find(requestId);

    auto performUpdateIt = m_findNoteToPerformUpdateRequestIds.find(requestId);
    auto unfavoriteIt = m_findNoteToUnfavoriteRequestIds.find(requestId);

    if ((restoreUpdateIt == m_findNoteToRestoreFailedUpdateRequestIds.end()) &&
        (performUpdateIt == m_findNoteToPerformUpdateRequestIds.end()) &&
        (unfavoriteIt == m_findNoteToUnfavoriteRequestIds.end()))
    {
        return;
    }

    QNDEBUG(
        "model::FavoritesModel",
        "FavoritesModel::onFindNoteFailed: note = "
            << note << ", with resource metadata = "
            << ((options &
                 LocalStorageManager::GetNoteOption::WithResourceMetadata)
                    ? "true"
                    : "false")
            << ", with resource binary data = "
            << ((options &
                 LocalStorageManager::GetNoteOption::WithResourceBinaryData)
                    ? "true"
                    : "false")
            << ", error description: " << errorDescription
            << ", request id = " << requestId);

    if (restoreUpdateIt != m_findNoteToRestoreFailedUpdateRequestIds.end()) {
        Q_UNUSED(
            m_findNoteToRestoreFailedUpdateRequestIds.erase(restoreUpdateIt))
    }
    else if (performUpdateIt != m_findNoteToPerformUpdateRequestIds.end()) {
        Q_UNUSED(m_findNoteToPerformUpdateRequestIds.erase(performUpdateIt))
    }
    else if (unfavoriteIt != m_findNoteToUnfavoriteRequestIds.end()) {
        Q_UNUSED(m_findNoteToUnfavoriteRequestIds.erase(unfavoriteIt))
    }

    Q_EMIT notifyError(errorDescription);
}

void FavoritesModel::onFindNotebookComplete(Notebook notebook, QUuid requestId)
{
    auto restoreUpdateIt =
        m_findNotebookToRestoreFailedUpdateRequestIds.find(requestId);

    auto performUpdateIt =
        m_findNotebookToPerformUpdateRequestIds.find(requestId);

    auto unfavoriteIt = m_findNotebookToUnfavoriteRequestIds.find(requestId);

    if ((restoreUpdateIt ==
         m_findNotebookToRestoreFailedUpdateRequestIds.end()) &&
        (performUpdateIt == m_findNotebookToPerformUpdateRequestIds.end()) &&
        (unfavoriteIt == m_findNotebookToUnfavoriteRequestIds.end()))
    {
        return;
    }

    QNDEBUG(
        "model::FavoritesModel",
        "FavoritesModel::onFindNotebookComplete: "
            << "notebook = " << notebook << "\nRequest id = " << requestId);

    if (restoreUpdateIt != m_findNotebookToRestoreFailedUpdateRequestIds.end())
    {
        Q_UNUSED(m_findNotebookToRestoreFailedUpdateRequestIds.erase(
            restoreUpdateIt))

        onNotebookAddedOrUpdated(notebook);
    }
    else if (performUpdateIt != m_findNotebookToPerformUpdateRequestIds.end()) {
        Q_UNUSED(m_findNotebookToPerformUpdateRequestIds.erase(performUpdateIt))
        m_notebookCache.put(notebook.localId(), notebook);

        auto & localIdIndex = m_data.get<ByLocalId>();
        auto it = localIdIndex.find(notebook.localId());
        if (it != localIdIndex.end()) {
            updateItemInLocalStorage(*it);
        }
    }
    else if (unfavoriteIt != m_findNotebookToUnfavoriteRequestIds.end()) {
        Q_UNUSED(m_findNotebookToPerformUpdateRequestIds.erase(performUpdateIt))
        m_notebookCache.put(notebook.localId(), notebook);
        unfavoriteNotebook(notebook.localId());
    }
}

void FavoritesModel::onFindNotebookFailed(
    Notebook notebook, ErrorString errorDescription, QUuid requestId)
{
    auto restoreUpdateIt =
        m_findNotebookToRestoreFailedUpdateRequestIds.find(requestId);

    auto performUpdateIt =
        m_findNotebookToPerformUpdateRequestIds.find(requestId);

    auto unfavoriteIt = m_findNotebookToUnfavoriteRequestIds.find(requestId);

    if ((restoreUpdateIt ==
         m_findNotebookToRestoreFailedUpdateRequestIds.end()) &&
        (performUpdateIt == m_findNotebookToPerformUpdateRequestIds.end()) &&
        (unfavoriteIt == m_findNotebookToUnfavoriteRequestIds.end()))
    {
        return;
    }

    QNDEBUG(
        "model::FavoritesModel",
        "FavoritesModel::onFindNotebookFailed: "
            << "notebook = " << notebook << "\nError description = "
            << errorDescription << ", request id = " << requestId);

    if (restoreUpdateIt != m_findNotebookToRestoreFailedUpdateRequestIds.end())
    {
        Q_UNUSED(m_findNotebookToRestoreFailedUpdateRequestIds.erase(
            restoreUpdateIt))
    }
    else if (performUpdateIt != m_findNotebookToPerformUpdateRequestIds.end()) {
        Q_UNUSED(m_findNotebookToPerformUpdateRequestIds.erase(performUpdateIt))
    }
    else if (unfavoriteIt != m_findNotebookToUnfavoriteRequestIds.end()) {
        Q_UNUSED(m_findNotebookToUnfavoriteRequestIds.erase(unfavoriteIt))
    }

    Q_EMIT notifyError(errorDescription);
}

void FavoritesModel::onUpdateTagFailed(
    Tag tag, ErrorString errorDescription, QUuid requestId)
{
    auto it = m_updateTagRequestIds.find(requestId);
    if (it == m_updateTagRequestIds.end()) {
        return;
    }

    QNDEBUG(
        "model::FavoritesModel",
        "FavoritesModel::onUpdateTagFailed: tag = "
            << tag << "\nError description = " << errorDescription
            << ", request id = " << requestId);

    Q_UNUSED(m_updateTagRequestIds.erase(it))

    requestId = QUuid::createUuid();
    Q_UNUSED(m_findTagToRestoreFailedUpdateRequestIds.insert(requestId))

    QNTRACE(
        "model::FavoritesModel",
        "Emitting the request to find a tag: "
            << "local id = " << tag.localId()
            << ", request id = " << requestId);

    Q_EMIT findTag(tag, requestId);
}

void FavoritesModel::onFindTagComplete(Tag tag, QUuid requestId)
{
    auto restoreUpdateIt =
        m_findTagToRestoreFailedUpdateRequestIds.find(requestId);

    auto performUpdateIt = m_findTagToPerformUpdateRequestIds.find(requestId);
    auto unfavoriteIt = m_findTagToUnfavoriteRequestIds.find(requestId);

    if ((restoreUpdateIt == m_findTagToRestoreFailedUpdateRequestIds.end()) &&
        (performUpdateIt == m_findTagToPerformUpdateRequestIds.end()) &&
        (unfavoriteIt == m_findTagToUnfavoriteRequestIds.end()))
    {
        return;
    }

    QNDEBUG(
        "model::FavoritesModel",
        "FavoritesModel::onFindTagComplete: tag = " << tag << "\nRequest id = "
                                                    << requestId);

    if (restoreUpdateIt != m_findTagToRestoreFailedUpdateRequestIds.end()) {
        Q_UNUSED(
            m_findTagToRestoreFailedUpdateRequestIds.erase(restoreUpdateIt))

        onTagAddedOrUpdated(tag);
    }
    else if (performUpdateIt != m_findTagToPerformUpdateRequestIds.end()) {
        Q_UNUSED(m_findTagToPerformUpdateRequestIds.erase(performUpdateIt))
        m_tagCache.put(tag.localId(), tag);

        auto & localIdIndex = m_data.get<ByLocalId>();
        auto it = localIdIndex.find(tag.localId());
        if (it != localIdIndex.end()) {
            updateItemInLocalStorage(*it);
        }
    }
    else if (unfavoriteIt != m_findTagToUnfavoriteRequestIds.end()) {
        Q_UNUSED(m_findTagToPerformUpdateRequestIds.erase(performUpdateIt))
        m_tagCache.put(tag.localId(), tag);
        unfavoriteTag(tag.localId());
    }
}

void FavoritesModel::onFindTagFailed(
    Tag tag, ErrorString errorDescription, QUuid requestId)
{
    auto restoreUpdateIt =
        m_findTagToRestoreFailedUpdateRequestIds.find(requestId);

    auto performUpdateIt = m_findTagToPerformUpdateRequestIds.find(requestId);
    auto unfavoriteIt = m_findTagToUnfavoriteRequestIds.find(requestId);

    if ((restoreUpdateIt == m_findTagToRestoreFailedUpdateRequestIds.end()) &&
        (performUpdateIt == m_findTagToPerformUpdateRequestIds.end()) &&
        (unfavoriteIt == m_findTagToUnfavoriteRequestIds.end()))
    {
        return;
    }

    QNDEBUG(
        "model::FavoritesModel",
        "FavoritesModel::onFindTagFailed: tag = "
            << tag << "\nError description = " << errorDescription
            << ", request id = " << requestId);

    if (restoreUpdateIt != m_findTagToRestoreFailedUpdateRequestIds.end()) {
        Q_UNUSED(
            m_findTagToRestoreFailedUpdateRequestIds.erase(restoreUpdateIt))
    }
    else if (performUpdateIt != m_findTagToPerformUpdateRequestIds.end()) {
        Q_UNUSED(m_findTagToPerformUpdateRequestIds.erase(performUpdateIt))
    }
    else if (unfavoriteIt != m_findTagToUnfavoriteRequestIds.end()) {
        Q_UNUSED(m_findTagToUnfavoriteRequestIds.erase(unfavoriteIt))
    }

    Q_EMIT notifyError(errorDescription);
}

void FavoritesModel::onUpdateSavedSearchFailed(
    SavedSearch search, ErrorString errorDescription, QUuid requestId)
{
    auto it = m_updateSavedSearchRequestIds.find(requestId);
    if (it == m_updateSavedSearchRequestIds.end()) {
        return;
    }

    QNDEBUG(
        "model::FavoritesModel",
        "FavoritesModel::onUpdateSavedSearchFailed: "
            << "search = " << search << "\nError description = "
            << errorDescription << ", request id = " << requestId);

    Q_UNUSED(m_updateSavedSearchRequestIds.erase(it))

    requestId = QUuid::createUuid();
    Q_UNUSED(m_findSavedSearchToRestoreFailedUpdateRequestIds.insert(requestId))

    QNTRACE(
        "model::FavoritesModel",
        "Emitting the request to find the saved search: "
            << "local id = " << search.localId()
            << ", request id = " << requestId);

    Q_EMIT findSavedSearch(search, requestId);
}

void FavoritesModel::onFindSavedSearchComplete(
    SavedSearch search, QUuid requestId)
{
    auto restoreUpdateIt =
        m_findSavedSearchToRestoreFailedUpdateRequestIds.find(requestId);

    auto performUpdateIt =
        m_findSavedSearchToPerformUpdateRequestIds.find(requestId);

    auto unfavoriteIt = m_findSavedSearchToUnfavoriteRequestIds.find(requestId);

    if ((restoreUpdateIt ==
         m_findSavedSearchToRestoreFailedUpdateRequestIds.end()) &&
        (performUpdateIt == m_findSavedSearchToPerformUpdateRequestIds.end()) &&
        (unfavoriteIt == m_findSavedSearchToUnfavoriteRequestIds.end()))
    {
        return;
    }

    QNDEBUG(
        "model::FavoritesModel",
        "FavoritesModel::onFindSavedSearchComplete: "
            << "search = " << search << "\nRequest id = " << requestId);

    if (restoreUpdateIt !=
        m_findSavedSearchToRestoreFailedUpdateRequestIds.end()) {
        Q_UNUSED(m_findSavedSearchToRestoreFailedUpdateRequestIds.erase(
            restoreUpdateIt))

        onSavedSearchAddedOrUpdated(search);
    }
    else if (
        performUpdateIt != m_findSavedSearchToPerformUpdateRequestIds.end()) {
        Q_UNUSED(
            m_findSavedSearchToPerformUpdateRequestIds.erase(performUpdateIt))

        m_savedSearchCache.put(search.localId(), search);

        auto & localIdIndex = m_data.get<ByLocalId>();
        auto it = localIdIndex.find(search.localId());
        if (it != localIdIndex.end()) {
            updateItemInLocalStorage(*it);
        }
    }
    else if (unfavoriteIt != m_findSavedSearchToUnfavoriteRequestIds.end()) {
        Q_UNUSED(
            m_findSavedSearchToPerformUpdateRequestIds.erase(performUpdateIt))

        m_savedSearchCache.put(search.localId(), search);
        unfavoriteSavedSearch(search.localId());
    }
}

void FavoritesModel::onFindSavedSearchFailed(
    SavedSearch search, ErrorString errorDescription, QUuid requestId)
{
    auto restoreUpdateIt =
        m_findSavedSearchToRestoreFailedUpdateRequestIds.find(requestId);

    auto performUpdateIt =
        m_findSavedSearchToPerformUpdateRequestIds.find(requestId);

    auto unfavoriteIt = m_findSavedSearchToUnfavoriteRequestIds.find(requestId);

    if ((restoreUpdateIt ==
         m_findSavedSearchToRestoreFailedUpdateRequestIds.end()) &&
        (performUpdateIt == m_findSavedSearchToPerformUpdateRequestIds.end()) &&
        (unfavoriteIt == m_findSavedSearchToUnfavoriteRequestIds.end()))
    {
        return;
    }

    QNWARNING(
        "model::FavoritesModel",
        "FavoritesModel::onFindSavedSearchFailed: "
            << "search = " << search << "\nError description = "
            << errorDescription << ", request id = " << requestId);

    if (restoreUpdateIt !=
        m_findSavedSearchToRestoreFailedUpdateRequestIds.end()) {
        Q_UNUSED(m_findSavedSearchToRestoreFailedUpdateRequestIds.erase(
            restoreUpdateIt))
    }
    else if (
        performUpdateIt != m_findSavedSearchToPerformUpdateRequestIds.end()) {
        Q_UNUSED(
            m_findSavedSearchToPerformUpdateRequestIds.erase(performUpdateIt))
    }
    else if (unfavoriteIt != m_findSavedSearchToUnfavoriteRequestIds.end()) {
        Q_UNUSED(m_findSavedSearchToUnfavoriteRequestIds.erase(unfavoriteIt))
    }

    Q_EMIT notifyError(errorDescription);
}
*/

void FavoritesModel::connectToLocalStorageEvents(
    local_storage::ILocalStorageNotifier * notifier)
{
    QObject::connect(
        notifier, &local_storage::ILocalStorageNotifier::notePut, this,
        [this](const qevercloud::Note & note) {
            onNoteAddedOrUpdated(note, NoteUpdate::WithTags);
        });

    QObject::connect(
        notifier, &local_storage::ILocalStorageNotifier::noteUpdated, this,
        [this](
            const qevercloud::Note & note,
            const local_storage::ILocalStorage::UpdateNoteOptions options) {
            onNoteAddedOrUpdated(
                note,
                options.testFlag(
                    local_storage::ILocalStorage::UpdateNoteOption::UpdateTags)
                    ? NoteUpdate::WithTags
                    : NoteUpdate::WithoutTags);
        });

    QObject::connect(
        notifier, &local_storage::ILocalStorageNotifier::noteExpunged, this,
        [this](const QString & noteLocalId) {
            removeItemByLocalId(noteLocalId);

            // Since it's unclear whether some notebook within the favorites
            // model was affected, need to check and re-subscribe to note counts
            // for all notebooks
            requestNoteCountForAllNotebooks(NoteCountRequestOption::Force);
            requestNoteCountForAllTags(NoteCountRequestOption::Force);
        });

    QObject::connect(
        notifier, &local_storage::ILocalStorageNotifier::notebookPut, this,
        [this](const qevercloud::Notebook & notebook) {
            onNotebookAddedOrUpdated(notebook);
        });

    QObject::connect(
        notifier, &local_storage::ILocalStorageNotifier::notebookExpunged, this,
        [this](const QString & notebookLocalId) {
            removeItemByLocalId(notebookLocalId);
        });

    QObject::connect(
        notifier, &local_storage::ILocalStorageNotifier::tagPut, this,
        [this](const qevercloud::Tag & tag) {
            onTagAddedOrUpdated(tag);
        });

    QObject::connect(
        notifier, &local_storage::ILocalStorageNotifier::tagExpunged, this,
        [this](const QString & tagLocalId, const QStringList & childTagLocalIds)
        {
            for (const auto & localId: std::as_const(childTagLocalIds)) {
                removeItemByLocalId(localId);
            }

            removeItemByLocalId(tagLocalId);
        });

    QObject::connect(
        notifier, &local_storage::ILocalStorageNotifier::savedSearchPut, this,
        [this](const qevercloud::SavedSearch & savedSearch) {
            onSavedSearchAddedOrUpdated(savedSearch);
        });

    QObject::connect(
        notifier, &local_storage::ILocalStorageNotifier::savedSearchExpunged,
        this,
        [this](const QString & localId) {
            removeItemByLocalId(localId);
        });
}

void FavoritesModel::requestNotesList()
{
    QNDEBUG(
        "model::FavoritesModel",
        "FavoritesModel::requestNotesList: offset = " << m_listNotesOffset);

    local_storage::ILocalStorage::ListNotesOptions options;
    options.m_filters.m_locallyFavoritedFilter =
        local_storage::ILocalStorage::ListObjectsFilter::Include;
    options.m_order = local_storage::ILocalStorage::ListNotesOrder::NoOrder;
    options.m_direction =
        local_storage::ILocalStorage::OrderDirection::Ascending;
    options.m_offset = m_listNotesOffset;

    QNTRACE(
        "model::FavoritesModel",
        "Requesting a list of notes: offset = " << m_listNotesOffset);

    auto listNotesFuture = m_localStorage->listNotes(
        local_storage::ILocalStorage::FetchNoteOptions{}, options);

    auto listNotesThenFuture = threading::then(
        std::move(listNotesFuture), this,
        [this](const QList<qevercloud::Note> & notes) {
            for (const auto & note: std::as_const(notes)) {
                onNoteAddedOrUpdated(note, NoteUpdate::WithTags);
            }

            if (!notes.isEmpty()) {
                QNTRACE(
                    "model::FavoritesModel",
                    "The number of found notes is greater than zero, "
                        << "requesting more notes from local storage");
                m_listNotesOffset += static_cast<quint64>(notes.size());
                requestNotesList();
                return;
            }

            checkAllItemsListed();
        });

    threading::onFailed(
        std::move(listNotesThenFuture), this,
        [this](const QException & e) {
            auto message = exceptionMessage(e);
            ErrorString error{QT_TR_NOOP(
                "Failed to list notes from local storage")};
            error.appendBase(message.base());
            error.appendBase(message.additionalBases());
            error.details() = message.details();
            QNWARNING("model::FavoritesModel", error);
            Q_EMIT notifyError(std::move(error));
        });
}

void FavoritesModel::requestNotebooksList()
{
    QNDEBUG(
        "model::FavoritesModel",
        "FavoritesModel::requestNotebooksList: offset = "
            << m_listNotebooksOffset);

    // NOTE: the subscription to all notebooks is necessary in order to receive
    // the information about the restrictions for various notebooks + for
    // the collection of notebook names to forbid any two notebooks within
    // the account to have the same name in a case-insensitive manner
    local_storage::ILocalStorage::ListNotebooksOptions options;
    options.m_order = local_storage::ILocalStorage::ListNotebooksOrder::NoOrder;
    options.m_direction =
        local_storage::ILocalStorage::OrderDirection::Ascending;

    auto listNotebooksFuture = m_localStorage->listNotebooks(options);
    auto listNotebooksThenFuture = threading::then(
        std::move(listNotebooksFuture), this,
        [this](const QList<qevercloud::Notebook> & notebooks) {
            for (const auto & notebook: std::as_const(notebooks)) {
                onNotebookAddedOrUpdated(notebook);
            }

            if (!notebooks.isEmpty()) {
                QNTRACE(
                    "model::FavoritesModel",
                    "The number of found notebooks is greater than zero, "
                        << "requesting more notebooks from local storage");
                m_listNotebooksOffset += static_cast<quint64>(notebooks.size());
                requestNotebooksList();
                return;
            }

            checkAllItemsListed();
        });

    threading::onFailed(
        std::move(listNotebooksThenFuture), this,
        [this](const QException & e) {
            auto message = exceptionMessage(e);
            ErrorString error{QT_TR_NOOP(
                "Failed to list notebooks from local storage")};
            error.appendBase(message.base());
            error.appendBase(message.additionalBases());
            error.details() = message.details();
            QNWARNING("model::FavoritesModel", error);
            Q_EMIT notifyError(std::move(error));
        });
}

void FavoritesModel::requestTagsList()
{
    QNDEBUG(
        "model::FavoritesModel",
        "FavoritesModel::requestTagsList: offset = " << m_listTagsOffset);

    // NOTE: the subscription to all tags is necessary for the collection of tag
    // names to forbid any two tags within the account to have the same name
    // in a case-insensitive manner
    local_storage::ILocalStorage::ListTagsOptions options;
    options.m_order = local_storage::ILocalStorage::ListTagsOrder::NoOrder;
    options.m_direction =
        local_storage::ILocalStorage::OrderDirection::Ascending;

    auto listTagsFuture = m_localStorage->listTags(options);
    auto listTagsThenFuture = threading::then(
        std::move(listTagsFuture), this,
        [this](const QList<qevercloud::Tag> & tags) {
            for (const auto & tag: std::as_const(tags)) {
                onTagAddedOrUpdated(tag);
            }

            if (!tags.isEmpty()) {
                QNTRACE(
                    "model::FavoritesModel",
                    "The number of found tags is greater than zero, requesting "
                        << "more tags from local storage");
                m_listTagsOffset += static_cast<quint64>(tags.size());
                requestTagsList();
                return;
            }

            checkAllItemsListed();
        });

    threading::onFailed(
        std::move(listTagsThenFuture), this,
        [this](const QException & e) {
            auto message = exceptionMessage(e);
            ErrorString error{QT_TR_NOOP(
                "Failed to list tags from local storage")};
            error.appendBase(message.base());
            error.appendBase(message.additionalBases());
            error.details() = message.details();
            QNWARNING("model::FavoritesModel", error);
            Q_EMIT notifyError(std::move(error));
        });
}

void FavoritesModel::requestSavedSearchesList()
{
    QNDEBUG(
        "model::FavoritesModel",
        "FavoritesModel::requestSavedSearchesList: "
            << "offset = " << m_listSavedSearchesOffset);

    // NOTE: the subscription to all saved searches is necessary for
    // the collection of saved search names to forbid any two saved searches
    // within the account to have the same name in a case-insensitive manner
    local_storage::ILocalStorage::ListSavedSearchesOptions options;
    options.m_order =
        local_storage::ILocalStorage::ListSavedSearchesOrder::NoOrder;

    options.m_direction =
        local_storage::ILocalStorage::OrderDirection::Ascending;

    auto listSavedSearchesFuture = m_localStorage->listSavedSearches(options);
    auto listSavedSearchesThenFuture = threading::then(
        std::move(listSavedSearchesFuture), this,
        [this](const QList<qevercloud::SavedSearch> & savedSearches) {
            for (const auto & savedSearch: std::as_const(savedSearches)) {
                onSavedSearchAddedOrUpdated(savedSearch);
            }

            if (!savedSearches.isEmpty()) {
                QNTRACE(
                    "model::FavoritesModel",
                    "The number of found saved searches is greater than zero, "
                        << "requesting more saved searches from local storage");
                m_listSavedSearchesOffset +=
                    static_cast<quint64>(savedSearches.size());
                requestSavedSearchesList();
                return;
            }

            checkAllItemsListed();
        });

    threading::onFailed(
        std::move(listSavedSearchesThenFuture), this,
        [this](const QException & e) {
            auto message = exceptionMessage(e);
            ErrorString error{QT_TR_NOOP(
                "Failed to list saved searches from local storage")};
            error.appendBase(message.base());
            error.appendBase(message.additionalBases());
            error.details() = message.details();
            QNWARNING("model::FavoritesModel", error);
            Q_EMIT notifyError(std::move(error));
        });
}

void FavoritesModel::requestNoteCountForNotebook(
    const QString & notebookLocalId, const NoteCountRequestOption option)
{
    QNDEBUG(
        "model::FavoritesModel",
        "FavoritesModel::requestNoteCountForNotebook: "
            << "notebook local id = " << notebookLocalId
            << ", note count request option = " << option);

    if (option != NoteCountRequestOption::Force) {
        if (const auto it =
                m_notebookLocalIdsPendingNoteCounts.constFind(notebookLocalId);
            it != m_notebookLocalIdsPendingNoteCounts.constEnd())
        {
            QNDEBUG(
                "model::FavoritesModel",
                "Already requested note count for this notebook");
            return;
        }
    }

    m_notebookLocalIdsPendingNoteCounts.insert(notebookLocalId);

    QNDEBUG(
        "model::FavoritesModel",
        "Requesting note count per notebook: notebook local id = "
            << notebookLocalId);

    auto noteCountFuture = m_localStorage->noteCountPerNotebookLocalId(
        notebookLocalId,
        local_storage::ILocalStorage::NoteCountOptions{} |
        local_storage::ILocalStorage::NoteCountOption::IncludeNonDeletedNotes);

    auto noteCountThenFuture = threading::then(
        std::move(noteCountFuture), this,
        [this, notebookLocalId](const quint32 noteCount) {
            m_notebookLocalIdsPendingNoteCounts.remove(notebookLocalId);

            auto & localIdIndex = m_data.get<ByLocalId>();
            const auto itemIt = localIdIndex.find(notebookLocalId);
            if (Q_UNLIKELY(itemIt == localIdIndex.end())) {
                QNDEBUG(
                    "model::FavoritesModel",
                    "Can't find notebook item within favorites model for which "
                        << "note count was received");
                return;
            }

            FavoritesModelItem item{*itemIt};
            item.setNoteCount(noteCount);
            localIdIndex.replace(itemIt, item);
            updateItemColumnInView(item, Column::NoteCount);
        });

    threading::onFailed(
        std::move(noteCountThenFuture), this,
        [this, notebookLocalId](const QException & e) {
            m_notebookLocalIdsPendingNoteCounts.remove(notebookLocalId);

            auto message = exceptionMessage(e);
            ErrorString error{QT_TR_NOOP(
                "Failed to get note count for notebook from local storage")};
            error.appendBase(message.base());
            error.appendBase(message.additionalBases());
            error.details() = message.details();
            QNWARNING(
                "model::FavoritesModel",
                error << ", notebook local id = " << notebookLocalId);
            Q_EMIT notifyError(std::move(error));
        });
}

void FavoritesModel::requestNoteCountForAllNotebooks(
    const NoteCountRequestOption option)
{
    QNDEBUG(
        "model::FavoritesModel",
        "FavoritesModel::requestNoteCountForAllNotebooks: "
            << "note count request option = " << option);

    const auto & localIdIndex = m_data.get<ByLocalId>();

    for (const auto & item: localIdIndex) {
        if (item.type() != FavoritesModelItem::Type::Notebook) {
            continue;
        }

        requestNoteCountForNotebook(item.localId(), option);
    }
}

void FavoritesModel::requestNoteCountForTag(
    const QString & tagLocalId, const NoteCountRequestOption option)
{
    QNDEBUG(
        "model::FavoritesModel",
        "FavoritesModel::requestNoteCountForTag: "
            << "tag local id = " << tagLocalId
            << ", note count request option = " << option);

    if (option != NoteCountRequestOption::Force) {
        if (const auto it =
                m_tagLocalIdsPendingNoteCounts.constFind(tagLocalId);
            it != m_tagLocalIdsPendingNoteCounts.end())
        {
            QNDEBUG(
                "model::FavoritesModel",
                "Already requested note count for this tag");
            return;
        }
    }

    m_tagLocalIdsPendingNoteCounts.insert(tagLocalId);

    QNDEBUG(
        "model::FavoritesModel",
        "Requesting note count per tag: tag local id = " << tagLocalId);

    auto noteCountFuture = m_localStorage->noteCountPerTagLocalId(
        tagLocalId,
        local_storage::ILocalStorage::NoteCountOptions{} |
        local_storage::ILocalStorage::NoteCountOption::IncludeNonDeletedNotes);

    auto noteCountThenFuture = threading::then(
        std::move(noteCountFuture), this,
        [this, tagLocalId](const quint32 noteCount) {
            m_tagLocalIdsPendingNoteCounts.remove(tagLocalId);

            auto & localIdIndex = m_data.get<ByLocalId>();
            const auto itemIt = localIdIndex.find(tagLocalId);
            if (Q_UNLIKELY(itemIt == localIdIndex.end())) {
                QNDEBUG(
                    "model::FavoritesModel",
                    "Can't find tag item within favorites model for which note "
                        << "count was received");
                return;
            }

            FavoritesModelItem item{*itemIt};
            item.setNoteCount(noteCount);
            localIdIndex.replace(itemIt, item);
            updateItemColumnInView(item, Column::NoteCount);
        });

    threading::onFailed(
        std::move(noteCountThenFuture), this,
        [this, tagLocalId](const QException & e) {
            m_tagLocalIdsPendingNoteCounts.remove(tagLocalId);

            auto message = exceptionMessage(e);
            ErrorString error{QT_TR_NOOP(
                "Failed to get note count for tag from local storage")};
            error.appendBase(message.base());
            error.appendBase(message.additionalBases());
            error.details() = message.details();
            QNWARNING(
                "model::FavoritesModel",
                error << ", tag local id = " << tagLocalId);
            Q_EMIT notifyError(std::move(error));
        });
}

void FavoritesModel::requestNoteCountForAllTags(
    const NoteCountRequestOption option)
{
    QNDEBUG(
        "model::FavoritesModel", "FavoritesModel::requestNoteCountForAllTags");

    const auto & localIdIndex = m_data.get<ByLocalId>();
    for (const auto & item: localIdIndex) {
        if (item.type() != FavoritesModelItem::Type::Tag) {
            continue;
        }

        requestNoteCountForTag(item.localId(), option);
    }
}

QVariant FavoritesModel::dataImpl(const int row, const Column column) const
{
    if (Q_UNLIKELY((row < 0) || (row >= static_cast<int>(m_data.size())))) {
        return {};
    }

    const auto & rowIndex = m_data.get<ByIndex>();
    const auto & item = rowIndex[static_cast<size_t>(row)];

    switch (column) {
    case Column::Type:
        return static_cast<qint64>(item.type());
    case Column::DisplayName:
        return item.displayName();
    case Column::NoteCount:
        return item.noteCount();
    default:
        return {};
    }
}

QVariant FavoritesModel::dataAccessibleText(
    const int row, const Column column) const
{
    if (Q_UNLIKELY((row < 0) || (row >= static_cast<int>(m_data.size())))) {
        return {};
    }

    const auto & rowIndex = m_data.get<ByIndex>();
    const auto & item = rowIndex[static_cast<size_t>(row)];

    const QString space = QStringLiteral(" ");
    const QString colon = QStringLiteral(":");

    QString accessibleText = tr("Favorited") + space;
    switch (item.type()) {
    case FavoritesModelItem::Type::Note:
        accessibleText += tr("note");
        break;
    case FavoritesModelItem::Type::Notebook:
        accessibleText += tr("notebook");
        break;
    case FavoritesModelItem::Type::Tag:
        accessibleText += tr("tag");
        break;
    case FavoritesModelItem::Type::SavedSearch:
        accessibleText += tr("saved search");
        break;
    default:
        return {};
    }

    switch (column) {
    case Column::Type:
        return accessibleText;
    case Column::DisplayName:
        accessibleText += colon + space + item.displayName();
        break;
    case Column::NoteCount:
        accessibleText += colon + space + tr("number of targeted notes is") +
            space + QString::number(item.noteCount());
        break;
    default:
        return {};
    }

    return accessibleText;
}

void FavoritesModel::removeItemByLocalId(const QString & localId)
{
    QNTRACE(
        "model::FavoritesModel",
        "FavoritesModel::removeItemByLocalId: "
            << "local id = " << localId);

    auto & localIdIndex = m_data.get<ByLocalId>();
    const auto itemIt = localIdIndex.find(localId);
    if (Q_UNLIKELY(itemIt == localIdIndex.end())) {
        QNDEBUG(
            "model::FavoritesModel",
            "Can't find item to remove from the favorites model");
        return;
    }

    const auto & item = *itemIt;

    switch (item.type()) {
    case FavoritesModelItem::Type::Notebook:
    {
        if (const auto nameIt =
                m_lowerCaseNotebookNames.find(item.displayName().toLower());
            nameIt != m_lowerCaseNotebookNames.end())
        {
            m_lowerCaseNotebookNames.erase(nameIt);
        }

        break;
    }
    case FavoritesModelItem::Type::Tag:
    {
        if (const auto nameIt =
                m_lowerCaseTagNames.find(item.displayName().toLower());
            nameIt != m_lowerCaseTagNames.end())
        {
            m_lowerCaseTagNames.erase(nameIt);
        }

        break;
    }
    case FavoritesModelItem::Type::SavedSearch:
    {
        if (const auto nameIt =
                m_lowerCaseSavedSearchNames.find(item.displayName().toLower());
            nameIt != m_lowerCaseSavedSearchNames.end())
        {
            m_lowerCaseSavedSearchNames.erase(nameIt);
        }

        break;
    }
    default:
        break;
    }

    auto & rowIndex = m_data.get<ByIndex>();
    const auto indexIt = m_data.project<ByIndex>(itemIt);
    if (Q_UNLIKELY(indexIt == rowIndex.end())) {
        QNWARNING(
            "model::FavoritesModel",
            "Can't determine the row index for "
                << "the favorites model item to remove: " << item);
        return;
    }

    const int row = static_cast<int>(std::distance(rowIndex.begin(), indexIt));
    if (Q_UNLIKELY((row < 0) || (row >= static_cast<int>(m_data.size())))) {
        QNWARNING(
            "model::FavoritesModel",
            "Invalid row index for the favorites model item to remove: index = "
                << row << ", item: " << item);
        return;
    }

    Q_EMIT aboutToRemoveItems();

    beginRemoveRows(QModelIndex{}, row, row);
    localIdIndex.erase(itemIt);
    endRemoveRows();

    Q_EMIT removedItems();
}

void FavoritesModel::updateItemRowWithRespectToSorting(
    const FavoritesModelItem & item)
{
    if (m_sortedColumn != Column::DisplayName) {
        // Sorting by other columns is not yet implemented
        return;
    }

    auto & localIdIndex = m_data.get<ByLocalId>();
    auto localIdIt = localIdIndex.find(item.localId());
    if (Q_UNLIKELY(localIdIt == localIdIndex.end())) {
        QNWARNING(
            "model::FavoritesModel",
            "Can't update item row with respect to "
                << "sorting: can't find the item within the model: " << item);
        return;
    }

    auto & rowIndex = m_data.get<ByIndex>();
    auto it = m_data.project<ByIndex>(localIdIt);
    if (Q_UNLIKELY(it == rowIndex.end())) {
        QNWARNING(
            "model::FavoritesModel",
            "Can't update item row with respect to "
                << "sorting: can't find item's original row; item: " << item);
        return;
    }

    int originalRow = static_cast<int>(std::distance(rowIndex.begin(), it));
    if (Q_UNLIKELY(
            (originalRow < 0) ||
            (originalRow >= static_cast<int>(m_data.size()))))
    {
        QNWARNING(
            "model::FavoritesModel",
            "Can't update item row with respect to "
                << "sorting: item's original row is beyond the acceptable "
                << "range: " << originalRow << ", item: " << item);
        return;
    }

    FavoritesModelItem itemCopy(item);

    beginRemoveRows(QModelIndex(), originalRow, originalRow);
    Q_UNUSED(rowIndex.erase(it))
    endRemoveRows();

    auto positionIter = std::lower_bound(
        rowIndex.begin(), rowIndex.end(), itemCopy,
        Comparator(m_sortedColumn, m_sortOrder));

    if (positionIter == rowIndex.end()) {
        int row = static_cast<int>(rowIndex.size());
        beginInsertRows(QModelIndex(), row, row);
        rowIndex.push_back(itemCopy);
        endInsertRows();
        return;
    }

    int row = static_cast<int>(std::distance(rowIndex.begin(), positionIter));
    beginInsertRows(QModelIndex(), row, row);
    Q_UNUSED(rowIndex.insert(positionIter, itemCopy))
    endInsertRows();
}

void FavoritesModel::updateItemInLocalStorage(const FavoritesModelItem & item)
{
    switch (item.type()) {
    case FavoritesModelItem::Type::Note:
        updateNoteInLocalStorage(item);
        break;
    case FavoritesModelItem::Type::Notebook:
        updateNotebookInLocalStorage(item);
        break;
    case FavoritesModelItem::Type::Tag:
        updateTagInLocalStorage(item);
        break;
    case FavoritesModelItem::Type::SavedSearch:
        updateSavedSearchInLocalStorage(item);
        break;
    default:
    {
        QNWARNING(
            "model::FavoritesModel",
            "Detected attempt to update favorites "
                << "model item in the local storage for wrong favorited "
                << "model item's type: " << item);
        break;
    }
    }
}

void FavoritesModel::updateNoteInLocalStorage(const FavoritesModelItem & item)
{
    QNDEBUG(
        "model::FavoritesModel",
        "FavoritesModel::updateNoteInLocalStorage: "
            << "local id = " << item.localId()
            << ", title = " << item.displayName());

    const auto * pCachedNote = m_noteCache.get(item.localId());
    if (Q_UNLIKELY(!pCachedNote)) {
        auto requestId = QUuid::createUuid();
        Q_UNUSED(m_findNoteToPerformUpdateRequestIds.insert(requestId))

        Note dummy;
        dummy.setLocalId(item.localId());

        QNTRACE(
            "model::FavoritesModel",
            "Emitting the request to find a note: "
                << "local id = " << item.localId()
                << ", request id = " << requestId);

        LocalStorageManager::GetNoteOptions options(
            LocalStorageManager::GetNoteOption::WithResourceMetadata);

        Q_EMIT findNote(dummy, options, requestId);
        return;
    }

    Note note = *pCachedNote;
    note.setLocalId(item.localId());

    bool dirty = note.isDirty() || !note.hasTitle() ||
        (note.title() != item.displayName());

    note.setDirty(dirty);
    note.setTitle(item.displayName());

    QUuid requestId = QUuid::createUuid();
    Q_UNUSED(m_updateNoteRequestIds.insert(requestId))

    // While the note is being updated in the local storage,
    // remove its stale copy from the cache
    Q_UNUSED(m_noteCache.remove(note.localId()))

    QNTRACE(
        "model::FavoritesModel",
        "Emitting the request to update the note in "
            << "the local storage: id = " << requestId << ", note: " << note);

    Q_EMIT updateNote(
        note,
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
        LocalStorageManager::UpdateNoteOptions(),
#else
        LocalStorageManager::UpdateNoteOptions(0),
#endif
        requestId);
}

void FavoritesModel::updateNotebookInLocalStorage(
    const FavoritesModelItem & item)
{
    QNDEBUG(
        "model::FavoritesModel",
        "FavoritesModel::updateNotebookInLocalStorage: "
            << "local id = " << item.localId()
            << ", name = " << item.displayName());

    const auto * pCachedNotebook = m_notebookCache.get(item.localId());
    if (Q_UNLIKELY(!pCachedNotebook)) {
        auto requestId = QUuid::createUuid();
        Q_UNUSED(m_findNotebookToPerformUpdateRequestIds.insert(requestId))

        Notebook dummy;
        dummy.setLocalId(item.localId());

        QNTRACE(
            "model::FavoritesModel",
            "Emitting the request to find a notebook: "
                << "local id = " << item.localId()
                << ", request id = " << requestId);

        Q_EMIT findNotebook(dummy, requestId);
        return;
    }

    Notebook notebook = *pCachedNotebook;
    notebook.setLocalId(item.localId());

    bool dirty = notebook.isDirty() || !notebook.hasName() ||
        (notebook.name() != item.displayName());

    notebook.setDirty(dirty);
    notebook.setName(item.displayName());

    auto requestId = QUuid::createUuid();
    Q_UNUSED(m_updateNotebookRequestIds.insert(requestId))

    // While the notebook is being updated in the local storage,
    // remove its stale copy from the cache
    Q_UNUSED(m_notebookCache.remove(notebook.localId()))

    QNTRACE(
        "model::FavoritesModel",
        "Emitting the request to update the notebook in "
            << "the local storage: id = " << requestId
            << ", notebook: " << notebook);

    Q_EMIT updateNotebook(notebook, requestId);
}

void FavoritesModel::updateTagInLocalStorage(const FavoritesModelItem & item)
{
    QNDEBUG(
        "model::FavoritesModel",
        "FavoritesModel::updateTagInLocalStorage: "
            << "local id = " << item.localId()
            << ", name = " << item.displayName());

    const auto * pCachedTag = m_tagCache.get(item.localId());
    if (Q_UNLIKELY(!pCachedTag)) {
        auto requestId = QUuid::createUuid();
        Q_UNUSED(m_findTagToPerformUpdateRequestIds.insert(requestId))

        Tag dummy;
        dummy.setLocalId(item.localId());

        QNTRACE(
            "model::FavoritesModel",
            "Emitting the request to find a tag: "
                << "local id = " << item.localId()
                << ", request id = " << requestId);

        Q_EMIT findTag(dummy, requestId);
        return;
    }

    Tag tag = *pCachedTag;
    tag.setLocalId(item.localId());

    bool dirty =
        tag.isDirty() || !tag.hasName() || (tag.name() != item.displayName());

    tag.setDirty(dirty);
    tag.setName(item.displayName());

    auto requestId = QUuid::createUuid();
    Q_UNUSED(m_updateTagRequestIds.insert(requestId))

    // While the tag is being updated in the local storage,
    // remove its stale copy from the cache
    Q_UNUSED(m_tagCache.remove(tag.localId()))

    QNTRACE(
        "model::FavoritesModel",
        "Emitting the request to update the tag in "
            << "the local storage: id = " << requestId << ", tag: " << tag);
    Q_EMIT updateTag(tag, requestId);
}

void FavoritesModel::updateSavedSearchInLocalStorage(
    const FavoritesModelItem & item)
{
    QNDEBUG(
        "model::FavoritesModel",
        "FavoritesModel::updateSavedSearchInLocalStorage: local id = "
            << item.localId() << ", display name = " << item.displayName());

    const auto * pCachedSearch = m_savedSearchCache.get(item.localId());
    if (Q_UNLIKELY(!pCachedSearch)) {
        auto requestId = QUuid::createUuid();
        Q_UNUSED(m_findSavedSearchToPerformUpdateRequestIds.insert(requestId))

        SavedSearch dummy;
        dummy.setLocalId(item.localId());

        QNTRACE(
            "model::FavoritesModel",
            "Emitting the request to find a saved "
                << "search: local id = " << item.localId()
                << ", request id = " << requestId);

        Q_EMIT findSavedSearch(dummy, requestId);
        return;
    }

    SavedSearch search = *pCachedSearch;
    search.setLocalId(item.localId());

    bool dirty = search.isDirty() || !search.hasName() ||
        (search.name() != item.displayName());

    search.setDirty(dirty);
    search.setName(item.displayName());

    auto requestId = QUuid::createUuid();
    Q_UNUSED(m_updateSavedSearchRequestIds.insert(requestId))

    // While the saved search is being updated in the local storage,
    // remove its stale copy from the cache
    Q_UNUSED(m_savedSearchCache.remove(search.localId()))

    QNTRACE(
        "model::FavoritesModel",
        "Emitting the request to update the saved "
            << "search in the local storage: id = " << requestId
            << ", saved search: " << search);

    Q_EMIT updateSavedSearch(search, requestId);
}

bool FavoritesModel::canUpdateNote(const QString & localId) const
{
    auto notebookLocalIdIt = m_notebookLocalIdByNoteLocalId.find(localId);
    if (notebookLocalIdIt == m_notebookLocalIdByNoteLocalId.end()) {
        return false;
    }

    auto notebookGuidIt =
        m_notebookLocalIdToGuid.find(notebookLocalIdIt.value());

    if (notebookGuidIt == m_notebookLocalIdToGuid.end()) {
        // NOTE: this must be the local, non-synchronizable notebook as it
        // doesn't have the Evernote service's guid;
        return true;
    }

    auto notebookRestrictionsDataIt =
        m_notebookRestrictionsData.find(notebookGuidIt.value());

    if (notebookRestrictionsDataIt == m_notebookRestrictionsData.end()) {
        return true;
    }

    const auto & restrictionsData = notebookRestrictionsDataIt.value();
    return restrictionsData.m_canUpdateNotes;
}

bool FavoritesModel::canUpdateNotebook(const QString & localId) const
{
    auto notebookGuidIt = m_notebookLocalIdToGuid.find(localId);
    if (notebookGuidIt == m_notebookLocalIdToGuid.end()) {
        // NOTE: this must be the local, non-synchronizable notebook as it
        // doesn't have the Evernote service's guid;
        return true;
    }

    auto notebookRestrictionsDataIt =
        m_notebookRestrictionsData.find(notebookGuidIt.value());

    if (notebookRestrictionsDataIt == m_notebookRestrictionsData.end()) {
        return true;
    }

    const auto & restrictionsData = notebookRestrictionsDataIt.value();
    return restrictionsData.m_canUpdateNotebook;
}

bool FavoritesModel::canUpdateTag(const QString & localId) const
{
    auto notebookGuidIt = m_tagLocalIdToLinkedNotebookGuid.find(localId);
    if (notebookGuidIt == m_tagLocalIdToLinkedNotebookGuid.end()) {
        return true;
    }

    auto notebookRestrictionsDataIt =
        m_notebookRestrictionsData.find(notebookGuidIt.value());

    if (notebookRestrictionsDataIt == m_notebookRestrictionsData.end()) {
        return true;
    }

    const auto & restrictionsData = notebookRestrictionsDataIt.value();
    return restrictionsData.m_canUpdateTags;
}

void FavoritesModel::unfavoriteNote(const QString & localId)
{
    QNDEBUG(
        "model::FavoritesModel",
        "FavoritesModel::unfavoriteNote: local id = " << localId);

    const auto * pCachedNote = m_noteCache.get(localId);
    if (Q_UNLIKELY(!pCachedNote)) {
        auto requestId = QUuid::createUuid();
        Q_UNUSED(m_findNoteToUnfavoriteRequestIds.insert(requestId))

        Note dummy;
        dummy.setLocalId(localId);

        QNTRACE(
            "model::FavoritesModel",
            "Emitting the request to find a note: "
                << "local id = " << localId
                << ", request id = " << requestId);

        LocalStorageManager::GetNoteOptions options(
            LocalStorageManager::GetNoteOption::WithResourceMetadata);

        Q_EMIT findNote(dummy, options, requestId);
        return;
    }

    Note note = *pCachedNote;
    note.setLocalId(localId);
    bool dirty = note.isDirty() || note.isFavorited();
    note.setDirty(dirty);
    note.setFavorited(false);

    auto requestId = QUuid::createUuid();
    Q_UNUSED(m_updateNoteRequestIds.insert(requestId))

    // While the note is being updated in the local storage,
    // remove its stale copy from the cache
    Q_UNUSED(m_noteCache.remove(note.localId()))

    QNTRACE(
        "model::FavoritesModel",
        "Emitting the request to update the note in "
            << "the local storage: id = " << requestId << ", note: " << note);

    Q_EMIT updateNote(
        note,
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
        LocalStorageManager::UpdateNoteOptions(),
#else
        LocalStorageManager::UpdateNoteOptions(0),
#endif
        requestId);
}

void FavoritesModel::unfavoriteNotebook(const QString & localId)
{
    QNDEBUG(
        "model::FavoritesModel",
        "FavoritesModel::unfavoriteNotebook: local "
            << "id = " << localId);

    const auto * pCachedNotebook = m_notebookCache.get(localId);
    if (Q_UNLIKELY(!pCachedNotebook)) {
        auto requestId = QUuid::createUuid();
        Q_UNUSED(m_findNotebookToUnfavoriteRequestIds.insert(requestId))

        Notebook dummy;
        dummy.setLocalId(localId);

        QNTRACE(
            "model::FavoritesModel",
            "Emitting the request to find a notebook: "
                << "local id = " << localId
                << ", request id = " << requestId);

        Q_EMIT findNotebook(dummy, requestId);
        return;
    }

    Notebook notebook = *pCachedNotebook;

    notebook.setLocalId(localId);
    bool dirty = notebook.isDirty() || notebook.isFavorited();
    notebook.setDirty(dirty);
    notebook.setFavorited(false);

    auto requestId = QUuid::createUuid();
    Q_UNUSED(m_updateNotebookRequestIds.insert(requestId))

    // While the notebook is being updated in the local storage,
    // remove its stale copy from the cache
    Q_UNUSED(m_notebookCache.remove(notebook.localId()))

    QNTRACE(
        "model::FavoritesModel",
        "Emitting the request to update the notebook in "
            << "the local storage: id = " << requestId
            << ", notebook: " << notebook);

    Q_EMIT updateNotebook(notebook, requestId);
}

void FavoritesModel::unfavoriteTag(const QString & localId)
{
    QNDEBUG(
        "model::FavoritesModel",
        "FavoritesModel::unfavoriteTag: local id = " << localId);

    const auto * pCachedTag = m_tagCache.get(localId);
    if (Q_UNLIKELY(!pCachedTag)) {
        auto requestId = QUuid::createUuid();
        Q_UNUSED(m_findTagToUnfavoriteRequestIds.insert(requestId))

        Tag dummy;
        dummy.setLocalId(localId);

        QNTRACE(
            "model::FavoritesModel",
            "Emitting the request to find a tag: "
                << "local id = " << localId
                << ", request id = " << requestId);

        Q_EMIT findTag(dummy, requestId);
        return;
    }

    Tag tag = *pCachedTag;

    tag.setLocalId(localId);
    bool dirty = tag.isDirty() || tag.isFavorited();
    tag.setDirty(dirty);
    tag.setFavorited(false);

    QUuid requestId = QUuid::createUuid();
    Q_UNUSED(m_updateTagRequestIds.insert(requestId))

    // While the tag is being updated in the local storage,
    // remove its stale copy from the cache
    Q_UNUSED(m_tagCache.remove(tag.localId()))

    QNTRACE(
        "model::FavoritesModel",
        "Emitting the request to update the tag in "
            << "the local storage: id = " << requestId << ", tag: " << tag);

    Q_EMIT updateTag(tag, requestId);
}

void FavoritesModel::unfavoriteSavedSearch(const QString & localId)
{
    QNDEBUG(
        "model::FavoritesModel",
        "FavoritesModel::unfavoriteSavedSearch: local id = " << localId);

    const auto * pCachedSearch = m_savedSearchCache.get(localId);
    if (Q_UNLIKELY(!pCachedSearch)) {
        auto requestId = QUuid::createUuid();
        Q_UNUSED(m_findSavedSearchToUnfavoriteRequestIds.insert(requestId))

        SavedSearch dummy;
        dummy.setLocalId(localId);

        QNTRACE(
            "model::FavoritesModel",
            "Emitting the request to find a saved "
                << "search: local id = " << localId
                << ", request id = " << requestId);

        Q_EMIT findSavedSearch(dummy, requestId);
        return;
    }

    SavedSearch search = *pCachedSearch;

    search.setLocalId(localId);
    bool dirty = search.isDirty() || search.isFavorited();
    search.setDirty(dirty);
    search.setFavorited(false);

    auto requestId = QUuid::createUuid();
    Q_UNUSED(m_updateSavedSearchRequestIds.insert(requestId))

    // While the saved search is being updated in the local storage,
    // remove its stale copy from the cache
    Q_UNUSED(m_savedSearchCache.remove(search.localId()))

    QNTRACE(
        "model::FavoritesModel",
        "Emitting the request to update the saved "
            << "search in the local storage: id = " << requestId
            << ", saved search: " << search);

    Q_EMIT updateSavedSearch(search, requestId);
}

void FavoritesModel::onNoteAddedOrUpdated(
    const Note & note, const bool tagsUpdated)
{
    QNDEBUG(
        "model::FavoritesModel",
        "FavoritesModel::onNoteAddedOrUpdated: "
            << "note local id = " << note.localId()
            << ", tags updated = " << (tagsUpdated ? "true" : "false"));

    if (tagsUpdated) {
        m_noteCache.put(note.localId(), note);
    }

    if (!note.hasNotebookLocalId()) {
        QNWARNING(
            "model::FavoritesModel",
            "Skipping the note not having "
                << "the notebook local id: " << note);
        return;
    }

    if (!note.isFavorited()) {
        removeItemByLocalId(note.localId());
        return;
    }

    FavoritesModelItem item;
    item.setType(FavoritesModelItem::Type::Note);
    item.setLocalId(note.localId());
    item.setNoteCount(0);

    if (note.hasTitle()) {
        item.setDisplayName(note.title());
    }
    else if (note.hasContent()) {
        QString plainText = note.plainText();
        plainText.truncate(160);
        item.setDisplayName(plainText);
        // NOTE: using the text preview in this way means updating the favorites
        // item's display name would actually create the title for the note
    }

    m_notebookLocalIdByNoteLocalId[note.localId()] = note.notebookLocalId();

    auto & rowIndex = m_data.get<ByIndex>();
    auto & localIdIndex = m_data.get<ByLocalId>();
    auto itemIt = localIdIndex.find(note.localId());
    if (itemIt == localIdIndex.end()) {
        QNDEBUG("model::FavoritesModel", "Detected newly favorited note");

        Q_EMIT aboutToAddItem();

        int row = static_cast<int>(rowIndex.size());
        beginInsertRows(QModelIndex(), row, row);
        rowIndex.push_back(item);
        endInsertRows();

        updateItemRowWithRespectToSorting(item);

        QModelIndex addedNoteIndex = indexForLocalId(item.localId());
        Q_EMIT addedItem(addedNoteIndex);

        return;
    }

    QNDEBUG("model::FavoritesModel", "Updating the already favorited item");

    auto indexIt = m_data.project<ByIndex>(itemIt);
    if (Q_UNLIKELY(indexIt == rowIndex.end())) {
        ErrorString error(
            QT_TR_NOOP("Internal error: can't project the local id "
                       "index iterator to the random access index "
                       "iterator within the favorites model"));

        QNWARNING(
            "model::FavoritesModel", error << ", favorites model item: " << item);

        Q_EMIT notifyError(error);
        return;
    }

    int row = static_cast<int>(std::distance(rowIndex.begin(), indexIt));

    auto modelIndex = createIndex(row, static_cast<int>(Column::DisplayName));
    Q_EMIT aboutToUpdateItem(modelIndex);

    Q_UNUSED(localIdIndex.replace(itemIt, item))

    Q_EMIT dataChanged(modelIndex, modelIndex);

    updateItemRowWithRespectToSorting(item);

    modelIndex = indexForLocalId(item.localId());
    Q_EMIT updatedItem(modelIndex);
}

void FavoritesModel::onNotebookAddedOrUpdated(const Notebook & notebook)
{
    QNDEBUG(
        "model::FavoritesModel",
        "FavoritesModel::onNotebookAddedOrUpdated: "
            << "local id = " << notebook.localId());

    m_notebookCache.put(notebook.localId(), notebook);

    auto & localIdIndex = m_data.get<ByLocalId>();
    auto itemIt = localIdIndex.find(notebook.localId());

    if (itemIt != localIdIndex.end()) {
        const auto & originalItem = *itemIt;

        auto nameIt =
            m_lowerCaseNotebookNames.find(originalItem.displayName().toLower());

        if (nameIt != m_lowerCaseNotebookNames.end()) {
            Q_UNUSED(m_lowerCaseNotebookNames.erase(nameIt))
        }
    }

    if (notebook.hasName()) {
        Q_UNUSED(m_lowerCaseNotebookNames.insert(notebook.name().toLower()))
    }

    if (notebook.hasGuid()) {
        auto & notebookRestrictionsData =
            m_notebookRestrictionsData[notebook.guid()];

        if (notebook.hasRestrictions()) {
            const auto & restrictions = notebook.restrictions();

            notebookRestrictionsData.m_canUpdateNotebook =
                !restrictions.noUpdateNotebook.isSet() ||
                !restrictions.noUpdateNotebook.ref();

            notebookRestrictionsData.m_canUpdateNotes =
                !restrictions.noUpdateNotes.isSet() ||
                !restrictions.noUpdateNotes.ref();

            notebookRestrictionsData.m_canUpdateTags =
                !restrictions.noUpdateTags.isSet() ||
                !restrictions.noUpdateTags.ref();
        }
        else {
            notebookRestrictionsData.m_canUpdateNotebook = true;
            notebookRestrictionsData.m_canUpdateNotes = true;
            notebookRestrictionsData.m_canUpdateTags = true;
        }

        QNTRACE(
            "model::FavoritesModel",
            "Updated restrictions data for notebook "
                << notebook.localId() << ", name "
                << (notebook.hasName() ? QStringLiteral("\"") +
                            notebook.name() + QStringLiteral("\"")
                                       : QStringLiteral("<not set>"))
                << ", guid = " << notebook.guid() << ": can update notebook = "
                << (notebookRestrictionsData.m_canUpdateNotebook ? "true"
                                                                 : "false")
                << ", can update notes = "
                << (notebookRestrictionsData.m_canUpdateNotes ? "true"
                                                              : "false")
                << ", can update tags = "
                << (notebookRestrictionsData.m_canUpdateTags ? "true"
                                                             : "false"));
    }

    if (!notebook.hasName()) {
        QNTRACE(
            "model::FavoritesModel",
            "Removing/skipping the notebook without "
                << "a name");
        removeItemByLocalId(notebook.localId());
        return;
    }

    if (!notebook.isFavorited()) {
        QNTRACE("model::FavoritesModel", "Removing/skipping non-favorited notebook");
        removeItemByLocalId(notebook.localId());
        return;
    }

    FavoritesModelItem item;
    item.setType(FavoritesModelItem::Type::Notebook);
    item.setLocalId(notebook.localId());
    item.setNoteCount(-1); // That means, not known yet
    item.setDisplayName(notebook.name());

    auto & rowIndex = m_data.get<ByIndex>();

    if (itemIt == localIdIndex.end()) {
        QNDEBUG("model::FavoritesModel", "Detected newly favorited notebook");

        Q_EMIT aboutToAddItem();

        int row = static_cast<int>(rowIndex.size());
        beginInsertRows(QModelIndex(), row, row);
        rowIndex.push_back(item);
        endInsertRows();

        updateItemRowWithRespectToSorting(item);

        auto addedNotebookIndex = indexForLocalId(item.localId());
        Q_EMIT addedItem(addedNotebookIndex);

        // Need to figure out how many notes this notebook targets
        requestNoteCountForNotebook(
            notebook.localId(), NoteCountRequestOption::IfNotAlreadyRunning);

        return;
    }

    QNDEBUG("model::FavoritesModel", "Updating the already favorited notebook item");

    const auto & originalItem = *itemIt;
    item.setNoteCount(originalItem.noteCount());

    auto indexIt = m_data.project<ByIndex>(itemIt);
    if (Q_UNLIKELY(indexIt == rowIndex.end())) {
        ErrorString error(
            QT_TR_NOOP("Internal error: can't project the local id "
                       "index iterator to the random access index "
                       "iterator within the favorites model"));

        QNWARNING(
            "model::FavoritesModel", error << ", favorites model item: " << item);

        Q_EMIT notifyError(error);
        return;
    }

    int row = static_cast<int>(std::distance(rowIndex.begin(), indexIt));

    auto modelIndex = createIndex(row, static_cast<int>(Column::DisplayName));
    Q_EMIT aboutToUpdateItem(modelIndex);

    Q_UNUSED(localIdIndex.replace(itemIt, item))

    Q_EMIT dataChanged(modelIndex, modelIndex);

    updateItemRowWithRespectToSorting(item);

    modelIndex = indexForLocalId(item.localId());
    Q_EMIT updatedItem(modelIndex);
}

void FavoritesModel::onTagAddedOrUpdated(const Tag & tag)
{
    QNTRACE(
        "model::FavoritesModel",
        "FavoritesModel::onTagAddedOrUpdated: "
            << "local id = " << tag.localId());

    m_tagCache.put(tag.localId(), tag);

    auto & localIdIndex = m_data.get<ByLocalId>();
    auto itemIt = localIdIndex.find(tag.localId());

    if (itemIt != localIdIndex.end()) {
        const auto & originalItem = *itemIt;

        auto nameIt =
            m_lowerCaseTagNames.find(originalItem.displayName().toLower());

        if (nameIt != m_lowerCaseTagNames.end()) {
            Q_UNUSED(m_lowerCaseTagNames.erase(nameIt))
        }
    }

    if (tag.hasName()) {
        Q_UNUSED(m_lowerCaseTagNames.insert(tag.name().toLower()))
    }
    else {
        QNTRACE("model::FavoritesModel", "Removing/skipping the tag without a name");
        removeItemByLocalId(tag.localId());
        return;
    }

    if (!tag.isFavorited()) {
        QNTRACE("model::FavoritesModel", "Removing/skipping non-favorited tag");
        removeItemByLocalId(tag.localId());
        return;
    }

    FavoritesModelItem item;
    item.setType(FavoritesModelItem::Type::Tag);
    item.setLocalId(tag.localId());
    item.setNoteCount(-1); // That means, not known yet
    item.setDisplayName(tag.name());

    auto & rowIndex = m_data.get<ByIndex>();

    if (itemIt == localIdIndex.end()) {
        QNDEBUG("model::FavoritesModel", "Detected newly favorited tag");

        Q_EMIT aboutToAddItem();

        int row = static_cast<int>(rowIndex.size());
        beginInsertRows(QModelIndex(), row, row);
        rowIndex.push_back(item);
        endInsertRows();

        updateItemRowWithRespectToSorting(item);

        auto addedTagIndex = indexForLocalId(item.localId());
        Q_EMIT addedItem(addedTagIndex);

        // Need to figure out how many notes this tag targets
        requestNoteCountForTag(
            tag.localId(), NoteCountRequestOption::IfNotAlreadyRunning);

        return;
    }

    QNDEBUG("model::FavoritesModel", "Updating the already favorited tag item");

    const auto & originalItem = *itemIt;
    item.setNoteCount(originalItem.noteCount());

    auto indexIt = m_data.project<ByIndex>(itemIt);
    if (Q_UNLIKELY(indexIt == rowIndex.end())) {
        ErrorString error(
            QT_TR_NOOP("Internal error: can't project the local id "
                       "index iterator to the random access index "
                       "iterator within the favorites model"));

        QNWARNING(
            "model::FavoritesModel", error << ", favorites model item: " << item);

        Q_EMIT notifyError(error);
        return;
    }

    int row = static_cast<int>(std::distance(rowIndex.begin(), indexIt));

    auto modelIndex = createIndex(row, static_cast<int>(Column::DisplayName));
    Q_EMIT aboutToUpdateItem(modelIndex);

    Q_UNUSED(localIdIndex.replace(itemIt, item))

    Q_EMIT dataChanged(modelIndex, modelIndex);

    updateItemRowWithRespectToSorting(item);

    modelIndex = indexForLocalId(item.localId());
    Q_EMIT updatedItem(modelIndex);
}

void FavoritesModel::onSavedSearchAddedOrUpdated(const SavedSearch & search)
{
    QNDEBUG(
        "model::FavoritesModel",
        "FavoritesModel::onSavedSearchAddedOrUpdated: "
            << "local id = " << search.localId());

    m_savedSearchCache.put(search.localId(), search);

    auto & localIdIndex = m_data.get<ByLocalId>();
    auto itemIt = localIdIndex.find(search.localId());

    if (itemIt != localIdIndex.end()) {
        const auto & originalItem = *itemIt;

        auto nameIt = m_lowerCaseSavedSearchNames.find(
            originalItem.displayName().toLower());

        if (nameIt != m_lowerCaseSavedSearchNames.end()) {
            Q_UNUSED(m_lowerCaseSavedSearchNames.erase(nameIt))
        }
    }

    if (search.hasName()) {
        Q_UNUSED(m_lowerCaseSavedSearchNames.insert(search.name().toLower()))
    }
    else {
        QNTRACE(
            "model::FavoritesModel",
            "Removing/skipping the search without "
                << "a name");
        removeItemByLocalId(search.localId());
        return;
    }

    if (!search.isFavorited()) {
        QNTRACE("model::FavoritesModel", "Removing/skipping non-favorited search");
        removeItemByLocalId(search.localId());
        return;
    }

    FavoritesModelItem item;
    item.setType(FavoritesModelItem::Type::SavedSearch);
    item.setLocalId(search.localId());
    item.setNoteCount(-1);
    item.setDisplayName(search.name());

    auto & rowIndex = m_data.get<ByIndex>();

    if (itemIt == localIdIndex.end()) {
        QNDEBUG("model::FavoritesModel", "Detected newly favorited saved search");

        Q_EMIT aboutToAddItem();

        int row = static_cast<int>(rowIndex.size());
        beginInsertRows(QModelIndex(), row, row);
        rowIndex.push_back(item);
        endInsertRows();

        updateItemRowWithRespectToSorting(item);

        auto addedSavedSearchIndex = indexForLocalId(item.localId());
        Q_EMIT addedItem(addedSavedSearchIndex);

        return;
    }

    QNDEBUG(
        "model::FavoritesModel",
        "Updating the already favorited saved search "
            << "item");

    auto indexIt = m_data.project<ByIndex>(itemIt);
    if (Q_UNLIKELY(indexIt == rowIndex.end())) {
        ErrorString error(
            QT_TR_NOOP("Internal error: can't project the local id "
                       "index iterator to the random access index "
                       "iterator within the favorites model"));

        QNWARNING(
            "model::FavoritesModel", error << ", favorites model item: " << item);

        Q_EMIT notifyError(error);
        return;
    }

    int row = static_cast<int>(std::distance(rowIndex.begin(), indexIt));

    auto modelIndex = createIndex(row, static_cast<int>(Column::DisplayName));
    Q_EMIT aboutToUpdateItem(modelIndex);

    Q_UNUSED(localIdIndex.replace(itemIt, item))

    Q_EMIT dataChanged(modelIndex, modelIndex);

    updateItemRowWithRespectToSorting(item);

    modelIndex = indexForLocalId(item.localId());
    Q_EMIT updatedItem(modelIndex);
}

void FavoritesModel::updateItemColumnInView(
    const FavoritesModelItem & item, const Column column)
{
    QNDEBUG(
        "model::FavoritesModel",
        "FavoritesModel::updateItemColumnInView: item = "
            << item << "\nColumn = " << column);

    const auto & rowIndex = m_data.get<ByIndex>();
    const auto & localIdIndex = m_data.get<ByLocalId>();

    auto itemIt = localIdIndex.find(item.localId());
    if (itemIt == localIdIndex.end()) {
        QNDEBUG("model::FavoritesModel", "Can't find item by local id");
        return;
    }

    if (m_sortedColumn != column) {
        auto itemIndexIt = m_data.project<ByIndex>(itemIt);
        if (Q_LIKELY(itemIndexIt != rowIndex.end())) {
            int row =
                static_cast<int>(std::distance(rowIndex.begin(), itemIndexIt));

            auto modelIndex = createIndex(row, static_cast<int>(column));

            QNTRACE(
                "model::FavoritesModel",
                "Emitting dataChanged signal for row " << row << " and column "
                                                       << column);

            Q_EMIT dataChanged(modelIndex, modelIndex);
            return;
        }

        ErrorString error(
            QT_TR_NOOP("Internal error: can't project the local id "
                       "index iterator to the random access index "
                       "iterator within the favorites model"));

        QNWARNING(
            "model::FavoritesModel", error << ", favorites model item: " << item);

        Q_EMIT notifyError(error);
    }

    updateItemRowWithRespectToSorting(item);
}

void FavoritesModel::checkAllItemsListed()
{
    if (m_allItemsListed) {
        return;
    }

    if (m_listNotesRequestId.isNull() && m_listNotebooksRequestId.isNull() &&
        m_listTagsRequestId.isNull() && m_listSavedSearchesRequestId.isNull())
    {
        QNDEBUG("model::FavoritesModel", "Listed all favorites model's items");
        m_allItemsListed = true;
        Q_EMIT notifyAllItemsListed();
    }
}

bool FavoritesModel::Comparator::operator()(
    const FavoritesModelItem & lhs, const FavoritesModelItem & rhs) const
{
    bool less = false;
    bool greater = false;

    switch (m_sortedColumn) {
    case Column::DisplayName:
    {
        int compareResult =
            lhs.displayName().localeAwareCompare(rhs.displayName());

        less = compareResult < 0;
        greater = compareResult > 0;
        break;
    }
    case Column::Type:
    {
        less = lhs.type() < rhs.type();
        greater = lhs.type() > rhs.type();
        break;
    }
    case Column::NoteCount:
    {
        less = lhs.noteCount() < rhs.noteCount();
        greater = lhs.noteCount() > rhs.noteCount();
        break;
    }
    default:
        return false;
    }

    if (m_sortOrder == Qt::AscendingOrder) {
        return less;
    }
    else {
        return greater;
    }
}

QDebug & operator<<(QDebug & dbg, const FavoritesModel::Column column)
{
    using Column = FavoritesModel::Column;

    switch (column) {
    case Column::Type:
        dbg << "Type";
        break;
    case Column::DisplayName:
        dbg << "DisplayName";
        break;
    case Column::NoteCount:
        dbg << "Note count";
        break;
    default:
        dbg << "Unknown (" << static_cast<qint64>(column) << ")";
        break;
    }

    return dbg;
}

} // namespace quentier
