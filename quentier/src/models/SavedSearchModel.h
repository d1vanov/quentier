#ifndef QUENTIER_MODELS_SAVED_SEARCH_MODEL_H
#define QUENTIER_MODELS_SAVED_SEARCH_MODEL_H

#include "SavedSearchModelItem.h"
#include "SavedSearchCache.h"
#include <quentier/types/SavedSearch.h>
#include <quentier/local_storage/LocalStorageManagerThreadWorker.h>
#include <quentier/utility/LRUCache.hpp>
#include <QAbstractItemModel>
#include <QUuid>
#include <QSet>

// NOTE: Workaround a bug in Qt4 which may prevent building with some boost versions
#ifndef Q_MOC_RUN
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index/random_access_index.hpp>
#include <boost/multi_index/ordered_index.hpp>
#endif

namespace quentier {

class SavedSearchModel: public QAbstractItemModel
{
    Q_OBJECT
public:
    explicit SavedSearchModel(LocalStorageManagerThreadWorker & localStorageManagerThreadWorker,
                              SavedSearchCache & cache, QObject * parent = Q_NULLPTR);
    virtual ~SavedSearchModel();

    struct Columns
    {
        enum type {
            Name = 0,
            Query,
            Synchronizable,
            Dirty
        };
    };

    int sortingColumn() const { return m_sortedColumn; }
    Qt::SortOrder sortOrder() const { return m_sortOrder; }

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

    virtual void sort(int column, Qt::SortOrder order) Q_DECL_OVERRIDE;

Q_SIGNALS:
    void notifyError(QNLocalizedString errorDescription);

// private signals
    void addSavedSearch(SavedSearch search, QUuid requestId);
    void updateSavedSearch(SavedSearch search, QUuid requestId);
    void findSavedSearch(SavedSearch search, QUuid requestId);
    void listSavedSearches(LocalStorageManager::ListObjectsOptions flag,
                           size_t limit, size_t offset,
                           LocalStorageManager::ListSavedSearchesOrder::type order,
                           LocalStorageManager::OrderDirection::type orderDirection,
                           QUuid requestId);
    void expungeSavedSearch(SavedSearch search, QUuid requestId);

private Q_SLOTS:
    // Slots for response to events from local storage
    void onAddSavedSearchComplete(SavedSearch search, QUuid requestId);
    void onAddSavedSearchFailed(SavedSearch search, QNLocalizedString errorDescription, QUuid requestId);
    void onUpdateSavedSearchComplete(SavedSearch search, QUuid requestId);
    void onUpdateSavedSearchFailed(SavedSearch search, QNLocalizedString errorDescription, QUuid requestId);
    void onFindSavedSearchComplete(SavedSearch search, QUuid requestId);
    void onFindSavedSearchFailed(SavedSearch search, QNLocalizedString errorDescription, QUuid requestId);
    void onListSavedSearchesComplete(LocalStorageManager::ListObjectsOptions flag,
                                     size_t limit, size_t offset,
                                     LocalStorageManager::ListSavedSearchesOrder::type order,
                                     LocalStorageManager::OrderDirection::type orderDirection,
                                     QList<SavedSearch> foundSearches, QUuid requestId);
    void onListSavedSearchesFailed(LocalStorageManager::ListObjectsOptions flag,
                                   size_t limit, size_t offset,
                                   LocalStorageManager::ListSavedSearchesOrder::type order,
                                   LocalStorageManager::OrderDirection::type orderDirection,
                                   QNLocalizedString errorDescription, QUuid requestId);
    void onExpungeSavedSearchComplete(SavedSearch search, QUuid requestId);
    void onExpungeSavedSearchFailed(SavedSearch search, QNLocalizedString errorDescription, QUuid requestId);

private:
    void createConnections(LocalStorageManagerThreadWorker & localStorageManagerThreadWorker);
    void requestSavedSearchesList();

    void onSavedSearchAddedOrUpdated(const SavedSearch & search);

    QVariant dataImpl(const int row, const Columns::type column) const;
    QVariant dataAccessibleText(const int row, const Columns::type column) const;

    QString nameForNewSavedSearch() const;

    // Returns the appropriate row before which the new item should be inserted according to the current sorting criteria and column
    int rowForNewItem(const SavedSearchModelItem & newItem) const;

    void updateRandomAccessIndexWithRespectToSorting(const SavedSearchModelItem & item);

    void updateSavedSearchInLocalStorage(const SavedSearchModelItem & item);

private:
    struct ByLocalUid{};
    struct ByIndex{};
    struct ByNameUpper{};

    typedef boost::multi_index_container<
        SavedSearchModelItem,
        boost::multi_index::indexed_by<
            boost::multi_index::random_access<
                boost::multi_index::tag<ByIndex>
            >,
            boost::multi_index::ordered_unique<
                boost::multi_index::tag<ByLocalUid>,
                boost::multi_index::member<SavedSearchModelItem,QString,&SavedSearchModelItem::m_localUid>
            >,
            boost::multi_index::ordered_unique<
                boost::multi_index::tag<ByNameUpper>,
                boost::multi_index::const_mem_fun<SavedSearchModelItem,QString,&SavedSearchModelItem::nameUpper>
            >
        >
    > SavedSearchData;

    typedef SavedSearchData::index<ByLocalUid>::type SavedSearchDataByLocalUid;
    typedef SavedSearchData::index<ByIndex>::type SavedSearchDataByIndex;
    typedef SavedSearchData::index<ByNameUpper>::type SavedSearchDataByNameUpper;

    struct LessByName
    {
        bool operator()(const SavedSearchModelItem & lhs, const SavedSearchModelItem & rhs) const;
    };

    struct GreaterByName
    {
        bool operator()(const SavedSearchModelItem & lhs, const SavedSearchModelItem & rhs) const;
    };

private:
    SavedSearchData         m_data;
    size_t                  m_listSavedSearchesOffset;
    QUuid                   m_listSavedSearchesRequestId;
    QSet<QUuid>             m_savedSearchItemsNotYetInLocalStorageUids;

    SavedSearchCache &      m_cache;

    QSet<QString>           m_lowerCaseSavedSearchNames;

    QSet<QUuid>             m_addSavedSearchRequestIds;
    QSet<QUuid>             m_updateSavedSearchRequestIds;
    QSet<QUuid>             m_expungeSavedSearchRequestIds;

    QSet<QUuid>             m_findSavedSearchToRestoreFailedUpdateRequestIds;
    QSet<QUuid>             m_findSavedSearchToPerformUpdateRequestIds;

    Columns::type           m_sortedColumn;
    Qt::SortOrder           m_sortOrder;

    mutable int             m_lastNewSavedSearchNameCounter;
};

} // namespace quentier

#endif // QUENTIER_MODELS_SAVED_SEARCH_MODEL_H
