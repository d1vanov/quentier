#ifndef QUTE_NOTE_TESTS_MODEL_TEST_NOTE_MODEL_TEST_HELPER_H
#define QUTE_NOTE_TESTS_MODEL_TEST_NOTE_MODEL_TEST_HELPER_H

#include <qute_note/local_storage/LocalStorageManagerThreadWorker.h>

namespace qute_note {

QT_FORWARD_DECLARE_CLASS(NoteModel)
QT_FORWARD_DECLARE_CLASS(NoteModelItem)

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

    void onAddNotebookFailed(Notebook notebook, QString errorDescription, QUuid requestId);
    void onUpdateNotebookComplete(Notebook notebook, QUuid requestId);
    void onUpdateNotebookFailed(Notebook notebook, QString errorDescription, QUuid requestId);

    void onAddTagFailed(Tag tag, QString errorDescription, QUuid requestId);

private:
    void testAfterNewNoteAddition();
    void testAfterNoteUpdate();

    void checkSorting(const NoteModel & model);

private:
    struct LessByCreationTimestamp
    {
        bool operator()(const NoteModelItem & lhs, const NoteModelItem & rhs) const;
    };

    struct GreaterByCreationTimestamp
    {
        bool operator()(const NoteModelItem & lhs, const NoteModelItem & rhs) const;
    };

    struct LessByModificationTimestamp
    {
        bool operator()(const NoteModelItem & lhs, const NoteModelItem & rhs) const;
    };

    struct GreaterByModificationTimestamp
    {
        bool operator()(const NoteModelItem & lhs, const NoteModelItem & rhs) const;
    };

    struct LessByDeletionTimestamp
    {
        bool operator()(const NoteModelItem & lhs, const NoteModelItem & rhs) const;
    };

    struct GreaterByDeletionTimestamp
    {
        bool operator()(const NoteModelItem & lhs, const NoteModelItem & rhs) const;
    };

    struct LessByTitle
    {
        bool operator()(const NoteModelItem & lhs, const NoteModelItem & rhs) const;
    };

    struct GreaterByTitle
    {
        bool operator()(const NoteModelItem & lhs, const NoteModelItem & rhs) const;
    };

    struct LessByPreviewText
    {
        bool operator()(const NoteModelItem & lhs, const NoteModelItem & rhs) const;
    };

    struct GreaterByPreviewText
    {
        bool operator()(const NoteModelItem & lhs, const NoteModelItem & rhs) const;
    };

    struct LessByNotebookName
    {
        bool operator()(const NoteModelItem & lhs, const NoteModelItem & rhs) const;
    };

    struct GreaterByNotebookName
    {
        bool operator()(const NoteModelItem & lhs, const NoteModelItem & rhs) const;
    };

    struct LessBySize
    {
        bool operator()(const NoteModelItem & lhs, const NoteModelItem & rhs) const;
    };

    struct GreaterBySize
    {
        bool operator()(const NoteModelItem & lhs, const NoteModelItem & rhs) const;
    };

    struct LessBySynchronizable
    {
        bool operator()(const NoteModelItem & lhs, const NoteModelItem & rhs) const;
    };

    struct GreaterBySynchronizable
    {
        bool operator()(const NoteModelItem & lhs, const NoteModelItem & rhs) const;
    };

    struct LessByDirty
    {
        bool operator()(const NoteModelItem & lhs, const NoteModelItem & rhs) const;
    };

    struct GreaterByDirty
    {
        bool operator()(const NoteModelItem & lhs, const NoteModelItem & rhs) const;
    };

private:
    LocalStorageManagerThreadWorker *   m_pLocalStorageManagerThreadWorker;
};

} // namespace qute_note

#endif // QUTE_NOTE_TESTS_MODEL_TEST_NOTE_MODEL_TEST_HELPER_H
