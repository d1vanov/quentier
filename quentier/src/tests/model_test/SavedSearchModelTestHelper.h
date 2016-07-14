#ifndef QUENTIER_TESTS_MODEL_TEST_SAVED_SEARCH_MODEL_TEST_HELPER_H
#define QUENTIER_TESTS_MODEL_TEST_SAVED_SEARCH_MODEL_TEST_HELPER_H

#include <quentier/local_storage/LocalStorageManagerThreadWorker.h>

namespace quentier {

QT_FORWARD_DECLARE_CLASS(SavedSearchModel)

class SavedSearchModelTestHelper: public QObject
{
    Q_OBJECT
public:
    explicit SavedSearchModelTestHelper(LocalStorageManagerThreadWorker * pLocalStorageManagerThreadWorker,
                                        QObject * parent = Q_NULLPTR);

Q_SIGNALS:
    void failure();
    void success();

public Q_SLOTS:
    void test();

private Q_SLOTS:
    void onAddSavedSearchFailed(SavedSearch search, QNLocalizedString errorDescription, QUuid requestId);
    void onUpdateSavedSearchFailed(SavedSearch search, QNLocalizedString errorDescription, QUuid requestId);
    void onFindSavedSearchFailed(SavedSearch search, QNLocalizedString errorDescription, QUuid requestId);
    void onListSavedSearchesFailed(LocalStorageManager::ListObjectsOptions flag,
                                   size_t limit, size_t offset,
                                   LocalStorageManager::ListSavedSearchesOrder::type order,
                                   LocalStorageManager::OrderDirection::type orderDirection,
                                   QNLocalizedString errorDescription, QUuid requestId);
    void onExpungeSavedSearchFailed(SavedSearch search, QNLocalizedString errorDescription, QUuid requestId);

private:
    bool checkSorting(const SavedSearchModel & model) const;

    struct LessByName
    {
        bool operator()(const QString & lhs, const QString & rhs) const;
    };

    struct GreaterByName
    {
        bool operator()(const QString & lhs, const QString & rhs) const;
    };

private:
    LocalStorageManagerThreadWorker *   m_pLocalStorageManagerThreadWorker;
};

} // namespace quentier

#endif // QUENTIER_TESTS_MODEL_TEST_SAVED_SEARCH_MODEL_TEST_HELPER_H
