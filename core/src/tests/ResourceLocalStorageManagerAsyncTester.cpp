#include "ResourceLocalStorageManagerAsyncTester.h"
#include <client/local_storage/LocalStorageManagerThread.h>
#include <logging/QuteNoteLogger.h>

namespace qute_note {
namespace test {

ResourceLocalStorageManagerAsyncTester::ResourceLocalStorageManagerAsyncTester(QObject * parent) :
    QObject(parent),
    m_state(STATE_UNINITIALIZED),
    m_pLocalStorageManagerThread(nullptr),
    m_pNotebook(),
    m_pNote(),
    m_pInitialResource(),
    m_pFoundResource(),
    m_pModifiedResource()
{}

ResourceLocalStorageManagerAsyncTester::~ResourceLocalStorageManagerAsyncTester()
{}

void ResourceLocalStorageManagerAsyncTester::onInitTestCase()
{
    QString username = "ResourceLocalStorageManagerAsyncTester";
    qint32 userId = 6;
    bool startFromScratch = true;

    if (m_pLocalStorageManagerThread != nullptr) {
        delete m_pLocalStorageManagerThread;
        m_pLocalStorageManagerThread = nullptr;
    }

    m_state = STATE_UNINITIALIZED;

    m_pLocalStorageManagerThread = new LocalStorageManagerThread(username, userId, startFromScratch, this);
    createConnections();

    m_pNotebook = QSharedPointer<Notebook>(new Notebook);
    m_pNotebook->setGuid("00000000-0000-0000-c000-000000000047");
    m_pNotebook->setUpdateSequenceNumber(1);
    m_pNotebook->setName("Fake notebook name");
    m_pNotebook->setCreationTimestamp(1);
    m_pNotebook->setModificationTimestamp(1);
    m_pNotebook->setDefaultNotebook(true);
    m_pNotebook->setLastUsed(false);
    m_pNotebook->setPublishingUri("Fake publishing uri");
    m_pNotebook->setPublishingOrder(1);
    m_pNotebook->setPublishingAscending(true);
    m_pNotebook->setPublishingPublicDescription("Fake public description");
    m_pNotebook->setPublished(true);
    m_pNotebook->setStack("Fake notebook stack");
    m_pNotebook->setBusinessNotebookDescription("Fake business notebook description");
    m_pNotebook->setBusinessNotebookPrivilegeLevel(1);
    m_pNotebook->setBusinessNotebookRecommended(true);

    QString errorDescription;
    if (!m_pNotebook->checkParameters(errorDescription)) {
        QNWARNING("Found invalid notebook: " << *m_pNotebook << ", error: " << errorDescription);
        emit failure(errorDescription);
        return;
    }

    m_state = STATE_SENT_ADD_NOTEBOOK_REQUEST;
    emit addNotebookRequest(m_pNotebook);
}

void ResourceLocalStorageManagerAsyncTester::onAddNotebookCompleted(QSharedPointer<Notebook> notebook)
{
    Q_ASSERT_X(!notebook.isNull(), "ResourceLocalStorageManagerAsyncTester::onAddNotebookCompleted slot",
               "Found NULL shared pointer to Notebook");

    QString errorDescription;

#define HANDLE_WRONG_STATE() \
    else { \
        errorDescription = "Internal error in ResourceLocalStorageManagerAsyncTester: " \
                           "found wrong state"; \
        QNWARNING(errorDescription << ": " << m_state); \
        emit failure(errorDescription); \
    }

    if (m_state == STATE_SENT_ADD_NOTEBOOK_REQUEST)
    {
        if (m_pNotebook != notebook) {
            errorDescription = "Internal error in ResourceLocalStorageManagerAsyncTester: "
                               "notebook in onAddNotebookCompleted slot doesn't match "
                               "the original Notebook";
            QNWARNING(errorDescription);
            emit failure(errorDescription);
            return;
        }

        m_pNote = QSharedPointer<Note>(new Note);
        m_pNote->setGuid("00000000-0000-0000-c000-000000000048");
        m_pNote->setUpdateSequenceNumber(1);
        m_pNote->setTitle("Fake note");
        m_pNote->setContent("Fake note content");
        m_pNote->setCreationTimestamp(1);
        m_pNote->setModificationTimestamp(1);
        m_pNote->setNotebookGuid(m_pNotebook->guid());
        m_pNote->setActive(true);

        m_state = STATE_SENT_ADD_NOTE_REQUEST;
        emit addNoteRequest(m_pNote, m_pNotebook);
    }
    HANDLE_WRONG_STATE();
}

void ResourceLocalStorageManagerAsyncTester::onAddNotebookFailed(QSharedPointer<Notebook> notebook, QString errorDescription)
{
    QNWARNING(errorDescription << ", Notebook: " << (notebook.isNull() ? QString("NULL") : notebook->ToQString()));
    emit failure(errorDescription);
}

void ResourceLocalStorageManagerAsyncTester::onAddNoteCompleted(QSharedPointer<Note> note, QSharedPointer<Notebook> notebook)
{
    Q_ASSERT_X(!note.isNull(), "ResourceLocalStorageManagerAsyncTester::onAddNoteCompleted slot",
               "Found NULL shared pointer to Note");

    QString errorDescription;

    if (m_state == STATE_SENT_ADD_NOTE_REQUEST)
    {
        if (m_pNote != note) {
            errorDescription = "Internal error in ResourceLocalStorageManagerAsyncTester: "
                               "note pointer in onAddNoteCompleted slot doesn't match "
                               "the pointer to the original Note";
            QNWARNING(errorDescription);
            emit failure(errorDescription);
            return;
        }

        m_pInitialResource = QSharedPointer<ResourceWrapper>(new ResourceWrapper);
        m_pInitialResource->setGuid("00000000-0000-0000-c000-000000000048");
        m_pInitialResource->setUpdateSequenceNumber(1);
        if (note->hasGuid()) {
            m_pInitialResource->setNoteGuid(note->guid());
        }
        if (!note->localGuid().isEmpty()) {
            m_pInitialResource->setNoteLocalGuid(note->localGuid());
        }
        m_pInitialResource->setIndexInNote(0);
        m_pInitialResource->setDataBody("Fake resource data body");
        m_pInitialResource->setDataSize(m_pInitialResource->dataBody().size());
        m_pInitialResource->setDataHash("Fake hash      1");

        m_pInitialResource->setRecognitionDataBody("Fake resource recognition data body");
        m_pInitialResource->setRecognitionDataSize(m_pInitialResource->recognitionDataBody().size());
        m_pInitialResource->setRecognitionDataHash("Fake hash      2");

        m_pInitialResource->setMime("text/plain");
        m_pInitialResource->setWidth(1);
        m_pInitialResource->setHeight(1);

        qevercloud::ResourceAttributes & attributes = m_pInitialResource->resourceAttributes();

        attributes.sourceURL = "Fake resource source URL";
        attributes.timestamp = 1;
        attributes.latitude = 0.0;
        attributes.longitude = 38.0;
        attributes.altitude = 12.0;
        attributes.cameraMake = "Fake resource camera make";
        attributes.cameraModel = "Fake resource camera model";
        attributes.fileName = "Fake resource file name";
        attributes.recoType = "unknown";

        attributes.applicationData = qevercloud::LazyMap();
        qevercloud::LazyMap & appData = attributes.applicationData.ref();
        appData.keysOnly = QSet<QString>();
        appData.keysOnly->insert("key_1");
        appData.keysOnly->insert("key_2");
        appData.keysOnly->insert("key_3");

        appData.fullMap = QMap<QString, QString>();
        appData.fullMap->operator[]("key_1") = "value_1";
        appData.fullMap->operator[]("key_2") = "value_2";
        appData.fullMap->operator[]("key_3") = "value_3";

        m_state = STATE_SENT_ADD_REQUEST;
        emit addResourceRequest(m_pInitialResource, m_pNote);
    }
    HANDLE_WRONG_STATE();
}

void ResourceLocalStorageManagerAsyncTester::onAddNoteFailed(QSharedPointer<Note> note, QSharedPointer<Notebook> notebook, QString errorDescription)
{
    QNWARNING(errorDescription << ", Note: " << (note.isNull() ? QString("NULL") : note->ToQString())
              << ", Notebook: " << (notebook.isNull() ? QString("NULL") : notebook->ToQString()));
    emit failure(errorDescription);
}

void ResourceLocalStorageManagerAsyncTester::onGetResourceCountCompleted(int count)
{
    // TODO: implement
    Q_UNUSED(count)
}

void ResourceLocalStorageManagerAsyncTester::onGetResourceCountFailed(QString errorDescription)
{
    QNWARNING(errorDescription);
    emit failure(errorDescription);
}

void ResourceLocalStorageManagerAsyncTester::onAddResourceCompleted(QSharedPointer<ResourceWrapper> resource)
{
    Q_ASSERT_X(!resource.isNull(), "ResourceLocalStorageManagerAsyncTester::onAddResourceCompleted slot",
               "Found NULL shared pointer to ResourceWrapper");

    QString errorDescription;

    if (m_state == STATE_SENT_ADD_REQUEST)
    {
        if (m_pInitialResource != resource) {
            errorDescription = "Internal error in ResourceLocalStorageManagerAsyncTester: "
                               "resource pointer in onAddResourceCompleted doesn't match "
                               "the pointer to the original Resource";
            QNWARNING(errorDescription);
            emit failure(errorDescription);
            return;
        }

        m_pFoundResource = QSharedPointer<ResourceWrapper>(new ResourceWrapper);
        m_pFoundResource->setLocalGuid(m_pInitialResource->localGuid());

        m_state = STATE_SENT_FIND_AFTER_ADD_REQUEST;
        bool withBinaryData = true;
        emit findResourceRequest(m_pFoundResource, withBinaryData);
    }
    HANDLE_WRONG_STATE();
}

void ResourceLocalStorageManagerAsyncTester::onAddResourceFailed(QSharedPointer<ResourceWrapper> resource, QString errorDescription)
{
    QNWARNING(errorDescription << ", Resource: " << (resource.isNull() ? QString("NULL") : resource->ToQString()));
    emit failure(errorDescription);
}

void ResourceLocalStorageManagerAsyncTester::onUpdateResourceCompleted(QSharedPointer<ResourceWrapper> resource)
{
    // TODO: implement
    Q_UNUSED(resource);
}

void ResourceLocalStorageManagerAsyncTester::onUpdateResourceFailed(QSharedPointer<ResourceWrapper> resource, QString errorDescription)
{
    QNWARNING(errorDescription << ", Resource: " << (resource.isNull() ? QString("NULL") : resource->ToQString()));
    emit failure(errorDescription);
}

void ResourceLocalStorageManagerAsyncTester::onFindResourceCompleted(QSharedPointer<ResourceWrapper> resource)
{
    // TODO: implement
    Q_UNUSED(resource);
}

void ResourceLocalStorageManagerAsyncTester::onFindResourceFailed(QSharedPointer<ResourceWrapper> resource, QString errorDescription)
{
    QNWARNING(errorDescription << ", Resource: " << (resource.isNull() ? QString("NULL") : resource->ToQString()));
    emit failure(errorDescription);
}

void ResourceLocalStorageManagerAsyncTester::onExpungeResourceCompleted(QSharedPointer<ResourceWrapper> resource)
{
    // TODO: implement
    Q_UNUSED(resource);
}

void ResourceLocalStorageManagerAsyncTester::onExpungeResourceFailed(QSharedPointer<ResourceWrapper> resource, QString errorDescription)
{
    QNWARNING(errorDescription << ", Resource: " << (resource.isNull() ? QString("NULL") : resource->ToQString()));
    emit failure(errorDescription);
}

void ResourceLocalStorageManagerAsyncTester::createConnections()
{
    // Request --> slot connections
    QObject::connect(this, SIGNAL(addNotebookRequest(QSharedPointer<Notebook>)),
                     m_pLocalStorageManagerThread, SLOT(onAddNotebookRequest(QSharedPointer<Notebook>)));
    QObject::connect(this, SIGNAL(addNoteRequest(QSharedPointer<Note>,QSharedPointer<Notebook>)),
                     m_pLocalStorageManagerThread, SLOT(onAddNoteRequest(QSharedPointer<Note>,QSharedPointer<Notebook>)));
    QObject::connect(this, SIGNAL(addResourceRequest(QSharedPointer<ResourceWrapper>,QSharedPointer<Note>)),
                     m_pLocalStorageManagerThread, SLOT(onAddResourceRequest(QSharedPointer<IResource>,QSharedPointer<Note>)));
    QObject::connect(this, SIGNAL(updateResourceRequest(QSharedPointer<ResourceWrapper>,QSharedPointer<Note>)),
                     m_pLocalStorageManagerThread, SLOT(onUpdateResourceRequest(QSharedPointer<IResource>,QSharedPointer<Note>)));
    QObject::connect(this, SIGNAL(findResourceRequest(QSharedPointer<ResourceWrapper>,bool)),
                     m_pLocalStorageManagerThread, SLOT(onFindResourceRequest(QSharedPointer<IResource>,bool)));
    QObject::connect(this, SIGNAL(getResourceCountRequest()), m_pLocalStorageManagerThread, SLOT(onGetResourceCountRequest()));
    QObject::connect(this, SIGNAL(expungeResourceRequest(QSharedPointer<ResourceWrapper>)),
                     m_pLocalStorageManagerThread, SLOT(onExpungeResourceRequest(QSharedPointer<IResource>)));

    // Slot <-- result connections
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(addNotebookComplete(QSharedPointer<Notebook>)),
                     this, SLOT(onAddNotebookCompleted(QSharedPointer<Notebook>)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(addNotebookFailed(QSharedPointer<Notebook>,QString)),
                     this, SLOT(onAddNotebookFailed(QSharedPointer<Notebook>,QString)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(addNoteComplete(QSharedPointer<Note>,QSharedPointer<Notebook>)),
                     this, SLOT(onAddNoteCompleted(QSharedPointer<Note>,QSharedPointer<Notebook>)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(addNoteFailed(QSharedPointer<Note>,QSharedPointer<Notebook>,QString)),
                     this, SLOT(onAddNoteFailed(QSharedPointer<Note>,QSharedPointer<Notebook>,QString)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(addResourceComplete(QSharedPointer<IResource>,QSharedPointer<Note>)),
                     this, SLOT(onAddResourceCompleted(QSharedPointer<ResourceWrapper>)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(addResourceFailed(QSharedPointer<IResource>,QSharedPointer<Note>,QString)),
                     this, SLOT(onAddResourceFailed(QSharedPointer<ResourceWrapper>,QString)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(updateResourceComplete(QSharedPointer<IResource>,QSharedPointer<Note>)),
                     this, SLOT(onUpdateResourceCompleted(QSharedPointer<ResourceWrapper>)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(updateResourceFailed(QSharedPointer<IResource>,QSharedPointer<Note>,QString)),
                     this, SLOT(onUpdateResourceFailed(QSharedPointer<ResourceWrapper>,QString)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(findResourceComplete(QSharedPointer<IResource>,bool)),
                     this, SLOT(onFindResourceCompleted(QSharedPointer<ResourceWrapper>)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(findResourceFailed(QSharedPointer<IResource>,bool,QString)),
                     this, SLOT(onFindResourceFailed(QSharedPointer<ResourceWrapper>,QString)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(expungeResourceComplete(QSharedPointer<IResource>)),
                     this, SLOT(onExpungeResourceCompleted(QSharedPointer<ResourceWrapper>)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(expungeResourceFailed(QSharedPointer<IResource>,QString)),
                     this, SLOT(onExpungeResourceFailed(QSharedPointer<ResourceWrapper>,QString)));
}

} // namespace test
} // namespace qute_note
