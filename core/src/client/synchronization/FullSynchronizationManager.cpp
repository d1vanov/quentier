#include "FullSynchronizationManager.h"
#include <client/local_storage/LocalStorageManagerThreadWorker.h>
#include <tools/QuteNoteCheckPtr.h>
#include <logging/QuteNoteLogger.h>
#include <algorithm>

namespace qute_note {

FullSynchronizationManager::FullSynchronizationManager(LocalStorageManagerThreadWorker & localStorageManagerThreadWorker,
                                                       QSharedPointer<qevercloud::NoteStore> pNoteStore,
                                                       QSharedPointer<qevercloud::EvernoteOAuthWebView::OAuthResult> pOAuthResult,
                                                       QObject * parent) :
    QObject(parent),
    m_localStorageManagerThreadWorker(localStorageManagerThreadWorker),
    m_pNoteStore(pNoteStore),
    m_pOAuthResult(pOAuthResult),
    m_maxSyncChunkEntries(50),
    m_syncChunks(),
    m_tags(),
    m_tagsToAddPerRequestId(),
    m_findTagByNameRequestIds(),
    m_findTagByGuidRequestIds(),
    m_addTagRequestIds(),
    m_updateTagRequestIds(),
    m_savedSearches(),
    m_savedSearchesToAddPerRequestId(),
    m_findSavedSearchByNameRequestIds(),
    m_findSavedSearchByGuidRequestIds(),
    m_addSavedSearchRequestIds(),
    m_updateSavedSearchRequestIds(),
    m_linkedNotebooks(),
    m_findLinkedNotebookRequestIds(),
    m_addLinkedNotebookRequestIds(),
    m_updateLinkedNotebookRequestIds(),
    m_notebooks(),
    m_notebooksToAddPerRequestId(),
    m_findNotebookByNameRequestIds(),
    m_findNotebookByGuidRequestIds(),
    m_addNotebookRequestIds(),
    m_updateNotebookRequestIds(),
    m_notes(),
    m_findNoteByGuidRequestIds(),
    m_addNoteRequestIds(),
    m_updateNoteRequestIds(),
    m_notesWithFindRequestIdsPerFindNotebookRequestId(),
    m_notebooksPerNoteGuids(),
    m_localGuidsOfElementsAlreadyAttemptedToFindByName(),
    m_notesToAddPerAPICallPostponeTimerId(),
    m_notesToUpdatePerAPICallPostponeTimerId(),
    m_listDirtyTagsRequestId(),
    m_listDirtySavedSearchesRequestId(),
    m_listDirtyNotebooksRequestId(),
    m_listDirtyNotesRequestId()
{
    createConnections();
}

void FullSynchronizationManager::start()
{
    QNDEBUG("FullSynchronizationManager::start");

    QUTE_NOTE_CHECK_PTR(m_pNoteStore.data());
    QUTE_NOTE_CHECK_PTR(m_pOAuthResult.data());

    clear();

    qint32 afterUsn = 0;
    qevercloud::SyncChunk * pSyncChunk = nullptr;

    QNDEBUG("Downloading sync chunks:");

    while(!pSyncChunk || (pSyncChunk->chunkHighUSN < pSyncChunk->updateCount))
    {
        if (pSyncChunk) {
            afterUsn = pSyncChunk->chunkHighUSN;
        }

        m_syncChunks.push_back(qevercloud::SyncChunk());
        pSyncChunk = &(m_syncChunks.back());

        try {
            *pSyncChunk = m_pNoteStore->getSyncChunk(afterUsn, m_maxSyncChunkEntries, true,
                                                     m_pOAuthResult->authenticationToken);
        }
        catch(const qevercloud::EvernoteException & exception)
        {
            QString errorDescription = QT_TR_NOOP("Can't perform full synchronization, "
                                                  "can't download the sync chunks: caught exception: " +
                                                  QString(exception.what()));
            const QSharedPointer<qevercloud::EverCloudExceptionData> exceptionData = exception.exceptionData();
            if (!exceptionData.isNull()) {
                errorDescription += ", " + exceptionData->errorMessage;
            }
            QNWARNING(errorDescription);
            emit failure(errorDescription);
            return;
        }

        QNDEBUG("Received sync chunk: " << *pSyncChunk);
    }

    QNDEBUG("Done. Processing tags from buffered sync chunks");

    launchTagsSync();
    launchSavedSearchSync();
    launchLinkedNotebookSync();
    launchNotebookSync();
}

template <>
void FullSynchronizationManager::emitAddRequest<Tag>(const Tag & tag)
{
    QUuid addTagRequestId = QUuid::createUuid();
    Q_UNUSED(m_addTagRequestIds.insert(addTagRequestId));
    emit addTag(tag, addTagRequestId);
}

template <>
void FullSynchronizationManager::emitAddRequest<SavedSearch>(const SavedSearch & search)
{
    QUuid addSavedSearchRequestId = QUuid::createUuid();
    Q_UNUSED(m_addSavedSearchRequestIds.insert(addSavedSearchRequestId));
    emit addSavedSearch(search, addSavedSearchRequestId);
}

template <>
void FullSynchronizationManager::emitAddRequest<LinkedNotebook>(const LinkedNotebook & linkedNotebook)
{
    QUuid addLinkedNotebookRequestId = QUuid::createUuid();
    Q_UNUSED(m_addLinkedNotebookRequestIds.insert(addLinkedNotebookRequestId));
    emit addLinkedNotebook(linkedNotebook, addLinkedNotebookRequestId);
}

template <>
void FullSynchronizationManager::emitAddRequest<Notebook>(const Notebook & notebook)
{
    QUuid addNotebookRequestId = QUuid::createUuid();
    Q_UNUSED(m_addNotebookRequestIds.insert(addNotebookRequestId));
    emit addNotebook(notebook, addNotebookRequestId);
}

template <>
void FullSynchronizationManager::emitAddRequest<Note>(const Note & note)
{
    QString noteGuid = (note.hasGuid() ? note.guid() : QString());
    QString noteLocalGuid = note.localGuid();
    QPair<QString,QString> key(noteGuid, noteLocalGuid);

    auto it = m_notebooksPerNoteGuids.find(key);
    if (it == m_notebooksPerNoteGuids.end()) {
        QString errorDescription = QT_TR_NOOP("detected attempt to add note for which no notebook was found in local storage");
        QNWARNING(errorDescription << ": " << note);
        emit failure(errorDescription);
        return;
    }

    const Notebook & notebook = it.value();

    QUuid addNoteRequestId = QUuid::createUuid();
    Q_UNUSED(m_addNoteRequestIds.insert(addNoteRequestId));
    emit addNote(note, notebook, addNoteRequestId);
}

template <>
void FullSynchronizationManager::emitUpdateRequest<Tag>(const Tag & tag,
                                                        const Tag * tagToAddLater)
{
    QNDEBUG("FullSynchronizationManager::emitUpdateRequest<Tag>: tag = " << tag
            << ", tagToAddLater = " << (tagToAddLater ? tagToAddLater->ToQString() : "<null>"));

    QUuid updateTagRequestId = QUuid::createUuid();
    Q_UNUSED(m_updateTagRequestIds.insert(updateTagRequestId));

    if (tagToAddLater) {
        m_tagsToAddPerRequestId[updateTagRequestId] = *tagToAddLater;
    }

    emit updateTag(tag, updateTagRequestId);
}

template <>
void FullSynchronizationManager::emitUpdateRequest<SavedSearch>(const SavedSearch & search,
                                                                const SavedSearch * searchToAddLater)
{
    QNDEBUG("FullSynchronizationManager::emitUpdateRequest<SavedSearch>: search = " << search
            << ", searchToAddLater = " << (searchToAddLater ? searchToAddLater->ToQString() : "<null>"));

    QUuid updateSavedSearchRequestId = QUuid::createUuid();
    Q_UNUSED(m_updateSavedSearchRequestIds.insert(updateSavedSearchRequestId));

    if (searchToAddLater) {
        m_savedSearchesToAddPerRequestId[updateSavedSearchRequestId] = *searchToAddLater;
    }

    emit updateSavedSearch(search, updateSavedSearchRequestId);
}

template <>
void FullSynchronizationManager::emitUpdateRequest<LinkedNotebook>(const LinkedNotebook & linkedNotebook,
                                                                   const LinkedNotebook *)
{
    QNDEBUG("FullSynchronizationManager::emitUpdateRequest<LinkedNotebook>: linked notebook = "
            << linkedNotebook);

    QUuid updateLinkedNotebookRequestId = QUuid::createUuid();
    Q_UNUSED(m_updateLinkedNotebookRequestIds.insert(updateLinkedNotebookRequestId));
    emit updateLinkedNotebook(linkedNotebook, updateLinkedNotebookRequestId);
}

template <>
void FullSynchronizationManager::emitUpdateRequest<Notebook>(const Notebook & notebook,
                                                             const Notebook * notebookToAddLater)
{
    QNDEBUG("FullSynchronizationManager::emitUpdateRequest<Notebook>: notebook = " << notebook
            << ", notebook to add later = " << (notebookToAddLater ? notebookToAddLater->ToQString() : "null"));

    QUuid updateNotebookRequestId = QUuid::createUuid();
    Q_UNUSED(m_updateNotebookRequestIds.insert(updateNotebookRequestId));

    if (notebookToAddLater) {
        m_notebooksToAddPerRequestId[updateNotebookRequestId] = *notebookToAddLater;
    }

    emit updateNotebook(notebook, updateNotebookRequestId);
}

template <>
void FullSynchronizationManager::emitUpdateRequest<Note>(const Note & note,
                                                         const Note *)
{
    QNDEBUG("FullSynchronizationManager::emitUpdateRequest<Note>: note = " << note);

    const Notebook * pNotebook = getNotebookPerNote(note);
    if (!pNotebook) {
        QString errorDescription = QT_TR_NOOP("Detected attempt to update note in local storage before its notebook is found");
        QNWARNING(errorDescription << ": " << note);
        emit failure(errorDescription);
        return;
    }

    // Dealing with separate Evernote API call for getting the content of note to be updated
    Note localNote = note;
    qint32 postponeAPICallSeconds = tryToGetFullNoteData(localNote);
    if (postponeAPICallSeconds < 0)
    {
        return;
    }
    else if (postponeAPICallSeconds > 0)
    {
        int timerId = startTimer(postponeAPICallSeconds);
        m_notesToUpdatePerAPICallPostponeTimerId[timerId] = localNote;
        return;
    }

    QUuid updateNoteRequestId = QUuid::createUuid();
    Q_UNUSED(m_updateNoteRequestIds.insert(updateNoteRequestId));

    emit updateNote(localNote, *pNotebook, updateNoteRequestId);
}

void FullSynchronizationManager::onFindUserCompleted(UserWrapper user, QUuid requestId)
{
    // TODO: implement
}

void FullSynchronizationManager::onFindUserFailed(UserWrapper user, QString errorDescription, QUuid requestId)
{
    // TODO: implement
}

void FullSynchronizationManager::onFindNotebookCompleted(Notebook notebook, QUuid requestId)
{
    bool foundByGuid = onFoundDuplicateByGuid(notebook, requestId, "Notebook",
                                              m_notebooks, m_findNotebookByGuidRequestIds);
    if (foundByGuid) {
        return;
    }

    bool foundByName = onFoundDuplicateByName(notebook, requestId, "Notebook",
                                              m_notebooks, m_findNotebookByNameRequestIds);
    if (foundByName) {
        return;
    }

    NoteDataPerFindNotebookRequestId::iterator rit = m_notesWithFindRequestIdsPerFindNotebookRequestId.find(requestId);
    if (rit != m_notesWithFindRequestIdsPerFindNotebookRequestId.end())
    {
        const QPair<Note,QUuid> & noteWithFindRequestId = rit.value();
        const Note & note = noteWithFindRequestId.first;
        const QUuid & findNoteRequestId = noteWithFindRequestId.second;

        QString noteGuid = (note.hasGuid() ? note.guid() : QString());
        QString noteLocalGuid = note.localGuid();

        QPair<QString,QString> key(noteGuid, noteLocalGuid);

        // NOTE: notebook for notes is only required for its pair of guid + local guid,
        // it shouldn't prohibit the creation or update of the notes during the synchronization procedure
        notebook.setCanCreateNotes(true);
        notebook.setCanUpdateNotes(true);

        m_notebooksPerNoteGuids[key] = notebook;

        Q_UNUSED(onFoundDuplicateByGuid(note, findNoteRequestId, "Note", m_notes, m_findNoteByGuidRequestIds));
    }
}

void FullSynchronizationManager::onFindNotebookFailed(Notebook notebook, QString errorDescription, QUuid requestId)
{
    bool failedToFindByGuid = onNoDuplicateByGuid(notebook, requestId, errorDescription, "Notebook",
                                                  m_notebooks, m_findNotebookByGuidRequestIds);
    if (failedToFindByGuid) {
        return;
    }

    bool failedToFindByName = onNoDuplicateByName(notebook, requestId, errorDescription, "Notebook",
                                                  m_notebooks, m_findNotebookByNameRequestIds);
    if (failedToFindByName) {
        return;
    }

    NoteDataPerFindNotebookRequestId::iterator rit = m_notesWithFindRequestIdsPerFindNotebookRequestId.find(requestId);
    if (rit != m_notesWithFindRequestIdsPerFindNotebookRequestId.end())
    {
        QString errorDescription = QT_TR_NOOP("Failed to find notebook in local storage for one of processed notes");
        QNWARNING(errorDescription << ": " << notebook);
        emit failure(errorDescription);
        return;
    }
}

void FullSynchronizationManager::onFindNoteCompleted(Note note, bool withResourceBinaryData, QUuid requestId)
{
    Q_UNUSED(withResourceBinaryData);

    bool foundDuplicate = m_findNoteByGuidRequestIds.contains(requestId);
    if (foundDuplicate)
    {
        // Need to find Notebook corresponding to the note in order to proceed
        if (!note.hasNotebookGuid()) {
            QString errorDescription = QT_TR_NOOP("Found duplicate note in local storage which doesn't have notebook guid set");
            QNWARNING(errorDescription << ": " << note);
            emit failure(errorDescription);
            return;
        }

        QUuid findNotebookPerNoteRequestId = QUuid::createUuid();
        m_notesWithFindRequestIdsPerFindNotebookRequestId[findNotebookPerNoteRequestId] =
                QPair<Note,QUuid>(note, requestId);

        Notebook notebookToFind;
        notebookToFind.unsetLocalGuid();
        notebookToFind.setGuid(note.notebookGuid());

        emit findNotebook(notebookToFind, findNotebookPerNoteRequestId);
        return;
    }
}

void FullSynchronizationManager::onFindNoteFailed(Note note, bool withResourceBinaryData, QString errorDescription, QUuid requestId)
{
    Q_UNUSED(withResourceBinaryData);

    QSet<QUuid>::iterator rit = m_findNoteByGuidRequestIds.find(requestId);
    if (rit != m_findNoteByGuidRequestIds.end())
    {
        QNDEBUG("FullSynchronizationManager::onFindNoteFailed: note = " << note << ", requestId = " << requestId);

        Q_UNUSED(m_findNoteByGuidRequestIds.erase(rit));

        auto it = findItemByGuid(m_notes, note, "Note");
        if (it == m_notes.end()) {
            return;
        }

        // Removing the note from the list of notes waiting for processing
        Q_UNUSED(m_notes.erase(it));

        qint32 postponeAPICallSeconds = tryToGetFullNoteData(note);
        if (postponeAPICallSeconds < 0) {
            return;
        }
        else if (postponeAPICallSeconds > 0) {
            int timerId = startTimer(postponeAPICallSeconds);
            m_notesToAddPerAPICallPostponeTimerId[timerId] = note;
            return;
        }

        // Note duplicates by title are completely allowed, so can add the note as is,
        // without any conflict by title resolution
        emitAddRequest(note);
    }
}

void FullSynchronizationManager::onFindTagCompleted(Tag tag, QUuid requestId)
{
    bool foundByGuid = onFoundDuplicateByGuid(tag, requestId, "Tag", m_tags, m_findTagByGuidRequestIds);
    if (foundByGuid) {
        return;
    }

    bool foundByName = onFoundDuplicateByName(tag, requestId, "Tag", m_tags, m_findTagByNameRequestIds);
    if (foundByName) {
        return;
    }
}

void FullSynchronizationManager::onFindTagFailed(Tag tag, QString errorDescription, QUuid requestId)
{
    bool failedToFindByGuid = onNoDuplicateByGuid(tag, requestId, errorDescription, "Tag",
                                                  m_tags, m_findTagByGuidRequestIds);
    if (failedToFindByGuid) {
        return;
    }

    bool failedToFindByName = onNoDuplicateByName(tag, requestId, errorDescription, "Tag",
                                                  m_tags, m_findTagByNameRequestIds);
    if (failedToFindByName) {
        return;
    }
}

void FullSynchronizationManager::onFindResourceCompleted(ResourceWrapper resource, bool withResourceBinaryData, QUuid requestId)
{
    // TODO: implement
}

void FullSynchronizationManager::onFindResourceFailed(ResourceWrapper resource, bool withResourceBinaryData, QString errorDescription, QUuid requestId)
{
    // TODO: implement
}

void FullSynchronizationManager::onFindLinkedNotebookCompleted(LinkedNotebook linkedNotebook, QUuid requestId)
{
    Q_UNUSED(onFoundDuplicateByGuid(linkedNotebook, requestId, "LinkedNotebook",
                                    m_linkedNotebooks, m_findLinkedNotebookRequestIds));
}

void FullSynchronizationManager::onFindLinkedNotebookFailed(LinkedNotebook linkedNotebook,
                                                            QString errorDescription, QUuid requestId)
{
    QSet<QUuid>::iterator rit = m_findLinkedNotebookRequestIds.find(requestId);
    if (rit == m_findLinkedNotebookRequestIds.end()) {
        return;
    }

    QNDEBUG("FullSynchronizationManager::onFindLinkedNotebookFailed: " << linkedNotebook
            << ", errorDescription = " << errorDescription << ", requestId = " << requestId);

    Q_UNUSED(m_findLinkedNotebookRequestIds.erase(rit));

    LinkedNotebooksList::iterator it = findItemByGuid(m_linkedNotebooks, linkedNotebook, "LinkedNotebook");
    if (it == m_linkedNotebooks.end()) {
        return;
    }

    // This linked notebook was not found in the local storage by guid, adding it there
    emitAddRequest(linkedNotebook);
}

void FullSynchronizationManager::onFindSavedSearchCompleted(SavedSearch savedSearch, QUuid requestId)
{
    bool foundByGuid = onFoundDuplicateByGuid(savedSearch, requestId, "SavedSearch",
                                              m_savedSearches, m_findSavedSearchByGuidRequestIds);
    if (foundByGuid) {
        return;
    }

    bool foundByName = onFoundDuplicateByName(savedSearch, requestId, "SavedSearch",
                                              m_savedSearches, m_findSavedSearchByNameRequestIds);
    if (foundByName) {
        return;
    }
}

void FullSynchronizationManager::onFindSavedSearchFailed(SavedSearch savedSearch, QString errorDescription, QUuid requestId)
{
    bool failedToFindByGuid = onNoDuplicateByGuid(savedSearch, requestId, errorDescription, "SavedSearch",
                                                  m_savedSearches, m_findSavedSearchByGuidRequestIds);
    if (failedToFindByGuid) {
        return;
    }

    bool failedToFindByName = onNoDuplicateByName(savedSearch, requestId, errorDescription, "SavedSearch",
                                                  m_savedSearches, m_findSavedSearchByNameRequestIds);
    if (failedToFindByName) {
        return;
    }
}

template <class ElementType>
void FullSynchronizationManager::onAddDataElementCompleted(const ElementType & element,
                                                           const QUuid & requestId,
                                                           const QString & typeName,
                                                           QSet<QUuid> & addElementRequestIds)
{
    QSet<QUuid>::iterator it = addElementRequestIds.find(requestId);
    if (it != addElementRequestIds.end())
    {
        QNDEBUG("FullSynchronizationManager::onAddDataElementCompleted<" << typeName
                << ">: " << typeName << " = " << element << ", requestId = " << requestId);

        Q_UNUSED(addElementRequestIds.erase(it));

        performPostAddOrUpdateChecks<ElementType>();

        checkServerDataMergeCompletion();
    }
}

void FullSynchronizationManager::onAddTagCompleted(Tag tag, QUuid requestId)
{
    onAddDataElementCompleted(tag, requestId, "Tag", m_addTagRequestIds);
}

void FullSynchronizationManager::onAddSavedSearchCompleted(SavedSearch search, QUuid requestId)
{
    onAddDataElementCompleted(search, requestId, "SavedSearch", m_addSavedSearchRequestIds);
}

template <class ElementType>
void FullSynchronizationManager::onAddDataElementFailed(const ElementType & element, const QUuid & requestId,
                                                        const QString & errorDescription, const QString & typeName,
                                                        QSet<QUuid> & addElementRequestIds)
{
    QSet<QUuid>::iterator it = addElementRequestIds.find(requestId);
    if (it != addElementRequestIds.end())
    {
        QNWARNING("FullSyncronizationManager::onAddDataElementFailed<" << typeName
                  << ">: " << typeName << " = " << element << ", error description = "
                  << errorDescription << ", requestId = " << requestId);

        Q_UNUSED(addElementRequestIds.erase(it));

        QString error = QT_TR_NOOP("Can't add remote " + typeName + " to local storage: ");
        error += errorDescription;
        emit failure(error);
    }
}

void FullSynchronizationManager::onAddTagFailed(Tag tag, QString errorDescription, QUuid requestId)
{
    onAddDataElementFailed(tag, requestId, errorDescription, "Tag", m_addTagRequestIds);
}

void FullSynchronizationManager::onAddSavedSearchFailed(SavedSearch search, QString errorDescription, QUuid requestId)
{
    onAddDataElementFailed(search, requestId, errorDescription, "SavedSearch", m_addSavedSearchRequestIds);
}

template <class ElementType, class ElementsToAddByUuid>
void FullSynchronizationManager::onUpdateDataElementCompleted(const ElementType & element,
                                                              const QUuid & requestId,
                                                              const QString & typeName,
                                                              QSet<QUuid> & updateElementRequestIds,
                                                              ElementsToAddByUuid & elementsToAddByRenameRequestId)
{
    QSet<QUuid>::iterator rit = updateElementRequestIds.find(requestId);
    if (rit == updateElementRequestIds.end()) {
        return;
    }

    QNDEBUG("FullSynchronizartionManager::onUpdateDataElementCompleted<" << typeName
            << ">: " << typeName << " = " << element << ", requestId = " << requestId);

    Q_UNUSED(updateElementRequestIds.erase(rit));

    typename ElementsToAddByUuid::iterator addIt = elementsToAddByRenameRequestId.find(requestId);
    if (addIt != elementsToAddByRenameRequestId.end())
    {
        ElementType & elementToAdd = addIt.value();
        const QString localGuid = elementToAdd.localGuid();
        if (localGuid.isEmpty()) {
            QString errorDescription = QT_TR_NOOP("detected " + typeName + " from local storage "
                                                  "with empty local guid");
            QNWARNING(errorDescription << ": " << elementToAdd);
            emit failure(errorDescription);
            return;
        }

        QSet<QString>::iterator git = m_localGuidsOfElementsAlreadyAttemptedToFindByName.find(localGuid);
        if (git == m_localGuidsOfElementsAlreadyAttemptedToFindByName.end())
        {
            QNDEBUG("Checking whether " << typeName << " with the same name already exists in local storage");
            Q_UNUSED(m_localGuidsOfElementsAlreadyAttemptedToFindByName.insert(localGuid));

            elementToAdd.unsetLocalGuid();
            elementToAdd.setGuid("");
            emitFindByNameRequest(elementToAdd);
        }
        else
        {
            QNDEBUG("Adding " << typeName << " to local storage");
            emitAddRequest(elementToAdd);
        }

        Q_UNUSED(elementsToAddByRenameRequestId.erase(addIt));
    }
    else {
        performPostAddOrUpdateChecks<ElementType>();
    }

    checkServerDataMergeCompletion();
}

void FullSynchronizationManager::onUpdateTagCompleted(Tag tag, QUuid requestId)
{
    onUpdateDataElementCompleted(tag, requestId, "Tag", m_updateTagRequestIds, m_tagsToAddPerRequestId);
}

void FullSynchronizationManager::onUpdateSavedSearchCompleted(SavedSearch search, QUuid requestId)
{
    onUpdateDataElementCompleted(search, requestId, "SavedSearch", m_updateSavedSearchRequestIds,
                                 m_savedSearchesToAddPerRequestId);
}

template <class ElementType, class ElementsToAddByUuid>
void FullSynchronizationManager::onUpdateDataElementFailed(const ElementType & element, const QUuid & requestId,
                                                           const QString & errorDescription, const QString & typeName,
                                                           QSet<QUuid> & updateElementRequestIds,
                                                           ElementsToAddByUuid & elementsToAddByRenameRequestId)
{
    QSet<QUuid>::iterator it = updateElementRequestIds.find(requestId);
    if (it == updateElementRequestIds.end()) {
        return;
    }

    QNWARNING("FullSynchronizationManager::onUpdateDataElementFailed<" << typeName
              << ">: " << typeName << " = " << element << ", errorDescription = "
              << errorDescription << ", requestId = " << requestId);

    Q_UNUSED(updateElementRequestIds.erase(it));

    typename ElementsToAddByUuid::iterator addIt = elementsToAddByRenameRequestId.find(requestId);
    if (addIt != elementsToAddByRenameRequestId.end())
    {
        Q_UNUSED(elementsToAddByRenameRequestId.erase(addIt));

        QString error = QT_TR_NOOP("Can't rename local dirty duplicate " + typeName +
                                   " in local storage: ");
        error += errorDescription;
        emit failure(error);
    }
    else
    {
        QString error = QT_TR_NOOP("Can't update remote " + typeName + " in local storage: ");
        error += errorDescription;
        emit failure(error);
    }
}

void FullSynchronizationManager::onUpdateTagFailed(Tag tag, QString errorDescription, QUuid requestId)
{
    onUpdateDataElementFailed(tag, requestId, errorDescription, "Tag", m_updateTagRequestIds,
                              m_tagsToAddPerRequestId);
}

void FullSynchronizationManager::onUpdateSavedSearchFailed(SavedSearch search, QString errorDescription, QUuid requestId)
{
    onUpdateDataElementFailed(search, requestId, errorDescription, "SavedSearch", m_updateSavedSearchRequestIds,
                              m_savedSearchesToAddPerRequestId);
}

template <class ElementType>
void FullSynchronizationManager::performPostAddOrUpdateChecks()
{
    // do nothing
}

template <>
void FullSynchronizationManager::performPostAddOrUpdateChecks<Tag>()
{
    checkNotebooksAndTagsSyncAndLaunchNotesSync();
}

template <>
void FullSynchronizationManager::performPostAddOrUpdateChecks<Notebook>()
{
    checkNotebooksAndTagsSyncAndLaunchNotesSync();
}

template <class ElementType>
void FullSynchronizationManager::unsetLocalGuid(ElementType & element)
{
    element.unsetLocalGuid();
}

template <>
void FullSynchronizationManager::unsetLocalGuid<LinkedNotebook>(LinkedNotebook &)
{
    // do nothing, local guid doesn't make any sense to linked notebook
}

template <>
void FullSynchronizationManager::emitFindByGuidRequest<Tag>(const QString & guid)
{
    Tag tag;
    tag.unsetLocalGuid();
    tag.setGuid(guid);

    QUuid requestId = QUuid::createUuid();
    Q_UNUSED(m_findTagByGuidRequestIds.insert(requestId));
    emit findTag(tag, requestId);
}

template <>
void FullSynchronizationManager::emitFindByGuidRequest<SavedSearch>(const QString & guid)
{
    SavedSearch search;
    search.unsetLocalGuid();
    search.setGuid(guid);

    QUuid requestId = QUuid::createUuid();
    Q_UNUSED(m_findSavedSearchByGuidRequestIds.insert(requestId));
    emit findSavedSearch(search, requestId);
}

template <>
void FullSynchronizationManager::emitFindByGuidRequest<Notebook>(const QString & guid)
{
    Notebook notebook;
    notebook.unsetLocalGuid();
    notebook.setGuid(guid);

    QUuid requestId = QUuid::createUuid();
    Q_UNUSED(m_findNotebookByGuidRequestIds.insert(requestId));
    emit findNotebook(notebook, requestId);
}

template <>
void FullSynchronizationManager::emitFindByGuidRequest<LinkedNotebook>(const QString & guid)
{
    LinkedNotebook linkedNotebook;
    linkedNotebook.setGuid(guid);

    QUuid requestId = QUuid::createUuid();
    Q_UNUSED(m_findLinkedNotebookRequestIds.insert(requestId));
    emit findLinkedNotebook(linkedNotebook, requestId);
}

template <>
void FullSynchronizationManager::emitFindByGuidRequest<Note>(const QString & guid)
{
    Note note;
    note.unsetLocalGuid();
    note.setGuid(guid);

    QUuid requestId = QUuid::createUuid();
    Q_UNUSED(m_findNoteByGuidRequestIds.insert(requestId));
    bool withResourceDataOption = false;
    emit findNote(note, withResourceDataOption, requestId);
}

void FullSynchronizationManager::onAddLinkedNotebookCompleted(LinkedNotebook linkedNotebook, QUuid requestId)
{
    onAddDataElementCompleted(linkedNotebook, requestId, "LinkedNotebook", m_addLinkedNotebookRequestIds);
}

void FullSynchronizationManager::onAddLinkedNotebookFailed(LinkedNotebook linkedNotebook, QString errorDescription, QUuid requestId)
{
    onAddDataElementFailed(linkedNotebook, requestId, errorDescription,
                           "LinkedNotebook", m_addLinkedNotebookRequestIds);
}

void FullSynchronizationManager::onUpdateLinkedNotebookCompleted(LinkedNotebook linkedNotebook, QUuid requestId)
{
    QSet<QUuid>::iterator it = m_updateLinkedNotebookRequestIds.find(requestId);
    if (it != m_updateLinkedNotebookRequestIds.end())
    {
        QNDEBUG("FullSynchronizationManager::onUpdateLinkedNotebookCompleted: linkedNotebook = "
                << linkedNotebook << ", requestId = " << requestId);

        Q_UNUSED(m_updateLinkedNotebookRequestIds.erase(it));

        checkServerDataMergeCompletion();
    }
}

void FullSynchronizationManager::onUpdateLinkedNotebookFailed(LinkedNotebook linkedNotebook, QString errorDescription, QUuid requestId)
{
    QSet<QUuid>::iterator it = m_updateLinkedNotebookRequestIds.find(requestId);
    if (it != m_updateLinkedNotebookRequestIds.end())
    {
        QNDEBUG("FullSynchronizationManager::onUpdateLinkedNotebookFailed: linkedNotebook = "
                << linkedNotebook << ", errorDescription = " << errorDescription
                << ", requestId = " << requestId);

        QString error = QT_TR_NOOP("Can't update linked notebook in local storage: ");
        error += errorDescription;
        emit failure(error);
    }
}

void FullSynchronizationManager::onAddNotebookCompleted(Notebook notebook, QUuid requestId)
{
    onAddDataElementCompleted(notebook, requestId, "Notebook", m_addNotebookRequestIds);
}

void FullSynchronizationManager::onAddNotebookFailed(Notebook notebook, QString errorDescription, QUuid requestId)
{
    onAddDataElementFailed(notebook, requestId, errorDescription, "Notebook", m_addNotebookRequestIds);
}

void FullSynchronizationManager::onUpdateNotebookCompleted(Notebook notebook, QUuid requestId)
{
    onUpdateDataElementCompleted(notebook, requestId, "Notebook", m_updateNotebookRequestIds,
                                 m_notebooksToAddPerRequestId);
}

void FullSynchronizationManager::onUpdateNotebookFailed(Notebook notebook, QString errorDescription, QUuid requestId)
{
    onUpdateDataElementFailed(notebook, requestId, errorDescription, "Notebook",
                              m_updateNotebookRequestIds, m_notebooksToAddPerRequestId);
}

void FullSynchronizationManager::onAddNoteCompleted(Note note, Notebook notebook, QUuid requestId)
{
    Q_UNUSED(notebook)
    onAddDataElementCompleted(note, requestId, "Note", m_addNoteRequestIds);
}

void FullSynchronizationManager::onAddNoteFailed(Note note, Notebook notebook, QString errorDescription, QUuid requestId)
{
    Q_UNUSED(notebook)
    onAddDataElementFailed(note, requestId, errorDescription, "Note", m_addNoteRequestIds);
}

void FullSynchronizationManager::onUpdateNoteCompleted(Note note, Notebook notebook, QUuid requestId)
{
    QSet<QUuid>::iterator it = m_updateNoteRequestIds.find(requestId);
    if (it != m_updateNoteRequestIds.end())
    {
        QNDEBUG("FullSynchronizationManager::onUpdateNoteCompleted: note = " << note
                << "\nnotebook = " << notebook << "\nrequestId = " << requestId);

        Q_UNUSED(m_updateNoteRequestIds.erase(it));

        checkServerDataMergeCompletion();
    }
}

void FullSynchronizationManager::onUpdateNoteFailed(Note note, Notebook notebook, QString errorDescription, QUuid requestId)
{
    QSet<QUuid>::iterator it = m_updateNoteRequestIds.find(requestId);
    if (it != m_updateNoteRequestIds.end())
    {
        QNDEBUG("FullSynchronizationManager::onUpdateNoteFailed: note = " << note
                << "\nnotebook = " << notebook << "\nerrorDescription = " << errorDescription
                << "\nrequestId = " << requestId);

        QString error = QT_TR_NOOP("Can't update note in local storage: ");
        error += errorDescription;
        emit failure(error);
    }
}

void FullSynchronizationManager::onListDirtyTagsCompleted(LocalStorageManager::ListObjectsOptions flag,
                                                          size_t limit, size_t offset,
                                                          LocalStorageManager::ListTagsOrder::type order,
                                                          LocalStorageManager::OrderDirection::type orderDirection,
                                                          QList<Tag> tags, QUuid requestId)
{
    QNDEBUG("FullSynchronizationManager::onListDirtyTagsCompleted: flag = " << flag
            << ", limit = " << limit << ", offset = " << offset << ", order = " << order
            << ", orderDirection = " << orderDirection << ", requestId = " << requestId);

    QUTE_NOTE_CHECK_PTR(m_pNoteStore);

    const int numTags = tags.size();
    for(int i = 0; i < numTags; ++i)
    {
        Tag & tag = tags[i];

        if (!tag.hasUpdateSequenceNumber())
        {
            // The tag is new, need to create it in the remote service
            try
            {
                // FIXME: oh shit, looks like a way to get the underlying qevercloud::Tag is required
                // tag = m_pNoteStore->createTag(tag);
            }
            catch(qevercloud::EDAMUserException & userException)
            {
                processEdamUserExceptionForTag(tag, userException, UserExceptionSource::Creation);
            }

            // TODO: continue from here: catch & process EDAMNotFoundException && EDAMSystemException
        }
    }
}

void FullSynchronizationManager::onListDirtyTagsFailed(LocalStorageManager::ListObjectsOptions flag,
                                                       size_t limit, size_t offset,
                                                       LocalStorageManager::ListTagsOrder::type order,
                                                       LocalStorageManager::OrderDirection::type orderDirection,
                                                       QString errorDescription, QUuid requestId)
{
    QNWARNING("FullSynchronizationManager::onListDirtyTagsFailed: flag = " << flag
              << ", limit = " << limit << ", offset = " << offset << ", order = "
              << order << ", orderDirection = " << orderDirection << ", error description = "
              << errorDescription << ", requestId = " << requestId);

    emit failure("Error listing dirty tags from local storage: " + errorDescription);
}

void FullSynchronizationManager::onListDirtySavedSearchesCompleted(LocalStorageManager::ListObjectsOptions flag,
                                                                   size_t limit, size_t offset,
                                                                   LocalStorageManager::ListSavedSearchesOrder::type order,
                                                                   LocalStorageManager::OrderDirection::type orderDirection,
                                                                   QList<SavedSearch> savedSearches, QUuid requestId)
{
    // TODO: implement
    Q_UNUSED(flag)
    Q_UNUSED(limit)
    Q_UNUSED(offset)
    Q_UNUSED(order)
    Q_UNUSED(orderDirection)
    Q_UNUSED(savedSearches)
    Q_UNUSED(requestId)
}

void FullSynchronizationManager::onListDirtySavedSearchesFailed(LocalStorageManager::ListObjectsOptions flag,
                                                                size_t limit, size_t offset,
                                                                LocalStorageManager::ListSavedSearchesOrder::type order,
                                                                LocalStorageManager::OrderDirection::type orderDirection,
                                                                QString errorDescription, QUuid requestId)
{
    // TODO: implement
    Q_UNUSED(flag)
    Q_UNUSED(limit)
    Q_UNUSED(offset)
    Q_UNUSED(order)
    Q_UNUSED(orderDirection)
    Q_UNUSED(errorDescription)
    Q_UNUSED(requestId)
}

void FullSynchronizationManager::onListDirtyNotebooksCompleted(LocalStorageManager::ListObjectsOptions flag,
                                                               size_t limit, size_t offset,
                                                               LocalStorageManager::ListNotebooksOrder::type order,
                                                               LocalStorageManager::OrderDirection::type orderDirection,
                                                               QList<Notebook> notebooks, QUuid requestId)
{
    // TODO: implement
    Q_UNUSED(flag)
    Q_UNUSED(limit)
    Q_UNUSED(offset)
    Q_UNUSED(order)
    Q_UNUSED(orderDirection)
    Q_UNUSED(notebooks)
    Q_UNUSED(requestId)
}

void FullSynchronizationManager::onListDirtyNotebooksFailed(LocalStorageManager::ListObjectsOptions flag,
                                                            size_t limit, size_t offset,
                                                            LocalStorageManager::ListNotebooksOrder::type order,
                                                            LocalStorageManager::OrderDirection::type orderDirection,
                                                            QString errorDescription, QUuid requestId)
{
    // TODO: implement
    Q_UNUSED(flag)
    Q_UNUSED(limit)
    Q_UNUSED(offset)
    Q_UNUSED(order)
    Q_UNUSED(orderDirection)
    Q_UNUSED(errorDescription)
    Q_UNUSED(requestId)
}

void FullSynchronizationManager::onListDirtyNotesCompleted(LocalStorageManager::ListObjectsOptions flag,
                                                           size_t limit, size_t offset,
                                                           LocalStorageManager::ListNotesOrder::type order,
                                                           LocalStorageManager::OrderDirection::type orderDirection,
                                                           QList<Note> notes, QUuid requestId)
{
    // TODO: implement
    Q_UNUSED(flag)
    Q_UNUSED(limit)
    Q_UNUSED(offset)
    Q_UNUSED(order)
    Q_UNUSED(orderDirection)
    Q_UNUSED(notes)
    Q_UNUSED(requestId)
}

void FullSynchronizationManager::onListDirtyNotesFailed(LocalStorageManager::ListObjectsOptions flag,
                                                        size_t limit, size_t offset,
                                                        LocalStorageManager::ListNotesOrder::type order,
                                                        LocalStorageManager::OrderDirection::type orderDirection,
                                                        QString errorDescription, QUuid requestId)
{
    // TODO: implement
    Q_UNUSED(flag)
    Q_UNUSED(limit)
    Q_UNUSED(offset)
    Q_UNUSED(order)
    Q_UNUSED(orderDirection)
    Q_UNUSED(errorDescription)
    Q_UNUSED(requestId)
}

void FullSynchronizationManager::createConnections()
{
    // Connect local signals with localStorageManagerThread's slots
    QObject::connect(this, SIGNAL(addUser(UserWrapper,QUuid)), &m_localStorageManagerThreadWorker, SLOT(onAddUserRequest(UserWrapper,QUuid)));
    QObject::connect(this, SIGNAL(updateUser(UserWrapper,QUuid)), &m_localStorageManagerThreadWorker, SLOT(onUpdateUserRequest(UserWrapper,QUuid)));
    QObject::connect(this, SIGNAL(findUser(UserWrapper,QUuid)), &m_localStorageManagerThreadWorker, SLOT(onFindUserRequest(UserWrapper,QUuid)));
    QObject::connect(this, SIGNAL(deleteUser(UserWrapper,QUuid)), &m_localStorageManagerThreadWorker, SLOT(onDeleteUserRequest(UserWrapper,QUuid)));
    QObject::connect(this, SIGNAL(expungeUser(UserWrapper,QUuid)), &m_localStorageManagerThreadWorker, SLOT(onExpungeUserRequest(UserWrapper,QUuid)));

    QObject::connect(this, SIGNAL(addNotebook(Notebook,QUuid)), &m_localStorageManagerThreadWorker, SLOT(onAddNotebookRequest(Notebook,QUuid)));
    QObject::connect(this, SIGNAL(updateNotebook(Notebook,QUuid)), &m_localStorageManagerThreadWorker, SLOT(onUpdateNotebookRequest(Notebook,QUuid)));
    QObject::connect(this, SIGNAL(findNotebook(Notebook,QUuid)), &m_localStorageManagerThreadWorker, SLOT(onFindNotebookRequest(Notebook,QUuid)));
    QObject::connect(this, SIGNAL(expungeNotebook(Notebook,QUuid)), &m_localStorageManagerThreadWorker, SLOT(onExpungeNotebookRequest(Notebook,QUuid)));

    QObject::connect(this, SIGNAL(addNote(Note,Notebook,QUuid)), &m_localStorageManagerThreadWorker, SLOT(onAddNoteRequest(Note,Notebook,QUuid)));
    QObject::connect(this, SIGNAL(updateNote(Note,Notebook,QUuid)), &m_localStorageManagerThreadWorker, SLOT(onUpdateNoteRequest(Note,Notebook,QUuid)));
    QObject::connect(this, SIGNAL(findNote(Note,bool,QUuid)), &m_localStorageManagerThreadWorker, SLOT(onFindNoteRequest(Note,bool,QUuid)));
    QObject::connect(this, SIGNAL(deleteNote(Note,QUuid)), &m_localStorageManagerThreadWorker, SLOT(onDeleteNoteRequest(Note,QUuid)));
    QObject::connect(this, SIGNAL(expungeNote(Note,QUuid)), &m_localStorageManagerThreadWorker, SLOT(onExpungeNoteRequest(Note,QUuid)));

    QObject::connect(this, SIGNAL(addTag(Tag,QUuid)), &m_localStorageManagerThreadWorker, SLOT(onAddTagRequest(Tag,QUuid)));
    QObject::connect(this, SIGNAL(updateTag(Tag,QUuid)), &m_localStorageManagerThreadWorker, SLOT(onUpdateTagRequest(Tag,QUuid)));
    QObject::connect(this, SIGNAL(findTag(Tag,QUuid)), &m_localStorageManagerThreadWorker, SLOT(onFindTagRequest(Tag,QUuid)));
    QObject::connect(this, SIGNAL(deleteTag(Tag,QUuid)), &m_localStorageManagerThreadWorker, SLOT(onDeleteTagRequest(Tag,QUuid)));
    QObject::connect(this, SIGNAL(expungeTag(Tag,QUuid)), &m_localStorageManagerThreadWorker, SLOT(onExpungeTagRequest(Tag,QUuid)));

    QObject::connect(this, SIGNAL(addResource(ResourceWrapper,Note,QUuid)), &m_localStorageManagerThreadWorker, SLOT(onAddResourceRequest(ResourceWrapper,Note,QUuid)));
    QObject::connect(this, SIGNAL(updateResource(ResourceWrapper,Note,QUuid)), &m_localStorageManagerThreadWorker, SLOT(onUpdateResourceRequest(ResourceWrapper,Note,QUuid)));
    QObject::connect(this, SIGNAL(findResource(ResourceWrapper,bool,QUuid)), &m_localStorageManagerThreadWorker, SLOT(onFindResourceRequest(ResourceWrapper,bool,QUuid)));
    QObject::connect(this, SIGNAL(expungeResource(ResourceWrapper,QUuid)), &m_localStorageManagerThreadWorker, SLOT(onExpungeResourceRequest(ResourceWrapper,QUuid)));

    QObject::connect(this, SIGNAL(addLinkedNotebook(LinkedNotebook,QUuid)), &m_localStorageManagerThreadWorker, SLOT(onAddLinkedNotebookRequest(LinkedNotebook,QUuid)));
    QObject::connect(this, SIGNAL(updateLinkedNotebook(LinkedNotebook,QUuid)), &m_localStorageManagerThreadWorker, SLOT(onUpdateLinkedNotebookRequest(LinkedNotebook,QUuid)));
    QObject::connect(this, SIGNAL(findLinkedNotebook(LinkedNotebook,QUuid)), &m_localStorageManagerThreadWorker, SLOT(onFindLinkedNotebookRequest(LinkedNotebook,QUuid)));
    QObject::connect(this, SIGNAL(expungeLinkedNotebook(LinkedNotebook,QUuid)), &m_localStorageManagerThreadWorker, SLOT(onExpungeLinkedNotebookRequest(LinkedNotebook,QUuid)));

    QObject::connect(this, SIGNAL(addSavedSearch(SavedSearch,QUuid)), &m_localStorageManagerThreadWorker, SLOT(onAddSavedSearchRequest(SavedSearch,QUuid)));
    QObject::connect(this, SIGNAL(updateSavedSearch(SavedSearch,QUuid)), &m_localStorageManagerThreadWorker, SLOT(onUpdateSavedSearchRequest(SavedSearch,QUuid)));
    QObject::connect(this, SIGNAL(findSavedSearch(SavedSearch,QUuid)), &m_localStorageManagerThreadWorker, SLOT(onFindSavedSearchRequest(SavedSearch,QUuid)));
    QObject::connect(this, SIGNAL(expungeSavedSearch(SavedSearch,QUuid)), &m_localStorageManagerThreadWorker, SLOT(onExpungeSavedSearch(SavedSearch,QUuid)));

    QObject::connect(this,
                     SIGNAL(requestLocalUnsynchronizedTags(LocalStorageManager::ListObjectsOptions,size_t,size_t,
                                                           LocalStorageManager::ListTagsOrder::type,
                                                           LocalStorageManager::OrderDirection::type,QUuid)),
                     &m_localStorageManagerThreadWorker,
                     SLOT(onListTagsRequest(LocalStorageManager::ListObjectsOptions,size_t,size_t,
                                            LocalStorageManager::ListTagsOrder::type,
                                            LocalStorageManager::OrderDirection::type,QUuid)));

    QObject::connect(this,
                     SIGNAL(requestLocalUnsynchronizedSavedSearches(LocalStorageManager::ListObjectsOptions,size_t,size_t,
                                                                    LocalStorageManager::ListSavedSearchesOrder::type,
                                                                    LocalStorageManager::OrderDirection::type,QUuid)),
                     &m_localStorageManagerThreadWorker,
                     SLOT(onListSavedSearchesRequest(LocalStorageManager::ListObjectsOptions,size_t,size_t,
                                                     LocalStorageManager::ListSavedSearchesOrder::type,
                                                     LocalStorageManager::OrderDirection::type,QUuid)));

    QObject::connect(this,
                     SIGNAL(requestLocalUnsynchronizedNotebooks(LocalStorageManager::ListObjectsOptions,size_t,size_t,
                                                                LocalStorageManager::ListNotebooksOrder::type,
                                                                LocalStorageManager::OrderDirection::type,QUuid)),
                     &m_localStorageManagerThreadWorker,
                     SLOT(onListSavedSearchesRequest(LocalStorageManager::ListObjectsOptions,size_t,size_t,
                                                     LocalStorageManager::ListSavedSearchesOrder::type,
                                                     LocalStorageManager::OrderDirection::type,QUuid)));

    QObject::connect(this,
                     SIGNAL(requestLocalUnsynchronizedNotes(LocalStorageManager::ListObjectsOptions,size_t,size_t,
                                                            LocalStorageManager::ListNotesOrder::type,
                                                            LocalStorageManager::OrderDirection::type,QUuid)),
                     &m_localStorageManagerThreadWorker,
                     SLOT(onListNotesRequest(LocalStorageManager::ListObjectsOptions,bool,size_t,size_t,
                                             LocalStorageManager::ListNotesOrder::type,
                                             LocalStorageManager::OrderDirection::type,QUuid)));

    // Connect localStorageManagerThread's signals to local slots
    QObject::connect(&m_localStorageManagerThreadWorker, SIGNAL(findUserComplete(UserWrapper,QUuid)), this, SLOT(onFindUserCompleted(UserWrapper,QUuid)));
    QObject::connect(&m_localStorageManagerThreadWorker, SIGNAL(findUserFailed(UserWrapper,QString,QUuid)), this, SLOT(onFindUserFailed(UserWrapper,QString,QUuid)));

    QObject::connect(&m_localStorageManagerThreadWorker, SIGNAL(findNotebookComplete(Notebook,QUuid)), this, SLOT(onFindNotebookCompleted(Notebook,QUuid)));
    QObject::connect(&m_localStorageManagerThreadWorker, SIGNAL(findNotebookFailed(Notebook,QString,QUuid)), this, SLOT(onFindNotebookFailed(Notebook,QString,QUuid)));

    QObject::connect(&m_localStorageManagerThreadWorker, SIGNAL(findNoteComplete(Note,bool,QUuid)), this, SLOT(onFindNoteCompleted(Note,bool,QUuid)));
    QObject::connect(&m_localStorageManagerThreadWorker, SIGNAL(findNoteFailed(Note,bool,QString,QUuid)), this, SLOT(onFindNoteFailed(Note,bool,QString,QUuid)));

    QObject::connect(&m_localStorageManagerThreadWorker, SIGNAL(findTagComplete(Tag,QUuid)), this, SLOT(onFindTagCompleted(Tag,QUuid)));
    QObject::connect(&m_localStorageManagerThreadWorker, SIGNAL(findTagFailed(Tag,QString,QUuid)), this, SLOT(onFindTagFailed(Tag,QString,QUuid)));

    QObject::connect(&m_localStorageManagerThreadWorker, SIGNAL(findResourceComplete(ResourceWrapper,bool,QUuid)), this, SLOT(onFindResourceCompleted(ResourceWrapper,bool,QUuid)));
    QObject::connect(&m_localStorageManagerThreadWorker, SIGNAL(findResourceFailed(ResourceWrapper,bool,QString,QUuid)), this, SLOT(onFindResourceFailed(ResourceWrapper,bool,QString,QUuid)));

    QObject::connect(&m_localStorageManagerThreadWorker, SIGNAL(findLinkedNotebookComplete(LinkedNotebook,QUuid)), this, SLOT(onFindLinkedNotebookCompleted(LinkedNotebook,QUuid)));
    QObject::connect(&m_localStorageManagerThreadWorker, SIGNAL(findLinkedNotebookFailed(LinkedNotebook,QString,QUuid)), this, SLOT(onFindLinkedNotebookFailed(LinkedNotebook,QString,QUuid)));

    QObject::connect(&m_localStorageManagerThreadWorker, SIGNAL(findSavedSearchComplete(SavedSearch,QUuid)), this, SLOT(onFindSavedSearchCompleted(SavedSearch,QUuid)));
    QObject::connect(&m_localStorageManagerThreadWorker, SIGNAL(findSavedSearchFailed(SavedSearch,QString,QUuid)), this, SLOT(onFindSavedSearchFailed(SavedSearch,QString,QUuid)));

    QObject::connect(&m_localStorageManagerThreadWorker, SIGNAL(addTagComplete(Tag,QUuid)), this, SLOT(onAddTagCompleted(Tag,QUuid)));
    QObject::connect(&m_localStorageManagerThreadWorker, SIGNAL(addTagFailed(Tag,QString,QUuid)), this, SLOT(onAddTagFailed(Tag,QString,QUuid)));

    QObject::connect(&m_localStorageManagerThreadWorker, SIGNAL(updateTagComplete(Tag,QUuid)), this, SLOT(onUpdateTagCompleted(Tag,QUuid)));
    QObject::connect(&m_localStorageManagerThreadWorker, SIGNAL(updateTagFailed(Tag,QString,QUuid)), this, SLOT(onUpdateTagFailed(Tag,QString,QUuid)));

    QObject::connect(&m_localStorageManagerThreadWorker, SIGNAL(addSavedSearchComplete(SavedSearch,QUuid)), this, SLOT(onAddSavedSearchCompleted(SavedSearch,QUuid)));
    QObject::connect(&m_localStorageManagerThreadWorker, SIGNAL(addSavedSearchFailed(SavedSearch,QString,QUuid)), this, SLOT(onAddSavedSearchFailed(SavedSearch,QString,QUuid)));

    QObject::connect(&m_localStorageManagerThreadWorker, SIGNAL(updateSavedSearchComplete(SavedSearch,QUuid)), this, SLOT(onUpdateSavedSearchCompleted(SavedSearch,QUuid)));
    QObject::connect(&m_localStorageManagerThreadWorker, SIGNAL(updateSavedSearchFailed(SavedSearch,QString,QUuid)), this, SLOT(onUpdateSavedSearchFailed(SavedSearch,QString,QUuid)));

    QObject::connect(&m_localStorageManagerThreadWorker, SIGNAL(addLinkedNotebookComplete(LinkedNotebook,QUuid)), this, SLOT(onAddLinkedNotebookCompleted(LinkedNotebook,QUuid)));
    QObject::connect(&m_localStorageManagerThreadWorker, SIGNAL(addLinkedNotebookFailed(LinkedNotebook,QString,QUuid)), this, SLOT(onAddLinkedNotebookFailed(LinkedNotebook,QString,QUuid)));

    QObject::connect(&m_localStorageManagerThreadWorker, SIGNAL(updateLinkedNotebookComplete(LinkedNotebook,QUuid)), this, SLOT(onUpdateLinkedNotebookCompleted(LinkedNotebook,QUuid)));
    QObject::connect(&m_localStorageManagerThreadWorker, SIGNAL(updateLinkedNotebookFailed(LinkedNotebook,QString,QUuid)), this, SLOT(onUpdateLinkedNotebookFailed(LinkedNotebook,QString,QUuid)));

    QObject::connect(&m_localStorageManagerThreadWorker, SIGNAL(addNotebookComplete(Notebook,QUuid)), this, SLOT(onAddNotebookCompleted(Notebook,QUuid)));
    QObject::connect(&m_localStorageManagerThreadWorker, SIGNAL(addNotebookFailed(Notebook,QString,QUuid)), this, SLOT(onAddNotebookFailed(Notebook,QString,QUuid)));

    QObject::connect(&m_localStorageManagerThreadWorker, SIGNAL(updateNotebookComplete(Notebook,QUuid)), this, SLOT(onUpdateNotebookCompleted(Notebook,QUuid)));
    QObject::connect(&m_localStorageManagerThreadWorker, SIGNAL(updateNotebookFailed(Notebook,QString,QUuid)), this, SLOT(onUpdateNotebookFailed(Notebook,QString,QUuid)));

    QObject::connect(&m_localStorageManagerThreadWorker, SIGNAL(updateNoteComplete(Note,Notebook,QUuid)), this, SLOT(onUpdateNoteCompleted(Note,Notebook,QUuid)));
    QObject::connect(&m_localStorageManagerThreadWorker, SIGNAL(updateNoteFailed(Note,Notebook,QString,QUuid)), this, SLOT(onUpdateNoteFailed(Note,Notebook,QString,QUuid)));

    QObject::connect(&m_localStorageManagerThreadWorker, SIGNAL(addNoteComplete(Note,Notebook,QUuid)), this, SLOT(onAddNoteCompleted(Note,Notebook,QUuid)));
    QObject::connect(&m_localStorageManagerThreadWorker, SIGNAL(addNoteFailed(Note,Notebook,QString,QUuid)), this, SLOT(onAddNoteFailed(Note,Notebook,QString,QUuid)));

    QObject::connect(&m_localStorageManagerThreadWorker,
                     SIGNAL(listTagsComplete(LocalStorageManager::ListObjectsOptions,size_t,size_t,
                                             LocalStorageManager::ListTagsOrder::type,
                                             LocalStorageManager::OrderDirection::type,QList<Tag>,QUuid)),
                     this,
                     SLOT(onListDirtyTagsCompleted(LocalStorageManager::ListObjectsOptions,size_t,size_t,
                                                   LocalStorageManager::ListTagsOrder::type,
                                                   LocalStorageManager::OrderDirection::type,QList<Tag>,QUuid)));

    QObject::connect(&m_localStorageManagerThreadWorker,
                     SIGNAL(listTagsFailed(LocalStorageManager::ListObjectsOptions,size_t,size_t,
                                           LocalStorageManager::ListTagsOrder::type,
                                           LocalStorageManager::OrderDirection::type,QString,QUuid)),
                     this,
                     SLOT(onListDirtyTagsFailed(LocalStorageManager::ListObjectsOptions,size_t,size_t,
                                                LocalStorageManager::ListTagsOrder::type,
                                                LocalStorageManager::OrderDirection::type,QString,QUuid)));

    QObject::connect(&m_localStorageManagerThreadWorker,
                     SIGNAL(listSavedSearchesComplete(LocalStorageManager::ListObjectsOptions,size_t,size_t,
                                                      LocalStorageManager::ListSavedSearchesOrder::type,
                                                      LocalStorageManager::OrderDirection::type,QList<SavedSearch>,QUuid)),
                     this,
                     SLOT(onListDirtySavedSearchesCompleted(LocalStorageManager::ListObjectsOptions,size_t,size_t,
                                                            LocalStorageManager::ListSavedSearchesOrder::type,
                                                            LocalStorageManager::OrderDirection::type,QList<SavedSearch>,QUuid)));

    QObject::connect(&m_localStorageManagerThreadWorker,
                     SIGNAL(listSavedSearchesFailed(LocalStorageManager::ListObjectsOptions,size_t,size_t,
                                                    LocalStorageManager::ListSavedSearchesOrder::type,
                                                    LocalStorageManager::OrderDirection::type,QString,QUuid)),
                     this,
                     SLOT(onListDirtySavedSearchesFailed(LocalStorageManager::ListObjectsOptions,size_t,size_t,
                                                         LocalStorageManager::ListSavedSearchesOrder::type,
                                                         LocalStorageManager::OrderDirection::type,QString,QUuid)));

    QObject::connect(&m_localStorageManagerThreadWorker,
                     SIGNAL(listNotebooksComplete(LocalStorageManager::ListObjectsOptions,size_t,size_t,
                                                  LocalStorageManager::ListNotebooksOrder::type,
                                                  LocalStorageManager::OrderDirection::type,QList<Notebook>,QUuid)),
                     this,
                     SLOT(onListDirtyNotebooksCompleted(LocalStorageManager::ListObjectsOptions,size_t,size_t,
                                                        LocalStorageManager::ListNotebooksOrder::type,
                                                        LocalStorageManager::OrderDirection::type,QList<Notebook>,QUuid)));

    QObject::connect(&m_localStorageManagerThreadWorker,
                     SIGNAL(listNotebooksFailed(LocalStorageManager::ListObjectsOptions,size_t,size_t,
                                                LocalStorageManager::ListNotebooksOrder::type,
                                                LocalStorageManager::OrderDirection::type,QString,QUuid)),
                     this,
                     SLOT(onListDirtyNotebooksFailed(LocalStorageManager::ListObjectsOptions,size_t,size_t,
                                                     LocalStorageManager::ListNotebooksOrder::type,
                                                     LocalStorageManager::OrderDirection::type,QString,QUuid)));

    QObject::connect(&m_localStorageManagerThreadWorker,
                     SIGNAL(listNotesComplete(LocalStorageManager::ListObjectsOptions,bool,size_t,size_t,
                                              LocalStorageManager::ListNotesOrder::type,
                                              LocalStorageManager::OrderDirection::type,QList<Note>,QUuid)),
                     this,
                     SLOT(onListDirtyNotesCompleted(LocalStorageManager::ListObjectsOptions,size_t,size_t,
                                                    LocalStorageManager::ListNotesOrder::type,
                                                    LocalStorageManager::OrderDirection::type,QList<Note>,QUuid)));

    QObject::connect(&m_localStorageManagerThreadWorker,
                     SIGNAL(listNotesFailed(LocalStorageManager::ListObjectsOptions,bool,size_t,size_t,
                                            LocalStorageManager::ListNotesOrder::type,
                                            LocalStorageManager::OrderDirection::type,QString,QUuid)),
                     this,
                     SLOT(onListDirtyNotesFailed(LocalStorageManager::ListObjectsOptions,size_t,size_t,
                                                 LocalStorageManager::ListNotesOrder::type,
                                                 LocalStorageManager::OrderDirection::type,QString,QUuid)));
}

void FullSynchronizationManager::launchTagsSync()
{
    QNDEBUG("FullSynchronizationManager::launchTagsSync");
    launchDataElementSync<TagsList, Tag>("Tag", m_tags);
}

void FullSynchronizationManager::launchSavedSearchSync()
{
    QNDEBUG("FullSynchronizationManager::launchSavedSearchSync");
    launchDataElementSync<SavedSearchesList, SavedSearch>("Saved search", m_savedSearches);
}

void FullSynchronizationManager::launchLinkedNotebookSync()
{
    QNDEBUG("FullSynchronizationManager::launchLinkedNotebookSync");
    launchDataElementSync<LinkedNotebooksList, LinkedNotebook>("Linked notebook", m_linkedNotebooks);
}

void FullSynchronizationManager::launchNotebookSync()
{
    QNDEBUG("FullSynchronizationManager::launchNotebookSync");
    launchDataElementSync<NotebooksList, Notebook>("Notebook", m_notebooks);
}

void FullSynchronizationManager::checkNotebooksAndTagsSyncAndLaunchNotesSync()
{
    QNDEBUG("FullSynchronizationManager::checkNotebooksAndTagsSyncAndLaunchNotesSync");

    if (m_updateNotebookRequestIds.empty() && m_addNotebookRequestIds.empty() &&
        m_updateTagRequestIds.empty() && m_addTagRequestIds.empty())
    {
        // All remote notebooks and tags were already either updated in the local storage or added there
        launchNotesSync();
    }
}

void FullSynchronizationManager::launchNotesSync()
{
    launchDataElementSync<NotesList, Note>("Note", m_notes);
}

void FullSynchronizationManager::checkLinkedNotebooksSyncAndLaunchLinkedNotebookContentSync()
{
    QNDEBUG("FullSynchronizationManager::checkLinkedNotebooksSyncAndLaunchLinkedNotebookContentSync");

    if (m_updateLinkedNotebookRequestIds.empty() && m_addLinkedNotebookRequestIds.empty()) {
        // All remote linked notebooks were already updated in the local storage or added there
        launchLinkedNotebookTagsSync();
        launchLinkedNotebookNotebooksSync();
    }
}

void FullSynchronizationManager::checkLinkedNotebooksNotebooksAndTagsSyncAndLaynchLinkedNotebookNotesSync()
{
    QNDEBUG("FullSynchronizationManager::checkLinkedNotebooksNotebooksAndTagsSyncAndLaynchLinkedNotebookNotesSync");

    // TODO: implement
}

void FullSynchronizationManager::launchLinkedNotebookTagsSync()
{
    QNDEBUG("FullSynchronizationManager::launchLinkedNotebookTagsSync");

    // TODO: implement
}

void FullSynchronizationManager::launchLinkedNotebookNotebooksSync()
{
    QNDEBUG("FullSynchronizationManager::launchLinkedNotebookNotebooksSync");

    // TODO: implement
}

void FullSynchronizationManager::launchLinkedNotebookNotesSync()
{
    QNDEBUG("FullSynchronizationManager::launchLinkedNotebookNotesSync");

    // TODO: implement
}

void FullSynchronizationManager::checkServerDataMergeCompletion()
{
    QNDEBUG("FullSynchronizationManager::checkServerDataMergeCompletion");

    // Need to check whether we are still waiting for the response from some add or update request
    bool tagsReady = m_updateTagRequestIds.empty() && m_addTagRequestIds.empty();
    if (!tagsReady) {
        QNDEBUG("Tags are not ready, pending response for " << m_updateTagRequestIds.size()
                << " tag update requests and/or " << m_addTagRequestIds.size() << " tag add requests");
        return;
    }

    bool searchesReady = m_updateSavedSearchRequestIds.empty() && m_addSavedSearchRequestIds.empty();
    if (!searchesReady) {
        QNDEBUG("Saved searches are not ready, pending response for " << m_updateSavedSearchRequestIds.size()
                << " saved search update requests and/or " << m_addSavedSearchRequestIds.size()
                << " saved search add requests");
        return;
    }

    bool linkedNotebooksReady = m_updateLinkedNotebookRequestIds.empty() && m_addLinkedNotebookRequestIds.empty();
    if (!linkedNotebooksReady) {
        QNDEBUG("Linked notebooks are not ready, pending response for " << m_updateLinkedNotebookRequestIds.size()
                << " linked notebook update requests and/or " << m_addLinkedNotebookRequestIds.size()
                << " linked notebook add requests");
        return;
    }

    bool notebooksReady = m_updateNotebookRequestIds.empty() && m_addNotebookRequestIds.empty();
    if (!notebooksReady) {
        QNDEBUG("Notebooks are not ready, pending response for " << m_updateNotebookRequestIds.size()
                << " and/or " << m_addNotebookRequestIds.size() << " notebook add requests");
        return;
    }

    bool notesReady = m_updateNoteRequestIds.empty() && m_addNoteRequestIds.empty();
    if (!notesReady) {
        QNDEBUG("Notes are not ready, pending response for " << m_updateNoteRequestIds.size()
                << " and/or " << m_addNoteRequestIds.size() << " note add requests");
        return;
    }

    // TODO: don't forget to account for content related to linked notebooks when its sync is implemented

    QNDEBUG("All server side data elements are processed, proceeding to sending local changes");
    requestLocalUnsynchronizedData();
}

void FullSynchronizationManager::requestLocalUnsynchronizedData()
{
    QNDEBUG("FullSynchronizationManager::requestLocalUnsynchronizedData");

    LocalStorageManager::ListObjectsOptions listDirtyTagsFlag =
            LocalStorageManager::ListDirty | LocalStorageManager::ListNonLocal;

    size_t limit = 0, offset = 0;
    LocalStorageManager::ListTagsOrder::type order = LocalStorageManager::ListTagsOrder::NoOrder;
    LocalStorageManager::OrderDirection::type orderDirection = LocalStorageManager::OrderDirection::Ascending;

    m_listDirtyTagsRequestId = QUuid::createUuid();
    emit requestLocalUnsynchronizedTags(listDirtyTagsFlag, limit, offset, order,
                                        orderDirection, m_listDirtyTagsRequestId);
}

void FullSynchronizationManager::processEdamUserExceptionForTag(const Tag & tag, const qevercloud::EDAMUserException & userException,
                                                                const FullSynchronizationManager::UserExceptionSource::type & source)
{
    bool thrownOnCreation = (source == UserExceptionSource::Creation);

    const auto exceptionData = userException.exceptionData();

    if (userException.errorCode == qevercloud::EDAMErrorCode::BAD_DATA_FORMAT)
    {
        QString error = QT_TR_NOOP("BAD_DATA_FORMAT exception during the attempt to " +
                                   QString(thrownOnCreation ? "create" : "update") + " tag");

        if (!userException.parameter.isSet())
        {
            if (!exceptionData.isNull() && !exceptionData->errorMessage.isEmpty()) {
                error += ": ";
                error += exceptionData->errorMessage;
            }

            emit failure(error);
            return;
        }

        if (userException.parameter.ref() == "Tag.name") {
            if (tag.hasName()) {
                error += QT_TR_NOOP("invalid length or pattern of tag's name: ");
                error += tag.name();
            }
            else {
                error += QT_TR_NOOP("tag has no name");
            }
        }
        else if (userException.parameter.ref() == "Tag.parentGuid") {
            if (tag.hasParentGuid()) {
                error += QT_TR_NOOP("malformed parent guid of tag: ");
                error += tag.parentGuid();
            }
            else {
                error += QT_TR_NOOP("error code indicates malformed parent guid but it is empty");
            }
        }
        else {
            error += QT_TR_NOOP("unexpected parameter: ");
            error += userException.parameter.ref();
        }

        emit failure(error);
        return;
    }
    else if (userException.errorCode == qevercloud::EDAMErrorCode::DATA_CONFLICT)
    {
        QString error = QT_TR_NOOP("DATA_CONFLICT exception during the attempt to " +
                                   QString(thrownOnCreation ? "create" : "update") + " tag");

        if (!userException.parameter.isSet())
        {
            if (!exceptionData.isNull() && !exceptionData->errorMessage.isEmpty()) {
                error += ": ";
                error += exceptionData->errorMessage;
            }

            emit failure(error);
            return;
        }

        if (userException.parameter.ref() == "Tag.name") {
            if (tag.hasName()) {
                error += QT_TR_NOOP("invalid length or pattern of tag's name: ");
                error += tag.name();
            }
            else {
                error += QT_TR_NOOP("tag has no name");
            }
        }

        if (!thrownOnCreation && (userException.parameter.ref() == "Tag.parentGuid")) {
            if (tag.hasParentGuid()) {
                error += QT_TR_NOOP("can't set parent for tag: circular parent-child correlation detected");
                error += tag.parentGuid();
            }
            else {
                error += QT_TR_NOOP("error code indicates the problem with circular parent-child correlation "
                                    "but tag's parent guid is empty");
            }
        }
        else {
            error += QT_TR_NOOP("unexpected parameter: ");
            error += userException.parameter.ref();
        }

        emit failure(error);
        return;
    }
    else if (thrownOnCreation && (userException.errorCode == qevercloud::EDAMErrorCode::LIMIT_REACHED))
    {
        QString error = QT_TR_NOOP("LIMIT_REACHED exception during the attempt to create tag");

        if (userException.parameter.isSet() && (userException.parameter.ref() == "Tag")) {
            error += QT_TR_NOOP(": already at max number of tags, please remove some of them");
        }

        emit failure(error);
        return;
    }
    else if (!thrownOnCreation && (userException.errorCode == qevercloud::EDAMErrorCode::PERMISSION_DENIED))
    {
        QString error = QT_TR_NOOP("PERMISSION_DENIED exception during the attempt to update tag");

        if (userException.parameter.isSet() && (userException.parameter.ref() == "Tag")) {
            error += QT_TR_NOOP(": user doesn't own the tag, it can't be updated");
        }

        emit failure(error);
        return;
    }

    // FIXME: check specifically for RATE_LIMIT_REACHED exception
    // FIXME: print fine error code instead of number
    QString error = QT_TR_NOOP("Unexpected EDAM user exception on attempt to " +
                               QString(thrownOnCreation ? "create" : "update") + " tag: errorCode = " +
                               QString::number(userException.errorCode));

    if (userException.parameter.isSet()) {
        error += ": ";
        error += QT_TR_NOOP("parameter: ");
        error += userException.parameter.ref();
    }

    emit failure(error);
}

void FullSynchronizationManager::clear()
{
    QNDEBUG("FullSynchronizationManager::clear");

    m_syncChunks.clear();

    m_tags.clear();
    m_tagsToAddPerRequestId.clear();
    m_findTagByNameRequestIds.clear();
    m_findTagByGuidRequestIds.clear();
    m_addTagRequestIds.clear();
    m_updateTagRequestIds.clear();

    m_savedSearches.clear();
    m_savedSearchesToAddPerRequestId.clear();
    m_findSavedSearchByNameRequestIds.clear();
    m_findSavedSearchByGuidRequestIds.clear();
    m_addSavedSearchRequestIds.clear();
    m_updateSavedSearchRequestIds.clear();

    m_linkedNotebooks.clear();
    m_findLinkedNotebookRequestIds.clear();
    m_addLinkedNotebookRequestIds.clear();
    m_updateLinkedNotebookRequestIds.clear();

    m_notebooks.clear();
    m_notebooksToAddPerRequestId.clear();
    m_findNotebookByNameRequestIds.clear();
    m_findNotebookByGuidRequestIds.clear();
    m_addNotebookRequestIds.clear();
    m_updateNotebookRequestIds.clear();

    m_localGuidsOfElementsAlreadyAttemptedToFindByName.clear();

    auto notesToAddPerAPICallPostponeTimerIdEnd = m_notesToAddPerAPICallPostponeTimerId.end();
    for(auto it = m_notesToAddPerAPICallPostponeTimerId.begin(); it != notesToAddPerAPICallPostponeTimerIdEnd; ++it) {
        int key = it.key();
        killTimer(key);
    }
    m_notesToAddPerAPICallPostponeTimerId.clear();

    auto notesToUpdateAndToAddLaterPerAPICallPostponeTimerIdEnd = m_notesToUpdatePerAPICallPostponeTimerId.end();
    for(auto it = m_notesToUpdatePerAPICallPostponeTimerId.begin(); it != notesToUpdateAndToAddLaterPerAPICallPostponeTimerIdEnd; ++it) {
        int key = it.key();
        killTimer(key);
    }
    m_notesToUpdatePerAPICallPostponeTimerId.clear();
}

void FullSynchronizationManager::timerEvent(QTimerEvent * pEvent)
{
    if (!pEvent) {
        QString errorDescription = QT_TR_NOOP("Qt error: detected null pointer to QTimerEvent");
        QNWARNING(errorDescription);
        emit failure(errorDescription);
        return;
    }

    int timerId = pEvent->timerId();
    killTimer(timerId);

    auto noteToAddIt = m_notesToAddPerAPICallPostponeTimerId.find(timerId);
    if (noteToAddIt != m_notesToAddPerAPICallPostponeTimerId.end())
    {
        Note note = noteToAddIt.value();

        Q_UNUSED(m_notesToAddPerAPICallPostponeTimerId.erase(noteToAddIt));

        int postponeAPICallSeconds = tryToGetFullNoteData(note);
        if (postponeAPICallSeconds < 0) {
            return;
        }
        else if (postponeAPICallSeconds > 0) {
            int timerId = startTimer(postponeAPICallSeconds);
            m_notesToAddPerAPICallPostponeTimerId[timerId] = note;
        }
        else {
            emitAddRequest(note);
        }

        return;
    }

    auto noteToUpdateIt = m_notesToUpdatePerAPICallPostponeTimerId.find(timerId);
    if (noteToUpdateIt != m_notesToUpdatePerAPICallPostponeTimerId.end())
    {
        Note noteToUpdate = noteToUpdateIt.value();

        Q_UNUSED(m_notesToUpdatePerAPICallPostponeTimerId.erase(noteToUpdateIt));

        int postponeAPICallSeconds = tryToGetFullNoteData(noteToUpdate);
        if (postponeAPICallSeconds < 0) {
            return;
        }
        else if (postponeAPICallSeconds > 0) {
            int timerId = startTimer(postponeAPICallSeconds);
            m_notesToUpdatePerAPICallPostponeTimerId[timerId] = noteToUpdate;
        }
        else {
            emitUpdateRequest(noteToUpdate);
        }

        return;
    }
}

qint32 FullSynchronizationManager::tryToGetFullNoteData(Note & note)
{
    if (!note.hasGuid()) {
        QString errorDescription = QT_TR_NOOP("detected attempt to get full note's data for note without guid");
        QNWARNING(errorDescription << ": " << note);
        emit failure(errorDescription);
        return -1;
    }

    if (m_pNoteStore.isNull()) {
        QString errorDescription = QT_TR_NOOP("detected null pointer to NoteStore when attempting to get full note's data");
        QNWARNING(errorDescription << ": " << note);
        emit failure(errorDescription);
        return -2;
    }

    try
    {
        bool withContent = true;
        bool withResourceData = true;
        bool withResourceRecognition = true;
        bool withResourceAlternateData = true;
        qevercloud::Note bulkNote = m_pNoteStore->getNote(note.guid(), withContent,
                                                          withResourceData,
                                                          withResourceRecognition,
                                                          withResourceAlternateData);
        note = bulkNote;
    }
    catch(const qevercloud::EDAMSystemException & systemException)
    {
        if (systemException.errorCode == qevercloud::EDAMErrorCode::RATE_LIMIT_REACHED)
        {
            if (!systemException.rateLimitDuration.isSet()) {
                QString errorDescription = QT_TR_NOOP("QEverCloud error: caught EDAMSystemException with error code of RATE_LIMIT_REACHED "
                                                      "but no rateLimitDuration is set");
                QNWARNING(errorDescription);
                emit failure(errorDescription);
                return -3;
            }

            qint32 secondsToWait = systemException.rateLimitDuration.ref();
            emit rateLimitExceeded(secondsToWait);
            return secondsToWait;
        }
    }
    catch(const qevercloud::EvernoteException & exception)
    {
        const auto exceptionData = exception.exceptionData();
        if (exceptionData.isNull()) {
            QString errorDescription = QT_TR_NOOP("QEverCloud error: caught EvernoteException when attempting to "
                                                  "get full note's data but exception data pointer is null");
            QNWARNING(errorDescription);
            emit failure(errorDescription);
            return -4;
        }

        QString what = exceptionData->errorMessage;
        QString errorDescription = QT_TR_NOOP("Caught exception when attempting to get full note's data: ");
        errorDescription += what;
        QNWARNING(errorDescription);
        emit failure(errorDescription);
        return -5;
    }

    return 0;
}

const Notebook * FullSynchronizationManager::getNotebookPerNote(const Note & note) const
{
    QNDEBUG("FullSynchronizationManager::getNotebookPerNote: note = " << note);

    QString noteGuid = (note.hasGuid() ? note.guid() : QString());
    QString noteLocalGuid = note.localGuid();

    QPair<QString,QString> key(noteGuid, noteLocalGuid);
    QHash<QPair<QString,QString>,Notebook>::const_iterator cit = m_notebooksPerNoteGuids.find(key);
    if (cit == m_notebooksPerNoteGuids.end()) {
        return nullptr;
    }
    else {
        return &(cit.value());
    }
}

template <>
void FullSynchronizationManager::appendDataElementsFromSyncChunkToContainer<FullSynchronizationManager::TagsList>(const qevercloud::SyncChunk & syncChunk,
                                                                                                                  FullSynchronizationManager::TagsList & container)
{
    if (syncChunk.tags.isSet()) {
        container.append(syncChunk.tags.ref());
    }

    if (syncChunk.expungedTags.isSet())
    {
        const auto & expungedTags = syncChunk.expungedTags.ref();
        const auto expungedTagsEnd = expungedTags.end();
        for(auto eit = expungedTags.begin(); eit != expungedTagsEnd; ++eit)
        {
            TagsList::iterator it = std::find_if(container.begin(), container.end(),
                                                 CompareItemByGuid<qevercloud::Tag>(*eit));
            if (it != container.end()) {
                Q_UNUSED(container.erase(it));
            }
        }
    }
}

template <>
void FullSynchronizationManager::appendDataElementsFromSyncChunkToContainer<FullSynchronizationManager::SavedSearchesList>(const qevercloud::SyncChunk & syncChunk,
                                                                                                                           FullSynchronizationManager::SavedSearchesList & container)
{
    if (syncChunk.searches.isSet()) {
        container.append(syncChunk.searches.ref());
    }

    if (syncChunk.expungedSearches.isSet())
    {
        const auto & expungedSearches = syncChunk.expungedSearches.ref();
        const auto expungedSearchesEnd = expungedSearches.end();
        for(auto eit = expungedSearches.begin(); eit != expungedSearchesEnd; ++eit)
        {
            SavedSearchesList::iterator it = std::find_if(container.begin(), container.end(),
                                                          CompareItemByGuid<qevercloud::SavedSearch>(*eit));
            if (it != container.end()) {
                Q_UNUSED(container.erase(it));
            }
        }
    }
}

template <>
void FullSynchronizationManager::appendDataElementsFromSyncChunkToContainer<FullSynchronizationManager::LinkedNotebooksList>(const qevercloud::SyncChunk & syncChunk,
                                                                                                                             FullSynchronizationManager::LinkedNotebooksList & container)
{
    if (syncChunk.linkedNotebooks.isSet()) {
        container.append(syncChunk.linkedNotebooks.ref());
    }

    if (syncChunk.expungedLinkedNotebooks.isSet())
    {
        const auto & expungedLinkedNotebooks = syncChunk.expungedLinkedNotebooks.ref();
        const auto expungedLinkedNotebooksEnd = expungedLinkedNotebooks.end();
        for(auto eit = expungedLinkedNotebooks.begin(); eit != expungedLinkedNotebooksEnd; ++eit)
        {
            LinkedNotebooksList::iterator it = std::find_if(container.begin(), container.end(),
                                                            CompareItemByGuid<qevercloud::LinkedNotebook>(*eit));
            if (it != container.end()) {
                Q_UNUSED(container.erase(it));
            }
        }
    }
}

template <>
void FullSynchronizationManager::appendDataElementsFromSyncChunkToContainer<FullSynchronizationManager::NotebooksList>(const qevercloud::SyncChunk & syncChunk,
                                                                                                                       FullSynchronizationManager::NotebooksList & container)
{
    if (syncChunk.notebooks.isSet()) {
        container.append(syncChunk.notebooks.ref());
    }

    if (syncChunk.expungedNotebooks.isSet())
    {
        const auto & expungedNotebooks = syncChunk.expungedNotebooks.ref();
        const auto expungedNotebooksEnd = expungedNotebooks.end();
        for(auto eit = expungedNotebooks.begin(); eit != expungedNotebooksEnd; ++eit)
        {
            NotebooksList::iterator it = std::find_if(container.begin(), container.end(),
                                                      CompareItemByGuid<qevercloud::Notebook>(*eit));
            if (it != container.end()) {
                Q_UNUSED(container.erase(it));
            }
        }
    }
}

template <>
void FullSynchronizationManager::appendDataElementsFromSyncChunkToContainer<FullSynchronizationManager::NotesList>(const qevercloud::SyncChunk & syncChunk,
                                                                                                                   FullSynchronizationManager::NotesList & container)
{
    if (syncChunk.notes.isSet()) {
        container.append(syncChunk.notes.ref());
    }

    if (syncChunk.expungedNotes.isSet())
    {
        const auto & expungedNotes = syncChunk.expungedNotes.ref();
        const auto expungedNotesEnd = expungedNotes.end();
        for(auto eit = expungedNotes.begin(); eit != expungedNotesEnd; ++eit)
        {
            NotesList::iterator it = std::find_if(container.begin(), container.end(),
                                                  CompareItemByGuid<qevercloud::Note>(*eit));
            if (it != container.end()) {
                Q_UNUSED(container.erase(it));
            }
        }
    }

    if (syncChunk.expungedNotebooks.isSet())
    {
        const auto & expungedNotebooks = syncChunk.expungedNotebooks.ref();
        const auto expungedNotebooksEnd = expungedNotebooks.end();
        for(auto eit = expungedNotebooks.begin(); eit != expungedNotebooksEnd; ++eit)
        {
            const QString & expungedNotebookGuid = *eit;

            for(auto it = container.begin(); it != container.end();)
            {
                qevercloud::Note & note = *it;
                if (note.notebookGuid.isSet() && (note.notebookGuid.ref() == expungedNotebookGuid)) {
                    it = container.erase(it);
                }
                else {
                    ++it;
                }
            }
        }
    }
}

template <class ContainerType, class ElementType>
typename ContainerType::iterator FullSynchronizationManager::findItemByName(ContainerType & container,
                                                                            const ElementType & element,
                                                                            const QString & typeName)
{
    QNDEBUG("FullSynchronizationManager::findItemByName<" << typeName << ">");

    // Attempt to find this data element by name within the list of elements waiting for processing;
    // first simply try the front element from the list to avoid the costly lookup
    if (!element.hasName()) {
        QString errorDescription = QT_TR_NOOP("Found " + typeName + " with empty name in local storage");
        QNWARNING(errorDescription << ": " << typeName << " = " << element);
        emit failure(errorDescription);
        return container.end();
    }

    if (container.empty()) {
        QString errorDescription = QT_TR_NOOP("detected attempt to find the element within the list "
                                              "of remote elements waiting for processing but that list is empty");
        QNWARNING(errorDescription << ": " << element);
        emit failure(errorDescription);
        return container.end();
    }

    // Try the front element first, in most cases it should be it
    const auto & frontItem = container.front();
    typename ContainerType::iterator it = container.begin();
    if (!frontItem.name.isSet() || (frontItem.name.ref() != element.name()))
    {
        it = std::find_if(container.begin(), container.end(),
                          CompareItemByName<typename ContainerType::value_type>(element.name()));
        if (it == container.end()) {
            QString errorDescription = QT_TR_NOOP("Can't find " + typeName + " by name within the list "
                                                  "of remote elements waiting for processing");
            QNWARNING(errorDescription << ": " << element);
            emit failure(errorDescription);
            return container.end();
        }
    }

    return it;
}

template<>
FullSynchronizationManager::NotesList::iterator FullSynchronizationManager::findItemByName<FullSynchronizationManager::NotesList, Note>(NotesList & container,
                                                                                                                                        const Note & element,
                                                                                                                                        const QString & typeName)
{
    QNDEBUG("FullSynchronizationManager::findItemByName<" << typeName << ">");

    // Attempt to find this data element by name within the list of elements waiting for processing;
    // first simply try the front element from the list to avoid the costly lookup
    if (!element.hasTitle()) {
        QString errorDescription = QT_TR_NOOP("Found " + typeName + " with empty name in local storage");
        QNWARNING(errorDescription << ": " << typeName << " = " << element);
        emit failure(errorDescription);
        return container.end();
    }

    if (container.empty()) {
        QString errorDescription = QT_TR_NOOP("detected attempt to find the element within the list "
                                              "of remote elements waiting for processing but that list is empty");
        QNWARNING(errorDescription << ": " << element);
        emit failure(errorDescription);
        return container.end();
    }

    // Try the front element first, in most cases it should be it
    const auto & frontItem = container.front();
    NotesList::iterator it = container.begin();
    if (!frontItem.title.isSet() || (frontItem.title.ref() != element.title()))
    {
        it = std::find_if(container.begin(), container.end(),
                          CompareItemByName<qevercloud::Note>(element.title()));
        if (it == container.end()) {
            QString errorDescription = QT_TR_NOOP("Can't find " + typeName + " by name within the list "
                                                  "of remote elements waiting for processing");
            QNWARNING(errorDescription << ": " << element);
            emit failure(errorDescription);
            return container.end();
        }
    }

    return it;
}

template <class ContainerType, class ElementType>
typename ContainerType::iterator FullSynchronizationManager::findItemByGuid(ContainerType & container,
                                                                            const ElementType & element,
                                                                            const QString & typeName)
{
    QNDEBUG("FullSynchronizationManager::findItemByGuid<" << typeName << ">");

    // Attempt to find this data element by guid within the list of elements waiting for processing;
    // first simply try the front element from the list to avoid the costly lookup
    if (!element.hasGuid()) {
        QString errorDescription = QT_TR_NOOP("Found " + typeName + " with empty guid in local storage");
        QNWARNING(errorDescription << ": " << typeName << " = " << element);
        emit failure(errorDescription);
        return container.end();
    }

    if (container.empty()) {
        QString errorDescription = QT_TR_NOOP("detected attempt to find the element within the list "
                                              "of remote elements waiting for processing but that list is empty");
        QNWARNING(errorDescription << ": " << element);
        emit failure(errorDescription);
        return container.end();
    }

    // Try the front element first, in most cases it should be it
    const auto & frontItem = container.front();
    typename ContainerType::iterator it = container.begin();
    if (!frontItem.guid.isSet() || (frontItem.guid.ref() != element.guid()))
    {
        it = std::find_if(container.begin(), container.end(),
                          CompareItemByGuid<typename ContainerType::value_type>(element.guid()));
        if (it == container.end()) {
            QString errorDescription = QT_TR_NOOP("can't find " + typeName + " by guid within the list "
                                                  "of elements from the remote storage waiting for processing");
            QNWARNING(errorDescription << ": " << element);
            emit failure(errorDescription);
            return container.end();
        }
    }

    return it;
}

template <class T>
bool FullSynchronizationManager::CompareItemByName<T>::operator()(const T & item) const
{
    if (item.name.isSet()) {
        return (m_name.toUpper() == item.name.ref().toUpper());
    }
    else {
        return false;
    }
}

template <>
bool FullSynchronizationManager::CompareItemByName<qevercloud::Note>::operator()(const qevercloud::Note & item) const
{
    if (item.title.isSet()) {
        return (m_name.toUpper() == item.title->toUpper());
    }
    else {
        return false;
    }
}

template <class T>
bool FullSynchronizationManager::CompareItemByGuid<T>::operator()(const T & item) const
{
    if (item.guid.isSet()) {
        return (m_guid == item.guid.ref());
    }
    else {
        return false;
    }
}

template <class ContainerType, class ElementType>
void FullSynchronizationManager::launchDataElementSync(const QString & typeName,
                                                       ContainerType & container)
{
    container.clear();
    int numSyncChunks = m_syncChunks.size();
    for(int i = 0; i < numSyncChunks; ++i) {
        const qevercloud::SyncChunk & syncChunk = m_syncChunks[i];
        appendDataElementsFromSyncChunkToContainer<ContainerType>(syncChunk, container);
    }

    if (container.empty()) {
        return;
    }

    int numElements = container.size();
    for(int i = 0; i < numElements; ++i)
    {
        const typename ContainerType::value_type & remoteElement = container[i];
        if (!remoteElement.guid.isSet()) {
            QString errorDescription = QT_TR_NOOP("found " + typeName + " from the remote storage "
                                                  "without guid set");
            QNWARNING(errorDescription << ": " << remoteElement);
            emit failure(errorDescription);
            return;
        }

        emitFindByGuidRequest<ElementType>(remoteElement.guid.ref());
    }
}

template <class ElementType>
void setConflictedBase(const QString & typeName, ElementType & element)
{
    QString currentDateTime = QDateTime::currentDateTime().toString(Qt::ISODate);

    element.setGuid(QString());
    element.setName(QObject::tr("Conflicted ") + typeName + element.name() + " (" + currentDateTime + ")");
    element.setDirty(true);
}

template <>
void setConflictedBase<Note>(const QString & typeName, Note & element)
{
    QString currentDateTime = QDateTime::currentDateTime().toString(Qt::ISODate);

    element.setGuid(QString());
    element.setTitle(QObject::tr("Conflicted ") + typeName + element.title() + " (" + currentDateTime + ")");
    element.setDirty(true);
    element.setLocal(true);
}

template <class ElementType>
void FullSynchronizationManager::setConflicted(const QString & typeName, ElementType & element)
{
    setConflictedBase(typeName, element);
}

template <>
void FullSynchronizationManager::setConflicted<Tag>(const QString & typeName, Tag & tag)
{
    setConflictedBase(typeName, tag);
    tag.setLocal(true);
}

template <>
void FullSynchronizationManager::emitFindByNameRequest<Tag>(const Tag & tag)
{
    if (!tag.hasName()) {
        QString errorDescription = QT_TR_NOOP("detected tag from remote storage which is "
                                              "to be searched by name in local storage but "
                                              "it has no name set");
        QNWARNING(errorDescription << ": " << tag);
        emit failure(errorDescription);
        return;
    }

    QUuid findElementRequestId = QUuid::createUuid();
    Q_UNUSED(m_findTagByNameRequestIds.insert(findElementRequestId));
    emit findTag(tag, findElementRequestId);
}

template <>
void FullSynchronizationManager::emitFindByNameRequest<SavedSearch>(const SavedSearch & search)
{
    if (!search.hasName()) {
        QString errorDescription = QT_TR_NOOP("detected saved search from remote storage which is "
                                              "to be searched by name in local storage but "
                                              "it has no name set");
        QNWARNING(errorDescription << ": " << search);
        emit failure(errorDescription);
    }

    QUuid findElementRequestId = QUuid::createUuid();
    Q_UNUSED(m_findSavedSearchByNameRequestIds.insert(findElementRequestId));
    emit findSavedSearch(search, findElementRequestId);
}

template <>
void FullSynchronizationManager::emitFindByNameRequest<Notebook>(const Notebook & notebook)
{
    if (!notebook.hasName()) {
        QString errorDescription = QT_TR_NOOP("detected notebook from remote storage which is "
                                              "to be searched by name in local storage but "
                                              "it has no name set");
        QNWARNING(errorDescription << ": " << notebook);
        emit failure(errorDescription);
    }

    QUuid findElementRequestId = QUuid::createUuid();
    Q_UNUSED(m_findNotebookByNameRequestIds.insert(findElementRequestId));
    emit findNotebook(notebook, findElementRequestId);
}

template <class ContainerType, class ElementType>
bool FullSynchronizationManager::onFoundDuplicateByName(ElementType element, const QUuid & requestId,
                                                        const QString & typeName, ContainerType & container,
                                                        QSet<QUuid> & findElementRequestIds)
{
    QSet<QUuid>::iterator rit = findElementRequestIds.find(requestId);
    if (rit == findElementRequestIds.end()) {
        return false;
    }

    QNDEBUG("FullSynchronizationManager::onFoundDuplicateByName<" << typeName << ">: "
            << typeName << " = " << element << ", requestId  = " << requestId);

    Q_UNUSED(findElementRequestIds.erase(rit));

    typename ContainerType::iterator it = findItemByName(container, element, typeName);
    if (it == container.end()) {
        return true;
    }

    // The element exists both in the client and in the server
    typedef typename ContainerType::value_type RemoteElementType;
    const RemoteElementType & remoteElement = *it;
    if (!remoteElement.updateSequenceNum.isSet()) {
        QString errorDescription = QT_TR_NOOP("Found " + typeName + " from sync chunk without the update sequence number");
        QNWARNING(errorDescription << ": " << remoteElement);
        emit failure(errorDescription);
        return true;
    }

    ElementType remoteElementAdapter(remoteElement);
    checkUpdateSequenceNumbersAndProcessConflictedElements(remoteElementAdapter, typeName, element);

    Q_UNUSED(container.erase(it));

    return true;
}

template <class ElementType, class ContainerType>
bool FullSynchronizationManager::onFoundDuplicateByGuid(ElementType element, const QUuid & requestId,
                                                        const QString & typeName, ContainerType & container,
                                                        QSet<QUuid> & findByGuidRequestIds)
{
    typename QSet<QUuid>::iterator rit = findByGuidRequestIds.find(requestId);
    if (rit == findByGuidRequestIds.end()) {
        return false;
    }

    QNDEBUG("onFoundDuplicateByGuid<" << typeName << ">: "
            << typeName << " = " << element << ", requestId = " << requestId);

    typename ContainerType::iterator it = findItemByGuid(container, element, typeName);
    if (it == container.end()) {
        QString errorDescription = QT_TR_NOOP("could not find the remote " + typeName + " by guid "
                                              "when reported of duplicate by guid in the local storage");
        QNWARNING(errorDescription << ": " << element);
        emit failure(errorDescription);
        return true;
    }

    typedef typename ContainerType::value_type RemoteElementType;
    const RemoteElementType & remoteElement = *it;
    if (!remoteElement.updateSequenceNum.isSet()) {
        QString errorDescription = QT_TR_NOOP("found " + typeName + " from remote storage without "
                                              "the update sequence number set");
        QNWARNING(errorDescription << ": " << remoteElement);
        emit failure(errorDescription);
        return true;
    }

    ElementType remoteElementAdapter(remoteElement);
    checkUpdateSequenceNumbersAndProcessConflictedElements(remoteElementAdapter, typeName, element);

    Q_UNUSED(container.erase(it));
    Q_UNUSED(findByGuidRequestIds.erase(rit));

    return true;
}

template <class ContainerType, class ElementType>
bool FullSynchronizationManager::onNoDuplicateByGuid(ElementType element, const QUuid & requestId,
                                                     const QString & errorDescription,
                                                     const QString & typeName, ContainerType & container,
                                                     QSet<QUuid> & findElementRequestIds)
{
    QSet<QUuid>::iterator rit = findElementRequestIds.find(requestId);
    if (rit == findElementRequestIds.end()) {
        return false;
    }

    QNDEBUG("FullSynchronizationManager::onNoDuplicateByGuid<" << typeName << ">: " << element
            << ", errorDescription = " << errorDescription << ", requestId = " << requestId);

    Q_UNUSED(findElementRequestIds.erase(rit));

    typename ContainerType::iterator it = findItemByGuid(container, element, typeName);
    if (it == container.end()) {
        return true;
    }

    // This element wasn't found in the local storage by guid, need to check whether
    // the element with similar name exists
    ElementType elementToFindByName(*it);
    emitFindByNameRequest(elementToFindByName);

    return true;
}

template <class ContainerType, class ElementType>
bool FullSynchronizationManager::onNoDuplicateByName(ElementType element, const QUuid & requestId,
                                                     const QString & errorDescription,
                                                     const QString & typeName, ContainerType & container,
                                                     QSet<QUuid> & findElementRequestIds)
{
    QSet<QUuid>::iterator rit = findElementRequestIds.find(requestId);
    if (rit == findElementRequestIds.end()) {
        return false;
    }

    QNDEBUG("FullSynchronizationManager::onNoDuplicateByUniqueKey<" << typeName << ">: " << element
            << ", errorDescription = " << errorDescription << ", requestId = " << requestId);

    Q_UNUSED(findElementRequestIds.erase(rit));

    typename ContainerType::iterator it = findItemByName(container, element, typeName);
    if (it == container.end()) {
        return true;
    }

    // This element wasn't found in the local storage by name, adding it to local storage
    ElementType newElement(*it);
    emitAddRequest(newElement);

    // also removing the element from the list of ones waiting for processing
    Q_UNUSED(container.erase(it));

    return true;
}

template <class ElementType>
void FullSynchronizationManager::processConflictedElement(const ElementType & remoteElement,
                                                          const QString & typeName, ElementType & element)
{
    setConflicted(typeName, element);

    emitUpdateRequest(element, &remoteElement);
}

template <>
void FullSynchronizationManager::processConflictedElement(const LinkedNotebook & remoteLinkedNotebook,
                                                          const QString &, LinkedNotebook & linkedNotebook)
{
    // Linked notebook itself is simply a pointer to another user's account;
    // The data it points to would have a separate synchronization procedure
    // including the synchronization for Notebook, Notes and Tags from another user's account
    // The linked notebook as a pointer to all this data would simply be overridden
    // by the server's version of it
    linkedNotebook = remoteLinkedNotebook;

    QUuid updateRequestId = QUuid::createUuid();
    Q_UNUSED(m_updateLinkedNotebookRequestIds.insert(updateRequestId));
    emit updateLinkedNotebook(linkedNotebook, updateRequestId);
}

template <class ElementType>
void FullSynchronizationManager::checkUpdateSequenceNumbersAndProcessConflictedElements(const ElementType & remoteElement,
                                                                                        const QString & typeName,
                                                                                        ElementType & localElement)
{
    if ( !localElement.hasUpdateSequenceNumber() ||
         (remoteElement.updateSequenceNumber() > localElement.updateSequenceNumber()) )
    {
        if (!localElement.isDirty())
        {
            // Remote element is more recent, need to update the element existing in local storage
            ElementType elementToUpdate(remoteElement);
            unsetLocalGuid(elementToUpdate);

            // NOTE: the hack with static_cast to pointer type below is required to get the stupid MSVC compiler
            // to instantiate the template method correctly. If Linus called gcc-4.9 pure and utter crap,
            // I don't know what to say about MSVC 2013...
            emitUpdateRequest<ElementType>(elementToUpdate, static_cast<const ElementType*>(nullptr));
        }
        else
        {
            // Remote element is more recent but the local one has been modified;
            // Evernote's synchronization protocol description suggests trying
            // to do field-by-field merge but it's overcomplicated and error-prone;
            // it's much easier to rename the existing local element to make it clear
            // it has a conflict with its remote counterpart and mark it dirty so that
            // it would be sent to the server along with other local changes
            processConflictedElement(remoteElement, typeName, localElement);
        }
    }
}


} // namespace qute_note
