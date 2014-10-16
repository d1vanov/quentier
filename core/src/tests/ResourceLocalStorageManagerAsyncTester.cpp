#include "ResourceLocalStorageManagerAsyncTester.h"
#include <client/local_storage/LocalStorageManagerThread.h>
#include <logging/QuteNoteLogger.h>

namespace qute_note {
namespace test {

ResourceLocalStorageManagerAsyncTester::ResourceLocalStorageManagerAsyncTester(QObject * parent) :
    QObject(parent),
    m_state(STATE_UNINITIALIZED),
    m_pLocalStorageManagerThread(nullptr),
    m_notebook(),
    m_note(),
    m_initialResource(),
    m_foundResource(),
    m_modifiedResource()
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

    m_notebook = Notebook();
    m_notebook.setGuid("00000000-0000-0000-c000-000000000047");
    m_notebook.setUpdateSequenceNumber(1);
    m_notebook.setName("Fake notebook name");
    m_notebook.setCreationTimestamp(1);
    m_notebook.setModificationTimestamp(1);
    m_notebook.setDefaultNotebook(true);
    m_notebook.setLastUsed(false);
    m_notebook.setPublishingUri("Fake publishing uri");
    m_notebook.setPublishingOrder(1);
    m_notebook.setPublishingAscending(true);
    m_notebook.setPublishingPublicDescription("Fake public description");
    m_notebook.setPublished(true);
    m_notebook.setStack("Fake notebook stack");
    m_notebook.setBusinessNotebookDescription("Fake business notebook description");
    m_notebook.setBusinessNotebookPrivilegeLevel(1);
    m_notebook.setBusinessNotebookRecommended(true);

    QString errorDescription;
    if (!m_notebook.checkParameters(errorDescription)) {
        QNWARNING("Found invalid notebook: " << m_notebook << ", error: " << errorDescription);
        emit failure(errorDescription);
        return;
    }

    m_state = STATE_SENT_ADD_NOTEBOOK_REQUEST;
    emit addNotebookRequest(m_notebook);
}

void ResourceLocalStorageManagerAsyncTester::onAddNotebookCompleted(Notebook notebook)
{
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
        if (m_notebook != notebook) {
            errorDescription = "Internal error in ResourceLocalStorageManagerAsyncTester: "
                               "notebook in onAddNotebookCompleted slot doesn't match "
                               "the original Notebook";
            QNWARNING(errorDescription);
            emit failure(errorDescription);
            return;
        }

        m_note = Note();
        m_note.setGuid("00000000-0000-0000-c000-000000000048");
        m_note.setUpdateSequenceNumber(1);
        m_note.setTitle("Fake note");
        m_note.setContent("<en-note><h1>Hello, world</h1></en-note>");
        m_note.setCreationTimestamp(1);
        m_note.setModificationTimestamp(1);
        m_note.setNotebookGuid(m_notebook.guid());
        m_note.setActive(true);

        m_state = STATE_SENT_ADD_NOTE_REQUEST;
        emit addNoteRequest(m_note, m_notebook);
    }
    HANDLE_WRONG_STATE();
}

void ResourceLocalStorageManagerAsyncTester::onAddNotebookFailed(Notebook notebook, QString errorDescription)
{
    QNWARNING(errorDescription << ", Notebook: " << notebook);
    emit failure(errorDescription);
}

void ResourceLocalStorageManagerAsyncTester::onAddNoteCompleted(Note note, Notebook notebook)
{
    QString errorDescription;

    if (m_state == STATE_SENT_ADD_NOTE_REQUEST)
    {
        if (m_note != note) {
            errorDescription = "Internal error in ResourceLocalStorageManagerAsyncTester: "
                               "note in onAddNoteCompleted slot doesn't match "
                               "the original Note";
            QNWARNING(errorDescription);
            emit failure(errorDescription);
            return;
        }

        m_initialResource.setGuid("00000000-0000-0000-c000-000000000048");
        m_initialResource.setUpdateSequenceNumber(1);
        if (note.hasGuid()) {
            m_initialResource.setNoteGuid(note.guid());
        }
        if (!note.localGuid().isEmpty()) {
            m_initialResource.setNoteLocalGuid(note.localGuid());
        }
        m_initialResource.setIndexInNote(0);
        m_initialResource.setDataBody("Fake resource data body");
        m_initialResource.setDataSize(m_initialResource.dataBody().size());
        m_initialResource.setDataHash("Fake hash      1");

        m_initialResource.setRecognitionDataBody("Fake resource recognition data body");
        m_initialResource.setRecognitionDataSize(m_initialResource.recognitionDataBody().size());
        m_initialResource.setRecognitionDataHash("Fake hash      2");

        m_initialResource.setMime("text/plain");
        m_initialResource.setWidth(1);
        m_initialResource.setHeight(1);

        qevercloud::ResourceAttributes & attributes = m_initialResource.resourceAttributes();

        attributes.sourceURL = "Fake resource source URL";
        attributes.timestamp = 1;
        attributes.latitude = 0.0;
        attributes.longitude = 38.0;
        attributes.altitude = 12.0;
        attributes.cameraMake = "Fake resource camera make";
        attributes.cameraModel = "Fake resource camera model";
        attributes.fileName = "Fake resource file name";

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
        emit addResourceRequest(m_initialResource, m_note);
    }
    HANDLE_WRONG_STATE();
}

void ResourceLocalStorageManagerAsyncTester::onAddNoteFailed(Note note, Notebook notebook, QString errorDescription)
{
    QNWARNING(errorDescription << ", Note: " << note << ", Notebook: " << notebook);
    emit failure(errorDescription);
}

void ResourceLocalStorageManagerAsyncTester::onGetResourceCountCompleted(int count)
{
    QString errorDescription;

    if (m_state == STATE_SENT_GET_COUNT_AFTER_UPDATE_REQUEST)
    {
        if (count != 1) {
            errorDescription = "GetResourceCount returned result different from the expected one (1): ";
            errorDescription += QString::number(count);
            QNWARNING(errorDescription);
            emit failure(errorDescription);
            return;
        }

        m_state = STATE_SENT_EXPUNGE_REQUEST;
        emit expungeResourceRequest(m_modifiedResource);
    }
    else if (m_state == STATE_SENT_GET_COUNT_AFTER_EXPUNGE_REQUEST)
    {
        if (count != 0) {
            errorDescription = "GetResourceCount returned result different from the expected one (0): ";
            errorDescription += QString::number(count);
            QNWARNING(errorDescription);
            emit failure(errorDescription);
            return;
        }

        emit success();
    }
    HANDLE_WRONG_STATE();
}

void ResourceLocalStorageManagerAsyncTester::onGetResourceCountFailed(QString errorDescription)
{
    QNWARNING(errorDescription);
    emit failure(errorDescription);
}

void ResourceLocalStorageManagerAsyncTester::onAddResourceCompleted(ResourceWrapper resource)
{
    QString errorDescription;
    if (m_state == STATE_SENT_ADD_REQUEST)
    {
        if (m_initialResource != resource) {
            errorDescription = "Internal error in ResourceLocalStorageManagerAsyncTester: "
                               "resource in onAddResourceCompleted doesn't match "
                               "the original Resource";
            QNWARNING(errorDescription);
            emit failure(errorDescription);
            return;
        }

        m_foundResource.clear();
        m_foundResource.setLocalGuid(m_initialResource.localGuid());

        m_state = STATE_SENT_FIND_AFTER_ADD_REQUEST;
        bool withBinaryData = true;
        emit findResourceRequest(m_foundResource, withBinaryData);
    }
    HANDLE_WRONG_STATE();
}

void ResourceLocalStorageManagerAsyncTester::onAddResourceFailed(ResourceWrapper resource,
                                                                 Note note, QString errorDescription)
{
    QNWARNING(errorDescription << ", Resource: " << resource << ", Note: " << note);
    emit failure(errorDescription);
}

void ResourceLocalStorageManagerAsyncTester::onUpdateResourceCompleted(ResourceWrapper resource)
{
    QString errorDescription;

    if (m_state == STATE_SENT_UPDATE_REQUEST)
    {
        if (m_modifiedResource != resource) {
            errorDescription = "Internal error in ResourceLocalStorageManagerAsyncTester: "
                               "resource in onUpdateResourceCompleted doesn't match "
                               "the original Resource";
            QNWARNING(errorDescription);
            emit failure(errorDescription);
            return;
        }

        m_foundResource.clear();
        m_foundResource.setLocalGuid(m_modifiedResource.localGuid());

        m_state = STATE_SENT_FIND_AFTER_UPDATE_REQUEST;
        bool withBinaryData = false;    // test find without binary data, for a change
        emit findResourceRequest(m_foundResource, withBinaryData);
    }
    HANDLE_WRONG_STATE();
}

void ResourceLocalStorageManagerAsyncTester::onUpdateResourceFailed(ResourceWrapper resource,
                                                                    Note note, QString errorDescription)
{
    QNWARNING(errorDescription << ", Resource: " << resource << ", Note: " << note);
    emit failure(errorDescription);
}

void ResourceLocalStorageManagerAsyncTester::onFindResourceCompleted(ResourceWrapper resource,
                                                                     bool withBinaryData)
{
    QString errorDescription;

    if (m_state == STATE_SENT_FIND_AFTER_ADD_REQUEST)
    {
        if (resource != m_initialResource) {
            errorDescription = "Added and found resources in local storage don't match";
            QNWARNING(errorDescription << ": Resource added to LocalStorageManager: " << m_initialResource
                      << "\nResource found in LocalStorageManager: " << resource);
            emit failure(errorDescription);
            return;
        }

        // Ok, found resource is good, updating it now
        m_modifiedResource = m_initialResource;
        m_modifiedResource.setUpdateSequenceNumber(m_initialResource.updateSequenceNumber() + 1);
        m_modifiedResource.setHeight(m_initialResource.height() + 1);
        m_modifiedResource.setWidth(m_initialResource.width() + 1);

        qevercloud::ResourceAttributes & attributes = m_modifiedResource.resourceAttributes();
        attributes.cameraMake.ref() += "_modified";
        attributes.cameraModel.ref() += "_modified";

        m_state = STATE_SENT_UPDATE_REQUEST;
        emit updateResourceRequest(m_modifiedResource, m_note);
    }
    else if (m_state == STATE_SENT_FIND_AFTER_UPDATE_REQUEST)
    {
        // Find after update was called without binary data, so need to remove it from modified
        // resource prior to the comparison
        if (m_modifiedResource.hasDataBody()) {
            m_modifiedResource.setDataBody(QByteArray());
        }

        if (m_modifiedResource.hasRecognitionDataBody()) {
            m_modifiedResource.setRecognitionDataBody(QByteArray());
        }

        if (resource != m_modifiedResource) {
            errorDescription = "Updated and found resources in local storage don't match";
            QNWARNING(errorDescription << ": Resource updated in LocalStorageManager: " << m_modifiedResource
                      << "\nResource found in LocalStorageManager: " << resource);
            emit failure(errorDescription);
            return;
        }

        m_state = STATE_SENT_GET_COUNT_AFTER_UPDATE_REQUEST;
        emit getResourceCountRequest();
    }
    else if (m_state == STATE_SENT_FIND_AFTER_EXPUNGE_REQUEST)
    {
        errorDescription = "Found resource which should have been expunged from local storage";
        QNWARNING(errorDescription << ": Resource expunged from LocalStorageManager: " << m_modifiedResource
                  << "\nResource fond in LocalStorageManager: " << resource);
        emit failure(errorDescription);
        return;
    }
    HANDLE_WRONG_STATE();
}

void ResourceLocalStorageManagerAsyncTester::onFindResourceFailed(ResourceWrapper resource,
                                                                  bool withBinaryData, QString errorDescription)
{
    if (m_state == STATE_SENT_FIND_AFTER_EXPUNGE_REQUEST) {
        m_state = STATE_SENT_GET_COUNT_AFTER_EXPUNGE_REQUEST;
        emit getResourceCountRequest();
        return;
    }

    QNWARNING(errorDescription << ", Resource: " << resource << ", withBinaryData = "
              << (withBinaryData ? "true" : "false"));
    emit failure(errorDescription);
}

void ResourceLocalStorageManagerAsyncTester::onExpungeResourceCompleted(ResourceWrapper resource)
{
    QString errorDescription;

    if (m_modifiedResource != resource) {
        errorDescription = "Internal error in ResourceLocalStorageManagerAsyncTester: "
                           "resource in onExpungeResourceCompleted slot doesn't match "
                           "the original expunged Resource";
        QNWARNING(errorDescription);
        emit failure(errorDescription);
        return;
    }

    m_state = STATE_SENT_FIND_AFTER_EXPUNGE_REQUEST;
    bool withBinaryData = true;
    emit findResourceRequest(m_foundResource, withBinaryData);
}

void ResourceLocalStorageManagerAsyncTester::onExpungeResourceFailed(ResourceWrapper resource, QString errorDescription)
{
    QNWARNING(errorDescription << ", Resource: " << resource);
    emit failure(errorDescription);
}

void ResourceLocalStorageManagerAsyncTester::createConnections()
{
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(failure(QString)),
                     this, SIGNAL(failure(QString)));

    // Request --> slot connections
    QObject::connect(this, SIGNAL(addNotebookRequest(Notebook)),
                     m_pLocalStorageManagerThread, SLOT(onAddNotebookRequest(Notebook)));
    QObject::connect(this, SIGNAL(addNoteRequest(Note,Notebook)),
                     m_pLocalStorageManagerThread, SLOT(onAddNoteRequest(Note,Notebook)));
    QObject::connect(this, SIGNAL(addResourceRequest(ResourceWrapper,Note)),
                     m_pLocalStorageManagerThread, SLOT(onAddResourceRequest(ResourceWrapper,Note)));
    QObject::connect(this, SIGNAL(updateResourceRequest(ResourceWrapper,Note)),
                     m_pLocalStorageManagerThread, SLOT(onUpdateResourceRequest(ResourceWrapper,Note)));
    QObject::connect(this, SIGNAL(findResourceRequest(ResourceWrapper,bool)),
                     m_pLocalStorageManagerThread, SLOT(onFindResourceRequest(ResourceWrapper,bool)));
    QObject::connect(this, SIGNAL(getResourceCountRequest()), m_pLocalStorageManagerThread, SLOT(onGetResourceCountRequest()));
    QObject::connect(this, SIGNAL(expungeResourceRequest(ResourceWrapper)),
                     m_pLocalStorageManagerThread, SLOT(onExpungeResourceRequest(ResourceWrapper)));

    // Slot <-- result connections
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(addNotebookComplete(Notebook)),
                     this, SLOT(onAddNotebookCompleted(Notebook)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(addNotebookFailed(Notebook,QString)),
                     this, SLOT(onAddNotebookFailed(Notebook,QString)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(addNoteComplete(Note,Notebook)),
                     this, SLOT(onAddNoteCompleted(Note,Notebook)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(addNoteFailed(Note,Notebook,QString)),
                     this, SLOT(onAddNoteFailed(Note,Notebook,QString)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(addResourceComplete(ResourceWrapper,Note)),
                     this, SLOT(onAddResourceCompleted(ResourceWrapper)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(addResourceFailed(ResourceWrapper,Note,QString)),
                     this, SLOT(onAddResourceFailed(ResourceWrapper,Note,QString)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(updateResourceComplete(ResourceWrapper,Note)),
                     this, SLOT(onUpdateResourceCompleted(ResourceWrapper)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(updateResourceFailed(ResourceWrapper,Note,QString)),
                     this, SLOT(onUpdateResourceFailed(ResourceWrapper,Note,QString)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(findResourceComplete(ResourceWrapper,bool)),
                     this, SLOT(onFindResourceCompleted(ResourceWrapper,bool)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(findResourceFailed(ResourceWrapper,bool,QString)),
                     this, SLOT(onFindResourceFailed(ResourceWrapper,bool,QString)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(getResourceCountComplete(int)),
                     this, SLOT(onGetResourceCountCompleted(int)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(getResourceCountFailed(QString)),
                     this, SLOT(onGetResourceCountFailed(QString)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(expungeResourceComplete(ResourceWrapper)),
                     this, SLOT(onExpungeResourceCompleted(ResourceWrapper)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(expungeResourceFailed(ResourceWrapper,QString)),
                     this, SLOT(onExpungeResourceFailed(ResourceWrapper,QString)));
}

} // namespace test
} // namespace qute_note
