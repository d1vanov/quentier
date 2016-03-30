#include "NotebookModel.h"
#include <qute_note/logging/QuteNoteLogger.h>

namespace qute_note {

NotebookModel::NotebookModel(LocalStorageManagerThreadWorker & localStorageManagerThreadWorker,
                             QObject * parent) :
    QAbstractItemModel(parent)
{
    createConnections(localStorageManagerThreadWorker);
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

void NotebookModel::createConnections(LocalStorageManagerThreadWorker & localStorageManagerThreadWorker)
{
    QNDEBUG("NotebookModel::createConnections");

    // TODO: implement
    Q_UNUSED(localStorageManagerThreadWorker)
}

} // namespace qute_note
