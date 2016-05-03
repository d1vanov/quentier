#include "ResourceLocalStorageManagerAsyncTester.h"
#include <qute_note/local_storage/LocalStorageManagerThreadWorker.h>
#include <qute_note/logging/QuteNoteLogger.h>
#include <QThread>

namespace qute_note {
namespace test {

ResourceLocalStorageManagerAsyncTester::ResourceLocalStorageManagerAsyncTester(QObject * parent) :
    QObject(parent),
    m_state(STATE_UNINITIALIZED),
    m_pLocalStorageManagerThreadWorker(Q_NULLPTR),
    m_pLocalStorageManagerThread(Q_NULLPTR),
    m_notebook(),
    m_note(),
    m_initialResource(),
    m_foundResource(),
    m_modifiedResource()
{}

ResourceLocalStorageManagerAsyncTester::~ResourceLocalStorageManagerAsyncTester()
{
    if (m_pLocalStorageManagerThread) {
        m_pLocalStorageManagerThread->quit();
        m_pLocalStorageManagerThread->wait();
    }

    if (m_pLocalStorageManagerThreadWorker) {
        delete m_pLocalStorageManagerThreadWorker;
    }
}

void ResourceLocalStorageManagerAsyncTester::onInitTestCase()
{
    QString username = "ResourceLocalStorageManagerAsyncTester";
    qint32 userId = 6;
    bool startFromScratch = true;
    bool overrideLock = false;

    if (m_pLocalStorageManagerThread) {
        delete m_pLocalStorageManagerThread;
        m_pLocalStorageManagerThread = Q_NULLPTR;
    }

    if (m_pLocalStorageManagerThreadWorker) {
        delete m_pLocalStorageManagerThreadWorker;
        m_pLocalStorageManagerThreadWorker = Q_NULLPTR;
    }

    m_state = STATE_UNINITIALIZED;

    m_pLocalStorageManagerThread = new QThread(this);
    m_pLocalStorageManagerThreadWorker = new LocalStorageManagerThreadWorker(username, userId, startFromScratch, overrideLock);
    m_pLocalStorageManagerThreadWorker->moveToThread(m_pLocalStorageManagerThread);

    createConnections();

    m_pLocalStorageManagerThread->start();
}

void ResourceLocalStorageManagerAsyncTester::onWorkerInitialized()
{
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

void ResourceLocalStorageManagerAsyncTester::onAddNotebookCompleted(Notebook notebook, QUuid requestId)
{
    Q_UNUSED(requestId)

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
        m_note.setNotebookLocalUid(m_notebook.localUid());
        m_note.setActive(true);

        m_state = STATE_SENT_ADD_NOTE_REQUEST;
        emit addNoteRequest(m_note);
    }
    HANDLE_WRONG_STATE();
}

void ResourceLocalStorageManagerAsyncTester::onAddNotebookFailed(Notebook notebook, QString errorDescription, QUuid requestId)
{
    QNWARNING(errorDescription << ", requestId = " << requestId << ", Notebook: " << notebook);
    emit failure(errorDescription);
}

void ResourceLocalStorageManagerAsyncTester::onAddNoteCompleted(Note note, QUuid requestId)
{
    Q_UNUSED(requestId)

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
        if (!note.localUid().isEmpty()) {
            m_initialResource.setNoteLocalUid(note.localUid());
        }
        m_initialResource.setIndexInNote(0);
        m_initialResource.setDataBody("Fake resource data body");
        m_initialResource.setDataSize(m_initialResource.dataBody().size());
        m_initialResource.setDataHash("Fake hash      1");

        m_initialResource.setRecognitionDataBody("<recoIndex docType=\"handwritten\" objType=\"image\" objID=\"fc83e58282d8059be17debabb69be900\" "
                                                 "engineVersion=\"5.5.22.7\" recoType=\"service\" lang=\"en\" objWidth=\"2398\" objHeight=\"1798\"> "
                                                 "<item x=\"437\" y=\"589\" w=\"1415\" h=\"190\">"
                                                 "<t w=\"87\">EVER ?</t>"
                                                 "<t w=\"83\">EVER NOTE</t>"
                                                 "<t w=\"82\">EVERNOTE</t>"
                                                 "<t w=\"71\">EVER NaTE</t>"
                                                 "<t w=\"67\">EVER nine</t>"
                                                 "<t w=\"67\">EVER none</t>"
                                                 "<t w=\"66\">EVER not</t>"
                                                 "<t w=\"62\">over NOTE</t>"
                                                 "<t w=\"62\">even NOTE</t>"
                                                 "<t w=\"61\">EVER nose</t>"
                                                 "<t w=\"50\">EV£RNoTE</t>"
                                                 "</item>"
                                                 "<item x=\"1850\" y=\"1465\" w=\"14\" h=\"12\">"
                                                 "<t w=\"11\">et</t>"
                                                 "<t w=\"10\">TQ</t>"
                                                 "</item>"
                                                 "</recoIndex>");
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
        auto & keysOnly = appData.keysOnly.ref();
        keysOnly.reserve(3);
        keysOnly.insert("key_1");
        keysOnly.insert("key_2");
        keysOnly.insert("key_3");

        appData.fullMap = QMap<QString, QString>();
        auto & fullMap = appData.fullMap.ref();
        fullMap = QMap<QString, QString>();
        fullMap["key_1"] = "value_1";
        fullMap["key_2"] = "value_2";
        fullMap["key_3"] = "value_3";

        m_state = STATE_SENT_ADD_REQUEST;
        emit addResourceRequest(m_initialResource, m_note);
    }
    HANDLE_WRONG_STATE();
}

void ResourceLocalStorageManagerAsyncTester::onAddNoteFailed(Note note, QString errorDescription, QUuid requestId)
{
    QNWARNING(errorDescription << ", requestId = " << requestId << ", Note: " << note);
    emit failure(errorDescription);
}

void ResourceLocalStorageManagerAsyncTester::onGetResourceCountCompleted(int count, QUuid requestId)
{
    Q_UNUSED(requestId)

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

void ResourceLocalStorageManagerAsyncTester::onGetResourceCountFailed(QString errorDescription, QUuid requestId)
{
    QNWARNING(errorDescription << ", requestId = " << requestId);
    emit failure(errorDescription);
}

void ResourceLocalStorageManagerAsyncTester::onAddResourceCompleted(ResourceWrapper resource, Note note, QUuid requestId)
{
    Q_UNUSED(requestId)
    Q_UNUSED(note)

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
        m_foundResource.setLocalUid(m_initialResource.localUid());

        m_state = STATE_SENT_FIND_AFTER_ADD_REQUEST;
        bool withBinaryData = true;
        emit findResourceRequest(m_foundResource, withBinaryData);
    }
    HANDLE_WRONG_STATE();
}

void ResourceLocalStorageManagerAsyncTester::onAddResourceFailed(ResourceWrapper resource,
                                                                 Note note, QString errorDescription, QUuid requestId)
{
    QNWARNING(errorDescription << ", requestId = " << requestId << ", Resource: " << resource << ", Note: " << note);
    emit failure(errorDescription);
}

void ResourceLocalStorageManagerAsyncTester::onUpdateResourceCompleted(ResourceWrapper resource, Note note, QUuid requestId)
{
    Q_UNUSED(requestId)
    Q_UNUSED(note)

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
        m_foundResource.setLocalUid(m_modifiedResource.localUid());

        m_state = STATE_SENT_FIND_AFTER_UPDATE_REQUEST;
        bool withBinaryData = false;    // test find without binary data, for a change
        emit findResourceRequest(m_foundResource, withBinaryData);
    }
    HANDLE_WRONG_STATE();
}

void ResourceLocalStorageManagerAsyncTester::onUpdateResourceFailed(ResourceWrapper resource,
                                                                    Note note, QString errorDescription, QUuid requestId)
{
    QNWARNING(errorDescription << ", requestId = " << requestId << ", Resource: " << resource << ", Note: " << note);
    emit failure(errorDescription);
}

void ResourceLocalStorageManagerAsyncTester::onFindResourceCompleted(ResourceWrapper resource,
                                                                     bool withBinaryData, QUuid requestId)
{
    Q_UNUSED(requestId)
    Q_UNUSED(withBinaryData)

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
                                                                  bool withBinaryData, QString errorDescription, QUuid requestId)
{
    if (m_state == STATE_SENT_FIND_AFTER_EXPUNGE_REQUEST) {
        m_state = STATE_SENT_GET_COUNT_AFTER_EXPUNGE_REQUEST;
        emit getResourceCountRequest();
        return;
    }

    QNWARNING(errorDescription << ", requestId = " << requestId << ", Resource: " << resource << ", withBinaryData = "
              << (withBinaryData ? "true" : "false"));
    emit failure(errorDescription);
}

void ResourceLocalStorageManagerAsyncTester::onExpungeResourceCompleted(ResourceWrapper resource, QUuid requestId)
{
    Q_UNUSED(requestId)

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

void ResourceLocalStorageManagerAsyncTester::onExpungeResourceFailed(ResourceWrapper resource, QString errorDescription, QUuid requestId)
{
    QNWARNING(errorDescription << ", requestId = " << requestId << ", Resource: " << resource);
    emit failure(errorDescription);
}

void ResourceLocalStorageManagerAsyncTester::createConnections()
{
    QObject::connect(m_pLocalStorageManagerThreadWorker, SIGNAL(failure(QString)),
                     this, SIGNAL(failure(QString)));

    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(started()), m_pLocalStorageManagerThreadWorker, SLOT(init()));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(finished()), m_pLocalStorageManagerThread, SLOT(deleteLater()));

    QObject::connect(m_pLocalStorageManagerThreadWorker, SIGNAL(initialized()), this, SLOT(onWorkerInitialized()));

    // Request --> slot connections
    QObject::connect(this, SIGNAL(addNotebookRequest(Notebook,QUuid)),
                     m_pLocalStorageManagerThreadWorker, SLOT(onAddNotebookRequest(Notebook,QUuid)));
    QObject::connect(this, SIGNAL(addNoteRequest(Note,QUuid)),
                     m_pLocalStorageManagerThreadWorker, SLOT(onAddNoteRequest(Note,QUuid)));
    QObject::connect(this, SIGNAL(addResourceRequest(ResourceWrapper,Note,QUuid)),
                     m_pLocalStorageManagerThreadWorker, SLOT(onAddResourceRequest(ResourceWrapper,Note,QUuid)));
    QObject::connect(this, SIGNAL(updateResourceRequest(ResourceWrapper,Note,QUuid)),
                     m_pLocalStorageManagerThreadWorker, SLOT(onUpdateResourceRequest(ResourceWrapper,Note,QUuid)));
    QObject::connect(this, SIGNAL(findResourceRequest(ResourceWrapper,bool,QUuid)),
                     m_pLocalStorageManagerThreadWorker, SLOT(onFindResourceRequest(ResourceWrapper,bool,QUuid)));
    QObject::connect(this, SIGNAL(getResourceCountRequest(QUuid)), m_pLocalStorageManagerThreadWorker, SLOT(onGetResourceCountRequest(QUuid)));
    QObject::connect(this, SIGNAL(expungeResourceRequest(ResourceWrapper,QUuid)),
                     m_pLocalStorageManagerThreadWorker, SLOT(onExpungeResourceRequest(ResourceWrapper,QUuid)));

    // Slot <-- result connections
    QObject::connect(m_pLocalStorageManagerThreadWorker, SIGNAL(addNotebookComplete(Notebook,QUuid)),
                     this, SLOT(onAddNotebookCompleted(Notebook,QUuid)));
    QObject::connect(m_pLocalStorageManagerThreadWorker, SIGNAL(addNotebookFailed(Notebook,QString,QUuid)),
                     this, SLOT(onAddNotebookFailed(Notebook,QString,QUuid)));
    QObject::connect(m_pLocalStorageManagerThreadWorker, SIGNAL(addNoteComplete(Note,QUuid)),
                     this, SLOT(onAddNoteCompleted(Note,QUuid)));
    QObject::connect(m_pLocalStorageManagerThreadWorker, SIGNAL(addNoteFailed(Note,QString,QUuid)),
                     this, SLOT(onAddNoteFailed(Note,QString,QUuid)));
    QObject::connect(m_pLocalStorageManagerThreadWorker, SIGNAL(addResourceComplete(ResourceWrapper,Note,QUuid)),
                     this, SLOT(onAddResourceCompleted(ResourceWrapper,Note,QUuid)));
    QObject::connect(m_pLocalStorageManagerThreadWorker, SIGNAL(addResourceFailed(ResourceWrapper,Note,QString,QUuid)),
                     this, SLOT(onAddResourceFailed(ResourceWrapper,Note,QString,QUuid)));
    QObject::connect(m_pLocalStorageManagerThreadWorker, SIGNAL(updateResourceComplete(ResourceWrapper,Note,QUuid)),
                     this, SLOT(onUpdateResourceCompleted(ResourceWrapper,Note,QUuid)));
    QObject::connect(m_pLocalStorageManagerThreadWorker, SIGNAL(updateResourceFailed(ResourceWrapper,Note,QString,QUuid)),
                     this, SLOT(onUpdateResourceFailed(ResourceWrapper,Note,QString,QUuid)));
    QObject::connect(m_pLocalStorageManagerThreadWorker, SIGNAL(findResourceComplete(ResourceWrapper,bool,QUuid)),
                     this, SLOT(onFindResourceCompleted(ResourceWrapper,bool,QUuid)));
    QObject::connect(m_pLocalStorageManagerThreadWorker, SIGNAL(findResourceFailed(ResourceWrapper,bool,QString,QUuid)),
                     this, SLOT(onFindResourceFailed(ResourceWrapper,bool,QString,QUuid)));
    QObject::connect(m_pLocalStorageManagerThreadWorker, SIGNAL(getResourceCountComplete(int,QUuid)),
                     this, SLOT(onGetResourceCountCompleted(int,QUuid)));
    QObject::connect(m_pLocalStorageManagerThreadWorker, SIGNAL(getResourceCountFailed(QString,QUuid)),
                     this, SLOT(onGetResourceCountFailed(QString,QUuid)));
    QObject::connect(m_pLocalStorageManagerThreadWorker, SIGNAL(expungeResourceComplete(ResourceWrapper,QUuid)),
                     this, SLOT(onExpungeResourceCompleted(ResourceWrapper,QUuid)));
    QObject::connect(m_pLocalStorageManagerThreadWorker, SIGNAL(expungeResourceFailed(ResourceWrapper,QString,QUuid)),
                     this, SLOT(onExpungeResourceFailed(ResourceWrapper,QString,QUuid)));
}

} // namespace test
} // namespace qute_note
