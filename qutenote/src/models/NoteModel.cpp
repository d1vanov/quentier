#include "NoteModel.h"
#include <qute_note/logging/QuteNoteLogger.h>
#include <qute_note/utility/UidGenerator.h>
#include <qute_note/utility/Utility.h>
#include <qute_note/types/ResourceAdapter.h>

// Limit for the queries to the local storage
#define NOTE_LIST_LIMIT (100)

#define NUM_NOTE_MODEL_COLUMNS (11)

namespace qute_note {

NoteModel::NoteModel(LocalStorageManagerThreadWorker & localStorageManagerThreadWorker, NoteCache & noteCache,
                     NotebookCache & notebookCache, QObject * parent, const IncludedNotes::type includedNotes) :
    QAbstractItemModel(parent),
    m_includedNotes(includedNotes),
    m_data(),
    m_listNotesOffset(0),
    m_listNotesRequestId(),
    m_noteItemsNotYetInLocalStorageUids(),
    m_cache(noteCache),
    m_notebookCache(notebookCache),
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
    m_tagDataByTagLocalUid(),
    m_findTagRequestForTagLocalUid(),
    m_tagLocalUidToNoteLocalUid()
{
    createConnections(localStorageManagerThreadWorker);
    requestNotesList();
}

NoteModel::~NoteModel()
{}

QModelIndex NoteModel::indexForLocalUid(const QString & localUid) const
{
    const NoteDataByLocalUid & localUidIndex = m_data.get<ByLocalUid>();
    auto it = localUidIndex.find(localUid);
    if (Q_UNLIKELY(it == localUidIndex.end())) {
        QNDEBUG("Can't find the note item by local uid: " << localUid);
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

const NoteModelItem * NoteModel::itemForLocalUid(const QString & localUid) const
{
    const NoteDataByLocalUid & localUidIndex = m_data.get<ByLocalUid>();
    auto it = localUidIndex.find(localUid);
    if (Q_UNLIKELY(it == localUidIndex.end())) {
        QNDEBUG("Can't find the note item by local uid");
        return Q_NULLPTR;
    }

    return &(*it);
}

const NoteModelItem *NoteModel::itemAtRow(const int row) const
{
    const NoteDataByIndex & index = m_data.get<ByIndex>();
    if (Q_UNLIKELY((row < 0) || (index.size() <= static_cast<size_t>(row)))) {
        QNDEBUG("Detected attempt to get the note model item for non-existing row");
        return Q_NULLPTR;
    }

    return &(index[static_cast<size_t>(row)]);
}

QModelIndex NoteModel::createNoteItem(const QString & notebookLocalUid)
{
    auto notebookIt = m_notebookDataByNotebookLocalUid.find(notebookLocalUid);
    if (notebookIt == m_notebookDataByNotebookLocalUid.end()) {
        QString error = QT_TR_NOOP("Can't create new note: can't identify the notebook");
        QNWARNING(error << ", notebook local uid = " << notebookLocalUid);
        emit notifyError(error);
        return QModelIndex();
    }

    const NotebookData & notebookData = notebookIt.value();
    if (!notebookData.m_canCreateNotes) {
        QString error = QT_TR_NOOP("Can't create new note: notebook restrictions apply");
        QNWARNING(error << ", notebook local uid = " << notebookLocalUid << ", notebook name = "
                  << notebookData.m_name);
        emit notifyError(error);
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
    case Columns::DeletionTimestamp:
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
    case Columns::DeletionTimestamp:
        {
            qint64 timestamp = 0;

            if (!value.isNull())
            {
                bool conversionResult = false;
                timestamp = value.toLongLong(&conversionResult);

                if (!conversionResult) {
                    QString error = QT_TR_NOOP("Can't change the deleted state of the note: wrong deletion timestamp value");
                    QNINFO(error);
                    emit notifyError(error);
                    return false;
                }
            }

            dirty |= (timestamp != item.deletionTimestamp());
            item.setDeletionTimestamp(timestamp);
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
        QNTRACE("Emitting the request to expunge the note from the local storage: request id = "
                << requestId << ", note local uid: " << *it);
        emit expungeNote(note, requestId);
    }

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
        requestNotesList();
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
    updateNotebookData(notebook);
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

void NoteModel::onAddNotebookComplete(Notebook notebook, QUuid requestId)
{
    QNDEBUG("NoteModel::onAddNotebookComplete: local uid = " << notebook.localUid());
    Q_UNUSED(requestId)
    m_notebookCache.put(notebook.localUid(), notebook);
    updateNotebookData(notebook);
}

void NoteModel::onUpdateNotebookComplete(Notebook notebook, QUuid requestId)
{
    QNDEBUG("NoteModel::onUpdateNotebookComplete: local uid = " << notebook.localUid());
    Q_UNUSED(requestId)
    m_notebookCache.put(notebook.localUid(), notebook);
    updateNotebookData(notebook);
}

void NoteModel::onExpungeNotebookComplete(Notebook notebook, QUuid requestId)
{
    QNDEBUG("NoteModel::onExpungeNotebookComplete: local uid = " << notebook.localUid());

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

    QNDEBUG("NoteModel::onFindTagComplete: tag: " << tag << "\nRequest id = " << requestId);

    Q_UNUSED(m_findTagRequestForTagLocalUid.right.erase(it))

    updateTagData(tag);
}

void NoteModel::onFindTagFailed(Tag tag, QString errorDescription, QUuid requestId)
{
    auto it = m_findTagRequestForTagLocalUid.right.find(requestId);
    if (it == m_findTagRequestForTagLocalUid.right.end()) {
        return;
    }

    QNWARNING("NoteModel::onFindTagFailed: tag: " << tag << "\nError description = "
              << errorDescription << ", request id = " << requestId);

    Q_UNUSED(m_findTagRequestForTagLocalUid.right.erase(it))

    emit notifyError(errorDescription);
}

void NoteModel::onAddTagComplete(Tag tag, QUuid requestId)
{
    QNDEBUG("NoteModel::onAddTagComplete: tag = " << tag << ", request id = " << requestId);
    updateTagData(tag);
}

void NoteModel::onUpdateTagComplete(Tag tag, QUuid requestId)
{
    QNDEBUG("NoteModel::onUpdateTagComplete: tag = " << tag << ", request id = " << requestId);
    updateTagData(tag);
}

void NoteModel::onExpungeTagComplete(Tag tag, QUuid requestId)
{
    QNDEBUG("NoteModel::onExpungeTagComplete: tag = " << tag << ", request id = " << requestId);

    auto it = m_tagDataByTagLocalUid.find(tag.localUid());
    if (it == m_tagDataByTagLocalUid.end()) {
        QNTRACE("Tag data corresponding to the expunged tag was not found within the note model");
        return;
    }

    QString tagGuid = it->m_guid;
    QString tagName = it->m_name;

    Q_UNUSED(m_tagDataByTagLocalUid.erase(it))

    auto noteIt = m_tagLocalUidToNoteLocalUid.find(tag.localUid());
    if (noteIt == m_tagLocalUidToNoteLocalUid.end()) {
        return;
    }

    NoteDataByLocalUid & localUidIndex = m_data.get<ByLocalUid>();

    while(noteIt != m_tagLocalUidToNoteLocalUid.end())
    {
        if (noteIt.key() != tag.localUid()) {
            break;
        }

        auto noteItemIt = localUidIndex.find(noteIt.value());
        if (Q_UNLIKELY(noteItemIt == localUidIndex.end())) {
            QNDEBUG("Can't find the note pointed to by the expunged tag by local uid: note local uid = " << noteIt.value());
            Q_UNUSED(m_tagLocalUidToNoteLocalUid.erase(noteIt++))
            continue;
        }

        NoteModelItem item = *noteItemIt;

        item.removeTagGuid(tagGuid);
        item.removeTagName(tagName);

        Q_UNUSED(localUidIndex.replace(noteItemIt, item))
        ++noteIt;

        QModelIndex modelIndex = indexForLocalUid(item.localUid());
        modelIndex = createIndex(modelIndex.row(), Columns::TagNameList);
        emit dataChanged(modelIndex, modelIndex);
    }
}

void NoteModel::createConnections(LocalStorageManagerThreadWorker & localStorageManagerThreadWorker)
{
    QNDEBUG("NoteModel::createConnections");

    // Local signals to localStorageManagerThreadWorker's slots
    QObject::connect(this, QNSIGNAL(NoteModel,addNote,Note,QUuid),
                     &localStorageManagerThreadWorker, QNSLOT(LocalStorageManagerThreadWorker,onAddNoteRequest,Note,QUuid));
    QObject::connect(this, QNSIGNAL(NoteModel,updateNote,Note,bool,bool,QUuid),
                     &localStorageManagerThreadWorker, QNSLOT(LocalStorageManagerThreadWorker,onUpdateNoteRequest,Note,bool,bool,QUuid));
    QObject::connect(this, QNSIGNAL(NoteModel,findNote,Note,bool,QUuid),
                     &localStorageManagerThreadWorker, QNSLOT(LocalStorageManagerThreadWorker,onFindNoteRequest,Note,bool,QUuid));
    QObject::connect(this, QNSIGNAL(NoteModel,listNotes,LocalStorageManager::ListObjectsOptions,bool,size_t,size_t,
                                    LocalStorageManager::ListNotesOrder::type,LocalStorageManager::OrderDirection::type,QUuid),
                     &localStorageManagerThreadWorker, QNSLOT(LocalStorageManagerThreadWorker,onListNotesRequest,
                                                              LocalStorageManager::ListObjectsOptions,bool,size_t,size_t,
                                                              LocalStorageManager::ListNotesOrder::type,
                                                              LocalStorageManager::OrderDirection::type,QUuid));
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
    QObject::connect(&localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,expungeNoteComplete,Note,QUuid),
                     this, QNSLOT(NoteModel,onExpungeNoteComplete,Note,QUuid));
    QObject::connect(&localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,expungeNoteFailed,Note,QString,QUuid),
                     this, QNSLOT(NoteModel,onExpungeNoteFailed,Note,QString,QUuid));
    QObject::connect(&localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,findNotebookComplete,Notebook,QUuid),
                     this, QNSLOT(NoteModel,onFindNotebookComplete,Notebook,QUuid));
    QObject::connect(&localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,findNotebookFailed,Notebook,QString,QUuid),
                     this, QNSLOT(NoteModel,onFindNotebookFailed,Notebook,QString,QUuid));
    QObject::connect(&localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,addNotebookComplete,Notebook,QUuid),
                     this, QNSLOT(NoteModel,onAddNotebookComplete,Notebook,QUuid));
    QObject::connect(&localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,updateNotebookComplete,Notebook,QUuid),
                     this, QNSLOT(NoteModel,onUpdateNotebookComplete,Notebook,QUuid));
    QObject::connect(&localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,expungeNotebookComplete,Notebook,QUuid),
                     this, QNSLOT(NoteModel,onExpungeNotebookComplete,Notebook,QUuid));
    QObject::connect(&localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,findTagComplete,Tag,QUuid),
                     this, QNSLOT(NoteModel,onFindTagComplete,Tag,QUuid));
    QObject::connect(&localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,findTagFailed,Tag,QString,QUuid),
                     this, QNSLOT(NoteModel,onFindTagFailed,Tag,QString,QUuid));
    QObject::connect(&localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,addTagComplete,Tag,QUuid),
                     this, QNSLOT(NoteModel,onAddTagComplete,Tag,QUuid));
    QObject::connect(&localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,updateTagComplete,Tag,QUuid),
                     this, QNSLOT(NoteModel,onUpdateTagComplete,Tag,QUuid));
    QObject::connect(&localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,expungeTagComplete,Tag,QUuid),
                     this, QNSLOT(NoteModel,onExpungeTagComplete,Tag,QUuid));
}

void NoteModel::requestNotesList()
{
    QNDEBUG("NoteModel::requestNotesList: offset = " << m_listNotesOffset);

    LocalStorageManager::ListObjectsOptions flags = LocalStorageManager::ListAll;
    LocalStorageManager::ListNotesOrder::type order = LocalStorageManager::ListNotesOrder::NoOrder;
    LocalStorageManager::OrderDirection::type direction = LocalStorageManager::OrderDirection::Ascending;

    m_listNotesRequestId = QUuid::createUuid();
    QNTRACE("Emitting the request to list notes: offset = " << m_listNotesOffset << ", request id = " << m_listNotesRequestId);
    emit listNotes(flags, /* with resource binary data = */ false, NOTE_LIST_LIMIT, m_listNotesOffset, order, direction, m_listNotesRequestId);
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
    case Columns::ThumbnailImageFilePath:
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
    case Columns::DeletionTimestamp:
        {
            if (item.deletionTimestamp() == 0) {
                accessibleText += QT_TR_NOOP("deletion timestamp is not set");
            }
            else {
                accessibleText += QT_TR_NOOP("deleted at") + space +
                                  printableDateTimeFromTimestamp(item.deletionTimestamp());
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

    Q_UNUSED(index.insert(positionIter, itemCopy))
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
    note.setDeletionTimestamp(item.deletionTimestamp());
    note.setTagLocalUids(item.tagLocalUids());
    note.setTagGuids(item.tagGuids());
    note.setTitle(item.title());
    note.setLocal(!item.isSynchronizable());
    note.setDirty(item.isDirty());

    m_cache.put(note.localUid(), note);

    QUuid requestId = QUuid::createUuid();

    if (notYetSavedItemIt != m_noteItemsNotYetInLocalStorageUids.end())
    {
        Q_UNUSED(m_addNoteRequestIds.insert(requestId))
        Q_UNUSED(m_noteItemsNotYetInLocalStorageUids.erase(notYetSavedItemIt))

        QNTRACE("Emitting the request to add the note to local storage: id = " << requestId
                << ", note: " << note);
        emit addNote(note, requestId);
    }
    else
    {
        Q_UNUSED(m_updateNoteRequestIds.insert(requestId))

        QNTRACE("Emitting the request to update the note in local storage: id = " << requestId
                << ", note: " << note);
        emit updateNote(note, /* update resources = */ false, /* update tags = */ false, requestId);
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
        QNDEBUG("Can't find the notebook data for note with local uid " << item.localUid());
        return false;
    }

    const NotebookData & notebookData = it.value();
    return notebookData.m_canUpdateNotes;
}

bool NoteModel::canCreateNoteItem(const QString & notebookLocalUid) const
{
    if (notebookLocalUid.isEmpty()) {
        QNDEBUG("NoteModel::canCreateNoteItem: empty notebook local uid");
        return false;
    }

    auto it = m_notebookDataByNotebookLocalUid.find(notebookLocalUid);
    if (it != m_notebookDataByNotebookLocalUid.end()) {
        return it->m_canCreateNotes;
    }

    QNDEBUG("Can't find the notebook data for notebook local uid " << notebookLocalUid);
    return false;
}

void NoteModel::updateNotebookData(const Notebook & notebook)
{
    QNDEBUG("NoteModel::updateNotebookData: local uid = " << notebook.localUid());

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

    QNDEBUG("Collected notebook data from notebook with local uid " << notebook.localUid()
            << ": guid = " << notebookData.m_guid << "; name = " << notebookData.m_name
            << ": can create notes = " << (notebookData.m_canCreateNotes ? "true" : "false")
            << ": can update notes = " << (notebookData.m_canUpdateNotes ? "true" : "false"));

    checkAddedNoteItemsPendingNotebookData(notebook.localUid(), notebookData);
}

void NoteModel::updateTagData(const Tag & tag)
{
    QNDEBUG("NoteModel::updateTagData: tag local uid = " << tag.localUid());

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

    while(noteIt != m_tagLocalUidToNoteLocalUid.end())
    {
        if (noteIt.key() != tag.localUid()) {
            break;
        }

        auto noteItemIt = localUidIndex.find(noteIt.value());
        if (Q_UNLIKELY(noteItemIt == localUidIndex.end())) {
            QNDEBUG("Can't find the note pointed to by a tag by local uid: note local uid = " << noteIt.value());
            Q_UNUSED(m_tagLocalUidToNoteLocalUid.erase(noteIt++))
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
                QNTRACE("Still no tag data for tag with local uid " << *tagLocalUidIt);
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
        ++noteIt;

        QModelIndex modelIndex = indexForLocalUid(item.localUid());
        modelIndex = createIndex(modelIndex.row(), Columns::TagNameList);
        emit dataChanged(modelIndex, modelIndex);
    }
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
    m_cache.put(note.localUid(), note);

    QNDEBUG("NoteModel::onNoteAddedOrUpdated: note local uid = " << note.localUid());

    if (!note.hasNotebookLocalUid()) {
        QNWARNING("Skipping the note not having the notebook local uid: " << note);
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
            QNTRACE("Emitting the request to find notebook local uid: " << " = " << item.notebookLocalUid() << ", request id = " << requestId);
            emit findNotebook(notebook, requestId);
        }

        return;
    }

    const NotebookData & notebookData = notebookIt.value();
    addOrUpdateNoteItem(item, notebookData);
}

void NoteModel::addOrUpdateNoteItem(NoteModelItem & item, const NotebookData & notebookData)
{
    QNDEBUG("NoteModel::addOrUpdateNoteItem: note local uid = " << item.localUid() << ", notebook local uid = "
            << item.notebookLocalUid() << ", notebook name = " << notebookData.m_name);

    if (!notebookData.m_canCreateNotes)
    {
        QString error = QT_TR_NOOP("Can't create a new note in the notebook") + QStringLiteral(" ") + notebookData.m_name +
                        QStringLiteral(": ") + QT_TR_NOOP("notebook restrictions apply");
        QNINFO(error);
        emit notifyError(error);
        return;
    }

    item.setNotebookName(notebookData.m_name);

    const QStringList & tagLocalUids = item.tagLocalUids();
    if (!tagLocalUids.isEmpty())
    {
        for(auto it = tagLocalUids.begin(), end = tagLocalUids.end(); it != end; ++it)
        {
            const QString & tagLocalUid = *it;

            Q_UNUSED(m_tagLocalUidToNoteLocalUid.insert(tagLocalUid, item.localUid()))

            auto tagDataIt = m_tagDataByTagLocalUid.find(tagLocalUid);
            if (tagDataIt != m_tagDataByTagLocalUid.end()) {
                QNTRACE("Found tag data for tag local uid " << tagLocalUid
                        << ": tag name = " << tagDataIt->m_name);
                item.addTagName(tagDataIt->m_name);
                continue;
            }

            QNTRACE("Tag data for tag local uid " << tagLocalUid << " was not found");

            auto requestIt = m_findTagRequestForTagLocalUid.left.find(tagLocalUid);
            if (requestIt != m_findTagRequestForTagLocalUid.left.end()) {
                QNTRACE("The request to find tag corresponding to local uid " << tagLocalUid
                        << " has already been sent: request id = " << requestIt->second);
                continue;
            }

            QUuid requestId = QUuid::createUuid();
            Q_UNUSED(m_findTagRequestForTagLocalUid.insert(LocalUidToRequestIdBimap::value_type(tagLocalUid, requestId)))

            Tag tag;
            tag.setLocalUid(tagLocalUid);
            QNTRACE("Emitting the request to find tag: tag local uid = " << tagLocalUid << ", request id = " << requestId);
            emit findTag(tag, requestId);
        }
    }

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
                if (item.deletionTimestamp() == 0) {
                    QNDEBUG("Won't add note as it's not deleted and the model instance only considers the deleted notes");
                    return;
                }
                break;
            }
        case IncludedNotes::NonDeleted:
            {
                if (item.deletionTimestamp() != 0) {
                    QNDEBUG("Won't add note as it's deleted and the model instance only considers the non-deleted notes");
                    return;
                }
                break;
            }
        }

        Q_UNUSED(localUidIndex.insert(item))
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
                if (item.deletionTimestamp() == 0)
                {
                    QNDEBUG("Removing the updated note item from the model as the item is not deleted "
                            "and the model instance only considers the deleted notes");
                    shouldRemoveItem = true;
                }
                break;
            }
        case IncludedNotes::NonDeleted:
            {
                if (item.deletionTimestamp() != 0)
                {
                    QNDEBUG("Removing the updated note item from the model as the item is deleted "
                            "and the model instance only considers the non-deleted notes");
                    shouldRemoveItem = true;
                }
                break;
            }
        }

        NoteDataByIndex & index = m_data.get<ByIndex>();
        auto indexIt = m_data.project<ByIndex>(it);
        if (Q_UNLIKELY(indexIt == index.end())) {
            QString error = QT_TR_NOOP("Can't project the local uid index to the note item to the random access index iterator");
            QNWARNING(error << ", note model item: " << item);
            emit notifyError(error);
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
            QModelIndex modelIndexTo = createIndex(row, Columns::Dirty);
            Q_UNUSED(localUidIndex.replace(it, item))
            emit dataChanged(modelIndexFrom, modelIndexTo);
        }
    }
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
        previewText.truncate(140);
        item.setPreviewText(previewText);
    }

    item.setThumbnail(note.thumbnail());

    if (note.hasTagLocalUids()) {
        QStringList tagLocalUids;
        note.tagLocalUids(tagLocalUids);
        item.setTagLocalUids(tagLocalUids);
    }

    if (note.hasTagGuids()) {
        QStringList tagGuids;
        note.tagGuids(tagGuids);
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

    qint64 sizeInBytes = 0;
    if (note.hasContent()) {
        sizeInBytes += note.content().size();
    }

    if (note.hasResources())
    {
        QList<ResourceAdapter> resourceAdapters = note.resourceAdapters();
        for(auto it = resourceAdapters.begin(), end = resourceAdapters.end(); it != end; ++it)
        {
            const ResourceAdapter & resourceAdapter = *it;

            if (resourceAdapter.hasDataBody()) {
                sizeInBytes += resourceAdapter.dataBody().size();
            }

            if (resourceAdapter.hasRecognitionDataBody()) {
                sizeInBytes += resourceAdapter.recognitionDataBody().size();
            }

            if (resourceAdapter.hasAlternateDataBody()) {
                sizeInBytes += resourceAdapter.alternateDataBody().size();
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
    case Columns::ThumbnailImageFilePath:
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

} // namespace qute_note
