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
    d(new NoteData)
{}

Note::Note(const qevercloud::Note & other) :
    DataElementWithShortcut(),
    d(new NoteData(other))
{}

Note::Note(Note && other) :
    DataElementWithShortcut(std::move(other)),
    d(std::move(other.d))
{}

Note & Note::operator=(const qevercloud::Note & other)
{
    d->operator=(other);
    return *this;
}

Note & Note::operator=(Note && other)
{
    if (this != &other) {
        NoteStoreDataElement::operator=(other);
        d = other.d;
    }

    return *this;
}

Note::~Note()
{}

bool Note::operator==(const Note & other) const
{
    return ((d->m_qecNote == other.d->m_qecNote) && (isDirty() == other.isDirty()) &&
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
    return d->m_qecNote.guid.isSet();
}

const QString & Note::guid() const
{
    return d->m_qecNote.guid;
}

void Note::setGuid(const QString & guid)
{
    d->m_qecNote.guid = guid;
}

bool Note::hasUpdateSequenceNumber() const
{
    return d->m_qecNote.updateSequenceNum.isSet();
}

qint32 Note::updateSequenceNumber() const
{
    return d->m_qecNote.updateSequenceNum;
}

void Note::setUpdateSequenceNumber(const qint32 usn)
{
    // TODO: see whether it's really necessary
    if (usn >= 0) {
        d->m_qecNote.updateSequenceNum = usn;
    }
    else {
        d->m_qecNote.updateSequenceNum.clear();
    }
}

void Note::clear()
{
    d->clear();
}

bool Note::checkParameters(QString & errorDescription) const
{
    if (localGuid().isEmpty() && !d->m_qecNote.guid.isSet()) {
        errorDescription = QT_TR_NOOP("Both Note's local and remote guids are empty");
        return false;
    }

    if (d->m_qecNote.guid.isSet() && !CheckGuid(d->m_qecNote.guid.ref())) {
        errorDescription = QT_TR_NOOP("Note's guid is invalid");
        return false;
    }

    if (d->m_qecNote.updateSequenceNum.isSet() && !CheckUpdateSequenceNumber(d->m_qecNote.updateSequenceNum)) {
        errorDescription = QT_TR_NOOP("Note's update sequence number is invalid");
        return false;
    }

    if (d->m_qecNote.title.isSet())
    {
        int titleSize = d->m_qecNote.title->size();

        if ( (titleSize < qevercloud::EDAM_NOTE_TITLE_LEN_MIN) ||
             (titleSize > qevercloud::EDAM_NOTE_TITLE_LEN_MAX) )
        {
            errorDescription = QT_TR_NOOP("Note's title length is invalid");
            return false;
        }
    }

    if (d->m_qecNote.content.isSet())
    {
        int contentSize = d->m_qecNote.content->size();

        if ( (contentSize < qevercloud::EDAM_NOTE_CONTENT_LEN_MIN) ||
             (contentSize > qevercloud::EDAM_NOTE_CONTENT_LEN_MAX) )
        {
            errorDescription = QT_TR_NOOP("Note's content length is invalid");
            return false;
        }
    }

    if (d->m_qecNote.contentHash.isSet())
    {
        size_t contentHashSize = d->m_qecNote.contentHash->size();

        if (contentHashSize != qevercloud::EDAM_HASH_LEN) {
            errorDescription = QT_TR_NOOP("Note's content hash size is invalid");
            return false;
        }
    }

    if (d->m_qecNote.notebookGuid.isSet() && !CheckGuid(d->m_qecNote.notebookGuid.ref())) {
        errorDescription = QT_TR_NOOP("Note's notebook guid is invalid");
        return false;
    }

    if (d->m_qecNote.tagGuids.isSet())
    {
        int numTagGuids = d->m_qecNote.tagGuids->size();

        if (numTagGuids > qevercloud::EDAM_NOTE_TAGS_MAX) {
            errorDescription = QT_TR_NOOP("Note has too many tags, max allowed ");
            errorDescription.append(QString::number(qevercloud::EDAM_NOTE_TAGS_MAX));
            return false;
        }
    }

    if (d->m_qecNote.resources.isSet())
    {
        int numResources = d->m_qecNote.resources->size();

        if (numResources > qevercloud::EDAM_NOTE_RESOURCES_MAX) {
            errorDescription = QT_TR_NOOP("Note has too many resources, max allowed ");
            errorDescription.append(QString::number(qevercloud::EDAM_NOTE_RESOURCES_MAX));
            return false;
        }
    }


    if (d->m_qecNote.attributes.isSet())
    {
        const qevercloud::NoteAttributes & attributes = d->m_qecNote.attributes;

#define CHECK_NOTE_ATTRIBUTE(name) \
    if (attributes.name.isSet()) { \
        int name##Size = attributes.name->size(); \
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

        if (attributes.contentClass.isSet())
        {
            int contentClassSize = attributes.contentClass->size();
            if ( (contentClassSize < qevercloud::EDAM_NOTE_CONTENT_CLASS_LEN_MIN) ||
                 (contentClassSize > qevercloud::EDAM_NOTE_CONTENT_CLASS_LEN_MAX) )
            {
                errorDescription = QT_TR_NOOP("Note attributes' content class has invalid size");
                return false;
            }
        }

        if (attributes.applicationData.isSet())
        {
            const qevercloud::LazyMap & applicationData = attributes.applicationData;

            if (applicationData.keysOnly.isSet())
            {
                const QSet<QString> & keysOnly = applicationData.keysOnly;
                foreach(const QString & key, keysOnly) {
                    int keySize = key.size();
                    if ( (keySize < qevercloud::EDAM_APPLICATIONDATA_NAME_LEN_MIN) ||
                         (keySize > qevercloud::EDAM_APPLICATIONDATA_NAME_LEN_MAX) )
                    {
                        errorDescription = QT_TR_NOOP("Note's attributes application data has invalid key (in keysOnly part)");
                        return false;
                    }
                }
            }

            if (applicationData.fullMap.isSet())
            {
                const QMap<QString, QString> & fullMap = applicationData.fullMap;
                for(QMap<QString, QString>::const_iterator it = fullMap.constBegin(); it != fullMap.constEnd(); ++it)
                {
                    int keySize = it.key().size();
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
        }

        if (attributes.classifications.isSet())
        {
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
    }

    return true;
}

bool Note::hasTitle() const
{
    return d->m_qecNote.title.isSet();
}

const QString & Note::title() const
{
    return d->m_qecNote.title;
}

void Note::setTitle(const QString & title)
{
    d->m_qecNote.title = title;
}

bool Note::hasContent() const
{
    return d->m_qecNote.content.isSet();
}

const QString & Note::content() const
{
    return d->m_qecNote.content;
}

void Note::setContent(const QString & content)
{
    d->m_qecNote.content = content;
    d->m_lazyPlainTextIsValid = false;    // Mark any existing plain text as invalid but don't free memory
    d->m_lazyListOfWordsIsValid = false;
    d->m_lazyContainsCheckedToDo = -1;
    d->m_lazyContainsUncheckedToDo = -1;
    d->m_lazyContainsEncryption = -1;
}

bool Note::hasContentHash() const
{
    return d->m_qecNote.contentHash.isSet();
}

const QByteArray & Note::contentHash() const
{
    return d->m_qecNote.contentHash;
}

void Note::setContentHash(const QByteArray & contentHash)
{
    d->m_qecNote.contentHash = contentHash;
}

bool Note::hasContentLength() const
{
    return d->m_qecNote.contentLength.isSet();
}

qint32 Note::contentLength() const
{
    return d->m_qecNote.contentLength;
}

void Note::setContentLength(const qint32 length)
{
    if (length >= 0) {
        d->m_qecNote.contentLength = length;
    }
    else {
        d->m_qecNote.contentLength.clear();
    }
}

bool Note::hasCreationTimestamp() const
{
    return d->m_qecNote.created.isSet();
}

qint64 Note::creationTimestamp() const
{
    return d->m_qecNote.created;
}

void Note::setCreationTimestamp(const qint64 timestamp)
{
    if (timestamp >= 0) {
        d->m_qecNote.created = timestamp;
    }
    else {
        d->m_qecNote.created.clear();
    }
}

bool Note::hasModificationTimestamp() const
{
    return d->m_qecNote.updated.isSet();
}

qint64 Note::modificationTimestamp() const
{
    return d->m_qecNote.updated;
}

void Note::setModificationTimestamp(const qint64 timestamp)
{
    if (timestamp >= 0) {
        d->m_qecNote.updated = timestamp;
    }
    else {
        d->m_qecNote.updated.clear();
    }
}

bool Note::hasDeletionTimestamp() const
{
    return d->m_qecNote.deleted.isSet();
}

qint64 Note::deletionTimestamp() const
{
    return d->m_qecNote.deleted;
}

void Note::setDeletionTimestamp(const qint64 timestamp)
{
    if (timestamp >= 0) {
        d->m_qecNote.deleted = timestamp;
    }
    else {
        d->m_qecNote.deleted.clear();
    }
}

bool Note::hasActive() const
{
    return d->m_qecNote.active.isSet();
}

bool Note::active() const
{
    return d->m_qecNote.active;
}

void Note::setActive(const bool active)
{
    d->m_qecNote.active = active;
}

bool Note::hasNotebookGuid() const
{
    return d->m_qecNote.notebookGuid.isSet();
}

const QString & Note::notebookGuid() const
{
    return d->m_qecNote.notebookGuid;
}

void Note::setNotebookGuid(const QString & guid)
{
    d->m_qecNote.notebookGuid = guid;
}

bool Note::hasTagGuids() const
{
    return d->m_qecNote.tagGuids.isSet();
}

void Note::tagGuids(QStringList & guids) const
{
    guids.clear();

    if (!d->m_qecNote.tagGuids.isSet()) {
        QNDEBUG("Note::tagGuids:: no tag guids set");
        return;
    }

    const QList<QString> & localTagGuids = d->m_qecNote.tagGuids;
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
        d->m_qecNote.tagGuids.clear();
        return;
    }

    int numTagGuids = guids.size();

    if (!d->m_qecNote.tagGuids.isSet()) {
        d->m_qecNote.tagGuids = QList<QString>();
    }

    QList<QString> & tagGuids = d->m_qecNote.tagGuids;
    tagGuids.clear();

    tagGuids.reserve(numTagGuids);
    foreach(const QString & guid, guids) {
        tagGuids << guid;
    }

    QNDEBUG("Added " << numTagGuids << " tag guids to note");
}

void Note::addTagGuid(const QString & guid)
{
    if (guid.isEmpty()) {
        return;
    }

    if (!d->m_qecNote.tagGuids.isSet()) {
        d->m_qecNote.tagGuids = QList<QString>();
    }

    if (!d->m_qecNote.tagGuids->contains(guid)) {
        d->m_qecNote.tagGuids.ref() << guid;
        QNDEBUG("Added tag with guid " << guid << " to note");
    }
}

void Note::removeTagGuid(const QString & guid)
{
    if (guid.isEmpty()) {
        QNDEBUG("Cannot remove empty tag guid");
        return;
    }

    if (!d->m_qecNote.tagGuids.isSet()) {
        QNDEBUG("No tag guids are set, cannot remove one");
        return;
    }

    int removed = d->m_qecNote.tagGuids->removeAll(guid);
    if (removed > 0) {
        QNDEBUG("Removed tag guid " << guid << " (" << removed << ") occurences");
    }
    else {
        QNDEBUG("Haven't removed tag guid " << guid << " because there was no such guid within note's tag guids");
    }
}

bool Note::hasResources() const
{
    return d->m_qecNote.resources.isSet();
}

QList<ResourceAdapter> Note::resourceAdapters() const
{
    QList<ResourceAdapter> resources;

    if (!d->m_qecNote.resources.isSet()) {
        return resources;
    }

    const QList<qevercloud::Resource> & noteResources = d->m_qecNote.resources.ref();
    int numResources = noteResources.size();
    int numResourceAdditionalInfoEntries = d->m_resourcesAdditionalInfo.size();

    resources.reserve(std::max(numResources, 0));
    for(int i = 0; i < numResources; ++i)
    {
        resources << ResourceAdapter(noteResources[i]);
        ResourceAdapter & resource = resources.back();

        if (i < numResourceAdditionalInfoEntries) {
            const NoteData::ResourceAdditionalInfo & info = d->m_resourcesAdditionalInfo[i];
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

    if (!d->m_qecNote.resources.isSet()) {
        return resources;
    }

    const QList<qevercloud::Resource> & noteResources = d->m_qecNote.resources.ref();
    int numResources = noteResources.size();
    int numResourceAdditionalInfoEntries = d->m_resourcesAdditionalInfo.size();

    resources.reserve(qMax(numResources, 0));
    for(int i = 0; i < numResources; ++i)
    {
        resources << ResourceWrapper(noteResources[i]);
        ResourceWrapper & resource = resources.back();

        if (i < numResourceAdditionalInfoEntries) {
            const NoteData::ResourceAdditionalInfo & info = d->m_resourcesAdditionalInfo[i];
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
            d->m_qecNote.resources = QList<qevercloud::Resource>(); \
            d->m_resourcesAdditionalInfo.clear(); \
            NoteData::ResourceAdditionalInfo info; \
            for(QList<resource_type>::const_iterator it = resources.constBegin(); \
                it != resources.constEnd(); ++it) \
            { \
                d->m_qecNote.resources.ref() << it->GetEnResource(); \
                info.localGuid = it->localGuid(); \
                if (it->hasNoteLocalGuid()) { \
                    info.noteLocalGuid = it->noteLocalGuid(); \
                } \
                info.isDirty = it->isDirty(); \
                d->m_resourcesAdditionalInfo.push_back(info); \
            } \
            QNDEBUG("Added " << resources.size() << " resources to note"); \
        } \
        else \
        { \
            d->m_qecNote.resources.clear(); \
        } \
    }

SET_RESOURCES(ResourceWrapper)
SET_RESOURCES(ResourceAdapter)

#undef SET_RESOURCES

void Note::addResource(const IResource & resource)
{
    if (!d->m_qecNote.resources.isSet()) {
        d->m_qecNote.resources = QList<qevercloud::Resource>();
    }

    if (d->m_qecNote.resources->contains(resource.GetEnResource())) {
        QNDEBUG("Can't add resource to note: this note already has this resource");
        return;
    }

    d->m_qecNote.resources.ref() << resource.GetEnResource();
    NoteData::ResourceAdditionalInfo info;
    info.localGuid = resource.localGuid();
    if (resource.hasNoteLocalGuid()) {
        info.noteLocalGuid = resource.noteLocalGuid();
    }
    info.isDirty = resource.isDirty();
    d->m_resourcesAdditionalInfo.push_back(info);

    QNDEBUG("Added resource to note, local guid = " << resource.localGuid());
}

void Note::removeResource(const IResource & resource)
{
    if (!d->m_qecNote.resources.isSet()) {
        QNDEBUG("Can't remove resource from note: note has no attached resources");
        return;
    }

    int removed = d->m_qecNote.resources->removeAll(resource.GetEnResource());
    if (removed > 0) {
        QNDEBUG("Removed resource " << resource << " from note (" << removed << ") occurences");
        NoteData::ResourceAdditionalInfo info;
        info.localGuid = resource.localGuid();
        if (resource.hasNoteLocalGuid()) {
            info.noteLocalGuid = resource.noteLocalGuid();
        }
        info.isDirty = resource.isDirty();
        d->m_resourcesAdditionalInfo.removeAll(info);
    }
    else {
        QNDEBUG("Haven't removed resource " << resource << " because there was no such resource attached to the note");
    }
}

bool Note::hasNoteAttributes() const
{
    return d->m_qecNote.attributes.isSet();
}

const qevercloud::NoteAttributes & Note::noteAttributes() const
{
    return d->m_qecNote.attributes;
}

qevercloud::NoteAttributes & Note::noteAttributes()
{
    if (!d->m_qecNote.attributes.isSet()) {
        d->m_qecNote.attributes = qevercloud::NoteAttributes();
    }

    return d->m_qecNote.attributes;
}

bool Note::isLocal() const
{
    return d->m_isLocal;
}

void Note::setLocal(const bool local)
{
    d->m_isLocal = local;
}

QImage Note::thumbnail() const
{
    return d->m_thumbnail;
}

void Note::setThumbnail(const QImage & thumbnail)
{
    d->m_thumbnail = thumbnail;
}

QString Note::plainText(QString * errorMessage) const
{
    if (d->m_lazyPlainTextIsValid) {
        return d->m_lazyPlainText;
    }

    if (!d->m_qecNote.content.isSet()) {
        if (errorMessage) {
            *errorMessage = "Note content is not set";
        }
        return QString();
    }

    ENMLConverter converter;
    QString error;
    bool res = converter.noteContentToPlainText(d->m_qecNote.content.ref(),
                                                d->m_lazyPlainText, error);
    if (!res) {
        QNWARNING(error);
        if (errorMessage) {
            *errorMessage = error;
        }
        return QString();
    }

    d->m_lazyPlainTextIsValid = true;

    return d->m_lazyPlainText;
}

QStringList Note::listOfWords(QString * errorMessage) const
{
    if (d->m_lazyListOfWordsIsValid) {
        return d->m_lazyListOfWords;
    }

    if (d->m_lazyPlainTextIsValid) {
        d->m_lazyListOfWords = ENMLConverter::plainTextToListOfWords(d->m_lazyPlainText);
        d->m_lazyListOfWordsIsValid = true;
        return d->m_lazyListOfWords;
    }

    // If still not returned, there's no plain text available so will get the list of words
    // from Note's content instead

    ENMLConverter converter;
    QString error;
    bool res = converter.noteContentToListOfWords(d->m_qecNote.content.ref(),
                                                  d->m_lazyListOfWords,
                                                  error, &(d->m_lazyPlainText));
    if (!res) {
        QNWARNING(error);
        if (errorMessage) {
            *errorMessage = error;
        }
        return QStringList();
    }

    d->m_lazyPlainTextIsValid = true;
    d->m_lazyListOfWordsIsValid = true;
    return d->m_lazyListOfWords;
}

bool Note::containsCheckedTodo() const
{
    return d->containsToDoImpl(/* checked = */ true);
}

bool Note::containsUncheckedTodo() const
{
    return d->containsToDoImpl(/* checked = */ false);
}

bool Note::containsTodo() const
{
    return (containsUncheckedTodo() || containsCheckedTodo());
}

bool Note::containsEncryption() const
{
    if (d->m_lazyContainsEncryption > (-1)) {
        if (d->m_lazyContainsEncryption == 0) {
            return false;
        }
        else {
            return true;
        }
    }

    if (!d->m_qecNote.content.isSet()) {
        d->m_lazyContainsEncryption = 0;
        return false;
    }

    QDomDocument enXmlDomDoc;
    int errorLine = -1, errorColumn = -1;
    QString errorMessage;
    bool res = enXmlDomDoc.setContent(d->m_qecNote.content.ref(), &errorMessage, &errorLine, &errorColumn);
    if (!res) {
        // TRANSLATOR Explaining the error of XML parsing
        errorMessage += QT_TR_NOOP(". Error happened at line ") +
                        QString::number(errorLine) + QT_TR_NOOP(", at column ") +
                        QString::number(errorColumn);
        QNWARNING("Note content parsing error: " << errorMessage);
        d->m_lazyContainsEncryption = 0;
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
            if (tagName == "en-crypt") {
                d->m_lazyContainsEncryption = 1;
                return true;
            }
        }
        nextNode = nextNode.nextSibling();
    }

    d->m_lazyContainsEncryption = 0;
    return false;
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

    if (d->m_qecNote.guid.isSet()) {
        strm << "guid: " << d->m_qecNote.guid;
    }
    else {
        strm << "guid is not set";
    }
    INSERT_DELIMITER;

    if (d->m_qecNote.updateSequenceNum.isSet()) {
        strm << "updateSequenceNumber: " << QString::number(d->m_qecNote.updateSequenceNum);
    }
    else {
        strm << "updateSequenceNumber is not set";
    }
    INSERT_DELIMITER;

    if (d->m_qecNote.title.isSet()) {
        strm << "title: " << d->m_qecNote.title;
    }
    else {
        strm << "title is not set";
    }
    INSERT_DELIMITER;

    if (d->m_qecNote.content.isSet()) {
        strm << "content: " << d->m_qecNote.content;
    }
    else {
        strm << "content is not set";
    }
    INSERT_DELIMITER;

    if (d->m_lazyContainsCheckedToDo > (-1)) {
        strm << "m_lazyContainsCheckedToDo = " << QString::number(d->m_lazyContainsCheckedToDo);
        INSERT_DELIMITER;
    }

    if (d->m_lazyContainsUncheckedToDo > (-1)) {
        strm << "m_lazyContainsUncheckedToDo = " << QString::number(d->m_lazyContainsUncheckedToDo);
        INSERT_DELIMITER;
    }

    if (d->m_lazyContainsEncryption > (-1)) {
        strm << "m_lazyContainsEncryption = " << QString::number(d->m_lazyContainsEncryption);
        INSERT_DELIMITER;
    }

    if (d->m_qecNote.contentHash.isSet()) {
        strm << "contentHash: " << d->m_qecNote.contentHash;
    }
    else {
        strm << "contentHash is not set";
    }
    INSERT_DELIMITER;

    if (d->m_qecNote.contentLength.isSet()) {
        strm << "contentLength: " << QString::number(d->m_qecNote.contentLength);
    }
    else {
        strm << "contentLength is not set";
    }
    INSERT_DELIMITER;

    if (d->m_qecNote.created.isSet()) {
        strm << "creationTimestamp: " << d->m_qecNote.created << ", datetime: "
             << PrintableDateTimeFromTimestamp(d->m_qecNote.created);
    }
    else {
        strm << "creationTimestamp is not set";
    }
    INSERT_DELIMITER;

    if (d->m_qecNote.updated.isSet()) {
        strm << "modificationTimestamp: " << d->m_qecNote.updated << ", datetime: "
             << PrintableDateTimeFromTimestamp(d->m_qecNote.updated);
    }
    else {
        strm << "modificationTimestamp is not set";
    }
    INSERT_DELIMITER;

    if (d->m_qecNote.deleted.isSet()) {
        strm << "deletionTimestamp: " << d->m_qecNote.deleted << ", datetime: "
             << PrintableDateTimeFromTimestamp(d->m_qecNote.deleted);
    }
    else {
        strm << "deletionTimestamp is not set";
    }
    INSERT_DELIMITER;

    if (d->m_qecNote.active.isSet()) {
        strm << "active: " << (d->m_qecNote.active ? "true" : "false");
    }
    else {
        strm << "active is not set";
    }
    INSERT_DELIMITER;

    if (d->m_qecNote.notebookGuid.isSet()) {
        strm << "notebookGuid: " << d->m_qecNote.notebookGuid;
    }
    else {
        strm << "notebookGuid is not set";
    }
    INSERT_DELIMITER;

    if (d->m_qecNote.tagGuids.isSet())
    {
        strm << "tagGuids: {";
        foreach(const QString & tagGuid, d->m_qecNote.tagGuids.ref()) {
            strm << "'" << tagGuid << "'; ";
        }
        strm << "}";
    }
    else {
        strm << "tagGuids are not set";
    }
    INSERT_DELIMITER;

    if (d->m_qecNote.resources.isSet())
    {
        strm << "resources: { \n";
        foreach(const qevercloud::Resource & resource, d->m_qecNote.resources.ref()) {
            strm << resource << "; \n";
        }
        strm << "}";
    }
    else {
        strm << "resources are not set";
    }
    INSERT_DELIMITER;

    if (d->m_qecNote.attributes.isSet()) {
        strm << "attributes: " << d->m_qecNote.attributes;
    }
    else {
        strm << "attributes are not set";
    }
    INSERT_DELIMITER;

    strm << "isDirty: " << (isDirty() ? "true" : "false");
    INSERT_DELIMITER;

    strm << "isLocal: " << (d->m_isLocal ? "true" : "false");
    INSERT_DELIMITER;

    strm << "hasShortcut = " << (hasShortcut() ? "true" : "false");
    INSERT_DELIMITER;

    strm << "lazyPlainText: " << d->m_lazyPlainText;
    INSERT_DELIMITER;

    strm << "lazyPlainText is " << (d->m_lazyPlainTextIsValid ? "valid" : "invalid");
    INSERT_DELIMITER;

    strm << "lazyListOfWords: " << d->m_lazyListOfWords.join(",");
    INSERT_DELIMITER;

    strm << "lazyListOfWords is " << (d->m_lazyListOfWordsIsValid ? "valid" : "invalid");
    INSERT_DELIMITER;

#undef INSERT_DELIMITER

    return strm;
}

} // namespace qute_note
