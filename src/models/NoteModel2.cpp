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

int NoteModel2::sortingColumn() const
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

    // TODO: implement further
    return QModelIndex();
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

} // namespace quentier
