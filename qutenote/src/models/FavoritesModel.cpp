#include "FavoritesModel.h"
#include <qute_note/logging/QuteNoteLogger.h>

// Limit for the queries to the local storage
#define NOTE_LIST_LIMIT (40)
#define NOTEBOOK_LIST_LIMIT (40)
#define TAG_LIST_LIMIT (40)
#define SAVED_SEARCH_LIST_LIMIT (40)

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
    m_lowerCaseNotebookNames(),
    m_lowerCaseTagNames(),
    m_lowerCaseSavedSearchNames(),
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
    m_findNoteToUnfavoriteRequestIds(),
    m_updateNotebookRequestIds(),
    m_findNotebookToRestoreFailedUpdateRequestIds(),
    m_findNotebookToPerformUpdateRequestIds(),
    m_findNotebookToUnfavoriteRequestIds(),
    m_updateTagRequestIds(),
    m_findTagToRestoreFailedUpdateRequestIds(),
    m_findTagToPerformUpdateRequestIds(),
    m_findTagToUnfavoriteRequestIds(),
    m_updateSavedSearchRequestIds(),
    m_findSavedSearchToRestoreFailedUpdateRequestIds(),
    m_findSavedSearchToPerformUpdateRequestIds(),
    m_findSavedSearchToUnfavoriteRequestIds(),
    m_tagLocalUidToLinkedNotebookGuid(),
    m_notebookLocalUidToGuid(),
    m_noteLocalUidToNotebookLocalUid(),
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

    switch(column)
    {
    case Columns::DisplayName:
        {
            QString newDisplayName = value.toString().trimmed();
            bool changed = (item.displayName() != newDisplayName);
            if (!changed) {
                return true;
            }

            switch(item.type())
            {
            case FavoritesModelItem::Type::Notebook:
                {
                    auto it = m_lowerCaseNotebookNames.find(newDisplayName.toLower());
                    if (it != m_lowerCaseNotebookNames.end()) {
                        QString error = QT_TR_NOOP("Can't rename notebook: no two notebooks within the account are allowed "
                                                   "to have the same name in the case-insensitive manner");
                        QNINFO(error << ", suggested new name = " << newDisplayName);
                        emit notifyError(error);
                        return false;
                    }

                    QString error;
                    if (!Notebook::validateName(newDisplayName, &error)) {
                        error = QT_TR_NOOP("Can't rename notebook") + QStringLiteral(": ") + error;
                        QNINFO(error << ", suggested new name = " << newDisplayName);
                        emit notifyError(error);
                        return false;
                    }

                    break;
                }
            case FavoritesModelItem::Type::Tag:
                {
                    auto it = m_lowerCaseTagNames.find(newDisplayName.toLower());
                    if (it != m_lowerCaseTagNames.end()) {
                        QString error = QT_TR_NOOP("Can't rename tag: no two tags within the account are allowed "
                                                   "to have the same name in the case-insensitive manner");
                        QNINFO(error);
                        emit notifyError(error);
                        return false;
                    }

                    QString error;
                    if (!Tag::validateName(newDisplayName, &error)) {
                        error = QT_TR_NOOP("Can't rename tag") + QStringLiteral(": ") + error;
                        QNINFO(error << ", suggested new name = " << newDisplayName);
                        emit notifyError(error);
                        return false;
                    }

                    break;
                }
            case FavoritesModelItem::Type::SavedSearch:
                {
                    auto it = m_lowerCaseSavedSearchNames.find(newDisplayName.toLower());
                    if (it != m_lowerCaseSavedSearchNames.end()) {
                        QString error = QT_TR_NOOP("Can't rename saved search: no two saved searches within the account are allowed to have the same name in the case-insensitive manner");
                        QNINFO(error);
                        emit notifyError(error);
                        return false;
                    }

                    QString error;
                    if (!SavedSearch::validateName(newDisplayName, &error)) {
                        error = QT_TR_NOOP("Can't rename saved search") + QStringLiteral(": ") + error;
                        QNINFO(error << ", suggested new name = " << newDisplayName);
                        emit notifyError(error);
                        return false;
                    }

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
    auto unfavoriteIt = m_findNoteToUnfavoriteRequestIds.find(requestId);

    if ((restoreUpdateIt == m_findNoteToRestoreFailedUpdateRequestIds.end()) &&
        (performUpdateIt == m_findNoteToPerformUpdateRequestIds.end()) &&
        (unfavoriteIt == m_findNoteToUnfavoriteRequestIds.end()))
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
    else if (unfavoriteIt != m_findNoteToUnfavoriteRequestIds.end())
    {
        Q_UNUSED(m_findNoteToUnfavoriteRequestIds.erase(unfavoriteIt))
        m_noteCache.put(note.localUid(), note);
        unfavoriteNote(note.localUid());
    }
}

void FavoritesModel::onFindNoteFailed(Note note, bool withResourceBinaryData, QString errorDescription, QUuid requestId)
{
    auto restoreUpdateIt = m_findNoteToRestoreFailedUpdateRequestIds.find(requestId);
    auto performUpdateIt = m_findNoteToPerformUpdateRequestIds.find(requestId);
    auto unfavoriteIt = m_findNoteToUnfavoriteRequestIds.find(requestId);

    if ((restoreUpdateIt == m_findNoteToRestoreFailedUpdateRequestIds.end()) &&
        (performUpdateIt == m_findNoteToPerformUpdateRequestIds.end()) &&
        (unfavoriteIt == m_findNoteToUnfavoriteRequestIds.end()))
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
    else if (unfavoriteIt != m_findNoteToUnfavoriteRequestIds.end()) {
        Q_UNUSED(m_findNoteToUnfavoriteRequestIds.erase(unfavoriteIt))
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
    auto unfavoriteIt = m_findNotebookToUnfavoriteRequestIds.find(requestId);

    if ((restoreUpdateIt == m_findNotebookToRestoreFailedUpdateRequestIds.end()) &&
        (performUpdateIt == m_findNotebookToPerformUpdateRequestIds.end()) &&
        (unfavoriteIt == m_findNotebookToUnfavoriteRequestIds.end()))
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
    else if (unfavoriteIt != m_findNotebookToUnfavoriteRequestIds.end())
    {
        Q_UNUSED(m_findNotebookToPerformUpdateRequestIds.erase(performUpdateIt))
        m_notebookCache.put(notebook.localUid(), notebook);
        unfavoriteNotebook(notebook.localUid());
    }
}

void FavoritesModel::onFindNotebookFailed(Notebook notebook, QString errorDescription, QUuid requestId)
{
    auto restoreUpdateIt = m_findNotebookToRestoreFailedUpdateRequestIds.find(requestId);
    auto performUpdateIt = m_findNotebookToPerformUpdateRequestIds.find(requestId);
    auto unfavoriteIt = m_findNotebookToUnfavoriteRequestIds.find(requestId);

    if ((restoreUpdateIt == m_findNotebookToRestoreFailedUpdateRequestIds.end()) &&
        (performUpdateIt == m_findNotebookToPerformUpdateRequestIds.end()) &&
        (unfavoriteIt == m_findNotebookToUnfavoriteRequestIds.end()))
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
    else if (unfavoriteIt != m_findNotebookToUnfavoriteRequestIds.end()) {
        Q_UNUSED(m_findNotebookToUnfavoriteRequestIds.erase(unfavoriteIt))
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
    auto unfavoriteIt = m_findTagToUnfavoriteRequestIds.find(requestId);

    if ((restoreUpdateIt == m_findTagToRestoreFailedUpdateRequestIds.end()) &&
        (performUpdateIt == m_findTagToPerformUpdateRequestIds.end()) &&
        (unfavoriteIt == m_findTagToUnfavoriteRequestIds.end()))
    {
        return;
    }

    QNDEBUG("FavoritesModel::onFindTagComplete: tag = " << tag << "\nRequest id = " << requestId);

    if (restoreUpdateIt != m_findTagToRestoreFailedUpdateRequestIds.end())
    {
        Q_UNUSED(m_findTagToRestoreFailedUpdateRequestIds.erase(restoreUpdateIt))
        onTagAddedOrUpdated(tag);
    }
    else if (performUpdateIt != m_findTagToPerformUpdateRequestIds.end())
    {
        Q_UNUSED(m_findTagToPerformUpdateRequestIds.erase(performUpdateIt))
        m_tagCache.put(tag.localUid(), tag);
        FavoritesDataByLocalUid & localUidIndex = m_data.get<ByLocalUid>();
        auto it = localUidIndex.find(tag.localUid());
        if (it != localUidIndex.end()) {
            updateItemInLocalStorage(*it);
        }
    }
    else if (unfavoriteIt != m_findTagToUnfavoriteRequestIds.end())
    {
        Q_UNUSED(m_findTagToPerformUpdateRequestIds.erase(performUpdateIt))
        m_tagCache.put(tag.localUid(), tag);
        unfavoriteTag(tag.localUid());
    }
}

void FavoritesModel::onFindTagFailed(Tag tag, QString errorDescription, QUuid requestId)
{

    auto restoreUpdateIt = m_findTagToRestoreFailedUpdateRequestIds.find(requestId);
    auto performUpdateIt = m_findTagToPerformUpdateRequestIds.find(requestId);
    auto unfavoriteIt = m_findTagToUnfavoriteRequestIds.find(requestId);

    if ((restoreUpdateIt == m_findTagToRestoreFailedUpdateRequestIds.end()) &&
        (performUpdateIt == m_findTagToPerformUpdateRequestIds.end()) &&
        (unfavoriteIt == m_findTagToUnfavoriteRequestIds.end()))
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
    else if (unfavoriteIt != m_findTagToUnfavoriteRequestIds.end()) {
        Q_UNUSED(m_findTagToUnfavoriteRequestIds.erase(unfavoriteIt))
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
    QNDEBUG("FavoritesModel::onAddSavedSearchComplete: " << search << "\nRequest id = " << requestId);
    onSavedSearchAddedOrUpdated(search);
}

void FavoritesModel::onUpdateSavedSearchComplete(SavedSearch search, QUuid requestId)
{
    QNDEBUG("FavoritesModel::onUpdateSavedSearchComplete: " << search << "\nRequest id = " << requestId);

    auto it = m_updateSavedSearchRequestIds.find(requestId);
    if (it != m_updateSavedSearchRequestIds.end()) {
        Q_UNUSED(m_updateSavedSearchRequestIds.erase(it))
        return;
    }

    onSavedSearchAddedOrUpdated(search);
}

void FavoritesModel::onUpdateSavedSearchFailed(SavedSearch search, QString errorDescription, QUuid requestId)
{
    auto it = m_updateSavedSearchRequestIds.find(requestId);
    if (it == m_updateSavedSearchRequestIds.end()) {
        return;
    }

    QNDEBUG("FavoritesModel::onUpdateSavedSearchFailed: search = " << search << "\nError description = "
            << errorDescription << ", request id = " << requestId);

    Q_UNUSED(m_updateSavedSearchRequestIds.erase(it))

    requestId = QUuid::createUuid();
    Q_UNUSED(m_findSavedSearchToRestoreFailedUpdateRequestIds.insert(requestId))
    QNTRACE("Emitting the request to find the saved search: local uid = " << search.localUid()
            << ", request id = " << requestId);
    emit findSavedSearch(search, requestId);
}

void FavoritesModel::onFindSavedSearchComplete(SavedSearch search, QUuid requestId)
{
    auto restoreUpdateIt = m_findSavedSearchToRestoreFailedUpdateRequestIds.find(requestId);
    auto performUpdateIt = m_findSavedSearchToPerformUpdateRequestIds.find(requestId);
    auto unfavoriteIt = m_findSavedSearchToUnfavoriteRequestIds.find(requestId);

    if ( (restoreUpdateIt == m_findSavedSearchToRestoreFailedUpdateRequestIds.end()) &&
         (performUpdateIt == m_findSavedSearchToPerformUpdateRequestIds.end()) &&
         (unfavoriteIt == m_findSavedSearchToUnfavoriteRequestIds.end()) )
    {
        return;
    }

    QNDEBUG("FavoritesModel::onFindSavedSearchComplete: search = " << search << "\nRequest id = " << requestId);

    if (restoreUpdateIt != m_findSavedSearchToRestoreFailedUpdateRequestIds.end())
    {
        Q_UNUSED(m_findSavedSearchToRestoreFailedUpdateRequestIds.erase(restoreUpdateIt))
        onSavedSearchAddedOrUpdated(search);
    }
    else if (performUpdateIt != m_findSavedSearchToPerformUpdateRequestIds.end())
    {
        Q_UNUSED(m_findSavedSearchToPerformUpdateRequestIds.erase(performUpdateIt))
        m_savedSearchCache.put(search.localUid(), search);
        FavoritesDataByLocalUid & localUidIndex = m_data.get<ByLocalUid>();
        auto it = localUidIndex.find(search.localUid());
        if (it != localUidIndex.end()) {
            updateItemInLocalStorage(*it);
        }
    }
    else if (unfavoriteIt != m_findSavedSearchToUnfavoriteRequestIds.end())
    {
        Q_UNUSED(m_findSavedSearchToPerformUpdateRequestIds.erase(performUpdateIt))
        m_savedSearchCache.put(search.localUid(), search);
        unfavoriteSavedSearch(search.localUid());
    }
}

void FavoritesModel::onFindSavedSearchFailed(SavedSearch search, QString errorDescription, QUuid requestId)
{
    auto restoreUpdateIt = m_findSavedSearchToRestoreFailedUpdateRequestIds.find(requestId);
    auto performUpdateIt = m_findSavedSearchToPerformUpdateRequestIds.find(requestId);
    auto unfavoriteIt = m_findSavedSearchToUnfavoriteRequestIds.find(requestId);

    if ( (restoreUpdateIt == m_findSavedSearchToRestoreFailedUpdateRequestIds.end()) &&
         (performUpdateIt == m_findSavedSearchToPerformUpdateRequestIds.end()) &&
         (unfavoriteIt == m_findSavedSearchToUnfavoriteRequestIds.end()) )
    {
        return;
    }

    QNWARNING("FavoritesModel::onFindSavedSearchFailed: search = " << search << "\nError description = "
              << errorDescription << ", request id = " << requestId);

    if (restoreUpdateIt != m_findSavedSearchToRestoreFailedUpdateRequestIds.end()) {
        Q_UNUSED(m_findSavedSearchToRestoreFailedUpdateRequestIds.erase(restoreUpdateIt))
    }
    else if (performUpdateIt != m_findSavedSearchToPerformUpdateRequestIds.end()) {
        Q_UNUSED(m_findSavedSearchToPerformUpdateRequestIds.erase(performUpdateIt))
    }
    else if (unfavoriteIt != m_findSavedSearchToUnfavoriteRequestIds.end()) {
        Q_UNUSED(m_findSavedSearchToUnfavoriteRequestIds.erase(unfavoriteIt))
    }

    emit notifyError(errorDescription);
}

void FavoritesModel::onListSavedSearchesComplete(LocalStorageManager::ListObjectsOptions flag,
                                                 size_t limit, size_t offset,
                                                 LocalStorageManager::ListSavedSearchesOrder::type order,
                                                 LocalStorageManager::OrderDirection::type orderDirection,
                                                 QList<SavedSearch> foundSearches, QUuid requestId)
{
    if (requestId != m_listSavedSearchesRequestId) {
        return;
    }

    QNDEBUG("FavoritesModel::onListSavedSearchesComplete: flag = " << flag << ", limit = " << limit
            << ", offset = " << offset << ", order = " << order << ", direction = " << orderDirection
            << ", num found searches = " << foundSearches.size() << ", request id = " << requestId);

    if (foundSearches.isEmpty()) {
        return;
    }

    for(auto it = foundSearches.begin(), end = foundSearches.end(); it != end; ++it) {
        onSavedSearchAddedOrUpdated(*it);
    }

    m_listSavedSearchesRequestId = QUuid();
    if (foundSearches.size() == static_cast<int>(limit)) {
        QNTRACE("The number of found saved searches matches the limit, requesting more saved searches from the local storage");
        m_listSavedSearchesOffset += limit;
        requestSavedSearchesList();
    }
}

void FavoritesModel::onListSavedSearchesFailed(LocalStorageManager::ListObjectsOptions flag,
                                               size_t limit, size_t offset,
                                               LocalStorageManager::ListSavedSearchesOrder::type order,
                                               LocalStorageManager::OrderDirection::type orderDirection,
                                               QString errorDescription, QUuid requestId)
{
    if (requestId != m_listSavedSearchesRequestId) {
        return;
    }

    QNDEBUG("FavoritesModel::onListSavedSearchesFailed: flag = " << flag << ", limit = " << limit
            << ", offset = " << offset << ", order = " << order << ", direction = " << orderDirection
            << ", error: " << errorDescription << ", request id = " << requestId);

    m_listSavedSearchesRequestId = QUuid();

    emit notifyError(errorDescription);
}

void FavoritesModel::onExpungeSavedSearchComplete(SavedSearch search, QUuid requestId)
{
    QNDEBUG("FavoritesModel::onExpungeSavedSearchComplete: search = " << search << "\nRequest id = " << requestId);
    removeItemByLocalUid(search.localUid());
}

void FavoritesModel::onNoteCountPerNotebookComplete(int noteCount, Notebook notebook, QUuid requestId)
{
    // TODO: implement
    Q_UNUSED(noteCount)
    Q_UNUSED(notebook)
    Q_UNUSED(requestId)
}

void FavoritesModel::onNoteCountPerNotebookFailed(QString errorDescription, Notebook notebook, QUuid requestId)
{
    // TODO: implement
    Q_UNUSED(errorDescription)
    Q_UNUSED(notebook)
    Q_UNUSED(requestId)
}

void FavoritesModel::onNoteCountPerTagComplete(int noteCount, Tag tag, QUuid requestId)
{
    // TODO: implement
    Q_UNUSED(noteCount)
    Q_UNUSED(tag)
    Q_UNUSED(requestId)
}

void FavoritesModel::onNoteCountPerTagFailed(QString errorDescription, Tag tag, QUuid requestId)
{
    // TODO: implement
    Q_UNUSED(errorDescription)
    Q_UNUSED(tag)
    Q_UNUSED(requestId)
}

void FavoritesModel::createConnections(LocalStorageManagerThreadWorker & localStorageManagerThreadWorker)
{
    QNDEBUG("FavoritesModel::createConnections");

    // Local signals to localStorageManagerThreadWorker's slots
    QObject::connect(this, QNSIGNAL(FavoritesModel,updateNote,Note,bool,bool,QUuid),
                     &localStorageManagerThreadWorker, QNSLOT(LocalStorageManagerThreadWorker,onUpdateNoteRequest,Note,bool,bool,QUuid));
    QObject::connect(this, QNSIGNAL(FavoritesModel,findNote,Note,bool,QUuid),
                     &localStorageManagerThreadWorker, QNSLOT(LocalStorageManagerThreadWorker,onFindNoteRequest,Note,bool,QUuid));
    QObject::connect(this, QNSIGNAL(FavoritesModel,listNotes,LocalStorageManager::ListObjectsOptions,bool,size_t,size_t,
                                    LocalStorageManager::ListNotesOrder::type,LocalStorageManager::OrderDirection::type,QUuid),
                     &localStorageManagerThreadWorker, QNSLOT(LocalStorageManagerThreadWorker,onListNotesRequest,
                                                              LocalStorageManager::ListObjectsOptions,bool,size_t,size_t,
                                                              LocalStorageManager::ListNotesOrder::type,
                                                              LocalStorageManager::OrderDirection::type,QUuid));
    QObject::connect(this, QNSIGNAL(FavoritesModel,updateNotebook,Notebook,QUuid),
                     &localStorageManagerThreadWorker, QNSLOT(LocalStorageManagerThreadWorker,onUpdateNotebookRequest,Notebook,QUuid));
    QObject::connect(this, QNSIGNAL(FavoritesModel,findNotebook,Notebook,QUuid),
                     &localStorageManagerThreadWorker, QNSLOT(LocalStorageManagerThreadWorker,onFindNotebookRequest,Notebook,QUuid));
    QObject::connect(this, QNSIGNAL(FavoritesModel,listNotebooks,LocalStorageManager::ListObjectsOptions,
                                    size_t,size_t,LocalStorageManager::ListNotebooksOrder::type,
                                    LocalStorageManager::OrderDirection::type,QString,QUuid),
                     &localStorageManagerThreadWorker, QNSLOT(LocalStorageManagerThreadWorker,onListNotebooksRequest,
                                                              LocalStorageManager::ListObjectsOptions,
                                                              size_t,size_t,LocalStorageManager::ListNotebooksOrder::type,
                                                              LocalStorageManager::OrderDirection::type,QString,QUuid));
    QObject::connect(this, QNSIGNAL(FavoritesModel,updateTag,Tag,QUuid),
                     &localStorageManagerThreadWorker, QNSLOT(LocalStorageManagerThreadWorker,onUpdateTagRequest,Tag,QUuid));
    QObject::connect(this, QNSIGNAL(FavoritesModel,findTag,Tag,QUuid),
                     &localStorageManagerThreadWorker, QNSLOT(LocalStorageManagerThreadWorker,onFindTagRequest,Tag,QUuid));
    QObject::connect(this, QNSIGNAL(FavoritesModel,listTags,LocalStorageManager::ListObjectsOptions,size_t,size_t,
                                    LocalStorageManager::ListTagsOrder::type,LocalStorageManager::OrderDirection::type,QString,QUuid),
                     &localStorageManagerThreadWorker, QNSLOT(LocalStorageManagerThreadWorker,onListTagsRequest,LocalStorageManager::ListObjectsOptions,
                                                              size_t,size_t,LocalStorageManager::ListTagsOrder::type,
                                                              LocalStorageManager::OrderDirection::type,QString,QUuid));
    QObject::connect(this, QNSIGNAL(FavoritesModel,updateSavedSearch,SavedSearch,QUuid),
                     &localStorageManagerThreadWorker, QNSLOT(LocalStorageManagerThreadWorker,onUpdateSavedSearchRequest,SavedSearch,QUuid));
    QObject::connect(this, QNSIGNAL(FavoritesModel,findSavedSearch,SavedSearch,QUuid),
                     &localStorageManagerThreadWorker, QNSLOT(LocalStorageManagerThreadWorker,onFindSavedSearchRequest,SavedSearch,QUuid));
    QObject::connect(this, QNSIGNAL(FavoritesModel,listSavedSearches,LocalStorageManager::ListObjectsOptions,
                                    size_t,size_t,LocalStorageManager::ListSavedSearchesOrder::type,
                                    LocalStorageManager::OrderDirection::type,QUuid),
                     &localStorageManagerThreadWorker, QNSLOT(LocalStorageManagerThreadWorker,onListSavedSearchesRequest,
                                                              LocalStorageManager::ListObjectsOptions,
                                                              size_t,size_t,LocalStorageManager::ListSavedSearchesOrder::type,
                                                              LocalStorageManager::OrderDirection::type,QUuid));
    QObject::connect(this, QNSIGNAL(FavoritesModel,noteCountPerNotebook,Notebook,QUuid),
                     &localStorageManagerThreadWorker, QNSLOT(LocalStorageManagerThreadWorker,onNoteCountPerNotebookRequest,Notebook,QUuid));
    QObject::connect(this, QNSIGNAL(FavoritesModel,noteCountPerTag,Tag,QUuid),
                     &localStorageManagerThreadWorker, QNSLOT(LocalStorageManagerThreadWorker,onNoteCountPerTagRequest,Tag,QUuid));

    // localStorageManagerThreadWorker's signals to local slots
    QObject::connect(&localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,addNoteComplete,Note,QUuid),
                     this, QNSLOT(FavoritesModel,onAddNoteComplete,Note,QUuid));
    QObject::connect(&localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,updateNoteComplete,Note,bool,bool,QUuid),
                     this, QNSLOT(FavoritesModel,onUpdateNoteComplete,Note,bool,bool,QUuid));
    QObject::connect(&localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,updateNoteFailed,Note,bool,bool,QString,QUuid),
                     this, QNSLOT(FavoritesModel,onUpdateNoteFailed,Note,bool,bool,QString,QUuid));
    QObject::connect(&localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,findNoteComplete,Note,bool,QUuid),
                     this, QNSLOT(FavoritesModel,onFindNoteComplete,Note,bool,QUuid));
    QObject::connect(&localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,findNoteFailed,Note,bool,QString,QUuid),
                     this, QNSLOT(FavoritesModel,onFindNoteFailed,Note,bool,QString,QUuid));
    QObject::connect(&localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,listNotesComplete,LocalStorageManager::ListObjectsOptions,bool,size_t,size_t,
                                                                LocalStorageManager::ListNotesOrder::type,LocalStorageManager::OrderDirection::type,
                                                                QList<Note>,QUuid),
                     this, QNSLOT(FavoritesModel,onListNotesComplete,LocalStorageManager::ListObjectsOptions,bool,size_t,size_t,
                                  LocalStorageManager::ListNotesOrder::type,LocalStorageManager::OrderDirection::type,QList<Note>,QUuid));
    QObject::connect(&localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,listNotesFailed,LocalStorageManager::ListObjectsOptions,bool,size_t,size_t,
                                                                LocalStorageManager::ListNotesOrder::type,LocalStorageManager::OrderDirection::type,QString,QUuid),
                     this, QNSLOT(FavoritesModel,onListNotesFailed,LocalStorageManager::ListObjectsOptions,bool,size_t,size_t,
                                  LocalStorageManager::ListNotesOrder::type,LocalStorageManager::OrderDirection::type,QString,QUuid));
    QObject::connect(&localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,expungeNoteComplete,Note,QUuid),
                     this, QNSLOT(FavoritesModel,onExpungeNoteComplete,Note,QUuid));
    QObject::connect(&localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,addNotebookComplete,Notebook,QUuid),
                     this, QNSLOT(FavoritesModel,onAddNotebookComplete,Notebook,QUuid));
    QObject::connect(&localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,updateNotebookComplete,Notebook,QUuid),
                     this, QNSLOT(FavoritesModel,onUpdateNotebookComplete,Notebook,QUuid));
    QObject::connect(&localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,updateNotebookFailed,Notebook,QString,QUuid),
                     this, QNSLOT(FavoritesModel,onUpdateNotebookFailed,Notebook,QString,QUuid));
    QObject::connect(&localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,findNotebookComplete,Notebook,QUuid),
                     this, QNSLOT(FavoritesModel,onFindNotebookComplete,Notebook,QUuid));
    QObject::connect(&localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,findNotebookFailed,Notebook,QString,QUuid),
                     this, QNSLOT(FavoritesModel,onFindNotebookFailed,Notebook,QString,QUuid));
    QObject::connect(&localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,listNotebooksComplete,
                                                                LocalStorageManager::ListObjectsOptions,size_t,size_t,
                                                                LocalStorageManager::ListNotebooksOrder::type,
                                                                LocalStorageManager::OrderDirection::type,QString,
                                                                QList<Notebook>,QUuid),
                     this, QNSLOT(FavoritesModel,onListNotebooksComplete,LocalStorageManager::ListObjectsOptions,
                                  size_t,size_t,LocalStorageManager::ListNotebooksOrder::type,
                                  LocalStorageManager::OrderDirection::type,QString,QList<Notebook>,QUuid));
    QObject::connect(&localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,listNotebooksFailed,
                                                                LocalStorageManager::ListObjectsOptions,size_t,size_t,
                                                                LocalStorageManager::ListNotebooksOrder::type,
                                                                LocalStorageManager::OrderDirection::type,
                                                                QString,QString,QUuid),
                     this, QNSLOT(FavoritesModel,onListNotebooksFailed,LocalStorageManager::ListObjectsOptions,
                                  size_t,size_t,LocalStorageManager::ListNotebooksOrder::type,
                                  LocalStorageManager::OrderDirection::type,QString,QString,QUuid));
    QObject::connect(&localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,expungeNotebookComplete,Notebook,QUuid),
                     this, QNSLOT(FavoritesModel,onExpungeNotebookComplete,Notebook,QUuid));
    QObject::connect(&localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,addTagComplete,Tag,QUuid),
                     this, QNSLOT(FavoritesModel,onAddTagComplete,Tag,QUuid));
    QObject::connect(&localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,updateTagComplete,Tag,QUuid),
                     this, QNSLOT(FavoritesModel,onUpdateTagComplete,Tag,QUuid));
    QObject::connect(&localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,updateTagFailed,Tag,QString,QUuid),
                     this, QNSLOT(FavoritesModel,onUpdateTagFailed,Tag,QString,QUuid));
    QObject::connect(&localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,findTagComplete,Tag,QUuid),
                     this, QNSLOT(FavoritesModel,onFindTagComplete,Tag,QUuid));
    QObject::connect(&localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,findTagFailed,Tag,QString,QUuid),
                     this, QNSLOT(FavoritesModel,onFindTagFailed,Tag,QString,QUuid));
    QObject::connect(&localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,listTagsComplete,LocalStorageManager::ListObjectsOptions,
                                                                size_t,size_t,LocalStorageManager::ListTagsOrder::type,LocalStorageManager::OrderDirection::type,
                                                                QString,QList<Tag>,QUuid),
                     this, QNSLOT(FavoritesModel,onListTagsComplete,LocalStorageManager::ListObjectsOptions,size_t,size_t,
                                  LocalStorageManager::ListTagsOrder::type,LocalStorageManager::OrderDirection::type,QString,QList<Tag>,QUuid));
    QObject::connect(&localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,listTagsFailed,LocalStorageManager::ListObjectsOptions,
                                                                size_t,size_t,LocalStorageManager::ListTagsOrder::type,LocalStorageManager::OrderDirection::type,
                                                                QString,QString,QUuid),
                     this, QNSLOT(FavoritesModel,onListTagsFailed,LocalStorageManager::ListObjectsOptions,size_t,size_t,
                                  LocalStorageManager::ListTagsOrder::type,LocalStorageManager::OrderDirection::type,QString,QString,QUuid));
    QObject::connect(&localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,expungeTagComplete,Tag,QUuid),
                     this, QNSLOT(FavoritesModel,onExpungeTagComplete,Tag,QUuid));
    QObject::connect(&localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,addSavedSearchComplete,SavedSearch,QUuid),
                     this, QNSLOT(FavoritesModel,onAddSavedSearchComplete,SavedSearch,QUuid));
    QObject::connect(&localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,updateSavedSearchComplete,SavedSearch,QUuid),
                     this, QNSLOT(FavoritesModel,onUpdateSavedSearchComplete,SavedSearch,QUuid));
    QObject::connect(&localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,updateSavedSearchFailed,SavedSearch,QString,QUuid),
                     this, QNSLOT(FavoritesModel,onUpdateSavedSearchFailed,SavedSearch,QString,QUuid));
    QObject::connect(&localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,findSavedSearchComplete,SavedSearch,QUuid),
                     this, QNSLOT(FavoritesModel,onFindSavedSearchComplete,SavedSearch,QUuid));
    QObject::connect(&localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,findSavedSearchFailed,SavedSearch,QString,QUuid),
                     this, QNSLOT(FavoritesModel,onFindSavedSearchFailed,SavedSearch,QString,QUuid));
    QObject::connect(&localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,listSavedSearchesComplete,LocalStorageManager::ListObjectsOptions,
                                                                size_t,size_t,LocalStorageManager::ListSavedSearchesOrder::type,
                                                                LocalStorageManager::OrderDirection::type,QList<SavedSearch>,QUuid),
                     this, QNSLOT(FavoritesModel,onListSavedSearchesComplete,LocalStorageManager::ListObjectsOptions,
                                  size_t,size_t,LocalStorageManager::ListSavedSearchesOrder::type, LocalStorageManager::OrderDirection::type,
                                  QList<SavedSearch>,QUuid));
    QObject::connect(&localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,listSavedSearchesFailed,
                                                                LocalStorageManager::ListObjectsOptions,
                                                                size_t,size_t,LocalStorageManager::ListSavedSearchesOrder::type,
                                                                LocalStorageManager::OrderDirection::type,QString,QUuid),
                     this, QNSLOT(FavoritesModel,onListSavedSearchesFailed,LocalStorageManager::ListObjectsOptions,
                                  size_t,size_t,LocalStorageManager::ListSavedSearchesOrder::type,LocalStorageManager::OrderDirection::type,
                                  QString,QUuid));
    QObject::connect(&localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,expungeSavedSearchComplete,SavedSearch,QUuid),
                     this, QNSLOT(FavoritesModel,onExpungeSavedSearchComplete,SavedSearch,QUuid));
    QObject::connect(&localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,noteCountPerNotebookComplete,int,Notebook,QUuid),
                     this, QNSLOT(FavoritesModel,onNoteCountPerNotebookComplete,int,Notebook,QUuid));
    QObject::connect(&localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,noteCountPerNotebookFailed,QString,Notebook,QUuid),
                     this, QNSLOT(FavoritesModel,onNoteCountPerNotebookFailed,QString,Notebook,QUuid));
    QObject::connect(&localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,noteCountPerTagComplete,int,Tag,QUuid),
                     this, QNSLOT(FavoritesModel,onNoteCountPerTagComplete,int,Tag,QUuid));
    QObject::connect(&localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,noteCountPerTagFailed,QString,Tag,QUuid),
                     this, QNSLOT(FavoritesModel,onNoteCountPerTagFailed,QString,Tag,QUuid));
}

void FavoritesModel::requestNotesList()
{
    QNDEBUG("FavoritesModel::requestNotesList: offset = " << m_listNotesOffset);

    LocalStorageManager::ListObjectsOptions flags = LocalStorageManager::ListElementsWithShortcuts;
    LocalStorageManager::ListNotesOrder::type order = LocalStorageManager::ListNotesOrder::NoOrder;
    LocalStorageManager::OrderDirection::type direction = LocalStorageManager::OrderDirection::Ascending;

    m_listNotesRequestId = QUuid::createUuid();
    QNTRACE("Emitting the request to list notes: offset = " << m_listNotesOffset << ", request id = " << m_listNotesRequestId);
    emit listNotes(flags, /* with resource binary data = */ false, NOTE_LIST_LIMIT, m_listNotesOffset, order, direction, m_listNotesRequestId);
}

void FavoritesModel::requestNotebooksList()
{
    QNDEBUG("FavoritesModel::requestNotebooksList: offset = " << m_listNotebooksOffset);

    // NOTE: the subscription to all notebooks is necessary in order to receive the information about the restrictions for various notebooks +
    // for the collection of notebook names to forbid any two notebooks within the account to have the same name in a case-insensitive manner
    LocalStorageManager::ListObjectsOptions flags = LocalStorageManager::ListAll;
    LocalStorageManager::ListNotebooksOrder::type order = LocalStorageManager::ListNotebooksOrder::NoOrder;
    LocalStorageManager::OrderDirection::type direction = LocalStorageManager::OrderDirection::Ascending;

    m_listNotebooksRequestId = QUuid::createUuid();
    QNTRACE("Emitting the request to list notebooks: offset = " << m_listNotebooksOffset << ", request id = " << m_listNotebooksRequestId);
    emit listNotebooks(flags, NOTEBOOK_LIST_LIMIT, m_listNotebooksOffset, order, direction, QString(), m_listNotebooksRequestId);
}

void FavoritesModel::requestTagsList()
{
    QNDEBUG("FavoritesModel::requestTagsList: offset = " << m_listTagsOffset);

    // NOTE: the subscription to all tags is necessary for the collection of tag names to forbid any two tags
    // within the account to have the same name in a case-insensitive manner
    LocalStorageManager::ListObjectsOptions flags = LocalStorageManager::ListAll;
    LocalStorageManager::ListTagsOrder::type order = LocalStorageManager::ListTagsOrder::NoOrder;
    LocalStorageManager::OrderDirection::type direction = LocalStorageManager::OrderDirection::Ascending;

    m_listTagsRequestId = QUuid::createUuid();
    QNTRACE("Emitting the request to list tags: offset = " << m_listTagsOffset << ", request id = " << m_listTagsRequestId);
    emit listTags(flags, TAG_LIST_LIMIT, m_listTagsOffset, order, direction, QString(), m_listTagsRequestId);
}

void FavoritesModel::requestSavedSearchesList()
{
    QNDEBUG("FavoritesModel::requestSavedSearchesList: offset = " << m_listSavedSearchesOffset);

    // NOTE: the subscription to all saved searches is necessary for the collection of saved search names to forbid any two saved searches
    // within the account to have the same name in a case-insensitive manner
    LocalStorageManager::ListObjectsOptions flags = LocalStorageManager::ListAll;
    LocalStorageManager::ListSavedSearchesOrder::type order = LocalStorageManager::ListSavedSearchesOrder::NoOrder;
    LocalStorageManager::OrderDirection::type direction = LocalStorageManager::OrderDirection::Ascending;

    m_listSavedSearchesRequestId = QUuid::createUuid();
    QNTRACE("Emitting the request to list saved searches: offset = " << m_listSavedSearchesOffset
            << ", request id = " << m_listSavedSearchesRequestId);
    emit listSavedSearches(flags, SAVED_SEARCH_LIST_LIMIT, m_listSavedSearchesOffset,
                           order, direction, m_listSavedSearchesRequestId);
}

QVariant FavoritesModel::dataImpl(const int row, const Columns::type column) const
{
    if (Q_UNLIKELY((row < 0) || (row >= static_cast<int>(m_data.size())))) {
        return QVariant();
    }

    const FavoritesDataByIndex & index = m_data.get<ByIndex>();
    const FavoritesModelItem & item = index[static_cast<size_t>(row)];

    switch(column)
    {
    case Columns::Type:
        return item.type();
    case Columns::DisplayName:
        return item.displayName();
    case Columns::NumNotesTargeted:
        return item.numNotesTargeted();
    default:
        return QVariant();
    }
}

QVariant FavoritesModel::dataAccessibleText(const int row, const Columns::type column) const
{
    if (Q_UNLIKELY((row < 0) || (row >= static_cast<int>(m_data.size())))) {
        return QVariant();
    }

    const FavoritesDataByIndex & index = m_data.get<ByIndex>();
    const FavoritesModelItem & item = index[static_cast<size_t>(row)];

    QString space(" ");
    QString colon(":");

    QString accessibleText = QT_TR_NOOP("Favorited") + space;
    switch(item.type())
    {
    case FavoritesModelItem::Type::Note:
        accessibleText += QT_TR_NOOP("note");
        break;
    case FavoritesModelItem::Type::Notebook:
        accessibleText += QT_TR_NOOP("notebook");
        break;
    case FavoritesModelItem::Type::Tag:
        accessibleText += QT_TR_NOOP("tag");
    case FavoritesModelItem::Type::SavedSearch:
        accessibleText += QT_TR_NOOP("saved search");
    default:
        return QVariant();
    }

    switch(column)
    {
    case Columns::Type:
        return accessibleText;
    case Columns::DisplayName:
        accessibleText += colon + space + item.displayName();
        break;
    case Columns::NumNotesTargeted:
        accessibleText += colon + space + QT_TR_NOOP("number of targeted notes is") + space + QString::number(item.numNotesTargeted());
        break;
    default:
        return QVariant();
    }

    return accessibleText;
}

void FavoritesModel::removeItemByLocalUid(const QString & localUid)
{
    QNDEBUG("FavoritesModel::removeItemByLocalUid: local uid = " << localUid);

    FavoritesDataByLocalUid & localUidIndex = m_data.get<ByLocalUid>();
    auto itemIt = localUidIndex.find(localUid);
    if (Q_UNLIKELY(itemIt == localUidIndex.end())) {
        QNDEBUG("Can't find item to remove from the favorites model");
        return;
    }

    const FavoritesModelItem & item = *itemIt;

    switch(item.type())
    {
    case FavoritesModelItem::Type::Notebook:
        {
            auto nameIt = m_lowerCaseNotebookNames.find(item.displayName().toLower());
            if (nameIt != m_lowerCaseNotebookNames.end()) {
                Q_UNUSED(m_lowerCaseNotebookNames.erase(nameIt))
            }

            break;
        }
    case FavoritesModelItem::Type::Tag:
        {
            auto nameIt = m_lowerCaseTagNames.find(item.displayName().toLower());
            if (nameIt != m_lowerCaseTagNames.end()) {
                Q_UNUSED(m_lowerCaseTagNames.erase(nameIt))
            }

            break;
        }
    case FavoritesModelItem::Type::SavedSearch:
        {
            auto nameIt = m_lowerCaseSavedSearchNames.find(item.displayName().toLower());
            if (nameIt != m_lowerCaseSavedSearchNames.end()) {
                Q_UNUSED(m_lowerCaseSavedSearchNames.erase(nameIt))
            }

            break;
        }
    default:
        break;
    }

    FavoritesDataByIndex & index = m_data.get<ByIndex>();
    auto indexIt = m_data.project<ByIndex>(itemIt);
    if (Q_UNLIKELY(indexIt == index.end())) {
        QNWARNING("Can't determine the row index for the favorites model item to remove: " << item);
        return;
    }

    int row = static_cast<int>(std::distance(index.begin(), indexIt));
    if (Q_UNLIKELY((row < 0) || (row >= static_cast<int>(m_data.size())))) {
        QNWARNING("Invalid row index for the favorites model item to remove: index = " << row
                  << ", item: " << item);
        return;
    }

    beginRemoveRows(QModelIndex(), row, row);
    Q_UNUSED(localUidIndex.erase(itemIt))
    endRemoveRows();
}

void FavoritesModel::updateItemRowWithRespectToSorting(const FavoritesModelItem & item)
{
    FavoritesDataByIndex & index = m_data.get<ByIndex>();

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

    FavoritesModelItem itemCopy(item);
    Q_UNUSED(index.erase(it))

    auto positionIter = std::lower_bound(index.begin(), index.end(), itemCopy,
                                         Comparator(m_sortedColumn, m_sortOrder));
    if (positionIter == index.end()) {
        index.push_back(itemCopy);
        return;
    }

    Q_UNUSED(index.insert(positionIter, itemCopy))
}

void FavoritesModel::updateItemInLocalStorage(const FavoritesModelItem & item)
{
    switch(item.type())
    {
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
            QNWARNING("Detected attempt to update favorites model item in local storage for wrong favorited model item's type: " << item);
            break;
        }
    }
}

void FavoritesModel::updateNoteInLocalStorage(const FavoritesModelItem & item)
{
    QNDEBUG("FavoritesModel::updateNoteInLocalStorage: local uid = " << item.localUid()
            << ", title = " << item.displayName());

    const Note * pCachedNote = m_noteCache.get(item.localUid());
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

    Note note = *pCachedNote;

    note.setLocalUid(item.localUid());
    bool dirty = note.isDirty() || !note.hasTitle() || (note.title() != item.displayName());
    note.setDirty(dirty);
    note.setTitle(item.displayName());

    QUuid requestId = QUuid::createUuid();
    Q_UNUSED(m_updateNoteRequestIds.insert(requestId))

    QNTRACE("Emitting the request to update the note in local storage: id = " << requestId
            << ", note: " << note);
    emit updateNote(note, /* update resources = */ false, /* update tags = */ false, requestId);
}

void FavoritesModel::updateNotebookInLocalStorage(const FavoritesModelItem & item)
{
    QNDEBUG("FavoritesModel::updateNotebookInLocalStorage: local uid = " << item.localUid()
            << ", name = " << item.displayName());

    const Notebook * pCachedNotebook = m_notebookCache.get(item.localUid());
    if (Q_UNLIKELY(!pCachedNotebook))
    {
        QUuid requestId = QUuid::createUuid();
        Q_UNUSED(m_findNotebookToPerformUpdateRequestIds.insert(requestId))
        Notebook dummy;
        dummy.setLocalUid(item.localUid());
        QNTRACE("Emitting the request to find notebook: local uid = " << item.localUid()
                << ", request id = " << requestId);
        emit findNotebook(dummy, requestId);
        return;
    }

    Notebook notebook = *pCachedNotebook;

    notebook.setLocalUid(item.localUid());
    bool dirty = notebook.isDirty() || !notebook.hasName() || (notebook.name() != item.displayName());
    notebook.setDirty(dirty);
    notebook.setName(item.displayName());

    QUuid requestId = QUuid::createUuid();
    Q_UNUSED(m_updateNotebookRequestIds.insert(requestId))

    QNTRACE("Emitting the request to update the notebook in local storage: id = " << requestId
            << ", notebook: " << notebook);
    emit updateNotebook(notebook, requestId);
}

void FavoritesModel::updateTagInLocalStorage(const FavoritesModelItem & item)
{
    QNDEBUG("FavoritesModel::updateTagInLocalStorage: local uid = " << item.localUid()
            << ", name = " << item.displayName());

    const Tag * pCachedTag = m_tagCache.get(item.localUid());
    if (Q_UNLIKELY(!pCachedTag))
    {
        QUuid requestId = QUuid::createUuid();
        Q_UNUSED(m_findTagToPerformUpdateRequestIds.insert(requestId))
        Tag dummy;
        dummy.setLocalUid(item.localUid());
        QNTRACE("Emitting the request to find tag: local uid = " << item.localUid()
                << ", request id = " << requestId);
        emit findTag(dummy, requestId);
        return;
    }

    Tag tag = *pCachedTag;

    tag.setLocalUid(item.localUid());
    bool dirty = tag.isDirty() || !tag.hasName() || (tag.name() != item.displayName());
    tag.setDirty(dirty);
    tag.setName(item.displayName());

    QUuid requestId = QUuid::createUuid();
    Q_UNUSED(m_updateTagRequestIds.insert(requestId))

    QNTRACE("Emitting the request to update the tag in local storage: id = " << requestId
            << ", tag: " << tag);
    emit updateTag(tag, requestId);
}

void FavoritesModel::updateSavedSearchInLocalStorage(const FavoritesModelItem & item)
{
    QNDEBUG("FavoritesModel::updateSavedSearchInLocalStorage: local uid = " << item.localUid()
            << ", display name = " << item.displayName());

    const SavedSearch * pCachedSearch = m_savedSearchCache.get(item.localUid());
    if (Q_UNLIKELY(!pCachedSearch))
    {
        QUuid requestId = QUuid::createUuid();
        Q_UNUSED(m_findSavedSearchToPerformUpdateRequestIds.insert(requestId))
        SavedSearch dummy;
        dummy.setLocalUid(item.localUid());
        QNTRACE("Emitting the request to find saved search: local uid = " << item.localUid()
                << ", request id = " << requestId);
        emit findSavedSearch(dummy, requestId);
        return;
    }

    SavedSearch search = *pCachedSearch;

    search.setLocalUid(item.localUid());
    bool dirty = search.isDirty() || !search.hasName() || (search.name() != item.displayName());
    search.setDirty(dirty);
    search.setName(item.displayName());

    QUuid requestId = QUuid::createUuid();
    Q_UNUSED(m_updateSavedSearchRequestIds.insert(requestId))

    QNTRACE("Emitting the request to update the saved search in local storage: id = " << requestId
            << ", saved search: " << search);
    emit updateSavedSearch(search, requestId);
}

bool FavoritesModel::canUpdateNote(const QString & localUid) const
{
    auto notebookLocalUidIt = m_noteLocalUidToNotebookLocalUid.find(localUid);
    if (notebookLocalUidIt == m_noteLocalUidToNotebookLocalUid.end()) {
        return false;
    }

    auto notebookGuidIt = m_notebookLocalUidToGuid.find(notebookLocalUidIt.value());
    if (notebookGuidIt == m_notebookLocalUidToGuid.end()) {
        // NOTE: this must be the local, non-synchronizable notebook as it doesn't have the Evernote service's guid;
        return true;
    }

    auto notebookRestrictionsDataIt = m_notebookRestrictionsData.find(notebookGuidIt.value());
    if (notebookRestrictionsDataIt == m_notebookRestrictionsData.end()) {
        return true;
    }

    const NotebookRestrictionsData & restrictionsData = notebookRestrictionsDataIt.value();
    return restrictionsData.m_canUpdateNotes;
}

bool FavoritesModel::canUpdateNotebook(const QString & localUid) const
{
    auto notebookGuidIt = m_notebookLocalUidToGuid.find(localUid);
    if (notebookGuidIt == m_notebookLocalUidToGuid.end()) {
        // NOTE: this must be the local, non-synchronizable notebook as it doesn't have the Evernote service's guid;
        return true;
    }

    auto notebookRestrictionsDataIt = m_notebookRestrictionsData.find(notebookGuidIt.value());
    if (notebookRestrictionsDataIt == m_notebookRestrictionsData.end()) {
        return true;
    }

    const NotebookRestrictionsData & restrictionsData = notebookRestrictionsDataIt.value();
    return restrictionsData.m_canUpdateNotebook;
}

bool FavoritesModel::canUpdateTag(const QString & localUid) const
{
    auto notebookGuidIt = m_tagLocalUidToLinkedNotebookGuid.find(localUid);
    if (notebookGuidIt == m_tagLocalUidToLinkedNotebookGuid.end()) {
        return true;
    }

    auto notebookRestrictionsDataIt = m_notebookRestrictionsData.find(notebookGuidIt.value());
    if (notebookRestrictionsDataIt == m_notebookRestrictionsData.end()) {
        return true;
    }

    const NotebookRestrictionsData & restrictionsData = notebookRestrictionsDataIt.value();
    return restrictionsData.m_canUpdateTags;
}

void FavoritesModel::unfavoriteNote(const QString & localUid)
{
    QNDEBUG("FavoritesModel::unfavoriteNote: local uid = " << localUid);

    const Note * pCachedNote = m_noteCache.get(localUid);
    if (Q_UNLIKELY(!pCachedNote))
    {
        QUuid requestId = QUuid::createUuid();
        Q_UNUSED(m_findNoteToUnfavoriteRequestIds.insert(requestId))
        Note dummy;
        dummy.setLocalUid(localUid);
        QNTRACE("Emitting the request to find note: local uid = " << localUid
                << ", request id = " << requestId);
        emit findNote(dummy, /* with resource binary data = */ false, requestId);
        return;
    }

    Note note = *pCachedNote;

    note.setLocalUid(localUid);
    bool dirty = note.isDirty() || note.hasShortcut();
    note.setDirty(dirty);
    note.setShortcut(false);

    QUuid requestId = QUuid::createUuid();
    Q_UNUSED(m_updateNoteRequestIds.insert(requestId))

    QNTRACE("Emitting the request to update the note in local storage: id = " << requestId
            << ", note: " << note);
    emit updateNote(note, /* update resources = */ false, /* update tags = */ false, requestId);
}

void FavoritesModel::unfavoriteNotebook(const QString & localUid)
{
    QNDEBUG("FavoritesModel::unfavoriteNotebook: local uid = " << localUid);

    const Notebook * pCachedNotebook = m_notebookCache.get(localUid);
    if (Q_UNLIKELY(!pCachedNotebook))
    {
        QUuid requestId = QUuid::createUuid();
        Q_UNUSED(m_findNotebookToUnfavoriteRequestIds.insert(requestId))
        Notebook dummy;
        dummy.setLocalUid(localUid);
        QNTRACE("Emitting the request to find notebook: local uid = " << localUid
                << ", request id = " << requestId);
        emit findNotebook(dummy, requestId);
        return;
    }

    Notebook notebook = *pCachedNotebook;

    notebook.setLocalUid(localUid);
    bool dirty = notebook.isDirty() || notebook.hasShortcut();
    notebook.setDirty(dirty);
    notebook.setShortcut(false);

    QUuid requestId = QUuid::createUuid();
    Q_UNUSED(m_updateNotebookRequestIds.insert(requestId))

    QNTRACE("Emitting the request to update the notebook in local storage: id = " << requestId
            << ", notebook: " << notebook);
    emit updateNotebook(notebook, requestId);
}

void FavoritesModel::unfavoriteTag(const QString & localUid)
{
    QNDEBUG("FavoritesModel::unfavoriteTag: local uid = " << localUid);

    const Tag * pCachedTag = m_tagCache.get(localUid);
    if (Q_UNLIKELY(!pCachedTag))
    {
        QUuid requestId = QUuid::createUuid();
        Q_UNUSED(m_findTagToUnfavoriteRequestIds.insert(requestId))
        Tag dummy;
        dummy.setLocalUid(localUid);
        QNTRACE("Emitting the request to find tag: local uid = " << localUid
                << ", request id = " << requestId);
        emit findTag(dummy, requestId);
        return;
    }

    Tag tag = *pCachedTag;

    tag.setLocalUid(localUid);
    bool dirty = tag.isDirty() || tag.hasShortcut();
    tag.setDirty(dirty);
    tag.setShortcut(false);

    QUuid requestId = QUuid::createUuid();
    Q_UNUSED(m_updateTagRequestIds.insert(requestId))

    QNTRACE("Emitting the request to update the tag in local storage: id = " << requestId
            << ", tag: " << tag);
    emit updateTag(tag, requestId);
}

void FavoritesModel::unfavoriteSavedSearch(const QString & localUid)
{
    QNDEBUG("FavoritesModel::unfavoriteSavedSearch: local uid = " << localUid);

    const SavedSearch * pCachedSearch = m_savedSearchCache.get(localUid);
    if (Q_UNLIKELY(!pCachedSearch))
    {
        QUuid requestId = QUuid::createUuid();
        Q_UNUSED(m_findSavedSearchToUnfavoriteRequestIds.insert(requestId))
        SavedSearch dummy;
        dummy.setLocalUid(localUid);
        QNTRACE("Emitting the request to find saved search: local uid = " << localUid
                << ", request id = " << requestId);
        emit findSavedSearch(dummy, requestId);
        return;
    }

    SavedSearch search = *pCachedSearch;

    search.setLocalUid(localUid);
    bool dirty = search.isDirty() || search.hasShortcut();
    search.setDirty(dirty);
    search.setShortcut(false);

    QUuid requestId = QUuid::createUuid();
    Q_UNUSED(m_updateSavedSearchRequestIds.insert(requestId))

    QNTRACE("Emitting the request to update the saved search in local storage: id = " << requestId
            << ", saved search: " << search);
    emit updateSavedSearch(search, requestId);
}

void FavoritesModel::onNoteAddedOrUpdated(const Note & note)
{
    QNDEBUG("FavoritesModel::onNoteAddedOrUpdated: note local uid = " << note.localUid());

    m_noteCache.put(note.localUid(), note);

    if (!note.hasNotebookLocalUid()) {
        QNWARNING("Skipping the note not having the notebook local uid: " << note);
        return;
    }

    if (!note.hasShortcut()) {
        removeItemByLocalUid(note.localUid());
        return;
    }

    FavoritesModelItem item;
    item.setType(FavoritesModelItem::Type::Note);
    item.setLocalUid(note.localUid());
    item.setNumNotesTargeted(0);

    if (note.hasTitle())
    {
        item.setDisplayName(note.title());
    }
    else if (note.hasContent())
    {
        QString plainText = note.plainText();
        plainText.chop(160);
        item.setDisplayName(plainText);
        // NOTE: using the text preview in this way means updating the favorites item's display name would actually create the title for the note
    }

    m_noteLocalUidToNotebookLocalUid[note.localUid()] = note.notebookLocalUid();

    FavoritesDataByIndex & index = m_data.get<ByIndex>();
    FavoritesDataByLocalUid & localUidIndex = m_data.get<ByLocalUid>();
    auto itemIt = localUidIndex.find(note.localUid());
    if (itemIt == localUidIndex.end())
    {
        QNDEBUG("Detected newly favorited note");

        emit layoutAboutToBeChanged();
        Q_UNUSED(localUidIndex.insert(item))
        updateItemRowWithRespectToSorting(item);
        emit layoutChanged();
    }
    else
    {
        QNDEBUG("Updating the already favorited item");

        auto indexIt = m_data.project<ByIndex>(itemIt);
        if (Q_UNLIKELY(indexIt == index.end())) {
            QString error = QT_TR_NOOP("Can't project the local uid index to the favorited note item to the random access index iterator");
            QNWARNING(error << ", favorites model item: " << item);
            emit notifyError(error);
            return;
        }

        int row = static_cast<int>(std::distance(index.begin(), indexIt));

        localUidIndex.replace(itemIt, item);
        QModelIndex modelIndex = createIndex(row, Columns::DisplayName);
        emit dataChanged(modelIndex, modelIndex);

        emit layoutAboutToBeChanged();
        updateItemRowWithRespectToSorting(item);
        emit layoutChanged();
    }
}

void FavoritesModel::onNotebookAddedOrUpdated(const Notebook & notebook)
{
    QNDEBUG("FavoritesModel::onNotebookAddedOrUpdated: local uid =  " << notebook.localUid());

    m_notebookCache.put(notebook.localUid(), notebook);

    FavoritesDataByLocalUid & localUidIndex = m_data.get<ByLocalUid>();
    auto itemIt = localUidIndex.find(notebook.localUid());

    if (itemIt != localUidIndex.end())
    {
        const FavoritesModelItem & originalItem = *itemIt;
        auto nameIt = m_lowerCaseNotebookNames.find(originalItem.displayName().toLower());
        if (nameIt != m_lowerCaseNotebookNames.end()) {
            Q_UNUSED(m_lowerCaseNotebookNames.erase(nameIt))
        }
    }

    if (notebook.hasName()) {
        Q_UNUSED(m_lowerCaseNotebookNames.insert(notebook.name().toLower()))
    }

    if (notebook.hasGuid())
    {
        NotebookRestrictionsData & notebookRestrictionsData = m_notebookRestrictionsData[notebook.guid()];

        if (notebook.hasRestrictions())
        {
            const qevercloud::NotebookRestrictions & restrictions = notebook.restrictions();

            notebookRestrictionsData.m_canUpdateNotebook = !restrictions.noUpdateNotebook.isSet() || !restrictions.noUpdateNotebook.ref();
            notebookRestrictionsData.m_canUpdateNotes = !restrictions.noUpdateNotes.isSet() || !restrictions.noUpdateNotes.ref();
            notebookRestrictionsData.m_canUpdateTags = !restrictions.noUpdateTags.isSet() || !restrictions.noUpdateTags.ref();
        }
        else
        {
            notebookRestrictionsData.m_canUpdateNotebook = true;
            notebookRestrictionsData.m_canUpdateNotes = true;
            notebookRestrictionsData.m_canUpdateTags = true;
        }

        QNTRACE("Updated restrictions data for notebook " << notebook.localUid() << ", name "
                << (notebook.hasName() ? QStringLiteral("\"") + notebook.name() + QStringLiteral("\"") : QStringLiteral("<not set>"))
                << ", guid = " << notebook.guid() << ": can update notebook = "
                << (notebookRestrictionsData.m_canUpdateNotebook ? "true" : "false") << ", can update notes = "
                << (notebookRestrictionsData.m_canUpdateNotes ? "true" : "false") << ", can update tags = "
                << (notebookRestrictionsData.m_canUpdateTags ? "true" : "false"));
    }

    if (!notebook.hasName()) {
        QNTRACE("Removing/skipping the notebook without a name");
        removeItemByLocalUid(notebook.localUid());
        return;
    }

    if (!notebook.hasShortcut()) {
        QNTRACE("Removing/skipping non-favorited notebook")
        removeItemByLocalUid(notebook.localUid());
        return;
    }

    FavoritesModelItem item;
    item.setType(FavoritesModelItem::Type::Notebook);
    item.setLocalUid(notebook.localUid());
    item.setNumNotesTargeted(-1);   // That means, not known yet
    item.setDisplayName(notebook.name());

    if (itemIt == localUidIndex.end())
    {
        QNDEBUG("Detected newly favorited notebook");

        emit layoutAboutToBeChanged();
        Q_UNUSED(localUidIndex.insert(item))
        updateItemRowWithRespectToSorting(item);
        emit layoutChanged();
    }
    else
    {
        QNDEBUG("Updating the already favorited notebook item");

        FavoritesDataByIndex & index = m_data.get<ByIndex>();
        auto indexIt = m_data.project<ByIndex>(itemIt);
        if (Q_UNLIKELY(indexIt == index.end())) {
            QString error = QT_TR_NOOP("Can't project the local uid index to the favorited notebook item to the random access index iterator");
            QNWARNING(error << ", favorites model item: " << item);
            emit notifyError(error);
            return;
        }

        int row = static_cast<int>(std::distance(index.begin(), indexIt));

        localUidIndex.replace(itemIt, item);
        QModelIndex modelIndex = createIndex(row, Columns::DisplayName);
        emit dataChanged(modelIndex, modelIndex);

        emit layoutAboutToBeChanged();
        updateItemRowWithRespectToSorting(item);
        emit layoutChanged();
    }
}

void FavoritesModel::onTagAddedOrUpdated(const Tag & tag)
{
    QNDEBUG("FavoritesModel::onTagAddedOrUpdated: local uid = " << tag.localUid());

    m_tagCache.put(tag.localUid(), tag);

    FavoritesDataByLocalUid & localUidIndex = m_data.get<ByLocalUid>();
    auto itemIt = localUidIndex.find(tag.localUid());

    if (itemIt != localUidIndex.end())
    {
        const FavoritesModelItem & originalItem = *itemIt;
        auto nameIt = m_lowerCaseTagNames.find(originalItem.displayName().toLower());
        if (nameIt != m_lowerCaseTagNames.end()) {
            Q_UNUSED(m_lowerCaseTagNames.erase(nameIt))
        }
    }

    if (tag.hasName()) {
        Q_UNUSED(m_lowerCaseTagNames.insert(tag.name().toLower()))
    }
    else {
        QNTRACE("Removing/skipping the tag without a name");
        removeItemByLocalUid(tag.localUid());
        return;
    }

    if (!tag.hasShortcut()) {
        QNTRACE("Removing/skipping non-favorited tag");
        removeItemByLocalUid(tag.localUid());
        return;
    }

    FavoritesModelItem item;
    item.setType(FavoritesModelItem::Type::Tag);
    item.setLocalUid(tag.localUid());
    item.setNumNotesTargeted(-1);   // That means, not known yet
    item.setDisplayName(tag.name());

    if (itemIt == localUidIndex.end())
    {
        QNDEBUG("Detected newly favorited tag");

        emit layoutAboutToBeChanged();
        Q_UNUSED(localUidIndex.insert(item))
        updateItemRowWithRespectToSorting(item);
        emit layoutChanged();
    }
    else
    {
        QNDEBUG("Updating the already favorited tag item");

        FavoritesDataByIndex & index = m_data.get<ByIndex>();
        auto indexIt = m_data.project<ByIndex>(itemIt);
        if (Q_UNLIKELY(indexIt == index.end())) {
            QString error = QT_TR_NOOP("Can't project the local uid index to the favorited tag item to the random access index iterator");
            QNWARNING(error << ", favorites model item: " << item);
            emit notifyError(error);
            return;
        }

        int row = static_cast<int>(std::distance(index.begin(), indexIt));

        localUidIndex.replace(itemIt, item);
        QModelIndex modelIndex = createIndex(row, Columns::DisplayName);
        emit dataChanged(modelIndex, modelIndex);

        emit layoutAboutToBeChanged();
        updateItemRowWithRespectToSorting(item);
        emit layoutChanged();
    }
}

void FavoritesModel::onSavedSearchAddedOrUpdated(const SavedSearch & search)
{
    QNDEBUG("FavoritesModel::onSavedSearchAddedOrUpdated: local uid = " << search.localUid());

    m_savedSearchCache.put(search.localUid(), search);

    FavoritesDataByLocalUid & localUidIndex = m_data.get<ByLocalUid>();
    auto itemIt = localUidIndex.find(search.localUid());

    if (itemIt != localUidIndex.end())
    {
        const FavoritesModelItem & originalItem = *itemIt;
        auto nameIt = m_lowerCaseSavedSearchNames.find(originalItem.displayName().toLower());
        if (nameIt != m_lowerCaseSavedSearchNames.end()) {
            Q_UNUSED(m_lowerCaseSavedSearchNames.erase(nameIt))
        }
    }

    if (search.hasName()) {
        Q_UNUSED(m_lowerCaseSavedSearchNames.insert(search.name().toLower()))
    }
    else {
        QNTRACE("Removing/skipping the search without a name");
        removeItemByLocalUid(search.localUid());
        return;
    }

    if (!search.hasShortcut()) {
        QNTRACE("Removing/skipping non-favorited search");
        removeItemByLocalUid(search.localUid());
        return;
    }

    FavoritesModelItem item;
    item.setType(FavoritesModelItem::Type::SavedSearch);
    item.setLocalUid(search.localUid());
    item.setNumNotesTargeted(-1);
    item.setDisplayName(search.name());

    if (itemIt == localUidIndex.end())
    {
        QNDEBUG("Detected newly favorited saved search");

        emit layoutAboutToBeChanged();
        Q_UNUSED(localUidIndex.insert(item))
        updateItemRowWithRespectToSorting(item);
        emit layoutChanged();
    }
    else
    {
        QNDEBUG("Updating the already favorited saved search item");

        FavoritesDataByIndex & index = m_data.get<ByIndex>();
        auto indexIt = m_data.project<ByIndex>(itemIt);
        if (Q_UNLIKELY(indexIt == index.end())) {
            QString error = QT_TR_NOOP("Can't project the local uid index to the favorited tag item to the random access index iterator");
            QNWARNING(error << ", favorites model item: " << item);
            emit notifyError(error);
            return;
        }

        int row = static_cast<int>(std::distance(index.begin(), indexIt));

        localUidIndex.replace(itemIt, item);
        QModelIndex modelIndex = createIndex(row, Columns::DisplayName);
        emit dataChanged(modelIndex, modelIndex);

        emit layoutAboutToBeChanged();
        updateItemRowWithRespectToSorting(item);
        emit layoutChanged();
    }
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
