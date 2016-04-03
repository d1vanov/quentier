#ifndef __QUTE_NOTE__MODELS__NOTEBOOK_MODEL_H
#define __QUTE_NOTE__MODELS__NOTEBOOK_MODEL_H

#include "NotebookModelItem.h"
#include <qute_note/types/Notebook.h>
#include <qute_note/local_storage/LocalStorageManagerThreadWorker.h>
#include <qute_note/utility/LRUCache.hpp>
#include <QAbstractItemModel>
#include <QUuid>
#include <QSet>
#include <QHash>

// NOTE: Workaround a bug in Qt4 which may prevent building with some boost versions
#ifndef Q_MOC_RUN
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index/ordered_index.hpp>
#endif

#define NOTEBOOK_MODEL_MIME_TYPE "application/x-com.qute_note.notebookmodeldatalist"
#define NOTEBOOK_MODEL_DATA_MAX_COMPRESSION (9)

namespace qute_note {

class NotebookModel: public QAbstractItemModel
{
    Q_OBJECT
public:
    explicit NotebookModel(LocalStorageManagerThreadWorker & localStorageManagerThreadWorker,
                           QObject * parent = Q_NULLPTR);
    virtual ~NotebookModel();

    struct Columns
    {
        enum type {
            Name = 0,
            Synchronizable,
            Dirty,
            Default,
            Published,
            FromLinkedNotebook
        };
    };

    const NotebookModelItem * itemForIndex(const QModelIndex & index) const;
    QModelIndex indexForItem(const NotebookModelItem * item) const;
    QModelIndex indexForLocalUid(const QString & localUid) const;

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

Q_SIGNALS:
    void notifyError(QString errorDescription);

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

private Q_SLOTS:
    // Slots for response to events from local storage
    void onAddNotebookComplete(Notebook notebook, QUuid requestId);
    void onAddNotebookFailed(Notebook notebook, QString errorDescription, QUuid requestId);
    void onUpdateNotebookComplete(Notebook notebook, QUuid requestId);
    void onUpdateNotebookFailed(Notebook notebook, QString errorDescription, QUuid requestId);
    void onFindNotebookComplete(Notebook notebook, QUuid requestId);
    void onFindNotebookFailed(Notebook notebook, QString errorDescription, QUuid requestId);
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
                               QString linkedNotebookGuid, QString errorDescription, QUuid requestId);
    void onExpungeNotebookComplete(Notebook notebook, QUuid requestId);
    void onExpungeNotebookFailed(Notebook notebook, QString errorDescription, QUuid requestId);

private:
    void createConnections(LocalStorageManagerThreadWorker & localStorageManagerThreadWorker);
    void requestNotebooksList();

    QVariant dataText(const NotebookModelItem & item, const Columns::type column) const;
    QVariant dataAccessibleText(const NotebookModelItem & item, const Columns::type column) const;

    bool canUpdateNotebookItem(const NotebookItem & item) const;

    void updateNotebookInLocalStorage(const NotebookItem & item);

    QString nameForNewNotebook() const;

private:
    struct ByLocalUid{};
    struct ByNameUpper{};
    struct ByStack{};

    typedef boost::multi_index_container<
        NotebookItem,
        boost::multi_index::indexed_by<
            boost::multi_index::ordered_unique<
                boost::multi_index::tag<ByLocalUid>,
                boost::multi_index::const_mem_fun<NotebookItem,const QString&,&NotebookItem::localUid>
            >,
            boost::multi_index::ordered_unique<
                boost::multi_index::tag<ByNameUpper>,
                boost::multi_index::const_mem_fun<NotebookItem,QString,&NotebookItem::nameUpper>
            >,
            boost::multi_index::ordered_non_unique<
                boost::multi_index::tag<ByStack>,
                boost::multi_index::const_mem_fun<NotebookItem,const QString&,&NotebookItem::stack>
            >
        >
    > NotebookData;

    typedef NotebookData::index<ByLocalUid>::type NotebookDataByLocalUid;
    typedef NotebookData::index<ByNameUpper>::type NotebookDataByNameUpper;
    typedef NotebookData::index<ByStack>::type NotebookDataByStack;

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

    typedef QHash<QString, NotebookModelItem> ModelItems;
    typedef QHash<QString, NotebookStackItem> StackItems;

    typedef LRUCache<QString, Notebook> Cache;

private:
    NotebookData            m_data;
    NotebookModelItem *     m_fakeRootItem;
    const NotebookItem *    m_defaultNotebookItem;

    ModelItems              m_modelItemsByLocalUid;
    ModelItems              m_modelItemsByStack;
    StackItems              m_stackItems;

    Cache                   m_cache;

    size_t                  m_listNotebooksOffset;
    QUuid                   m_listNotebooksRequestId;
    QSet<QUuid>             m_notebookItemsNotYetInLocalStorageUids;

    QSet<QUuid>             m_addNotebookRequestIds;
    QSet<QUuid>             m_updateNotebookRequestIds;
    QSet<QUuid>             m_expungeNotebookRequestIds;

    QSet<QUuid>             m_findNotebookToRestoreFailedUpdateRequestIds;
    QSet<QUuid>             m_findNotebookToPerformUpdateRequestIds;

    Columns::type           m_sortedColumn;
    Qt::SortOrder           m_sortOrder;

    mutable int             m_lastNewNotebookNameCounter;
};

} // namespace qute_note

#endif // __QUTE_NOTE__MODELS__NOTEBOOK_MODEL_H
