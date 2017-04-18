#include "EnexImporter.h"
#include "models/TagModel.h"
#include <quentier/local_storage/LocalStorageManagerAsync.h>
#include <quentier/enml/ENMLConverter.h>
#include <quentier/logging/QuentierLogger.h>
#include <QFile>

namespace quentier {

EnexImporter::EnexImporter(const QString & enexFilePath,
                           LocalStorageManagerAsync & localStorageManagerAsync,
                           TagModel & tagModel, QObject * parent) :
    QObject(parent),
    m_localStorageManagerAsync(localStorageManagerAsync),
    m_tagModel(tagModel),
    m_enexFilePath(enexFilePath),
    m_tagNamesByImportedNoteLocalUid(),
    m_addTagRequestIdByTagNameBimap(),
    m_expungedTagLocalUids(),
    m_notesPendingTagAddition(),
    m_addNoteRequestIds(),
    m_connectedToLocalStorage(false)
{
    if (!m_tagModel.allTagsListed()) {
        QObject::connect(&m_tagModel, QNSIGNAL(TagModel,notifyAllTagsListed),
                         this, QNSLOT(EnexImporter,onAllTagsListed));
    }
}

bool EnexImporter::isInProgress() const
{
    QNDEBUG(QStringLiteral("EnexImporter::isInProgress"));

    if (!m_addTagRequestIdByTagNameBimap.empty()) {
        QNDEBUG(QStringLiteral("There are ") << m_addTagRequestIdByTagNameBimap.size()
                << QStringLiteral(" pending requests to add tag"));
        return true;
    }

    if (!m_addNoteRequestIds.isEmpty()) {
        QNDEBUG(QStringLiteral("There are ") << m_addNoteRequestIds.size()
                << QStringLiteral(" pending requests to add note"));
        return true;
    }

    // Maybe we are waiting for the tag model?
    if (!m_tagModel.allTagsListed() && !m_notesPendingTagAddition.isEmpty()) {
        QNDEBUG(QStringLiteral("Not all tags were listed in the tag model yet + there are ")
                << m_notesPendingTagAddition.size() << QStringLiteral(" notes pending tag addition"));
        return true;
    }

    QNDEBUG(QStringLiteral("No signs of pending progress detected"));
    return false;
}

void EnexImporter::start()
{
    QNDEBUG(QStringLiteral("EnexImporter::start"));

    clear();

    QFile enexFile(m_enexFilePath);
    bool res = enexFile.open(QIODevice::ReadOnly);
    if (Q_UNLIKELY(!res)) {
        ErrorString errorDescription(QT_TRANSLATE_NOOP("", "Can't import ENEX: can't open enex file for writing"));
        errorDescription.details() = m_enexFilePath;
        QNWARNING(errorDescription);
        emit enexImportFailed(errorDescription);
        return;
    }

    QString enex = QString::fromLocal8Bit(enexFile.readAll());
    enexFile.close();

    QVector<Note> importedNotes;
    ErrorString errorDescription;
    ENMLConverter converter;
    res = converter.importEnex(enex, importedNotes, m_tagNamesByImportedNoteLocalUid, errorDescription);
    if (!res) {
        emit enexImportFailed(errorDescription);
        return;
    }

    for(auto it = importedNotes.begin(); it != importedNotes.end(); )
    {
        const Note & note = *it;

        auto tagIt = m_tagNamesByImportedNoteLocalUid.find(note.localUid());
        if (tagIt == m_tagNamesByImportedNoteLocalUid.end()) {
            QNTRACE(QStringLiteral("Imported note doesn't have tag names assigned to it, can add it to local storage right away: ")
                    << note);
            addNoteToLocalStorage(note);
            it = importedNotes.erase(it);
            continue;
        }

        QStringList & tagNames = tagIt.value();
        for(auto tagNameIt = tagNames.begin(); tagNameIt != tagNames.end(); )
        {
            const QString & tagName = *tagNameIt;
            if (!tagName.isEmpty()) {
                ++tagNameIt;
                continue;
            }

            QNDEBUG(QStringLiteral("Removing empty tag name from the list of tag names for note ") << note.localUid());
            tagNameIt = tagNames.erase(tagNameIt);
        }

        if (Q_UNLIKELY(tagNames.isEmpty()))
        {
            QNDEBUG(QStringLiteral("No tag names are left for note ") << note.localUid()
                    << QStringLiteral(" after the cleanup of empty tag names, can add the note to local storage right away"));

            addNoteToLocalStorage(note);
            it = importedNotes.erase(it);
            continue;
        }

        ++it;
    }

    if (importedNotes.isEmpty()) {
        QNDEBUG(QStringLiteral("No notes had any tags, waiting for the completion of all add note requests"));
        return;
    }

    m_notesPendingTagAddition = importedNotes;
    QNDEBUG(QStringLiteral("There are ") << m_notesPendingTagAddition << QStringLiteral(" which need tags assignment to them"));

    if (!m_tagModel.allTagsListed()) {
        QNDEBUG(QStringLiteral("Not all tags were listed from the tag model, waiting for it"));
        return;
    }

    processNotesPendingTagAddition();
}

void EnexImporter::clear()
{
    QNDEBUG(QStringLiteral("EnexImporter::clear"));

    m_tagNamesByImportedNoteLocalUid.clear();
    m_addTagRequestIdByTagNameBimap.clear();
    m_expungedTagLocalUids.clear();

    m_notesPendingTagAddition.clear();
    m_addNoteRequestIds.clear();

    disconnectFromLocalStorage();
}

void EnexImporter::onAddTagComplete(Tag tag, QUuid requestId)
{
    auto it = m_addTagRequestIdByTagNameBimap.right.find(requestId);
    if (it == m_addTagRequestIdByTagNameBimap.right.end()) {
        return;
    }

    QNDEBUG(QStringLiteral("EnexImporter::onAddTagComplete: request id = ")
            << requestId << QStringLiteral(", tag: ") << tag);

    Q_UNUSED(m_addTagRequestIdByTagNameBimap.right.erase(it))

    if (Q_UNLIKELY(!tag.hasName())) {
        ErrorString errorDescription(QT_TRANSLATE_NOOP("", "Can't import ENEX: internal error, "
                                                       "could not create a new tag in the local storage: "
                                                       "the added tag has no name"));
        QNWARNING(errorDescription);
        emit enexImportFailed(errorDescription);
        return;
    }

    QString tagName = tag.name().toLower();

    for(auto noteIt = m_notesPendingTagAddition.begin(); noteIt != m_notesPendingTagAddition.end(); )
    {
        Note & note = *noteIt;

        auto tagIt = m_tagNamesByImportedNoteLocalUid.find(note.localUid());
        if (Q_UNLIKELY(tagIt == m_tagNamesByImportedNoteLocalUid.end())) {
            QNWARNING(QStringLiteral("Detected note within the list of those pending tags addition which doesn't "
                                     "really wait for tags addition"));
            addNoteToLocalStorage(note);
            noteIt = m_notesPendingTagAddition.erase(noteIt);
            continue;
        }

        QStringList & tagNames = tagIt.value();
        for(auto tagNameIt = tagNames.begin(); tagNameIt != tagNames.end(); )
        {
            const QString & currentTagName = *tagNameIt;
            if (currentTagName.toLower() != tagName) {
                ++tagNameIt;
                continue;
            }

            QNDEBUG(QStringLiteral("Found tag for note ") << note.localUid() << QStringLiteral(": name = ")
                    << tagName << QStringLiteral(", local uid = ") << tag.localUid());

            if (!note.hasTagLocalUids() || !note.tagLocalUids().contains(tag.localUid())) {
                note.addTagLocalUid(tag.localUid());
            }

            Q_UNUSED(tagNames.erase(tagNameIt))
            break;
        }

        if (!tagNames.isEmpty()) {
            QNTRACE(QStringLiteral("Still pending ") << tagNames.size() << QStringLiteral(" tag names for note ")
                    << note.localUid());
            ++noteIt;
            continue;
        }

        QNDEBUG(QStringLiteral("Added the last missing tag for note ") << note.localUid()
                << QStringLiteral(", can send it to the local storage right away"));

        Q_UNUSED(m_tagNamesByImportedNoteLocalUid.erase(tagIt))

        addNoteToLocalStorage(note);
        noteIt = m_notesPendingTagAddition.erase(noteIt);
    }
}

void EnexImporter::onAddTagFailed(Tag tag, ErrorString errorDescription, QUuid requestId)
{
    auto it = m_addTagRequestIdByTagNameBimap.right.find(requestId);
    if (it == m_addTagRequestIdByTagNameBimap.right.end()) {
        return;
    }

    QNDEBUG(QStringLiteral("EnexImporter::onAddTagFailed: request id = ")
            << requestId << QStringLiteral(", error description = ")
            << errorDescription << QStringLiteral(", tag: ") << tag);

    Q_UNUSED(m_addTagRequestIdByTagNameBimap.right.erase(it))

    ErrorString error(QT_TRANSLATE_NOOP("", "Can't import ENEX"));
    error.additionalBases().append(errorDescription.base());
    error.additionalBases().append(errorDescription.additionalBases());
    error.details() = errorDescription.details();
    emit enexImportFailed(error);
}

void EnexImporter::onExpungeTagComplete(Tag tag, QStringList expungedChildTagLocalUids, QUuid requestId)
{
    QNDEBUG(QStringLiteral("EnexImporter::onExpungeTagComplete: request id = ")
            << requestId << QStringLiteral(", tag: ") << tag << QStringLiteral("\nExpunged child tag local uids: ")
            << expungedChildTagLocalUids.join(QStringLiteral(", ")));

    if (!isInProgress()) {
        QNDEBUG(QStringLiteral("Not in progress right now, won't do anything"));
        return;
    }

    QStringList expungedTagLocalUids;
    expungedTagLocalUids.reserve(expungedChildTagLocalUids.size() + 1);
    expungedTagLocalUids = expungedChildTagLocalUids;
    expungedTagLocalUids << tag.localUid();

    for(auto it = expungedTagLocalUids.constBegin(), end = expungedTagLocalUids.constEnd(); it != end; ++it) {
        Q_UNUSED(m_expungedTagLocalUids.insert(*it))
    }

    // Just in case check if some of our pending notes have either of these tag local uids, if so, remove them
    for(auto it = m_notesPendingTagAddition.begin(), end = m_notesPendingTagAddition.end(); it != end; ++it)
    {
        Note & note = *it;

        if (!note.hasTagLocalUids()) {
            continue;
        }

        QStringList tagLocalUids = note.tagLocalUids();
        for(auto tagIt = expungedTagLocalUids.constBegin(), tagEnd = expungedTagLocalUids.constEnd(); tagIt != tagEnd; ++tagIt)
        {
            if (tagLocalUids.contains(*tagIt)) {
                Q_UNUSED(tagLocalUids.removeAll(*tagIt))
            }
        }

        note.setTagLocalUids(tagLocalUids);
    }
}

void EnexImporter::onAddNoteComplete(Note note, QUuid requestId)
{
    auto it = m_addNoteRequestIds.find(requestId);
    if (it == m_addNoteRequestIds.end()) {
        return;
    }

    QNDEBUG(QStringLiteral("EnexImporter::onAddNoteComplete: request id = ") << requestId
            << QStringLiteral(", note: ") << note);

    Q_UNUSED(m_addNoteRequestIds.erase(it))

    if (!m_addNoteRequestIds.isEmpty()) {
        QNDEBUG(QStringLiteral("Still pending ") << m_addNoteRequestIds.size() << QStringLiteral(" add note request ids"));
        return;
    }

    if (!m_notesPendingTagAddition.isEmpty()) {
        QNDEBUG(QStringLiteral("There are still ") << m_notesPendingTagAddition.size() << QStringLiteral(" notes pending tag addition"));
        return;
    }

    QNDEBUG(QStringLiteral("There are no pending add note requests and no notes pending tags addition => "
                           "it looks like the import has finished"));
    emit enexImportedSuccessfully(m_enexFilePath);
}

void EnexImporter::onAddNoteFailed(Note note, ErrorString errorDescription, QUuid requestId)
{
    auto it = m_addNoteRequestIds.find(requestId);
    if (it == m_addNoteRequestIds.end()) {
        return;
    }

    QNWARNING(QStringLiteral("EnexImporter::onAddNoteFailed: request id = ") << requestId
              << QStringLiteral(", error description = ") << errorDescription
              << QStringLiteral(", note: ") << note);

    Q_UNUSED(m_addNoteRequestIds.erase(it))

    ErrorString error(QT_TRANSLATE_NOOP("", "Can't import ENEX"));
    error.additionalBases().append(errorDescription.base());
    error.additionalBases().append(errorDescription.additionalBases());
    error.details() = errorDescription.details();
    emit enexImportFailed(error);
}

void EnexImporter::onAllTagsListed()
{
    QNDEBUG(QStringLiteral("EnexImporter::onAllTagsListed"));

    QObject::disconnect(&m_tagModel, QNSIGNAL(TagModel,notifyAllTagsListed),
                        this, QNSLOT(EnexImporter,onAllTagsListed));

    if (!isInProgress()) {
        return;
    }

    processNotesPendingTagAddition();
}

void EnexImporter::connectToLocalStorage()
{
    QNDEBUG(QStringLiteral("EnexImporter::connectToLocalStorage"));

    if (m_connectedToLocalStorage) {
        QNDEBUG(QStringLiteral("Already connected to the local storage"));
        return;
    }

    QObject::connect(this, QNSIGNAL(EnexImporter,addTag,Tag,QUuid),
                     &m_localStorageManagerAsync, QNSLOT(LocalStorageManagerAsync,onAddTagRequest,Tag,QUuid));
    QObject::connect(&m_localStorageManagerAsync, QNSIGNAL(LocalStorageManagerAsync,addTagComplete,Tag,QUuid),
                     this, QNSLOT(EnexImporter,onAddTagComplete,Tag,QUuid));
    QObject::connect(&m_localStorageManagerAsync, QNSIGNAL(LocalStorageManagerAsync,addTagFailed,Tag,ErrorString,QUuid),
                     this, QNSLOT(EnexImporter,onAddTagFailed,Tag,ErrorString,QUuid));
    QObject::connect(&m_localStorageManagerAsync, QNSIGNAL(LocalStorageManagerAsync,expungeTagComplete,Tag,QStringList,QUuid),
                     this, QNSLOT(EnexImporter,onExpungeTagComplete,Tag,QStringList,QUuid));

    QObject::connect(this, QNSIGNAL(EnexImporter,addNote,Note,QUuid),
                     &m_localStorageManagerAsync, QNSLOT(LocalStorageManagerAsync,onAddNoteRequest,Note,QUuid));
    QObject::connect(&m_localStorageManagerAsync, QNSIGNAL(LocalStorageManagerAsync,addNoteComplete,Note,QUuid),
                     this, QNSLOT(EnexImporter,onAddNoteComplete,Note,QUuid));
    QObject::connect(&m_localStorageManagerAsync, QNSIGNAL(LocalStorageManagerAsync,addNoteFailed,Note,ErrorString,QUuid),
                     this, QNSLOT(EnexImporter,onAddNoteFailed,Note,ErrorString,QUuid));

    m_connectedToLocalStorage = true;
}

void EnexImporter::disconnectFromLocalStorage()
{
    QNDEBUG(QStringLiteral("EnexImporter::disconnectFromLocalStorage"));

    if (!m_connectedToLocalStorage) {
        QNTRACE(QStringLiteral("Not connected to local storage at the moment"));
        return;
    }

    QObject::disconnect(this, QNSIGNAL(EnexImporter,addTag,Tag,QUuid),
                        &m_localStorageManagerAsync, QNSLOT(LocalStorageManagerAsync,onAddTagRequest,Tag,QUuid));
    QObject::disconnect(&m_localStorageManagerAsync, QNSIGNAL(LocalStorageManagerAsync,addTagComplete,Tag,QUuid),
                        this, QNSLOT(EnexImporter,onAddTagComplete,Tag,QUuid));
    QObject::disconnect(&m_localStorageManagerAsync, QNSIGNAL(LocalStorageManagerAsync,addTagFailed,Tag,ErrorString,QUuid),
                        this, QNSLOT(EnexImporter,onAddTagFailed,Tag,ErrorString,QUuid));
    QObject::disconnect(&m_localStorageManagerAsync, QNSIGNAL(LocalStorageManagerAsync,expungeTagComplete,Tag,QStringList,QUuid),
                        this, QNSLOT(EnexImporter,onExpungeTagComplete,Tag,QStringList,QUuid));

    QObject::disconnect(this, QNSIGNAL(EnexImporter,addNote,Note,QUuid),
                        &m_localStorageManagerAsync, QNSLOT(LocalStorageManagerAsync,onAddNoteRequest,Note,QUuid));
    QObject::disconnect(&m_localStorageManagerAsync, QNSIGNAL(LocalStorageManagerAsync,addNoteComplete,Note,QUuid),
                        this, QNSLOT(EnexImporter,onAddNoteComplete,Note,QUuid));
    QObject::disconnect(&m_localStorageManagerAsync, QNSIGNAL(LocalStorageManagerAsync,addNoteFailed,Note,ErrorString,QUuid),
                        this, QNSLOT(EnexImporter,onAddNoteFailed,Note,ErrorString,QUuid));

    m_connectedToLocalStorage = false;
}

void EnexImporter::processNotesPendingTagAddition()
{
    QNDEBUG(QStringLiteral("EnexImporter::processNotesPendingTagAddition"));

    for(auto it = m_notesPendingTagAddition.begin(); it != m_notesPendingTagAddition.end(); )
    {
        Note & note = *it;

        auto tagIt = m_tagNamesByImportedNoteLocalUid.find(note.localUid());
        if (Q_UNLIKELY(tagIt == m_tagNamesByImportedNoteLocalUid.end())) {
            QNWARNING(QStringLiteral("Detected note within the list of those pending tags addition which doesn't "
                                     "really wait for tags addition"));
            addNoteToLocalStorage(note);
            it = m_notesPendingTagAddition.erase(it);
            continue;
        }

        QStringList & tagNames = tagIt.value();
        QStringList nonexistentTagNames;
        nonexistentTagNames.reserve(tagNames.size());

        for(auto tagNameIt = tagNames.begin(); tagNameIt != tagNames.end(); )
        {
            const QString & tagName = *tagNameIt;

            QString tagLocalUid = m_tagModel.localUidForItemName(tagName);
            if (tagLocalUid.isEmpty()) {
                nonexistentTagNames << tagName;
                QNDEBUG(QStringLiteral("No tag called \"") << tagName << QStringLiteral("\" exists, it would need to be created"));
                ++tagNameIt;
                continue;
            }

            if (m_expungedTagLocalUids.contains(tagLocalUid)) {
                nonexistentTagNames << tagName;
                QNDEBUG(QStringLiteral("Tag local uid ") << tagLocalUid
                        << QStringLiteral(" found within the model has been marked as the one of expunged tag; "
                                          "will need to create a new tag with such name"));
                ++tagNameIt;
                continue;
            }

            QNTRACE(QStringLiteral("Local uid for tag name ") << tagName << QStringLiteral(" is ") << tagLocalUid);

            if (!note.hasTagLocalUids() || !note.tagLocalUids().contains(tagLocalUid)) {
                note.addTagLocalUid(tagLocalUid);
            }

            tagNameIt = tagNames.erase(tagNameIt);
        }

        if (tagNames.isEmpty()) {
            QNTRACE(QStringLiteral("Found all tags for note with local uid ") << note.localUid());
            Q_UNUSED(m_tagNamesByImportedNoteLocalUid.erase(tagIt))
        }

        if (nonexistentTagNames.isEmpty()) {
            QNDEBUG(QStringLiteral("Found no nonexistent tags, can send this note to local storage right away"));
            addNoteToLocalStorage(note);
            it = m_notesPendingTagAddition.erase(it);
            continue;
        }

        for(auto nonexistentTagIt = nonexistentTagNames.constBegin(),
            nonexistentTagEnd = nonexistentTagNames.constEnd();
            nonexistentTagIt != nonexistentTagEnd; ++nonexistentTagIt)
        {
            const QString & nonexistentTag = *nonexistentTagIt;

            auto requestIdIt = m_addTagRequestIdByTagNameBimap.left.find(nonexistentTag.toLower());
            if (requestIdIt != m_addTagRequestIdByTagNameBimap.left.end()) {
                QNTRACE(QStringLiteral("Nonexistent tag ") << nonexistentTag
                        << QStringLiteral(" is already being added to the local storage"));
            }
            else {
                addTagToLocalStorage(nonexistentTag);
            }
        }

        ++it;
    }
}

void EnexImporter::addNoteToLocalStorage(const Note & note)
{
    QNDEBUG(QStringLiteral("EnexImporter::addNoteToLocalStorage"));

    QUuid requestId = QUuid::createUuid();
    Q_UNUSED(m_addNoteRequestIds.insert(requestId));
    QNTRACE(QStringLiteral("Emitting the request to add note to local storage: request id = ")
            << requestId << QStringLiteral(", note: ") << note);
    emit addNote(note, requestId);
}

void EnexImporter::addTagToLocalStorage(const QString & tagName)
{
    QNDEBUG(QStringLiteral("EnexImporter::addTagToLocalStorage: ") << tagName);

    Tag newTag;
    newTag.setName(tagName);

    QUuid requestId = QUuid::createUuid();
    Q_UNUSED(m_addTagRequestIdByTagNameBimap.insert(AddTagRequestIdByTagNameBimap::value_type(tagName.toLower(), requestId)))
    QNTRACE(QStringLiteral("Emitting the request to add tag to local storage: request id = ")
            << requestId << QStringLiteral(", tag: ") << newTag);
    emit addTag(newTag, requestId);
}

} // namespace quentier
