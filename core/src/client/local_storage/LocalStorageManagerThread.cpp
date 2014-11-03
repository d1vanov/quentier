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
    m_pWorker->init();
}

void LocalStorageManagerThread::createConnections()
{   
    QObject::connect(m_pWorker, SIGNAL(failure(QString)), this, SIGNAL(failure(QString)));

    // User-related signal-slot connections:
    QObject::connect(this, SIGNAL(getUserCountRequest()), m_pWorker, SLOT(onGetUserCountRequest()));
    QObject::connect(this, SIGNAL(switchUserRequest(QString,qint32,bool)),
                     m_pWorker, SLOT(onSwitchUserRequest(QString,qint32,bool)));
    QObject::connect(this, SIGNAL(addUserRequest(UserWrapper)),
                     m_pWorker, SLOT(onAddUserRequest(UserWrapper)));
    QObject::connect(this, SIGNAL(updateUserRequest(UserWrapper)),
                     m_pWorker, SLOT(onUpdateUserRequest(UserWrapper)));
    QObject::connect(this, SIGNAL(findUserRequest(UserWrapper)),
                     m_pWorker, SLOT(onFindUserRequest(UserWrapper)));
    QObject::connect(this, SIGNAL(deleteUserRequest(UserWrapper)),
                     m_pWorker, SLOT(onDeleteUserRequest(UserWrapper)));
    QObject::connect(this, SIGNAL(expungeUserRequest(UserWrapper)),
                     m_pWorker, SLOT(onExpungeUserRequest(UserWrapper)));

    // User-related signal-signal connections:
    QObject::connect(m_pWorker, SIGNAL(getUserCountComplete(int)), this, SIGNAL(getUserCountComplete(int)));
    QObject::connect(m_pWorker, SIGNAL(getUserCountFailed(QString)), this, SIGNAL(getUserCountFailed(QString)));
    QObject::connect(m_pWorker, SIGNAL(switchUserComplete(qint32)),
                     this, SIGNAL(switchUserComplete(qint32)));
    QObject::connect(m_pWorker, SIGNAL(switchUserFailed(qint32,QString)),
                     this, SIGNAL(switchUserFailed(qint32,QString)));
    QObject::connect(m_pWorker, SIGNAL(addUserComplete(UserWrapper)),
                     this, SIGNAL(addUserComplete(UserWrapper)));
    QObject::connect(m_pWorker, SIGNAL(addUserFailed(UserWrapper,QString)),
                     this, SIGNAL(addUserFailed(UserWrapper,QString)));
    QObject::connect(m_pWorker, SIGNAL(updateUserComplete(UserWrapper)),
                     this, SIGNAL(updateUserComplete(UserWrapper)));
    QObject::connect(m_pWorker, SIGNAL(updateUserFailed(UserWrapper,QString)),
                     this, SIGNAL(updateUserFailed(UserWrapper,QString)));
    QObject::connect(m_pWorker, SIGNAL(findUserComplete(UserWrapper)),
                     this, SIGNAL(findUserComplete(UserWrapper)));
    QObject::connect(m_pWorker, SIGNAL(findUserFailed(UserWrapper,QString)),
                     this, SIGNAL(findUserFailed(UserWrapper,QString)));
    QObject::connect(m_pWorker, SIGNAL(deleteUserComplete(UserWrapper)),
                     this, SIGNAL(deleteUserComplete(UserWrapper)));
    QObject::connect(m_pWorker, SIGNAL(deleteUserFailed(UserWrapper,QString)),
                     this, SIGNAL(deleteUserFailed(UserWrapper,QString)));
    QObject::connect(m_pWorker, SIGNAL(expungeUserComplete(UserWrapper)),
                     this, SIGNAL(expungeUserComplete(UserWrapper)));
    QObject::connect(m_pWorker, SIGNAL(expungeUserFailed(UserWrapper,QString)),
                     this, SIGNAL(expungeUserFailed(UserWrapper,QString)));

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
    QObject::connect(this, SIGNAL(addLinkedNotebookRequest(LinkedNotebook)),
                     m_pWorker, SLOT(onAddLinkedNotebookRequest(LinkedNotebook)));
    QObject::connect(this, SIGNAL(updateLinkedNotebookRequest(LinkedNotebook)),
                     m_pWorker, SLOT(onUpdateLinkedNotebookRequest(LinkedNotebook)));
    QObject::connect(this, SIGNAL(findLinkedNotebookRequest(LinkedNotebook)),
                     m_pWorker, SLOT(onFindLinkedNotebookRequest(LinkedNotebook)));
    QObject::connect(this, SIGNAL(listAllLinkedNotebooksRequest()), m_pWorker, SLOT(onListAllLinkedNotebooksRequest()));
    QObject::connect(this, SIGNAL(expungeLinkedNotebookRequest(LinkedNotebook)),
                     m_pWorker, SLOT(onExpungeLinkedNotebookRequest(LinkedNotebook)));

    // Linked notebook-related signal-signal connections:
    QObject::connect(m_pWorker, SIGNAL(getLinkedNotebookCountComplete(int)), this, SIGNAL(getLinkedNotebookCountComplete(int)));
    QObject::connect(m_pWorker, SIGNAL(getLinkedNotebookCountFailed(QString)), this, SIGNAL(getLinkedNotebookCountFailed(QString)));
    QObject::connect(m_pWorker, SIGNAL(addLinkedNotebookComplete(LinkedNotebook)),
                     this, SIGNAL(addLinkedNotebookComplete(LinkedNotebook)));
    QObject::connect(m_pWorker, SIGNAL(addLinkedNotebookFailed(LinkedNotebook,QString)),
                     this, SIGNAL(addLinkedNotebookFailed(LinkedNotebook,QString)));
    QObject::connect(m_pWorker, SIGNAL(updateLinkedNotebookComplete(LinkedNotebook)),
                     this, SIGNAL(updateLinkedNotebookComplete(LinkedNotebook)));
    QObject::connect(m_pWorker, SIGNAL(updateLinkedNotebookFailed(LinkedNotebook,QString)),
                     this, SIGNAL(updateLinkedNotebookFailed(LinkedNotebook,QString)));
    QObject::connect(m_pWorker, SIGNAL(findLinkedNotebookComplete(LinkedNotebook)),
                     this, SIGNAL(findLinkedNotebookComplete(LinkedNotebook)));
    QObject::connect(m_pWorker, SIGNAL(findLinkedNotebookFailed(LinkedNotebook,QString)),
                     this, SIGNAL(findLinkedNotebookFailed(LinkedNotebook,QString)));
    QObject::connect(m_pWorker, SIGNAL(listAllLinkedNotebooksComplete(QList<LinkedNotebook>)),
                     this, SIGNAL(listAllLinkedNotebooksComplete(QList<LinkedNotebook>)));
    QObject::connect(m_pWorker, SIGNAL(listAllLinkedNotebooksFailed(QString)),
                     this, SIGNAL(listAllLinkedNotebooksFailed(QString)));
    QObject::connect(m_pWorker, SIGNAL(expungeLinkedNotebookComplete(LinkedNotebook)),
                     this, SIGNAL(expungeLinkedNotebookComplete(LinkedNotebook)));
    QObject::connect(m_pWorker, SIGNAL(expungeLinkedNotebookFailed(LinkedNotebook,QString)),
                     this, SIGNAL(expungeLinkedNotebookFailed(LinkedNotebook,QString)));

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
    QObject::connect(this, SIGNAL(addTagRequest(Tag)),
                     m_pWorker, SLOT(onAddTagRequest(Tag)));
    QObject::connect(this, SIGNAL(updateTagRequest(Tag)),
                     m_pWorker, SLOT(onUpdateTagRequest(Tag)));
    QObject::connect(this, SIGNAL(linkTagWithNoteRequest(Tag,Note)),
                     m_pWorker, SLOT(onLinkTagWithNoteRequest(Tag,Note)));
    QObject::connect(this, SIGNAL(findTagRequest(Tag)),
                     m_pWorker, SLOT(onFindTagRequest(Tag)));
    QObject::connect(this, SIGNAL(listAllTagsPerNoteRequest(Note)),
                     m_pWorker, SLOT(onListAllTagsPerNoteRequest(Note)));
    QObject::connect(this, SIGNAL(listAllTagsRequest()), m_pWorker, SLOT(onListAllTagsRequest()));
    QObject::connect(this, SIGNAL(deleteTagRequest(Tag)),
                     m_pWorker, SLOT(onDeleteTagRequest(Tag)));
    QObject::connect(this, SIGNAL(expungeTagRequest(Tag)),
                     m_pWorker, SLOT(onExpungeTagRequest(Tag)));

    // Tag-related signal-signal connections:
    QObject::connect(m_pWorker, SIGNAL(getTagCountComplete(int)), this, SIGNAL(getTagCountComplete(int)));
    QObject::connect(m_pWorker, SIGNAL(getTagCountFailed(QString)), this, SIGNAL(getTagCountFailed(QString)));
    QObject::connect(m_pWorker, SIGNAL(addTagComplete(Tag)),
                     this, SIGNAL(addTagComplete(Tag)));
    QObject::connect(m_pWorker, SIGNAL(addTagFailed(Tag,QString)),
                     this, SIGNAL(addTagFailed(Tag,QString)));
    QObject::connect(m_pWorker, SIGNAL(updateTagComplete(Tag)),
                     this, SIGNAL(updateTagComplete(Tag)));
    QObject::connect(m_pWorker, SIGNAL(updateTagFailed(Tag,QString)),
                     this, SIGNAL(updateTagFailed(Tag,QString)));
    QObject::connect(m_pWorker, SIGNAL(linkTagWithNoteComplete(Tag,Note)),
                     this, SIGNAL(linkTagWithNoteComplete(Tag,Note)));
    QObject::connect(m_pWorker, SIGNAL(linkTagWithNoteFailed(Tag,Note,QString)),
                     this, SIGNAL(linkTagWithNoteFailed(Tag,Note,QString)));
    QObject::connect(m_pWorker, SIGNAL(findTagComplete(Tag)),
                     this, SIGNAL(findTagComplete(Tag)));
    QObject::connect(m_pWorker, SIGNAL(findTagFailed(Tag,QString)),
                     this, SIGNAL(findTagFailed(Tag,QString)));
    QObject::connect(m_pWorker, SIGNAL(listAllTagsPerNoteComplete(QList<Tag>,Note)),
                     this, SIGNAL(listAllTagsPerNoteComplete(QList<Tag>,Note)));
    QObject::connect(m_pWorker, SIGNAL(listAllTagsPerNoteFailed(Note,QString)),
                     this, SIGNAL(listAllTagsPerNoteFailed(Note,QString)));
    QObject::connect(m_pWorker, SIGNAL(listAllTagsComplete(QList<Tag>)),
                     this, SIGNAL(listAllTagsComplete(QList<Tag>)));
    QObject::connect(m_pWorker, SIGNAL(listAllTagsFailed(QString)),
                     this, SIGNAL(listAllTagsFailed(QString)));
    QObject::connect(m_pWorker, SIGNAL(deleteTagComplete(Tag)),
                     this, SIGNAL(deleteTagComplete(Tag)));
    QObject::connect(m_pWorker, SIGNAL(deleteTagFailed(Tag,QString)),
                     this, SIGNAL(deleteTagFailed(Tag,QString)));
    QObject::connect(m_pWorker, SIGNAL(expungeTagComplete(Tag)),
                     this, SIGNAL(expungeTagComplete(Tag)));
    QObject::connect(m_pWorker, SIGNAL(expungeTagFailed(Tag,QString)),
                     this, SIGNAL(expungeTagFailed(Tag,QString)));

    // Resource-related signal-slot connections:
    QObject::connect(this, SIGNAL(getResourceCountRequest()), m_pWorker, SLOT(onGetResourceCountRequest()));
    QObject::connect(this, SIGNAL(addResourceRequest(ResourceWrapper,Note)),
                     m_pWorker, SLOT(onAddResourceRequest(ResourceWrapper,Note)));
    QObject::connect(this, SIGNAL(updateResourceRequest(ResourceWrapper,Note)),
                     m_pWorker, SLOT(onUpdateResourceRequest(ResourceWrapper,Note)));
    QObject::connect(this, SIGNAL(findResourceRequest(ResourceWrapper,bool)),
                     m_pWorker, SLOT(onFindResourceRequest(ResourceWrapper,bool)));
    QObject::connect(this, SIGNAL(expungeResourceRequest(ResourceWrapper)),
                     m_pWorker, SLOT(onExpungeResourceRequest(ResourceWrapper)));

    // Resource-related signal-signal connections:
    QObject::connect(m_pWorker, SIGNAL(getResourceCountComplete(int)), this, SIGNAL(getResourceCountComplete(int)));
    QObject::connect(m_pWorker, SIGNAL(getResourceCountFailed(QString)), this, SIGNAL(getResourceCountFailed(QString)));
    QObject::connect(m_pWorker, SIGNAL(addResourceComplete(ResourceWrapper,Note)),
                     this, SIGNAL(addResourceComplete(ResourceWrapper,Note)));
    QObject::connect(m_pWorker, SIGNAL(addResourceFailed(ResourceWrapper,Note,QString)),
                     this, SIGNAL(addResourceFailed(ResourceWrapper,Note,QString)));
    QObject::connect(m_pWorker, SIGNAL(updateResourceComplete(ResourceWrapper,Note)),
                     this, SIGNAL(updateResourceComplete(ResourceWrapper,Note)));
    QObject::connect(m_pWorker, SIGNAL(updateResourceFailed(ResourceWrapper,Note,QString)),
                     this, SIGNAL(updateResourceFailed(ResourceWrapper,Note,QString)));
    QObject::connect(m_pWorker, SIGNAL(findResourceComplete(ResourceWrapper,bool)),
                     this, SIGNAL(findResourceComplete(ResourceWrapper,bool)));
    QObject::connect(m_pWorker, SIGNAL(findResourceFailed(ResourceWrapper,bool,QString)),
                     this, SIGNAL(findResourceFailed(ResourceWrapper,bool,QString)));
    QObject::connect(m_pWorker, SIGNAL(expungeResourceComplete(ResourceWrapper)),
                     this, SIGNAL(expungeResourceComplete(ResourceWrapper)));
    QObject::connect(m_pWorker, SIGNAL(expungeResourceFailed(ResourceWrapper,QString)),
                     this, SIGNAL(expungeResourceFailed(ResourceWrapper,QString)));

    // Saved search-related signal-slot connections:
    QObject::connect(this, SIGNAL(getSavedSearchCountRequest()), m_pWorker, SLOT(onGetSavedSearchCountRequest()));
    QObject::connect(this, SIGNAL(addSavedSearchRequest(SavedSearch)),
                     m_pWorker, SLOT(onAddSavedSearchRequest(SavedSearch)));
    QObject::connect(this, SIGNAL(updateSavedSearchRequest(SavedSearch)),
                     m_pWorker, SLOT(onUpdateSavedSearchRequest(SavedSearch)));
    QObject::connect(this, SIGNAL(findSavedSearchRequest(SavedSearch)),
                     m_pWorker, SLOT(onFindSavedSearchRequest(SavedSearch)));
    QObject::connect(this, SIGNAL(listAllSavedSearchesRequest()), m_pWorker, SLOT(onListAllSavedSearchesRequest()));
    QObject::connect(this, SIGNAL(expungeSavedSearchRequest(SavedSearch)),
                     m_pWorker, SLOT(onExpungeSavedSearch(SavedSearch)));

    // Saved search-related signal-signal connections:
    QObject::connect(m_pWorker, SIGNAL(getSavedSearchCountComplete(int)), this, SIGNAL(getSavedSearchCountComplete(int)));
    QObject::connect(m_pWorker, SIGNAL(getSavedSearchCountFailed(QString)), this, SIGNAL(getSavedSearchCountFailed(QString)));
    QObject::connect(m_pWorker, SIGNAL(addSavedSearchComplete(SavedSearch)),
                     this, SIGNAL(addSavedSearchComplete(SavedSearch)));
    QObject::connect(m_pWorker, SIGNAL(addSavedSearchFailed(SavedSearch,QString)),
                     this, SIGNAL(addSavedSearchFailed(SavedSearch,QString)));
    QObject::connect(m_pWorker, SIGNAL(updateSavedSearchComplete(SavedSearch)),
                     this, SIGNAL(updateSavedSearchComplete(SavedSearch)));
    QObject::connect(m_pWorker, SIGNAL(updateSavedSearchFailed(SavedSearch,QString)),
                     this, SIGNAL(updateSavedSearchFailed(SavedSearch,QString)));
    QObject::connect(m_pWorker, SIGNAL(findSavedSearchComplete(SavedSearch)),
                     this, SIGNAL(findSavedSearchComplete(SavedSearch)));
    QObject::connect(m_pWorker, SIGNAL(findSavedSearchFailed(SavedSearch,QString)),
                     this, SIGNAL(findSavedSearchFailed(SavedSearch,QString)));
    QObject::connect(m_pWorker, SIGNAL(listAllSavedSearchesComplete(QList<SavedSearch>)),
                     this, SIGNAL(listAllSavedSearchesComplete(QList<SavedSearch>)));
    QObject::connect(m_pWorker, SIGNAL(listAllSavedSearchesFailed(QString)),
                     this, SIGNAL(listAllSavedSearchesFailed(QString)));
    QObject::connect(m_pWorker, SIGNAL(expungeSavedSearchComplete(SavedSearch)),
                     this, SIGNAL(expungeSavedSearchComplete(SavedSearch)));
    QObject::connect(m_pWorker, SIGNAL(expungeSavedSearchFailed(SavedSearch,QString)),
                     this, SIGNAL(expungeSavedSearchFailed(SavedSearch,QString)));
}

LocalStorageManagerThread::~LocalStorageManagerThread()
{}

void LocalStorageManagerThread::setUseCache(const bool useCache)
{
    m_pWorker->setUseCache(useCache);
}

const LocalStorageCacheManager * LocalStorageManagerThread::localStorageCacheManager() const
{
    return m_pWorker->localStorageCacheManager();
}

void LocalStorageManagerThread::onGetUserCountRequest()
{
    emit getUserCountRequest();
}

void LocalStorageManagerThread::onSwitchUserRequest(QString username, qint32 userId, bool startFromScratch)
{
    emit switchUserRequest(username, userId, startFromScratch);
}

void LocalStorageManagerThread::onAddUserRequest(UserWrapper user)
{
    emit addUserRequest(user);
}

void LocalStorageManagerThread::onUpdateUserRequest(UserWrapper user)
{
    emit updateUserRequest(user);
}

void LocalStorageManagerThread::onFindUserRequest(UserWrapper user)
{
    emit findUserRequest(user);
}

void LocalStorageManagerThread::onDeleteUserRequest(UserWrapper user)
{
    emit deleteUserRequest(user);
}

void LocalStorageManagerThread::onExpungeUserRequest(UserWrapper user)
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

void LocalStorageManagerThread::onAddLinkedNotebookRequest(LinkedNotebook linkedNotebook)
{
    emit addLinkedNotebookRequest(linkedNotebook);
}

void LocalStorageManagerThread::onUpdateLinkedNotebookRequest(LinkedNotebook linkedNotebook)
{
    emit updateLinkedNotebookRequest(linkedNotebook);
}

void LocalStorageManagerThread::onFindLinkedNotebookRequest(LinkedNotebook linkedNotebook)
{
    emit findLinkedNotebookRequest(linkedNotebook);
}

void LocalStorageManagerThread::onListAllLinkedNotebooksRequest()
{
    emit listAllLinkedNotebooksRequest();
}

void LocalStorageManagerThread::onExpungeLinkedNotebookRequest(LinkedNotebook linkedNotebook)
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

void LocalStorageManagerThread::onAddTagRequest(Tag tag)
{
    emit addTagRequest(tag);
}

void LocalStorageManagerThread::onUpdateTagRequest(Tag tag)
{
    emit updateTagRequest(tag);
}

void LocalStorageManagerThread::onLinkTagWithNoteRequest(Tag tag, Note note)
{
    emit linkTagWithNoteRequest(tag, note);
}

void LocalStorageManagerThread::onFindTagRequest(Tag tag)
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

void LocalStorageManagerThread::onDeleteTagRequest(Tag tag)
{
    emit deleteTagRequest(tag);
}

void LocalStorageManagerThread::onExpungeTagRequest(Tag tag)
{
    emit expungeTagRequest(tag);
}

void LocalStorageManagerThread::onGetResourceCountRequest()
{
    emit getResourceCountRequest();
}

void LocalStorageManagerThread::onAddResourceRequest(ResourceWrapper resource, Note note)
{
    emit addResourceRequest(resource, note);
}

void LocalStorageManagerThread::onUpdateResourceRequest(ResourceWrapper resource, Note note)
{
    emit updateResourceRequest(resource, note);
}

void LocalStorageManagerThread::onFindResourceRequest(ResourceWrapper resource, bool withBinaryData)
{
    emit findResourceRequest(resource, withBinaryData);
}

void LocalStorageManagerThread::onExpungeResourceRequest(ResourceWrapper resource)
{
    emit expungeResourceRequest(resource);
}

void LocalStorageManagerThread::onGetSavedSearchCountRequest()
{
    emit getSavedSearchCountRequest();
}

void LocalStorageManagerThread::onAddSavedSearchRequest(SavedSearch search)
{
    emit addSavedSearchRequest(search);
}

void LocalStorageManagerThread::onUpdateSavedSearchRequest(SavedSearch search)
{
    emit updateSavedSearchRequest(search);
}

void LocalStorageManagerThread::onFindSavedSearchRequest(SavedSearch search)
{
    emit findSavedSearchRequest(search);
}

void LocalStorageManagerThread::onListAllSavedSearchesRequest()
{
    emit listAllSavedSearchesRequest();
}

void LocalStorageManagerThread::onExpungeSavedSearch(SavedSearch search)
{
    emit expungeSavedSearchRequest(search);
}

}
