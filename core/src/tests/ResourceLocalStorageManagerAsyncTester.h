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
    void addNotebookRequest(QSharedPointer<Notebook> notebook);
    void addNoteRequest(QSharedPointer<Note> note, QSharedPointer<Notebook> notebook);
    void getResourceCountRequest();
    void addResourceRequest(QSharedPointer<ResourceWrapper> resource, QSharedPointer<Note> note);
    void updateResourceRequest(QSharedPointer<ResourceWrapper> resource, QSharedPointer<Note> note);
    void findResourceRequest(QSharedPointer<ResourceWrapper> resource, bool withBinaryData);
    void expungeResourceRequest(QSharedPointer<ResourceWrapper> resource);

private Q_SLOTS:
    void onAddNotebookCompleted(QSharedPointer<Notebook> notebook);
    void onAddNotebookFailed(QSharedPointer<Notebook> notebook, QString errorDescription);
    void onAddNoteCompleted(QSharedPointer<Note> note, QSharedPointer<Notebook> notebook);
    void onAddNoteFailed(QSharedPointer<Note> note, QSharedPointer<Notebook> notebook, QString errorDescription);
    void onGetResourceCountCompleted(int count);
    void onGetResourceCountFailed(QString errorDescription);
    void onAddResourceCompleted(QSharedPointer<ResourceWrapper> resource);
    void onAddResourceFailed(QSharedPointer<ResourceWrapper> resource, QString errorDescription);
    void onUpdateResourceCompleted(QSharedPointer<ResourceWrapper> resource);
    void onUpdateResourceFailed(QSharedPointer<ResourceWrapper> resource, QString errorDescription);
    void onFindResourceCompleted(QSharedPointer<ResourceWrapper> resource);
    void onFindResourceFailed(QSharedPointer<ResourceWrapper> resource, QString errorDescription);
    void onExpungeResourceCompleted(QSharedPointer<ResourceWrapper> resource);
    void onExpungeResourceFailed(QSharedPointer<ResourceWrapper> resource, QString errorDescription);

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
    QSharedPointer<Notebook>    m_pNotebook;
    QSharedPointer<Note>        m_pNote;
    QSharedPointer<ResourceWrapper> m_pInitialResource;
    QSharedPointer<ResourceWrapper> m_pFoundResource;
    QSharedPointer<ResourceWrapper> m_pModifiedResource;
};

} // namespace test
} // namespace qute_note

#endif // __QUTE_NOTE__CORE__TESTS__RESOURCE_LOCAL_STORAGE_MANAGER_ASYNC_TESTER_H
