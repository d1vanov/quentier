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

#ifndef QUENTIER_MODELS_NOTEBOOK_MODEL_H
#define QUENTIER_MODELS_NOTEBOOK_MODEL_H

#include "ItemModel.h"
#include "NotebookModelItem.h"
#include "NotebookCache.h"
#include <quentier/local_storage/LocalStorageManagerAsync.h>
#include <quentier/types/Account.h>
#include <QAbstractItemModel>
#include <QUuid>
#include <QSet>
#include <QMap>
#include <QHash>
#include <QFlags>

// NOTE: Workaround a bug in Qt4 which may prevent building with some boost versions
#ifndef Q_MOC_RUN
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/bimap.hpp>
#endif

#define NOTEBOOK_MODEL_MIME_TYPE QStringLiteral("application/x-com.quentier.notebookmodeldatalist")
#define NOTEBOOK_MODEL_MIME_DATA_MAX_COMPRESSION (9)

namespace quentier {

QT_FORWARD_DECLARE_CLASS(NoteModel)

class NotebookModel: public ItemModel
{
    Q_OBJECT
public:
    explicit NotebookModel(const Account & account, const NoteModel & noteModel,
                           LocalStorageManagerAsync & localStorageManagerAsync,
                           NotebookCache & cache, QObject * parent = Q_NULLPTR);
    virtual ~NotebookModel();

    const Account & account() const { return m_account; }
    void updateAccount(const Account & account);

    struct Columns
    {
        enum type {
            Name = 0,
            Synchronizable,
            Dirty,
            Default,
            LastUsed,
            Published,
            FromLinkedNotebook,
            NumNotesPerNotebook
        };
    };

    const NotebookModelItem * itemForIndex(const QModelIndex & index) const;
    QModelIndex indexForItem(const NotebookModelItem * item) const;

    QModelIndex indexForLocalUid(const QString & localUid) const;

    QModelIndex indexForNotebookName(const QString & notebookName,
                                     const QString & linkedNotebookGuid = QString()) const;

    /**
     * @param The name of the notebook stack which item's index is required
     * @param The linked notebook guid for which the notebook stack item's index is required;
     * if null or empty, the item corresponding to the notebook stack from user's own account it considered,
     * otherwise the item corresponding linked notebook guid would be considered
     * @return The model index of the NotebookStackItem corresponding to the given stack
     */
    QModelIndex indexForNotebookStack(const QString & stack, const QString & linkedNotebookGuid) const;

    /**
     * @param The linked notebook guid which corresponding linked notebook item's index is required
     * @return The model index of the NotebookLinkedNotebookRootItem corresponding to the given linked notebook guid
     */
    QModelIndex indexForLinkedNotebookGuid(const QString & linkedNotebookGuid) const;

    /**
     * C++98-style struct wrapper for the enum defining filter for methods listing stuff
     * related to multiple notebook items
     */
    struct NotebookFilter
    {
        enum type {
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
    };

    Q_DECLARE_FLAGS(NotebookFilters, NotebookFilter::type)

    /**
     * @param filter - filter for the listed notebook names
     * @param linkedNotebookGuid - the option guid of the linked notebook to which the returned notebook names belong;
     * if it is null (i.e. linkedNotebookGuid.isNull() returns true), the notebook names would be returned ignoring
     * their belonging to user's own account or linked notebook; if it's not null but empty (i.e. linkedNotebookGuid.isEmpty()
     * returns true), only the names of notebooks from user's own account would be returned. Otherwise only the names
     * of notebooks from the corresponding linked notebook would be returned
     * @return The sorted list of notebook names conformant with the filter
     */
    QStringList notebookNames(const NotebookFilters filters, const QString & linkedNotebookGuid = QString()) const;

    /**
     * @return The list of indexes stored as persistent indexes in the model
     */
    QModelIndexList persistentIndexes() const;

    /**
     * @return the index of the default notebook item if such one exists
     * or invalid model index otherwise
     */
    QModelIndex defaultNotebookIndex() const;

    /**
     * @brief lastUsedNotebookIndex
     * @return the index of the last used notebook item if such one exists
     * or invalid model index otherwise
     */
    QModelIndex lastUsedNotebookIndex() const;

    /**
     * @brief moveToStack - moves the notebook item pointed to by the index to the specified stack
     * @param index - the index of the notebook item to be moved to the stack
     * @param stack - the stack to which the notebook item needs to be moved
     * @return the index of the moved notebook item or invalid index if something went wrong,
     * for example, if the passed in index did not really point to the notebook item
     *
     * @note There is no ambiguity here between stacks from user's own account and those from linked notebooks -
     * the notebook item would be moved to the stack which it corresponds to: if the notebook belongs to user's
     * own account, the stack would also be from user's own account; otherwise, the stack would be that
     * of the corresponding linked notebook
     */
    QModelIndex moveToStack(const QModelIndex & index, const QString & stack);

    /**
     * @brief removeFromStack - removes the notebook item pointed to by the index from its stack (if any)
     * @param index - the index of the notebook item to be removed from its stack
     * @return the index of the notebook item removed from its stack or invalid index
     * if something went wrong, for example, if the passed in index did not really point
     * to the notebook item
     */
    QModelIndex removeFromStack(const QModelIndex & index);

    /**
     * @brief stacks
     * @param linkedNotebookGuid - the guid of the linked notebook from which the stacks are requested;
     * if empty, the stacks from user's own account are returned
     * @return the sorted list of existing notebook stacks
     */
    QStringList stacks(const QString & linkedNotebookGuid = QString()) const;

    /**
     * @brief createNotebook - convenience method to create a new notebook within the model
     * @param notebookName - the name of the new notebook
     * @param notebookStack - the stack of the new notebook
     * @param errorDescription - the textual description of the error if notebook
     * was not created successfully
     * @return either valid model index if notebook was created successfully or
     * invalid model index otherwise
     */
    QModelIndex createNotebook(const QString & notebookName, const QString & notebookStack,
                               ErrorString & errorDescription);

    /**
     * @brief columnName
     * @param column - the column which name needs to be returned
     * @return the name of the column
     */
    QString columnName(const Columns::type column) const;

    /**
     * @brief allNotebooksListed
     * @return true if the notebook model has received the information it needs about all notebooks
     * stored in the local storage by the moment; false otherwise
     */
    bool allNotebooksListed() const;

    /**
     * @brief favoriteNotebook - marks the notebook pointed to by the index as favorited
     *
     * Favorited property of @link Notebook @endlink class is not represented as a column within
     * the @link NotebookModel @endlink so this method doesn't change anything in the model but only
     * the underlying notebook object persisted in the local storage
     *
     * @param index - the index of the notebook to be favorited
     */
    void favoriteNotebook(const QModelIndex & index);

    /**
     * @brief unfavoriteNotebook - removes the favorited mark from the notebook pointed to by the index; does nothing
     * if the notebook was not favorited prior to the call
     *
     * Favorited property of @link Notebook @endlink class is not represented as a column within
     * the @link NotebookModel @endlink so this method doesn't change anything in the model but only
     * the underlying notebook object persisted in the local storage
     *
     * @param index - the index of the notebook to be unfavorited
     */
    void unfavoriteNotebook(const QModelIndex & index);

public:
    // ItemModel interface
    virtual QString localUidForItemName(const QString & itemName) const Q_DECL_OVERRIDE;
    virtual QString itemNameForLocalUid(const QString & localUid) const Q_DECL_OVERRIDE;
    virtual QStringList itemNames() const Q_DECL_OVERRIDE;
    virtual int nameColumn() const Q_DECL_OVERRIDE { return Columns::Name; }
    virtual int sortingColumn() const Q_DECL_OVERRIDE { return m_sortedColumn; }
    virtual Qt::SortOrder sortOrder() const Q_DECL_OVERRIDE { return m_sortOrder; }
    virtual bool allItemsListed() const Q_DECL_OVERRIDE { return allNotebooksListed(); }

public:
    // QAbstractItemModel interface
    virtual Qt::ItemFlags flags(const QModelIndex & index) const Q_DECL_OVERRIDE;
    virtual QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;

    virtual int rowCount(const QModelIndex & parent = QModelIndex()) const Q_DECL_OVERRIDE;
    virtual int columnCount(const QModelIndex & parent = QModelIndex()) const Q_DECL_OVERRIDE;
    virtual QModelIndex index(int row, int column, const QModelIndex & parent = QModelIndex()) const Q_DECL_OVERRIDE;
    virtual QModelIndex parent(const QModelIndex & index) const Q_DECL_OVERRIDE;

    virtual bool setHeaderData(int section, Qt::Orientation orientation, const QVariant & value, int role = Qt::EditRole) Q_DECL_OVERRIDE;
    virtual bool setData(const QModelIndex & index, const QVariant & value, int role = Qt::EditRole) Q_DECL_OVERRIDE;
    virtual bool insertRows(int row, int count, const QModelIndex & parent = QModelIndex()) Q_DECL_OVERRIDE;
    virtual bool removeRows(int row, int count, const QModelIndex & parent = QModelIndex()) Q_DECL_OVERRIDE;

    virtual void sort(int column, Qt::SortOrder order) Q_DECL_OVERRIDE;

    // Drag-n-drop interfaces
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    virtual Qt::DropActions supportedDragActions() const Q_DECL_OVERRIDE { return Qt::MoveAction; }
#else
    Qt::DropActions supportedDragActions() const { return Qt::MoveAction; }
#endif

    virtual Qt::DropActions supportedDropActions() const Q_DECL_OVERRIDE { return Qt::MoveAction; }
    virtual QStringList mimeTypes() const Q_DECL_OVERRIDE;
    virtual QMimeData * mimeData(const QModelIndexList & indexes) const Q_DECL_OVERRIDE;
    virtual bool dropMimeData(const QMimeData * data, Qt::DropAction action,
                              int row, int column, const QModelIndex & parent) Q_DECL_OVERRIDE;

Q_SIGNALS:
    void notifyError(ErrorString errorDescription);

    /**
     * @brief notifyAllNotebooksListed - the signal emitted when the notebook model
     * has received the information on all notebooks stored in the local storage
     */
    void notifyAllNotebooksListed();

    /**
     * @brief notifyNotebookStackRenamed - the signal emitted after the notebook stack
     * has been renamed
     * @param previousStackName - the name of the notebook stack prior to renaming
     * @param newStackName - the new name of the notebook stack
     * @param linkedNotebookGuid - if the stack belonged to the linked notebook, this parameter is the guid of it
     */
    void notifyNotebookStackRenamed(const QString & previousStackName,
                                    const QString & newStackName,
                                    const QString & linkedNotebookGuid);

    /**
     * @brief notifyNotebookStackChanged - the signal emitted after the notebook has been assigned to another stack
     * @param notebookIndex - the model index of the notebook which stack has been changed
     */
    void notifyNotebookStackChanged(const QModelIndex & notebookIndex);

    // Informative signals for views, so that they can prepare to the changes in the tree of notebooks/stacks
    // and do some recovery after that
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
    void listNotebooks(LocalStorageManager::ListObjectsOptions flag,
                       size_t limit, size_t offset,
                       LocalStorageManager::ListNotebooksOrder::type order,
                       LocalStorageManager::OrderDirection::type orderDirection,
                       QString linkedNotebookGuid, QUuid requestId);
    void expungeNotebook(Notebook notebook, QUuid requestId);
    void requestNoteCountPerNotebook(Notebook notebook, QUuid requestId);

    void listAllLinkedNotebooks(const size_t limit, const size_t offset,
                                const LocalStorageManager::ListLinkedNotebooksOrder::type order,
                                const LocalStorageManager::OrderDirection::type orderDirection, QUuid requestId);

private Q_SLOTS:
    void onAllNotesListed();

    // Slots for response to events from local storage
    void onAddNotebookComplete(Notebook notebook, QUuid requestId);
    void onAddNotebookFailed(Notebook notebook, ErrorString errorDescription, QUuid requestId);
    void onUpdateNotebookComplete(Notebook notebook, QUuid requestId);
    void onUpdateNotebookFailed(Notebook notebook, ErrorString errorDescription, QUuid requestId);
    void onFindNotebookComplete(Notebook notebook, QUuid requestId);
    void onFindNotebookFailed(Notebook notebook, ErrorString errorDescription, QUuid requestId);
    void onListNotebooksComplete(LocalStorageManager::ListObjectsOptions flag,
                                 size_t limit, size_t offset,
                                 LocalStorageManager::ListNotebooksOrder::type order,
                                 LocalStorageManager::OrderDirection::type orderDirection,
                                 QString linkedNotebookGuid, QList<Notebook> foundNotebooks,
                                 QUuid requestId);
    void onListNotebooksFailed(LocalStorageManager::ListObjectsOptions flag,
                               size_t limit, size_t offset,
                               LocalStorageManager::ListNotebooksOrder::type order,
                               LocalStorageManager::OrderDirection::type orderDirection,
                               QString linkedNotebookGuid, ErrorString errorDescription, QUuid requestId);
    void onExpungeNotebookComplete(Notebook notebook, QUuid requestId);
    void onExpungeNotebookFailed(Notebook notebook, ErrorString errorDescription, QUuid requestId);

    void onGetNoteCountPerNotebookComplete(int noteCount, Notebook notebook, QUuid requestId);
    void onGetNoteCountPerNotebookFailed(ErrorString errorDescription, Notebook notebook, QUuid requestId);

    void onAddNoteComplete(Note note, QUuid requestId);
    void onUpdateNoteComplete(Note note, bool updateResources, bool updateTags, QUuid requestId);
    void onExpungeNoteComplete(Note note, QUuid requestId);

    void onAddLinkedNotebookComplete(LinkedNotebook linkedNotebook, QUuid requestId);
    void onUpdateLinkedNotebookComplete(LinkedNotebook linkedNotebook, QUuid requestId);
    void onExpungeLinkedNotebookComplete(LinkedNotebook linkedNotebook, QUuid requestId);

    void onListAllLinkedNotebooksComplete(size_t limit, size_t offset,
                                          LocalStorageManager::ListLinkedNotebooksOrder::type order,
                                          LocalStorageManager::OrderDirection::type orderDirection,
                                          QList<LinkedNotebook> foundLinkedNotebooks,
                                          QUuid requestId);
    void onListAllLinkedNotebooksFailed(size_t limit, size_t offset,
                                        LocalStorageManager::ListLinkedNotebooksOrder::type order,
                                        LocalStorageManager::OrderDirection::type orderDirection,
                                        ErrorString errorDescription, QUuid requestId);

private:
    void createConnections(const NoteModel & noteModel, LocalStorageManagerAsync & localStorageManagerAsync);
    void requestNotebooksList();
    void requestNoteCountForNotebook(const Notebook & notebook);
    void requestNoteCountForAllNotebooks();
    void requestLinkedNotebooksList();

    QVariant dataImpl(const NotebookModelItem & item, const Columns::type column) const;
    QVariant dataAccessibleText(const NotebookModelItem & item, const Columns::type column) const;

    bool canUpdateNotebookItem(const NotebookItem & item) const;

    void updateNotebookInLocalStorage(const NotebookItem & item);
    void expungeNotebookFromLocalStorage(const QString & localUid);

    QString nameForNewNotebook() const;

    void removeItemByLocalUid(const QString & localUid);
    void notebookToItem(const Notebook & notebook, NotebookItem & item);

    void removeModelItemFromParent(const NotebookModelItem & modelItem);

    // Returns the appropriate row before which the new item should be inserted according to the current sorting criteria and column
    int rowForNewItem(const NotebookModelItem & parentItem, const NotebookModelItem & newItem) const;

    void updateItemRowWithRespectToSorting(const NotebookModelItem & modelItem);

    void updatePersistentModelIndices();

    // Returns true if successfully incremented the note count for the notebook item with the corresponding local uid
    bool onAddNoteWithNotebookLocalUid(const QString & notebookLocalUid);

    // Returns true if successfully decremented the note count for the notebook item with the corresponding local uid
    bool onExpungeNoteWithNotebookLocalUid(const QString & notebookLocalUid);

    void switchDefaultNotebookLocalUid(const QString & localUid);
    void switchLastUsedNotebookLocalUid(const QString & localUid);

    void checkAndRemoveEmptyStackItem(const NotebookModelItem & modelItem);

    void setNotebookFavorited(const QModelIndex & index, const bool favorited);

    void beginRemoveNotebooks();
    void endRemoveNotebooks();

    void buildNotebookLocalUidByNoteLocalUidsHash(const NoteModel & noteModel);

    const NotebookModelItem & findOrCreateLinkedNotebookModelItem(const QString & linkedNotebookGuid);

private:
    struct ByLocalUid{};
    struct ByNameUpper{};
    struct ByStack{};
    struct ByLinkedNotebookGuid{};

    typedef boost::multi_index_container<
        NotebookItem,
        boost::multi_index::indexed_by<
            boost::multi_index::ordered_unique<
                boost::multi_index::tag<ByLocalUid>,
                boost::multi_index::const_mem_fun<NotebookItem,const QString&,&NotebookItem::localUid>
            >,
            boost::multi_index::ordered_non_unique<
                boost::multi_index::tag<ByNameUpper>,
                boost::multi_index::const_mem_fun<NotebookItem,QString,&NotebookItem::nameUpper>
            >,
            boost::multi_index::ordered_non_unique<
                boost::multi_index::tag<ByStack>,
                boost::multi_index::const_mem_fun<NotebookItem,const QString&,&NotebookItem::stack>
            >,
            boost::multi_index::ordered_non_unique<
                boost::multi_index::tag<ByLinkedNotebookGuid>,
                boost::multi_index::const_mem_fun<NotebookItem,const QString&,&NotebookItem::linkedNotebookGuid>
            >
        >
    > NotebookData;

    typedef NotebookData::index<ByLocalUid>::type NotebookDataByLocalUid;
    typedef NotebookData::index<ByNameUpper>::type NotebookDataByNameUpper;
    typedef NotebookData::index<ByStack>::type NotebookDataByStack;
    typedef NotebookData::index<ByLinkedNotebookGuid>::type NotebookDataByLinkedNotebookGuid;

    struct LessByName
    {
        bool operator()(const NotebookItem & lhs, const NotebookItem & rhs) const;
        bool operator()(const NotebookItem * lhs, const NotebookItem * rhs) const;

        bool operator()(const NotebookStackItem & lhs, const NotebookStackItem & rhs) const;
        bool operator()(const NotebookStackItem * lhs, const NotebookStackItem * rhs) const;

        bool operator()(const NotebookModelItem & lhs, const NotebookModelItem & rhs) const;
        bool operator()(const NotebookModelItem * lhs, const NotebookModelItem * rhs) const;
    };

    struct GreaterByName
    {
        bool operator()(const NotebookItem & lhs, const NotebookItem & rhs) const;
        bool operator()(const NotebookItem * lhs, const NotebookItem * rhs) const;

        bool operator()(const NotebookStackItem & lhs, const NotebookStackItem & rhs) const;
        bool operator()(const NotebookStackItem * lhs, const NotebookStackItem * rhs) const;

        bool operator()(const NotebookModelItem & lhs, const NotebookModelItem & rhs) const;
        bool operator()(const NotebookModelItem * lhs, const NotebookModelItem * rhs) const;
    };

    typedef QMap<QString, NotebookModelItem> ModelItems;
    typedef QMap<QString, NotebookStackItem> StackItems;
    typedef QMap<QString, NotebookLinkedNotebookRootItem> LinkedNotebookItems;

#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
    typedef quint32 IndexId;
#else
    typedef quintptr IndexId;
#endif

    typedef boost::bimap<IndexId, QString> IndexIdToLocalUidBimap;
    typedef boost::bimap<IndexId, QString> IndexIdToStackBimap;
    typedef boost::bimap<IndexId, QString> IndexIdToLinkedNotebookGuidBimap;

    class RemoveRowsScopeGuard
    {
    public:
        RemoveRowsScopeGuard(NotebookModel & model);
        ~RemoveRowsScopeGuard();

    private:
        NotebookModel &     m_model;
    };

    friend class RemoveRowsScopeGuard;

private:
    void onNotebookAddedOrUpdated(const Notebook & notebook);
    void onNotebookAdded(const Notebook & notebook);
    void onNotebookUpdated(const Notebook & notebook, NotebookDataByLocalUid::iterator it);

    void onLinkedNotebookAddedOrUpdated(const LinkedNotebook & linkedNotebook);

    const NotebookModelItem * itemForId(const IndexId id) const;
    IndexId idForItem(const NotebookModelItem & item) const;

    // Returns true if successfully incremented the note count for the notebook item with the corresponding local uid
    bool updateNoteCountPerNotebookIndex(const NotebookItem & item, const NotebookDataByLocalUid::iterator it);

    ModelItems::iterator addNewStackModelItem(const NotebookStackItem & stackItem,
                                              const NotebookModelItem & parentItem,
                                              ModelItems & modelItemsByStack);

private:
    Account                 m_account;

    NotebookData            m_data;
    NotebookModelItem *     m_fakeRootItem;

    QString                 m_defaultNotebookLocalUid;
    QString                 m_lastUsedNotebookLocalUid;

    ModelItems                  m_modelItemsByLocalUid;
    ModelItems                  m_modelItemsByStack;
    ModelItems                  m_modelItemsByLinkedNotebookGuid;
    QMap<QString, ModelItems>   m_modelItemsByStackByLinkedNotebookGuid;

    StackItems                  m_stackItems;
    QMap<QString, StackItems>   m_stackItemsByLinkedNotebookGuid;

    LinkedNotebookItems         m_linkedNotebookItems;

    mutable IndexIdToLocalUidBimap              m_indexIdToLocalUidBimap;
    mutable IndexIdToStackBimap                 m_indexIdToStackBimap;
    mutable IndexIdToLinkedNotebookGuidBimap    m_indexIdToLinkedNotebookGuidBimap;
    mutable IndexId                             m_lastFreeIndexId;

    NotebookCache &         m_cache;

    size_t                  m_listNotebooksOffset;
    QUuid                   m_listNotebooksRequestId;
    QSet<QUuid>             m_notebookItemsNotYetInLocalStorageUids;

    QSet<QUuid>             m_addNotebookRequestIds;
    QSet<QUuid>             m_updateNotebookRequestIds;
    QSet<QUuid>             m_expungeNotebookRequestIds;

    QSet<QUuid>             m_findNotebookToRestoreFailedUpdateRequestIds;
    QSet<QUuid>             m_findNotebookToPerformUpdateRequestIds;

    QSet<QUuid>             m_noteCountPerNotebookRequestIds;

    QHash<QString, QString> m_notebookLocalUidByNoteLocalUid;
    bool                    m_receivedNotebookLocalUidsForAllNotes;

    QHash<QString,QString>  m_linkedNotebookOwnerUsernamesByLinkedNotebookGuids;
    size_t                  m_listLinkedNotebooksOffset;
    QUuid                   m_listLinkedNotebooksRequestId;

    Columns::type           m_sortedColumn;
    Qt::SortOrder           m_sortOrder;

    mutable int             m_lastNewNotebookNameCounter;

    bool                    m_allNotebooksListed;
    bool                    m_allLinkedNotebooksListed;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(NotebookModel::NotebookFilters)

} // namespace quentier

#endif // QUENTIER_MODELS_NOTEBOOK_MODEL_H
