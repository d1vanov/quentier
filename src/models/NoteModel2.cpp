/*
 * Copyright 2019 Dmitry Ivanov
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

#include "NoteModel2.h"
#include <quentier/logging/QuentierLogger.h>

// Separate logging macros for the note model - to distinguish the one
// for deleted notes from the one for non-deleted notes

inline QString includedNotesStr(
    const quentier::NoteModel2::IncludedNotes::type includedNotes)
{
    if (includedNotes == quentier::NoteModel2::IncludedNotes::All) {
        return QStringLiteral("[all notes] ");
    }

    if (includedNotes == quentier::NoteModel2::IncludedNotes::NonDeleted) {
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

#define NUM_NOTE_MODEL_COLUMNS (12)

#define REPORT_ERROR(error, ...) \
    ErrorString errorDescription(error); \
    NMWARNING(errorDescription << QStringLiteral("" __VA_ARGS__ "")); \
    Q_EMIT notifyError(errorDescription)

namespace quentier {

NoteModel2::NoteModel2(const Account & account,
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
    m_data(),
    m_totalFilteredNotesCount(0),
    m_cache(noteCache),
    m_notebookCache(notebookCache),
    m_totalAccountNotesCount(0),
    m_pFilters(pFilters),
    m_pUpdatedNoteFilters(Q_NULLPTR),
    m_listNotesOffset(0),
    m_listNotesRequestId(),
    m_getNoteCountRequestId()
{
    if (m_pFilters.isNull()) {
        m_pFilters.reset(new NoteFilters);
    }

    createConnections(localStorageManagerAsync);
    requestNotesListAndCount();
}

NoteModel2::~NoteModel2()
{}

void NoteModel2::updateAccount(const Account & account)
{
    NMDEBUG(QStringLiteral("NoteModel2::updateAccount: ") << account);
    m_account = account;
}

NoteModel2::Columns::type NoteModel2::sortingColumn() const
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

Qt::SortOrder NoteModel2::sortOrder() const
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

QModelIndex NoteModel2::indexForLocalUid(const QString & localUid) const
{
    const NoteDataByLocalUid & localUidIndex = m_data.get<ByLocalUid>();
    auto it = localUidIndex.find(localUid);
    if (Q_UNLIKELY(it == localUidIndex.end())) {
        NMDEBUG(QStringLiteral("Can't find note item by local uid: ") << localUid);
        return QModelIndex();
    }

    const NoteDataByIndex & index = m_data.get<ByIndex>();
    auto indexIt = m_data.project<ByIndex>(it);
    if (Q_UNLIKELY(indexIt == index.end())) {
        NMWARNING(QStringLiteral("Can't find the indexed reference to the note ")
                  << QStringLiteral("item: ") << *it);
        return QModelIndex();
    }

    int rowIndex = static_cast<int>(std::distance(index.begin(), indexIt));
    return createIndex(rowIndex, Columns::Title);
}

const NoteModelItem * NoteModel2::itemForLocalUid(const QString & localUid) const
{
    const NoteDataByLocalUid & localUidIndex = m_data.get<ByLocalUid>();
    auto it = localUidIndex.find(localUid);
    if (Q_UNLIKELY(it == localUidIndex.end())) {
        NMDEBUG(QStringLiteral("Can't find note item by local uid: ") << localUid);
        return Q_NULLPTR;
    }

    return &(*it);
}

const NoteModelItem * NoteModel2::itemAtRow(const int row) const
{
    const NoteDataByIndex & index = m_data.get<ByIndex>();
    if (Q_UNLIKELY((row < 0) || (index.size() <= static_cast<size_t>(row)))) {
        return Q_NULLPTR;
    }

    return &(index[static_cast<size_t>(row)]);
}

const NoteModelItem * NoteModel2::itemForIndex(const QModelIndex & index) const
{
    if (!index.isValid()) {
        return Q_NULLPTR;
    }

    if (index.parent().isValid()) {
        return Q_NULLPTR;
    }

    return itemAtRow(index.row());
}

bool NoteModel2::hasFilters() const
{
    return !m_pFilters->isEmpty();
}

const QStringList & NoteModel2::filteredNotebookLocalUids() const
{
    return m_pFilters->filteredNotebookLocalUids();
}

void NoteModel2::setFilteredNotebookLocalUids(const QStringList & notebookLocalUids)
{
    NMDEBUG(QStringLiteral("NoteModel2::setFilteredNotebookLocalUids: ")
            << notebookLocalUids.join(QStringLiteral(", ")));

    if (!m_pUpdatedNoteFilters.isNull()) {
        m_pUpdatedNoteFilters->setFilteredNotebookLocalUids(notebookLocalUids);
        return;
    }

    m_pFilters->setFilteredNotebookLocalUids(notebookLocalUids);
    resetModel();
}

const QStringList & NoteModel2::filteredTagLocalUids() const
{
    return m_pFilters->filteredTagLocalUids();
}

void NoteModel2::setFilteredTagLocalUids(const QStringList & tagLocalUids)
{
    NMDEBUG(QStringLiteral("NoteModel2::filteredTagLocalUids: ")
            << tagLocalUids.join(QStringLiteral(", ")));

    if (!m_pUpdatedNoteFilters.isNull()) {
        m_pUpdatedNoteFilters->setFilteredTagLocalUids(tagLocalUids);
        return;
    }

    m_pFilters->setFilteredTagLocalUids(tagLocalUids);
    resetModel();
}

const QStringList & NoteModel2::filteredNoteLocalUids() const
{
    return m_pFilters->filteredNoteLocalUids();
}

void NoteModel2::setFilteredNoteLocalUids(const QStringList & noteLocalUids)
{
    NMDEBUG(QStringLiteral("NoteModel2::setFilteredNoteLocalUids: ")
            << noteLocalUids.join(QStringLiteral(", ")));

    if (!m_pUpdatedNoteFilters.isNull()) {
        m_pUpdatedNoteFilters->setFilteredNoteLocalUids(noteLocalUids);
        return;
    }

    m_pFilters->setFilteredNoteLocalUids(noteLocalUids);
    resetModel();
}

void NoteModel2::beginUpdateFilter()
{
    NMDEBUG(QStringLiteral("NoteModel2::beginUpdateFilter"));
    m_pUpdatedNoteFilters.reset(new NoteFilters);
}

void NoteModel2::endUpdateFilter()
{
    NMDEBUG(QStringLiteral("NoteModel2::endUpdateFilter"));

    if (m_pUpdatedNoteFilters.isNull()) {
        return;
    }

    m_pFilters.swap(m_pUpdatedNoteFilters);
    m_pUpdatedNoteFilters.reset();

    resetModel();
}

int NoteModel2::totalFilteredNotesCount() const
{
    return m_totalFilteredNotesCount;
}

QModelIndex NoteModel2::createNoteItem(const QString & notebookLocalUid,
                                       ErrorString & errorDescription)
{
    NMDEBUG(QStringLiteral("NoteModel2::createNoteItem: notebook local uid = ")
            << notebookLocalUid);

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

bool NoteModel2::deleteNote(const QString & noteLocalUid,
                            ErrorString & errorDescription)
{
    NMDEBUG(QStringLiteral("NoteModel2::deleteNote: ") << noteLocalUid);

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

bool NoteModel2::moveNoteToNotebook(const QString & noteLocalUid,
                                    const QString & notebookName,
                                    ErrorString & errorDescription)
{
    NMDEBUG(QStringLiteral("NoteModel2::moveNoteToNotebook: note local uid = ")
            << noteLocalUid << QStringLiteral(", notebook name = ")
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
    NMTRACE(QStringLiteral("Emitting the request to find a notebook by name for ")
            << QStringLiteral("moving the note to it: request id = ")
            << requestId << QStringLiteral(", notebook name = ") << notebookName
            << QStringLiteral(", note local uid = ") << noteLocalUid);
    Q_EMIT findNotebook(dummy, requestId);

    return true;
}

bool NoteModel2::favoriteNote(const QString & noteLocalUid,
                              ErrorString & errorDescription)
{
    NMDEBUG(QStringLiteral("NoteModel2::favoriteNote: ") << noteLocalUid);
    return setNoteFavorited(noteLocalUid, true, errorDescription);
}

bool NoteModel2::unfavoriteNote(const QString & noteLocalUid,
                                ErrorString & errorDescription)
{
    NMDEBUG(QStringLiteral("NoteModel2::unfavoriteNote: ") << noteLocalUid);
    return setNoteFavorited(noteLocalUid, false, errorDescription);
}

Qt::ItemFlags NoteModel2::flags(const QModelIndex & modelIndex) const
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

QVariant NoteModel2::data(const QModelIndex & index, int role) const
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

QVariant NoteModel2::headerData(int section, Qt::Orientation orientation,
                                int role) const
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

int NoteModel2::rowCount(const QModelIndex & parent) const
{
    if (parent.isValid()) {
        return 0;
    }

    return static_cast<int>(m_data.size());
}

int NoteModel2::columnCount(const QModelIndex & parent) const
{
    if (parent.isValid()) {
        return 0;
    }

    return NUM_NOTE_MODEL_COLUMNS;
}

QModelIndex NoteModel2::index(int row, int column, const QModelIndex & parent) const
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

QModelIndex NoteModel2::parent(const QModelIndex & index) const
{
    Q_UNUSED(index)
    return QModelIndex();
}

bool NoteModel2::setHeaderData(int section, Qt::Orientation orientation,
                               const QVariant & value, int role)
{
    Q_UNUSED(section)
    Q_UNUSED(orientation)
    Q_UNUSED(value)
    Q_UNUSED(role)
    return false;
}

bool NoteModel2::setData(const QModelIndex & modelIndex,
                         const QVariant & value, int role)
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

bool NoteModel2::insertRows(int row, int count, const QModelIndex & parent)
{
    // NOTE: NoteModel2's own API is used to create new note items
    Q_UNUSED(row)
    Q_UNUSED(count)
    Q_UNUSED(parent)
    return false;
}

bool NoteModel2::removeRows(int row, int count, const QModelIndex & parent)
{
    if (Q_UNLIKELY(parent.isValid())) {
        NMDEBUG(QStringLiteral("Ignoring the attempt to remove rows from note "
                               "model for valid parent model index"));
        return false;
    }

    ErrorString errorDescription;
    bool res = removeRowsImpl(row, count, errorDescription);
    if (!res) {
        Q_EMIT notifyError(errorDescription);
    }

    return res;
}

void NoteModel2::createConnections(LocalStorageManagerAsync & localStorageManagerAsync)
{
    NMDEBUG(QStringLiteral("NoteModel2::createConnections"));

    // Local signals to localStorageManagerAsync's slots
    QObject::connect(this,
                     QNSIGNAL(NoteModel2,addNote,Note,QUuid),
                     &localStorageManagerAsync,
                     QNSLOT(LocalStorageManagerAsync,
                            onAddNoteRequest,Note,QUuid));
    QObject::connect(this,
                     QNSIGNAL(NoteModel2,updateNote,
                              Note,LocalStorageManager::UpdateNoteOptions,QUuid),
                     &localStorageManagerAsync,
                     QNSLOT(LocalStorageManagerAsync,onUpdateNoteRequest,
                            Note,LocalStorageManager::UpdateNoteOptions,QUuid));
    QObject::connect(this,
                     QNSIGNAL(NoteModel2,findNote,
                              Note,LocalStorageManager::GetNoteOptions,QUuid),
                     &localStorageManagerAsync,
                     QNSLOT(LocalStorageManagerAsync,onFindNoteRequest,
                            Note,LocalStorageManager::GetNoteOptions,QUuid));
    QObject::connect(this,
                     QNSIGNAL(NoteModel2,listNotes,
                              LocalStorageManager::ListObjectsOptions,
                              LocalStorageManager::GetNoteOptions,size_t,size_t,
                              LocalStorageManager::ListNotesOrder::type,
                              LocalStorageManager::OrderDirection::type,
                              QString,QUuid),
                     &localStorageManagerAsync,
                     QNSLOT(LocalStorageManagerAsync,onListNotesRequest,
                            LocalStorageManager::ListObjectsOptions,
                            LocalStorageManager::GetNoteOptions,size_t,size_t,
                            LocalStorageManager::ListNotesOrder::type,
                            LocalStorageManager::OrderDirection::type,
                            QString,QUuid));
    QObject::connect(this,
                     QNSIGNAL(NoteModel2,listNotesPerNotebooksAndTags,
                              QStringList,QStringList,
                              LocalStorageManager::ListObjectsOptions,
                              LocalStorageManager::GetNoteOptions,size_t,size_t,
                              LocalStorageManager::ListNotesOrder::type,
                              LocalStorageManager::OrderDirection::type,QUuid),
                     &localStorageManagerAsync,
                     QNSLOT(LocalStorageManagerAsync,
                            onListNotesPerNotebooksAndTagsRequest,
                            QStringList,QStringList,
                            LocalStorageManager::ListObjectsOptions,
                            LocalStorageManager::GetNoteOptions,size_t,size_t,
                            LocalStorageManager::ListNotesOrder::type,
                            LocalStorageManager::OrderDirection::type,QUuid));
    QObject::connect(this,
                     QNSIGNAL(NoteModel2,listNotesByLocalUids,
                              QStringList,LocalStorageManager::ListObjectsOptions,
                              LocalStorageManager::GetNoteOptions,size_t,size_t,
                              LocalStorageManager::ListNotesOrder::type,
                              LocalStorageManager::OrderDirection::type,QUuid),
                     &localStorageManagerAsync,
                     QNSLOT(LocalStorageManagerAsync,
                            onListNotesByLocalUidsRequest,
                            QStringList,LocalStorageManager::ListObjectsOptions,
                            LocalStorageManager::GetNoteOptions,size_t,size_t,
                            LocalStorageManager::ListNotesOrder::type,
                            LocalStorageManager::OrderDirection::type,QUuid));
    QObject::connect(this,
                     QNSIGNAL(NoteModel2,expungeNote,Note,QUuid),
                     &localStorageManagerAsync,
                     QNSLOT(LocalStorageManagerAsync,onExpungeNoteRequest,
                            Note,QUuid));
    QObject::connect(this,
                     QNSIGNAL(NoteModel2,findNotebook,Notebook,QUuid),
                     &localStorageManagerAsync,
                     QNSLOT(LocalStorageManagerAsync,onFindNotebookRequest,
                            Notebook,QUuid));
    QObject::connect(this,
                     QNSIGNAL(NoteModel2,findTag,Tag,QUuid),
                     &localStorageManagerAsync,
                     QNSLOT(LocalStorageManagerAsync,onFindTagRequest,Tag,QUuid));

    // localStorageManagerAsync's signals to local slots
    QObject::connect(&localStorageManagerAsync,
                     QNSIGNAL(LocalStorageManagerAsync,addNoteComplete,
                              Note,QUuid),
                     this,
                     QNSLOT(NoteModel2,onAddNoteComplete,Note,QUuid));
    QObject::connect(&localStorageManagerAsync,
                     QNSIGNAL(LocalStorageManagerAsync,addNoteFailed,
                              Note,ErrorString,QUuid),
                     this,
                     QNSLOT(NoteModel2,onAddNoteFailed,Note,ErrorString,QUuid));
    QObject::connect(&localStorageManagerAsync,
                     QNSIGNAL(LocalStorageManagerAsync,updateNoteComplete,
                              Note,LocalStorageManager::UpdateNoteOptions,QUuid),
                     this,
                     QNSLOT(NoteModel2,onUpdateNoteComplete,
                            Note,LocalStorageManager::UpdateNoteOptions,QUuid));
    QObject::connect(&localStorageManagerAsync,
                     QNSIGNAL(LocalStorageManagerAsync,updateNoteFailed,
                              Note,LocalStorageManager::UpdateNoteOptions,
                              ErrorString,QUuid),
                     this,
                     QNSLOT(NoteModel2,onUpdateNoteFailed,
                            Note,LocalStorageManager::UpdateNoteOptions,
                            ErrorString,QUuid));
    QObject::connect(&localStorageManagerAsync,
                     QNSIGNAL(LocalStorageManagerAsync,findNoteComplete,
                              Note,LocalStorageManager::GetNoteOptions,QUuid),
                     this,
                     QNSLOT(NoteModel2,onFindNoteComplete,
                            Note,LocalStorageManager::GetNoteOptions,QUuid));
    QObject::connect(&localStorageManagerAsync,
                     QNSIGNAL(LocalStorageManagerAsync,findNoteFailed,
                              Note,LocalStorageManager::GetNoteOptions,
                              ErrorString,QUuid),
                     this,
                     QNSLOT(NoteModel2,onFindNoteFailed,
                            Note,LocalStorageManager::GetNoteOptions,
                            ErrorString,QUuid));
    QObject::connect(&localStorageManagerAsync,
                     QNSIGNAL(LocalStorageManagerAsync,listNotesComplete,
                              LocalStorageManager::ListObjectsOptions,
                              LocalStorageManager::GetNoteOptions,size_t,size_t,
                              LocalStorageManager::ListNotesOrder::type,
                              LocalStorageManager::OrderDirection::type,
                              QString,QList<Note>,QUuid),
                     this,
                     QNSLOT(NoteModel2,onListNotesComplete,
                            LocalStorageManager::ListObjectsOptions,
                            LocalStorageManager::GetNoteOptions,size_t,size_t,
                            LocalStorageManager::ListNotesOrder::type,
                            LocalStorageManager::OrderDirection::type,
                            QString,QList<Note>,QUuid));
    QObject::connect(&localStorageManagerAsync,
                     QNSIGNAL(LocalStorageManagerAsync,listNotesFailed,
                              LocalStorageManager::ListObjectsOptions,
                              LocalStorageManager::GetNoteOptions,size_t,size_t,
                              LocalStorageManager::ListNotesOrder::type,
                              LocalStorageManager::OrderDirection::type,
                              QString,ErrorString,QUuid),
                     this,
                     QNSLOT(NoteModel2,onListNotesFailed,
                            LocalStorageManager::ListObjectsOptions,
                            LocalStorageManager::GetNoteOptions,size_t,size_t,
                            LocalStorageManager::ListNotesOrder::type,
                            LocalStorageManager::OrderDirection::type,
                            QString,ErrorString,QUuid));
    QObject::connect(&localStorageManagerAsync,
                     QNSIGNAL(LocalStorageManagerAsync,
                              listNotesPerNotebooksAndTagsComplete,
                              int,QStringList,QStringList,
                              LocalStorageManager::ListObjectsOptions,
                              LocalStorageManager::GetNoteOptions,size_t,size_t,
                              LocalStorageManager::ListNotesOrder::type,
                              LocalStorageManager::OrderDirection::type,QUuid),
                     this,
                     QNSLOT(NoteModel2,onListNotesPerNotebooksAndTagsComplete,
                            int,QStringList,QStringList,
                            LocalStorageManager::ListObjectsOptions,
                            LocalStorageManager::GetNoteOptions,size_t,size_t,
                            LocalStorageManager::ListNotesOrder::type,
                            LocalStorageManager::OrderDirection::type,QUuid));
    QObject::connect(&localStorageManagerAsync,
                     QNSIGNAL(LocalStorageManagerAsync,
                              listNotesPerNotebooksAndTagsFailed,
                              ErrorString,QStringList,QStringList,
                              LocalStorageManager::ListObjectsOptions,
                              LocalStorageManager::GetNoteOptions,size_t,size_t,
                              LocalStorageManager::ListNotesOrder::type,
                              LocalStorageManager::OrderDirection::type,QUuid),
                     this,
                     QNSLOT(NoteModel2,onListNotesPerNotebooksAndTagsFailed,
                            ErrorString,QStringList,QStringList,
                            LocalStorageManager::ListObjectsOptions,
                            LocalStorageManager::GetNoteOptions,size_t,size_t,
                            LocalStorageManager::ListNotesOrder::type,
                            LocalStorageManager::OrderDirection::type,QUuid));
    QObject::connect(&localStorageManagerAsync,
                     QNSIGNAL(LocalStorageManagerAsync,
                              listNotesByLocalUidsComplete,
                              int,QString,
                              LocalStorageManager::ListObjectsOptions,
                              LocalStorageManager::GetNoteOptions,size_t,size_t,
                              LocalStorageManager::ListNotesOrder::type,
                              LocalStorageManager::OrderDirection::type,QUuid),
                     this,
                     QNSLOT(NoteModel2,onListNotesByLocalUidsComplete,
                            int,QString,
                            LocalStorageManager::ListObjectsOptions,
                            LocalStorageManager::GetNoteOptions,size_t,size_t,
                            LocalStorageManager::ListNotesOrder::type,
                            LocalStorageManager::OrderDirection::type,QUuid));
    QObject::connect(&localStorageManagerAsync,
                     QNSIGNAL(LocalStorageManagerAsync,
                              listNotesByLocalUidsFailed,
                              ErrorString,QString,
                              LocalStorageManager::ListObjectsOptions,
                              LocalStorageManager::GetNoteOptions,size_t,size_t,
                              LocalStorageManager::ListNotesOrder::type,
                              LocalStorageManager::OrderDirection::type,QUuid),
                     this,
                     QNSLOT(NoteModel2,onListNotesByLocalUidsFailed,
                            ErrorString,QString,
                            LocalStorageManager::ListObjectsOptions,
                            LocalStorageManager::GetNoteOptions,size_t,size_t,
                            LocalStorageManager::ListNotesOrder::type,
                            LocalStorageManager::OrderDirection::type,QUuid));
    QObject::connect(&localStorageManagerAsync,
                     QNSIGNAL(LocalStorageManagerAsync,expungeNoteComplete,
                              Note,QUuid),
                     this,
                     QNSLOT(NoteModel2,onExpungeNoteComplete,Note,QUuid));
    QObject::connect(&localStorageManagerAsync,
                     QNSIGNAL(LocalStorageManagerAsync,expungeNoteFailed,
                              Note,ErrorString,QUuid),
                     this,
                     QNSLOT(NoteModel2,onExpungeNoteFailed,
                            Note,ErrorString,QUuid));
    QObject::connect(&localStorageManagerAsync,
                     QNSIGNAL(LocalStorageManagerAsync,findNotebookComplete,
                              Notebook,QUuid),
                     this,
                     QNSLOT(NoteModel2,onFindNotebookComplete,Notebook,QUuid));
    QObject::connect(&localStorageManagerAsync,
                     QNSIGNAL(LocalStorageManagerAsync,findNotebookFailed,
                              Notebook,ErrorString,QUuid),
                     this,
                     QNSLOT(NoteModel2,onFindNotebookFailed,
                            Notebook,ErrorString,QUuid));
    QObject::connect(&localStorageManagerAsync,
                     QNSIGNAL(LocalStorageManagerAsync,addNotebookComplete,
                              Notebook,QUuid),
                     this,
                     QNSLOT(NoteModel2,onAddNotebookComplete,Notebook,QUuid));
    QObject::connect(&localStorageManagerAsync,
                     QNSIGNAL(LocalStorageManagerAsync,updateNotebookComplete,
                              Notebook,QUuid),
                     this,
                     QNSLOT(NoteModel2,onUpdateNotebookComplete,Notebook,QUuid));
    QObject::connect(&localStorageManagerAsync,
                     QNSIGNAL(LocalStorageManagerAsync,expungeNotebookComplete,
                              Notebook,QUuid),
                     this,
                     QNSLOT(NoteModel2,onExpungeNotebookComplete,Notebook,QUuid));
    QObject::connect(&localStorageManagerAsync,
                     QNSIGNAL(LocalStorageManagerAsync,findTagComplete,Tag,QUuid),
                     this,
                     QNSLOT(NoteModel2,onFindTagComplete,Tag,QUuid));
    QObject::connect(&localStorageManagerAsync,
                     QNSIGNAL(LocalStorageManagerAsync,findTagFailed,
                              Tag,ErrorString,QUuid),
                     this,
                     QNSLOT(NoteModel2,onFindTagFailed,Tag,ErrorString,QUuid));
    QObject::connect(&localStorageManagerAsync,
                     QNSIGNAL(LocalStorageManagerAsync,addTagComplete,Tag,QUuid),
                     this,
                     QNSLOT(NoteModel2,onAddTagComplete,Tag,QUuid));
    QObject::connect(&localStorageManagerAsync,
                     QNSIGNAL(LocalStorageManagerAsync,updateTagComplete,
                              Tag,QUuid),
                     this,
                     QNSLOT(NoteModel2,onUpdateTagComplete,Tag,QUuid));
    QObject::connect(&localStorageManagerAsync,
                     QNSIGNAL(LocalStorageManagerAsync,expungeTagComplete,
                              Tag,QStringList,QUuid),
                     this,
                     QNSLOT(NoteModel2,onExpungeTagComplete,Tag,QStringList,QUuid));
}

void NoteModel2::requestNotesListAndCount()
{
    NMDEBUG(QStringLiteral("NoteModel2::requestNotesListAndCount"));

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

    if (m_totalAccountNotesCount == 0)
    {
        m_getFullNoteCountPerAccountRequestId = QUuid::createUuid();

        NMDEBUG(QStringLiteral("Emitting the request to get full note count per ")
                << QStringLiteral("account: request id = ")
                << m_getFullNoteCountPerAccountRequestId);

        LocalStorageManager::NoteCountOptions options(
            LocalStorageManager::NoteCountOption::IncludeNonDeletedNotes |
            LocalStorageManager::NoteCountOption::IncludeDeletedNotes);

        Q_EMIT getNoteCount(options, m_getFullNoteCountPerAccountRequestId);
    }

    m_listNotesRequestId = QUuid::createUuid();

    if (!hasFilters())
    {
        NMDEBUG(QStringLiteral("Emitting the request to list notes: offset = ")
                << m_listNotesOffset << QStringLiteral(", request id = ")
                << m_listNotesRequestId << QStringLiteral(", order = ")
                << order << QStringLiteral(", direction = ") << direction);

        Q_EMIT listNotes(flags, LocalStorageManager::GetNoteOptions(0),
                         NOTE_LIST_LIMIT, m_listNotesOffset, order, direction,
                         QString(), m_listNotesRequestId);

        if (m_totalFilteredNotesCount == 0)
        {
            m_getNoteCountRequestId = QUuid::createUuid();
            LocalStorageManager::NoteCountOptions options = noteCountOptions();

            NMDEBUG(QStringLiteral("Emitting the request to get note count: ")
                    << QStringLiteral("options = ") << options
                    << QStringLiteral(", request id = ") << m_getNoteCountRequestId);

            Q_EMIT getNoteCount(options, m_getNoteCountRequestId);
        }

        return;
    }

    const QStringList & filteredNoteLocalUids = m_pFilters->filteredNoteLocalUids();
    if (!filteredNoteLocalUids.isEmpty())
    {
        int end = static_cast<int>(m_listNotesOffset) + NOTE_LIST_LIMIT;
        end = std::min(end, filteredNoteLocalUids.size());

        QStringList noteLocalUids;
        noteLocalUids.reserve(std::max((end - static_cast<int>(m_listNotesOffset)), 0));
        for(int i = m_listNotesOffset; i < end; ++i) {
            noteLocalUids << filteredNoteLocalUids[i];
        }

        NMDEBUG(QStringLiteral("Emitting the request to list notes by local uids: ")
                << QStringLiteral(", request id = ") << m_listNotesRequestId
                << QStringLiteral(", order = ") << order
                << QStringLiteral(", direction = ") << direction
                << QStringLiteral(", note local uids: ")
                << noteLocalUids.join(QStringLiteral(", ")));

        Q_EMIT listNotesByLocalUids(noteLocalUids,
                                    LocalStorageManager::GetNoteOptions(0),
                                    flags, NOTE_LIST_LIMIT, 0, order, direction,
                                    m_listNotesRequestId);
        return;
    }

    const QStringList & notebookLocalUids = m_pFilters->filteredNotebookLocalUids();
    const QStringList & tagLocalUids = m_pFilters->filteredTagLocalUids();

    NMDEBUG(QStringLiteral("Emitting the request to list notes per notebooks ")
            << QStringLiteral("and tags: offset = ") << m_listNotesOffset
            << QStringLiteral(", request id = ") << m_listNotesRequestId
            << QStringLiteral(", order = ") << order
            << QStringLiteral(", direction = ") << direction
            << QStringLiteral(", notebook local uids: ")
            << notebookLocalUids.join(QStringLiteral(", "))
            << QStringLiteral("; tag local uids: ")
            << tagLocalUids.join(QStringLiteral(", ")));

    Q_EMIT listNotesPerNotebooksAndTags(notebookLocalUids, tagLocalUids,
                                        LocalStorageManager::GetNoteOptions(0),
                                        flags, NOTE_LIST_LIMIT, m_listNotesOffset,
                                        order, direction, m_listNotesRequestId);

    if (m_totalFilteredNotesCount == 0)
    {
        m_getNoteCountRequestId = QUuid::createUuid();
        LocalStorageManager::NoteCountOptions options = noteCountOptions();

        NMDEBUG(QStringLiteral("Emitting the request to get note count per ")
                << QStringLiteral("notebooks and tags: options = ") << options
                << QStringLiteral(", request id = ") << m_getNoteCountRequestId);

        Q_EMIT getNoteCountPerNotebooksAndTags(notebookLocalUids, tagLocalUids,
                                               options, m_getNoteCountRequestId);
    }
}

void NoteModel2::clearModel()
{
    NMDEBUG(QStringLiteral("NoteModel2::clearModel"));

    // TODO: implement
}

void NoteModel2::resetModel()
{
    NMDEBUG(QStringLiteral("NoteModel2::resetModel"));

    clearModel();
    requestNotesListAndCount();
}

LocalStorageManager::NoteCountOptions NoteModel2::noteCountOptions() const
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

bool NoteModel2::updateItemRowWithRespectToSorting(const NoteModelItem & item,
                                                   ErrorString & errorDescription)
{
    NMDEBUG(QStringLiteral("NoteModel2::updateItemRowWithRespectToSorting: ")
            << QStringLiteral("item local uid = ") << item.localUid());

    NoteDataByLocalUid & localUidIndex = m_data.get<ByLocalUid>();
    auto localUidIt = localUidIndex.find(item.localUid());
    if (Q_UNLIKELY(localUidIt == localUidIndex.end())) {
        errorDescription.setBase(QT_TR_NOOP("can't find appropriate position "
                                            "for note within the model: can't "
                                            "find the note by local uid"));
        NMWARNING(errorDescription << QStringLiteral(": ") << item);
        return false;
    }

    NoteDataByIndex & index = m_data.get<ByIndex>();

    auto it = m_data.project<ByIndex>(localUidIt);
    if (Q_UNLIKELY(it == index.end())) {
        errorDescription.setBase(QT_TR_NOOP("can't find appropriate position for "
                                            "note within the model: internal "
                                            "error, can't find note's current "
                                            "position"));
        NMWARNING(errorDescription << QStringLiteral(": ") << item);
        return false;
    }

    int originalRow = static_cast<int>(std::distance(index.begin(), it));
    if (Q_UNLIKELY((originalRow < 0) ||
                   (originalRow >= static_cast<int>(m_data.size()))))
    {
        errorDescription.setBase(QT_TR_NOOP("can't find appropriate position for ")
                                            "note within the model: internal "
                                            "error, note's current position is "
                                            "beyond the acceptable range");
        NMWARNING(errorDescription << QStringLiteral(": current row = ")
                  << originalRow << QStringLiteral(", item: ") << item);
        return false;
    }

    NoteModelItem itemCopy(item);

    NMTRACE(QStringLiteral("Removing the moved item from the original row ")
            << originalRow);
    beginRemoveRows(QModelIndex(), originalRow, originalRow);
    Q_UNUSED(index.erase(it))
    endRemoveRows();

    auto positionIter = std::lower_bound(index.begin(), index.end(), itemCopy,
                                         NoteComparator(sortingColumn(), sortOrder()));
    if (positionIter == index.end())
    {
        int newRow = static_cast<int>(index.size());

        NMTRACE(QStringLiteral("Inserting the moved item at row ") << newRow);
        beginInsertRows(QModelIndex(), newRow, newRow);
        index.push_back(itemCopy);
        endInsertRows();

        return true;
    }

    int newRow = static_cast<int>(std::distance(index.begin(), positionIter));

    NMTRACE(QStringLiteral("Inserting the moved item at row ") << newRow);
    beginInsertRows(QModelIndex(), newRow, newRow);
    Q_UNUSED(index.insert(positionIter, itemCopy))
    endInsertRows();

    return true;
}

bool NoteModel2::setDataImpl(const QModelIndex & modelIndex,
                             const QVariant & value,
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
                NMINFO(errorDescription << QStringLiteral(", item: ") << item);
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
                NMDEBUG(errorDescription << QStringLiteral(", item: ") << item);
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
                    NMDEBUG(errorDescription << QStringLiteral(", item: ")
                            << item << QStringLiteral("\nTimestamp: ")
                            << value);
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
                    NMDEBUG(errorDescription << QStringLiteral(", item: ")
                            << item << QStringLiteral("\nTimestamp: ") << value);
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
                    NMDEBUG(errorDescription << QStringLiteral(", item: ")
                            << item << QStringLiteral("\nTimestamp: ") << value);
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
        NMWARNING(QStringLiteral("Failed to update note item's row with respect ")
                  << QStringLiteral("to sorting: ") << errorDescription
                  << QStringLiteral(", note item: ") << item);
    }

    saveNoteInLocalStorage(item);
    return true;
}

bool NoteModel2::removeRowsImpl(int row, int count, ErrorString & errorDescription)
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
        NMTRACE(QStringLiteral("Emitting the request to expunge the note from ")
                << QStringLiteral("the local storage: request id = ")
                << requestId << QStringLiteral(", note local uid: ") << *it);
        Q_EMIT expungeNote(note, requestId);
    }

    endRemoveRows();

    return true;
}

bool NoteModel2::setNoteFavorited(const QString & noteLocalUid,
                                  const bool favorited,
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
        NMDEBUG(QStringLiteral("Favorited flag's value hasn't changed"));
        return true;
    }

    NoteModelItem itemCopy(item);
    itemCopy.setFavorited(favorited);

    localUidIndex.replace(it, itemCopy);
    saveNoteInLocalStorage(itemCopy);

    return true;
}

// WARNING: this method assumes the iterator passed to it is not end()
bool NoteModel2::moveNoteToNotebookImpl(NoteDataByLocalUid::iterator it,
                                        const Notebook & notebook,
                                        ErrorString & errorDescription)
{
    NoteModelItem item = *it;

    NMTRACE(QStringLiteral("NoteModel2::moveNoteToNotebookImpl: notebook = ")
            << notebook << QStringLiteral(", note item: ") << item);

    if (Q_UNLIKELY(item.notebookLocalUid() == notebook.localUid())) {
        NMDEBUG(QStringLiteral("The note is already within its target notebook, "
                               "nothing to do"));
        return true;
    }

    if (!notebook.canCreateNotes()) {
        errorDescription.setBase(QT_TR_NOOP("the target notebook doesn't allow "
                                            "to create notes in it"));
        NMINFO(errorDescription << QStringLiteral(", notebook: ") << notebook);
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

} // namespace quentier
