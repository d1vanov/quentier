#include "FullSynchronizationManager.h"
#include <client/local_storage/LocalStorageManagerThreadWorker.h>
#include <tools/QuteNoteCheckPtr.h>
#include <logging/QuteNoteLogger.h>

namespace qute_note {

FullSynchronizationManager::FullSynchronizationManager(LocalStorageManagerThreadWorker & localStorageManagerThreadWorker,
                                                       QSharedPointer<qevercloud::NoteStore> pNoteStore,
                                                       QSharedPointer<qevercloud::EvernoteOAuthWebView::OAuthResult> pOAuthResult,
                                                       QObject * parent) :
    QObject(parent),
    m_pNoteStore(pNoteStore),
    m_pOAuthResult(pOAuthResult)
{
    createConnections(localStorageManagerThreadWorker);
}

void FullSynchronizationManager::init()
{
    // TODO: implement
}

void FullSynchronizationManager::onFindUserCompleted(UserWrapper user)
{
    // TODO: implement
}

void FullSynchronizationManager::onFindUserFailed(UserWrapper user)
{
    // TODO: implement
}

void FullSynchronizationManager::onFindNotebookCompleted(Notebook notebook)
{
    // TODO: implement
}

void FullSynchronizationManager::onFindNotebookFailed(Notebook notebook)
{
    // TODO: implement
}

void FullSynchronizationManager::onFindNoteCompleted(Note note)
{
    // TODO: implement
}

void FullSynchronizationManager::onFindNoteFailed(Note note)
{
    // TODO: implement
}

void FullSynchronizationManager::onFindTagCompleted(Tag tag)
{
    // TODO: implement
}

void FullSynchronizationManager::onFindTagFailed(Tag tag)
{
    // TODO: implement
}

void FullSynchronizationManager::onFindResourceCompleted(ResourceWrapper resource)
{
    // TODO: implement
}

void FullSynchronizationManager::onFindResourceFailed(ResourceWrapper resource)
{
    // TODO: implement
}

void FullSynchronizationManager::onFindLinkedNotebookCompleted(LinkedNotebook linkedNotebook)
{
    // TODO: implement
}

void FullSynchronizationManager::onFindLinkedNotebookFailed(LinkedNotebook linkedNotebook)
{
    // TODO: implement
}

void FullSynchronizationManager::onFindSavedSearchCompleted(SavedSearch savedSearch)
{
    // TODO: implement
}

void FullSynchronizationManager::onFindSavedSearchFailed(SavedSearch savedSearch)
{
    // TODO: implement
}

void FullSynchronizationManager::createConnections(LocalStorageManagerThreadWorker & localStorageManagerThreadWorker)
{
    // Connect local signals with localStorageManagerThread's slots
    QObject::connect(this, SIGNAL(addUser(UserWrapper)), &localStorageManagerThreadWorker, SLOT(onAddUserRequest(UserWrapper)));
    QObject::connect(this, SIGNAL(updateUser(UserWrapper)), &localStorageManagerThreadWorker, SLOT(onUpdateUserRequest(UserWrapper)));
    QObject::connect(this, SIGNAL(findUser(UserWrapper)), &localStorageManagerThreadWorker, SLOT(onFindUserRequest(UserWrapper)));
    QObject::connect(this, SIGNAL(deleteUser(UserWrapper)), &localStorageManagerThreadWorker, SLOT(onDeleteUserRequest(UserWrapper)));
    QObject::connect(this, SIGNAL(expungeUser(UserWrapper)), &localStorageManagerThreadWorker, SLOT(onExpungeUserRequest(UserWrapper)));

    QObject::connect(this, SIGNAL(addNotebook(Notebook)), &localStorageManagerThreadWorker, SLOT(onAddNotebookRequest(Notebook)));
    QObject::connect(this, SIGNAL(updateNotebook(Notebook)), &localStorageManagerThreadWorker, SLOT(onUpdateNotebookRequest(Notebook)));
    QObject::connect(this, SIGNAL(findNotebook(Notebook)), &localStorageManagerThreadWorker, SLOT(onFindNotebookRequest(Notebook)));
    QObject::connect(this, SIGNAL(expungeNotebook(Notebook)), &localStorageManagerThreadWorker, SLOT(onExpungeNotebookRequest(Notebook)));

    QObject::connect(this, SIGNAL(addNote(Note,Notebook)), &localStorageManagerThreadWorker, SLOT(onAddNoteRequest(Note,Notebook)));
    QObject::connect(this, SIGNAL(updateNote(Note,Notebook)), &localStorageManagerThreadWorker, SLOT(onUpdateNoteRequest(Note,Notebook)));
    QObject::connect(this, SIGNAL(findNote(Note,bool)), &localStorageManagerThreadWorker, SLOT(onFindNoteRequest(Note,bool)));
    QObject::connect(this, SIGNAL(deleteNote(Note)), &localStorageManagerThreadWorker, SLOT(onDeleteNoteRequest(Note)));
    QObject::connect(this, SIGNAL(expungeNote(Note)), &localStorageManagerThreadWorker, SLOT(onExpungeNoteRequest(Note)));

    QObject::connect(this, SIGNAL(addTag(Tag)), &localStorageManagerThreadWorker, SLOT(onAddTagRequest(Tag)));
    QObject::connect(this, SIGNAL(updateTag(Tag)), &localStorageManagerThreadWorker, SLOT(onUpdateTagRequest(Tag)));
    QObject::connect(this, SIGNAL(findTag(Tag)), &localStorageManagerThreadWorker, SLOT(onFindTagRequest(Tag)));
    QObject::connect(this, SIGNAL(deleteTag(Tag)), &localStorageManagerThreadWorker, SLOT(onDeleteTagRequest(Tag)));
    QObject::connect(this, SIGNAL(expungeTag(Tag)), &localStorageManagerThreadWorker, SLOT(onExpungeTagRequest(Tag)));

    QObject::connect(this, SIGNAL(addResource(ResourceWrapper,Note)), &localStorageManagerThreadWorker, SLOT(onAddResourceRequest(ResourceWrapper,Note)));
    QObject::connect(this, SIGNAL(updateResource(ResourceWrapper,Note)), &localStorageManagerThreadWorker, SLOT(onUpdateResourceRequest(ResourceWrapper,Note)));
    QObject::connect(this, SIGNAL(findResource(ResourceWrapper,bool)), &localStorageManagerThreadWorker, SLOT(onFindResourceRequest(ResourceWrapper,bool)));
    QObject::connect(this, SIGNAL(expungeResource(ResourceWrapper)), &localStorageManagerThreadWorker, SLOT(onExpungeResourceRequest(ResourceWrapper)));

    QObject::connect(this, SIGNAL(addLinkedNotebook(LinkedNotebook)), &localStorageManagerThreadWorker, SLOT(onAddLinkedNotebookRequest(LinkedNotebook)));
    QObject::connect(this, SIGNAL(updateLinkedNotebook(LinkedNotebook)), &localStorageManagerThreadWorker, SLOT(onUpdateLinkedNotebookRequest(LinkedNotebook)));
    QObject::connect(this, SIGNAL(findLinkedNotebook(LinkedNotebook)), &localStorageManagerThreadWorker, SLOT(onFindLinkedNotebookRequest(LinkedNotebook)));
    QObject::connect(this, SIGNAL(expungeLinkedNotebook(LinkedNotebook)), &localStorageManagerThreadWorker, SLOT(onExpungeLinkedNotebookRequest(LinkedNotebook)));

    QObject::connect(this, SIGNAL(addSavedSearch(SavedSearch)), &localStorageManagerThreadWorker, SLOT(onAddSavedSearchRequest(SavedSearch)));
    QObject::connect(this, SIGNAL(updateSavedSearch(SavedSearch)), &localStorageManagerThreadWorker, SLOT(onUpdateSavedSearchRequest(SavedSearch)));
    QObject::connect(this, SIGNAL(findSavedSearch(SavedSearch)), &localStorageManagerThreadWorker, SLOT(onFindSavedSearchRequest(SavedSearch)));
    QObject::connect(this, SIGNAL(expungeSavedSearch(SavedSearch)), &localStorageManagerThreadWorker, SLOT(onExpungeSavedSearch(SavedSearch)));

    // Connect localStorageManagerThread's signals to local slots
    QObject::connect(&localStorageManagerThreadWorker, SIGNAL(findUserComplete(UserWrapper)), this, SLOT(onFindUserCompleted(UserWrapper)));
    QObject::connect(&localStorageManagerThreadWorker, SIGNAL(findUserFailed(UserWrapper,QString)), this, SLOT(onFindUserFailed(UserWrapper)));

    QObject::connect(&localStorageManagerThreadWorker, SIGNAL(findNotebookComplete(Notebook)), this, SLOT(onFindNotebookCompleted(Notebook)));
    QObject::connect(&localStorageManagerThreadWorker, SIGNAL(findNotebookFailed(Notebook,QString)), this, SLOT(onFindNotebookFailed(Notebook)));

    QObject::connect(&localStorageManagerThreadWorker, SIGNAL(findNoteComplete(Note,bool)), this, SLOT(onFindNoteCompleted(Note)));
    QObject::connect(&localStorageManagerThreadWorker, SIGNAL(findNoteFailed(Note,bool,QString)), this, SLOT(onFindNoteFailed(Note)));

    QObject::connect(&localStorageManagerThreadWorker, SIGNAL(findTagComplete(Tag)), this, SLOT(onFindTagCompleted(Tag)));
    QObject::connect(&localStorageManagerThreadWorker, SIGNAL(findTagFailed(Tag,QString)), this, SLOT(onFindTagFailed(Tag)));

    QObject::connect(&localStorageManagerThreadWorker, SIGNAL(findResourceComplete(ResourceWrapper,bool)), this, SLOT(onFindResourceCompleted(ResourceWrapper)));
    QObject::connect(&localStorageManagerThreadWorker, SIGNAL(findResourceFailed(ResourceWrapper,bool,QString)), this, SLOT(onFindResourceFailed(ResourceWrapper)));

    QObject::connect(&localStorageManagerThreadWorker, SIGNAL(findLinkedNotebookComplete(LinkedNotebook)), this, SLOT(onFindLinkedNotebookCompleted(LinkedNotebook)));
    QObject::connect(&localStorageManagerThreadWorker, SIGNAL(findLinkedNotebookFailed(LinkedNotebook,QString)), this, SLOT(onFindLinkedNotebookFailed(LinkedNotebook)));

    QObject::connect(&localStorageManagerThreadWorker, SIGNAL(findSavedSearchComplete(SavedSearch)), this, SLOT(onFindSavedSearchCompleted(SavedSearch)));
    QObject::connect(&localStorageManagerThreadWorker, SIGNAL(findSavedSearchFailed(SavedSearch,QString)), this, SLOT(onFindSavedSearchFailed(SavedSearch)));
}

} // namespace qute_note
