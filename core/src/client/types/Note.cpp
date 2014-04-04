#include "Note.h"
#include "ResourceAdapter.h"
#include "../Utility.h"
#include "../Serialization.h"
#include <logging/QuteNoteLogger.h>
#include <Limits_constants.h>

namespace qute_note {

Note::Note() :
    NoteStoreDataElement(),
    m_enNote(),
    m_isLocal(true),
    m_isDeleted(false)
{}

Note::Note(const Note & other) :
    NoteStoreDataElement(other),
    m_enNote(other.m_enNote),
    m_isLocal(other.m_isLocal),
    m_isDeleted(other.m_isDeleted)
{}

Note & Note::operator=(const Note & other)
{
    if (this != &other) {
        NoteStoreDataElement::operator =(other);
        m_enNote = other.m_enNote;
        m_isLocal = other.m_isLocal;
        m_isDeleted = other.m_isDeleted;
    }

    return *this;
}

Note::~Note()
{}

bool Note::hasGuid() const
{
    return m_enNote.__isset.guid;
}

const QString Note::guid() const
{
    return std::move(QString::fromStdString(m_enNote.guid));
}

void Note::setGuid(const QString & guid)
{
    m_enNote.guid = guid.toStdString();

    if (!guid.isEmpty()) {
        m_enNote.__isset.guid = true;
    }
    else {
        m_enNote.__isset.guid = false;
    }
}

bool Note::hasUpdateSequenceNumber() const
{
    return m_enNote.__isset.updateSequenceNum;
}

qint32 Note::updateSequenceNumber() const
{
    return m_enNote.updateSequenceNum;
}

void Note::setUpdateSequenceNumber(const qint32 usn)
{
    m_enNote.updateSequenceNum = usn;

    if (usn >= 0) {
        m_enNote.__isset.updateSequenceNum = true;
    }
    else {
        m_enNote.__isset.updateSequenceNum = false;
    }
}

void Note::clear()
{
    m_enNote = evernote::edam::Note();
}

bool Note::checkParameters(QString & errorDescription) const
{
    if (!m_enNote.__isset.guid) {
        errorDescription = QObject::tr("Note's guid is not set");
        return false;
    }
    else if (!CheckGuid(m_enNote.guid)) {
        errorDescription = QObject::tr("Note's guid is invalid");
        return false;
    }

    if (!m_enNote.__isset.updateSequenceNum) {
        errorDescription = QObject::tr("Note's update sequence number is not set");
        return false;
    }
    else if (!CheckUpdateSequenceNumber(m_enNote.updateSequenceNum)) {
        errorDescription = QObject::tr("Note's update sequence number is invalid");
        return false;
    }

    if (!m_enNote.__isset.title) {
        errorDescription = QObject::tr("Note's title is not set");
        return false;
    }
    else {
        size_t titleSize = m_enNote.title.size();

        if ( (titleSize < evernote::limits::g_Limits_constants.EDAM_NOTE_TITLE_LEN_MIN) ||
             (titleSize > evernote::limits::g_Limits_constants.EDAM_NOTE_TITLE_LEN_MAX) )
        {
            errorDescription = QObject::tr("Note's title length is invalid");
            return false;
        }
    }

    if (!m_enNote.__isset.content) {
        errorDescription = QObject::tr("Note's content is not set");
        return false;
    }
    else {
        size_t contentSize = m_enNote.content.size();

        if ( (contentSize < evernote::limits::g_Limits_constants.EDAM_NOTE_CONTENT_LEN_MIN) ||
             (contentSize > evernote::limits::g_Limits_constants.EDAM_NOTE_CONTENT_LEN_MAX) )
        {
            errorDescription = QObject::tr("Note's content length is invalid");
            return false;
        }
    }

    if (m_enNote.__isset.contentHash) {
        size_t contentHashSize = m_enNote.contentHash.size();

        if (contentHashSize != evernote::limits::g_Limits_constants.EDAM_HASH_LEN) {
            errorDescription = QObject::tr("Note's content hash size is invalid");
            return false;
        }
    }

    if (!m_enNote.__isset.notebookGuid) {
        errorDescription = QObject::tr("Note's notebook Guid is not set");
        return false;
    }
    else if (!CheckGuid(m_enNote.notebookGuid)) {
        errorDescription = QObject::tr("Note's notebook guid is invalid");
        return false;
    }

    if (m_enNote.__isset.tagGuids) {
        size_t numTagGuids = m_enNote.tagGuids.size();

        if (numTagGuids > evernote::limits::g_Limits_constants.EDAM_NOTE_TAGS_MAX) {
            errorDescription = QObject::tr("Note has too many tags, max allowed ");
            errorDescription.append(QString::number(evernote::limits::g_Limits_constants.EDAM_NOTE_TAGS_MAX));
            return false;
        }
    }

    if (m_enNote.__isset.resources) {
        size_t numResources = m_enNote.resources.size();

        if (numResources > evernote::limits::g_Limits_constants.EDAM_NOTE_RESOURCES_MAX) {
            errorDescription = QObject::tr("Note has too many resources, max allowed ");
            errorDescription.append(QString::number(evernote::limits::g_Limits_constants.EDAM_NOTE_RESOURCES_MAX));
            return false;
        }
    }

    if (!m_enNote.__isset.created) {
        errorDescription = QObject::tr("Note's creation timestamp is not set");
        return false;
    }

    if (!m_enNote.__isset.updated) {
        errorDescription = QObject::tr("Note's modification timestamp is not set");
        return false;
    }

    if (!m_enNote.__isset.attributes) {
        return true;
    }

    const auto & attributes = m_enNote.attributes;

#define CHECK_NOTE_ATTRIBUTE(name) \
    if (attributes.__isset.name) { \
        size_t name##Size = attributes.name.size(); \
        \
        if ( (name##Size < evernote::limits::g_Limits_constants.EDAM_ATTRIBUTE_LEN_MIN) || \
             (name##Size > evernote::limits::g_Limits_constants.EDAM_ATTRIBUTE_LEN_MAX) ) \
        { \
            errorDescription = QObject::tr("Note attributes' " #name " field has invalid size"); \
            return false; \
        } \
    }

    CHECK_NOTE_ATTRIBUTE(author);
    CHECK_NOTE_ATTRIBUTE(source);
    CHECK_NOTE_ATTRIBUTE(sourceURL);
    CHECK_NOTE_ATTRIBUTE(sourceApplication);

#undef CHECK_NOTE_ATTRIBUTE

    if (attributes.__isset.contentClass) {
        size_t contentClassSize = attributes.contentClass.size();

        if ( (contentClassSize < evernote::limits::g_Limits_constants.EDAM_NOTE_CONTENT_CLASS_LEN_MIN) ||
             (contentClassSize > evernote::limits::g_Limits_constants.EDAM_NOTE_CONTENT_CLASS_LEN_MAX) )
        {
            errorDescription = QObject::tr("Note attributes' content class has invalid size");
            return false;
        }
    }

    if (attributes.__isset.applicationData) {
        const auto & applicationData = attributes.applicationData;
        const auto & keysOnly = applicationData.keysOnly;
        const auto & fullMap = applicationData.fullMap;

        for(const auto & key: keysOnly) {
            size_t keySize = key.size();
            if ( (keySize < evernote::limits::g_Limits_constants.EDAM_APPLICATIONDATA_NAME_LEN_MIN) ||
                 (keySize > evernote::limits::g_Limits_constants.EDAM_APPLICATIONDATA_NAME_LEN_MAX) )
            {
                errorDescription = QObject::tr("Note's attributes application data has invalid key (in keysOnly part)");
                return false;
            }
        }

        for(const auto & pair: fullMap) {
            size_t keySize = pair.first.size();
            if ( (keySize < evernote::limits::g_Limits_constants.EDAM_APPLICATIONDATA_NAME_LEN_MIN) ||
                 (keySize > evernote::limits::g_Limits_constants.EDAM_APPLICATIONDATA_NAME_LEN_MAX) )
            {
                errorDescription = QObject::tr("Note's attributes application data has invalid key (in fullMap part)");
                return false;
            }

            size_t valueSize = pair.second.size();
            if ( (valueSize < evernote::limits::g_Limits_constants.EDAM_APPLICATIONDATA_VALUE_LEN_MIN) ||
                 (valueSize > evernote::limits::g_Limits_constants.EDAM_APPLICATIONDATA_VALUE_LEN_MAX) )
            {
                errorDescription = QObject::tr("Note's attributes application data has invalid value");
                return false;
            }

            size_t sumSize = keySize + valueSize;
            if (sumSize > evernote::limits::g_Limits_constants.EDAM_APPLICATIONDATA_ENTRY_LEN_MAX) {
                errorDescription = QObject::tr("Note's attributes application data has invalid entry size");
                return false;
            }
        }
    }

    if (attributes.__isset.classifications) {
        const auto & classifications = attributes.classifications;
        for(const auto & pair: classifications) {
            const auto & value = pair.second;
            size_t pos = value.find("CLASSIFICATION_");
            if (pos == std::string::npos || pos != 0) {
                errorDescription = QObject::tr("Note's attributes classifications has invalid classification value");
                return false;
            }
        }
    }

    return true;
}

bool Note::hasTitle() const
{
    return m_enNote.__isset.title;
}

const QString Note::title() const
{
    return std::move(QString::fromStdString(m_enNote.title));
}

void Note::setTitle(const QString & title)
{
    m_enNote.title = title.toStdString();

    if (!title.isEmpty()) {
        m_enNote.__isset.title = true;
    }
    else {
        m_enNote.__isset.title = false;
    }
}

bool Note::hasContent() const
{
    return m_enNote.__isset.content;
}

const QString Note::content() const
{
    return std::move(QString::fromStdString(m_enNote.content));
}

void Note::setContent(const QString &content)
{
    m_enNote.content = content.toStdString();

    if (!content.isEmpty()) {
        m_enNote.__isset.content = true;
    }
    else {
        m_enNote.__isset.content = false;
    }
}

bool Note::hasContentHash() const
{
    return m_enNote.__isset.contentHash;
}

const QString Note::contentHash() const
{
    return std::move(QString::fromStdString(m_enNote.contentHash));
}

void Note::setContentHash(const QString &contentHash)
{
    m_enNote.contentHash = contentHash.toStdString();

    if (!contentHash.isEmpty()) {
        m_enNote.__isset.contentHash = true;
    }
    else {
        m_enNote.__isset.contentHash = false;
    }
}

bool Note::hasContentLength() const
{
    return m_enNote.__isset.contentLength;
}

qint32 Note::contentLength() const
{
    return m_enNote.contentLength;
}

void Note::setContentLength(const qint32 length)
{
    m_enNote.contentLength = length;

    if (length >= 0) {
        m_enNote.__isset.contentLength = true;
    }
    else {
        m_enNote.__isset.contentLength = false;
    }
}

bool Note::hasCreationTimestamp() const
{
    return m_enNote.__isset.created;
}

qint64 Note::creationTimestamp() const
{
    return m_enNote.created;
}

void Note::setCreationTimestamp(const qint64 timestamp)
{
    m_enNote.created = timestamp;

    if (timestamp >= 0) {
        m_enNote.__isset.created = true;
    }
    else {
        m_enNote.__isset.created = false;
    }
}

bool Note::hasModificationTimestamp() const
{
    return m_enNote.__isset.updated;
}

qint64 Note::modificationTimestamp() const
{
    return m_enNote.updated;
}

void Note::setModificationTimestamp(const qint64 timestamp)
{
    m_enNote.updated = timestamp;

    if (timestamp >= 0) {
        m_enNote.__isset.updated = true;
    }
    else {
        m_enNote.__isset.updated = false;
    }
}

bool Note::hasDeletionTimestamp() const
{
    return m_enNote.__isset.deleted;
}

qint64 Note::deletionTimestamp() const
{
    return m_enNote.deleted;
}

void Note::setDeletionTimestamp(const qint64 timestamp)
{
    m_enNote.deleted = timestamp;

    if (timestamp >= 0) {
        m_enNote.__isset.deleted = true;
    }
    else {
        m_enNote.__isset.deleted = false;
    }
}

bool Note::hasActive() const
{
    return m_enNote.__isset.active;
}

bool Note::active() const
{
    return m_enNote.active;
}

void Note::setActive(const bool active)
{
    m_enNote.active = active;
    m_enNote.__isset.active = true;
}

bool Note::hasNotebookGuid() const
{
    return m_enNote.__isset.notebookGuid;
}

const QString Note::notebookGuid() const
{
    return std::move(QString::fromStdString(m_enNote.notebookGuid));
}

void Note::setNotebookGuid(const QString & guid)
{
    m_enNote.notebookGuid = guid.toStdString();

    if (!guid.isEmpty()) {
        m_enNote.__isset.notebookGuid = true;
    }
    else {
        m_enNote.__isset.notebookGuid = false;
    }
}

bool Note::hasTagGuids() const
{
    return m_enNote.__isset.tagGuids;
}

void Note::tagGuids(std::vector<QString> & guids) const
{
    guids.clear();

    const auto & localTagGuids = m_enNote.tagGuids;
    size_t numTagGuids = localTagGuids.size();
    if (numTagGuids == 0) {
        QNDEBUG("Note::tagGuids: no tags");
        return;
    }

    guids.reserve(numTagGuids);
    for(size_t i = 0; i < numTagGuids; ++i) {
        guids.push_back(QString::fromStdString(localTagGuids[i]));
    }
}

void Note::setTagGuids(const std::vector<QString> & guids)
{
    auto & localTagGuids = m_enNote.tagGuids;
    localTagGuids.clear();

    size_t numTagGuids = guids.size();
    if (numTagGuids == 0) {
        m_enNote.__isset.tagGuids = false;
        QNDEBUG("Note::setTagGuids: no tags");
        return;
    }

    localTagGuids.reserve(numTagGuids);
    for(size_t i = 0; i < numTagGuids; ++i) {
        localTagGuids.push_back(guids[i].toStdString());
    }

    m_enNote.__isset.tagGuids = true;
    QNDEBUG("Added " << numTagGuids << " tag guids to note");
}

void Note::addTagGuid(const QString & guid)
{
    const auto localGuid = guid.toStdString();
    auto & localGuids = m_enNote.tagGuids;

    if (m_enNote.__isset.tagGuids)
    {
        if (std::find(localGuids.cbegin(), localGuids.cend(), localGuid) != localGuids.cend()) {
            QNDEBUG("Note::addTagGuid: already has tag with guid " << guid);
            return;
        }
    }
    else
    {
        localGuids.clear();
        m_enNote.__isset.tagGuids = true;
    }

    localGuids.push_back(localGuid);
    QNDEBUG("Added tag with guid " << guid << " to note");
}

void Note::removeTagGuid(const QString & guid)
{
    const auto localGuid = guid.toStdString();
    auto & localGuids = m_enNote.tagGuids;

    if (m_enNote.__isset.tagGuids)
    {
        auto it = std::find(localGuids.begin(), localGuids.end(), localGuid);
        if (it == localGuids.end()) {
            QNDEBUG("Note::removeTagGuid: note has no tag with guid " << guid);
            return;
        }

        localGuids.erase(it);
        QNDEBUG("Removed tag with guid " << guid << " from note");
    }
    else
    {
        QNDEBUG("Note::removeTagGuid: note has no tags set, nothing to remove");
    }
}



bool Note::hasResources() const
{
    return m_enNote.__isset.resources;
}

void Note::resources(std::vector<ResourceAdapter> & resources) const
{
    resources.clear();

    const auto & localResources = m_enNote.resources;
    size_t numResources = localResources.size();
    if (numResources == 0) {
        QNDEBUG("Note::resources: no resources");
        return;
    }

    resources.reserve(numResources);
    for(size_t i = 0; i < numResources; ++i) {
        resources.push_back(ResourceAdapter(localResources[i]));
    }
}

void Note::setResources(const std::vector<IResource> & resources)
{
    auto & localResources = m_enNote.resources;
    localResources.clear();

    size_t numResources = resources.size();
    if (numResources == 0) {
        m_enNote.__isset.resources = false;
        QNDEBUG("Note::setResources: no resources");
        return;
    }

    localResources.reserve(numResources);
    for(size_t i = 0; i < numResources; ++i) {
        localResources.push_back(resources[i].GetEnResource());
    }

    m_enNote.__isset.resources = true;
    QNDEBUG("Added " << numResources << " resources to note");
}

void Note::addResource(const IResource & resource)
{
    auto & localResources = m_enNote.resources;

    if (m_enNote.__isset.resources)
    {
        if (std::find(localResources.cbegin(), localResources.cend(), resource.GetEnResource()) != localResources.cend()) {
            QNDEBUG("Note::addResource: already has resource with guid " << resource.guid());
            return;
        }
    }
    else
    {
        localResources.clear();
        m_enNote.__isset.resources = true;
    }

    localResources.push_back(resource.GetEnResource());
    QNDEBUG("Added resource with guid " << resource.guid() << " to note");
}

void Note::removeResource(const IResource & resource)
{
    auto & localResources = m_enNote.resources;

    if (m_enNote.__isset.resources)
    {
        auto it = std::find(localResources.begin(), localResources.end(), resource.GetEnResource());
        if (it == localResources.end()) {
            QNDEBUG("Note::removeResource: note has no resource with guid " << resource.guid());
            return;
        }

        localResources.erase(it);
        QNDEBUG("Removed resource with guid " << resource.guid() << " from note");
    }
    else
    {
        QNDEBUG("Note::removeResource: note has no resources set, nothing to remove");
    }
}

bool Note::hasNoteAttributes() const
{
    return m_enNote.__isset.attributes;
}

const QByteArray Note::noteAttributes() const
{
    return std::move(GetSerializedNoteAttributes(m_enNote.attributes));
}

void Note::setNoteAttributes(const QByteArray & noteAttributes)
{
    m_enNote.attributes = GetDeserializedNoteAttributes(noteAttributes);

    if (!noteAttributes.isEmpty()) {
        m_enNote.__isset.attributes = true;
    }
    else {
        m_enNote.__isset.attributes = false;
    }
}

bool Note::isLocal() const
{
    return m_isLocal;
}

void Note::setLocal(const bool local)
{
    m_isLocal = local;
}

bool Note::isDeleted() const
{
    return m_isDeleted;
}

void Note::setDeleted(const bool deleted)
{
    m_isDeleted = deleted;
}

QTextStream & Note::Print(QTextStream & strm) const
{
    strm << "Note: { \n";

    const auto & isSet = m_enNote.__isset;

#define INSERT_DELIMITER \
    strm << "; \n";

    if (isSet.guid) {
        strm << "guid: " << QString::fromStdString(m_enNote.guid);
    }
    else {
        strm << "guid is not set";
    }
    INSERT_DELIMITER;

    if (isSet.updateSequenceNum) {
        strm << "updateSequenceNumber: " << QString::number(m_enNote.updateSequenceNum);
    }
    else {
        strm << "updateSequenceNumber is not set";
    }
    INSERT_DELIMITER;

    if (isSet.title) {
        strm << "title: " << QString::fromStdString(m_enNote.title);
    }
    else {
        strm << "title is not set";
    }
    INSERT_DELIMITER;

    if (isSet.content) {
        strm << "content: " << QString::fromStdString(m_enNote.content);
    }
    else {
        strm << "content is not set";
    }
    INSERT_DELIMITER;

    if (isSet.contentHash) {
        strm << "contentHash: " << QString::fromStdString(m_enNote.contentHash);
    }
    else {
        strm << "contentHash is not set";
    }
    INSERT_DELIMITER;

    if (isSet.contentLength) {
        strm << "contentLength: " << QString::number(m_enNote.contentLength);
    }
    else {
        strm << "contentLength is not set";
    }
    INSERT_DELIMITER;

    if (isSet.created) {
        strm << "creationTimestamp: " << m_enNote.created << ", datetime: "
             << PrintableDateTimeFromTimestamp(m_enNote.created);
    }
    else {
        strm << "creationTimestamp is not set";
    }
    INSERT_DELIMITER;

    if (isSet.updated) {
        strm << "modificationTimestamp: " << m_enNote.updated << ", datetime: "
             << PrintableDateTimeFromTimestamp(m_enNote.updated);
    }
    else {
        strm << "modificationTimestamp is not set";
    }
    INSERT_DELIMITER;

    if (isSet.deleted) {
        strm << "deletionTimestamp: " << m_enNote.deleted << ", datetime: "
             << PrintableDateTimeFromTimestamp(m_enNote.deleted);
    }
    else {
        strm << "deletionTimestamp is not set";
    }
    INSERT_DELIMITER;

    if (isSet.active) {
        strm << "active: " << (m_enNote.active ? "true" : "false");
    }
    else {
        strm << "active is not set";
    }
    INSERT_DELIMITER;

    if (isSet.notebookGuid) {
        strm << "notebookGuid: " << QString::fromStdString(m_enNote.notebookGuid);
    }
    else {
        strm << "notebookGuid is not set";
    }
    INSERT_DELIMITER;

    if (isSet.tagGuids)
    {
        strm << "tagGuids: {";
        for(const auto & tagGuid: m_enNote.tagGuids) {
            strm << QString::fromStdString(tagGuid) << "; ";
        }
        strm << "}";
    }
    else {
        strm << "tagGuids are not set";
    }
    INSERT_DELIMITER;

    if (isSet.resources)
    {
        strm << "resources: { \n";
        for(const auto & resource: m_enNote.resources) {
            strm << ResourceAdapter(resource) << "; \n";
        }
        strm << "}";
    }
    else {
        strm << "resources are not set";
    }
    INSERT_DELIMITER;

    if (isSet.attributes) {
        strm << "attributes: " << m_enNote.attributes;
    }
    else {
        strm << "attributes are not set";
    }
    INSERT_DELIMITER;

    strm << "isDirty: " << (isDirty() ? "true" : "false");
    INSERT_DELIMITER;

    strm << "isLocal: " << (m_isLocal ? "true" : "false");
    INSERT_DELIMITER;

    strm << "isDeleted: " << (m_isDeleted ? "true" : "false");
    INSERT_DELIMITER;

#undef INSERT_DELIMITER

    return strm;
}

} // namespace qute_note
