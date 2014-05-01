#include "Note.h"
#include "ResourceAdapter.h"
#include "QEverCloudHelpers.h"
#include "../Utility.h"
#include "../Serialization.h"
#include <logging/QuteNoteLogger.h>
#include <functional>
#include <Limits_constants.h>

namespace qute_note {

Note::Note(const qevercloud::Note & other) :
    NoteStoreDataElement(),
    m_qecNote(other),
    m_isLocal(false),
    m_isDeleted(false)
{}

Note::Note(qevercloud::Note && other) :
    NoteStoreDataElement(),
    m_qecNote(std::move(other)),
    m_isLocal(false),
    m_isDeleted(false)
{}

Note & Note::operator=(const qevercloud::Note & other)
{
    m_qecNote = other;
}

Note & Note::operator=(qevercloud::Note && other)
{
    m_qecNote = std::move(other);
}

Note::~Note()
{}

bool Note::operator==(const Note & other) const
{
    return ((m_qecNote == other.m_qecNote) && (isDirty() == other.isDirty()) &&
            (isDeleted() == other.isDeleted()));
}

bool Note::operator!=(const Note & other) const
{
    return !(*this == other);
}

bool Note::hasGuid() const
{
    return m_qecNote.guid.isSet();
}

const QString Note::guid() const
{
    return m_qecNote.guid;
}

void Note::setGuid(const QString & guid)
{
    m_qecNote.guid = guid;
}

bool Note::hasUpdateSequenceNumber() const
{
    return m_qecNote.updateSequenceNum.isSet();
}

qint32 Note::updateSequenceNumber() const
{
    return m_qecNote.updateSequenceNum;
}

void Note::setUpdateSequenceNumber(const qint32 usn)
{
    // TODO: set whether it's really necessary
    if (usn >= 0) {
        m_qecNote.updateSequenceNum = usn;
    }
    else {
        m_qecNote.updateSequenceNum.clear();
    }
}

void Note::clear()
{
    m_qecNote = qevercloud::Note();
}

bool Note::checkParameters(QString & errorDescription) const
{
    if (!m_qecNote.guid.isSet()) {
        errorDescription = QObject::tr("Note's guid is not set");
        return false;
    }
    else if (!CheckGuid(m_qecNote.guid)) {
        errorDescription = QObject::tr("Note's guid is invalid");
        return false;
    }

    if (!m_qecNote.updateSequenceNum.isSet()) {
        errorDescription = QObject::tr("Note's update sequence number is not set");
        return false;
    }
    else if (!CheckUpdateSequenceNumber(m_qecNote.updateSequenceNum)) {
        errorDescription = QObject::tr("Note's update sequence number is invalid");
        return false;
    }

    if (!m_qecNote.title.isSet()) {
        errorDescription = QObject::tr("Note's title is not set");
        return false;
    }

    size_t titleSize = m_qecNote.title->size();

    if ( (titleSize < qevercloud::EDAM_NOTE_TITLE_LEN_MIN) ||
         (titleSize > qevercloud::EDAM_NOTE_TITLE_LEN_MAX) )
    {
        errorDescription = QObject::tr("Note's title length is invalid");
        return false;
    }

    if (!m_qecNote.content.isSet()) {
        errorDescription = QObject::tr("Note's content is not set");
        return false;
    }

    size_t contentSize = m_qecNote.content->size();

    if ( (contentSize < qevercloud::EDAM_NOTE_CONTENT_LEN_MIN) ||
         (contentSize > qevercloud::EDAM_NOTE_CONTENT_LEN_MAX) )
    {
        errorDescription = QObject::tr("Note's content length is invalid");
        return false;
    }

    if (m_qecNote.contentHash.isSet()) {
        size_t contentHashSize = m_qecNote.contentHash->size();

        if (contentHashSize != qevercloud::EDAM_HASH_LEN) {
            errorDescription = QObject::tr("Note's content hash size is invalid");
            return false;
        }
    }

    if (!m_qecNote.notebookGuid.isSet()) {
        errorDescription = QObject::tr("Note's notebook Guid is not set");
        return false;
    }
    else if (!CheckGuid(m_qecNote.notebookGuid)) {
        errorDescription = QObject::tr("Note's notebook guid is invalid");
        return false;
    }

    if (m_qecNote.tagGuids.isSet()) {
        size_t numTagGuids = m_qecNote.tagGuids->size();

        if (numTagGuids > qevercloud::EDAM_NOTE_TAGS_MAX) {
            errorDescription = QObject::tr("Note has too many tags, max allowed ");
            errorDescription.append(QString::number(qevercloud::EDAM_NOTE_TAGS_MAX));
            return false;
        }
    }

    if (m_qecNote.resources.isSet()) {
        size_t numResources = m_qecNote.resources->size();

        if (numResources > qevercloud::EDAM_NOTE_RESOURCES_MAX) {
            errorDescription = QObject::tr("Note has too many resources, max allowed ");
            errorDescription.append(QString::number(qevercloud::EDAM_NOTE_RESOURCES_MAX));
            return false;
        }
    }

    if (!m_qecNote.created.isSet()) {
        errorDescription = QObject::tr("Note's creation timestamp is not set");
        return false;
    }

    if (!m_qecNote.updated.isSet()) {
        errorDescription = QObject::tr("Note's modification timestamp is not set");
        return false;
    }

    if (!m_qecNote.attributes.isSet()) {
        return true;
    }

    const qevercloud::NoteAttributes & attributes = m_qecNote.attributes;

#define CHECK_NOTE_ATTRIBUTE(name) \
    if (attributes.name.isSet()) { \
        size_t name##Size = attributes.name->size(); \
        \
        if ( (name##Size < qevercloud::EDAM_ATTRIBUTE_LEN_MIN) || \
             (name##Size > qevercloud::EDAM_ATTRIBUTE_LEN_MAX) ) \
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

    if (attributes.contentClass.isSet()) {
        int contentClassSize = attributes.contentClass->size();

        if ( (contentClassSize < qevercloud::EDAM_NOTE_CONTENT_CLASS_LEN_MIN) ||
             (contentClassSize > qevercloud::EDAM_NOTE_CONTENT_CLASS_LEN_MAX) )
        {
            errorDescription = QObject::tr("Note attributes' content class has invalid size");
            return false;
        }
    }

    if (attributes.applicationData.isSet()) {
        const qevercloud::LazyMap & applicationData = attributes.applicationData;
        const QSet<QString> & keysOnly = applicationData.keysOnly;
        const QMap<QString, QString> & fullMap = applicationData.fullMap;

        foreach(const QString & key, keysOnly) {
            int keySize = key.size();
            if ( (keySize < qevercloud::EDAM_APPLICATIONDATA_NAME_LEN_MIN) ||
                 (keySize > qevercloud::EDAM_APPLICATIONDATA_NAME_LEN_MAX) )
            {
                errorDescription = QObject::tr("Note's attributes application data has invalid key (in keysOnly part)");
                return false;
            }
        }

        for(QMap<QString, QString>::const_iterator it = fullMap.constBegin(); it != fullMap.constEnd(); ++it) {
            int keySize = it.value().size();
            if ( (keySize < qevercloud::EDAM_APPLICATIONDATA_NAME_LEN_MIN) ||
                 (keySize > qevercloud::EDAM_APPLICATIONDATA_NAME_LEN_MAX) )
            {
                errorDescription = QObject::tr("Note's attributes application data has invalid key (in fullMap part)");
                return false;
            }

            int valueSize = it.value().size();
            if ( (valueSize < qevercloud::EDAM_APPLICATIONDATA_VALUE_LEN_MIN) ||
                 (valueSize > qevercloud::EDAM_APPLICATIONDATA_VALUE_LEN_MAX) )
            {
                errorDescription = QObject::tr("Note's attributes application data has invalid value");
                return false;
            }

            int sumSize = keySize + valueSize;
            if (sumSize > qevercloud::EDAM_APPLICATIONDATA_ENTRY_LEN_MAX) {
                errorDescription = QObject::tr("Note's attributes application data has invalid entry size");
                return false;
            }
        }
    }

    if (attributes.classifications.isSet()) {
        const QMap<QString, QString> & classifications = attributes.classifications;
        for(QMap<QString, QString>::const_iterator it = classifications.constBegin();
            it != classifications.constEnd(); ++it)
        {
            const QString & value = it.value();
            if (!value.startsWith("CLASSIFICATION_")) {
                errorDescription = QObject::tr("Note's attributes classifications has invalid classification value");
                return false;
            }
        }
    }

    return true;
}

bool Note::hasTitle() const
{
    return m_qecNote.title.isSet();
}

const QString & Note::title() const
{
    return m_qecNote.title;
}

void Note::setTitle(const QString & title)
{
    m_qecNote.title = title;
}

bool Note::hasContent() const
{
    return m_qecNote.content.isSet();
}

const QString & Note::content() const
{
    return m_qecNote.content;
}

void Note::setContent(const QString & content)
{
    m_qecNote.content = content;
}

bool Note::hasContentHash() const
{
    return m_qecNote.contentHash.isSet();
}

const QString & Note::contentHash() const
{
    return m_qecNote.contentHash;
}

void Note::setContentHash(const QString & contentHash)
{
    m_qecNote.contentHash = contentHash;
}

bool Note::hasContentLength() const
{
    return m_qecNote.contentLength.isSet();
}

qint32 Note::contentLength() const
{
    return m_qecNote.contentLength;
}

void Note::setContentLength(const qint32 length)
{
    if (length >= 0) {
        m_qecNote.contentLength = length;
    }
    else {
        m_qecNote.contentLength.clear();
    }
}

bool Note::hasCreationTimestamp() const
{
    return m_qecNote.created.isSet();
}

qint64 Note::creationTimestamp() const
{
    return m_qecNote.created;
}

void Note::setCreationTimestamp(const qint64 timestamp)
{
    if (timestamp >= 0) {
        m_qecNote.created = timestamp;
    }
    else {
        m_qecNote.created.clear();
    }
}

bool Note::hasModificationTimestamp() const
{
    return m_qecNote.updated.isSet();
}

qint64 Note::modificationTimestamp() const
{
    return m_qecNote.updated;
}

void Note::setModificationTimestamp(const qint64 timestamp)
{
    if (timestamp >= 0) {
        m_qecNote.updated = timestamp;
    }
    else {
        m_qecNote.updated.clear();
    }
}

bool Note::hasDeletionTimestamp() const
{
    return m_qecNote.deleted.isSet();
}

qint64 Note::deletionTimestamp() const
{
    return m_qecNote.deleted;
}

void Note::setDeletionTimestamp(const qint64 timestamp)
{
    if (timestamp >= 0) {
        m_qecNote.deleted = timestamp;
    }
    else {
        m_qecNote.deleted.clear();
    }
}

bool Note::hasActive() const
{
    return m_qecNote.active.isSet();
}

bool Note::active() const
{
    return m_qecNote.active;
}

void Note::setActive(const bool active)
{
    m_qecNote.active = active;
}

bool Note::hasNotebookGuid() const
{
    return m_qecNote.notebookGuid.isSet();
}

const QString & Note::notebookGuid() const
{
    return m_qecNote.notebookGuid;
}

void Note::setNotebookGuid(const QString & guid)
{
    m_qecNote.notebookGuid = guid;
}

bool Note::hasTagGuids() const
{
    return m_qecNote.tagGuids.isSet();
}

void Note::tagGuids(QStringList & guids) const
{
    guids.clear();

    if (!m_qecNote.tagGuids.isSet()) {
        QNDEBUG("Note::tagGuids:: no tag guids set");
        return;
    }

    const QList<QString> & localTagGuids = m_qecNote.tagGuids;
    size_t numTagGuids = localTagGuids.size();
    if (numTagGuids == 0) {
        QNDEBUG("Note::tagGuids: no tags");
        return;
    }

    guids.reserve(numTagGuids);
    foreach(const QString & guid, localTagGuids) {
        guids << guid;
    }
}

void Note::setTagGuids(const QStringList & guids)
{
    if (guids.isEmpty()) {
        m_qecNote.tagGuids.clear();
        return;
    }

    int numTagGuids = guids.size();

    if (!m_qecNote.tagGuids.isSet()) {
        m_qecNote.tagGuids = QList<QString>();
    }

    QList<QString> & localTagGuids = m_qecNote.tagGuids;
    localTagGuids.clear();

    localTagGuids.reserve(numTagGuids);
    foreach(const QString & guid, guids) {
        localTagGuids << guid;
    }

    QNDEBUG("Added " << numTagGuids << " tag guids to note");
}

void Note::addTagGuid(const QString & guid)
{
    if (guid.isEmpty()) {
        return;
    }

    if (!m_qecNote.tagGuids.isSet()) {
        m_qecNote.tagGuids = QList<QString>();
    }

    if (!m_qecNote.tagGuids->contains(guid)) {
        m_qecNote.tagGuids << guid;
        QNDEBUG("Added tag with guid " << guid << " to note");
    }
}

void Note::removeTagGuid(const QString & guid)
{
    if (guid.isEmpty()) {
        QNDEBUG("Cannot remove empty tag guid");
        return;
    }

    if (!m_qecNote.tagGuids.isSet()) {
        QNDEBUG("No tag guids are set, cannot remove one");
        return;
    }

    int removed = m_qecNote.tagGuids->removeAll(guid);
    if (removed > 0) {
        QNDEBUG("Removed tag guid " << guid << " (" << removed << ") occurences");
    }
    else {
        QNDEBUG("Haven't removed tag guid " << guid << " because there was no such guid within note's tag guids");
    }
}

bool Note::hasResources() const
{
    return m_qecNote.resources.isSet();
}

void Note::resources(QList<ResourceAdapter> & resources) const
{
    resources.clear();

    if (!m_qecNote.resources.isSet()) {
        return;
    }

    int numResources = m_qecNote.resources->size();
    resources.reserve(std::max(numResources, 0));
    foreach(const qevercloud::Resource & resource, m_qecNote.resources) {
        resources << ResourceAdapter(resource);
    }
}

// TODO: continue from here

void Note::setResources(const QList<IResource> &resources)
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
