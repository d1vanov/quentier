#include "FavoritesModel.h"
#include <qute_note/logging/QuteNoteLogger.h>

#define NUM_FAVORITES_MODEL_COLUMNS (3)

namespace qute_note {

FavoritesModel::FavoritesModel(LocalStorageManagerThreadWorker & localStorageManagerThreadWorker,
                               NoteCache & noteCache, NotebookCache & notebookCache, TagCache & tagCache,
                               SavedSearchCache & savedSearchCache, QObject * parent) :
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
    // TODO: do what's necessary here
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

    // TODO: figure out whether the item is editable

    return indexFlags;
}

QVariant FavoritesModel::data(const QModelIndex & index, int role) const
{
    // TODO: implement
    Q_UNUSED(index)
    Q_UNUSED(role)
    return QVariant();
}

QVariant FavoritesModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    // TODO: implement
    Q_UNUSED(section)
    Q_UNUSED(orientation)
    Q_UNUSED(role)
    return QVariant();
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
    // TODO: implement
    Q_UNUSED(row)
    Q_UNUSED(column)
    Q_UNUSED(column)
    return QModelIndex();
}

QModelIndex FavoritesModel::parent(const QModelIndex & index) const
{
    // TODO: implement
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

bool FavoritesModel::setData(const QModelIndex & index, const QVariant & value, int role)
{
    // TODO: implement
    Q_UNUSED(index)
    Q_UNUSED(value)
    Q_UNUSED(role)
    return false;
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
    // TODO: implement
    Q_UNUSED(row)
    Q_UNUSED(count)
    Q_UNUSED(parent)
    return false;
}

void FavoritesModel::sort(int column, Qt::SortOrder order)
{
    // TODO: implement
    Q_UNUSED(column)
    Q_UNUSED(order)
}

void FavoritesModel::onAddNoteComplete(Note note, QUuid requestId)
{
    // TODO: implement
    Q_UNUSED(note)
    Q_UNUSED(requestId)
}

void FavoritesModel::onUpdateNoteComplete(Note note, bool updateResources, bool updateTags, QUuid requestId)
{
    // TODO: implement
    Q_UNUSED(note)
    Q_UNUSED(updateResources)
    Q_UNUSED(updateTags)
    Q_UNUSED(requestId)
}

void FavoritesModel::onUpdateNoteFailed(Note note, bool updateResources, bool updateTags,
                                        QString errorDescription, QUuid requestId)
{
    // TODO: implement
    Q_UNUSED(note)
    Q_UNUSED(updateResources)
    Q_UNUSED(updateTags)
    Q_UNUSED(errorDescription)
    Q_UNUSED(requestId)
}

void FavoritesModel::onFindNoteComplete(Note note, bool withResourceBinaryData, QUuid requestId)
{
    // TODO: implement
    Q_UNUSED(note)
    Q_UNUSED(withResourceBinaryData)
    Q_UNUSED(requestId)
}

void FavoritesModel::onFindNoteFailed(Note note, bool withResourceBinaryData, QString errorDescription, QUuid requestId)
{
    // TODO: implement
    Q_UNUSED(note)
    Q_UNUSED(withResourceBinaryData)
    Q_UNUSED(errorDescription)
    Q_UNUSED(requestId)
}

void FavoritesModel::onListNotesComplete(LocalStorageManager::ListObjectsOptions flag, bool withResourceBinaryData,
                                         size_t limit, size_t offset, LocalStorageManager::ListNotesOrder::type order,
                                         LocalStorageManager::OrderDirection::type orderDirection,
                                         QList<Note> foundNotes, QUuid requestId)
{
    // TODO: implement
    Q_UNUSED(flag)
    Q_UNUSED(withResourceBinaryData)
    Q_UNUSED(limit)
    Q_UNUSED(offset)
    Q_UNUSED(order)
    Q_UNUSED(orderDirection)
    Q_UNUSED(foundNotes)
    Q_UNUSED(requestId)

}

void FavoritesModel::onListNotesFailed(LocalStorageManager::ListObjectsOptions flag, bool withResourceBinaryData,
                                       size_t limit, size_t offset, LocalStorageManager::ListNotesOrder::type order,
                                       LocalStorageManager::OrderDirection::type orderDirection,
                                       QString errorDescription, QUuid requestId)
{
    // TODO: implement
    Q_UNUSED(flag)
    Q_UNUSED(withResourceBinaryData)
    Q_UNUSED(limit)
    Q_UNUSED(offset)
    Q_UNUSED(order)
    Q_UNUSED(orderDirection)
    Q_UNUSED(errorDescription)
    Q_UNUSED(requestId)
}

void FavoritesModel::onExpungeNoteComplete(Note note, QUuid requestId)
{
    // TODO: implement
    Q_UNUSED(note)
    Q_UNUSED(requestId)
}

void FavoritesModel::onAddNotebookComplete(Notebook notebook, QUuid requestId)
{
    // TODO: implement
    Q_UNUSED(notebook)
    Q_UNUSED(requestId)
}

void FavoritesModel::onUpdateNotebookComplete(Notebook notebook, QUuid requestId)
{
    // TODO: implement
    Q_UNUSED(notebook)
    Q_UNUSED(requestId)
}

void FavoritesModel::onUpdateNotebookFailed(Notebook notebook, QString errorDescription, QUuid requestId)
{
    // TODO: implement
    Q_UNUSED(notebook)
    Q_UNUSED(errorDescription)
    Q_UNUSED(requestId)
}

} // namespace qute_note
