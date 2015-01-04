#ifndef __QUTE_NOTE__CORE__CLIENT__SYNCHRONIZATION__SEND_LOCAL_CHANGES_MANAGER_H
#define __QUTE_NOTE__CORE__CLIENT__SYNCHRONIZATION__SEND_LOCAL_CHANGES_MANAGER_H

#include "NoteStore.h"
#include <client/local_storage/LocalStorageManager.h>
#include <client/types/Tag.h>
#include <client/types/SavedSearch.h>
#include <client/types/Notebook.h>
#include <client/types/Note.h>
#include <QObject>

namespace qute_note {

QT_FORWARD_DECLARE_CLASS(LocalStorageManagerThreadWorker)

class SendLocalChangesManager: public QObject
{
    Q_OBJECT
public:
    explicit SendLocalChangesManager(LocalStorageManagerThreadWorker & localStorageManagerThreadWorker,
                                     QSharedPointer<qevercloud::NoteStore> pNoteStore,
                                     QObject * parent = nullptr);

Q_SIGNALS:
    void failure(QString errorDescription);
    void finished();
    void rateLimitExceeded(qint32 secondsToWait);

public Q_SLOTS:
    void start();

// private signals:
Q_SIGNALS:
    // signals to request dirty & not yet synchronized objects from local storage
    void requestLocalUnsynchronizedTags(LocalStorageManager::ListObjectsOptions flag,
                                        size_t limit, size_t offset,
                                        LocalStorageManager::ListTagsOrder::type order,
                                        LocalStorageManager::OrderDirection::type orderDirection,
                                        QUuid requestId);
    void requestLocalUnsynchronizedSavedSearches(LocalStorageManager::ListObjectsOptions flag,
                                                 size_t limit, size_t offset,
                                                 LocalStorageManager::ListSavedSearchesOrder::type order,
                                                 LocalStorageManager::OrderDirection::type orderDirection,
                                                 QUuid requestId);
    void requestLocalUnsynchronizedNotebooks(LocalStorageManager::ListObjectsOptions flag,
                                             size_t limit, size_t offset,
                                             LocalStorageManager::ListNotebooksOrder::type order,
                                             LocalStorageManager::OrderDirection::type orderDirection,
                                             QUuid requestId);
    void requestLocalUnsynchronizedNotes(LocalStorageManager::ListObjectsOptions flag,
                                         size_t limit, size_t offset,
                                         LocalStorageManager::ListNotesOrder::type order,
                                         LocalStorageManager::OrderDirection::type orderDirection,
                                         QUuid requestId);

private Q_SLOTS:
    void onListDirtyTagsCompleted(LocalStorageManager::ListObjectsOptions flag,
                                  size_t limit, size_t offset,
                                  LocalStorageManager::ListTagsOrder::type order,
                                  LocalStorageManager::OrderDirection::type orderDirection,
                                  QList<Tag> tags, QUuid requestId);
    void onListDirtyTagsFailed(LocalStorageManager::ListObjectsOptions flag,
                               size_t limit, size_t offset,
                               LocalStorageManager::ListTagsOrder::type order,
                               LocalStorageManager::OrderDirection::type orderDirection,
                               QString errorDescription, QUuid requestId);

    void onListDirtySavedSearchesCompleted(LocalStorageManager::ListObjectsOptions flag,
                                           size_t limit, size_t offset,
                                           LocalStorageManager::ListSavedSearchesOrder::type order,
                                           LocalStorageManager::OrderDirection::type orderDirection,
                                           QList<SavedSearch> savedSearches, QUuid requestId);
    void onListDirtySavedSearchesFailed(LocalStorageManager::ListObjectsOptions flag,
                                        size_t limit, size_t offset,
                                        LocalStorageManager::ListSavedSearchesOrder::type order,
                                        LocalStorageManager::OrderDirection::type orderDirection,
                                        QString errorDescription, QUuid requestId);

    void onListDirtyNotebooksCompleted(LocalStorageManager::ListObjectsOptions flag,
                                       size_t limit, size_t offset,
                                       LocalStorageManager::ListNotebooksOrder::type order,
                                       LocalStorageManager::OrderDirection::type orderDirection,
                                       QList<Notebook> notebooks, QUuid requestId);
    void onListDirtyNotebooksFailed(LocalStorageManager::ListObjectsOptions flag,
                                    size_t limit, size_t offset,
                                    LocalStorageManager::ListNotebooksOrder::type order,
                                    LocalStorageManager::OrderDirection::type orderDirection,
                                    QString errorDescription, QUuid requestId);

    void onListDirtyNotesCompleted(LocalStorageManager::ListObjectsOptions flag,
                                   size_t limit, size_t offset,
                                   LocalStorageManager::ListNotesOrder::type order,
                                   LocalStorageManager::OrderDirection::type orderDirection,
                                   QList<Note> notes, QUuid requestId);
    void onListDirtyNotesFailed(LocalStorageManager::ListObjectsOptions flag,
                                size_t limit, size_t offset,
                                LocalStorageManager::ListNotesOrder::type order,
                                LocalStorageManager::OrderDirection::type orderDirection,
                                QString errorDescription, QUuid requestId);

private:
    SendLocalChangesManager() Q_DECL_DELETE;

private:
    void createConnections();

private:
    LocalStorageManagerThreadWorker &   m_localStorageManagerThreadWorker;
    NoteStore                           m_noteStore;

    QUuid                               m_listDirtyTagsRequestId;
    QUuid                               m_listDirtySavedSearchesRequestId;
    QUuid                               m_listDirtyNotebooksRequestId;
    QUuid                               m_listDirtyNotesRequestId;
};

} // namespace qute_note

#endif // __QUTE_NOTE__CORE__CLIENT__SYNCHRONIZATION__SEND_LOCAL_CHANGES_MANAGER_H
