#ifndef __QUTE_NOTE__CLIENT__LOCAL_STORAGE__I_ASYNC_LOCAL_STORAGE_MANAGER_H
#define __QUTE_NOTE__CLIENT__LOCAL_STORAGE__I_ASYNC_LOCAL_STORAGE_MANAGER_H

#include <tools/Linkage.h>
#include <QString>
#include <QSharedPointer>

namespace qute_note {

QT_FORWARD_DECLARE_CLASS(IUser)
QT_FORWARD_DECLARE_CLASS(Notebook)
QT_FORWARD_DECLARE_CLASS(SharedNotebookWrapper)
QT_FORWARD_DECLARE_CLASS(LinkedNotebook)

/**
 * @brief The IAsyncLocalStorageManager class defines the interfaces for signals and slots
 * used for asynchronous access to local storage database. Typically it is a couple of signals
 * "complete"/"failed" for each public method in LocalStorageManager. In order to be able
 * to identify objects for which the operation succeeded or failed, guids are passed
 * by the signals. For objects which may have both local and remote guids set the passed guid
 * is remote guid if it was passed inside the object when invoking the slot or local guid otherwise
 */
class IAsyncLocalStorageManager
{
public:
    virtual ~IAsyncLocalStorageManager();

protected:
    IAsyncLocalStorageManager() = default;
    IAsyncLocalStorageManager(const IAsyncLocalStorageManager & other) = default;
    IAsyncLocalStorageManager(IAsyncLocalStorageManager && other) = default;
    IAsyncLocalStorageManager & operator=(const IAsyncLocalStorageManager & other) = default;
    IAsyncLocalStorageManager & operator=(IAsyncLocalStorageManager && other) = default;

    // Pure virtual prototypes for signals to be emitted from subclasses:

    // Prototypes for user-related signals:
    virtual void switchUserComplete(qint32 userId) = 0;
    virtual void switchUserFailed(qint32 userId, QString errorDescription) = 0;

    virtual void addUserComplete(qint32 userId) = 0;
    virtual void addUserFailed(qint32 userId, QString errorDescription) = 0;

    virtual void updateUserComplete(qint32 userId) = 0;
    virtual void updateUserFailed(qint32 userId, QString errorDecription) = 0;

    virtual void findUserComplete(QSharedPointer<IUser> foundUser) = 0;
    virtual void findUserFailed(qint32 userId, QString errorDescription) = 0;

    virtual void deleteUserComplete(qint32 userId) = 0;
    virtual void deleteUserFailed(qint32 userId, QString errorDescription) = 0;

    virtual void expungeUserComplete(qint32 userId) = 0;
    virtual void expungeUserFailed(qint32 userId, QString errorDescription) = 0;

    // Prototypes for notebook-related signals:
    virtual void addNotebookComplete(QString notebookGuid) = 0;
    virtual void addNotebookFailed(QString notebookGuid, QString errorDescription) = 0;

    virtual void updateNotebookComplete(QString notebookGuid) = 0;
    virtual void updateNotebookFailed(QString notebookGuid, QString errorDescription) = 0;

    virtual void findNotebookComplete(QSharedPointer<Notebook> foundNotebook) = 0;
    virtual void findNotebookFailed(QString notebookGuid, QString errorDescription) = 0;

    virtual void listAllNotebooksComplete(QList<Notebook> foundNotebooks) = 0;
    virtual void listAllNotebooksFailed(QString errorDescription) = 0;

    virtual void listAllSharedNotebooksComplete(QList<SharedNotebookWrapper> foundSharedNotebooks) = 0;
    virtual void listAllSharedNotebooksFailed(QString errorDescription) = 0;

    virtual void expungeNotebookComplete(QString notebookGuid) = 0;
    virtual void expungeNotebookFailed(QString notebookGuid, QString errorDescription) = 0;

    // Prototypes for linked notebook-related signals:
    virtual void addLinkedNotebookComplete(QString linkedNotebookGuid) = 0;
    virtual void addLinkedNotebookFailed(QString linkedNotebookGuid, QString errorDescription) = 0;

    virtual void updateLinkedNotebookComplete(QString linkedNotebookGuid) = 0;
    virtual void updateLinkedNotebookFailed(QString linkedNotebookGuid, QString errorDescription) = 0;

    virtual void findLinkedNotebookCompleted(QSharedPointer<LinkedNotebook> foundLinkedNotebook) = 0;
    virtual void findLinkedNotebookFailed(QString linkedNotebookGuid, QString errorDescription) = 0;

    virtual void listAllLinkedNotebooksComplete(QList<LinkedNotebook> foundLinkedNotebooks) = 0;
    virtual void listAllLinkedNotebooksFailed(QString errorDescription) = 0;

    virtual void expungeLinkedNotebookCompleted(QString linkedNotebookGuid) = 0;
    virtual void expungeLinkedNotebookFailed(QString linkedNotebookGuid, QString errorDescription) = 0;

    // Prototypes for note-related signals:
    virtual void addNoteComplete(QString noteGuid) = 0;
    virtual void addNoteFailed(QString noteGuid, QString errorDescription) = 0;

    virtual void updateNoteComplete(QString noteGuid) = 0;
    virtual void updateNoteFailed(QString noteGuid, QString errorDescription) = 0;

    virtual void findNoteComplete(QSharedPointer<Note> foundNote) = 0;
    virtual void findNoteFailed(QString noteGuid, QString errorDescription) = 0;

    virtual void listAllNotesPerNotebookComplete(QString notebookGuid, QList<Note> foundNotes) = 0;
    virtual void listAllNotesPerNotebookFailed(QString notebookGuid, QString errorDescription) = 0;

    virtual void deleteNoteComplete(QString noteGuid) = 0;
    virtual void deleteNoteFailed(QString noteGuid, QString errorDescription) = 0;

    virtual void expungeNoteComplete(QString noteGuid) = 0;
    virtual void expungeNoteFailed(QString noteGuid, QString errorDescription) = 0;

    // TODO: continue from here. Prototypes for signals for Tag, Resource, SavedSearch
};

}

#endif // __QUTE_NOTE__CLIENT__LOCAL_STORAGE__I_ASYNC_LOCAL_STORAGE_MANAGER_H
