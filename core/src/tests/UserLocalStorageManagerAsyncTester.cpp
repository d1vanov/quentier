#include "UserLocalStorageManagerAsyncTester.h"
#include <client/local_storage/LocalStorageManagerThread.h>
#include <logging/QuteNoteLogger.h>

namespace qute_note {
namespace test {

UserLocalStorageManagerAsyncTester::UserLocalStorageManagerAsyncTester(QObject * parent) :
    QObject(parent),
    m_state(STATE_UNINITIALIZED),
    m_pLocalStorageManagerThread(nullptr),
    m_initialUser(),
    m_foundUser(),
    m_modifiedUser()
{}

UserLocalStorageManagerAsyncTester::~UserLocalStorageManagerAsyncTester()
{
    // NOTE: shouldn't attempt to delete m_pLocalStorageManagerThread as Qt's parent-child system
    // should take care of that
}

void UserLocalStorageManagerAsyncTester::onInitTestCase()
{
    QString username = "UserLocalStorageManagerAsyncTester";
    qint32 userId = 3;
    bool startFromScratch = true;

    if (m_pLocalStorageManagerThread != nullptr) {
        delete m_pLocalStorageManagerThread;
        m_pLocalStorageManagerThread = nullptr;
    }

    m_state = STATE_UNINITIALIZED;

    m_pLocalStorageManagerThread = new LocalStorageManagerThread(username, userId, startFromScratch, this);
    createConnections();

    m_initialUser.setUsername("fakeusername");
    m_initialUser.setId(userId);
    m_initialUser.setEmail("Fake user email");
    m_initialUser.setName("Fake user name");
    m_initialUser.setTimezone("Europe/Moscow");
    m_initialUser.setPrivilegeLevel(qevercloud::PrivilegeLevel::NORMAL);
    m_initialUser.setCreationTimestamp(3);
    m_initialUser.setModificationTimestamp(3);
    m_initialUser.setActive(true);

    QString errorDescription;
    if (!m_initialUser.checkParameters(errorDescription)) {
        QNWARNING("Found invalid user: " << m_initialUser << ", error: " << errorDescription);
        emit failure(errorDescription);
        return;
    }

    m_state = STATE_SENT_ADD_REQUEST;
    emit addUserRequest(m_initialUser);
}

void UserLocalStorageManagerAsyncTester::onGetUserCountCompleted(int count)
{
    QString errorDescription;

#define HANDLE_WRONG_STATE() \
    else { \
        errorDescription = "Internal error in UserLocalStorageManagerAsyncTester: " \
                           "found wrong state"; \
        emit failure(errorDescription); \
    }

    if (m_state == STATE_SENT_GET_COUNT_AFTER_UPDATE_REQUEST)
    {
        if (count != 1) {
            errorDescription = "GetUserCount returned result different from the expected one (1): ";
            errorDescription += QString::number(count);
            emit failure(errorDescription);
            return;
        }

        m_modifiedUser.setLocal(false);
        m_modifiedUser.setDeletionTimestamp(13);
        m_state = STATE_SENT_DELETE_REQUEST;
        emit deleteUserRequest(m_modifiedUser);
    }
    else if (m_state == STATE_SENT_GET_COUNT_AFTER_EXPUNGE_REQUEST)
    {
        if (count != 0) {
            errorDescription = "GetUserCount returned result different from the expected one (0): ";
            errorDescription += QString::number(count);
            emit failure(errorDescription);
            return;
        }

        emit success();
    }
    HANDLE_WRONG_STATE();
}

void UserLocalStorageManagerAsyncTester::onGetUserCountFailed(QString errorDescription)
{
    QNWARNING(errorDescription);
    emit failure(errorDescription);
}

void UserLocalStorageManagerAsyncTester::onAddUserCompleted(UserWrapper user)
{
    QString errorDescription;

    if (m_state == STATE_SENT_ADD_REQUEST)
    {
        if (m_initialUser != user) {
            errorDescription = "Internal error in UserLocalStorageManagerAsyncTester: "
                               "user in onAddUserCompleted doesn't match the original UserWrapper";
            QNWARNING(errorDescription);
            emit failure(errorDescription);
            return;
        }

        m_foundUser = UserWrapper();
        m_foundUser.setId(user.id());

        m_state = STATE_SENT_FIND_AFTER_ADD_REQUEST;
        emit findUserRequest(m_foundUser);
    }
    HANDLE_WRONG_STATE();
}

void UserLocalStorageManagerAsyncTester::onAddUserFailed(UserWrapper user, QString errorDescription)
{
    QNWARNING(errorDescription << ", UserWrapper: " << user);
    emit failure(errorDescription);
}

void UserLocalStorageManagerAsyncTester::onUpdateUserCompleted(UserWrapper user)
{
    QString errorDescription;

    if (m_state == STATE_SENT_UPDATE_REQUEST)
    {
        if (m_modifiedUser != user) {
            errorDescription = "Internal error in UserLocalStorageManagerAsyncTester: "
                               "user in onUpdateUserCompleted slot doesn't match "
                               "the original modified UserWrapper";
            QNWARNING(errorDescription);
            emit failure(errorDescription);
            return;
        }

        m_state = STATE_SENT_FIND_AFTER_UPDATE_REQUEST;
        emit findUserRequest(m_foundUser);
    }
    HANDLE_WRONG_STATE();
}

void UserLocalStorageManagerAsyncTester::onUpdateUserFailed(UserWrapper user, QString errorDescription)
{
    QNWARNING(errorDescription << ", user: " << user);
    emit failure(errorDescription);
}

void UserLocalStorageManagerAsyncTester::onFindUserCompleted(UserWrapper user)
{
    QString errorDescription;

    if (m_state == STATE_SENT_FIND_AFTER_ADD_REQUEST)
    {
        if (user != m_initialUser) {
            errorDescription = "Added and found users in local storage don't match";
            QNWARNING(errorDescription << ": UserWrapper added to LocalStorageManager: " << m_initialUser
                      << "\nUserWrapper found in LocalStorageManager: " << user);
            emit failure(errorDescription);
            return;
        }

        // Ok, found user is good, updating it now
        m_modifiedUser = m_initialUser;
        m_modifiedUser.setUsername(m_initialUser.username() + "_modified");
        m_modifiedUser.setName(m_initialUser.name() + "_modified");

        m_state = STATE_SENT_UPDATE_REQUEST;
        emit updateUserRequest(m_modifiedUser);
    }
    else if (m_state == STATE_SENT_FIND_AFTER_UPDATE_REQUEST)
    {
        if (user != m_modifiedUser) {
            errorDescription = "Updated and found users in local storage don't match";
            QNWARNING(errorDescription << ": UserWrapper updated in LocalStorageManager: " << m_modifiedUser
                      << "\nUserWrapper found in LocalStorageManager: " << user);
            emit failure(errorDescription);
            return;
        }

        m_state = STATE_SENT_GET_COUNT_AFTER_UPDATE_REQUEST;
        emit getUserCountRequest();
    }
    else if (m_state == STATE_SENT_FIND_AFTER_EXPUNGE_REQUEST)
    {
        errorDescription = "Error: found user which should have been expunged from local storage";
        QNWARNING(errorDescription << ": UserWrapper expunged from LocalStorageManager: " << m_modifiedUser
                  << "\nUserWrapper found in LocalStorageManager: " << user);
        emit failure(errorDescription);
        return;
    }
    HANDLE_WRONG_STATE();
}

void UserLocalStorageManagerAsyncTester::onFindUserFailed(UserWrapper user, QString errorDescription)
{
    if (m_state == STATE_SENT_FIND_AFTER_EXPUNGE_REQUEST) {
        m_state = STATE_SENT_GET_COUNT_AFTER_EXPUNGE_REQUEST;
        emit getUserCountRequest();
        return;
    }

    QNWARNING(errorDescription << ", user: " << user);
    emit failure(errorDescription);
}

void UserLocalStorageManagerAsyncTester::onDeleteUserCompleted(UserWrapper user)
{
    QString errorDescription;

    if (m_modifiedUser != user) {
        errorDescription = "Internal error in UserLocalStorageManagerAsyncTester: "
                           "user in onDeleteUserCompleted slot doesn't match "
                           "the original deleted UserWrapper";
        QNWARNING(errorDescription << "; original deleted user: " << m_modifiedUser << ", user: " << user);
        emit failure(errorDescription);
        return;
    }

    m_modifiedUser.setLocal(true);
    m_state = STATE_SENT_EXPUNGE_REQUEST;
    emit expungeUserRequest(m_modifiedUser);
}

void UserLocalStorageManagerAsyncTester::onDeleteUserFailed(UserWrapper user, QString errorDescription)
{
    QNWARNING(errorDescription << ", user: " << user);
    emit failure(errorDescription);
}

void UserLocalStorageManagerAsyncTester::onExpungeUserCompleted(UserWrapper user)
{
    QString errorDescription;

    if (m_modifiedUser != user) {
        errorDescription = "Internal error in UserLocalStorageManagerAsyncTester: "
                           "user in onExpungeUserCompleted slot doesn't match "
                           "the original expunged UserWrapper";
        QNWARNING(errorDescription);
        emit failure(errorDescription);
    }

    m_state = STATE_SENT_FIND_AFTER_EXPUNGE_REQUEST;
    emit findUserRequest(m_foundUser);
}

void UserLocalStorageManagerAsyncTester::onExpungeUserFailed(UserWrapper user, QString errorDescription)
{
    QNWARNING(errorDescription << ", UserWrapper: " << user);
    emit failure(errorDescription);
}

void UserLocalStorageManagerAsyncTester::createConnections()
{
    // Request --> slot connections
    QObject::connect(this, SIGNAL(getUserCountRequest()), m_pLocalStorageManagerThread,
                     SLOT(onGetUserCountRequest()));
    QObject::connect(this, SIGNAL(addUserRequest(UserWrapper)),
                     m_pLocalStorageManagerThread, SLOT(onAddUserRequest(UserWrapper)));
    QObject::connect(this, SIGNAL(updateUserRequest(UserWrapper)),
                     m_pLocalStorageManagerThread, SLOT(onUpdateUserRequest(UserWrapper)));
    QObject::connect(this, SIGNAL(findUserRequest(UserWrapper)),
                     m_pLocalStorageManagerThread, SLOT(onFindUserRequest(UserWrapper)));
    QObject::connect(this, SIGNAL(deleteUserRequest(UserWrapper)),
                     m_pLocalStorageManagerThread, SLOT(onDeleteUserRequest(UserWrapper)));
    QObject::connect(this, SIGNAL(expungeUserRequest(UserWrapper)),
                     m_pLocalStorageManagerThread, SLOT(onExpungeUserRequest(UserWrapper)));

    // Slot <-- result connections
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(getUserCountComplete(int)),
                     this, SLOT(onGetUserCountCompleted(int)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(getUserCountFailed(QString)),
                     this, SLOT(onGetUserCountFailed(QString)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(addUserComplete(UserWrapper)),
                     this, SLOT(onAddUserCompleted(UserWrapper)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(addUserFailed(UserWrapper,QString)),
                     this, SLOT(onAddUserFailed(UserWrapper,QString)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(updateUserComplete(UserWrapper)),
                     this, SLOT(onUpdateUserCompleted(UserWrapper)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(updateUserFailed(UserWrapper,QString)),
                     this, SLOT(onUpdateUserFailed(UserWrapper,QString)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(findUserComplete(UserWrapper)),
                     this, SLOT(onFindUserCompleted(UserWrapper)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(findUserFailed(UserWrapper,QString)),
                     this, SLOT(onFindUserFailed(UserWrapper,QString)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(deleteUserComplete(UserWrapper)),
                     this, SLOT(onDeleteUserCompleted(UserWrapper)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(deleteUserFailed(UserWrapper,QString)),
                     this, SLOT(onDeleteUserFailed(UserWrapper,QString)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(expungeUserComplete(UserWrapper)),
                     this, SLOT(onExpungeUserCompleted(UserWrapper)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(expungeUserFailed(UserWrapper,QString)),
                     this, SLOT(onExpungeUserFailed(UserWrapper,QString)));
}

#undef HANDLE_WRONG_STATE

} // namespace test
} // namespace qute_note


