#ifndef __QUTE_NOTE__CORE__TESTS__USER_LOCAL_STORAGE_MANAGER_ASYNC_TESTER_H_H
#define __QUTE_NOTE__CORE__TESTS__USER_LOCAL_STORAGE_MANAGER_ASYNC_TESTER_H_H

#include <client/types/UserWrapper.h>
#include <QSharedPointer>

namespace qute_note {

QT_FORWARD_DECLARE_CLASS(LocalStorageManagerThread)

namespace test {

class UserLocalStorageManagerAsyncTester : public QObject
{
    Q_OBJECT
public:
    explicit UserLocalStorageManagerAsyncTester(QObject * parent = nullptr);
    ~UserLocalStorageManagerAsyncTester();
    
public Q_SLOTS:
    void onInitTestCase();

Q_SIGNALS:
    void success();
    void failure(QString errorDescription);

// private signals:
    void getUserCountRequest();
    void addUserRequest(QSharedPointer<UserWrapper> user);
    void updateUserRequest(QSharedPointer<UserWrapper> user);
    void findUserRequest(QSharedPointer<UserWrapper> user);
    void deleteUserRequest(QSharedPointer<UserWrapper> user);
    void expungeUserRequest(QSharedPointer<UserWrapper> user);

private Q_SLOTS:
    void onGetUserCountCompleted(int count);
    void onGetUserCountFailed(QString errorDescription);
    void onAddUserCompleted(QSharedPointer<UserWrapper> user);
    void onAddUserFailed(QSharedPointer<UserWrapper> user, QString errorDescription);
    void onUpdateUserCompleted(QSharedPointer<UserWrapper> user);
    void onUpdateUserFailed(QSharedPointer<UserWrapper> user, QString errorDescription);
    void onFindUserCompleted(QSharedPointer<UserWrapper> user);
    void onFindUserFailed(QSharedPointer<UserWrapper> user, QString errorDescription);
    void onDeleteUserCompleted(QSharedPointer<UserWrapper> user);
    void onDeleteUserFailed(QSharedPointer<UserWrapper> user, QString errorDescription);
    void onExpungeUserCompleted(QSharedPointer<UserWrapper> user);
    void onExpungeUserFailed(QSharedPointer<UserWrapper> user, QString errorDescription);

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
    };

    State m_state;
    LocalStorageManagerThread * m_pLocalStorageManagerThread;
    QSharedPointer<UserWrapper> m_pInitialUser;
    QSharedPointer<UserWrapper> m_pFoundUser;
    QSharedPointer<UserWrapper> m_pModifiedUser;
};

} // namespace test
} // namespace qute_note

#endif // __QUTE_NOTE__CORE__TESTS__USER_LOCAL_STORAGE_MANAGER_ASYNC_TESTER_H_H
