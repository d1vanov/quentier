#include "Note.h"
#include "ResourceAdapter.h"
#include "ResourceWrapper.h"
#include "QEverCloudHelpers.h"
#include "../Utility.h"
#include <client/enml/ENMLConverter.h>
#include <logging/QuteNoteLogger.h>
#include <QDomDocument>

namespace qute_note {

Note::Note() :
    DataElementWithShortcut(),
    m_isLocal(true),
    m_thumbnail(),
    m_lazyPlainText(),
    m_lazyPlainTextIsValid(false),
    m_lazyListOfWords(),
    m_lazyListOfWordsIsValid(false),
    m_lazyContainsCheckedToDo(-1),
    m_lazyContainsUncheckedToDo(-1)
{}

Note::Note(const qevercloud::Note & other) :
    DataElementWithShortcut(),
    m_qecNote(other),
    m_isLocal(false),
    m_thumbnail(),
    m_lazyPlainText(),
    m_lazyPlainTextIsValid(false),
    m_lazyListOfWords(),
    m_lazyListOfWordsIsValid(false),
    m_lazyContainsCheckedToDo(-1),
    m_lazyContainsUncheckedToDo(-1)
{}

Note::Note(Note && other) :
    DataElementWithShortcut(std::move(other)),
    m_qecNote(std::move(other.m_qecNote)),
    m_isLocal(std::move(other.m_isLocal)),
    m_thumbnail(std::move(other.m_thumbnail)),
    m_lazyPlainText(),
    m_lazyPlainTextIsValid(other.m_lazyPlainTextIsValid),
    m_lazyListOfWords(),
    m_lazyListOfWordsIsValid(false),
    m_lazyContainsCheckedToDo(other.m_lazyContainsCheckedToDo),
    m_lazyContainsUncheckedToDo(other.m_lazyContainsUncheckedToDo)
{
    m_lazyPlainText = other.m_lazyPlainText;
    other.m_lazyPlainText.clear();

    m_lazyPlainTextIsValid = other.m_lazyPlainTextIsValid;
    other.m_lazyPlainTextIsValid = false;

    m_lazyListOfWords = other.m_lazyListOfWords;
    other.m_lazyListOfWords.clear();

    m_lazyListOfWordsIsValid = other.m_lazyListOfWordsIsValid;
    other.m_lazyListOfWordsIsValid = false;
}

Note::Note(qevercloud::Note && other) :
    DataElementWithShortcut(),
    m_qecNote(std::move(other)),
    m_isLocal(false),
    m_thumbnail(),
    m_lazyPlainText(),
    m_lazyPlainTextIsValid(false),
    m_lazyListOfWords(),
    m_lazyListOfWordsIsValid(false),
    m_lazyContainsCheckedToDo(-1),
    m_lazyContainsUncheckedToDo(-1)
{}

Note & Note::operator=(const qevercloud::Note & other)
{
    m_qecNote = other;
    m_lazyPlainTextIsValid = false;    // Mark any existing plain text as invalid but don't free memory
    m_lazyListOfWordsIsValid = false;
    m_lazyContainsCheckedToDo = -1;
    m_lazyContainsUncheckedToDo = -1;
    return *this;
}

Note & Note::operator=(Note && other)
{
    if (this != &other)
    {
        NoteStoreDataElement::operator=(other);
        m_qecNote = std::move(other.m_qecNote);
        m_isLocal = std::move(other.m_isLocal);
        m_thumbnail = std::move(other.m_thumbnail);

        m_lazyPlainText = other.m_lazyPlainText;
        other.m_lazyPlainText.clear();

        m_lazyPlainTextIsValid = other.m_lazyPlainTextIsValid;
        other.m_lazyPlainTextIsValid = false;

        m_lazyListOfWords = other.m_lazyListOfWords;
        other.m_lazyListOfWords.clear();

        m_lazyListOfWordsIsValid = other.m_lazyListOfWordsIsValid;
        other.m_lazyListOfWordsIsValid = false;

        m_lazyContainsCheckedToDo = other.m_lazyContainsCheckedToDo;
        m_lazyContainsUncheckedToDo = other.m_lazyContainsUncheckedToDo;
    }

    return *this;
}

Note & Note::operator=(qevercloud::Note && other)
{
    m_qecNote = std::move(other);
    m_lazyPlainTextIsValid = false;    // Mark any existing plain text as invalid but don't free memory
    m_lazyListOfWordsIsValid = false;
    m_lazyContainsCheckedToDo = -1;
    m_lazyContainsUncheckedToDo = -1;
    return *this;
}

Note::~Note()
{}

bool Note::operator==(const Note & other) const
{
    return ((m_qecNote == other.m_qecNote) && (isDirty() == other.isDirty()) &&
            (hasShortcut() == other.hasShortcut()));
    // NOTE: thumbnail doesn't take part in comparison because it's merely a helper
    // for note displaying widget, nothing more
}

bool Note::operator!=(const Note & other) const
{
    return !(*this == other);
}

bool Note::hasGuid() const
{
    return m_qecNote.guid.isSet();
}

const QString & Note::guid() const
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
    // TODO: see whether it's really necessary
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
    m_lazyPlainTextIsValid = false;    // Mark any existing plain text as invalid but don't free memory
    m_lazyListOfWordsIsValid = false;
    m_lazyContainsCheckedToDo = -1;
    m_lazyContainsUncheckedToDo = -1;
}

bool Note::checkParameters(QString & errorDescription) const
{
    if (localGuid().isEmpty() && !m_qecNote.guid.isSet()) {
        errorDescription = QT_TR_NOOP("Both Note's local and remote guids are empty");
        return false;
    }

    if (m_qecNote.guid.isSet() && !CheckGuid(m_qecNote.guid.ref())) {
        errorDescription = QT_TR_NOOP("Note's guid is invalid");
        return false;
    }

    if (!m_qecNote.updateSequenceNum.isSet()) {
        errorDescription = QT_TR_NOOP("Note's update sequence number is not set");
        return false;
    }
    else if (!CheckUpdateSequenceNumber(m_qecNote.updateSequenceNum)) {
        errorDescription = QT_TR_NOOP("Note's update sequence number is invalid");
        return false;
    }

    if (!m_qecNote.title.isSet()) {
        errorDescription = QT_TR_NOOP("Note's title is not set");
        return false;
    }

    size_t titleSize = m_qecNote.title->size();

    if ( (titleSize < qevercloud::EDAM_NOTE_TITLE_LEN_MIN) ||
         (titleSize > qevercloud::EDAM_NOTE_TITLE_LEN_MAX) )
    {
        errorDescription = QT_TR_NOOP("Note's title length is invalid");
        return false;
    }

    if (!m_qecNote.content.isSet()) {
        errorDescription = QT_TR_NOOP("Note's content is not set");
        return false;
    }

    size_t contentSize = m_qecNote.content->size();

    if ( (contentSize < qevercloud::EDAM_NOTE_CONTENT_LEN_MIN) ||
         (contentSize > qevercloud::EDAM_NOTE_CONTENT_LEN_MAX) )
    {
        errorDescription = QT_TR_NOOP("Note's content length is invalid");
        return false;
    }

    if (m_qecNote.contentHash.isSet()) {
        size_t contentHashSize = m_qecNote.contentHash->size();

        if (contentHashSize != qevercloud::EDAM_HASH_LEN) {
            errorDescription = QT_TR_NOOP("Note's content hash size is invalid");
            return false;
        }
    }

    if (!m_qecNote.notebookGuid.isSet()) {
        errorDescription = QT_TR_NOOP("Note's notebook Guid is not set");
        return false;
    }
    else if (!CheckGuid(m_qecNote.notebookGuid.ref())) {
        errorDescription = QT_TR_NOOP("Note's notebook guid is invalid");
        return false;
    }

    if (m_qecNote.tagGuids.isSet()) {
        size_t numTagGuids = m_qecNote.tagGuids->size();

        if (numTagGuids > qevercloud::EDAM_NOTE_TAGS_MAX) {
            errorDescription = QT_TR_NOOP("Note has too many tags, max allowed ");
            errorDescription.append(QString::number(qevercloud::EDAM_NOTE_TAGS_MAX));
            return false;
        }
    }

    if (m_qecNote.resources.isSet()) {
        size_t numResources = m_qecNote.resources->size();

        if (numResources > qevercloud::EDAM_NOTE_RESOURCES_MAX) {
            errorDescription = QT_TR_NOOP("Note has too many resources, max allowed ");
            errorDescription.append(QString::number(qevercloud::EDAM_NOTE_RESOURCES_MAX));
            return false;
        }
    }

    if (!m_qecNote.created.isSet()) {
        errorDescription = QT_TR_NOOP("Note's creation timestamp is not set");
        return false;
    }

    if (!m_qecNote.updated.isSet()) {
        errorDescription = QT_TR_NOOP("Note's modification timestamp is not set");
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
            errorDescription = QT_TR_NOOP("Note attributes' " #name " field has invalid size"); \
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
            errorDescription = QT_TR_NOOP("Note attributes' content class has invalid size");
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
                errorDescription = QT_TR_NOOP("Note's attributes application data has invalid key (in keysOnly part)");
                return false;
            }
        }

        for(QMap<QString, QString>::const_iterator it = fullMap.constBegin(); it != fullMap.constEnd(); ++it) {
            int keySize = it.value().size();
            if ( (keySize < qevercloud::EDAM_APPLICATIONDATA_NAME_LEN_MIN) ||
                 (keySize > qevercloud::EDAM_APPLICATIONDATA_NAME_LEN_MAX) )
            {
                errorDescription = QT_TR_NOOP("Note's attributes application data has invalid key (in fullMap part)");
                return false;
            }

            int valueSize = it.value().size();
            if ( (valueSize < qevercloud::EDAM_APPLICATIONDATA_VALUE_LEN_MIN) ||
                 (valueSize > qevercloud::EDAM_APPLICATIONDATA_VALUE_LEN_MAX) )
            {
                errorDescription = QT_TR_NOOP("Note's attributes application data has invalid value");
                return false;
            }

            int sumSize = keySize + valueSize;
            if (sumSize > qevercloud::EDAM_APPLICATIONDATA_ENTRY_LEN_MAX) {
                errorDescription = QT_TR_NOOP("Note's attributes application data has invalid entry size");
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
                errorDescription = QT_TR_NOOP("Note's attributes classifications has invalid classification value");
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
    m_lazyPlainTextIsValid = false;    // Mark any existing plain text as invalid but don't free memory
    m_lazyListOfWordsIsValid = false;
    m_lazyContainsCheckedToDo = -1;
    m_lazyContainsUncheckedToDo = -1;
}

bool Note::hasContentHash() const
{
    return m_qecNote.contentHash.isSet();
}

const QByteArray & Note::contentHash() const
{
    return m_qecNote.contentHash;
}

void Note::setContentHash(const QByteArray & contentHash)
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
        m_qecNote.tagGuids.ref() << guid;
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

QList<ResourceAdapter> Note::resourceAdapters() const
{
    QList<ResourceAdapter> resources;

    if (!m_qecNote.resources.isSet()) {
        return resources;
    }

    const QList<qevercloud::Resource> & noteResources = m_qecNote.resources.ref();
    int numResources = noteResources.size();
    int numResourceAdditionalInfoEntries = m_resourcesAdditionalInfo.size();

    resources.reserve(std::max(numResources, 0));
    for(int i = 0; i < numResources; ++i)
    {
        resources << ResourceAdapter(noteResources[i]);
        ResourceAdapter & resource = resources.back();

        if (i < numResourceAdditionalInfoEntries) {
            const ResourceAdditionalInfo & info = m_resourcesAdditionalInfo[i];
            resource.setLocalGuid(info.localGuid);
            if (!info.noteLocalGuid.isEmpty()) {
                resource.setNoteLocalGuid(info.noteLocalGuid);
            }
            resource.setDirty(info.isDirty);
        }

        resource.setIndexInNote(i);
    }
    return resources;
}

QList<ResourceWrapper> Note::resources() const
{
    QList<ResourceWrapper> resources;

    if (!m_qecNote.resources.isSet()) {
        return resources;
    }

    const QList<qevercloud::Resource> & noteResources = m_qecNote.resources.ref();
    int numResources = noteResources.size();
    int numResourceAdditionalInfoEntries = m_resourcesAdditionalInfo.size();

    resources.reserve(qMax(numResources, 0));
    for(int i = 0; i < numResources; ++i)
    {
        resources << ResourceWrapper(noteResources[i]);
        ResourceWrapper & resource = resources.back();

        if (i < numResourceAdditionalInfoEntries) {
            const ResourceAdditionalInfo & info = m_resourcesAdditionalInfo[i];
            resource.setLocalGuid(info.localGuid);
            if (!info.noteLocalGuid.isEmpty()) {
                resource.setNoteLocalGuid(info.noteLocalGuid);
            }
            resource.setDirty(info.isDirty);
        }

        resource.setIndexInNote(i);
    }
    return resources;
}

#define SET_RESOURCES(resource_type) \
    void Note::setResources(const QList<resource_type> & resources) \
    { \
        if (!resources.empty()) \
        { \
            m_qecNote.resources = QList<qevercloud::Resource>(); \
            m_resourcesAdditionalInfo.clear(); \
            ResourceAdditionalInfo info; \
            for(QList<resource_type>::const_iterator it = resources.constBegin(); \
                it != resources.constEnd(); ++it) \
            { \
                m_qecNote.resources.ref() << it->GetEnResource(); \
                info.localGuid = it->localGuid(); \
                if (it->hasNoteLocalGuid()) { \
                    info.noteLocalGuid = it->noteLocalGuid(); \
                } \
                info.isDirty = it->isDirty(); \
                m_resourcesAdditionalInfo.push_back(info); \
            } \
            QNDEBUG("Added " << resources.size() << " resources to note"); \
        } \
        else \
        { \
            m_qecNote.resources.clear(); \
        } \
    }

SET_RESOURCES(ResourceWrapper)
SET_RESOURCES(ResourceAdapter)

#undef SET_RESOURCES

void Note::addResource(const IResource & resource)
{
    if (!m_qecNote.resources.isSet()) {
        m_qecNote.resources = QList<qevercloud::Resource>();
    }

    if (m_qecNote.resources->contains(resource.GetEnResource())) {
        QNDEBUG("Can't add resource to note: this note already has this resource");
        return;
    }

    m_qecNote.resources.ref() << resource.GetEnResource();
    ResourceAdditionalInfo info;
    info.localGuid = resource.localGuid();
    if (resource.hasNoteLocalGuid()) {
        info.noteLocalGuid = resource.noteLocalGuid();
    }
    info.isDirty = resource.isDirty();
    m_resourcesAdditionalInfo.push_back(info);

    QNDEBUG("Added resource to note, local guid = " << resource.localGuid());
}

void Note::removeResource(const IResource & resource)
{
    if (!m_qecNote.resources.isSet()) {
        QNDEBUG("Can't remove resource from note: note has no attached resources");
        return;
    }

    int removed = m_qecNote.resources->removeAll(resource.GetEnResource());
    if (removed > 0) {
        QNDEBUG("Removed resource " << resource << " from note (" << removed << ") occurences");
        ResourceAdditionalInfo info;
        info.localGuid = resource.localGuid();
        if (resource.hasNoteLocalGuid()) {
            info.noteLocalGuid = resource.noteLocalGuid();
        }
        info.isDirty = resource.isDirty();
        m_resourcesAdditionalInfo.removeAll(info);
    }
    else {
        QNDEBUG("Haven't removed resource " << resource << " because there was no such resource attached to the note");
    }
}

bool Note::hasNoteAttributes() const
{
    return m_qecNote.attributes.isSet();
}

const qevercloud::NoteAttributes & Note::noteAttributes() const
{
    return m_qecNote.attributes;
}

qevercloud::NoteAttributes & Note::noteAttributes()
{
    if (!m_qecNote.attributes.isSet()) {
        m_qecNote.attributes = qevercloud::NoteAttributes();
    }

    return m_qecNote.attributes;
}

bool Note::isLocal() const
{
    return m_isLocal;
}

void Note::setLocal(const bool local)
{
    m_isLocal = local;
}

QImage Note::thumbnail() const
{
    return m_thumbnail;
}

void Note::setThumbnail(const QImage & thumbnail)
{
    m_thumbnail = thumbnail;
}

QString Note::plainText(QString * errorMessage) const
{
    if (m_lazyPlainTextIsValid) {
        return m_lazyPlainText;
    }

    if (!m_qecNote.content.isSet()) {
        if (errorMessage) {
            *errorMessage = "Note content is not set";
        }
        return QString();
    }

    ENMLConverter converter;
    QString error;
    bool res = converter.noteContentToPlainText(m_qecNote.content.ref(),
                                                m_lazyPlainText, error);
    if (!res) {
        QNWARNING(error);
        if (errorMessage) {
            *errorMessage = error;
        }
        return QString();
    }

    m_lazyPlainTextIsValid = true;

    return m_lazyPlainText;
}

QStringList Note::listOfWords(QString * errorMessage) const
{
    if (m_lazyListOfWordsIsValid) {
        return m_lazyListOfWords;
    }

    if (m_lazyPlainTextIsValid) {
        m_lazyListOfWords = ENMLConverter::plainTextToListOfWords(m_lazyPlainText);
        m_lazyListOfWordsIsValid = true;
        return m_lazyListOfWords;
    }

    // If still not returned, there's no plain text available so will get the list of words
    // from Note's content instead

    ENMLConverter converter;
    QString error;
    bool res = converter.noteContentToListOfWords(m_qecNote.content.ref(), m_lazyListOfWords,
                                                  error, &m_lazyPlainText);
    if (!res) {
        QNWARNING(error);
        if (errorMessage) {
            *errorMessage = error;
        }
        return QStringList();
    }

    m_lazyPlainTextIsValid = true;
    m_lazyListOfWordsIsValid = true;
    return m_lazyListOfWords;
}

bool Note::containsCheckedTodo() const
{
    return containsToDoImpl(/* checked = */ true);
}

bool Note::containsUncheckedTodo() const
{
    return containsToDoImpl(/* checked = */ false);
}

bool Note::containsTodo() const
{
    return (containsUncheckedTodo() || containsCheckedTodo());
}

QTextStream & Note::Print(QTextStream & strm) const
{
    strm << "Note: { \n";

#define INSERT_DELIMITER \
    strm << "; \n";

    const QString _localGuid = localGuid();
    if (!_localGuid.isEmpty()) {
        strm << "localGuid: " << _localGuid;
    }
    else {
        strm << "localGuid is not set";
    }
    INSERT_DELIMITER;

    if (m_qecNote.guid.isSet()) {
        strm << "guid: " << m_qecNote.guid;
    }
    else {
        strm << "guid is not set";
    }
    INSERT_DELIMITER;

    if (m_qecNote.updateSequenceNum.isSet()) {
        strm << "updateSequenceNumber: " << QString::number(m_qecNote.updateSequenceNum);
    }
    else {
        strm << "updateSequenceNumber is not set";
    }
    INSERT_DELIMITER;

    if (m_qecNote.title.isSet()) {
        strm << "title: " << m_qecNote.title;
    }
    else {
        strm << "title is not set";
    }
    INSERT_DELIMITER;

    if (m_qecNote.content.isSet()) {
        strm << "content: " << m_qecNote.content;
    }
    else {
        strm << "content is not set";
    }
    INSERT_DELIMITER;

    if (m_lazyContainsCheckedToDo > (-1)) {
        strm << "m_lazyContainsCheckedToDo = " << QString::number(m_lazyContainsCheckedToDo);
        INSERT_DELIMITER;
    }

    if (m_lazyContainsUncheckedToDo > (-1)) {
        strm << "m_lazyContainsUncheckedToDo = " << QString::number(m_lazyContainsUncheckedToDo);
        INSERT_DELIMITER;
    }

    if (m_qecNote.contentHash.isSet()) {
        strm << "contentHash: " << m_qecNote.contentHash;
    }
    else {
        strm << "contentHash is not set";
    }
    INSERT_DELIMITER;

    if (m_qecNote.contentLength.isSet()) {
        strm << "contentLength: " << QString::number(m_qecNote.contentLength);
    }
    else {
        strm << "contentLength is not set";
    }
    INSERT_DELIMITER;

    if (m_qecNote.created.isSet()) {
        strm << "creationTimestamp: " << m_qecNote.created << ", datetime: "
             << PrintableDateTimeFromTimestamp(m_qecNote.created);
    }
    else {
        strm << "creationTimestamp is not set";
    }
    INSERT_DELIMITER;

    if (m_qecNote.updated.isSet()) {
        strm << "modificationTimestamp: " << m_qecNote.updated << ", datetime: "
             << PrintableDateTimeFromTimestamp(m_qecNote.updated);
    }
    else {
        strm << "modificationTimestamp is not set";
    }
    INSERT_DELIMITER;

    if (m_qecNote.deleted.isSet()) {
        strm << "deletionTimestamp: " << m_qecNote.deleted << ", datetime: "
             << PrintableDateTimeFromTimestamp(m_qecNote.deleted);
    }
    else {
        strm << "deletionTimestamp is not set";
    }
    INSERT_DELIMITER;

    if (m_qecNote.active.isSet()) {
        strm << "active: " << (m_qecNote.active ? "true" : "false");
    }
    else {
        strm << "active is not set";
    }
    INSERT_DELIMITER;

    if (m_qecNote.notebookGuid.isSet()) {
        strm << "notebookGuid: " << m_qecNote.notebookGuid;
    }
    else {
        strm << "notebookGuid is not set";
    }
    INSERT_DELIMITER;

    if (m_qecNote.tagGuids.isSet())
    {
        strm << "tagGuids: {";
        foreach(const QString & tagGuid, m_qecNote.tagGuids.ref()) {
            strm << tagGuid << "; ";
        }
        strm << "}";
    }
    else {
        strm << "tagGuids are not set";
    }
    INSERT_DELIMITER;

    if (m_qecNote.resources.isSet())
    {
        strm << "resources: { \n";
        foreach(const qevercloud::Resource & resource, m_qecNote.resources.ref()) {
            strm << ResourceAdapter(resource) << "; \n";
        }
        strm << "}";
    }
    else {
        strm << "resources are not set";
    }
    INSERT_DELIMITER;

    if (m_qecNote.attributes.isSet()) {
        strm << "attributes: " << m_qecNote.attributes;
    }
    else {
        strm << "attributes are not set";
    }
    INSERT_DELIMITER;

    strm << "isDirty: " << (isDirty() ? "true" : "false");
    INSERT_DELIMITER;

    strm << "isLocal: " << (m_isLocal ? "true" : "false");
    INSERT_DELIMITER;

    strm << "hasShortcut = " << (hasShortcut() ? "true" : "false");
    INSERT_DELIMITER;

    strm << "lazyPlainText: " << m_lazyPlainText;
    INSERT_DELIMITER;

    strm << "lazyPlainText is " << (m_lazyPlainTextIsValid ? "valid" : "invalid");
    INSERT_DELIMITER;

    strm << "lazyListOfWords: " << m_lazyListOfWords.join(",");
    INSERT_DELIMITER;

    strm << "lazyListOfWords is " << (m_lazyListOfWordsIsValid ? "valid" : "invalid");
    INSERT_DELIMITER;

#undef INSERT_DELIMITER

    return strm;
}

bool Note::containsToDoImpl(const bool checked) const
{
    int & refLazyContainsToDo = (checked ? m_lazyContainsCheckedToDo : m_lazyContainsUncheckedToDo);
    if (refLazyContainsToDo > (-1)) {
        if (refLazyContainsToDo == 0) {
            return false;
        }
        else {
            return true;
        }
    }

    if (!m_qecNote.content.isSet()) {
        refLazyContainsToDo = 0;
        return false;
    }

    QDomDocument enXmlDomDoc;
    int errorLine = -1, errorColumn = -1;
    QString errorMessage;
    bool res = enXmlDomDoc.setContent(m_qecNote.content.ref(), &errorMessage, &errorLine, &errorColumn);
    if (!res) {
        // TRANSLATOR Explaining the error of XML parsing
        errorMessage += QT_TR_NOOP(". Error happened at line ") +
                        QString::number(errorLine) + QT_TR_NOOP(", at column ") +
                        QString::number(errorColumn);
        QNWARNING("Note content parsing error: " << errorMessage);
        refLazyContainsToDo = 0;
        return false;
    }

    QDomElement docElem = enXmlDomDoc.documentElement();
    QDomNode nextNode = docElem.firstChild();
    while (!nextNode.isNull())
    {
        QDomElement element = nextNode.toElement();
        if (!element.isNull())
        {
            QString tagName = element.tagName();
            if (tagName == "en-todo")
            {
                QString checkedStr = element.attribute("checked", "false");
                if (checked && (checkedStr == "true")) {
                    refLazyContainsToDo = 1;
                    return true;
                }
                else if (!checked && (checkedStr == "false")) {
                    refLazyContainsToDo = 1;
                    return true;
                }
            }
        }
        nextNode = nextNode.nextSibling();
    }

    refLazyContainsToDo = 0;
    return false;
}

} // namespace qute_note
