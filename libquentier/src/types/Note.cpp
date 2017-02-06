/*
 * Copyright 2016 Dmitry Ivanov
 *
 * This file is part of libquentier
 *
 * libquentier is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * libquentier is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with libquentier. If not, see <http://www.gnu.org/licenses/>.
 */

#include "data/NoteData.h"
#include <quentier/types/Note.h>
#include <quentier/types/Resource.h>
#include <quentier/utility/Utility.h>
#include <quentier/logging/QuentierLogger.h>
#include <algorithm>

namespace quentier {

struct CompareSharedNotesByIndexInNote
{
    bool operator()(const SharedNote & lhs, const SharedNote & rhs) const
    { return lhs.indexInNote() < rhs.indexInNote(); }
};

QN_DEFINE_LOCAL_UID(Note)
QN_DEFINE_DIRTY(Note)
QN_DEFINE_LOCAL(Note)
QN_DEFINE_FAVORITED(Note)

Note::Note() :
    d(new NoteData)
{}

Note::Note(const Note & other) :
    d(other.d)
{}

Note::Note(const qevercloud::Note & other) :
    d(new NoteData(other))
{}

Note::Note(Note && other) :
    d(std::move(other.d))
{}

Note & Note::operator=(const Note & other)
{
    if (this != &other) {
        d = other.d;
    }

    return *this;
}

Note & Note::operator=(const qevercloud::Note & other)
{
    d = new NoteData(other);
    return *this;
}

Note & Note::operator=(Note && other)
{
    if (this != &other) {
        d = std::move(other.d);
    }

    return *this;
}

Note::~Note()
{}

bool Note::operator==(const Note & other) const
{
    return ( (d->m_qecNote == other.d->m_qecNote) &&
             (d->m_notebookLocalUid.isEqual(other.d->m_notebookLocalUid)) &&
             (d->m_tagLocalUids == other.d->m_tagLocalUids) &&
             (d->m_resourcesAdditionalInfo == other.d->m_resourcesAdditionalInfo) &&
             (isDirty() == other.isDirty()) &&
             (isLocal() == other.isLocal()) &&
             (isFavorited() == other.isFavorited()) );
    // NOTE: thumbnail doesn't take part in comparison because it's merely a helper
    // for note displaying widget, nothing more
}

bool Note::operator!=(const Note & other) const
{
    return !(*this == other);
}

Note::operator const qevercloud::Note &() const
{
    return d->m_qecNote;
}

Note::operator qevercloud::Note &()
{
    return d->m_qecNote;
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
    if (!guid.isEmpty()) {
        d->m_qecNote.guid = guid;
    }
    else {
        d->m_qecNote.guid.clear();
    }
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

bool Note::validateTitle(const QString & title, ErrorString * pErrorDescription)
{
    if (title != title.trimmed())
    {
        if (pErrorDescription) {
            pErrorDescription->base() = QT_TRANSLATE_NOOP("", "Note title cannot start or end with whitespace");
            pErrorDescription->details() = title;
        }

        return false;
    }

    int len = title.length();
    if (len < qevercloud::EDAM_NOTE_TITLE_LEN_MIN)
    {
        if (pErrorDescription) {
            pErrorDescription->base() = QT_TRANSLATE_NOOP("", "Note title's length is too small");
            pErrorDescription->details() = title;
        }

        return false;
    }
    else if (len > qevercloud::EDAM_NOTE_TITLE_LEN_MAX)
    {
        if (pErrorDescription) {
            pErrorDescription->base() = QT_TRANSLATE_NOOP("", "Note title's length is too large");
            pErrorDescription->details() = title;
        }

        return false;
    }

    return true;
}

bool Note::checkParameters(ErrorString & errorDescription) const
{
    if (localUid().isEmpty() && !d->m_qecNote.guid.isSet()) {
        errorDescription.base() = QT_TRANSLATE_NOOP("", "Both Note's local and remote guids are empty");
        return false;
    }

    return d->checkParameters(errorDescription);
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
    if (!title.isEmpty()) {
        d->m_qecNote.title = title;
    }
    else {
        d->m_qecNote.title.clear();
    }
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
    d->setContent(content);
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
    if (!guid.isEmpty()) {
        d->m_qecNote.notebookGuid = guid;
    }
    else {
        d->m_qecNote.notebookGuid.clear();
    }
}

bool Note::hasNotebookLocalUid() const
{
    return d->m_notebookLocalUid.isSet();
}

const QString & Note::notebookLocalUid() const
{
    return d->m_notebookLocalUid.ref();
}

void Note::setNotebookLocalUid(const QString & notebookLocalUid)
{
    if (notebookLocalUid.isEmpty()) {
        d->m_notebookLocalUid.clear();
    }
    else {
        d->m_notebookLocalUid = notebookLocalUid;
    }
}

bool Note::hasTagGuids() const
{
    return d->m_qecNote.tagGuids.isSet();
}

const QStringList Note::tagGuids() const
{
    return d->m_qecNote.tagGuids.ref();
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
    for(auto it = guids.begin(), end = guids.end(); it != end; ++it) {
        tagGuids << *it;
    }

    QNDEBUG(QStringLiteral("Added ") << numTagGuids << QStringLiteral(" tag guids to note"));
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
        QNDEBUG(QStringLiteral("Added tag guid ") << guid << QStringLiteral(" to the note"));
    }
}

void Note::removeTagGuid(const QString & guid)
{
    if (guid.isEmpty()) {
        QNDEBUG(QStringLiteral("Cannot remove empty tag guid"));
        return;
    }

    if (!d->m_qecNote.tagGuids.isSet()) {
        QNDEBUG(QStringLiteral("No tag guids are set, cannot remove one"));
        return;
    }

    QList<qevercloud::Guid> & tagGuids = d->m_qecNote.tagGuids.ref();
    int removed = tagGuids.removeAll(guid);
    if (removed > 0) {
        QNDEBUG(QStringLiteral("Removed tag guid ") << guid << QStringLiteral(" (") << removed << QStringLiteral(") occurences"));
    }
    else {
        QNDEBUG(QStringLiteral("Haven't removed tag guid ") << guid << QStringLiteral(" because there was no such guid within the note's tag guids"));
    }
}

bool Note::hasTagLocalUids() const
{
    return !d->m_tagLocalUids.isEmpty();
}

const QStringList & Note::tagLocalUids() const
{
    return d->m_tagLocalUids;
}

void Note::setTagLocalUids(const QStringList & tagLocalUids)
{
    d->m_tagLocalUids = tagLocalUids;
}

void Note::addTagLocalUid(const QString & tagLocalUid)
{
    if (tagLocalUid.isEmpty()) {
        return;
    }

    if (!d->m_tagLocalUids.contains(tagLocalUid)) {
        d->m_tagLocalUids << tagLocalUid;
        QNDEBUG(QStringLiteral("Added tag local uid ") << tagLocalUid << QStringLiteral(" to the note"));
    }
}

void Note::removeTagLocalUid(const QString & tagLocalUid)
{
    if (tagLocalUid.isEmpty()) {
        QNDEBUG(QStringLiteral("Cannot remove empty tag local uid"));
        return;
    }

    if (d->m_tagLocalUids.isEmpty()) {
        QNDEBUG(QStringLiteral("No tag local uids are set, cannot remove one"));
        return;
    }

    int removed = d->m_tagLocalUids.removeAll(tagLocalUid);
    if (removed > 0) {
        QNDEBUG(QStringLiteral("Removed tag local uid ") << tagLocalUid << QStringLiteral(" (") << removed << QStringLiteral(") occurrences"));
    }
    else {
        QNDEBUG(QStringLiteral("Haven't removed tag local uid ") << tagLocalUid << QStringLiteral(" because there was no such uid within the note's tag local uids"));
    }
}

bool Note::hasResources() const
{
    return d->m_qecNote.resources.isSet();
}

int Note::numResources() const
{
    return (d->m_qecNote.resources.isSet() ? d->m_qecNote.resources->size() : 0);
}

QList<Resource> Note::resources() const
{
    QList<Resource> resources;

    if (!d->m_qecNote.resources.isSet()) {
        return resources;
    }

    QString noteLocalUid = localUid();
    const QList<qevercloud::Resource> & noteResources = d->m_qecNote.resources.ref();
    int numResources = noteResources.size();
    int numResourceAdditionalInfoEntries = d->m_resourcesAdditionalInfo.size();

    resources.reserve(qMax(numResources, 0));
    for(int i = 0; i < numResources; ++i)
    {
        resources << Resource(noteResources[i]);
        Resource & resource = resources.back();

        if (i < numResourceAdditionalInfoEntries) {
            const NoteData::ResourceAdditionalInfo & info = d->m_resourcesAdditionalInfo[i];
            resource.setLocalUid(info.localUid);
            resource.setNoteLocalUid(noteLocalUid);
            resource.setDirty(info.isDirty);
        }

        resource.setIndexInNote(i);
    }
    return resources;
}

void Note::setResources(const QList<Resource> & resources)
{
    if (!resources.empty())
    {
        d->m_qecNote.resources = QList<qevercloud::Resource>();
        d->m_resourcesAdditionalInfo.clear();
        NoteData::ResourceAdditionalInfo info;
        for(auto it = resources.constBegin(); it != resources.constEnd(); ++it)
        {
            d->m_qecNote.resources.ref() << static_cast<const qevercloud::Resource&>(*it);
            info.localUid = it->localUid();
            info.isDirty = it->isDirty();
            d->m_resourcesAdditionalInfo.push_back(info);
        }
        QNDEBUG(QStringLiteral("Added ") << resources.size() << QStringLiteral(" resources to note"));
    }
    else
    {
        d->m_qecNote.resources.clear();
    }
}


void Note::addResource(const Resource & resource)
{
    if (!d->m_qecNote.resources.isSet()) {
        d->m_qecNote.resources = QList<qevercloud::Resource>();
    }

    if (d->m_qecNote.resources->contains(static_cast<const qevercloud::Resource&>(resource))) {
        QNDEBUG(QStringLiteral("Can't add resource to note: this note already has this resource"));
        return;
    }

    d->m_qecNote.resources.ref() << static_cast<const qevercloud::Resource&>(resource);
    NoteData::ResourceAdditionalInfo info;
    info.localUid = resource.localUid();
    info.isDirty = resource.isDirty();
    d->m_resourcesAdditionalInfo.push_back(info);
    QNDEBUG(QStringLiteral("Added resource to note, local uid = ") << resource.localUid());
}

bool Note::updateResource(const Resource & resource)
{
    if (!d->m_qecNote.resources.isSet()) {
        QNDEBUG(QStringLiteral("Can't update resource in note: note has no attached resources"));
        return false;
    }

    int targetResourceIndex = -1;
    const int numResources = d->m_resourcesAdditionalInfo.size();
    for(int i = 0; i < numResources; ++i)
    {
        if (d->m_resourcesAdditionalInfo[i].localUid == resource.localUid()) {
            targetResourceIndex = i;
            break;
        }
    }

    if (targetResourceIndex < 0) {
        QNDEBUG(QStringLiteral("Can't update resource in note: can't find resource to update"));
        return false;
    }

    d->m_qecNote.resources.ref()[targetResourceIndex] = static_cast<const qevercloud::Resource&>(resource);
    d->m_resourcesAdditionalInfo[targetResourceIndex].isDirty = resource.isDirty();
    return true;
}

bool Note::removeResource(const Resource & resource)
{
    if (!d->m_qecNote.resources.isSet()) {
        QNDEBUG(QStringLiteral("Can't remove resource from note: note has no attached resources"));
        return false;
    }

    QList<qevercloud::Resource> & resources = d->m_qecNote.resources.ref();
    int removed = resources.removeAll(static_cast<const qevercloud::Resource&>(resource));
    if (removed <= 0) {
        QNDEBUG(QStringLiteral("Haven't removed resource ") << resource << QStringLiteral(" because there was no such resource attached to the note"));
        return false;
    }

    QNDEBUG(QStringLiteral("Removed resource ") << resource << QStringLiteral(" from note (") << removed << QStringLiteral(") occurences"));
    NoteData::ResourceAdditionalInfo info;
    info.localUid = resource.localUid();
    info.isDirty = resource.isDirty();
    d->m_resourcesAdditionalInfo.removeAll(info);

    return true;
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

void Note::clearNoteAttributes()
{
    d->m_qecNote.attributes.clear();
}

bool Note::hasSharedNotes() const
{
    return d->m_qecNote.sharedNotes.isSet() && !d->m_qecNote.sharedNotes->isEmpty();
}

QList<SharedNote> Note::sharedNotes() const
{
    QList<SharedNote> result;

    if (!d->m_qecNote.sharedNotes.isSet() || d->m_qecNote.sharedNotes->isEmpty()) {
        return result;
    }

    result.reserve(d->m_qecNote.sharedNotes->size());

    int noteIndex = 0;
    for(auto it = d->m_qecNote.sharedNotes->begin(), end = d->m_qecNote.sharedNotes->end(); it != end; ++it)
    {
        SharedNote sharedNote(*it);
        if (hasGuid()) {
            sharedNote.setNoteGuid(guid());
        }
        sharedNote.setIndexInNote(noteIndex++);
        result << sharedNote;
    }

    return result;
}

void Note::setSharedNotes(const QList<SharedNote> & sharedNotes)
{
    if (sharedNotes.isEmpty()) {
        d->m_qecNote.sharedNotes.clear();
        return;
    }

    QList<SharedNote> sortedSharedNotes = sharedNotes;
    std::sort(sortedSharedNotes.begin(), sortedSharedNotes.end(), CompareSharedNotesByIndexInNote());

    QList<qevercloud::SharedNote> internalSharedNotes;
    internalSharedNotes.reserve(sortedSharedNotes.size());

    for(auto it = sortedSharedNotes.constBegin(), end = sortedSharedNotes.constEnd(); it != end; ++it) {
        const qevercloud::SharedNote & qecSharedNote = static_cast<const qevercloud::SharedNote&>(*it);
        internalSharedNotes << qecSharedNote;
    }

    d->m_qecNote.sharedNotes = internalSharedNotes;
}

void Note::addSharedNote(const SharedNote & sharedNote)
{
    if (!d->m_qecNote.sharedNotes.isSet()) {
        d->m_qecNote.sharedNotes = QList<qevercloud::SharedNote>();
    }

    const qevercloud::SharedNote & qecSharedNote = static_cast<const qevercloud::SharedNote&>(sharedNote);

    if (d->m_qecNote.sharedNotes->contains(qecSharedNote)) {
        QNDEBUG(QStringLiteral("Can't add shared note: this note already has this shared note"));
        return;
    }

    d->m_qecNote.sharedNotes.ref() << qecSharedNote;
}

bool Note::updateSharedNote(const SharedNote & sharedNote)
{
    if (!d->m_qecNote.sharedNotes.isSet() || d->m_qecNote.sharedNotes->isEmpty()) {
        return false;
    }

    int index = sharedNote.indexInNote();
    if ((index < 0) || (index >= d->m_qecNote.sharedNotes->size())) {
        return false;
    }

    const qevercloud::SharedNote & qecSharedNote = static_cast<const qevercloud::SharedNote&>(sharedNote);
    d->m_qecNote.sharedNotes.ref()[index] = qecSharedNote;
    return true;
}

bool Note::removeSharedNote(const SharedNote & sharedNote)
{
    if (!d->m_qecNote.sharedNotes.isSet() || d->m_qecNote.sharedNotes->isEmpty()) {
        return false;
    }

    int index = sharedNote.indexInNote();
    if ((index < 0) || (index >= d->m_qecNote.sharedNotes->size())) {
        return false;
    }

    d->m_qecNote.sharedNotes->removeAt(index);
    return true;
}

bool Note::hasNoteRestrictions() const
{
    return d->m_qecNote.restrictions.isSet();
}

const qevercloud::NoteRestrictions & Note::noteRestrictions() const
{
    return d->m_qecNote.restrictions.ref();
}

qevercloud::NoteRestrictions & Note::noteRestrictions()
{
    return d->m_qecNote.restrictions.ref();
}

void Note::setNoteRestrictions(qevercloud::NoteRestrictions && restrictions)
{
    d->m_qecNote.restrictions = std::move(restrictions);
}

bool Note::hasNoteLimits() const
{
    return d->m_qecNote.limits.isSet();
}

const qevercloud::NoteLimits & Note::noteLimits() const
{
    return d->m_qecNote.limits.ref();
}

qevercloud::NoteLimits & Note::noteLimits()
{
    return d->m_qecNote.limits.ref();
}

void Note::setNoteLimits(qevercloud::NoteLimits && limits)
{
    d->m_qecNote.limits = std::move(limits);
}

QImage Note::thumbnail() const
{
    return d->m_thumbnail;
}

void Note::setThumbnail(const QImage & thumbnail)
{
    d->m_thumbnail = thumbnail;
}

bool Note::isInkNote() const
{
    if (!d->m_qecNote.resources.isSet()) {
        return false;
    }

    const QList<qevercloud::Resource> & resources = d->m_qecNote.resources.ref();

    // NOTE: it is not known for sure how many resources there might be within an ink note. Probably just one in most cases.
    const int numResources = resources.size();
    for(int i = 0; i < numResources; ++i)
    {
        const qevercloud::Resource & resource = resources[i];
        if (!resource.mime.isSet()) {
            return false;
        }
        else if (resource.mime.ref() != QStringLiteral("application/vnd.evernote.ink")) {
            return false;
        }
    }

    return true;
}

QString Note::plainText(ErrorString * pErrorMessage) const
{
    return d->plainText(pErrorMessage);
}

QStringList Note::listOfWords(ErrorString * pErrorMessage) const
{
    return d->listOfWords(pErrorMessage);
}

std::pair<QString, QStringList> Note::plainTextAndListOfWords(ErrorString * pErrorMessage) const
{
    return d->plainTextAndListOfWords(pErrorMessage);
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
    return d->containsEncryption();
}

QTextStream & Note::print(QTextStream & strm) const
{
    strm << QStringLiteral("Note: { \n");

#define INSERT_DELIMITER \
    strm << QStringLiteral("; \n");

    const QString localUid_ = localUid();
    if (!localUid_.isEmpty()) {
        strm << QStringLiteral("localUid: ") << localUid_;
    }
    else {
        strm << QStringLiteral("localUid is not set");
    }
    INSERT_DELIMITER;

    if (d->m_qecNote.guid.isSet()) {
        strm << QStringLiteral("guid: ") << d->m_qecNote.guid;
    }
    else {
        strm << QStringLiteral("guid is not set");
    }
    INSERT_DELIMITER;

    if (d->m_qecNote.updateSequenceNum.isSet()) {
        strm << QStringLiteral("updateSequenceNumber: ") << QString::number(d->m_qecNote.updateSequenceNum);
    }
    else {
        strm << QStringLiteral("updateSequenceNumber is not set");
    }
    INSERT_DELIMITER;

    if (d->m_qecNote.title.isSet()) {
        strm << QStringLiteral("title: ") << d->m_qecNote.title;
    }
    else {
        strm << QStringLiteral("title is not set");
    }
    INSERT_DELIMITER;

    if (d->m_qecNote.content.isSet()) {
        strm << QStringLiteral("content: ") << d->m_qecNote.content;
    }
    else {
        strm << QStringLiteral("content is not set");
    }
    INSERT_DELIMITER;

    if (d->m_qecNote.contentHash.isSet()) {
        strm << QStringLiteral("contentHash: ") << d->m_qecNote.contentHash;
    }
    else {
        strm << QStringLiteral("contentHash is not set");
    }
    INSERT_DELIMITER;

    if (d->m_qecNote.contentLength.isSet()) {
        strm << QStringLiteral("contentLength: ") << QString::number(d->m_qecNote.contentLength);
    }
    else {
        strm << QStringLiteral("contentLength is not set");
    }
    INSERT_DELIMITER;

    if (d->m_qecNote.created.isSet()) {
        strm << QStringLiteral("creationTimestamp: ") << d->m_qecNote.created << QStringLiteral(", datetime: ")
             << printableDateTimeFromTimestamp(d->m_qecNote.created);
    }
    else {
        strm << QStringLiteral("creationTimestamp is not set");
    }
    INSERT_DELIMITER;

    if (d->m_qecNote.updated.isSet()) {
        strm << QStringLiteral("modificationTimestamp: ") << d->m_qecNote.updated << QStringLiteral(", datetime: ")
             << printableDateTimeFromTimestamp(d->m_qecNote.updated);
    }
    else {
        strm << QStringLiteral("modificationTimestamp is not set");
    }
    INSERT_DELIMITER;

    if (d->m_qecNote.deleted.isSet()) {
        strm << QStringLiteral("deletionTimestamp: ") << d->m_qecNote.deleted << QStringLiteral(", datetime: ")
             << printableDateTimeFromTimestamp(d->m_qecNote.deleted);
    }
    else {
        strm << QStringLiteral("deletionTimestamp is not set");
    }
    INSERT_DELIMITER;

    if (d->m_qecNote.active.isSet()) {
        strm << QStringLiteral("active: ") << (d->m_qecNote.active ? QStringLiteral("true") : QStringLiteral("false"));
    }
    else {
        strm << QStringLiteral("active is not set");
    }
    INSERT_DELIMITER;

    if (d->m_qecNote.notebookGuid.isSet()) {
        strm << QStringLiteral("notebookGuid: ") << d->m_qecNote.notebookGuid;
    }
    else {
        strm << QStringLiteral("notebookGuid is not set");
    }
    INSERT_DELIMITER;

    if (d->m_qecNote.tagGuids.isSet())
    {
        strm << QStringLiteral("tagGuids: {");
        const QStringList tagGuids = d->m_qecNote.tagGuids.ref();
        for(auto it = tagGuids.constBegin(), end = tagGuids.constEnd(); it != end; ++it) {
            strm << QStringLiteral("'") << *it << QStringLiteral("'; ");
        }
        strm << QStringLiteral("}");
    }
    else {
        strm << QStringLiteral("tagGuids are not set");
    }
    INSERT_DELIMITER;

    if (!d->m_tagLocalUids.isEmpty())
    {
        strm << QStringLiteral("tagLocalUids: {");
        const QStringList tagLocalUids = d->m_tagLocalUids;
        for(auto it = tagLocalUids.constBegin(), end = tagLocalUids.constEnd(); it != end; ++it) {
            strm << QStringLiteral("'") << *it << QStringLiteral("';");
        }
        strm << QStringLiteral("}");
    }
    else {
        strm << QStringLiteral("tagLocalUids are not set");
    }
    INSERT_DELIMITER

    if (d->m_qecNote.resources.isSet())
    {
        strm << QStringLiteral("resources: { \n");
        const QList<qevercloud::Resource> resources = d->m_qecNote.resources.ref();
        for(auto it = resources.constBegin(), end = resources.constEnd(); it != end; ++it) {
            strm << *it << QStringLiteral("; \n");
        }
        strm << QStringLiteral("}");
    }
    else {
        strm << QStringLiteral("resources are not set");
    }
    INSERT_DELIMITER;

    if (d->m_qecNote.attributes.isSet()) {
        strm << QStringLiteral("attributes: ") << d->m_qecNote.attributes;
    }
    else {
        strm << QStringLiteral("attributes are not set");
    }
    INSERT_DELIMITER;

    strm << QStringLiteral("isDirty: ") << (isDirty() ? QStringLiteral("true") : QStringLiteral("false"));
    INSERT_DELIMITER;

    strm << QStringLiteral("isLocal: ") << (d->m_isLocal ? QStringLiteral("true") : QStringLiteral("false"));
    INSERT_DELIMITER;

    strm << QStringLiteral("isFavorited = ") << (isFavorited() ? QStringLiteral("true") : QStringLiteral("false"));
    INSERT_DELIMITER;

#undef INSERT_DELIMITER

    strm << QStringLiteral("}; \n");
    return strm;
}

} // namespace quentier
