/*
 * Copyright 2016 Dmitry Ivanov
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
#include <quentier/logging/QuentierLogger.h>
#include <quentier/utility/UidGenerator.h>
#include <quentier/utility/Utility.h>
#include <quentier/types/Resource.h>
#include <QDateTime>
#include <iterator>

// Separate logging macros for the note model - to distinguish the one
// for deleted notes from the one for non-deleted notes

inline QString includedNotesStr(const quentier::NoteModel::IncludedNotes::type includedNotes)
{
    if (includedNotes == quentier::NoteModel::IncludedNotes::All) {
        return QStringLiteral("[all notes] ");
    }

    if (includedNotes == quentier::NoteModel::IncludedNotes::NonDeleted) {
        return QStringLiteral("[non-deleted notes] ");
    }

    return QStringLiteral("[deleted notes] ");
}

#define NMTRACE(message) \
    QNTRACE(includedNotesStr(m_includedNotes) << message)

#define NMDEBUG(message) \
    QNDEBUG(includedNotesStr(m_includedNotes) << message)

#define NMINFO(message) \
    QNINFO(includedNotesStr(m_includedNotes) << message)

#define NMWARNING(message) \
    QNWARNING(includedNotesStr(m_includedNotes) << message)

#define NMCRITICAL(message) \
    QNCRITICAL(includedNotesStr(m_includedNotes) << message)

#define NMFATAL(message) \
    QNFATAL(includedNotesStr(m_includedNotes) << message)

// Limit for the queries to the local storage
#define NOTE_LIST_LIMIT (100)

#define NOTE_PREVIEW_TEXT_SIZE (500)

#define NUM_NOTE_MODEL_COLUMNS (12)

#define REPORT_ERROR(error, ...) \
    ErrorString errorDescription(error); \
    NMWARNING(errorDescription << QStringLiteral("" __VA_ARGS__ "")); \
    Q_EMIT notifyError(errorDescription)

namespace quentier {

NoteModel::NoteModel(const Account & account, LocalStorageManagerAsync & localStorageManagerAsync,
                     NoteCache & noteCache, NotebookCache & notebookCache, QObject * parent,
                     const IncludedNotes::type includedNotes) :
    QAbstractItemModel(parent),
    m_account(account),
    m_includedNotes(includedNotes),
    m_data(),
    m_listNotesOffset(0),
    m_listNotesRequestId(),
    m_noteItemsNotYetInLocalStorageUids(),
    m_cache(noteCache),
    m_notebookCache(notebookCache),
    m_numberOfNotesPerAccount(0),
    m_addNoteRequestIds(),
    m_updateNoteRequestIds(),
    m_expungeNoteRequestIds(),
    m_findNoteToRestoreFailedUpdateRequestIds(),
    m_findNoteToPerformUpdateRequestIds(),
    m_sortedColumn(Columns::ModificationTimestamp),
    m_sortOrder(Qt::AscendingOrder),
    m_notebookDataByNotebookLocalUid(),
    m_findNotebookRequestForNotebookLocalUid(),
    m_noteItemsPendingNotebookDataUpdate(),
    m_noteLocalUidToFindNotebookRequestIdForMoveNoteToNotebookBimap(),
    m_tagDataByTagLocalUid(),
    m_findTagRequestForTagLocalUid(),
    m_tagLocalUidToNoteLocalUid(),
    m_allNotesListed(false)
{
    createConnections(localStorageManagerAsync);
    requestNotesList();
}

NoteModel::~NoteModel()
{}

void NoteModel::updateAccount(const Account & account)
{
    NMDEBUG(QStringLiteral("NoteModel::updateAccount: ") << account);
    m_account = account;
}

QModelIndex NoteModel::indexForLocalUid(const QString & localUid) const
{
    const NoteDataByLocalUid & localUidIndex = m_data.get<ByLocalUid>();
    auto it = localUidIndex.find(localUid);
    if (Q_UNLIKELY(it == localUidIndex.end())) {
        NMDEBUG(QStringLiteral("Can't find the note item by local uid: ") << localUid);
        return QModelIndex();
    }

    const NoteDataByIndex & index = m_data.get<ByIndex>();
    auto indexIt = m_data.project<ByIndex>(it);
    if (Q_UNLIKELY(indexIt == index.end())) {
        NMWARNING(QStringLiteral("Can't find the indexed reference to the note item: ") << *it);
        return QModelIndex();
    }

    int rowIndex = static_cast<int>(std::distance(index.begin(), indexIt));
    return createIndex(rowIndex, Columns::Title);
}

const NoteModelItem * NoteModel::itemForLocalUid(const QString & localUid) const
{
    const NoteDataByLocalUid & localUidIndex = m_data.get<ByLocalUid>();
    auto it = localUidIndex.find(localUid);
    if (Q_UNLIKELY(it == localUidIndex.end())) {
        NMDEBUG(QStringLiteral("Can't find the note item by local uid"));
        return Q_NULLPTR;
    }

    return &(*it);
}

const NoteModelItem * NoteModel::itemAtRow(const int row) const
{
    const NoteDataByIndex & index = m_data.get<ByIndex>();
    if (Q_UNLIKELY((row < 0) || (index.size() <= static_cast<size_t>(row)))) {
        NMDEBUG(QStringLiteral("Detected attempt to get the note model item for non-existing row"));
        return Q_NULLPTR;
    }

    return &(index[static_cast<size_t>(row)]);
}

const NoteModelItem * NoteModel::itemForIndex(const QModelIndex & index) const
{
    if (!index.isValid()) {
        return Q_NULLPTR;
    }

    if (index.parent().isValid()) {
        return Q_NULLPTR;
    }

    return itemAtRow(index.row());
}

QModelIndex NoteModel::createNoteItem(const QString & notebookLocalUid)
{
    if (Q_UNLIKELY((m_includedNotes != IncludedNotes::Deleted) &&
                   (m_numberOfNotesPerAccount >= m_account.noteCountMax())))
    {
        ErrorString error(QT_TR_NOOP("Can't create a new note: the account already contains "
                                     "the max allowed number of notes"));
        error.details() = QString::number(m_account.noteCountMax());
        NMINFO(error);
        Q_EMIT notifyError(error);
        return QModelIndex();
    }

    auto notebookIt = m_notebookDataByNotebookLocalUid.find(notebookLocalUid);
    if (notebookIt == m_notebookDataByNotebookLocalUid.end()) {
        REPORT_ERROR(QT_TR_NOOP("Can't create a new note: internal error, can't identify the notebook "
                                "in which the note needs to be created"));
        return QModelIndex();
    }

    const NotebookData & notebookData = notebookIt.value();
    if (!notebookData.m_canCreateNotes) {
        REPORT_ERROR(QT_TR_NOOP("Can't create a new note: notebook restrictions apply"));
        return QModelIndex();
    }

    NoteModelItem item;
    item.setLocalUid(UidGenerator::Generate());
    item.setNotebookLocalUid(notebookLocalUid);
    item.setNotebookGuid(notebookData.m_guid);
    item.setNotebookName(notebookData.m_name);
    item.setCreationTimestamp(QDateTime::currentMSecsSinceEpoch());
    item.setModificationTimestamp(item.creationTimestamp());
    item.setDirty(true);
    item.setSynchronizable(m_account.type() != Account::Type::Local);

    int row = rowForNewItem(item);
    beginInsertRows(QModelIndex(), row, row);

    NoteDataByIndex & index = m_data.get<ByIndex>();
    NoteDataByIndex::iterator indexIt = index.begin() + row;
    Q_UNUSED(index.insert(indexIt, item))

    endInsertRows();

    Q_UNUSED(m_noteItemsNotYetInLocalStorageUids.insert(item.localUid()))
    updateNoteInLocalStorage(item);

    return createIndex(row, Columns::Title);
}

bool NoteModel::deleteNote(const QString & noteLocalUid)
{
    NMDEBUG(QStringLiteral("NoteModel::deleteNote: ") << noteLocalUid);

    QModelIndex itemIndex = indexForLocalUid(noteLocalUid);
    if (!itemIndex.isValid()) {
        NMDEBUG(QStringLiteral("Index for this local uid is invalid"));
        return false;
    }

    itemIndex = index(itemIndex.row(), Columns::DeletionTimestamp, itemIndex.parent());
    qint64 timestamp = QDateTime::currentMSecsSinceEpoch();
    return setData(itemIndex, timestamp);
}

void NoteModel::moveNoteToNotebook(const QString & noteLocalUid, const QString & notebookName)
{
    NMDEBUG(QStringLiteral("NoteModel::moveNoteToNotebook: note local uid = ") << noteLocalUid
            << QStringLiteral(", notebook name = ") << notebookName);

    if (Q_UNLIKELY(notebookName.isEmpty())) {
        REPORT_ERROR(QT_TR_NOOP("Can't move the note to another notebook: the name of the target notebook is empty"));
        return;
    }

    NoteDataByLocalUid & localUidIndex = m_data.get<ByLocalUid>();
    auto it = localUidIndex.find(noteLocalUid);
    if (Q_UNLIKELY(it == localUidIndex.end())) {
        REPORT_ERROR(QT_TR_NOOP("Can't move the note to another notebook: internal error, can't find note item "
                                "within the note model by local uid"));
        return;
    }

    // 1) First try to find the notebook in the cache; the cache is indexed by local uid, not by name
    // so need to do the linear search. It should be OK since the cache is intended to be small

    for(auto nit = m_notebookCache.begin(), end = m_notebookCache.end(); nit != end; ++nit)
    {
        const Notebook & notebook = nit->second;
        if (notebook.hasName() && (notebook.name() == notebookName)) {
            moveNoteToNotebookImpl(it, notebook);
            return;
        }
    }

    // 2) No such notebook in the cache; attempt to find it within the local storage asynchronously then
    Notebook dummy;
    dummy.setName(notebookName);
    dummy.setLocalUid(QString());   // Set empty local uid as a hint for local storage to search the notebook by name
    QUuid requestId = QUuid::createUuid();
    Q_UNUSED(m_noteLocalUidToFindNotebookRequestIdForMoveNoteToNotebookBimap.insert(LocalUidToRequestIdBimap::value_type(noteLocalUid, requestId)))
    NMTRACE(QStringLiteral("Emitting the request to find a notebook by name for moving the note to it: request id = ")
            << requestId << QStringLiteral(", notebook name = ") << notebookName << QStringLiteral(", note local uid = ")
            << noteLocalUid);
    Q_EMIT findNotebook(dummy, requestId);
}

void NoteModel::favoriteNote(const QString & noteLocalUid)
{
    NMDEBUG(QStringLiteral("NoteModel::favoriteNote: local uid = ") << noteLocalUid);
    setNoteFavorited(noteLocalUid, true);
}

void NoteModel::unfavoriteNote(const QString & noteLocalUid)
{
    NMDEBUG(QStringLiteral("NoteModel::unfavoriteNote: local uid = ") << noteLocalUid);
    setNoteFavorited(noteLocalUid, false);
}

Qt::ItemFlags NoteModel::flags(const QModelIndex & modelIndex) const
{
    Qt::ItemFlags indexFlags = QAbstractItemModel::flags(modelIndex);
    if (!modelIndex.isValid()) {
        return indexFlags;
    }

    int row = modelIndex.row();
    int column = modelIndex.column();

    if ((row < 0) || (row >= static_cast<int>(m_data.size())) ||
        (column < 0) || (column >= NUM_NOTE_MODEL_COLUMNS))
    {
        return indexFlags;
    }

    indexFlags |= Qt::ItemIsSelectable;
    indexFlags |= Qt::ItemIsEnabled;

    if ((column == Columns::Dirty) ||
        (column == Columns::Size) ||
        (column == Columns::Synchronizable) ||
        (column == Columns::HasResources))

    {
        return indexFlags;
    }

    const NoteDataByIndex & index = m_data.get<ByIndex>();
    const NoteModelItem & item = index.at(static_cast<size_t>(row));
    if (!canUpdateNoteItem(item)) {
        return indexFlags;
    }

    indexFlags |= Qt::ItemIsEditable;
    return indexFlags;
}

QVariant NoteModel::data(const QModelIndex & index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    int rowIndex = index.row();
    int columnIndex = index.column();

    if ((rowIndex < 0) || (rowIndex >= static_cast<int>(m_data.size())) ||
        (columnIndex < 0) || (columnIndex >= NUM_NOTE_MODEL_COLUMNS))
    {
        return QVariant();
    }

    if (role == Qt::ToolTipRole) {
        return dataImpl(rowIndex, Columns::Title);
    }

    Columns::type column;
    switch(columnIndex)
    {
    case Columns::CreationTimestamp:
    case Columns::ModificationTimestamp:
    case Columns::DeletionTimestamp:
    case Columns::Title:
    case Columns::PreviewText:
    case Columns::ThumbnailImage:
    case Columns::NotebookName:
    case Columns::TagNameList:
    case Columns::Size:
    case Columns::Synchronizable:
    case Columns::Dirty:
    case Columns::HasResources:
        column = static_cast<Columns::type>(columnIndex);
        break;
    default:
        return QVariant();
    }

    switch(role)
    {
    case Qt::DisplayRole:
    case Qt::EditRole:
    case Qt::ToolTipRole:
        return dataImpl(rowIndex, column);
    case Qt::AccessibleTextRole:
    case Qt::AccessibleDescriptionRole:
        return dataAccessibleText(rowIndex, column);
    default:
        return QVariant();
    }
}

QVariant NoteModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole) {
        return QVariant();
    }

    if (orientation == Qt::Vertical) {
        return QVariant(section + 1);
    }

    switch(section)
    {
    case Columns::CreationTimestamp:
        // TRANSLATOR: note's creation timestamp
        return QVariant(tr("Created"));
    case Columns::ModificationTimestamp:
        // TRANSLATOR: note's modification timestamp
        return QVariant(tr("Modified"));
    case Columns::DeletionTimestamp:
        // TRANSLATOR: note's deletion timestamp
        return QVariant(tr("Deleted"));
    case Columns::Title:
        return QVariant(tr("Title"));
    case Columns::PreviewText:
        // TRANSLATOR: a short excerpt of note's text
        return QVariant(tr("Preview"));
    case Columns::NotebookName:
        return QVariant(tr("Notebook"));
    case Columns::TagNameList:
        // TRANSLATOR: the list of note's tags
        return QVariant(tr("Tags"));
    case Columns::Size:
        // TRANSLATOR: size of note in bytes
        return QVariant(tr("Size"));
    case Columns::Synchronizable:
        return QVariant(tr("Synchronizable"));
    case Columns::Dirty:
        return QVariant(tr("Dirty"));
    case Columns::HasResources:
        return QVariant(tr("Has attachments"));
    // NOTE: intentional fall-through
    case Columns::ThumbnailImage:
    default:
        return QVariant();
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

    return NUM_NOTE_MODEL_COLUMNS;
}

QModelIndex NoteModel::index(int row, int column, const QModelIndex & parent) const
{
    if (parent.isValid()) {
        return QModelIndex();
    }

    if ((row < 0) || (row >= static_cast<int>(m_data.size())) ||
        (column < 0) || (column >= NUM_NOTE_MODEL_COLUMNS))
    {
        return QModelIndex();
    }

    return createIndex(row, column);
}

QModelIndex NoteModel::parent(const QModelIndex & index) const
{
    Q_UNUSED(index)
    return QModelIndex();
}

bool NoteModel::setHeaderData(int section, Qt::Orientation orientation, const QVariant & value, int role)
{
    Q_UNUSED(section)
    Q_UNUSED(orientation)
    Q_UNUSED(value)
    Q_UNUSED(role)
    return false;
}

bool NoteModel::setData(const QModelIndex & modelIndex, const QVariant & value, int role)
{
    if (!modelIndex.isValid()) {
        return false;
    }

    if (role != Qt::EditRole) {
        return false;
    }

    int row = modelIndex.row();
    int column = modelIndex.column();

    if ((row < 0) || (row >= static_cast<int>(m_data.size())) ||
        (column < 0) || (column >= NUM_NOTE_MODEL_COLUMNS))
    {
        return false;
    }

    NoteDataByIndex & index = m_data.get<ByIndex>();
    NoteModelItem item = index.at(static_cast<size_t>(row));

    if (!canUpdateNoteItem(item)) {
        return false;
    }

    bool dirty = item.isDirty();
    switch(column)
    {
    case Columns::Title:
        {
            if (!item.canUpdateTitle()) {
                REPORT_ERROR(QT_TR_NOOP("Can't update the note title: note restrictions apply"));
                return false;
            }

            QString title = value.toString();
            dirty |= (title != item.title());
            item.setTitle(title);
            break;
        }
    case Columns::Synchronizable:
        {
            if (item.isSynchronizable()) {
                REPORT_ERROR(QT_TR_NOOP("Can't make already synchronizable note not synchronizable"));
                return false;
            }

            dirty |= (value.toBool() != item.isSynchronizable());
            item.setSynchronizable(value.toBool());
            break;
        }
    case Columns::DeletionTimestamp:
        {
            qint64 timestamp = -1;

            if (!value.isNull())
            {
                bool conversionResult = false;
                timestamp = value.toLongLong(&conversionResult);

                if (!conversionResult) {
                    REPORT_ERROR(QT_TR_NOOP("Can't change the note's deleted state: "
                                            "wrong deletion timestamp value"));
                    return false;
                }
            }

            dirty |= (timestamp != item.deletionTimestamp());
            item.setDeletionTimestamp(timestamp);
            break;
        }
    case Columns::CreationTimestamp:
        {
            qint64 timestamp = -1;

            if (!value.isNull())
            {
                bool conversionResult = false;
                timestamp = value.toLongLong(&conversionResult);

                if (!conversionResult) {
                    REPORT_ERROR(QT_TR_NOOP("Can't update the note's creation datetime: wrong creation timestamp value"));
                    return false;
                }
            }

            dirty |= (timestamp != item.creationTimestamp());
            item.setCreationTimestamp(timestamp);
            break;
        }
    case Columns::ModificationTimestamp:
        {
            qint64 timestamp = -1;

            if (!value.isNull())
            {
                bool conversionResult = false;
                timestamp = value.toLongLong(&conversionResult);

                if (!conversionResult) {
                    REPORT_ERROR(QT_TR_NOOP("Can't update the note's modification datetime: wrong modification timestamp value"));
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

    if ( (m_includedNotes != IncludedNotes::NonDeleted) ||
         ((column != Columns::DeletionTimestamp) && (column != Columns::CreationTimestamp)) )
    {
        item.setModificationTimestamp(QDateTime::currentMSecsSinceEpoch());
    }

    index.replace(index.begin() + row, item);

    int firstColumn = column;
    if (firstColumn > Columns::ModificationTimestamp) {
        firstColumn = Columns::ModificationTimestamp;
    }
    if (dirtyFlagChanged && (firstColumn > Columns::Dirty)) {
        firstColumn = Columns::Dirty;
    }

    int lastColumn = column;
    if (lastColumn < Columns::ModificationTimestamp) {
        lastColumn = Columns::ModificationTimestamp;
    }
    if (dirtyFlagChanged && (lastColumn < Columns::Dirty)) {
        lastColumn = Columns::Dirty;
    }

    QModelIndex topLeftChangedIndex = createIndex(modelIndex.row(), firstColumn);
    QModelIndex bottomRightChangedIndex = createIndex(modelIndex.row(), lastColumn);
    Q_EMIT dataChanged(topLeftChangedIndex, bottomRightChangedIndex);

    updateItemRowWithRespectToSorting(item);
    updateNoteInLocalStorage(item);
    return true;
}

bool NoteModel::insertRows(int row, int count, const QModelIndex & parent)
{
    // NOTE: NoteModel's own API is used to add rows
    Q_UNUSED(row)
    Q_UNUSED(count)
    Q_UNUSED(parent)
    return false;
}

bool NoteModel::removeRows(int row, int count, const QModelIndex & parent)
{
    if (Q_UNLIKELY(parent.isValid())) {
        NMDEBUG(QStringLiteral("Ignoring the attempt to remove rows from note model for valid parent model index"));
        return false;
    }

    if (Q_UNLIKELY((row + count) > static_cast<int>(m_data.size()))) {
        REPORT_ERROR(QT_TR_NOOP("Detected attempt to remove more rows than the note model contains"));
        return false;
    }

    NoteDataByIndex & index = m_data.get<ByIndex>();

    for(int i = 0; i < count; ++i)
    {
        auto it = index.begin() + row;
        if (it->isSynchronizable()) {
            REPORT_ERROR(QT_TR_NOOP("Can't remove the synchronizable note"));
            return false;
        }
    }

    beginRemoveRows(QModelIndex(), row, row + count - 1);

    QStringList localUidsToRemove;
    localUidsToRemove.reserve(count);
    for(int i = 0; i < count; ++i) {
        auto it = index.begin() + row + i;
        localUidsToRemove << it->localUid();
    }
    Q_UNUSED(index.erase(index.begin() + row, index.begin() + row + count))

    for(auto it = localUidsToRemove.begin(), end = localUidsToRemove.end(); it != end; ++it)
    {
        Note note;
        note.setLocalUid(*it);

        QUuid requestId = QUuid::createUuid();
        Q_UNUSED(m_expungeNoteRequestIds.insert(requestId))
        NMTRACE(QStringLiteral("Emitting the request to expunge the note from the local storage: request id = ")
                << requestId << QStringLiteral(", note local uid: ") << *it);
        Q_EMIT expungeNote(note, requestId);
    }

    endRemoveRows();

    return true;
}

void NoteModel::sort(int column, Qt::SortOrder order)
{
    NMDEBUG(QStringLiteral("NoteModel::sort: column = ") << column << QStringLiteral(", order = ") << order
            << QStringLiteral(" (") << (order == Qt::AscendingOrder ? QStringLiteral("ascending") : QStringLiteral("descending"))
            << QStringLiteral(")"));

    if ( (column == Columns::ThumbnailImage) ||
         (column == Columns::TagNameList) )
    {
        // Should not sort by these columns
        return;
    }

    if (Q_UNLIKELY((column < 0) || (column >= NUM_NOTE_MODEL_COLUMNS))) {
        return;
    }

    NoteDataByIndex & index = m_data.get<ByIndex>();

    if (column == m_sortedColumn)
    {
        if (order == m_sortOrder) {
            NMDEBUG(QStringLiteral("Neither sorted column nor sort order have changed, nothing to do"));
            return;
        }

        m_sortOrder = order;

        NMDEBUG(QStringLiteral("Only the sort order has changed, reversing the index"));

        Q_EMIT layoutAboutToBeChanged();
        index.reverse();
        Q_EMIT layoutChanged();

        return;
    }

    m_sortedColumn = static_cast<Columns::type>(column);
    m_sortOrder = order;

    Q_EMIT layoutAboutToBeChanged();

    QModelIndexList persistentIndices = persistentIndexList();
    QVector<std::pair<QString, int> > localUidsToUpdateWithColumns;
    localUidsToUpdateWithColumns.reserve(persistentIndices.size());

    QStringList localUidsToUpdate;
    for(auto it = persistentIndices.begin(), end = persistentIndices.end(); it != end; ++it)
    {
        const QModelIndex & modelIndex = *it;
        int column = modelIndex.column();

        if (!modelIndex.isValid()) {
            localUidsToUpdateWithColumns << std::pair<QString, int>(QString(), column);
            continue;
        }

        int row = modelIndex.row();

        if ((row < 0) || (row >= static_cast<int>(m_data.size())) ||
            (column < 0) || (column >= NUM_NOTE_MODEL_COLUMNS))
        {
            localUidsToUpdateWithColumns << std::pair<QString, int>(QString(), column);
        }

        auto itemIt = index.begin() + row;
        const NoteModelItem & item = *itemIt;
        localUidsToUpdateWithColumns << std::pair<QString, int>(item.localUid(), column);
    }

    std::vector<boost::reference_wrapper<const NoteModelItem> > items(index.begin(), index.end());
    std::sort(items.begin(), items.end(), NoteComparator(m_sortedColumn, m_sortOrder));
    index.rearrange(items.begin());

    QModelIndexList replacementIndices;
    replacementIndices.reserve(std::max(localUidsToUpdateWithColumns.size(), 0));
    for(auto it = localUidsToUpdateWithColumns.begin(), end = localUidsToUpdateWithColumns.end(); it != end; ++it)
    {
        const QString & localUid = it->first;
        const int column = it->second;

        if (localUid.isEmpty()) {
            replacementIndices << QModelIndex();
            continue;
        }

        QModelIndex newIndex = indexForLocalUid(localUid);
        if (!newIndex.isValid()) {
            replacementIndices << QModelIndex();
            continue;
        }

        QModelIndex newIndexWithColumn = createIndex(newIndex.row(), column);
        replacementIndices << newIndexWithColumn;
    }

    changePersistentIndexList(persistentIndices, replacementIndices);

    Q_EMIT layoutChanged();
}

void NoteModel::onAddNoteComplete(Note note, QUuid requestId)
{
    NMDEBUG(QStringLiteral("NoteModel::onAddNoteComplete: ") << note << QStringLiteral("\nRequest id = ") << requestId);

    ++m_numberOfNotesPerAccount;

    auto it = m_addNoteRequestIds.find(requestId);
    if (it != m_addNoteRequestIds.end()) {
        Q_UNUSED(m_addNoteRequestIds.erase(it))
        return;
    }

    onNoteAddedOrUpdated(note);
}

void NoteModel::onAddNoteFailed(Note note, ErrorString errorDescription, QUuid requestId)
{
    auto it = m_addNoteRequestIds.find(requestId);
    if (it == m_addNoteRequestIds.end()) {
        return;
    }

    NMDEBUG(QStringLiteral("NoteModel::onAddNoteFailed: note = ") << note << QStringLiteral("\nError description = ")
            << errorDescription << QStringLiteral(", request id = ") << requestId);

    Q_UNUSED(m_addNoteRequestIds.erase(it))

    Q_EMIT notifyError(errorDescription);
    removeItemByLocalUid(note.localUid());
}

void NoteModel::onUpdateNoteComplete(Note note, bool updateResources, bool updateTags, QUuid requestId)
{
    NMDEBUG(QStringLiteral("NoteModel::onUpdateNoteComplete: note = ") << note << QStringLiteral("\nRequest id = ") << requestId);

    Q_UNUSED(updateResources)
    Q_UNUSED(updateTags)

    bool shouldRemoveNoteFromModel = (note.hasDeletionTimestamp() && (m_includedNotes == IncludedNotes::NonDeleted));
    shouldRemoveNoteFromModel |= (!note.hasDeletionTimestamp() && (m_includedNotes == IncludedNotes::Deleted));

    if (shouldRemoveNoteFromModel) {
        removeItemByLocalUid(note.localUid());
    }

    auto it = m_updateNoteRequestIds.find(requestId);
    if (it != m_updateNoteRequestIds.end())
    {
        NMDEBUG(QStringLiteral("This update was initiated by the note model"));
        Q_UNUSED(m_updateNoteRequestIds.erase(it))

        const NoteDataByLocalUid & localUidIndex = m_data.get<ByLocalUid>();
        auto itemIt = localUidIndex.find(note.localUid());
        if (itemIt != localUidIndex.end()) {
            const NoteModelItem & item = *itemIt;
            note.setTagLocalUids(item.tagLocalUids());
            note.setTagGuids(item.tagGuids());
            NMDEBUG(QStringLiteral("Complemented the note with tag local uids and guids: ") << note);
        }

        m_cache.put(note.localUid(), note);

        return;
    }

    NMDEBUG(QStringLiteral("This update was not initiated by the note model: ") << note
            << QStringLiteral(", request id = ") << requestId << QStringLiteral(", update tags = ")
            << (updateTags ? QStringLiteral("true") : QStringLiteral("false"))
            << QStringLiteral(", should remove note from model = ")
            << (shouldRemoveNoteFromModel ? QStringLiteral("true") : QStringLiteral("false")));

    if (!shouldRemoveNoteFromModel)
    {
        if (!updateTags)
        {
            const NoteDataByLocalUid & localUidIndex = m_data.get<ByLocalUid>();
            auto noteItemIt = localUidIndex.find(note.localUid());
            if (noteItemIt != localUidIndex.end()) {
                const NoteModelItem & item = *noteItemIt;
                note.setTagGuids(item.tagGuids());
                note.setTagLocalUids(item.tagLocalUids());
                NMDEBUG(QStringLiteral("Complemented the note with tag local uids and guids: ") << note);
            }
        }

        onNoteAddedOrUpdated(note);
    }
}

void NoteModel::onUpdateNoteFailed(Note note, bool updateResources, bool updateTags,
                                   ErrorString errorDescription, QUuid requestId)
{
    Q_UNUSED(updateResources)
    Q_UNUSED(updateTags)

    auto it = m_updateNoteRequestIds.find(requestId);
    if (it == m_updateNoteRequestIds.end()) {
        return;
    }

    NMDEBUG(QStringLiteral("NoteModel::onUpdateNoteFailed: note = ") << note << QStringLiteral("\nError description = ")
            << errorDescription << QStringLiteral(", request id = ") << requestId);

    Q_UNUSED(m_updateNoteRequestIds.erase(it))

    requestId = QUuid::createUuid();
    Q_UNUSED(m_findNoteToRestoreFailedUpdateRequestIds.insert(requestId))
    NMTRACE(QStringLiteral("Emitting the request to find a note: local uid = ") << note.localUid()
            << QStringLiteral(", request id = ") << requestId);
    Q_EMIT findNote(note, /* with resource binary data = */ false, requestId);
}

void NoteModel::onFindNoteComplete(Note note, bool withResourceBinaryData, QUuid requestId)
{
    Q_UNUSED(withResourceBinaryData)

    auto restoreUpdateIt = m_findNoteToRestoreFailedUpdateRequestIds.find(requestId);
    auto performUpdateIt = m_findNoteToPerformUpdateRequestIds.find(requestId);

    if ((restoreUpdateIt == m_findNoteToRestoreFailedUpdateRequestIds.end()) &&
        (performUpdateIt == m_findNoteToPerformUpdateRequestIds.end()))
    {
        return;
    }

    NMDEBUG(QStringLiteral("NoteModel::onFindNoteComplete: note = ") << note << QStringLiteral("\nRequest id = ") << requestId);

    if (restoreUpdateIt != m_findNoteToRestoreFailedUpdateRequestIds.end())
    {
        Q_UNUSED(m_findNoteToRestoreFailedUpdateRequestIds.erase(restoreUpdateIt))
        onNoteAddedOrUpdated(note);
    }
    else if (performUpdateIt != m_findNoteToPerformUpdateRequestIds.end())
    {
        Q_UNUSED(m_findNoteToPerformUpdateRequestIds.erase(performUpdateIt))
        m_cache.put(note.localUid(), note);

        NoteDataByLocalUid & localUidIndex = m_data.get<ByLocalUid>();
        auto it = localUidIndex.find(note.localUid());
        if (it != localUidIndex.end()) {
            updateNoteInLocalStorage(*it);
        }
    }
}

void NoteModel::onFindNoteFailed(Note note, bool withResourceBinaryData, ErrorString errorDescription, QUuid requestId)
{
    Q_UNUSED(withResourceBinaryData)

    auto restoreUpdateIt = m_findNoteToRestoreFailedUpdateRequestIds.find(requestId);
    auto performUpdateIt = m_findNoteToPerformUpdateRequestIds.find(requestId);
    if ((restoreUpdateIt == m_findNoteToRestoreFailedUpdateRequestIds.end()) &&
        (performUpdateIt == m_findNoteToPerformUpdateRequestIds.end()))
    {
        return;
    }

    NMDEBUG(QStringLiteral("NoteModel::onFindNoteFailed: note = ") << note << QStringLiteral("\nError description = ")
            << errorDescription << QStringLiteral(", request id = ") << requestId);

    if (restoreUpdateIt != m_findNoteToRestoreFailedUpdateRequestIds.end()) {
        Q_UNUSED(m_findNoteToRestoreFailedUpdateRequestIds.erase(restoreUpdateIt))
    }
    else if (performUpdateIt != m_findNoteToPerformUpdateRequestIds.end()) {
        Q_UNUSED(m_findNoteToPerformUpdateRequestIds.erase(performUpdateIt))
    }

    Q_EMIT notifyError(errorDescription);
}

void NoteModel::onListNotesComplete(LocalStorageManager::ListObjectsOptions flag, bool withResourceBinaryData,
                                    size_t limit, size_t offset, LocalStorageManager::ListNotesOrder::type order,
                                    LocalStorageManager::OrderDirection::type orderDirection,
                                    QString linkedNotebookGuid, QList<Note> foundNotes, QUuid requestId)
{
    if (requestId != m_listNotesRequestId) {
        return;
    }

    NMDEBUG(QStringLiteral("NoteModel::onListNotesComplete: flag = ") << flag << QStringLiteral(", with resource binary data = ")
            << (withResourceBinaryData ? QStringLiteral("true") : QStringLiteral("false")) << QStringLiteral(", limit = ") << limit
            << QStringLiteral(", offset = ") << offset << QStringLiteral(", order = ") << order << QStringLiteral(", direction = ")
            << orderDirection << QStringLiteral(", linked notebook guid = ") << linkedNotebookGuid << QStringLiteral(", num found notes = ")
            << foundNotes.size() << QStringLiteral(", request id = ") << requestId);

    for(auto it = foundNotes.begin(), end = foundNotes.end(); it != end; ++it) {
        ++m_numberOfNotesPerAccount;
        onNoteAddedOrUpdated(*it);
    }

    m_listNotesRequestId = QUuid();

    if (!foundNotes.isEmpty()) {
        NMTRACE(QStringLiteral("The number of found notes is greater than zero, requesting more notes from the local storage"));
        m_listNotesOffset += static_cast<size_t>(foundNotes.size());
        requestNotesList();
        return;
    }

    checkAndNotifyAllNotesListed();
}

void NoteModel::onListNotesFailed(LocalStorageManager::ListObjectsOptions flag, bool withResourceBinaryData,
                                  size_t limit, size_t offset, LocalStorageManager::ListNotesOrder::type order,
                                  LocalStorageManager::OrderDirection::type orderDirection,
                                  QString linkedNotebookGuid, ErrorString errorDescription, QUuid requestId)
{
    if (requestId != m_listNotesRequestId) {
        return;
    }

    NMDEBUG(QStringLiteral("NoteModel::onListNotesFailed: flag = ") << flag << QStringLiteral(", with resource binary data = ")
            << (withResourceBinaryData ? QStringLiteral("true") : QStringLiteral("false")) << QStringLiteral(", limit = ")
            << limit << QStringLiteral(", offset = ") << offset << QStringLiteral(", order = ") << order << QStringLiteral(", direction = ")
            << orderDirection << QStringLiteral(", linked notebook guid = ") << linkedNotebookGuid
            << QStringLiteral(", error description = ") << errorDescription << QStringLiteral(", request id = ") << requestId);

    m_listNotesRequestId = QUuid();
    Q_EMIT notifyError(errorDescription);
}

void NoteModel::onExpungeNoteComplete(Note note, QUuid requestId)
{
    NMDEBUG(QStringLiteral("NoteModel::onExpungeNoteComplete: note = ") << note << QStringLiteral("\nRequest id = ") << requestId);

    --m_numberOfNotesPerAccount;

    auto it = m_expungeNoteRequestIds.find(requestId);
    if (it != m_expungeNoteRequestIds.end()) {
        Q_UNUSED(m_expungeNoteRequestIds.erase(it))
        return;
    }

    removeItemByLocalUid(note.localUid());
}

void NoteModel::onExpungeNoteFailed(Note note, ErrorString errorDescription, QUuid requestId)
{
    auto it = m_expungeNoteRequestIds.find(requestId);
    if (it == m_expungeNoteRequestIds.end()) {
        return;
    }

    NMDEBUG(QStringLiteral("NoteModel::onExpungeNoteFailed: note = ") << note << QStringLiteral("\nError description = ")
            << errorDescription << QStringLiteral(", request id = ") << requestId);

    Q_UNUSED(m_expungeNoteRequestIds.erase(it))

    onNoteAddedOrUpdated(note);
}

void NoteModel::onFindNotebookComplete(Notebook notebook, QUuid requestId)
{
    auto fit = m_findNotebookRequestForNotebookLocalUid.right.find(requestId);
    auto mit = ((fit == m_findNotebookRequestForNotebookLocalUid.right.end())
                ? m_noteLocalUidToFindNotebookRequestIdForMoveNoteToNotebookBimap.right.end()
                : m_noteLocalUidToFindNotebookRequestIdForMoveNoteToNotebookBimap.right.find(requestId));

    if ( (fit == m_findNotebookRequestForNotebookLocalUid.right.end()) &&
         (mit == m_noteLocalUidToFindNotebookRequestIdForMoveNoteToNotebookBimap.right.end()) )
    {
        return;
    }

    NMDEBUG(QStringLiteral("NoteModel::onFindNotebookComplete: notebook: ") << notebook << QStringLiteral("\nRequest id = ")
            << requestId);

    m_notebookCache.put(notebook.localUid(), notebook);

    if (fit != m_findNotebookRequestForNotebookLocalUid.right.end())
    {
        Q_UNUSED(m_findNotebookRequestForNotebookLocalUid.right.erase(fit))
        updateNotebookData(notebook);
        checkAndNotifyAllNotesListed();
    }
    else if (mit != m_noteLocalUidToFindNotebookRequestIdForMoveNoteToNotebookBimap.right.end())
    {
        QString noteLocalUid = mit->second;
        Q_UNUSED(m_noteLocalUidToFindNotebookRequestIdForMoveNoteToNotebookBimap.right.erase(mit))

        NoteDataByLocalUid & localUidIndex = m_data.get<ByLocalUid>();
        auto it = localUidIndex.find(noteLocalUid);
        if (it == localUidIndex.end()) {
            REPORT_ERROR(QT_TR_NOOP("Can't move the note to another notebook: internal error, can't find the item "
                                    "within the note model by local uid"));
            return;
        }

        moveNoteToNotebookImpl(it, notebook);
    }
}

void NoteModel::onFindNotebookFailed(Notebook notebook, ErrorString errorDescription, QUuid requestId)
{
    auto fit = m_findNotebookRequestForNotebookLocalUid.right.find(requestId);
    auto mit = ((fit == m_findNotebookRequestForNotebookLocalUid.right.end())
                ? m_noteLocalUidToFindNotebookRequestIdForMoveNoteToNotebookBimap.right.end()
                : m_noteLocalUidToFindNotebookRequestIdForMoveNoteToNotebookBimap.right.find(requestId));

    if ( (fit == m_findNotebookRequestForNotebookLocalUid.right.end()) &&
         (mit == m_noteLocalUidToFindNotebookRequestIdForMoveNoteToNotebookBimap.right.end()) )
    {
        return;
    }

    NMWARNING(QStringLiteral("NoteModel::onFindNotebookFailed: notebook = ") << notebook << QStringLiteral("\nError description = ")
              << errorDescription << QStringLiteral(", request id = ") << requestId);

    if (fit != m_findNotebookRequestForNotebookLocalUid.right.end())
    {
        Q_UNUSED(m_findNotebookRequestForNotebookLocalUid.right.erase(fit))
        checkAndNotifyAllNotesListed();
    }
    else if (mit != m_noteLocalUidToFindNotebookRequestIdForMoveNoteToNotebookBimap.right.end())
    {
        Q_UNUSED(m_noteLocalUidToFindNotebookRequestIdForMoveNoteToNotebookBimap.right.erase(mit))

        ErrorString error(QT_TR_NOOP("Can't move the note to another notebook: failed to find the target notebook"));
        error.appendBase(errorDescription.base());
        error.appendBase(errorDescription.additionalBases());
        error.details() = errorDescription.details();
        NMDEBUG(error);
        Q_EMIT notifyError(error);
    }
}

void NoteModel::onAddNotebookComplete(Notebook notebook, QUuid requestId)
{
    NMDEBUG(QStringLiteral("NoteModel::onAddNotebookComplete: local uid = ") << notebook.localUid());
    Q_UNUSED(requestId)
    m_notebookCache.put(notebook.localUid(), notebook);
    updateNotebookData(notebook);
}

void NoteModel::onUpdateNotebookComplete(Notebook notebook, QUuid requestId)
{
    NMDEBUG(QStringLiteral("NoteModel::onUpdateNotebookComplete: local uid = ") << notebook.localUid());
    Q_UNUSED(requestId)
    m_notebookCache.put(notebook.localUid(), notebook);
    updateNotebookData(notebook);
}

void NoteModel::onExpungeNotebookComplete(Notebook notebook, QUuid requestId)
{
    NMDEBUG(QStringLiteral("NoteModel::onExpungeNotebookComplete: local uid = ") << notebook.localUid());

    Q_UNUSED(requestId)
    Q_UNUSED(m_notebookCache.remove(notebook.localUid()))

    auto it = m_notebookDataByNotebookLocalUid.find(notebook.localUid());
    if (it != m_notebookDataByNotebookLocalUid.end()) {
        Q_UNUSED(m_notebookDataByNotebookLocalUid.erase(it))
    }
}

void NoteModel::onFindTagComplete(Tag tag, QUuid requestId)
{
    auto it = m_findTagRequestForTagLocalUid.right.find(requestId);
    if (it == m_findTagRequestForTagLocalUid.right.end()) {
        return;
    }

    NMDEBUG(QStringLiteral("NoteModel::onFindTagComplete: tag: ") << tag << QStringLiteral("\nRequest id = ") << requestId);

    Q_UNUSED(m_findTagRequestForTagLocalUid.right.erase(it))
    updateTagData(tag);

    checkAndNotifyAllNotesListed();
}

void NoteModel::onFindTagFailed(Tag tag, ErrorString errorDescription, QUuid requestId)
{
    auto it = m_findTagRequestForTagLocalUid.right.find(requestId);
    if (it == m_findTagRequestForTagLocalUid.right.end()) {
        return;
    }

    NMWARNING(QStringLiteral("NoteModel::onFindTagFailed: tag: ") << tag << QStringLiteral("\nError description = ")
              << errorDescription << QStringLiteral(", request id = ") << requestId);

    Q_UNUSED(m_findTagRequestForTagLocalUid.right.erase(it))
    Q_EMIT notifyError(errorDescription);
    checkAndNotifyAllNotesListed();
}

void NoteModel::onAddTagComplete(Tag tag, QUuid requestId)
{
    NMDEBUG(QStringLiteral("NoteModel::onAddTagComplete: tag = ") << tag << QStringLiteral(", request id = ") << requestId);
    updateTagData(tag);
}

void NoteModel::onUpdateTagComplete(Tag tag, QUuid requestId)
{
    NMDEBUG(QStringLiteral("NoteModel::onUpdateTagComplete: tag = ") << tag << QStringLiteral(", request id = ") << requestId);
    updateTagData(tag);
}

void NoteModel::onExpungeTagComplete(Tag tag, QStringList expungedChildTagLocalUids, QUuid requestId)
{
    NMDEBUG(QStringLiteral("NoteModel::onExpungeTagComplete: tag = ") << tag
            << QStringLiteral("\nExpunged child tag local uids = ") << expungedChildTagLocalUids.join(QStringLiteral(", "))
            << QStringLiteral(", request id = ") << requestId);

    QStringList expungedTagLocalUids;
    expungedTagLocalUids << tag.localUid();
    expungedTagLocalUids << expungedChildTagLocalUids;
    for(auto it = expungedTagLocalUids.constBegin(), end = expungedTagLocalUids.constEnd(); it != end; ++it) {
        processTagExpunging(*it);
    }
}

void NoteModel::createConnections(LocalStorageManagerAsync & localStorageManagerAsync)
{
    NMDEBUG(QStringLiteral("NoteModel::createConnections"));

    // Local signals to localStorageManagerAsync's slots
    QObject::connect(this, QNSIGNAL(NoteModel,addNote,Note,QUuid),
                     &localStorageManagerAsync, QNSLOT(LocalStorageManagerAsync,onAddNoteRequest,Note,QUuid));
    QObject::connect(this, QNSIGNAL(NoteModel,updateNote,Note,bool,bool,QUuid),
                     &localStorageManagerAsync, QNSLOT(LocalStorageManagerAsync,onUpdateNoteRequest,Note,bool,bool,QUuid));
    QObject::connect(this, QNSIGNAL(NoteModel,findNote,Note,bool,QUuid),
                     &localStorageManagerAsync, QNSLOT(LocalStorageManagerAsync,onFindNoteRequest,Note,bool,QUuid));
    QObject::connect(this, QNSIGNAL(NoteModel,listNotes,LocalStorageManager::ListObjectsOptions,bool,size_t,size_t,
                                    LocalStorageManager::ListNotesOrder::type,LocalStorageManager::OrderDirection::type,QString,QUuid),
                     &localStorageManagerAsync, QNSLOT(LocalStorageManagerAsync,onListNotesRequest,
                                                       LocalStorageManager::ListObjectsOptions,bool,size_t,size_t,
                                                       LocalStorageManager::ListNotesOrder::type,
                                                       LocalStorageManager::OrderDirection::type,QString,QUuid));
    QObject::connect(this, QNSIGNAL(NoteModel,expungeNote,Note,QUuid),
                     &localStorageManagerAsync, QNSLOT(LocalStorageManagerAsync,onExpungeNoteRequest,Note,QUuid));
    QObject::connect(this, QNSIGNAL(NoteModel,findNotebook,Notebook,QUuid),
                     &localStorageManagerAsync, QNSLOT(LocalStorageManagerAsync,onFindNotebookRequest,Notebook,QUuid));
    QObject::connect(this, QNSIGNAL(NoteModel,findTag,Tag,QUuid),
                     &localStorageManagerAsync, QNSLOT(LocalStorageManagerAsync,onFindTagRequest,Tag,QUuid));

    // localStorageManagerAsync's signals to local slots
    QObject::connect(&localStorageManagerAsync, QNSIGNAL(LocalStorageManagerAsync,addNoteComplete,Note,QUuid),
                     this, QNSLOT(NoteModel,onAddNoteComplete,Note,QUuid));
    QObject::connect(&localStorageManagerAsync, QNSIGNAL(LocalStorageManagerAsync,addNoteFailed,Note,ErrorString,QUuid),
                     this, QNSLOT(NoteModel,onAddNoteFailed,Note,ErrorString,QUuid));
    QObject::connect(&localStorageManagerAsync, QNSIGNAL(LocalStorageManagerAsync,updateNoteComplete,Note,bool,bool,QUuid),
                     this, QNSLOT(NoteModel,onUpdateNoteComplete,Note,bool,bool,QUuid));
    QObject::connect(&localStorageManagerAsync, QNSIGNAL(LocalStorageManagerAsync,updateNoteFailed,Note,bool,bool,ErrorString,QUuid),
                     this, QNSLOT(NoteModel,onUpdateNoteFailed,Note,bool,bool,ErrorString,QUuid));
    QObject::connect(&localStorageManagerAsync, QNSIGNAL(LocalStorageManagerAsync,findNoteComplete,Note,bool,QUuid),
                     this, QNSLOT(NoteModel,onFindNoteComplete,Note,bool,QUuid));
    QObject::connect(&localStorageManagerAsync, QNSIGNAL(LocalStorageManagerAsync,findNoteFailed,Note,bool,ErrorString,QUuid),
                     this, QNSLOT(NoteModel,onFindNoteFailed,Note,bool,ErrorString,QUuid));
    QObject::connect(&localStorageManagerAsync, QNSIGNAL(LocalStorageManagerAsync,listNotesComplete,LocalStorageManager::ListObjectsOptions,bool,size_t,size_t,
                                                         LocalStorageManager::ListNotesOrder::type,LocalStorageManager::OrderDirection::type,
                                                         QString,QList<Note>,QUuid),
                     this, QNSLOT(NoteModel,onListNotesComplete,LocalStorageManager::ListObjectsOptions,bool,size_t,size_t,
                                  LocalStorageManager::ListNotesOrder::type,LocalStorageManager::OrderDirection::type,QString,QList<Note>,QUuid));
    QObject::connect(&localStorageManagerAsync, QNSIGNAL(LocalStorageManagerAsync,listNotesFailed,LocalStorageManager::ListObjectsOptions,bool,size_t,size_t,
                                                         LocalStorageManager::ListNotesOrder::type,LocalStorageManager::OrderDirection::type,QString,ErrorString,QUuid),
                     this, QNSLOT(NoteModel,onListNotesFailed,LocalStorageManager::ListObjectsOptions,bool,size_t,size_t,
                                  LocalStorageManager::ListNotesOrder::type,LocalStorageManager::OrderDirection::type,QString,ErrorString,QUuid));
    QObject::connect(&localStorageManagerAsync, QNSIGNAL(LocalStorageManagerAsync,expungeNoteComplete,Note,QUuid),
                     this, QNSLOT(NoteModel,onExpungeNoteComplete,Note,QUuid));
    QObject::connect(&localStorageManagerAsync, QNSIGNAL(LocalStorageManagerAsync,expungeNoteFailed,Note,ErrorString,QUuid),
                     this, QNSLOT(NoteModel,onExpungeNoteFailed,Note,ErrorString,QUuid));
    QObject::connect(&localStorageManagerAsync, QNSIGNAL(LocalStorageManagerAsync,findNotebookComplete,Notebook,QUuid),
                     this, QNSLOT(NoteModel,onFindNotebookComplete,Notebook,QUuid));
    QObject::connect(&localStorageManagerAsync, QNSIGNAL(LocalStorageManagerAsync,findNotebookFailed,Notebook,ErrorString,QUuid),
                     this, QNSLOT(NoteModel,onFindNotebookFailed,Notebook,ErrorString,QUuid));
    QObject::connect(&localStorageManagerAsync, QNSIGNAL(LocalStorageManagerAsync,addNotebookComplete,Notebook,QUuid),
                     this, QNSLOT(NoteModel,onAddNotebookComplete,Notebook,QUuid));
    QObject::connect(&localStorageManagerAsync, QNSIGNAL(LocalStorageManagerAsync,updateNotebookComplete,Notebook,QUuid),
                     this, QNSLOT(NoteModel,onUpdateNotebookComplete,Notebook,QUuid));
    QObject::connect(&localStorageManagerAsync, QNSIGNAL(LocalStorageManagerAsync,expungeNotebookComplete,Notebook,QUuid),
                     this, QNSLOT(NoteModel,onExpungeNotebookComplete,Notebook,QUuid));
    QObject::connect(&localStorageManagerAsync, QNSIGNAL(LocalStorageManagerAsync,findTagComplete,Tag,QUuid),
                     this, QNSLOT(NoteModel,onFindTagComplete,Tag,QUuid));
    QObject::connect(&localStorageManagerAsync, QNSIGNAL(LocalStorageManagerAsync,findTagFailed,Tag,ErrorString,QUuid),
                     this, QNSLOT(NoteModel,onFindTagFailed,Tag,ErrorString,QUuid));
    QObject::connect(&localStorageManagerAsync, QNSIGNAL(LocalStorageManagerAsync,addTagComplete,Tag,QUuid),
                     this, QNSLOT(NoteModel,onAddTagComplete,Tag,QUuid));
    QObject::connect(&localStorageManagerAsync, QNSIGNAL(LocalStorageManagerAsync,updateTagComplete,Tag,QUuid),
                     this, QNSLOT(NoteModel,onUpdateTagComplete,Tag,QUuid));
    QObject::connect(&localStorageManagerAsync, QNSIGNAL(LocalStorageManagerAsync,expungeTagComplete,Tag,QStringList,QUuid),
                     this, QNSLOT(NoteModel,onExpungeTagComplete,Tag,QStringList,QUuid));
}

void NoteModel::requestNotesList()
{
    NMDEBUG(QStringLiteral("NoteModel::requestNotesList: offset = ") << m_listNotesOffset);

    LocalStorageManager::ListObjectsOptions flags = LocalStorageManager::ListAll;
    LocalStorageManager::ListNotesOrder::type order = LocalStorageManager::ListNotesOrder::NoOrder;
    LocalStorageManager::OrderDirection::type direction = LocalStorageManager::OrderDirection::Ascending;

    m_listNotesRequestId = QUuid::createUuid();
    NMTRACE(QStringLiteral("Emitting the request to list notes: offset = ") << m_listNotesOffset
            << QStringLiteral(", request id = ") << m_listNotesRequestId);
    Q_EMIT listNotes(flags, /* with resource binary data = */ false, NOTE_LIST_LIMIT, m_listNotesOffset, order, direction, QString(), m_listNotesRequestId);
}

QVariant NoteModel::dataImpl(const int row, const Columns::type column) const
{
    if (Q_UNLIKELY((row < 0) || (row >= static_cast<int>(m_data.size())))) {
        return QVariant();
    }

    const NoteDataByIndex & index = m_data.get<ByIndex>();
    const NoteModelItem & item = index[static_cast<size_t>(row)];

    switch(column)
    {
    case Columns::CreationTimestamp:
        return item.creationTimestamp();
    case Columns::ModificationTimestamp:
        return item.modificationTimestamp();
    case Columns::DeletionTimestamp:
        return item.deletionTimestamp();
    case Columns::Title:
        return item.title();
    case Columns::PreviewText:
        return item.previewText();
    case Columns::ThumbnailImage:
        return item.thumbnail();
    case Columns::NotebookName:
        return item.notebookName();
    case Columns::TagNameList:
        return item.tagNameList();
    case Columns::Size:
        return item.sizeInBytes();
    case Columns::Synchronizable:
        return item.isSynchronizable();
    case Columns::Dirty:
        return item.isDirty();
    case Columns::HasResources:
        return item.hasResources();
    default:
        return QVariant();
    }
}

QVariant NoteModel::dataAccessibleText(const int row, const Columns::type column) const
{
    if (Q_UNLIKELY((row < 0) || (row >= static_cast<int>(m_data.size())))) {
        return QVariant();
    }

    const NoteDataByIndex & index = m_data.get<ByIndex>();
    const NoteModelItem & item = index[static_cast<size_t>(row)];

    QString space = QStringLiteral(" ");
    QString colon = QStringLiteral(":");
    QString accessibleText = tr("Note") + colon + space;

    switch(column)
    {
    case Columns::CreationTimestamp:
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
    case Columns::ModificationTimestamp:
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
    case Columns::DeletionTimestamp:
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
    case Columns::Title:
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
    case Columns::PreviewText:
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
    case Columns::NotebookName:
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
    case Columns::TagNameList:
        {
            const QStringList & tagNameList = item.tagNameList();
            if (tagNameList.isEmpty()) {
                accessibleText += tr("tag list is empty");
            }
            else {
                accessibleText += tr("has tags") + colon + space + tagNameList.join(QStringLiteral(", "));
            }
            break;
        }
    case Columns::Size:
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
    case Columns::Synchronizable:
        accessibleText += (item.isSynchronizable() ? tr("synchronizable") : tr("not synchronizable"));
        break;
    case Columns::Dirty:
        accessibleText += (item.isDirty() ? tr("dirty") : tr("not dirty"));
        break;
    case Columns::HasResources:
        accessibleText += (item.hasResources() ? tr("has attachments") : tr("has no attachments"));
        break;
    case Columns::ThumbnailImage:
    default:
        return QVariant();
    }

    return accessibleText;
}

void NoteModel::processTagExpunging(const QString & tagLocalUid)
{
    NMDEBUG(QStringLiteral("NoteModel::processTagExpunging: tag local uid = ") << tagLocalUid);

    auto tagDataIt = m_tagDataByTagLocalUid.find(tagLocalUid);
    if (tagDataIt == m_tagDataByTagLocalUid.end()) {
        NMTRACE(QStringLiteral("Tag data corresponding to the expunged tag was not found within the note model"));
        return;
    }

    QString tagGuid = tagDataIt->m_guid;
    QString tagName = tagDataIt->m_name;

    Q_UNUSED(m_tagDataByTagLocalUid.erase(tagDataIt))

    auto noteIt = m_tagLocalUidToNoteLocalUid.find(tagLocalUid);
    if (noteIt == m_tagLocalUidToNoteLocalUid.end()) {
        return;
    }

    NoteDataByLocalUid & localUidIndex = m_data.get<ByLocalUid>();

    QStringList affectedNotesLocalUids;
    while(noteIt != m_tagLocalUidToNoteLocalUid.end())
    {
        if (noteIt.key() != tagLocalUid) {
            break;
        }

        affectedNotesLocalUids << noteIt.value();
        ++noteIt;
    }

    Q_UNUSED(m_tagLocalUidToNoteLocalUid.remove(tagLocalUid))

    NMDEBUG("Affected notes local uids: " << affectedNotesLocalUids.join(QStringLiteral(", ")));
    for(auto it = affectedNotesLocalUids.constBegin(), end = affectedNotesLocalUids.constEnd(); it != end; ++it)
    {
        auto noteItemIt = localUidIndex.find(*it);
        if (Q_UNLIKELY(noteItemIt == localUidIndex.end())) {
            NMDEBUG(QStringLiteral("Can't find the note pointed to by the expunged tag by local uid: note local uid = ") << noteIt.value());
            continue;
        }

        NoteModelItem item = *noteItemIt;
        item.removeTagGuid(tagGuid);
        item.removeTagName(tagName);
        item.removeTagLocalUid(tagLocalUid);

        Q_UNUSED(localUidIndex.replace(noteItemIt, item))

        QModelIndex modelIndex = indexForLocalUid(item.localUid());
        modelIndex = createIndex(modelIndex.row(), Columns::TagNameList);
        Q_EMIT dataChanged(modelIndex, modelIndex);

        // This note's cache entry is clearly stale now, need to ensure
        // it won't be present in the cache
        Q_UNUSED(m_cache.remove(item.localUid()))
    }
}

void NoteModel::removeItemByLocalUid(const QString & localUid)
{
    NMDEBUG(QStringLiteral("NoteModel::removeItemByLocalUid: ") << localUid);

    NoteDataByLocalUid & localUidIndex = m_data.get<ByLocalUid>();
    auto itemIt = localUidIndex.find(localUid);
    if (Q_UNLIKELY(itemIt == localUidIndex.end())) {
        NMDEBUG(QStringLiteral("Can't find item to remove from the note model"));
        return;
    }

    const NoteModelItem & item = *itemIt;

    NoteDataByIndex & index = m_data.get<ByIndex>();
    auto indexIt = m_data.project<ByIndex>(itemIt);
    if (Q_UNLIKELY(indexIt == index.end())) {
        NMWARNING(QStringLiteral("Can't determine the row index for the note model item to remove: ") << item);
        return;
    }

    int row = static_cast<int>(std::distance(index.begin(), indexIt));
    if (Q_UNLIKELY((row < 0) || (row >= static_cast<int>(m_data.size())))) {
        NMWARNING(QStringLiteral("Invalid row index for the note model item to remove: index = ") << row
                  << QStringLiteral(", item: ") << item);
        return;
    }

    beginRemoveRows(QModelIndex(), row, row);
    Q_UNUSED(localUidIndex.erase(itemIt))
    endRemoveRows();
}

void NoteModel::updateItemRowWithRespectToSorting(const NoteModelItem & item)
{
    NMDEBUG(QStringLiteral("NoteModel::updateItemRowWithRespectToSorting: item local uid = ")
            << item.localUid());

    NoteDataByLocalUid & localUidIndex = m_data.get<ByLocalUid>();
    auto localUidIt = localUidIndex.find(item.localUid());
    if (Q_UNLIKELY(localUidIt == localUidIndex.end())) {
        NMWARNING(QStringLiteral("Can't update item row with respect to sorting: can't find the within the model: ") << item);
        return;
    }

    NoteDataByIndex & index = m_data.get<ByIndex>();

    auto it = m_data.project<ByIndex>(localUidIt);
    if (Q_UNLIKELY(it == index.end())) {
        NMWARNING(QStringLiteral("Can't update item row with respect to sorting: can't find item's original row; item: ") << item);
        return;
    }

    int originalRow = static_cast<int>(std::distance(index.begin(), it));
    if (Q_UNLIKELY((originalRow < 0) || (originalRow >= static_cast<int>(m_data.size())))) {
        NMWARNING(QStringLiteral("Can't update item row with respect to sorting: item's original row is beyond the acceptable range: ")
                  << originalRow << QStringLiteral(", item: ") << item);
        return;
    }

    NoteModelItem itemCopy(item);

    NMTRACE(QStringLiteral("Removing the moved item from the original row ") << originalRow);
    beginRemoveRows(QModelIndex(), originalRow, originalRow);
    Q_UNUSED(index.erase(it))
    endRemoveRows();

    auto positionIter = std::lower_bound(index.begin(), index.end(), itemCopy,
                                         NoteComparator(m_sortedColumn, m_sortOrder));
    if (positionIter == index.end())
    {
        int newRow = static_cast<int>(index.size());

        NMTRACE(QStringLiteral("Inserting the moved item at row ") << newRow);
        beginInsertRows(QModelIndex(), newRow, newRow);
        index.push_back(itemCopy);
        endInsertRows();

        return;
    }

    int newRow = static_cast<int>(std::distance(index.begin(), positionIter));

    NMTRACE(QStringLiteral("Inserting the moved item at row ") << newRow);
    beginInsertRows(QModelIndex(), newRow, newRow);
    Q_UNUSED(index.insert(positionIter, itemCopy))
    endInsertRows();
}

void NoteModel::updateNoteInLocalStorage(const NoteModelItem & item, const bool updateTags)
{
    NMDEBUG(QStringLiteral("NoteModel::updateNoteInLocalStorage: local uid = ")
            << item.localUid() << QStringLiteral(", update tags = ")
            << (updateTags ? QStringLiteral("true") : QStringLiteral("false")));

    Note note;

    auto notYetSavedItemIt = m_noteItemsNotYetInLocalStorageUids.find(item.localUid());
    if (notYetSavedItemIt == m_noteItemsNotYetInLocalStorageUids.end())
    {
        NMDEBUG(QStringLiteral("Updating the note"));

        const Note * pCachedNote = m_cache.get(item.localUid());
        if (Q_UNLIKELY(!pCachedNote))
        {
            QUuid requestId = QUuid::createUuid();
            Q_UNUSED(m_findNoteToPerformUpdateRequestIds.insert(requestId))
            Note dummy;
            dummy.setLocalUid(item.localUid());
            NMTRACE(QStringLiteral("Emitting the request to find note: local uid = ") << item.localUid()
                    << QStringLiteral(", request id = ") << requestId);
            Q_EMIT findNote(dummy, /* with resource binary data = */ false, requestId);
            return;
        }

        note = *pCachedNote;
    }

    note.setLocalUid(item.localUid());
    note.setGuid(item.guid());
    note.setNotebookLocalUid(item.notebookLocalUid());
    note.setNotebookGuid(item.notebookGuid());
    note.setCreationTimestamp(item.creationTimestamp());
    note.setModificationTimestamp(item.modificationTimestamp());
    note.setDeletionTimestamp(item.deletionTimestamp());
    note.setTagLocalUids(item.tagLocalUids());
    note.setTagGuids(item.tagGuids());
    note.setTitle(item.title());
    note.setLocal(!item.isSynchronizable());
    note.setDirty(item.isDirty());
    note.setFavorited(item.isFavorited());

    QUuid requestId = QUuid::createUuid();

    if (notYetSavedItemIt != m_noteItemsNotYetInLocalStorageUids.end())
    {
        Q_UNUSED(m_addNoteRequestIds.insert(requestId))
        Q_UNUSED(m_noteItemsNotYetInLocalStorageUids.erase(notYetSavedItemIt))

        NMTRACE(QStringLiteral("Emitting the request to add the note to local storage: id = ") << requestId
                << QStringLiteral(", note: ") << note);
        Q_EMIT addNote(note, requestId);
    }
    else
    {
        Q_UNUSED(m_updateNoteRequestIds.insert(requestId))

        // While the note is being updated in the local storage,
        // remove its stale copy from the cache
        Q_UNUSED(m_cache.remove(note.localUid()))

        NMTRACE(QStringLiteral("Emitting the request to update the note in local storage: id = ") << requestId
                << QStringLiteral(", note: ") << note);
        Q_EMIT updateNote(note, /* update resources = */ false, /* update tags = */ updateTags, requestId);
    }
}

int NoteModel::rowForNewItem(const NoteModelItem & item) const
{
    const NoteDataByIndex & index = m_data.get<ByIndex>();

    auto it = std::lower_bound(index.begin(), index.end(), item, NoteComparator(m_sortedColumn, m_sortOrder));
    if (it == index.end()) {
        return static_cast<int>(index.size());
    }

    return static_cast<int>(std::distance(index.begin(), it));
}

bool NoteModel::canUpdateNoteItem(const NoteModelItem & item) const
{
    auto it = m_notebookDataByNotebookLocalUid.find(item.notebookLocalUid());
    if (it == m_notebookDataByNotebookLocalUid.end()) {
        NMDEBUG(QStringLiteral("Can't find the notebook data for note with local uid ") << item.localUid());
        return false;
    }

    const NotebookData & notebookData = it.value();
    return notebookData.m_canUpdateNotes;
}

bool NoteModel::canCreateNoteItem(const QString & notebookLocalUid) const
{
    if (notebookLocalUid.isEmpty()) {
        NMDEBUG(QStringLiteral("NoteModel::canCreateNoteItem: empty notebook local uid"));
        return false;
    }

    auto it = m_notebookDataByNotebookLocalUid.find(notebookLocalUid);
    if (it != m_notebookDataByNotebookLocalUid.end()) {
        return it->m_canCreateNotes;
    }

    NMDEBUG(QStringLiteral("Can't find the notebook data for notebook local uid ") << notebookLocalUid);
    return false;
}

void NoteModel::updateNotebookData(const Notebook & notebook)
{
    NMDEBUG(QStringLiteral("NoteModel::updateNotebookData: local uid = ") << notebook.localUid());

    NotebookData & notebookData = m_notebookDataByNotebookLocalUid[notebook.localUid()];

    if (!notebook.hasRestrictions())
    {
        notebookData.m_canCreateNotes = true;
        notebookData.m_canUpdateNotes = true;
    }
    else
    {
        const qevercloud::NotebookRestrictions & notebookRestrictions = notebook.restrictions();
        notebookData.m_canCreateNotes = (notebookRestrictions.noCreateNotes.isSet()
                                         ? (!notebookRestrictions.noCreateNotes.ref())
                                         : true);
        notebookData.m_canUpdateNotes = (notebookRestrictions.noUpdateNotes.isSet()
                                         ? (!notebookRestrictions.noUpdateNotes.ref())
                                         : true);
    }

    if (notebook.hasName()) {
        notebookData.m_name = notebook.name();
    }

    if (notebook.hasGuid()) {
        notebookData.m_guid = notebook.guid();
    }

    NMDEBUG(QStringLiteral("Collected notebook data from notebook with local uid ") << notebook.localUid()
            << QStringLiteral(": guid = ") << notebookData.m_guid << QStringLiteral("; name = ") << notebookData.m_name
            << QStringLiteral(": can create notes = ") << (notebookData.m_canCreateNotes ? QStringLiteral("true") : QStringLiteral("false"))
            << QStringLiteral(": can update notes = ") << (notebookData.m_canUpdateNotes ? QStringLiteral("true") : QStringLiteral("false")));

    checkAddedNoteItemsPendingNotebookData(notebook.localUid(), notebookData);
}

void NoteModel::updateTagData(const Tag & tag)
{
    NMDEBUG(QStringLiteral("NoteModel::updateTagData: tag local uid = ") << tag.localUid());

    bool hasName = tag.hasName();
    bool hasGuid = tag.hasGuid();

    TagData & tagData = m_tagDataByTagLocalUid[tag.localUid()];

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

    auto noteIt = m_tagLocalUidToNoteLocalUid.find(tag.localUid());
    if (noteIt == m_tagLocalUidToNoteLocalUid.end()) {
        return;
    }

    NoteDataByLocalUid & localUidIndex = m_data.get<ByLocalUid>();

    QStringList affectedNotesLocalUids;
    while(noteIt != m_tagLocalUidToNoteLocalUid.end())
    {
        if (noteIt.key() != tag.localUid()) {
            break;
        }

        affectedNotesLocalUids << noteIt.value();
        ++noteIt;
    }

    NMDEBUG("Affected notes local uids: " << affectedNotesLocalUids.join(QStringLiteral(", ")));
    for(auto it = affectedNotesLocalUids.constBegin(), end = affectedNotesLocalUids.constEnd(); it != end; ++it)
    {
        auto noteItemIt = localUidIndex.find(*it);
        if (Q_UNLIKELY(noteItemIt == localUidIndex.end())) {
            NMDEBUG(QStringLiteral("Can't find the note pointed to by a tag by local uid: "
                                   "note local uid = ") << *it);
            continue;
        }

        NoteModelItem item = *noteItemIt;
        QStringList tagLocalUids = item.tagLocalUids();

        // Need to refresh all the tag names and guids because it is generally unknown which particular tag was updated
        item.setTagNameList(QStringList());
        item.setTagGuids(QStringList());

        for(auto tagLocalUidIt = tagLocalUids.begin(), tagLocalUidEnd = tagLocalUids.end(); tagLocalUidIt != tagLocalUidEnd; ++tagLocalUidIt)
        {
            auto tagDataIt = m_tagDataByTagLocalUid.find(*tagLocalUidIt);
            if (tagDataIt == m_tagDataByTagLocalUid.end()) {
                NMTRACE(QStringLiteral("Still no tag data for tag with local uid ") << *tagLocalUidIt);
                continue;
            }

            const TagData & tagData = tagDataIt.value();

            if (!tagData.m_name.isEmpty()) {
                item.addTagName(tagData.m_name);
            }

            if (!tagData.m_guid.isEmpty()) {
                item.addTagGuid(tagData.m_guid);
            }
        }

        Q_UNUSED(localUidIndex.replace(noteItemIt, item))

        QModelIndex modelIndex = indexForLocalUid(item.localUid());
        modelIndex = createIndex(modelIndex.row(), Columns::TagNameList);
        Q_EMIT dataChanged(modelIndex, modelIndex);
    }
}

void NoteModel::setNoteFavorited(const QString & noteLocalUid, const bool favorited)
{
    NoteDataByLocalUid & localUidIndex = m_data.get<ByLocalUid>();
    auto it = localUidIndex.find(noteLocalUid);
    if (Q_UNLIKELY(it == localUidIndex.end())) {
        REPORT_ERROR(QT_TR_NOOP("Can't favorite/unfavorite the note: internal error, the note "
                                "to be favorited/unfavorited was not found within the model"));
        return;
    }

    const NoteModelItem & item = *it;

    if (favorited == item.isFavorited()) {
        NMDEBUG(QStringLiteral("Favorited flag's value hasn't changed"));
        return;
    }

    NoteModelItem itemCopy(item);
    itemCopy.setFavorited(favorited);

    localUidIndex.replace(it, itemCopy);
    updateNoteInLocalStorage(itemCopy);
}

void NoteModel::checkAddedNoteItemsPendingNotebookData(const QString & notebookLocalUid, const NotebookData & notebookData)
{
    auto it = m_noteItemsPendingNotebookDataUpdate.find(notebookLocalUid);
    while(it != m_noteItemsPendingNotebookDataUpdate.end())
    {
        if (it.key() != notebookLocalUid) {
            break;
        }

        addOrUpdateNoteItem(it.value(), notebookData);
        Q_UNUSED(m_noteItemsPendingNotebookDataUpdate.erase(it++))
    }
}

void NoteModel::onNoteAddedOrUpdated(const Note & note)
{
    NMDEBUG(QStringLiteral("NoteModel::onNoteAddedOrUpdated: note local uid = ") << note.localUid());
    NMTRACE(note);

    m_cache.put(note.localUid(), note);

    if (!note.hasNotebookLocalUid()) {
        NMWARNING(QStringLiteral("Skipping the note not having the notebook local uid: ") << note);
        return;
    }

    NoteModelItem item;
    noteToItem(note, item);

    auto notebookIt = m_notebookDataByNotebookLocalUid.find(item.notebookLocalUid());
    if (notebookIt == m_notebookDataByNotebookLocalUid.end())
    {
        bool findNotebookRequestSent = false;

        Q_UNUSED(m_noteItemsPendingNotebookDataUpdate.insert(item.notebookLocalUid(), item))

        auto it = m_findNotebookRequestForNotebookLocalUid.left.find(item.notebookLocalUid());
        if (it != m_findNotebookRequestForNotebookLocalUid.left.end()) {
            findNotebookRequestSent = true;
        }

        if (!findNotebookRequestSent)
        {
            Notebook notebook;
            if (note.hasNotebookLocalUid()) {
                notebook.setLocalUid(note.notebookLocalUid());
            }
            else {
                notebook.setLocalUid(QString());
                notebook.setGuid(note.notebookGuid());
            }

            QUuid requestId = QUuid::createUuid();
            Q_UNUSED(m_findNotebookRequestForNotebookLocalUid.insert(LocalUidToRequestIdBimap::value_type(item.notebookLocalUid(), requestId)))
            NMTRACE(QStringLiteral("Emitting the request to find notebook local uid: = ") << item.notebookLocalUid()
                    << QStringLiteral(", request id = ") << requestId);
            Q_EMIT findNotebook(notebook, requestId);
        }
        else
        {
            NMTRACE(QStringLiteral("The request to find notebook for this note has already been sent"));
        }

        return;
    }

    const NotebookData & notebookData = notebookIt.value();
    addOrUpdateNoteItem(item, notebookData);
}

void NoteModel::addOrUpdateNoteItem(NoteModelItem & item, const NotebookData & notebookData)
{
    NMDEBUG(QStringLiteral("NoteModel::addOrUpdateNoteItem: note local uid = ") << item.localUid()
            << QStringLiteral(", notebook local uid = ") << item.notebookLocalUid()
            << QStringLiteral(", notebook name = ") << notebookData.m_name);

    if (!notebookData.m_canCreateNotes)
    {
        ErrorString error(QT_TR_NOOP("Can't create a new note: notebook restrictions apply"));
        NMINFO(error << QStringLiteral(", notebook name = ") << notebookData.m_name);
        Q_EMIT notifyError(error);
        return;
    }

    item.setNotebookName(notebookData.m_name);
    findTagNamesForItem(item);

    NoteDataByLocalUid & localUidIndex = m_data.get<ByLocalUid>();
    auto it = localUidIndex.find(item.localUid());
    if (it == localUidIndex.end())
    {
        switch(m_includedNotes)
        {
        case IncludedNotes::All:
            break;
        case IncludedNotes::Deleted:
            {
                if (item.deletionTimestamp() < 0) {
                    NMDEBUG(QStringLiteral("Won't add note as it's not deleted and the model instance only considers the deleted notes"));
                    return;
                }
                break;
            }
        case IncludedNotes::NonDeleted:
            {
                if (item.deletionTimestamp() >= 0) {
                    NMDEBUG(QStringLiteral("Won't add note as it's deleted and the model instance only considers the non-deleted notes"));
                    return;
                }
                break;
            }
        }

        int row = static_cast<int>(localUidIndex.size());
        beginInsertRows(QModelIndex(), row, row);
        Q_UNUSED(localUidIndex.insert(item))
        endInsertRows();

        updateItemRowWithRespectToSorting(item);
    }
    else
    {
        bool shouldRemoveItem = false;

        switch(m_includedNotes)
        {
        case IncludedNotes::All:
            break;
        case IncludedNotes::Deleted:
            {
                if (item.deletionTimestamp() < 0)
                {
                    NMDEBUG(QStringLiteral("Removing the updated note item from the model as the item is not deleted "
                                           "and the model instance only considers the deleted notes"));
                    shouldRemoveItem = true;
                }
                break;
            }
        case IncludedNotes::NonDeleted:
            {
                if (item.deletionTimestamp() >= 0)
                {
                    NMDEBUG("Removing the updated note item from the model as the item is deleted "
                            "and the model instance only considers the non-deleted notes");
                    shouldRemoveItem = true;
                }
                break;
            }
        }

        NoteDataByIndex & index = m_data.get<ByIndex>();
        auto indexIt = m_data.project<ByIndex>(it);
        if (Q_UNLIKELY(indexIt == index.end())) {
            REPORT_ERROR(QT_TR_NOOP("Internal error: can't project the local uid index iterator "
                                    "to the random access index iterator in note model"));
            return;
        }

        int row = static_cast<int>(std::distance(index.begin(), indexIt));

        if (shouldRemoveItem)
        {
            beginRemoveRows(QModelIndex(), row, row);
            Q_UNUSED(localUidIndex.erase(it))
            endRemoveRows();
        }
        else
        {
            QModelIndex modelIndexFrom = createIndex(row, Columns::CreationTimestamp);
            QModelIndex modelIndexTo = createIndex(row, Columns::HasResources);
            Q_UNUSED(localUidIndex.replace(it, item))
            Q_EMIT dataChanged(modelIndexFrom, modelIndexTo);

            updateItemRowWithRespectToSorting(item);
        }
    }
}

void NoteModel::findTagNamesForItem(NoteModelItem & item)
{
    NMDEBUG(QStringLiteral("NoteModel::findTagNamesForItem: ") << item);

    const QStringList & tagLocalUids = item.tagLocalUids();
    for(auto it = tagLocalUids.constBegin(), end = tagLocalUids.constEnd(); it != end; ++it)
    {
        const QString & tagLocalUid = *it;

        bool alreadyGotNoteLocalUidMapped = false;

        auto tagToNoteIt = m_tagLocalUidToNoteLocalUid.find(tagLocalUid);
        while(tagToNoteIt != m_tagLocalUidToNoteLocalUid.end())
        {
            if (tagToNoteIt.key() != tagLocalUid) {
                break;
            }

            if (tagToNoteIt.value() == item.localUid()) {
                alreadyGotNoteLocalUidMapped = true;
                break;
            }

            ++tagToNoteIt;
        }

        if (!alreadyGotNoteLocalUidMapped) {
            Q_UNUSED(m_tagLocalUidToNoteLocalUid.insert(tagLocalUid, item.localUid()))
            NMDEBUG("Tag local uid " << tagLocalUid << " points to note model item "
                    << item.localUid() << ", title = " << item.title());
        }

        auto tagDataIt = m_tagDataByTagLocalUid.find(tagLocalUid);
        if (tagDataIt != m_tagDataByTagLocalUid.end()) {
            NMTRACE(QStringLiteral("Found tag data for tag local uid ") << tagLocalUid
                    << QStringLiteral(": tag name = ") << tagDataIt->m_name);
            item.addTagName(tagDataIt->m_name);
            continue;
        }

        NMTRACE(QStringLiteral("Tag data for tag local uid ") << tagLocalUid << QStringLiteral(" was not found"));

        auto requestIt = m_findTagRequestForTagLocalUid.left.find(tagLocalUid);
        if (requestIt != m_findTagRequestForTagLocalUid.left.end()) {
            NMTRACE(QStringLiteral("The request to find tag corresponding to local uid ") << tagLocalUid
                    << QStringLiteral(" has already been sent: request id = ") << requestIt->second);
            continue;
        }

        QUuid requestId = QUuid::createUuid();
        Q_UNUSED(m_findTagRequestForTagLocalUid.insert(LocalUidToRequestIdBimap::value_type(tagLocalUid, requestId)))

        Tag tag;
        tag.setLocalUid(tagLocalUid);
        NMDEBUG(QStringLiteral("Emitting the request to find tag: tag local uid = ") << tagLocalUid
                << QStringLiteral(", request id = ") << requestId);
        Q_EMIT findTag(tag, requestId);
    }
}

// WARNING: this method assumes the iterator passed to it is not end()
void NoteModel::moveNoteToNotebookImpl(NoteDataByLocalUid::iterator it, const Notebook & notebook)
{
    NoteModelItem item = *it;

    NMDEBUG(QStringLiteral("NoteModel::moveNoteToNotebookImpl: notebook = ") << notebook
            << QStringLiteral(", note item: ") << item);

    if (Q_UNLIKELY(item.notebookLocalUid() == notebook.localUid())) {
        NMDEBUG(QStringLiteral("The note is already within its target notebook, nothing to do"));
        return;
    }

    if (!notebook.canCreateNotes()) {
        ErrorString error(QT_TR_NOOP("Can't move the note to another notebook: the target notebook "
                                     "doesn't allow to create notes in it"));
        NMINFO(error << QStringLiteral(", notebook: ") << notebook);
        Q_EMIT notifyError(error);
        return;
    }

    item.setNotebookLocalUid(notebook.localUid());
    item.setNotebookName(notebook.hasName() ? notebook.name() : QString());
    item.setNotebookGuid(notebook.hasGuid() ? notebook.guid() : QString());

    item.setDirty(true);
    item.setModificationTimestamp(QDateTime::currentMSecsSinceEpoch());

    QModelIndex itemIndex = indexForLocalUid(item.localUid());
    itemIndex = index(itemIndex.row(), Columns::NotebookName, itemIndex.parent());

    NoteDataByLocalUid & localUidIndex = m_data.get<ByLocalUid>();
    localUidIndex.replace(it, item);
    Q_EMIT dataChanged(itemIndex, itemIndex);

    updateItemRowWithRespectToSorting(*it);
    updateNoteInLocalStorage(item);
}

void NoteModel::checkAndNotifyAllNotesListed()
{
    NMDEBUG(QStringLiteral("NoteModel::checkAndNotifyAllNotesListed"));

    if (m_allNotesListed) {
        NMDEBUG(QStringLiteral("All notes are already listed"));
        return;
    }

    if (!m_listNotesRequestId.isNull()) {
        NMDEBUG(QStringLiteral("Not all notes have been listed yet"));
        return;
    }

    if (!m_findNotebookRequestForNotebookLocalUid.empty()) {
        NMDEBUG(QStringLiteral("Not all notebooks for notes have been found yet, currently waiting for ")
                << m_findNotebookRequestForNotebookLocalUid.size() << QStringLiteral(" notebooks to be found"));
        return;
    }

    if (!m_findTagRequestForTagLocalUid.empty()) {
        NMDEBUG(QStringLiteral("Not all tags for notes have been found yet, currently waiting for ")
                << m_findTagRequestForTagLocalUid.size() << QStringLiteral(" tags to be found"));
        return;
    }

    NMDEBUG(QStringLiteral("All the necessary data items were listed"));
    m_allNotesListed = true;
    Q_EMIT notifyAllNotesListed();
}

void NoteModel::noteToItem(const Note & note, NoteModelItem & item)
{
    item.setLocalUid(note.localUid());

    if (note.hasGuid()) {
        item.setGuid(note.guid());
    }

    if (note.hasNotebookGuid()) {
        item.setNotebookGuid(note.notebookGuid());
    }

    if (note.hasNotebookLocalUid()) {
        item.setNotebookLocalUid(note.notebookLocalUid());
    }

    if (note.hasTitle()) {
        item.setTitle(note.title());
    }

    if (note.hasContent()) {
        QString previewText = note.plainText();
        previewText.truncate(NOTE_PREVIEW_TEXT_SIZE);
        item.setPreviewText(previewText);
    }

    item.setThumbnail(note.thumbnail());

    if (note.hasTagLocalUids())
    {
        const QStringList & tagLocalUids = note.tagLocalUids();
        item.setTagLocalUids(tagLocalUids);

        QStringList tagNames;
        tagNames.reserve(tagLocalUids.size());

        for(auto it = tagLocalUids.constBegin(), end = tagLocalUids.constEnd(); it != end; ++it)
        {
            auto tagIt = m_tagDataByTagLocalUid.find(*it);
            if (tagIt != m_tagDataByTagLocalUid.end()) {
                const TagData & tagData = tagIt.value();
                tagNames << tagData.m_name;
            }
        }

        item.setTagNameList(tagNames);
    }

    if (note.hasTagGuids()) {
        const QStringList tagGuids = note.tagGuids();
        item.setTagGuids(tagGuids);
    }

    if (note.hasCreationTimestamp()) {
        item.setCreationTimestamp(note.creationTimestamp());
    }

    if (note.hasModificationTimestamp()) {
        item.setModificationTimestamp(note.modificationTimestamp());
    }

    if (note.hasDeletionTimestamp()) {
        item.setDeletionTimestamp(note.deletionTimestamp());
    }

    item.setSynchronizable(!note.isLocal());
    item.setDirty(note.isDirty());
    item.setFavorited(note.isFavorited());
    item.setHasResources(note.hasResources() && (note.numResources() > 0));

    if (note.hasNoteRestrictions()) {
        const qevercloud::NoteRestrictions & restrictions = note.noteRestrictions();
        item.setCanUpdateTitle(!restrictions.noUpdateTitle.isSet() || !restrictions.noUpdateTitle.ref());
        item.setCanUpdateContent(!restrictions.noUpdateContent.isSet() || !restrictions.noUpdateContent.ref());
        item.setCanEmail(!restrictions.noEmail.isSet() || !restrictions.noEmail.ref());
        item.setCanShare(!restrictions.noShare.isSet() || !restrictions.noShare.ref());
        item.setCanSharePublicly(!restrictions.noSharePublicly.isSet() || !restrictions.noSharePublicly.ref());
    }
    else {
        item.setCanUpdateTitle(true);
        item.setCanUpdateContent(true);
        item.setCanEmail(true);
        item.setCanShare(true);
        item.setCanSharePublicly(true);
    }

    qint64 sizeInBytes = 0;
    if (note.hasContent()) {
        sizeInBytes += note.content().size();
    }

    if (note.hasResources())
    {
        QList<Resource> resources = note.resources();
        for(auto it = resources.constBegin(), end = resources.constEnd(); it != end; ++it)
        {
            const Resource & resource = *it;

            if (resource.hasDataBody()) {
                sizeInBytes += resource.dataBody().size();
            }

            if (resource.hasRecognitionDataBody()) {
                sizeInBytes += resource.recognitionDataBody().size();
            }

            if (resource.hasAlternateDataBody()) {
                sizeInBytes += resource.alternateDataBody().size();
            }
        }
    }

    sizeInBytes = std::max(qint64(0), sizeInBytes);
    item.setSizeInBytes(static_cast<quint64>(sizeInBytes));
}

bool NoteModel::NoteComparator::operator()(const NoteModelItem & lhs, const NoteModelItem & rhs) const
{
    bool less = false;
    bool greater = false;

    switch(m_sortedColumn)
    {
    case Columns::CreationTimestamp:
        less = (lhs.creationTimestamp() < rhs.creationTimestamp());
        greater = (lhs.creationTimestamp() > rhs.creationTimestamp());
        break;
    case Columns::ModificationTimestamp:
        less = (lhs.modificationTimestamp() < rhs.modificationTimestamp());
        greater = (lhs.modificationTimestamp() > rhs.modificationTimestamp());
        break;
    case Columns::DeletionTimestamp:
        less = (lhs.deletionTimestamp() < rhs.deletionTimestamp());
        greater = (lhs.deletionTimestamp() > rhs.deletionTimestamp());
        break;
    case Columns::Title:
        {
            QString leftTitleOrPreview = lhs.title();
            if (leftTitleOrPreview.isEmpty()) {
                leftTitleOrPreview = lhs.previewText();
            }

            QString rightTitleOrPreview = rhs.title();
            if (rightTitleOrPreview.isEmpty()) {
                rightTitleOrPreview = rhs.previewText();
            }

            int compareResult = leftTitleOrPreview.localeAwareCompare(rightTitleOrPreview);
            less = (compareResult < 0);
            greater = (compareResult > 0);
            break;
        }
    case Columns::PreviewText:
        {
            QString leftTitleOrPreview = lhs.title();
            if (leftTitleOrPreview.isEmpty()) {
                leftTitleOrPreview = lhs.previewText();
            }

            QString rightTitleOrPreview = rhs.title();
            if (rightTitleOrPreview.isEmpty()) {
                rightTitleOrPreview = rhs.previewText();
            }

            int compareResult = leftTitleOrPreview.localeAwareCompare(rightTitleOrPreview);
            less = (compareResult < 0);
            greater = (compareResult > 0);
            break;
        }
    case Columns::NotebookName:
        {
            int compareResult = lhs.notebookName().localeAwareCompare(rhs.notebookName());
            less = (compareResult < 0);
            greater = (compareResult > 0);
            break;
        }
    case Columns::Size:
        less = (lhs.sizeInBytes() < rhs.sizeInBytes());
        greater = (lhs.sizeInBytes() > rhs.sizeInBytes());
        break;
    case Columns::Synchronizable:
        less = (!lhs.isSynchronizable() && rhs.isSynchronizable());
        greater = (lhs.isSynchronizable() && !rhs.isSynchronizable());
        break;
    case Columns::Dirty:
        less = (!lhs.isDirty() && rhs.isDirty());
        greater = (lhs.isDirty() && !rhs.isDirty());
        break;
    case Columns::HasResources:
        less = (!lhs.hasResources() && rhs.hasResources());
        greater = (lhs.hasResources() && !rhs.hasResources());
        break;
    case Columns::ThumbnailImage:
    case Columns::TagNameList:
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
