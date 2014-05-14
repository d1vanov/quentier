#include "LocalStorageManagerThread.h"
#include "LocalStorageManagerThreadWorker.h"

namespace qute_note {

LocalStorageManagerThread::LocalStorageManagerThread(const QString & username,
                                                     const qint32 userId,
                                                     const bool startFromScratch,
                                                     QObject * parent) :
    QThread(parent),
    m_pWorker(new LocalStorageManagerThreadWorker(username, userId, startFromScratch, this))
{
    createConnections();
}

void LocalStorageManagerThread::createConnections()
{
    // User-related signal-slot connections:
    QObject::connect(this, SIGNAL(switchUserRequest(QString,qint32,bool)),
                     m_pWorker, SLOT(onSwitchUserRequest(QString,qint32,bool)));
    QObject::connect(this, SIGNAL(addUserRequest(QSharedPointer<IUser>)),
                     m_pWorker, SLOT(onAddUserRequest(QSharedPointer<IUser>)));
    QObject::connect(this, SIGNAL(updateUserRequest(QSharedPointer<IUser>)),
                     m_pWorker, SLOT(onUpdateUserRequest(QSharedPointer<IUser>)));
    QObject::connect(this, SIGNAL(findUserRequest(QSharedPointer<IUser>)),
                     m_pWorker, SLOT(onFindUserRequest(QSharedPointer<IUser>)));
    QObject::connect(this, SIGNAL(deleteUserRequest(QSharedPointer<IUser>)),
                     m_pWorker, SLOT(onDeleteUserRequest(QSharedPointer<IUser>)));
    QObject::connect(this, SIGNAL(expungeUserRequest(QSharedPointer<IUser>)),
                     m_pWorker, SLOT(onExpungeUserRequest(QSharedPointer<IUser>)));

    // User-related signal-signal connections:
    QObject::connect(m_pWorker, SIGNAL(switchUserComplete(qint32)),
                     this, SIGNAL(switchUserComplete(qint32)));
    QObject::connect(m_pWorker, SIGNAL(switchUserFailed(qint32,QString)),
                     this, SIGNAL(switchUserFailed(qint32,QString)));
    QObject::connect(m_pWorker, SIGNAL(addUserComplete(QSharedPointer<IUser>)),
                     this, SIGNAL(addUserComplete(QSharedPointer<IUser>)));
    QObject::connect(m_pWorker, SIGNAL(addUserFailed(QSharedPointer<IUser>,QString)),
                     this, SIGNAL(addUserFailed(QSharedPointer<IUser>,QString)));
    QObject::connect(m_pWorker, SIGNAL(updateUserComplete(QSharedPointer<IUser>)),
                     this, SIGNAL(updateUserComplete(QSharedPointer<IUser>)));
    QObject::connect(m_pWorker, SIGNAL(updateUserFailed(QSharedPointer<IUser>,QString)),
                     this, SIGNAL(updateUserFailed(QSharedPointer<IUser>,QString)));
    QObject::connect(m_pWorker, SIGNAL(findUserComplete(QSharedPointer<IUser>)),
                     this, SIGNAL(findUserComplete(QSharedPointer<IUser>)));
    QObject::connect(m_pWorker, SIGNAL(findUserFailed(QSharedPointer<IUser>,QString)),
                     this, SIGNAL(findUserFailed(QSharedPointer<IUser>,QString)));
    QObject::connect(m_pWorker, SIGNAL(deleteUserComplete(QSharedPointer<IUser>)),
                     this, SIGNAL(deleteUserComplete(QSharedPointer<IUser>)));
    QObject::connect(m_pWorker, SIGNAL(deleteUserFailed(QSharedPointer<IUser>,QString)),
                     this, SIGNAL(deleteUserFailed(QSharedPointer<IUser>,QString)));
    QObject::connect(m_pWorker, SIGNAL(expungeUserComplete(QSharedPointer<IUser>)),
                     this, SIGNAL(expungeUserComplete(QSharedPointer<IUser>)));
    QObject::connect(m_pWorker, SIGNAL(expungeUserFailed(QSharedPointer<IUser>,QString)),
                     this, SIGNAL(expungeUserFailed(QSharedPointer<IUser>,QString)));
}

LocalStorageManagerThread::~LocalStorageManagerThread()
{}

void LocalStorageManagerThread::onSwitchUserRequest(QString username, qint32 userId, bool startFromScratch)
{
    emit switchUserRequest(username, userId, startFromScratch);
}

void LocalStorageManagerThread::onAddUserRequest(QSharedPointer<IUser> user)
{
    emit addUserRequest(user);
}

void LocalStorageManagerThread::onUpdateUserRequest(QSharedPointer<IUser> user)
{
    emit updateUserRequest(user);
}

void LocalStorageManagerThread::onFindUserRequest(QSharedPointer<IUser> user)
{
    emit findUserRequest(user);
}

void LocalStorageManagerThread::onDeleteUserRequest(QSharedPointer<IUser> user)
{
    emit deleteUserRequest(user);
}

void LocalStorageManagerThread::onExpungeUserRequest(QSharedPointer<IUser> user)
{
    emit expungeUserRequest(user);
}

}
