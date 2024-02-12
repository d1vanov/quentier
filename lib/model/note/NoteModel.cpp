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

#include "NoteModel.h"

#include <lib/exception/Utils.h>

#include <quentier/local_storage/ILocalStorage.h>
#include <quentier/local_storage/ILocalStorageNotifier.h>
#include <quentier/logging/QuentierLogger.h>
#include <quentier/threading/Future.h>
#include <quentier/types/NoteUtils.h>
#include <quentier/types/Validation.h>
#include <quentier/utility/DateTime.h>
#include <quentier/utility/Size.h>
#include <quentier/utility/UidGenerator.h>

#include <QImage>

#include <cstddef>
#include <iterator>
#include <utility>

// Separate logging macros for the note model - to distinguish the one
// for deleted notes from the one for non-deleted notes

[[nodiscard]] inline QString includedNotesStr(
    const quentier::NoteModel::IncludedNotes includedNotes)
{
    if (includedNotes == quentier::NoteModel::IncludedNotes::All) {
        return QStringLiteral("[all notes] ");
    }

    if (includedNotes == quentier::NoteModel::IncludedNotes::NonDeleted) {
        return QStringLiteral("[non-deleted notes] ");
    }

    return QStringLiteral("[deleted notes] ");
}

#define NMTRACE(message)                                                       \
    QNTRACE("model:note", includedNotesStr(m_includedNotes) << message)

#define NMDEBUG(message)                                                       \
    QNDEBUG("model:note", includedNotesStr(m_includedNotes) << message)

#define NMINFO(message)                                                        \
    QNINFO("model:note", includedNotesStr(m_includedNotes) << message)

#define NMWARNING(message)                                                     \
    QNWARNING("model:note", includedNotesStr(m_includedNotes) << message)

#define NMERROR(message)                                                       \
    QNERROR("model:note", includedNotesStr(m_includedNotes) << message)

#define REPORT_ERROR(error, ...)                                               \
    ErrorString errorDescription(error);                                       \
    NMWARNING(errorDescription << QLatin1String("" __VA_ARGS__ ""));           \
    Q_EMIT notifyError(errorDescription) // REPORT_ERROR

namespace quentier {

namespace {

// Minimum number of notes which the model attempts to load from the local
// storage
constexpr int gNoteMinCacheSize = 30;

constexpr int gNoteModelColumnCount = 12;

// Limit for list notes from local storage queries
constexpr int gNoteListQueryLimit = 10;

} // namespace

NoteModel::NoteModel(
    Account account,
    local_storage::ILocalStoragePtr localStorage, NoteCache & noteCache,
    NotebookCache & notebookCache, QObject * parent,
    const IncludedNotes includedNotes,
    const NoteSortingMode noteSortingMode, std::optional<NoteFilters> filters) :
    QAbstractItemModel{parent},
    m_localStorage{std::move(localStorage)}, m_includedNotes{includedNotes},
    m_account{std::move(account)}, m_noteSortingMode{noteSortingMode},
    m_cache{noteCache}, m_notebookCache{notebookCache},
    m_filters{std::move(filters)}, m_maxNoteCount{gNoteMinCacheSize * 2}
{}

NoteModel::~NoteModel() = default;

void NoteModel::updateAccount(Account account)
{
    NMDEBUG("NoteModel::updateAccount: " << account);
    m_account = std::move(account);
}

NoteModel::Column NoteModel::sortingColumn() const noexcept
{
    switch (m_noteSortingMode) {
    case NoteSortingMode::CreatedAscending:
    case NoteSortingMode::CreatedDescending:
        return Column::CreationTimestamp;
    case NoteSortingMode::ModifiedAscending:
    case NoteSortingMode::ModifiedDescending:
        return Column::ModificationTimestamp;
    case NoteSortingMode::TitleAscending:
    case NoteSortingMode::TitleDescending:
        return Column::Title;
    case NoteSortingMode::SizeAscending:
    case NoteSortingMode::SizeDescending:
        return Column::Size;
    default:
        return Column::ModificationTimestamp;
    }
}

Qt::SortOrder NoteModel::sortOrder() const noexcept
{
    switch (m_noteSortingMode) {
    case NoteSortingMode::CreatedDescending:
    case NoteSortingMode::ModifiedDescending:
    case NoteSortingMode::TitleDescending:
    case NoteSortingMode::SizeDescending:
        return Qt::DescendingOrder;
    default:
        return Qt::AscendingOrder;
    }
}

QModelIndex NoteModel::indexForLocalId(const QString & localId) const
{
    const auto & localIdIndex = m_data.get<ByLocalId>();
    const auto it = localIdIndex.find(localId);
    if (Q_UNLIKELY(it == localIdIndex.end())) {
        NMDEBUG("Can't find note item by local id: " << localId);
        return {};
    }

    const auto & index = m_data.get<ByIndex>();
    const auto indexIt = m_data.project<ByIndex>(it);
    if (Q_UNLIKELY(indexIt == index.end())) {
        NMWARNING("Can't find the indexed reference to the note item: " << *it);
        return {};
    }

    const int rowIndex =
        static_cast<int>(std::distance(index.begin(), indexIt));
    return createIndex(rowIndex, static_cast<int>(Column::Title));
}

const NoteModelItem * NoteModel::itemForLocalId(const QString & localId) const
{
    const auto & localIdIndex = m_data.get<ByLocalId>();
    const auto it = localIdIndex.find(localId);
    if (Q_UNLIKELY(it == localIdIndex.end())) {
        NMDEBUG("Can't find note item by local id: " << localId);
        return nullptr;
    }

    return &(*it);
}

const NoteModelItem * NoteModel::itemAtRow(const int row) const
{
    const auto & index = m_data.get<ByIndex>();
    if (Q_UNLIKELY(
            (row < 0) || (index.size() <= static_cast<std::size_t>(row))))
    {
        return nullptr;
    }

    return &(index[static_cast<size_t>(row)]);
}

const NoteModelItem * NoteModel::itemForIndex(const QModelIndex & index) const
{
    if (!index.isValid()) {
        return nullptr;
    }

    if (index.parent().isValid()) {
        return nullptr;
    }

    return itemAtRow(index.row());
}

bool NoteModel::hasFilters() const noexcept
{
    return !m_filters->isEmpty();
}

const QStringList & NoteModel::filteredNotebookLocalIds() const noexcept
{
    return m_filters->filteredNotebookLocalIds();
}

void NoteModel::setFilteredNotebookLocalIds(QStringList notebookLocalIds)
{
    NMDEBUG(
        "NoteModel::setFilteredNotebookLocalIds: "
        << notebookLocalIds.join(QStringLiteral(", ")));

    if (m_updatedNoteFilters) {
        m_updatedNoteFilters->setFilteredNotebookLocalIds(notebookLocalIds);
        return;
    }

    m_filters->setFilteredNotebookLocalIds(notebookLocalIds);

    if (m_isStarted) {
        resetModel();
    }
}

void NoteModel::clearFilteredNotebookLocalIds()
{
    NMDEBUG("NoteModel::clearFilteredNotebookLocalIds");

    if (m_updatedNoteFilters) {
        m_updatedNoteFilters->clearFilteredNotebookLocalIds();
        return;
    }

    m_filters->clearFilteredNotebookLocalIds();

    if (m_isStarted) {
        resetModel();
    }
}

const QStringList & NoteModel::filteredTagLocalIds() const noexcept
{
    return m_filters->filteredTagLocalIds();
}

void NoteModel::setFilteredTagLocalIds(QStringList tagLocalIds)
{
    NMDEBUG(
        "NoteModel::setFilteredTagLocalIds: "
        << tagLocalIds.join(QStringLiteral(", ")));

    if (m_updatedNoteFilters) {
        m_updatedNoteFilters->setFilteredTagLocalIds(std::move(tagLocalIds));
        return;
    }

    m_filters->setFilteredTagLocalIds(std::move(tagLocalIds));

    if (m_isStarted) {
        resetModel();
    }
}

void NoteModel::clearFilteredTagLocalIds()
{
    NMDEBUG("NoteModel::clearFilteredTagLocalIds");

    if (m_updatedNoteFilters) {
        m_updatedNoteFilters->clearFilteredTagLocalIds();
        return;
    }

    m_filters->clearFilteredTagLocalIds();

    if (m_isStarted) {
        resetModel();
    }
}

const QSet<QString> & NoteModel::filteredNoteLocalIds() const noexcept
{
    return m_filters->filteredNoteLocalIds();
}

void NoteModel::setFilteredNoteLocalIds(QSet<QString> noteLocalIds)
{
    if (QuentierIsLogLevelActive(LogLevel::Debug)) {
        QString str;
        QTextStream strm{&str};
        strm << "NoteModel::setFilteredNoteLocalIds: ";
        for (auto it = noteLocalIds.constBegin(),
                  end = noteLocalIds.constEnd();
             it != end; ++it)
        {
            if (it != noteLocalIds.constBegin()) {
                strm << ", ";
            }
            strm << *it;
        }
        strm.flush();
        NMDEBUG(str);
    }

    if (m_updatedNoteFilters) {
        m_updatedNoteFilters->setFilteredNoteLocalIds(std::move(noteLocalIds));
        return;
    }

    if (m_filters->setFilteredNoteLocalIds(std::move(noteLocalIds))) {
        if (m_isStarted) {
            resetModel();
        }
    }
    else {
        NMDEBUG("The set of filtered note local ids hasn't changed");
    }
}

void NoteModel::setFilteredNoteLocalIds(const QStringList & noteLocalIds)
{
    NMDEBUG(
        "NoteModel::setFilteredNoteLocalIds: "
        << noteLocalIds.join(QStringLiteral(", ")));

    if (m_updatedNoteFilters) {
        m_updatedNoteFilters->setFilteredNoteLocalIds(noteLocalIds);
        return;
    }

    if (m_filters->setFilteredNoteLocalIds(noteLocalIds)) {
        if (m_isStarted) {
            resetModel();
        }
    }
    else {
        NMDEBUG("The set of filtered note local ids hasn't changed");
    }
}

void NoteModel::clearFilteredNoteLocalIds()
{
    NMDEBUG("NoteModel::clearFilteredNoteLocalIds");

    if (m_updatedNoteFilters) {
        m_updatedNoteFilters->clearFilteredNoteLocalIds();
        return;
    }

    m_filters->clearFilteredNoteLocalIds();
    resetModel();
}

void NoteModel::beginUpdateFilter()
{
    NMDEBUG("NoteModel::beginUpdateFilter");
    m_updatedNoteFilters.emplace(NoteFilters{});
}

void NoteModel::endUpdateFilter()
{
    NMDEBUG("NoteModel::endUpdateFilter");

    if (!m_updatedNoteFilters) {
        return;
    }

    m_filters.swap(m_updatedNoteFilters);
    m_updatedNoteFilters.reset();

    resetModel();
}

qint32 NoteModel::totalFilteredNotesCount() const noexcept
{
    return m_totalFilteredNotesCount;
}

qint32 NoteModel::totalAccountNotesCount() const noexcept
{
    return m_totalAccountNotesCount;
}

QModelIndex NoteModel::createNoteItem(
    const QString & notebookLocalId, ErrorString & errorDescription)
{
    NMDEBUG(
        "NoteModel::createNoteItem: notebook local id = " << notebookLocalId);

    if (Q_UNLIKELY(!m_isStarted)) {
        errorDescription.setBase(
            QT_TR_NOOP("Can't create new note: note model is not started"));
        NMWARNING(errorDescription);
        return {};
    }

    if (Q_UNLIKELY(m_includedNotes == IncludedNotes::Deleted)) {
        errorDescription.setBase(QT_TR_NOOP("Won't create new deleted note"));
        NMDEBUG(errorDescription);
        return {};
    }

    if (m_pendingFullNoteCountPerAccount) {
        errorDescription.setBase(
            QT_TR_NOOP("Note model is not ready to create a new note, please "
                       "wait"));
        NMDEBUG(errorDescription);
        return {};
    }

    if (m_totalAccountNotesCount >= m_account.noteCountMax()) {
        errorDescription.setBase(
            QT_TR_NOOP("Can't create a new note: the account already contains "
                       "max allowed number of notes"));
        errorDescription.details() = QString::number(m_account.noteCountMax());
        NMINFO(errorDescription);
        return {};
    }

    const auto notebookIt =
        m_notebookDataByNotebookLocalId.find(notebookLocalId);
    if (notebookIt == m_notebookDataByNotebookLocalId.end()) {
        errorDescription.setBase(
            QT_TR_NOOP("Can't create a new note: internal error, can't "
                       "identify the notebook in which the note needs to be "
                       "created"));
        NMINFO(errorDescription);
        return {};
    }

    const auto & notebookData = notebookIt.value();
    if (!notebookData.m_canCreateNotes) {
        errorDescription.setBase(
            QT_TR_NOOP("Can't create a new note: notebook restrictions apply"));
        NMDEBUG(errorDescription);
        return {};
    }

    NoteModelItem item;
    item.setLocalId(UidGenerator::Generate());
    item.setNotebookLocalId(notebookLocalId);
    item.setNotebookGuid(notebookData.m_guid);
    item.setNotebookName(notebookData.m_name);
    item.setCreationTimestamp(QDateTime::currentMSecsSinceEpoch());
    item.setModificationTimestamp(item.creationTimestamp());
    item.setDirty(true);
    item.setSynchronizable(m_account.type() != Account::Type::Local);

    const int row = rowForNewItem(item);
    beginInsertRows(QModelIndex(), row, row);

    auto & index = m_data.get<ByIndex>();
    const auto indexIt = index.begin() + row;
    index.insert(indexIt, item);

    endInsertRows();

    Q_UNUSED(
        m_localIdsOfNewNotesBeingAddedToLocalStorage.insert(item.localId()))

    saveNoteInLocalStorage(item);

    return createIndex(row, static_cast<int>(Column::Title));
}

bool NoteModel::deleteNote(
    const QString & noteLocalId, ErrorString & errorDescription)
{
    NMDEBUG("NoteModel::deleteNote: " << noteLocalId);

    auto itemIndex = indexForLocalId(noteLocalId);
    if (!itemIndex.isValid()) {
        errorDescription.setBase(
            QT_TR_NOOP("note to be deleted was not found"));
        NMDEBUG(errorDescription);
        return false;
    }

    itemIndex = index(
        itemIndex.row(), static_cast<int>(Column::DeletionTimestamp),
        itemIndex.parent());

    const qint64 timestamp = QDateTime::currentMSecsSinceEpoch();
    return setDataImpl(itemIndex, timestamp, errorDescription);
}

bool NoteModel::moveNoteToNotebook(
    const QString & noteLocalId, const QString & notebookName,
    ErrorString & errorDescription)
{
    NMDEBUG(
        "NoteModel::moveNoteToNotebook: note local id = "
        << noteLocalId << ", notebook name = " << notebookName);

    if (Q_UNLIKELY(notebookName.isEmpty())) {
        errorDescription.setBase(
            QT_TR_NOOP("the name of the target notebook is empty"));
        return false;
    }

    auto & localIdIndex = m_data.get<ByLocalId>();
    const auto it = localIdIndex.find(noteLocalId);
    if (Q_UNLIKELY(it == localIdIndex.end())) {
        errorDescription.setBase(
            QT_TR_NOOP("can't find the note to be moved to another notebook"));
        return false;
    }

    /**
     * First try to find the notebook in the cache; the cache is indexed by
     * local id, not by name so need to do the linear search. It should be OK
     * since the cache is intended to be small
     */
    for (const auto & pair: m_notebookCache) {
        const auto & notebook = pair.second;
        if (notebook.name() && *notebook.name() == notebookName) {
            return moveNoteToNotebookImpl(it, notebook, errorDescription);
        }
    }

    /**
     * 2) No such notebook in the cache; attempt to find it within the local
     * storage asynchronously then
     */
    auto findNotebookFuture = m_localStorage->findNotebookByName(notebookName);

    auto findNotebookThenFuture = threading::then(
        std::move(findNotebookFuture), this,
        [this, notebookName,
         noteLocalId](const std::optional<qevercloud::Notebook> & notebook) {
            if (Q_UNLIKELY(!notebook)) {
                ErrorString error{
                    QT_TR_NOOP("Can't move the note to another notebook: "
                               "failed to find the target notebook")};
                NMWARNING(
                    error << ", note local id = " << noteLocalId
                          << ", notebook name = " << notebookName);
                Q_EMIT notifyError(std::move(error));
                return;
            }

            m_notebookCache.put(notebook->localId(), *notebook);

            auto & localIdIndex = m_data.get<ByLocalId>();
            const auto it = localIdIndex.find(noteLocalId);
            if (it == localIdIndex.end()) {
                REPORT_ERROR(QT_TR_NOOP(
                    "Can't move the note to another notebook: internal "
                    "error, can't find the item within the note model "
                    "by local id"));
                return;
            }

            ErrorString error;
            if (!moveNoteToNotebookImpl(it, *notebook, error)) {
                ErrorString errorDescription{
                    QT_TR_NOOP("Can't move note to another notebook")};

                errorDescription.appendBase(error.base());
                errorDescription.appendBase(error.additionalBases());
                errorDescription.details() = error.details();
                NMWARNING(
                    errorDescription << ", note local id = " << noteLocalId
                                     << ", notebook name = " << notebookName);
                Q_EMIT notifyError(std::move(errorDescription));
            }
        });

    threading::onFailed(
        std::move(findNotebookThenFuture), this,
        [this, notebookName, noteLocalId](const QException & e) {
            auto message = exceptionMessage(e);
            ErrorString errorDescription{
                QT_TR_NOOP("Can't move note to another notebook")};

            errorDescription.appendBase(message.base());
            errorDescription.appendBase(message.additionalBases());
            errorDescription.details() = message.details();
            NMWARNING(
                errorDescription << ", note local id = " << noteLocalId
                                 << ", notebook name = " << notebookName);
            Q_EMIT notifyError(std::move(errorDescription));
        });

    return true;
}

bool NoteModel::favoriteNote(
    const QString & noteLocalId, ErrorString & errorDescription)
{
    NMDEBUG("NoteModel::favoriteNote: " << noteLocalId);
    return setNoteFavorited(noteLocalId, true, errorDescription);
}

bool NoteModel::unfavoriteNote(
    const QString & noteLocalId, ErrorString & errorDescription)
{
    NMDEBUG("NoteModel::unfavoriteNote: " << noteLocalId);
    return setNoteFavorited(noteLocalId, false, errorDescription);
}

Qt::ItemFlags NoteModel::flags(const QModelIndex & modelIndex) const
{
    Qt::ItemFlags indexFlags = QAbstractItemModel::flags(modelIndex);
    if (!modelIndex.isValid()) {
        return indexFlags;
    }

    const int row = modelIndex.row();
    const int column = modelIndex.column();

    if ((row < 0) || (row >= static_cast<int>(m_data.size())) || (column < 0) ||
        (column >= gNoteModelColumnCount))
    {
        return indexFlags;
    }

    indexFlags |= Qt::ItemIsSelectable;
    indexFlags |= Qt::ItemIsEnabled;

    if (column == static_cast<int>(Column::Dirty) ||
        column == static_cast<int>(Column::Size) ||
        column == static_cast<int>(Column::Synchronizable) ||
        column == static_cast<int>(Column::HasResources))
    {
        return indexFlags;
    }

    const auto & index = m_data.get<ByIndex>();
    const auto & item = index.at(static_cast<size_t>(row));
    if (!canUpdateNoteItem(item)) {
        return indexFlags;
    }

    indexFlags |= Qt::ItemIsEditable;
    return indexFlags;
}

QVariant NoteModel::data(const QModelIndex & index, int role) const
{
    if (!index.isValid()) {
        return {};
    }

    const int rowIndex = index.row();
    const int columnIndex = index.column();

    if ((rowIndex < 0) || (rowIndex >= static_cast<int>(m_data.size())) ||
        (columnIndex < 0) || (columnIndex >= gNoteModelColumnCount))
    {
        return {};
    }

    if (role == Qt::ToolTipRole) {
        return dataImpl(rowIndex, Column::Title);
    }

    Column column;
    switch (columnIndex) {
    case static_cast<int>(Column::CreationTimestamp):
    case static_cast<int>(Column::ModificationTimestamp):
    case static_cast<int>(Column::DeletionTimestamp):
    case static_cast<int>(Column::Title):
    case static_cast<int>(Column::PreviewText):
    case static_cast<int>(Column::ThumbnailImage):
    case static_cast<int>(Column::NotebookName):
    case static_cast<int>(Column::TagNameList):
    case static_cast<int>(Column::Size):
    case static_cast<int>(Column::Synchronizable):
    case static_cast<int>(Column::Dirty):
    case static_cast<int>(Column::HasResources):
        column = static_cast<Column>(columnIndex);
        break;
    default:
        return {};
    }

    switch (role) {
    case Qt::DisplayRole:
    case Qt::EditRole:
    case Qt::ToolTipRole:
        return dataImpl(rowIndex, column);
    case Qt::AccessibleTextRole:
    case Qt::AccessibleDescriptionRole:
        return dataAccessibleText(rowIndex, column);
    default:
        return {};
    }
}

QVariant NoteModel::headerData(
    int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole) {
        return {};
    }

    if (orientation == Qt::Vertical) {
        return QVariant{section + 1};
    }

    switch (section) {
    case static_cast<int>(Column::CreationTimestamp):
        // TRANSLATOR: note's creation timestamp
        return QVariant{tr("Created")};
    case static_cast<int>(Column::ModificationTimestamp):
        // TRANSLATOR: note's modification timestamp
        return QVariant{tr("Modified")};
    case static_cast<int>(Column::DeletionTimestamp):
        // TRANSLATOR: note's deletion timestamp
        return QVariant{tr("Deleted")};
    case static_cast<int>(Column::Title):
        return QVariant{tr("Title")};
    case static_cast<int>(Column::PreviewText):
        // TRANSLATOR: a short excerpt of note's text
        return QVariant{tr("Preview")};
    case static_cast<int>(Column::NotebookName):
        return QVariant{tr("Notebook")};
    case static_cast<int>(Column::TagNameList):
        // TRANSLATOR: the list of note's tags
        return QVariant{tr("Tags")};
    case static_cast<int>(Column::Size):
        // TRANSLATOR: size of note in bytes
        return QVariant{tr("Size")};
    case static_cast<int>(Column::Synchronizable):
        return QVariant{tr("Synchronizable")};
    case static_cast<int>(Column::Dirty):
        return QVariant{tr("Dirty")};
    case static_cast<int>(Column::HasResources):
        return QVariant{tr("Has attachments")};
    // NOTE: intentional fall-through
    case static_cast<int>(Column::ThumbnailImage):
    default:
        return {};
    }
}

int NoteModel::rowCount(const QModelIndex & parent) const
{
    if (parent.isValid()) {
        return 0;
    }

    return static_cast<int>(m_data.size());
}

int NoteModel::columnCount(const QModelIndex & parent) const
{
    if (parent.isValid()) {
        return 0;
    }

    return gNoteModelColumnCount;
}

QModelIndex NoteModel::index(
    const int row, const int column, const QModelIndex & parent) const
{
    if (parent.isValid()) {
        return {};
    }

    if ((row < 0) || (row >= static_cast<int>(m_data.size())) || (column < 0) ||
        (column >= gNoteModelColumnCount))
    {
        return {};
    }

    return createIndex(row, column);
}

QModelIndex NoteModel::parent(const QModelIndex & index) const
{
    Q_UNUSED(index)
    return {};
}

bool NoteModel::setHeaderData(
    const int section, const Qt::Orientation orientation,
    const QVariant & value, const int role)
{
    Q_UNUSED(section)
    Q_UNUSED(orientation)
    Q_UNUSED(value)
    Q_UNUSED(role)
    return false;
}

bool NoteModel::setData(
    const QModelIndex & modelIndex, const QVariant & value, const int role)
{
    if (role != Qt::EditRole) {
        return false;
    }

    ErrorString errorDescription;
    if (!setDataImpl(modelIndex, value, errorDescription)) {
        Q_EMIT notifyError(std::move(errorDescription));
    }

    return true;
}

bool NoteModel::insertRows(
    const int row, const int count, const QModelIndex & parent)
{
    // NOTE: NoteModel's own API is used to create new note items
    Q_UNUSED(row)
    Q_UNUSED(count)
    Q_UNUSED(parent)
    return false;
}

bool NoteModel::removeRows(int row, int count, const QModelIndex & parent)
{
    if (Q_UNLIKELY(parent.isValid())) {
        NMDEBUG(
            "Ignoring the attempt to remove rows from note "
            << "model for valid parent model index");
        return false;
    }

    ErrorString errorDescription;
    if (!removeRowsImpl(row, count, errorDescription)) {
        Q_EMIT notifyError(std::move(errorDescription));
    }

    return true;
}

void NoteModel::sort(int column, Qt::SortOrder order)
{
    NMTRACE(
        "NoteModel::sort: column = "
        << column << ", order = " << order << " ("
        << (order == Qt::AscendingOrder ? "ascending" : "descending") << ")");

    if (column == static_cast<int>(Column::ThumbnailImage) ||
        column == static_cast<int>(Column::TagNameList))
    {
        // Should not sort by these columns
        return;
    }

    if (Q_UNLIKELY((column < 0) || (column >= gNoteModelColumnCount))) {
        return;
    }

    if (column == static_cast<int>(sortingColumn())) {
        if (order == sortOrder()) {
            NMDEBUG(
                "Neither sorted column nor sort order have changed, nothing to "
                    << "do");
            return;
        }

        setSortingOrder(order);

        NMDEBUG("Only the sort order has changed");
    }
    else {
        setSortingColumnAndOrder(column, order);
    }

    if (m_isStarted) {
        resetModel();
    }
}

bool NoteModel::canFetchMore(const QModelIndex & parent) const
{
    if (parent.isValid()) {
        return false;
    }

    NMDEBUG("NoteModel::canFetchMore");

    if (Q_UNLIKELY(!m_isStarted)) {
        NMDEBUG("Note model is not started");
        return false;
    }

    if (m_totalFilteredNotesCount > 0) {
        NMDEBUG(
            "Total filtered notes count = " << m_totalFilteredNotesCount
                                            << ", num loaded notes = "
                                            << m_data.size());
        return (m_data.size() < static_cast<size_t>(m_totalFilteredNotesCount));
    }

    if (m_pendingNoteCount) {
        NMDEBUG("Still pending note count");
        return false;
    }

    if (m_pendingNotesList) {
        NMDEBUG("Still pending notes list");
        return false;
    }

    return true;
}

void NoteModel::fetchMore(const QModelIndex & parent)
{
    NMDEBUG("NoteModel::fetchMore");

    if (!canFetchMore(parent)) {
        NMDEBUG("Can't fetch more");
        return;
    }

    m_maxNoteCount += gNoteListQueryLimit;
    requestNotesList();
}

void NoteModel::start()
{
    NMDEBUG("NoteModel::start");

    if (m_isStarted) {
        NMDEBUG("Already started");
        return;
    }

    m_isStarted = true;

    if (!m_filters) {
        m_filters.emplace(NoteFilters{});
    }

    connectToLocalStorageEvents(m_localStorage->notifier());
    requestNotesListAndCount();
}

void NoteModel::stop(const StopMode stopMode)
{
    NMDEBUG("NoteModel::stop: mode = " << stopMode);

    if (!m_isStarted) {
        NMDEBUG("Already stopped");
        return;
    }

    m_isStarted = false;
    disconnectFromLocalStorageEvents();
    clearModel();
}

// FIXME: remove this when it's no longer needed
/*
void NoteModel::onAddNoteFailed(
    Note note, ErrorString errorDescription, QUuid requestId)
{
    auto it = m_addNoteRequestIds.find(requestId);
    if (it == m_addNoteRequestIds.end()) {
        return;
    }

    NMDEBUG(
        "NoteModel::onAddNoteFailed: note = "
        << note << "\nError description = " << errorDescription
        << ", request id = " << requestId);

    Q_UNUSED(m_addNoteRequestIds.erase(it))

    Q_EMIT notifyError(errorDescription);
    removeItemByLocalId(note.localId());
}

void NoteModel::onUpdateNoteFailed(
    Note note, LocalStorageManager::UpdateNoteOptions options,
    ErrorString errorDescription, QUuid requestId)
{
    Q_UNUSED(options)

    auto it = m_updateNoteRequestIds.find(requestId);
    if (it == m_updateNoteRequestIds.end()) {
        return;
    }

    NMDEBUG(
        "NoteModel::onUpdateNoteFailed: note = "
        << note << "\nError description = " << errorDescription
        << ", request id = " << requestId);

    Q_UNUSED(m_updateNoteRequestIds.erase(it))

    findNoteToRestoreFailedUpdate(note);
}

void NoteModel::onFindNoteComplete(
    Note note, LocalStorageManager::GetNoteOptions options, QUuid requestId)
{
    Q_UNUSED(options)

    auto restoreUpdateIt =
        m_findNoteToRestoreFailedUpdateRequestIds.find(requestId);

    auto performUpdateIt = m_findNoteToPerformUpdateRequestIds.find(requestId);

    if ((restoreUpdateIt == m_findNoteToRestoreFailedUpdateRequestIds.end()) &&
        (performUpdateIt == m_findNoteToPerformUpdateRequestIds.end()))
    {
        return;
    }

    NMDEBUG(
        "NoteModel::onFindNoteComplete: note = " << note << "\nRequest id = "
                                                 << requestId);

    if (restoreUpdateIt != m_findNoteToRestoreFailedUpdateRequestIds.end()) {
        Q_UNUSED(
            m_findNoteToRestoreFailedUpdateRequestIds.erase(restoreUpdateIt))

        onNoteAddedOrUpdated(note);
    }
    else if (performUpdateIt != m_findNoteToPerformUpdateRequestIds.end()) {
        Q_UNUSED(m_findNoteToPerformUpdateRequestIds.erase(performUpdateIt))

        m_cache.put(note.localId(), note);

        auto & localIdIndex = m_data.get<ByLocalId>();
        auto it = localIdIndex.find(note.localId());
        if (it != localIdIndex.end()) {
            saveNoteInLocalStorage(*it);
        }
    }
}

void NoteModel::onFindNoteFailed(
    Note note, LocalStorageManager::GetNoteOptions options,
    ErrorString errorDescription, QUuid requestId)
{
    Q_UNUSED(options)

    auto restoreUpdateIt =
        m_findNoteToRestoreFailedUpdateRequestIds.find(requestId);

    auto performUpdateIt = m_findNoteToPerformUpdateRequestIds.find(requestId);

    if ((restoreUpdateIt == m_findNoteToRestoreFailedUpdateRequestIds.end()) &&
        (performUpdateIt == m_findNoteToPerformUpdateRequestIds.end()))
    {
        return;
    }

    NMDEBUG(
        "NoteModel::onFindNoteFailed: note = "
        << note << "\nError description = " << errorDescription
        << ", request id = " << requestId);

    if (restoreUpdateIt != m_findNoteToRestoreFailedUpdateRequestIds.end()) {
        Q_UNUSED(
            m_findNoteToRestoreFailedUpdateRequestIds.erase(restoreUpdateIt))
    }
    else if (performUpdateIt != m_findNoteToPerformUpdateRequestIds.end()) {
        Q_UNUSED(m_findNoteToPerformUpdateRequestIds.erase(performUpdateIt))
    }

    Q_EMIT notifyError(errorDescription);
}

void NoteModel::onListNotesComplete(
    LocalStorageManager::ListObjectsOptions flag,
    LocalStorageManager::GetNoteOptions options, size_t limit, size_t offset,
    LocalStorageManager::ListNotesOrder order,
    LocalStorageManager::OrderDirection orderDirection,
    QString linkedNotebookGuid, QList<Note> foundNotes, QUuid requestId)
{
    if (requestId != m_listNotesRequestId) {
        return;
    }

    NMDEBUG(
        "NoteModel::onListNotesComplete: flag = "
        << flag << ", with resource metadata = "
        << ((options & LocalStorageManager::GetNoteOption::WithResourceMetadata)
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

    onListNotesCompleteImpl(foundNotes);
}

void NoteModel::onListNotesFailed(
    LocalStorageManager::ListObjectsOptions flag,
    LocalStorageManager::GetNoteOptions options, size_t limit, size_t offset,
    LocalStorageManager::ListNotesOrder order,
    LocalStorageManager::OrderDirection orderDirection,
    QString linkedNotebookGuid, ErrorString errorDescription, QUuid requestId)
{
    if (requestId != m_listNotesRequestId) {
        return;
    }

    NMDEBUG(
        "NoteModel::onListNotesFailed: flag = "
        << flag << ", with resource metadata = "
        << ((options & LocalStorageManager::GetNoteOption::WithResourceMetadata)
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

void NoteModel::onListNotesPerNotebooksAndTagsComplete(
    QStringList notebookLocalIds, QStringList tagLocalIds,
    LocalStorageManager::GetNoteOptions options,
    LocalStorageManager::ListObjectsOptions flag, size_t limit, size_t offset,
    LocalStorageManager::ListNotesOrder order,
    LocalStorageManager::OrderDirection orderDirection, QList<Note> foundNotes,
    QUuid requestId)
{
    if (requestId != m_listNotesRequestId) {
        return;
    }

    NMDEBUG(
        "NoteModel::onListNotesPerNotebooksAndTagsComplete: "
        << "flag = " << flag << ", with resource metadata = "
        << ((options & LocalStorageManager::GetNoteOption::WithResourceMetadata)
                ? "true"
                : "false")
        << ", with resource binary data = "
        << ((options &
             LocalStorageManager::GetNoteOption::WithResourceBinaryData)
                ? "true"
                : "false")
        << ", limit = " << limit << ", offset = " << offset
        << ", order = " << order << ", direction = " << orderDirection
        << ", notebook local ids: "
        << notebookLocalIds.join(QStringLiteral(", "))
        << ", tag local ids: " << tagLocalIds.join(QStringLiteral(", "))
        << ", num found notes = " << foundNotes.size()
        << ", request id = " << requestId);

    onListNotesCompleteImpl(foundNotes);
}

void NoteModel::onListNotesPerNotebooksAndTagsFailed(
    QStringList notebookLocalIds, QStringList tagLocalIds,
    LocalStorageManager::GetNoteOptions options,
    LocalStorageManager::ListObjectsOptions flag, size_t limit, size_t offset,
    LocalStorageManager::ListNotesOrder order,
    LocalStorageManager::OrderDirection orderDirection,
    ErrorString errorDescription, QUuid requestId)
{
    if (requestId != m_listNotesRequestId) {
        return;
    }

    NMDEBUG(
        "NoteModel::onListNotesPerNotebooksAndTagsFailed: "
        << "flag = " << flag << ", with resource metadata = "
        << ((options & LocalStorageManager::GetNoteOption::WithResourceMetadata)
                ? "true"
                : "false")
        << ", with resource binary data = "
        << ((options &
             LocalStorageManager::GetNoteOption::WithResourceBinaryData)
                ? "true"
                : "false")
        << ", limit = " << limit << ", offset = " << offset
        << ", order = " << order << ", direction = " << orderDirection
        << ", notebook local ids: "
        << notebookLocalIds.join(QStringLiteral(", "))
        << ", tag local ids: " << tagLocalIds.join(QStringLiteral(", "))
        << ", error description = " << errorDescription
        << ", request id = " << requestId);

    m_listNotesRequestId = QUuid();
    Q_EMIT notifyError(errorDescription);
}

void NoteModel::onListNotesByLocalIdsComplete(
    QStringList noteLocalIds, LocalStorageManager::GetNoteOptions options,
    LocalStorageManager::ListObjectsOptions flag, size_t limit, size_t offset,
    LocalStorageManager::ListNotesOrder order,
    LocalStorageManager::OrderDirection orderDirection, QList<Note> foundNotes,
    QUuid requestId)
{
    if (requestId != m_listNotesRequestId) {
        return;
    }

    NMDEBUG(
        "NoteModel::onListNotesByLocalIdsComplete: "
        << "flag = " << flag << ", with resource metadata = "
        << ((options & LocalStorageManager::GetNoteOption::WithResourceMetadata)
                ? "true"
                : "false")
        << ", with resource binary data = "
        << ((options &
             LocalStorageManager::GetNoteOption::WithResourceBinaryData)
                ? "true"
                : "false")
        << ", limit = " << limit << ", offset = " << offset
        << ", order = " << order << ", direction = " << orderDirection
        << ", note local ids: " << noteLocalIds.join(QStringLiteral(", "))
        << ", num found notes = " << foundNotes.size()
        << ", request id = " << requestId);

    onListNotesCompleteImpl(foundNotes);
}

void NoteModel::onListNotesByLocalIdsFailed(
    QStringList noteLocalIds, LocalStorageManager::GetNoteOptions options,
    LocalStorageManager::ListObjectsOptions flag, size_t limit, size_t offset,
    LocalStorageManager::ListNotesOrder order,
    LocalStorageManager::OrderDirection orderDirection,
    ErrorString errorDescription, QUuid requestId)
{
    if (requestId != m_listNotesRequestId) {
        return;
    }

    NMDEBUG(
        "NoteModel::onListNotesByLocalIdsFailed: "
        << "flag = " << flag << ", with resource metadata = "
        << ((options & LocalStorageManager::GetNoteOption::WithResourceMetadata)
                ? "true"
                : "false")
        << ", with resource binary data = "
        << ((options &
             LocalStorageManager::GetNoteOption::WithResourceBinaryData)
                ? "true"
                : "false")
        << ", limit = " << limit << ", offset = " << offset
        << ", order = " << order << ", direction = " << orderDirection
        << ", note local ids: " << noteLocalIds.join(QStringLiteral(", "))
        << ", error description = " << errorDescription
        << ", request id = " << requestId);

    m_listNotesRequestId = QUuid();
    Q_EMIT notifyError(errorDescription);
}

void NoteModel::onGetNoteCountComplete(
    int noteCount, LocalStorageManager::NoteCountOptions options,
    QUuid requestId)
{
    Q_UNUSED(options)

    if (m_getFullNoteCountPerAccountRequestId == requestId) {
        NMDEBUG(
            "NoteModel::onGetNoteCountComplete: received total "
            << "note count per account: " << noteCount);

        m_getFullNoteCountPerAccountRequestId = QUuid();

        m_totalAccountNotesCount = noteCount;
        Q_EMIT noteCountPerAccountUpdated(m_totalAccountNotesCount);

        return;
    }

    if (m_getNoteCountRequestId == requestId) {
        NMDEBUG(
            "NoteModel::onGetNoteCountComplete: received "
            << "filtered notes count: " << noteCount);

        m_getNoteCountRequestId = QUuid();

        m_totalFilteredNotesCount = noteCount;
        Q_EMIT filteredNotesCountUpdated(m_totalFilteredNotesCount);

        return;
    }
}

void NoteModel::onGetNoteCountFailed(
    ErrorString errorDescription, LocalStorageManager::NoteCountOptions options,
    QUuid requestId)
{
    Q_UNUSED(options)

    if (m_getFullNoteCountPerAccountRequestId == requestId) {
        NMWARNING(
            "NoteModel::onGetNoteCountFailed: failed to get "
            << "total note count per account: " << errorDescription);

        m_getFullNoteCountPerAccountRequestId = QUuid();

        m_totalAccountNotesCount = 0;
        Q_EMIT noteCountPerAccountUpdated(m_totalAccountNotesCount);

        Q_EMIT notifyError(errorDescription);
        return;
    }

    if (m_getNoteCountRequestId == requestId) {
        NMWARNING(
            "NoteModel::onGetNoteCountFailed: failed to get "
            << "filtered notes count: " << errorDescription);

        m_getNoteCountRequestId = QUuid();

        m_totalFilteredNotesCount = 0;
        Q_EMIT filteredNotesCountUpdated(m_totalFilteredNotesCount);

        Q_EMIT notifyError(errorDescription);
        return;
    }
}

void NoteModel::onGetNoteCountPerNotebooksAndTagsComplete(
    int noteCount, QStringList notebookLocalIds, QStringList tagLocalIds,
    LocalStorageManager::NoteCountOptions options, QUuid requestId)
{
    Q_UNUSED(options)

    if (m_getNoteCountRequestId != requestId) {
        return;
    }

    NMDEBUG(
        "NoteModel::onGetNoteCountPerNotebooksAndTagsComplete: "
        << " note count = " << noteCount << ", notebook local ids: "
        << notebookLocalIds.join(QStringLiteral(", "))
        << ", tag local ids: " << tagLocalIds.join(QStringLiteral(", ")));

    m_getNoteCountRequestId = QUuid();

    m_totalFilteredNotesCount = noteCount;
    Q_EMIT filteredNotesCountUpdated(m_totalFilteredNotesCount);
}

void NoteModel::onGetNoteCountPerNotebooksAndTagsFailed(
    ErrorString errorDescription, QStringList notebookLocalIds,
    QStringList tagLocalIds, LocalStorageManager::NoteCountOptions options,
    QUuid requestId)
{
    Q_UNUSED(options)

    if (m_getNoteCountRequestId != requestId) {
        return;
    }

    NMWARNING(
        "NoteModel::onGetNoteCountPerNotebooksAndTagsFailed: "
        << errorDescription << ", notebook local ids: "
        << notebookLocalIds.join(QStringLiteral(", "))
        << ", tag local ids: " << tagLocalIds.join(QStringLiteral(", ")));

    m_getNoteCountRequestId = QUuid();

    m_totalFilteredNotesCount = 0;
    Q_EMIT filteredNotesCountUpdated(m_totalFilteredNotesCount);

    Q_EMIT notifyError(errorDescription);
}

void NoteModel::onExpungeNoteFailed(
    Note note, ErrorString errorDescription, QUuid requestId)
{
    auto it = m_expungeNoteRequestIds.find(requestId);
    if (it == m_expungeNoteRequestIds.end()) {
        return;
    }

    NMTRACE(
        "NoteModel::onExpungeNoteFailed: note = "
        << note << "\nError description = " << errorDescription
        << ", request id = " << requestId);

    Q_UNUSED(m_expungeNoteRequestIds.erase(it))

    findNoteToRestoreFailedUpdate(note);
}

void NoteModel::onFindNotebookComplete(Notebook notebook, QUuid requestId)
{
    auto fit = m_findNotebookRequestForNotebookLocalId.right.find(requestId);
    auto mit =
        ((fit != m_findNotebookRequestForNotebookLocalId.right.end())
             ? m_noteLocalIdToFindNotebookRequestIdForMoveNoteToNotebookBimap
                   .right.end()
             : m_noteLocalIdToFindNotebookRequestIdForMoveNoteToNotebookBimap
                   .right.find(requestId));

    if ((fit == m_findNotebookRequestForNotebookLocalId.right.end()) &&
        (mit ==
         m_noteLocalIdToFindNotebookRequestIdForMoveNoteToNotebookBimap.right
             .end()))
    {
        return;
    }

    NMTRACE(
        "NoteModel::onFindNotebookComplete: notebook: "
        << notebook << "\nRequest id = " << requestId);

    m_notebookCache.put(notebook.localId(), notebook);

    if (fit != m_findNotebookRequestForNotebookLocalId.right.end()) {
        Q_UNUSED(m_findNotebookRequestForNotebookLocalId.right.erase(fit))
        updateNotebookData(notebook);
    }
    else if (
        mit !=
        m_noteLocalIdToFindNotebookRequestIdForMoveNoteToNotebookBimap.right
            .end())
    {
        QString noteLocalId = mit->second;

        Q_UNUSED(m_noteLocalIdToFindNotebookRequestIdForMoveNoteToNotebookBimap
                     .right.erase(mit))

        auto & localIdIndex = m_data.get<ByLocalId>();
        auto it = localIdIndex.find(noteLocalId);
        if (it == localIdIndex.end()) {
            REPORT_ERROR(
                QT_TR_NOOP("Can't move the note to another notebook: internal "
                           "error, can't find the item within the note model "
                           "by local id"));
            return;
        }

        ErrorString error;
        if (!moveNoteToNotebookImpl(it, notebook, error)) {
            ErrorString errorDescription(
                QT_TR_NOOP("Can't move note to another notebook"));

            errorDescription.appendBase(error.base());
            errorDescription.appendBase(error.additionalBases());
            errorDescription.details() = error.details();
            NMWARNING(errorDescription);
            Q_EMIT notifyError(errorDescription);
        }
    }
}

void NoteModel::onFindNotebookFailed(
    Notebook notebook, ErrorString errorDescription, QUuid requestId)
{
    auto fit = m_findNotebookRequestForNotebookLocalId.right.find(requestId);
    auto mit =
        ((fit != m_findNotebookRequestForNotebookLocalId.right.end())
             ? m_noteLocalIdToFindNotebookRequestIdForMoveNoteToNotebookBimap
                   .right.end()
             : m_noteLocalIdToFindNotebookRequestIdForMoveNoteToNotebookBimap
                   .right.find(requestId));

    if ((fit == m_findNotebookRequestForNotebookLocalId.right.end()) &&
        (mit ==
         m_noteLocalIdToFindNotebookRequestIdForMoveNoteToNotebookBimap.right
             .end()))
    {
        return;
    }

    NMWARNING(
        "NoteModel::onFindNotebookFailed: notebook = "
        << notebook << "\nError description = " << errorDescription
        << ", request id = " << requestId);

    if (fit != m_findNotebookRequestForNotebookLocalId.right.end()) {
        Q_UNUSED(m_findNotebookRequestForNotebookLocalId.right.erase(fit))
    }
    else if (
        mit !=
        m_noteLocalIdToFindNotebookRequestIdForMoveNoteToNotebookBimap.right
            .end())
    {
        Q_UNUSED(m_noteLocalIdToFindNotebookRequestIdForMoveNoteToNotebookBimap
                     .right.erase(mit))

        ErrorString error(
            QT_TR_NOOP("Can't move the note to another notebook: "
                       "failed to find the target notebook"));

        error.appendBase(errorDescription.base());
        error.appendBase(errorDescription.additionalBases());
        error.details() = errorDescription.details();
        NMDEBUG(error);
        Q_EMIT notifyError(error);
    }
}

void NoteModel::onFindTagComplete(Tag tag, QUuid requestId)
{
    auto it = m_findTagRequestForTagLocalId.right.find(requestId);
    if (it == m_findTagRequestForTagLocalId.right.end()) {
        return;
    }

    NMTRACE(
        "NoteModel::onFindTagComplete: tag: " << tag << "\nRequest id = "
                                              << requestId);

    Q_UNUSED(m_findTagRequestForTagLocalId.right.erase(it))
    updateTagData(tag);
}

void NoteModel::onFindTagFailed(
    Tag tag, ErrorString errorDescription, QUuid requestId)
{
    auto it = m_findTagRequestForTagLocalId.right.find(requestId);
    if (it == m_findTagRequestForTagLocalId.right.end()) {
        return;
    }

    NMWARNING(
        "NoteModel::onFindTagFailed: tag: " << tag << "\nError description = "
                                            << errorDescription
                                            << ", request id = " << requestId);

    Q_UNUSED(m_findTagRequestForTagLocalId.right.erase(it))
    Q_EMIT notifyError(errorDescription);
}
*/

void NoteModel::connectToLocalStorageEvents(
    local_storage::ILocalStorageNotifier * notifier)
{
    NMDEBUG("NoteModel::connectToLocalStorageEvents");

    if (m_connectedToLocalStorage) {
        NMDEBUG("Already connected to local storage");
        return;
    }

    QObject::connect(
        notifier, &local_storage::ILocalStorageNotifier::notePut, this,
        [this](const qevercloud::Note & note) { onNotePut(note); });

    QObject::connect(
        notifier, &local_storage::ILocalStorageNotifier::noteUpdated, this,
        [this](
            qevercloud::Note note,
            const local_storage::ILocalStorage::UpdateNoteOptions options) {
            onNoteUpdated(
                std::move(note),
                options.testFlag(
                    local_storage::ILocalStorage::UpdateNoteOption::UpdateTags)
                    ? NoteUpdate::WithTags
                    : NoteUpdate::WithoutTags);
        });

    QObject::connect(
        notifier, &local_storage::ILocalStorageNotifier::noteExpunged, this,
        [this](const QString & localId) {
            if (!m_pendingFullNoteCountPerAccount) {
                requestTotalNotesCountPerAccount();
            }

            if (!m_pendingNoteCount) {
                requestTotalFilteredNotesCount();
            }

            removeItemByLocalId(localId);
        });

    QObject::connect(
        notifier, &local_storage::ILocalStorageNotifier::notebookPut, this,
        [this](const qevercloud::Notebook & notebook) {
            m_notebookCache.put(notebook.localId(), notebook);
            updateNotebookData(notebook);
        });

    QObject::connect(
        notifier, &local_storage::ILocalStorageNotifier::notebookExpunged, this,
        [this](const QString & localId) {
            m_notebookCache.remove(localId);
            m_notebookDataByNotebookLocalId.remove(localId);
        });

    QObject::connect(
        notifier, &local_storage::ILocalStorageNotifier::tagPut, this,
        [this](const qevercloud::Tag & tag) {
            updateTagData(tag);
        });

    QObject::connect(
        notifier, &local_storage::ILocalStorageNotifier::tagExpunged, this,
        [this](const QString & localId, const QStringList & childTagLocalIds) {
            processTagExpunging(localId);
            for (const auto & tagLocalId: std::as_const(childTagLocalIds)) {
                processTagExpunging(tagLocalId);
            }
        });

    m_connectedToLocalStorage = true;
}

void NoteModel::disconnectFromLocalStorageEvents()
{
    NMDEBUG("NoteModel::disconnectFromLocalStorageEvents");

    if (!m_connectedToLocalStorage) {
        NMDEBUG("Already disconnected from local storage");
        return;
    }

    auto * notifier = m_localStorage->notifier();
    Q_ASSERT(notifier);
    notifier->disconnect(this);

    m_connectedToLocalStorage = false;
}

void NoteModel::onNotePut(const qevercloud::Note & note)
{
    const auto & localIdIndex = m_data.get<ByLocalId>();
    if (const auto it = localIdIndex.find(note.localId());
        it != localIdIndex.end())
    {
        onNoteUpdated(note, NoteUpdate::WithTags);
    }
    else {
        onNoteAdded(note);
    }
}

void NoteModel::onNoteAdded(const qevercloud::Note & note)
{
    bool noteIncluded = false;
    if (note.deleted()) {
        noteIncluded |= (m_includedNotes != IncludedNotes::NonDeleted);
    }
    else {
        noteIncluded |= (m_includedNotes != IncludedNotes::Deleted);
    }

    if (noteIncluded && !m_pendingFullNoteCountPerAccount) {
        ++m_totalAccountNotesCount;

        NMTRACE(
            "Note count per account increased to " << m_totalAccountNotesCount);

        Q_EMIT noteCountPerAccountUpdated(m_totalAccountNotesCount);
    }

    if (noteIncluded && !m_pendingNoteCount && noteConformsToFilter(note)) {
        ++m_totalFilteredNotesCount;

        NMTRACE(
            "Filtered notes count increased to " << m_totalFilteredNotesCount);

        Q_EMIT filteredNotesCountUpdated(m_totalFilteredNotesCount);
    }

    onNoteAddedOrUpdated(note);
}

void NoteModel::onNoteUpdated(
    qevercloud::Note note, const NoteUpdate noteUpdate)
{
    bool shouldRemoveNoteFromModel =
        (note.deleted() &&
         (m_includedNotes == IncludedNotes::NonDeleted));

    shouldRemoveNoteFromModel |=
        (!note.deleted() &&
         (m_includedNotes == IncludedNotes::Deleted));

    if (shouldRemoveNoteFromModel) {
        removeItemByLocalId(note.localId());
    }

    if (!shouldRemoveNoteFromModel) {
        if (noteUpdate == NoteUpdate::WithoutTags) {
            const auto & localIdIndex = m_data.get<ByLocalId>();
            if (const auto noteItemIt = localIdIndex.find(note.localId());
                noteItemIt != localIdIndex.end()) {
                const auto & item = *noteItemIt;
                note.setTagGuids(item.tagGuids());
                note.setTagLocalIds(item.tagLocalIds());
                NMTRACE(
                    "Complemented the note with tag local ids and guids: "
                    << note);
            }
        }

        onNoteAddedOrUpdated(note);
    }
}

void NoteModel::onNoteAddedOrUpdated(
    const qevercloud::Note & note, const NoteSource noteSource)
{
    if (noteSource != NoteSource::Listing && !noteConformsToFilter(note)) {
        NMDEBUG(
            "Skipping the note not conforming to "
            << "the specified filter: " << note);
        return;
    }

    NoteModelItem item;
    noteToItem(note, item);

    auto notebookIt =
        m_notebookDataByNotebookLocalId.find(item.notebookLocalId());
    if (notebookIt == m_notebookDataByNotebookLocalId.end()) {
        const auto * notebook = m_notebookCache.get(item.notebookLocalId());
        if (notebook) {
            updateNotebookData(*notebook);
            notebookIt =
                m_notebookDataByNotebookLocalId.find(item.notebookLocalId());
        }
    }

    if (notebookIt == m_notebookDataByNotebookLocalId.end()) {
        bool findNotebookRequestSent = false;

        m_noteItemsPendingNotebookDataUpdate.insert(
            item.notebookLocalId(), item);

        const auto it = m_notebookLocalIdsPendingFindingInLocalStorage.find(
            item.notebookLocalId());

        if (it != m_notebookLocalIdsPendingFindingInLocalStorage.end()) {
            findNotebookRequestSent = true;
        }

        if (!findNotebookRequestSent) {
            auto findNotebookFuture = m_localStorage->findNotebookByLocalId(
                note.notebookLocalId());

            auto findNotebookThenFuture = threading::then(
                std::move(findNotebookFuture), this,
                [this, notebookLocalId = note.notebookLocalId()](
                    const std::optional<qevercloud::Notebook> & notebook) {
                    m_notebookLocalIdsPendingFindingInLocalStorage.remove(
                        notebookLocalId);

                    if (Q_UNLIKELY(!notebook)) {
                        NMWARNING(
                            "Failed to find notebook corresponding to note "
                                << "by local id: " << notebookLocalId);
                        return;
                    }
 
                    updateNotebookData(*notebook);
                });

            threading::onFailed(
                std::move(findNotebookThenFuture), this,
                [this, notebookLocalId = note.notebookLocalId()](
                    const QException & e) {
                    auto message = exceptionMessage(e);
                    ErrorString error{QT_TR_NOOP(
                        "Failed to find notebook corresponding to note by "
                        "local id")};
                    error.appendBase(message.base());
                    error.appendBase(message.additionalBases());
                    error.details() = message.details();
                    NMWARNING(
                        error << ", notebook local id = " << notebookLocalId);
                    Q_EMIT notifyError(std::move(error));
                });

        }
        else {
            NMTRACE(
                "The request to find notebook for this note has already been "
                    << "sent");
        }

        return;
    }

    const auto & notebookData = notebookIt.value();
    addOrUpdateNoteItem(item, notebookData, noteSource);
}

void NoteModel::noteToItem(const qevercloud::Note & note, NoteModelItem & item)
{
    item.setLocalId(note.localId());

    if (note.guid()) {
        item.setGuid(*note.guid());
    }

    if (note.notebookGuid()) {
        item.setNotebookGuid(*note.notebookGuid());
    }

    if (!note.notebookLocalId().isEmpty()) {
        item.setNotebookLocalId(note.notebookLocalId());
    }

    if (note.title()) {
        item.setTitle(*note.title());
    }

    if (note.content()) {
        QString previewText = noteContentToPlainText(*note.content());
        previewText.truncate(500);
        item.setPreviewText(previewText);
    }

    item.setThumbnailData(note.thumbnailData());

    if (!note.tagLocalIds().isEmpty()) {
        const QStringList & tagLocalIds = note.tagLocalIds();
        item.setTagLocalIds(tagLocalIds);

        QStringList tagNames;
        tagNames.reserve(tagLocalIds.size());

        for (const auto & tagLocalId: std::as_const(tagLocalIds)) {
            if (const auto tagIt = m_tagDataByTagLocalId.find(tagLocalId);
                tagIt != m_tagDataByTagLocalId.end())
            {
                const auto & tagData = tagIt.value();
                tagNames << tagData.m_name;
            }
        }

        item.setTagNameList(tagNames);
    }

    if (note.tagGuids()) {
        const QStringList & tagGuids = *note.tagGuids();
        item.setTagGuids(tagGuids);
    }

    if (note.created()) {
        item.setCreationTimestamp(*note.created());
    }

    if (note.updated()) {
        item.setModificationTimestamp(*note.updated());
    }

    if (note.deleted()) {
        item.setDeletionTimestamp(*note.deleted());
    }

    item.setSynchronizable(!note.isLocalOnly());
    item.setDirty(note.isLocallyModified());
    item.setFavorited(note.isLocallyFavorited());
    item.setActive(note.active().value_or(true));
    item.setHasResources(note.resources() && !note.resources()->isEmpty());

    if (note.restrictions()) {
        const auto & restrictions = *note.restrictions();

        item.setCanUpdateTitle(
            !restrictions.noUpdateTitle().value_or(false));

        item.setCanUpdateContent(
            !restrictions.noUpdateContent().value_or(false));

        item.setCanEmail(!restrictions.noEmail().value_or(false));
        item.setCanShare(!restrictions.noShare().value_or(false));
        item.setCanSharePublicly(
            !restrictions.noSharePublicly().value_or(false));
    }
    else {
        item.setCanUpdateTitle(true);
        item.setCanUpdateContent(true);
        item.setCanEmail(true);
        item.setCanShare(true);
        item.setCanSharePublicly(true);
    }

    qint64 sizeInBytes = 0;
    if (note.content()) {
        sizeInBytes += note.content()->size();
    }

    if (note.resources()) {
        auto resources = *note.resources();
        for (const auto & resource: std::as_const(resources)) {
            if (resource.data() && resource.data()->body()) {
                sizeInBytes += resource.data()->body()->size();
            }

            if (resource.recognition() && resource.recognition()->body()) {
                sizeInBytes += resource.recognition()->body()->size();
            }

            if (resource.alternateData() && resource.alternateData()->body()) {
                sizeInBytes += resource.alternateData()->body()->size();
            }
        }
    }

    sizeInBytes = std::max(qint64(0), sizeInBytes);
    item.setSizeInBytes(static_cast<quint64>(sizeInBytes));
}

bool NoteModel::noteConformsToFilter(const qevercloud::Note & note) const
{
    if (Q_UNLIKELY(note.notebookLocalId().isEmpty())) {
        return false;
    }

    if (!m_filters) {
        return true;
    }

    const auto & filteredNoteLocalIds = m_filters->filteredNoteLocalIds();
    if (!filteredNoteLocalIds.isEmpty() &&
        !filteredNoteLocalIds.contains(note.localId()))
    {
        return false;
    }

    const auto & filteredNotebookLocalIds =
        m_filters->filteredNotebookLocalIds();

    if (!filteredNotebookLocalIds.isEmpty() &&
        !filteredNotebookLocalIds.contains(note.notebookLocalId()))
    {
        return false;
    }

    const auto & filteredTagLocalIds = m_filters->filteredTagLocalIds();
    if (!filteredTagLocalIds.isEmpty()) {
        bool foundTag = false;
        const auto & tagLocalIds = note.tagLocalIds();
        for (auto it = tagLocalIds.constBegin(), end = tagLocalIds.end();
             it != end; ++it)
        {
            if (filteredTagLocalIds.contains(*it)) {
                foundTag = true;
                break;
            }
        }

        if (!foundTag) {
            return false;
        }
    }

    return true;
}

void NoteModel::onListNotesCompleteImpl(
    const QList<qevercloud::Note> & foundNotes)
{
    for (const auto & foundNote: std::as_const(foundNotes)) {
        onNoteAddedOrUpdated(foundNote, NoteSource::Listing);
    }

    m_listNotesOffset += static_cast<quint64>(foundNotes.size());

    if (!foundNotes.isEmpty() && (m_data.size() < gNoteMinCacheSize)) {
        NMTRACE(
            "The number of found notes is greater than zero, "
            << "requesting more notes from the local storage");
        requestNotesList();
    }
    else {
        NMDEBUG("Emitting minimalNotesBatchLoaded signal");
        Q_EMIT minimalNotesBatchLoaded();
    }
}

void NoteModel::requestNotesListAndCount()
{
    NMDEBUG("NoteModel::requestNotesListAndCount");

    requestNotesList();
    requestNotesCount();
}

void NoteModel::requestNotesList()
{
    NMDEBUG("NoteModel::requestNotesList");

    local_storage::ILocalStorage::ListNotesOptions options;
    options.m_order =
        local_storage::ILocalStorage::ListNotesOrder::ByModificationTimestamp;

    options.m_direction =
        local_storage::ILocalStorage::OrderDirection::Ascending;

    switch (m_noteSortingMode) {
    case NoteSortingMode::CreatedAscending:
        options.m_order =
            local_storage::ILocalStorage::ListNotesOrder::ByCreationTimestamp;
        options.m_direction =
            local_storage::ILocalStorage::OrderDirection::Ascending;
        break;
    case NoteSortingMode::CreatedDescending:
        options.m_order =
            local_storage::ILocalStorage::ListNotesOrder::ByCreationTimestamp;
        options.m_direction =
            local_storage::ILocalStorage::OrderDirection::Descending;
        break;
    case NoteSortingMode::ModifiedAscending:
        options.m_order =
            local_storage::ILocalStorage::ListNotesOrder::ByModificationTimestamp;
        options.m_direction =
            local_storage::ILocalStorage::OrderDirection::Ascending;
        break;
    case NoteSortingMode::ModifiedDescending:
        options.m_order =
            local_storage::ILocalStorage::ListNotesOrder::ByModificationTimestamp;
        options.m_direction =
            local_storage::ILocalStorage::OrderDirection::Descending;
        break;
    case NoteSortingMode::TitleAscending:
        options.m_order = local_storage::ILocalStorage::ListNotesOrder::ByTitle;
        options.m_direction =
            local_storage::ILocalStorage::OrderDirection::Ascending;
        break;
    case NoteSortingMode::TitleDescending:
        options.m_order = local_storage::ILocalStorage::ListNotesOrder::ByTitle;
        options.m_direction =
            local_storage::ILocalStorage::OrderDirection::Descending;
        break;
    // NOTE: no sorting by side is supported by the local storage so leaving
    // it as is for now
    default:
        break;
    }

    options.m_limit = gNoteListQueryLimit;
    options.m_offset = m_listNotesOffset;

    if (!hasFilters()) {
        NMDEBUG(
            "Requesting notes llist: offset = "
            << m_listNotesOffset << ", order = " << options.m_order
            << ", direction = " << options.m_direction);

        auto listNotesFuture = m_localStorage->listNotes(
            local_storage::ILocalStorage::FetchNoteOptions{},
            options);

        auto listNotesThenFuture = threading::then(
            std::move(listNotesFuture), this,
            [this](const QList<qevercloud::Note> & notes) {
                onListNotesCompleteImpl(notes);
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
                NMWARNING(error);
                Q_EMIT notifyError(std::move(error));
            });

        return;
    }

    const auto & filteredNoteLocalIds = m_filters->filteredNoteLocalIds();
    if (!filteredNoteLocalIds.isEmpty()) {
        int end = static_cast<int>(m_listNotesOffset) + gNoteListQueryLimit;
        end = std::min(end, filteredNoteLocalIds.size());

        auto beginIt = filteredNoteLocalIds.begin();
        std::advance(beginIt, static_cast<int>(m_listNotesOffset));

        auto endIt = filteredNoteLocalIds.begin();
        std::advance(endIt, end);

        QStringList noteLocalIds;

        noteLocalIds.reserve(
            std::max((end - static_cast<int>(m_listNotesOffset)), 0));

        for (auto it = beginIt; it != endIt; ++it) {
            noteLocalIds << *it;
        }

        NMDEBUG(
            "Requesting notes list by local ids: "
            << ", order = " << options.m_order << ", direction = "
            << options.m_direction << ", note local ids: "
            << noteLocalIds.join(QStringLiteral(", ")));

        auto listNotesFuture = m_localStorage->listNotesByLocalIds(
            std::move(noteLocalIds),
            local_storage::ILocalStorage::FetchNoteOptions{}, options);

        auto listNotesThenFuture = threading::then(
            std::move(listNotesFuture), this,
            [this](const QList<qevercloud::Note> & notes) {
                onListNotesCompleteImpl(notes);
            });

        threading::onFailed(
            std::move(listNotesThenFuture), this,
            [this](const QException & e) {
                auto message = exceptionMessage(e);
                ErrorString error{QT_TR_NOOP(
                    "Failed to list notes from local storage by local ids")};
                error.appendBase(message.base());
                error.appendBase(message.additionalBases());
                error.details() = message.details();
                NMWARNING(error);
                Q_EMIT notifyError(std::move(error));
            });

        return;
    }

    const auto & notebookLocalIds = m_filters->filteredNotebookLocalIds();
    const auto & tagLocalIds = m_filters->filteredTagLocalIds();

    NMDEBUG(
        "Requesting notes list per notebooks "
        << "and tags: offset = " << m_listNotesOffset
        << ", order = " << options.m_order
        << ", direction = " << options.m_direction << ", notebook local ids: "
        << notebookLocalIds.join(QStringLiteral(", "))
        << "; tag local ids: " << tagLocalIds.join(QStringLiteral(", ")));

    auto listNotesFuture = m_localStorage->listNotesPerNotebookAndTagLocalIds(
        std::move(notebookLocalIds), std::move(tagLocalIds),
        local_storage::ILocalStorage::FetchNoteOptions{}, options);

    auto listNotesThenFuture = threading::then(
        std::move(listNotesFuture), this,
        [this](const QList<qevercloud::Note> & notes) {
            onListNotesCompleteImpl(notes);
        });

    threading::onFailed(
        std::move(listNotesThenFuture), this,
        [this](const QException & e) {
            auto message = exceptionMessage(e);
            ErrorString error{QT_TR_NOOP(
                "Failed to list notes from local storage by notebook and "
                "tag local ids")};
            error.appendBase(message.base());
            error.appendBase(message.additionalBases());
            error.details() = message.details();
            NMWARNING(error);
            Q_EMIT notifyError(std::move(error));
        });
}

void NoteModel::requestNotesCount()
{
    NMDEBUG("NoteModel::requestNotesCount");

    if (m_totalAccountNotesCount == 0) {
        requestTotalNotesCountPerAccount();
    }

    if (m_totalFilteredNotesCount == 0) {
        requestTotalFilteredNotesCount();
    }
}

void NoteModel::requestTotalNotesCountPerAccount()
{
    NMDEBUG("NoteModel::requestTotalNotesCountPerAccount");

    m_getFullNoteCountPerAccountRequestId = QUuid::createUuid();

    NMDEBUG(
        "Emitting the request to get full note count per account: request "
        << "id = " << m_getFullNoteCountPerAccountRequestId);

    LocalStorageManager::NoteCountOptions options = noteCountOptions();
    Q_EMIT getNoteCount(options, m_getFullNoteCountPerAccountRequestId);
}

void NoteModel::requestTotalFilteredNotesCount()
{
    NMDEBUG("NoteModel::requestTotalFilteredNotesCount");

    if (!hasFilters()) {
        m_getNoteCountRequestId = QUuid::createUuid();
        LocalStorageManager::NoteCountOptions options = noteCountOptions();

        NMDEBUG(
            "Emitting the request to get note count: options = "
            << options << ", request id = " << m_getNoteCountRequestId);

        Q_EMIT getNoteCount(options, m_getNoteCountRequestId);
        return;
    }

    const auto & filteredNoteLocalIds = m_filters->filteredNoteLocalIds();
    if (!filteredNoteLocalIds.isEmpty()) {
        m_getNoteCountRequestId = QUuid();
        m_totalFilteredNotesCount = filteredNoteLocalIds.size();
        Q_EMIT filteredNotesCountUpdated(m_totalFilteredNotesCount);
        return;
    }

    const auto & notebookLocalIds = m_filters->filteredNotebookLocalIds();
    const auto & tagLocalIds = m_filters->filteredTagLocalIds();

    m_getNoteCountRequestId = QUuid::createUuid();
    LocalStorageManager::NoteCountOptions options = noteCountOptions();

    NMDEBUG(
        "Emitting the request to get note count per notebooks and tags: "
        << "options = " << options
        << ", request id = " << m_getNoteCountRequestId);

    Q_EMIT getNoteCountPerNotebooksAndTags(
        notebookLocalIds, tagLocalIds, options, m_getNoteCountRequestId);
}

void NoteModel::findNoteToRestoreFailedUpdate(const Note & note)
{
    NMDEBUG(
        "NoteModel::findNoteToRestoreFailedUpdate: local id = "
        << note.localId());

    auto requestId = QUuid::createUuid();
    Q_UNUSED(m_findNoteToRestoreFailedUpdateRequestIds.insert(requestId))

    NMTRACE(
        "Emitting the request to find a note: local id = "
        << note.localId() << ", request id = " << requestId);

    LocalStorageManager::GetNoteOptions getNoteOptions(
        LocalStorageManager::GetNoteOption::WithResourceMetadata);

    Q_EMIT findNote(note, getNoteOptions, requestId);
}

void NoteModel::clearModel()
{
    NMDEBUG("NoteModel::clearModel");

    beginResetModel();

    m_data.clear();
    m_totalFilteredNotesCount = 0;
    m_maxNoteCount = gNoteMinCacheSize * 2;
    m_listNotesOffset = 0;
    m_listNotesRequestId = QUuid();
    m_getNoteCountRequestId = QUuid();
    m_totalAccountNotesCount = 0;
    m_getFullNoteCountPerAccountRequestId = QUuid();
    m_notebookDataByNotebookLocalId.clear();
    m_findNotebookRequestForNotebookLocalId.clear();
    m_localIdsOfNewNotesBeingAddedToLocalStorage.clear();
    m_addNoteRequestIds.clear();
    m_updateNoteRequestIds.clear();
    m_expungeNoteRequestIds.clear();
    m_findNoteToRestoreFailedUpdateRequestIds.clear();
    m_findNoteToPerformUpdateRequestIds.clear();
    m_noteItemsPendingNotebookDataUpdate.clear();
    m_noteLocalIdToFindNotebookRequestIdForMoveNoteToNotebookBimap.clear();
    m_tagDataByTagLocalId.clear();
    m_findTagRequestForTagLocalId.clear();
    m_tagLocalIdToNoteLocalId.clear();

    endResetModel();
}

void NoteModel::resetModel()
{
    NMDEBUG("NoteModel::resetModel");

    clearModel();
    requestNotesListAndCount();
}

int NoteModel::rowForNewItem(const NoteModelItem & item) const
{
    const NoteDataByIndex & index = m_data.get<ByIndex>();

    auto it = std::lower_bound(
        index.begin(), index.end(), item,
        NoteComparator(sortingColumn(), sortOrder()));

    if (it == index.end()) {
        return static_cast<int>(index.size());
    }

    return static_cast<int>(std::distance(index.begin(), it));
}

LocalStorageManager::NoteCountOptions NoteModel::noteCountOptions() const
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    LocalStorageManager::NoteCountOptions noteCountOptions;
#else
    LocalStorageManager::NoteCountOptions noteCountOptions = 0;
#endif

    if (m_includedNotes != IncludedNotes::Deleted) {
        noteCountOptions |=
            LocalStorageManager::NoteCountOption::IncludeNonDeletedNotes;
    }

    if (m_includedNotes != IncludedNotes::NonDeleted) {
        noteCountOptions |=
            LocalStorageManager::NoteCountOption::IncludeDeletedNotes;
    }

    return noteCountOptions;
}

void NoteModel::processTagExpunging(const QString & tagLocalId)
{
    NMTRACE("NoteModel::processTagExpunging: tag local id = " << tagLocalId);

    auto tagDataIt = m_tagDataByTagLocalId.find(tagLocalId);
    if (tagDataIt == m_tagDataByTagLocalId.end()) {
        NMTRACE(
            "Tag data corresponding to the expunged tag was not found "
            << "within the note model");
        return;
    }

    QString tagGuid = tagDataIt->m_guid;
    QString tagName = tagDataIt->m_name;

    Q_UNUSED(m_tagDataByTagLocalId.erase(tagDataIt))

    auto noteIt = m_tagLocalIdToNoteLocalId.find(tagLocalId);
    if (noteIt == m_tagLocalIdToNoteLocalId.end()) {
        return;
    }

    auto & localIdIndex = m_data.get<ByLocalId>();

    QStringList affectedNotesLocalIds;
    while (noteIt != m_tagLocalIdToNoteLocalId.end()) {
        if (noteIt.key() != tagLocalId) {
            break;
        }

        affectedNotesLocalIds << noteIt.value();
        ++noteIt;
    }

    Q_UNUSED(m_tagLocalIdToNoteLocalId.remove(tagLocalId))

    NMTRACE(
        "Affected notes local ids: "
        << affectedNotesLocalIds.join(QStringLiteral(", ")));

    for (const auto & noteLocalId: qAsConst(affectedNotesLocalIds)) {
        auto noteItemIt = localIdIndex.find(noteLocalId);
        if (Q_UNLIKELY(noteItemIt == localIdIndex.end())) {
            NMDEBUG(
                "Can't find the note pointed to by the expunged "
                << "tag by local id: note local id = " << noteIt.value());
            continue;
        }

        NoteModelItem item = *noteItemIt;
        item.removeTagGuid(tagGuid);
        item.removeTagName(tagName);
        item.removeTagLocalId(tagLocalId);

        Q_UNUSED(localIdIndex.replace(noteItemIt, item))

        auto modelIndex = indexForLocalId(item.localId());
        modelIndex = createIndex(modelIndex.row(), Column::TagNameList);
        Q_EMIT dataChanged(modelIndex, modelIndex);

        // This note's cache entry is clearly stale now, need to ensure
        // it won't be present in the cache
        Q_UNUSED(m_cache.remove(item.localId()))
    }
}

void NoteModel::removeItemByLocalId(const QString & localId)
{
    NMDEBUG("NoteModel::removeItemByLocalId: " << localId);

    auto & localIdIndex = m_data.get<ByLocalId>();
    auto itemIt = localIdIndex.find(localId);
    if (Q_UNLIKELY(itemIt == localIdIndex.end())) {
        NMDEBUG("Can't find item to remove from the note model");
        return;
    }

    const auto & item = *itemIt;

    auto & index = m_data.get<ByIndex>();
    auto indexIt = m_data.project<ByIndex>(itemIt);
    if (Q_UNLIKELY(indexIt == index.end())) {
        NMWARNING(
            "Can't determine the row index for the note "
            << "model item to remove: " << item);
        return;
    }

    int row = static_cast<int>(std::distance(index.begin(), indexIt));
    if (Q_UNLIKELY((row < 0) || (row >= static_cast<int>(m_data.size())))) {
        NMWARNING(
            "Invalid row index for the note model item to remove: "
            << "index = " << row << ", item: " << item);
        return;
    }

    beginRemoveRows(QModelIndex(), row, row);
    Q_UNUSED(localIdIndex.erase(itemIt))
    endRemoveRows();
}

bool NoteModel::updateItemRowWithRespectToSorting(
    const NoteModelItem & item, ErrorString & errorDescription)
{
    NMDEBUG(
        "NoteModel::updateItemRowWithRespectToSorting: item local id = "
        << item.localId());

    auto & localIdIndex = m_data.get<ByLocalId>();
    auto localIdIt = localIdIndex.find(item.localId());
    if (Q_UNLIKELY(localIdIt == localIdIndex.end())) {
        errorDescription.setBase(
            QT_TR_NOOP("can't find appropriate position for note within "
                       "the model: can't find the note by local id"));

        NMWARNING(errorDescription << ": " << item);
        return false;
    }

    auto & index = m_data.get<ByIndex>();

    auto it = m_data.project<ByIndex>(localIdIt);
    if (Q_UNLIKELY(it == index.end())) {
        errorDescription.setBase(
            QT_TR_NOOP("can't find appropriate position for note within "
                       "the model: internal error, can't find note's current "
                       "position"));

        NMWARNING(errorDescription << ": " << item);
        return false;
    }

    int originalRow = static_cast<int>(std::distance(index.begin(), it));
    if (Q_UNLIKELY(
            (originalRow < 0) ||
            (originalRow >= static_cast<int>(m_data.size()))))
    {
        errorDescription.setBase(
            QT_TR_NOOP("can't find appropriate position for note within "
                       "the model: internal error, note's current position is "
                       "beyond the acceptable range"));

        NMWARNING(
            errorDescription << ": current row = " << originalRow
                             << ", item: " << item);
        return false;
    }

    NoteModelItem itemCopy(item);

    NMTRACE("Removing the moved item from the original row " << originalRow);
    beginRemoveRows(QModelIndex(), originalRow, originalRow);
    Q_UNUSED(index.erase(it))
    endRemoveRows();

    auto positionIter = std::lower_bound(
        index.begin(), index.end(), itemCopy,
        NoteComparator(sortingColumn(), sortOrder()));

    if (positionIter == index.end()) {
        int newRow = static_cast<int>(index.size());

        NMTRACE("Inserting the moved item at row " << newRow);
        beginInsertRows(QModelIndex(), newRow, newRow);
        index.push_back(itemCopy);
        endInsertRows();

        return true;
    }

    int newRow = static_cast<int>(std::distance(index.begin(), positionIter));

    NMTRACE("Inserting the moved item at row " << newRow);
    beginInsertRows(QModelIndex(), newRow, newRow);
    Q_UNUSED(index.insert(positionIter, itemCopy))
    endInsertRows();

    return true;
}

void NoteModel::saveNoteInLocalStorage(
    const NoteModelItem & item, const bool saveTags)
{
    NMTRACE(
        "NoteModel::saveNoteInLocalStorage: local id = "
        << item.localId()
        << ", update tags = " << (saveTags ? "true" : "false"));

    Note note;

    auto notYetSavedItemIt =
        m_localIdsOfNewNotesBeingAddedToLocalStorage.find(item.localId());

    if (notYetSavedItemIt ==
        m_localIdsOfNewNotesBeingAddedToLocalStorage.end()) {
        NMTRACE("Updating the note");

        const auto * pCachedNote = m_cache.get(item.localId());
        if (Q_UNLIKELY(!pCachedNote)) {
            auto requestId = QUuid::createUuid();
            Q_UNUSED(m_findNoteToPerformUpdateRequestIds.insert(requestId))

            Note dummy;
            dummy.setLocalId(item.localId());

            NMTRACE(
                "Emitting the request to find note: local id = "
                << item.localId() << ", request id = " << requestId);

            LocalStorageManager::GetNoteOptions getNoteOptions(
                LocalStorageManager::GetNoteOption::WithResourceMetadata);

            Q_EMIT findNote(dummy, getNoteOptions, requestId);
            return;
        }

        note = *pCachedNote;
    }

    note.setLocalId(item.localId());
    note.setGuid(item.guid());
    note.setNotebookLocalId(item.notebookLocalId());
    note.setNotebookGuid(item.notebookGuid());
    note.setCreationTimestamp(item.creationTimestamp());
    note.setModificationTimestamp(item.modificationTimestamp());
    note.setDeletionTimestamp(item.deletionTimestamp());
    note.setTagLocalIds(item.tagLocalIds());
    note.setTagGuids(item.tagGuids());
    note.setTitle(item.title());
    note.setLocal(!item.isSynchronizable());
    note.setDirty(item.isDirty());
    note.setFavorited(item.isFavorited());
    note.setActive(item.isActive());

    auto requestId = QUuid::createUuid();

    if (notYetSavedItemIt !=
        m_localIdsOfNewNotesBeingAddedToLocalStorage.end()) {
        Q_UNUSED(m_addNoteRequestIds.insert(requestId))

        Q_UNUSED(m_localIdsOfNewNotesBeingAddedToLocalStorage.erase(
            notYetSavedItemIt))

        NMTRACE(
            "Emitting the request to add the note to local storage: id = "
            << requestId << ", note: " << note);

        Q_EMIT addNote(note, requestId);
    }
    else {
        Q_UNUSED(m_updateNoteRequestIds.insert(requestId))

        // While the note is being updated in the local storage,
        // remove its stale copy from the cache
        Q_UNUSED(m_cache.remove(note.localId()))

        NMTRACE(
            "Emitting the request to update the note in local storage: "
            << "id = " << requestId << ", note: " << note);

#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
        LocalStorageManager::UpdateNoteOptions options;
#else
        LocalStorageManager::UpdateNoteOptions options(0);
#endif

        if (saveTags) {
            options |= LocalStorageManager::UpdateNoteOption::UpdateTags;
        }

        Q_EMIT updateNote(note, options, requestId);
    }
}

QVariant NoteModel::dataImpl(const int row, const Column::type column) const
{
    if (Q_UNLIKELY((row < 0) || (row >= static_cast<int>(m_data.size())))) {
        return QVariant();
    }

    const auto & index = m_data.get<ByIndex>();
    const auto & item = index[static_cast<size_t>(row)];

    switch (column) {
    case Column::CreationTimestamp:
        return item.creationTimestamp();
    case Column::ModificationTimestamp:
        return item.modificationTimestamp();
    case Column::DeletionTimestamp:
        return item.deletionTimestamp();
    case Column::Title:
        return item.title();
    case Column::PreviewText:
        return item.previewText();
    case Column::ThumbnailImage:
    {
        QImage thumbnail;
        Q_UNUSED(thumbnail.loadFromData(item.thumbnailData(), "PNG"))
        return thumbnail;
    }
    case Column::NotebookName:
        return item.notebookName();
    case Column::TagNameList:
        return item.tagNameList();
    case Column::Size:
        return item.sizeInBytes();
    case Column::Synchronizable:
        return item.isSynchronizable();
    case Column::Dirty:
        return item.isDirty();
    case Column::HasResources:
        return item.hasResources();
    default:
        return QVariant();
    }
}

QVariant NoteModel::dataAccessibleText(
    const int row, const Column::type column) const
{
    if (Q_UNLIKELY((row < 0) || (row >= static_cast<int>(m_data.size())))) {
        return QVariant();
    }

    const NoteDataByIndex & index = m_data.get<ByIndex>();
    const NoteModelItem & item = index[static_cast<size_t>(row)];

    QString space = QStringLiteral(" ");
    QString colon = QStringLiteral(":");
    QString accessibleText = tr("Note") + colon + space;

    switch (column) {
    case Column::CreationTimestamp:
    {
        if (item.creationTimestamp() < 0) {
            accessibleText += tr("creation time is not set");
        }
        else {
            accessibleText += tr("was created at") + space +
                printableDateTimeFromTimestamp(item.creationTimestamp());
        }
        break;
    }
    case Column::ModificationTimestamp:
    {
        if (item.modificationTimestamp() < 0) {
            accessibleText += tr("last modification timestamp is not set");
        }
        else {
            accessibleText += tr("was last modified at") + space +
                printableDateTimeFromTimestamp(item.modificationTimestamp());
        }
        break;
    }
    case Column::DeletionTimestamp:
    {
        if (item.deletionTimestamp() < 0) {
            accessibleText += tr("deletion timestamp is not set");
        }
        else {
            accessibleText += tr("deleted at") + space +
                printableDateTimeFromTimestamp(item.deletionTimestamp());
        }
        break;
    }
    case Column::Title:
    {
        const QString & title = item.title();
        if (title.isEmpty()) {
            accessibleText += tr("title is not set");
        }
        else {
            accessibleText += tr("title is") + space + title;
        }
        break;
    }
    case Column::PreviewText:
    {
        const QString & previewText = item.previewText();
        if (previewText.isEmpty()) {
            accessibleText += tr("preview text is not available");
        }
        else {
            accessibleText += tr("preview text") + colon + space + previewText;
        }
        break;
    }
    case Column::NotebookName:
    {
        const QString & notebookName = item.notebookName();
        if (notebookName.isEmpty()) {
            accessibleText += tr("notebook name is not available");
        }
        else {
            accessibleText += tr("notebook name is") + space + notebookName;
        }
        break;
    }
    case Column::TagNameList:
    {
        const QStringList & tagNameList = item.tagNameList();
        if (tagNameList.isEmpty()) {
            accessibleText += tr("tag list is empty");
        }
        else {
            accessibleText += tr("has tags") + colon + space +
                tagNameList.join(QStringLiteral(", "));
        }
        break;
    }
    case Column::Size:
    {
        const quint64 bytes = item.sizeInBytes();
        if (bytes == 0) {
            accessibleText += tr("size is not available");
        }
        else {
            accessibleText += tr("size is") + space + humanReadableSize(bytes);
        }
        break;
    }
    case Column::Synchronizable:
        accessibleText +=
            (item.isSynchronizable() ? tr("synchronizable")
                                     : tr("not synchronizable"));
        break;
    case Column::Dirty:
        accessibleText += (item.isDirty() ? tr("dirty") : tr("not dirty"));
        break;
    case Column::HasResources:
        accessibleText +=
            (item.hasResources() ? tr("has attachments")
                                 : tr("has no attachments"));
        break;
    case Column::ThumbnailImage:
    default:
        return QVariant();
    }

    return accessibleText;
}

bool NoteModel::setDataImpl(
    const QModelIndex & modelIndex, const QVariant & value,
    ErrorString & errorDescription)
{
    if (!modelIndex.isValid()) {
        return false;
    }

    int row = modelIndex.row();
    int column = modelIndex.column();

    if ((row < 0) || (row >= static_cast<int>(m_data.size())) || (column < 0) ||
        (column >= gNoteModelColumnCount))
    {
        return false;
    }

    auto & index = m_data.get<ByIndex>();
    auto item = index.at(static_cast<size_t>(row));

    if (!canUpdateNoteItem(item)) {
        return false;
    }

    bool dirty = item.isDirty();
    switch (column) {
    case Column::Title:
    {
        if (!item.canUpdateTitle()) {
            errorDescription.setBase(
                QT_TR_NOOP("Can't update the note title: note restrictions "
                           "apply"));

            NMINFO(errorDescription << ", item: " << item);
            return false;
        }

        QString title = value.toString();
        dirty |= (title != item.title());
        item.setTitle(title);
        break;
    }
    case Column::Synchronizable:
    {
        if (item.isSynchronizable()) {
            errorDescription.setBase(
                QT_TR_NOOP("Can't make already synchronizable note not "
                           "synchronizable"));

            NMDEBUG(errorDescription << ", item: " << item);
            return false;
        }

        dirty |= (value.toBool() != item.isSynchronizable());
        item.setSynchronizable(value.toBool());
        break;
    }
    case Column::DeletionTimestamp:
    {
        qint64 timestamp = -1;

        if (!value.isNull()) {
            bool conversionResult = false;
            timestamp = value.toLongLong(&conversionResult);

            if (!conversionResult) {
                errorDescription.setBase(
                    QT_TR_NOOP("Can't change note's deleted state: wrong "
                               "deletion timestamp value"));

                NMDEBUG(
                    errorDescription << ", item: " << item
                                     << "\nTimestamp: " << value);
                return false;
            }
        }

        dirty |= (timestamp != item.deletionTimestamp());
        item.setDeletionTimestamp(timestamp);
        bool isActive = (timestamp < 0);
        item.setActive(isActive);
        break;
    }
    case Column::CreationTimestamp:
    {
        qint64 timestamp = -1;

        if (!value.isNull()) {
            bool conversionResult = false;
            timestamp = value.toLongLong(&conversionResult);

            if (!conversionResult) {
                errorDescription.setBase(
                    QT_TR_NOOP("Can't update note's creation datetime: "
                               "wrong creation timestamp value"));

                NMDEBUG(
                    errorDescription << ", item: " << item
                                     << "\nTimestamp: " << value);
                return false;
            }
        }

        dirty |= (timestamp != item.creationTimestamp());
        item.setCreationTimestamp(timestamp);
        break;
    }
    case Column::ModificationTimestamp:
    {
        qint64 timestamp = -1;

        if (!value.isNull()) {
            bool conversionResult = false;
            timestamp = value.toLongLong(&conversionResult);

            if (!conversionResult) {
                errorDescription.setBase(
                    QT_TR_NOOP("Can't update note's modification datetime: "
                               "wrong modification timestamp value"));

                NMDEBUG(
                    errorDescription << ", item: " << item
                                     << "\nTimestamp: " << value);
                return false;
            }
        }

        dirty |= (timestamp != item.modificationTimestamp());
        item.setModificationTimestamp(timestamp);
        break;
    }
    default:
        return false;
    }

    bool dirtyFlagChanged = (item.isDirty() != dirty);
    item.setDirty(dirty);

    if ((m_includedNotes != IncludedNotes::NonDeleted) ||
        ((column != Column::DeletionTimestamp) &&
         (column != Column::CreationTimestamp)))
    {
        item.setModificationTimestamp(QDateTime::currentMSecsSinceEpoch());
    }

    index.replace(index.begin() + row, item);

    int firstColumn = column;
    if (firstColumn > Column::ModificationTimestamp) {
        firstColumn = Column::ModificationTimestamp;
    }
    if (dirtyFlagChanged && (firstColumn > Column::Dirty)) {
        firstColumn = Column::Dirty;
    }

    int lastColumn = column;
    if (lastColumn < Column::ModificationTimestamp) {
        lastColumn = Column::ModificationTimestamp;
    }
    if (dirtyFlagChanged && (lastColumn < Column::Dirty)) {
        lastColumn = Column::Dirty;
    }

    auto topLeftChangedIndex = createIndex(modelIndex.row(), firstColumn);
    auto bottomRightChangedIndex = createIndex(modelIndex.row(), lastColumn);
    Q_EMIT dataChanged(topLeftChangedIndex, bottomRightChangedIndex);

    if (!updateItemRowWithRespectToSorting(item, errorDescription)) {
        NMWARNING(
            "Failed to update note item's row with respect "
            << "to sorting: " << errorDescription << ", note item: " << item);
    }

    saveNoteInLocalStorage(item);
    return true;
}

bool NoteModel::removeRowsImpl(
    int row, int count, ErrorString & errorDescription)
{
    if (Q_UNLIKELY((row + count) > static_cast<int>(m_data.size()))) {
        errorDescription.setBase(
            QT_TR_NOOP("Detected attempt to remove more rows than the note "
                       "model contains"));
        NMWARNING(errorDescription);
        Q_EMIT notifyError(errorDescription);
        return false;
    }

    auto & index = m_data.get<ByIndex>();

    for (int i = 0; i < count; ++i) {
        auto it = index.begin() + row;
        if (!it->guid().isEmpty() && (it->deletionTimestamp() < 0)) {
            errorDescription.setBase(
                QT_TR_NOOP("Cannot remove non-deleted note which has Evernote "
                           "assigned guid"));
            NMWARNING(errorDescription);
            Q_EMIT notifyError(errorDescription);
            return false;
        }
    }

    beginRemoveRows(QModelIndex(), row, row + count - 1);

    QStringList localIdsToRemove;
    localIdsToRemove.reserve(count);
    for (int i = 0; i < count; ++i) {
        auto it = index.begin() + row + i;
        localIdsToRemove << it->localId();
    }
    Q_UNUSED(index.erase(index.begin() + row, index.begin() + row + count))

    for (const auto & noteLocalId: qAsConst(localIdsToRemove)) {
        Note note;
        note.setLocalId(noteLocalId);

        auto requestId = QUuid::createUuid();
        Q_UNUSED(m_expungeNoteRequestIds.insert(requestId))

        NMDEBUG(
            "Emitting the request to expunge the note from "
            << "the local storage: request id = " << requestId
            << ", note local id: " << noteLocalId);

        Q_EMIT expungeNote(note, requestId);
    }

    endRemoveRows();

    return true;
}

bool NoteModel::canUpdateNoteItem(const NoteModelItem & item) const
{
    auto it = m_notebookDataByNotebookLocalId.find(item.notebookLocalId());
    if (it == m_notebookDataByNotebookLocalId.end()) {
        NMDEBUG(
            "Can't find the notebook data for note with local id "
            << item.localId());
        return false;
    }

    const auto & notebookData = it.value();
    return notebookData.m_canUpdateNotes;
}

bool NoteModel::canCreateNoteItem(const QString & notebookLocalId) const
{
    if (notebookLocalId.isEmpty()) {
        NMDEBUG("NoteModel::canCreateNoteItem: empty notebook local id");
        return false;
    }

    auto it = m_notebookDataByNotebookLocalId.find(notebookLocalId);
    if (it != m_notebookDataByNotebookLocalId.end()) {
        return it->m_canCreateNotes;
    }

    NMDEBUG(
        "Can't find the notebook data for notebook local id "
        << notebookLocalId);

    return false;
}

void NoteModel::updateNotebookData(const Notebook & notebook)
{
    NMTRACE(
        "NoteModel::updateNotebookData: local id = " << notebook.localId());

    auto & notebookData = m_notebookDataByNotebookLocalId[notebook.localId()];

    if (!notebook.hasRestrictions()) {
        notebookData.m_canCreateNotes = true;
        notebookData.m_canUpdateNotes = true;
    }
    else {
        const auto & notebookRestrictions = notebook.restrictions();

        notebookData.m_canCreateNotes =
            (notebookRestrictions.noCreateNotes.isSet()
                 ? (!notebookRestrictions.noCreateNotes.ref())
                 : true);

        notebookData.m_canUpdateNotes =
            (notebookRestrictions.noUpdateNotes.isSet()
                 ? (!notebookRestrictions.noUpdateNotes.ref())
                 : true);
    }

    if (notebook.hasName()) {
        notebookData.m_name = notebook.name();
    }

    if (notebook.hasGuid()) {
        notebookData.m_guid = notebook.guid();
    }

    NMTRACE(
        "Collected notebook data from notebook with local id "
        << notebook.localId() << ": guid = " << notebookData.m_guid
        << "; name = " << notebookData.m_name << ": can create notes = "
        << (notebookData.m_canCreateNotes ? "true" : "false")
        << ": can update notes = "
        << (notebookData.m_canUpdateNotes ? "true" : "false"));

    checkAddedNoteItemsPendingNotebookData(notebook.localId(), notebookData);
}

bool NoteModel::setNoteFavorited(
    const QString & noteLocalId, const bool favorited,
    ErrorString & errorDescription)
{
    auto & localIdIndex = m_data.get<ByLocalId>();
    auto it = localIdIndex.find(noteLocalId);
    if (Q_UNLIKELY(it == localIdIndex.end())) {
        errorDescription.setBase(
            QT_TR_NOOP("internal error, the note to be favorited/unfavorited "
                       "was not found within the model"));
        NMWARNING(errorDescription);
        return false;
    }

    const auto & item = *it;

    if (favorited == item.isFavorited()) {
        NMDEBUG("Favorited flag's value hasn't changed");
        return true;
    }

    NoteModelItem itemCopy(item);
    itemCopy.setFavorited(favorited);

    localIdIndex.replace(it, itemCopy);
    saveNoteInLocalStorage(itemCopy);

    return true;
}

void NoteModel::setSortingColumnAndOrder(
    const int column, const Qt::SortOrder order)
{
    switch (column) {
    case Column::CreationTimestamp:
        if (order == Qt::AscendingOrder) {
            m_noteSortingMode = NoteSortingMode::CreatedAscending;
        }
        else {
            m_noteSortingMode = NoteSortingMode::CreatedDescending;
        }
        break;
    case Column::Title:
        if (order == Qt::AscendingOrder) {
            m_noteSortingMode = NoteSortingMode::TitleAscending;
        }
        else {
            m_noteSortingMode = NoteSortingMode::TitleDescending;
        }
        break;
    case Column::Size:
        if (order == Qt::AscendingOrder) {
            m_noteSortingMode = NoteSortingMode::SizeAscending;
        }
        else {
            m_noteSortingMode = NoteSortingMode::SizeDescending;
        }
        break;
    default:
        if (order == Qt::AscendingOrder) {
            m_noteSortingMode = NoteSortingMode::ModifiedAscending;
        }
        else {
            m_noteSortingMode = NoteSortingMode::ModifiedDescending;
        }
        break;
    }
}

void NoteModel::setSortingOrder(const Qt::SortOrder order)
{
    switch (m_noteSortingMode) {
    case NoteSortingMode::CreatedAscending:
        if (order == Qt::DescendingOrder) {
            m_noteSortingMode = NoteSortingMode::CreatedDescending;
        }
        return;
    case NoteSortingMode::ModifiedAscending:
        if (order == Qt::DescendingOrder) {
            m_noteSortingMode = NoteSortingMode::ModifiedDescending;
        }
        return;
    case NoteSortingMode::TitleAscending:
        if (order == Qt::DescendingOrder) {
            m_noteSortingMode = NoteSortingMode::TitleDescending;
        }
        return;
    case NoteSortingMode::SizeAscending:
        if (order == Qt::DescendingOrder) {
            m_noteSortingMode = NoteSortingMode::SizeDescending;
        }
        return;
    case NoteSortingMode::CreatedDescending:
        if (order == Qt::AscendingOrder) {
            m_noteSortingMode = NoteSortingMode::CreatedAscending;
        }
        return;
    case NoteSortingMode::ModifiedDescending:
        if (order == Qt::AscendingOrder) {
            m_noteSortingMode = NoteSortingMode::ModifiedAscending;
        }
        return;
    case NoteSortingMode::TitleDescending:
        if (order == Qt::AscendingOrder) {
            m_noteSortingMode = NoteSortingMode::TitleAscending;
        }
        return;
    case NoteSortingMode::SizeDescending:
        if (order == Qt::AscendingOrder) {
            m_noteSortingMode = NoteSortingMode::SizeAscending;
        }
        return;
    default:
        return;
    }
}

// WARNING: this method assumes the iterator passed to it is not end()
bool NoteModel::moveNoteToNotebookImpl(
    NoteDataByLocalId::iterator it, const Notebook & notebook,
    ErrorString & errorDescription)
{
    auto item = *it;

    NMTRACE(
        "NoteModel::moveNoteToNotebookImpl: notebook = "
        << notebook << ", note item: " << item);

    if (Q_UNLIKELY(item.notebookLocalId() == notebook.localId())) {
        NMDEBUG(
            "The note is already within its target notebook, nothing to "
            << "do");
        return true;
    }

    if (!notebook.canCreateNotes()) {
        errorDescription.setBase(
            QT_TR_NOOP("the target notebook doesn't allow to create notes in "
                       "it"));
        NMINFO(errorDescription << ", notebook: " << notebook);
        return false;
    }

    item.setNotebookLocalId(notebook.localId());
    item.setNotebookName(notebook.hasName() ? notebook.name() : QString());
    item.setNotebookGuid(notebook.hasGuid() ? notebook.guid() : QString());

    item.setDirty(true);
    item.setModificationTimestamp(QDateTime::currentMSecsSinceEpoch());

    auto itemIndex = indexForLocalId(item.localId());

    itemIndex =
        index(itemIndex.row(), Column::NotebookName, itemIndex.parent());

    auto & localIdIndex = m_data.get<ByLocalId>();
    localIdIndex.replace(it, item);
    Q_EMIT dataChanged(itemIndex, itemIndex);

    // NOTE: deliberately ignoring the returned result as it's too late
    // to undo the change anyway
    Q_UNUSED(updateItemRowWithRespectToSorting(*it, errorDescription))

    saveNoteInLocalStorage(item);
    return true;
}

void NoteModel::addOrUpdateNoteItem(
    NoteModelItem & item, const NotebookData & notebookData,
    const bool fromNotesListing)
{
    NMTRACE(
        "NoteModel::addOrUpdateNoteItem: note local id = "
        << item.localId()
        << ", notebook local id = " << item.notebookLocalId()
        << ", notebook name = " << notebookData.m_name
        << ", from notes listing = " << (fromNotesListing ? "true" : "false"));

    item.setNotebookName(notebookData.m_name);

    auto & localIdIndex = m_data.get<ByLocalId>();
    auto it = localIdIndex.find(item.localId());
    if (it == localIdIndex.end()) {
        switch (m_includedNotes) {
        case IncludedNotes::All:
            break;
        case IncludedNotes::Deleted:
        {
            if (item.deletionTimestamp() < 0) {
                NMTRACE(
                    "Won't add note as it's not deleted and the model "
                    << "instance only considers the deleted notes");
                return;
            }
            break;
        }
        case IncludedNotes::NonDeleted:
        {
            if (item.deletionTimestamp() >= 0) {
                NMDEBUG(
                    "Won't add note as it's deleted and the model "
                    << "instance only considers the non-deleted notes");
                return;
            }
            break;
        }
        }

        NMDEBUG("Adding new item to the note model");

        if (fromNotesListing) {
            findTagNamesForItem(item);

            int row = static_cast<int>(localIdIndex.size());
            beginInsertRows(QModelIndex(), row, row);
            Q_UNUSED(localIdIndex.insert(item))
            endInsertRows();

            ErrorString errorDescription;
            if (!updateItemRowWithRespectToSorting(item, errorDescription)) {
                NMWARNING(
                    "Could not update new note model item's row: "
                    << errorDescription << "; item: " << item);
            }

            checkMaxNoteCountAndRemoveLastNoteIfNeeded();
            return;
        }

        auto & index = m_data.get<ByIndex>();

        auto positionIter = std::lower_bound(
            index.begin(), index.end(), item,
            NoteComparator(sortingColumn(), sortOrder()));

        int newRow =
            static_cast<int>(std::distance(index.begin(), positionIter));

        if (newRow >= static_cast<int>(m_maxNoteCount)) {
            NMDEBUG(
                "Skip adding note: new row "
                << newRow << " is larger than or equal to the max note count "
                << m_maxNoteCount);
            return;
        }

        NMTRACE("Inserting new item at row " << newRow);
        beginInsertRows(QModelIndex(), newRow, newRow);
        Q_UNUSED(index.insert(positionIter, item))
        endInsertRows();

        checkMaxNoteCountAndRemoveLastNoteIfNeeded();
        findTagNamesForItem(item);
    }
    else {
        bool shouldRemoveItem = false;

        switch (m_includedNotes) {
        case IncludedNotes::All:
            break;
        case IncludedNotes::Deleted:
        {
            if (item.deletionTimestamp() < 0) {
                NMDEBUG(
                    "Removing the updated note item from the model as "
                    << "the item is not deleted and the model instance "
                    << "only considers the deleted notes");
                shouldRemoveItem = true;
            }
            break;
        }
        case IncludedNotes::NonDeleted:
        {
            if (item.deletionTimestamp() >= 0) {
                NMDEBUG(
                    "Removing the updated note item from the model as "
                    << "the item is deleted and the model instance only "
                    << "considers the non-deleted notes");
                shouldRemoveItem = true;
            }
            break;
        }
        }

        auto & index = m_data.get<ByIndex>();
        auto indexIt = m_data.project<ByIndex>(it);
        if (Q_UNLIKELY(indexIt == index.end())) {
            REPORT_ERROR(
                QT_TR_NOOP("Internal error: can't project the local id "
                           "index iterator to the random access index "
                           "iterator in note model"));
            return;
        }

        int row = static_cast<int>(std::distance(index.begin(), indexIt));

        if (shouldRemoveItem) {
            beginRemoveRows(QModelIndex(), row, row);
            Q_UNUSED(localIdIndex.erase(it))
            endRemoveRows();
        }
        else {
            auto modelIndexFrom = createIndex(row, Column::CreationTimestamp);
            auto modelIndexTo = createIndex(row, Column::HasResources);
            Q_UNUSED(localIdIndex.replace(it, item))
            Q_EMIT dataChanged(modelIndexFrom, modelIndexTo);

            ErrorString errorDescription;
            if (!updateItemRowWithRespectToSorting(item, errorDescription)) {
                NMWARNING(
                    "Could not update note model item's row: "
                    << errorDescription << "; item: " << item);
            }

            findTagNamesForItem(item);
        }
    }
}

void NoteModel::checkMaxNoteCountAndRemoveLastNoteIfNeeded()
{
    NMDEBUG("NoteModel::checkMaxNoteCountAndRemoveLastNoteIfNeeded");

    auto & localIdIndex = m_data.get<ByLocalId>();
    size_t indexSize = localIdIndex.size();
    if (indexSize <= m_maxNoteCount) {
        return;
    }

    NMDEBUG(
        "Note model's size is outside the acceptable range, "
        << "removing the last row's note to keep the cache minimal");

    int lastRow = static_cast<int>(indexSize - 1);
    auto & index = m_data.get<ByIndex>();
    auto indexIt = index.begin();
    std::advance(indexIt, lastRow);
    auto it = m_data.project<ByLocalId>(indexIt);
    if (Q_UNLIKELY(it == localIdIndex.end())) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't project the random access index "
                       "iterator to the local id index iterator in note "
                       "model"));
    }
    else {
        beginRemoveRows(QModelIndex(), lastRow, lastRow);
        Q_UNUSED(localIdIndex.erase(it))
        endRemoveRows();
    }
}

void NoteModel::checkAddedNoteItemsPendingNotebookData(
    const QString & notebookLocalId, const NotebookData & notebookData)
{
    auto it = m_noteItemsPendingNotebookDataUpdate.find(notebookLocalId);
    while (it != m_noteItemsPendingNotebookDataUpdate.end()) {
        if (it.key() != notebookLocalId) {
            break;
        }

        addOrUpdateNoteItem(it.value(), notebookData, true);
        it = m_noteItemsPendingNotebookDataUpdate.erase(it);
    }
}

void NoteModel::findTagNamesForItem(NoteModelItem & item)
{
    NMTRACE("NoteModel::findTagNamesForItem: " << item);

    const auto & tagLocalIds = item.tagLocalIds();
    for (const auto & tagLocalId: qAsConst(tagLocalIds)) {
        bool alreadyGotNoteLocalIdMapped = false;

        auto tagToNoteIt = m_tagLocalIdToNoteLocalId.find(tagLocalId);
        while (tagToNoteIt != m_tagLocalIdToNoteLocalId.end()) {
            if (tagToNoteIt.key() != tagLocalId) {
                break;
            }

            if (tagToNoteIt.value() == item.localId()) {
                alreadyGotNoteLocalIdMapped = true;
                break;
            }

            ++tagToNoteIt;
        }

        if (!alreadyGotNoteLocalIdMapped) {
            Q_UNUSED(m_tagLocalIdToNoteLocalId.insert(
                tagLocalId, item.localId()))

            NMDEBUG(
                "Tag local id " << tagLocalId << " points to note model item "
                                 << item.localId()
                                 << ", title = " << item.title());
        }

        auto tagDataIt = m_tagDataByTagLocalId.find(tagLocalId);
        if (tagDataIt != m_tagDataByTagLocalId.end()) {
            NMTRACE(
                "Found tag data for tag local id "
                << tagLocalId << ": tag name = " << tagDataIt->m_name);
            item.addTagName(tagDataIt->m_name);
            continue;
        }

        NMTRACE(
            "Tag data for tag local id " << tagLocalId << " was not found");

        auto requestIt = m_findTagRequestForTagLocalId.left.find(tagLocalId);
        if (requestIt != m_findTagRequestForTagLocalId.left.end()) {
            NMTRACE(
                "The request to find tag corresponding to local id "
                << tagLocalId << " has already been sent: request id = "
                << requestIt->second);
            continue;
        }

        auto requestId = QUuid::createUuid();
        Q_UNUSED(m_findTagRequestForTagLocalId.insert(
            LocalIdToRequestIdBimap::value_type(tagLocalId, requestId)))

        Tag tag;
        tag.setLocalId(tagLocalId);

        NMDEBUG(
            "Emitting the request to find tag: tag local id = "
            << tagLocalId << ", request id = " << requestId);

        Q_EMIT findTag(tag, requestId);
    }
}

void NoteModel::updateTagData(const Tag & tag)
{
    NMTRACE("NoteModel::updateTagData: tag local id = " << tag.localId());

    bool hasName = tag.hasName();
    bool hasGuid = tag.hasGuid();

    auto & tagData = m_tagDataByTagLocalId[tag.localId()];

    if (hasName) {
        tagData.m_name = tag.name();
    }
    else {
        tagData.m_name.resize(0);
    }

    if (hasGuid) {
        tagData.m_guid = tag.guid();
    }
    else {
        tagData.m_guid.resize(0);
    }

    auto noteIt = m_tagLocalIdToNoteLocalId.find(tag.localId());
    if (noteIt == m_tagLocalIdToNoteLocalId.end()) {
        return;
    }

    auto & localIdIndex = m_data.get<ByLocalId>();

    QStringList affectedNotesLocalIds;
    while (noteIt != m_tagLocalIdToNoteLocalId.end()) {
        if (noteIt.key() != tag.localId()) {
            break;
        }

        affectedNotesLocalIds << noteIt.value();
        ++noteIt;
    }

    NMTRACE(
        "Affected notes local ids: "
        << affectedNotesLocalIds.join(QStringLiteral(", ")));

    for (const auto & noteLocalId: qAsConst(affectedNotesLocalIds)) {
        auto noteItemIt = localIdIndex.find(noteLocalId);
        if (Q_UNLIKELY(noteItemIt == localIdIndex.end())) {
            NMDEBUG(
                "Can't find the note pointed to by a tag by "
                << "local id: note local id = " << noteLocalId);
            continue;
        }

        NoteModelItem item = *noteItemIt;
        auto tagLocalIds = item.tagLocalIds();

        // Need to refresh all the tag names and guids because it is generally
        // unknown which particular tag was updated
        item.setTagNameList(QStringList());
        item.setTagGuids(QStringList());

        for (auto tagLocalIdIt = tagLocalIds.begin(),
                  tagLocalIdEnd = tagLocalIds.end();
             tagLocalIdIt != tagLocalIdEnd; ++tagLocalIdIt)
        {
            auto tagDataIt = m_tagDataByTagLocalId.find(*tagLocalIdIt);
            if (tagDataIt == m_tagDataByTagLocalId.end()) {
                NMTRACE(
                    "Still no tag data for tag with local id "
                    << *tagLocalIdIt);
                continue;
            }

            const auto & tagData = tagDataIt.value();

            if (!tagData.m_name.isEmpty()) {
                item.addTagName(tagData.m_name);
            }

            if (!tagData.m_guid.isEmpty()) {
                item.addTagGuid(tagData.m_guid);
            }
        }

        Q_UNUSED(localIdIndex.replace(noteItemIt, item))

        auto modelIndex = indexForLocalId(item.localId());
        modelIndex = createIndex(modelIndex.row(), Column::TagNameList);
        Q_EMIT dataChanged(modelIndex, modelIndex);
    }
}

bool NoteModel::NoteFilters::isEmpty() const
{
    return m_filteredNotebookLocalIds.isEmpty() &&
        m_filteredTagLocalIds.isEmpty() && m_filteredNoteLocalIds.isEmpty();
}

const QStringList & NoteModel::NoteFilters::filteredNotebookLocalIds() const
{
    return m_filteredNotebookLocalIds;
}

void NoteModel::NoteFilters::setFilteredNotebookLocalIds(
    const QStringList & notebookLocalIds)
{
    m_filteredNotebookLocalIds = notebookLocalIds;
}

void NoteModel::NoteFilters::clearFilteredNotebookLocalIds()
{
    m_filteredNotebookLocalIds.clear();
}

const QStringList & NoteModel::NoteFilters::filteredTagLocalIds() const
{
    return m_filteredTagLocalIds;
}

void NoteModel::NoteFilters::setFilteredTagLocalIds(
    const QStringList & tagLocalIds)
{
    m_filteredTagLocalIds = tagLocalIds;
}

void NoteModel::NoteFilters::clearFilteredTagLocalIds()
{
    m_filteredTagLocalIds.clear();
}

const QSet<QString> & NoteModel::NoteFilters::filteredNoteLocalIds() const
{
    return m_filteredNoteLocalIds;
}

bool NoteModel::NoteFilters::setFilteredNoteLocalIds(
    const QSet<QString> & noteLocalIds)
{
    if (m_filteredNoteLocalIds == noteLocalIds) {
        return false;
    }

    m_filteredNoteLocalIds = noteLocalIds;
    return true;
}

bool NoteModel::NoteFilters::setFilteredNoteLocalIds(
    const QStringList & noteLocalIds)
{
    return setFilteredNoteLocalIds(
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
        QSet<QString>(noteLocalIds.constBegin(), noteLocalIds.constEnd()));
#else
        QSet<QString>::fromList(noteLocalIds));
#endif
}

void NoteModel::NoteFilters::clearFilteredNoteLocalIds()
{
    m_filteredNoteLocalIds.clear();
}

bool NoteModel::NoteComparator::operator()(
    const NoteModelItem & lhs, const NoteModelItem & rhs) const
{
    bool less = false;
    bool greater = false;

    switch (m_sortedColumn) {
    case Column::CreationTimestamp:
        less = (lhs.creationTimestamp() < rhs.creationTimestamp());
        greater = (lhs.creationTimestamp() > rhs.creationTimestamp());
        break;
    case Column::ModificationTimestamp:
        less = (lhs.modificationTimestamp() < rhs.modificationTimestamp());
        greater = (lhs.modificationTimestamp() > rhs.modificationTimestamp());
        break;
    case Column::DeletionTimestamp:
        less = (lhs.deletionTimestamp() < rhs.deletionTimestamp());
        greater = (lhs.deletionTimestamp() > rhs.deletionTimestamp());
        break;
    case Column::Title:
    {
        QString leftTitleOrPreview = lhs.title();
        if (leftTitleOrPreview.isEmpty()) {
            leftTitleOrPreview = lhs.previewText();
        }

        QString rightTitleOrPreview = rhs.title();
        if (rightTitleOrPreview.isEmpty()) {
            rightTitleOrPreview = rhs.previewText();
        }

        int compareResult =
            leftTitleOrPreview.localeAwareCompare(rightTitleOrPreview);
        less = (compareResult < 0);
        greater = (compareResult > 0);
        break;
    }
    case Column::PreviewText:
    {
        QString leftTitleOrPreview = lhs.title();
        if (leftTitleOrPreview.isEmpty()) {
            leftTitleOrPreview = lhs.previewText();
        }

        QString rightTitleOrPreview = rhs.title();
        if (rightTitleOrPreview.isEmpty()) {
            rightTitleOrPreview = rhs.previewText();
        }

        int compareResult =
            leftTitleOrPreview.localeAwareCompare(rightTitleOrPreview);
        less = (compareResult < 0);
        greater = (compareResult > 0);
        break;
    }
    case Column::NotebookName:
    {
        int compareResult =
            lhs.notebookName().localeAwareCompare(rhs.notebookName());
        less = (compareResult < 0);
        greater = (compareResult > 0);
        break;
    }
    case Column::Size:
        less = (lhs.sizeInBytes() < rhs.sizeInBytes());
        greater = (lhs.sizeInBytes() > rhs.sizeInBytes());
        break;
    case Column::Synchronizable:
        less = (!lhs.isSynchronizable() && rhs.isSynchronizable());
        greater = (lhs.isSynchronizable() && !rhs.isSynchronizable());
        break;
    case Column::Dirty:
        less = (!lhs.isDirty() && rhs.isDirty());
        greater = (lhs.isDirty() && !rhs.isDirty());
        break;
    case Column::HasResources:
        less = (!lhs.hasResources() && rhs.hasResources());
        greater = (lhs.hasResources() && !rhs.hasResources());
        break;
    case Column::ThumbnailImage:
    case Column::TagNameList:
        less = false;
        greater = false;
        break;
    }

    if (m_sortOrder == Qt::AscendingOrder) {
        return less;
    }
    else {
        return greater;
    }
}

} // namespace quentier
