#ifndef __QUTE_NOTE__TESTS__MODEL_TEST__TAG_MODEL_TEST_HELPER_H
#define __QUTE_NOTE__TESTS__MODEL_TEST__TAG_MODEL_TEST_HELPER_H

#include <qute_note/local_storage/LocalStorageManagerThreadWorker.h>

namespace qute_note {

class TagModelTestHelper: public QObject
{
    Q_OBJECT
public:
    explicit TagModelTestHelper(LocalStorageManagerThreadWorker * pLocalStorageManagerThreadWorker,
                                QObject * parent = Q_NULLPTR);

Q_SIGNALS:
    void failure();
    void success();

public Q_SLOTS:
    void test();

private Q_SLOTS:
    void onAddTagFailed(Tag tag, QString errorDescription, QUuid requestId);
    void onUpdateTagFailed(Tag tag, QString errorDescription, QUuid requestId);
    void onFindTagFailed(Tag tag, QString errorDescription, QUuid requestId);
    void onListTagsFailed(LocalStorageManager::ListObjectsOptions flag,
                          size_t limit, size_t offset,
                          LocalStorageManager::ListTagsOrder::type order,
                          LocalStorageManager::OrderDirection::type orderDirection,
                          QString linkedNotebookGuid,
                          QString errorDescription, QUuid requestId);
    void onExpungeTagFailed(Tag tag, QString errorDescription, QUuid requestId);

private:
    LocalStorageManagerThreadWorker *   m_pLocalStorageManagerThreadWorker;
};

} // namespace qute_note

#endif // __QUTE_NOTE__TESTS__MODEL_TEST__TAG_MODEL_TEST_HELPER_H
