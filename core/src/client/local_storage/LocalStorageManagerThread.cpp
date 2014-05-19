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

    // Note-related signal-slot connections:
    QObject::connect(this, SIGNAL(addNoteRequest(QSharedPointer<Note>,QSharedPointer<Notebook>)),
                     m_pWorker, SLOT(onAddNoteRequest(QSharedPointer<Note>,QSharedPointer<Notebook>)));
    QObject::connect(this, SIGNAL(updateNoteRequest(QSharedPointer<Note>,QSharedPointer<Notebook>)),
                     m_pWorker, SLOT(onUpdateNoteRequest(QSharedPointer<Note>,QSharedPointer<Notebook>)));
    QObject::connect(this, SIGNAL(findNoteRequest(QSharedPointer<Note>,bool)),
                     m_pWorker, SLOT(onFindNoteRequest(QSharedPointer<Note>,bool)));
    QObject::connect(this, SIGNAL(listAllNotesPerNotebookRequest(QSharedPointer<Notebook>,bool)),
                     m_pWorker, SLOT(onListAllNotesPerNotebookRequest(QSharedPointer<Notebook>,bool)));
    QObject::connect(this, SIGNAL(deleteNoteRequest(QSharedPointer<Note>)),
                     m_pWorker, SLOT(onDeleteNoteRequest(QSharedPointer<Note>)));
    QObject::connect(this, SIGNAL(expungeNoteRequest(QSharedPointer<Note>)),
                     m_pWorker, SLOT(onExpungeNoteRequest(QSharedPointer<Note>)));

    // Note-related signal-signal connections:
    QObject::connect(m_pWorker, SIGNAL(addNoteComplete(QSharedPointer<Note>,QSharedPointer<Notebook>)),
                     this, SIGNAL(addNoteComplete(QSharedPointer<Note>,QSharedPointer<Notebook>)));
    QObject::connect(m_pWorker, SIGNAL(addNoteFailed(QSharedPointer<Note>,QSharedPointer<Notebook>,QString)),
                     this, SIGNAL(addNoteFailed(QSharedPointer<Note>,QSharedPointer<Notebook>,QString)));
    QObject::connect(m_pWorker, SIGNAL(updateNoteComplete(QSharedPointer<Note>,QSharedPointer<Notebook>)),
                     this, SIGNAL(updateNoteComplete(QSharedPointer<Note>,QSharedPointer<Notebook>,QString)));
    QObject::connect(m_pWorker, SIGNAL(updateNoteFailed(QSharedPointer<Note>,QSharedPointer<Notebook>,QString)),
                     this, SIGNAL(updateNoteFailed(QSharedPointer<Note>,QSharedPointer<Notebook>,QString)));
    QObject::connect(m_pWorker, SIGNAL(findNoteComplete(QSharedPointer<Note>)),
                     this, SIGNAL(findNoteComplete(QSharedPointer<Note>)));
    QObject::connect(m_pWorker, SIGNAL(findNoteFailed(QSharedPointer<Note>,QString)),
                     this, SIGNAL(findNoteFailed(QSharedPointer<Note>,QString)));
    QObject::connect(m_pWorker, SIGNAL(listAllNotesPerNotebookComplete(QSharedPointer<Notebook>,QList<Note>)),
                     this, SIGNAL(listAllNotesPerNotebookComplete(QSharedPointer<Notebook>,QList<Note>)));
    QObject::connect(m_pWorker, SIGNAL(listAllNotesPerNotebookFailed(QSharedPointer<Notebook>,QString)),
                     this, SIGNAL(listAllNotesPerNotebookFailed(QSharedPointer<Notebook>,QString)));
    QObject::connect(m_pWorker, SIGNAL(deleteNoteComplete(QSharedPointer<Note>)),
                     this, SIGNAL(deleteNoteComplete(QSharedPointer<Note>)));
    QObject::connect(m_pWorker, SIGNAL(deleteNoteFailed(QSharedPointer<Note>,QString)),
                     this, SIGNAL(deleteNoteFailed(QSharedPointer<Note>,QString)));
    QObject::connect(m_pWorker, SIGNAL(expungeNoteComplete(QSharedPointer<Note>)),
                     this, SIGNAL(expungeNoteComplete(QSharedPointer<Note>)));
    QObject::connect(m_pWorker, SIGNAL(expungeNoteFailed(QSharedPointer<Note>,QString)),
                     this, SIGNAL(expungeNoteFailed(QSharedPointer<Note>,QString)));

    // Tag-related signal-slot connections:
    QObject::connect(this, SIGNAL(addTagRequest(QSharedPointer<Tag>)),
                     m_pWorker, SLOT(onAddTagRequest(QSharedPointer<Tag>)));
    QObject::connect(this, SIGNAL(updateTagRequest(QSharedPointer<Tag>)),
                     m_pWorker, SLOT(onUpdateTagRequest(QSharedPointer<Tag>)));
    QObject::connect(this, SIGNAL(linkTagWithNoteRequest(QSharedPointer<Tag>,QSharedPointer<Note>)),
                     m_pWorker, SLOT(onLinkTagWithNoteRequest(QSharedPointer<Tag>,QSharedPointer<Note>)));
    QObject::connect(this, SIGNAL(findTagRequest(QSharedPointer<Tag>)),
                     m_pWorker, SLOT(onFindTagRequest(QSharedPointer<Tag>)));
    QObject::connect(this, SIGNAL(listAllTagsPerNoteRequest(QSharedPointer<Note>)),
                     m_pWorker, SLOT(onListAllTagsPerNoteRequest(QSharedPointer<Note>)));
    QObject::connect(this, SIGNAL(listAllTagsRequest()), m_pWorker, SLOT(onListAllTagsRequest()));
    QObject::connect(this, SIGNAL(deleteTagRequest(QSharedPointer<Tag>)),
                     m_pWorker, SLOT(onDeleteTagRequest(QSharedPointer<Tag>)));
    QObject::connect(this, SIGNAL(expungeTagRequest(QSharedPointer<Tag>)),
                     m_pWorker, SLOT(onExpungeTagRequest(QSharedPointer<Tag>)));

    // Tag-related signal-signal connections:
    QObject::connect(m_pWorker, SIGNAL(addTagComplete(QSharedPointer<Tag>)),
                     this, SIGNAL(addTagComplete(QSharedPointer<Tag>)));
    QObject::connect(m_pWorker, SIGNAL(addTagFailed(QSharedPointer<Tag>,QString)),
                     this, SIGNAL(addTagFailed(QSharedPointer<Tag>,QString)));
    QObject::connect(m_pWorker, SIGNAL(updateTagComplete(QSharedPointer<Tag>)),
                     this, SIGNAL(updateTagComplete(QSharedPointer<Tag>)));
    QObject::connect(m_pWorker, SIGNAL(updateTagFailed(QSharedPointer<Tag>,QString)),
                     this, SIGNAL(updateTagFailed(QSharedPointer<Tag>,QString)));
    QObject::connect(m_pWorker, SIGNAL(linkTagWithNoteComplete(QSharedPointer<Tag>,QSharedPointer<Note>)),
                     this, SIGNAL(linkTagWithNoteComplete(QSharedPointer<Tag>,QSharedPointer<Note>)));
    QObject::connect(m_pWorker, SIGNAL(linkTagWithNoteFailed(QSharedPointer<Tag>,QSharedPointer<Note>,QString)),
                     this, SIGNAL(linkTagWithNoteFailed(QSharedPointer<Tag>,QSharedPointer<Note>,QString)));
    QObject::connect(m_pWorker, SIGNAL(findTagComplete(QSharedPointer<Tag>)),
                     this, SIGNAL(findTagComplete(QSharedPointer<Tag>)));
    QObject::connect(m_pWorker, SIGNAL(findTagFailed(QSharedPointer<Tag>,QString)),
                     this, SIGNAL(findTagFailed(QSharedPointer<Tag>,QString)));
    QObject::connect(m_pWorker, SIGNAL(listAllTagsPerNoteComplete(QList<Tag>,QSharedPointer<Note>)),
                     this, SIGNAL(listAllTagsPerNoteComplete(QList<Tag>,QSharedPointer<Note>)));
    QObject::connect(m_pWorker, SIGNAL(listAllTagsPerNoteFailed(QSharedPointer<Note>,QString)),
                     this, SIGNAL(listAllTagsPerNoteFailed(QSharedPointer<Note>,QString)));
    QObject::connect(m_pWorker, SIGNAL(listAllTagsComplete(QList<Tag>)),
                     this, SIGNAL(listAllTagsComplete(QList<Tag>)));
    QObject::connect(m_pWorker, SIGNAL(listAllTagsFailed(QString)),
                     this, SIGNAL(listAllTagsFailed(QString)));
    QObject::connect(m_pWorker, SIGNAL(deleteTagComplete(QSharedPointer<Tag>)),
                     this, SIGNAL(deleteTagComplete(QSharedPointer<Tag>)));
    QObject::connect(m_pWorker, SIGNAL(deleteTagFailed(QSharedPointer<Tag>,QString)),
                     this, SIGNAL(deleteTagFailed(QSharedPointer<Tag>,QString)));
    QObject::connect(m_pWorker, SIGNAL(expungeTagComplete(QSharedPointer<Tag>)),
                     this, SIGNAL(expungeTagComplete(QSharedPointer<Tag>)));
    QObject::connect(m_pWorker, SIGNAL(expungeTagFailed(QSharedPointer<Tag>,QString)),
                     this, SIGNAL(expungeTagFailed(QSharedPointer<Tag>,QString)));

    // Resource-related signal-slot connections:
    QObject::connect(this, SIGNAL(addResourceRequest(QSharedPointer<IResource>,QSharedPointer<Note>)),
                     m_pWorker, SLOT(onAddResourceRequest(QSharedPointer<IResource>,QSharedPointer<Note>)));
    QObject::connect(this, SIGNAL(updateResourceRequest(QSharedPointer<IResource>,QSharedPointer<Note>)),
                     m_pWorker, SLOT(onUpdateResourceRequest(QSharedPointer<IResource>,QSharedPointer<Note>)));
    QObject::connect(this, SIGNAL(findResourceRequest(QSharedPointer<IResource>,bool)),
                     m_pWorker, SLOT(onFindResourceRequest(QSharedPointer<IResource>,bool)));
    QObject::connect(this, SIGNAL(expungeResourceRequest(QSharedPointer<IResource>)),
                     m_pWorker, SLOT(onExpungeResourceRequest(QSharedPointer<IResource>)));

    // Resource-related signal-signal connections:
    QObject::connect(m_pWorker, SIGNAL(addResourceComplete(QSharedPointer<IResource>,QSharedPointer<Note>)),
                     this, SIGNAL(addResourceComplete(QSharedPointer<IResource>,QSharedPointer<Note>)));
    QObject::connect(m_pWorker, SIGNAL(addResourceFailed(QSharedPointer<IResource>,QSharedPointer<Note>,QString)),
                     this, SIGNAL(addResourceFailed(QSharedPointer<IResource>,QSharedPointer<Note>,QString)));
    QObject::connect(m_pWorker, SIGNAL(updateResourceComplete(QSharedPointer<IResource>,QSharedPointer<Note>)),
                     this, SIGNAL(updateResourceComplete(QSharedPointer<IResource>,QSharedPointer<Note>)));
    QObject::connect(m_pWorker, SIGNAL(updateResourceFailed(QSharedPointer<IResource>,QSharedPointer<Note>,QString)),
                     this, SIGNAL(updateResourceFailed(QSharedPointer<IResource>,QSharedPointer<Note>,QString)));
    QObject::connect(m_pWorker, SIGNAL(findResourceComplete(QSharedPointer<IResource>,bool)),
                     this, SIGNAL(findResourceComplete(QSharedPointer<IResource>,bool)));
    QObject::connect(m_pWorker, SIGNAL(findResourceFailed(QSharedPointer<IResource>,bool,QString)),
                     this, SIGNAL(findResourceFailed(QSharedPointer<IResource>,bool,QString)));
    QObject::connect(m_pWorker, SIGNAL(expungeResourceComplete(QSharedPointer<IResource>)),
                     this, SIGNAL(expungeResourceComplete(QSharedPointer<IResource>)));
    QObject::connect(m_pWorker, SIGNAL(expungeResourceFailed(QSharedPointer<IResource>,QString)),
                     this, SIGNAL(expungeResourceFailed(QSharedPointer<IResource>,QString)));
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

void LocalStorageManagerThread::onAddNoteRequest(QSharedPointer<Note> note, QSharedPointer<Notebook> notebook)
{
    emit addNoteRequest(note, notebook);
}

void LocalStorageManagerThread::onUpdateNoteRequest(QSharedPointer<Note> note, QSharedPointer<Notebook> notebook)
{
    emit updateNoteRequest(note, notebook);
}

void LocalStorageManagerThread::onFindNoteRequest(QSharedPointer<Note> note, bool withResourceBinaryData)
{
    emit findNoteRequest(note, withResourceBinaryData);
}

void LocalStorageManagerThread::onListAllNotesPerNotebookRequest(QSharedPointer<Notebook> notebook, bool withResourceBinaryData)
{
    emit listAllNotesPerNotebookRequest(notebook, withResourceBinaryData);
}

void LocalStorageManagerThread::onDeleteNoteRequest(QSharedPointer<Note> note)
{
    emit deleteNoteRequest(note);
}

void LocalStorageManagerThread::onExpungeNoteRequest(QSharedPointer<Note> note)
{
    emit expungeNoteRequest(note);
}

void LocalStorageManagerThread::onAddTagRequest(QSharedPointer<Tag> tag)
{
    emit addTagRequest(tag);
}

void LocalStorageManagerThread::onUpdateTagRequest(QSharedPointer<Tag> tag)
{
    emit updateTagRequest(tag);
}

void LocalStorageManagerThread::onLinkTagWithNoteRequest(QSharedPointer<Tag> tag, QSharedPointer<Note> note)
{
    emit linkTagWithNoteRequest(tag, note);
}

void LocalStorageManagerThread::onFindTagRequest(QSharedPointer<Tag> tag)
{
    emit findTagRequest(tag);
}

void LocalStorageManagerThread::onListAllTagsPerNoteRequest(QSharedPointer<Note> note)
{
    emit listAllTagsPerNoteRequest(note);
}

void LocalStorageManagerThread::onListAllTagsRequest()
{
    emit listAllTagsRequest();
}

void LocalStorageManagerThread::onDeleteTagRequest(QSharedPointer<Tag> tag)
{
    emit deleteTagRequest(tag);
}

void LocalStorageManagerThread::onExpungeTagRequest(QSharedPointer<Tag> tag)
{
    emit expungeTagRequest(tag);
}

void LocalStorageManagerThread::onAddResourceRequest(QSharedPointer<IResource> resource, QSharedPointer<Note> note)
{
    emit addResourceRequest(resource, note);
}

void LocalStorageManagerThread::onUpdateResourceRequest(QSharedPointer<IResource> resource, QSharedPointer<Note> note)
{
    emit updateResourceRequest(resource, note);
}

void LocalStorageManagerThread::onFindResourceRequest(QSharedPointer<IResource> resource, bool withBinaryData)
{
    emit findResourceRequest(resource, withBinaryData);
}

void LocalStorageManagerThread::onExpungeResourceRequest(QSharedPointer<IResource> resource)
{
    emit expungeResourceRequest(resource);
}

}
