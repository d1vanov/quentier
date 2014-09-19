#ifndef __QUTE_NOTE__CORE__TESTS__RESOURCE_LOCAL_STORAGE_MANAGER_ASYNC_TESTER_H
#define __QUTE_NOTE__CORE__TESTS__RESOURCE_LOCAL_STORAGE_MANAGER_ASYNC_TESTER_H

#include <client/types/Notebook.h>
#include <client/types/Note.h>
#include <client/types/ResourceWrapper.h>
#include <QSharedPointer>

namespace qute_note {

QT_FORWARD_DECLARE_CLASS(LocalStorageManagerThread)

namespace test {

class ResourceLocalStorageManagerAsyncTester: public QObject
{
    Q_OBJECT
public:
    explicit ResourceLocalStorageManagerAsyncTester(QObject * parent = nullptr);
    ~ResourceLocalStorageManagerAsyncTester();

public Q_SLOTS:
    void onInitTestCase();

Q_SIGNALS:
    void success();
    void failure(QString errorDescription);

// private signals:
    void addNotebookRequest(Notebook notebook);
    void addNoteRequest(Note note, Notebook notebook);
    void getResourceCountRequest();
    void addResourceRequest(ResourceWrapper resource, Note note);
    void updateResourceRequest(ResourceWrapper resource, Note note);
    void findResourceRequest(ResourceWrapper resource, bool withBinaryData);
    void expungeResourceRequest(ResourceWrapper resource);

private Q_SLOTS:
    void onAddNotebookCompleted(Notebook notebook);
    void onAddNotebookFailed(Notebook notebook, QString errorDescription);
    void onAddNoteCompleted(Note note, Notebook notebook);
    void onAddNoteFailed(Note note, Notebook notebook, QString errorDescription);
    void onGetResourceCountCompleted(int count);
    void onGetResourceCountFailed(QString errorDescription);
    void onAddResourceCompleted(ResourceWrapper resource);
    void onAddResourceFailed(ResourceWrapper resource, Note note, QString errorDescription);
    void onUpdateResourceCompleted(ResourceWrapper resource);
    void onUpdateResourceFailed(ResourceWrapper resource, Note note, QString errorDescription);
    void onFindResourceCompleted(ResourceWrapper resource, bool withBinaryData);
    void onFindResourceFailed(ResourceWrapper resource, bool withBinaryData,
                              QString errorDescription);
    void onExpungeResourceCompleted(ResourceWrapper resource);
    void onExpungeResourceFailed(ResourceWrapper resource, QString errorDescription);

private:
    void createConnections();

    enum State
    {
        STATE_UNINITIALIZED,
        STATE_SENT_ADD_NOTEBOOK_REQUEST,
        STATE_SENT_ADD_NOTE_REQUEST,
        STATE_SENT_ADD_REQUEST,
        STATE_SENT_FIND_AFTER_ADD_REQUEST,
        STATE_SENT_UPDATE_REQUEST,
        STATE_SENT_FIND_AFTER_UPDATE_REQUEST,
        STATE_SENT_GET_COUNT_AFTER_UPDATE_REQUEST,
        STATE_SENT_EXPUNGE_REQUEST,
        STATE_SENT_FIND_AFTER_EXPUNGE_REQUEST,
        STATE_SENT_GET_COUNT_AFTER_EXPUNGE_REQUEST
    };

    State m_state;
    LocalStorageManagerThread * m_pLocalStorageManagerThread;
    Notebook                    m_notebook;
    Note                        m_note;
    ResourceWrapper             m_initialResource;
    ResourceWrapper             m_foundResource;
    ResourceWrapper             m_modifiedResource;
};

} // namespace test
} // namespace qute_note

#endif // __QUTE_NOTE__CORE__TESTS__RESOURCE_LOCAL_STORAGE_MANAGER_ASYNC_TESTER_H
