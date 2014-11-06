#ifndef __QUTE_NOTE__CORE__CLIENT__SYNCHRONIZATION__FULL_SYNCHRONIZATION_MANAGER_H
#define __QUTE_NOTE__CORE__CLIENT__SYNCHRONIZATION__FULL_SYNCHRONIZATION_MANAGER_H

#include <client/types/UserWrapper.h>
#include <client/types/Notebook.h>
#include <client/types/Note.h>
#include <client/types/Tag.h>
#include <client/types/ResourceWrapper.h>
#include <client/types/LinkedNotebook.h>
#include <client/types/SavedSearch.h>
#include <QEverCloud.h>
#include <oauth.h>
#include <QSharedPointer>

namespace qute_note {

QT_FORWARD_DECLARE_CLASS(LocalStorageManagerThreadWorker)

class FullSynchronizationManager: public QObject
{
    Q_OBJECT
public:
     explicit FullSynchronizationManager(LocalStorageManagerThreadWorker & localStorageManagerThreadWorker,
                                         QSharedPointer<qevercloud::NoteStore> pNoteStore,
                                         QSharedPointer<qevercloud::EvernoteOAuthWebView::OAuthResult> pOAuthResult,
                                         QObject * parent = nullptr);

Q_SIGNALS:
    void error(QString errorDescription);
    void finished();

public Q_SLOTS:
    void start();

// private signals
Q_SIGNALS:
    void addUser(UserWrapper user);
    void updateUser(UserWrapper user);
    void findUser(UserWrapper user);
    void deleteUser(UserWrapper user);
    void expungeUser(UserWrapper user);

    void addNotebook(Notebook notebook);
    void updateNotebook(Notebook notebook);
    void findNotebook(Notebook notebook);
    void expungeNotebook(Notebook notebook);

    void addNote(Note note, Notebook notebook);
    void updateNote(Note note, Notebook notebook);
    void findNote(Note note, bool withResourceBinaryData);
    void deleteNote(Note note);
    void expungeNote(Note note);

    void addTag(Tag tag);
    void updateTag(Tag tag);
    void findTag(Tag tag);
    void deleteTag(Tag tag);
    void expungeTag(Tag tag);

    void addResource(ResourceWrapper resource, Note note);
    void updateResource(ResourceWrapper resource, Note note);
    void findResource(ResourceWrapper resource, bool withBinaryData);
    void expungeResource(ResourceWrapper resource);

    void addLinkedNotebook(LinkedNotebook notebook);
    void updateLinkedNotebook(LinkedNotebook notebook);
    void findLinkedNotebook(LinkedNotebook linkedNotebook);
    void expungeLinkedNotebook(LinkedNotebook notebook);

    void addSavedSearch(SavedSearch savedSearch);
    void updateSavedSearch(SavedSearch savedSearch);
    void findSavedSearch(SavedSearch savedSearch);
    void expungeSavedSearch(SavedSearch savedSearch);

private Q_SLOTS:
    void onFindUserCompleted(UserWrapper user);
    void onFindUserFailed(UserWrapper user);
    void onFindNotebookCompleted(Notebook notebook);
    void onFindNotebookFailed(Notebook notebook);
    void onFindNoteCompleted(Note note);
    void onFindNoteFailed(Note note);
    void onFindTagCompleted(Tag tag);
    void onFindTagFailed(Tag tag);
    void onFindResourceCompleted(ResourceWrapper resource);
    void onFindResourceFailed(ResourceWrapper resource);
    void onFindLinkedNotebookCompleted(LinkedNotebook linkedNotebook);
    void onFindLinkedNotebookFailed(LinkedNotebook linkedNotebook);
    void onFindSavedSearchCompleted(SavedSearch savedSearch);
    void onFindSavedSearchFailed(SavedSearch savedSearch);

private:
    void createConnections();

    void launchTagsSync();

private:
    FullSynchronizationManager() Q_DECL_DELETE;

private:
    LocalStorageManagerThreadWorker &                               m_localStorageManagerThreadWorker;
    QSharedPointer<qevercloud::NoteStore>                           m_pNoteStore;
    QSharedPointer<qevercloud::EvernoteOAuthWebView::OAuthResult>   m_pOAuthResult;
    qint32                                                          m_maxSyncChunkEntries;

    QVector<qevercloud::SyncChunk>  m_syncChunks;
    QList<qevercloud::Tag>          m_tags;
};
} // namespace qute_note

#endif // __QUTE_NOTE__CORE__CLIENT__SYNCHRONIZATION__FULL_SYNCHRONIZATION_MANAGER_H
