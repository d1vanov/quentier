/*
 * Copyright 2016-2021 Dmitry Ivanov
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

#include <quentier/logging/QuentierLogger.h>
#include <quentier/types/NoteUtils.h>
#include <quentier/types/Validation.h>

#include <algorithm>
#include <cstddef>
#include <utility>

namespace quentier {

namespace {

// Limit for the queries to the local storage
constexpr std::size_t gNoteListLimit = 40;
constexpr std::size_t gNotebookListLimit = 40;
constexpr std::size_t gTagListLimit = 40;
constexpr std::size_t gSavedSearchListLimit = 40;

constexpr int gFavoritesModelColumnCount = 3;

} // namespace

FavoritesModel::FavoritesModel(
    const Account & account,
    LocalStorageManagerAsync & localStorageManagerAsync, NoteCache & noteCache,
    NotebookCache & notebookCache, TagCache & tagCache,
    SavedSearchCache & savedSearchCache, QObject * parent) :
    AbstractItemModel(account, parent),
    m_noteCache(noteCache), m_notebookCache(notebookCache),
    m_tagCache(tagCache), m_savedSearchCache(savedSearchCache)
{
    createConnections(localStorageManagerAsync);

    requestNotebooksList();
    requestTagsList();
    requestNotesList();
    requestSavedSearchesList();
}

FavoritesModel::~FavoritesModel() = default;

const FavoritesModelItem * FavoritesModel::itemForLocalId(
    const QString & localId) const
{
    const auto & localIdIndex = m_data.get<ByLocalId>();
    const auto it = localIdIndex.find(localId);
    if (Q_UNLIKELY(it == localIdIndex.end())) {
        QNDEBUG(
            "model:favorites",
            "Can't find favorites model item by local id: " << localId);
        return nullptr;
    }

    return &(*it);
}

const FavoritesModelItem * FavoritesModel::itemAtRow(const int row) const
{
    const auto & rowIndex = m_data.get<ByIndex>();
    if (Q_UNLIKELY((row < 0) ||
                   (rowIndex.size() <= static_cast<std::size_t>(row))))
    {
        QNDEBUG(
            "model:favorites",
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
            "model:favorites",
            "Can't find favorites model item by local id: " << localId);
        return {};
    }

    const auto & rowIndex = m_data.get<ByIndex>();
    const auto indexIt = m_data.project<ByIndex>(it);
    if (Q_UNLIKELY(indexIt == rowIndex.end())) {
        QNWARNING(
            "model:favorites",
            "Can't find indexed reference to the favorites model item: "
                << *it);
        return {};
    }

    const int row = static_cast<int>(std::distance(rowIndex.begin(), indexIt));
    return createIndex(row, static_cast<int>(Column::DisplayName));
}

QString FavoritesModel::itemNameForLocalId(const QString & localId) const
{
    const auto * pItem = itemForLocalId(localId);
    if (!pItem) {
        return {};
    }

    return pItem->displayName();
}

AbstractItemModel::ItemInfo FavoritesModel::itemInfoForLocalId(
    const QString & localId) const
{
    const auto * pItem = itemForLocalId(localId);
    if (!pItem) {
        return {};
    }

    ItemInfo info;
    info.m_localId = pItem->localId();
    info.m_name = pItem->localId();
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
    const auto & item = rowIndex[static_cast<std::size_t>(index.row())];
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
        return QVariant(section + 1);
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
        QString newDisplayName = value.toString().trimmed();
        bool changed = (item.displayName() != newDisplayName);
        if (!changed) {
            return true;
        }

        switch (item.type()) {
        case FavoritesModelItem::Type::Notebook:
        {
            auto it = m_lowerCaseNotebookNames.find(newDisplayName.toLower());
            if (it != m_lowerCaseNotebookNames.end()) {
                ErrorString error(
                    QT_TR_NOOP("Can't rename the notebook: no two "
                               "notebooks within the account are "
                               "allowed to have the same name in "
                               "case-insensitive manner"));
                error.details() = newDisplayName;
                QNINFO("model:favorites", error);
                Q_EMIT notifyError(error);
                return false;
            }

            ErrorString errorDescription;

            if (!validateNotebookName(newDisplayName, &errorDescription)) {
                ErrorString error(QT_TR_NOOP("Can't rename the notebook"));

                error.appendBase(errorDescription.base());
                error.appendBase(errorDescription.additionalBases());
                error.details() = errorDescription.details();

                QNINFO(
                    "model:favorites",
                    error << ", suggested new "
                          << "name = " << newDisplayName);

                Q_EMIT notifyError(error);
                return false;
            }

            // Removing the previous name and inserting the new one
            it = m_lowerCaseNotebookNames.find(item.displayName().toLower());

            if (Q_LIKELY(it != m_lowerCaseNotebookNames.end())) {
                Q_UNUSED(m_lowerCaseNotebookNames.erase(it))
            }

            Q_UNUSED(m_lowerCaseNotebookNames.insert(newDisplayName.toLower()))

            break;
        }
        case FavoritesModelItem::Type::Tag:
        {
            auto it = m_lowerCaseTagNames.find(newDisplayName.toLower());

            if (it != m_lowerCaseTagNames.end()) {
                ErrorString error(
                    QT_TR_NOOP("Can't rename the tag: no two tags "
                               "within the account are allowed to have "
                               "the same name in case-insensitive "
                               "manner"));

                QNINFO("model:favorites", error);
                Q_EMIT notifyError(error);
                return false;
            }

            ErrorString errorDescription;
            if (!validateTagName(newDisplayName, &errorDescription)) {
                ErrorString error(QT_TR_NOOP("Can't rename the tag"));
                error.appendBase(errorDescription.base());
                error.appendBase(errorDescription.additionalBases());
                error.details() = errorDescription.details();

                QNINFO(
                    "model:favorites",
                    error << ", suggested new "
                          << "name = " << newDisplayName);
                Q_EMIT notifyError(error);
                return false;
            }

            // Removing the previous name and inserting the new one
            it = m_lowerCaseTagNames.find(item.displayName().toLower());
            if (Q_LIKELY(it != m_lowerCaseTagNames.end())) {
                Q_UNUSED(m_lowerCaseTagNames.erase(it))
            }

            Q_UNUSED(m_lowerCaseTagNames.insert(newDisplayName.toLower()))

            break;
        }
        case FavoritesModelItem::Type::SavedSearch:
        {
            auto it =
                m_lowerCaseSavedSearchNames.find(newDisplayName.toLower());

            if (it != m_lowerCaseSavedSearchNames.end()) {
                ErrorString error(
                    QT_TR_NOOP("Can't rename the saved search: no two "
                               "saved searches within the account are "
                               "allowed to have the same name in "
                               "case-insensitive manner"));

                QNINFO("model:favorites", error);
                Q_EMIT notifyError(error);
                return false;
            }

            ErrorString errorDescription;

            if (!validateSavedSearchName(newDisplayName, &errorDescription)) {
                ErrorString error(QT_TR_NOOP("Can't rename the saved search"));

                error.appendBase(errorDescription.base());
                error.appendBase(errorDescription.additionalBases());
                error.details() = errorDescription.details();

                QNINFO(
                    "model:favorites",
                    error << ", suggested new "
                          << "name = " << newDisplayName);

                Q_EMIT notifyError(error);
                return false;
            }

            // Removing the previous name and inserting the new one
            it = m_lowerCaseSavedSearchNames.find(item.displayName().toLower());

            if (Q_LIKELY(it != m_lowerCaseSavedSearchNames.end())) {
                Q_UNUSED(m_lowerCaseSavedSearchNames.erase(it))
            }

            Q_UNUSED(
                m_lowerCaseSavedSearchNames.insert(newDisplayName.toLower()))

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
            "model:favorites",
            "Ignoring the attempt to remove rows from "
                << "favorites model for valid parent model index");
        return false;
    }

    if (Q_UNLIKELY((row + count - 1) >= static_cast<int>(m_data.size()))) {
        ErrorString error(
            QT_TR_NOOP("Detected attempt to remove more rows than "
                       "the favorites model contains"));

        QNINFO(
            "model:favorites",
            error << ", row = " << row << ", count = " << count
                  << ", number of favorites model items = " << m_data.size());

        Q_EMIT notifyError(error);
        return false;
    }

    FavoritesDataByIndex & rowIndex = m_data.get<ByIndex>();

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
                "model:favorites",
                "Favorites model item with unknown type detected: " << item);
            break;
        }
        }
    }

    Q_UNUSED(
        rowIndex.erase(rowIndex.begin() + row, rowIndex.begin() + row + count))

    for (auto it = notebookLocalIdsToRemove.begin(),
              end = notebookLocalIdsToRemove.end();
         it != end; ++it)
    {
        unfavoriteNotebook(*it);
    }

    for (auto it = noteLocalIdsToRemove.begin(),
              end = noteLocalIdsToRemove.end();
         it != end; ++it)
    {
        unfavoriteNote(*it);
    }

    for (auto it = tagLocalIdsToRemove.begin(),
              end = tagLocalIdsToRemove.end();
         it != end; ++it)
    {
        unfavoriteTag(*it);
    }

    for (auto it = savedSearchLocalIdsToRemove.begin(),
              end = savedSearchLocalIdsToRemove.end();
         it != end; ++it)
    {
        unfavoriteSavedSearch(*it);
    }

    endRemoveRows();

    return true;
}

void FavoritesModel::sort(int column, Qt::SortOrder order)
{
    QNDEBUG(
        "model:favorites",
        "FavoritesModel::sort: column = "
            << column << ", order = " << order << " ("
            << (order == Qt::AscendingOrder ? "ascending" : "descending")
            << ")");

    if (Q_UNLIKELY((column < 0) || (column >= gFavoritesModelColumnCount))) {
        return;
    }

    auto & rowIndex = m_data.get<ByIndex>();

    if (column == static_cast<int>(m_sortedColumn)) {
        if (order == m_sortOrder) {
            QNDEBUG(
                "model:favorites",
                "Neither sorted column nor sort order "
                    << "have changed, nothing to do");
            return;
        }

        m_sortOrder = order;

        QNDEBUG(
            "model:favorites",
            "Only the sort order has changed, reversing "
                << "the index");

        Q_EMIT layoutAboutToBeChanged();
        rowIndex.reverse();
        Q_EMIT layoutChanged();

        return;
    }

    m_sortedColumn = static_cast<Column>(column);
    m_sortOrder = order;

    Q_EMIT layoutAboutToBeChanged();

    auto persistentIndices = persistentIndexList();
    QVector<std::pair<QString, int>> localIdsToUpdateWithColumns;
    localIdsToUpdateWithColumns.reserve(persistentIndices.size());

    QStringList localIdsToUpdate;
    for (const auto & modelIndex: qAsConst(persistentIndices)) {
        int column = modelIndex.column();

        if (!modelIndex.isValid()) {
            localIdsToUpdateWithColumns
                << std::pair<QString, int>(QString(), column);
            continue;
        }

        const int row = modelIndex.row();

        if ((row < 0) || (row >= static_cast<int>(m_data.size())) ||
            (column < 0) || (column >= gFavoritesModelColumnCount))
        {
            localIdsToUpdateWithColumns
                << std::pair<QString, int>(QString(), column);
        }

        auto itemIt = rowIndex.begin() + row;
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

    for (const auto & pair: qAsConst(localIdsToUpdateWithColumns)) {
        const QString & localId = pair.first;
        const int column = pair.second;

        if (localId.isEmpty()) {
            replacementIndices << QModelIndex();
            continue;
        }

        auto newIndex = indexForLocalId(localId);
        if (!newIndex.isValid()) {
            replacementIndices << QModelIndex();
            continue;
        }

        auto newIndexWithColumn = createIndex(newIndex.row(), column);
        replacementIndices << newIndexWithColumn;
    }

    changePersistentIndexList(persistentIndices, replacementIndices);

    Q_EMIT layoutChanged();
}

void FavoritesModel::onAddNoteComplete(qevercloud::Note note, QUuid requestId)
{
    QNDEBUG(
        "model:favorites",
        "FavoritesModel::onAddNoteComplete: note = "
            << note << "\nRequest id = " << requestId);

    onNoteAddedOrUpdated(note);
}

void FavoritesModel::onUpdateNoteComplete(
    qevercloud::Note note, LocalStorageManager::UpdateNoteOptions options,
    QUuid requestId)
{
    QNDEBUG(
        "model:favorites",
        "FavoritesModel::onUpdateNoteComplete: note = "
            << note << "\nUpdate resource metadata = "
            << ((options &
                 LocalStorageManager::UpdateNoteOption::UpdateResourceMetadata)
                    ? "true"
                    : "false")
            << ", update resource binary data = "
            << ((options &
                 LocalStorageManager::UpdateNoteOption::
                     UpdateResourceBinaryData)
                    ? "true"
                    : "false")
            << ", update tags = "
            << ((options & LocalStorageManager::UpdateNoteOption::UpdateTags)
                    ? "true"
                    : "false")
            << ", request id = " << requestId);

    const auto it = m_updateNoteRequestIds.find(requestId);
    if (it != m_updateNoteRequestIds.end()) {
        Q_UNUSED(m_updateNoteRequestIds.erase(it))
        return;
    }

    onNoteAddedOrUpdated(
        note, (options & LocalStorageManager::UpdateNoteOption::UpdateTags));
}

void FavoritesModel::onNoteMovedToAnotherNotebook(
    QString noteLocalId, QString previousNotebookLocalId,
    QString newNotebookLocalId)
{
    QNDEBUG(
        "model:favorites",
        "FavoritesModel::onNoteMovedToAnotherNotebook: "
            << "note local id = " << noteLocalId
            << ", previous notebook local id = " << previousNotebookLocalId
            << ", new notebook local id = " << newNotebookLocalId);

    checkAndDecrementNoteCountPerNotebook(previousNotebookLocalId);
    checkAndIncrementNoteCountPerNotebook(newNotebookLocalId);
}

void FavoritesModel::onNoteTagListChanged(
    QString noteLocalId, QStringList previousNoteTagLocalIds,
    QStringList newNoteTagLocalIds)
{
    QNDEBUG(
        "model:favorites",
        "FavoritesModel::onNoteTagListChanged: "
            << "note local id = " << noteLocalId
            << ", previous tag local ids = "
            << previousNoteTagLocalIds.join(QStringLiteral(","))
            << ", new tag local ids = "
            << newNoteTagLocalIds.join(QStringLiteral(",")));

    std::sort(previousNoteTagLocalIds.begin(), previousNoteTagLocalIds.end());
    std::sort(newNoteTagLocalIds.begin(), newNoteTagLocalIds.end());

    std::vector<QString> commonTagLocalIds;

    std::set_intersection(
        previousNoteTagLocalIds.begin(), previousNoteTagLocalIds.end(),
        newNoteTagLocalIds.begin(), newNoteTagLocalIds.end(),
        std::back_inserter(commonTagLocalIds));

    for (const auto & tagLocalId: qAsConst(previousNoteTagLocalIds)) {
        const auto it = std::find(
            commonTagLocalIds.begin(), commonTagLocalIds.end(), tagLocalId);

        if (it != commonTagLocalIds.end()) {
            continue;
        }

        checkAndDecrementNoteCountPerTag(tagLocalId);
    }

    for (const auto & tagLocalId: qAsConst(newNoteTagLocalIds)) {
        const auto it = std::find(
            commonTagLocalIds.begin(), commonTagLocalIds.end(), tagLocalId);

        if (it != commonTagLocalIds.end()) {
            continue;
        }

        checkAndIncrementNoteCountPerTag(tagLocalId);
    }
}

void FavoritesModel::onUpdateNoteFailed(
    qevercloud::Note note, LocalStorageManager::UpdateNoteOptions options,
    ErrorString errorDescription, QUuid requestId)
{
    const auto it = m_updateNoteRequestIds.find(requestId);
    if (it == m_updateNoteRequestIds.end()) {
        return;
    }

    QNDEBUG(
        "model:favorites",
        "FavoritesModel::onUpdateNoteFailed: note = "
            << note << "\nUpdate resource metadata = "
            << ((options &
                 LocalStorageManager::UpdateNoteOption::UpdateResourceMetadata)
                    ? "true"
                    : "false")
            << ", update resource binary data = "
            << ((options &
                 LocalStorageManager::UpdateNoteOption::
                     UpdateResourceBinaryData)
                    ? "true"
                    : "false")
            << ", update tags = "
            << ((options & LocalStorageManager::UpdateNoteOption::UpdateTags)
                    ? "true"
                    : "false")
            << ", error descrition: " << errorDescription
            << ", request id = " << requestId);

    Q_UNUSED(m_updateNoteRequestIds.erase(it))

    requestId = QUuid::createUuid();
    Q_UNUSED(m_findNoteToRestoreFailedUpdateRequestIds.insert(requestId))

    QNTRACE(
        "model:favorites",
        "Emitting the request to find a note: "
            << "local id = " << note.localId()
            << ", request id = " << requestId);

    const LocalStorageManager::GetNoteOptions getNoteOptions(
        LocalStorageManager::GetNoteOption::WithResourceMetadata);

    Q_EMIT findNote(note, getNoteOptions, requestId);
}

void FavoritesModel::onFindNoteComplete(
    qevercloud::Note note, LocalStorageManager::GetNoteOptions options,
    QUuid requestId)
{
    const auto restoreUpdateIt =
        m_findNoteToRestoreFailedUpdateRequestIds.find(requestId);

    const auto performUpdateIt =
        (restoreUpdateIt == m_findNoteToRestoreFailedUpdateRequestIds.end()
         ? m_findNoteToPerformUpdateRequestIds.find(requestId)
         : m_findNoteToPerformUpdateRequestIds.end());

    const auto unfavoriteIt =
        ((restoreUpdateIt == m_findNoteToRestoreFailedUpdateRequestIds.end() &&
          performUpdateIt == m_findNoteToPerformUpdateRequestIds.end())
         ? m_findNoteToUnfavoriteRequestIds.find(requestId)
         : m_findNoteToUnfavoriteRequestIds.end());

    if ((restoreUpdateIt == m_findNoteToRestoreFailedUpdateRequestIds.end()) &&
        (performUpdateIt == m_findNoteToPerformUpdateRequestIds.end()) &&
        (unfavoriteIt == m_findNoteToUnfavoriteRequestIds.end()))
    {
        return;
    }

    QNDEBUG(
        "model:favorites",
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
    qevercloud::Note note, LocalStorageManager::GetNoteOptions options,
    ErrorString errorDescription, QUuid requestId)
{
    const auto restoreUpdateIt =
        m_findNoteToRestoreFailedUpdateRequestIds.find(requestId);

    const auto performUpdateIt =
        (restoreUpdateIt == m_findNoteToRestoreFailedUpdateRequestIds.end()
         ? m_findNoteToPerformUpdateRequestIds.find(requestId)
         : m_findNoteToPerformUpdateRequestIds.end());

    const auto unfavoriteIt =
        ((restoreUpdateIt == m_findNoteToRestoreFailedUpdateRequestIds.end() &&
          performUpdateIt == m_findNoteToPerformUpdateRequestIds.end())
         ? m_findNoteToUnfavoriteRequestIds.find(requestId)
         : m_findNoteToUnfavoriteRequestIds.end());

    if ((restoreUpdateIt == m_findNoteToRestoreFailedUpdateRequestIds.end()) &&
        (performUpdateIt == m_findNoteToPerformUpdateRequestIds.end()) &&
        (unfavoriteIt == m_findNoteToUnfavoriteRequestIds.end()))
    {
        return;
    }

    QNDEBUG(
        "model:favorites",
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

void FavoritesModel::onListNotesComplete(
    LocalStorageManager::ListObjectsOptions flag,
    LocalStorageManager::GetNoteOptions options, size_t limit, size_t offset,
    LocalStorageManager::ListNotesOrder order,
    LocalStorageManager::OrderDirection orderDirection,
    QString linkedNotebookGuid, QList<qevercloud::Note> foundNotes,
    QUuid requestId)
{
    if (requestId != m_listNotesRequestId) {
        return;
    }

    QNDEBUG(
        "model:favorites",
        "FavoritesModel::onListNotesComplete: flag = "
            << flag << ", with resource metadata = "
            << ((options &
                 LocalStorageManager::GetNoteOption::WithResourceMetadata)
                    ? "true"
                    : "false")
            << ", with resource binary data = "
            << ((options &
                 LocalStorageManager::GetNoteOption::WithResourceBinaryData)
                    ? "true"
                    : "false")
            << ", limit = " << limit << ", offset = " << offset
            << ", order = " << order << ", direction = " << orderDirection
            << ", linked notebook guid = " << linkedNotebookGuid
            << ", num found notes = " << foundNotes.size()
            << ", request id = " << requestId);

    for (const auto & foundNote: qAsConst(foundNotes)) {
        onNoteAddedOrUpdated(foundNote);
    }

    m_listNotesRequestId = QUuid();

    if (!foundNotes.isEmpty()) {
        QNTRACE(
            "model:favorites",
            "The number of found notes is greater than "
                << "zero, requesting more notes from the local storage");
        m_listNotesOffset += static_cast<quint64>(foundNotes.size());
        requestNotesList();
        return;
    }

    checkAllItemsListed();
}

void FavoritesModel::onListNotesFailed(
    LocalStorageManager::ListObjectsOptions flag,
    LocalStorageManager::GetNoteOptions options, size_t limit, size_t offset,
    LocalStorageManager::ListNotesOrder order,
    LocalStorageManager::OrderDirection orderDirection,
    QString linkedNotebookGuid, ErrorString errorDescription, QUuid requestId)
{
    if (requestId != m_listNotesRequestId) {
        return;
    }

    QNDEBUG(
        "model:favorites",
        "FavoritesModel::onListNotesFailed: flag = "
            << flag << ", with resource metadata = "
            << ((options &
                 LocalStorageManager::GetNoteOption::WithResourceMetadata)
                    ? "true"
                    : "false")
            << ", with resource binary data = "
            << ((options &
                 LocalStorageManager::GetNoteOption::WithResourceBinaryData)
                    ? "true"
                    : "false")
            << ", limit = " << limit << ", offset = " << offset
            << ", order = " << order << ", direction = " << orderDirection
            << ", linked notebook guid = " << linkedNotebookGuid
            << ", error description = " << errorDescription
            << ", request id = " << requestId);

    m_listNotesRequestId = QUuid();

    Q_EMIT notifyError(errorDescription);
}

void FavoritesModel::onExpungeNoteComplete(
    qevercloud::Note note, QUuid requestId)
{
    QNDEBUG(
        "model:favorites",
        "FavoritesModel::onExpungeNoteComplete: note = "
            << note << "\nRequest id = " << requestId);

    removeItemByLocalId(note.localId());

    // Since it's unclear whether some notebook within the favorites model was
    // affected, need to check and re-subscribe to the note counts for all
    // notebooks
    requestNoteCountForAllNotebooks(NoteCountRequestOption::Force);
    requestNoteCountForAllTags(NoteCountRequestOption::Force);
}

void FavoritesModel::onAddNotebookComplete(
    qevercloud::Notebook notebook, QUuid requestId)
{
    QNDEBUG(
        "model:favorites",
        "FavoritesModel::onAddNotebookComplete: "
            << "notebook = " << notebook << ", request id = " << requestId);

    onNotebookAddedOrUpdated(notebook);
}

void FavoritesModel::onUpdateNotebookComplete(
    qevercloud::Notebook notebook, QUuid requestId)
{
    QNDEBUG(
        "model:favorites",
        "FavoritesModel::onUpdateNotebookComplete: "
            << "notebook = " << notebook << ", request id = " << requestId);

    const auto it = m_updateNotebookRequestIds.find(requestId);
    if (it != m_updateNotebookRequestIds.end()) {
        Q_UNUSED(m_updateNotebookRequestIds.erase(it))
        return;
    }

    onNotebookAddedOrUpdated(notebook);
}

void FavoritesModel::onUpdateNotebookFailed(
    qevercloud::Notebook notebook, ErrorString errorDescription,
    QUuid requestId)
{
    const auto it = m_updateNotebookRequestIds.find(requestId);
    if (it == m_updateNotebookRequestIds.end()) {
        return;
    }

    QNDEBUG(
        "model:favorites",
        "FavoritesModel::onUpdateNotebookFailed: "
            << "notebook = " << notebook << "\nError description = "
            << errorDescription << ", request id = " << requestId);

    Q_UNUSED(m_updateNotebookRequestIds.erase(it))

    requestId = QUuid::createUuid();
    Q_UNUSED(m_findNotebookToRestoreFailedUpdateRequestIds.insert(requestId))

    QNTRACE(
        "model:favorites",
        "Emitting the request to find a notebook: "
            << "local id = " << notebook.localId()
            << ", request id = " << requestId);

    Q_EMIT findNotebook(notebook, requestId);
}

void FavoritesModel::onFindNotebookComplete(
    qevercloud::Notebook notebook, QUuid requestId)
{
    const auto restoreUpdateIt =
        m_findNotebookToRestoreFailedUpdateRequestIds.find(requestId);

    auto performUpdateIt =
        (restoreUpdateIt == m_findNotebookToRestoreFailedUpdateRequestIds.end()
         ? m_findNotebookToPerformUpdateRequestIds.find(requestId)
         : m_findNotebookToPerformUpdateRequestIds.end());

    const auto unfavoriteIt =
        ((restoreUpdateIt == m_findNotebookToRestoreFailedUpdateRequestIds.end()
          && performUpdateIt == m_findNotebookToPerformUpdateRequestIds.end())
         ? m_findNotebookToUnfavoriteRequestIds.find(requestId)
         : m_findNotebookToUnfavoriteRequestIds.end());

    if ((restoreUpdateIt ==
         m_findNotebookToRestoreFailedUpdateRequestIds.end()) &&
        (performUpdateIt == m_findNotebookToPerformUpdateRequestIds.end()) &&
        (unfavoriteIt == m_findNotebookToUnfavoriteRequestIds.end()))
    {
        return;
    }

    QNDEBUG(
        "model:favorites",
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
        const auto it = localIdIndex.find(notebook.localId());
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
    qevercloud::Notebook notebook, ErrorString errorDescription,
    QUuid requestId)
{
    const auto restoreUpdateIt =
        m_findNotebookToRestoreFailedUpdateRequestIds.find(requestId);

    auto performUpdateIt =
        (restoreUpdateIt == m_findNotebookToRestoreFailedUpdateRequestIds.end()
         ? m_findNotebookToPerformUpdateRequestIds.find(requestId)
         : m_findNotebookToPerformUpdateRequestIds.end());

    const auto unfavoriteIt =
        ((restoreUpdateIt == m_findNotebookToRestoreFailedUpdateRequestIds.end()
          && performUpdateIt == m_findNotebookToPerformUpdateRequestIds.end())
         ? m_findNotebookToUnfavoriteRequestIds.find(requestId)
         : m_findNotebookToUnfavoriteRequestIds.end());

    if ((restoreUpdateIt ==
         m_findNotebookToRestoreFailedUpdateRequestIds.end()) &&
        (performUpdateIt == m_findNotebookToPerformUpdateRequestIds.end()) &&
        (unfavoriteIt == m_findNotebookToUnfavoriteRequestIds.end()))
    {
        return;
    }

    QNDEBUG(
        "model:favorites",
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

void FavoritesModel::onListNotebooksComplete(
    LocalStorageManager::ListObjectsOptions flag, size_t limit, size_t offset,
    LocalStorageManager::ListNotebooksOrder order,
    LocalStorageManager::OrderDirection orderDirection,
    QString linkedNotebookGuid, QList<qevercloud::Notebook> foundNotebooks,
    QUuid requestId)
{
    if (requestId != m_listNotebooksRequestId) {
        return;
    }

    QNDEBUG(
        "model:favorites",
        "FavoritesModel::onListNotebooksComplete: "
            << "flag = " << flag << ", limit = " << limit
            << ", offset = " << offset << ", order = " << order
            << ", direction = " << orderDirection << ", linked notebook guid = "
            << (linkedNotebookGuid.isNull() ? QStringLiteral("<null>")
                                            : linkedNotebookGuid)
            << ", num found notebooks = " << foundNotebooks.size()
            << ", request id = " << requestId);

    for (const auto & foundNotebook: qAsConst(foundNotebooks)) {
        onNotebookAddedOrUpdated(foundNotebook);
    }

    m_listNotebooksRequestId = QUuid();

    if (!foundNotebooks.isEmpty()) {
        QNTRACE(
            "model:favorites",
            "The number of found notebooks is greater "
                << "than zero, requesting more notebooks from the local "
                   "storage");
        m_listNotebooksOffset += static_cast<quint64>(foundNotebooks.size());
        requestNotebooksList();
        return;
    }

    checkAllItemsListed();
}

void FavoritesModel::onListNotebooksFailed(
    LocalStorageManager::ListObjectsOptions flag, size_t limit, size_t offset,
    LocalStorageManager::ListNotebooksOrder order,
    LocalStorageManager::OrderDirection orderDirection,
    QString linkedNotebookGuid, ErrorString errorDescription, QUuid requestId)
{
    if (requestId != m_listNotebooksRequestId) {
        return;
    }

    QNDEBUG(
        "model:favorites",
        "FavoritesModel::onListNotebooksFailed: flag = "
            << flag << ", limit = " << limit << ", offset = " << offset
            << ", order = " << order << ", direction = " << orderDirection
            << ", linked notebook guid = "
            << (linkedNotebookGuid.isNull() ? QStringLiteral("<null>")
                                            : linkedNotebookGuid)
            << ", error description = " << errorDescription
            << ", request id = " << requestId);

    m_listNotebooksRequestId = QUuid();

    Q_EMIT notifyError(errorDescription);
}

void FavoritesModel::onExpungeNotebookComplete(
    qevercloud::Notebook notebook, QUuid requestId)
{
    QNDEBUG(
        "model:favorites",
        "FavoritesModel::onExpungeNotebookComplete: notebook = " << notebook
            << "\nRequest id = " << requestId);

    removeItemByLocalId(notebook.localId());
}

void FavoritesModel::onAddTagComplete(qevercloud::Tag tag, QUuid requestId)
{
    QNDEBUG(
        "model:favorites",
        "FavoritesModel::onAddTagComplete: tag = " << tag << "\nRequest id = "
                                                   << requestId);

    onTagAddedOrUpdated(tag);
}

void FavoritesModel::onUpdateTagComplete(qevercloud::Tag tag, QUuid requestId)
{
    QNDEBUG(
        "model:favorites",
        "FavoritesModel::onUpdateTagComplete: tag = "
            << tag << "\nRequest id = " << requestId);

    const auto it = m_updateTagRequestIds.find(requestId);
    if (it != m_updateTagRequestIds.end()) {
        Q_UNUSED(m_updateTagRequestIds.erase(it))
        return;
    }

    onTagAddedOrUpdated(tag);
}

void FavoritesModel::onUpdateTagFailed(
    qevercloud::Tag tag, ErrorString errorDescription, QUuid requestId)
{
    const auto it = m_updateTagRequestIds.find(requestId);
    if (it == m_updateTagRequestIds.end()) {
        return;
    }

    QNDEBUG(
        "model:favorites",
        "FavoritesModel::onUpdateTagFailed: tag = "
            << tag << "\nError description = " << errorDescription
            << ", request id = " << requestId);

    Q_UNUSED(m_updateTagRequestIds.erase(it))

    requestId = QUuid::createUuid();
    Q_UNUSED(m_findTagToRestoreFailedUpdateRequestIds.insert(requestId))

    QNTRACE(
        "model:favorites",
        "Emitting the request to find a tag: local id = " << tag.localId()
            << ", request id = " << requestId);

    Q_EMIT findTag(tag, requestId);
}

void FavoritesModel::onFindTagComplete(qevercloud::Tag tag, QUuid requestId)
{
    const auto restoreUpdateIt =
        m_findTagToRestoreFailedUpdateRequestIds.find(requestId);

    const auto performUpdateIt =
        (restoreUpdateIt == m_findTagToRestoreFailedUpdateRequestIds.end()
         ? m_findTagToPerformUpdateRequestIds.find(requestId)
         : m_findTagToPerformUpdateRequestIds.end());

    const auto unfavoriteIt =
        ((restoreUpdateIt == m_findTagToRestoreFailedUpdateRequestIds.end() &&
          performUpdateIt == m_findTagToPerformUpdateRequestIds.end())
         ? m_findTagToUnfavoriteRequestIds.find(requestId)
         : m_findTagToUnfavoriteRequestIds.end());

    if ((restoreUpdateIt == m_findTagToRestoreFailedUpdateRequestIds.end()) &&
        (performUpdateIt == m_findTagToPerformUpdateRequestIds.end()) &&
        (unfavoriteIt == m_findTagToUnfavoriteRequestIds.end()))
    {
        return;
    }

    QNDEBUG(
        "model:favorites",
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
    qevercloud::Tag tag, ErrorString errorDescription, QUuid requestId)
{
    const auto restoreUpdateIt =
        m_findTagToRestoreFailedUpdateRequestIds.find(requestId);

    const auto performUpdateIt =
        (restoreUpdateIt == m_findTagToRestoreFailedUpdateRequestIds.end()
         ? m_findTagToPerformUpdateRequestIds.find(requestId)
         : m_findTagToPerformUpdateRequestIds.end());

    const auto unfavoriteIt =
        ((restoreUpdateIt == m_findTagToRestoreFailedUpdateRequestIds.end() &&
          performUpdateIt == m_findTagToPerformUpdateRequestIds.end())
         ? m_findTagToUnfavoriteRequestIds.find(requestId)
         : m_findTagToUnfavoriteRequestIds.end());

    if ((restoreUpdateIt == m_findTagToRestoreFailedUpdateRequestIds.end()) &&
        (performUpdateIt == m_findTagToPerformUpdateRequestIds.end()) &&
        (unfavoriteIt == m_findTagToUnfavoriteRequestIds.end()))
    {
        return;
    }

    QNDEBUG(
        "model:favorites",
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

void FavoritesModel::onListTagsComplete(
    LocalStorageManager::ListObjectsOptions flag, size_t limit, size_t offset,
    LocalStorageManager::ListTagsOrder order,
    LocalStorageManager::OrderDirection orderDirection,
    QString linkedNotebookGuid, QList<qevercloud::Tag> foundTags,
    QUuid requestId)
{
    if (requestId != m_listTagsRequestId) {
        return;
    }

    QNDEBUG(
        "model:favorites",
        "FavoritesModel::onListTagsComplete: flag = "
            << flag << ", limit = " << limit << ", offset = " << offset
            << ", order = " << order << ", direction = " << orderDirection
            << ", linked notebook guid = "
            << (linkedNotebookGuid.isNull() ? QStringLiteral("<null>")
                                            : linkedNotebookGuid)
            << ", num found tags = " << foundTags.size()
            << ", request id = " << requestId);

    for (const auto & foundTag: qAsConst(foundTags)) {
        onTagAddedOrUpdated(foundTag);
    }

    m_listTagsRequestId = QUuid();

    if (!foundTags.isEmpty()) {
        QNTRACE(
            "model:favorites",
            "The number of found tags is greater than zero, requesting more "
                << "tags from the local storage");
        m_listTagsOffset += static_cast<quint64>(foundTags.size());
        requestTagsList();
        return;
    }

    checkAllItemsListed();
}

void FavoritesModel::onListTagsFailed(
    LocalStorageManager::ListObjectsOptions flag, size_t limit, size_t offset,
    LocalStorageManager::ListTagsOrder order,
    LocalStorageManager::OrderDirection orderDirection,
    QString linkedNotebookGuid, ErrorString errorDescription, QUuid requestId)
{
    if (requestId != m_listTagsRequestId) {
        return;
    }

    QNDEBUG(
        "model:favorites",
        "FavoritesModel::onListTagsFailed: flag = "
            << flag << ", limit = " << limit << ", offset = " << offset
            << ", order = " << order << ", direction = " << orderDirection
            << ", linked notebook guid = "
            << (linkedNotebookGuid.isNull() ? QStringLiteral("<null>")
                                            : linkedNotebookGuid)
            << ", error description = " << errorDescription
            << ", request id = " << requestId);

    m_listTagsRequestId = QUuid();

    Q_EMIT notifyError(errorDescription);
}

void FavoritesModel::onExpungeTagComplete(
    qevercloud::Tag tag, QStringList expungedChildTagLocalIds, QUuid requestId)
{
    QNDEBUG(
        "model:favorites",
        "FavoritesModel::onExpungeTagComplete: tag = "
            << tag << "\nExpunged child tag local ids: "
            << expungedChildTagLocalIds.join(QStringLiteral(", "))
            << ", request id = " << requestId);

    for (const auto & tagLocalId: qAsConst(expungedChildTagLocalIds)) {
        removeItemByLocalId(tagLocalId);
    }

    removeItemByLocalId(tag.localId());
}

void FavoritesModel::onAddSavedSearchComplete(
    qevercloud::SavedSearch search, QUuid requestId)
{
    QNDEBUG(
        "model:favorites",
        "FavoritesModel::onAddSavedSearchComplete: "
            << search << "\nRequest id = " << requestId);

    onSavedSearchAddedOrUpdated(search);
}

void FavoritesModel::onUpdateSavedSearchComplete(
    qevercloud::SavedSearch search, QUuid requestId)
{
    QNDEBUG(
        "model:favorites",
        "FavoritesModel::onUpdateSavedSearchComplete: "
            << search << "\nRequest id = " << requestId);

    const auto it = m_updateSavedSearchRequestIds.find(requestId);
    if (it != m_updateSavedSearchRequestIds.end()) {
        Q_UNUSED(m_updateSavedSearchRequestIds.erase(it))
        return;
    }

    onSavedSearchAddedOrUpdated(search);
}

void FavoritesModel::onUpdateSavedSearchFailed(
    qevercloud::SavedSearch search, ErrorString errorDescription,
    QUuid requestId)
{
    const auto it = m_updateSavedSearchRequestIds.find(requestId);
    if (it == m_updateSavedSearchRequestIds.end()) {
        return;
    }

    QNDEBUG(
        "model:favorites",
        "FavoritesModel::onUpdateSavedSearchFailed: "
            << "search = " << search << "\nError description = "
            << errorDescription << ", request id = " << requestId);

    Q_UNUSED(m_updateSavedSearchRequestIds.erase(it))

    requestId = QUuid::createUuid();
    Q_UNUSED(m_findSavedSearchToRestoreFailedUpdateRequestIds.insert(requestId))

    QNTRACE(
        "model:favorites",
        "Emitting the request to find the saved search: "
            << "local id = " << search.localId()
            << ", request id = " << requestId);

    Q_EMIT findSavedSearch(search, requestId);
}

void FavoritesModel::onFindSavedSearchComplete(
    qevercloud::SavedSearch search, QUuid requestId)
{
    const auto restoreUpdateIt =
        m_findSavedSearchToRestoreFailedUpdateRequestIds.find(requestId);

    const auto performUpdateIt =
        (restoreUpdateIt ==
         m_findSavedSearchToRestoreFailedUpdateRequestIds.end()
         ? m_findSavedSearchToPerformUpdateRequestIds.find(requestId)
         : m_findSavedSearchToPerformUpdateRequestIds.end());

    const auto unfavoriteIt =
        ((restoreUpdateIt ==
          m_findSavedSearchToRestoreFailedUpdateRequestIds.end() &&
          performUpdateIt == m_findSavedSearchToPerformUpdateRequestIds.end())
         ? m_findSavedSearchToUnfavoriteRequestIds.find(requestId)
         : m_findSavedSearchToUnfavoriteRequestIds.end());

    if ((restoreUpdateIt ==
         m_findSavedSearchToRestoreFailedUpdateRequestIds.end()) &&
        (performUpdateIt == m_findSavedSearchToPerformUpdateRequestIds.end()) &&
        (unfavoriteIt == m_findSavedSearchToUnfavoriteRequestIds.end()))
    {
        return;
    }

    QNDEBUG(
        "model:favorites",
        "FavoritesModel::onFindSavedSearchComplete: search = " << search
            << "\nRequest id = " << requestId);

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
        const auto it = localIdIndex.find(search.localId());
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
    qevercloud::SavedSearch search, ErrorString errorDescription,
    QUuid requestId)
{
    const auto restoreUpdateIt =
        m_findSavedSearchToRestoreFailedUpdateRequestIds.find(requestId);

    const auto performUpdateIt =
        (restoreUpdateIt ==
         m_findSavedSearchToRestoreFailedUpdateRequestIds.end()
         ? m_findSavedSearchToPerformUpdateRequestIds.find(requestId)
         : m_findSavedSearchToPerformUpdateRequestIds.end());

    const auto unfavoriteIt =
        ((restoreUpdateIt ==
          m_findSavedSearchToRestoreFailedUpdateRequestIds.end() &&
          performUpdateIt == m_findSavedSearchToPerformUpdateRequestIds.end())
         ? m_findSavedSearchToUnfavoriteRequestIds.find(requestId)
         : m_findSavedSearchToUnfavoriteRequestIds.end());

    if ((restoreUpdateIt ==
         m_findSavedSearchToRestoreFailedUpdateRequestIds.end()) &&
        (performUpdateIt == m_findSavedSearchToPerformUpdateRequestIds.end()) &&
        (unfavoriteIt == m_findSavedSearchToUnfavoriteRequestIds.end()))
    {
        return;
    }

    QNWARNING(
        "model:favorites",
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

void FavoritesModel::onListSavedSearchesComplete(
    LocalStorageManager::ListObjectsOptions flag, size_t limit, size_t offset,
    LocalStorageManager::ListSavedSearchesOrder order,
    LocalStorageManager::OrderDirection orderDirection,
    QList<qevercloud::SavedSearch> foundSearches, QUuid requestId)
{
    if (requestId != m_listSavedSearchesRequestId) {
        return;
    }

    QNDEBUG(
        "model:favorites",
        "FavoritesModel::onListSavedSearchesComplete: flag = " << flag
            << ", limit = " << limit << ", offset = " << offset << ", order = "
            << order << ", direction = " << orderDirection
            << ", num found searches = " << foundSearches.size()
            << ", request id = " << requestId);

    for (auto it = foundSearches.begin(), end = foundSearches.end(); it != end;
         ++it)
    {
        onSavedSearchAddedOrUpdated(*it);
    }

    m_listSavedSearchesRequestId = QUuid();

    if (!foundSearches.isEmpty()) {
        QNTRACE(
            "model:favorites",
            "The number of found saved searches is not empty, requesting more "
                << "saved searches from the local storage");
        m_listSavedSearchesOffset += static_cast<quint64>(foundSearches.size());
        requestSavedSearchesList();
        return;
    }

    checkAllItemsListed();
}

void FavoritesModel::onListSavedSearchesFailed(
    LocalStorageManager::ListObjectsOptions flag, size_t limit, size_t offset,
    LocalStorageManager::ListSavedSearchesOrder order,
    LocalStorageManager::OrderDirection orderDirection,
    ErrorString errorDescription, QUuid requestId)
{
    if (requestId != m_listSavedSearchesRequestId) {
        return;
    }

    QNDEBUG(
        "model:favorites",
        "FavoritesModel::onListSavedSearchesFailed: flag = " << flag
            << ", limit = " << limit << ", offset = " << offset << ", order = "
            << order << ", direction = " << orderDirection << ", error: "
            << errorDescription << ", request id = " << requestId);

    m_listSavedSearchesRequestId = QUuid();

    Q_EMIT notifyError(errorDescription);
}

void FavoritesModel::onExpungeSavedSearchComplete(
    qevercloud::SavedSearch search, QUuid requestId)
{
    QNDEBUG(
        "model:favorites",
        "FavoritesModel::onExpungeSavedSearchComplete: search = " << search
            << "\nRequest id = " << requestId);

    removeItemByLocalId(search.localId());
}

void FavoritesModel::onGetNoteCountPerNotebookComplete(
    int noteCount, qevercloud::Notebook notebook,
    LocalStorageManager::NoteCountOptions options, QUuid requestId)
{
    Q_UNUSED(options)

    const auto it =
        m_notebookLocalIdToNoteCountRequestIdBimap.right.find(requestId);

    if (it == m_notebookLocalIdToNoteCountRequestIdBimap.right.end()) {
        return;
    }

    QNDEBUG(
        "model:favorites",
        "FavoritesModel::onGetNoteCountPerNotebookComplete: note count = "
            << noteCount << ", notebook local id = " << notebook.localId()
            << ", request id = " << requestId);

    Q_UNUSED(m_notebookLocalIdToNoteCountRequestIdBimap.right.erase(it))

    auto & localIdIndex = m_data.get<ByLocalId>();
    const auto itemIt = localIdIndex.find(notebook.localId());
    if (Q_UNLIKELY(itemIt == localIdIndex.end())) {
        QNDEBUG(
            "model:favorites",
            "Can't find the notebook item within the favorites model for which "
                << "the note count was received");
        return;
    }

    FavoritesModelItem item = *itemIt;
    item.setNoteCount(noteCount);
    Q_UNUSED(localIdIndex.replace(itemIt, item))
    updateItemColumnInView(item, Column::NoteCount);
}

void FavoritesModel::onGetNoteCountPerNotebookFailed(
    ErrorString errorDescription, qevercloud::Notebook notebook,
    LocalStorageManager::NoteCountOptions options, QUuid requestId)
{
    Q_UNUSED(options)

    const auto it =
        m_notebookLocalIdToNoteCountRequestIdBimap.right.find(requestId);
    if (it == m_notebookLocalIdToNoteCountRequestIdBimap.right.end()) {
        return;
    }

    QNDEBUG(
        "model:favorites",
        "FavoritesModel::onGetNoteCountPerNotebookFailed: "
            << "error description = " << errorDescription
            << "\nNotebook local id = " << notebook.localId()
            << ", request id = " << requestId);

    Q_UNUSED(m_notebookLocalIdToNoteCountRequestIdBimap.right.erase(it))

    QNWARNING(
        "model:favorites", errorDescription << ", notebook: " << notebook);

    Q_EMIT notifyError(errorDescription);
}

void FavoritesModel::onGetNoteCountPerTagComplete(
    int noteCount, qevercloud::Tag tag,
    LocalStorageManager::NoteCountOptions options, QUuid requestId)
{
    Q_UNUSED(options)

    const auto it = m_tagLocalIdToNoteCountRequestIdBimap.right.find(requestId);
    if (it == m_tagLocalIdToNoteCountRequestIdBimap.right.end()) {
        return;
    }

    QNDEBUG(
        "model:favorites",
        "FavoritesModel::onGetNoteCountPerTagComplete: note count = "
            << noteCount << ", tag local id = " << tag.localId()
            << ", request id = " << requestId);

    Q_UNUSED(m_tagLocalIdToNoteCountRequestIdBimap.right.erase(it))

    auto & localIdIndex = m_data.get<ByLocalId>();
    const auto itemIt = localIdIndex.find(tag.localId());
    if (Q_UNLIKELY(itemIt == localIdIndex.end())) {
        QNDEBUG(
            "model:favorites",
            "Can't find the tag item within "
                << "the favorites model for which the note count was received");
        return;
    }

    FavoritesModelItem item = *itemIt;
    item.setNoteCount(noteCount);
    Q_UNUSED(localIdIndex.replace(itemIt, item))
    updateItemColumnInView(item, Column::NoteCount);
}

void FavoritesModel::onGetNoteCountPerTagFailed(
    ErrorString errorDescription, qevercloud::Tag tag,
    LocalStorageManager::NoteCountOptions options, QUuid requestId)
{
    Q_UNUSED(options)

    const auto it = m_tagLocalIdToNoteCountRequestIdBimap.right.find(requestId);
    if (it == m_tagLocalIdToNoteCountRequestIdBimap.right.end()) {
        return;
    }

    QNDEBUG(
        "model:favorites",
        "FavoritesModel::onGetNoteCountPerTagFailed: "
            << "error description = " << errorDescription
            << "\nTag local id = " << tag.localId()
            << ", request id = " << requestId);

    Q_UNUSED(m_tagLocalIdToNoteCountRequestIdBimap.right.erase(it))

    QNWARNING("model:favorites", errorDescription << ", tag: " << tag);
    Q_EMIT notifyError(errorDescription);
}

void FavoritesModel::createConnections(
    LocalStorageManagerAsync & localStorageManagerAsync)
{
    QNDEBUG("model:favorites", "FavoritesModel::createConnections");

    // Connect local signals to localStorageManagerAsync's slots
    QObject::connect(
        this, &FavoritesModel::updateNote, &localStorageManagerAsync,
        &LocalStorageManagerAsync::onUpdateNoteRequest);

    QObject::connect(
        this, &FavoritesModel::findNote, &localStorageManagerAsync,
        &LocalStorageManagerAsync::onFindNoteRequest);

    QObject::connect(
        this, &FavoritesModel::listNotes, &localStorageManagerAsync,
        &LocalStorageManagerAsync::onListNotesRequest);

    QObject::connect(
        this, &FavoritesModel::updateNotebook, &localStorageManagerAsync,
        &LocalStorageManagerAsync::onUpdateNotebookRequest);

    QObject::connect(
        this, &FavoritesModel::findNotebook, &localStorageManagerAsync,
        &LocalStorageManagerAsync::onFindNotebookRequest);

    QObject::connect(
        this, &FavoritesModel::listNotebooks, &localStorageManagerAsync,
        &LocalStorageManagerAsync::onListNotebooksRequest);

    QObject::connect(
        this, &FavoritesModel::updateTag, &localStorageManagerAsync,
        &LocalStorageManagerAsync::onUpdateTagRequest);

    QObject::connect(
        this, &FavoritesModel::findTag, &localStorageManagerAsync,
        &LocalStorageManagerAsync::onFindTagRequest);

    QObject::connect(
        this, &FavoritesModel::listTags, &localStorageManagerAsync,
        &LocalStorageManagerAsync::onListTagsRequest);

    QObject::connect(
        this, &FavoritesModel::updateSavedSearch, &localStorageManagerAsync,
        &LocalStorageManagerAsync::onUpdateSavedSearchRequest);

    QObject::connect(
        this, &FavoritesModel::findSavedSearch, &localStorageManagerAsync,
        &LocalStorageManagerAsync::onFindSavedSearchRequest);

    QObject::connect(
        this, &FavoritesModel::listSavedSearches, &localStorageManagerAsync,
        &LocalStorageManagerAsync::onListSavedSearchesRequest);

    QObject::connect(
        this, &FavoritesModel::noteCountPerNotebook, &localStorageManagerAsync,
        &LocalStorageManagerAsync::onGetNoteCountPerNotebookRequest);

    QObject::connect(
        this, &FavoritesModel::noteCountPerTag, &localStorageManagerAsync,
        &LocalStorageManagerAsync::onGetNoteCountPerTagRequest);

    // Connect localStorageManagerAsync's signals to local slots
    QObject::connect(
        &localStorageManagerAsync, &LocalStorageManagerAsync::addNoteComplete,
        this, &FavoritesModel::onAddNoteComplete);

    QObject::connect(
        &localStorageManagerAsync,
        &LocalStorageManagerAsync::updateNoteComplete, this,
        &FavoritesModel::onUpdateNoteComplete);

    QObject::connect(
        &localStorageManagerAsync,
        &LocalStorageManagerAsync::noteMovedToAnotherNotebook, this,
        &FavoritesModel::onNoteMovedToAnotherNotebook);

    QObject::connect(
        &localStorageManagerAsync,
        &LocalStorageManagerAsync::noteTagListChanged, this,
        &FavoritesModel::onNoteTagListChanged);

    QObject::connect(
        &localStorageManagerAsync, &LocalStorageManagerAsync::updateNoteFailed,
        this, &FavoritesModel::onUpdateNoteFailed);

    QObject::connect(
        &localStorageManagerAsync, &LocalStorageManagerAsync::findNoteComplete,
        this, &FavoritesModel::onFindNoteComplete);

    QObject::connect(
        &localStorageManagerAsync, &LocalStorageManagerAsync::findNoteFailed,
        this, &FavoritesModel::onFindNoteFailed);

    QObject::connect(
        &localStorageManagerAsync, &LocalStorageManagerAsync::listNotesComplete,
        this, &FavoritesModel::onListNotesComplete);

    QObject::connect(
        &localStorageManagerAsync, &LocalStorageManagerAsync::listNotesFailed,
        this, &FavoritesModel::onListNotesFailed);

    QObject::connect(
        &localStorageManagerAsync,
        &LocalStorageManagerAsync::expungeNoteComplete, this,
        &FavoritesModel::onExpungeNoteComplete);

    QObject::connect(
        &localStorageManagerAsync,
        &LocalStorageManagerAsync::addNotebookComplete, this,
        &FavoritesModel::onAddNotebookComplete);

    QObject::connect(
        &localStorageManagerAsync,
        &LocalStorageManagerAsync::updateNotebookComplete, this,
        &FavoritesModel::onUpdateNotebookComplete);

    QObject::connect(
        &localStorageManagerAsync,
        &LocalStorageManagerAsync::updateNotebookFailed, this,
        &FavoritesModel::onUpdateNotebookFailed);

    QObject::connect(
        &localStorageManagerAsync,
        &LocalStorageManagerAsync::findNotebookComplete, this,
        &FavoritesModel::onFindNotebookComplete);

    QObject::connect(
        &localStorageManagerAsync,
        &LocalStorageManagerAsync::findNotebookFailed, this,
        &FavoritesModel::onFindNotebookFailed);

    QObject::connect(
        &localStorageManagerAsync,
        &LocalStorageManagerAsync::listNotebooksComplete, this,
        &FavoritesModel::onListNotebooksComplete);

    QObject::connect(
        &localStorageManagerAsync,
        &LocalStorageManagerAsync::listNotebooksFailed, this,
        &FavoritesModel::onListNotebooksFailed);

    QObject::connect(
        &localStorageManagerAsync,
        &LocalStorageManagerAsync::expungeNotebookComplete, this,
        &FavoritesModel::onExpungeNotebookComplete);

    QObject::connect(
        &localStorageManagerAsync, &LocalStorageManagerAsync::addTagComplete,
        this, &FavoritesModel::onAddTagComplete);

    QObject::connect(
        &localStorageManagerAsync, &LocalStorageManagerAsync::updateTagComplete,
        this, &FavoritesModel::onUpdateTagComplete);

    QObject::connect(
        &localStorageManagerAsync, &LocalStorageManagerAsync::updateTagFailed,
        this, &FavoritesModel::onUpdateTagFailed);

    QObject::connect(
        &localStorageManagerAsync, &LocalStorageManagerAsync::findTagComplete,
        this, &FavoritesModel::onFindTagComplete);

    QObject::connect(
        &localStorageManagerAsync, &LocalStorageManagerAsync::findTagFailed,
        this, &FavoritesModel::onFindTagFailed);

    QObject::connect(
        &localStorageManagerAsync, &LocalStorageManagerAsync::listTagsComplete,
        this, &FavoritesModel::onListTagsComplete);

    QObject::connect(
        &localStorageManagerAsync, &LocalStorageManagerAsync::listTagsFailed,
        this, &FavoritesModel::onListTagsFailed);

    QObject::connect(
        &localStorageManagerAsync,
        &LocalStorageManagerAsync::expungeTagComplete, this,
        &FavoritesModel::onExpungeTagComplete);

    QObject::connect(
        &localStorageManagerAsync,
        &LocalStorageManagerAsync::addSavedSearchComplete, this,
        &FavoritesModel::onAddSavedSearchComplete);

    QObject::connect(
        &localStorageManagerAsync,
        &LocalStorageManagerAsync::updateSavedSearchComplete, this,
        &FavoritesModel::onUpdateSavedSearchComplete);

    QObject::connect(
        &localStorageManagerAsync,
        &LocalStorageManagerAsync::updateSavedSearchFailed, this,
        &FavoritesModel::onUpdateSavedSearchFailed);

    QObject::connect(
        &localStorageManagerAsync,
        &LocalStorageManagerAsync::findSavedSearchComplete, this,
        &FavoritesModel::onFindSavedSearchComplete);

    QObject::connect(
        &localStorageManagerAsync,
        &LocalStorageManagerAsync::findSavedSearchFailed, this,
        &FavoritesModel::onFindSavedSearchFailed);

    QObject::connect(
        &localStorageManagerAsync,
        &LocalStorageManagerAsync::listSavedSearchesComplete, this,
        &FavoritesModel::onListSavedSearchesComplete);

    QObject::connect(
        &localStorageManagerAsync,
        &LocalStorageManagerAsync::listSavedSearchesFailed, this,
        &FavoritesModel::onListSavedSearchesFailed);

    QObject::connect(
        &localStorageManagerAsync,
        &LocalStorageManagerAsync::expungeSavedSearchComplete, this,
        &FavoritesModel::onExpungeSavedSearchComplete);

    QObject::connect(
        &localStorageManagerAsync,
        &LocalStorageManagerAsync::getNoteCountPerNotebookComplete, this,
        &FavoritesModel::onGetNoteCountPerNotebookComplete);

    QObject::connect(
        &localStorageManagerAsync,
        &LocalStorageManagerAsync::getNoteCountPerNotebookFailed, this,
        &FavoritesModel::onGetNoteCountPerNotebookFailed);

    QObject::connect(
        &localStorageManagerAsync,
        &LocalStorageManagerAsync::getNoteCountPerTagComplete, this,
        &FavoritesModel::onGetNoteCountPerTagComplete);

    QObject::connect(
        &localStorageManagerAsync,
        &LocalStorageManagerAsync::getNoteCountPerTagFailed, this,
        &FavoritesModel::onGetNoteCountPerTagFailed);
}

void FavoritesModel::requestNotesList()
{
    QNDEBUG(
        "model:favorites",
        "FavoritesModel::requestNotesList: offset = " << m_listNotesOffset);

    LocalStorageManager::ListObjectsOptions flags =
        LocalStorageManager::ListObjectsOption::ListFavoritedElements;

    LocalStorageManager::ListNotesOrder order =
        LocalStorageManager::ListNotesOrder::NoOrder;

    LocalStorageManager::OrderDirection direction =
        LocalStorageManager::OrderDirection::Ascending;

    m_listNotesRequestId = QUuid::createUuid();

    QNTRACE(
        "model:favorites",
        "Emitting the request to list notes: offset = "
            << m_listNotesOffset << ", request id = " << m_listNotesRequestId);

    Q_EMIT listNotes(
        flags,
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
        LocalStorageManager::GetNoteOptions(),
#else
        LocalStorageManager::GetNoteOptions(0),
#endif
        gNoteListLimit, m_listNotesOffset, order, direction, QString(),
        m_listNotesRequestId);
}

void FavoritesModel::requestNotebooksList()
{
    QNDEBUG(
        "model:favorites",
        "FavoritesModel::requestNotebooksList: offset = "
            << m_listNotebooksOffset);

    // NOTE: the subscription to all notebooks is necessary in order to receive
    // the information about the restrictions for various notebooks + for
    // the collection of notebook names to forbid any two notebooks within
    // the account to have the same name in a case-insensitive manner
    LocalStorageManager::ListObjectsOptions flags =
        LocalStorageManager::ListObjectsOption::ListAll;

    LocalStorageManager::ListNotebooksOrder order =
        LocalStorageManager::ListNotebooksOrder::NoOrder;

    LocalStorageManager::OrderDirection direction =
        LocalStorageManager::OrderDirection::Ascending;

    m_listNotebooksRequestId = QUuid::createUuid();

    QNTRACE(
        "model:favorites",
        "Emitting the request to list notebooks: "
            << "offset = " << m_listNotebooksOffset
            << ", request id = " << m_listNotebooksRequestId);

    Q_EMIT listNotebooks(
        flags, gNotebookListLimit, m_listNotebooksOffset, order, direction,
        QString(), m_listNotebooksRequestId);
}

void FavoritesModel::requestTagsList()
{
    QNDEBUG(
        "model:favorites",
        "FavoritesModel::requestTagsList: offset = " << m_listTagsOffset);

    // NOTE: the subscription to all tags is necessary for the collection of tag
    // names to forbid any two tags within the account to have the same name
    // in a case-insensitive manner
    LocalStorageManager::ListObjectsOptions flags =
        LocalStorageManager::ListObjectsOption::ListAll;

    LocalStorageManager::ListTagsOrder order =
        LocalStorageManager::ListTagsOrder::NoOrder;

    LocalStorageManager::OrderDirection direction =
        LocalStorageManager::OrderDirection::Ascending;

    m_listTagsRequestId = QUuid::createUuid();

    QNTRACE(
        "model:favorites",
        "Emitting the request to list tags: offset = "
            << m_listTagsOffset << ", request id = " << m_listTagsRequestId);

    Q_EMIT listTags(
        flags, gTagListLimit, m_listTagsOffset, order, direction, QString(),
        m_listTagsRequestId);
}

void FavoritesModel::requestSavedSearchesList()
{
    QNDEBUG(
        "model:favorites",
        "FavoritesModel::requestSavedSearchesList: "
            << "offset = " << m_listSavedSearchesOffset);

    // NOTE: the subscription to all saved searches is necessary for
    // the collection of saved search names to forbid any two saved searches
    // within the account to have the same name in a case-insensitive manner
    LocalStorageManager::ListObjectsOptions flags =
        LocalStorageManager::ListObjectsOption::ListAll;

    LocalStorageManager::ListSavedSearchesOrder order =
        LocalStorageManager::ListSavedSearchesOrder::NoOrder;

    LocalStorageManager::OrderDirection direction =
        LocalStorageManager::OrderDirection::Ascending;

    m_listSavedSearchesRequestId = QUuid::createUuid();

    QNTRACE(
        "model:favorites",
        "Emitting the request to list saved searches: "
            << "offset = " << m_listSavedSearchesOffset
            << ", request id = " << m_listSavedSearchesRequestId);

    Q_EMIT listSavedSearches(
        flags, gSavedSearchListLimit, m_listSavedSearchesOffset, order,
        direction, m_listSavedSearchesRequestId);
}

void FavoritesModel::requestNoteCountForNotebook(
    const QString & notebookLocalId, const NoteCountRequestOption option)
{
    QNDEBUG(
        "model:favorites",
        "FavoritesModel::requestNoteCountForNotebook: "
            << "notebook lcoal uid = " << notebookLocalId
            << ", note count request option = " << option);

    if (option != NoteCountRequestOption::Force) {
        const auto it = m_notebookLocalIdToNoteCountRequestIdBimap.left.find(
            notebookLocalId);

        if (it != m_notebookLocalIdToNoteCountRequestIdBimap.left.end()) {
            QNDEBUG(
                "model:favorites",
                "There's an active request to fetch "
                    << "the note count for this notebook local id");
            return;
        }
    }

    const auto requestId = QUuid::createUuid();

    m_notebookLocalIdToNoteCountRequestIdBimap.insert(
        LocalIdToRequestIdBimap::value_type(notebookLocalId, requestId));

    qevercloud::Notebook dummyNotebook;
    dummyNotebook.setLocalId(notebookLocalId);

    QNTRACE(
        "model:favorites",
        "Emitting the request to get the note count "
            << "per notebook: notebook local id = " << notebookLocalId
            << ", request id = " << requestId);

    LocalStorageManager::NoteCountOptions options(
        LocalStorageManager::NoteCountOption::IncludeNonDeletedNotes);

    Q_EMIT noteCountPerNotebook(dummyNotebook, options, requestId);
}

void FavoritesModel::requestNoteCountForAllNotebooks(
    const NoteCountRequestOption option)
{
    QNDEBUG(
        "model:favorites",
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

void FavoritesModel::checkAndIncrementNoteCountPerNotebook(
    const QString & notebookLocalId)
{
    QNDEBUG(
        "model:favorites",
        "FavoritesModel::checkAndIncrementNoteCountPerNotebook: "
            << notebookLocalId);

    checkAndAdjustNoteCountPerNotebook(
        notebookLocalId,
        /* increment = */ true);
}

void FavoritesModel::checkAndDecrementNoteCountPerNotebook(
    const QString & notebookLocalId)
{
    QNDEBUG(
        "model:favorites",
        "FavoritesModel::checkAndDecrementNoteCountPerNotebook: "
            << notebookLocalId);

    checkAndAdjustNoteCountPerNotebook(
        notebookLocalId,
        /* increment = */ false);
}

void FavoritesModel::checkAndAdjustNoteCountPerNotebook(
    const QString & notebookLocalId, const bool increment)
{
    const auto requestIt =
        m_notebookLocalIdToNoteCountRequestIdBimap.left.find(notebookLocalId);

    if (requestIt != m_notebookLocalIdToNoteCountRequestIdBimap.left.end()) {
        QNDEBUG(
            "model:favorites",
            "There's an active request to fetch "
                << "the note count for notebook " << notebookLocalId << ": "
                << requestIt->second
                << ", need to restart it to ensure the proper "
                << "number of notes per notebook");

        requestNoteCountForNotebook(
            notebookLocalId, NoteCountRequestOption::Force);

        return;
    }

    auto & localIdIndex = m_data.get<ByLocalId>();
    const auto notebookItemIt = localIdIndex.find(notebookLocalId);
    if (notebookItemIt == localIdIndex.end()) {
        QNDEBUG(
            "model:favorites",
            "Notebook is not within "
                << "the favorites model");
        return;
    }

    FavoritesModelItem item = *notebookItemIt;
    int noteCount = item.noteCount();

    if (increment) {
        ++noteCount;
        QNDEBUG("model:favorites", "Incremented to " << noteCount);
    }
    else {
        --noteCount;
        QNDEBUG("model:favorites", "Decremented to " << noteCount);
    }

    item.setNoteCount(std::max(noteCount, 0));
    Q_UNUSED(localIdIndex.replace(notebookItemIt, item))

    updateItemColumnInView(item, Column::NoteCount);
}

void FavoritesModel::requestNoteCountForTag(
    const QString & tagLocalId, const NoteCountRequestOption option)
{
    QNDEBUG(
        "model:favorites",
        "FavoritesModel::requestNoteCountForTag: "
            << "tag local id = " << tagLocalId
            << ", note count request option = " << option);

    if (option != NoteCountRequestOption::Force) {
        const auto it =
            m_tagLocalIdToNoteCountRequestIdBimap.left.find(tagLocalId);
        if (it != m_tagLocalIdToNoteCountRequestIdBimap.left.end()) {
            QNDEBUG(
                "model:favorites",
                "There's an active request to fetch "
                    << "the note count for this tag local id");
            return;
        }
    }

    QUuid requestId = QUuid::createUuid();

    m_tagLocalIdToNoteCountRequestIdBimap.insert(
        LocalIdToRequestIdBimap::value_type(tagLocalId, requestId));

    qevercloud::Tag dummyTag;
    dummyTag.setLocalId(tagLocalId);

    QNTRACE(
        "model:favorites",
        "Emitting the request to get the note count per "
            << "tag: tag local id = " << tagLocalId
            << ", request id = " << requestId);

    LocalStorageManager::NoteCountOptions options(
        LocalStorageManager::NoteCountOption::IncludeNonDeletedNotes);

    Q_EMIT noteCountPerTag(dummyTag, options, requestId);
}

void FavoritesModel::requestNoteCountForAllTags(
    const NoteCountRequestOption option)
{
    QNDEBUG("model:favorites", "FavoritesModel::requestNoteCountForAllTags");

    const auto & localIdIndex = m_data.get<ByLocalId>();
    for (const auto & item: localIdIndex) {
        if (item.type() != FavoritesModelItem::Type::Tag) {
            continue;
        }

        requestNoteCountForTag(item.localId(), option);
    }
}

void FavoritesModel::checkAndIncrementNoteCountPerTag(
    const QString & tagLocalId)
{
    QNDEBUG(
        "model:favorites",
        "FavoritesModel::checkAndIncrementNoteCountPerTag: " << tagLocalId);

    checkAndAdjustNoteCountPerTag(tagLocalId, /* increment = */ true);
}

void FavoritesModel::checkAndDecrementNoteCountPerTag(
    const QString & tagLocalId)
{
    QNDEBUG(
        "model:favorites",
        "FavoritesModel::checkAndDecrementNoteCountPerTag: " << tagLocalId);

    checkAndAdjustNoteCountPerTag(tagLocalId, /* increment = */ false);
}

void FavoritesModel::checkAndAdjustNoteCountPerTag(
    const QString & tagLocalId, const bool increment)
{
    const auto requestIt =
        m_tagLocalIdToNoteCountRequestIdBimap.left.find(tagLocalId);

    if (requestIt != m_tagLocalIdToNoteCountRequestIdBimap.left.end()) {
        QNDEBUG(
            "model:favorites",
            "There's an active request to fetch "
                << "the note count for tag " << tagLocalId << ": "
                << requestIt->second
                << ", need to restart it to ensure the proper "
                << "number of notes per tag");
        requestNoteCountForTag(tagLocalId, NoteCountRequestOption::Force);
        return;
    }

    auto & localIdIndex = m_data.get<ByLocalId>();
    const auto tagItemIt = localIdIndex.find(tagLocalId);
    if (tagItemIt == localIdIndex.end()) {
        QNDEBUG("model:favorites", "Tag is not within the favorites model");
        return;
    }

    FavoritesModelItem item = *tagItemIt;
    int noteCount = item.noteCount();

    if (increment) {
        ++noteCount;
        QNDEBUG("model:favorites", "Incremented to " << noteCount);
    }
    else {
        --noteCount;
        QNDEBUG("model:favorites", "Decremented to " << noteCount);
    }

    item.setNoteCount(std::max(noteCount, 0));
    Q_UNUSED(localIdIndex.replace(tagItemIt, item))

    updateItemColumnInView(item, Column::NoteCount);
}

QVariant FavoritesModel::dataImpl(const int row, const Column column) const
{
    if (Q_UNLIKELY((row < 0) || (row >= static_cast<int>(m_data.size())))) {
        return {};
    }

    const auto & rowIndex = m_data.get<ByIndex>();
    const auto & item = rowIndex[static_cast<std::size_t>(row)];

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
        return QVariant();
    }

    const auto & rowIndex = m_data.get<ByIndex>();
    const auto & item = rowIndex[static_cast<std::size_t>(row)];

    static const QString space = QStringLiteral(" ");
    static const QString colon = QStringLiteral(":");

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
        "model:favorites",
        "FavoritesModel::removeItemByLocalId: local id = " << localId);

    auto & localIdIndex = m_data.get<ByLocalId>();
    const auto itemIt = localIdIndex.find(localId);
    if (Q_UNLIKELY(itemIt == localIdIndex.end())) {
        QNDEBUG(
            "model:favorites",
            "Can't find item to remove from "
                << "the favorites model");
        return;
    }

    const auto & item = *itemIt;

    switch (item.type()) {
    case FavoritesModelItem::Type::Notebook:
    {
        const auto nameIt =
            m_lowerCaseNotebookNames.find(item.displayName().toLower());

        if (nameIt != m_lowerCaseNotebookNames.end()) {
            Q_UNUSED(m_lowerCaseNotebookNames.erase(nameIt))
        }

        break;
    }
    case FavoritesModelItem::Type::Tag:
    {
        const auto nameIt =
            m_lowerCaseTagNames.find(item.displayName().toLower());

        if (nameIt != m_lowerCaseTagNames.end()) {
            Q_UNUSED(m_lowerCaseTagNames.erase(nameIt))
        }

        break;
    }
    case FavoritesModelItem::Type::SavedSearch:
    {
        const auto nameIt =
            m_lowerCaseSavedSearchNames.find(item.displayName().toLower());

        if (nameIt != m_lowerCaseSavedSearchNames.end()) {
            Q_UNUSED(m_lowerCaseSavedSearchNames.erase(nameIt))
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
            "model:favorites",
            "Can't determine the row index for the favorites model item to "
                << "remove: " << item);
        return;
    }

    const int row = static_cast<int>(std::distance(rowIndex.begin(), indexIt));
    if (Q_UNLIKELY((row < 0) || (row >= static_cast<int>(m_data.size())))) {
        QNWARNING(
            "model:favorites",
            "Invalid row index for the favorites model item to remove: index = "
                << row << ", item: " << item);
        return;
    }

    Q_EMIT aboutToRemoveItems();

    beginRemoveRows(QModelIndex(), row, row);
    Q_UNUSED(localIdIndex.erase(itemIt))
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
    const auto localIdIt = localIdIndex.find(item.localId());
    if (Q_UNLIKELY(localIdIt == localIdIndex.end())) {
        QNWARNING(
            "model:favorites",
            "Can't update item row with respect to "
                << "sorting: can't find the item within the model: " << item);
        return;
    }

    auto & rowIndex = m_data.get<ByIndex>();
    const auto it = m_data.project<ByIndex>(localIdIt);
    if (Q_UNLIKELY(it == rowIndex.end())) {
        QNWARNING(
            "model:favorites",
            "Can't update item row with respect to "
                << "sorting: can't find item's original row; item: " << item);
        return;
    }

    const int originalRow = static_cast<int>(std::distance(rowIndex.begin(), it));
    if (Q_UNLIKELY(
            (originalRow < 0) ||
            (originalRow >= static_cast<int>(m_data.size()))))
    {
        QNWARNING(
            "model:favorites",
            "Can't update item row with respect to sorting: item's original "
                << "row is beyond the acceptable range: " << originalRow
                << ", item: " << item);
        return;
    }

    FavoritesModelItem itemCopy(item);

    beginRemoveRows(QModelIndex(), originalRow, originalRow);
    Q_UNUSED(rowIndex.erase(it))
    endRemoveRows();

    const auto positionIter = std::lower_bound(
        rowIndex.begin(), rowIndex.end(), itemCopy,
        Comparator(m_sortedColumn, m_sortOrder));

    if (positionIter == rowIndex.end()) {
        const int row = static_cast<int>(rowIndex.size());
        beginInsertRows(QModelIndex(), row, row);
        rowIndex.push_back(itemCopy);
        endInsertRows();
        return;
    }

    const int row =
        static_cast<int>(std::distance(rowIndex.begin(), positionIter));

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
            "model:favorites",
            "Detected attempt to update favorites model item in the local "
                << "storage for wrong favorited model item's type: " << item);
        break;
    }
    }
}

void FavoritesModel::updateNoteInLocalStorage(const FavoritesModelItem & item)
{
    QNDEBUG(
        "model:favorites",
        "FavoritesModel::updateNoteInLocalStorage: "
            << "local id = " << item.localId()
            << ", title = " << item.displayName());

    const auto * pCachedNote = m_noteCache.get(item.localId());
    if (Q_UNLIKELY(!pCachedNote)) {
        const auto requestId = QUuid::createUuid();
        Q_UNUSED(m_findNoteToPerformUpdateRequestIds.insert(requestId))

        qevercloud::Note dummy;
        dummy.setLocalId(item.localId());

        QNTRACE(
            "model:favorites",
            "Emitting the request to find a note: local id = " << item.localId()
                << ", request id = " << requestId);

        const LocalStorageManager::GetNoteOptions options(
            LocalStorageManager::GetNoteOption::WithResourceMetadata);

        Q_EMIT findNote(dummy, options, requestId);
        return;
    }

    qevercloud::Note note = *pCachedNote;
    note.setLocalId(item.localId());

    const bool dirty = note.isLocallyModified() || !note.title() ||
        (*note.title() != item.displayName());

    note.setLocallyModified(dirty);
    note.setTitle(item.displayName());

    const auto requestId = QUuid::createUuid();
    Q_UNUSED(m_updateNoteRequestIds.insert(requestId))

    // While the note is being updated in the local storage,
    // remove its stale copy from the cache
    Q_UNUSED(m_noteCache.remove(note.localId()))

    QNTRACE(
        "model:favorites",
        "Emitting the request to update the note in the local storage: id = "
            << requestId << ", note: " << note);

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
        "model:favorites",
        "FavoritesModel::updateNotebookInLocalStorage: "
            << "local id = " << item.localId()
            << ", name = " << item.displayName());

    const auto * pCachedNotebook = m_notebookCache.get(item.localId());
    if (Q_UNLIKELY(!pCachedNotebook)) {
        const auto requestId = QUuid::createUuid();
        Q_UNUSED(m_findNotebookToPerformUpdateRequestIds.insert(requestId))

        qevercloud::Notebook dummy;
        dummy.setLocalId(item.localId());

        QNTRACE(
            "model:favorites",
            "Emitting the request to find a notebook: "
                << "local id = " << item.localId()
                << ", request id = " << requestId);

        Q_EMIT findNotebook(dummy, requestId);
        return;
    }

    qevercloud::Notebook notebook = *pCachedNotebook;
    notebook.setLocalId(item.localId());

    const bool dirty = notebook.isLocallyModified() || !notebook.name() ||
        (*notebook.name() != item.displayName());

    notebook.setLocallyModified(dirty);
    notebook.setName(item.displayName());

    const auto requestId = QUuid::createUuid();
    Q_UNUSED(m_updateNotebookRequestIds.insert(requestId))

    // While the notebook is being updated in the local storage,
    // remove its stale copy from the cache
    Q_UNUSED(m_notebookCache.remove(notebook.localId()))

    QNTRACE(
        "model:favorites",
        "Emitting the request to update the notebook in "
            << "the local storage: id = " << requestId
            << ", notebook: " << notebook);

    Q_EMIT updateNotebook(notebook, requestId);
}

void FavoritesModel::updateTagInLocalStorage(const FavoritesModelItem & item)
{
    QNDEBUG(
        "model:favorites",
        "FavoritesModel::updateTagInLocalStorage: "
            << "local id = " << item.localId()
            << ", name = " << item.displayName());

    const auto * pCachedTag = m_tagCache.get(item.localId());
    if (Q_UNLIKELY(!pCachedTag)) {
        const auto requestId = QUuid::createUuid();
        Q_UNUSED(m_findTagToPerformUpdateRequestIds.insert(requestId))

        qevercloud::Tag dummy;
        dummy.setLocalId(item.localId());

        QNTRACE(
            "model:favorites",
            "Emitting the request to find a tag: "
                << "local id = " << item.localId()
                << ", request id = " << requestId);

        Q_EMIT findTag(dummy, requestId);
        return;
    }

    qevercloud::Tag tag = *pCachedTag;
    tag.setLocalId(item.localId());

    const bool dirty = tag.isLocallyModified() || !tag.name() ||
        (*tag.name() != item.displayName());

    tag.setLocallyModified(dirty);
    tag.setName(item.displayName());

    const auto requestId = QUuid::createUuid();
    Q_UNUSED(m_updateTagRequestIds.insert(requestId))

    // While the tag is being updated in the local storage,
    // remove its stale copy from the cache
    Q_UNUSED(m_tagCache.remove(tag.localId()))

    QNTRACE(
        "model:favorites",
        "Emitting the request to update the tag in "
            << "the local storage: id = " << requestId << ", tag: " << tag);
    Q_EMIT updateTag(tag, requestId);
}

void FavoritesModel::updateSavedSearchInLocalStorage(
    const FavoritesModelItem & item)
{
    QNDEBUG(
        "model:favorites",
        "FavoritesModel::updateSavedSearchInLocalStorage: local id = "
            << item.localId() << ", display name = " << item.displayName());

    const auto * pCachedSearch = m_savedSearchCache.get(item.localId());
    if (Q_UNLIKELY(!pCachedSearch)) {
        const auto requestId = QUuid::createUuid();
        Q_UNUSED(m_findSavedSearchToPerformUpdateRequestIds.insert(requestId))

        qevercloud::SavedSearch dummy;
        dummy.setLocalId(item.localId());

        QNTRACE(
            "model:favorites",
            "Emitting the request to find a saved search: local id = "
                << item.localId() << ", request id = " << requestId);

        Q_EMIT findSavedSearch(dummy, requestId);
        return;
    }

    qevercloud::SavedSearch search = *pCachedSearch;
    search.setLocalId(item.localId());

    const bool dirty = search.isLocallyModified() || !search.name() ||
        (*search.name() != item.displayName());

    search.setLocallyModified(dirty);
    search.setName(item.displayName());

    const auto requestId = QUuid::createUuid();
    Q_UNUSED(m_updateSavedSearchRequestIds.insert(requestId))

    // While the saved search is being updated in the local storage,
    // remove its stale copy from the cache
    Q_UNUSED(m_savedSearchCache.remove(search.localId()))

    QNTRACE(
        "model:favorites",
        "Emitting the request to update the saved "
            << "search in the local storage: id = " << requestId
            << ", saved search: " << search);

    Q_EMIT updateSavedSearch(search, requestId);
}

bool FavoritesModel::canUpdateNote(const QString & localId) const
{
    const auto notebookLocalIdIt = m_notebookLocalIdByNoteLocalId.find(localId);
    if (notebookLocalIdIt == m_notebookLocalIdByNoteLocalId.end()) {
        return false;
    }

    const auto notebookGuidIt =
        m_notebookLocalIdToGuid.find(notebookLocalIdIt.value());

    if (notebookGuidIt == m_notebookLocalIdToGuid.end()) {
        // NOTE: this must be the local, non-synchronizable notebook as it
        // doesn't have the Evernote service's guid;
        return true;
    }

    const auto notebookRestrictionsDataIt =
        m_notebookRestrictionsData.find(notebookGuidIt.value());

    if (notebookRestrictionsDataIt == m_notebookRestrictionsData.end()) {
        return true;
    }

    const auto & restrictionsData = notebookRestrictionsDataIt.value();
    return restrictionsData.m_canUpdateNotes;
}

bool FavoritesModel::canUpdateNotebook(const QString & localId) const
{
    const auto notebookGuidIt = m_notebookLocalIdToGuid.find(localId);
    if (notebookGuidIt == m_notebookLocalIdToGuid.end()) {
        // NOTE: this must be the local, non-synchronizable notebook as it
        // doesn't have the Evernote service's guid;
        return true;
    }

    const auto notebookRestrictionsDataIt =
        m_notebookRestrictionsData.find(notebookGuidIt.value());

    if (notebookRestrictionsDataIt == m_notebookRestrictionsData.end()) {
        return true;
    }

    const auto & restrictionsData = notebookRestrictionsDataIt.value();
    return restrictionsData.m_canUpdateNotebook;
}

bool FavoritesModel::canUpdateTag(const QString & localId) const
{
    const auto notebookGuidIt = m_tagLocalIdToLinkedNotebookGuid.find(localId);
    if (notebookGuidIt == m_tagLocalIdToLinkedNotebookGuid.end()) {
        return true;
    }

    const auto notebookRestrictionsDataIt =
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
        "model:favorites",
        "FavoritesModel::unfavoriteNote: local id = " << localId);

    const auto * pCachedNote = m_noteCache.get(localId);
    if (Q_UNLIKELY(!pCachedNote)) {
        const auto requestId = QUuid::createUuid();
        Q_UNUSED(m_findNoteToUnfavoriteRequestIds.insert(requestId))

        qevercloud::Note dummy;
        dummy.setLocalId(localId);

        QNTRACE(
            "model:favorites",
            "Emitting the request to find a note: "
                << "local id = " << localId
                << ", request id = " << requestId);

        const LocalStorageManager::GetNoteOptions options(
            LocalStorageManager::GetNoteOption::WithResourceMetadata);

        Q_EMIT findNote(dummy, options, requestId);
        return;
    }

    qevercloud::Note note = *pCachedNote;
    note.setLocalId(localId);
    const bool dirty = note.isLocallyModified() || note.isLocallyFavorited();
    note.setLocallyModified(dirty);
    note.setLocallyFavorited(false);

    const auto requestId = QUuid::createUuid();
    Q_UNUSED(m_updateNoteRequestIds.insert(requestId))

    // While the note is being updated in the local storage,
    // remove its stale copy from the cache
    Q_UNUSED(m_noteCache.remove(note.localId()))

    QNTRACE(
        "model:favorites",
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
        "model:favorites",
        "FavoritesModel::unfavoriteNotebook: local "
            << "uid = " << localId);

    const auto * pCachedNotebook = m_notebookCache.get(localId);
    if (Q_UNLIKELY(!pCachedNotebook)) {
        const auto requestId = QUuid::createUuid();
        Q_UNUSED(m_findNotebookToUnfavoriteRequestIds.insert(requestId))

        qevercloud::Notebook dummy;
        dummy.setLocalId(localId);

        QNTRACE(
            "model:favorites",
            "Emitting the request to find a notebook: "
                << "local id = " << localId
                << ", request id = " << requestId);

        Q_EMIT findNotebook(dummy, requestId);
        return;
    }

    qevercloud::Notebook notebook = *pCachedNotebook;

    notebook.setLocalId(localId);
    const bool dirty = notebook.isLocallyModified() ||
        notebook.isLocallyFavorited();
    notebook.setLocallyModified(dirty);
    notebook.setLocallyFavorited(false);

    const auto requestId = QUuid::createUuid();
    Q_UNUSED(m_updateNotebookRequestIds.insert(requestId))

    // While the notebook is being updated in the local storage,
    // remove its stale copy from the cache
    Q_UNUSED(m_notebookCache.remove(notebook.localId()))

    QNTRACE(
        "model:favorites",
        "Emitting the request to update the notebook in "
            << "the local storage: id = " << requestId
            << ", notebook: " << notebook);

    Q_EMIT updateNotebook(notebook, requestId);
}

void FavoritesModel::unfavoriteTag(const QString & localId)
{
    QNDEBUG(
        "model:favorites",
        "FavoritesModel::unfavoriteTag: local id = " << localId);

    const auto * pCachedTag = m_tagCache.get(localId);
    if (Q_UNLIKELY(!pCachedTag)) {
        const auto requestId = QUuid::createUuid();
        Q_UNUSED(m_findTagToUnfavoriteRequestIds.insert(requestId))

        qevercloud::Tag dummy;
        dummy.setLocalId(localId);

        QNTRACE(
            "model:favorites",
            "Emitting the request to find a tag: "
                << "local id = " << localId
                << ", request id = " << requestId);

        Q_EMIT findTag(dummy, requestId);
        return;
    }

    qevercloud::Tag tag = *pCachedTag;

    tag.setLocalId(localId);
    const bool dirty = tag.isLocallyModified() || tag.isLocallyFavorited();
    tag.setLocallyModified(dirty);
    tag.setLocallyFavorited(false);

    QUuid requestId = QUuid::createUuid();
    Q_UNUSED(m_updateTagRequestIds.insert(requestId))

    // While the tag is being updated in the local storage,
    // remove its stale copy from the cache
    Q_UNUSED(m_tagCache.remove(tag.localId()))

    QNTRACE(
        "model:favorites",
        "Emitting the request to update the tag in "
            << "the local storage: id = " << requestId << ", tag: " << tag);

    Q_EMIT updateTag(tag, requestId);
}

void FavoritesModel::unfavoriteSavedSearch(const QString & localId)
{
    QNDEBUG(
        "model:favorites",
        "FavoritesModel::unfavoriteSavedSearch: local id = " << localId);

    const auto * pCachedSearch = m_savedSearchCache.get(localId);
    if (Q_UNLIKELY(!pCachedSearch)) {
        const auto requestId = QUuid::createUuid();
        Q_UNUSED(m_findSavedSearchToUnfavoriteRequestIds.insert(requestId))

        qevercloud::SavedSearch dummy;
        dummy.setLocalId(localId);

        QNTRACE(
            "model:favorites",
            "Emitting the request to find a saved search: local id = "
                << localId << ", request id = " << requestId);

        Q_EMIT findSavedSearch(dummy, requestId);
        return;
    }

    qevercloud::SavedSearch search = *pCachedSearch;

    search.setLocalId(localId);
    const bool dirty = search.isLocallyModified() ||
        search.isLocallyFavorited();
    search.setLocallyModified(dirty);
    search.setLocallyFavorited(false);

    const auto requestId = QUuid::createUuid();
    Q_UNUSED(m_updateSavedSearchRequestIds.insert(requestId))

    // While the saved search is being updated in the local storage,
    // remove its stale copy from the cache
    Q_UNUSED(m_savedSearchCache.remove(search.localId()))

    QNTRACE(
        "model:favorites",
        "Emitting the request to update the saved "
            << "search in the local storage: id = " << requestId
            << ", saved search: " << search);

    Q_EMIT updateSavedSearch(search, requestId);
}

void FavoritesModel::onNoteAddedOrUpdated(
    const qevercloud::Note & note, const bool tagsUpdated)
{
    QNDEBUG(
        "model:favorites",
        "FavoritesModel::onNoteAddedOrUpdated: "
            << "note local id = " << note.localId()
            << ", tags updated = " << (tagsUpdated ? "true" : "false"));

    if (tagsUpdated) {
        m_noteCache.put(note.localId(), note);
    }

    const auto notebookLocalId = note.notebookLocalId();
    if (notebookLocalId.isEmpty()) {
        QNWARNING(
            "model:favorites",
            "Skipping the note not having the notebook local id: " << note);
        return;
    }

    if (!note.isLocallyFavorited()) {
        removeItemByLocalId(note.localId());
        return;
    }

    FavoritesModelItem item;
    item.setType(FavoritesModelItem::Type::Note);
    item.setLocalId(note.localId());
    item.setNoteCount(0);

    if (note.title()) {
        item.setDisplayName(*note.title());
    }
    else if (note.content()) {
        QString plainText = noteContentToPlainText(*note.content());
        plainText.truncate(160);
        item.setDisplayName(plainText);
        // NOTE: using the text preview in this way means updating the favorites
        // item's display name would actually create the title for the note
    }

    m_notebookLocalIdByNoteLocalId[note.localId()] = note.notebookLocalId();

    auto & rowIndex = m_data.get<ByIndex>();
    auto & localIdIndex = m_data.get<ByLocalId>();
    const auto itemIt = localIdIndex.find(note.localId());
    if (itemIt == localIdIndex.end()) {
        QNDEBUG("model:favorites", "Detected newly favorited note");

        Q_EMIT aboutToAddItem();

        const int row = static_cast<int>(rowIndex.size());
        beginInsertRows(QModelIndex(), row, row);
        rowIndex.push_back(item);
        endInsertRows();

        updateItemRowWithRespectToSorting(item);

        const QModelIndex addedNoteIndex = indexForLocalId(item.localId());
        Q_EMIT addedItem(addedNoteIndex);
        return;
    }

    QNDEBUG("model:favorites", "Updating the already favorited item");

    const auto indexIt = m_data.project<ByIndex>(itemIt);
    if (Q_UNLIKELY(indexIt == rowIndex.end())) {
        ErrorString error(
            QT_TR_NOOP("Internal error: can't project the local id "
                       "index iterator to the random access index "
                       "iterator within the favorites model"));

        QNWARNING(
            "model:favorites", error << ", favorites model item: " << item);

        Q_EMIT notifyError(error);
        return;
    }

    const int row = static_cast<int>(std::distance(rowIndex.begin(), indexIt));

    auto modelIndex =
        createIndex(row, static_cast<int>(Column::DisplayName));

    Q_EMIT aboutToUpdateItem(modelIndex);

    Q_UNUSED(localIdIndex.replace(itemIt, item))

    Q_EMIT dataChanged(modelIndex, modelIndex);

    updateItemRowWithRespectToSorting(item);

    modelIndex = indexForLocalId(item.localId());
    Q_EMIT updatedItem(modelIndex);
}

void FavoritesModel::onNotebookAddedOrUpdated(
    const qevercloud::Notebook & notebook)
{
    QNDEBUG(
        "model:favorites",
        "FavoritesModel::onNotebookAddedOrUpdated: "
            << "local id = " << notebook.localId());

    m_notebookCache.put(notebook.localId(), notebook);

    auto & localIdIndex = m_data.get<ByLocalId>();
    const auto itemIt = localIdIndex.find(notebook.localId());

    if (itemIt != localIdIndex.end()) {
        const auto & originalItem = *itemIt;

        const auto nameIt =
            m_lowerCaseNotebookNames.find(originalItem.displayName().toLower());

        if (nameIt != m_lowerCaseNotebookNames.end()) {
            Q_UNUSED(m_lowerCaseNotebookNames.erase(nameIt))
        }
    }

    if (notebook.name()) {
        Q_UNUSED(m_lowerCaseNotebookNames.insert(notebook.name()->toLower()))
    }

    if (notebook.guid()) {
        auto & notebookRestrictionsData =
            m_notebookRestrictionsData[*notebook.guid()];

        if (notebook.restrictions()) {
            const auto & restrictions = *notebook.restrictions();

            notebookRestrictionsData.m_canUpdateNotebook =
                !restrictions.noUpdateNotebook() ||
                !*restrictions.noUpdateNotebook();

            notebookRestrictionsData.m_canUpdateNotes =
                !restrictions.noUpdateNotes() ||
                !*restrictions.noUpdateNotes();

            notebookRestrictionsData.m_canUpdateTags =
                !restrictions.noUpdateTags() ||
                !*restrictions.noUpdateTags();
        }
        else {
            notebookRestrictionsData.m_canUpdateNotebook = true;
            notebookRestrictionsData.m_canUpdateNotes = true;
            notebookRestrictionsData.m_canUpdateTags = true;
        }

        QNTRACE(
            "model:favorites",
            "Updated restrictions data for notebook "
                << notebook.localId() << ", name "
                << (notebook.name()
                    ? QStringLiteral("\"") +
                        *notebook.name() + QStringLiteral("\"")
                    : QStringLiteral("<not set>"))
                << ", guid = " << *notebook.guid() << ": can update notebook = "
                << (notebookRestrictionsData.m_canUpdateNotebook ? "true"
                                                                 : "false")
                << ", can update notes = "
                << (notebookRestrictionsData.m_canUpdateNotes ? "true"
                                                              : "false")
                << ", can update tags = "
                << (notebookRestrictionsData.m_canUpdateTags ? "true"
                                                             : "false"));
    }

    if (!notebook.name()) {
        QNTRACE(
            "model:favorites",
            "Removing/skipping the notebook without a name");
        removeItemByLocalId(notebook.localId());
        return;
    }

    if (!notebook.isLocallyFavorited()) {
        QNTRACE("model:favorites", "Removing/skipping non-favorited notebook");
        removeItemByLocalId(notebook.localId());
        return;
    }

    FavoritesModelItem item;
    item.setType(FavoritesModelItem::Type::Notebook);
    item.setLocalId(notebook.localId());
    item.setNoteCount(-1); // That means, not known yet
    item.setDisplayName(*notebook.name());

    auto & rowIndex = m_data.get<ByIndex>();

    if (itemIt == localIdIndex.end()) {
        QNDEBUG("model:favorites", "Detected newly favorited notebook");

        Q_EMIT aboutToAddItem();

        const int row = static_cast<int>(rowIndex.size());
        beginInsertRows(QModelIndex(), row, row);
        rowIndex.push_back(item);
        endInsertRows();

        updateItemRowWithRespectToSorting(item);

        const auto addedNotebookIndex = indexForLocalId(item.localId());
        Q_EMIT addedItem(addedNotebookIndex);

        // Need to figure out how many notes this notebook targets
        requestNoteCountForNotebook(
            notebook.localId(), NoteCountRequestOption::IfNotAlreadyRunning);

        return;
    }

    QNDEBUG("model:favorites", "Updating the already favorited notebook item");

    const auto & originalItem = *itemIt;
    item.setNoteCount(originalItem.noteCount());

    const auto indexIt = m_data.project<ByIndex>(itemIt);
    if (Q_UNLIKELY(indexIt == rowIndex.end())) {
        ErrorString error(
            QT_TR_NOOP("Internal error: can't project the local id "
                       "index iterator to the random access index "
                       "iterator within the favorites model"));

        QNWARNING(
            "model:favorites", error << ", favorites model item: " << item);

        Q_EMIT notifyError(error);
        return;
    }

    const int row = static_cast<int>(std::distance(rowIndex.begin(), indexIt));

    auto modelIndex = createIndex(row, static_cast<int>(Column::DisplayName));
    Q_EMIT aboutToUpdateItem(modelIndex);

    Q_UNUSED(localIdIndex.replace(itemIt, item))

    Q_EMIT dataChanged(modelIndex, modelIndex);

    updateItemRowWithRespectToSorting(item);

    modelIndex = indexForLocalId(item.localId());
    Q_EMIT updatedItem(modelIndex);
}

void FavoritesModel::onTagAddedOrUpdated(const qevercloud::Tag & tag)
{
    QNTRACE(
        "model:favorites",
        "FavoritesModel::onTagAddedOrUpdated: local id = " << tag.localId());

    m_tagCache.put(tag.localId(), tag);

    auto & localIdIndex = m_data.get<ByLocalId>();
    const auto itemIt = localIdIndex.find(tag.localId());

    if (itemIt != localIdIndex.end()) {
        const auto & originalItem = *itemIt;

        const auto nameIt =
            m_lowerCaseTagNames.find(originalItem.displayName().toLower());

        if (nameIt != m_lowerCaseTagNames.end()) {
            Q_UNUSED(m_lowerCaseTagNames.erase(nameIt))
        }
    }

    if (tag.name()) {
        Q_UNUSED(m_lowerCaseTagNames.insert(tag.name()->toLower()))
    }
    else {
        QNTRACE("model:favorites", "Removing/skipping the tag without a name");
        removeItemByLocalId(tag.localId());
        return;
    }

    if (!tag.isLocallyFavorited()) {
        QNTRACE("model:favorites", "Removing/skipping non-favorited tag");
        removeItemByLocalId(tag.localId());
        return;
    }

    FavoritesModelItem item;
    item.setType(FavoritesModelItem::Type::Tag);
    item.setLocalId(tag.localId());
    item.setNoteCount(-1); // That means, not known yet
    item.setDisplayName(*tag.name());

    auto & rowIndex = m_data.get<ByIndex>();

    if (itemIt == localIdIndex.end()) {
        QNDEBUG("model:favorites", "Detected newly favorited tag");

        Q_EMIT aboutToAddItem();

        const int row = static_cast<int>(rowIndex.size());
        beginInsertRows(QModelIndex(), row, row);
        rowIndex.push_back(item);
        endInsertRows();

        updateItemRowWithRespectToSorting(item);

        const auto addedTagIndex = indexForLocalId(item.localId());
        Q_EMIT addedItem(addedTagIndex);

        // Need to figure out how many notes this tag targets
        requestNoteCountForTag(
            tag.localId(), NoteCountRequestOption::IfNotAlreadyRunning);

        return;
    }

    QNDEBUG("model:favorites", "Updating the already favorited tag item");

    const auto & originalItem = *itemIt;
    item.setNoteCount(originalItem.noteCount());

    const auto indexIt = m_data.project<ByIndex>(itemIt);
    if (Q_UNLIKELY(indexIt == rowIndex.end())) {
        ErrorString error(
            QT_TR_NOOP("Internal error: can't project the local id "
                       "index iterator to the random access index "
                       "iterator within the favorites model"));

        QNWARNING(
            "model:favorites", error << ", favorites model item: " << item);

        Q_EMIT notifyError(error);
        return;
    }

    const int row = static_cast<int>(std::distance(rowIndex.begin(), indexIt));

    auto modelIndex = createIndex(row, static_cast<int>(Column::DisplayName));
    Q_EMIT aboutToUpdateItem(modelIndex);

    Q_UNUSED(localIdIndex.replace(itemIt, item))

    Q_EMIT dataChanged(modelIndex, modelIndex);

    updateItemRowWithRespectToSorting(item);

    modelIndex = indexForLocalId(item.localId());
    Q_EMIT updatedItem(modelIndex);
}

void FavoritesModel::onSavedSearchAddedOrUpdated(
    const qevercloud::SavedSearch & search)
{
    QNDEBUG(
        "model:favorites",
        "FavoritesModel::onSavedSearchAddedOrUpdated: "
            << "local id = " << search.localId());

    m_savedSearchCache.put(search.localId(), search);

    auto & localIdIndex = m_data.get<ByLocalId>();
    const auto itemIt = localIdIndex.find(search.localId());

    if (itemIt != localIdIndex.end()) {
        const auto & originalItem = *itemIt;

        const auto nameIt = m_lowerCaseSavedSearchNames.find(
            originalItem.displayName().toLower());

        if (nameIt != m_lowerCaseSavedSearchNames.end()) {
            Q_UNUSED(m_lowerCaseSavedSearchNames.erase(nameIt))
        }
    }

    if (search.name()) {
        Q_UNUSED(m_lowerCaseSavedSearchNames.insert(search.name()->toLower()))
    }
    else {
        QNTRACE(
            "model:favorites",
            "Removing/skipping the search without "
                << "a name");
        removeItemByLocalId(search.localId());
        return;
    }

    if (!search.isLocallyFavorited()) {
        QNTRACE("model:favorites", "Removing/skipping non-favorited search");
        removeItemByLocalId(search.localId());
        return;
    }

    FavoritesModelItem item;
    item.setType(FavoritesModelItem::Type::SavedSearch);
    item.setLocalId(search.localId());
    item.setNoteCount(-1);
    item.setDisplayName(*search.name());

    auto & rowIndex = m_data.get<ByIndex>();

    if (itemIt == localIdIndex.end()) {
        QNDEBUG("model:favorites", "Detected newly favorited saved search");

        Q_EMIT aboutToAddItem();

        const int row = static_cast<int>(rowIndex.size());
        beginInsertRows(QModelIndex(), row, row);
        rowIndex.push_back(item);
        endInsertRows();

        updateItemRowWithRespectToSorting(item);

        auto addedSavedSearchIndex = indexForLocalId(item.localId());
        Q_EMIT addedItem(addedSavedSearchIndex);

        return;
    }

    QNDEBUG(
        "model:favorites",
        "Updating the already favorited saved search item");

    const auto indexIt = m_data.project<ByIndex>(itemIt);
    if (Q_UNLIKELY(indexIt == rowIndex.end())) {
        ErrorString error(
            QT_TR_NOOP("Internal error: can't project the local id "
                       "index iterator to the random access index "
                       "iterator within the favorites model"));

        QNWARNING(
            "model:favorites", error << ", favorites model item: " << item);

        Q_EMIT notifyError(error);
        return;
    }

    const int row = static_cast<int>(std::distance(rowIndex.begin(), indexIt));

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
        "model:favorites",
        "FavoritesModel::updateItemColumnInView: item = "
            << item << "\nColumn = " << column);

    const auto & rowIndex = m_data.get<ByIndex>();
    const auto & localIdIndex = m_data.get<ByLocalId>();

    const auto itemIt = localIdIndex.find(item.localId());
    if (itemIt == localIdIndex.end()) {
        QNDEBUG("model:favorites", "Can't find item by local id");
        return;
    }

    if (m_sortedColumn != column) {
        const auto itemIndexIt = m_data.project<ByIndex>(itemIt);
        if (Q_LIKELY(itemIndexIt != rowIndex.end())) {
            const int row =
                static_cast<int>(std::distance(rowIndex.begin(), itemIndexIt));

            const auto modelIndex = createIndex(row, static_cast<int>(column));

            QNTRACE(
                "model:favorites",
                "Emitting dataChanged signal for row " << row << " and column "
                                                       << column);

            Q_EMIT dataChanged(modelIndex, modelIndex);
            return;
        }

        ErrorString error{
            QT_TR_NOOP("Internal error: can't project the local id "
                       "index iterator to the random access index "
                       "iterator within the favorites model")};

        QNWARNING(
            "model:favorites", error << ", favorites model item: " << item);

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
        QNDEBUG("model:favorites", "Listed all favorites model's items");
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

QDebug & operator<<(
    QDebug & dbg, const FavoritesModel::NoteCountRequestOption option)
{
    using NoteCountRequestOption = FavoritesModel::NoteCountRequestOption;

    switch (option)
    {
    case NoteCountRequestOption::Force:
        dbg << "Force";
        break;
    case NoteCountRequestOption::IfNotAlreadyRunning:
        dbg << "If not already running";
        break;
    default:
        dbg << "Unknown (" << static_cast<qint64>(option) << ")";
        break;
    }

    return dbg;
}

} // namespace quentier
