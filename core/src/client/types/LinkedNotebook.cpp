#include "LinkedNotebook.h"
#include "QEverCloudHelpers.h"
#include "../Utility.h"

namespace qute_note {

LinkedNotebook::LinkedNotebook(const qevercloud::LinkedNotebook & linkedNotebook) :
    NoteStoreDataElement(),
    m_qecLinkedNotebook(linkedNotebook)
{}

LinkedNotebook::LinkedNotebook(qevercloud::LinkedNotebook && linkedNotebook) :
    NoteStoreDataElement(),
    m_qecLinkedNotebook(std::move(linkedNotebook))
{}

LinkedNotebook::~LinkedNotebook()
{}

LinkedNotebook::operator qevercloud::LinkedNotebook & ()
{
    return m_qecLinkedNotebook;
}

LinkedNotebook::operator const qevercloud::LinkedNotebook & () const
{
    return m_qecLinkedNotebook;
}

bool LinkedNotebook::operator==(const LinkedNotebook & other) const
{
    return ((isDirty() == other.isDirty()) && (m_qecLinkedNotebook == other.m_qecLinkedNotebook));
}

bool LinkedNotebook::operator!=(const LinkedNotebook & other) const
{
    return !(*this == other);
}

void LinkedNotebook::clear()
{
    m_qecLinkedNotebook = qevercloud::LinkedNotebook();
}

bool LinkedNotebook::hasGuid() const
{
    return m_qecLinkedNotebook.guid.isSet();
}

const QString & LinkedNotebook::guid() const
{
    return m_qecLinkedNotebook.guid.ref();
}

void LinkedNotebook::setGuid(const QString & guid)
{
    m_qecLinkedNotebook.guid = guid;
}

bool LinkedNotebook::hasUpdateSequenceNumber() const
{
    return m_qecLinkedNotebook.updateSequenceNum.isSet();
}

qint32 LinkedNotebook::updateSequenceNumber() const
{
    return m_qecLinkedNotebook.updateSequenceNum;
}

void LinkedNotebook::setUpdateSequenceNumber(const qint32 usn)
{
    m_qecLinkedNotebook.updateSequenceNum = usn;
}

bool LinkedNotebook::checkParameters(QString & errorDescription) const
{    
    if (!m_qecLinkedNotebook.guid.isSet()) {
        errorDescription = QObject::tr("Linked notebook's guid is not set");
        return false;
    }
    else if (!CheckGuid(m_qecLinkedNotebook.guid.ref())) {
        errorDescription = QObject::tr("Linked notebook's guid is invalid");
        return false;
    }

    if (!m_qecLinkedNotebook.shareName.isSet()) {
        errorDescription = QObject::tr("Linked notebook's custom name is not set");
        return false;
    }
    else if (m_qecLinkedNotebook.shareName->isEmpty()) {
        errorDescription = QObject::tr("Linked notebook's custom name is empty");
        return false;
    }
    else
    {
        QString simplifiedShareName = QString(m_qecLinkedNotebook.shareName.ref()).replace(" ", "");
        if (simplifiedShareName.isEmpty()) {
            errorDescription = QObject::tr("Linked notebook's custom name must contain non-space characters");
            return false;
        }
    }

    return true;
}

bool LinkedNotebook::hasShareName() const
{
    return m_qecLinkedNotebook.shareName.isSet();
}

const QString & LinkedNotebook::shareName() const
{
    return m_qecLinkedNotebook.shareName;
}

void LinkedNotebook::setShareName(const QString & shareName)
{
    m_qecLinkedNotebook.shareName = shareName;
}

bool LinkedNotebook::hasUsername() const
{
    return m_qecLinkedNotebook.username.isSet();
}

const QString & LinkedNotebook::username() const
{
    return m_qecLinkedNotebook.username;
}

void LinkedNotebook::setUsername(const QString & username)
{
    m_qecLinkedNotebook.username = username;
}

bool LinkedNotebook::hasShardId() const
{
    return m_qecLinkedNotebook.shardId.isSet();
}

const QString & LinkedNotebook::shardId() const
{
    return m_qecLinkedNotebook.shardId;
}

void LinkedNotebook::setShardId(const QString & shardId)
{
    m_qecLinkedNotebook.shardId = shardId;
}

bool LinkedNotebook::hasShareKey() const
{
    return m_qecLinkedNotebook.shareKey.isSet();
}

const QString & LinkedNotebook::shareKey() const
{
    return m_qecLinkedNotebook.shareKey;
}

void LinkedNotebook::setShareKey(const QString & shareKey)
{
    m_qecLinkedNotebook.shareKey = shareKey;
}

bool LinkedNotebook::hasUri() const
{
    return m_qecLinkedNotebook.uri.isSet();
}

const QString & LinkedNotebook::uri() const
{
    return m_qecLinkedNotebook.uri;
}

void LinkedNotebook::setUri(const QString & uri)
{
    m_qecLinkedNotebook.uri = uri;
}

bool LinkedNotebook::hasNoteStoreUrl() const
{
    return m_qecLinkedNotebook.noteStoreUrl.isSet();
}

const QString & LinkedNotebook::noteStoreUrl() const
{
    return m_qecLinkedNotebook.noteStoreUrl;
}

void LinkedNotebook::setNoteStoreUrl(const QString & noteStoreUr)
{
    m_qecLinkedNotebook.noteStoreUrl = noteStoreUr;
}

bool LinkedNotebook::hasWebApiUrlPrefix() const
{
    return m_qecLinkedNotebook.webApiUrlPrefix.isSet();
}

const QString & LinkedNotebook::webApiUrlPrefix() const
{
    return m_qecLinkedNotebook.webApiUrlPrefix;
}

void LinkedNotebook::setWebApiUrlPrefix(const QString & webApiUrlPrefix)
{
    m_qecLinkedNotebook.webApiUrlPrefix = webApiUrlPrefix;
}

bool LinkedNotebook::hasStack() const
{
    return m_qecLinkedNotebook.stack.isSet();
}

const QString & LinkedNotebook::stack() const
{
    return m_qecLinkedNotebook.stack;
}

void LinkedNotebook::setStack(const QString & stack)
{
    m_qecLinkedNotebook.stack = stack;
}

bool LinkedNotebook::hasBusinessId() const
{
    return m_qecLinkedNotebook.businessId.isSet();
}

qint32 LinkedNotebook::businessId() const
{
    return m_qecLinkedNotebook.businessId;
}

void LinkedNotebook::setBusinessId(const qint32 businessId)
{
    m_qecLinkedNotebook.businessId = businessId;
}

QTextStream & LinkedNotebook::Print(QTextStream & strm) const
{
    strm << "LinkedNotebook: { \n";
    strm << "isDirty = " << (isDirty() ? "true" : "false") << "\n";

    if (m_qecLinkedNotebook.guid.isSet()) {
        strm << "guid = " << m_qecLinkedNotebook.guid << "\n";
    }
    else {
        strm << "guid is not set" << "\n";
    }

    if (m_qecLinkedNotebook.updateSequenceNum.isSet()) {
        strm << "updateSequenceNum = " << QString::number(m_qecLinkedNotebook.updateSequenceNum) << "\n";
    }
    else {
        strm << "updateSequenceNum is not set" << "\n";
    }

    if (m_qecLinkedNotebook.shareName.isSet()) {
        strm << "shareName = " << m_qecLinkedNotebook.shareName << "\n";
    }
    else {
        strm << "shareName is not set" << "\n";
    }

    if (m_qecLinkedNotebook.shardId.isSet()) {
        strm << "shardId = " << m_qecLinkedNotebook.shardId << "\n";
    }
    else {
        strm << "shardId is not set" << "\n";
    }

    if (m_qecLinkedNotebook.shareKey.isSet()) {
        strm << "shareKey = " << m_qecLinkedNotebook.shareKey << "\n";
    }
    else {
        strm << "shareKey is not set" << "\n";
    }

    if (m_qecLinkedNotebook.uri.isSet()) {
        strm << "uri = " << m_qecLinkedNotebook.uri << "\n";
    }
    else {
        strm << "uri is not set" << "\n";
    }

    if (m_qecLinkedNotebook.noteStoreUrl.isSet()) {
        strm << "noteStoreUrl = " << m_qecLinkedNotebook.noteStoreUrl << "\n";
    }
    else {
        strm << "noteStoreUrl is not set" << "\n";
    }

    if (m_qecLinkedNotebook.webApiUrlPrefix.isSet()) {
        strm << "webApiUrlPrefix = " << m_qecLinkedNotebook.webApiUrlPrefix << "\n";
    }
    else {
        strm << "webApiUrlPrefix is not set" << "\n";
    }

    if (m_qecLinkedNotebook.stack.isSet()) {
        strm << "stack = " << m_qecLinkedNotebook.stack << "\n";
    }
    else {
        strm << "stack is not set" << "\n";
    }

    if (m_qecLinkedNotebook.businessId.isSet()) {
        strm << "businessId = " << m_qecLinkedNotebook.businessId << "\n";
    }
    else {
        strm << "businessId is not set" << "\n";
    }

    return strm;
}

} // namespace qute_note
