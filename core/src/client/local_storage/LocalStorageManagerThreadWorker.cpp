#include "LocalStorageManagerThreadWorker.h"
#include <client/types/Notebook.h>
#include <client/types/SharedNotebookWrapper.h>
#include <client/types/LinkedNotebook.h>
#include <client/types/Note.h>
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

void LocalStorageManagerThreadWorker::onAddNotebookRequest(QSharedPointer<Notebook> notebook)
{
    QString errorDescription;

    if (notebook.isNull()) {
        errorDescription = QObject::tr("Internal error: detected null pointer "
                                       "on attempt to add notebook to local storage");
        QNCRITICAL(errorDescription);
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
        errorDescription = QObject::tr("Internal error: detected null pointer "
                                       "on attempt to update notebook in local storage");
        QNCRITICAL(errorDescription);
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
        errorDescription = QObject::tr("Internal error: detected null pointer "
                                       "on attempt to find notebook in local storage");
        QNCRITICAL(errorDescription);
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
        errorDescription = QObject::tr("Internal error: detected null pointer "
                                       "on attempt to expunge notebook from local storage");
        QNCRITICAL(errorDescription);
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

void LocalStorageManagerThreadWorker::onAddLinkedNotebookRequest(QSharedPointer<LinkedNotebook> linkedNotebook)
{
    QString errorDescription;

    if (linkedNotebook.isNull()) {
        errorDescription = QObject::tr("Internal error: detected null pointer "
                                       "on attempt to add linked notebook to local storage");
        QNCRITICAL(errorDescription);
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
        errorDescription = QObject::tr("Internal error: detected null pointer "
                                       "on attempt to update linked notebook in local storage");
        QNCRITICAL(errorDescription);
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
        errorDescription = QObject::tr("Internal error: detected null pointer "
                                       "on attempt to find linked notebook in local storage");
        QNCRITICAL(errorDescription);
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
        errorDescription = QObject::tr("Internal error: detected null pointer "
                                       "on attempt to expunge linked notebook from local storage");
        QNCRITICAL(errorDescription);
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

void LocalStorageManagerThreadWorker::onAddNoteRequest(QSharedPointer<Note> note, QSharedPointer<Notebook> notebook)
{
    QString errorDescription;

    if (note.isNull()) {
        errorDescription = QObject::tr("Internal error: detected null pointer to note "
                                       "on attempt to add note to local storage");
        QNCRITICAL(errorDescription);
        emit addNoteFailed(note, notebook, errorDescription);
        return;
    }

    if (notebook.isNull()) {
        errorDescription = QObject::tr("Internal error: detected null pointer to notebook "
                                       "on attempt to add note to local storage");
        QNCRITICAL(errorDescription);
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
        errorDescription = QObject::tr("Internal error: detected null pointer to note "
                                       "on attempt to update note in local storage");
        QNCRITICAL(errorDescription);
        emit updateNoteFailed(note, notebook, errorDescription);
        return;
    }

    if (notebook.isNull()) {
        errorDescription = QObject::tr("Internal error: detected null pointer to notebook "
                                       "on attempt to update note in local storage");
        QNCRITICAL(errorDescription);
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
        errorDescription = QObject::tr("Internal error: detected null pointer to note "
                                       "on attempt to find note in local storage");
        QNCRITICAL(errorDescription);
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
        errorDescription = QObject::tr("Internal error: detected null pointer to notebook "
                                       "on attempt to list notes per notebook in local storage");
        QNCRITICAL(errorDescription);
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
        errorDescription = QObject::tr("Internal error: detected null pointer to note "
                                       "on attempt to delete note from local storage");
        QNCRITICAL(errorDescription);
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
        errorDescription = QObject::tr("Internal error: detected null pointer to note "
                                       "on attempt to expunge note from local storage");
        QNCRITICAL(errorDescription);
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

} // namespace qute_note
