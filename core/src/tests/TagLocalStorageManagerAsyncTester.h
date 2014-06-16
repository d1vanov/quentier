#ifndef __QUTE_NOTE__CORE__TESTS__TAG_LOCAL_STORAGE_MANAGER_ASYNC_TESTER_H
#define __QUTE_NOTE__CORE__TESTS__TAG_LOCAL_STORAGE_MANAGER_ASYNC_TESTER_H

#include <client/types/Tag.h>
#include <QSharedPointer>

namespace qute_note {

QT_FORWARD_DECLARE_CLASS(LocalStorageManagerThread)

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
    void addTagRequest(QSharedPointer<Tag> tag);
    void updateTagRequest(QSharedPointer<Tag> tag);
    void findTagRequest(QSharedPointer<Tag> tag);
    void listAllTagsRequest();
    void deleteTagRequest(QSharedPointer<Tag> tag);
    void expungeTagRequest(QSharedPointer<Tag> tag);

private Q_SLOTS:
    void onGetTagCountCompleted(int count);
    void onGetTagCountFailed(QString errorDescription);
    void onAddTagCompleted(QSharedPointer<Tag> tag);
    void onAddTagFailed(QSharedPointer<Tag> tag, QString errorDescription);
    void onUpdateTagCompleted(QSharedPointer<Tag> tag);
    void onUpdateTagFailed(QSharedPointer<Tag> tag, QString errorDescription);
    void onFindTagCompleted(QSharedPointer<Tag> tag);
    void onFindTagFailed(QSharedPointer<Tag> tag, QString errorDescription);
    void onListAllTagsCompleted(QList<Tag> tags);
    void onListAllTagsFailed(QString errorDescription);
    void onDeleteTagCompleted(QSharedPointer<Tag> tag);
    void onDeleteTagFailed(QSharedPointer<Tag> tag, QString errorDescription);
    void onExpungeTagCompleted(QSharedPointer<Tag> tag);
    void onExpungeTagFailed(QSharedPointer<Tag> tag, QString errorDescription);

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
    LocalStorageManagerThread * m_pLocalStorageManagerThread;
    QSharedPointer<Tag>         m_pInitialTag;
    QSharedPointer<Tag>         m_pFoundTag;
    QSharedPointer<Tag>         m_pModifiedTag;
    QList<Tag>  m_initialTags;
};

} // namespace test
} // namespace qute_note

#endif // __QUTE_NOTE__CORE__TESTS__TAG_LOCAL_STORAGE_MANAGER_ASYNC_TESTER_H
