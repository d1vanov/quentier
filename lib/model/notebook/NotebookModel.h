/*
 * Copyright 2016-2020 Dmitry Ivanov
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

SAVE_WARNINGS

MSVC_SUPPRESS_WARNING(4834)

#include <boost/bimap.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index_container.hpp>

RESTORE_WARNINGS

#include <utility>

#define NOTEBOOK_MODEL_MIME_TYPE                                               \
    QStringLiteral("application/x-com.quentier.notebookmodeldatalist")

#define NOTEBOOK_MODEL_MIME_DATA_MAX_COMPRESSION (9)

QT_FORWARD_DECLARE_CLASS(QDebug)

namespace quentier {

class NotebookModel : public AbstractItemModel
{
    Q_OBJECT
public:
    explicit NotebookModel(
        const Account & account,
        LocalStorageManagerAsync & localStorageManagerAsync,
        NotebookCache & cache, QObject * parent = nullptr);

    virtual ~NotebookModel();

    const Account & account() const
    {
        return m_account;
    }

    void setAccount(Account account);

    enum class Column
    {
        Name = 0,
        Synchronizable,
        Dirty,
        Default,
        LastUsed,
        Published,
        FromLinkedNotebook,
        NoteCount
    };

    friend QDebug & operator<<(QDebug & dbg, const Column column);

    INotebookModelItem * itemForIndex(const QModelIndex & index) const;
    QModelIndex indexForItem(const INotebookModelItem * pItem) const;

    QModelIndex indexForNotebookName(
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
    QModelIndex indexForNotebookStack(
        const QString & stack, const QString & linkedNotebookGuid) const;

    /**
     * @param                   The linked notebook guid which corresponding
     *                          linked notebook item's index is required
     * @return                  The model index of
     *                          the LinkedNotebookRootItem
     *                          corresponding to the given linked notebook guid
     */
    QModelIndex indexForLinkedNotebookGuid(
        const QString & linkedNotebookGuid) const;

    /**
     * @return                  The model index of the AllNotebooksRootItem
     */
    QModelIndex allNotebooksRootItemIndex() const;

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

    friend QDebug & operator<<(QDebug & dbg, const Filter filter);
    friend QDebug & operator<<(QDebug & dbg, const Filters filters);

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
    QStringList notebookNames(
        const Filters filters, const QString & linkedNotebookGuid = {}) const;

    /**
     * @return                      The index of the default notebook item if
     *                              such one exists or invalid model index
     *                              otherwise
     */
    QModelIndex defaultNotebookIndex() const;

    /**
     * @brief lastUsedNotebookIndex
     * @return                      The index of the last used notebook item if
     *                              such one exists or invalid model index
     *                              otherwise
     */
    QModelIndex lastUsedNotebookIndex() const;

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
    QModelIndex moveToStack(const QModelIndex & index, const QString & stack);

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
    QModelIndex removeFromStack(const QModelIndex & index);

    /**
     * @brief stacks
     * @param linkedNotebookGuid    The guid of the linked notebook from which
     *                              the stacks are requested; if empty,
     *                              the stacks from user's own account are
     *                              returned
     * @return                      The sorted list of existing notebook stacks
     */
    QStringList stacks(const QString & linkedNotebookGuid = {}) const;

    /**
     * @return the linked notebook owners' names by linked notebook guid
     */
    const QHash<QString, QString> & linkedNotebookOwnerNamesByGuid() const;

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
    QModelIndex createNotebook(
        const QString & notebookName, const QString & notebookStack,
        ErrorString & errorDescription);

    /**
     * @brief columnName
     * @param column                The column which name needs to be returned
     * @return                      The name of the column
     */
    QString columnName(const Column column) const;

    /**
     * @brief allNotebooksListed
     * @return                      True if the notebook model has received
     *                              the information it needs about all notebooks
     *                              stored in the local storage by the moment;
     *                              false otherwise
     */
    bool allNotebooksListed() const;

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
    virtual QString localUidForItemName(
        const QString & itemName,
        const QString & linkedNotebookGuid) const override;

    virtual QModelIndex indexForLocalUid(
        const QString & localUid) const override;

    virtual QString itemNameForLocalUid(
        const QString & localUid) const override;

    virtual ItemInfo itemInfoForLocalUid(
        const QString & localUid) const override;

    virtual QStringList itemNames(
        const QString & linkedNotebookGuid) const override;

    virtual QVector<LinkedNotebookInfo> linkedNotebooksInfo() const override;

    virtual QString linkedNotebookUsername(
        const QString & linkedNotebookGuid) const override;

    virtual int nameColumn() const override
    {
        return static_cast<int>(Column::Name);
    }

    virtual int sortingColumn() const override
    {
        return static_cast<int>(m_sortedColumn);
    }

    virtual Qt::SortOrder sortOrder() const override
    {
        return m_sortOrder;
    }

    virtual bool allItemsListed() const override
    {
        return allNotebooksListed();
    }

    virtual QModelIndex allItemsRootItemIndex() const override;

    virtual QString localUidForItemIndex(
        const QModelIndex & index) const override;

    virtual QString linkedNotebookGuidForItemIndex(
        const QModelIndex & index) const override;

public:
    // QAbstractItemModel interface
    virtual Qt::ItemFlags flags(const QModelIndex & index) const override;

    virtual QVariant data(
        const QModelIndex & index, int role = Qt::DisplayRole) const override;

    virtual QVariant headerData(
        int section, Qt::Orientation orientation,
        int role = Qt::DisplayRole) const override;

    virtual int rowCount(
        const QModelIndex & parent = QModelIndex()) const override;

    virtual int columnCount(
        const QModelIndex & parent = QModelIndex()) const override;

    virtual QModelIndex index(
        int row, int column,
        const QModelIndex & parent = QModelIndex()) const override;

    virtual QModelIndex parent(const QModelIndex & index) const override;

    virtual bool setHeaderData(
        int section, Qt::Orientation orientation, const QVariant & value,
        int role = Qt::EditRole) override;

    virtual bool setData(
        const QModelIndex & index, const QVariant & value,
        int role = Qt::EditRole) override;

    virtual bool insertRows(
        int row, int count,
        const QModelIndex & parent = QModelIndex()) override;

    virtual bool removeRows(
        int row, int count,
        const QModelIndex & parent = QModelIndex()) override;

    virtual void sort(int column, Qt::SortOrder order) override;

    // Drag-n-drop interfaces
    virtual Qt::DropActions supportedDragActions() const override
    {
        return Qt::MoveAction;
    }

    virtual Qt::DropActions supportedDropActions() const override
    {
        return Qt::MoveAction;
    }

    virtual QStringList mimeTypes() const override;

    virtual QMimeData * mimeData(
        const QModelIndexList & indexes) const override;

    virtual bool dropMimeData(
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
    void addNotebook(Notebook notebook, QUuid requestId);
    void updateNotebook(Notebook notebook, QUuid requestId);
    void findNotebook(Notebook notebook, QUuid requestId);

    void listNotebooks(
        LocalStorageManager::ListObjectsOptions flag, size_t limit,
        size_t offset, LocalStorageManager::ListNotebooksOrder order,
        LocalStorageManager::OrderDirection orderDirection,
        QString linkedNotebookGuid, QUuid requestId);

    void expungeNotebook(Notebook notebook, QUuid requestId);

    void requestNoteCountPerNotebook(
        Notebook notebook, LocalStorageManager::NoteCountOptions options,
        QUuid requestId);

    void listAllLinkedNotebooks(
        const size_t limit, const size_t offset,
        const LocalStorageManager::ListLinkedNotebooksOrder order,
        const LocalStorageManager::OrderDirection orderDirection,
        QUuid requestId);

private Q_SLOTS:
    // Slots for response to events from local storage
    void onAddNotebookComplete(Notebook notebook, QUuid requestId);

    void onAddNotebookFailed(
        Notebook notebook, ErrorString errorDescription, QUuid requestId);

    void onUpdateNotebookComplete(Notebook notebook, QUuid requestId);

    void onUpdateNotebookFailed(
        Notebook notebook, ErrorString errorDescription, QUuid requestId);

    void onFindNotebookComplete(Notebook notebook, QUuid requestId);

    void onFindNotebookFailed(
        Notebook notebook, ErrorString errorDescription, QUuid requestId);

    void onListNotebooksComplete(
        LocalStorageManager::ListObjectsOptions flag, size_t limit,
        size_t offset, LocalStorageManager::ListNotebooksOrder order,
        LocalStorageManager::OrderDirection orderDirection,
        QString linkedNotebookGuid, QList<Notebook> foundNotebooks,
        QUuid requestId);

    void onListNotebooksFailed(
        LocalStorageManager::ListObjectsOptions flag, size_t limit,
        size_t offset, LocalStorageManager::ListNotebooksOrder order,
        LocalStorageManager::OrderDirection orderDirection,
        QString linkedNotebookGuid, ErrorString errorDescription,
        QUuid requestId);

    void onExpungeNotebookComplete(Notebook notebook, QUuid requestId);

    void onExpungeNotebookFailed(
        Notebook notebook, ErrorString errorDescription, QUuid requestId);

    void onGetNoteCountPerNotebookComplete(
        int noteCount, Notebook notebook,
        LocalStorageManager::NoteCountOptions options, QUuid requestId);

    void onGetNoteCountPerNotebookFailed(
        ErrorString errorDescription, Notebook notebook,
        LocalStorageManager::NoteCountOptions options, QUuid requestId);

    void onAddNoteComplete(Note note, QUuid requestId);

    void onNoteMovedToAnotherNotebook(
        QString noteLocalUid, QString previousNotebookLocalUid,
        QString newNotebookLocalUid);

    void onExpungeNoteComplete(Note note, QUuid requestId);

    void onAddLinkedNotebookComplete(
        LinkedNotebook linkedNotebook, QUuid requestId);

    void onUpdateLinkedNotebookComplete(
        LinkedNotebook linkedNotebook, QUuid requestId);

    void onExpungeLinkedNotebookComplete(
        LinkedNotebook linkedNotebook, QUuid requestId);

    void onListAllLinkedNotebooksComplete(
        size_t limit, size_t offset,
        LocalStorageManager::ListLinkedNotebooksOrder order,
        LocalStorageManager::OrderDirection orderDirection,
        QList<LinkedNotebook> foundLinkedNotebooks, QUuid requestId);

    void onListAllLinkedNotebooksFailed(
        size_t limit, size_t offset,
        LocalStorageManager::ListLinkedNotebooksOrder order,
        LocalStorageManager::OrderDirection orderDirection,
        ErrorString errorDescription, QUuid requestId);

private:
    void createConnections(LocalStorageManagerAsync & localStorageManagerAsync);
    void requestNotebooksList();
    void requestNoteCountForNotebook(const Notebook & notebook);
    void requestNoteCountForAllNotebooks();
    void requestLinkedNotebooksList();

    QVariant dataImpl(
        const INotebookModelItem & item, const Column column) const;

    QVariant dataAccessibleText(
        const INotebookModelItem & item, const Column column) const;

    bool canUpdateNotebookItem(const NotebookItem & item) const;

    void updateNotebookInLocalStorage(const NotebookItem & item);
    void expungeNotebookFromLocalStorage(const QString & localUid);

    QString nameForNewNotebook() const;

    void removeItemByLocalUid(const QString & localUid);
    void notebookToItem(const Notebook & notebook, NotebookItem & item);

    enum class RemoveEmptyParentStack
    {
        Yes,
        No
    };

    friend QDebug & operator<<(
        QDebug & dbg, const RemoveEmptyParentStack option);

    void removeModelItemFromParent(
        INotebookModelItem & modelItem,
        RemoveEmptyParentStack option = RemoveEmptyParentStack::Yes);

    // Returns the appropriate row before which the new item should be inserted
    // according to the current sorting criteria and column
    int rowForNewItem(
        const INotebookModelItem & parentItem,
        const INotebookModelItem & newItem) const;

    void updateItemRowWithRespectToSorting(INotebookModelItem & modelItem);

    void updatePersistentModelIndices();

    // Returns true if successfully incremented the note count for the notebook
    // item with the corresponding local uid
    bool incrementNoteCountForNotebook(const QString & notebookLocalUid);

    // Returns true if successfully decremented the note count for the notebook
    // item with the corresponding local uid
    bool decrementNoteCountForNotebook(const QString & notebookLocalUid);

    void switchDefaultNotebookLocalUid(const QString & localUid);
    void switchLastUsedNotebookLocalUid(const QString & localUid);

    void checkAndRemoveEmptyStackItem(INotebookModelItem & modelItem);

    void setNotebookFavorited(const QModelIndex & index, const bool favorited);

    void beginRemoveNotebooks();
    void endRemoveNotebooks();

    INotebookModelItem & findOrCreateLinkedNotebookModelItem(
        const QString & linkedNotebookGuid);

    void checkAndCreateModelRootItems();

private:
    struct ByLocalUid
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
                boost::multi_index::tag<ByLocalUid>,
                boost::multi_index::const_mem_fun<
                    NotebookItem, const QString &, &NotebookItem::localUid>>,
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

    using NotebookDataByLocalUid = NotebookData::index<ByLocalUid>::type;
    using NotebookDataByNameUpper = NotebookData::index<ByNameUpper>::type;
    using NotebookDataByStack = NotebookData::index<ByStack>::type;

    using NotebookDataByLinkedNotebookGuid =
        NotebookData::index<ByLinkedNotebookGuid>::type;

    struct LessByName
    {
        bool operator()(
            const NotebookItem & lhs, const NotebookItem & rhs) const;

        bool operator()(
            const NotebookItem * pLhs, const NotebookItem * pRhs) const;

        bool operator()(const StackItem & lhs, const StackItem & rhs) const;

        bool operator()(const StackItem * pLhs, const StackItem * pRhs) const;

        bool operator()(
            const LinkedNotebookRootItem & lhs,
            const LinkedNotebookRootItem & rhs) const;

        bool operator()(
            const LinkedNotebookRootItem * lhs,
            const LinkedNotebookRootItem * rhs) const;

        bool operator()(
            const INotebookModelItem & lhs,
            const INotebookModelItem & rhs) const;

        bool operator()(
            const INotebookModelItem * pLhs,
            const INotebookModelItem * pRhs) const;
    };

    struct GreaterByName
    {
        bool operator()(
            const NotebookItem & lhs, const NotebookItem & rhs) const;

        bool operator()(
            const NotebookItem * pLhs, const NotebookItem * pRhs) const;

        bool operator()(const StackItem & lhs, const StackItem & rhs) const;

        bool operator()(const StackItem * pLhs, const StackItem * pRhs) const;

        bool operator()(
            const LinkedNotebookRootItem & lhs,
            const LinkedNotebookRootItem & rhs) const;

        bool operator()(
            const LinkedNotebookRootItem * pLhs,
            const LinkedNotebookRootItem * pRhs) const;

        bool operator()(
            const INotebookModelItem & lhs,
            const INotebookModelItem & rhs) const;

        bool operator()(
            const INotebookModelItem * pLhs,
            const INotebookModelItem * pRhs) const;
    };

    using ModelItems = QMap<QString, INotebookModelItem *>;
    using StackItems = QMap<QString, StackItem>;
    using LinkedNotebookItems = QMap<QString, LinkedNotebookRootItem>;

    using IndexId = quintptr;

    using IndexIdToLocalUidBimap = boost::bimap<IndexId, QString>;

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
    void onNotebookAddedOrUpdated(const Notebook & notebook);
    void onNotebookAdded(const Notebook & notebook);

    void onNotebookUpdated(
        const Notebook & notebook, NotebookDataByLocalUid::iterator it);

    void onLinkedNotebookAddedOrUpdated(const LinkedNotebook & linkedNotebook);

    INotebookModelItem * itemForId(const IndexId id) const;
    IndexId idForItem(const INotebookModelItem & item) const;

    Qt::ItemFlags adjustFlagsForColumn(
        const int column, Qt::ItemFlags flags) const;

    // Helper methods for notebook items

    bool setNotebookData(
        NotebookItem & notebookItem, const QModelIndex & modelIndex,
        const QVariant & value);

    bool setNotebookName(NotebookItem & notebookItem, const QString & newName);

    bool setNotebookSynchronizable(
        NotebookItem & notebookItem, const bool synchronizable);

    bool setNotebookIsDefault(
        NotebookItem & notebookItem, const bool isDefault);

    bool setNotebookIsLastUsed(
        NotebookItem & notebookItem, const bool isLastUsed);

    QVariant notebookData(
        const NotebookItem & notebookItem, const Column column) const;

    QVariant notebookAccessibleData(
        const NotebookItem & notebookItem, const Column column) const;

    bool canRemoveNotebookItem(const NotebookItem & notebookItem);

    // Returns true if successfully incremented the note count for the notebook
    // item with the corresponding local uid
    bool updateNoteCountPerNotebookIndex(
        const NotebookItem & item, const NotebookDataByLocalUid::iterator it);

    bool notebookItemMatchesByLinkedNotebook(
        const NotebookItem & item, const QString & linkedNotebookGuid) const;

    Qt::ItemFlags flagsForNotebookItem(
        const NotebookItem & notebookItem, const int column,
        Qt::ItemFlags flags) const;

    // Helper methods for stack items

    bool setStackData(
        StackItem & stackItem, const QModelIndex & modelIndex,
        const QVariant & value);

    QVariant stackData(const StackItem & stackItem, const Column column) const;

    QVariant stackAccessibleData(
        const StackItem & stackItem, const Column column) const;

    const StackItems * stackItems(const QString & linkedNotebookGuid) const;

    std::pair<StackItems *, INotebookModelItem *> stackItemsWithParent(
        const QString & linkedNotebookGuid);

    StackItem & findOrCreateStackItem(
        const QString & stack, StackItems & stackItems,
        INotebookModelItem * pParentItem);

    Qt::ItemFlags flagsForStackItem(
        const StackItem & stackItem, const int column,
        Qt::ItemFlags flags) const;

private:
    NotebookData m_data;

    INotebookModelItem * m_pInvisibleRootItem = nullptr;

    INotebookModelItem * m_pAllNotebooksRootItem = nullptr;
    IndexId m_allNotebooksRootItemIndexId = 1;

    QString m_defaultNotebookLocalUid;
    QString m_lastUsedNotebookLocalUid;

    StackItems m_stackItems;
    QMap<QString, StackItems> m_stackItemsByLinkedNotebookGuid;

    LinkedNotebookItems m_linkedNotebookItems;

    mutable IndexIdToLocalUidBimap m_indexIdToLocalUidBimap;
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
