/*
 * Copyright 2016 Dmitry Ivanov
 *
 * This file is part of libquentier
 *
 * libquentier is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * libquentier is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with libquentier. If not, see <http://www.gnu.org/licenses/>.
 */

#include "UserLocalStorageManagerAsyncTester.h"
#include <quentier/local_storage/LocalStorageManagerThreadWorker.h>
#include <quentier/logging/QuentierLogger.h>
#include <QThread>

namespace quentier {
namespace test {

UserLocalStorageManagerAsyncTester::UserLocalStorageManagerAsyncTester(QObject * parent) :
    QObject(parent),
    m_state(STATE_UNINITIALIZED),
    m_pLocalStorageManagerThreadWorker(Q_NULLPTR),
    m_pLocalStorageManagerThread(Q_NULLPTR),
    m_userId(3),
    m_initialUser(),
    m_foundUser(),
    m_modifiedUser()
{}

UserLocalStorageManagerAsyncTester::~UserLocalStorageManagerAsyncTester()
{
    // NOTE: shouldn't attempt to delete m_pLocalStorageManagerThread as Qt's parent-child system
    // should take care of that
    if (m_pLocalStorageManagerThread) {
        m_pLocalStorageManagerThread->quit();
        m_pLocalStorageManagerThread->wait();
    }

    if (m_pLocalStorageManagerThreadWorker) {
        delete m_pLocalStorageManagerThreadWorker;
    }
}

void UserLocalStorageManagerAsyncTester::onInitTestCase()
{
    QString username = QStringLiteral("UserLocalStorageManagerAsyncTester");
    bool startFromScratch = true;
    bool overrideLock = false;

    if (m_pLocalStorageManagerThread) {
        delete m_pLocalStorageManagerThread;
        m_pLocalStorageManagerThread = Q_NULLPTR;
    }

    if (m_pLocalStorageManagerThreadWorker) {
        delete m_pLocalStorageManagerThreadWorker;
        m_pLocalStorageManagerThreadWorker = Q_NULLPTR;
    }

    m_state = STATE_UNINITIALIZED;

    m_pLocalStorageManagerThread = new QThread(this);
    Account account(username, Account::Type::Evernote, m_userId);
    m_pLocalStorageManagerThreadWorker = new LocalStorageManagerThreadWorker(account, startFromScratch, overrideLock);
    m_pLocalStorageManagerThreadWorker->moveToThread(m_pLocalStorageManagerThread);

    createConnections();

    m_pLocalStorageManagerThread->start();
}

void UserLocalStorageManagerAsyncTester::onWorkerInitialized()
{
    m_initialUser.setUsername(QStringLiteral("fakeusername"));
    m_initialUser.setId(m_userId);
    m_initialUser.setEmail(QStringLiteral("Fake user email"));
    m_initialUser.setName(QStringLiteral("Fake user name"));
    m_initialUser.setTimezone(QStringLiteral("Europe/Moscow"));
    m_initialUser.setPrivilegeLevel(qevercloud::PrivilegeLevel::NORMAL);
    m_initialUser.setCreationTimestamp(3);
    m_initialUser.setModificationTimestamp(3);
    m_initialUser.setActive(true);

    ErrorString errorDescription;
    if (!m_initialUser.checkParameters(errorDescription)) {
        QNWARNING(QStringLiteral("Found invalid user: ") << m_initialUser << QStringLiteral(", error: ") << errorDescription);
        emit failure(errorDescription.nonLocalizedString());
        return;
    }

    m_state = STATE_SENT_ADD_REQUEST;
    emit addUserRequest(m_initialUser);
}

void UserLocalStorageManagerAsyncTester::onGetUserCountCompleted(int count, QUuid requestId)
{
    Q_UNUSED(requestId)

    ErrorString errorDescription;

#define HANDLE_WRONG_STATE() \
    else { \
        errorDescription.base() = QStringLiteral("Internal error in UserLocalStorageManagerAsyncTester: found wrong state"); \
        emit failure(errorDescription.nonLocalizedString()); \
    }

    if (m_state == STATE_SENT_GET_COUNT_AFTER_UPDATE_REQUEST)
    {
        if (count != 1) {
            errorDescription.base() = QStringLiteral("GetUserCount returned result different from the expected one (1)");
            errorDescription.details() = QString::number(count);
            emit failure(errorDescription.nonLocalizedString());
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
            errorDescription.base() = QStringLiteral("GetUserCount returned result different from the expected one (0)");
            errorDescription.details() = QString::number(count);
            emit failure(errorDescription.nonLocalizedString());
            return;
        }

        emit success();
    }
    HANDLE_WRONG_STATE();
}

void UserLocalStorageManagerAsyncTester::onGetUserCountFailed(ErrorString errorDescription, QUuid requestId)
{
    QNWARNING(errorDescription << QStringLiteral(", requestId = ") << requestId);
    emit failure(errorDescription.nonLocalizedString());
}

void UserLocalStorageManagerAsyncTester::onAddUserCompleted(User user, QUuid requestId)
{
    Q_UNUSED(requestId)

    ErrorString errorDescription;

    if (m_state == STATE_SENT_ADD_REQUEST)
    {
        if (m_initialUser != user) {
            errorDescription.base() = QStringLiteral("Internal error in UserLocalStorageManagerAsyncTester: "
                                                     "user in onAddUserCompleted doesn't match the original User");
            QNWARNING(errorDescription);
            emit failure(errorDescription.nonLocalizedString());
            return;
        }

        m_foundUser = User();
        m_foundUser.setId(user.id());

        m_state = STATE_SENT_FIND_AFTER_ADD_REQUEST;
        emit findUserRequest(m_foundUser);
    }
    HANDLE_WRONG_STATE();
}

void UserLocalStorageManagerAsyncTester::onAddUserFailed(User user, ErrorString errorDescription, QUuid requestId)
{
    QNWARNING(errorDescription << QStringLiteral(", requestId = ") << requestId << QStringLiteral(", user: ") << user);
    emit failure(errorDescription.nonLocalizedString());
}

void UserLocalStorageManagerAsyncTester::onUpdateUserCompleted(User user, QUuid requestId)
{
    Q_UNUSED(requestId)

    ErrorString errorDescription;

    if (m_state == STATE_SENT_UPDATE_REQUEST)
    {
        if (m_modifiedUser != user) {
            errorDescription.base() = QStringLiteral("Internal error in UserLocalStorageManagerAsyncTester: "
                                                     "user in onUpdateUserCompleted slot doesn't match "
                                                     "the original modified User");
            QNWARNING(errorDescription);
            emit failure(errorDescription.nonLocalizedString());
            return;
        }

        m_state = STATE_SENT_FIND_AFTER_UPDATE_REQUEST;
        emit findUserRequest(m_foundUser);
    }
    HANDLE_WRONG_STATE();
}

void UserLocalStorageManagerAsyncTester::onUpdateUserFailed(User user, ErrorString errorDescription, QUuid requestId)
{
    QNWARNING(errorDescription << QStringLiteral(", requestId = ") << requestId << QStringLiteral(", user: ") << user);
    emit failure(errorDescription.nonLocalizedString());
}

void UserLocalStorageManagerAsyncTester::onFindUserCompleted(User user, QUuid requestId)
{
    Q_UNUSED(requestId)

    ErrorString errorDescription;

    if (m_state == STATE_SENT_FIND_AFTER_ADD_REQUEST)
    {
        if (user != m_initialUser) {
            errorDescription.base() = QStringLiteral("Added and found users in local storage don't match");
            QNWARNING(errorDescription << QStringLiteral(": User added to LocalStorageManager: ") << m_initialUser
                      << QStringLiteral("\nUserWrapper found in LocalStorageManager: ") << user);
            emit failure(errorDescription.nonLocalizedString());
            return;
        }

        // Ok, found user is good, updating it now
        m_modifiedUser = m_initialUser;
        m_modifiedUser.setUsername(m_initialUser.username() + QStringLiteral("_modified"));
        m_modifiedUser.setName(m_initialUser.name() + QStringLiteral("_modified"));

        m_state = STATE_SENT_UPDATE_REQUEST;
        emit updateUserRequest(m_modifiedUser);
    }
    else if (m_state == STATE_SENT_FIND_AFTER_UPDATE_REQUEST)
    {
        if (user != m_modifiedUser) {
            errorDescription.base() = QStringLiteral("Updated and found users in local storage don't match");
            QNWARNING(errorDescription << QStringLiteral(": User updated in LocalStorageManager: ") << m_modifiedUser
                      << QStringLiteral("\nUserWrapper found in LocalStorageManager: ") << user);
            emit failure(errorDescription.nonLocalizedString());
            return;
        }

        m_state = STATE_SENT_GET_COUNT_AFTER_UPDATE_REQUEST;
        emit getUserCountRequest();
    }
    else if (m_state == STATE_SENT_FIND_AFTER_EXPUNGE_REQUEST)
    {
        errorDescription.base() = QStringLiteral("Error: found user which should have been expunged from local storage");
        QNWARNING(errorDescription << QStringLiteral(": User expunged from LocalStorageManager: ") << m_modifiedUser
                  << QStringLiteral("\nUserWrapper found in LocalStorageManager: ") << user);
        emit failure(errorDescription.nonLocalizedString());
        return;
    }
    HANDLE_WRONG_STATE();
}

void UserLocalStorageManagerAsyncTester::onFindUserFailed(User user, ErrorString errorDescription, QUuid requestId)
{
    if (m_state == STATE_SENT_FIND_AFTER_EXPUNGE_REQUEST) {
        m_state = STATE_SENT_GET_COUNT_AFTER_EXPUNGE_REQUEST;
        emit getUserCountRequest();
        return;
    }

    QNWARNING(errorDescription << QStringLiteral(", requestId = ") << requestId << QStringLiteral(", user: ") << user);
    emit failure(errorDescription.nonLocalizedString());
}

void UserLocalStorageManagerAsyncTester::onDeleteUserCompleted(User user, QUuid requestId)
{
    Q_UNUSED(requestId)

    ErrorString errorDescription;

    if (m_modifiedUser != user) {
        errorDescription.base() = QStringLiteral("Internal error in UserLocalStorageManagerAsyncTester: "
                                                 "user in onDeleteUserCompleted slot doesn't match "
                                                 "the original deleted User");
        QNWARNING(errorDescription << QStringLiteral("; original deleted user: ") << m_modifiedUser << QStringLiteral(", user: ") << user);
        emit failure(errorDescription.nonLocalizedString());
        return;
    }

    m_modifiedUser.setLocal(true);
    m_state = STATE_SENT_EXPUNGE_REQUEST;
    emit expungeUserRequest(m_modifiedUser);
}

void UserLocalStorageManagerAsyncTester::onDeleteUserFailed(User user, ErrorString errorDescription, QUuid requestId)
{
    QNWARNING(errorDescription << QStringLiteral(", requestId = ") << requestId << QStringLiteral(", user: ") << user);
    emit failure(errorDescription.nonLocalizedString());
}

void UserLocalStorageManagerAsyncTester::onExpungeUserCompleted(User user, QUuid requestId)
{
    Q_UNUSED(requestId)

    ErrorString errorDescription;

    if (m_modifiedUser != user) {
        errorDescription.base() = QStringLiteral("Internal error in UserLocalStorageManagerAsyncTester: "
                                                 "user in onExpungeUserCompleted slot doesn't match "
                                                 "the original expunged User");
        QNWARNING(errorDescription);
        emit failure(errorDescription.nonLocalizedString());
    }

    m_state = STATE_SENT_FIND_AFTER_EXPUNGE_REQUEST;
    emit findUserRequest(m_foundUser);
}

void UserLocalStorageManagerAsyncTester::onExpungeUserFailed(User user, ErrorString errorDescription, QUuid requestId)
{
    QNWARNING(errorDescription << QStringLiteral(", requestId = ") << requestId << QStringLiteral(", user: ") << user);
    emit failure(errorDescription.nonLocalizedString());
}

void UserLocalStorageManagerAsyncTester::onFailure(ErrorString errorDescription)
{
    QNWARNING("UserLocalStorageManagerAsyncTester::onFailure: " << errorDescription);
    emit failure(errorDescription.nonLocalizedString());
}

void UserLocalStorageManagerAsyncTester::createConnections()
{
    QObject::connect(m_pLocalStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,failure,ErrorString),
                     this, QNSLOT(UserLocalStorageManagerAsyncTester,onFailure,ErrorString));

    QObject::connect(m_pLocalStorageManagerThread, QNSIGNAL(QThread,started),
                     m_pLocalStorageManagerThreadWorker, QNSLOT(LocalStorageManagerThreadWorker,init));
    QObject::connect(m_pLocalStorageManagerThread, QNSIGNAL(QThread,finished),
                     m_pLocalStorageManagerThread, QNSLOT(QThread,deleteLater));

    QObject::connect(m_pLocalStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,initialized),
                     this, QNSLOT(UserLocalStorageManagerAsyncTester,onWorkerInitialized));

    // Request --> slot connections
    QObject::connect(this, QNSIGNAL(UserLocalStorageManagerAsyncTester,getUserCountRequest,QUuid),
                     m_pLocalStorageManagerThreadWorker, QNSLOT(LocalStorageManagerThreadWorker,onGetUserCountRequest,QUuid));
    QObject::connect(this, QNSIGNAL(UserLocalStorageManagerAsyncTester,addUserRequest,User,QUuid),
                     m_pLocalStorageManagerThreadWorker, QNSLOT(LocalStorageManagerThreadWorker,onAddUserRequest,User,QUuid));
    QObject::connect(this, QNSIGNAL(UserLocalStorageManagerAsyncTester,updateUserRequest,User,QUuid),
                     m_pLocalStorageManagerThreadWorker, QNSLOT(LocalStorageManagerThreadWorker,onUpdateUserRequest,User,QUuid));
    QObject::connect(this, QNSIGNAL(UserLocalStorageManagerAsyncTester,findUserRequest,User,QUuid),
                     m_pLocalStorageManagerThreadWorker, QNSLOT(LocalStorageManagerThreadWorker,onFindUserRequest,User,QUuid));
    QObject::connect(this, QNSIGNAL(UserLocalStorageManagerAsyncTester,deleteUserRequest,User,QUuid),
                     m_pLocalStorageManagerThreadWorker, QNSLOT(LocalStorageManagerThreadWorker,onDeleteUserRequest,User,QUuid));
    QObject::connect(this, QNSIGNAL(UserLocalStorageManagerAsyncTester,expungeUserRequest,User,QUuid),
                     m_pLocalStorageManagerThreadWorker, QNSLOT(LocalStorageManagerThreadWorker,onExpungeUserRequest,User,QUuid));

    // Slot <-- result connections
    QObject::connect(m_pLocalStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,getUserCountComplete,int,QUuid),
                     this, QNSLOT(UserLocalStorageManagerAsyncTester,onGetUserCountCompleted,int,QUuid));
    QObject::connect(m_pLocalStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,getUserCountFailed,ErrorString,QUuid),
                     this, QNSLOT(UserLocalStorageManagerAsyncTester,onGetUserCountFailed,ErrorString,QUuid));
    QObject::connect(m_pLocalStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,addUserComplete,User,QUuid),
                     this, QNSLOT(UserLocalStorageManagerAsyncTester,onAddUserCompleted,User,QUuid));
    QObject::connect(m_pLocalStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,addUserFailed,User,ErrorString,QUuid),
                     this, QNSLOT(UserLocalStorageManagerAsyncTester,onAddUserFailed,User,ErrorString,QUuid));
    QObject::connect(m_pLocalStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,updateUserComplete,User,QUuid),
                     this, QNSLOT(UserLocalStorageManagerAsyncTester,onUpdateUserCompleted,User,QUuid));
    QObject::connect(m_pLocalStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,updateUserFailed,User,ErrorString,QUuid),
                     this, QNSLOT(UserLocalStorageManagerAsyncTester,onUpdateUserFailed,User,ErrorString,QUuid));
    QObject::connect(m_pLocalStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,findUserComplete,User,QUuid),
                     this, QNSLOT(UserLocalStorageManagerAsyncTester,onFindUserCompleted,User,QUuid));
    QObject::connect(m_pLocalStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,findUserFailed,User,ErrorString,QUuid),
                     this, QNSLOT(UserLocalStorageManagerAsyncTester,onFindUserFailed,User,ErrorString,QUuid));
    QObject::connect(m_pLocalStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,deleteUserComplete,User,QUuid),
                     this, QNSLOT(UserLocalStorageManagerAsyncTester,onDeleteUserCompleted,User,QUuid));
    QObject::connect(m_pLocalStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,deleteUserFailed,User,ErrorString,QUuid),
                     this, QNSLOT(UserLocalStorageManagerAsyncTester,onDeleteUserFailed,User,ErrorString,QUuid));
    QObject::connect(m_pLocalStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,expungeUserComplete,User,QUuid),
                     this, QNSLOT(UserLocalStorageManagerAsyncTester,onExpungeUserCompleted,User,QUuid));
    QObject::connect(m_pLocalStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,expungeUserFailed,User,ErrorString,QUuid),
                     this, QNSLOT(UserLocalStorageManagerAsyncTester,onExpungeUserFailed,User,ErrorString,QUuid));
}

#undef HANDLE_WRONG_STATE

} // namespace test
} // namespace quentier

