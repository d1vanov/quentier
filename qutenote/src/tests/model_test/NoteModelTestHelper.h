#ifndef __QUTE_NOTE__TESTS__MODEL_TEST__NOTE_MODEL_TEST_HELPER_H
#define __QUTE_NOTE__TESTS__MODEL_TEST__NOTE_MODEL_TEST_HELPER_H

#include <qute_note/local_storage/LocalStorageManagerThreadWorker.h>

namespace qute_note {

class NoteModelTestHelper: public QObject
{
    Q_OBJECT
public:
    explicit NoteModelTestHelper(LocalStorageManagerThreadWorker * pLocalStorageManagerThreadWorker,
                                 QObject * parent = Q_NULLPTR);

Q_SIGNALS:
    void failure();
    void success();

public Q_SLOTS:
    void launchTest();

private Q_SLOTS:
    void onAddNoteComplete(Note note, QUuid requestId);
    void onAddNoteFailed(Note note, QString errorDescription, QUuid requestId);
    void onUpdateNoteComplete(Note note, bool updateResources, bool updateTags, QUuid requestId);
    void onUpdateNoteFailed(Note note, bool updateResources, bool updateTags,
                            QString errorDescription, QUuid requestId);
    void onFindNoteFailed(Note note, bool withResourceBinaryData, QString errorDescription, QUuid requestId);
    void onListNotesFailed(LocalStorageManager::ListObjectsOptions flag, bool withResourceBinaryData,
                           size_t limit, size_t offset, LocalStorageManager::ListNotesOrder::type order,
                           LocalStorageManager::OrderDirection::type orderDirection,
                           QString errorDescription, QUuid requestId);
    void onDeleteNoteComplete(Note note, QUuid requestId);
    void onDeleteNoteFailed(Note note, QString errorDescription, QUuid requestId);
    void onExpungeNoteComplete(Note note, QUuid requestId);
    void onExpungeNoteFailed(Note note, QString errorDescription, QUuid requestId);

private:
    void testAfterNewNoteAddition();
    void testAfterNoteUpdate();

private:
    LocalStorageManagerThreadWorker *   m_pLocalStorageManagerThreadWorker;
};

} // namespace qute_note

#endif // __QUTE_NOTE__TESTS__MODEL_TEST__NOTE_MODEL_TEST_HELPER_H
