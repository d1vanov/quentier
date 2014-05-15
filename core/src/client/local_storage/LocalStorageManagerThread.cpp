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

    // Notebook-related signal-slot connections:
    QObject::connect(this, SIGNAL(addNotebookRequest(QSharedPointer<Notebook>)),
                     m_pWorker, SLOT(onAddNotebookRequest(QSharedPointer<Notebook>)));
    QObject::connect(this, SIGNAL(updateNotebookRequest(QSharedPointer<Notebook>)),
                     m_pWorker, SLOT(onUpdateNotebookRequest(QSharedPointer<Notebook>)));
    QObject::connect(this, SIGNAL(findNotebookRequest(QSharedPointer<Notebook>)),
                     m_pWorker, SLOT(onFindNotebookRequest(QSharedPointer<Notebook>)));
    QObject::connect(this, SIGNAL(listAllNotebooksRequest()), m_pWorker, SLOT(onListAllNotebooksRequest()));
    QObject::connect(this, SIGNAL(listAllSharedNotebooksRequest()), m_pWorker, SLOT(onListAllSharedNotebooksRequest()));
    QObject::connect(this, SIGNAL(listSharedNotebooksPerNotebookGuidRequest(QString)),
                     m_pWorker, SLOT(onListSharedNotebooksPerNotebookGuidRequest(QString)));
    QObject::connect(this, SIGNAL(expungeNotebookRequest(QSharedPointer<Notebook>)),
                     m_pWorker, SLOT(onExpungeNotebookRequest(QSharedPointer<Notebook>)));

    // Notebook-related signal-signal connections:
    QObject::connect(m_pWorker, SIGNAL(addNotebookComplete(QSharedPointer<Notebook>)),
                     this, SIGNAL(addNotebookComplete(QSharedPointer<Notebook>)));
    QObject::connect(m_pWorker, SIGNAL(addNotebookFailed(QSharedPointer<Notebook>,QString)),
                     this, SIGNAL(addNotebookFailed(QSharedPointer<Notebook>,QString)));
    QObject::connect(m_pWorker, SIGNAL(updateNotebookComplete(QSharedPointer<Notebook>)),
                     this, SIGNAL(updateNotebookComplete(QSharedPointer<Notebook>)));
    QObject::connect(m_pWorker, SIGNAL(updateNotebookFailed(QSharedPointer<Notebook>,QString)),
                     this, SIGNAL(updateNotebookFailed(QSharedPointer<Notebook>,QString)));
    QObject::connect(m_pWorker, SIGNAL(findNotebookComplete(QSharedPointer<Notebook>)),
                     this, SIGNAL(findNotebookComplete(QSharedPointer<Notebook>)));
    QObject::connect(m_pWorker, SIGNAL(findNotebookFailed(QSharedPointer<Notebook>,QString)),
                     this, SIGNAL(findNotebookFailed(QSharedPointer<Notebook>,QString)));
    QObject::connect(m_pWorker, SIGNAL(listAllLinkedNotebooksComplete(QList<LinkedNotebook>)),
                     this, SIGNAL(listAllLinkedNotebooksComplete(QList<LinkedNotebook>)));
    QObject::connect(m_pWorker, SIGNAL(listAllLinkedNotebooksFailed(QString)),
                     this, SIGNAL(listAllLinkedNotebooksFailed(QString)));
    QObject::connect(m_pWorker, SIGNAL(listAllSharedNotebooksComplete(QList<SharedNotebookWrapper>)),
                     this, SIGNAL(listAllSharedNotebooksComplete(QList<SharedNotebookWrapper>)));
    QObject::connect(m_pWorker, SIGNAL(listAllSharedNotebooksFailed(QString)),
                     this, SIGNAL(listAllSharedNotebooksFailed(QString)));
    QObject::connect(m_pWorker, SIGNAL(listSharedNotebooksPerNotebookGuidComplete(QString,QList<SharedNotebookWrapper>)),
                     this, SIGNAL(listSharedNotebooksPerNotebookGuidComplete(QString,QList<SharedNotebookWrapper>)));
    QObject::connect(m_pWorker, SIGNAL(listSharedNotebooksPerNotebookGuidFailed(QString,QString)),
                     this, SIGNAL(listSharedNotebooksPerNotebookGuidFailed(QString,QString)));
    QObject::connect(m_pWorker, SIGNAL(expungeNotebookComplete(QSharedPointer<Notebook>)),
                     this, SIGNAL(expungeNotebookComplete(QSharedPointer<Notebook>)));
    QObject::connect(m_pWorker, SIGNAL(expungeNotebookFailed(QSharedPointer<Notebook>,QString)),
                     this, SIGNAL(expungeNotebookFailed(QSharedPointer<Notebook>,QString)));

    // Linked notebook-related signal-slot connections:
    QObject::connect(this, SIGNAL(addLinkedNotebookRequest(QSharedPointer<LinkedNotebook>)),
                     m_pWorker, SLOT(onAddLinkedNotebookRequest(QSharedPointer<LinkedNotebook>)));
    QObject::connect(this, SIGNAL(updateLinkedNotebookRequest(QSharedPointer<LinkedNotebook>)),
                     m_pWorker, SLOT(onUpdateLinkedNotebookRequest(QSharedPointer<LinkedNotebook>)));
    QObject::connect(this, SIGNAL(findLinkedNotebookRequest(QSharedPointer<LinkedNotebook>)),
                     m_pWorker, SLOT(onFindLinkedNotebookRequest(QSharedPointer<LinkedNotebook>)));
    QObject::connect(this, SIGNAL(listAllLinkedNotebooksRequest()), m_pWorker, SLOT(onListAllLinkedNotebooksRequest()));
    QObject::connect(this, SIGNAL(expungeLinkedNotebookRequest(QSharedPointer<LinkedNotebook>)),
                     m_pWorker, SLOT(onExpungeLinkedNotebookRequest(QSharedPointer<LinkedNotebook>)));

    // Linked notebook-related signal-signal connections:
    QObject::connect(m_pWorker, SIGNAL(addLinkedNotebookComplete(QSharedPointer<LinkedNotebook>)),
                     this, SIGNAL(addLinkedNotebookComplete(QSharedPointer<LinkedNotebook>)));
    QObject::connect(m_pWorker, SIGNAL(addLinkedNotebookFailed(QSharedPointer<LinkedNotebook>,QString)),
                     this, SIGNAL(addLinkedNotebookFailed(QSharedPointer<LinkedNotebook>,QString)));
    QObject::connect(m_pWorker, SIGNAL(updateLinkedNotebookComplete(QSharedPointer<LinkedNotebook>)),
                     this, SIGNAL(updateLinkedNotebookComplete(QSharedPointer<LinkedNotebook>)));
    QObject::connect(m_pWorker, SIGNAL(updateLinkedNotebookFailed(QSharedPointer<LinkedNotebook>,QString)),
                     this, SIGNAL(updateLinkedNotebookFailed(QSharedPointer<LinkedNotebook>,QString)));
    QObject::connect(m_pWorker, SIGNAL(findLinkedNotebookCompleted(QSharedPointer<LinkedNotebook>)),
                     this, SIGNAL(findLinkedNotebookCompleted(QSharedPointer<LinkedNotebook>)));
    QObject::connect(m_pWorker, SIGNAL(findLinkedNotebookFailed(QSharedPointer<LinkedNotebook>,QString)),
                     this, SIGNAL(findLinkedNotebookFailed(QSharedPointer<LinkedNotebook>,QString)));
    QObject::connect(m_pWorker, SIGNAL(listAllLinkedNotebooksComplete(QList<LinkedNotebook>)),
                     this, SIGNAL(listAllLinkedNotebooksComplete(QList<LinkedNotebook>)));
    QObject::connect(m_pWorker, SIGNAL(listAllLinkedNotebooksFailed(QString)),
                     this, SIGNAL(listAllLinkedNotebooksFailed(QString)));
    QObject::connect(m_pWorker, SIGNAL(expungeLinkedNotebookCompleted(QSharedPointer<LinkedNotebook>)),
                     this, SIGNAL(expungeLinkedNotebookCompleted(QSharedPointer<LinkedNotebook>)));
    QObject::connect(m_pWorker, SIGNAL(expungeLinkedNotebookFailed(QSharedPointer<LinkedNotebook>,QString)),
                     this, SIGNAL(expungeLinkedNotebookFailed(QSharedPointer<LinkedNotebook>,QString)));
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

void LocalStorageManagerThread::onAddNotebookRequest(QSharedPointer<Notebook> notebook)
{
    emit addNotebookRequest(notebook);
}

void LocalStorageManagerThread::onUpdateNotebookRequest(QSharedPointer<Notebook> notebook)
{
    emit updateNotebookRequest(notebook);
}

void LocalStorageManagerThread::onFindNotebookRequest(QSharedPointer<Notebook> notebook)
{
    emit findNotebookRequest(notebook);
}

void LocalStorageManagerThread::onListAllNotebooksRequest()
{
    emit listAllNotebooksRequest();
}

void LocalStorageManagerThread::onListAllSharedNotebooksRequest()
{
    emit listAllSharedNotebooksRequest();
}

void LocalStorageManagerThread::onListSharedNotebooksPerNotebookGuidRequest(QString notebookGuid)
{
    emit listSharedNotebooksPerNotebookGuidRequest(notebookGuid);
}

void LocalStorageManagerThread::onExpungeNotebookRequest(QSharedPointer<Notebook> notebook)
{
    emit expungeNotebookRequest(notebook);
}

void LocalStorageManagerThread::onAddLinkedNotebookRequest(QSharedPointer<LinkedNotebook> linkedNotebook)
{
    emit addLinkedNotebookRequest(linkedNotebook);
}

void LocalStorageManagerThread::onUpdateLinkedNotebookRequest(QSharedPointer<LinkedNotebook> linkedNotebook)
{
    emit updateLinkedNotebookRequest(linkedNotebook);
}

void LocalStorageManagerThread::onFindLinkedNotebookRequest(QSharedPointer<LinkedNotebook> linkedNotebook)
{
    emit findLinkedNotebookRequest(linkedNotebook);
}

void LocalStorageManagerThread::onListAllLinkedNotebooksRequest()
{
    emit listAllLinkedNotebooksRequest();
}

void LocalStorageManagerThread::onExpungeLinkedNotebookRequest(QSharedPointer<LinkedNotebook> linkedNotebook)
{
    emit expungeLinkedNotebookRequest(linkedNotebook);
}

}
