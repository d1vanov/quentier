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
    QObject::connect(this, SIGNAL(getUserCountRequest()), m_pWorker, SLOT(onGetUserCountRequest()));
    QObject::connect(this, SIGNAL(switchUserRequest(QString,qint32,bool)),
                     m_pWorker, SLOT(onSwitchUserRequest(QString,qint32,bool)));
    QObject::connect(this, SIGNAL(addUserRequest(QSharedPointer<UserWrapper>)),
                     m_pWorker, SLOT(onAddUserRequest(QSharedPointer<UserWrapper>)));
    QObject::connect(this, SIGNAL(updateUserRequest(QSharedPointer<UserWrapper>)),
                     m_pWorker, SLOT(onUpdateUserRequest(QSharedPointer<UserWrapper>)));
    QObject::connect(this, SIGNAL(findUserRequest(QSharedPointer<UserWrapper>)),
                     m_pWorker, SLOT(onFindUserRequest(QSharedPointer<UserWrapper>)));
    QObject::connect(this, SIGNAL(deleteUserRequest(QSharedPointer<UserWrapper>)),
                     m_pWorker, SLOT(onDeleteUserRequest(QSharedPointer<UserWrapper>)));
    QObject::connect(this, SIGNAL(expungeUserRequest(QSharedPointer<UserWrapper>)),
                     m_pWorker, SLOT(onExpungeUserRequest(QSharedPointer<UserWrapper>)));

    // User-related signal-signal connections:
    QObject::connect(m_pWorker, SIGNAL(getUserCountComplete(int)), this, SIGNAL(getUserCountComplete(int)));
    QObject::connect(m_pWorker, SIGNAL(getUserCountFailed(QString)), this, SIGNAL(getUserCountFailed(QString)));
    QObject::connect(m_pWorker, SIGNAL(switchUserComplete(qint32)),
                     this, SIGNAL(switchUserComplete(qint32)));
    QObject::connect(m_pWorker, SIGNAL(switchUserFailed(qint32,QString)),
                     this, SIGNAL(switchUserFailed(qint32,QString)));
    QObject::connect(m_pWorker, SIGNAL(addUserComplete(QSharedPointer<UserWrapper>)),
                     this, SIGNAL(addUserComplete(QSharedPointer<UserWrapper>)));
    QObject::connect(m_pWorker, SIGNAL(addUserFailed(QSharedPointer<UserWrapper>,QString)),
                     this, SIGNAL(addUserFailed(QSharedPointer<UserWrapper>,QString)));
    QObject::connect(m_pWorker, SIGNAL(updateUserComplete(QSharedPointer<UserWrapper>)),
                     this, SIGNAL(updateUserComplete(QSharedPointer<UserWrapper>)));
    QObject::connect(m_pWorker, SIGNAL(updateUserFailed(QSharedPointer<UserWrapper>,QString)),
                     this, SIGNAL(updateUserFailed(QSharedPointer<UserWrapper>,QString)));
    QObject::connect(m_pWorker, SIGNAL(findUserComplete(QSharedPointer<UserWrapper>)),
                     this, SIGNAL(findUserComplete(QSharedPointer<UserWrapper>)));
    QObject::connect(m_pWorker, SIGNAL(findUserFailed(QSharedPointer<UserWrapper>,QString)),
                     this, SIGNAL(findUserFailed(QSharedPointer<UserWrapper>,QString)));
    QObject::connect(m_pWorker, SIGNAL(deleteUserComplete(QSharedPointer<UserWrapper>)),
                     this, SIGNAL(deleteUserComplete(QSharedPointer<UserWrapper>)));
    QObject::connect(m_pWorker, SIGNAL(deleteUserFailed(QSharedPointer<UserWrapper>,QString)),
                     this, SIGNAL(deleteUserFailed(QSharedPointer<UserWrapper>,QString)));
    QObject::connect(m_pWorker, SIGNAL(expungeUserComplete(QSharedPointer<UserWrapper>)),
                     this, SIGNAL(expungeUserComplete(QSharedPointer<UserWrapper>)));
    QObject::connect(m_pWorker, SIGNAL(expungeUserFailed(QSharedPointer<UserWrapper>,QString)),
                     this, SIGNAL(expungeUserFailed(QSharedPointer<UserWrapper>,QString)));

    // Notebook-related signal-slot connections:
    QObject::connect(this, SIGNAL(getNotebookCountRequest()), m_pWorker, SLOT(onGetNotebookCountRequest()));
    QObject::connect(this, SIGNAL(addNotebookRequest(Notebook)),
                     m_pWorker, SLOT(onAddNotebookRequest(Notebook)));
    QObject::connect(this, SIGNAL(updateNotebookRequest(Notebook)),
                     m_pWorker, SLOT(onUpdateNotebookRequest(Notebook)));
    QObject::connect(this, SIGNAL(findNotebookRequest(Notebook)),
                     m_pWorker, SLOT(onFindNotebookRequest(Notebook)));
    QObject::connect(this, SIGNAL(findDefaultNotebookRequest(Notebook)),
                     m_pWorker, SLOT(onFindDefaultNotebookRequest(Notebook)));
    QObject::connect(this, SIGNAL(findLastUsedNotebookRequest(Notebook)),
                     m_pWorker, SLOT(onFindLastUsedNotebookRequest(Notebook)));
    QObject::connect(this, SIGNAL(findDefaultOrLastUsedNotebookRequest(Notebook)),
                     m_pWorker, SLOT(onFindDefaultOrLastUsedNotebookRequest(Notebook)));
    QObject::connect(this, SIGNAL(listAllNotebooksRequest()), m_pWorker, SLOT(onListAllNotebooksRequest()));
    QObject::connect(this, SIGNAL(listAllSharedNotebooksRequest()), m_pWorker, SLOT(onListAllSharedNotebooksRequest()));
    QObject::connect(this, SIGNAL(listSharedNotebooksPerNotebookGuidRequest(QString)),
                     m_pWorker, SLOT(onListSharedNotebooksPerNotebookGuidRequest(QString)));
    QObject::connect(this, SIGNAL(expungeNotebookRequest(Notebook)),
                     m_pWorker, SLOT(onExpungeNotebookRequest(Notebook)));

    // Notebook-related signal-signal connections:
    QObject::connect(m_pWorker, SIGNAL(getNotebookCountComplete(int)), this, SIGNAL(getNotebookCountComplete(int)));
    QObject::connect(m_pWorker, SIGNAL(getNotebookCountFailed(QString)), this, SIGNAL(getNotebookCountFailed(QString)));
    QObject::connect(m_pWorker, SIGNAL(addNotebookComplete(Notebook)),
                     this, SIGNAL(addNotebookComplete(Notebook)));
    QObject::connect(m_pWorker, SIGNAL(addNotebookFailed(Notebook,QString)),
                     this, SIGNAL(addNotebookFailed(Notebook,QString)));
    QObject::connect(m_pWorker, SIGNAL(updateNotebookComplete(Notebook)),
                     this, SIGNAL(updateNotebookComplete(Notebook)));
    QObject::connect(m_pWorker, SIGNAL(updateNotebookFailed(Notebook,QString)),
                     this, SIGNAL(updateNotebookFailed(Notebook,QString)));
    QObject::connect(m_pWorker, SIGNAL(findNotebookComplete(Notebook)),
                     this, SIGNAL(findNotebookComplete(Notebook)));
    QObject::connect(m_pWorker, SIGNAL(findNotebookFailed(Notebook,QString)),
                     this, SIGNAL(findNotebookFailed(Notebook,QString)));
    QObject::connect(m_pWorker, SIGNAL(findDefaultNotebookComplete(Notebook)),
                     this, SIGNAL(findDefaultNotebookComplete(Notebook)));
    QObject::connect(m_pWorker, SIGNAL(findDefaultNotebookFailed(Notebook,QString)),
                     this, SIGNAL(findDefaultNotebookFailed(Notebook,QString)));
    QObject::connect(m_pWorker, SIGNAL(findLastUsedNotebookComplete(Notebook)),
                     this, SIGNAL(findLastUsedNotebookComplete(Notebook)));
    QObject::connect(m_pWorker, SIGNAL(findLastUsedNotebookFailed(Notebook,QString)),
                     this, SIGNAL(findLastUsedNotebookFailed(Notebook,QString)));
    QObject::connect(m_pWorker, SIGNAL(findDefaultOrLastUsedNotebookComplete(Notebook)),
                     this, SIGNAL(findDefaultOrLastUsedNotebookComplete(Notebook)));
    QObject::connect(m_pWorker, SIGNAL(findDefaultOrLastUsedNotebookFailed(Notebook,QString)),
                     this, SIGNAL(findDefaultOrLastUsedNotebookFailed(Notebook,QString)));
    QObject::connect(m_pWorker, SIGNAL(listAllNotebooksComplete(QList<Notebook>)),
                     this, SIGNAL(listAllNotebooksComplete(QList<Notebook>)));
    QObject::connect(m_pWorker, SIGNAL(listAllNotebooksFailed(QString)),
                     this, SIGNAL(listAllNotebooksFailed(QString)));
    QObject::connect(m_pWorker, SIGNAL(listAllSharedNotebooksComplete(QList<SharedNotebookWrapper>)),
                     this, SIGNAL(listAllSharedNotebooksComplete(QList<SharedNotebookWrapper>)));
    QObject::connect(m_pWorker, SIGNAL(listAllSharedNotebooksFailed(QString)),
                     this, SIGNAL(listAllSharedNotebooksFailed(QString)));
    QObject::connect(m_pWorker, SIGNAL(listSharedNotebooksPerNotebookGuidComplete(QString,QList<SharedNotebookWrapper>)),
                     this, SIGNAL(listSharedNotebooksPerNotebookGuidComplete(QString,QList<SharedNotebookWrapper>)));
    QObject::connect(m_pWorker, SIGNAL(listSharedNotebooksPerNotebookGuidFailed(QString,QString)),
                     this, SIGNAL(listSharedNotebooksPerNotebookGuidFailed(QString,QString)));
    QObject::connect(m_pWorker, SIGNAL(expungeNotebookComplete(Notebook)),
                     this, SIGNAL(expungeNotebookComplete(Notebook)));
    QObject::connect(m_pWorker, SIGNAL(expungeNotebookFailed(Notebook,QString)),
                     this, SIGNAL(expungeNotebookFailed(Notebook,QString)));

    // Linked notebook-related signal-slot connections:
    QObject::connect(this, SIGNAL(getLinkedNotebookCountRequest()), m_pWorker, SLOT(onGetLinkedNotebookCountRequest()));
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
    QObject::connect(m_pWorker, SIGNAL(getLinkedNotebookCountComplete(int)), this, SIGNAL(getLinkedNotebookCountComplete(int)));
    QObject::connect(m_pWorker, SIGNAL(getLinkedNotebookCountFailed(QString)), this, SIGNAL(getLinkedNotebookCountFailed(QString)));
    QObject::connect(m_pWorker, SIGNAL(addLinkedNotebookComplete(QSharedPointer<LinkedNotebook>)),
                     this, SIGNAL(addLinkedNotebookComplete(QSharedPointer<LinkedNotebook>)));
    QObject::connect(m_pWorker, SIGNAL(addLinkedNotebookFailed(QSharedPointer<LinkedNotebook>,QString)),
                     this, SIGNAL(addLinkedNotebookFailed(QSharedPointer<LinkedNotebook>,QString)));
    QObject::connect(m_pWorker, SIGNAL(updateLinkedNotebookComplete(QSharedPointer<LinkedNotebook>)),
                     this, SIGNAL(updateLinkedNotebookComplete(QSharedPointer<LinkedNotebook>)));
    QObject::connect(m_pWorker, SIGNAL(updateLinkedNotebookFailed(QSharedPointer<LinkedNotebook>,QString)),
                     this, SIGNAL(updateLinkedNotebookFailed(QSharedPointer<LinkedNotebook>,QString)));
    QObject::connect(m_pWorker, SIGNAL(findLinkedNotebookComplete(QSharedPointer<LinkedNotebook>)),
                     this, SIGNAL(findLinkedNotebookComplete(QSharedPointer<LinkedNotebook>)));
    QObject::connect(m_pWorker, SIGNAL(findLinkedNotebookFailed(QSharedPointer<LinkedNotebook>,QString)),
                     this, SIGNAL(findLinkedNotebookFailed(QSharedPointer<LinkedNotebook>,QString)));
    QObject::connect(m_pWorker, SIGNAL(listAllLinkedNotebooksComplete(QList<LinkedNotebook>)),
                     this, SIGNAL(listAllLinkedNotebooksComplete(QList<LinkedNotebook>)));
    QObject::connect(m_pWorker, SIGNAL(listAllLinkedNotebooksFailed(QString)),
                     this, SIGNAL(listAllLinkedNotebooksFailed(QString)));
    QObject::connect(m_pWorker, SIGNAL(expungeLinkedNotebookComplete(QSharedPointer<LinkedNotebook>)),
                     this, SIGNAL(expungeLinkedNotebookComplete(QSharedPointer<LinkedNotebook>)));
    QObject::connect(m_pWorker, SIGNAL(expungeLinkedNotebookFailed(QSharedPointer<LinkedNotebook>,QString)),
                     this, SIGNAL(expungeLinkedNotebookFailed(QSharedPointer<LinkedNotebook>,QString)));

    // Note-related signal-slot connections:
    QObject::connect(this, SIGNAL(getNoteCountRequest()), m_pWorker, SLOT(onGetNoteCountRequest()));
    QObject::connect(this, SIGNAL(addNoteRequest(Note,Notebook)),
                     m_pWorker, SLOT(onAddNoteRequest(Note,Notebook)));
    QObject::connect(this, SIGNAL(updateNoteRequest(Note,Notebook)),
                     m_pWorker, SLOT(onUpdateNoteRequest(Note,Notebook)));
    QObject::connect(this, SIGNAL(findNoteRequest(Note,bool)),
                     m_pWorker, SLOT(onFindNoteRequest(Note,bool)));
    QObject::connect(this, SIGNAL(listAllNotesPerNotebookRequest(Notebook,bool)),
                     m_pWorker, SLOT(onListAllNotesPerNotebookRequest(Notebook,bool)));
    QObject::connect(this, SIGNAL(deleteNoteRequest(Note)),
                     m_pWorker, SLOT(onDeleteNoteRequest(Note)));
    QObject::connect(this, SIGNAL(expungeNoteRequest(Note)),
                     m_pWorker, SLOT(onExpungeNoteRequest(Note)));

    // Note-related signal-signal connections:
    QObject::connect(m_pWorker, SIGNAL(getNoteCountComplete(int)), this, SIGNAL(getNoteCountComplete(int)));
    QObject::connect(m_pWorker, SIGNAL(getNoteCountFailed(QString)), this, SIGNAL(getNoteCountFailed(QString)));
    QObject::connect(m_pWorker, SIGNAL(addNoteComplete(Note,Notebook)),
                     this, SIGNAL(addNoteComplete(Note,Notebook)));
    QObject::connect(m_pWorker, SIGNAL(addNoteFailed(Note,Notebook,QString)),
                     this, SIGNAL(addNoteFailed(Note,Notebook,QString)));
    QObject::connect(m_pWorker, SIGNAL(updateNoteComplete(Note,Notebook)),
                     this, SIGNAL(updateNoteComplete(Note,Notebook)));
    QObject::connect(m_pWorker, SIGNAL(updateNoteFailed(Note,Notebook,QString)),
                     this, SIGNAL(updateNoteFailed(Note,Notebook,QString)));
    QObject::connect(m_pWorker, SIGNAL(findNoteComplete(Note,bool)),
                     this, SIGNAL(findNoteComplete(Note,bool)));
    QObject::connect(m_pWorker, SIGNAL(findNoteFailed(Note,bool,QString)),
                     this, SIGNAL(findNoteFailed(Note,bool,QString)));
    QObject::connect(m_pWorker, SIGNAL(listAllNotesPerNotebookComplete(Notebook,bool,QList<Note>)),
                     this, SIGNAL(listAllNotesPerNotebookComplete(Notebook,bool,QList<Note>)));
    QObject::connect(m_pWorker, SIGNAL(listAllNotesPerNotebookFailed(Notebook,bool,QString)),
                     this, SIGNAL(listAllNotesPerNotebookFailed(Notebook,bool,QString)));
    QObject::connect(m_pWorker, SIGNAL(deleteNoteComplete(Note)),
                     this, SIGNAL(deleteNoteComplete(Note)));
    QObject::connect(m_pWorker, SIGNAL(deleteNoteFailed(Note,QString)),
                     this, SIGNAL(deleteNoteFailed(Note,QString)));
    QObject::connect(m_pWorker, SIGNAL(expungeNoteComplete(Note)),
                     this, SIGNAL(expungeNoteComplete(Note)));
    QObject::connect(m_pWorker, SIGNAL(expungeNoteFailed(Note,QString)),
                     this, SIGNAL(expungeNoteFailed(Note,QString)));

    // Tag-related signal-slot connections:
    QObject::connect(this, SIGNAL(getTagCountRequest()), m_pWorker, SLOT(onGetTagCountRequest()));
    QObject::connect(this, SIGNAL(addTagRequest(QSharedPointer<Tag>)),
                     m_pWorker, SLOT(onAddTagRequest(QSharedPointer<Tag>)));
    QObject::connect(this, SIGNAL(updateTagRequest(QSharedPointer<Tag>)),
                     m_pWorker, SLOT(onUpdateTagRequest(QSharedPointer<Tag>)));
    QObject::connect(this, SIGNAL(linkTagWithNoteRequest(QSharedPointer<Tag>,Note)),
                     m_pWorker, SLOT(onLinkTagWithNoteRequest(QSharedPointer<Tag>,Note)));
    QObject::connect(this, SIGNAL(findTagRequest(QSharedPointer<Tag>)),
                     m_pWorker, SLOT(onFindTagRequest(QSharedPointer<Tag>)));
    QObject::connect(this, SIGNAL(listAllTagsPerNoteRequest(Note)),
                     m_pWorker, SLOT(onListAllTagsPerNoteRequest(Note)));
    QObject::connect(this, SIGNAL(listAllTagsRequest()), m_pWorker, SLOT(onListAllTagsRequest()));
    QObject::connect(this, SIGNAL(deleteTagRequest(QSharedPointer<Tag>)),
                     m_pWorker, SLOT(onDeleteTagRequest(QSharedPointer<Tag>)));
    QObject::connect(this, SIGNAL(expungeTagRequest(QSharedPointer<Tag>)),
                     m_pWorker, SLOT(onExpungeTagRequest(QSharedPointer<Tag>)));

    // Tag-related signal-signal connections:
    QObject::connect(m_pWorker, SIGNAL(getTagCountComplete(int)), this, SIGNAL(getTagCountComplete(int)));
    QObject::connect(m_pWorker, SIGNAL(getTagCountFailed(QString)), this, SIGNAL(getTagCountFailed(QString)));
    QObject::connect(m_pWorker, SIGNAL(addTagComplete(QSharedPointer<Tag>)),
                     this, SIGNAL(addTagComplete(QSharedPointer<Tag>)));
    QObject::connect(m_pWorker, SIGNAL(addTagFailed(QSharedPointer<Tag>,QString)),
                     this, SIGNAL(addTagFailed(QSharedPointer<Tag>,QString)));
    QObject::connect(m_pWorker, SIGNAL(updateTagComplete(QSharedPointer<Tag>)),
                     this, SIGNAL(updateTagComplete(QSharedPointer<Tag>)));
    QObject::connect(m_pWorker, SIGNAL(updateTagFailed(QSharedPointer<Tag>,QString)),
                     this, SIGNAL(updateTagFailed(QSharedPointer<Tag>,QString)));
    QObject::connect(m_pWorker, SIGNAL(linkTagWithNoteComplete(QSharedPointer<Tag>,Note)),
                     this, SIGNAL(linkTagWithNoteComplete(QSharedPointer<Tag>,Note)));
    QObject::connect(m_pWorker, SIGNAL(linkTagWithNoteFailed(QSharedPointer<Tag>,Note,QString)),
                     this, SIGNAL(linkTagWithNoteFailed(QSharedPointer<Tag>,Note,QString)));
    QObject::connect(m_pWorker, SIGNAL(findTagComplete(QSharedPointer<Tag>)),
                     this, SIGNAL(findTagComplete(QSharedPointer<Tag>)));
    QObject::connect(m_pWorker, SIGNAL(findTagFailed(QSharedPointer<Tag>,QString)),
                     this, SIGNAL(findTagFailed(QSharedPointer<Tag>,QString)));
    QObject::connect(m_pWorker, SIGNAL(listAllTagsPerNoteComplete(QList<Tag>,Note)),
                     this, SIGNAL(listAllTagsPerNoteComplete(QList<Tag>,Note)));
    QObject::connect(m_pWorker, SIGNAL(listAllTagsPerNoteFailed(Note,QString)),
                     this, SIGNAL(listAllTagsPerNoteFailed(Note,QString)));
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
    QObject::connect(this, SIGNAL(getResourceCountRequest()), m_pWorker, SLOT(onGetResourceCountRequest()));
    QObject::connect(this, SIGNAL(addResourceRequest(QSharedPointer<ResourceWrapper>,Note)),
                     m_pWorker, SLOT(onAddResourceRequest(QSharedPointer<ResourceWrapper>,Note)));
    QObject::connect(this, SIGNAL(updateResourceRequest(QSharedPointer<ResourceWrapper>,Note)),
                     m_pWorker, SLOT(onUpdateResourceRequest(QSharedPointer<ResourceWrapper>,Note)));
    QObject::connect(this, SIGNAL(findResourceRequest(QSharedPointer<ResourceWrapper>,bool)),
                     m_pWorker, SLOT(onFindResourceRequest(QSharedPointer<ResourceWrapper>,bool)));
    QObject::connect(this, SIGNAL(expungeResourceRequest(QSharedPointer<ResourceWrapper>)),
                     m_pWorker, SLOT(onExpungeResourceRequest(QSharedPointer<ResourceWrapper>)));

    // Resource-related signal-signal connections:
    QObject::connect(m_pWorker, SIGNAL(getResourceCountComplete(int)), this, SIGNAL(getResourceCountComplete(int)));
    QObject::connect(m_pWorker, SIGNAL(getResourceCountFailed(QString)), this, SIGNAL(getResourceCountFailed(QString)));
    QObject::connect(m_pWorker, SIGNAL(addResourceComplete(QSharedPointer<ResourceWrapper>,Note)),
                     this, SIGNAL(addResourceComplete(QSharedPointer<ResourceWrapper>,Note)));
    QObject::connect(m_pWorker, SIGNAL(addResourceFailed(QSharedPointer<ResourceWrapper>,Note,QString)),
                     this, SIGNAL(addResourceFailed(QSharedPointer<ResourceWrapper>,Note,QString)));
    QObject::connect(m_pWorker, SIGNAL(updateResourceComplete(QSharedPointer<ResourceWrapper>,Note)),
                     this, SIGNAL(updateResourceComplete(QSharedPointer<ResourceWrapper>,Note)));
    QObject::connect(m_pWorker, SIGNAL(updateResourceFailed(QSharedPointer<ResourceWrapper>,Note,QString)),
                     this, SIGNAL(updateResourceFailed(QSharedPointer<ResourceWrapper>,Note,QString)));
    QObject::connect(m_pWorker, SIGNAL(findResourceComplete(QSharedPointer<ResourceWrapper>,bool)),
                     this, SIGNAL(findResourceComplete(QSharedPointer<ResourceWrapper>,bool)));
    QObject::connect(m_pWorker, SIGNAL(findResourceFailed(QSharedPointer<ResourceWrapper>,bool,QString)),
                     this, SIGNAL(findResourceFailed(QSharedPointer<ResourceWrapper>,bool,QString)));
    QObject::connect(m_pWorker, SIGNAL(expungeResourceComplete(QSharedPointer<ResourceWrapper>)),
                     this, SIGNAL(expungeResourceComplete(QSharedPointer<ResourceWrapper>)));
    QObject::connect(m_pWorker, SIGNAL(expungeResourceFailed(QSharedPointer<ResourceWrapper>,QString)),
                     this, SIGNAL(expungeResourceFailed(QSharedPointer<ResourceWrapper>,QString)));

    // Saved search-related signal-slot connections:
    QObject::connect(this, SIGNAL(getSavedSearchCountRequest()), m_pWorker, SLOT(onGetSavedSearchCountRequest()));
    QObject::connect(this, SIGNAL(addSavedSearchRequest(QSharedPointer<SavedSearch>)),
                     m_pWorker, SLOT(onAddSavedSearchRequest(QSharedPointer<SavedSearch>)));
    QObject::connect(this, SIGNAL(updateSavedSearchRequest(QSharedPointer<SavedSearch>)),
                     m_pWorker, SLOT(onUpdateSavedSearchRequest(QSharedPointer<SavedSearch>)));
    QObject::connect(this, SIGNAL(findSavedSearchRequest(QSharedPointer<SavedSearch>)),
                     m_pWorker, SLOT(onFindSavedSearchRequest(QSharedPointer<SavedSearch>)));
    QObject::connect(this, SIGNAL(listAllSavedSearchesRequest()), m_pWorker, SLOT(onListAllSavedSearchesRequest()));
    QObject::connect(this, SIGNAL(expungeSavedSearchRequest(QSharedPointer<SavedSearch>)),
                     m_pWorker, SLOT(onExpungeSavedSearch(QSharedPointer<SavedSearch>)));

    // Saved search-related signal-signal connections:
    QObject::connect(m_pWorker, SIGNAL(getSavedSearchCountComplete(int)), this, SIGNAL(getSavedSearchCountComplete(int)));
    QObject::connect(m_pWorker, SIGNAL(getSavedSearchCountFailed(QString)), this, SIGNAL(getSavedSearchCountFailed(QString)));
    QObject::connect(m_pWorker, SIGNAL(addSavedSearchComplete(QSharedPointer<SavedSearch>)),
                     this, SIGNAL(addSavedSearchComplete(QSharedPointer<SavedSearch>)));
    QObject::connect(m_pWorker, SIGNAL(addSavedSearchFailed(QSharedPointer<SavedSearch>,QString)),
                     this, SIGNAL(addSavedSearchFailed(QSharedPointer<SavedSearch>,QString)));
    QObject::connect(m_pWorker, SIGNAL(updateSavedSearchComplete(QSharedPointer<SavedSearch>)),
                     this, SIGNAL(updateSavedSearchComplete(QSharedPointer<SavedSearch>)));
    QObject::connect(m_pWorker, SIGNAL(updateSavedSearchFailed(QSharedPointer<SavedSearch>,QString)),
                     this, SIGNAL(updateSavedSearchFailed(QSharedPointer<SavedSearch>,QString)));
    QObject::connect(m_pWorker, SIGNAL(findSavedSearchComplete(QSharedPointer<SavedSearch>)),
                     this, SIGNAL(findSavedSearchComplete(QSharedPointer<SavedSearch>)));
    QObject::connect(m_pWorker, SIGNAL(findSavedSearchFailed(QSharedPointer<SavedSearch>,QString)),
                     this, SIGNAL(findSavedSearchFailed(QSharedPointer<SavedSearch>,QString)));
    QObject::connect(m_pWorker, SIGNAL(listAllSavedSearchesComplete(QList<SavedSearch>)),
                     this, SIGNAL(listAllSavedSearchesComplete(QList<SavedSearch>)));
    QObject::connect(m_pWorker, SIGNAL(listAllSavedSearchesFailed(QString)),
                     this, SIGNAL(listAllSavedSearchesFailed(QString)));
    QObject::connect(m_pWorker, SIGNAL(expungeSavedSearchComplete(QSharedPointer<SavedSearch>)),
                     this, SIGNAL(expungeSavedSearchComplete(QSharedPointer<SavedSearch>)));
    QObject::connect(m_pWorker, SIGNAL(expungeSavedSearchFailed(QSharedPointer<SavedSearch>,QString)),
                     this, SIGNAL(expungeSavedSearchFailed(QSharedPointer<SavedSearch>,QString)));
}

LocalStorageManagerThread::~LocalStorageManagerThread()
{}

void LocalStorageManagerThread::onGetUserCountRequest()
{
    emit getUserCountRequest();
}

void LocalStorageManagerThread::onSwitchUserRequest(QString username, qint32 userId, bool startFromScratch)
{
    emit switchUserRequest(username, userId, startFromScratch);
}

void LocalStorageManagerThread::onAddUserRequest(QSharedPointer<UserWrapper> user)
{
    emit addUserRequest(user);
}

void LocalStorageManagerThread::onUpdateUserRequest(QSharedPointer<UserWrapper> user)
{
    emit updateUserRequest(user);
}

void LocalStorageManagerThread::onFindUserRequest(QSharedPointer<UserWrapper> user)
{
    emit findUserRequest(user);
}

void LocalStorageManagerThread::onDeleteUserRequest(QSharedPointer<UserWrapper> user)
{
    emit deleteUserRequest(user);
}

void LocalStorageManagerThread::onExpungeUserRequest(QSharedPointer<UserWrapper> user)
{
    emit expungeUserRequest(user);
}

void LocalStorageManagerThread::onGetNotebookCountRequest()
{
    emit getNotebookCountRequest();
}

void LocalStorageManagerThread::onAddNotebookRequest(Notebook notebook)
{
    emit addNotebookRequest(notebook);
}

void LocalStorageManagerThread::onUpdateNotebookRequest(Notebook notebook)
{
    emit updateNotebookRequest(notebook);
}

void LocalStorageManagerThread::onFindNotebookRequest(Notebook notebook)
{
    emit findNotebookRequest(notebook);
}

void LocalStorageManagerThread::onFindDefaultNotebookRequest(Notebook notebook)
{
    emit findDefaultNotebookRequest(notebook);
}

void LocalStorageManagerThread::onFindLastUsedNotebookRequest(Notebook notebook)
{
    emit findLastUsedNotebookRequest(notebook);
}

void LocalStorageManagerThread::onFindDefaultOrLastUsedNotebookRequest(Notebook notebook)
{
    emit findDefaultOrLastUsedNotebookRequest(notebook);
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

void LocalStorageManagerThread::onExpungeNotebookRequest(Notebook notebook)
{
    emit expungeNotebookRequest(notebook);
}

void LocalStorageManagerThread::onGetLinkedNotebookCountRequest()
{
    emit getLinkedNotebookCountRequest();
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

void LocalStorageManagerThread::onGetNoteCountRequest()
{
    emit getNoteCountRequest();
}

void LocalStorageManagerThread::onAddNoteRequest(Note note, Notebook notebook)
{
    emit addNoteRequest(note, notebook);
}

void LocalStorageManagerThread::onUpdateNoteRequest(Note note, Notebook notebook)
{
    emit updateNoteRequest(note, notebook);
}

void LocalStorageManagerThread::onFindNoteRequest(Note note, bool withResourceBinaryData)
{
    emit findNoteRequest(note, withResourceBinaryData);
}

void LocalStorageManagerThread::onListAllNotesPerNotebookRequest(Notebook notebook, bool withResourceBinaryData)
{
    emit listAllNotesPerNotebookRequest(notebook, withResourceBinaryData);
}

void LocalStorageManagerThread::onDeleteNoteRequest(Note note)
{
    emit deleteNoteRequest(note);
}

void LocalStorageManagerThread::onExpungeNoteRequest(Note note)
{
    emit expungeNoteRequest(note);
}

void LocalStorageManagerThread::onGetTagCountRequest()
{
    emit getTagCountRequest();
}

void LocalStorageManagerThread::onAddTagRequest(QSharedPointer<Tag> tag)
{
    emit addTagRequest(tag);
}

void LocalStorageManagerThread::onUpdateTagRequest(QSharedPointer<Tag> tag)
{
    emit updateTagRequest(tag);
}

void LocalStorageManagerThread::onLinkTagWithNoteRequest(QSharedPointer<Tag> tag, Note note)
{
    emit linkTagWithNoteRequest(tag, note);
}

void LocalStorageManagerThread::onFindTagRequest(QSharedPointer<Tag> tag)
{
    emit findTagRequest(tag);
}

void LocalStorageManagerThread::onListAllTagsPerNoteRequest(Note note)
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

void LocalStorageManagerThread::onGetResourceCountRequest()
{
    emit getResourceCountRequest();
}

void LocalStorageManagerThread::onAddResourceRequest(QSharedPointer<ResourceWrapper> resource, Note note)
{
    emit addResourceRequest(resource, note);
}

void LocalStorageManagerThread::onUpdateResourceRequest(QSharedPointer<ResourceWrapper> resource, Note note)
{
    emit updateResourceRequest(resource, note);
}

void LocalStorageManagerThread::onFindResourceRequest(QSharedPointer<ResourceWrapper> resource, bool withBinaryData)
{
    emit findResourceRequest(resource, withBinaryData);
}

void LocalStorageManagerThread::onExpungeResourceRequest(QSharedPointer<ResourceWrapper> resource)
{
    emit expungeResourceRequest(resource);
}

void LocalStorageManagerThread::onGetSavedSearchCountRequest()
{
    emit getSavedSearchCountRequest();
}

void LocalStorageManagerThread::onAddSavedSearchRequest(QSharedPointer<SavedSearch> search)
{
    emit addSavedSearchRequest(search);
}

void LocalStorageManagerThread::onUpdateSavedSearchRequest(QSharedPointer<SavedSearch> search)
{
    emit updateSavedSearchRequest(search);
}

void LocalStorageManagerThread::onFindSavedSearchRequest(QSharedPointer<SavedSearch> search)
{
    emit findSavedSearchRequest(search);
}

void LocalStorageManagerThread::onListAllSavedSearchesRequest()
{
    emit listAllSavedSearchesRequest();
}

void LocalStorageManagerThread::onExpungeSavedSearch(QSharedPointer<SavedSearch> search)
{
    emit expungeSavedSearchRequest(search);
}

}
