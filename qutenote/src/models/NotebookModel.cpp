#include "NotebookModel.h"
#include <qute_note/logging/QuteNoteLogger.h>

namespace qute_note {

NotebookModel::NotebookModel(LocalStorageManagerThreadWorker & localStorageManagerThreadWorker,
                             QObject * parent) :
    QAbstractItemModel(parent),
    m_data(),
    m_fakeRootItem(Q_NULLPTR),
    m_modelItemsByLocalUid(),
    m_modelItemsByStack(),
    m_cache(),
    m_listNotebooksOffset(0),
    m_listNotebooksRequestId(),
    m_addNotebookRequestIds(),
    m_updateNotebookRequestIds(),
    m_expungeNotebookRequestIds(),
    m_findNotebookToRestoreFailedUpdateRequestIds(),
    m_findNotebookToPerformUpdateRequestIds(),
    m_sortedColumn(Columns::Name),
    m_sortOrder(Qt::AscendingOrder),
    m_lastNewNotebookNameCounter(0)
{
    createConnections(localStorageManagerThreadWorker);
    requestNotebooksList();
}

NotebookModel::~NotebookModel()
{}

const NotebookModelItem * NotebookModel::itemForIndex(const QModelIndex & index) const
{
    // TODO: implement
    Q_UNUSED(index)
    return Q_NULLPTR;
}

QModelIndex NotebookModel::indexForItem(const NotebookModelItem * item) const
{
    // TODO: implement
    Q_UNUSED(item)
    return QModelIndex();
}

QModelIndex NotebookModel::indexForLocalUid(const QString & localUid) const
{
    // TODO: implement
    Q_UNUSED(localUid)
    return QModelIndex();
}

Qt::ItemFlags NotebookModel::flags(const QModelIndex & index) const
{
    Qt::ItemFlags indexFlags = QAbstractItemModel::flags(index);
    if (!index.isValid()) {
        return indexFlags;
    }

    indexFlags |= Qt::ItemIsSelectable;
    indexFlags |= Qt::ItemIsEnabled;

    if ((index.column() == Columns::Dirty) ||
        (index.column() == Columns::FromLinkedNotebook))
    {
        return indexFlags;
    }

    if (index.column() == Columns::Synchronizable)
    {
        const NotebookModelItem * item = itemForIndex(index);
        if (Q_UNLIKELY(!item)) {
            QNWARNING("Can't find notebook model item for a given index: row = " << index.row()
                      << ", column = " << index.column());
            return indexFlags;
        }

        if (item->type() == NotebookModelItem::Type::Stack)
        {
            const NotebookStackItem * stackItem = item->notebookStackItem();
            if (Q_UNLIKELY(!stackItem)) {
                QNWARNING("Internal inconsistency detected: notebook model item has stack type "
                          "but the pointer to the stack item is null");
                return indexFlags;
            }

            QList<const NotebookItem*> notebooks = stackItem->children();
            for(auto it = notebooks.begin(), end = notebooks.end(); it != end; ++it)
            {
                const NotebookItem * notebookItem = *it;
                if (Q_UNLIKELY(!notebookItem)) {
                    QNWARNING("Detected null pointer to notebook item within the children of notebook stack item");
                    continue;
                }

                if (notebookItem->isSynchronizable()) {
                    return indexFlags;
                }
            }
        }
        else
        {
            const NotebookItem * notebookItem = item->notebookItem();
            if (Q_UNLIKELY(!notebookItem)) {
                QNWARNING("Detected null pointer to notebook item within the notebook model");
                return indexFlags;
            }

            if (notebookItem->isSynchronizable()) {
                return indexFlags;
            }
        }
    }

    indexFlags |= Qt::ItemIsEditable;

    return indexFlags;
}

QVariant NotebookModel::data(const QModelIndex & index, int role) const
{
    // TODO: implement
    Q_UNUSED(index)
    Q_UNUSED(role)
    return QVariant();
}

QVariant NotebookModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    // TODO: implement
    Q_UNUSED(section)
    Q_UNUSED(orientation)
    Q_UNUSED(role)
    return QVariant();
}

int NotebookModel::rowCount(const QModelIndex & parent) const
{
    // TODO: implement
    Q_UNUSED(parent)
    return 0;
}

int NotebookModel::columnCount(const QModelIndex & parent) const
{
    // TODO: implement
    Q_UNUSED(parent)
    return 0;
}

QModelIndex NotebookModel::index(int row, int column, const QModelIndex & parent) const
{
    // TODO: implement
    Q_UNUSED(row)
    Q_UNUSED(column)
    Q_UNUSED(parent)
    return QModelIndex();
}

QModelIndex NotebookModel::parent(const QModelIndex & index) const
{
    // TODO: implement
    Q_UNUSED(index)
    return QModelIndex();
}

bool NotebookModel::setHeaderData(int section, Qt::Orientation orientation, const QVariant & value, int role)
{
    Q_UNUSED(section)
    Q_UNUSED(orientation)
    Q_UNUSED(value)
    Q_UNUSED(role)
    return false;
}

bool NotebookModel::setData(const QModelIndex & index, const QVariant & value, int role)
{
    // TODO: implement
    Q_UNUSED(index)
    Q_UNUSED(value)
    Q_UNUSED(role)
    return false;
}

bool NotebookModel::insertRows(int row, int count, const QModelIndex & parent)
{
    // TODO: implement
    Q_UNUSED(row)
    Q_UNUSED(count)
    Q_UNUSED(parent)
    return false;
}

bool NotebookModel::removeRows(int row, int count, const QModelIndex & parent)
{
    // TODO: implement
    Q_UNUSED(row)
    Q_UNUSED(count)
    Q_UNUSED(parent)
    return false;
}

void NotebookModel::onAddNotebookComplete(Notebook notebook, QUuid requestId)
{
    QNDEBUG("NotebookModel::onAddNotebookComplete: notebook = " << notebook << "\nRequest id = " << requestId);

    // TODO: implement
}

void NotebookModel::onAddNotebookFailed(Notebook notebook, QString errorDescription, QUuid requestId)
{
    // TODO: implement
    Q_UNUSED(notebook)
    Q_UNUSED(errorDescription)
    Q_UNUSED(requestId)
}

void NotebookModel::onUpdateNotebookComplete(Notebook notebook, QUuid requestId)
{
    // TODO: implement
    Q_UNUSED(notebook)
    Q_UNUSED(requestId)
}

void NotebookModel::onUpdateNotebookFailed(Notebook notebook, QString errorDescription, QUuid requestId)
{
    // TODO: implement
    Q_UNUSED(notebook)
    Q_UNUSED(errorDescription)
    Q_UNUSED(requestId)
}

void NotebookModel::onFindNotebookComplete(Notebook notebook, QUuid requestId)
{
    // TODO: implement
    Q_UNUSED(notebook)
    Q_UNUSED(requestId)
}

void NotebookModel::onFindNotebookFailed(Notebook notebook, QString errorDescription, QUuid requestId)
{
    // TODO: implement
    Q_UNUSED(notebook)
    Q_UNUSED(errorDescription)
    Q_UNUSED(requestId)
}

void NotebookModel::onListNotebooksComplete(LocalStorageManager::ListObjectsOptions flag,
                                            size_t limit, size_t offset,
                                            LocalStorageManager::ListNotebooksOrder::type order,
                                            LocalStorageManager::OrderDirection::type orderDirection,
                                            QString linkedNotebookGuid, QList<Notebook> foundNotebooks, QUuid requestId)
{
    // TODO: implement
    Q_UNUSED(flag)
    Q_UNUSED(limit)
    Q_UNUSED(offset)
    Q_UNUSED(order)
    Q_UNUSED(orderDirection)
    Q_UNUSED(linkedNotebookGuid)
    Q_UNUSED(foundNotebooks)
    Q_UNUSED(requestId)
}

void NotebookModel::onListNotebooksFailed(LocalStorageManager::ListObjectsOptions flag,
                                          size_t limit, size_t offset,
                                          LocalStorageManager::ListNotebooksOrder::type order,
                                          LocalStorageManager::OrderDirection::type orderDirection,
                                          QString linkedNotebookGuid, QString errorDescription, QUuid requestId)
{
    // TODO: implement
    Q_UNUSED(flag)
    Q_UNUSED(limit)
    Q_UNUSED(offset)
    Q_UNUSED(order)
    Q_UNUSED(orderDirection)
    Q_UNUSED(linkedNotebookGuid)
    Q_UNUSED(errorDescription)
    Q_UNUSED(requestId)
}

void NotebookModel::onExpungeNotebookComplete(Notebook notebook, QUuid requestId)
{
    // TODO: implement
    Q_UNUSED(notebook)
    Q_UNUSED(requestId)
}

void NotebookModel::onExpungeNotebookFailed(Notebook notebook, QString errorDescription, QUuid requestId)
{
    // TODO: implement
    Q_UNUSED(notebook)
    Q_UNUSED(errorDescription)
    Q_UNUSED(requestId)
}

void NotebookModel::createConnections(LocalStorageManagerThreadWorker & localStorageManagerThreadWorker)
{
    QNDEBUG("NotebookModel::createConnections");

    // Local signals to localStorageManagerThreadWorker's slots
    QObject::connect(this, QNSIGNAL(NotebookModel,addNotebook,Notebook,QUuid),
                     &localStorageManagerThreadWorker, QNSLOT(LocalStorageManagerThreadWorker,onAddNotebookRequest,Notebook,QUuid));
    QObject::connect(this, QNSIGNAL(NotebookModel,updateNotebook,Notebook,QUuid),
                     &localStorageManagerThreadWorker, QNSLOT(LocalStorageManagerThreadWorker,onUpdateNotebookRequest,Notebook,QUuid));
    QObject::connect(this, QNSIGNAL(NotebookModel,findNotebook,Notebook,QUuid),
                     &localStorageManagerThreadWorker, QNSLOT(LocalStorageManagerThreadWorker,onFindNotebookRequest,Notebook,QUuid));
    QObject::connect(this, QNSIGNAL(NotebookModel,listNotebooks,LocalStorageManager::ListObjectsOptions,
                                    size_t,size_t,LocalStorageManager::ListNotebooksOrder::type,
                                    LocalStorageManager::OrderDirection::type,QString,QUuid),
                     &localStorageManagerThreadWorker, QNSLOT(LocalStorageManagerThreadWorker,onListNotebooksRequest,
                                                              LocalStorageManager::ListObjectsOptions,
                                                              size_t,size_t,LocalStorageManager::ListNotebooksOrder::type,
                                                              LocalStorageManager::OrderDirection::type,QString,QUuid));
    QObject::connect(this, QNSIGNAL(NotebookModel,expungeNotebook,Notebook,QUuid),
                     &localStorageManagerThreadWorker, QNSLOT(LocalStorageManagerThreadWorker,onExpungeNotebookRequest,Notebook,QUuid));

    // localStorageManagerThreadWorker's signals to local slots
    QObject::connect(&localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,addNotebookComplete,Notebook,QUuid),
                     this, QNSLOT(NotebookModel,onAddNotebookComplete,Notebook,QUuid));
    QObject::connect(&localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,addNotebookFailed,Notebook,QString,QUuid),
                     this, QNSLOT(NotebookModel,onAddNotebookFailed,Notebook,QString,QUuid));
    QObject::connect(&localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,updateNotebookComplete,Notebook,QUuid),
                     this, QNSLOT(NotebookModel,onUpdateNotebookComplete,Notebook,QUuid));
    QObject::connect(&localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,updateNotebookFailed,Notebook,QString,QUuid),
                     this, QNSLOT(NotebookModel,onUpdateNotebookFailed,Notebook,QString,QUuid));
    QObject::connect(&localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,findNotebookComplete,Notebook,QUuid),
                     this, QNSLOT(NotebookModel,onFindNotebookComplete,Notebook,QUuid));
    QObject::connect(&localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,findNotebookFailed,Notebook,QString,QUuid),
                     this, QNSLOT(NotebookModel,onFindNotebookFailed,Notebook,QString,QUuid));
    QObject::connect(&localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,listNotebooksComplete,
                                                                LocalStorageManager::ListObjectsOptions,size_t,size_t,
                                                                LocalStorageManager::ListNotebooksOrder::type,
                                                                LocalStorageManager::OrderDirection::type,QString,
                                                                QList<Notebook>,QUuid),
                     this, QNSLOT(NotebookModel,onListNotebooksComplete,LocalStorageManager::ListObjectsOptions,
                                  size_t,size_t,LocalStorageManager::ListNotebooksOrder::type,
                                  LocalStorageManager::OrderDirection::type,QString,QList<Notebook>,QUuid));
    QObject::connect(&localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,listNotebooksFailed,
                                                                LocalStorageManager::ListObjectsOptions,size_t,size_t,
                                                                LocalStorageManager::ListNotebooksOrder::type,
                                                                LocalStorageManager::OrderDirection::type,
                                                                QString,QString,QUuid),
                     this, QNSLOT(NotebookModel,onListNotebooksFailed,LocalStorageManager::ListObjectsOptions,
                                  size_t,size_t,LocalStorageManager::ListNotebooksOrder::type,
                                  LocalStorageManager::OrderDirection::type,QString,QString,QUuid));
    QObject::connect(&localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,expungeNotebookComplete,Notebook,QUuid),
                     this, QNSLOT(NotebookModel,onExpungeNotebookComplete,Notebook,QUuid));
    QObject::connect(&localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,expungeNotebookFailed,Notebook,QString,QUuid),
                     this, QNSLOT(NotebookModel,onExpungeNotebookFailed,Notebook,QString,QUuid));
}

void NotebookModel::requestNotebooksList()
{
    QNDEBUG("NotebookModel::requestNotebooksList");

    // TODO: implement
}

bool NotebookModel::LessByName::operator()(const NotebookItem & lhs, const NotebookItem & rhs) const
{
    return (lhs.nameUpper().localeAwareCompare(rhs.nameUpper()) <= 0);
}

#define ITEM_PTR_LESS(lhs, rhs) \
    if (!lhs) { \
        return true; \
    } \
    else if (!rhs) { \
        return false; \
    } \
    else { \
        return this->operator()(*lhs, *rhs); \
    }

bool NotebookModel::LessByName::operator()(const NotebookItem * lhs, const NotebookItem * rhs) const
{
    ITEM_PTR_LESS(lhs, rhs)
}

#define MODEL_ITEM_NAME(item, itemName) \
    if ((item.type() == NotebookModelItem::Type::Notebook) && item.notebookItem()) { \
        itemName = item.notebookItem()->nameUpper(); \
    } \
    else if ((item.type() == NotebookModelItem::Type::Stack) && item.notebookStackItem()) { \
        itemName = item.notebookStackItem()->name().toUpper(); \
    }

bool NotebookModel::LessByName::operator()(const NotebookModelItem & lhs, const NotebookModelItem & rhs) const
{
    QString lhsName;
    MODEL_ITEM_NAME(lhs, lhsName)

    QString rhsName;
    MODEL_ITEM_NAME(rhs, rhsName)

    return (lhsName.localeAwareCompare(rhsName) <= 0);
}

bool NotebookModel::LessByName::operator()(const NotebookModelItem * lhs, const NotebookModelItem * rhs) const
{
    ITEM_PTR_LESS(lhs, rhs)
}

bool NotebookModel::LessByName::operator()(const NotebookStackItem & lhs, const NotebookStackItem & rhs) const
{
    return (lhs.name().localeAwareCompare(rhs.name()) <= 0);
}

bool NotebookModel::LessByName::operator()(const NotebookStackItem * lhs, const NotebookStackItem * rhs) const
{
    ITEM_PTR_LESS(lhs, rhs)
}

bool NotebookModel::GreaterByName::operator()(const NotebookItem & lhs, const NotebookItem & rhs) const
{
    return (lhs.nameUpper().localeAwareCompare(rhs.nameUpper()) > 0);
}

#define ITEM_PTR_GREATER(lhs, rhs) \
    if (!lhs) { \
        return false; \
    } \
    else if (!rhs) { \
        return true; \
    } \
    else { \
        return this->operator()(*lhs, *rhs); \
    }

bool NotebookModel::GreaterByName::operator()(const NotebookItem * lhs, const NotebookItem * rhs) const
{
    ITEM_PTR_GREATER(lhs, rhs)
}

bool NotebookModel::GreaterByName::operator()(const NotebookStackItem & lhs, const NotebookStackItem & rhs) const
{
    return (lhs.name().localeAwareCompare(rhs.name()) > 0);
}

bool NotebookModel::GreaterByName::operator()(const NotebookStackItem * lhs, const NotebookStackItem * rhs) const
{
    ITEM_PTR_GREATER(lhs, rhs)
}

bool NotebookModel::GreaterByName::operator()(const NotebookModelItem & lhs, const NotebookModelItem & rhs) const
{
    QString lhsName;
    MODEL_ITEM_NAME(lhs, lhsName)

    QString rhsName;
    MODEL_ITEM_NAME(rhs, rhsName)

    return (lhsName.localeAwareCompare(rhsName) > 0);
}

bool NotebookModel::GreaterByName::operator()(const NotebookModelItem * lhs, const NotebookModelItem * rhs) const
{
    ITEM_PTR_GREATER(lhs, rhs)
}

} // namespace qute_note
