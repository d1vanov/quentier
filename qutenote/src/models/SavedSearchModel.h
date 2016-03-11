#ifndef __QUTE_NOTE__MODELS__SAVED_SEARCH_MODEL_H
#define __QUTE_NOTE__MODELS__SAVED_SEARCH_MODEL_H

#include "SavedSearchModelItem.h"
#include <qute_note/types/SavedSearch.h>
#include <qute_note/local_storage/LocalStorageManagerThreadWorker.h>
#include <qute_note/utility/Qt4Helper.h>
#include <QAbstractItemModel>
#include <QUuid>
#include <QCache>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index/ordered_index.hpp>

namespace qute_note {

class SavedSearchModel: public QAbstractItemModel
{
    Q_OBJECT
public:
    explicit SavedSearchModel(LocalStorageManagerThreadWorker & localStorageManagerThreadWorker,
                              QObject * parent = nullptr);
    virtual ~SavedSearchModel();

private:
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
    void getSavedSearchCount(QUuid requestId);
    void addSavedSearch(SavedSearch search, QUuid requestId);
    void updateSavedSearch(SavedSearch search, QUuid requestId);
    void listSavedSearches(LocalStorageManager::ListObjectsOptions flag,
                           size_t limit, size_t offset,
                           LocalStorageManager::ListSavedSearchesOrder::type order,
                           LocalStorageManager::OrderDirection::type orderDirection,
                           QUuid requestId);
    void expungeSavedSearch(SavedSearch search, QUuid requestId);

private Q_SLOTS:
    // Slots for response to events from local storage
    void onGetSavedSearchCountComplete(int savedSearchCount, QUuid requestId);
    void onGetSavedSearchCountFailed(QString errorDescription, QUuid requestId);
    void onAddSavedSearchComplete(SavedSearch search, QUuid requestId);
    void onAddSavedSearchFailed(SavedSearch search, QString errorDescription, QUuid requestId);
    void onUpdateSavedSearchComplete(SavedSearch search, QUuid requestId);
    void onUpdateSavedSearchFailed(SavedSearch search, QString errorDescription, QUuid requestId);
    void onListSavedSearchesComplete(LocalStorageManager::ListObjectsOptions flag,
                                     size_t limit, size_t offset,
                                     LocalStorageManager::ListSavedSearchesOrder::type order,
                                     LocalStorageManager::OrderDirection::type orderDirection,
                                     QList<SavedSearch> foundSearches, QUuid requestId);
    void onListSavedSearchesFailed(LocalStorageManager::ListObjectsOptions flag,
                                   size_t limit, size_t offset,
                                   LocalStorageManager::ListSavedSearchesOrder::type order,
                                   LocalStorageManager::OrderDirection::type orderDirection,
                                   QString errorDescription, QUuid requestId);
    void onExpungeSavedSearchComplete(SavedSearch search, QUuid requestId);
    void onExpungeSavedSearchFailed(SavedSearch search, QString errorDescription, QUuid requestId);

private:
    void createConnections(LocalStorageManagerThreadWorker & localStorageManagerThreadWorker);

private:
    typedef boost::multi_index_container<
        SavedSearchModelItem,
        boost::multi_index::indexed_by<
            boost::multi_index::ordered_non_unique<
                boost::multi_index::tag<SavedSearchModelItem::ByName>,
                boost::multi_index::member<SavedSearchModelItem,QString,&SavedSearchModelItem::m_name>
            >,
            boost::multi_index::ordered_unique<
                boost::multi_index::tag<SavedSearchModelItem::ByLocalUid>,
                boost::multi_index::member<SavedSearchModelItem,QString,&SavedSearchModelItem::m_localUid>
            >
        >
    > SavedSearchData;

    typedef QCache<QString, SavedSearch> SavedSearchCache;

private:
    SavedSearchData         m_data;
    SavedSearchCache        m_cache;

    // Offset for the next local storage query
    size_t                  m_currentOffset;

    QUuid                   m_lastListSavedSearchesRequestId;
};

} // namespace qute_note

#endif // __QUTE_NOTE__MODELS__SAVED_SEARCH_MODEL_H
