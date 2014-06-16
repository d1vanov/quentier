#include "UserLocalStorageManagerAsyncTester.h"
#include <client/local_storage/LocalStorageManagerThread.h>
#include <logging/QuteNoteLogger.h>

namespace qute_note {
namespace test {

UserLocalStorageManagerAsyncTester::UserLocalStorageManagerAsyncTester(QObject * parent) :
    QObject(parent),
    m_state(STATE_UNINITIALIZED),
    m_pLocalStorageManagerThread(nullptr),
    m_pInitialUser(),
    m_pFoundUser(),
    m_pModifiedUser()
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

    m_pInitialUser = QSharedPointer<UserWrapper>(new UserWrapper);
    m_pInitialUser->setUsername(username);
    m_pInitialUser->setId(userId);
    m_pInitialUser->setEmail("Fake user email");
    m_pInitialUser->setName("Fake user name");
    m_pInitialUser->setTimezone("Europe/Moscow");
    m_pInitialUser->setPrivilegeLevel(qevercloud::PrivilegeLevel::NORMAL);

    QString errorDescription;
    if (!m_pInitialUser->checkParameters(errorDescription)) {
        QNWARNING("Found invalid user: " << *m_pInitialUser << ", error: " << errorDescription);
        emit failure(errorDescription);
        return;
    }

    m_state = STATE_SENT_ADD_REQUEST;
    emit addUserRequest(m_pInitialUser);
}

void UserLocalStorageManagerAsyncTester::onGetUserCountCompleted(int count)
{
    QString errorDescription;

#define HANDLE_WRONG_STATE() \
    else { \
        errorDescription = QObject::tr("Internal error in UserLocalStorageManagerAsyncTester: " \
                                       "found wring state"); \
        emit failure(errorDescription); \
    }

    if (m_state == STATE_SENT_GET_COUNT_AFTER_UPDATE_REQUEST)
    {
        if (count != 1) {
            errorDescription = QObject::tr("GetUserCount returned result different from the expected one (1): ");
            errorDescription += QString::number(count);
            emit failure(errorDescription);
            return;
        }

        m_pModifiedUser->setLocal(false);
        m_pModifiedUser->setDeletionTimestamp(13);
        m_state = STATE_SENT_DELETE_REQUEST;
        emit deleteUserRequest(m_pModifiedUser);
    }
    else if (m_state == STATE_SENT_GET_COUNT_AFTER_EXPUNGE_REQUEST)
    {
        if (count != 0) {
            errorDescription = QObject::tr("GetUserCount returned result different from the expected one (0): ");
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

void UserLocalStorageManagerAsyncTester::onAddUserCompleted(QSharedPointer<UserWrapper> user)
{
    Q_ASSERT_X(!user.isNull(), "UserLocalStorageManagerAsyncTester::onAddUserCompleted slot",
               "Found NULL shared pointer to UserWrapper");

    QString errorDescription;

    if (m_state == STATE_SENT_ADD_REQUEST)
    {
        if (m_pInitialUser != user) {
            errorDescription = "Internal error in UserLocalStorageManagerAsyncTester: "
                               "user in onAddUserCompleted doesn't match the original UserWrapper";
            QNWARNING(errorDescription);
            errorDescription = QObject::tr(qPrintable(errorDescription));
            emit failure(errorDescription);
            return;
        }

        m_pFoundUser = QSharedPointer<UserWrapper>(new UserWrapper);
        m_pFoundUser->setId(user->id());

        m_state = STATE_SENT_FIND_AFTER_ADD_REQUEST;
        emit findUserRequest(m_pFoundUser);
    }
    HANDLE_WRONG_STATE();
}

void UserLocalStorageManagerAsyncTester::onAddUserFailed(QSharedPointer<UserWrapper> user, QString errorDescription)
{
    QNWARNING(errorDescription << ", UserWrapper: " << (user.isNull() ? QString("NULL") : user->ToQString()));
    emit failure(errorDescription);
}

void UserLocalStorageManagerAsyncTester::onUpdateUserCompleted(QSharedPointer<UserWrapper> user)
{
    Q_ASSERT_X(!user.isNull(), "UserLocalStorageManagerAsyncTester::onUpdateUserCompleted slot",
               "Found NULL shared pointer to UserWrapper");

    QString errorDescription;

    if (m_state == STATE_SENT_UPDATE_REQUEST)
    {
        if (m_pModifiedUser != user) {
            errorDescription = "Internal error in UserLocalStorageManagerAsyncTester: "
                               "user pointer in onUpdateUserCompleted slot doesn't match "
                               "the pointer to the original modified UserWrapper";
            QNWARNING(errorDescription);
            errorDescription = QObject::tr(qPrintable(errorDescription));
            emit failure(errorDescription);
            return;
        }

        m_state = STATE_SENT_FIND_AFTER_UPDATE_REQUEST;
        emit findUserRequest(m_pFoundUser);
    }
    HANDLE_WRONG_STATE();
}

void UserLocalStorageManagerAsyncTester::onUpdateUserFailed(QSharedPointer<UserWrapper> user, QString errorDescription)
{
    QNWARNING(errorDescription << ", user: " << (user.isNull() ? QString("NULL") : user->ToQString()));
    emit failure(errorDescription);
}

void UserLocalStorageManagerAsyncTester::onFindUserCompleted(QSharedPointer<UserWrapper> user)
{
    Q_ASSERT_X(!user.isNull(), "UserLocalStorageManagerAsyncTester::onFindUserCompleted slot",
               "Found NULL shared pointer to UserWrapper");

    QString errorDescription;

    if (m_state == STATE_SENT_FIND_AFTER_ADD_REQUEST)
    {
        if (m_pFoundUser != user) {
            errorDescription = "Internal error in UserLocalStorageManagerAsyncTester: "
                               "user pointer in onFindUserCompleted slot doesn't match "
                               "the pointer to the original UserWrapper";
            QNWARNING(errorDescription);
            errorDescription = QObject::tr(qPrintable(errorDescription));
            emit failure(errorDescription);
            return;
        }

        Q_ASSERT(!m_pInitialUser.isNull());
        if (*m_pFoundUser != *m_pInitialUser) {
            errorDescription = "Added and found users in local storage don't match";
            QNWARNING(errorDescription << ": UserWrapper added to LocalStorageManager: " << *m_pInitialUser
                      << "\nUserWrapper found in LocalStorageManager: " << *m_pFoundUser);
            errorDescription = QObject::tr(qPrintable(errorDescription));
            emit failure(errorDescription);
            return;
        }

        // Ok, found user is good, updating it now
        m_pModifiedUser = QSharedPointer<UserWrapper>(new UserWrapper(*m_pInitialUser));
        m_pModifiedUser->setUsername(m_pInitialUser->username() + "_modified");
        m_pModifiedUser->setName(m_pInitialUser->name() + "_modified");

        m_state = STATE_SENT_UPDATE_REQUEST;
        emit updateUserRequest(m_pModifiedUser);
    }
    // TODO: continue from here
}

void UserLocalStorageManagerAsyncTester::onFindUserFailed(QSharedPointer<UserWrapper> user, QString errorDescription)
{
    Q_UNUSED(user)
    Q_UNUSED(errorDescription)
    // TODO: implement
}

void UserLocalStorageManagerAsyncTester::onDeleteUserCompleted(QSharedPointer<UserWrapper> user)
{
    Q_UNUSED(user)
    // TODO: implement
}

void UserLocalStorageManagerAsyncTester::onDeleteUserFailed(QSharedPointer<UserWrapper> user, QString errorDescription)
{
    Q_UNUSED(user)
    Q_UNUSED(errorDescription)
    // TODO: implement
}

void UserLocalStorageManagerAsyncTester::onExpungeUserCompleted(QSharedPointer<UserWrapper> user)
{
    Q_UNUSED(user)
    // TODO: implement
}

void UserLocalStorageManagerAsyncTester::onExpungeUserFailed(QSharedPointer<UserWrapper> user, QString errorDescription)
{
    Q_UNUSED(user)
    Q_UNUSED(errorDescription)
    // TODO: implement
}

void UserLocalStorageManagerAsyncTester::createConnections()
{
    // Request --> slot connections
    QObject::connect(this, SIGNAL(getUserCountRequest()), m_pLocalStorageManagerThread,
                     SLOT(onGetUserCountRequest()));
    QObject::connect(this, SIGNAL(addUserRequest(QSharedPointer<UserWrapper>)),
                     m_pLocalStorageManagerThread, SLOT(onAddUserRequest(QSharedPointer<UserWrapper>)));
    QObject::connect(this, SIGNAL(updateUserRequest(QSharedPointer<UserWrapper>)),
                     m_pLocalStorageManagerThread, SLOT(onUpdateUserRequest(QSharedPointer<UserWrapper>)));
    QObject::connect(this, SIGNAL(findUserRequest(QSharedPointer<UserWrapper>)),
                     m_pLocalStorageManagerThread, SLOT(onFindUserRequest(QSharedPointer<UserWrapper>)));
    QObject::connect(this, SIGNAL(deleteUserRequest(QSharedPointer<UserWrapper>)),
                     m_pLocalStorageManagerThread, SLOT(onDeleteUserRequest(QSharedPointer<UserWrapper>)));
    QObject::connect(this, SIGNAL(expungeUserRequest(QSharedPointer<UserWrapper>)),
                     m_pLocalStorageManagerThread, SLOT(onExpungeUserRequest(QSharedPointer<UserWrapper>)));

    // Slot <-- result connections
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(getUserCountComplete(int)),
                     this, SLOT(onGetUserCountCompleted(int)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(getUserCountFailed(QString)),
                     this, SLOT(onGetUserCountFailed(QString)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(addUserComplete(QSharedPointer<UserWrapper>)),
                     this, SLOT(onAddUserCompleted(QSharedPointer<UserWrapper>)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(addUserFailed(QSharedPointer<UserWrapper>,QString)),
                     this, SLOT(onAddUserFailed(QSharedPointer<UserWrapper>,QString)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(updateUserComplete(QSharedPointer<UserWrapper>)),
                     this, SLOT(onUpdateUserCompleted(QSharedPointer<UserWrapper>)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(updateUserFailed(QSharedPointer<UserWrapper>,QString)),
                     this, SLOT(onUpdateUserFailed(QSharedPointer<UserWrapper>,QString)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(findUserComplete(QSharedPointer<UserWrapper>)),
                     this, SLOT(onFindUserCompleted(QSharedPointer<UserWrapper>)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(findUserFailed(QSharedPointer<UserWrapper>,QString)),
                     this, SLOT(onFindUserFailed(QSharedPointer<UserWrapper>,QString)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(deleteUserComplete(QSharedPointer<UserWrapper>)),
                     this, SLOT(onDeleteUserCompleted(QSharedPointer<UserWrapper>)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(deleteUserFailed(QSharedPointer<UserWrapper>,QString)),
                     this, SLOT(onDeleteUserFailed(QSharedPointer<UserWrapper>,QString)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(expungeUserComplete(QSharedPointer<UserWrapper>)),
                     this, SLOT(onExpungeUserCompleted(QSharedPointer<UserWrapper>)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(expungeUserFailed(QSharedPointer<UserWrapper>,QString)),
                     this, SLOT(onExpungeUserFailed(QSharedPointer<UserWrapper>,QString)));
}

} // namespace test
} // namespace qute_note


