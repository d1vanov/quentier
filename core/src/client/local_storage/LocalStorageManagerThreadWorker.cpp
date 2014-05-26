#include "LocalStorageManagerThreadWorker.h"
#include <client/types/IUser.h>
#include <client/types/Notebook.h>
#include <client/types/SharedNotebookWrapper.h>
#include <client/types/LinkedNotebook.h>
#include <client/types/Note.h>
#include <client/types/Tag.h>
#include <client/types/IResource.h>
#include <client/types/SavedSearch.h>
#include <logging/QuteNoteLogger.h>

#define QNTRANSLATE(string) \
    string = QObject::tr(qPrintable(string));

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

void LocalStorageManagerThreadWorker::onAddUserRequest(QSharedPointer<IUser> user)
{
    QString errorDescription;

    if (user.isNull()) {
        errorDescription = "Internal error: detected null pointer on attempt "
                           "to add user to local storage";
        QNCRITICAL(errorDescription);
        QNTRANSLATE(errorDescription);
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
        errorDescription = "Internal error: detected null pointer on attempt "
                           "to update user in local storage";
        QNCRITICAL(errorDescription);
        QNTRANSLATE(errorDescription);
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
        errorDescription = "Internal error: detected null pointer "
                           "on attempt to find user in local storage";
        QNCRITICAL(errorDescription);
        QNTRANSLATE(errorDescription);
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
        errorDescription = "Internal error: detected null pointer "
                           "on attempt to delete user in local storage";
        QNCRITICAL(errorDescription);
        QNTRANSLATE(errorDescription);
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
        errorDescription = "Internal error: detected null pointer "
                           "on attempt to expunge user from local storage";
        QNCRITICAL(errorDescription);
        QNTRANSLATE(errorDescription);
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

void LocalStorageManagerThreadWorker::onAddNotebookRequest(QSharedPointer<Notebook> notebook)
{
    QString errorDescription;

    if (notebook.isNull()) {
        errorDescription = "Internal error: detected null pointer "
                           "on attempt to add notebook to local storage";
        QNCRITICAL(errorDescription);
        QNTRANSLATE(errorDescription);
        emit addNotebookFailed(notebook, errorDescription);
        return;
    }

    bool res = m_localStorageManager.AddNotebook(*notebook, errorDescription);
    if (!res) {
        emit addNotebookFailed(notebook, errorDescription);
        return;
    }

    emit addNotebookComplete(notebook);
}

void LocalStorageManagerThreadWorker::onUpdateNotebookRequest(QSharedPointer<Notebook> notebook)
{
    QString errorDescription;

    if (notebook.isNull()) {
        errorDescription = "Internal error: detected null pointer "
                           "on attempt to update notebook in local storage";
        QNCRITICAL(errorDescription);
        QNTRANSLATE(errorDescription);
        emit updateNotebookFailed(notebook, errorDescription);
        return;
    }

    bool res = m_localStorageManager.UpdateNotebook(*notebook, errorDescription);
    if (!res) {
        emit updateNotebookFailed(notebook, errorDescription);
        return;
    }

    emit updateNotebookComplete(notebook);
}

void LocalStorageManagerThreadWorker::onFindNotebookRequest(QSharedPointer<Notebook> notebook)
{
    QString errorDescription;

    if (notebook.isNull()) {
        errorDescription = "Internal error: detected null pointer "
                           "on attempt to find notebook in local storage";
        QNCRITICAL(errorDescription);
        QNTRANSLATE(errorDescription);
        emit findNotebookFailed(notebook, errorDescription);
        return;
    }

    bool res = m_localStorageManager.FindNotebook(*notebook, errorDescription);
    if (!res) {
        emit findNotebookFailed(notebook, errorDescription);
        return;
    }

    emit findNotebookComplete(notebook);
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

void LocalStorageManagerThreadWorker::onExpungeNotebookRequest(QSharedPointer<Notebook> notebook)
{
    QString errorDescription;

    if (notebook.isNull()) {
        errorDescription = "Internal error: detected null pointer "
                           "on attempt to expunge notebook from local storage";
        QNCRITICAL(errorDescription);
        QNTRANSLATE(errorDescription);
        emit expungeNotebookFailed(notebook, errorDescription);
        return;
    }

    bool res = m_localStorageManager.ExpungeNotebook(*notebook, errorDescription);
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

void LocalStorageManagerThreadWorker::onAddLinkedNotebookRequest(QSharedPointer<LinkedNotebook> linkedNotebook)
{
    QString errorDescription;

    if (linkedNotebook.isNull()) {
        errorDescription = "Internal error: detected null pointer "
                           "on attempt to add linked notebook to local storage";
        QNCRITICAL(errorDescription);
        QNTRANSLATE(errorDescription);
        emit addLinkedNotebookFailed(linkedNotebook, errorDescription);
        return;
    }

    bool res = m_localStorageManager.AddLinkedNotebook(*linkedNotebook, errorDescription);
    if (!res) {
        emit addLinkedNotebookFailed(linkedNotebook, errorDescription);
        return;
    }

    emit addLinkedNotebookComplete(linkedNotebook);
}

void LocalStorageManagerThreadWorker::onUpdateLinkedNotebookRequest(QSharedPointer<LinkedNotebook> linkedNotebook)
{
    QString errorDescription;

    if (linkedNotebook.isNull()) {
        errorDescription = "Internal error: detected null pointer "
                           "on attempt to update linked notebook in local storage";
        QNCRITICAL(errorDescription);
        QNTRANSLATE(errorDescription);
        emit updateLinkedNotebookFailed(linkedNotebook, errorDescription);
        return;
    }

    bool res = m_localStorageManager.UpdateLinkedNotebook(*linkedNotebook, errorDescription);
    if (!res) {
        emit updateLinkedNotebookFailed(linkedNotebook, errorDescription);
        return;
    }

    emit updateLinkedNotebookComplete(linkedNotebook);
}

void LocalStorageManagerThreadWorker::onFindLinkedNotebookRequest(QSharedPointer<LinkedNotebook> linkedNotebook)
{
    QString errorDescription;

    if (linkedNotebook.isNull()) {
        errorDescription = "Internal error: detected null pointer "
                           "on attempt to find linked notebook in local storage";
        QNCRITICAL(errorDescription);
        QNTRANSLATE(errorDescription);
        emit findLinkedNotebookFailed(linkedNotebook, errorDescription);
        return;
    }

    bool res = m_localStorageManager.FindLinkedNotebook(*linkedNotebook, errorDescription);
    if (!res) {
        emit findLinkedNotebookFailed(linkedNotebook, errorDescription);
        return;
    }

    emit findLinkedNotebookCompleted(linkedNotebook);
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

void LocalStorageManagerThreadWorker::onExpungeLinkedNotebookRequest(QSharedPointer<LinkedNotebook> linkedNotebook)
{
    QString errorDescription;

    if (linkedNotebook.isNull()) {
        errorDescription = "Internal error: detected null pointer "
                           "on attempt to expunge linked notebook from local storage";
        QNCRITICAL(errorDescription);
        QNTRANSLATE(errorDescription);
        emit expungeLinkedNotebookFailed(linkedNotebook, errorDescription);
        return;
    }

    bool res = m_localStorageManager.ExpungeLinkedNotebook(*linkedNotebook, errorDescription);
    if (!res) {
        emit expungeLinkedNotebookFailed(linkedNotebook, errorDescription);
        return;
    }

    emit expungeLinkedNotebookCompleted(linkedNotebook);
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

void LocalStorageManagerThreadWorker::onAddNoteRequest(QSharedPointer<Note> note, QSharedPointer<Notebook> notebook)
{
    QString errorDescription;

    if (note.isNull()) {
        errorDescription = "Internal error: detected null pointer to note "
                           "on attempt to add note to local storage";
        QNCRITICAL(errorDescription);
        QNTRANSLATE(errorDescription);
        emit addNoteFailed(note, notebook, errorDescription);
        return;
    }

    if (notebook.isNull()) {
        errorDescription = "Internal error: detected null pointer to notebook "
                           "on attempt to add note to local storage";
        QNCRITICAL(errorDescription);
        QNTRANSLATE(errorDescription);
        emit addNoteFailed(note, notebook, errorDescription);
        return;
    }

    bool res = m_localStorageManager.AddNote(*note, *notebook, errorDescription);
    if (!res) {
        emit addNoteFailed(note, notebook, errorDescription);
        return;
    }

    emit addNoteComplete(note, notebook);
}

void LocalStorageManagerThreadWorker::onUpdateNoteRequest(QSharedPointer<Note> note, QSharedPointer<Notebook> notebook)
{
    QString errorDescription;

    if (note.isNull()) {
        errorDescription = "Internal error: detected null pointer to note "
                           "on attempt to update note in local storage";
        QNCRITICAL(errorDescription);
        QNTRANSLATE(errorDescription);
        emit updateNoteFailed(note, notebook, errorDescription);
        return;
    }

    if (notebook.isNull()) {
        errorDescription = "Internal error: detected null pointer to notebook "
                           "on attempt to update note in local storage";
        QNCRITICAL(errorDescription);
        QNTRANSLATE(errorDescription);
        emit updateNoteFailed(note, notebook, errorDescription);
        return;
    }

    bool res = m_localStorageManager.UpdateNote(*note, *notebook, errorDescription);
    if (!res) {
        emit updateNoteFailed(note, notebook, errorDescription);
        return;
    }

    emit updateNoteComplete(note, notebook);
}

void LocalStorageManagerThreadWorker::onFindNoteRequest(QSharedPointer<Note> note, bool withResourceBinaryData)
{
    QString errorDescription;

    if (note.isNull()) {
        errorDescription = "Internal error: detected null pointer to note "
                           "on attempt to find note in local storage";
        QNCRITICAL(errorDescription);
        QNTRANSLATE(errorDescription);
        emit findNoteFailed(note, errorDescription);
        return;
    }

    bool res = m_localStorageManager.FindNote(*note, errorDescription, withResourceBinaryData);
    if (!res) {
        emit findNoteFailed(note, errorDescription);
        return;
    }

    emit findNoteComplete(note);
}

void LocalStorageManagerThreadWorker::onListAllNotesPerNotebookRequest(QSharedPointer<Notebook> notebook,
                                                                       bool withResourceBinaryData)
{
    QString errorDescription;

    if (notebook.isNull()) {
        errorDescription = "Internal error: detected null pointer to notebook "
                           "on attempt to list notes per notebook in local storage";
        QNCRITICAL(errorDescription);
        QNTRANSLATE(errorDescription);
        emit listAllNotesPerNotebookFailed(notebook, errorDescription);
        return;
    }

    QList<Note> notes = m_localStorageManager.ListAllNotesPerNotebook(*notebook, errorDescription,
                                                                      withResourceBinaryData);
    if (notes.isEmpty() && !errorDescription.isEmpty()) {
        emit listAllNotesPerNotebookFailed(notebook, errorDescription);
        return;
    }

    emit listAllNotesPerNotebookComplete(notebook, notes);
}

void LocalStorageManagerThreadWorker::onDeleteNoteRequest(QSharedPointer<Note> note)
{
    QString errorDescription;

    if (note.isNull()) {
        errorDescription = "Internal error: detected null pointer to note "
                           "on attempt to delete note from local storage";
        QNCRITICAL(errorDescription);
        QNTRANSLATE(errorDescription);
        emit deleteNoteFailed(note, errorDescription);
        return;
    }

    bool res = m_localStorageManager.DeleteNote(*note, errorDescription);
    if (!res) {
        emit deleteNoteFailed(note, errorDescription);
        return;
    }

    emit deleteNoteComplete(note);
}

void LocalStorageManagerThreadWorker::onExpungeNoteRequest(QSharedPointer<Note> note)
{
    QString errorDescription;

    if (note.isNull()) {
        errorDescription = "Internal error: detected null pointer to note "
                           "on attempt to expunge note from local storage";
        QNCRITICAL(errorDescription);
        QNTRANSLATE(errorDescription);
        emit expungeNoteFailed(note, errorDescription);
        return;
    }

    bool res = m_localStorageManager.ExpungeNote(*note, errorDescription);
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

void LocalStorageManagerThreadWorker::onAddTagRequest(QSharedPointer<Tag> tag)
{
    QString errorDescription;

    if (tag.isNull()) {
        errorDescription = "Internal error: detected null pointer to tag "
                           "on attempt to add tag to local storage";
        QNCRITICAL(errorDescription);
        QNTRANSLATE(errorDescription);
        emit addTagFailed(tag, errorDescription);
        return;
    }

    bool res = m_localStorageManager.AddTag(*tag, errorDescription);
    if (!res) {
        emit addTagFailed(tag, errorDescription);
        return;
    }

    emit addTagComplete(tag);
}

void LocalStorageManagerThreadWorker::onUpdateTagRequest(QSharedPointer<Tag> tag)
{
    QString errorDescription;

    if (tag.isNull()) {
        errorDescription = "Internal error: detected null pointer to tag "
                           "on attempt to update tag in local storage";
        QNCRITICAL(errorDescription);
        QNTRANSLATE(errorDescription);
        emit updateTagFailed(tag, errorDescription);
        return;
    }

    bool res = m_localStorageManager.UpdateTag(*tag, errorDescription);
    if (!res) {
        emit updateTagFailed(tag, errorDescription);
        return;
    }

    emit updateTagComplete(tag);
}

void LocalStorageManagerThreadWorker::onLinkTagWithNoteRequest(QSharedPointer<Tag> tag, QSharedPointer<Note> note)
{
    QString errorDescription;

    if (tag.isNull()) {
        errorDescription = "Internal error: detected null pointer to tag "
                           "on attempt to link tag with note";
        QNCRITICAL(errorDescription);
        QNTRANSLATE(errorDescription);
        emit linkTagWithNoteFailed(tag, note, errorDescription);
        return;
    }

    if (note.isNull()) {
        errorDescription = "Internal error: detected null pointer to note "
                           "on attempt to link tag with note in local storage";
        QNCRITICAL(errorDescription);
        QNTRANSLATE(errorDescription);
        emit linkTagWithNoteFailed(tag, note, errorDescription);
        return;
    }

    bool res = m_localStorageManager.LinkTagWithNote(*tag, *note, errorDescription);
    if (!res) {
        emit linkTagWithNoteFailed(tag, note, errorDescription);
        return;
    }

    emit linkTagWithNoteComplete(tag, note);
}

void LocalStorageManagerThreadWorker::onFindTagRequest(QSharedPointer<Tag> tag)
{
    QString errorDescription;

    if (tag.isNull()) {
        errorDescription = "Internal error: detected null pointer to tag "
                           "on attempt to find tag in local storage";
        QNCRITICAL(errorDescription);
        QNTRANSLATE(errorDescription);
        emit findTagFailed(tag, errorDescription);
        return;
    }

    bool res = m_localStorageManager.FindTag(*tag, errorDescription);
    if (!res) {
        emit findTagFailed(tag, errorDescription);
        return;
    }

    emit findTagComplete(tag);
}

void LocalStorageManagerThreadWorker::onListAllTagsPerNoteRequest(QSharedPointer<Note> note)
{
    QString errorDescription;

    if (note.isNull()) {
        errorDescription = "Internal error: detected null pointer to note "
                           "on attempt to list all tags per note in local storage";
        QNCRITICAL(errorDescription);
        QNTRANSLATE(errorDescription);
        emit listAllTagsPerNoteFailed(note, errorDescription);
        return;
    }

    QList<Tag> tags = m_localStorageManager.ListAllTagsPerNote(*note, errorDescription);
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

void LocalStorageManagerThreadWorker::onDeleteTagRequest(QSharedPointer<Tag> tag)
{
    QString errorDescription;

    if (tag.isNull()) {
        errorDescription = "Internal error: detected null pointet to tag "
                           "on attempt to delete tag in local storage";
        QNCRITICAL(errorDescription);
        QNTRANSLATE(errorDescription);
        emit deleteTagFailed(tag, errorDescription);
        return;
    }

    bool res = m_localStorageManager.DeleteTag(*tag, errorDescription);
    if (!res) {
        emit deleteTagFailed(tag, errorDescription);
        return;
    }

    emit deleteTagComplete(tag);
}

void LocalStorageManagerThreadWorker::onExpungeTagRequest(QSharedPointer<Tag> tag)
{
    QString errorDescription;

    if (tag.isNull()) {
        errorDescription = "Internal error: detected null pointer to tag "
                           "on attempt to expunge tag from local storage";
        QNCRITICAL(errorDescription);
        QNTRANSLATE(errorDescription);
        emit expungeTagFailed(tag, errorDescription);
        return;
    }

    bool res = m_localStorageManager.ExpungeTag(*tag, errorDescription);
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

void LocalStorageManagerThreadWorker::onAddResourceRequest(QSharedPointer<IResource> resource, QSharedPointer<Note> note)
{
    QString errorDescription;

    if (resource.isNull()) {
        errorDescription = "Internal error: detected null pointer to resource "
                           "on attempt to add resource to local storage";
        QNCRITICAL(errorDescription);
        QNTRANSLATE(errorDescription);
        emit addResourceFailed(resource, note, errorDescription);
        return;
    }

    if (note.isNull()) {
        errorDescription = "Internal error: detected null pointer to note "
                           "on attempt to add resource to local storage";
        QNCRITICAL(errorDescription);
        QNTRANSLATE(errorDescription);
        emit addResourceFailed(resource, note, errorDescription);
        return;
    }

    bool res = m_localStorageManager.AddEnResource(*resource, *note, errorDescription);
    if (!res) {
        emit addResourceFailed(resource, note, errorDescription);
        return;
    }

    emit addResourceComplete(resource, note);
}

void LocalStorageManagerThreadWorker::onUpdateResourceRequest(QSharedPointer<IResource> resource, QSharedPointer<Note> note)
{
    QString errorDescription;

    if (resource.isNull()) {
        errorDescription = "Internal error: detected null pointer to resource "
                           "on attempt to update resource in local storage";
        QNCRITICAL(errorDescription);
        QNTRANSLATE(errorDescription);
        emit updateResourceFailed(resource, note, errorDescription);
        return;
    }

    if (note.isNull()) {
        errorDescription = "Internal error: detected null pointer to note "
                           "on attempt to update resource in local storage";
        QNCRITICAL(errorDescription);
        QNTRANSLATE(errorDescription);
        emit updateResourceFailed(resource, note, errorDescription);
        return;
    }

    bool res = m_localStorageManager.UpdateEnResource(*resource, *note, errorDescription);
    if (!res) {
        emit updateResourceFailed(resource, note, errorDescription);
        return;
    }

    emit updateResourceComplete(resource, note);
}

void LocalStorageManagerThreadWorker::onFindResourceRequest(QSharedPointer<IResource> resource, bool withBinaryData)
{
    QString errorDescription;

    if (resource.isNull()) {
        errorDescription = "Internal error: detected null pointer to resource "
                           "on attempt to find resource in local storage";
        QNCRITICAL(errorDescription);
        QNTRANSLATE(errorDescription);
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

void LocalStorageManagerThreadWorker::onExpungeResourceRequest(QSharedPointer<IResource> resource)
{
    QString errorDescription;

    if (resource.isNull()) {
        errorDescription = "Internal error: detected null pointer to resource "
                           "on attempt to expunge resource from local storage";
        QNCRITICAL(errorDescription);
        QNTRANSLATE(errorDescription);
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

void LocalStorageManagerThreadWorker::onAddSavedSearchRequest(QSharedPointer<SavedSearch> search)
{
    QString errorDescription;

    if (search.isNull()) {
        errorDescription = "Internal error: detected null pointer to saved search "
                           "on attempt to add saved search to local storage";
        QNCRITICAL(errorDescription);
        QNTRANSLATE(errorDescription);
        emit addSavedSearchFailed(search, errorDescription);
        return;
    }

    bool res = m_localStorageManager.AddSavedSearch(*search, errorDescription);
    if (!res) {
        emit addSavedSearchFailed(search, errorDescription);
        return;
    }

    emit addSavedSearchComplete(search);
}

void LocalStorageManagerThreadWorker::onUpdateSavedSearchRequest(QSharedPointer<SavedSearch> search)
{
    QString errorDescription;

    if (search.isNull()) {
        errorDescription = "Internal error: detected null pointer to saved search "
                           "on attempt to update saved search in local storage";
        QNCRITICAL(errorDescription);
        QNTRANSLATE(errorDescription);
        emit updateSavedSearchFailed(search, errorDescription);
        return;
    }

    bool res = m_localStorageManager.UpdateSavedSearch(*search, errorDescription);
    if (!res) {
        emit updateSavedSearchFailed(search, errorDescription);
        return;
    }

    emit updateSavedSearchComplete(search);
}

void LocalStorageManagerThreadWorker::onFindSavedSearchRequest(QSharedPointer<SavedSearch> search)
{
    QString errorDescription;

    if (search.isNull()) {
        errorDescription = "Internal error: detected null pointer to saved search "
                           "on attempt to find saved search in local storage";
        QNCRITICAL(errorDescription);
        QNTRANSLATE(errorDescription);
        emit findSavedSearchFailed(search, errorDescription);
        return;
    }

    bool res = m_localStorageManager.FindSavedSearch(*search, errorDescription);
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

void LocalStorageManagerThreadWorker::onExpungeSavedSearch(QSharedPointer<SavedSearch> search)
{
    QString errorDescription;

    if (search.isNull()) {
        errorDescription = "Internal error: detected null pointer to saved search "
                           "on attempt to expunge saved search from local storage";
        QNCRITICAL(errorDescription);
        QNTRANSLATE(errorDescription);
        emit expungeSavedSearchFailed(search, errorDescription);
        return;
    }

    bool res = m_localStorageManager.ExpungeSavedSearch(*search, errorDescription);
    if (!res) {
        emit expungeSavedSearchFailed(search, errorDescription);
        return;
    }

    emit expungeSavedSearchComplete(search);
}

} // namespace qute_note

#undef QNTRANSLATE
