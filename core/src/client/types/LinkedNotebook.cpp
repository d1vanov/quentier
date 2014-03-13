#include "LinkedNotebook.h"
#include "../Utility.h"

namespace qute_note {

LinkedNotebook::LinkedNotebook() :
    NoteStoreDataElement(),
    m_enLinkedNotebook()
{}

LinkedNotebook::LinkedNotebook(const LinkedNotebook & other) :
    NoteStoreDataElement(other),
    m_enLinkedNotebook(other.m_enLinkedNotebook)
{}

LinkedNotebook & LinkedNotebook::operator =(const LinkedNotebook & other)
{
    if (this != &other) {
        NoteStoreDataElement::operator =(other);
        m_enLinkedNotebook = m_enLinkedNotebook;
    }

    return *this;
}

LinkedNotebook::~LinkedNotebook()
{}

void LinkedNotebook::clear()
{
    m_enLinkedNotebook = evernote::edam::LinkedNotebook();
}

bool LinkedNotebook::hasGuid() const
{
    return m_enLinkedNotebook.__isset.guid;
}

const QString LinkedNotebook::guid() const
{
    return std::move(QString::fromStdString(m_enLinkedNotebook.guid));
}

void LinkedNotebook::setGuid(const QString & guid)
{
    m_enLinkedNotebook.guid = guid.toStdString();
    m_enLinkedNotebook.__isset.guid = true;
}

bool LinkedNotebook::hasUpdateSequenceNumber() const
{
    return m_enLinkedNotebook.__isset.updateSequenceNum;
}

qint32 LinkedNotebook::updateSequenceNumber() const
{
    return m_enLinkedNotebook.updateSequenceNum;
}

void LinkedNotebook::setUpdateSequenceNumber(const qint32 usn)
{
    m_enLinkedNotebook.updateSequenceNum = usn;
    m_enLinkedNotebook.__isset.updateSequenceNum = true;
}

bool LinkedNotebook::checkParameters(QString & errorDescription) const
{
    if (!m_enLinkedNotebook.__isset.guid) {
        errorDescription = QObject::tr("Linked notebook's guid is not set");
        return false;
    }
    else if (!CheckGuid(m_enLinkedNotebook.guid)) {
        errorDescription = QObject::tr("Linked notebook's guid is invalid");
        return false;
    }

    if (!m_enLinkedNotebook.__isset.shareName) {
        errorDescription = QObject::tr("Linked notebook's custom name is not set");
        return false;
    }
    else if (m_enLinkedNotebook.shareName.empty()) {
        errorDescription = QObject::tr("Linked notebook's custom name is empty");
        return false;
    }
    else
    {
        QString simplifiedShareName = QString::fromStdString(m_enLinkedNotebook.shareName).replace(" ", "");
        if (simplifiedShareName.isEmpty()) {
            errorDescription = QObject::tr("Linked notebook's custom must contain non-space characters");
            return false;
        }
    }

    return true;
}

bool LinkedNotebook::hasShareName() const
{
    return m_enLinkedNotebook.__isset.shareName;
}

const QString LinkedNotebook::shareName() const
{
    return std::move(QString::fromStdString(m_enLinkedNotebook.shareName));
}

void LinkedNotebook::setShareName(const QString & shareName)
{
    m_enLinkedNotebook.shareName = shareName.toStdString();
    m_enLinkedNotebook.__isset.shareName = true;
}

bool LinkedNotebook::hasUsername() const
{
    return m_enLinkedNotebook.__isset.username;
}

const QString LinkedNotebook::username() const
{
    return std::move(QString::fromStdString(m_enLinkedNotebook.username));
}

void LinkedNotebook::setUsername(const QString & username)
{
    m_enLinkedNotebook.username = username.toStdString();
    m_enLinkedNotebook.__isset.username = true;
}

bool LinkedNotebook::hasShardId() const
{
    return m_enLinkedNotebook.__isset.shardId;
}

const QString LinkedNotebook::shardId() const
{
    return std::move(QString::fromStdString(m_enLinkedNotebook.shardId));
}

void LinkedNotebook::setShardId(const QString & shardId)
{
    m_enLinkedNotebook.shardId = shardId.toStdString();
    m_enLinkedNotebook.__isset.shardId = true;
}

bool LinkedNotebook::hasShareKey() const
{
    return m_enLinkedNotebook.__isset.shareKey;
}

const QString LinkedNotebook::shareKey() const
{
    return std::move(QString::fromStdString(m_enLinkedNotebook.shareKey));
}

void LinkedNotebook::setShareKey(const QString & shareKey)
{
    m_enLinkedNotebook.shareKey = shareKey.toStdString();
    m_enLinkedNotebook.__isset.shareKey = true;
}

bool LinkedNotebook::hasUri() const
{
    return m_enLinkedNotebook.__isset.uri;
}

const QString LinkedNotebook::uri() const
{
    return std::move(QString::fromStdString(m_enLinkedNotebook.uri));
}

void LinkedNotebook::setUri(const QString & uri)
{
    m_enLinkedNotebook.uri = uri.toStdString();
    m_enLinkedNotebook.__isset.uri = true;
}

bool LinkedNotebook::hasNoteStoreUrl() const
{
    return m_enLinkedNotebook.__isset.noteStoreUrl;
}

const QString LinkedNotebook::noteStoreUrl() const
{
    return std::move(QString::fromStdString(m_enLinkedNotebook.noteStoreUrl));
}

void LinkedNotebook::setNoteStoreUrl(const QString & noteStoreUr)
{
    m_enLinkedNotebook.noteStoreUrl = noteStoreUr.toStdString();
    m_enLinkedNotebook.__isset.noteStoreUrl = true;
}

bool LinkedNotebook::hasWebApiUrlPrefix() const
{
    return m_enLinkedNotebook.__isset.webApiUrlPrefix;
}

const QString LinkedNotebook::webApiUrlPrefix() const
{
    return std::move(QString::fromStdString(m_enLinkedNotebook.webApiUrlPrefix));
}

void LinkedNotebook::setWebApiUrlPrefix(const QString & webApiUrlPrefix)
{
    m_enLinkedNotebook.webApiUrlPrefix = webApiUrlPrefix.toStdString();
    m_enLinkedNotebook.__isset.webApiUrlPrefix = true;
}

bool LinkedNotebook::hasStack() const
{
    return m_enLinkedNotebook.__isset.stack;
}

const QString LinkedNotebook::stack() const
{
    return std::move(QString::fromStdString(m_enLinkedNotebook.stack));
}

void LinkedNotebook::setStack(const QString & stack)
{
    m_enLinkedNotebook.stack = stack.toStdString();
}

bool LinkedNotebook::hasBusinessId() const
{
    return m_enLinkedNotebook.__isset.businessId;
}

qint32 LinkedNotebook::businessId() const
{
    return m_enLinkedNotebook.businessId;
}

void LinkedNotebook::setBusinessId(const qint32 businessId)
{
    m_enLinkedNotebook.businessId = businessId;
    m_enLinkedNotebook.__isset.businessId = true;
}

QTextStream & LinkedNotebook::Print(QTextStream & strm) const
{
    strm << "LinkedNotebook: { \n";
    strm << "isDirty = " << (isDirty() ? "true" : "false") << "\n";

    const auto & isSet = m_enLinkedNotebook.__isset;

    if (isSet.guid) {
        strm << "guid = " << m_enLinkedNotebook.guid << "\n";
    }
    else {
        strm << "guid is not set" << "\n";
    }

    if (isSet.updateSequenceNum) {
        strm << "updateSequenceNum = " << m_enLinkedNotebook.updateSequenceNum << "\n";
    }
    else {
        strm << "updateSequenceNum is not set" << "\n";
    }

    if (isSet.shareName) {
        strm << "shareName = " << m_enLinkedNotebook.shareName << "\n";
    }
    else {
        strm << "shareName is not set" << "\n";
    }

    if (isSet.shardId) {
        strm << "shardId = " << m_enLinkedNotebook.shardId << "\n";
    }
    else {
        strm << "shardId is not set" << "\n";
    }

    if (isSet.shareKey) {
        strm << "shareKey = " << m_enLinkedNotebook.shareKey << "\n";
    }
    else {
        strm << "shareKey is not set" << "\n";
    }

    if (isSet.uri) {
        strm << "uri = " << m_enLinkedNotebook.uri << "\n";
    }
    else {
        strm << "uri is not set" << "\n";
    }

    if (isSet.noteStoreUrl) {
        strm << "noteStoreUrl = " << m_enLinkedNotebook.noteStoreUrl << "\n";
    }
    else {
        strm << "noteStoreUrl is not set" << "\n";
    }

    if (isSet.webApiUrlPrefix) {
        strm << "webApiUrlPrefix = " << m_enLinkedNotebook.webApiUrlPrefix << "\n";
    }
    else {
        strm << "webApiUrlPrefix is not set" << "\n";
    }

    if (isSet.stack) {
        strm << "stack = " << m_enLinkedNotebook.stack << "\n";
    }
    else {
        strm << "stack is not set" << "\n";
    }

    if (isSet.businessId) {
        strm << "businessId = " << m_enLinkedNotebook.businessId << "\n";
    }
    else {
        strm << "businessId is not set" << "\n";
    }

    return strm;
}

} // namespace qute_note
