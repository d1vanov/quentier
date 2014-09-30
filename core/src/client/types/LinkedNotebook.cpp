#include "LinkedNotebook.h"
#include "data/LinkedNotebookData.h"
#include "QEverCloudHelpers.h"

namespace qute_note {

QN_DEFINE_LOCAL_GUID(LinkedNotebook)
QN_DEFINE_DIRTY(LinkedNotebook)

LinkedNotebook::LinkedNotebook() :
    d(new LinkedNotebookData)
{}

LinkedNotebook::LinkedNotebook(const LinkedNotebook & other) :
    INoteStoreDataElement(other),
    d(other.d)
{}

LinkedNotebook::LinkedNotebook(LinkedNotebook && other) :
    INoteStoreDataElement(std::move(other)),
    d(other.d)
{}

LinkedNotebook & LinkedNotebook::operator=(const LinkedNotebook & other)
{
    if (this != std::addressof(other)) {
        d = other.d;
    }

    return *this;
}

LinkedNotebook & LinkedNotebook::operator=(LinkedNotebook && other)
{
    if (this != std::addressof(other)) {
        d = other.d;
    }

    return *this;
}

LinkedNotebook::LinkedNotebook(const qevercloud::LinkedNotebook & linkedNotebook) :
    INoteStoreDataElement(),
    d(new LinkedNotebookData(linkedNotebook))
{}

LinkedNotebook::LinkedNotebook(qevercloud::LinkedNotebook && linkedNotebook) :
    INoteStoreDataElement(),
    d(new LinkedNotebookData(std::move(linkedNotebook)))
{}

LinkedNotebook::~LinkedNotebook()
{}

LinkedNotebook::operator qevercloud::LinkedNotebook & ()
{
    return d->m_qecLinkedNotebook;
}

LinkedNotebook::operator const qevercloud::LinkedNotebook & () const
{
    return d->m_qecLinkedNotebook;
}

bool LinkedNotebook::operator==(const LinkedNotebook & other) const
{
    return ((isDirty() == other.isDirty()) && (d->m_qecLinkedNotebook == other.d->m_qecLinkedNotebook));
}

bool LinkedNotebook::operator!=(const LinkedNotebook & other) const
{
    return !(*this == other);
}

void LinkedNotebook::clear()
{
    d->m_qecLinkedNotebook = qevercloud::LinkedNotebook();
}

bool LinkedNotebook::hasGuid() const
{
    return d->m_qecLinkedNotebook.guid.isSet();
}

const QString & LinkedNotebook::guid() const
{
    return d->m_qecLinkedNotebook.guid.ref();
}

void LinkedNotebook::setGuid(const QString & guid)
{
    d->m_qecLinkedNotebook.guid = guid;
}

bool LinkedNotebook::hasUpdateSequenceNumber() const
{
    return d->m_qecLinkedNotebook.updateSequenceNum.isSet();
}

qint32 LinkedNotebook::updateSequenceNumber() const
{
    return d->m_qecLinkedNotebook.updateSequenceNum;
}

void LinkedNotebook::setUpdateSequenceNumber(const qint32 usn)
{
    d->m_qecLinkedNotebook.updateSequenceNum = usn;
}

bool LinkedNotebook::checkParameters(QString & errorDescription) const
{    
    return d->checkParameters(errorDescription);
}

bool LinkedNotebook::hasShareName() const
{
    return d->m_qecLinkedNotebook.shareName.isSet();
}

const QString & LinkedNotebook::shareName() const
{
    return d->m_qecLinkedNotebook.shareName;
}

void LinkedNotebook::setShareName(const QString & shareName)
{
    d->m_qecLinkedNotebook.shareName = shareName;
}

bool LinkedNotebook::hasUsername() const
{
    return d->m_qecLinkedNotebook.username.isSet();
}

const QString & LinkedNotebook::username() const
{
    return d->m_qecLinkedNotebook.username;
}

void LinkedNotebook::setUsername(const QString & username)
{
    d->m_qecLinkedNotebook.username = username;
}

bool LinkedNotebook::hasShardId() const
{
    return d->m_qecLinkedNotebook.shardId.isSet();
}

const QString & LinkedNotebook::shardId() const
{
    return d->m_qecLinkedNotebook.shardId;
}

void LinkedNotebook::setShardId(const QString & shardId)
{
    d->m_qecLinkedNotebook.shardId = shardId;
}

bool LinkedNotebook::hasShareKey() const
{
    return d->m_qecLinkedNotebook.shareKey.isSet();
}

const QString & LinkedNotebook::shareKey() const
{
    return d->m_qecLinkedNotebook.shareKey;
}

void LinkedNotebook::setShareKey(const QString & shareKey)
{
    d->m_qecLinkedNotebook.shareKey = shareKey;
}

bool LinkedNotebook::hasUri() const
{
    return d->m_qecLinkedNotebook.uri.isSet();
}

const QString & LinkedNotebook::uri() const
{
    return d->m_qecLinkedNotebook.uri;
}

void LinkedNotebook::setUri(const QString & uri)
{
    d->m_qecLinkedNotebook.uri = uri;
}

bool LinkedNotebook::hasNoteStoreUrl() const
{
    return d->m_qecLinkedNotebook.noteStoreUrl.isSet();
}

const QString & LinkedNotebook::noteStoreUrl() const
{
    return d->m_qecLinkedNotebook.noteStoreUrl;
}

void LinkedNotebook::setNoteStoreUrl(const QString & noteStoreUr)
{
    d->m_qecLinkedNotebook.noteStoreUrl = noteStoreUr;
}

bool LinkedNotebook::hasWebApiUrlPrefix() const
{
    return d->m_qecLinkedNotebook.webApiUrlPrefix.isSet();
}

const QString & LinkedNotebook::webApiUrlPrefix() const
{
    return d->m_qecLinkedNotebook.webApiUrlPrefix;
}

void LinkedNotebook::setWebApiUrlPrefix(const QString & webApiUrlPrefix)
{
    d->m_qecLinkedNotebook.webApiUrlPrefix = webApiUrlPrefix;
}

bool LinkedNotebook::hasStack() const
{
    return d->m_qecLinkedNotebook.stack.isSet();
}

const QString & LinkedNotebook::stack() const
{
    return d->m_qecLinkedNotebook.stack;
}

void LinkedNotebook::setStack(const QString & stack)
{
    d->m_qecLinkedNotebook.stack = stack;
}

bool LinkedNotebook::hasBusinessId() const
{
    return d->m_qecLinkedNotebook.businessId.isSet();
}

qint32 LinkedNotebook::businessId() const
{
    return d->m_qecLinkedNotebook.businessId;
}

void LinkedNotebook::setBusinessId(const qint32 businessId)
{
    d->m_qecLinkedNotebook.businessId = businessId;
}

QTextStream & LinkedNotebook::Print(QTextStream & strm) const
{
    strm << "LinkedNotebook: { \n";
    strm << "isDirty = " << (isDirty() ? "true" : "false") << "\n";

    if (d->m_qecLinkedNotebook.guid.isSet()) {
        strm << "guid = " << d->m_qecLinkedNotebook.guid << "\n";
    }
    else {
        strm << "guid is not set" << "\n";
    }

    if (d->m_qecLinkedNotebook.updateSequenceNum.isSet()) {
        strm << "updateSequenceNum = " << QString::number(d->m_qecLinkedNotebook.updateSequenceNum) << "\n";
    }
    else {
        strm << "updateSequenceNum is not set" << "\n";
    }

    if (d->m_qecLinkedNotebook.shareName.isSet()) {
        strm << "shareName = " << d->m_qecLinkedNotebook.shareName << "\n";
    }
    else {
        strm << "shareName is not set" << "\n";
    }

    if (d->m_qecLinkedNotebook.shardId.isSet()) {
        strm << "shardId = " << d->m_qecLinkedNotebook.shardId << "\n";
    }
    else {
        strm << "shardId is not set" << "\n";
    }

    if (d->m_qecLinkedNotebook.shareKey.isSet()) {
        strm << "shareKey = " << d->m_qecLinkedNotebook.shareKey << "\n";
    }
    else {
        strm << "shareKey is not set" << "\n";
    }

    if (d->m_qecLinkedNotebook.uri.isSet()) {
        strm << "uri = " << d->m_qecLinkedNotebook.uri << "\n";
    }
    else {
        strm << "uri is not set" << "\n";
    }

    if (d->m_qecLinkedNotebook.noteStoreUrl.isSet()) {
        strm << "noteStoreUrl = " << d->m_qecLinkedNotebook.noteStoreUrl << "\n";
    }
    else {
        strm << "noteStoreUrl is not set" << "\n";
    }

    if (d->m_qecLinkedNotebook.webApiUrlPrefix.isSet()) {
        strm << "webApiUrlPrefix = " << d->m_qecLinkedNotebook.webApiUrlPrefix << "\n";
    }
    else {
        strm << "webApiUrlPrefix is not set" << "\n";
    }

    if (d->m_qecLinkedNotebook.stack.isSet()) {
        strm << "stack = " << d->m_qecLinkedNotebook.stack << "\n";
    }
    else {
        strm << "stack is not set" << "\n";
    }

    if (d->m_qecLinkedNotebook.businessId.isSet()) {
        strm << "businessId = " << d->m_qecLinkedNotebook.businessId << "\n";
    }
    else {
        strm << "businessId is not set" << "\n";
    }

    return strm;
}

} // namespace qute_note
