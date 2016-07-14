#ifndef LIB_QUENTIER_TESTS_USER_LOCAL_STORAGE_MANAGER_ASYNC_TESTER_H
#define LIB_QUENTIER_TESTS_USER_LOCAL_STORAGE_MANAGER_ASYNC_TESTER_H

#include <quentier/utility/Qt4Helper.h>
#include <quentier/utility/QNLocalizedString.h>
#include <quentier/types/UserWrapper.h>
#include <QUuid>

namespace quentier {

QT_FORWARD_DECLARE_CLASS(LocalStorageManagerThreadWorker)

namespace test {

class UserLocalStorageManagerAsyncTester : public QObject
{
    Q_OBJECT
public:
    explicit UserLocalStorageManagerAsyncTester(QObject * parent = Q_NULLPTR);
    ~UserLocalStorageManagerAsyncTester();

public Q_SLOTS:
    void onInitTestCase();

Q_SIGNALS:
    void success();
    void failure(QString errorDescription);

// private signals:
    void getUserCountRequest(QUuid requestId = QUuid());
    void addUserRequest(UserWrapper user, QUuid requestId = QUuid());
    void updateUserRequest(UserWrapper user, QUuid requestId = QUuid());
    void findUserRequest(UserWrapper user, QUuid requestId = QUuid());
    void deleteUserRequest(UserWrapper user, QUuid requestId = QUuid());
    void expungeUserRequest(UserWrapper user, QUuid requestId = QUuid());

private Q_SLOTS:
    void onWorkerInitialized();
    void onGetUserCountCompleted(int count, QUuid requestId);
    void onGetUserCountFailed(QNLocalizedString errorDescription, QUuid requestId);
    void onAddUserCompleted(UserWrapper user, QUuid requestId);
    void onAddUserFailed(UserWrapper user, QNLocalizedString errorDescription, QUuid requestId);
    void onUpdateUserCompleted(UserWrapper user, QUuid requestId);
    void onUpdateUserFailed(UserWrapper user, QNLocalizedString errorDescription, QUuid requestId);
    void onFindUserCompleted(UserWrapper user, QUuid requestId);
    void onFindUserFailed(UserWrapper user, QNLocalizedString errorDescription, QUuid requestId);
    void onDeleteUserCompleted(UserWrapper user, QUuid requestId);
    void onDeleteUserFailed(UserWrapper user, QNLocalizedString errorDescription, QUuid requestId);
    void onExpungeUserCompleted(UserWrapper user, QUuid requestId);
    void onExpungeUserFailed(UserWrapper user, QNLocalizedString errorDescription, QUuid requestId);

    void onFailure(QNLocalizedString errorDescription);

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
        STATE_SENT_GET_COUNT_AFTER_EXPUNGE_REQUEST
    };

    State m_state;

    LocalStorageManagerThreadWorker * m_pLocalStorageManagerThreadWorker;
    QThread *                   m_pLocalStorageManagerThread;

    qint32                      m_userId;

    UserWrapper                 m_initialUser;
    UserWrapper                 m_foundUser;
    UserWrapper                 m_modifiedUser;
};

} // namespace test
} // namespace quentier

#endif // LIB_QUENTIER_TESTS_USER_LOCAL_STORAGE_MANAGER_ASYNC_TESTER_H
