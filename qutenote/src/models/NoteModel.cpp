#include "NoteModel.h"
#include <qute_note/logging/QuteNoteLogger.h>
#include <qute_note/utility/UidGenerator.h>
#include <qute_note/utility/Utility.h>

// Limit for the queries to the local storage
#define NOTE_LIST_LIMIT (100)

#define NOTE_CACHE_LIMIT (20)

#define NUM_NOTE_MODEL_COLUMNS (10)

namespace qute_note {

NoteModel::NoteModel(LocalStorageManagerThreadWorker & localStorageManagerThreadWorker,
                     NotebookCache & notebookCache, QObject * parent) :
    QAbstractItemModel(parent),
    m_data(),
    m_listNotesOffset(0),
    m_listNotesRequestId(),
    m_noteItemsNotYetInLocalStorageUids(),
    m_cache(NOTE_CACHE_LIMIT),
    m_notebookCache(notebookCache),
    m_addNoteRequestIds(),
    m_updateNoteRequestIds(),
    m_deleteNoteRequestIds(),
    m_expungeNoteRequestIds(),
    m_findNoteToRestoreFailedUpdateRequestIds(),
    m_findNoteToPerformUpdateRequestIds(),
    m_sortedColumn(Columns::ModificationTimestamp),
    m_sortOrder(Qt::AscendingOrder),
    m_noteRestrictionsByNotebookLocalUid(),
    m_findNotebookRequestForNotebookLocalUid()
{
    createConnections(localStorageManagerThreadWorker);
    requestNoteList();
}

NoteModel::~NoteModel()
{}

QModelIndex NoteModel::indexForLocalUid(const QString & localUid) const
{
    const NoteDataByLocalUid & localUidIndex = m_data.get<ByLocalUid>();
    auto it = localUidIndex.find(localUid);
    if (Q_UNLIKELY(it == localUidIndex.end())) {
        QNDEBUG("Can't find the note item by local uid");
        return QModelIndex();
    }

    const NoteDataByIndex & index = m_data.get<ByIndex>();
    auto indexIt = m_data.project<ByIndex>(it);
    if (Q_UNLIKELY(indexIt == index.end())) {
        QNWARNING("Can't find the indexed reference to the note item: " << *it);
        return QModelIndex();
    }

    int rowIndex = static_cast<int>(std::distance(index.begin(), indexIt));
    return createIndex(rowIndex, Columns::Title);
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
        (column == Columns::Synchronizable))

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

    Columns::type column;
    switch(columnIndex)
    {
    case Columns::CreationTimestamp:
    case Columns::ModificationTimestamp:
    case Columns::Title:
    case Columns::PreviewText:
    case Columns::ThumbnailImageFilePath:
    case Columns::NotebookName:
    case Columns::TagNameList:
    case Columns::Size:
    case Columns::Synchronizable:
    case Columns::Dirty:
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
        return dataText(rowIndex, column);
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
    // NOTE: intentional fall-through
    case Columns::ThumbnailImageFilePath:
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

    int row= modelIndex.row();
    int column= modelIndex.column();

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
            QString title = value.toString();
            dirty |= (title != item.title());
            item.setTitle(title);
            break;
        }
    case Columns::Synchronizable:
        {
            if (item.isSynchronizable()) {
                QString error = QT_TR_NOOP("Can't make already synchronizable note not synchronizable");
                QNINFO(error << ", already synchronizable note item: " << item);
                emit notifyError(error);
                return false;
            }

            dirty |= (value.toBool() != item.isSynchronizable());
            item.setSynchronizable(value.toBool());
            break;
        }
    default:
        return false;
    }

    item.setDirty(dirty);
    item.setModificationTimestamp(QDateTime::currentMSecsSinceEpoch());

    index.replace(index.begin() + row, item);
    emit dataChanged(modelIndex, modelIndex);

    emit layoutAboutToBeChanged();
    updateItemRowWithRespectToSorting(item);
    emit layoutChanged();

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
        QNDEBUG("Ignoring the attempt to remove rows from note model for valid parent model index");
        return false;
    }

    if (Q_UNLIKELY((row + count) >= static_cast<int>(m_data.size())))
    {
        QString error = QT_TR_NOOP("Detected attempt to remove more rows than the note model contains: row") +
                        QStringLiteral(" = ") + QString::number(row) + QStringLiteral(", ") + QT_TR_NOOP("count") +
                        QStringLiteral(" = ") + QString::number(count) + QStringLiteral(", ") + QT_TR_NOOP("number of note model items") +
                        QStringLiteral(" = ") + QString::number(static_cast<int>(m_data.size()));
        QNINFO(error);
        emit notifyError(error);
        return false;
    }

    NoteDataByIndex & index = m_data.get<ByIndex>();

    for(int i = 0; i < count; ++i)
    {
        auto it = index.begin() + row;
        if (it->isSynchronizable()) {
            QString error = QT_TR_NOOP("Can't remove synchronizable note");
            QNINFO(error << ", synchronizable note item: " << *it);
            emit notifyError(error);
            return false;
        }
    }

    beginRemoveRows(QModelIndex(), row, row + count - 1);
    for(int i = 0; i < count; ++i)
    {
        auto it = index.begin() + row + i;

        Note note;
        note.setLocalUid(it->localUid());

        QUuid requestId = QUuid::createUuid();
        Q_UNUSED(m_expungeNoteRequestIds.insert(requestId))
        QNTRACE("Emitting the request to expunge the note from the local storage: request id = "
                << requestId << ", note local uid: " << it->localUid());
        emit expungeNote(note, requestId);
    }
    Q_UNUSED(index.erase(index.begin() + row, index.begin() + row + count))
    emit endRemoveRows();

    return true;
}

void NoteModel::sort(int column, Qt::SortOrder order)
{
    QNDEBUG("NoteModel::sort: column = " << column << ", order = " << order
            << " (" << (order == Qt::AscendingOrder ? "ascending" : "descending") << ")");

    if ( (column == Columns::ThumbnailImageFilePath) ||
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
            QNDEBUG("Neither sorted column nor sort order have changed, nothing to do");
            return;
        }

        m_sortOrder = order;

        QNDEBUG("Only the sort order has changed, reversing the index");

        emit layoutAboutToBeChanged();
        index.reverse();
        emit layoutChanged();

        return;
    }

    m_sortedColumn = static_cast<Columns::type>(column);
    m_sortOrder = order;

    emit layoutAboutToBeChanged();

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

    emit layoutChanged();
}

void NoteModel::onAddNoteComplete(Note note, QUuid requestId)
{
    QNDEBUG("NoteModel::onAddNoteComplete: " << note << "\nRequest id = " << requestId);

    auto it = m_addNoteRequestIds.find(requestId);
    if (it != m_addNoteRequestIds.end()) {
        Q_UNUSED(m_addNoteRequestIds.erase(it))
        return;
    }

    onNoteAddedOrUpdated(note);
}

void NoteModel::onAddNoteFailed(Note note, QString errorDescription, QUuid requestId)
{
    auto it = m_addNoteRequestIds.find(requestId);
    if (it == m_addNoteRequestIds.end()) {
        return;
    }

    QNDEBUG("NoteModel::onAddNoteFailed: note = " << note << "\nError description = " << errorDescription
            << ", request id = " << requestId);

    Q_UNUSED(m_addNoteRequestIds.erase(it))

    emit notifyError(errorDescription);
    removeItemByLocalUid(note.localUid());
}

void NoteModel::onUpdateNoteComplete(Note note, bool updateResources, bool updateTags, QUuid requestId)
{
    QNDEBUG("NoteModel::onUpdateNoteComplete: note = " << note << "\nRequest id = " << requestId);

    Q_UNUSED(updateResources)
    Q_UNUSED(updateTags)

    auto it = m_updateNoteRequestIds.find(requestId);
    if (it != m_updateNoteRequestIds.end()) {
        Q_UNUSED(m_updateNoteRequestIds.erase(it))
        return;
    }

    onNoteAddedOrUpdated(note);
}

void NoteModel::onUpdateNoteFailed(Note note, bool updateResources, bool updateTags,
                                   QString errorDescription, QUuid requestId)
{
    Q_UNUSED(updateResources)
    Q_UNUSED(updateTags)

    auto it = m_updateNoteRequestIds.find(requestId);
    if (it == m_updateNoteRequestIds.end()) {
        return;
    }

    QNDEBUG("NoteModel::onUpdateNoteFailed: note = " << note << "\nError description = " << errorDescription
            << ", request id = " << requestId);

    Q_UNUSED(m_updateNoteRequestIds.erase(it))

    requestId = QUuid::createUuid();
    Q_UNUSED(m_findNoteToRestoreFailedUpdateRequestIds.insert(requestId))
    QNTRACE("Emitting the request to find a note: local uid = " << note.localUid()
            << ", request id = " << requestId);
    emit findNote(note, /* with resource binary data = */ false, requestId);
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

    QNDEBUG("NoteModel::onFindNoteComplete: note = " << note << "\nRequest id = " << requestId);

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

void NoteModel::onFindNoteFailed(Note note, bool withResourceBinaryData, QString errorDescription, QUuid requestId)
{
    Q_UNUSED(withResourceBinaryData)

    auto restoreUpdateIt = m_findNoteToRestoreFailedUpdateRequestIds.find(requestId);
    auto performUpdateIt = m_findNoteToPerformUpdateRequestIds.find(requestId);
    if ((restoreUpdateIt == m_findNoteToRestoreFailedUpdateRequestIds.end()) &&
        (performUpdateIt == m_findNoteToPerformUpdateRequestIds.end()))
    {
        return;
    }

    QNDEBUG("NoteModel::onFindNoteFailed: note = " << note << "\nError description = " << errorDescription
            << ", request id = " << requestId);

    if (restoreUpdateIt != m_findNoteToRestoreFailedUpdateRequestIds.end()) {
        Q_UNUSED(m_findNoteToRestoreFailedUpdateRequestIds.erase(restoreUpdateIt))
    }
    else if (performUpdateIt != m_findNoteToPerformUpdateRequestIds.end()) {
        Q_UNUSED(m_findNoteToPerformUpdateRequestIds.erase(performUpdateIt))
    }

    emit notifyError(errorDescription);
}

void NoteModel::onListNotesComplete(LocalStorageManager::ListObjectsOptions flag, bool withResourceBinaryData,
                                    size_t limit, size_t offset, LocalStorageManager::ListNotesOrder::type order,
                                    LocalStorageManager::OrderDirection::type orderDirection,
                                    QList<Note> foundNotes, QUuid requestId)
{
    if (requestId != m_listNotesRequestId) {
        return;
    }

    QNDEBUG("NoteModel::onListNotesComplete: flag = " << flag << ", with resource binary data = "
            << (withResourceBinaryData ? "true" : "false") << ", limit = " << limit << ", offset = "
            << offset << ", order = " << order << ", direction = " << orderDirection << ", num found notes = "
            << foundNotes.size() << ", request id = " << requestId);

    for(auto it = foundNotes.begin(), end = foundNotes.end(); it != end; ++it) {
        onNoteAddedOrUpdated(*it);
    }

    m_listNotesRequestId = QUuid();

    if (foundNotes.size() == static_cast<int>(limit)) {
        QNTRACE("The number of found notes matches the limit, requesting more notes from the local storage");
        m_listNotesOffset += limit;
        requestNoteList();
    }
}

void NoteModel::onListNotesFailed(LocalStorageManager::ListObjectsOptions flag, bool withResourceBinaryData,
                                  size_t limit, size_t offset, LocalStorageManager::ListNotesOrder::type order,
                                  LocalStorageManager::OrderDirection::type orderDirection,
                                  QString errorDescription, QUuid requestId)
{
    if (requestId != m_listNotesRequestId) {
        return;
    }

    QNDEBUG("NoteModel::onListNotesFailed: flag = " << flag << ", with resource binary data = "
            << (withResourceBinaryData ? "true" : "false") << ", limit = " << limit
            << ", offset = " << offset << ", order = " << order << ", direction = " << orderDirection
            << ", error description = " << errorDescription << ", request id = " << requestId);

    m_listNotesRequestId = QUuid();

    emit notifyError(errorDescription);
}

void NoteModel::onDeleteNoteComplete(Note note, QUuid requestId)
{
    QNDEBUG("NoteModel::onDeleteNoteComplete: note = " << note << "\nRequest id = " << requestId);

    auto it = m_deleteNoteRequestIds.find(requestId);
    if (it != m_deleteNoteRequestIds.end()) {
        Q_UNUSED(m_deleteNoteRequestIds.erase(it))
        return;
    }

    onNoteAddedOrUpdated(note);
}

void NoteModel::onDeleteNoteFailed(Note note, QString errorDescription, QUuid requestId)
{
    auto it = m_deleteNoteRequestIds.find(requestId);
    if (it == m_deleteNoteRequestIds.end()) {
        return;
    }

    QNDEBUG("NoteModel::onDeleteNoteFailed: note = " << note << "\nError description = " << errorDescription
            << ", request id = " << requestId);

    Q_UNUSED(m_deleteNoteRequestIds.erase(it))

    note.setDeletionTimestamp(static_cast<qint64>(0));  // NOTE: it's a way to say setDeleted(false)
    onNoteAddedOrUpdated(note);
}

void NoteModel::onExpungeNoteComplete(Note note, QUuid requestId)
{
    QNDEBUG("NoteModel::onExpungeNoteComplete: note = " << note << "\nRequest id = " << requestId);

    auto it = m_expungeNoteRequestIds.find(requestId);
    if (it != m_expungeNoteRequestIds.end()) {
        Q_UNUSED(m_expungeNoteRequestIds.erase(it))
        return;
    }

    removeItemByLocalUid(note.localUid());
}

void NoteModel::onExpungeNoteFailed(Note note, QString errorDescription, QUuid requestId)
{
    auto it = m_expungeNoteRequestIds.find(requestId);
    if (it == m_expungeNoteRequestIds.end()) {
        return;
    }

    QNDEBUG("NoteModel::onExpungeNoteFailed: note = " << note << "\nError description = " << errorDescription
            << ", request id = " << requestId);

    Q_UNUSED(m_expungeNoteRequestIds.erase(it))

    onNoteAddedOrUpdated(note);
}

void NoteModel::onFindNotebookComplete(Notebook notebook, QUuid requestId)
{
    auto it = m_findNotebookRequestForNotebookLocalUid.right.find(requestId);
    if (it == m_findNotebookRequestForNotebookLocalUid.right.end()) {
        return;
    }

    QNDEBUG("NoteModel::onFindNotebookComplete: notebook: " << notebook << "\nRequest id = " << requestId);

    Q_UNUSED(m_findNotebookRequestForNotebookLocalUid.right.erase(it))

    m_notebookCache.put(notebook.localUid(), notebook);
    updateRestrictionsFromNotebook(notebook);
}

void NoteModel::onFindNotebookFailed(Notebook notebook, QString errorDescription, QUuid requestId)
{
    auto it = m_findNotebookRequestForNotebookLocalUid.right.find(requestId);
    if (it == m_findNotebookRequestForNotebookLocalUid.right.end()) {
        return;
    }

    QNWARNING("NoteModel::onFindNotebookFailed: notebook = " << notebook << "\nError description = "
              << errorDescription << ", request id = " << requestId);

    Q_UNUSED(m_findNotebookRequestForNotebookLocalUid.right.erase(it))
}

void NoteModel::onUpdateNotebookComplete(Notebook notebook, QUuid requestId)
{
    QNDEBUG("NoteModel::onUpdateNotebookComplete: local uid = " << notebook.localUid());
    Q_UNUSED(requestId)
    m_notebookCache.put(notebook.localUid(), notebook);
    updateRestrictionsFromNotebook(notebook);
}

void NoteModel::onExpungeNotebookComplete(Notebook notebook, QUuid requestId)
{
    QNDEBUG("NoteModel::onExpungeNotebookComplete: local uid = " << notebook.localUid());

    Q_UNUSED(requestId)
    Q_UNUSED(m_notebookCache.remove(notebook.localUid()))

    auto it = m_noteRestrictionsByNotebookLocalUid.find(notebook.localUid());
    if (it == m_noteRestrictionsByNotebookLocalUid.end()) {
        Restrictions restrictions;
        restrictions.m_canCreateNotes = false;
        restrictions.m_canUpdateNotes = false;
        m_noteRestrictionsByNotebookLocalUid[notebook.localUid()] = restrictions;
        return;
    }

    it->m_canCreateNotes = false;
    it->m_canUpdateNotes = false;
}

void NoteModel::createConnections(LocalStorageManagerThreadWorker & localStorageManagerThreadWorker)
{
    QNDEBUG("NoteModel::createConnections");

    // Local signals to localStorageManagerThreadWorker's slots
    QObject::connect(this, QNSIGNAL(NoteModel,addNote,Note,QUuid),
                     &localStorageManagerThreadWorker, QNSLOT(LocalStorageManagerThreadWorker,onAddNoteRequest,Note,bool,bool,QUuid));
    QObject::connect(this, QNSIGNAL(NoteModel,updateNote,Note,bool,bool,QUuid),
                     &localStorageManagerThreadWorker, QNSLOT(LocalStorageManagerThreadWorker,onUpdateNoteRequest,Note,QUuid));
    QObject::connect(this, QNSIGNAL(NoteModel,findNote,Note,bool,QUuid),
                     &localStorageManagerThreadWorker, QNSLOT(LocalStorageManagerThreadWorker,onFindNoteRequest,Note,bool,QUuid));
    QObject::connect(this, QNSIGNAL(NoteModel,listNotes,LocalStorageManager::ListObjectsOptions,bool,size_t,size_t,
                                    LocalStorageManager::ListNotesOrder::type,LocalStorageManager::OrderDirection::type,QUuid),
                     &localStorageManagerThreadWorker, QNSLOT(LocalStorageManagerThreadWorker,onListNotesRequest,bool,size_t,size_t,
                                                              LocalStorageManager::ListNotesOrder::type,LocalStorageManager::OrderDirection::type,QUuid));
    QObject::connect(this, QNSIGNAL(NoteModel,deleteNote,Note,QUuid),
                     &localStorageManagerThreadWorker, QNSLOT(LocalStorageManagerThreadWorker,onDeleteNoteRequest,Note,QUuid));
    QObject::connect(this, QNSIGNAL(NoteModel,expungeNote,Note,QUuid),
                     &localStorageManagerThreadWorker, QNSLOT(LocalStorageManagerThreadWorker,onExpungeNoteRequest,Note,QUuid));
    QObject::connect(this, QNSIGNAL(NoteModel,findNotebook,Notebook,QUuid),
                     &localStorageManagerThreadWorker, QNSLOT(LocalStorageManagerThreadWorker,onFindNotebookRequest,Notebook,QUuid));

    // localStorageManagerThreadWorker's signals to local slots
    QObject::connect(&localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,addNoteComplete,Note,QUuid),
                     this, QNSLOT(NoteModel,onAddNoteComplete,Note,QUuid));
    QObject::connect(&localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,addNoteFailed,Note,QString,QUuid),
                     this, QNSLOT(NoteModel,onAddNoteFailed,Note,QString,QUuid));
    QObject::connect(&localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,updateNoteComplete,Note,bool,bool,QUuid),
                     this, QNSLOT(NoteModel,onUpdateNoteComplete,Note,bool,bool,QUuid));
    QObject::connect(&localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,updateNoteFailed,Note,bool,bool,QString,QUuid),
                     this, QNSLOT(NoteModel,onUpdateNoteFailed,Note,bool,bool,QString,QUuid));
    QObject::connect(&localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,findNoteComplete,Note,bool,QUuid),
                     this, QNSLOT(NoteModel,onFindNoteComplete,Note,bool,QUuid));
    QObject::connect(&localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,findNoteFailed,Note,bool,QString,QUuid),
                     this, QNSLOT(NoteModel,onFindNoteFailed,Note,bool,QString,QUuid));
    QObject::connect(&localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,listNotesComplete,LocalStorageManager::ListObjectsOptions,bool,size_t,size_t,
                                                                LocalStorageManager::ListNotesOrder::type,LocalStorageManager::OrderDirection::type,
                                                                QList<Note>,QUuid),
                     this, QNSLOT(NoteModel,onListNotesComplete,LocalStorageManager::ListObjectsOptions,bool,size_t,size_t,
                                  LocalStorageManager::ListNotesOrder::type,LocalStorageManager::OrderDirection::type,QList<Note>,QUuid));
    QObject::connect(&localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,listNotesFailed,LocalStorageManager::ListObjectsOptions,bool,size_t,size_t,
                                                                LocalStorageManager::ListNotesOrder::type,LocalStorageManager::OrderDirection::type,QString,QUuid),
                     this, QNSLOT(NoteModel,onListNotesFailed,LocalStorageManager::ListObjectsOptions,bool,size_t,size_t,
                                  LocalStorageManager::ListNotesOrder::type,LocalStorageManager::OrderDirection::type,QString,QUuid));
    QObject::connect(&localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,deleteNoteComplete,Note,QUuid),
                     this, QNSLOT(NoteModel,onDeleteNoteComplete,Note,QUuid));
    QObject::connect(&localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,deleteNoteFailed,Note,QString,QUuid),
                     this, QNSLOT(NoteModel,onDeleteNoteFailed,Note,QString,QUuid));
    QObject::connect(&localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,expungeNoteComplete,Note,QUuid),
                     this, QNSLOT(NoteModel,onExpungeNoteComplete,Note,QUuid));
    QObject::connect(&localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,expungeNoteFailed,Note,QString,QUuid),
                     this, QNSLOT(NoteModel,onExpungeNoteFailed,Note,QString,QUuid));
    QObject::connect(&localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,findNotebookComplete,Notebook,QUuid),
                     this, QNSLOT(NoteModel,onFindNotebookComplete,Notebook,QUuid));
    QObject::connect(&localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,findNotebookFailed,Notebook,QString,QUuid),
                     this, QNSLOT(NoteModel,onFindNotebookFailed,Notebook,QString,QUuid));
    QObject::connect(&localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,updateNotebookComplete,Notebook,QUuid),
                     this, QNSLOT(NoteModel,onUpdateNotebookComplete,Notebook,QUuid));
    QObject::connect(&localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,expungeNotebookComplete,Notebook,QUuid),
                     this, QNSLOT(NoteModel,onExpungeNotebookComplete,Notebook,QUuid));
}

void NoteModel::requestNoteList()
{
    QNDEBUG("NoteModel::requestNoteList: offset = " << m_listNotesOffset);

    LocalStorageManager::ListObjectsOptions flags = LocalStorageManager::ListAll;
    LocalStorageManager::ListNotesOrder::type order = LocalStorageManager::ListNotesOrder::NoOrder;
    LocalStorageManager::OrderDirection::type direction = LocalStorageManager::OrderDirection::Ascending;

    m_listNotesRequestId = QUuid::createUuid();
    QNTRACE("Emitting the request to list notes: offset = " << m_listNotesOffset << ", request id = " << m_listNotesRequestId);
    emit listNotes(flags, /* with resource binary data = */ false, NOTE_LIST_LIMIT, m_listNotesOffset, order, direction, m_listNotesRequestId);
}

QVariant NoteModel::dataText(const int row, const Columns::type column) const
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
    case Columns::Title:
        return item.title();
    case Columns::PreviewText:
        return item.previewText();
    case Columns::ThumbnailImageFilePath:
        return item.thumbnailImageFilePath();
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

    QString space(" ");
    QString colon(":");
    QString accessibleText = QT_TR_NOOP("Note") + colon + space;

    switch(column)
    {
    case Columns::CreationTimestamp:
        {
            if (item.creationTimestamp() == 0) {
                accessibleText += QT_TR_NOOP("creation time is not set");
            }
            else {
                accessibleText += QT_TR_NOOP("was created at") + space +
                                  printableDateTimeFromTimestamp(item.creationTimestamp());
            }
            break;
        }
    case Columns::ModificationTimestamp:
        {
            if (item.modificationTimestamp() == 0) {
                accessibleText += QT_TR_NOOP("last modification timestamp is not set");
            }
            else {
                accessibleText += QT_TR_NOOP("was last modified at") + space +
                                  printableDateTimeFromTimestamp(item.modificationTimestamp());
            }
            break;
        }
    case Columns::Title:
        {
            const QString & title = item.title();
            if (title.isEmpty()) {
                accessibleText += QT_TR_NOOP("title is not set");
            }
            else {
                accessibleText += QT_TR_NOOP("title is") + space + title;
            }
            break;
        }
    case Columns::PreviewText:
        {
            const QString & previewText = item.previewText();
            if (previewText.isEmpty()) {
                accessibleText += QT_TR_NOOP("preview text is not available");
            }
            else {
                accessibleText += QT_TR_NOOP("preview text") + colon + space + previewText;
            }
            break;
        }
    case Columns::NotebookName:
        {
            const QString & notebookName = item.notebookName();
            if (notebookName.isEmpty()) {
                accessibleText += QT_TR_NOOP("notebook name is not available");
            }
            else {
                accessibleText += QT_TR_NOOP("notebook name is") + space + notebookName;
            }
            break;
        }
    case Columns::TagNameList:
        {
            const QStringList & tagNameList = item.tagNameList();
            if (tagNameList.isEmpty()) {
                accessibleText += QT_TR_NOOP("tag list is empty");
            }
            else {
                accessibleText += QT_TR_NOOP("has tags") + colon + space + tagNameList.join(", ");
            }
            break;
        }
    case Columns::Size:
        {
            const quint64 bytes = item.sizeInBytes();
            if (bytes == 0) {
                accessibleText += QT_TR_NOOP("size is not available");
            }
            else {
                accessibleText += QT_TR_NOOP("size is") + space + humanReadableSize(bytes);
            }
            break;
        }
    case Columns::Synchronizable:
        accessibleText += (item.isSynchronizable() ? QT_TR_NOOP("synchronizable") : QT_TR_NOOP("not synchronizable"));
        break;
    case Columns::Dirty:
        accessibleText += (item.isDirty() ? QT_TR_NOOP("dirty") : QT_TR_NOOP("not dirty"));
        break;
    case Columns::ThumbnailImageFilePath:
    default:
        return QVariant();
    }

    return accessibleText;
}

void NoteModel::removeItemByLocalUid(const QString & localUid)
{
    QNDEBUG("NoteModel::removeItemByLocalUid: " << localUid);

    NoteDataByLocalUid & localUidIndex = m_data.get<ByLocalUid>();
    auto itemIt = localUidIndex.find(localUid);
    if (Q_UNLIKELY(itemIt == localUidIndex.end())) {
        QNDEBUG("Can't find item to remove from the note model");
        return;
    }

    const NoteModelItem & item = *itemIt;

    NoteDataByIndex & index = m_data.get<ByIndex>();
    auto indexIt = m_data.project<ByIndex>(itemIt);
    if (Q_UNLIKELY(indexIt == index.end())) {
        QNWARNING("Can't determine the row index for the note model item to remove: " << item);
        return;
    }

    int row = static_cast<int>(std::distance(index.begin(), indexIt));
    if (Q_UNLIKELY((row < 0) || (row >= static_cast<int>(m_data.size())))) {
        QNWARNING("Invalid row index for the note model item to remove: index = " << row
                  << ", item: " << item);
        return;
    }

    beginRemoveRows(QModelIndex(), row, row);
    Q_UNUSED(localUidIndex.erase(itemIt))
    endRemoveRows();
}

void NoteModel::updateItemRowWithRespectToSorting(const NoteModelItem & item)
{
    NoteDataByIndex & index = m_data.get<ByIndex>();

    auto it = index.iterator_to(item);
    if (Q_UNLIKELY(it == index.end())) {
        QNWARNING("Can't update item row with respect to sorting: can't find item's original row; item: " << item);
        return;
    }

    int originalRow = static_cast<int>(std::distance(index.begin(), it));
    if (Q_UNLIKELY((originalRow < 0) || (originalRow >= static_cast<int>(m_data.size())))) {
        QNWARNING("Can't update item row with respect to sorting: item's original row is beyond the acceptable range: "
                  << originalRow << ", item: " << item);
        return;
    }

    NoteModelItem itemCopy(item);
    Q_UNUSED(index.erase(it))

    auto positionIter = std::lower_bound(index.begin(), index.end(), itemCopy,
                                         NoteComparator(m_sortedColumn, m_sortOrder));
    if (positionIter == index.end()) {
        index.push_back(itemCopy);
        return;
    }

    index.insert(positionIter, itemCopy);
}

void NoteModel::updateNoteInLocalStorage(const NoteModelItem & item)
{
    QNDEBUG("NoteModel::updateNoteInLocalStorage: local uid = " << item.localUid());

    Note note;

    auto notYetSavedItemIt = m_noteItemsNotYetInLocalStorageUids.find(item.localUid());
    if (notYetSavedItemIt == m_noteItemsNotYetInLocalStorageUids.end())
    {
        QNDEBUG("Updating the note");

        const Note * pCachedNote = m_cache.get(item.localUid());
        if (Q_UNLIKELY(!pCachedNote))
        {
            QUuid requestId = QUuid::createUuid();
            Q_UNUSED(m_findNoteToPerformUpdateRequestIds.insert(requestId))
            Note dummy;
            dummy.setLocalUid(item.localUid());
            QNTRACE("Emitting the request to find note: local uid = " << item.localUid()
                    << ", request id = " << requestId);
            emit findNote(dummy, /* with resource binary data = */ false, requestId);
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
    note.setTitle(item.title());
    note.setLocal(!item.isSynchronizable());
    note.setDirty(item.isDirty());

    // TODO: think of whether it should be possible to set the ENML content of the new note here

    m_cache.put(note.localUid(), note);

    QUuid requestId = QUuid::createUuid();

    if (notYetSavedItemIt != m_noteItemsNotYetInLocalStorageUids.end())
    {
        Q_UNUSED(m_addNoteRequestIds.insert(requestId))

        QNTRACE("Emitting the request to add the note to local storage: id = " << requestId
                << ", note: " << note);
        emit addNote(note, requestId);

        Q_UNUSED(m_noteItemsNotYetInLocalStorageUids.erase(notYetSavedItemIt))
    }
    else
    {
        Q_UNUSED(m_updateNoteRequestIds.insert(requestId))

        QNTRACE("Emitting the request to update the note in local storage: id = " << requestId
                << ", note: " << note);
        emit updateNote(note, /* update resources = */ false, /* update tags = */ false, requestId);
    }
}

bool NoteModel::canUpdateNoteItem(const NoteModelItem & item) const
{
    auto it = m_noteRestrictionsByNotebookLocalUid.find(item.notebookLocalUid());
    if (it == m_noteRestrictionsByNotebookLocalUid.end()) {
        return false;
    }

    const Restrictions & restrictions = it.value();
    return restrictions.m_canUpdateNotes;
}

bool NoteModel::canCreateNoteItem(const QString & notebookLocalUid) const
{
    auto it = m_noteRestrictionsByNotebookLocalUid.find(notebookLocalUid);
    if (it == m_noteRestrictionsByNotebookLocalUid.end()) {
        return false;
    }

    const Restrictions & restrictions = it.value();
    return restrictions.m_canCreateNotes;
}

void NoteModel::updateRestrictionsFromNotebook(const Notebook & notebook)
{
    QNDEBUG("NoteModel::updateRestrictionsFromNotebook: local uid = " << notebook.localUid());

    Restrictions restrictions;

    if (!notebook.hasRestrictions())
    {
        restrictions.m_canCreateNotes = true;
        restrictions.m_canUpdateNotes = true;
    }
    else
    {
        const qevercloud::NotebookRestrictions & notebookRestrictions = notebook.restrictions();
        restrictions.m_canCreateNotes = (notebookRestrictions.noCreateNotes.isSet()
                                         ? (!notebookRestrictions.noCreateNotes.ref())
                                         : true);
        restrictions.m_canUpdateNotes = (notebookRestrictions.noUpdateNotes.isSet()
                                         ? (!notebookRestrictions.noUpdateNotes.ref())
                                         : true);
    }

    m_noteRestrictionsByNotebookLocalUid[notebook.localUid()] = restrictions;
    QNDEBUG("Set restrictions for notes from notebook with local uid " << notebook.localUid()
            << ": can create notes = " << (restrictions.m_canCreateNotes ? "true" : "false")
            << ": can update notes = " << (restrictions.m_canUpdateNotes ? "true" : "false"));
}

void NoteModel::onNoteAddedOrUpdated(const Note & note, const Notebook * pNotebook)
{
    NoteDataByLocalUid & localUidIndex = m_data.get<ByLocalUid>();

    m_cache.put(note.localUid(), note);

    auto itemIt = localUidIndex.find(note.localUid());
    bool newNote = (itemIt == localUidIndex.end());
    if (newNote) {
        onNoteAdded(note, pNotebook);
    }
    else {
        onNoteUpdated(note, pNotebook, itemIt);
    }
}

void NoteModel::onNoteAdded(const Note & note, const Notebook * pNotebook)
{
    QNDEBUG("NoteModel::onNoteAdded: note local uid = " << note.localUid());

    NoteModelItem item;
    noteToItem(note, item);

    // TODO: implement
    Q_UNUSED(note)
    Q_UNUSED(pNotebook)
}

void NoteModel::onNoteUpdated(const Note & note, const Notebook * pNotebook, NoteDataByLocalUid::iterator it)
{
    // TODO: implement
    Q_UNUSED(note)
    Q_UNUSED(pNotebook)
    Q_UNUSED(it)
}

void NoteModel::noteToItem(const Note & note, NoteModelItem & item)
{
    // TODO: implement
    Q_UNUSED(note)
    Q_UNUSED(item)
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
    case Columns::Title:
        {
            int compareResult = lhs.title().localeAwareCompare(rhs.title());
            less = (compareResult < 0);
            greater = (compareResult > 0);
            break;
        }
    case Columns::PreviewText:
        {
            int compareResult = lhs.previewText().localeAwareCompare(rhs.previewText());
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
    default:
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

} // namespace qute_note
