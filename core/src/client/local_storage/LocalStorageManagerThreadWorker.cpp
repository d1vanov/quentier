#include "LocalStorageManagerThreadWorker.h"
#include <logging/QuteNoteLogger.h>

namespace qute_note {

LocalStorageManagerThreadWorker::LocalStorageManagerThreadWorker(const QString & username,
                                                                 const qint32 userId,
                                                                 const bool startFromScratch, QObject * parent) :
    QObject(parent),
    m_localStorageManager(username, userId, startFromScratch)
{}

LocalStorageManagerThreadWorker::~LocalStorageManagerThreadWorker()
{}

void LocalStorageManagerThreadWorker::onSwitchUserRequest(QString username, qint32 userId,
                                                          bool startFromScratch)
{
    try {
        m_localStorageManager.SwitchUser(username, userId, startFromScratch);
    }
    catch(const std::exception & exception) {
        emit switchUserFailed(userId, QString(exception.what()));
        return;
    }

    emit switchUserComplete(userId);
}

void LocalStorageManagerThreadWorker::onAddUserRequest(QSharedPointer<IUser> user)
{
    QString errorDescription;

    if (user.isNull()) {
        errorDescription = QObject::tr("Internal error: detected null pointer on attempt "
                                       "to add user to local storage");
        QNCRITICAL(errorDescription);
        emit addUserFailed(user, errorDescription);
        return;
    }

    bool res = m_localStorageManager.AddUser(*user, errorDescription);
    if (!res) {
        emit addUserFailed(user, errorDescription);
        return;
    }

    emit addUserComplete(user);
}

void LocalStorageManagerThreadWorker::onUpdateUserRequest(QSharedPointer<IUser> user)
{
    QString errorDescription;

    if (user.isNull()) {
        errorDescription = QObject::tr("Internal error: detected null pointer on attempt "
                                       "to update user in local storage");
        QNCRITICAL(errorDescription);
        emit updateUserFailed(user, errorDescription);
        return;
    }

    bool res = m_localStorageManager.UpdateUser(*user, errorDescription);
    if (!res) {
        emit updateUserFailed(user, errorDescription);
        return;
    }

    emit updateUserComplete(user);
}

void LocalStorageManagerThreadWorker::onFindUserRequest(QSharedPointer<IUser> user)
{
    QString errorDescription;

    if (user.isNull()) {
        errorDescription = QObject::tr("Internal error: detected null pointer "
                                       "on attempt to find user in local storage");
        QNCRITICAL(errorDescription);
        emit findUserFailed(user, errorDescription);
        return;
    }

    bool res = m_localStorageManager.FindUser(*user, errorDescription);
    if (!res) {
        emit findUserFailed(user, errorDescription);
        return;
    }

    emit findUserComplete(user);
}

void LocalStorageManagerThreadWorker::onDeleteUserRequest(QSharedPointer<IUser> user)
{
    QString errorDescription;

    if (user.isNull()) {
        errorDescription = QObject::tr("Internal error: detected null pointer "
                                       "on attempt to delete user in local storage");
        QNCRITICAL(errorDescription);
        emit deleteUserFailed(user, errorDescription);
        return;
    }

    bool res = m_localStorageManager.DeleteUser(*user, errorDescription);
    if (!res) {
        emit deleteUserFailed(user, errorDescription);
        return;
    }

    emit deleteUserComplete(user);
}

void LocalStorageManagerThreadWorker::onExpungeUserRequest(QSharedPointer<IUser> user)
{
    QString errorDescription;

    if (user.isNull()) {
        errorDescription = QObject::tr("Internal error: detected null pointer "
                                       "on attempt to expunge user from local storage");
        QNCRITICAL(errorDescription);
        emit expungeUserFailed(user, errorDescription);
        return;
    }

    bool res = m_localStorageManager.ExpungeUser(*user, errorDescription);
    if (!res) {
        emit expungeUserFailed(user, errorDescription);
        return;
    }

    emit expungeUserComplete(user);
}

} // namespace qute_note
