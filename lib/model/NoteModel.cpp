/*
 * Copyright 2016-2019 Dmitry Ivanov
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
#include <quentier/utility/Utility.h>

#include <iterator>

// Separate logging macros for the note model - to distinguish the one
// for deleted notes from the one for non-deleted notes

inline QString includedNotesStr(
    const quentier::NoteModel::IncludedNotes::type includedNotes)
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
    QNTRACE(includedNotesStr(m_includedNotes) << message)                      \
// NMTRACE

#define NMDEBUG(message)                                                       \
    QNDEBUG(includedNotesStr(m_includedNotes) << message)                      \
// NMDEBUG

#define NMINFO(message)                                                        \
    QNINFO(includedNotesStr(m_includedNotes) << message)                       \
// NMINFO

#define NMWARNING(message)                                                     \
    QNWARNING(includedNotesStr(m_includedNotes) << message)                    \
// NMWARNING

#define NMCRITICAL(message)                                                    \
    QNCRITICAL(includedNotesStr(m_includedNotes) << message)                   \
// NMCRITICAL

#define NMFATAL(message)                                                       \
    QNFATAL(includedNotesStr(m_includedNotes) << message)                      \
// NMFATAL

// Limit for the queries to the local storage
#define NOTE_LIST_QUERY_LIMIT (10)

// Minimum number of notes which the model attempts to load from the local
// storage
#define NOTE_MIN_CACHE_SIZE (30)

#define NOTE_PREVIEW_TEXT_SIZE (500)

#define NUM_NOTE_MODEL_COLUMNS (12)

#define REPORT_ERROR(error, ...)                                               \
    ErrorString errorDescription(error);                                       \
    NMWARNING(errorDescription << QLatin1String("" __VA_ARGS__ ""));           \
    Q_EMIT notifyError(errorDescription)                                       \
// REPORT_ERROR

namespace quentier {

NoteModel::NoteModel(
        const Account & account,
        LocalStorageManagerAsync & localStorageManagerAsync,
        NoteCache & noteCache, NotebookCache & notebookCache,
        QObject * parent,
        const IncludedNotes::type includedNotes,
        const NoteSortingMode::type noteSortingMode,
        NoteFilters * pFilters) :
    QAbstractItemModel(parent),
    m_account(account),
    m_includedNotes(includedNotes),
    m_noteSortingMode(noteSortingMode),
    m_localStorageManagerAsync(localStorageManagerAsync),
    m_connectedToLocalStorage(false),
    m_isStarted(false),
    m_data(),
    m_totalFilteredNotesCount(0),
    m_cache(noteCache),
    m_notebookCache(notebookCache),
    m_pFilters(pFilters),
    m_pUpdatedNoteFilters(nullptr),
    m_maxNoteCount(NOTE_MIN_CACHE_SIZE * 2),
    m_listNotesOffset(0),
    m_listNotesRequestId(),
    m_getNoteCountRequestId(),
    m_totalAccountNotesCount(0),
    m_getFullNoteCountPerAccountRequestId(),
    m_notebookDataByNotebookLocalUid(),
    m_findNotebookRequestForNotebookLocalUid(),
    m_localUidsOfNewNotesBeingAddedToLocalStorage(),
    m_addNoteRequestIds(),
    m_updateNoteRequestIds(),
    m_expungeNoteRequestIds(),
    m_findNoteToRestoreFailedUpdateRequestIds(),
    m_findNoteToPerformUpdateRequestIds(),
    m_noteItemsPendingNotebookDataUpdate(),
    m_noteLocalUidToFindNotebookRequestIdForMoveNoteToNotebookBimap(),
    m_tagDataByTagLocalUid(),
    m_findTagRequestForTagLocalUid(),
    m_tagLocalUidToNoteLocalUid()
{}

NoteModel::~NoteModel()
{}

void NoteModel::updateAccount(const Account & account)
{
    NMDEBUG("NoteModel::updateAccount: " << account);
    m_account = account;
}

NoteModel::Columns::type NoteModel::sortingColumn() const
{
    switch(m_noteSortingMode)
    {
    case NoteSortingMode::CreatedAscending:
    case NoteSortingMode::CreatedDescending:
        return Columns::CreationTimestamp;
    case NoteSortingMode::ModifiedAscending:
    case NoteSortingMode::ModifiedDescending:
        return Columns::ModificationTimestamp;
    case NoteSortingMode::TitleAscending:
    case NoteSortingMode::TitleDescending:
        return Columns::Title;
    case NoteSortingMode::SizeAscending:
    case NoteSortingMode::SizeDescending:
        return Columns::Size;
    default:
        return Columns::ModificationTimestamp;
    }
}

Qt::SortOrder NoteModel::sortOrder() const
{
    switch(m_noteSortingMode)
    {
    case NoteSortingMode::CreatedDescending:
    case NoteSortingMode::ModifiedDescending:
    case NoteSortingMode::TitleDescending:
    case NoteSortingMode::SizeDescending:
        return Qt::DescendingOrder;
    default:
        return Qt::AscendingOrder;
    }
}

QModelIndex NoteModel::indexForLocalUid(const QString & localUid) const
{
    const NoteDataByLocalUid & localUidIndex = m_data.get<ByLocalUid>();
    auto it = localUidIndex.find(localUid);
    if (Q_UNLIKELY(it == localUidIndex.end())) {
        NMDEBUG("Can't find note item by local uid: " << localUid);
        return QModelIndex();
    }

    const NoteDataByIndex & index = m_data.get<ByIndex>();
    auto indexIt = m_data.project<ByIndex>(it);
    if (Q_UNLIKELY(indexIt == index.end())) {
        NMWARNING("Can't find the indexed reference to the note "
                  << "item: " << *it);
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
        NMDEBUG("Can't find note item by local uid: " << localUid);
        return nullptr;
    }

    return &(*it);
}

const NoteModelItem * NoteModel::itemAtRow(const int row) const
{
    const NoteDataByIndex & index = m_data.get<ByIndex>();
    if (Q_UNLIKELY((row < 0) || (index.size() <= static_cast<size_t>(row)))) {
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

bool NoteModel::hasFilters() const
{
    return !m_pFilters->isEmpty();
}

const QStringList & NoteModel::filteredNotebookLocalUids() const
{
    return m_pFilters->filteredNotebookLocalUids();
}

void NoteModel::setFilteredNotebookLocalUids(const QStringList & notebookLocalUids)
{
    NMDEBUG("NoteModel::setFilteredNotebookLocalUids: "
            << notebookLocalUids.join(QStringLiteral(", ")));

    if (!m_pUpdatedNoteFilters.isNull()) {
        m_pUpdatedNoteFilters->setFilteredNotebookLocalUids(notebookLocalUids);
        return;
    }

    m_pFilters->setFilteredNotebookLocalUids(notebookLocalUids);

    if (m_isStarted) {
        resetModel();
    }
}

void NoteModel::clearFilteredNotebookLocalUids()
{
    NMDEBUG("NoteModel::clearFilteredNotebookLocalUids");

    if (!m_pUpdatedNoteFilters.isNull()) {
        m_pUpdatedNoteFilters->clearFilteredNotebookLocalUids();
        return;
    }

    m_pFilters->clearFilteredNotebookLocalUids();

    if (m_isStarted) {
        resetModel();
    }
}

const QStringList & NoteModel::filteredTagLocalUids() const
{
    return m_pFilters->filteredTagLocalUids();
}

void NoteModel::setFilteredTagLocalUids(const QStringList & tagLocalUids)
{
    NMDEBUG("NoteModel::filteredTagLocalUids: "
            << tagLocalUids.join(QStringLiteral(", ")));

    if (!m_pUpdatedNoteFilters.isNull()) {
        m_pUpdatedNoteFilters->setFilteredTagLocalUids(tagLocalUids);
        return;
    }

    m_pFilters->setFilteredTagLocalUids(tagLocalUids);

    if (m_isStarted) {
        resetModel();
    }
}

void NoteModel::clearFilteredTagLocalUids()
{
    NMDEBUG("NoteModel::clearFilteredTagLocalUids");

    if (!m_pUpdatedNoteFilters.isNull()) {
        m_pUpdatedNoteFilters->clearFilteredTagLocalUids();
        return;
    }

    m_pFilters->clearFilteredTagLocalUids();

    if (m_isStarted) {
        resetModel();
    }
}

const QSet<QString> & NoteModel::filteredNoteLocalUids() const
{
    return m_pFilters->filteredNoteLocalUids();
}

void NoteModel::setFilteredNoteLocalUids(const QSet<QString> & noteLocalUids)
{
    if (QuentierIsLogLevelActive(LogLevel::DebugLevel))
    {
        QString str;
        QTextStream strm(&str);
        strm << "NoteModel::setFilteredNoteLocalUids: ";
        for(auto it = noteLocalUids.constBegin(),
            end = noteLocalUids.constEnd(); it != end; ++it)
        {
            if (it != noteLocalUids.constBegin()) {
                strm << ", ";
            }
            strm << *it;
        }
        strm.flush();
        NMDEBUG(str);
    }

    if (!m_pUpdatedNoteFilters.isNull()) {
        Q_UNUSED(m_pUpdatedNoteFilters->setFilteredNoteLocalUids(noteLocalUids))
        return;
    }

    if (m_pFilters->setFilteredNoteLocalUids(noteLocalUids))
    {
        if (m_isStarted) {
            resetModel();
        }
    }
    else {
        NMDEBUG("The set of filtered note local uids hasn't changed");
    }
}

void NoteModel::setFilteredNoteLocalUids(const QStringList & noteLocalUids)
{
    NMDEBUG("NoteModel::setFilteredNoteLocalUids: "
            << noteLocalUids.join(QStringLiteral(", ")));

    if (!m_pUpdatedNoteFilters.isNull()) {
        Q_UNUSED(m_pUpdatedNoteFilters->setFilteredNoteLocalUids(noteLocalUids))
        return;
    }

    if (m_pFilters->setFilteredNoteLocalUids(noteLocalUids))
    {
        if (m_isStarted) {
            resetModel();
        }
    }
    else {
        NMDEBUG("The set of filtered note local uids hasn't changed");
    }
}

void NoteModel::clearFilteredNoteLocalUids()
{
    NMDEBUG("NoteModel::clearFilteredNoteLocalUids");

    if (!m_pUpdatedNoteFilters.isNull()) {
        m_pUpdatedNoteFilters->clearFilteredNoteLocalUids();
        return;
    }

    m_pFilters->clearFilteredNoteLocalUids();
    resetModel();
}

void NoteModel::beginUpdateFilter()
{
    NMDEBUG("NoteModel::beginUpdateFilter");
    m_pUpdatedNoteFilters.reset(new NoteFilters);
}

void NoteModel::endUpdateFilter()
{
    NMDEBUG("NoteModel::endUpdateFilter");

    if (m_pUpdatedNoteFilters.isNull()) {
        return;
    }

    m_pFilters.swap(m_pUpdatedNoteFilters);
    m_pUpdatedNoteFilters.reset();

    resetModel();
}

qint32 NoteModel::totalFilteredNotesCount() const
{
    return m_totalFilteredNotesCount;
}

qint32 NoteModel::totalAccountNotesCount() const
{
    return m_totalAccountNotesCount;
}

QModelIndex NoteModel::createNoteItem(
    const QString & notebookLocalUid, ErrorString & errorDescription)
{
    NMDEBUG("NoteModel::createNoteItem: notebook local uid = "
            << notebookLocalUid);

    if (Q_UNLIKELY(!m_isStarted)) {
        errorDescription.setBase(
            QT_TR_NOOP("Can't create new note: note model is not started"));
        NMWARNING(errorDescription);
        return QModelIndex();
    }

    if (Q_UNLIKELY(m_includedNotes == IncludedNotes::Deleted)) {
        errorDescription.setBase(QT_TR_NOOP("Won't create new deleted note"));
        NMDEBUG(errorDescription);
        return QModelIndex();
    }

    if (m_getFullNoteCountPerAccountRequestId != QUuid()) {
        errorDescription.setBase(QT_TR_NOOP("Note model is not ready to create "
                                            "a new note, please wait"));
        NMDEBUG(errorDescription);
        return QModelIndex();
    }

    if (m_totalAccountNotesCount >= m_account.noteCountMax())
    {
        errorDescription.setBase(QT_TR_NOOP("Can't create a new note: the account "
                                            "already contains max allowed "
                                            "number of notes"));
        errorDescription.details() = QString::number(m_account.noteCountMax());
        NMINFO(errorDescription);
        return QModelIndex();
    }

    auto notebookIt = m_notebookDataByNotebookLocalUid.find(notebookLocalUid);
    if (notebookIt == m_notebookDataByNotebookLocalUid.end()) {
        errorDescription.setBase(QT_TR_NOOP("Can't create a new note: internal "
                                            "error, can't identify the notebook "
                                            "in which the note needs to be created"));
        NMINFO(errorDescription);
        return QModelIndex();
    }

    const NotebookData & notebookData = notebookIt.value();
    if (!notebookData.m_canCreateNotes) {
        errorDescription.setBase(QT_TR_NOOP("Can't create a new note: notebook "
                                            "restrictions apply"));
        NMDEBUG(errorDescription);
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

    Q_UNUSED(m_localUidsOfNewNotesBeingAddedToLocalStorage.insert(item.localUid()))
    saveNoteInLocalStorage(item);

    return createIndex(row, Columns::Title);
}

bool NoteModel::deleteNote(
    const QString & noteLocalUid, ErrorString & errorDescription)
{
    NMDEBUG("NoteModel::deleteNote: " << noteLocalUid);

    QModelIndex itemIndex = indexForLocalUid(noteLocalUid);
    if (!itemIndex.isValid()) {
        errorDescription.setBase(QT_TR_NOOP("note to be deleted was not found"));
        NMDEBUG(errorDescription);
        return false;
    }

    itemIndex = index(itemIndex.row(), Columns::DeletionTimestamp,
                      itemIndex.parent());
    qint64 timestamp = QDateTime::currentMSecsSinceEpoch();
    return setDataImpl(itemIndex, timestamp, errorDescription);
}

bool NoteModel::moveNoteToNotebook(
    const QString & noteLocalUid, const QString & notebookName,
    ErrorString & errorDescription)
{
    NMDEBUG("NoteModel::moveNoteToNotebook: note local uid = "
            << noteLocalUid << ", notebook name = "
            << notebookName);

    if (Q_UNLIKELY(notebookName.isEmpty())) {
        errorDescription.setBase(QT_TR_NOOP("the name of the target notebook "
                                            "is empty"));
        return false;
    }

    NoteDataByLocalUid & localUidIndex = m_data.get<ByLocalUid>();
    auto it = localUidIndex.find(noteLocalUid);
    if (Q_UNLIKELY(it == localUidIndex.end())) {
        errorDescription.setBase(QT_TR_NOOP("can't find the note to be moved to "
                                            "another notebook"));
        return false;
    }

    /**
     * First try to find the notebook in the cache; the cache is indexed by
     * local uid, not by name so need to do the linear search. It should be OK
     * since the cache is intended to be small
     */
    for(auto nit = m_notebookCache.begin(),
        end = m_notebookCache.end(); nit != end; ++nit)
    {
        const Notebook & notebook = nit->second;
        if (notebook.hasName() && (notebook.name() == notebookName)) {
            return moveNoteToNotebookImpl(it, notebook, errorDescription);
        }
    }

    /**
     * 2) No such notebook in the cache; attempt to find it within the local
     * storage asynchronously then
     */
    Notebook dummy;
    dummy.setName(notebookName);

    // Set empty local uid as a hint for local storage to search the notebook
    // by name
    dummy.setLocalUid(QString());
    QUuid requestId = QUuid::createUuid();
    Q_UNUSED(m_noteLocalUidToFindNotebookRequestIdForMoveNoteToNotebookBimap.insert(
             LocalUidToRequestIdBimap::value_type(noteLocalUid, requestId)))
    NMTRACE("Emitting the request to find a notebook by name for "
            << "moving the note to it: request id = "
            << requestId << ", notebook name = " << notebookName
            << ", note local uid = " << noteLocalUid);
    Q_EMIT findNotebook(dummy, requestId);

    return true;
}

bool NoteModel::favoriteNote(
    const QString & noteLocalUid, ErrorString & errorDescription)
{
    NMDEBUG("NoteModel::favoriteNote: " << noteLocalUid);
    return setNoteFavorited(noteLocalUid, true, errorDescription);
}

bool NoteModel::unfavoriteNote(
    const QString & noteLocalUid, ErrorString & errorDescription)
{
    NMDEBUG("NoteModel::unfavoriteNote: " << noteLocalUid);
    return setNoteFavorited(noteLocalUid, false, errorDescription);
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

QVariant NoteModel::headerData(
    int section, Qt::Orientation orientation, int role) const
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

QModelIndex NoteModel::index(
    int row, int column, const QModelIndex & parent) const
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

bool NoteModel::setHeaderData(
    int section, Qt::Orientation orientation, const QVariant & value, int role)
{
    Q_UNUSED(section)
    Q_UNUSED(orientation)
    Q_UNUSED(value)
    Q_UNUSED(role)
    return false;
}

bool NoteModel::setData(
    const QModelIndex & modelIndex, const QVariant & value, int role)
{
    if (role != Qt::EditRole) {
        return false;
    }

    ErrorString errorDescription;
    bool res = setDataImpl(modelIndex, value, errorDescription);
    if (!res) {
        Q_EMIT notifyError(errorDescription);
    }

    return res;
}

bool NoteModel::insertRows(int row, int count, const QModelIndex & parent)
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
        NMDEBUG("Ignoring the attempt to remove rows from note "
                "model for valid parent model index");
        return false;
    }

    ErrorString errorDescription;
    bool res = removeRowsImpl(row, count, errorDescription);
    if (!res) {
        Q_EMIT notifyError(errorDescription);
    }

    return res;
}

void NoteModel::sort(int column, Qt::SortOrder order)
{
    NMTRACE("NoteModel::sort: column = " << column
            << ", order = " << order << " ("
            << (order == Qt::AscendingOrder ? "ascending" : "descending")
            << ")");

    if ( (column == Columns::ThumbnailImage) ||
         (column == Columns::TagNameList) )
    {
        // Should not sort by these columns
        return;
    }

    if (Q_UNLIKELY((column < 0) || (column >= NUM_NOTE_MODEL_COLUMNS))) {
        return;
    }

    if (column == sortingColumn())
    {
        if (order == sortOrder()) {
            NMDEBUG("Neither sorted column nor sort order "
                    "have changed, nothing to do");
            return;
        }

        setSortingOrder(order);

        NMDEBUG("Only the sort order has changed");
    }
    else
    {
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
        NMDEBUG("Total filtered notes count = " << m_totalFilteredNotesCount
                << ", num loaded notes = " << m_data.size());
        return (m_data.size() < static_cast<size_t>(m_totalFilteredNotesCount));
    }

    if (m_getNoteCountRequestId != QUuid()) {
        NMDEBUG("Still pending get note count request");
        return false;
    }

    if (m_listNotesRequestId != QUuid()) {
        NMDEBUG("Still pending list notes request");
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

    m_maxNoteCount += NOTE_LIST_QUERY_LIMIT;
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

    if (m_pFilters.isNull()) {
        m_pFilters.reset(new NoteFilters);
    }

    connectToLocalStorage();
    requestNotesListAndCount();
}

void NoteModel::stop(const StopMode::type stopMode)
{
    NMDEBUG("NoteModel::stop: mode = " << stopMode);

    if (!m_isStarted) {
        NMDEBUG("Already stopped");
        return;
    }

    m_isStarted = false;
    disconnectFromLocalStorage();
    clearModel();
}

void NoteModel::onAddNoteComplete(Note note, QUuid requestId)
{
    NMDEBUG("NoteModel::onAddNoteComplete: " << note
            << "\nRequest id = " << requestId);

    bool noteIncluded = false;
    if (note.hasDeletionTimestamp()) {
        noteIncluded |= (m_includedNotes != IncludedNotes::NonDeleted);
    }
    else {
        noteIncluded |= (m_includedNotes != IncludedNotes::Deleted);
    }

    if (noteIncluded && (m_getFullNoteCountPerAccountRequestId == QUuid())) {
        ++m_totalAccountNotesCount;
        NMTRACE("Note count per account increased to "
                << m_totalAccountNotesCount);
        Q_EMIT noteCountPerAccountUpdated(m_totalAccountNotesCount);
    }

    if (noteIncluded &&
        (m_getNoteCountRequestId == QUuid()) &&
        noteConformsToFilter(note))
    {
        ++m_totalFilteredNotesCount;
        NMTRACE("Filtered notes count increased to "
                << m_totalFilteredNotesCount);
        Q_EMIT filteredNotesCountUpdated(m_totalFilteredNotesCount);
    }

    auto it = m_addNoteRequestIds.find(requestId);
    if (it != m_addNoteRequestIds.end()) {
        Q_UNUSED(m_addNoteRequestIds.erase(it))
        return;
    }

    onNoteAddedOrUpdated(note);
}

void NoteModel::onAddNoteFailed(
    Note note, ErrorString errorDescription, QUuid requestId)
{
    auto it = m_addNoteRequestIds.find(requestId);
    if (it == m_addNoteRequestIds.end()) {
        return;
    }

    NMDEBUG("NoteModel::onAddNoteFailed: note = " << note
            << "\nError description = " << errorDescription
            << ", request id = " << requestId);

    Q_UNUSED(m_addNoteRequestIds.erase(it))

    Q_EMIT notifyError(errorDescription);
    removeItemByLocalUid(note.localUid());
}

void NoteModel::onUpdateNoteComplete(
    Note note, LocalStorageManager::UpdateNoteOptions options, QUuid requestId)
{
    NMDEBUG("NoteModel::onUpdateNoteComplete: note = " << note
            << "\nRequest id = " << requestId);

    bool shouldRemoveNoteFromModel =
        (note.hasDeletionTimestamp() &&
         (m_includedNotes == IncludedNotes::NonDeleted));

    shouldRemoveNoteFromModel |=
        (!note.hasDeletionTimestamp() &&
         (m_includedNotes == IncludedNotes::Deleted));

    if (shouldRemoveNoteFromModel) {
        removeItemByLocalUid(note.localUid());
    }

    auto it = m_updateNoteRequestIds.find(requestId);
    if (it != m_updateNoteRequestIds.end())
    {
        NMDEBUG("This update was initiated by the note model");
        Q_UNUSED(m_updateNoteRequestIds.erase(it))

        const NoteDataByLocalUid & localUidIndex = m_data.get<ByLocalUid>();
        auto itemIt = localUidIndex.find(note.localUid());
        if (itemIt != localUidIndex.end()) {
            const NoteModelItem & item = *itemIt;
            note.setTagLocalUids(item.tagLocalUids());
            note.setTagGuids(item.tagGuids());
            NMTRACE("Complemented the note with tag local uids and guids: "
                    << note);
        }

        m_cache.put(note.localUid(), note);
        return;
    }

    NMTRACE("This update was not initiated by the note model: "
            << note << ", request id = " << requestId
            << ", update tags = "
            << ((options & LocalStorageManager::UpdateNoteOption::UpdateTags)
                ? "true"
                : "false")
            << ", should remove note from model = "
            << (shouldRemoveNoteFromModel
                ? "true"
                : "false"));

    if (!shouldRemoveNoteFromModel)
    {
        if (!(options & LocalStorageManager::UpdateNoteOption::UpdateTags))
        {
            const NoteDataByLocalUid & localUidIndex = m_data.get<ByLocalUid>();
            auto noteItemIt = localUidIndex.find(note.localUid());
            if (noteItemIt != localUidIndex.end()) {
                const NoteModelItem & item = *noteItemIt;
                note.setTagGuids(item.tagGuids());
                note.setTagLocalUids(item.tagLocalUids());
                NMTRACE("Complemented the note with tag local uids and guids: "
                        << note);
            }
        }

        onNoteAddedOrUpdated(note);
    }
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

    NMDEBUG("NoteModel::onUpdateNoteFailed: note = " << note
            << "\nError description = " << errorDescription
            << ", request id = " << requestId);

    Q_UNUSED(m_updateNoteRequestIds.erase(it))

    findNoteToRestoreFailedUpdate(note);
}

void NoteModel::onFindNoteComplete(
    Note note, LocalStorageManager::GetNoteOptions options, QUuid requestId)
{
    Q_UNUSED(options)

    auto restoreUpdateIt = m_findNoteToRestoreFailedUpdateRequestIds.find(requestId);
    auto performUpdateIt = m_findNoteToPerformUpdateRequestIds.find(requestId);

    if ((restoreUpdateIt == m_findNoteToRestoreFailedUpdateRequestIds.end()) &&
        (performUpdateIt == m_findNoteToPerformUpdateRequestIds.end()))
    {
        return;
    }

    NMDEBUG("NoteModel::onFindNoteComplete: note = " << note
            << "\nRequest id = " << requestId);

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
            saveNoteInLocalStorage(*it);
        }
    }
}

void NoteModel::onFindNoteFailed(
    Note note, LocalStorageManager::GetNoteOptions options,
    ErrorString errorDescription, QUuid requestId)
{
    Q_UNUSED(options)

    auto restoreUpdateIt = m_findNoteToRestoreFailedUpdateRequestIds.find(requestId);
    auto performUpdateIt = m_findNoteToPerformUpdateRequestIds.find(requestId);
    if ((restoreUpdateIt == m_findNoteToRestoreFailedUpdateRequestIds.end()) &&
        (performUpdateIt == m_findNoteToPerformUpdateRequestIds.end()))
    {
        return;
    }

    NMDEBUG("NoteModel::onFindNoteFailed: note = " << note
            << "\nError description = " << errorDescription
            << ", request id = " << requestId);

    if (restoreUpdateIt != m_findNoteToRestoreFailedUpdateRequestIds.end()) {
        Q_UNUSED(m_findNoteToRestoreFailedUpdateRequestIds.erase(restoreUpdateIt))
    }
    else if (performUpdateIt != m_findNoteToPerformUpdateRequestIds.end()) {
        Q_UNUSED(m_findNoteToPerformUpdateRequestIds.erase(performUpdateIt))
    }

    Q_EMIT notifyError(errorDescription);
}

void NoteModel::onListNotesComplete(
    LocalStorageManager::ListObjectsOptions flag,
    LocalStorageManager::GetNoteOptions options,
    size_t limit, size_t offset,
    LocalStorageManager::ListNotesOrder::type order,
    LocalStorageManager::OrderDirection::type orderDirection,
    QString linkedNotebookGuid, QList<Note> foundNotes,
    QUuid requestId)
{
    if (requestId != m_listNotesRequestId) {
        return;
    }

    NMDEBUG("NoteModel::onListNotesComplete: flag = " << flag
            << ", with resource metadata = "
            << ((options & LocalStorageManager::GetNoteOption::WithResourceMetadata)
                ? "true"
                : "false")
            << ", with resource binary data = "
            << ((options & LocalStorageManager::GetNoteOption::WithResourceBinaryData)
                ? "true"
                : "false")
            << ", limit = " << limit << ", offset = "
            << offset << ", order = " << order
            << ", direction = " << orderDirection
            << ", linked notebook guid = " << linkedNotebookGuid
            << ", num found notes = " << foundNotes.size()
            << ", request id = " << requestId);

    onListNotesCompleteImpl(foundNotes);
}

void NoteModel::onListNotesFailed(
    LocalStorageManager::ListObjectsOptions flag,
    LocalStorageManager::GetNoteOptions options,
    size_t limit, size_t offset,
    LocalStorageManager::ListNotesOrder::type order,
    LocalStorageManager::OrderDirection::type orderDirection,
    QString linkedNotebookGuid, ErrorString errorDescription,
    QUuid requestId)
{
    if (requestId != m_listNotesRequestId) {
        return;
    }

    NMDEBUG("NoteModel::onListNotesFailed: flag = " << flag
            << ", with resource metadata = "
            << ((options & LocalStorageManager::GetNoteOption::WithResourceMetadata)
                ? "true"
                : "false")
            << ", with resource binary data = "
            << ((options & LocalStorageManager::GetNoteOption::WithResourceBinaryData)
                ? "true"
                : "false")
            << ", limit = " << limit
            << ", offset = " << offset
            << ", order = " << order
            << ", direction = " << orderDirection
            << ", linked notebook guid = " << linkedNotebookGuid
            << ", error description = " << errorDescription
            << ", request id = " << requestId);

    m_listNotesRequestId = QUuid();
    Q_EMIT notifyError(errorDescription);
}

void NoteModel::onListNotesPerNotebooksAndTagsComplete(
    QStringList notebookLocalUids,
    QStringList tagLocalUids,
    LocalStorageManager::GetNoteOptions options,
    LocalStorageManager::ListObjectsOptions flag,
    size_t limit, size_t offset,
    LocalStorageManager::ListNotesOrder::type order,
    LocalStorageManager::OrderDirection::type orderDirection,
    QList<Note> foundNotes, QUuid requestId)
{
    if (requestId != m_listNotesRequestId) {
        return;
    }

    NMDEBUG("NoteModel::onListNotesPerNotebooksAndTagsComplete: "
            << "flag = " << flag << ", with resource metadata = "
            << ((options & LocalStorageManager::GetNoteOption::WithResourceMetadata)
                ? "true"
                : "false")
            << ", with resource binary data = "
            << ((options & LocalStorageManager::GetNoteOption::WithResourceBinaryData)
                ? "true"
                : "false")
            << ", limit = " << limit << ", offset = "
            << offset << ", order = " << order
            << ", direction = " << orderDirection
            << ", notebook local uids: "
            << notebookLocalUids.join(QStringLiteral(", "))
            << ", tag local uids: "
            << tagLocalUids.join(QStringLiteral(", "))
            << ", num found notes = " << foundNotes.size()
            << ", request id = " << requestId);

    onListNotesCompleteImpl(foundNotes);
}

void NoteModel::onListNotesPerNotebooksAndTagsFailed(
    QStringList notebookLocalUids,
    QStringList tagLocalUids,
    LocalStorageManager::GetNoteOptions options,
    LocalStorageManager::ListObjectsOptions flag,
    size_t limit, size_t offset,
    LocalStorageManager::ListNotesOrder::type order,
    LocalStorageManager::OrderDirection::type orderDirection,
    ErrorString errorDescription, QUuid requestId)
{
    if (requestId != m_listNotesRequestId) {
        return;
    }

    NMDEBUG("NoteModel::onListNotesPerNotebooksAndTagsFailed: "
            << "flag = " << flag
            << ", with resource metadata = "
            << ((options & LocalStorageManager::GetNoteOption::WithResourceMetadata)
                ? "true"
                : "false")
            << ", with resource binary data = "
            << ((options & LocalStorageManager::GetNoteOption::WithResourceBinaryData)
                ? "true"
                : "false")
            << ", limit = " << limit
            << ", offset = " << offset
            << ", order = " << order
            << ", direction = " << orderDirection
            << ", notebook local uids: "
            << notebookLocalUids.join(QStringLiteral(", "))
            << ", tag local uids: "
            << tagLocalUids.join(QStringLiteral(", "))
            << ", error description = " << errorDescription
            << ", request id = " << requestId);

    m_listNotesRequestId = QUuid();
    Q_EMIT notifyError(errorDescription);
}

void NoteModel::onListNotesByLocalUidsComplete(
    QStringList noteLocalUids,
    LocalStorageManager::GetNoteOptions options,
    LocalStorageManager::ListObjectsOptions flag,
    size_t limit, size_t offset,
    LocalStorageManager::ListNotesOrder::type order,
    LocalStorageManager::OrderDirection::type orderDirection,
    QList<Note> foundNotes, QUuid requestId)
{
    if (requestId != m_listNotesRequestId) {
        return;
    }

    NMDEBUG("NoteModel::onListNotesByLocalUidsComplete: "
            << "flag = " << flag << ", with resource metadata = "
            << ((options & LocalStorageManager::GetNoteOption::WithResourceMetadata)
                ? "true"
                : "false")
            << ", with resource binary data = "
            << ((options & LocalStorageManager::GetNoteOption::WithResourceBinaryData)
                ? "true"
                : "false")
            << ", limit = " << limit << ", offset = "
            << offset << ", order = " << order
            << ", direction = " << orderDirection
            << ", note local uids: "
            << noteLocalUids.join(QStringLiteral(", "))
            << ", num found notes = " << foundNotes.size()
            << ", request id = " << requestId);

    onListNotesCompleteImpl(foundNotes);
}

void NoteModel::onListNotesByLocalUidsFailed(
    QStringList noteLocalUids,
    LocalStorageManager::GetNoteOptions options,
    LocalStorageManager::ListObjectsOptions flag,
    size_t limit, size_t offset,
    LocalStorageManager::ListNotesOrder::type order,
    LocalStorageManager::OrderDirection::type orderDirection,
    ErrorString errorDescription, QUuid requestId)
{
    if (requestId != m_listNotesRequestId) {
        return;
    }

    NMDEBUG("NoteModel::onListNotesByLocalUidsFailed: "
            << "flag = " << flag << ", with resource metadata = "
            << ((options & LocalStorageManager::GetNoteOption::WithResourceMetadata)
                ? "true"
                : "false")
            << ", with resource binary data = "
            << ((options & LocalStorageManager::GetNoteOption::WithResourceBinaryData)
                ? "true"
                : "false")
            << ", limit = " << limit
            << ", offset = " << offset
            << ", order = " << order
            << ", direction = " << orderDirection
            << ", note local uids: "
            << noteLocalUids.join(QStringLiteral(", "))
            << ", error description = " << errorDescription
            << ", request id = " << requestId);

    m_listNotesRequestId = QUuid();
    Q_EMIT notifyError(errorDescription);
}

void NoteModel::onGetNoteCountComplete(
    int noteCount, LocalStorageManager::NoteCountOptions options, QUuid requestId)
{
    Q_UNUSED(options)

    if (m_getFullNoteCountPerAccountRequestId == requestId)
    {
        NMDEBUG("NoteModel::onGetNoteCountComplete: received total "
                << "note count per account: " << noteCount);

        m_getFullNoteCountPerAccountRequestId = QUuid();

        m_totalAccountNotesCount = noteCount;
        Q_EMIT noteCountPerAccountUpdated(m_totalAccountNotesCount);

        return;
    }

    if (m_getNoteCountRequestId == requestId)
    {
        NMDEBUG("NoteModel::onGetNoteCountComplete: received "
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

    if (m_getFullNoteCountPerAccountRequestId == requestId)
    {
        NMWARNING("NoteModel::onGetNoteCountFailed: failed to get "
                  << "total note count per account: " << errorDescription);

        m_getFullNoteCountPerAccountRequestId = QUuid();

        m_totalAccountNotesCount = 0;
        Q_EMIT noteCountPerAccountUpdated(m_totalAccountNotesCount);

        Q_EMIT notifyError(errorDescription);
        return;
    }

    if (m_getNoteCountRequestId == requestId)
    {
        NMWARNING("NoteModel::onGetNoteCountFailed: failed to get "
                  << "filtered notes count: " << errorDescription);

        m_getNoteCountRequestId = QUuid();

        m_totalFilteredNotesCount = 0;
        Q_EMIT filteredNotesCountUpdated(m_totalFilteredNotesCount);

        Q_EMIT notifyError(errorDescription);
        return;
    }
}

void NoteModel::onGetNoteCountPerNotebooksAndTagsComplete(
    int noteCount, QStringList notebookLocalUids, QStringList tagLocalUids,
    LocalStorageManager::NoteCountOptions options, QUuid requestId)
{
    Q_UNUSED(options)

    if (m_getNoteCountRequestId != requestId) {
        return;
    }

    NMDEBUG("NoteModel::onGetNoteCountPerNotebooksAndTagsComplete: "
            << " note count = " << noteCount
            << ", notebook local uids: "
            << notebookLocalUids.join(QStringLiteral(", "))
            << ", tag local uids: "
            << tagLocalUids.join(QStringLiteral(", ")));

    m_getNoteCountRequestId = QUuid();

    m_totalFilteredNotesCount = noteCount;
    Q_EMIT filteredNotesCountUpdated(m_totalFilteredNotesCount);
}

void NoteModel::onGetNoteCountPerNotebooksAndTagsFailed(
    ErrorString errorDescription,
    QStringList notebookLocalUids, QStringList tagLocalUids,
    LocalStorageManager::NoteCountOptions options,
    QUuid requestId)
{
    Q_UNUSED(options)

    if (m_getNoteCountRequestId != requestId) {
        return;
    }

    NMWARNING("NoteModel::onGetNoteCountPerNotebooksAndTagsFailed: "
              << errorDescription << ", notebook local uids: "
              << notebookLocalUids.join(QStringLiteral(", "))
              << ", tag local uids: "
              << tagLocalUids.join(QStringLiteral(", ")));

    m_getNoteCountRequestId = QUuid();

    m_totalFilteredNotesCount = 0;
    Q_EMIT filteredNotesCountUpdated(m_totalFilteredNotesCount);

    Q_EMIT notifyError(errorDescription);
}

void NoteModel::onExpungeNoteComplete(Note note, QUuid requestId)
{
    NMTRACE("NoteModel::onExpungeNoteComplete: note = " << note
            << "\nRequest id = " << requestId);

    if (m_getFullNoteCountPerAccountRequestId == QUuid()) {
        requestTotalNotesCountPerAccount();
    }

    if (m_getNoteCountRequestId == QUuid()) {
        requestTotalFilteredNotesCount();
    }

    auto it = m_expungeNoteRequestIds.find(requestId);
    if (it != m_expungeNoteRequestIds.end()) {
        Q_UNUSED(m_expungeNoteRequestIds.erase(it))
        return;
    }

    removeItemByLocalUid(note.localUid());
}

void NoteModel::onExpungeNoteFailed(
    Note note, ErrorString errorDescription, QUuid requestId)
{
    auto it = m_expungeNoteRequestIds.find(requestId);
    if (it == m_expungeNoteRequestIds.end()) {
        return;
    }

    NMTRACE("NoteModel::onExpungeNoteFailed: note = " << note
            << "\nError description = " << errorDescription
            << ", request id = " << requestId);

    Q_UNUSED(m_expungeNoteRequestIds.erase(it))

    findNoteToRestoreFailedUpdate(note);
}

void NoteModel::onFindNotebookComplete(Notebook notebook, QUuid requestId)
{
    auto fit = m_findNotebookRequestForNotebookLocalUid.right.find(requestId);
    auto mit =
        ((fit == m_findNotebookRequestForNotebookLocalUid.right.end())
         ? m_noteLocalUidToFindNotebookRequestIdForMoveNoteToNotebookBimap.right.end()
         : m_noteLocalUidToFindNotebookRequestIdForMoveNoteToNotebookBimap.right.find(requestId));

    if ( (fit == m_findNotebookRequestForNotebookLocalUid.right.end()) &&
         (mit == m_noteLocalUidToFindNotebookRequestIdForMoveNoteToNotebookBimap.right.end()) )
    {
        return;
    }

    NMTRACE("NoteModel::onFindNotebookComplete: notebook: "
            << notebook << "\nRequest id = " << requestId);

    m_notebookCache.put(notebook.localUid(), notebook);

    if (fit != m_findNotebookRequestForNotebookLocalUid.right.end())
    {
        Q_UNUSED(m_findNotebookRequestForNotebookLocalUid.right.erase(fit))
        updateNotebookData(notebook);
    }
    else if (mit != m_noteLocalUidToFindNotebookRequestIdForMoveNoteToNotebookBimap.right.end())
    {
        QString noteLocalUid = mit->second;
        Q_UNUSED(m_noteLocalUidToFindNotebookRequestIdForMoveNoteToNotebookBimap.right.erase(mit))

        NoteDataByLocalUid & localUidIndex = m_data.get<ByLocalUid>();
        auto it = localUidIndex.find(noteLocalUid);
        if (it == localUidIndex.end()) {
            REPORT_ERROR(QT_TR_NOOP("Can't move the note to another notebook: "
                                    "internal error, can't find the item "
                                    "within the note model by local uid"));
            return;
        }

        ErrorString error;
        if (!moveNoteToNotebookImpl(it, notebook, error)) {
            ErrorString errorDescription(QT_TR_NOOP("Can't move note to another notebook"));
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
    auto fit = m_findNotebookRequestForNotebookLocalUid.right.find(requestId);
    auto mit =
        ((fit == m_findNotebookRequestForNotebookLocalUid.right.end())
         ? m_noteLocalUidToFindNotebookRequestIdForMoveNoteToNotebookBimap.right.end()
         : m_noteLocalUidToFindNotebookRequestIdForMoveNoteToNotebookBimap.right.find(requestId));

    if ( (fit == m_findNotebookRequestForNotebookLocalUid.right.end()) &&
         (mit == m_noteLocalUidToFindNotebookRequestIdForMoveNoteToNotebookBimap.right.end()) )
    {
        return;
    }

    NMWARNING("NoteModel::onFindNotebookFailed: notebook = " << notebook
              << "\nError description = " << errorDescription
              << ", request id = " << requestId);

    if (fit != m_findNotebookRequestForNotebookLocalUid.right.end())
    {
        Q_UNUSED(m_findNotebookRequestForNotebookLocalUid.right.erase(fit))
    }
    else if (mit != m_noteLocalUidToFindNotebookRequestIdForMoveNoteToNotebookBimap.right.end())
    {
        Q_UNUSED(m_noteLocalUidToFindNotebookRequestIdForMoveNoteToNotebookBimap.right.erase(mit))

        ErrorString error(QT_TR_NOOP("Can't move the note to another notebook: "
                                     "failed to find the target notebook"));
        error.appendBase(errorDescription.base());
        error.appendBase(errorDescription.additionalBases());
        error.details() = errorDescription.details();
        NMDEBUG(error);
        Q_EMIT notifyError(error);
    }
}

void NoteModel::onAddNotebookComplete(Notebook notebook, QUuid requestId)
{
    NMDEBUG("NoteModel::onAddNotebookComplete: local uid = "
            << notebook.localUid());
    Q_UNUSED(requestId)
    m_notebookCache.put(notebook.localUid(), notebook);
    updateNotebookData(notebook);
}

void NoteModel::onUpdateNotebookComplete(Notebook notebook, QUuid requestId)
{
    NMTRACE("NoteModel::onUpdateNotebookComplete: local uid = "
            << notebook.localUid());
    Q_UNUSED(requestId)
    m_notebookCache.put(notebook.localUid(), notebook);
    updateNotebookData(notebook);
}

void NoteModel::onExpungeNotebookComplete(Notebook notebook, QUuid requestId)
{
    NMTRACE("NoteModel::onExpungeNotebookComplete: local uid = "
            << notebook.localUid());

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

    NMTRACE("NoteModel::onFindTagComplete: tag: " << tag
            << "\nRequest id = " << requestId);

    Q_UNUSED(m_findTagRequestForTagLocalUid.right.erase(it))
    updateTagData(tag);
}

void NoteModel::onFindTagFailed(
    Tag tag, ErrorString errorDescription, QUuid requestId)
{
    auto it = m_findTagRequestForTagLocalUid.right.find(requestId);
    if (it == m_findTagRequestForTagLocalUid.right.end()) {
        return;
    }

    NMWARNING("NoteModel::onFindTagFailed: tag: " << tag
              << "\nError description = " << errorDescription
              << ", request id = " << requestId);

    Q_UNUSED(m_findTagRequestForTagLocalUid.right.erase(it))
    Q_EMIT notifyError(errorDescription);
}

void NoteModel::onAddTagComplete(Tag tag, QUuid requestId)
{
    NMTRACE("NoteModel::onAddTagComplete: tag = " << tag
            << ", request id = " << requestId);
    updateTagData(tag);
}

void NoteModel::onUpdateTagComplete(Tag tag, QUuid requestId)
{
    NMTRACE("NoteModel::onUpdateTagComplete: tag = " << tag
            << ", request id = " << requestId);
    updateTagData(tag);
}

void NoteModel::onExpungeTagComplete(
    Tag tag, QStringList expungedChildTagLocalUids, QUuid requestId)
{
    NMTRACE("NoteModel::onExpungeTagComplete: tag = " << tag
            << "\nExpunged child tag local uids = "
            << expungedChildTagLocalUids.join(QStringLiteral(", "))
            << ", request id = " << requestId);

    QStringList expungedTagLocalUids;
    expungedTagLocalUids << tag.localUid();
    expungedTagLocalUids << expungedChildTagLocalUids;
    for(auto it = expungedTagLocalUids.constBegin(),
        end = expungedTagLocalUids.constEnd(); it != end; ++it)
    {
        processTagExpunging(*it);
    }
}

void NoteModel::connectToLocalStorage()
{
    NMDEBUG("NoteModel::connectToLocalStorage");

    if (m_connectedToLocalStorage) {
        NMDEBUG("Already connected to local storage");
        return;
    }

    // Local signals to LocalStorageManagerAsync's slots
    QObject::connect(this,
                     QNSIGNAL(NoteModel,addNote,Note,QUuid),
                     &m_localStorageManagerAsync,
                     QNSLOT(LocalStorageManagerAsync,
                            onAddNoteRequest,Note,QUuid));
    QObject::connect(this,
                     QNSIGNAL(NoteModel,updateNote,
                              Note,LocalStorageManager::UpdateNoteOptions,QUuid),
                     &m_localStorageManagerAsync,
                     QNSLOT(LocalStorageManagerAsync,onUpdateNoteRequest,
                            Note,LocalStorageManager::UpdateNoteOptions,QUuid));
    QObject::connect(this,
                     QNSIGNAL(NoteModel,findNote,
                              Note,LocalStorageManager::GetNoteOptions,QUuid),
                     &m_localStorageManagerAsync,
                     QNSLOT(LocalStorageManagerAsync,onFindNoteRequest,
                            Note,LocalStorageManager::GetNoteOptions,QUuid));
    QObject::connect(this,
                     QNSIGNAL(NoteModel,listNotes,
                              LocalStorageManager::ListObjectsOptions,
                              LocalStorageManager::GetNoteOptions,size_t,size_t,
                              LocalStorageManager::ListNotesOrder::type,
                              LocalStorageManager::OrderDirection::type,
                              QString,QUuid),
                     &m_localStorageManagerAsync,
                     QNSLOT(LocalStorageManagerAsync,onListNotesRequest,
                            LocalStorageManager::ListObjectsOptions,
                            LocalStorageManager::GetNoteOptions,size_t,size_t,
                            LocalStorageManager::ListNotesOrder::type,
                            LocalStorageManager::OrderDirection::type,
                            QString,QUuid));
    QObject::connect(this,
                     QNSIGNAL(NoteModel,listNotesPerNotebooksAndTags,
                              QStringList,QStringList,
                              LocalStorageManager::GetNoteOptions,
                              LocalStorageManager::ListObjectsOptions,
                              size_t,size_t,
                              LocalStorageManager::ListNotesOrder::type,
                              LocalStorageManager::OrderDirection::type,QUuid),
                     &m_localStorageManagerAsync,
                     QNSLOT(LocalStorageManagerAsync,
                            onListNotesPerNotebooksAndTagsRequest,
                            QStringList,QStringList,
                            LocalStorageManager::GetNoteOptions,
                            LocalStorageManager::ListObjectsOptions,
                            size_t,size_t,
                            LocalStorageManager::ListNotesOrder::type,
                            LocalStorageManager::OrderDirection::type,QUuid));
    QObject::connect(this,
                     QNSIGNAL(NoteModel,listNotesByLocalUids,
                              QStringList,LocalStorageManager::GetNoteOptions,
                              LocalStorageManager::ListObjectsOptions,
                              size_t,size_t,
                              LocalStorageManager::ListNotesOrder::type,
                              LocalStorageManager::OrderDirection::type,QUuid),
                     &m_localStorageManagerAsync,
                     QNSLOT(LocalStorageManagerAsync,
                            onListNotesByLocalUidsRequest,
                            QStringList,LocalStorageManager::GetNoteOptions,
                            LocalStorageManager::ListObjectsOptions,
                            size_t,size_t,
                            LocalStorageManager::ListNotesOrder::type,
                            LocalStorageManager::OrderDirection::type,QUuid));
    QObject::connect(this,
                     QNSIGNAL(NoteModel,getNoteCount,
                              LocalStorageManager::NoteCountOptions,QUuid),
                     &m_localStorageManagerAsync,
                     QNSLOT(LocalStorageManagerAsync,onGetNoteCountRequest,
                            LocalStorageManager::NoteCountOptions,QUuid));
    QObject::connect(this,
                     QNSIGNAL(NoteModel,getNoteCountPerNotebooksAndTags,
                              QStringList,QStringList,
                              LocalStorageManager::NoteCountOptions,QUuid),
                     &m_localStorageManagerAsync,
                     QNSLOT(LocalStorageManagerAsync,
                            onGetNoteCountPerNotebooksAndTagsRequest,
                            QStringList,QStringList,
                            LocalStorageManager::NoteCountOptions,QUuid));
    QObject::connect(this,
                     QNSIGNAL(NoteModel,expungeNote,Note,QUuid),
                     &m_localStorageManagerAsync,
                     QNSLOT(LocalStorageManagerAsync,onExpungeNoteRequest,
                            Note,QUuid));
    QObject::connect(this,
                     QNSIGNAL(NoteModel,findNotebook,Notebook,QUuid),
                     &m_localStorageManagerAsync,
                     QNSLOT(LocalStorageManagerAsync,onFindNotebookRequest,
                            Notebook,QUuid));
    QObject::connect(this,
                     QNSIGNAL(NoteModel,findTag,Tag,QUuid),
                     &m_localStorageManagerAsync,
                     QNSLOT(LocalStorageManagerAsync,onFindTagRequest,Tag,QUuid));

    // LocalStorageManagerAsync's signals to local slots
    QObject::connect(&m_localStorageManagerAsync,
                     QNSIGNAL(LocalStorageManagerAsync,addNoteComplete,
                              Note,QUuid),
                     this,
                     QNSLOT(NoteModel,onAddNoteComplete,Note,QUuid));
    QObject::connect(&m_localStorageManagerAsync,
                     QNSIGNAL(LocalStorageManagerAsync,addNoteFailed,
                              Note,ErrorString,QUuid),
                     this,
                     QNSLOT(NoteModel,onAddNoteFailed,Note,ErrorString,QUuid));
    QObject::connect(&m_localStorageManagerAsync,
                     QNSIGNAL(LocalStorageManagerAsync,updateNoteComplete,
                              Note,LocalStorageManager::UpdateNoteOptions,QUuid),
                     this,
                     QNSLOT(NoteModel,onUpdateNoteComplete,
                            Note,LocalStorageManager::UpdateNoteOptions,QUuid));
    QObject::connect(&m_localStorageManagerAsync,
                     QNSIGNAL(LocalStorageManagerAsync,updateNoteFailed,
                              Note,LocalStorageManager::UpdateNoteOptions,
                              ErrorString,QUuid),
                     this,
                     QNSLOT(NoteModel,onUpdateNoteFailed,
                            Note,LocalStorageManager::UpdateNoteOptions,
                            ErrorString,QUuid));
    QObject::connect(&m_localStorageManagerAsync,
                     QNSIGNAL(LocalStorageManagerAsync,findNoteComplete,
                              Note,LocalStorageManager::GetNoteOptions,QUuid),
                     this,
                     QNSLOT(NoteModel,onFindNoteComplete,
                            Note,LocalStorageManager::GetNoteOptions,QUuid));
    QObject::connect(&m_localStorageManagerAsync,
                     QNSIGNAL(LocalStorageManagerAsync,findNoteFailed,
                              Note,LocalStorageManager::GetNoteOptions,
                              ErrorString,QUuid),
                     this,
                     QNSLOT(NoteModel,onFindNoteFailed,
                            Note,LocalStorageManager::GetNoteOptions,
                            ErrorString,QUuid));
    QObject::connect(&m_localStorageManagerAsync,
                     QNSIGNAL(LocalStorageManagerAsync,listNotesComplete,
                              LocalStorageManager::ListObjectsOptions,
                              LocalStorageManager::GetNoteOptions,size_t,size_t,
                              LocalStorageManager::ListNotesOrder::type,
                              LocalStorageManager::OrderDirection::type,
                              QString,QList<Note>,QUuid),
                     this,
                     QNSLOT(NoteModel,onListNotesComplete,
                            LocalStorageManager::ListObjectsOptions,
                            LocalStorageManager::GetNoteOptions,size_t,size_t,
                            LocalStorageManager::ListNotesOrder::type,
                            LocalStorageManager::OrderDirection::type,
                            QString,QList<Note>,QUuid));
    QObject::connect(&m_localStorageManagerAsync,
                     QNSIGNAL(LocalStorageManagerAsync,listNotesFailed,
                              LocalStorageManager::ListObjectsOptions,
                              LocalStorageManager::GetNoteOptions,size_t,size_t,
                              LocalStorageManager::ListNotesOrder::type,
                              LocalStorageManager::OrderDirection::type,
                              QString,ErrorString,QUuid),
                     this,
                     QNSLOT(NoteModel,onListNotesFailed,
                            LocalStorageManager::ListObjectsOptions,
                            LocalStorageManager::GetNoteOptions,size_t,size_t,
                            LocalStorageManager::ListNotesOrder::type,
                            LocalStorageManager::OrderDirection::type,
                            QString,ErrorString,QUuid));
    QObject::connect(&m_localStorageManagerAsync,
                     QNSIGNAL(LocalStorageManagerAsync,
                              listNotesPerNotebooksAndTagsComplete,
                              QStringList,QStringList,
                              LocalStorageManager::GetNoteOptions,
                              LocalStorageManager::ListObjectsOptions,
                              size_t,size_t,
                              LocalStorageManager::ListNotesOrder::type,
                              LocalStorageManager::OrderDirection::type,
                              QList<Note>,QUuid),
                     this,
                     QNSLOT(NoteModel,onListNotesPerNotebooksAndTagsComplete,
                            QStringList,QStringList,
                            LocalStorageManager::GetNoteOptions,
                            LocalStorageManager::ListObjectsOptions,
                            size_t,size_t,
                            LocalStorageManager::ListNotesOrder::type,
                            LocalStorageManager::OrderDirection::type,
                            QList<Note>,QUuid));
    QObject::connect(&m_localStorageManagerAsync,
                     QNSIGNAL(LocalStorageManagerAsync,
                              listNotesPerNotebooksAndTagsFailed,
                              QStringList,QStringList,
                              LocalStorageManager::GetNoteOptions,
                              LocalStorageManager::ListObjectsOptions,
                              size_t,size_t,
                              LocalStorageManager::ListNotesOrder::type,
                              LocalStorageManager::OrderDirection::type,
                              ErrorString,QUuid),
                     this,
                     QNSLOT(NoteModel,onListNotesPerNotebooksAndTagsFailed,
                            QStringList,QStringList,
                            LocalStorageManager::GetNoteOptions,
                            LocalStorageManager::ListObjectsOptions,
                            size_t,size_t,
                            LocalStorageManager::ListNotesOrder::type,
                            LocalStorageManager::OrderDirection::type,
                            ErrorString,QUuid));
    QObject::connect(&m_localStorageManagerAsync,
                     QNSIGNAL(LocalStorageManagerAsync,
                              listNotesByLocalUidsComplete,
                              QStringList,LocalStorageManager::GetNoteOptions,
                              LocalStorageManager::ListObjectsOptions,
                              size_t,size_t,
                              LocalStorageManager::ListNotesOrder::type,
                              LocalStorageManager::OrderDirection::type,
                              QList<Note>,QUuid),
                     this,
                     QNSLOT(NoteModel,onListNotesByLocalUidsComplete,
                            QStringList,LocalStorageManager::GetNoteOptions,
                            LocalStorageManager::ListObjectsOptions,
                            size_t,size_t,
                            LocalStorageManager::ListNotesOrder::type,
                            LocalStorageManager::OrderDirection::type,
                            QList<Note>,QUuid));
    QObject::connect(&m_localStorageManagerAsync,
                     QNSIGNAL(LocalStorageManagerAsync,
                              listNotesByLocalUidsFailed,
                              QStringList,LocalStorageManager::GetNoteOptions,
                              LocalStorageManager::ListObjectsOptions,
                              size_t,size_t,
                              LocalStorageManager::ListNotesOrder::type,
                              LocalStorageManager::OrderDirection::type,
                              ErrorString,QUuid),
                     this,
                     QNSLOT(NoteModel,onListNotesByLocalUidsFailed,
                            QStringList,LocalStorageManager::GetNoteOptions,
                            LocalStorageManager::ListObjectsOptions,
                            size_t,size_t,
                            LocalStorageManager::ListNotesOrder::type,
                            LocalStorageManager::OrderDirection::type,
                            ErrorString,QUuid));
    QObject::connect(&m_localStorageManagerAsync,
                     QNSIGNAL(LocalStorageManagerAsync,getNoteCountComplete,
                              int,LocalStorageManager::NoteCountOptions,QUuid),
                     this,
                     QNSLOT(NoteModel,onGetNoteCountComplete,
                            int,LocalStorageManager::NoteCountOptions,QUuid));
    QObject::connect(&m_localStorageManagerAsync,
                     QNSIGNAL(LocalStorageManagerAsync,getNoteCountFailed,
                              ErrorString,LocalStorageManager::NoteCountOptions,
                              QUuid),
                     this,
                     QNSLOT(NoteModel,onGetNoteCountFailed,
                            ErrorString,LocalStorageManager::NoteCountOptions,
                            QUuid));
    QObject::connect(&m_localStorageManagerAsync,
                     QNSIGNAL(LocalStorageManagerAsync,
                              getNoteCountPerNotebooksAndTagsComplete,
                              int,QStringList,QStringList,
                              LocalStorageManager::NoteCountOptions,QUuid),
                     this,
                     QNSLOT(NoteModel,onGetNoteCountPerNotebooksAndTagsComplete,
                            int,QStringList,QStringList,
                            LocalStorageManager::NoteCountOptions,QUuid));
    QObject::connect(&m_localStorageManagerAsync,
                     QNSIGNAL(LocalStorageManagerAsync,
                              getNoteCountPerNotebooksAndTagsFailed,
                              ErrorString,QStringList,QStringList,
                              LocalStorageManager::NoteCountOptions,QUuid),
                     this,
                     QNSLOT(NoteModel,onGetNoteCountPerNotebooksAndTagsFailed,
                            ErrorString,QStringList,QStringList,
                            LocalStorageManager::NoteCountOptions,QUuid));
    QObject::connect(&m_localStorageManagerAsync,
                     QNSIGNAL(LocalStorageManagerAsync,expungeNoteComplete,
                              Note,QUuid),
                     this,
                     QNSLOT(NoteModel,onExpungeNoteComplete,Note,QUuid));
    QObject::connect(&m_localStorageManagerAsync,
                     QNSIGNAL(LocalStorageManagerAsync,expungeNoteFailed,
                              Note,ErrorString,QUuid),
                     this,
                     QNSLOT(NoteModel,onExpungeNoteFailed,
                            Note,ErrorString,QUuid));
    QObject::connect(&m_localStorageManagerAsync,
                     QNSIGNAL(LocalStorageManagerAsync,findNotebookComplete,
                              Notebook,QUuid),
                     this,
                     QNSLOT(NoteModel,onFindNotebookComplete,Notebook,QUuid));
    QObject::connect(&m_localStorageManagerAsync,
                     QNSIGNAL(LocalStorageManagerAsync,findNotebookFailed,
                              Notebook,ErrorString,QUuid),
                     this,
                     QNSLOT(NoteModel,onFindNotebookFailed,
                            Notebook,ErrorString,QUuid));
    QObject::connect(&m_localStorageManagerAsync,
                     QNSIGNAL(LocalStorageManagerAsync,addNotebookComplete,
                              Notebook,QUuid),
                     this,
                     QNSLOT(NoteModel,onAddNotebookComplete,Notebook,QUuid));
    QObject::connect(&m_localStorageManagerAsync,
                     QNSIGNAL(LocalStorageManagerAsync,updateNotebookComplete,
                              Notebook,QUuid),
                     this,
                     QNSLOT(NoteModel,onUpdateNotebookComplete,Notebook,QUuid));
    QObject::connect(&m_localStorageManagerAsync,
                     QNSIGNAL(LocalStorageManagerAsync,expungeNotebookComplete,
                              Notebook,QUuid),
                     this,
                     QNSLOT(NoteModel,onExpungeNotebookComplete,Notebook,QUuid));
    QObject::connect(&m_localStorageManagerAsync,
                     QNSIGNAL(LocalStorageManagerAsync,findTagComplete,Tag,QUuid),
                     this,
                     QNSLOT(NoteModel,onFindTagComplete,Tag,QUuid));
    QObject::connect(&m_localStorageManagerAsync,
                     QNSIGNAL(LocalStorageManagerAsync,findTagFailed,
                              Tag,ErrorString,QUuid),
                     this,
                     QNSLOT(NoteModel,onFindTagFailed,Tag,ErrorString,QUuid));
    QObject::connect(&m_localStorageManagerAsync,
                     QNSIGNAL(LocalStorageManagerAsync,addTagComplete,Tag,QUuid),
                     this,
                     QNSLOT(NoteModel,onAddTagComplete,Tag,QUuid));
    QObject::connect(&m_localStorageManagerAsync,
                     QNSIGNAL(LocalStorageManagerAsync,updateTagComplete,
                              Tag,QUuid),
                     this,
                     QNSLOT(NoteModel,onUpdateTagComplete,Tag,QUuid));
    QObject::connect(&m_localStorageManagerAsync,
                     QNSIGNAL(LocalStorageManagerAsync,expungeTagComplete,
                              Tag,QStringList,QUuid),
                     this,
                     QNSLOT(NoteModel,onExpungeTagComplete,Tag,QStringList,QUuid));

    m_connectedToLocalStorage = true;
}

void NoteModel::disconnectFromLocalStorage()
{
    NMDEBUG("NoteModel::disconnectFromLocalStorage");

    if (!m_connectedToLocalStorage) {
        NMDEBUG("Already disconnected from local storage");
        return;
    }

    QObject::disconnect(&m_localStorageManagerAsync);
    m_localStorageManagerAsync.disconnect(this);
    m_connectedToLocalStorage = false;
}

void NoteModel::onNoteAddedOrUpdated(
    const Note & note, const bool fromNotesListing)
{
    if (!note.hasNotebookLocalUid()) {
        NMWARNING("Skipping the note not having the notebook local uid: "
                  << note);
        return;
    }

    if (!fromNotesListing && !noteConformsToFilter(note)) {
        NMDEBUG("Skipping the note not conforming to "
                << "the specified filter: " << note);
        return;
    }

    NoteModelItem item;
    noteToItem(note, item);

    auto notebookIt = m_notebookDataByNotebookLocalUid.find(item.notebookLocalUid());
    if (notebookIt == m_notebookDataByNotebookLocalUid.end())
    {
        bool findNotebookRequestSent = false;

        Q_UNUSED(m_noteItemsPendingNotebookDataUpdate.insert(
            item.notebookLocalUid(),
            item))

        auto it = m_findNotebookRequestForNotebookLocalUid.left.find(
            item.notebookLocalUid());
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
            Q_UNUSED(m_findNotebookRequestForNotebookLocalUid.insert(
                    LocalUidToRequestIdBimap::value_type(item.notebookLocalUid(),
                                                         requestId)))
            NMTRACE("Emitting the request to find notebook local uid: = "
                    << item.notebookLocalUid() << ", request id = "
                    << requestId);
            Q_EMIT findNotebook(notebook, requestId);
        }
        else
        {
            NMTRACE("The request to find notebook for this note "
                    "has already been sent");
        }

        return;
    }

    const NotebookData & notebookData = notebookIt.value();
    addOrUpdateNoteItem(item, notebookData, fromNotesListing);
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

    item.setThumbnailData(note.thumbnailData());

    if (note.hasTagLocalUids())
    {
        const QStringList & tagLocalUids = note.tagLocalUids();
        item.setTagLocalUids(tagLocalUids);

        QStringList tagNames;
        tagNames.reserve(tagLocalUids.size());

        for(auto it = tagLocalUids.constBegin(),
            end = tagLocalUids.constEnd(); it != end; ++it)
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
    item.setActive(note.hasActive() ? note.active() : true);
    item.setHasResources(note.hasResources() && (note.numResources() > 0));

    if (note.hasNoteRestrictions())
    {
        const qevercloud::NoteRestrictions & restrictions = note.noteRestrictions();
        item.setCanUpdateTitle(!restrictions.noUpdateTitle.isSet() ||
                               !restrictions.noUpdateTitle.ref());
        item.setCanUpdateContent(!restrictions.noUpdateContent.isSet() ||
                                 !restrictions.noUpdateContent.ref());
        item.setCanEmail(!restrictions.noEmail.isSet() ||
                         !restrictions.noEmail.ref());
        item.setCanShare(!restrictions.noShare.isSet() ||
                         !restrictions.noShare.ref());
        item.setCanSharePublicly(!restrictions.noSharePublicly.isSet() ||
                                 !restrictions.noSharePublicly.ref());
    }
    else
    {
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
        for(auto it = resources.constBegin(),
            end = resources.constEnd(); it != end; ++it)
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

bool NoteModel::noteConformsToFilter(const Note & note) const
{
    if (Q_UNLIKELY(!note.hasNotebookLocalUid())) {
        return false;
    }

    if (!m_pFilters) {
        return true;
    }

    const QSet<QString> & filteredNoteLocalUids = m_pFilters->filteredNoteLocalUids();
    if (!filteredNoteLocalUids.isEmpty() &&
        !filteredNoteLocalUids.contains(note.localUid()))
    {
        return false;
    }

    const QStringList & filteredNotebookLocalUids =
        m_pFilters->filteredNotebookLocalUids();
    if (!filteredNotebookLocalUids.isEmpty() &&
        !filteredNotebookLocalUids.contains(note.notebookLocalUid()))
    {
        return false;
    }

    const QStringList & filteredTagLocalUids = m_pFilters->filteredTagLocalUids();
    if (!filteredTagLocalUids.isEmpty())
    {
        bool foundTag = false;
        const QStringList & tagLocalUids = note.tagLocalUids();
        for(auto it = tagLocalUids.constBegin(),
            end = tagLocalUids.end(); it != end; ++it)
        {
            if (filteredTagLocalUids.contains(*it)) {
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

void NoteModel::onListNotesCompleteImpl(const QList<Note> foundNotes)
{
    bool fromNotesListing = true;
    for(auto it = foundNotes.begin(), end = foundNotes.end(); it != end; ++it) {
        onNoteAddedOrUpdated(*it, fromNotesListing);
    }
    m_listNotesOffset += static_cast<size_t>(foundNotes.size());

    m_listNotesRequestId = QUuid();

    if (!foundNotes.isEmpty() && (m_data.size() < NOTE_MIN_CACHE_SIZE)) {
        NMTRACE("The number of found notes is greater than zero, "
                "requesting more notes from the local storage");
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

    LocalStorageManager::ListObjectsOptions flags =
        LocalStorageManager::ListAll;
    LocalStorageManager::ListNotesOrder::type order =
        LocalStorageManager::ListNotesOrder::ByModificationTimestamp;
    LocalStorageManager::OrderDirection::type direction =
        LocalStorageManager::OrderDirection::Ascending;

    switch(m_noteSortingMode)
    {
    case NoteSortingMode::CreatedAscending:
        order = LocalStorageManager::ListNotesOrder::ByCreationTimestamp;
        direction = LocalStorageManager::OrderDirection::Ascending;
        break;
    case NoteSortingMode::CreatedDescending:
        order = LocalStorageManager::ListNotesOrder::ByCreationTimestamp;
        direction = LocalStorageManager::OrderDirection::Descending;
        break;
    case NoteSortingMode::ModifiedAscending:
        order = LocalStorageManager::ListNotesOrder::ByModificationTimestamp;
        direction = LocalStorageManager::OrderDirection::Ascending;
        break;
    case NoteSortingMode::ModifiedDescending:
        order = LocalStorageManager::ListNotesOrder::ByModificationTimestamp;
        direction = LocalStorageManager::OrderDirection::Descending;
        break;
    case NoteSortingMode::TitleAscending:
        order = LocalStorageManager::ListNotesOrder::ByTitle;
        direction = LocalStorageManager::OrderDirection::Ascending;
        break;
    case NoteSortingMode::TitleDescending:
        order = LocalStorageManager::ListNotesOrder::ByTitle;
        direction = LocalStorageManager::OrderDirection::Descending;
        break;
    // NOTE: no sorting by side is supported by the local storage so leaving
    // it as is for now
    default:
        break;
    }

    m_listNotesRequestId = QUuid::createUuid();

    if (!hasFilters())
    {
        NMDEBUG("Emitting the request to list notes: offset = "
                << m_listNotesOffset << ", request id = "
                << m_listNotesRequestId << ", order = "
                << order << ", direction = " << direction);

        Q_EMIT listNotes(flags, LocalStorageManager::GetNoteOptions(0),
                         NOTE_LIST_QUERY_LIMIT, m_listNotesOffset, order,
                         direction, QString(), m_listNotesRequestId);
        return;
    }

    const QSet<QString> & filteredNoteLocalUids = m_pFilters->filteredNoteLocalUids();
    if (!filteredNoteLocalUids.isEmpty())
    {
        int end = static_cast<int>(m_listNotesOffset) + NOTE_LIST_QUERY_LIMIT;
        end = std::min(end, filteredNoteLocalUids.size());

        auto beginIt = filteredNoteLocalUids.begin();
        std::advance(beginIt, static_cast<int>(m_listNotesOffset));

        auto endIt = filteredNoteLocalUids.begin();
        std::advance(endIt, end);

        QStringList noteLocalUids;
        noteLocalUids.reserve(std::max((end - static_cast<int>(m_listNotesOffset)), 0));
        for(auto it = beginIt; it != endIt; ++it) {
            noteLocalUids << *it;
        }

        NMDEBUG("Emitting the request to list notes by local uids: "
                << ", request id = " << m_listNotesRequestId
                << ", order = " << order
                << ", direction = " << direction
                << ", note local uids: "
                << noteLocalUids.join(QStringLiteral(", ")));

        Q_EMIT listNotesByLocalUids(noteLocalUids,
                                    LocalStorageManager::GetNoteOptions(0),
                                    flags, NOTE_LIST_QUERY_LIMIT, 0, order, direction,
                                    m_listNotesRequestId);
        return;
    }

    const QStringList & notebookLocalUids = m_pFilters->filteredNotebookLocalUids();
    const QStringList & tagLocalUids = m_pFilters->filteredTagLocalUids();

    NMDEBUG("Emitting the request to list notes per notebooks "
            << "and tags: offset = " << m_listNotesOffset
            << ", request id = " << m_listNotesRequestId
            << ", order = " << order
            << ", direction = " << direction
            << ", notebook local uids: "
            << notebookLocalUids.join(QStringLiteral(", "))
            << "; tag local uids: "
            << tagLocalUids.join(QStringLiteral(", ")));

    Q_EMIT listNotesPerNotebooksAndTags(
        notebookLocalUids, tagLocalUids,
        LocalStorageManager::GetNoteOptions(0),
        flags, NOTE_LIST_QUERY_LIMIT, m_listNotesOffset,
        order, direction, m_listNotesRequestId);
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

    NMDEBUG("Emitting the request to get full note count per "
            << "account: request id = "
            << m_getFullNoteCountPerAccountRequestId);

    LocalStorageManager::NoteCountOptions options = noteCountOptions();
    Q_EMIT getNoteCount(options, m_getFullNoteCountPerAccountRequestId);
}

void NoteModel::requestTotalFilteredNotesCount()
{
    NMDEBUG("NoteModel::requestTotalFilteredNotesCount");

    if (!hasFilters())
    {
        m_getNoteCountRequestId = QUuid::createUuid();
        LocalStorageManager::NoteCountOptions options = noteCountOptions();

        NMDEBUG("Emitting the request to get note count: "
                << "options = " << options
                << ", request id = " << m_getNoteCountRequestId);

        Q_EMIT getNoteCount(options, m_getNoteCountRequestId);
        return;
    }

    const QSet<QString> & filteredNoteLocalUids = m_pFilters->filteredNoteLocalUids();
    if (!filteredNoteLocalUids.isEmpty()) {
        m_getNoteCountRequestId = QUuid();
        m_totalFilteredNotesCount = filteredNoteLocalUids.size();
        Q_EMIT filteredNotesCountUpdated(m_totalFilteredNotesCount);
        return;
    }

    const QStringList & notebookLocalUids = m_pFilters->filteredNotebookLocalUids();
    const QStringList & tagLocalUids = m_pFilters->filteredTagLocalUids();

    m_getNoteCountRequestId = QUuid::createUuid();
    LocalStorageManager::NoteCountOptions options = noteCountOptions();

    NMDEBUG("Emitting the request to get note count per "
            << "notebooks and tags: options = " << options
            << ", request id = " << m_getNoteCountRequestId);

    Q_EMIT getNoteCountPerNotebooksAndTags(
        notebookLocalUids, tagLocalUids, options, m_getNoteCountRequestId);
}

void NoteModel::findNoteToRestoreFailedUpdate(const Note & note)
{
    NMDEBUG("NoteModel::findNoteToRestoreFailedUpdate: local uid = "
            << note.localUid());

    QUuid requestId = QUuid::createUuid();
    Q_UNUSED(m_findNoteToRestoreFailedUpdateRequestIds.insert(requestId))
    NMTRACE("Emitting the request to find a note: local uid = "
            << note.localUid() << ", request id = " << requestId);
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
    m_maxNoteCount = NOTE_MIN_CACHE_SIZE * 2;
    m_listNotesOffset = 0;
    m_listNotesRequestId = QUuid();
    m_getNoteCountRequestId = QUuid();
    m_totalAccountNotesCount = 0;
    m_getFullNoteCountPerAccountRequestId = QUuid();
    m_notebookDataByNotebookLocalUid.clear();
    m_findNotebookRequestForNotebookLocalUid.clear();
    m_localUidsOfNewNotesBeingAddedToLocalStorage.clear();
    m_addNoteRequestIds.clear();
    m_updateNoteRequestIds.clear();
    m_expungeNoteRequestIds.clear();
    m_findNoteToRestoreFailedUpdateRequestIds.clear();
    m_findNoteToPerformUpdateRequestIds.clear();
    m_noteItemsPendingNotebookDataUpdate.clear();
    m_noteLocalUidToFindNotebookRequestIdForMoveNoteToNotebookBimap.clear();
    m_tagDataByTagLocalUid.clear();
    m_findTagRequestForTagLocalUid.clear();
    m_tagLocalUidToNoteLocalUid.clear();

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

    auto it = std::lower_bound(index.begin(), index.end(), item,
                               NoteComparator(sortingColumn(), sortOrder()));
    if (it == index.end()) {
        return static_cast<int>(index.size());
    }

    return static_cast<int>(std::distance(index.begin(), it));
}

LocalStorageManager::NoteCountOptions NoteModel::noteCountOptions() const
{
    LocalStorageManager::NoteCountOptions noteCountOptions = 0;

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

void NoteModel::processTagExpunging(const QString & tagLocalUid)
{
    NMTRACE("NoteModel::processTagExpunging: tag local uid = "
            << tagLocalUid);

    auto tagDataIt = m_tagDataByTagLocalUid.find(tagLocalUid);
    if (tagDataIt == m_tagDataByTagLocalUid.end()) {
        NMTRACE("Tag data corresponding to the expunged tag was "
                "not found within the note model");
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

    NMTRACE("Affected notes local uids: "
            << affectedNotesLocalUids.join(QStringLiteral(", ")));
    for(auto it = affectedNotesLocalUids.constBegin(),
        end = affectedNotesLocalUids.constEnd(); it != end; ++it)
    {
        auto noteItemIt = localUidIndex.find(*it);
        if (Q_UNLIKELY(noteItemIt == localUidIndex.end())) {
            NMDEBUG("Can't find the note pointed to by the expunged "
                    << "tag by local uid: note local uid = " << noteIt.value());
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
    NMDEBUG("NoteModel::removeItemByLocalUid: " << localUid);

    NoteDataByLocalUid & localUidIndex = m_data.get<ByLocalUid>();
    auto itemIt = localUidIndex.find(localUid);
    if (Q_UNLIKELY(itemIt == localUidIndex.end())) {
        NMDEBUG("Can't find item to remove from the note model");
        return;
    }

    const NoteModelItem & item = *itemIt;

    NoteDataByIndex & index = m_data.get<ByIndex>();
    auto indexIt = m_data.project<ByIndex>(itemIt);
    if (Q_UNLIKELY(indexIt == index.end())) {
        NMWARNING("Can't determine the row index for the note "
                  << "model item to remove: " << item);
        return;
    }

    int row = static_cast<int>(std::distance(index.begin(), indexIt));
    if (Q_UNLIKELY((row < 0) || (row >= static_cast<int>(m_data.size())))) {
        NMWARNING("Invalid row index for the note model item "
                  << "to remove: index = " << row
                  << ", item: " << item);
        return;
    }

    beginRemoveRows(QModelIndex(), row, row);
    Q_UNUSED(localUidIndex.erase(itemIt))
    endRemoveRows();
}

bool NoteModel::updateItemRowWithRespectToSorting(
    const NoteModelItem & item, ErrorString & errorDescription)
{
    NMDEBUG("NoteModel::updateItemRowWithRespectToSorting: item local uid = "
            << item.localUid());

    NoteDataByLocalUid & localUidIndex = m_data.get<ByLocalUid>();
    auto localUidIt = localUidIndex.find(item.localUid());
    if (Q_UNLIKELY(localUidIt == localUidIndex.end())) {
        errorDescription.setBase(QT_TR_NOOP("can't find appropriate position "
                                            "for note within the model: can't "
                                            "find the note by local uid"));
        NMWARNING(errorDescription << ": " << item);
        return false;
    }

    NoteDataByIndex & index = m_data.get<ByIndex>();

    auto it = m_data.project<ByIndex>(localUidIt);
    if (Q_UNLIKELY(it == index.end())) {
        errorDescription.setBase(QT_TR_NOOP("can't find appropriate position for "
                                            "note within the model: internal "
                                            "error, can't find note's current "
                                            "position"));
        NMWARNING(errorDescription << ": " << item);
        return false;
    }

    int originalRow = static_cast<int>(std::distance(index.begin(), it));
    if (Q_UNLIKELY((originalRow < 0) ||
                   (originalRow >= static_cast<int>(m_data.size()))))
    {
        errorDescription.setBase(QT_TR_NOOP("can't find appropriate position for "
                                            "note within the model: internal "
                                            "error, note's current position is "
                                            "beyond the acceptable range"));
        NMWARNING(errorDescription << ": current row = "
                  << originalRow << ", item: " << item);
        return false;
    }

    NoteModelItem itemCopy(item);

    NMTRACE("Removing the moved item from the original row " << originalRow);
    beginRemoveRows(QModelIndex(), originalRow, originalRow);
    Q_UNUSED(index.erase(it))
    endRemoveRows();

    auto positionIter =
        std::lower_bound(index.begin(), index.end(), itemCopy,
                         NoteComparator(sortingColumn(), sortOrder()));
    if (positionIter == index.end())
    {
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
    NMTRACE("NoteModel::saveNoteInLocalStorage: local uid = "
            << item.localUid() << ", update tags = "
            << (saveTags ? "true" : "false"));

    Note note;

    auto notYetSavedItemIt =
        m_localUidsOfNewNotesBeingAddedToLocalStorage.find(item.localUid());
    if (notYetSavedItemIt == m_localUidsOfNewNotesBeingAddedToLocalStorage.end())
    {
        NMTRACE("Updating the note");

        const Note * pCachedNote = m_cache.get(item.localUid());
        if (Q_UNLIKELY(!pCachedNote))
        {
            QUuid requestId = QUuid::createUuid();
            Q_UNUSED(m_findNoteToPerformUpdateRequestIds.insert(requestId))
            Note dummy;
            dummy.setLocalUid(item.localUid());
            NMTRACE("Emitting the request to find note: local uid = " << item.localUid()
                    << ", request id = " << requestId);
            LocalStorageManager::GetNoteOptions getNoteOptions(
                LocalStorageManager::GetNoteOption::WithResourceMetadata);
            Q_EMIT findNote(dummy, getNoteOptions, requestId);
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
    note.setActive(item.isActive());

    QUuid requestId = QUuid::createUuid();

    if (notYetSavedItemIt != m_localUidsOfNewNotesBeingAddedToLocalStorage.end())
    {
        Q_UNUSED(m_addNoteRequestIds.insert(requestId))
        Q_UNUSED(m_localUidsOfNewNotesBeingAddedToLocalStorage.erase(notYetSavedItemIt))

        NMTRACE("Emitting the request to add the note to local storage: id = "
                << requestId << ", note: " << note);
        Q_EMIT addNote(note, requestId);
    }
    else
    {
        Q_UNUSED(m_updateNoteRequestIds.insert(requestId))

        // While the note is being updated in the local storage,
        // remove its stale copy from the cache
        Q_UNUSED(m_cache.remove(note.localUid()))

        NMTRACE("Emitting the request to update the note in local storage: id = "
                << requestId << ", note: " << note);
        LocalStorageManager::UpdateNoteOptions options(0);
        if (saveTags) {
            options |= LocalStorageManager::UpdateNoteOption::UpdateTags;
        }
        Q_EMIT updateNote(note, options, requestId);
    }
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
        {
            QImage thumbnail;
            Q_UNUSED(thumbnail.loadFromData(item.thumbnailData(), "PNG"))
            return thumbnail;
        }
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

QVariant NoteModel::dataAccessibleText(
    const int row, const Columns::type column) const
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
                                  printableDateTimeFromTimestamp(
                                      item.creationTimestamp());
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
                                  printableDateTimeFromTimestamp(
                                      item.modificationTimestamp());
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
                                  printableDateTimeFromTimestamp(
                                      item.deletionTimestamp());
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
                accessibleText += tr("has tags") + colon + space +
                                  tagNameList.join(QStringLiteral(", "));
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
                accessibleText += tr("size is") + space +
                                  humanReadableSize(bytes);
            }
            break;
        }
    case Columns::Synchronizable:
        accessibleText += (item.isSynchronizable()
                           ? tr("synchronizable")
                           : tr("not synchronizable"));
        break;
    case Columns::Dirty:
        accessibleText += (item.isDirty() ? tr("dirty") : tr("not dirty"));
        break;
    case Columns::HasResources:
        accessibleText += (item.hasResources()
                           ? tr("has attachments")
                           : tr("has no attachments"));
        break;
    case Columns::ThumbnailImage:
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
                errorDescription.setBase(QT_TR_NOOP("Can't update the note title: "
                                                    "note restrictions apply"));
                NMINFO(errorDescription << ", item: " << item);
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
                errorDescription.setBase(QT_TR_NOOP("Can't make already "
                                                    "synchronizable note not "
                                                    "synchronizable"));
                NMDEBUG(errorDescription << ", item: " << item);
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
                    errorDescription.setBase(QT_TR_NOOP("Can't change note's "
                                                        "deleted state: wrong "
                                                        "deletion timestamp value"));
                    NMDEBUG(errorDescription << ", item: " << item
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
    case Columns::CreationTimestamp:
        {
            qint64 timestamp = -1;

            if (!value.isNull())
            {
                bool conversionResult = false;
                timestamp = value.toLongLong(&conversionResult);

                if (!conversionResult) {
                    errorDescription.setBase(QT_TR_NOOP("Can't update note's "
                                                        "creation datetime: "
                                                        "wrong creation timestamp "
                                                        "value"));
                    NMDEBUG(errorDescription << ", item: " << item
                            << "\nTimestamp: " << value);
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
                    errorDescription.setBase(QT_TR_NOOP("Can't update note's "
                                                        "modification datetime: "
                                                        "wrong modification "
                                                        "timestamp value"));
                    NMDEBUG(errorDescription << ", item: "
                            << item << "\nTimestamp: " << value);
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
         ((column != Columns::DeletionTimestamp) &&
          (column != Columns::CreationTimestamp)) )
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

    if (!updateItemRowWithRespectToSorting(item, errorDescription)) {
        NMWARNING("Failed to update note item's row with respect "
                  << "to sorting: " << errorDescription
                  << ", note item: " << item);
    }

    saveNoteInLocalStorage(item);
    return true;
}

bool NoteModel::removeRowsImpl(int row, int count, ErrorString & errorDescription)
{
    if (Q_UNLIKELY((row + count) > static_cast<int>(m_data.size()))) {
        errorDescription.setBase(QT_TR_NOOP("Detected attempt to remove more "
                                            "rows than the note model contains"));
        NMDEBUG(errorDescription);
        return false;
    }

    NoteDataByIndex & index = m_data.get<ByIndex>();

    for(int i = 0; i < count; ++i)
    {
        auto it = index.begin() + row;
        if (!it->guid().isEmpty()) {
            errorDescription.setBase(QT_TR_NOOP("Can't remove the synchronizable "
                                                "note"));
            NMDEBUG(errorDescription);
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

    for(auto it = localUidsToRemove.begin(),
        end = localUidsToRemove.end(); it != end; ++it)
    {
        Note note;
        note.setLocalUid(*it);

        QUuid requestId = QUuid::createUuid();
        Q_UNUSED(m_expungeNoteRequestIds.insert(requestId))
        NMTRACE("Emitting the request to expunge the note from "
                << "the local storage: request id = "
                << requestId << ", note local uid: " << *it);
        Q_EMIT expungeNote(note, requestId);
    }

    endRemoveRows();

    return true;
}

bool NoteModel::canUpdateNoteItem(const NoteModelItem & item) const
{
    auto it = m_notebookDataByNotebookLocalUid.find(item.notebookLocalUid());
    if (it == m_notebookDataByNotebookLocalUid.end()) {
        NMDEBUG("Can't find the notebook data for note with local uid "
                << item.localUid());
        return false;
    }

    const NotebookData & notebookData = it.value();
    return notebookData.m_canUpdateNotes;
}

bool NoteModel::canCreateNoteItem(const QString & notebookLocalUid) const
{
    if (notebookLocalUid.isEmpty()) {
        NMDEBUG("NoteModel::canCreateNoteItem: empty notebook local uid");
        return false;
    }

    auto it = m_notebookDataByNotebookLocalUid.find(notebookLocalUid);
    if (it != m_notebookDataByNotebookLocalUid.end()) {
        return it->m_canCreateNotes;
    }

    NMDEBUG("Can't find the notebook data for notebook local uid "
            << notebookLocalUid);
    return false;
}

void NoteModel::updateNotebookData(const Notebook & notebook)
{
    NMTRACE("NoteModel::updateNotebookData: local uid = "
            << notebook.localUid());

    NotebookData & notebookData =
        m_notebookDataByNotebookLocalUid[notebook.localUid()];

    if (!notebook.hasRestrictions())
    {
        notebookData.m_canCreateNotes = true;
        notebookData.m_canUpdateNotes = true;
    }
    else
    {
        const qevercloud::NotebookRestrictions & notebookRestrictions =
            notebook.restrictions();
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

    NMTRACE("Collected notebook data from notebook with local uid "
            << notebook.localUid() << ": guid = "
            << notebookData.m_guid << "; name = "
            << notebookData.m_name << ": can create notes = "
            << (notebookData.m_canCreateNotes ? "true" : "false")
            << ": can update notes = "
            << (notebookData.m_canUpdateNotes ? "true" : "false"));

    checkAddedNoteItemsPendingNotebookData(notebook.localUid(), notebookData);
}

bool NoteModel::setNoteFavorited(
    const QString & noteLocalUid, const bool favorited,
    ErrorString & errorDescription)
{
    NoteDataByLocalUid & localUidIndex = m_data.get<ByLocalUid>();
    auto it = localUidIndex.find(noteLocalUid);
    if (Q_UNLIKELY(it == localUidIndex.end())) {
        errorDescription.setBase(QT_TR_NOOP("internal error, the note to be "
                                            "favorited/unfavorited was "
                                            "not found within the model"));
        NMWARNING(errorDescription);
        return false;
    }

    const NoteModelItem & item = *it;

    if (favorited == item.isFavorited()) {
        NMDEBUG("Favorited flag's value hasn't changed");
        return true;
    }

    NoteModelItem itemCopy(item);
    itemCopy.setFavorited(favorited);

    localUidIndex.replace(it, itemCopy);
    saveNoteInLocalStorage(itemCopy);

    return true;
}

void NoteModel::setSortingColumnAndOrder(
    const int column, const Qt::SortOrder order)
{
    switch(column)
    {
    case Columns::CreationTimestamp:
        if (order == Qt::AscendingOrder) {
            m_noteSortingMode = NoteSortingMode::CreatedAscending;
        }
        else {
            m_noteSortingMode = NoteSortingMode::CreatedDescending;
        }
        break;
    case Columns::Title:
        if (order == Qt::AscendingOrder) {
            m_noteSortingMode = NoteSortingMode::TitleAscending;
        }
        else {
            m_noteSortingMode = NoteSortingMode::TitleDescending;
        }
        break;
    case Columns::Size:
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
    switch(m_noteSortingMode)
    {
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
    NoteDataByLocalUid::iterator it, const Notebook & notebook,
    ErrorString & errorDescription)
{
    NoteModelItem item = *it;

    NMTRACE("NoteModel::moveNoteToNotebookImpl: notebook = "
            << notebook << ", note item: " << item);

    if (Q_UNLIKELY(item.notebookLocalUid() == notebook.localUid())) {
        NMDEBUG("The note is already within its target notebook, nothing to do");
        return true;
    }

    if (!notebook.canCreateNotes()) {
        errorDescription.setBase(QT_TR_NOOP("the target notebook doesn't allow "
                                            "to create notes in it"));
        NMINFO(errorDescription << ", notebook: " << notebook);
        return false;
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
    NMTRACE("NoteModel::addOrUpdateNoteItem: note local uid = "
            << item.localUid() << ", notebook local uid = "
            << item.notebookLocalUid() << ", notebook name = "
            << notebookData.m_name << ", from notes listing = "
            << (fromNotesListing ? "true" : "false"));

    item.setNotebookName(notebookData.m_name);

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
                    NMTRACE("Won't add note as it's not deleted and the model "
                            "instance only considers the deleted notes");
                    return;
                }
                break;
            }
        case IncludedNotes::NonDeleted:
            {
                if (item.deletionTimestamp() >= 0) {
                    NMDEBUG("Won't add note as it's deleted and the model "
                            "instance only considers the non-deleted notes");
                    return;
                }
                break;
            }
        }

        NMDEBUG("Adding new item to the note model");

        if (fromNotesListing)
        {
            findTagNamesForItem(item);

            int row = static_cast<int>(localUidIndex.size());
            beginInsertRows(QModelIndex(), row, row);
            Q_UNUSED(localUidIndex.insert(item))
            endInsertRows();

            ErrorString errorDescription;
            if (!updateItemRowWithRespectToSorting(item, errorDescription)) {
                NMWARNING("Could not update new note model item's row: "
                          << errorDescription << "; item: " << item);
            }

            checkMaxNoteCountAndRemoveLastNoteIfNeeded();
            return;
        }

        NoteDataByIndex & index = m_data.get<ByIndex>();

        auto positionIter = std::lower_bound(
            index.begin(), index.end(), item,
            NoteComparator(sortingColumn(), sortOrder()));

        int newRow = static_cast<int>(std::distance(index.begin(), positionIter));

        if (newRow >= static_cast<int>(m_maxNoteCount)) {
            NMDEBUG("Skip adding note: new row " << newRow <<
                    " is larger than or equal to the max note count "
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
                    NMDEBUG("Removing the updated note item from the model as "
                            "the item is not deleted and the model instance "
                            "only considers the deleted notes");
                    shouldRemoveItem = true;
                }
                break;
            }
        case IncludedNotes::NonDeleted:
            {
                if (item.deletionTimestamp() >= 0)
                {
                    NMDEBUG("Removing the updated note item from the model as "
                            "the item is deleted and the model instance only "
                            "considers the non-deleted notes");
                    shouldRemoveItem = true;
                }
                break;
            }
        }

        NoteDataByIndex & index = m_data.get<ByIndex>();
        auto indexIt = m_data.project<ByIndex>(it);
        if (Q_UNLIKELY(indexIt == index.end())) {
            REPORT_ERROR(QT_TR_NOOP("Internal error: can't project the local uid "
                                    "index iterator to the random access index "
                                    "iterator in note model"));
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

            ErrorString errorDescription;
            if (!updateItemRowWithRespectToSorting(item, errorDescription)) {
                NMWARNING("Could not update note model item's row: "
                          << errorDescription << "; item: " << item);
            }

            findTagNamesForItem(item);
        }
    }
}

void NoteModel::checkMaxNoteCountAndRemoveLastNoteIfNeeded()
{
    QNDEBUG("NoteModel::checkMaxNoteCountAndRemoveLastNoteIfNeeded");

    NoteDataByLocalUid & localUidIndex = m_data.get<ByLocalUid>();
    size_t indexSize = localUidIndex.size();
    if (indexSize <= m_maxNoteCount) {
        return;
    }

    QNDEBUG("Note model's size is outside the acceptable range, "
            "removing the last row's note to keep the cache minimal");

    int lastRow = static_cast<int>(indexSize - 1);
    NoteDataByIndex & index = m_data.get<ByIndex>();
    auto indexIt = index.begin();
    std::advance(indexIt, lastRow);
    auto it = m_data.project<ByLocalUid>(indexIt);
    if (Q_UNLIKELY(it == localUidIndex.end())) {
        REPORT_ERROR(QT_TR_NOOP("Internal error: can't project "
                                "the random access index iterator "
                                "to the local uid index iterator "
                                "in note model"));
    }
    else {
        beginRemoveRows(QModelIndex(), lastRow, lastRow);
        Q_UNUSED(localUidIndex.erase(it))
        endRemoveRows();
    }
}

void NoteModel::checkAddedNoteItemsPendingNotebookData(
    const QString & notebookLocalUid, const NotebookData & notebookData)
{
    auto it = m_noteItemsPendingNotebookDataUpdate.find(notebookLocalUid);
    while(it != m_noteItemsPendingNotebookDataUpdate.end())
    {
        if (it.key() != notebookLocalUid) {
            break;
        }

        addOrUpdateNoteItem(it.value(), notebookData, true);
        Q_UNUSED(m_noteItemsPendingNotebookDataUpdate.erase(it++))
    }
}

void NoteModel::findTagNamesForItem(NoteModelItem & item)
{
    NMTRACE("NoteModel::findTagNamesForItem: " << item);

    const QStringList & tagLocalUids = item.tagLocalUids();
    for(auto it = tagLocalUids.constBegin(),
        end = tagLocalUids.constEnd(); it != end; ++it)
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
            NMDEBUG("Tag local uid " << tagLocalUid
                    << " points to note model item " << item.localUid()
                    << ", title = " << item.title());
        }

        auto tagDataIt = m_tagDataByTagLocalUid.find(tagLocalUid);
        if (tagDataIt != m_tagDataByTagLocalUid.end()) {
            NMTRACE("Found tag data for tag local uid " << tagLocalUid
                    << ": tag name = " << tagDataIt->m_name);
            item.addTagName(tagDataIt->m_name);
            continue;
        }

        NMTRACE("Tag data for tag local uid " << tagLocalUid << " was not found");

        auto requestIt = m_findTagRequestForTagLocalUid.left.find(tagLocalUid);
        if (requestIt != m_findTagRequestForTagLocalUid.left.end()) {
            NMTRACE("The request to find tag corresponding to local uid "
                    << tagLocalUid
                    << " has already been sent: request id = "
                    << requestIt->second);
            continue;
        }

        QUuid requestId = QUuid::createUuid();
        Q_UNUSED(m_findTagRequestForTagLocalUid.insert(
                LocalUidToRequestIdBimap::value_type(tagLocalUid, requestId)))

        Tag tag;
        tag.setLocalUid(tagLocalUid);
        NMDEBUG("Emitting the request to find tag: tag local uid = "
                << tagLocalUid << ", request id = " << requestId);
        Q_EMIT findTag(tag, requestId);
    }
}

void NoteModel::updateTagData(const Tag & tag)
{
    NMTRACE("NoteModel::updateTagData: tag local uid = " << tag.localUid());

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

    NMTRACE("Affected notes local uids: "
            << affectedNotesLocalUids.join(QStringLiteral(", ")));
    for(auto it = affectedNotesLocalUids.constBegin(),
        end = affectedNotesLocalUids.constEnd(); it != end; ++it)
    {
        auto noteItemIt = localUidIndex.find(*it);
        if (Q_UNLIKELY(noteItemIt == localUidIndex.end())) {
            NMDEBUG("Can't find the note pointed to by a tag by "
                    << "local uid: note local uid = " << *it);
            continue;
        }

        NoteModelItem item = *noteItemIt;
        QStringList tagLocalUids = item.tagLocalUids();

        // Need to refresh all the tag names and guids because it is generally
        // unknown which particular tag was updated
        item.setTagNameList(QStringList());
        item.setTagGuids(QStringList());

        for(auto tagLocalUidIt = tagLocalUids.begin(),
            tagLocalUidEnd = tagLocalUids.end();
            tagLocalUidIt != tagLocalUidEnd; ++tagLocalUidIt)
        {
            auto tagDataIt = m_tagDataByTagLocalUid.find(*tagLocalUidIt);
            if (tagDataIt == m_tagDataByTagLocalUid.end()) {
                NMTRACE("Still no tag data for tag with local uid "
                        << *tagLocalUidIt);
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

NoteModel::NoteFilters::NoteFilters() :
    m_filteredNotebookLocalUids(),
    m_filteredTagLocalUids(),
    m_filteredNoteLocalUids()
{}

bool NoteModel::NoteFilters::isEmpty() const
{
    return m_filteredNotebookLocalUids.isEmpty() &&
           m_filteredTagLocalUids.isEmpty() &&
           m_filteredNoteLocalUids.isEmpty();
}

const QStringList & NoteModel::NoteFilters::filteredNotebookLocalUids() const
{
    return m_filteredNotebookLocalUids;
}

void NoteModel::NoteFilters::setFilteredNotebookLocalUids(
    const QStringList & notebookLocalUids)
{
    m_filteredNotebookLocalUids = notebookLocalUids;
}

void NoteModel::NoteFilters::clearFilteredNotebookLocalUids()
{
    m_filteredNotebookLocalUids.clear();
}

const QStringList & NoteModel::NoteFilters::filteredTagLocalUids() const
{
    return m_filteredTagLocalUids;
}

void NoteModel::NoteFilters::setFilteredTagLocalUids(
    const QStringList & tagLocalUids)
{
    m_filteredTagLocalUids = tagLocalUids;
}

void NoteModel::NoteFilters::clearFilteredTagLocalUids()
{
    m_filteredTagLocalUids.clear();
}

const QSet<QString> & NoteModel::NoteFilters::filteredNoteLocalUids() const
{
    return m_filteredNoteLocalUids;
}

bool NoteModel::NoteFilters::setFilteredNoteLocalUids(
    const QSet<QString> & noteLocalUids)
{
    if (m_filteredNoteLocalUids == noteLocalUids) {
        return false;
    }

    m_filteredNoteLocalUids = noteLocalUids;
    return true;
}

bool NoteModel::NoteFilters::setFilteredNoteLocalUids(
    const QStringList & noteLocalUids)
{
    return setFilteredNoteLocalUids(QSet<QString>::fromList(noteLocalUids));
}

void NoteModel::NoteFilters::clearFilteredNoteLocalUids()
{
    m_filteredNoteLocalUids.clear();
}

bool NoteModel::NoteComparator::operator()(
    const NoteModelItem & lhs, const NoteModelItem & rhs) const
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

            int compareResult =
                leftTitleOrPreview.localeAwareCompare(rightTitleOrPreview);
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

            int compareResult =
                leftTitleOrPreview.localeAwareCompare(rightTitleOrPreview);
            less = (compareResult < 0);
            greater = (compareResult > 0);
            break;
        }
    case Columns::NotebookName:
        {
            int compareResult =
                lhs.notebookName().localeAwareCompare(rhs.notebookName());
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
