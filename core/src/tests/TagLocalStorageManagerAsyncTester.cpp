#include "TagLocalStorageManagerAsyncTester.h"
#include <client/local_storage/LocalStorageManagerThreadWorker.h>
#include <logging/QuteNoteLogger.h>
#include <QThread>

namespace qute_note {
namespace test {

TagLocalStorageManagerAsyncTester::TagLocalStorageManagerAsyncTester(QObject * parent) :
    QObject(parent),
    m_state(STATE_UNINITIALIZED),
    m_pLocalStorageManagerThreadWorker(nullptr),
    m_pLocalStorageManagerThread(nullptr),
    m_initialTag(),
    m_foundTag(),
    m_modifiedTag(),
    m_initialTags()
{}

TagLocalStorageManagerAsyncTester::~TagLocalStorageManagerAsyncTester()
{
    // NOTE: shouldn't attempt to delete m_pLocalStorageManagerThread as Qt's parent-child system
    // should take care of that
    if (m_pLocalStorageManagerThread) {
        m_pLocalStorageManagerThread->quit();
        m_pLocalStorageManagerThread->wait();
    }

    if (m_pLocalStorageManagerThreadWorker) {
        delete m_pLocalStorageManagerThreadWorker;
    }
}

void TagLocalStorageManagerAsyncTester::onInitTestCase()
{
    QString username = "TagLocalStorageManagerAsyncTester";
    qint32 userId = 2;
    bool startFromScratch = true;

    if (m_pLocalStorageManagerThreadWorker != nullptr) {
        delete m_pLocalStorageManagerThreadWorker;
        m_pLocalStorageManagerThreadWorker = nullptr;
    }

    m_state = STATE_UNINITIALIZED;

    m_pLocalStorageManagerThread = new QThread(this);
    m_pLocalStorageManagerThreadWorker = new LocalStorageManagerThreadWorker(username, userId, startFromScratch);
    m_pLocalStorageManagerThreadWorker->moveToThread(m_pLocalStorageManagerThread);

    createConnections();

    m_pLocalStorageManagerThread->start();
}

void TagLocalStorageManagerAsyncTester::onWorkerInitialized()
{
    m_initialTag = Tag();
    m_initialTag.setGuid("00000000-0000-0000-c000-000000000046");
    m_initialTag.setUpdateSequenceNumber(3);
    m_initialTag.setName("Fake tag name");

    QString errorDescription;
    if (!m_initialTag.checkParameters(errorDescription)) {
        QNWARNING("Found invalid Tag: " << m_initialTag << ", error: " << errorDescription);
        emit failure(errorDescription);
        return;
    }

    m_state = STATE_SENT_ADD_REQUEST;
    emit addTagRequest(m_initialTag);
}

void TagLocalStorageManagerAsyncTester::onGetTagCountCompleted(int count)
{
    QString errorDescription;

#define HANDLE_WRONG_STATE() \
    else { \
        errorDescription = "Internal error in TagLocalStorageManagerAsyncTester: " \
                           "found wrong state"; \
        emit failure(errorDescription); \
        return; \
    }

    if (m_state == STATE_SENT_GET_COUNT_AFTER_UPDATE_REQUEST)
    {
        if (count != 1) {
            errorDescription = "GetTagCount returned result different from the expected one (1): ";
            errorDescription += QString::number(count);
            QNWARNING(errorDescription);
            emit failure(errorDescription);
            return;
        }

        m_modifiedTag.setLocal(false);
        m_modifiedTag.setDeleted(true);
        m_state = STATE_SENT_DELETE_REQUEST;
        emit deleteTagRequest(m_modifiedTag);
    }
    else if (m_state == STATE_SENT_GET_COUNT_AFTER_EXPUNGE_REQUEST)
    {
        if (count != 0) {
            errorDescription = "GetTagCount returned result different from the expected one (0): ";
            errorDescription += QString::number(count);
            QNWARNING(errorDescription);
            emit failure(errorDescription);
            return;
        }

        Tag extraTag;
        extraTag.setGuid("00000000-0000-0000-c000-000000000001");
        extraTag.setUpdateSequenceNumber(1);
        extraTag.setName("Extra tag name one");

        m_state = STATE_SENT_ADD_EXTRA_TAG_ONE_REQUEST;
        emit addTagRequest(extraTag);
    }
    HANDLE_WRONG_STATE();
}

void TagLocalStorageManagerAsyncTester::onGetTagCountFailed(QString errorDescription)
{
    QNWARNING(errorDescription);
    emit failure(errorDescription);
}

void TagLocalStorageManagerAsyncTester::onAddTagCompleted(Tag tag)
{
    QString errorDescription;

    if (m_state == STATE_SENT_ADD_REQUEST)
    {
        if (m_initialTag != tag) {
            errorDescription = "Internal error in TagLocalStorageManagerAsyncTester: "
                               "tag in onAddTagCompleted slot doesn't match the original Tag";
            QNWARNING(errorDescription);
            emit failure(errorDescription);
            return;
        }

        m_foundTag = Tag();
        m_foundTag.setLocalGuid(tag.localGuid());

        m_state = STATE_SENT_FIND_AFTER_ADD_REQUEST;
        emit findTagRequest(m_foundTag);
    }
    else if (m_state == STATE_SENT_ADD_EXTRA_TAG_ONE_REQUEST)
    {
        m_initialTags << tag;

        Tag extraTag;
        extraTag.setGuid("00000000-0000-0000-c000-000000000002");
        extraTag.setUpdateSequenceNumber(2);
        extraTag.setName("Extra tag name two");
        extraTag.setParentGuid(tag.guid());

        m_state = STATE_SENT_ADD_EXTRA_TAG_TWO_REQUEST;
        emit addTagRequest(extraTag);
    }
    else if (m_state == STATE_SENT_ADD_EXTRA_TAG_TWO_REQUEST)
    {
        m_initialTags << tag;

        m_state = STATE_SENT_LIST_TAGS_REQUEST;
        emit listAllTagsRequest();
    }
    HANDLE_WRONG_STATE();
}

void TagLocalStorageManagerAsyncTester::onAddTagFailed(Tag tag, QString errorDescription)
{
    QNWARNING(errorDescription << ", Tag: " << tag);
    emit failure(errorDescription);
}

void TagLocalStorageManagerAsyncTester::onUpdateTagCompleted(Tag tag)
{
    QString errorDescription;

    if (m_state == STATE_SENT_UPDATE_REQUEST)
    {
        if (m_modifiedTag != tag) {
            errorDescription = "Internal error in TagLocalStorageManagerAsyncTester: "
                               "tag in onUpdateTagCompleted slot doesn't match "
                               "the original modified Tag";
            QNWARNING(errorDescription);
            emit failure(errorDescription);
            return;
        }

        m_state = STATE_SENT_FIND_AFTER_UPDATE_REQUEST;
        emit findTagRequest(m_foundTag);
    }
    HANDLE_WRONG_STATE();
}

void TagLocalStorageManagerAsyncTester::onUpdateTagFailed(Tag tag, QString errorDescription)
{
    QNWARNING(errorDescription << ", tag: " << tag);
    emit failure(errorDescription);
}

void TagLocalStorageManagerAsyncTester::onFindTagCompleted(Tag tag)
{
    QString errorDescription;

    if (m_state == STATE_SENT_FIND_AFTER_ADD_REQUEST)
    {
        if (tag != m_initialTag) {
            errorDescription = "Added and found tags in local storage don't match";
            QNWARNING(errorDescription << ": Tag added to LocalStorageManager: " << m_initialTag
                      << "\nTag found in LocalStorageManager: " << m_foundTag);
            emit failure(errorDescription);
            return;
        }

        // Attempt to find tag by name now
        Tag tagToFindByName;
        tagToFindByName.unsetLocalGuid();
        tagToFindByName.setName(m_initialTag.name());

        m_state = STATE_SENT_FIND_BY_NAME_AFTER_ADD_REQUEST;
        emit findTagRequest(tagToFindByName);
    }
    else if (m_state == STATE_SENT_FIND_BY_NAME_AFTER_ADD_REQUEST)
    {
        if (tag != m_initialTag) {
            errorDescription = "Added and found by name tags in local storage don't match";
            QNWARNING(errorDescription << ": Tag added to LocalStorageManager: " << m_initialTag
                      << "\nTag found in LocalStorageManager: " << m_foundTag);
            emit failure(errorDescription);
            return;
        }

        // Ok, found tag is good, updating it now
        m_modifiedTag = m_initialTag;
        m_modifiedTag.setUpdateSequenceNumber(m_initialTag.updateSequenceNumber() + 1);
        m_modifiedTag.setName(m_initialTag.name() + "_modified");

        m_state = STATE_SENT_UPDATE_REQUEST;
        emit updateTagRequest(m_modifiedTag);
    }
    else if (m_state == STATE_SENT_FIND_AFTER_UPDATE_REQUEST)
    {
        if (tag != m_modifiedTag) {
            errorDescription = "Updated and found tags in local storage don't match";
            QNWARNING(errorDescription << ": Tag updated in LocalStorageManager: " << m_modifiedTag
                      << "\nTag found in LocalStorageManager: " << m_foundTag);
            emit failure(errorDescription);
            return;
        }

        m_state = STATE_SENT_GET_COUNT_AFTER_UPDATE_REQUEST;
        emit getTagCountRequest();
    }
    else if (m_state == STATE_SENT_FIND_AFTER_EXPUNGE_REQUEST)
    {
        errorDescription = "Found tag which should have been expunged from local storage";
        QNWARNING(errorDescription << ": Tag expunged from LocalStorageManager: " << m_modifiedTag
                  << "\nTag found in LocalStorageManager: " << m_foundTag);
        emit failure(errorDescription);
        return;
    }
    HANDLE_WRONG_STATE();
}

void TagLocalStorageManagerAsyncTester::onFindTagFailed(Tag tag, QString errorDescription)
{
    if (m_state == STATE_SENT_FIND_AFTER_EXPUNGE_REQUEST) {
        m_state = STATE_SENT_GET_COUNT_AFTER_EXPUNGE_REQUEST;
        emit getTagCountRequest();
        return;
    }

    QNWARNING(errorDescription << ", tag: " << tag);
    emit failure(errorDescription);
}

void TagLocalStorageManagerAsyncTester::onListAllTagsCompleted(QList<Tag> tags)
{
    int numInitialTags = m_initialTags.size();
    int numFoundTags = tags.size();

    QString errorDescription;

    if (numInitialTags != numFoundTags) {
        errorDescription = "Error: number of found tags does not correspond "
                           "to the number of original added tags";
        QNWARNING(errorDescription);
        emit failure(errorDescription);
        return;
    }

    foreach(const Tag & tag, m_initialTags)
    {
        if (!tags.contains(tag)) {
            errorDescription = "One of initial tags was not found within found tags";
            QNWARNING(errorDescription);
            emit failure(errorDescription);
            return;
        }
    }

    emit success();
}

void TagLocalStorageManagerAsyncTester::onListAllTagsFailed(QString errorDescription)
{
    QNWARNING(errorDescription);
    emit failure(errorDescription);
}

void TagLocalStorageManagerAsyncTester::onDeleteTagCompleted(Tag tag)
{
    QString errorDescription;

    if (m_modifiedTag != tag) {
        errorDescription = "Internal error in TagLocalStorageManagerAsyncTester: "
                           "tag in onDeleteTagCompleted slot doesn't match "
                           "the original deleted Tag";
        QNWARNING(errorDescription);
        emit failure(errorDescription);
        return;
    }

    m_modifiedTag.setLocal(true);
    m_state = STATE_SENT_EXPUNGE_REQUEST;
    emit expungeTagRequest(m_modifiedTag);
}

void TagLocalStorageManagerAsyncTester::onDeleteTagFailed(Tag tag, QString errorDescription)
{
    QNWARNING(errorDescription  << ", tag: " << tag);
    emit failure(errorDescription);
}

void TagLocalStorageManagerAsyncTester::onExpungeTagCompleted(Tag tag)
{
    QString errorDescription;

    if (m_modifiedTag != tag) {
        errorDescription = "Internal error in TagLocalStorageManagerAsyncTester: "
                           "tag in onExpungeTagCompleted slot doesn't match "
                           "the original expunged Tag";
        QNWARNING(errorDescription);
        emit failure(errorDescription);
        return;
    }

    m_state = STATE_SENT_FIND_AFTER_EXPUNGE_REQUEST;
    emit findTagRequest(m_foundTag);
}

void TagLocalStorageManagerAsyncTester::onExpungeTagFailed(Tag tag, QString errorDescription)
{
    QNWARNING(errorDescription << ", Tag: " << tag);
    emit failure(errorDescription);
}

void TagLocalStorageManagerAsyncTester::createConnections()
{
    QObject::connect(m_pLocalStorageManagerThreadWorker, SIGNAL(failure(QString)),
                     this, SIGNAL(failure(QString)));

    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(started()), m_pLocalStorageManagerThreadWorker, SLOT(init()));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(finished()), m_pLocalStorageManagerThread, SLOT(deleteLater()));

    QObject::connect(m_pLocalStorageManagerThreadWorker, SIGNAL(initialized()), this, SLOT(onWorkerInitialized()));

    // Request --> slot connections
    QObject::connect(this, SIGNAL(getTagCountRequest()), m_pLocalStorageManagerThreadWorker,
                     SLOT(onGetTagCountRequest()));
    QObject::connect(this, SIGNAL(addTagRequest(Tag)), m_pLocalStorageManagerThreadWorker,
                     SLOT(onAddTagRequest(Tag)));
    QObject::connect(this, SIGNAL(updateTagRequest(Tag)), m_pLocalStorageManagerThreadWorker,
                     SLOT(onUpdateTagRequest(Tag)));
    QObject::connect(this, SIGNAL(findTagRequest(Tag)), m_pLocalStorageManagerThreadWorker,
                     SLOT(onFindTagRequest(Tag)));
    QObject::connect(this, SIGNAL(listAllTagsRequest()), m_pLocalStorageManagerThreadWorker,
                     SLOT(onListAllTagsRequest()));
    QObject::connect(this, SIGNAL(deleteTagRequest(Tag)),
                     m_pLocalStorageManagerThreadWorker, SLOT(onDeleteTagRequest(Tag)));
    QObject::connect(this, SIGNAL(expungeTagRequest(Tag)),
                     m_pLocalStorageManagerThreadWorker, SLOT(onExpungeTagRequest(Tag)));

    // Slot <-- result connections
    QObject::connect(m_pLocalStorageManagerThreadWorker, SIGNAL(getTagCountComplete(int)),
                     this, SLOT(onGetTagCountCompleted(int)));
    QObject::connect(m_pLocalStorageManagerThreadWorker, SIGNAL(getTagCountFailed(QString)),
                     this, SLOT(onGetTagCountFailed(QString)));
    QObject::connect(m_pLocalStorageManagerThreadWorker, SIGNAL(addTagComplete(Tag)),
                     this, SLOT(onAddTagCompleted(Tag)));
    QObject::connect(m_pLocalStorageManagerThreadWorker, SIGNAL(addTagFailed(Tag,QString)),
                     this, SLOT(onAddTagFailed(Tag,QString)));
    QObject::connect(m_pLocalStorageManagerThreadWorker, SIGNAL(updateTagComplete(Tag)),
                     this, SLOT(onUpdateTagCompleted(Tag)));
    QObject::connect(m_pLocalStorageManagerThreadWorker, SIGNAL(updateTagFailed(Tag,QString)),
                     this, SLOT(onUpdateTagFailed(Tag,QString)));
    QObject::connect(m_pLocalStorageManagerThreadWorker, SIGNAL(findTagComplete(Tag)),
                     this, SLOT(onFindTagCompleted(Tag)));
    QObject::connect(m_pLocalStorageManagerThreadWorker, SIGNAL(findTagFailed(Tag,QString)),
                     this, SLOT(onFindTagFailed(Tag,QString)));
    QObject::connect(m_pLocalStorageManagerThreadWorker, SIGNAL(listAllTagsComplete(QList<Tag>)),
                     this, SLOT(onListAllTagsCompleted(QList<Tag>)));
    QObject::connect(m_pLocalStorageManagerThreadWorker, SIGNAL(listAllTagsFailed(QString)),
                     this, SLOT(onListAllTagsFailed(QString)));
    QObject::connect(m_pLocalStorageManagerThreadWorker, SIGNAL(deleteTagComplete(Tag)),
                     this, SLOT(onDeleteTagCompleted(Tag)));
    QObject::connect(m_pLocalStorageManagerThreadWorker, SIGNAL(deleteTagFailed(Tag,QString)),
                     this, SLOT(onDeleteTagFailed(Tag,QString)));
    QObject::connect(m_pLocalStorageManagerThreadWorker, SIGNAL(expungeTagComplete(Tag)),
                     this, SLOT(onExpungeTagCompleted(Tag)));
    QObject::connect(m_pLocalStorageManagerThreadWorker, SIGNAL(expungeTagFailed(Tag,QString)),
                     this, SLOT(onExpungeTagFailed(Tag,QString)));
}

#undef HANDLE_WRONG_STATE

} // namespace test
} // namespace qute_note
