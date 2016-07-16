/*
 * Copyright 2016 Dmitry Ivanov
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

#include "NotebookModel.h"
#include "NewItemNameGenerator.hpp"
#include <quentier/logging/QuentierLogger.h>
#include <QMimeData>

namespace quentier {

// Limit for the queries to the local storage
#define NOTEBOOK_LIST_LIMIT (40)

#define NUM_NOTEBOOK_MODEL_COLUMNS (6)

NotebookModel::NotebookModel(LocalStorageManagerThreadWorker & localStorageManagerThreadWorker,
                             NotebookCache & cache, QObject * parent) :
    QAbstractItemModel(parent),
    m_data(),
    m_fakeRootItem(Q_NULLPTR),
    m_defaultNotebookItem(Q_NULLPTR),
    m_modelItemsByLocalUid(),
    m_modelItemsByStack(),
    m_stackItems(),
    m_cache(cache),
    m_lowerCaseNotebookNames(),
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
{
    delete m_fakeRootItem;
}

const NotebookModelItem * NotebookModel::itemForIndex(const QModelIndex & index) const
{
    if (!index.isValid()) {
        return m_fakeRootItem;
    }

    const NotebookModelItem * item = reinterpret_cast<const NotebookModelItem*>(index.internalPointer());
    if (item) {
        return item;
    }

    return m_fakeRootItem;
}

QModelIndex NotebookModel::indexForItem(const NotebookModelItem * item) const
{
    if (!item) {
        return QModelIndex();
    }

    if (item == m_fakeRootItem) {
        return QModelIndex();
    }

    const NotebookModelItem * parentItem = item->parent();
    if (!parentItem) {
        return QModelIndex();
    }

    int row = parentItem->rowForChild(item);
    if (Q_UNLIKELY(row < 0)) {
        QNWARNING("Internal error: can't get the row of the child item in parent in NotebookModel, child item: "
                  << *item << "\nParent item: " << *parentItem);
        return QModelIndex();
    }

    // NOTE: as long as we stick to using the model index's internal pointer only inside the model, it's fine
    return createIndex(row, Columns::Name, const_cast<NotebookModelItem*>(item));
}

QModelIndex NotebookModel::indexForLocalUid(const QString & localUid) const
{
    auto it = m_modelItemsByLocalUid.find(localUid);
    if (it == m_modelItemsByLocalUid.end()) {
        return QModelIndex();
    }

    const NotebookModelItem * item = &(it.value());
    return indexForItem(item);
}

QModelIndex NotebookModel::moveToStack(const QModelIndex & index, const QString & stack)
{
    QNDEBUG("NotebookModel::moveToStack: stack = " << stack);

    if (Q_UNLIKELY(stack.isEmpty())) {
        return removeFromStack(index);
    }

    const NotebookModelItem * item = reinterpret_cast<const NotebookModelItem*>(index.internalPointer());
    if (Q_UNLIKELY(!item)) {
        QNWARNING("Detected attempt to move notebook item from stack but the model index has no internal pointer "
                  "to the notebook model item");
        return index;
    }

    if (item->type() != NotebookModelItem::Type::Notebook) {
        QNDEBUG("Can't move the non-notebook model item to stack");
        return index;
    }

    const NotebookItem * notebookItem = item->notebookItem();
    if (Q_UNLIKELY(!notebookItem)) {
        QNWARNING("Found notebook model item of notebook type but its pointer to the notebook item is null");
        return index;
    }

    if (notebookItem->stack() == stack) {
        QNDEBUG("The stack of the item hasn't changed, nothing to do");
        return index;
    }

    removeModelItemFromParent(*item);

    NotebookDataByLocalUid & localUidIndex = m_data.get<ByLocalUid>();
    auto notebookItemIt = localUidIndex.find(notebookItem->localUid());
    if (Q_UNLIKELY(notebookItemIt == localUidIndex.end()))
    {
        QNWARNING("Can't find the notebook item being moved to stack " << stack << " by local uid; might be the stale pointer "
                  "to removed notebook item, cleaning it up");
        auto it = m_modelItemsByLocalUid.find(notebookItem->localUid());
        if (it != m_modelItemsByLocalUid.end()) {
            Q_UNUSED(m_modelItemsByLocalUid.erase(it))
        }
        return QModelIndex();
    }

    auto it = m_modelItemsByStack.end();
    auto stackItemIt = m_stackItems.find(stack);
    if (stackItemIt == m_stackItems.end()) {
        stackItemIt = m_stackItems.insert(stack, NotebookStackItem(stack));
        it = addNewStackModelItem(stackItemIt.value());
    }

    if (it == m_modelItemsByStack.end())
    {
        it = m_modelItemsByStack.find(stack);
        if (it == m_modelItemsByStack.end()) {
            QNWARNING("Internal error: no notebook model item while it's expected to be here; "
                      "will try to auto-fix it and add the new model item");
            it = addNewStackModelItem(stackItemIt.value());
        }
    }

    if (it == m_modelItemsByStack.end()) {
        QNCRITICAL("Internal error: no notebook model item while it's expected to be here; failed to auto-fix the problem");
        return index;
    }

    NotebookItem notebookItemCopy(*notebookItem);
    notebookItemCopy.setStack(stack);
    localUidIndex.replace(notebookItemIt, notebookItemCopy);

    const NotebookModelItem * newParentItem = &(*it);
    item->setParent(newParentItem);
    updateItemRowWithRespectToSorting(*item);

    return indexForItem(item);
}

QModelIndex NotebookModel::removeFromStack(const QModelIndex & index)
{
    QNDEBUG("NotebookModel::removeFromStack");

    const NotebookModelItem * item = reinterpret_cast<const NotebookModelItem*>(index.internalPointer());
    if (Q_UNLIKELY(!item)) {
        QNWARNING("Detected attempt to remove the notebook item from stack but the model index has no internal pointer "
                  "to the notebook model item");
        return index;
    }

    if (item->type() != NotebookModelItem::Type::Notebook) {
        QNDEBUG("Can't remove the non-notebook model item from stack");
        return index;
    }

    const NotebookItem * notebookItem = item->notebookItem();
    if (Q_UNLIKELY(!notebookItem)) {
        QNWARNING("Found notebook model item of notebook type but its pointer to the notebook item is null");
        return index;
    }

    if (notebookItem->stack().isEmpty())
    {
        QNWARNING("The notebook doesn't appear to have the stack but will continue the attempt to remove it from the stack anyway");
    }
    else
    {
        NotebookDataByLocalUid & localUidIndex = m_data.get<ByLocalUid>();
        auto it = localUidIndex.find(notebookItem->localUid());
        if (Q_UNLIKELY(it == localUidIndex.end())) {
            QNWARNING("Can't find the notebook item to be removed from the stack by the local uid: " << notebookItem->localUid());
            return index;
        }

        NotebookItem notebookItemCopy(*notebookItem);
        notebookItemCopy.setStack(QString());
        localUidIndex.replace(it, notebookItemCopy);
    }

    if (!m_fakeRootItem) {
        m_fakeRootItem = new NotebookModelItem;
    }

    const NotebookModelItem * parentItem = item->parent();
    if (!parentItem) {
        QNDEBUG("Notebook item doesn't have the parent, will set it to fake root item");
        item->setParent(m_fakeRootItem);
        updateItemRowWithRespectToSorting(*item);
        return indexForItem(item);
    }

    if (parentItem == m_fakeRootItem) {
        QNDEBUG("The notebook item doesn't belong to any stack, nothing to do");
        return index;
    }

    removeModelItemFromParent(*item);
    item->setParent(m_fakeRootItem);
    updateItemRowWithRespectToSorting(*item);
    return indexForItem(item);
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

            QList<const NotebookModelItem*> children = item->children();
            for(auto it = children.begin(), end = children.end(); it != end; ++it)
            {
                const NotebookModelItem * childItem = *it;
                if (Q_UNLIKELY(!childItem)) {
                    QNWARNING("Detected null pointer to notebook model item within the children of another notebook model item");
                    continue;
                }

                if (Q_UNLIKELY(childItem->type() == NotebookModelItem::Type::Stack)) {
                    QNWARNING("Detected nested notebook stack items which is unexpected and most probably incorrect");
                    continue;
                }

                const NotebookItem * notebookItem = childItem->notebookItem();
                if (Q_UNLIKELY(!notebookItem)) {
                    QNWARNING("Detected null pointer to notebook item in notebook model item having a type of notebook (not stack)");
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
    if (!index.isValid()) {
        return QVariant();
    }

    int columnIndex = index.column();
    if ((columnIndex < 0) || (columnIndex >= NUM_NOTEBOOK_MODEL_COLUMNS)) {
        return QVariant();
    }

    const NotebookModelItem * item = itemForIndex(index);
    if (!item) {
        return QVariant();
    }

    if (item == m_fakeRootItem) {
        return QVariant();
    }

    Columns::type column;
    switch(columnIndex)
    {
    case Columns::Name:
        column = Columns::Name;
        break;
    case Columns::Synchronizable:
        column = Columns::Synchronizable;
        break;
    case Columns::Dirty:
        column = Columns::Dirty;
        break;
    case Columns::Default:
        column = Columns::Default;
        break;
    case Columns::Published:
        column = Columns::Published;
        break;
    case Columns::FromLinkedNotebook:
        column = Columns::FromLinkedNotebook;
        break;
    default:
        return QVariant();
    }

    switch(role)
    {
    case Qt::DisplayRole:
    case Qt::EditRole:
    case Qt::ToolTipRole:
        return dataImpl(*item, column);
    case Qt::AccessibleTextRole:
    case Qt::AccessibleDescriptionRole:
        return dataAccessibleText(*item, column);
    default:
        return QVariant();
    }
}

QVariant NotebookModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole) {
        return QVariant();
    }

    if (orientation != Qt::Horizontal) {
        return QVariant();
    }

    switch(section)
    {
    case Columns::Name:
        return QVariant(tr("Name"));
    case Columns::Synchronizable:
        return QVariant(tr("Synchronizable"));
    case Columns::Dirty:
        return QVariant(tr("Dirty"));
    case Columns::Default:
        return QVariant(tr("Default"));
    case Columns::Published:
        return QVariant(tr("Published"));
    case Columns::FromLinkedNotebook:
        return QVariant(tr("From linked notebook"));
    default:
        return QVariant();
    }
}

int NotebookModel::rowCount(const QModelIndex & parent) const
{
    if (parent.isValid() && (parent.column() != Columns::Name)) {
        return 0;
    }

    const NotebookModelItem * parentItem = itemForIndex(parent);
    return (parentItem ? parentItem->numChildren() : 0);
}

int NotebookModel::columnCount(const QModelIndex & parent) const
{
    if (parent.isValid() && (parent.column() != Columns::Name)) {
        return 0;
    }

    return NUM_NOTEBOOK_MODEL_COLUMNS;
}

QModelIndex NotebookModel::index(int row, int column, const QModelIndex & parent) const
{
    if (!m_fakeRootItem || (row < 0) || (column < 0) || (column >= NUM_NOTEBOOK_MODEL_COLUMNS) ||
        (parent.isValid() && (parent.column() != Columns::Name)))
    {
        return QModelIndex();
    }

    const NotebookModelItem * parentItem = itemForIndex(parent);
    if (!parentItem) {
        return QModelIndex();
    }

    const NotebookModelItem * item = parentItem->childAtRow(row);
    if (!item) {
        return QModelIndex();
    }

    // NOTE: as long as we stick to using the model index's internal pointer only inside the model, it's fine
    return createIndex(row, column, const_cast<NotebookModelItem*>(item));
}

QModelIndex NotebookModel::parent(const QModelIndex & index) const
{
    if (!index.isValid()) {
        return QModelIndex();
    }

    const NotebookModelItem * childItem = itemForIndex(index);
    if (!childItem) {
        return QModelIndex();
    }

    const NotebookModelItem * parentItem = childItem->parent();
    if (!parentItem) {
        return QModelIndex();
    }

    if (parentItem == m_fakeRootItem) {
        return QModelIndex();
    }

    const NotebookModelItem * grandParentItem = parentItem->parent();
    if (!grandParentItem) {
        return QModelIndex();
    }

    int row = grandParentItem->rowForChild(parentItem);
    if (Q_UNLIKELY(row < 0)) {
        QNWARNING("Internal inconsistency detected in NotebookModel: parent of the item can't find the item within its children: item = "
                  << *parentItem << "\nParent item: " << *grandParentItem);
        return QModelIndex();
    }

    // NOTE: as long as we stick to using the model index's internal pointer only inside the model, it's fine
    return createIndex(row, Columns::Name, const_cast<NotebookModelItem*>(parentItem));
}

bool NotebookModel::setHeaderData(int section, Qt::Orientation orientation, const QVariant & value, int role)
{
    Q_UNUSED(section)
    Q_UNUSED(orientation)
    Q_UNUSED(value)
    Q_UNUSED(role)
    return false;
}

bool NotebookModel::setData(const QModelIndex & modelIndex, const QVariant & value, int role)
{
    if (role != Qt::EditRole) {
        return false;
    }

    if (!modelIndex.isValid()) {
        return false;
    }

    if (modelIndex.column() == Columns::Dirty) {
        QNWARNING("The \"dirty\" flag can't be set manually in NotebookModel");
        return false;
    }

    if (modelIndex.column() == Columns::Published) {
        QNWARNING("The \"published\" flag can't be set manually in NotebookModel");
        return false;
    }

    if (modelIndex.column() == Columns::FromLinkedNotebook) {
        QNWARNING("The \"from linked notebook\" flag can't be set manually in NotebookModel");
        return false;
    }

    const NotebookModelItem * item = itemForIndex(modelIndex);
    if (!item) {
        return false;
    }

    if (item == m_fakeRootItem) {
        return false;
    }

    bool isNotebookItem = (item->type() == NotebookModelItem::Type::Notebook);

    if (Q_UNLIKELY(isNotebookItem && !item->notebookItem())) {
        QNWARNING("Internal inconsistency detected in NotebookModel: model item of notebook type "
                  "has a null pointer to notebook item");
        return false;
    }

    if (Q_UNLIKELY(!isNotebookItem && !item->notebookStackItem())) {
        QNWARNING("Internal inconsistency detected in NotebookModel: model item of stack type "
                  "has a null pointer to stack item");
        return false;
    }

    if (isNotebookItem)
    {
        const NotebookItem * notebookItem = item->notebookItem();

        if (!canUpdateNotebookItem(*notebookItem)) {
            QNLocalizedString error = QT_TR_NOOP("can't update notebook");
            error += " \"";
            error += notebookItem->name();
            error += "\", ";
            error += QT_TR_NOOP("notebook restrictions apply");
            QNINFO(error << ", notebookItem = " << *notebookItem);
            emit notifyError(error);
            return false;
        }

        NotebookDataByLocalUid & localUidIndex = m_data.get<ByLocalUid>();
        auto notebookItemIt = localUidIndex.find(notebookItem->localUid());
        if (Q_UNLIKELY(notebookItemIt == localUidIndex.end())) {
            QNLocalizedString error = QT_TR_NOOP("can't update notebook: can't find the notebook being updated in the model");
            QNWARNING(error << ", notebook item: " << *notebookItem);
            emit notifyError(error);
            return false;
        }

        NotebookItem notebookItemCopy = *notebookItem;
        bool dirty = notebookItemCopy.isDirty();

        switch(modelIndex.column())
        {
        case Columns::Name:
            {
                QString newName = value.toString().trimmed();
                bool changed = (newName != notebookItemCopy.name());
                if (!changed) {
                    return true;
                }

                auto nameIt = m_lowerCaseNotebookNames.find(newName.toLower());
                if (nameIt != m_lowerCaseNotebookNames.end()) {
                    QNLocalizedString error = QT_TR_NOOP("can't rename notebook: no two notebooks within the account are allowed "
                                                         "to have the same name in a case-insensitive manner");
                    QNINFO(error << ", suggested new name = " << newName);
                    emit notifyError(error);
                    return false;
                }

                QNLocalizedString errorDescription;
                if (!Notebook::validateName(newName, &errorDescription)) {
                    QNLocalizedString error = QT_TR_NOOP("an't rename notebook");
                    error += ": ";
                    error += errorDescription;
                    QNINFO(error << "; suggested new name = " << newName);
                    emit notifyError(error);
                    return false;
                }

                dirty = true;
                notebookItemCopy.setName(newName);
                break;
            }
        case Columns::Synchronizable:
            {
                if (notebookItemCopy.isSynchronizable() && !value.toBool()) {
                    QNLocalizedString error = QT_TR_NOOP("can't make already synchronizable notebook not synchronizable");
                    QNINFO(error << ", already synchronizable notebook item: " << notebookItemCopy);
                    emit notifyError(error);
                    return false;
                }

                dirty |= (value.toBool() != notebookItemCopy.isSynchronizable());
                notebookItemCopy.setSynchronizable(value.toBool());
                break;
            }
        case Columns::Default:
            {
                if (notebookItemCopy.isDefault() == value.toBool()) {
                    QNDEBUG("The default state of the notebook hasn't changed, nothing to do");
                    return true;
                }

                if (notebookItemCopy.isDefault() && !value.toBool()) {
                    QNLocalizedString error = QT_TR_NOOP("in order to stop notebook being the default one please choose another default notebook");
                    QNDEBUG(error);
                    emit notifyError(error);
                    return false;
                }

                notebookItemCopy.setDefault(true);
                dirty = true;

                if (m_defaultNotebookItem)
                {
                    auto previousDefaultItemIt = localUidIndex.find(m_defaultNotebookItem->localUid());
                    if (previousDefaultItemIt != localUidIndex.end())
                    {
                        NotebookItem previousDefaultItemCopy(*previousDefaultItemIt);
                        previousDefaultItemCopy.setDefault(false);
                        localUidIndex.replace(previousDefaultItemIt, previousDefaultItemCopy);
                        updateNotebookInLocalStorage(previousDefaultItemCopy);
                    }
                }

                m_defaultNotebookItem = &(*notebookItemIt);
                break;
            }
        default:
            return false;
        }

        notebookItemCopy.setDirty(dirty);
        localUidIndex.replace(notebookItemIt, notebookItemCopy);
        updateNotebookInLocalStorage(notebookItemCopy);
    }
    else
    {
        if (modelIndex.column() != Columns::Name) {
            QNWARNING("Detected attempt to change something rather than name for the notebook stack item, ignoring it");
            return false;
        }

        const NotebookStackItem * notebookStackItem = item->notebookStackItem();
        QString newStack = value.toString();
        QString previousStack = notebookStackItem->name();
        if (newStack == previousStack) {
            QNDEBUG("Notebook stack hasn't changed, nothing to do");
            return true;
        }

#define CHECK_ITEM(childItem) \
            if (Q_UNLIKELY(!childItem)) { \
                QNWARNING("Detected null pointer to notebook model item within the children of another item: item = " << *item); \
                continue; \
            } \
            \
            if (Q_UNLIKELY(childItem->type() == NotebookModelItem::Type::Stack)) { \
                QNWARNING("Internal inconsistency detected: found notebook stack item being a child of another notebook stack item: " \
                          "item = " << *item); \
                continue; \
            } \
            \
            const NotebookItem * notebookItem = childItem->notebookItem(); \
            if (Q_UNLIKELY(!notebookItem)) { \
                QNWARNING("Detected null pointer to notebook item within the children of notebook stack item: item = " << *item); \
                continue; \
            }

        QList<const NotebookModelItem*> children = item->children();
        for(auto it = children.begin(), end = children.end(); it != end; ++it)
        {
            const NotebookModelItem * childItem = *it;
            CHECK_ITEM(childItem)

            if (!canUpdateNotebookItem(*notebookItem)) {
                QNLocalizedString error = QT_TR_NOOP("can't update notebook stack: restrictions on at least one of stacked notebooks' update apply");
                QNINFO(error << ", notebook item for which the restrictions apply: " << *notebookItem);
                emit notifyError(error);
                return false;
            }
        }

        // Change the stack item
        auto stackModelItemIt = m_modelItemsByStack.find(previousStack);
        if (stackModelItemIt == m_modelItemsByStack.end()) {
            QNLocalizedString error = QT_TR_NOOP("can't update notebook stack item: can't find the item for the previous stack value");
            QNWARNING(error << ", previous stack: \"" << previousStack << "\", new stack: \"" << newStack << "\"");
            emit notifyError(error);
            return false;
        }

        NotebookModelItem stackModelItemCopy(stackModelItemIt.value());
        if (Q_UNLIKELY(stackModelItemCopy.type() != NotebookModelItem::Type::Stack)) {
            QNWARNING("Internal inconsistency detected: non-stack item is kept within the hash of model items supposed to be stack items: "
                      "item: " << stackModelItemCopy);
            return false;
        }

        const NotebookStackItem * stackItem = stackModelItemCopy.notebookStackItem();
        if (Q_UNLIKELY(!stackItem)) {
            QNWARNING("Detected null pointer to notebook stack item in notebook model item wrapping it; item: " << stackModelItemCopy);
            return false;
        }

        NotebookStackItem newStackItem(*stackItem);
        newStackItem.setName(newStack);

        auto stackItemIt = m_stackItems.find(previousStack);
        if (stackItemIt != m_stackItems.end()) {
            Q_UNUSED(m_stackItems.erase(stackItemIt))
        }

        stackItemIt = m_stackItems.insert(newStack, newStackItem);
        stackModelItemCopy.setNotebookStackItem(&(*stackItemIt));

        Q_UNUSED(m_modelItemsByStack.erase(stackModelItemIt));
        stackModelItemIt = m_modelItemsByStack.insert(newStack, stackModelItemCopy);

        // Change all the child items
        NotebookDataByLocalUid & localUidIndex = m_data.get<ByLocalUid>();

        // Refresh the list of children; shouldn't be necessary but just n case
        children = stackModelItemIt->children();

        for(auto it = children.begin(), end = children.end(); it != end; ++it)
        {
            const NotebookModelItem * childItem = *it;
            CHECK_ITEM(childItem)

            auto notebookItemIt = localUidIndex.find(notebookItem->localUid());
            if (notebookItemIt == localUidIndex.end()) {
                QNWARNING("Internal inconsistency detected: can't find the iterator for the notebook item for which the stack it being changed: "
                          "non-found notebook item: " << *notebookItem);
                continue;
            }

            NotebookItem notebookItemCopy(*notebookItem);
            notebookItemCopy.setStack(newStack);
            localUidIndex.replace(notebookItemIt, notebookItemCopy);
            updateNotebookInLocalStorage(notebookItemCopy);
        }

#undef CHECK_ITEM

    }

    return true;
}

bool NotebookModel::insertRows(int row, int count, const QModelIndex & parent)
{
    QNDEBUG("NotebookModel::insertRows: row = " << row << ", count = " << count
            << ", parent: is valid = " << (parent.isValid() ? "true" : "false")
            << ", row = " << parent.row() << ", column = " << parent.column());

    if (!m_fakeRootItem) {
        m_fakeRootItem = new NotebookModelItem;
    }

    const NotebookModelItem * parentItem = (parent.isValid()
                                            ? itemForIndex(parent)
                                            : m_fakeRootItem);
    if (!parentItem) {
        QNDEBUG("No model item for given model index");
        return false;
    }

    if (parentItem->type() == NotebookModelItem::Type::Notebook) {
        QNDEBUG("Can't insert notebook under another notebook, only under the notebook stack");
        return false;
    }

    const NotebookStackItem * stackItem = parentItem->notebookStackItem();
    if (Q_UNLIKELY(!stackItem)) {
        QNDEBUG("Detected null pointer to notebook stack item within the notebook model item of stack type: model item = "
                << *parentItem);
        return false;
    }

    NotebookDataByLocalUid & localUidIndex = m_data.get<ByLocalUid>();

    std::vector<NotebookDataByLocalUid::iterator> addedItems;
    addedItems.reserve(static_cast<size_t>(std::max(count, 0)));

    beginInsertRows(parent, row, row + count - 1);
    for(int i = 0; i < count; ++i)
    {
        // Adding notebook item
        NotebookItem item;
        item.setLocalUid(UidGenerator::Generate());
        Q_UNUSED(m_notebookItemsNotYetInLocalStorageUids.insert(item.localUid()))

        item.setName(nameForNewNotebook());
        item.setDirty(true);
        item.setStack(stackItem->name());

        auto insertionResult = localUidIndex.insert(item);
        addedItems.push_back(insertionResult.first);

        // Adding wrapping notebook model item
        NotebookModelItem modelItem(NotebookModelItem::Type::Notebook, &(*(addedItems.back())));
        auto modelItemInsertionResult = m_modelItemsByLocalUid.insert(item.localUid(), modelItem);
        modelItemInsertionResult.value().setParent(parentItem);
    }
    endInsertRows();

    for(auto it = addedItems.begin(), end = addedItems.end(); it != end; ++it) {
        const NotebookItem & item = *(*it);
        updateNotebookInLocalStorage(item);
    }

    if (m_sortedColumn != Columns::Name) {
        QNDEBUG("Not sorting by name, returning");
        return true;
    }

    emit layoutAboutToBeChanged();
    for(auto it = m_modelItemsByLocalUid.begin(), end = m_modelItemsByLocalUid.end(); it != end; ++it) {
        updateItemRowWithRespectToSorting(*it);
    }
    emit layoutChanged();

    return true;
}

bool NotebookModel::removeRows(int row, int count, const QModelIndex & parent)
{
    if (!m_fakeRootItem) {
        return false;
    }

    const NotebookModelItem * parentItem = (parent.isValid()
                                            ? itemForIndex(parent)
                                            : m_fakeRootItem);
    if (!parentItem) {
        return false;
    }

    if (Q_UNLIKELY((parentItem != m_fakeRootItem) && (parentItem->type() != NotebookModelItem::Type::Stack))) {
        QNDEBUG("Can't remove row(s) from parent item not being a stack item: " << *parentItem);
        return false;
    }

    QList<const NotebookItem*> notebookItemsToRemove;
    QList<const NotebookStackItem*> notebookStackItemsToRemove;

    // First simply collect all the items to be removed while checking for each of them whether they can be safely removed
    for(int i = 0; i < count; ++i)
    {
        const NotebookModelItem * childItem = parentItem->childAtRow(row + i);
        if (!childItem) {
            QNWARNING("Detected null pointer to child notebook model item on attempt to remove row "
                      << row + i << " from parent item: " << *parentItem);
            continue;
        }

        bool isNotebookItem = (childItem->type() == NotebookModelItem::Type::Notebook);
        if (Q_UNLIKELY(isNotebookItem && !childItem->notebookItem())) {
            QNWARNING("Detected null pointer to notebook item in notebook model item being removed: row in parent = "
                      << row + i << ", parent item: " << *parentItem << "\nChild item: " << *childItem);
            continue;
        }

        if (Q_UNLIKELY(!isNotebookItem && !childItem->notebookStackItem())) {
            QNWARNING("Detected null pointer to notebook stack item in notebook model item being removed: row in parent = "
                      << row + i << ", parent item: " << *parentItem << "\nChild item: " << *childItem);
            continue;
        }

        if (isNotebookItem)
        {
            const NotebookItem * notebookItem = childItem->notebookItem();

#define CHECK_NOTEBOOK_ITEM(notebookItem) \
            if (notebookItem->isSynchronizable()) { \
                QNLocalizedString error = QT_TR_NOOP("one of notebooks being removed along with the stack containing it " \
                                                     "is synchronizable, it can't be removed"); \
                QNINFO(error << ", notebook: " << *notebookItem); \
                emit notifyError(error); \
                return false; \
            } \
            \
            if (notebookItem->isLinkedNotebook()) { \
                QNLocalizedString error = QT_TR_NOOP("one of notebooks being removed along with the stack containing it " \
                                                     "is the linked notebook from another account, it can't be removed"); \
                QNINFO(error << ", notebook: " << *notebookItem); \
                emit notifyError(error); \
                return false; \
            }

            CHECK_NOTEBOOK_ITEM(notebookItem)
            notebookItemsToRemove.push_back(notebookItem);
        }
        else
        {
            QList<const NotebookModelItem*> notebookModelItemsWithinStack = childItem->children();
            for(int j = 0, size = notebookModelItemsWithinStack.size(); j < size; ++j)
            {
                const NotebookModelItem * notebookModelItem = notebookModelItemsWithinStack[j];
                if (Q_UNLIKELY(!notebookModelItem)) {
                    QNWARNING("Detected null pointer to notebook model item within the children of the stack item being removed: " << *childItem);
                    continue;
                }

                if (Q_UNLIKELY(notebookModelItem->type() != NotebookModelItem::Type::Notebook)) {
                    QNWARNING("Detected notebook model item within the stack item which is not a notebook by type; stack item: " << *childItem
                              << "\nIts child with wrong type: " << *notebookModelItem);
                    continue;
                }

                const NotebookItem * notebookItem = notebookModelItem->notebookItem();
                if (Q_UNLIKELY(!notebookItem)) {
                    QNWARNING("Detected null pointer to notebook item in notebook model item being one of those removed "
                              "along with the stack item containing them; stack item: " << *childItem);
                    continue;
                }

                CHECK_NOTEBOOK_ITEM(notebookItem)
                notebookItemsToRemove.push_back(notebookItem);
                Q_UNUSED(childItem->takeChild(j))
            }

            notebookStackItemsToRemove.push_back(childItem->notebookStackItem());
        }
    }

    // Now we are sure all the items collected for the deletion can actually be deleted

    beginRemoveRows(parent, row, row + count - 1);

    NotebookDataByLocalUid & localUidIndex = m_data.get<ByLocalUid>();

    for(int i = 0, size = notebookItemsToRemove.size(); i < size; ++i)
    {
        const NotebookItem * notebookItem = notebookItemsToRemove[i];
        QString localUid = notebookItem->localUid();
        auto it = localUidIndex.find(localUid);
        if (Q_LIKELY(it != localUidIndex.end())) {
            Q_UNUSED(localUidIndex.erase(it))
        }
        else {
            QNWARNING("Internal error detected: can't find the notebook item to remove from the NotebookModel: local uid = "
                      << localUid);
        }

        auto modelItemIt = m_modelItemsByLocalUid.find(localUid);
        if (Q_LIKELY(modelItemIt != m_modelItemsByLocalUid.end()))
        {
            const NotebookModelItem & modelItem = *modelItemIt;
            removeModelItemFromParent(modelItem);

            Q_UNUSED(m_modelItemsByLocalUid.erase(modelItemIt))
        }
        else
        {
            QNWARNING("Internal error detected: can't find the notebook model item corresponding to the notebook item being removed: "
                      "local uid = " << localUid);
        }
    }

    for(int i = 0, size = notebookStackItemsToRemove.size(); i < size; ++i)
    {
        const NotebookStackItem * stackItem = notebookStackItemsToRemove[i];
        QString stack = stackItem->name();
        auto stackItemIt = m_stackItems.find(stack);
        if (Q_LIKELY(stackItemIt != m_stackItems.end())) {
            Q_UNUSED(m_stackItems.erase(stackItemIt))
        }
        else {
            QNWARNING("Internal error detected: can't find the notebook stack item being removed from the notebook model: stack = "
                      << stack);
        }

        auto stackModelItemIt = m_modelItemsByStack.find(stack);
        if (Q_LIKELY(stackModelItemIt != m_modelItemsByStack.end()))
        {
            const NotebookModelItem & stackModelItem = *stackModelItemIt;
            removeModelItemFromParent(stackModelItem);
            Q_UNUSED(m_modelItemsByStack.erase(stackModelItemIt))
        }
        else
        {
            QNWARNING("Internal error detected: can't find the notebook model item corresponding to the notebook stack item "
                      "being removed from the notebook model: stack = " << stack);
        }
    }

    endRemoveRows();

    return true;
}

void NotebookModel::sort(int column, Qt::SortOrder order)
{
    QNDEBUG("NotebookModel::sort: column = " << column << ", order = " << order
            << " (" << (order == Qt::AscendingOrder ? "ascending" : "descending") << ")");

    if (column != Columns::Name) {
        // Sorting by other columns is not yet implemented
        return;
    }

    if (order == m_sortOrder) {
        QNDEBUG("The sort order already established, nothing to do");
        return;
    }

    m_sortOrder = order;

    emit layoutAboutToBeChanged();

    for(auto it = m_modelItemsByLocalUid.begin(), end = m_modelItemsByLocalUid.end(); it != end; ++it) {
        updateItemRowWithRespectToSorting(*it);
    }

    for(auto it = m_modelItemsByStack.begin(), end = m_modelItemsByStack.end(); it != end; ++it) {
        updateItemRowWithRespectToSorting(*it);
    }

    updatePersistentModelIndices();
    emit layoutChanged();
}

QStringList NotebookModel::mimeTypes() const
{
    QStringList list;
    list << NOTEBOOK_MODEL_MIME_TYPE;
    return list;
}

QMimeData * NotebookModel::mimeData(const QModelIndexList & indexes) const
{
    if (indexes.count() != 1) {
        return Q_NULLPTR;
    }

    const NotebookModelItem * item = itemForIndex(indexes.at(0));
    if (!item) {
        return Q_NULLPTR;
    }

    QByteArray encodedItem;
    QDataStream out(&encodedItem, QIODevice::WriteOnly);
    out << *item;

    QMimeData * mimeData = new QMimeData;
    mimeData->setData(NOTEBOOK_MODEL_MIME_TYPE, qCompress(encodedItem, NOTEBOOK_MODEL_MIME_DATA_MAX_COMPRESSION));
    return mimeData;
}

bool NotebookModel::dropMimeData(const QMimeData * mimeData, Qt::DropAction action,
                                 int row, int column, const QModelIndex & parentIndex)
{
    QNDEBUG("NotebookModel::dropMimeData: action = " << action << ", row = " << row
            << ", column = " << column << ", parent index: is valid = " << (parentIndex.isValid() ? "true" : "false")
            << ", parent row = " << parentIndex.row() << ", parent column = " << (parentIndex.column())
            << ", mime data formats: " << (mimeData ? mimeData->formats().join("; ") : QString("<null>")));

    if (action == Qt::IgnoreAction) {
        return true;
    }

    if (action != Qt::MoveAction) {
        return false;
    }

    if (!mimeData || !mimeData->hasFormat(NOTEBOOK_MODEL_MIME_TYPE)) {
        return false;
    }

    const NotebookModelItem * parentItem = itemForIndex(parentIndex);
    if (!parentItem) {
        return false;
    }

    if ((parentItem != m_fakeRootItem) && (parentItem->type() != NotebookModelItem::Type::Stack)) {
        return false;
    }

    QByteArray data = qUncompress(mimeData->data(NOTEBOOK_MODEL_MIME_TYPE));
    NotebookModelItem item;
    QDataStream in(&data, QIODevice::ReadOnly);
    in >> item;

    if (item.type() != NotebookModelItem::Type::Notebook) {
        return false;
    }

    auto it = m_modelItemsByLocalUid.find(item.notebookItem()->localUid());
    if (it == m_modelItemsByLocalUid.end()) {
        return false;
    }

    const NotebookModelItem * originalParentItem = it->parent();
    int originalRow = -1;
    if (originalParentItem) {
        // Need to manually remove the item from its original parent
        originalRow = originalParentItem->rowForChild(&(*it));
    }

    if (originalRow >= 0) {
        QModelIndex originalParentIndex = indexForItem(originalParentItem);
        beginRemoveRows(originalParentIndex, originalRow, originalRow);
        Q_UNUSED(originalParentItem->takeChild(originalRow))
    }

    Q_UNUSED(m_modelItemsByLocalUid.erase(it))

    if (originalRow >= 0) {
        endRemoveRows();
    }

    beginInsertRows(parentIndex, row, row);
    it = m_modelItemsByLocalUid.insert(item.notebookItem()->localUid(), item);
    parentItem->insertChild(row, &(*it));
    endInsertRows();

    emit layoutAboutToBeChanged();
    updateItemRowWithRespectToSorting(*it);
    emit layoutChanged();

    return true;
}

void NotebookModel::onAddNotebookComplete(Notebook notebook, QUuid requestId)
{
    QNDEBUG("NotebookModel::onAddNotebookComplete: notebook = " << notebook << "\nRequest id = " << requestId);

    auto it = m_addNotebookRequestIds.find(requestId);
    if (it != m_addNotebookRequestIds.end()) {
        Q_UNUSED(m_addNotebookRequestIds.erase(it))
        return;
    }

    onNotebookAddedOrUpdated(notebook);
}

void NotebookModel::onAddNotebookFailed(Notebook notebook, QNLocalizedString errorDescription, QUuid requestId)
{
    auto it = m_addNotebookRequestIds.find(requestId);
    if (it == m_addNotebookRequestIds.end()) {
        return;
    }

    QNDEBUG("NotebookModel::onAddNotebookFailed: notebook = " << notebook << "\nError description = "
            << errorDescription << ", request id = " << requestId);

    Q_UNUSED(m_addNotebookRequestIds.erase(it))

    emit notifyError(errorDescription);

    removeItemByLocalUid(notebook.localUid());
}

void NotebookModel::onUpdateNotebookComplete(Notebook notebook, QUuid requestId)
{
    QNDEBUG("NotebookModel::onUpdateNotebookComplete: notebook = " << notebook << "\nRequest id = " << requestId);

    auto it = m_updateNotebookRequestIds.find(requestId);
    if (it != m_updateNotebookRequestIds.end()) {
        Q_UNUSED(m_updateNotebookRequestIds.erase(it))
        return;
    }

    onNotebookAddedOrUpdated(notebook);
}

void NotebookModel::onUpdateNotebookFailed(Notebook notebook, QNLocalizedString errorDescription, QUuid requestId)
{
    auto it = m_updateNotebookRequestIds.find(requestId);
    if (it == m_updateNotebookRequestIds.end()) {
        return;
    }

    QNDEBUG("NotebookModel::onUpdateNotebookFailed: notebook = " << notebook << "\nError description = "
            << errorDescription << ", request id = " << requestId);

    Q_UNUSED(m_updateNotebookRequestIds.erase(it))

    requestId = QUuid::createUuid();
    Q_UNUSED(m_findNotebookToRestoreFailedUpdateRequestIds.insert(requestId))
    QNTRACE("Emitting the request to find the notebook: local uid = " << notebook.localUid()
            << ", request id = " << requestId);
    emit findNotebook(notebook, requestId);
}

void NotebookModel::onFindNotebookComplete(Notebook notebook, QUuid requestId)
{
    auto restoreUpdateIt = m_findNotebookToRestoreFailedUpdateRequestIds.find(requestId);
    auto performUpdateIt = m_findNotebookToPerformUpdateRequestIds.find(requestId);
    if ((restoreUpdateIt == m_findNotebookToRestoreFailedUpdateRequestIds.end()) &&
        (performUpdateIt == m_findNotebookToPerformUpdateRequestIds.end()))
    {
        return;
    }

    QNDEBUG("NotebookModel::onFindNotebookComplete: notebook = " << notebook << "\nRequest id = " << requestId);

    if (restoreUpdateIt != m_findNotebookToRestoreFailedUpdateRequestIds.end())
    {
        Q_UNUSED(m_findNotebookToRestoreFailedUpdateRequestIds.erase(restoreUpdateIt))
        onNotebookAddedOrUpdated(notebook);
    }
    else if (performUpdateIt != m_findNotebookToPerformUpdateRequestIds.end())
    {
        Q_UNUSED(m_findNotebookToPerformUpdateRequestIds.erase(performUpdateIt))
        m_cache.put(notebook.localUid(), notebook);
        NotebookDataByLocalUid & localUidIndex = m_data.get<ByLocalUid>();
        auto it = localUidIndex.find(notebook.localUid());
        if (it != localUidIndex.end()) {
            updateNotebookInLocalStorage(*it);
        }
    }
}

void NotebookModel::onFindNotebookFailed(Notebook notebook, QNLocalizedString errorDescription, QUuid requestId)
{
    auto restoreUpdateIt = m_findNotebookToRestoreFailedUpdateRequestIds.find(requestId);
    auto performUpdateIt = m_findNotebookToPerformUpdateRequestIds.find(requestId);
    if ((restoreUpdateIt == m_findNotebookToRestoreFailedUpdateRequestIds.end()) &&
        (performUpdateIt == m_findNotebookToPerformUpdateRequestIds.end()))
    {
        return;
    }

    QNDEBUG("NotebookModel::onFindNotebookFailed: notebook = " << notebook << "\nError description = "
            << errorDescription << ", request id = " << requestId);

    if (restoreUpdateIt != m_findNotebookToRestoreFailedUpdateRequestIds.end()) {
        Q_UNUSED(m_findNotebookToRestoreFailedUpdateRequestIds.erase(restoreUpdateIt))
    }
    else if (performUpdateIt != m_findNotebookToPerformUpdateRequestIds.end()) {
        Q_UNUSED(m_findNotebookToPerformUpdateRequestIds.erase(performUpdateIt))
    }

    emit notifyError(errorDescription);
}

void NotebookModel::onListNotebooksComplete(LocalStorageManager::ListObjectsOptions flag,
                                            size_t limit, size_t offset,
                                            LocalStorageManager::ListNotebooksOrder::type order,
                                            LocalStorageManager::OrderDirection::type orderDirection,
                                            QString linkedNotebookGuid, QList<Notebook> foundNotebooks, QUuid requestId)
{
    if (requestId != m_listNotebooksRequestId) {
        return;
    }

    QNDEBUG("NotebookModel::onListNotebooksComplete: flag = " << flag << ", limit = " << limit << ", offset = " << offset
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

void NotebookModel::onListNotebooksFailed(LocalStorageManager::ListObjectsOptions flag,
                                          size_t limit, size_t offset,
                                          LocalStorageManager::ListNotebooksOrder::type order,
                                          LocalStorageManager::OrderDirection::type orderDirection,
                                          QString linkedNotebookGuid, QNLocalizedString errorDescription, QUuid requestId)
{
    if (requestId != m_listNotebooksRequestId) {
        return;
    }

    QNDEBUG("NotebookModel::onListNotebooksFailed: flag = " << flag << ", limit = " << limit << ", offset = "
            << offset << ", order = " << order << ", direction = " << orderDirection << ", linked notebook guid = "
            << (linkedNotebookGuid.isNull() ? QString("<null>") : linkedNotebookGuid) << ", error description = "
            << errorDescription << ", request id = " << requestId);

    m_listNotebooksRequestId = QUuid();

    emit notifyError(errorDescription);
}

void NotebookModel::onExpungeNotebookComplete(Notebook notebook, QUuid requestId)
{
    QNDEBUG("NotebookModel::onExpungeNotebookComplete: notebook = " << notebook
            << "\nRequest id = " << requestId);

    auto it = m_expungeNotebookRequestIds.find(requestId);
    if (it != m_expungeNotebookRequestIds.end()) {
        Q_UNUSED(m_expungeNotebookRequestIds.erase(it))
        return;
    }

    removeItemByLocalUid(notebook.localUid());
}

void NotebookModel::onExpungeNotebookFailed(Notebook notebook, QNLocalizedString errorDescription, QUuid requestId)
{
    auto it = m_expungeNotebookRequestIds.find(requestId);
    if (it == m_expungeNotebookRequestIds.end()) {
        return;
    }

    QNDEBUG("NotebookModel::onExpungeNotebookFailed: notebook = " << notebook << "\nError description = "
            << errorDescription << ", request id = " << requestId);

    Q_UNUSED(m_expungeNotebookRequestIds.erase(it))

    onNotebookAddedOrUpdated(notebook);
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
    QObject::connect(&localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,addNotebookFailed,Notebook,QNLocalizedString,QUuid),
                     this, QNSLOT(NotebookModel,onAddNotebookFailed,Notebook,QNLocalizedString,QUuid));
    QObject::connect(&localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,updateNotebookComplete,Notebook,QUuid),
                     this, QNSLOT(NotebookModel,onUpdateNotebookComplete,Notebook,QUuid));
    QObject::connect(&localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,updateNotebookFailed,Notebook,QNLocalizedString,QUuid),
                     this, QNSLOT(NotebookModel,onUpdateNotebookFailed,Notebook,QNLocalizedString,QUuid));
    QObject::connect(&localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,findNotebookComplete,Notebook,QUuid),
                     this, QNSLOT(NotebookModel,onFindNotebookComplete,Notebook,QUuid));
    QObject::connect(&localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,findNotebookFailed,Notebook,QNLocalizedString,QUuid),
                     this, QNSLOT(NotebookModel,onFindNotebookFailed,Notebook,QNLocalizedString,QUuid));
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
                                                                QString,QNLocalizedString,QUuid),
                     this, QNSLOT(NotebookModel,onListNotebooksFailed,LocalStorageManager::ListObjectsOptions,
                                  size_t,size_t,LocalStorageManager::ListNotebooksOrder::type,
                                  LocalStorageManager::OrderDirection::type,QString,QNLocalizedString,QUuid));
    QObject::connect(&localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,expungeNotebookComplete,Notebook,QUuid),
                     this, QNSLOT(NotebookModel,onExpungeNotebookComplete,Notebook,QUuid));
    QObject::connect(&localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,expungeNotebookFailed,Notebook,QNLocalizedString,QUuid),
                     this, QNSLOT(NotebookModel,onExpungeNotebookFailed,Notebook,QNLocalizedString,QUuid));
}

void NotebookModel::requestNotebooksList()
{
    QNDEBUG("NotebookModel::requestNotebooksList: offset = " << m_listNotebooksOffset);

    LocalStorageManager::ListObjectsOptions flags = LocalStorageManager::ListAll;
    LocalStorageManager::ListNotebooksOrder::type order = LocalStorageManager::ListNotebooksOrder::NoOrder;
    LocalStorageManager::OrderDirection::type direction = LocalStorageManager::OrderDirection::Ascending;

    m_listNotebooksRequestId = QUuid::createUuid();
    QNTRACE("Emitting the request to list notebooks: offset = " << m_listNotebooksOffset << ", request id = " << m_listNotebooksRequestId);
    emit listNotebooks(flags, NOTEBOOK_LIST_LIMIT, m_listNotebooksOffset, order, direction, QString(), m_listNotebooksRequestId);
}

QVariant NotebookModel::dataImpl(const NotebookModelItem & item, const Columns::type column) const
{
    bool isNotebookItem = (item.type() == NotebookModelItem::Type::Notebook);
    if (Q_UNLIKELY((isNotebookItem && !item.notebookItem()) || (!isNotebookItem && !item.notebookStackItem()))) {
        QNWARNING("Detected null pointer to " << (isNotebookItem ? "notebook" : "stack") << " item inside the noteobok model item");
        return QVariant();
    }

    switch(column)
    {
    case Columns::Name:
        {
            if (isNotebookItem) {
                return QVariant(item.notebookItem()->name());
            }
            else {
                return QVariant(item.notebookStackItem()->name());
            }
        }
    case Columns::Synchronizable:
        {
            if (isNotebookItem) {
                return QVariant(item.notebookItem()->isSynchronizable());
            }
            else {
                return QVariant();
            }
        }
    case Columns::Dirty:
        {
            if (isNotebookItem) {
                return QVariant(item.notebookItem()->isDirty());
            }
            else {
                return QVariant();
            }
        }
    case Columns::Default:
        {
            if (isNotebookItem) {
                return QVariant(item.notebookItem()->isDefault());
            }
            else {
                return QVariant();
            }
        }
    case Columns::Published:
        {
            if (isNotebookItem) {
                return QVariant(item.notebookItem()->isPublished());
            }
            else {
                return QVariant();
            }
        }
    case Columns::FromLinkedNotebook:
        {
            if (isNotebookItem) {
                return QVariant(item.notebookItem()->isLinkedNotebook());
            }
            else {
                return QVariant();
            }
        }
    default:
        return QVariant();
    }
}

QVariant NotebookModel::dataAccessibleText(const NotebookModelItem & item, const Columns::type column) const
{
    bool isNotebookItem = (item.type() == NotebookModelItem::Type::Notebook);
    if (Q_UNLIKELY((isNotebookItem && !item.notebookItem()) || (!isNotebookItem && !item.notebookStackItem()))) {
        QNWARNING("Detected null pointer to " << (isNotebookItem ? "notebook" : "stack") << " item inside the noteobok model item");
        return QVariant();
    }

    QVariant textData = dataImpl(item, column);
    if (textData.isNull()) {
        return QVariant();
    }

    QString accessibleText = (isNotebookItem ? tr("Notebook: ") : tr("Notebook stack: "));

    switch(column)
    {
    case Columns::Name:
        accessibleText += tr("name is ") + textData.toString();
        break;
    case Columns::Synchronizable:
        {
            if (!isNotebookItem) {
                return QVariant();
            }

            accessibleText += (textData.toBool() ? tr("synchronizable") : tr("not synchronizable"));
            break;
        }
    case Columns::Dirty:
        {
            if (!isNotebookItem) {
                return QVariant();
            }

            accessibleText += (textData.toBool() ? tr("dirty") : tr("not dirty"));
            break;
        }
    case Columns::Default:
        {
            if (!isNotebookItem) {
                return QVariant();
            }

            accessibleText += (textData.toBool() ? tr("default") : tr("not default"));
            break;
        }
    case Columns::Published:
        {
            if (!isNotebookItem) {
                return QVariant();
            }

            accessibleText += (textData.toBool() ? tr("published") : tr("not published"));
            break;
        }
    case Columns::FromLinkedNotebook:
        {
            if (!isNotebookItem) {
                return QVariant();
            }

            accessibleText += (textData.toBool() ? tr("from linked notebook") : tr("from own account"));
            break;
        }
    default:
        return QVariant();
    }

    return QVariant(accessibleText);
}

bool NotebookModel::canUpdateNotebookItem(const NotebookItem & item) const
{
    return item.isUpdatable();
}

void NotebookModel::updateNotebookInLocalStorage(const NotebookItem & item)
{
    QNDEBUG("NotebookModel::updateNotebookInLocalStorage: local uid = " << item.localUid());

    Notebook notebook;

    auto notYetSavedItemIt = m_notebookItemsNotYetInLocalStorageUids.find(item.localUid());
    if (notYetSavedItemIt == m_notebookItemsNotYetInLocalStorageUids.end())
    {
        QNDEBUG("Updating the notebook");

        const Notebook * pCachedNotebook = m_cache.get(item.localUid());
        if (Q_UNLIKELY(!pCachedNotebook))
        {
            QUuid requestId = QUuid::createUuid();
            Q_UNUSED(m_findNotebookToPerformUpdateRequestIds.insert(requestId))
            Notebook dummy;
            dummy.setLocalUid(item.localUid());
            emit findNotebook(dummy, requestId);
            QNTRACE("Emitted the request to find the notebook: local uid = " << item.localUid()
                    << ", request id = " << requestId);
            return;
        }

        notebook = *pCachedNotebook;
    }

    notebook.setLocalUid(item.localUid());
    notebook.setGuid(item.guid());
    notebook.setLinkedNotebookGuid(item.isLinkedNotebook() ? item.guid() : QString());
    notebook.setName(item.name());
    notebook.setLocal(!item.isSynchronizable());
    notebook.setDirty(item.isDirty());
    notebook.setDefaultNotebook(item.isDefault());
    notebook.setPublished(item.isPublished());

    // NOTE: deliberately not setting the updatable property from the item as it can't be changed by the model
    // and only serves the utilitary purpose inside it

    QUuid requestId = QUuid::createUuid();

    if (notYetSavedItemIt != m_notebookItemsNotYetInLocalStorageUids.end())
    {
        Q_UNUSED(m_addNotebookRequestIds.insert(requestId))
        emit addNotebook(notebook, requestId);

        QNTRACE("Emitted the request to add the notebook to local storage: id = " << requestId
                << ", notebook = " << notebook);

        Q_UNUSED(m_notebookItemsNotYetInLocalStorageUids.erase(notYetSavedItemIt))
    }
    else
    {
        Q_UNUSED(m_updateNotebookRequestIds.insert(requestId))
        emit updateNotebook(notebook, requestId);

        QNTRACE("Emitted the request to update the notebook in the local storage: id = " << requestId
                << ", notebook = " << notebook);
    }
}

QString NotebookModel::nameForNewNotebook() const
{
    QString baseName = tr("New notebook");
    const NotebookDataByNameUpper & nameIndex = m_data.get<ByNameUpper>();
    return newItemName<NotebookDataByNameUpper>(nameIndex, m_lastNewNotebookNameCounter, baseName);
}

void NotebookModel::onNotebookAddedOrUpdated(const Notebook & notebook)
{
    NotebookDataByLocalUid & localUidIndex = m_data.get<ByLocalUid>();

    m_cache.put(notebook.localUid(), notebook);

    auto itemIt = localUidIndex.find(notebook.localUid());
    bool newNotebook = (itemIt == localUidIndex.end());
    if (newNotebook) {
        onNotebookAdded(notebook);
    }
    else {
        onNotebookUpdated(notebook, itemIt);
    }
}

void NotebookModel::onNotebookAdded(const Notebook & notebook)
{
    QNDEBUG("NotebookModel::onNotebookAdded: notebook local uid = " << notebook.localUid());

    if (notebook.hasName()) {
        Q_UNUSED(m_lowerCaseNotebookNames.insert(notebook.name().toLower()))
    }

    NotebookDataByLocalUid & localUidIndex = m_data.get<ByLocalUid>();

    const NotebookModelItem * parentItem = Q_NULLPTR;

    if (notebook.hasStack())
    {
        const QString & stack = notebook.stack();
        auto it = m_modelItemsByStack.find(stack);
        if (it == m_modelItemsByStack.end()) {
            auto stackItemIt = m_stackItems.insert(stack, NotebookStackItem(stack));
            it = addNewStackModelItem(stackItemIt.value());
        }

        parentItem = &(*it);
    }
    else
    {
        if (!m_fakeRootItem) {
            m_fakeRootItem = new NotebookModelItem;
        }

        parentItem = m_fakeRootItem;
    }

    QModelIndex parentIndex = indexForItem(parentItem);

    NotebookItem item;
    notebookToItem(notebook, item);

    int row = parentItem->numChildren();

    beginInsertRows(parentIndex, row, row);

    auto insertionResult = localUidIndex.insert(item);
    auto it = insertionResult.first;
    const NotebookItem * insertedItem = &(*it);

    auto modelItemIt = m_modelItemsByLocalUid.insert(item.localUid(), NotebookModelItem(NotebookModelItem::Type::Notebook, insertedItem, Q_NULLPTR));
    const NotebookModelItem * insertedModelItem = &(*modelItemIt);
    insertedModelItem->setParent(parentItem);
    updateItemRowWithRespectToSorting(*insertedModelItem);

    endInsertRows();
}

void NotebookModel::onNotebookUpdated(const Notebook & notebook, NotebookDataByLocalUid::iterator it)
{
    QNDEBUG("NotebookModel::onNotebookUpdated: notebook local uid = " << notebook.localUid());

    const NotebookItem & originalItem = *it;
    auto nameIt = m_lowerCaseNotebookNames.find(originalItem.name().toLower());
    if (nameIt != m_lowerCaseNotebookNames.end()) {
        Q_UNUSED(m_lowerCaseNotebookNames.erase(nameIt))
    }

    if (notebook.hasName()) {
        Q_UNUSED(m_lowerCaseNotebookNames.insert(notebook.name().toLower()))
    }

    NotebookItem notebookItemCopy;
    notebookToItem(notebook, notebookItemCopy);

    auto modelItemIt = m_modelItemsByLocalUid.find(notebook.localUid());
    if (Q_UNLIKELY(modelItemIt == m_modelItemsByLocalUid.end())) {
        QNWARNING("Can't find the notebook model item corresponding to the updated notebook item: " << notebookItemCopy);
        return;
    }

    const NotebookModelItem * modelItem = &(modelItemIt.value());

    const NotebookModelItem * parentItem = modelItem->parent();
    if (Q_UNLIKELY(!parentItem)) {
        QNWARNING("Can't find the parent notebook model item for updated notebook item: " << *modelItem);
        return;
    }

    int row = parentItem->rowForChild(modelItem);
    if (Q_UNLIKELY(row < 0)) {
        QNWARNING("Can't find the row of the child notebook model item within its parent model item: parent item = "
                  << *parentItem << "\nChild item = " << *modelItem);
        return;
    }

    if (parentItem->type() == NotebookModelItem::Type::Stack)
    {
        const NotebookStackItem * parentStackItem = parentItem->notebookStackItem();
        if (Q_UNLIKELY(!parentStackItem)) {
            QNWARNING("Detected null pointer to notebook stack item in the parent item for the updated notebook item: " << *parentItem);
            return;
        }

        bool shouldChangeParent = false;

        if (!notebook.hasStack())
        {
            // Need to remove the notebook model item from this parent and set the fake root item as the parent
            Q_UNUSED(parentItem->takeChild(row))

            if (!m_fakeRootItem) {
                m_fakeRootItem = new NotebookModelItem;
            }

            parentItem = m_fakeRootItem;
            shouldChangeParent = true;
        }
        else if (notebook.stack() != parentStackItem->name())
        {
            // Notebook stack has changed, need to remove the notebook model item from this parent and either find
            // the existing stack item corresponding to the new stack or create such item if it doesn't exist already
            Q_UNUSED(parentItem->takeChild(row))

            auto stackModelItemIt = m_modelItemsByStack.find(notebook.stack());
            if (stackModelItemIt == m_modelItemsByStack.end())
            {
                auto stackItemIt = m_stackItems.insert(notebook.stack(), NotebookStackItem(notebook.stack()));
                const NotebookStackItem * newStackItem = &(stackItemIt.value());

                if (!m_fakeRootItem) {
                    m_fakeRootItem = new NotebookModelItem;
                }

                NotebookModelItem newStackModelItem(NotebookModelItem::Type::Stack, Q_NULLPTR, newStackItem);
                stackModelItemIt = m_modelItemsByStack.insert(notebook.stack(), newStackModelItem);
                stackModelItemIt->setParent(m_fakeRootItem);
            }

            parentItem = &(stackModelItemIt.value());
            shouldChangeParent = true;
        }

        if (shouldChangeParent)
        {
            modelItem->setParent(parentItem);

            row = parentItem->rowForChild(modelItem);
            if (Q_UNLIKELY(row < 0)) {
                QNWARNING("Can't find the row for the child notebook item within its parent right after setting "
                          "the parent item to the child item! Parent item = " << *parentItem << "\nChild item = "
                          << *modelItem);
                return;
            }
        }
    }
    else if (parentItem != m_fakeRootItem)
    {
        QNWARNING("The updated notebook item has parent which is not of notebook stack type and not a fake root item");
        return;
    }

    NotebookDataByLocalUid & localUidIndex = m_data.get<ByLocalUid>();
    localUidIndex.replace(it, notebookItemCopy);

    // NOTE: as long as we stick to using the model index's internal pointer only inside the model, it's fine
    QModelIndex modelIndexFrom = createIndex(row, 0, const_cast<NotebookModelItem*>(modelItem));
    QModelIndex modelIndexTo = createIndex(row, NUM_NOTEBOOK_MODEL_COLUMNS - 1, const_cast<NotebookModelItem*>(modelItem));
    emit dataChanged(modelIndexFrom, modelIndexTo);

    if (m_sortedColumn != Columns::Name) {
        QNDEBUG("Not sorting by name, returning");
        return;
    }

    emit layoutAboutToBeChanged();
    updateItemRowWithRespectToSorting(*modelItem);
    emit layoutChanged();
}

NotebookModel::ModelItems::iterator NotebookModel::addNewStackModelItem(const NotebookStackItem & stackItem)
{
    QNDEBUG("NotebookModel::addNewStackModelItem: stack item = " << stackItem);

    if (!m_fakeRootItem) {
        m_fakeRootItem = new NotebookModelItem;
    }

    NotebookModelItem newStackItem(NotebookModelItem::Type::Stack, Q_NULLPTR, &stackItem);
    auto it = m_modelItemsByStack.insert(stackItem.name(), newStackItem);
    it->setParent(m_fakeRootItem);
    updateItemRowWithRespectToSorting(*it);
    return it;
}

void NotebookModel::removeItemByLocalUid(const QString & localUid)
{
    QNDEBUG("NotebookModel::removeItemByLocalUid: local uid = " << localUid);

    NotebookDataByLocalUid & localUidIndex = m_data.get<ByLocalUid>();
    auto itemIt = localUidIndex.find(localUid);
    if (Q_UNLIKELY(itemIt == localUidIndex.end())) {
        QNDEBUG("Can't find item to remove from the notebook model");
        return;
    }

    const NotebookItem & item = *itemIt;
    auto nameIt = m_lowerCaseNotebookNames.find(item.name().toLower());
    if (nameIt != m_lowerCaseNotebookNames.end()) {
        Q_UNUSED(m_lowerCaseNotebookNames.erase(nameIt))
    }

    auto notebookModelItemIt = m_modelItemsByLocalUid.find(item.localUid());
    if (Q_UNLIKELY(notebookModelItemIt == m_modelItemsByLocalUid.end())) {
        QNWARNING("Can't find the notebook model item corresponding the the notebook item with local uid " << item.localUid());
        return;
    }

    const NotebookModelItem * modelItem = &(*notebookModelItemIt);
    const NotebookModelItem * parentItem = modelItem->parent();
    if (Q_UNLIKELY(!parentItem)) {
        QNWARNING("Can't find the parent notebook model item for the notebook being removed from the model: local uid = "
                  << item.localUid());
        return;
    }

    int row = parentItem->rowForChild(modelItem);
    if (Q_LIKELY(row < 0)) {
        QNWARNING("Can't find the notebook item's row within its parent model item: notebook item = " << *modelItem
                  << "\nStack item = " << *parentItem);
        return;
    }

    QModelIndex parentItemModelIndex = indexForItem(parentItem);
    beginRemoveRows(parentItemModelIndex, row, row);
    Q_UNUSED(localUidIndex.erase(itemIt))
    Q_UNUSED(parentItem->takeChild(row))
    endRemoveRows();
}

void NotebookModel::notebookToItem(const Notebook & notebook, NotebookItem & item) const
{
    item.setLocalUid(notebook.localUid());

    if (notebook.hasGuid()) {
        item.setGuid(notebook.guid());
    }

    if (notebook.hasStack()) {
        item.setStack(notebook.stack());
    }

    if (notebook.hasName()) {
        item.setName(notebook.name());
    }

    item.setSynchronizable(!notebook.isLocal());
    item.setDirty(notebook.isDirty());
    item.setDefault(notebook.isDefaultNotebook());

    if (notebook.hasPublished()) {
        item.setPublished(notebook.isPublished());
    }

    if (notebook.hasRestrictions()) {
        const qevercloud::NotebookRestrictions & restrictions = notebook.restrictions();
        item.setUpdatable(!restrictions.noUpdateNotebook.isSet() || !restrictions.noUpdateNotebook.ref());
    }
    else {
        item.setUpdatable(true);
    }

    if (notebook.hasLinkedNotebookGuid()) {
        item.setLinkedNotebook(true);
    }
}

void NotebookModel::removeModelItemFromParent(const NotebookModelItem & modelItem)
{
    QNDEBUG("NotebookModel::removeModelItemFromParent: " << modelItem);

    const NotebookModelItem * parentItem = modelItem.parent();
    if (Q_UNLIKELY(!parentItem)) {
        QNDEBUG("No parent item, nothing to do");
        return;
    }

    int row = parentItem->rowForChild(&modelItem);
    if (Q_UNLIKELY(row < 0)) {
        QNWARNING("Can't find the child notebook model item's row within its parent; child item = " << modelItem
                  << ", parent item = " << *parentItem);
        return;
    }

    Q_UNUSED(parentItem->takeChild(row))
}

int NotebookModel::rowForNewItem(const NotebookModelItem & parentItem, const NotebookModelItem & newItem) const
{
    if (m_sortedColumn != Columns::Name) {
        // Sorting by other columns is not yet implemented
        return parentItem.numChildren();
    }

    QList<const NotebookModelItem*> children = parentItem.children();
    auto it = children.end();

    if (m_sortOrder == Qt::AscendingOrder) {
        it = std::lower_bound(children.begin(), children.end(), &newItem, LessByName());
    }
    else {
        it = std::lower_bound(children.begin(), children.end(), &newItem, GreaterByName());
    }

    if (it == children.end()) {
        return parentItem.numChildren();
    }

    int row = static_cast<int>(std::distance(children.begin(), it));
    return row;
}

void NotebookModel::updateItemRowWithRespectToSorting(const NotebookModelItem & modelItem)
{
    if (m_sortedColumn != Columns::Name) {
        // Sorting by other columns is not yet implemented
        return;
    }

    const NotebookModelItem * parentItem = modelItem.parent();
    if (!parentItem)
    {
        if (!m_fakeRootItem) {
            m_fakeRootItem = new NotebookModelItem;
        }

        parentItem = m_fakeRootItem;
        int row = rowForNewItem(*parentItem, modelItem);
        parentItem->insertChild(row, &modelItem);
        return;
    }

    int currentItemRow = parentItem->rowForChild(&modelItem);
    if (Q_UNLIKELY(currentItemRow < 0)) {
        QNWARNING("Can't update notebook model item's row: can't find its original row within parent: " << modelItem);
        return;
    }

    Q_UNUSED(parentItem->takeChild(currentItemRow))
    int appropriateRow = rowForNewItem(*parentItem, modelItem);
    parentItem->insertChild(appropriateRow, &modelItem);
    QNTRACE("Moved item from row " << currentItemRow << " to row " << appropriateRow << "; item: " << modelItem);
}

void NotebookModel::updatePersistentModelIndices()
{
    QNDEBUG("NotebookModel::updatePersistentModelIndices");

    // Ensure any persistent model indices would be updated appropriately
    QModelIndexList indices = persistentIndexList();
    for(auto it = indices.begin(), end = indices.end(); it != end; ++it)
    {
        const QModelIndex & index = *it;
        const NotebookModelItem * item = reinterpret_cast<const NotebookModelItem*>(index.internalPointer());
        QModelIndex replacementIndex = indexForItem(item);
        changePersistentIndex(index, replacementIndex);
    }
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

} // namespace quentier
