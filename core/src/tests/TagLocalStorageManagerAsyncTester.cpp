#include "TagLocalStorageManagerAsyncTester.h"
#include <client/local_storage/LocalStorageManagerThread.h>
#include <logging/QuteNoteLogger.h>

namespace qute_note {
namespace test {

TagLocalStorageManagerAsyncTester::TagLocalStorageManagerAsyncTester(QObject * parent) :
    QObject(parent),
    m_state(STATE_UNINITIALIZED),
    m_pLocalStorageManagerThread(nullptr),
    m_pInitialTag(),
    m_pFoundTag(),
    m_pModifiedTag(),
    m_initialTags()
{}

TagLocalStorageManagerAsyncTester::~TagLocalStorageManagerAsyncTester()
{
    // NOTE: shouldn't attempt to delete m_pLocalStorageManagerThread as Qt's parent-child system
    // should take care of that
}

void TagLocalStorageManagerAsyncTester::onInitTestCase()
{
    QString username = "TagLocalStorageManagerAsyncTester";
    qint32 userId = 2;
    bool startFromScratch = true;

    if (m_pLocalStorageManagerThread != nullptr) {
        delete m_pLocalStorageManagerThread;
        m_pLocalStorageManagerThread = nullptr;
    }

    m_state = STATE_UNINITIALIZED;

    m_pLocalStorageManagerThread = new LocalStorageManagerThread(username, userId, startFromScratch, this);
    createConnections();

    m_pInitialTag = QSharedPointer<Tag>(new Tag);
    m_pInitialTag->setGuid("00000000-0000-0000-c000-000000000046");
    m_pInitialTag->setUpdateSequenceNumber(3);
    m_pInitialTag->setName("Fake tag name");

    QString errorDescription;
    if (!m_pInitialTag->checkParameters(errorDescription)) {
        QNWARNING("Found invalid Tag: " << *m_pInitialTag << ", error: " << errorDescription);
        emit failure(errorDescription);
        return;
    }

    m_state = STATE_SENT_ADD_REQUEST;
    emit addTagRequest(m_pInitialTag);
}

void TagLocalStorageManagerAsyncTester::onGetTagCountCompleted(int count)
{
    QString errorDescription;

#define HANDLE_WRONG_STATE() \
    else { \
        errorDescription = QObject::tr("Internal error in TagLocalStorageManagerAsyncTester: " \
                                       "found wrong state"); \
        emit failure(errorDescription); \
        return; \
    }

    if (m_state == STATE_SENT_GET_COUNT_AFTER_UPDATE_REQUEST)
    {
        if (count != 1) {
            errorDescription = QObject::tr("GetTagCount returned result different from the expected one (1): ");
            errorDescription += QString::number(count);
            emit failure(errorDescription);
            return;
        }

        m_pModifiedTag->setDeleted(true);

        m_state = STATE_SENT_EXPUNGE_REQUEST;
        emit expungeTagRequest(m_pModifiedTag);
    }
    else if (m_state == STATE_SENT_GET_COUNT_AFTER_EXPUNGE_REQUEST)
    {
        if (count != 0) {
            errorDescription = QObject::tr("GetTagCount returned result different from the expected one (0): ");
            errorDescription += QString::number(count);
            emit failure(errorDescription);
            return;
        }

        QSharedPointer<Tag> extraTag = QSharedPointer<Tag>(new Tag);
        extraTag->setGuid("00000000-0000-0000-c000-000000000001");
        extraTag->setUpdateSequenceNumber(1);
        extraTag->setName("Extra tag name one");

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

void TagLocalStorageManagerAsyncTester::onAddTagCompleted(QSharedPointer<Tag> tag)
{
    Q_ASSERT_X(!tag.isNull(), "TagLocalStorageManagerAsyncTester::onAddTagCompleted slot",
               "Found NULL shared pointer to Tag");

    QString errorDescription;

    if (m_state == STATE_SENT_ADD_REQUEST)
    {
        if (m_pInitialTag != tag) {
            errorDescription = "Internal error in TagLocalStorageManagerAsyncTester: "
                               "tag in onAddTagCompleted slot doesn't match the original Tag";
            QNWARNING(errorDescription);
            errorDescription = QObject::tr(qPrintable(errorDescription));
            emit failure(errorDescription);
            return;
        }

        m_pFoundTag = QSharedPointer<Tag>(new Tag);
        m_pFoundTag->setLocalGuid(tag->localGuid());

        m_state = STATE_SENT_FIND_AFTER_ADD_REQUEST;
        emit findTagRequest(m_pFoundTag);
    }
    else if (m_state == STATE_SENT_ADD_EXTRA_TAG_ONE_REQUEST)
    {
        m_initialTags << *tag;

        QSharedPointer<Tag> extraTag = QSharedPointer<Tag>(new Tag);
        extraTag->setGuid("00000000-0000-0000-c000-000000000002");
        extraTag->setUpdateSequenceNumber(2);
        extraTag->setName("Extra tag name two");
        extraTag->setParentGuid(tag->guid());

        m_state = STATE_SENT_ADD_EXTRA_TAG_TWO_REQUEST;
        emit addTagRequest(extraTag);
    }
    else if (m_state == STATE_SENT_ADD_EXTRA_TAG_TWO_REQUEST)
    {
        m_initialTags << *tag;

        m_state = STATE_SENT_LIST_TAGS_REQUEST;
        emit listAllTagsRequest();
    }
    HANDLE_WRONG_STATE();
}

void TagLocalStorageManagerAsyncTester::onAddTagFailed(QSharedPointer<Tag> tag, QString errorDescription)
{
    QNWARNING(errorDescription << ", Tag: " << (tag.isNull() ? QString("NULL") : tag->ToQString()));
    emit failure(errorDescription);
}

void TagLocalStorageManagerAsyncTester::onUpdateTagCompleted(QSharedPointer<Tag> tag)
{
    Q_ASSERT_X(!tag.isNull(), "TagLocalStorageManagerAsyncTester::onUpdateTagCompleted slot",
               "Found NULL shared pointer to Tag");

    QString errorDescription;

    if (m_state == STATE_SENT_UPDATE_REQUEST)
    {
        if (m_pModifiedTag != tag) {
            errorDescription = "Internal error in TagLocalStorageManagerAsyncTester: "
                               "tag pointer in onUpdateTagCompleted slot doesn't match "
                               "the pointer to the original modified Tag";
            QNWARNING(errorDescription);
            errorDescription = QObject::tr(qPrintable(errorDescription));
            emit failure(errorDescription);
            return;
        }

        m_state = STATE_SENT_FIND_AFTER_UPDATE_REQUEST;
        emit findTagRequest(m_pFoundTag);
    }
    HANDLE_WRONG_STATE();
}

void TagLocalStorageManagerAsyncTester::onUpdateTagFailed(QSharedPointer<Tag> tag, QString errorDescription)
{
    QNWARNING(errorDescription << ", tag: " << (tag.isNull() ? QString("NULL") : tag->ToQString()));
    emit failure(errorDescription);
}

void TagLocalStorageManagerAsyncTester::onFindTagCompleted(QSharedPointer<Tag> tag)
{
    Q_ASSERT_X(!tag.isNull(), "TagLocalStorageManagerAsyncTester::onFindTagCompleted slot",
               "Found NULL shared pointed to Tag");

    QString errorDescription;

    if (m_state == STATE_SENT_FIND_AFTER_ADD_REQUEST)
    {
        if (m_pFoundTag != tag) {
            errorDescription = "Internal error in TagLocalStorageManagerAsyncTester: "
                               "tag pointer in onFindTagCompleted slot doesn't match "
                               "the pointer to the original Tag";
            QNWARNING(errorDescription);
            errorDescription = QObject::tr(qPrintable(errorDescription));
            emit failure(errorDescription);
            return;
        }

        Q_ASSERT(!m_pInitialTag.isNull());
        if (*m_pFoundTag != *m_pInitialTag) {
            errorDescription = "Added and found tags in local storage don't match";
            QNWARNING(errorDescription << ": Tag added to LocalStorageManager: " << *m_pInitialTag
                      << "\nTag found in LocalStorageManager: " << *m_pFoundTag);
            errorDescription = QObject::tr(qPrintable(errorDescription));
            emit failure(errorDescription);
            return;
        }

        // Ok, found tag is good, updating it now
        m_pModifiedTag = QSharedPointer<Tag>(new Tag(*m_pInitialTag));
        m_pModifiedTag->setUpdateSequenceNumber(m_pInitialTag->updateSequenceNumber() + 1);
        m_pModifiedTag->setName(m_pInitialTag->name() + "_modified");

        m_state = STATE_SENT_UPDATE_REQUEST;
        emit updateTagRequest(m_pModifiedTag);
    }
    else if (m_state == STATE_SENT_FIND_AFTER_UPDATE_REQUEST)
    {
        if (m_pFoundTag != tag) {
            errorDescription = "Internal error in TagLocalStorageManagerAsyncTester: "
                               "tag pointer in onFindTagCompleted slot doesn't match "
                               "the pointer to the original modified Tag";
            QNWARNING(errorDescription);
            errorDescription = QObject::tr(qPrintable(errorDescription));
            emit failure(errorDescription);
            return;
        }

        Q_ASSERT(!m_pModifiedTag.isNull());
        if (*m_pFoundTag != *m_pModifiedTag) {
            errorDescription = "Updated and found tags in local storage don't match";
            QNWARNING(errorDescription << ": Tag updated in LocalStorageManager: " << *m_pModifiedTag
                      << "\nTag found in LocalStorageManager: " << *m_pFoundTag);
            errorDescription = QObject::tr(qPrintable(errorDescription));
            emit failure(errorDescription);
            return;
        }

        m_state = STATE_SENT_GET_COUNT_AFTER_UPDATE_REQUEST;
        emit getTagCountRequest();
    }
    else if (m_state == STATE_SENT_FIND_AFTER_EXPUNGE_REQUEST)
    {
        Q_ASSERT(!m_pModifiedTag.isNull());
        errorDescription = "Error: found tag which should have been expunged from local storage";
        QNWARNING(errorDescription << ": Tag expunged from LocalStorageManager: " << *m_pModifiedTag
                  << "\nTag found in LocalStorageManager: " << *m_pFoundTag);
        errorDescription = QObject::tr(qPrintable(errorDescription));
        emit failure(errorDescription);
        return;
    }
    HANDLE_WRONG_STATE();
}

void TagLocalStorageManagerAsyncTester::onFindTagFailed(QSharedPointer<Tag> tag, QString errorDescription)
{
    if (m_state == STATE_SENT_FIND_AFTER_EXPUNGE_REQUEST) {
        m_state = STATE_SENT_GET_COUNT_AFTER_EXPUNGE_REQUEST;
        emit getTagCountRequest();
        return;
    }

    QNWARNING(errorDescription << ", tag: " << (tag.isNull() ? QString("NULL") : tag->ToQString()));
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
        errorDescription = QObject::tr(qPrintable(errorDescription));
        emit failure(errorDescription);
        return;
    }

    foreach(const Tag & tag, m_initialTags)
    {
        if (!tags.contains(tag)) {
            errorDescription = "One of initial tags was not found within gounf tags";
            QNWARNING(errorDescription);
            errorDescription = QObject::tr(qPrintable(errorDescription));
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

void TagLocalStorageManagerAsyncTester::onExpungeTagCompleted(QSharedPointer<Tag> tag)
{
    Q_ASSERT_X(!tag.isNull(), "TagLocalStorageManagerAsyncTester::onExpungeTagCompleted slot",
               "Found NULL pointer to Tag");

    QString errorDescription;

    if (m_pModifiedTag != tag) {
        errorDescription = "Internal error in TagLocalStorageManagerAsyncTester: "
                           "tag pointer in onExpungeTagCompleted slot doesn't match "
                           "the pointer to the original expunged Tag";
        QNWARNING(errorDescription);
        errorDescription = QObject::tr(qPrintable(errorDescription));
        emit failure(errorDescription);
        return;
    }

    Q_ASSERT(!m_pFoundTag.isNull());
    m_state = STATE_SENT_FIND_AFTER_EXPUNGE_REQUEST;
    emit findTagRequest(m_pFoundTag);
}

void TagLocalStorageManagerAsyncTester::onExpungeTagFailed(QSharedPointer<Tag> tag, QString errorDescription)
{
    QNWARNING(errorDescription << ", Tag: " << (tag.isNull() ? QString("NULL") : tag->ToQString()));
    emit failure(errorDescription);
}

void TagLocalStorageManagerAsyncTester::createConnections()
{
    // Request --> slot connections
    QObject::connect(this, SIGNAL(getTagCountRequest()), m_pLocalStorageManagerThread,
                     SLOT(onGetTagCountRequest()));
    QObject::connect(this, SIGNAL(addTagRequest(QSharedPointer<Tag>)), m_pLocalStorageManagerThread,
                     SLOT(onAddTagRequest(QSharedPointer<Tag>)));
    QObject::connect(this, SIGNAL(updateTagRequest(QSharedPointer<Tag>)), m_pLocalStorageManagerThread,
                     SLOT(onUpdateTagRequest(QSharedPointer<Tag>)));
    QObject::connect(this, SIGNAL(findTagRequest(QSharedPointer<Tag>)), m_pLocalStorageManagerThread,
                     SLOT(onFindTagRequest(QSharedPointer<Tag>)));
    QObject::connect(this, SIGNAL(listAllTagsRequest()), m_pLocalStorageManagerThread,
                     SLOT(onListAllTagsRequest()));
    QObject::connect(this, SIGNAL(expungeTagRequest(QSharedPointer<Tag>)),
                     m_pLocalStorageManagerThread, SLOT(onExpungeTagRequest(QSharedPointer<Tag>)));

    // Slot <-- result connections
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(getTagCountComplete(int)),
                     this, SLOT(onGetTagCountCompleted(int)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(getTagCountFailed(QString)),
                     this, SLOT(onGetTagCountFailed(QString)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(addTagComplete(QSharedPointer<Tag>)),
                     this, SLOT(onAddTagCompleted(QSharedPointer<Tag>)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(addTagFailed(QSharedPointer<Tag>,QString)),
                     this, SLOT(onAddTagFailed(QSharedPointer<Tag>,QString)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(updateTagComplete(QSharedPointer<Tag>)),
                     this, SLOT(onUpdateTagCompleted(QSharedPointer<Tag>)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(updateTagFailed(QSharedPointer<Tag>,QString)),
                     this, SLOT(onUpdateTagFailed(QSharedPointer<Tag>,QString)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(findTagComplete(QSharedPointer<Tag>)),
                     this, SLOT(onFindTagCompleted(QSharedPointer<Tag>)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(findTagFailed(QSharedPointer<Tag>,QString)),
                     this, SLOT(onFindTagFailed(QSharedPointer<Tag>,QString)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(listAllTagsComplete(QList<Tag>)),
                     this, SLOT(onListAllTagsCompleted(QList<Tag>)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(listAllTagsFailed(QString)),
                     this, SLOT(onListAllTagsFailed(QString)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(expungeTagComplete(QSharedPointer<Tag>)),
                     this, SLOT(onExpungeTagCompleted(QSharedPointer<Tag>)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(expungeTagFailed(QSharedPointer<Tag>,QString)),
                     this, SLOT(onExpungeTagFailed(QSharedPointer<Tag>,QString)));
}

#undef HANDLE_WRONG_STATE

} // namespace test
} // namespace qute_note
