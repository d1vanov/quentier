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
    Q_UNUSED(count)
    // TODO: implement
}

void UserLocalStorageManagerAsyncTester::onGetUserCountFailed(QString errorDescription)
{
    Q_UNUSED(errorDescription)
    // TODO: implement
}

void UserLocalStorageManagerAsyncTester::onAddUserCompleted(QSharedPointer<UserWrapper> user)
{
    Q_UNUSED(user)
    // TODO: implement
}

void UserLocalStorageManagerAsyncTester::onAddUserFailed(QSharedPointer<UserWrapper> user, QString errorDescription)
{
    Q_UNUSED(user)
    Q_UNUSED(errorDescription)
    // TODO: implement
}

void UserLocalStorageManagerAsyncTester::onUpdateUserCompleted(QSharedPointer<UserWrapper> user)
{
    Q_UNUSED(user)
    // TODO: implement
}

void UserLocalStorageManagerAsyncTester::onUpdateUserFailed(QSharedPointer<UserWrapper> user, QString errorDescription)
{
    Q_UNUSED(user)
    Q_UNUSED(errorDescription)
    // TODO: implement
}

void UserLocalStorageManagerAsyncTester::onFindUserCompleted(QSharedPointer<UserWrapper> user)
{
    Q_UNUSED(user)
    // TODO: implement
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


