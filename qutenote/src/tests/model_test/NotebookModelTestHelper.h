#ifndef QUTE_NOTE_TESTS_MODEL_TEST_NOTEBOOK_MODEL_TEST_HELPER_H
#define QUTE_NOTE_TESTS_MODEL_TEST_NOTEBOOK_MODEL_TEST_HELPER_H

#include <qute_note/local_storage/LocalStorageManagerThreadWorker.h>

namespace qute_note {

QT_FORWARD_DECLARE_CLASS(NotebookModel)
QT_FORWARD_DECLARE_CLASS(NotebookModelItem)

class NotebookModelTestHelper: public QObject
{
    Q_OBJECT
public:
    explicit NotebookModelTestHelper(LocalStorageManagerThreadWorker * pLocalStorageManagerThreadWorker,
                                     QObject * parent = Q_NULLPTR);

Q_SIGNALS:
    void failure();
    void success();

public Q_SLOTS:
    void test();

private Q_SLOTS:
    void onAddNotebookFailed(Notebook notebook, QString errorDescription, QUuid requestId);
    void onUpdateNotebookFailed(Notebook notebook, QString errorDescription, QUuid requestId);
    void onFindNotebookFailed(Notebook notebook, QString errorDescription, QUuid requestId);
    void onListNotebooksFailed(LocalStorageManager::ListObjectsOptions flag, size_t limit, size_t offset,
                               LocalStorageManager::ListNotebooksOrder::type order,
                               LocalStorageManager::OrderDirection::type orderDirection,
                               QString linkedNotebookGuid, QString errorDescription, QUuid requestId);
    void onExpungeNotebookFailed(Notebook notebook, QString errorDescription, QUuid requestId);

private:
    bool checkSorting(const NotebookModel & model, const NotebookModelItem * item) const;

    struct LessByName
    {
        bool operator()(const NotebookModelItem * lhs, const NotebookModelItem * rhs) const;
    };

    struct GreaterByName
    {
        bool operator()(const NotebookModelItem * lhs, const NotebookModelItem * rhs) const;
    };

private:
    LocalStorageManagerThreadWorker *   m_pLocalStorageManagerThreadWorker;
};

} // namespace qute_note

#endif // QUTE_NOTE_TESTS_MODEL_TEST_NOTEBOOK_MODEL_TEST_HELPER_H
