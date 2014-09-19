#include "LocalStorageManagerThreadWorker.h"
#include <client/types/UserWrapper.h>
#include <client/types/Notebook.h>
#include <client/types/SharedNotebookWrapper.h>
#include <client/types/LinkedNotebook.h>
#include <client/types/Note.h>
#include <client/types/Tag.h>
#include <client/types/ResourceWrapper.h>
#include <client/types/SavedSearch.h>
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

void LocalStorageManagerThreadWorker::onGetUserCountRequest()
{
    QString errorDescription;
    int count = m_localStorageManager.GetUserCount(errorDescription);
    if (count < 0) {
        emit getUserCountFailed(errorDescription);
    }
    else {
        emit getUserCountComplete(count);
    }
}

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

void LocalStorageManagerThreadWorker::onAddUserRequest(UserWrapper user)
{
    QString errorDescription;

    bool res = m_localStorageManager.AddUser(user, errorDescription);
    if (!res) {
        emit addUserFailed(user, errorDescription);
        return;
    }

    emit addUserComplete(user);
}

void LocalStorageManagerThreadWorker::onUpdateUserRequest(UserWrapper user)
{
    QString errorDescription;

    bool res = m_localStorageManager.UpdateUser(user, errorDescription);
    if (!res) {
        emit updateUserFailed(user, errorDescription);
        return;
    }

    emit updateUserComplete(user);
}

void LocalStorageManagerThreadWorker::onFindUserRequest(UserWrapper user)
{
    QString errorDescription;

    bool res = m_localStorageManager.FindUser(user, errorDescription);
    if (!res) {
        emit findUserFailed(user, errorDescription);
        return;
    }

    emit findUserComplete(user);
}

void LocalStorageManagerThreadWorker::onDeleteUserRequest(UserWrapper user)
{
    QString errorDescription;

    bool res = m_localStorageManager.DeleteUser(user, errorDescription);
    if (!res) {
        emit deleteUserFailed(user, errorDescription);
        return;
    }

    emit deleteUserComplete(user);
}

void LocalStorageManagerThreadWorker::onExpungeUserRequest(UserWrapper user)
{
    QString errorDescription;

    bool res = m_localStorageManager.ExpungeUser(user, errorDescription);
    if (!res) {
        emit expungeUserFailed(user, errorDescription);
        return;
    }

    emit expungeUserComplete(user);
}

void LocalStorageManagerThreadWorker::onGetNotebookCountRequest()
{
    QString errorDescription;
    int count = m_localStorageManager.GetNotebookCount(errorDescription);
    if (count < 0) {
        emit getNotebookCountFailed(errorDescription);
    }
    else {
        emit getNotebookCountComplete(count);
    }
}

void LocalStorageManagerThreadWorker::onAddNotebookRequest(Notebook notebook)
{
    QString errorDescription;

    bool res = m_localStorageManager.AddNotebook(notebook, errorDescription);
    if (!res) {
        emit addNotebookFailed(notebook, errorDescription);
        return;
    }

    emit addNotebookComplete(notebook);
}

void LocalStorageManagerThreadWorker::onUpdateNotebookRequest(Notebook notebook)
{
    QString errorDescription;

    bool res = m_localStorageManager.UpdateNotebook(notebook, errorDescription);
    if (!res) {
        emit updateNotebookFailed(notebook, errorDescription);
        return;
    }

    emit updateNotebookComplete(notebook);
}

void LocalStorageManagerThreadWorker::onFindNotebookRequest(Notebook notebook)
{
    QString errorDescription;

    bool res = m_localStorageManager.FindNotebook(notebook, errorDescription);
    if (!res) {
        emit findNotebookFailed(notebook, errorDescription);
        return;
    }

    emit findNotebookComplete(notebook);
}

void LocalStorageManagerThreadWorker::onFindDefaultNotebookRequest(Notebook notebook)
{
    QString errorDescription;

    bool res = m_localStorageManager.FindDefaultNotebook(notebook, errorDescription);
    if (!res) {
        emit findDefaultNotebookFailed(notebook, errorDescription);
        return;
    }

    emit findDefaultNotebookComplete(notebook);
}

void LocalStorageManagerThreadWorker::onFindLastUsedNotebookRequest(Notebook notebook)
{
    QString errorDescription;

    bool res = m_localStorageManager.FindLastUsedNotebook(notebook, errorDescription);
    if (!res) {
        emit findLastUsedNotebookFailed(notebook, errorDescription);
        return;
    }

    emit findLastUsedNotebookComplete(notebook);
}

void LocalStorageManagerThreadWorker::onFindDefaultOrLastUsedNotebookRequest(Notebook notebook)
{
    QString errorDescription;

    bool res = m_localStorageManager.FindDefaultOrLastUsedNotebook(notebook, errorDescription);
    if (!res) {
        emit findDefaultOrLastUsedNotebookFailed(notebook, errorDescription);
        return;
    }

    emit findDefaultOrLastUsedNotebookComplete(notebook);
}

void LocalStorageManagerThreadWorker::onListAllNotebooksRequest()
{
    QString errorDescription;
    QList<Notebook> notebooks = m_localStorageManager.ListAllNotebooks(errorDescription);
    if (notebooks.isEmpty() && !errorDescription.isEmpty()) {
        emit listAllNotebooksFailed(errorDescription);
        return;
    }

    emit listAllNotebooksComplete(notebooks);
}

void LocalStorageManagerThreadWorker::onListAllSharedNotebooksRequest()
{
    QString errorDescription;
    QList<SharedNotebookWrapper> sharedNotebooks = m_localStorageManager.ListAllSharedNotebooks(errorDescription);
    if (sharedNotebooks.isEmpty() && !errorDescription.isEmpty()) {
        emit listAllSharedNotebooksFailed(errorDescription);
        return;
    }

    emit listAllSharedNotebooksComplete(sharedNotebooks);
}

void LocalStorageManagerThreadWorker::onListSharedNotebooksPerNotebookGuidRequest(QString notebookGuid)
{
    QString errorDescription;
    QList<SharedNotebookWrapper> sharedNotebooks = m_localStorageManager.ListSharedNotebooksPerNotebookGuid(notebookGuid, errorDescription);
    if (sharedNotebooks.isEmpty() && !errorDescription.isEmpty()) {
        emit listSharedNotebooksPerNotebookGuidFailed(notebookGuid, errorDescription);
        return;
    }

    emit listSharedNotebooksPerNotebookGuidComplete(notebookGuid, sharedNotebooks);
}

void LocalStorageManagerThreadWorker::onExpungeNotebookRequest(Notebook notebook)
{
    QString errorDescription;

    bool res = m_localStorageManager.ExpungeNotebook(notebook, errorDescription);
    if (!res) {
        emit expungeNotebookFailed(notebook, errorDescription);
        return;
    }

    emit expungeNotebookComplete(notebook);
}

void LocalStorageManagerThreadWorker::onGetLinkedNotebookCountRequest()
{
    QString errorDescription;
    int count = m_localStorageManager.GetLinkedNotebookCount(errorDescription);
    if (count < 0) {
        emit getLinkedNotebookCountFailed(errorDescription);
    }
    else {
        emit getLinkedNotebookCountComplete(count);
    }
}

void LocalStorageManagerThreadWorker::onAddLinkedNotebookRequest(LinkedNotebook linkedNotebook)
{
    QString errorDescription;

    bool res = m_localStorageManager.AddLinkedNotebook(linkedNotebook, errorDescription);
    if (!res) {
        emit addLinkedNotebookFailed(linkedNotebook, errorDescription);
        return;
    }

    emit addLinkedNotebookComplete(linkedNotebook);
}

void LocalStorageManagerThreadWorker::onUpdateLinkedNotebookRequest(LinkedNotebook linkedNotebook)
{
    QString errorDescription;

    bool res = m_localStorageManager.UpdateLinkedNotebook(linkedNotebook, errorDescription);
    if (!res) {
        emit updateLinkedNotebookFailed(linkedNotebook, errorDescription);
        return;
    }

    emit updateLinkedNotebookComplete(linkedNotebook);
}

void LocalStorageManagerThreadWorker::onFindLinkedNotebookRequest(LinkedNotebook linkedNotebook)
{
    QString errorDescription;

    bool res = m_localStorageManager.FindLinkedNotebook(linkedNotebook, errorDescription);
    if (!res) {
        emit findLinkedNotebookFailed(linkedNotebook, errorDescription);
        return;
    }

    emit findLinkedNotebookComplete(linkedNotebook);
}

void LocalStorageManagerThreadWorker::onListAllLinkedNotebooksRequest()
{
    QString errorDescription;
    QList<LinkedNotebook> linkedNotebooks = m_localStorageManager.ListAllLinkedNotebooks(errorDescription);
    if (linkedNotebooks.isEmpty() && !errorDescription.isEmpty()) {
        emit listAllLinkedNotebooksFailed(errorDescription);
        return;
    }

    emit listAllLinkedNotebooksComplete(linkedNotebooks);
}

void LocalStorageManagerThreadWorker::onExpungeLinkedNotebookRequest(LinkedNotebook linkedNotebook)
{
    QString errorDescription;

    bool res = m_localStorageManager.ExpungeLinkedNotebook(linkedNotebook, errorDescription);
    if (!res) {
        emit expungeLinkedNotebookFailed(linkedNotebook, errorDescription);
        return;
    }

    emit expungeLinkedNotebookComplete(linkedNotebook);
}

void LocalStorageManagerThreadWorker::onGetNoteCountRequest()
{
    QString errorDescription;
    int count = m_localStorageManager.GetNoteCount(errorDescription);
    if (count < 0) {
        emit getNoteCountFailed(errorDescription);
    }
    else {
        emit getNoteCountComplete(count);
    }
}

void LocalStorageManagerThreadWorker::onAddNoteRequest(Note note, Notebook notebook)
{
    QString errorDescription;

    bool res = m_localStorageManager.AddNote(note, notebook, errorDescription);
    if (!res) {
        emit addNoteFailed(note, notebook, errorDescription);
        return;
    }

    emit addNoteComplete(note, notebook);
}

void LocalStorageManagerThreadWorker::onUpdateNoteRequest(Note note, Notebook notebook)
{
    QString errorDescription;

    bool res = m_localStorageManager.UpdateNote(note, notebook, errorDescription);
    if (!res) {
        emit updateNoteFailed(note, notebook, errorDescription);
        return;
    }

    emit updateNoteComplete(note, notebook);
}

void LocalStorageManagerThreadWorker::onFindNoteRequest(Note note, bool withResourceBinaryData)
{
    QString errorDescription;

    bool res = m_localStorageManager.FindNote(note, errorDescription, withResourceBinaryData);
    if (!res) {
        emit findNoteFailed(note, withResourceBinaryData, errorDescription);
        return;
    }

    emit findNoteComplete(note, withResourceBinaryData);
}

void LocalStorageManagerThreadWorker::onListAllNotesPerNotebookRequest(Notebook notebook,
                                                                       bool withResourceBinaryData)
{
    QString errorDescription;

    QList<Note> notes = m_localStorageManager.ListAllNotesPerNotebook(notebook, errorDescription,
                                                                      withResourceBinaryData);
    if (notes.isEmpty() && !errorDescription.isEmpty()) {
        emit listAllNotesPerNotebookFailed(notebook, withResourceBinaryData, errorDescription);
        return;
    }

    emit listAllNotesPerNotebookComplete(notebook, withResourceBinaryData, notes);
}

void LocalStorageManagerThreadWorker::onDeleteNoteRequest(Note note)
{
    QString errorDescription;

    bool res = m_localStorageManager.DeleteNote(note, errorDescription);
    if (!res) {
        emit deleteNoteFailed(note, errorDescription);
        return;
    }

    emit deleteNoteComplete(note);
}

void LocalStorageManagerThreadWorker::onExpungeNoteRequest(Note note)
{
    QString errorDescription;

    bool res = m_localStorageManager.ExpungeNote(note, errorDescription);
    if (!res) {
        emit expungeNoteFailed(note, errorDescription);
        return;
    }

    emit expungeNoteComplete(note);
}

void LocalStorageManagerThreadWorker::onGetTagCountRequest()
{
    QString errorDescription;
    int count = m_localStorageManager.GetTagCount(errorDescription);
    if (count < 0) {
        emit getTagCountFailed(errorDescription);
    }
    else {
        emit getTagCountComplete(count);
    }
}

void LocalStorageManagerThreadWorker::onAddTagRequest(Tag tag)
{
    QString errorDescription;

    bool res = m_localStorageManager.AddTag(tag, errorDescription);
    if (!res) {
        emit addTagFailed(tag, errorDescription);
        return;
    }

    emit addTagComplete(tag);
}

void LocalStorageManagerThreadWorker::onUpdateTagRequest(Tag tag)
{
    QString errorDescription;

    bool res = m_localStorageManager.UpdateTag(tag, errorDescription);
    if (!res) {
        emit updateTagFailed(tag, errorDescription);
        return;
    }

    emit updateTagComplete(tag);
}

void LocalStorageManagerThreadWorker::onLinkTagWithNoteRequest(Tag tag, Note note)
{
    QString errorDescription;

    bool res = m_localStorageManager.LinkTagWithNote(tag, note, errorDescription);
    if (!res) {
        emit linkTagWithNoteFailed(tag, note, errorDescription);
        return;
    }

    emit linkTagWithNoteComplete(tag, note);
}

void LocalStorageManagerThreadWorker::onFindTagRequest(Tag tag)
{
    QString errorDescription;

    bool res = m_localStorageManager.FindTag(tag, errorDescription);
    if (!res) {
        emit findTagFailed(tag, errorDescription);
        return;
    }

    emit findTagComplete(tag);
}

void LocalStorageManagerThreadWorker::onListAllTagsPerNoteRequest(Note note)
{
    QString errorDescription;

    QList<Tag> tags = m_localStorageManager.ListAllTagsPerNote(note, errorDescription);
    if (tags.isEmpty() && !errorDescription.isEmpty()) {
        emit listAllTagsPerNoteFailed(note, errorDescription);
        return;
    }

    emit listAllTagsPerNoteComplete(tags, note);
}

void LocalStorageManagerThreadWorker::onListAllTagsRequest()
{
    QString errorDescription;

    QList<Tag> tags = m_localStorageManager.ListAllTags(errorDescription);
    if (tags.isEmpty() && !errorDescription.isEmpty()) {
        emit listAllTagsFailed(errorDescription);
        return;
    }

    emit listAllTagsComplete(tags);
}

void LocalStorageManagerThreadWorker::onDeleteTagRequest(Tag tag)
{
    QString errorDescription;

    bool res = m_localStorageManager.DeleteTag(tag, errorDescription);
    if (!res) {
        emit deleteTagFailed(tag, errorDescription);
        return;
    }

    emit deleteTagComplete(tag);
}

void LocalStorageManagerThreadWorker::onExpungeTagRequest(Tag tag)
{
    QString errorDescription;

    bool res = m_localStorageManager.ExpungeTag(tag, errorDescription);
    if (!res) {
        emit expungeTagFailed(tag, errorDescription);
        return;
    }

    emit expungeTagComplete(tag);
}

void LocalStorageManagerThreadWorker::onGetResourceCountRequest()
{
    QString errorDescription;
    int count = m_localStorageManager.GetEnResourceCount(errorDescription);
    if (count < 0) {
        emit getResourceCountFailed(errorDescription);
    }
    else {
        emit getResourceCountComplete(count);
    }
}

void LocalStorageManagerThreadWorker::onAddResourceRequest(QSharedPointer<ResourceWrapper> resource, Note note)
{
    QString errorDescription;

    if (resource.isNull()) {
        errorDescription = "Internal error: detected null pointer to resource "
                           "on attempt to add resource to local storage";
        QNCRITICAL(errorDescription);
        emit addResourceFailed(resource, note, errorDescription);
        return;
    }

    bool res = m_localStorageManager.AddEnResource(*resource, note, errorDescription);
    if (!res) {
        emit addResourceFailed(resource, note, errorDescription);
        return;
    }

    emit addResourceComplete(resource, note);
}

void LocalStorageManagerThreadWorker::onUpdateResourceRequest(QSharedPointer<ResourceWrapper> resource, Note note)
{
    QString errorDescription;

    if (resource.isNull()) {
        errorDescription = "Internal error: detected null pointer to resource "
                           "on attempt to update resource in local storage";
        QNCRITICAL(errorDescription);
        emit updateResourceFailed(resource, note, errorDescription);
        return;
    }

    bool res = m_localStorageManager.UpdateEnResource(*resource, note, errorDescription);
    if (!res) {
        emit updateResourceFailed(resource, note, errorDescription);
        return;
    }

    emit updateResourceComplete(resource, note);
}

void LocalStorageManagerThreadWorker::onFindResourceRequest(QSharedPointer<ResourceWrapper> resource, bool withBinaryData)
{
    QString errorDescription;

    if (resource.isNull()) {
        errorDescription = "Internal error: detected null pointer to resource "
                           "on attempt to find resource in local storage";
        QNCRITICAL(errorDescription);
        emit findResourceFailed(resource, withBinaryData, errorDescription);
        return;
    }

    bool res = m_localStorageManager.FindEnResource(*resource, errorDescription, withBinaryData);
    if (!res) {
        emit findResourceFailed(resource, withBinaryData, errorDescription);
        return;
    }

    emit findResourceComplete(resource, withBinaryData);
}

void LocalStorageManagerThreadWorker::onExpungeResourceRequest(QSharedPointer<ResourceWrapper> resource)
{
    QString errorDescription;

    if (resource.isNull()) {
        errorDescription = "Internal error: detected null pointer to resource "
                           "on attempt to expunge resource from local storage";
        QNCRITICAL(errorDescription);
        emit expungeResourceFailed(resource, errorDescription);
        return;
    }

    bool res = m_localStorageManager.ExpungeEnResource(*resource, errorDescription);
    if (!res) {
        emit expungeResourceFailed(resource, errorDescription);
        return;
    }

    emit expungeResourceComplete(resource);
}

void LocalStorageManagerThreadWorker::onGetSavedSearchCountRequest()
{
    QString errorDescription;
    int count = m_localStorageManager.GetSavedSearchCount(errorDescription);
    if (count < 0) {
        emit getSavedSearchCountFailed(errorDescription);
    }
    else {
        emit getSavedSearchCountComplete(count);
    }
}

void LocalStorageManagerThreadWorker::onAddSavedSearchRequest(SavedSearch search)
{
    QString errorDescription;

    bool res = m_localStorageManager.AddSavedSearch(search, errorDescription);
    if (!res) {
        emit addSavedSearchFailed(search, errorDescription);
        return;
    }

    emit addSavedSearchComplete(search);
}

void LocalStorageManagerThreadWorker::onUpdateSavedSearchRequest(SavedSearch search)
{
    QString errorDescription;

    bool res = m_localStorageManager.UpdateSavedSearch(search, errorDescription);
    if (!res) {
        emit updateSavedSearchFailed(search, errorDescription);
        return;
    }

    emit updateSavedSearchComplete(search);
}

void LocalStorageManagerThreadWorker::onFindSavedSearchRequest(SavedSearch search)
{
    QString errorDescription;

    bool res = m_localStorageManager.FindSavedSearch(search, errorDescription);
    if (!res) {
        emit findSavedSearchFailed(search, errorDescription);
        return;
    }

    emit findSavedSearchComplete(search);
}

void LocalStorageManagerThreadWorker::onListAllSavedSearchesRequest()
{
    QString errorDescription;
    QList<SavedSearch> searches = m_localStorageManager.ListAllSavedSearches(errorDescription);
    if (searches.isEmpty() && !errorDescription.isEmpty()) {
        emit listAllSavedSearchesFailed(errorDescription);
        return;
    }

    emit listAllSavedSearchesComplete(searches);
}

void LocalStorageManagerThreadWorker::onExpungeSavedSearch(SavedSearch search)
{
    QString errorDescription;

    bool res = m_localStorageManager.ExpungeSavedSearch(search, errorDescription);
    if (!res) {
        emit expungeSavedSearchFailed(search, errorDescription);
        return;
    }

    emit expungeSavedSearchComplete(search);
}

} // namespace qute_note
