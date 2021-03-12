/*
 * Copyright 2016-2021 Dmitry Ivanov
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

#ifndef QUENTIER_LIB_MODEL_NOTEBOOK_MODEL_H
#define QUENTIER_LIB_MODEL_NOTEBOOK_MODEL_H

#include "NotebookCache.h"

#include "INotebookModelItem.h"
#include "LinkedNotebookRootItem.h"
#include "NotebookItem.h"
#include "StackItem.h"

#include <lib/model/common/AbstractItemModel.h>

#include <quentier/local_storage/LocalStorageManagerAsync.h>
#include <quentier/types/Account.h>
#include <quentier/utility/SuppressWarnings.h>

#include <QFlags>
#include <QHash>
#include <QMap>
#include <QSet>
#include <QUuid>

#include <boost/bimap.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index_container.hpp>

#include <utility>

#define NOTEBOOK_MODEL_MIME_TYPE                                               \
    QStringLiteral("application/x-com.quentier.notebookmodeldatalist")

#define NOTEBOOK_MODEL_MIME_DATA_MAX_COMPRESSION (9)

class QDebug;

namespace quentier {

class NotebookModel final: public AbstractItemModel
{
    Q_OBJECT
public:
    explicit NotebookModel(
        const Account & account,
        LocalStorageManagerAsync & localStorageManagerAsync,
        NotebookCache & cache, QObject * parent = nullptr);

    ~NotebookModel() override;

    [[nodiscard]] const Account & account() const noexcept
    {
        return m_account;
    }

    void setAccount(Account account);

    enum class Column
    {
        Name,
        Synchronizable,
        Dirty,
        Default,
        LastUsed,
        Published,
        FromLinkedNotebook,
        NoteCount
    };

    friend QDebug & operator<<(QDebug & dbg, Column column);

    [[nodiscard]] INotebookModelItem * itemForIndex(
        const QModelIndex & index) const;

    [[nodiscard]] QModelIndex indexForItem(
        const INotebookModelItem * pItem) const;

    [[nodiscard]] QModelIndex indexForNotebookName(
        const QString & notebookName,
        const QString & linkedNotebookGuid = {}) const;

    /**
     * @param                   The name of the notebook stack which item's
     *                          index is required
     * @param                   The linked notebook guid for which the notebook
     *                          stack item's index is required; if null or
     *                          empty, the item corresponding to the notebook
     *                          stack from user's own account it considered,
     *                          otherwise the item corresponding linked notebook
     *                          guid would be considered
     * @return                  The model index of the StackItem
     *                          corresponding to the given stack
     */
    [[nodiscard]] QModelIndex indexForNotebookStack(
        const QString & stack, const QString & linkedNotebookGuid) const;

    /**
     * @param                   The linked notebook guid which corresponding
     *                          linked notebook item's index is required
     * @return                  The model index of
     *                          the LinkedNotebookRootItem
     *                          corresponding to the given linked notebook guid
     */
    [[nodiscard]] QModelIndex indexForLinkedNotebookGuid(
        const QString & linkedNotebookGuid) const;

    /**
     * @return                  The model index of the AllNotebooksRootItem
     */
    [[nodiscard]] QModelIndex allNotebooksRootItemIndex() const;

    /**
     * Enum defining filter for methods listing stuff related to multiple
     * notebook items
     */
    enum class Filter
    {
        NoFilter = 0x0,
        CanCreateNotes = 0x1,
        CannotCreateNotes = 0x2,
        CanUpdateNotes = 0x3,
        CannotUpdateNotes = 0x4,
        CanUpdateNotebook = 0x5,
        CannotUpdateNotebook = 0x6,
        CanRenameNotebook = 0x7,
        CannotRenameNotebook = 0x8
    };

    Q_DECLARE_FLAGS(Filters, Filter)

    friend QDebug & operator<<(QDebug & dbg, Filter filter);
    friend QDebug & operator<<(QDebug & dbg, Filters filters);

    /**
     * @param filter                Filter for the listed notebook names
     * @param linkedNotebookGuid    The optional guid of the linked notebook to
     *                              which the returned notebook names belong;
     *                              if it is null (i.e.
     *                              linkedNotebookGuid.isNull() returns true),
     *                              the notebook names would be returned
     *                              ignoring their belonging to user's own
     *                              account or linked notebook; if it's not null
     *                              but empty (i.e. linkedNotebookGuid.isEmpty()
     *                              returns true), only the names of notebooks
     *                              from user's own account would be returned.
     *                              Otherwise only the names of notebooks from
     *                              the corresponding linked notebook would be
     *                              returned
     * @return                      The sorted list of notebook names conformant
     *                              with the filter
     */
    [[nodiscard]] QStringList notebookNames(
        Filters filters, const QString & linkedNotebookGuid = {}) const;

    /**
     * @return                      The index of the default notebook item if
     *                              such one exists or invalid model index
     *                              otherwise
     */
    [[nodiscard]] QModelIndex defaultNotebookIndex() const;

    /**
     * @brief lastUsedNotebookIndex
     * @return                      The index of the last used notebook item if
     *                              such one exists or invalid model index
     *                              otherwise
     */
    [[nodiscard]] QModelIndex lastUsedNotebookIndex() const;

    /**
     * @brief moveToStack moves the notebook item pointed to by the index
     * to the specified stack
     * @param index                 The index of the notebook item to be moved
     *                              to the stack
     * @param stack                 The stack to which the notebook item needs
     *                              to be moved
     * @return                      The index of the moved notebook item or
     *                              invalid index if something went wrong,
     *                              for example, if the passed in index did not
     *                              really point to the notebook item
     *
     * @note There is no ambiguity here between stacks from user's own account
     * and those from linked notebooks - the notebook item would be moved to
     * the stack which it corresponds to: if the notebook belongs to user's
     * own account, the stack would also be from user's own account; otherwise,
     * the stack would be that of the corresponding linked notebook
     */
    [[nodiscard]] QModelIndex moveToStack(
        const QModelIndex & index, const QString & stack);

    /**
     * @brief removeFromStack removes the notebook item pointed to by the index
     * from its stack (if any)
     * @param index                 The index of the notebook item to be removed
     *                              from its stack
     * @return                      The index of the notebook item removed from
     *                              its stack or invalid index if something went
     *                              wrong, for example, if the passed in index
     *                              did not really point to the notebook item
     */
    [[nodiscard]] QModelIndex removeFromStack(const QModelIndex & index);

    /**
     * @brief stacks
     * @param linkedNotebookGuid    The guid of the linked notebook from which
     *                              the stacks are requested; if empty,
     *                              the stacks from user's own account are
     *                              returned
     * @return                      The sorted list of existing notebook stacks
     */
    [[nodiscard]] QStringList stacks(const QString & linkedNotebookGuid = {}) const;

    /**
     * @return the linked notebook owners' names by linked notebook guid
     */
    [[nodiscard]] const QHash<QString, QString> &
    linkedNotebookOwnerNamesByGuid() const noexcept;

    /**
     * @brief createNotebook is the convenience method to create a new notebook
     * within the model
     * @param notebookName          The name of the new notebook
     * @param notebookStack         The stack of the new notebook
     * @param errorDescription      The textual description of the error if
     *                              the notebook could not be created
     * @return                      Either valid model index if notebook was
     *                              created successfully or invalid model index
     *                              otherwise
     */
    [[nodiscard]] QModelIndex createNotebook(
        const QString & notebookName, const QString & notebookStack,
        ErrorString & errorDescription);

    /**
     * @brief columnName
     * @param column                The column which name needs to be returned
     * @return                      The name of the column
     */
    [[nodiscard]] QString columnName(Column column) const;

    /**
     * @brief allNotebooksListed
     * @return                      True if the notebook model has received
     *                              the information it needs about all notebooks
     *                              stored in the local storage by the moment;
     *                              false otherwise
     */
    [[nodiscard]] bool allNotebooksListed() const noexcept;

    /**
     * @brief favoriteNotebook - marks the notebook pointed to by the index as
     * favorited
     *
     * Favorited property of Notebook class is not represented as a column
     * within the NotebookModel so this method doesn't change anything
     * in the model but only the underlying notebook object persisted
     * in the local storage
     *
     * @param index                 The index of the notebook to be favorited
     */
    void favoriteNotebook(const QModelIndex & index);

    /**
     * @brief unfavoriteNotebook - removes the favorited mark from the notebook
     * pointed to by the index; does nothing if the notebook was not favorited
     * prior to the call
     *
     * Favorited property of Notebook class is not represented as a column
     * within the NotebookModel so this method doesn't change anything in
     * the model but only the underlying notebook object persisted in
     * the local storage
     *
     * @param index                 The index of the notebook to be unfavorited
     */
    void unfavoriteNotebook(const QModelIndex & index);

public:
    // AbstractItemModel interface
    [[nodiscard]] QString localIdForItemName(
        const QString & itemName,
        const QString & linkedNotebookGuid) const override;

    [[nodiscard]] QModelIndex indexForLocalId(
        const QString & localId) const override;

    [[nodiscard]] QString itemNameForLocalId(
        const QString & localId) const override;

    [[nodiscard]] ItemInfo itemInfoForLocalId(
        const QString & localId) const override;

    [[nodiscard]] QStringList itemNames(
        const QString & linkedNotebookGuid) const override;

    [[nodiscard]] QVector<LinkedNotebookInfo> linkedNotebooksInfo() const
        override;

    [[nodiscard]] QString linkedNotebookUsername(
        const QString & linkedNotebookGuid) const override;

    [[nodiscard]] int nameColumn() const noexcept override
    {
        return static_cast<int>(Column::Name);
    }

    [[nodiscard]] int sortingColumn() const noexcept override
    {
        return static_cast<int>(m_sortedColumn);
    }

    [[nodiscard]] Qt::SortOrder sortOrder() const noexcept override
    {
        return m_sortOrder;
    }

    [[nodiscard]] bool allItemsListed() const noexcept override
    {
        return allNotebooksListed();
    }

    [[nodiscard]] QModelIndex allItemsRootItemIndex() const override;

    [[nodiscard]] QString localIdForItemIndex(
        const QModelIndex & index) const override;

    [[nodiscard]] QString linkedNotebookGuidForItemIndex(
        const QModelIndex & index) const override;

public:
    // QAbstractItemModel interface
    [[nodiscard]] Qt::ItemFlags flags(const QModelIndex & index) const override;

    [[nodiscard]] QVariant data(
        const QModelIndex & index, int role = Qt::DisplayRole) const override;

    [[nodiscard]] QVariant headerData(
        int section, Qt::Orientation orientation,
        int role = Qt::DisplayRole) const override;

    [[nodiscard]] int rowCount(const QModelIndex & parent = {}) const override;

    [[nodiscard]] int columnCount(
        const QModelIndex & parent = {}) const override;

    [[nodiscard]] QModelIndex index(
        int row, int column, const QModelIndex & parent = {}) const override;

    [[nodiscard]] QModelIndex parent(const QModelIndex & index) const override;

    [[nodiscard]] bool setHeaderData(
        int section, Qt::Orientation orientation, const QVariant & value,
        int role = Qt::EditRole) override;

    [[nodiscard]] bool setData(
        const QModelIndex & index, const QVariant & value,
        int role = Qt::EditRole) override;

    [[nodiscard]] bool insertRows(
        int row, int count, const QModelIndex & parent = {}) override;

    [[nodiscard]] bool removeRows(
        int row, int count, const QModelIndex & parent = {}) override;

    void sort(int column, Qt::SortOrder order) override;

    // Drag-n-drop interfaces
    [[nodiscard]] Qt::DropActions supportedDragActions() const override
    {
        return Qt::MoveAction;
    }

    [[nodiscard]] Qt::DropActions supportedDropActions() const override
    {
        return Qt::MoveAction;
    }

    [[nodiscard]] QStringList mimeTypes() const override;

    [[nodiscard]] QMimeData * mimeData(
        const QModelIndexList & indexes) const override;

    [[nodiscard]] bool dropMimeData(
        const QMimeData * data, Qt::DropAction action, int row, int column,
        const QModelIndex & parent) override;

Q_SIGNALS:
    void notifyError(ErrorString errorDescription);

    /**
     * @brief notifyAllNotebooksListed is the signal emitted when the notebook
     * model has received the information on all notebooks stored in
     * the local storage
     */
    void notifyAllNotebooksListed();

    /**
     * @brief notifyNotebookStackRenamed is the signal emitted after
     * the notebook stack has been renamed
     *
     * @param previousStackName         The name of the notebook stack prior to
     *                                  renaming
     * @param newStackName              The new name of the notebook stack
     * @param linkedNotebookGuid        If the stack belonged to the linked
     *                                  notebook, this parameter is its guid
     */
    void notifyNotebookStackRenamed(
        const QString & previousStackName, const QString & newStackName,
        const QString & linkedNotebookGuid);

    /**
     * @brief notifyNotebookStackChanged is the signal emitted after
     * the notebook has been assigned to another stack
     *
     * @param notebookIndex             The model index of the notebook which
     *                                  stack has been changed
     */
    void notifyNotebookStackChanged(const QModelIndex & notebookIndex);

    // Informative signals for views, so that they can prepare to the changes
    // in the tree of notebooks/stacks and do some recovery after that
    void aboutToAddNotebook();
    void addedNotebook(const QModelIndex & index);

    void aboutToUpdateNotebook(const QModelIndex & index);
    void updatedNotebook(const QModelIndex & index);

    void aboutToRemoveNotebooks();
    void removedNotebooks();

    // private signals
    void addNotebook(qevercloud::Notebook notebook, QUuid requestId);
    void updateNotebook(qevercloud::Notebook notebook, QUuid requestId);
    void findNotebook(qevercloud::Notebook notebook, QUuid requestId);

    void listNotebooks(
        LocalStorageManager::ListObjectsOptions flag, size_t limit,
        size_t offset, LocalStorageManager::ListNotebooksOrder order,
        LocalStorageManager::OrderDirection orderDirection,
        QString linkedNotebookGuid, QUuid requestId);

    void expungeNotebook(qevercloud::Notebook notebook, QUuid requestId);

    void requestNoteCountPerNotebook(
        qevercloud::Notebook notebook,
        LocalStorageManager::NoteCountOptions options, QUuid requestId);

    void listAllLinkedNotebooks(
        size_t limit, size_t offset,
        LocalStorageManager::ListLinkedNotebooksOrder order,
        LocalStorageManager::OrderDirection orderDirection, QUuid requestId);

private Q_SLOTS:
    // Slots for response to events from local storage
    void onAddNotebookComplete(qevercloud::Notebook notebook, QUuid requestId);

    void onAddNotebookFailed(
        qevercloud::Notebook notebook, ErrorString errorDescription,
        QUuid requestId);

    void onUpdateNotebookComplete(
        qevercloud::Notebook notebook, QUuid requestId);

    void onUpdateNotebookFailed(
        qevercloud::Notebook notebook, ErrorString errorDescription,
        QUuid requestId);

    void onFindNotebookComplete(qevercloud::Notebook notebook, QUuid requestId);

    void onFindNotebookFailed(
        qevercloud::Notebook notebook, ErrorString errorDescription,
        QUuid requestId);

    void onListNotebooksComplete(
        LocalStorageManager::ListObjectsOptions flag, size_t limit,
        size_t offset, LocalStorageManager::ListNotebooksOrder order,
        LocalStorageManager::OrderDirection orderDirection,
        QString linkedNotebookGuid, QList<qevercloud::Notebook> foundNotebooks,
        QUuid requestId);

    void onListNotebooksFailed(
        LocalStorageManager::ListObjectsOptions flag, size_t limit,
        size_t offset, LocalStorageManager::ListNotebooksOrder order,
        LocalStorageManager::OrderDirection orderDirection,
        QString linkedNotebookGuid, ErrorString errorDescription,
        QUuid requestId);

    void onExpungeNotebookComplete(
        qevercloud::Notebook notebook, QUuid requestId);

    void onExpungeNotebookFailed(
        qevercloud::Notebook notebook, ErrorString errorDescription,
        QUuid requestId);

    void onGetNoteCountPerNotebookComplete(
        int noteCount, qevercloud::Notebook notebook,
        LocalStorageManager::NoteCountOptions options, QUuid requestId);

    void onGetNoteCountPerNotebookFailed(
        ErrorString errorDescription, qevercloud::Notebook notebook,
        LocalStorageManager::NoteCountOptions options, QUuid requestId);

    void onAddNoteComplete(qevercloud::Note note, QUuid requestId);

    void onNoteMovedToAnotherNotebook(
        QString noteLocalId, QString previousNotebookLocalId,
        QString newNotebookLocalId);

    void onExpungeNoteComplete(qevercloud::Note note, QUuid requestId);

    void onAddLinkedNotebookComplete(
        qevercloud::LinkedNotebook linkedNotebook, QUuid requestId);

    void onUpdateLinkedNotebookComplete(
        qevercloud::LinkedNotebook linkedNotebook, QUuid requestId);

    void onExpungeLinkedNotebookComplete(
        qevercloud::LinkedNotebook linkedNotebook, QUuid requestId);

    void onListAllLinkedNotebooksComplete(
        size_t limit, size_t offset,
        LocalStorageManager::ListLinkedNotebooksOrder order,
        LocalStorageManager::OrderDirection orderDirection,
        QList<qevercloud::LinkedNotebook> foundLinkedNotebooks,
        QUuid requestId);

    void onListAllLinkedNotebooksFailed(
        size_t limit, size_t offset,
        LocalStorageManager::ListLinkedNotebooksOrder order,
        LocalStorageManager::OrderDirection orderDirection,
        ErrorString errorDescription, QUuid requestId);

private:
    void createConnections(LocalStorageManagerAsync & localStorageManagerAsync);
    void requestNotebooksList();
    void requestNoteCountForNotebook(const qevercloud::Notebook & notebook);
    void requestNoteCountForAllNotebooks();
    void requestLinkedNotebooksList();

    [[nodiscard]] QVariant dataImpl(
        const INotebookModelItem & item, Column column) const;

    [[nodiscard]] QVariant dataAccessibleText(
        const INotebookModelItem & item, Column column) const;

    [[nodiscard]] bool canUpdateNotebookItem(const NotebookItem & item) const;

    void updateNotebookInLocalStorage(const NotebookItem & item);
    void expungeNotebookFromLocalStorage(const QString & localId);

    [[nodiscard]] QString nameForNewNotebook() const;

    void removeItemByLocalId(const QString & localId);

    void notebookToItem(
        const qevercloud::Notebook & notebook, NotebookItem & item);

    enum class RemoveEmptyParentStack
    {
        Yes,
        No
    };

    friend QDebug & operator<<(QDebug & dbg, RemoveEmptyParentStack option);

    void removeModelItemFromParent(
        INotebookModelItem & modelItem,
        RemoveEmptyParentStack option = RemoveEmptyParentStack::Yes);

    // Returns the appropriate row before which the new item should be inserted
    // according to the current sorting criteria and column
    [[nodiscard]] int rowForNewItem(
        const INotebookModelItem & parentItem,
        const INotebookModelItem & newItem) const;

    void updateItemRowWithRespectToSorting(INotebookModelItem & modelItem);

    void updatePersistentModelIndices();

    // Returns true if successfully incremented the note count for the notebook
    // item with the corresponding local uid
    [[nodiscard]] bool incrementNoteCountForNotebook(
        const QString & notebookLocalId);

    // Returns true if successfully decremented the note count for the notebook
    // item with the corresponding local uid
    [[nodiscard]] bool decrementNoteCountForNotebook(
        const QString & notebookLocalId);

    void switchDefaultNotebookLocalId(const QString & localId);
    void switchLastUsedNotebookLocalId(const QString & localId);

    void checkAndRemoveEmptyStackItem(INotebookModelItem & modelItem);

    void setNotebookFavorited(const QModelIndex & index, bool favorited);

    void beginRemoveNotebooks();
    void endRemoveNotebooks();

    [[nodiscard]] INotebookModelItem & findOrCreateLinkedNotebookModelItem(
        const QString & linkedNotebookGuid);

    void checkAndCreateModelRootItems();

private:
    struct ByLocalId
    {};

    struct ByNameUpper
    {};

    struct ByStack
    {};

    struct ByLinkedNotebookGuid
    {};

    using NotebookData = boost::multi_index_container<
        NotebookItem,
        boost::multi_index::indexed_by<
            boost::multi_index::ordered_unique<
                boost::multi_index::tag<ByLocalId>,
                boost::multi_index::const_mem_fun<
                    NotebookItem, const QString &, &NotebookItem::localId>>,
            boost::multi_index::ordered_non_unique<
                boost::multi_index::tag<ByNameUpper>,
                boost::multi_index::const_mem_fun<
                    NotebookItem, QString, &NotebookItem::nameUpper>>,
            boost::multi_index::ordered_non_unique<
                boost::multi_index::tag<ByStack>,
                boost::multi_index::const_mem_fun<
                    NotebookItem, const QString &, &NotebookItem::stack>>,
            boost::multi_index::ordered_non_unique<
                boost::multi_index::tag<ByLinkedNotebookGuid>,
                boost::multi_index::const_mem_fun<
                    NotebookItem, const QString &,
                    &NotebookItem::linkedNotebookGuid>>>>;

    using NotebookDataByLocalId = NotebookData::index<ByLocalId>::type;
    using NotebookDataByNameUpper = NotebookData::index<ByNameUpper>::type;
    using NotebookDataByStack = NotebookData::index<ByStack>::type;

    using NotebookDataByLinkedNotebookGuid =
        NotebookData::index<ByLinkedNotebookGuid>::type;

    struct LessByName
    {
        [[nodiscard]] bool operator()(
            const NotebookItem & lhs, const NotebookItem & rhs) const noexcept;

        [[nodiscard]] bool operator()(
            const NotebookItem * pLhs,
            const NotebookItem * pRhs) const noexcept;

        [[nodiscard]] bool operator()(
            const StackItem & lhs, const StackItem & rhs) const noexcept;

        [[nodiscard]] bool operator()(
            const StackItem * pLhs, const StackItem * pRhs) const noexcept;

        [[nodiscard]] bool operator()(
            const LinkedNotebookRootItem & lhs,
            const LinkedNotebookRootItem & rhs) const noexcept;

        [[nodiscard]] bool operator()(
            const LinkedNotebookRootItem * lhs,
            const LinkedNotebookRootItem * rhs) const noexcept;

        [[nodiscard]] bool operator()(
            const INotebookModelItem & lhs,
            const INotebookModelItem & rhs) const noexcept;

        [[nodiscard]] bool operator()(
            const INotebookModelItem * pLhs,
            const INotebookModelItem * pRhs) const noexcept;
    };

    struct GreaterByName
    {
        [[nodiscard]] bool operator()(
            const NotebookItem & lhs, const NotebookItem & rhs) const noexcept;

        [[nodiscard]] bool operator()(
            const NotebookItem * pLhs,
            const NotebookItem * pRhs) const noexcept;

        [[nodiscard]] bool operator()(
            const StackItem & lhs, const StackItem & rhs) const noexcept;

        [[nodiscard]] bool operator()(
            const StackItem * pLhs, const StackItem * pRhs) const noexcept;

        [[nodiscard]] bool operator()(
            const LinkedNotebookRootItem & lhs,
            const LinkedNotebookRootItem & rhs) const noexcept;

        [[nodiscard]] bool operator()(
            const LinkedNotebookRootItem * pLhs,
            const LinkedNotebookRootItem * pRhs) const noexcept;

        [[nodiscard]] bool operator()(
            const INotebookModelItem & lhs,
            const INotebookModelItem & rhs) const noexcept;

        [[nodiscard]] bool operator()(
            const INotebookModelItem * pLhs,
            const INotebookModelItem * pRhs) const noexcept;
    };

    using ModelItems = QMap<QString, INotebookModelItem *>;
    using StackItems = QMap<QString, StackItem>;
    using LinkedNotebookItems = QMap<QString, LinkedNotebookRootItem>;

    using IndexId = quintptr;

    using IndexIdToLocalIdBimap = boost::bimap<IndexId, QString>;

    using IndexIdToStackAndLinkedNotebookGuidBimap =
        boost::bimap<IndexId, std::pair<QString, QString>>;

    using IndexIdToLinkedNotebookGuidBimap = boost::bimap<IndexId, QString>;

    class RemoveRowsScopeGuard
    {
    public:
        RemoveRowsScopeGuard(NotebookModel & model);
        ~RemoveRowsScopeGuard();

    private:
        NotebookModel & m_model;
    };

    friend class RemoveRowsScopeGuard;

private:
    void onNotebookAddedOrUpdated(const qevercloud::Notebook & notebook);
    void onNotebookAdded(const qevercloud::Notebook & notebook);

    void onNotebookUpdated(
        const qevercloud::Notebook & notebook,
        NotebookDataByLocalId::iterator it);

    void onLinkedNotebookAddedOrUpdated(
        const qevercloud::LinkedNotebook & linkedNotebook);

    [[nodiscard]] INotebookModelItem * itemForId(IndexId id) const;
    [[nodiscard]] IndexId idForItem(const INotebookModelItem & item) const;

    [[nodiscard]] Qt::ItemFlags adjustFlagsForColumn(
        int column, Qt::ItemFlags flags) const;

    // Helper methods for notebook items

    [[nodiscard]] bool setNotebookData(
        NotebookItem & notebookItem, const QModelIndex & modelIndex,
        const QVariant & value);

    [[nodiscard]] bool setNotebookName(
        NotebookItem & notebookItem, const QString & newName);

    [[nodiscard]] bool setNotebookSynchronizable(
        NotebookItem & notebookItem, bool synchronizable);

    [[nodiscard]] bool setNotebookIsDefault(
        NotebookItem & notebookItem, bool isDefault);

    [[nodiscard]] bool setNotebookIsLastUsed(
        NotebookItem & notebookItem, bool isLastUsed);

    [[nodiscard]] QVariant notebookData(
        const NotebookItem & notebookItem, Column column) const;

    [[nodiscard]] QVariant notebookAccessibleData(
        const NotebookItem & notebookItem, Column column) const;

    [[nodiscard]] bool canRemoveNotebookItem(const NotebookItem & notebookItem);

    // Returns true if successfully incremented the note count for the notebook
    // item with the corresponding local uid
    [[nodiscard]] bool updateNoteCountPerNotebookIndex(
        const NotebookItem & item, NotebookDataByLocalId::iterator it);

    [[nodiscard]] bool notebookItemMatchesByLinkedNotebook(
        const NotebookItem & item, const QString & linkedNotebookGuid) const;

    [[nodiscard]] Qt::ItemFlags flagsForNotebookItem(
        const NotebookItem & notebookItem, int column,
        Qt::ItemFlags flags) const;

    // Helper methods for stack items

    [[nodiscard]] bool setStackData(
        StackItem & stackItem, const QModelIndex & modelIndex,
        const QVariant & value);

    [[nodiscard]] QVariant stackData(
        const StackItem & stackItem, Column column) const;

    [[nodiscard]] QVariant stackAccessibleData(
        const StackItem & stackItem, Column column) const;

    [[nodiscard]] const StackItems * stackItems(
        const QString & linkedNotebookGuid) const;

    [[nodiscard]] std::pair<StackItems *, INotebookModelItem *>
    stackItemsWithParent(const QString & linkedNotebookGuid);

    [[nodiscard]] StackItem & findOrCreateStackItem(
        const QString & stack, StackItems & stackItems,
        INotebookModelItem * pParentItem);

    [[nodiscard]] Qt::ItemFlags flagsForStackItem(
        const StackItem & stackItem, int column,
        Qt::ItemFlags flags) const;

private:
    NotebookData m_data;

    INotebookModelItem * m_pInvisibleRootItem = nullptr;

    INotebookModelItem * m_pAllNotebooksRootItem = nullptr;
    IndexId m_allNotebooksRootItemIndexId = 1;

    QString m_defaultNotebookLocalId;
    QString m_lastUsedNotebookLocalId;

    StackItems m_stackItems;
    QMap<QString, StackItems> m_stackItemsByLinkedNotebookGuid;

    LinkedNotebookItems m_linkedNotebookItems;

    mutable IndexIdToLocalIdBimap m_indexIdToLocalIdBimap;

    mutable IndexIdToStackAndLinkedNotebookGuidBimap
        m_indexIdToStackAndLinkedNotebookGuidBimap;

    mutable IndexIdToLinkedNotebookGuidBimap m_indexIdToLinkedNotebookGuidBimap;
    mutable IndexId m_lastFreeIndexId = 2;

    NotebookCache & m_cache;

    size_t m_listNotebooksOffset = 0;
    QUuid m_listNotebooksRequestId;
    QSet<QUuid> m_notebookItemsNotYetInLocalStorageUids;

    QSet<QUuid> m_addNotebookRequestIds;
    QSet<QUuid> m_updateNotebookRequestIds;
    QSet<QUuid> m_expungeNotebookRequestIds;

    QSet<QUuid> m_findNotebookToRestoreFailedUpdateRequestIds;
    QSet<QUuid> m_findNotebookToPerformUpdateRequestIds;

    QSet<QUuid> m_noteCountPerNotebookRequestIds;

    QHash<QString, QString> m_linkedNotebookUsernamesByGuids;

    size_t m_listLinkedNotebooksOffset = 0;
    QUuid m_listLinkedNotebooksRequestId;

    Column m_sortedColumn = Column::Name;
    Qt::SortOrder m_sortOrder = Qt::AscendingOrder;

    mutable int m_lastNewNotebookNameCounter = 0;

    bool m_allNotebooksListed = false;
    bool m_allLinkedNotebooksListed = false;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(NotebookModel::Filters)

} // namespace quentier

#endif // QUENTIER_LIB_MODEL_NOTEBOOK_MODEL_H
