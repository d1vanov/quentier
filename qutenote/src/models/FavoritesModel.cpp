#include "FavoritesModel.h"
#include <qute_note/logging/QuteNoteLogger.h>

#define NUM_FAVORITES_MODEL_COLUMNS (3)

namespace qute_note {

FavoritesModel::FavoritesModel(LocalStorageManagerThreadWorker & localStorageManagerThreadWorker,
                               NoteCache & noteCache, NotebookCache & notebookCache, TagCache & tagCache,
                               SavedSearchCache & savedSearchCache, QObject * parent) :
    QAbstractItemModel(parent),
    m_data(),
    m_noteCache(noteCache),
    m_notebookCache(notebookCache),
    m_tagCache(tagCache),
    m_savedSearchCache(savedSearchCache),
    m_listNotesOffset(0),
    m_listNotesRequestId(),
    m_listNotebooksOffset(0),
    m_listNotebooksRequestId(),
    m_listTagsOffset(0),
    m_listTagsRequestId(),
    m_listSavedSearchesOffset(0),
    m_listSavedSearchesRequestId(),
    m_updateNoteRequestIds(),
    m_findNoteToRestoreFailedUpdateRequestIds(),
    m_findNoteToPerformUpdateRequestIds(),
    m_updateNotebookRequestIds(),
    m_findNotebookToRestoreFailedUpdateRequestIds(),
    m_findNotebookToPerformUpdateRequestIds(),
    m_updateTagRequestIds(),
    m_findTagToRestoreFailedUpdateRequestIds(),
    m_findTagToPerformUpdateRequestIds(),
    m_updateSavedSearchRequestIds(),
    m_findSavedSearchToRestoreFailedUpdateRequestIds(),
    m_findSavedSearchToPerformUpdateRequestIds(),
    m_sortedColumn(Columns::DisplayName),
    m_sortOrder(Qt::AscendingOrder)
{
    createConnections(localStorageManagerThreadWorker);
    requestNotebooksList();
    requestTagsList();
    requestNotesList();
    requestSavedSearchesList();
}

FavoritesModel::~FavoritesModel()
{}

QModelIndex FavoritesModel::indexForLocalUid(const QString & localUid) const
{
    const FavoritesDataByLocalUid & localUidIndex = m_data.get<ByLocalUid>();
    auto it = localUidIndex.find(localUid);
    if (Q_UNLIKELY(it == localUidIndex.end())) {
        QNDEBUG("Can't find favorites model item by local uid: " << localUid);
        return QModelIndex();
    }

    const FavoritesDataByIndex & index = m_data.get<ByIndex>();
    auto indexIt = m_data.project<ByIndex>(it);
    if (Q_UNLIKELY(indexIt == index.end())) {
        QNWARNING("Can't find the indexed reference to the favorites model item: " << *it);
        return QModelIndex();
    }

    int rowIndex = static_cast<int>(std::distance(index.begin(), indexIt));
    return createIndex(rowIndex, Columns::DisplayName);
}

const FavoritesModelItem * FavoritesModel::itemForLocalUid(const QString & localUid) const
{
    const FavoritesDataByLocalUid & localUidIndex = m_data.get<ByLocalUid>();
    auto it = localUidIndex.find(localUid);
    if (Q_UNLIKELY(it == localUidIndex.end())) {
        QNDEBUG("Can't find favorites model item by local uid: " << localUid);
        return Q_NULLPTR;
    }

    return &(*it);
}

const FavoritesModelItem * FavoritesModel::itemAtRow(const int row) const
{
    const FavoritesDataByIndex & index = m_data.get<ByIndex>();
    if (Q_UNLIKELY((row < 0) || (index.size() <= static_cast<size_t>(row)))) {
        QNDEBUG("Detected attempt to get the favorites model item for non-existing row");
        return Q_NULLPTR;
    }

    return &(index[static_cast<size_t>(row)]);
}

Qt::ItemFlags FavoritesModel::flags(const QModelIndex & modelIndex) const
{
    Qt::ItemFlags indexFlags = QAbstractItemModel::flags(modelIndex);
    if (!modelIndex.isValid()) {
        return indexFlags;
    }

    int row = modelIndex.row();
    int column = modelIndex.column();

    if ((row < 0) || (row >= static_cast<int>(m_data.size())) ||
        (column < 0) || (column >= NUM_FAVORITES_MODEL_COLUMNS))
    {
        return indexFlags;
    }

    indexFlags |= Qt::ItemIsSelectable;
    indexFlags |= Qt::ItemIsEnabled;

    const FavoritesDataByIndex & index = m_data.get<ByIndex>();
    const FavoritesModelItem & item = index.at(static_cast<size_t>(row));

    switch(item.type())
    {
    case FavoritesModelItem::Type::Notebook:
        if (canUpdateNotebook(item.localUid())) {
            indexFlags |= Qt::ItemIsEditable;
        }
        break;
    case FavoritesModelItem::Type::Note:
        if (canUpdateNote(item.localUid())) {
            indexFlags |= Qt::ItemIsEditable;
        }
        break;
    case FavoritesModelItem::Type::Tag:
        if (canUpdateTag(item.localUid())) {
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
        return QVariant();
    }

    int rowIndex = index.row();
    int columnIndex = index.column();

    if ((rowIndex < 0) || (rowIndex >= static_cast<int>(m_data.size())) ||
        (columnIndex < 0) || (columnIndex >= NUM_FAVORITES_MODEL_COLUMNS))
    {
        return QVariant();
    }

    Columns::type column;
    switch(columnIndex)
    {
    case Columns::Type:
    case Columns::DisplayName:
    case Columns::NumNotesTargeted:
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

QVariant FavoritesModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole) {
        return QVariant();
    }

    if (orientation == Qt::Vertical) {
        return QVariant(section + 1);
    }

    switch(section)
    {
    case Columns::Type:
        // TRANSLATOR: the type of the favorites model item - note, notebook, tag or saved search
        return QVariant(tr("Type"));
    case Columns::DisplayName:
        // TRANSLATOR: the displayed name of the favorites model item - the name of a notebook or tag or saved search of the note's title
        return QVariant(tr("Name"));
    case Columns::NumNotesTargeted:
        // TRANSLATOR: the number of items targeted by the favorites model item, in particular, the number of notes targeted by the notebook or tag
        return QVariant(tr("N items"));
    default:
        return QVariant();
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

    return NUM_FAVORITES_MODEL_COLUMNS;
}

QModelIndex FavoritesModel::index(int row, int column, const QModelIndex & parent) const
{
    if (parent.isValid()) {
        return QModelIndex();
    }

    if ((row < 0) || (row >= static_cast<int>(m_data.size())) ||
        (column < 0) || (column >= NUM_FAVORITES_MODEL_COLUMNS))
    {
        return QModelIndex();
    }

    return createIndex(row, column);
}

QModelIndex FavoritesModel::parent(const QModelIndex & index) const
{
    Q_UNUSED(index)
    return QModelIndex();
}

bool FavoritesModel::setHeaderData(int section, Qt::Orientation orientation, const QVariant & value, int role)
{
    Q_UNUSED(section)
    Q_UNUSED(orientation)
    Q_UNUSED(value)
    Q_UNUSED(role)
    return false;
}

bool FavoritesModel::setData(const QModelIndex & modelIndex, const QVariant & value, int role)
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
        (column < 0) || (column >= NUM_FAVORITES_MODEL_COLUMNS))
    {
        return false;
    }

    FavoritesDataByIndex & index = m_data.get<ByIndex>();
    FavoritesModelItem item = index.at(static_cast<size_t>(row));

    switch(item.type())
    {
    case FavoritesModelItem::Type::Notebook:
        if (!canUpdateNotebook(item.localUid())) {
            return false;
        }
        break;
    case FavoritesModelItem::Type::Note:
        if (!canUpdateNote(item.localUid())) {
            return false;
        }
        break;
    case FavoritesModelItem::Type::Tag:
        if (!canUpdateTag(item.localUid())) {
            return false;
        }
        break;
    default:
        break;
    }

    bool changed = false;

    switch(column)
    {
    case Columns::DisplayName:
        {
            QString newDisplayName = value.toString();
            changed = (item.displayName() != newDisplayName);
            item.setDisplayName(newDisplayName);
            break;
        }
    default:
        return false;
    }

    if (!changed) {
        // Nothing has really changed but ok, return true
        return true;
    }

    index.replace(index.begin() + row, item);
    emit dataChanged(modelIndex, modelIndex);

    emit layoutAboutToBeChanged();
    updateItemRowWithRespectToSorting(item);
    emit layoutChanged();

    updateItemInLocalStorage(item);

    return true;
}

bool FavoritesModel::insertRows(int row, int count, const QModelIndex & parent)
{
    Q_UNUSED(row)
    Q_UNUSED(count)
    Q_UNUSED(parent)
    return false;
}

bool FavoritesModel::removeRows(int row, int count, const QModelIndex & parent)
{
    if (Q_UNLIKELY(parent.isValid())) {
        QNDEBUG("Ignoring the attempt to remove rows from favorites model for valid parent model index");
        return false;
    }

    if (Q_UNLIKELY((row + count) >= static_cast<int>(m_data.size())))
    {
        QString error = QT_TR_NOOP("Detected attempt to remove more rows than the favorites model contains: row") +
                        QStringLiteral(" = ") + QString::number(row) + QStringLiteral(", ") + QT_TR_NOOP("count") +
                        QStringLiteral(" = ") + QString::number(count) + QStringLiteral(", ") + QT_TR_NOOP("number of favorites model items") +
                        QStringLiteral(" = ") + QString::number(static_cast<int>(m_data.size()));
        QNINFO(error);
        emit notifyError(error);
        return false;
    }

    FavoritesDataByIndex & index = m_data.get<ByIndex>();

    beginRemoveRows(QModelIndex(), row, row + count - 1);

    QStringList notebookLocalUidsToRemove;
    QStringList noteLocalUidsToRemove;
    QStringList tagLocalUidsToRemove;
    QStringList savedSearchLocalUidsToRemove;

    for(int i = 0; i < count; ++i)
    {
        auto it = index.begin() + row + i;
        const FavoritesModelItem & item = *it;

        switch(item.type())
        {
        case FavoritesModelItem::Type::Notebook:
            notebookLocalUidsToRemove << item.localUid();
            break;
        case FavoritesModelItem::Type::Note:
            noteLocalUidsToRemove << item.localUid();
            break;
        case FavoritesModelItem::Type::Tag:
            tagLocalUidsToRemove << item.localUid();
            break;
        case FavoritesModelItem::Type::SavedSearch:
            savedSearchLocalUidsToRemove << item.localUid();
            break;
        default:
            {
                QNDEBUG("Favorites model item with unknown type detected: " << item);
                break;
            }
        }
    }

    Q_UNUSED(index.erase(index.begin() + row, index.begin() + row + count))

    for(auto it = notebookLocalUidsToRemove.begin(), end = notebookLocalUidsToRemove.end(); it != end; ++it) {
        unfavoriteNotebook(*it);
    }

    for(auto it = noteLocalUidsToRemove.begin(), end = noteLocalUidsToRemove.end(); it != end; ++it) {
        unfavoriteNote(*it);
    }

    for(auto it = tagLocalUidsToRemove.begin(), end = tagLocalUidsToRemove.end(); it != end; ++it) {
        unfavoriteTag(*it);
    }

    for(auto it = savedSearchLocalUidsToRemove.begin(), end = savedSearchLocalUidsToRemove.end(); it != end; ++it) {
        unfavoriteSavedSearch(*it);
    }

    endRemoveRows();

    return false;
}

void FavoritesModel::sort(int column, Qt::SortOrder order)
{
    QNDEBUG("FavoritesModel::sort: column = " << column << ", order = " << order
            << " (" << (order == Qt::AscendingOrder ? "ascending" : "descending") << ")");

    if (Q_UNLIKELY((column < 0) || (column >= NUM_FAVORITES_MODEL_COLUMNS))) {
        return;
    }

    FavoritesDataByIndex & index = m_data.get<ByIndex>();

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
            (column < 0) || (column >= NUM_FAVORITES_MODEL_COLUMNS))
        {
            localUidsToUpdateWithColumns << std::pair<QString, int>(QString(), column);
        }

        auto itemIt = index.begin() + row;
        const FavoritesModelItem & item = *itemIt;
        localUidsToUpdateWithColumns << std::pair<QString, int>(item.localUid(), column);
    }

    std::vector<boost::reference_wrapper<const FavoritesModelItem> > items(index.begin(), index.end());
    std::sort(items.begin(), items.end(), Comparator(m_sortedColumn, m_sortOrder));
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

void FavoritesModel::onAddNoteComplete(Note note, QUuid requestId)
{
    QNDEBUG("FavoritesModel::onAddNoteComplete: note = " << note << "\nRequest id = " << requestId);
    onNoteAddedOrUpdated(note);
}

void FavoritesModel::onUpdateNoteComplete(Note note, bool updateResources, bool updateTags, QUuid requestId)
{
    QNDEBUG("FavoritesModel::onUpdateNoteComplete: note = " << note << "\nUpdate resources = "
            << (updateResources ? "true" : "false") << ", update tags = "
            << (updateTags ? "true" : "false") << ", request id = " << requestId);

    auto it = m_updateNoteRequestIds.find(requestId);
    if (it != m_updateNoteRequestIds.end()) {
        Q_UNUSED(m_updateNoteRequestIds.erase(it))
        return;
    }

    onNoteAddedOrUpdated(note);
}

void FavoritesModel::onUpdateNoteFailed(Note note, bool updateResources, bool updateTags,
                                        QString errorDescription, QUuid requestId)
{
    auto it = m_updateNoteRequestIds.find(requestId);
    if (it == m_updateNoteRequestIds.end()) {
        return;
    }

    QNDEBUG("FavoritesModel::onUpdateNoteFailed: note = " << note << "\nUpdate resources = "
            << (updateResources ? "true" : "false") << ", update tags = " << (updateTags ? "true" : "false")
            << ", error descrition: " << errorDescription << ", request id = " << requestId);

    Q_UNUSED(m_updateNoteRequestIds.erase(it))

    requestId = QUuid::createUuid();
    Q_UNUSED(m_findNoteToRestoreFailedUpdateRequestIds.insert(requestId))
    QNTRACE("Emitting the request to find a note: local uid = " << note.localUid()
            << ", request id = " << requestId);
    emit findNote(note, /* with resource binary data = */ false, requestId);
}

void FavoritesModel::onFindNoteComplete(Note note, bool withResourceBinaryData, QUuid requestId)
{
    auto restoreUpdateIt = m_findNoteToRestoreFailedUpdateRequestIds.find(requestId);
    auto performUpdateIt = m_findNoteToPerformUpdateRequestIds.find(requestId);
    if ((restoreUpdateIt == m_findNoteToRestoreFailedUpdateRequestIds.end()) &&
        (performUpdateIt == m_findNoteToPerformUpdateRequestIds.end()))
    {
        return;
    }

    QNDEBUG("FavoritesModel::onFindNoteComplete: note = " << note << ", with resource binary data = "
            << (withResourceBinaryData ? "true" : "false") << ", request id = " << requestId);

    if (restoreUpdateIt != m_findNoteToRestoreFailedUpdateRequestIds.end())
    {
        Q_UNUSED(m_findNoteToRestoreFailedUpdateRequestIds.erase(restoreUpdateIt))
        onNoteAddedOrUpdated(note);
    }
    else if (performUpdateIt != m_findNoteToPerformUpdateRequestIds.end())
    {
        Q_UNUSED(m_findNoteToPerformUpdateRequestIds.erase(performUpdateIt))
        m_noteCache.put(note.localUid(), note);

        FavoritesDataByLocalUid & localUidIndex = m_data.get<ByLocalUid>();
        auto it = localUidIndex.find(note.localUid());
        if (it != localUidIndex.end()) {
            updateItemInLocalStorage(*it);
        }
    }
}

void FavoritesModel::onFindNoteFailed(Note note, bool withResourceBinaryData, QString errorDescription, QUuid requestId)
{
    auto restoreUpdateIt = m_findNoteToRestoreFailedUpdateRequestIds.find(requestId);
    auto performUpdateIt = m_findNoteToPerformUpdateRequestIds.find(requestId);
    if ((restoreUpdateIt == m_findNoteToRestoreFailedUpdateRequestIds.end()) &&
        (performUpdateIt == m_findNoteToPerformUpdateRequestIds.end()))
    {
        return;
    }

    QNDEBUG("FavoritesModel::onFindNoteFailed: note = " << note << ", with resource binary data = "
            << (withResourceBinaryData ? "true" : "false") << ", error description: " << errorDescription
            << ", request id = " << requestId);

    if (restoreUpdateIt != m_findNoteToRestoreFailedUpdateRequestIds.end()) {
        Q_UNUSED(m_findNoteToRestoreFailedUpdateRequestIds.erase(restoreUpdateIt))
    }
    else if (performUpdateIt != m_findNoteToPerformUpdateRequestIds.end()) {
        Q_UNUSED(m_findNoteToPerformUpdateRequestIds.erase(performUpdateIt))
    }

    emit notifyError(errorDescription);
}

void FavoritesModel::onListNotesComplete(LocalStorageManager::ListObjectsOptions flag, bool withResourceBinaryData,
                                         size_t limit, size_t offset, LocalStorageManager::ListNotesOrder::type order,
                                         LocalStorageManager::OrderDirection::type orderDirection,
                                         QList<Note> foundNotes, QUuid requestId)
{
    if (requestId != m_listNotesRequestId) {
        return;
    }

    QNDEBUG("FavoritesModel::onListNotesComplete: flag = " << flag << ", with resource binary data = "
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

void FavoritesModel::onListNotesFailed(LocalStorageManager::ListObjectsOptions flag, bool withResourceBinaryData,
                                       size_t limit, size_t offset, LocalStorageManager::ListNotesOrder::type order,
                                       LocalStorageManager::OrderDirection::type orderDirection,
                                       QString errorDescription, QUuid requestId)
{
    if (requestId != m_listNotesRequestId) {
        return;
    }

    QNDEBUG("FavoritesModel::onListNotesFailed: flag = " << flag << ", with resource binary data = "
            << (withResourceBinaryData ? "true" : "false") << ", limit = " << limit
            << ", offset = " << offset << ", order = " << order << ", direction = " << orderDirection
            << ", error description = " << errorDescription << ", request id = " << requestId);

    m_listNotesRequestId = QUuid();

    emit notifyError(errorDescription);
}

void FavoritesModel::onExpungeNoteComplete(Note note, QUuid requestId)
{
    QNDEBUG("FavoritesModel::onExpungeNoteComplete: note = " << note << "\nRequest id = " << requestId);
    removeItemByLocalUid(note.localUid());
}

void FavoritesModel::onAddNotebookComplete(Notebook notebook, QUuid requestId)
{
    QNDEBUG("FavoritesModel::onAddNotebookComplete: notebook = " << notebook << ", request id = " << requestId);
    onNotebookAddedOrUpdated(notebook);
}

void FavoritesModel::onUpdateNotebookComplete(Notebook notebook, QUuid requestId)
{
    QNDEBUG("FavoritesModel::onUpdateNotebookComplete: notebook = " << notebook
            << ", request id = " << requestId);

    auto it = m_updateNotebookRequestIds.find(requestId);
    if (it != m_updateNotebookRequestIds.end()) {
        Q_UNUSED(m_updateNotebookRequestIds.erase(it))
        return;
    }

    onNotebookAddedOrUpdated(notebook);
}

void FavoritesModel::onUpdateNotebookFailed(Notebook notebook, QString errorDescription, QUuid requestId)
{
    auto it = m_updateNotebookRequestIds.find(requestId);
    if (it == m_updateNotebookRequestIds.end()) {
        return;
    }

    QNDEBUG("FavoritesModel::onUpdateNotebookFailed: notebook = " << notebook << "\nError description = "
            << errorDescription << ", request id = " << requestId);

    Q_UNUSED(m_updateNotebookRequestIds.erase(it))

    requestId = QUuid::createUuid();
    Q_UNUSED(m_findNotebookToRestoreFailedUpdateRequestIds.insert(requestId))
    QNTRACE("Emitting the request to find the notebook: local uid = " << notebook.localUid()
            << ", request id = " << requestId);
    emit findNotebook(notebook, requestId);
}

void FavoritesModel::onFindNotebookComplete(Notebook notebook, QUuid requestId)
{
    auto restoreUpdateIt = m_findNotebookToRestoreFailedUpdateRequestIds.find(requestId);
    auto performUpdateIt = m_findNotebookToPerformUpdateRequestIds.find(requestId);
    if ((restoreUpdateIt == m_findNotebookToRestoreFailedUpdateRequestIds.end()) &&
        (performUpdateIt == m_findNotebookToPerformUpdateRequestIds.end()))
    {
        return;
    }

    QNDEBUG("FavoritesModel::onFindNotebookComplete: notebook = " << notebook << "\nRequest id = " << requestId);

    if (restoreUpdateIt != m_findNotebookToRestoreFailedUpdateRequestIds.end())
    {
        Q_UNUSED(m_findNotebookToRestoreFailedUpdateRequestIds.erase(restoreUpdateIt))
        onNotebookAddedOrUpdated(notebook);
    }
    else if (performUpdateIt != m_findNotebookToPerformUpdateRequestIds.end())
    {
        Q_UNUSED(m_findNotebookToPerformUpdateRequestIds.erase(performUpdateIt))
        m_notebookCache.put(notebook.localUid(), notebook);
        FavoritesDataByLocalUid & localUidIndex = m_data.get<ByLocalUid>();
        auto it = localUidIndex.find(notebook.localUid());
        if (it != localUidIndex.end()) {
            updateItemInLocalStorage(*it);
        }
    }
}

void FavoritesModel::onFindNotebookFailed(Notebook notebook, QString errorDescription, QUuid requestId)
{
    auto restoreUpdateIt = m_findNotebookToRestoreFailedUpdateRequestIds.find(requestId);
    auto performUpdateIt = m_findNotebookToPerformUpdateRequestIds.find(requestId);
    if ((restoreUpdateIt == m_findNotebookToRestoreFailedUpdateRequestIds.end()) &&
        (performUpdateIt == m_findNotebookToPerformUpdateRequestIds.end()))
    {
        return;
    }

    QNDEBUG("FavoritesModel::onFindNotebookFailed: notebook = " << notebook << "\nError description = "
            << errorDescription << ", request id = " << requestId);

    if (restoreUpdateIt != m_findNotebookToRestoreFailedUpdateRequestIds.end()) {
        Q_UNUSED(m_findNotebookToRestoreFailedUpdateRequestIds.erase(restoreUpdateIt))
    }
    else if (performUpdateIt != m_findNotebookToPerformUpdateRequestIds.end()) {
        Q_UNUSED(m_findNotebookToPerformUpdateRequestIds.erase(performUpdateIt))
    }

    emit notifyError(errorDescription);
}

void FavoritesModel::onListNotebooksComplete(LocalStorageManager::ListObjectsOptions flag,
                                             size_t limit, size_t offset,
                                             LocalStorageManager::ListNotebooksOrder::type order,
                                             LocalStorageManager::OrderDirection::type orderDirection,
                                             QString linkedNotebookGuid, QList<Notebook> foundNotebooks,
                                             QUuid requestId)
{
    if (requestId != m_listNotebooksRequestId) {
        return;
    }

    QNDEBUG("FavoritesModel::onListNotebooksComplete: flag = " << flag << ", limit = " << limit << ", offset = " << offset
            << ", order = " << order << ", direction = " << orderDirection << ", linked notebook guid = "
            << (linkedNotebookGuid.isNull() ? QString("<null>") : linkedNotebookGuid) << ", num found notebooks = "
            << foundNotebooks.size() << ", request id = " << requestId);

    for(auto it = foundNotebooks.begin(), end = foundNotebooks.end(); it != end; ++it) {
        onNotebookAddedOrUpdated(*it);
    }

    m_listNotebooksRequestId = QUuid();

    if (foundNotebooks.size() == static_cast<int>(limit)) {
        QNTRACE("The number of found notebooks matches the limit, requesting more notebooks from the local storage");
        m_listNotebooksOffset += limit;
        requestNotebooksList();
    }
}

void FavoritesModel::onListNotebooksFailed(LocalStorageManager::ListObjectsOptions flag,
                                           size_t limit, size_t offset,
                                           LocalStorageManager::ListNotebooksOrder::type order,
                                           LocalStorageManager::OrderDirection::type orderDirection,
                                           QString linkedNotebookGuid, QString errorDescription, QUuid requestId)
{
    if (requestId != m_listNotebooksRequestId) {
        return;
    }

    QNDEBUG("FavoritesModel::onListNotebooksFailed: flag = " << flag << ", limit = " << limit << ", offset = "
            << offset << ", order = " << order << ", direction = " << orderDirection << ", linked notebook guid = "
            << (linkedNotebookGuid.isNull() ? QString("<null>") : linkedNotebookGuid) << ", error description = "
            << errorDescription << ", request id = " << requestId);

    m_listNotebooksRequestId = QUuid();

    emit notifyError(errorDescription);
}

void FavoritesModel::onExpungeNotebookComplete(Notebook notebook, QUuid requestId)
{
    QNDEBUG("FavoritesModel::onExpungeNotebookComplete: notebook = " << notebook
            << "\nRequest id = " << requestId);
    removeItemByLocalUid(notebook.localUid());
}

void FavoritesModel::onAddTagComplete(Tag tag, QUuid requestId)
{
    QNDEBUG("FavoritesModel::onAddTagComplete: tag = " << tag << "\nRequest id = " << requestId);
    onTagAddedOrUpdated(tag);
}

void FavoritesModel::onUpdateTagComplete(Tag tag, QUuid requestId)
{
    QNDEBUG("FavoritesModel::onUpdateTagComplete: tag = " << tag << "\nRequest id = " << requestId);

    auto it = m_updateTagRequestIds.find(requestId);
    if (it != m_updateTagRequestIds.end()) {
        Q_UNUSED(m_updateTagRequestIds.erase(it))
        return;
    }

    onTagAddedOrUpdated(tag);
}

void FavoritesModel::onUpdateTagFailed(Tag tag, QString errorDescription, QUuid requestId)
{
    auto it = m_updateTagRequestIds.find(requestId);
    if (it == m_updateTagRequestIds.end()) {
        return;
    }

    QNDEBUG("FavoritesModel::onUpdateTagFailed: tag = " << tag << "\nError description = " << errorDescription
            << ", request id = " << requestId);

    Q_UNUSED(m_updateTagRequestIds.erase(it))

    requestId = QUuid::createUuid();
    Q_UNUSED(m_findTagToRestoreFailedUpdateRequestIds.insert(requestId))
    QNTRACE("Emitting the request to find a tag: local uid = " << tag.localUid()
            << ", request id = " << requestId);
    emit findTag(tag, requestId);
}

void FavoritesModel::onFindTagComplete(Tag tag, QUuid requestId)
{
    auto restoreUpdateIt = m_findTagToRestoreFailedUpdateRequestIds.find(requestId);
    auto performUpdateIt = m_findTagToPerformUpdateRequestIds.find(requestId);
    if ((restoreUpdateIt == m_findTagToRestoreFailedUpdateRequestIds.end()) &&
        (performUpdateIt == m_findTagToPerformUpdateRequestIds.end()))
    {
        return;
    }

    QNDEBUG("FavoritesModel::onFindTagComplete: tag = " << tag << "\nRequest id = " << requestId);

    if (restoreUpdateIt != m_findTagToRestoreFailedUpdateRequestIds.end()) {
        Q_UNUSED(m_findTagToRestoreFailedUpdateRequestIds.erase(restoreUpdateIt))
        onTagAddedOrUpdated(tag);
    }
    else if (performUpdateIt != m_findTagToPerformUpdateRequestIds.end()) {
        Q_UNUSED(m_findTagToPerformUpdateRequestIds.erase(performUpdateIt))
        m_tagCache.put(tag.localUid(), tag);
        FavoritesDataByLocalUid & localUidIndex = m_data.get<ByLocalUid>();
        auto it = localUidIndex.find(tag.localUid());
        if (it != localUidIndex.end()) {
            updateItemInLocalStorage(*it);
        }
    }
}

void FavoritesModel::onFindTagFailed(Tag tag, QString errorDescription, QUuid requestId)
{
    auto restoreUpdateIt = m_findTagToRestoreFailedUpdateRequestIds.find(requestId);
    auto performUpdateIt = m_findTagToPerformUpdateRequestIds.find(requestId);
    if ((restoreUpdateIt == m_findTagToRestoreFailedUpdateRequestIds.end()) &&
        (performUpdateIt == m_findTagToPerformUpdateRequestIds.end()))
    {
        return;
    }

    QNDEBUG("FavoritesModel::onFindTagFailed: tag = " << tag << "\nError description = " << errorDescription
            << ", request id = " << requestId);

    if (restoreUpdateIt != m_findTagToRestoreFailedUpdateRequestIds.end()) {
        Q_UNUSED(m_findTagToRestoreFailedUpdateRequestIds.erase(restoreUpdateIt))
    }
    else if (performUpdateIt != m_findTagToPerformUpdateRequestIds.end()) {
        Q_UNUSED(m_findTagToPerformUpdateRequestIds.erase(performUpdateIt))
    }

    emit notifyError(errorDescription);
}

void FavoritesModel::onListTagsComplete(LocalStorageManager::ListObjectsOptions flag,
                                        size_t limit, size_t offset,
                                        LocalStorageManager::ListTagsOrder::type order,
                                        LocalStorageManager::OrderDirection::type orderDirection,
                                        QString linkedNotebookGuid, QList<Tag> foundTags, QUuid requestId)
{
    if (requestId != m_listTagsRequestId) {
        return;
    }

    QNDEBUG("FavoritesModel::onListTagsComplete: flag = " << flag << ", limit = " << limit
            << ", offset = " << offset << ", order = " << order << ", direction = "
            << orderDirection << ", linked notebook guid = "
            << (linkedNotebookGuid.isNull() ? QString("<null>") : linkedNotebookGuid)
            << ", num found tags = " << foundTags.size() << ", request id = " << requestId);

    for(auto it = foundTags.begin(), end = foundTags.end(); it != end; ++it) {
        onTagAddedOrUpdated(*it);
    }

    m_listTagsRequestId = QUuid();

    if (foundTags.size() == static_cast<int>(limit)) {
        QNTRACE("The number of found tags matches the limit, requesting more tags from the local storage");
        m_listTagsOffset += limit;
        requestTagsList();
    }
}

void FavoritesModel::onListTagsFailed(LocalStorageManager::ListObjectsOptions flag,
                                      size_t limit, size_t offset,
                                      LocalStorageManager::ListTagsOrder::type order,
                                      LocalStorageManager::OrderDirection::type orderDirection,
                                      QString linkedNotebookGuid, QString errorDescription, QUuid requestId)
{
    if (requestId != m_listTagsRequestId) {
        return;
    }

    QNDEBUG("FavoritesModel::onListTagsFailed: flag = " << flag << ", limit = " << limit
            << ", offset = " << offset << ", order = " << order << ", direction = "
            << orderDirection << ", linked notebook guid = "
            << (linkedNotebookGuid.isNull() ? QString("<null>") : linkedNotebookGuid)
            << ", error description = " << errorDescription << ", request id = " << requestId);

    m_listTagsRequestId = QUuid();

    emit notifyError(errorDescription);
}

void FavoritesModel::onExpungeTagComplete(Tag tag, QUuid requestId)
{
    QNDEBUG("FavoritesModel::onExpungeTagComplete: tag = " << tag << "\nRequest id = " << requestId);
    removeItemByLocalUid(tag.localUid());
}

void FavoritesModel::onAddSavedSearchComplete(SavedSearch search, QUuid requestId)
{
    // TODO: implement
    Q_UNUSED(search)
    Q_UNUSED(requestId)
}

void FavoritesModel::onUpdateSavedSearchComplete(SavedSearch search, QUuid requestId)
{
    // TODO: implement
    Q_UNUSED(search)
    Q_UNUSED(requestId)
}

void FavoritesModel::onUpdateSavedSearchFailed(SavedSearch search, QString errorDescription, QUuid requestId)
{
    // TODO: implement
    Q_UNUSED(search)
    Q_UNUSED(errorDescription)
    Q_UNUSED(requestId)
}

void FavoritesModel::onFindSavedSearchComplete(SavedSearch search, QUuid requestId)
{
    // TODO: implement
    Q_UNUSED(search)
    Q_UNUSED(requestId)
}

void FavoritesModel::onFindSavedSearchFailed(SavedSearch search, QString errorDescription, QUuid requestId)
{
    // TODO: implement
    Q_UNUSED(search)
    Q_UNUSED(errorDescription)
    Q_UNUSED(requestId)
}

void FavoritesModel::onListSavedSearchesComplete(LocalStorageManager::ListObjectsOptions flag,
                                                 size_t limit, size_t offset,
                                                 LocalStorageManager::ListSavedSearchesOrder::type order,
                                                 LocalStorageManager::OrderDirection::type orderDirection,
                                                 QList<SavedSearch> foundSearches, QUuid requestId)
{
    // TODO: implement
    Q_UNUSED(flag)
    Q_UNUSED(limit)
    Q_UNUSED(offset)
    Q_UNUSED(order)
    Q_UNUSED(orderDirection)
    Q_UNUSED(foundSearches)
    Q_UNUSED(requestId)
}

void FavoritesModel::onListSavedSearchesFailed(LocalStorageManager::ListObjectsOptions flag,
                                               size_t limit, size_t offset,
                                               LocalStorageManager::ListSavedSearchesOrder::type order,
                                               LocalStorageManager::OrderDirection::type orderDirection,
                                               QString errorDescription, QUuid requestId)
{
    // TODO: implement
    Q_UNUSED(flag)
    Q_UNUSED(limit)
    Q_UNUSED(offset)
    Q_UNUSED(order)
    Q_UNUSED(orderDirection)
    Q_UNUSED(errorDescription)
    Q_UNUSED(requestId)
}

void FavoritesModel::onExpungeSavedSearchComplete(SavedSearch search, QUuid requestId)
{
    // TODO: implement
    Q_UNUSED(search)
    Q_UNUSED(requestId)
}

void FavoritesModel::createConnections(LocalStorageManagerThreadWorker & localStorageManagerThreadWorker)
{
    // TODO: implement
    Q_UNUSED(localStorageManagerThreadWorker)
}

void FavoritesModel::requestNotesList()
{
    // TODO: implement
}

void FavoritesModel::requestNotebooksList()
{
    // TODO: implement
}

void FavoritesModel::requestTagsList()
{
    // TODO: implement
}

void FavoritesModel::requestSavedSearchesList()
{
    // TODO: implement
}

QVariant FavoritesModel::dataImpl(const int row, const Columns::type column) const
{
    // TODO: implement
    Q_UNUSED(row)
    Q_UNUSED(column)
    return QVariant();
}

QVariant FavoritesModel::dataAccessibleText(const int row, const Columns::type column) const
{
    // TODO: implement
    Q_UNUSED(row)
    Q_UNUSED(column)
    return QVariant();
}

void FavoritesModel::removeItemByLocalUid(const QString & localUid)
{
    // TODO: implement
    Q_UNUSED(localUid)
}

void FavoritesModel::updateItemRowWithRespectToSorting(const FavoritesModelItem & item)
{
    // TODO: implement
    Q_UNUSED(item)
}

void FavoritesModel::updateItemInLocalStorage(const FavoritesModelItem & item)
{
    // TODO: implement
    Q_UNUSED(item)
}

bool FavoritesModel::canUpdateNote(const QString & localUid) const
{
    // TODO: implement
    Q_UNUSED(localUid)
    return false;
}

bool FavoritesModel::canUpdateNotebook(const QString & localUid) const
{
    // TODO: implement
    Q_UNUSED(localUid)
    return false;
}

bool FavoritesModel::canUpdateTag(const QString & localUid) const
{
    // TODO: implement
    Q_UNUSED(localUid)
    return false;
}

void FavoritesModel::unfavoriteNote(const QString & localUid)
{
    // TODO: implement
    Q_UNUSED(localUid)
}

void FavoritesModel::unfavoriteNotebook(const QString & localUid)
{
    // TODO: implement
    Q_UNUSED(localUid)
}

void FavoritesModel::unfavoriteTag(const QString & localUid)
{
    // TODO: implement
    Q_UNUSED(localUid)
}

void FavoritesModel::unfavoriteSavedSearch(const QString & localUid)
{
    // TODO: implement
    Q_UNUSED(localUid)
}

void FavoritesModel::onNoteAddedOrUpdated(const Note & note)
{
    // TODO: implement
    Q_UNUSED(note)
}

void FavoritesModel::onNotebookAddedOrUpdated(const Notebook & notebook)
{
    // TODO: implement
    Q_UNUSED(notebook)
}

void FavoritesModel::onTagAddedOrUpdated(const Tag & tag)
{
    // TODO: implement
    Q_UNUSED(tag)
}

void FavoritesModel::onSavedSearchAddedOrUpdated(const SavedSearch & search)
{
    // TODO: implement
    Q_UNUSED(search)
}

bool FavoritesModel::Comparator::operator()(const FavoritesModelItem & lhs, const FavoritesModelItem & rhs) const
{
    bool less = false;
    bool greater = false;

    switch(m_sortedColumn)
    {
    case Columns::DisplayName:
        {
            int compareResult = lhs.displayName().localeAwareCompare(rhs.displayName());
            less = compareResult < 0;
            greater = compareResult > 0;
            break;
        }
    case Columns::Type:
        {
            less = lhs.type() < rhs.type();
            greater = lhs.type() > rhs.type();
            break;
        }
    case Columns::NumNotesTargeted:
        {
            less = lhs.numNotesTargeted() < rhs.numNotesTargeted();
            greater = lhs.numNotesTargeted() > rhs.numNotesTargeted();
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

} // namespace qute_note
