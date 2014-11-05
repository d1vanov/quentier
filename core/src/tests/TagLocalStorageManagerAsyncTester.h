#ifndef __QUTE_NOTE__CORE__TESTS__TAG_LOCAL_STORAGE_MANAGER_ASYNC_TESTER_H
#define __QUTE_NOTE__CORE__TESTS__TAG_LOCAL_STORAGE_MANAGER_ASYNC_TESTER_H

#include <client/types/Tag.h>

namespace qute_note {

QT_FORWARD_DECLARE_CLASS(LocalStorageManagerThreadWorker)

namespace test {

class TagLocalStorageManagerAsyncTester: public QObject
{
    Q_OBJECT
public:
    explicit TagLocalStorageManagerAsyncTester(QObject * parent = nullptr);
    ~TagLocalStorageManagerAsyncTester();

public Q_SLOTS:
    void onInitTestCase();

Q_SIGNALS:
    void success();
    void failure(QString errorDescription);

// private signals:
    void getTagCountRequest();
    void addTagRequest(Tag tag);
    void updateTagRequest(Tag tag);
    void findTagRequest(Tag tag);
    void listAllTagsRequest();
    void deleteTagRequest(Tag tag);
    void expungeTagRequest(Tag tag);

private Q_SLOTS:
    void onWorkerInitialized();
    void onGetTagCountCompleted(int count);
    void onGetTagCountFailed(QString errorDescription);
    void onAddTagCompleted(Tag tag);
    void onAddTagFailed(Tag tag, QString errorDescription);
    void onUpdateTagCompleted(Tag tag);
    void onUpdateTagFailed(Tag tag, QString errorDescription);
    void onFindTagCompleted(Tag tag);
    void onFindTagFailed(Tag tag, QString errorDescription);
    void onListAllTagsCompleted(QList<Tag> tags);
    void onListAllTagsFailed(QString errorDescription);
    void onDeleteTagCompleted(Tag tag);
    void onDeleteTagFailed(Tag tag, QString errorDescription);
    void onExpungeTagCompleted(Tag tag);
    void onExpungeTagFailed(Tag tag, QString errorDescription);

private:
    void createConnections();

    enum State
    {
        STATE_UNINITIALIZED,
        STATE_SENT_ADD_REQUEST,
        STATE_SENT_FIND_AFTER_ADD_REQUEST,
        STATE_SENT_UPDATE_REQUEST,
        STATE_SENT_FIND_AFTER_UPDATE_REQUEST,
        STATE_SENT_GET_COUNT_AFTER_UPDATE_REQUEST,
        STATE_SENT_DELETE_REQUEST,
        STATE_SENT_EXPUNGE_REQUEST,
        STATE_SENT_FIND_AFTER_EXPUNGE_REQUEST,
        STATE_SENT_GET_COUNT_AFTER_EXPUNGE_REQUEST,
        STATE_SENT_ADD_EXTRA_TAG_ONE_REQUEST,
        STATE_SENT_ADD_EXTRA_TAG_TWO_REQUEST,
        STATE_SENT_LIST_TAGS_REQUEST
    };

    State m_state;

    LocalStorageManagerThreadWorker * m_pLocalStorageManagerThreadWorker;
    QThread *   m_pLocalStorageManagerThread;

    Tag         m_initialTag;
    Tag         m_foundTag;
    Tag         m_modifiedTag;
    QList<Tag>  m_initialTags;
};

} // namespace test
} // namespace qute_note

#endif // __QUTE_NOTE__CORE__TESTS__TAG_LOCAL_STORAGE_MANAGER_ASYNC_TESTER_H
