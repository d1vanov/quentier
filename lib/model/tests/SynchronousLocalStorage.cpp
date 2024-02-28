/*
 * Copyright 2024 Dmitry Ivanov
 *
 * This file is part of Quentier.
 *
 * Quentier is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * Quentier is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Quentier. If not, see <http://www.gnu.org/licenses/>.
 */

#include "SynchronousLocalStorage.h"
#include "SynchronousLocalStorageNotifier.h"
#include "TestUtils.h"

#include <quentier/exception/InvalidArgument.h>

#include <utility>

namespace quentier {

namespace {

template <class T>
[[nodiscard]] bool checkFuture(QFuture<T> & f)
{
    try {
        f.waitForFinished();
    }
    catch (...) {
        return false;
    }

    return true;
}

} // namespace

SynchronousLocalStorage::SynchronousLocalStorage(
    local_storage::ILocalStoragePtr underlyingLocalStorage) :
    m_underlyingLocalStorage{std::move(underlyingLocalStorage)}
{
    if (Q_UNLIKELY(!m_underlyingLocalStorage)) {
        throw InvalidArgument{ErrorString{
            "SynchronousLocalStorage ctor: underlying local storage is null"}};
    }

    m_notifier = new SynchronousLocalStorageNotifier;
}

SynchronousLocalStorage::~SynchronousLocalStorage()
{
    m_notifier->deleteLater();
}

QFuture<bool> SynchronousLocalStorage::isVersionTooHigh() const
{
    auto f = m_underlyingLocalStorage->isVersionTooHigh();
    waitForFuture(f);
    return f;
}

QFuture<bool> SynchronousLocalStorage::requiresUpgrade() const
{
    auto f = m_underlyingLocalStorage->requiresUpgrade();
    waitForFuture(f);
    return f;
}

QFuture<QList<local_storage::IPatchPtr>>
SynchronousLocalStorage::requiredPatches() const
{
    auto f = m_underlyingLocalStorage->requiredPatches();
    waitForFuture(f);
    return f;
}

QFuture<qint32> SynchronousLocalStorage::version() const
{
    auto f = m_underlyingLocalStorage->version();
    waitForFuture(f);
    return f;
}

QFuture<qint32> SynchronousLocalStorage::highestSupportedVersion() const
{
    auto f = m_underlyingLocalStorage->highestSupportedVersion();
    waitForFuture(f);
    return f;
}

QFuture<quint32> SynchronousLocalStorage::userCount() const
{
    auto f = m_underlyingLocalStorage->userCount();
    waitForFuture(f);
    return f;
}

QFuture<void> SynchronousLocalStorage::putUser(qevercloud::User user)
{
    auto f = m_underlyingLocalStorage->putUser(user);
    waitForFuture(f);
    if (checkFuture(f)) {
        m_notifier->notifyUserPut(std::move(user));
    }
    return f;
}

QFuture<std::optional<qevercloud::User>> SynchronousLocalStorage::findUserById(
    qevercloud::UserID userId) const
{
    auto f = m_underlyingLocalStorage->findUserById(userId);
    waitForFuture(f);
    return f;
}

QFuture<void> SynchronousLocalStorage::expungeUserById(
    const qevercloud::UserID userId)
{
    auto f = m_underlyingLocalStorage->expungeUserById(userId);
    waitForFuture(f);
    if (checkFuture(f)) {
        m_notifier->notifyUserExpunged(userId);
    }
    return f;
}

QFuture<quint32> SynchronousLocalStorage::notebookCount() const
{
    auto f = m_underlyingLocalStorage->notebookCount();
    waitForFuture(f);
    return f;
}

QFuture<void> SynchronousLocalStorage::putNotebook(
    qevercloud::Notebook notebook)
{
    auto f = m_underlyingLocalStorage->putNotebook(notebook);
    waitForFuture(f);
    if (checkFuture(f)) {
        m_notifier->notifyNotebookPut(std::move(notebook));
    }
    return f;
}

QFuture<std::optional<qevercloud::Notebook>>
SynchronousLocalStorage::findNotebookByLocalId(QString notebookLocalId) const
{
    auto f = m_underlyingLocalStorage->findNotebookByLocalId(
        std::move(notebookLocalId));
    waitForFuture(f);
    return f;
}

QFuture<std::optional<qevercloud::Notebook>>
SynchronousLocalStorage::findNotebookByGuid(qevercloud::Guid guid) const
{
    auto f = m_underlyingLocalStorage->findNotebookByGuid(std::move(guid));
    waitForFuture(f);
    return f;
}

QFuture<std::optional<qevercloud::Notebook>> SynchronousLocalStorage::findNotebookByName(
    QString notebookName,
    std::optional<qevercloud::Guid> linkedNotebookGuid) const
{
    auto f = m_underlyingLocalStorage->findNotebookByName(
        std::move(notebookName), std::move(linkedNotebookGuid));
    waitForFuture(f);
    return f;
}

QFuture<std::optional<qevercloud::Notebook>>
SynchronousLocalStorage::findDefaultNotebook() const
{
    auto f = m_underlyingLocalStorage->findDefaultNotebook();
    waitForFuture(f);
    return f;
}

QFuture<void> SynchronousLocalStorage::expungeNotebookByLocalId(
    QString notebookLocalId)
{
    auto f = m_underlyingLocalStorage->expungeNotebookByLocalId(
        notebookLocalId);
    waitForFuture(f);
    if (checkFuture(f)) {
        m_notifier->notifyNotebookExpunged(std::move(notebookLocalId));
    }
    return f;
}

QFuture<void> SynchronousLocalStorage::expungeNotebookByGuid(
    qevercloud::Guid notebookGuid)
{
    QString notebookLocalId;
    {
        auto f = m_underlyingLocalStorage->findNotebookByGuid(notebookGuid);
        waitForFuture(f);

        try {
            Q_ASSERT(f.resultCount() == 1);
            const auto notebook = f.result();
            if (notebook) {
                notebookLocalId = notebook->localId();
            }
        }
        catch (...) {
        }
    }

    auto f = m_underlyingLocalStorage->expungeNotebookByGuid(
        std::move(notebookGuid));
    waitForFuture(f);
    if (!notebookLocalId.isEmpty() && checkFuture(f)) {
        m_notifier->notifyNotebookExpunged(std::move(notebookLocalId));
    }
    return f;
}

QFuture<void> SynchronousLocalStorage::expungeNotebookByName(
    QString name,
    std::optional<qevercloud::Guid> linkedNotebookGuid)
{
    QString notebookLocalId;
    {
        auto f = m_underlyingLocalStorage->findNotebookByName(
            name, linkedNotebookGuid);
        waitForFuture(f);

        try {
            Q_ASSERT(f.resultCount() == 1);
            const auto notebook = f.result();
            if (notebook) {
                notebookLocalId = notebook->localId();
            }
        }
        catch (...) {
        }
    }

    auto f = m_underlyingLocalStorage->expungeNotebookByName(
        std::move(name), std::move(linkedNotebookGuid));
    waitForFuture(f);
    if (!notebookLocalId.isEmpty() && checkFuture(f)) {
        m_notifier->notifyNotebookExpunged(std::move(notebookLocalId));
    }
    return f;
}

QFuture<QList<qevercloud::Notebook>> SynchronousLocalStorage::listNotebooks(
    ListNotebooksOptions options) const
{
    auto f = m_underlyingLocalStorage->listNotebooks(std::move(options));
    waitForFuture(f);
    return f;
}

QFuture<QList<qevercloud::SharedNotebook>>
SynchronousLocalStorage::listSharedNotebooks(
    qevercloud::Guid notebookGuid) const
{
    auto f =
        m_underlyingLocalStorage->listSharedNotebooks(std::move(notebookGuid));
    waitForFuture(f);
    return f;
}

QFuture<QSet<qevercloud::Guid>> SynchronousLocalStorage::listNotebookGuids(
    ListGuidsFilters filters,
    std::optional<qevercloud::Guid> linkedNotebookGuid) const
{
    auto f = m_underlyingLocalStorage->listNotebookGuids(
        std::move(filters), std::move(linkedNotebookGuid));
    waitForFuture(f);
    return f;
}

QFuture<quint32> SynchronousLocalStorage::linkedNotebookCount() const
{
    auto f = m_underlyingLocalStorage->linkedNotebookCount();
    waitForFuture(f);
    return f;
}

QFuture<void> SynchronousLocalStorage::putLinkedNotebook(
    qevercloud::LinkedNotebook linkedNotebook)
{
    auto f = m_underlyingLocalStorage->putLinkedNotebook(linkedNotebook);
    waitForFuture(f);
    if (checkFuture(f)) {
        m_notifier->notifyLinkedNotebookPut(std::move(linkedNotebook));
    }
    return f;
}

QFuture<std::optional<qevercloud::LinkedNotebook>>
SynchronousLocalStorage::findLinkedNotebookByGuid(qevercloud::Guid guid) const
{
    auto f =
        m_underlyingLocalStorage->findLinkedNotebookByGuid(std::move(guid));
    waitForFuture(f);
    return f;
}

QFuture<void> SynchronousLocalStorage::expungeLinkedNotebookByGuid(
    qevercloud::Guid guid)
{
    auto f = m_underlyingLocalStorage->expungeLinkedNotebookByGuid(guid);
    waitForFuture(f);
    if (checkFuture(f)) {
        m_notifier->notifyLinkedNotebookExpunged(std::move(guid));
    }
    return f;
}

QFuture<QList<qevercloud::LinkedNotebook>>
SynchronousLocalStorage::listLinkedNotebooks(
    ListLinkedNotebooksOptions options) const
{
    auto f = m_underlyingLocalStorage->listLinkedNotebooks(std::move(options));
    waitForFuture(f);
    return f;
}

QFuture<quint32> SynchronousLocalStorage::noteCount(
    const NoteCountOptions options) const
{
    auto f = m_underlyingLocalStorage->noteCount(options);
    waitForFuture(f);
    return f;
}

QFuture<quint32> SynchronousLocalStorage::noteCountPerNotebookLocalId(
    QString notebookLocalId, const NoteCountOptions options) const
{
    auto f = m_underlyingLocalStorage->noteCountPerNotebookLocalId(
        std::move(notebookLocalId), options);
    waitForFuture(f);
    return f;
}

QFuture<quint32> SynchronousLocalStorage::noteCountPerTagLocalId(
    QString tagLocalId, const NoteCountOptions options) const
{
    auto f = m_underlyingLocalStorage->noteCountPerTagLocalId(
        std::move(tagLocalId), options);
    waitForFuture(f);
    return f;
}

QFuture<QHash<QString, quint32>> SynchronousLocalStorage::noteCountsPerTags(
    ListTagsOptions listTagsOptions, const NoteCountOptions options) const
{
    auto f = m_underlyingLocalStorage->noteCountsPerTags(
        std::move(listTagsOptions), options);
    waitForFuture(f);
    return f;
}

QFuture<quint32> SynchronousLocalStorage::noteCountPerNotebookAndTagLocalIds(
    QStringList notebookLocalIds, QStringList tagLocalIds,
    const NoteCountOptions options) const
{
    auto f = m_underlyingLocalStorage->noteCountPerNotebookAndTagLocalIds(
        std::move(notebookLocalIds), std::move(tagLocalIds), options);
    waitForFuture(f);
    return f;
}

QFuture<void> SynchronousLocalStorage::putNote(qevercloud::Note note)
{
    auto f = m_underlyingLocalStorage->putNote(note);
    waitForFuture(f);
    if (checkFuture(f)) {
        m_notifier->notifyNotePut(std::move(note));
    }
    return f;
}

QFuture<void> SynchronousLocalStorage::updateNote(
    qevercloud::Note note, const UpdateNoteOptions options)
{
    auto f = m_underlyingLocalStorage->updateNote(note, options);
    waitForFuture(f);
    if (checkFuture(f)) {
        m_notifier->notifyNoteUpdated(std::move(note), options);
    }
    return f;
}

QFuture<std::optional<qevercloud::Note>>
SynchronousLocalStorage::findNoteByLocalId(
    QString noteLocalId, const FetchNoteOptions options) const
{
    auto f = m_underlyingLocalStorage->findNoteByLocalId(
        std::move(noteLocalId), options);
    waitForFuture(f);
    return f;
}

QFuture<std::optional<qevercloud::Note>>
SynchronousLocalStorage::findNoteByGuid(
    qevercloud::Guid noteGuid, const FetchNoteOptions options) const
{
    auto f =
        m_underlyingLocalStorage->findNoteByGuid(std::move(noteGuid), options);
    waitForFuture(f);
    return f;
}

QFuture<QList<qevercloud::Note>> SynchronousLocalStorage::listNotes(
    const FetchNoteOptions fetchOptions, ListNotesOptions listOptions) const
{
    auto f = m_underlyingLocalStorage->listNotes(
        fetchOptions, std::move(listOptions));
    waitForFuture(f);
    return f;
}

QFuture<QList<qevercloud::Note>>
SynchronousLocalStorage::listNotesPerNotebookLocalId(
    QString notebookLocalId, const FetchNoteOptions fetchOptions,
    ListNotesOptions listOptions) const
{
    auto f = m_underlyingLocalStorage->listNotesPerNotebookLocalId(
        std::move(notebookLocalId), fetchOptions, std::move(listOptions));
    waitForFuture(f);
    return f;
}

QFuture<QList<qevercloud::Note>>
SynchronousLocalStorage::listNotesPerTagLocalId(
    QString tagLocalId, const FetchNoteOptions fetchOptions,
    ListNotesOptions listOptions) const
{
    auto f = m_underlyingLocalStorage->listNotesPerTagLocalId(
        std::move(tagLocalId), fetchOptions, std::move(listOptions));
    waitForFuture(f);
    return f;
}

QFuture<QList<qevercloud::Note>>
SynchronousLocalStorage::listNotesPerNotebookAndTagLocalIds(
    QStringList notebookLocalIds, QStringList tagLocalIds,
    const FetchNoteOptions fetchOptions, ListNotesOptions listOptions) const
{
    auto f = m_underlyingLocalStorage->listNotesPerNotebookAndTagLocalIds(
        std::move(notebookLocalIds), std::move(tagLocalIds), fetchOptions,
        std::move(listOptions));
    waitForFuture(f);
    return f;
}

QFuture<QList<qevercloud::Note>> SynchronousLocalStorage::listNotesByLocalIds(
    QStringList noteLocalIds, const FetchNoteOptions fetchOptions,
    ListNotesOptions listOptions) const
{
    auto f = m_underlyingLocalStorage->listNotesByLocalIds(
        std::move(noteLocalIds), fetchOptions, std::move(listOptions));
    waitForFuture(f);
    return f;
}

QFuture<QSet<qevercloud::Guid>> SynchronousLocalStorage::listNoteGuids(
    ListGuidsFilters filters,
    std::optional<qevercloud::Guid> linkedNotebookGuid) const
{
    auto f = m_underlyingLocalStorage->listNoteGuids(
        std::move(filters), std::move(linkedNotebookGuid));
    waitForFuture(f);
    return f;
}

QFuture<QList<qevercloud::Note>> SynchronousLocalStorage::queryNotes(
    local_storage::NoteSearchQuery query,
    const FetchNoteOptions fetchOptions) const
{
    auto f =
        m_underlyingLocalStorage->queryNotes(std::move(query), fetchOptions);
    waitForFuture(f);
    return f;
}

QFuture<QStringList> SynchronousLocalStorage::queryNoteLocalIds(
    local_storage::NoteSearchQuery query) const
{
    auto f = m_underlyingLocalStorage->queryNoteLocalIds(std::move(query));
    waitForFuture(f);
    return f;
}

QFuture<void> SynchronousLocalStorage::expungeNoteByLocalId(QString noteLocalId)
{
    auto f = m_underlyingLocalStorage->expungeNoteByLocalId(noteLocalId);
    waitForFuture(f);
    if (checkFuture(f)) {
        m_notifier->notifyNoteExpunged(std::move(noteLocalId));
    }
    return f;
}

QFuture<void> SynchronousLocalStorage::expungeNoteByGuid(
    qevercloud::Guid noteGuid)
{
    QString noteLocalId;
    {
        auto f = m_underlyingLocalStorage->findNoteByGuid(
            noteGuid, local_storage::ILocalStorage::FetchNoteOptions{});
        waitForFuture(f);

        try {
            Q_ASSERT(f.resultCount() == 1);
            const auto note = f.result();
            if (note) {
                noteLocalId = note->localId();
            }
        }
        catch (...) {
        }
    }

    auto f = m_underlyingLocalStorage->expungeNoteByGuid(std::move(noteGuid));
    waitForFuture(f);
    if (!noteLocalId.isEmpty() && checkFuture(f)) {
        m_notifier->notifyNoteExpunged(std::move(noteLocalId));
    }
    return f;
}

QFuture<quint32> SynchronousLocalStorage::tagCount() const
{
    auto f = m_underlyingLocalStorage->tagCount();
    waitForFuture(f);
    return f;
}

QFuture<void> SynchronousLocalStorage::putTag(qevercloud::Tag tag)
{
    auto f = m_underlyingLocalStorage->putTag(tag);
    waitForFuture(f);
    if (checkFuture(f)) {
        m_notifier->notifyTagPut(std::move(tag));
    }
    return f;
}

QFuture<std::optional<qevercloud::Tag>>
SynchronousLocalStorage::findTagByLocalId(QString tagLocalId) const
{
    auto f = m_underlyingLocalStorage->findTagByLocalId(std::move(tagLocalId));
    waitForFuture(f);
    return f;
}

QFuture<std::optional<qevercloud::Tag>> SynchronousLocalStorage::findTagByGuid(
    qevercloud::Guid tagGuid) const
{
    auto f = m_underlyingLocalStorage->findTagByGuid(std::move(tagGuid));
    waitForFuture(f);
    return f;
}

QFuture<std::optional<qevercloud::Tag>> SynchronousLocalStorage::findTagByName(
    QString tagName, std::optional<qevercloud::Guid> linkedNotebookGuid) const
{
    auto f = m_underlyingLocalStorage->findTagByName(
        std::move(tagName), std::move(linkedNotebookGuid));
    waitForFuture(f);
    return f;
}

QFuture<QList<qevercloud::Tag>> SynchronousLocalStorage::listTags(
    ListTagsOptions options) const
{
    auto f = m_underlyingLocalStorage->listTags(std::move(options));
    waitForFuture(f);
    return f;
}

QFuture<QList<qevercloud::Tag>> SynchronousLocalStorage::listTagsPerNoteLocalId(
    QString noteLocalId, ListTagsOptions options) const
{
    auto f = m_underlyingLocalStorage->listTagsPerNoteLocalId(
        std::move(noteLocalId), std::move(options));
    waitForFuture(f);
    return f;
}

QFuture<QSet<qevercloud::Guid>> SynchronousLocalStorage::listTagGuids(
    ListGuidsFilters filters,
    std::optional<qevercloud::Guid> linkedNotebookGuid) const
{
    auto f = m_underlyingLocalStorage->listTagGuids(
        std::move(filters), std::move(linkedNotebookGuid));
    waitForFuture(f);
    return f;
}

QFuture<void> SynchronousLocalStorage::expungeTagByLocalId(QString tagLocalId)
{
    QStringList childTagLocalIds;
    {
        auto f = m_underlyingLocalStorage->listTags();
        waitForFuture(f);

        try {
            Q_ASSERT(f.resultCount() == 1);
            const auto tags = f.result();
            for (const auto & tag: std::as_const(tags)) {
                if (tag.parentTagLocalId() == tagLocalId) {
                    childTagLocalIds << tag.localId();
                }
            }
        }
        catch (...) {
        }
    }

    auto f = m_underlyingLocalStorage->expungeTagByLocalId(tagLocalId);
    waitForFuture(f);
    if (checkFuture(f)) {
        m_notifier->notifyTagExpunged(
            std::move(tagLocalId), std::move(childTagLocalIds));
    }
    return f;
}

QFuture<void> SynchronousLocalStorage::expungeTagByGuid(
    qevercloud::Guid tagGuid)
{
    QString tagLocalId;
    {
        auto f = m_underlyingLocalStorage->findTagByGuid(tagGuid);
        waitForFuture(f);

        try {
            Q_ASSERT(f.resultCount() == 1);
            const auto tag = f.result();
            if (tag) {
                tagLocalId = tag->localId();
            }
        }
        catch (...) {
        }
    }

    QStringList childTagLocalIds;
    if (!tagLocalId.isEmpty())
    {
        auto f = m_underlyingLocalStorage->listTags();
        waitForFuture(f);

        try {
            Q_ASSERT(f.resultCount() == 1);
            const auto tags = f.result();
            for (const auto & tag: std::as_const(tags)) {
                if (tag.parentTagLocalId() == tagLocalId) {
                    childTagLocalIds << tag.localId();
                }
            }
        }
        catch (...) {
        }
    }

    auto f = m_underlyingLocalStorage->expungeTagByGuid(std::move(tagGuid));
    waitForFuture(f);
    if (!tagLocalId.isEmpty() && checkFuture(f)) {
        m_notifier->notifyTagExpunged(
            std::move(tagLocalId), std::move(childTagLocalIds));
    }
    return f;
}

QFuture<void> SynchronousLocalStorage::expungeTagByName(
    QString name, std::optional<qevercloud::Guid> linkedNotebookGuid)
{
    QString tagLocalId;
    {
        auto f =
            m_underlyingLocalStorage->findTagByName(name, linkedNotebookGuid);
        waitForFuture(f);

        try {
            Q_ASSERT(f.resultCount() == 1);
            const auto tag = f.result();
            if (tag) {
                tagLocalId = tag->localId();
            }
        }
        catch (...) {
        }
    }

    QStringList childTagLocalIds;
    if (!tagLocalId.isEmpty())
    {
        auto f = m_underlyingLocalStorage->listTags();
        waitForFuture(f);

        try {
            Q_ASSERT(f.resultCount() == 1);
            const auto tags = f.result();
            for (const auto & tag: std::as_const(tags)) {
                if (tag.parentTagLocalId() == tagLocalId) {
                    childTagLocalIds << tag.localId();
                }
            }
        }
        catch (...) {
        }
    }

    auto f = m_underlyingLocalStorage->expungeTagByName(
        std::move(name), std::move(linkedNotebookGuid));
    waitForFuture(f);
    if (!tagLocalId.isEmpty() && checkFuture(f)) {
        m_notifier->notifyTagExpunged(
            std::move(tagLocalId), std::move(childTagLocalIds));
    }
    return f;
}

QFuture<quint32> SynchronousLocalStorage::resourceCount(
    NoteCountOptions options) const
{
    auto f = m_underlyingLocalStorage->resourceCount(options);
    waitForFuture(f);
    return f;
}

QFuture<quint32> SynchronousLocalStorage::resourceCountPerNoteLocalId(
    QString noteLocalId) const
{
    auto f = m_underlyingLocalStorage->resourceCountPerNoteLocalId(
        std::move(noteLocalId));
    waitForFuture(f);
    return f;
}

QFuture<void> SynchronousLocalStorage::putResource(
    qevercloud::Resource resource)
{
    auto f = m_underlyingLocalStorage->putResource(resource);
    waitForFuture(f);
    if (checkFuture(f)) {
        m_notifier->notifyResourcePut(std::move(resource));
    }
    return f;
}

QFuture<std::optional<qevercloud::Resource>>
SynchronousLocalStorage::findResourceByLocalId(
    QString resourceLocalId, FetchResourceOptions options) const
{
    auto f = m_underlyingLocalStorage->findResourceByLocalId(
        std::move(resourceLocalId), std::move(options));
    waitForFuture(f);
    return f;
}

QFuture<std::optional<qevercloud::Resource>>
SynchronousLocalStorage::findResourceByGuid(
    qevercloud::Guid resourceGuid, FetchResourceOptions options) const
{
    auto f = m_underlyingLocalStorage->findResourceByGuid(
        std::move(resourceGuid), std::move(options));
    waitForFuture(f);
    return f;
}

QFuture<void> SynchronousLocalStorage::expungeResourceByLocalId(
    QString resourceLocalId)
{
    auto f = m_underlyingLocalStorage->expungeResourceByLocalId(
        resourceLocalId);
    waitForFuture(f);
    if (checkFuture(f)) {
        m_notifier->notifyResourceExpunged(std::move(resourceLocalId));
    }
    return f;
}

QFuture<void> SynchronousLocalStorage::expungeResourceByGuid(
    qevercloud::Guid resourceGuid)
{
    QString resourceLocalId;
    {
        auto f = m_underlyingLocalStorage->findResourceByGuid(resourceGuid);
        waitForFuture(f);

        try {
            Q_ASSERT(f.resultCount() == 1);
            const auto resource = f.result();
            if (resource) {
                resourceLocalId = resource->localId();
            }
        }
        catch (...) {
        }
    }

    auto f = m_underlyingLocalStorage->expungeResourceByGuid(
        std::move(resourceGuid));
    waitForFuture(f);
    if (!resourceLocalId.isEmpty() && checkFuture(f)) {
        m_notifier->notifyResourceExpunged(std::move(resourceLocalId));
    }
    return f;
}

QFuture<quint32> SynchronousLocalStorage::savedSearchCount() const
{
    auto f = m_underlyingLocalStorage->savedSearchCount();
    waitForFuture(f);
    return f;
}

QFuture<void> SynchronousLocalStorage::putSavedSearch(
    qevercloud::SavedSearch search)
{
    auto f = m_underlyingLocalStorage->putSavedSearch(search);
    waitForFuture(f);
    if (checkFuture(f)) {
        m_notifier->notifySavedSearchPut(std::move(search));
    }
    return f;
}

QFuture<std::optional<qevercloud::SavedSearch>>
SynchronousLocalStorage::findSavedSearchByLocalId(
    QString savedSearchLocalId) const
{
    auto f = m_underlyingLocalStorage->findSavedSearchByLocalId(
        std::move(savedSearchLocalId));
    waitForFuture(f);
    return f;
}

QFuture<std::optional<qevercloud::SavedSearch>>
SynchronousLocalStorage::findSavedSearchByGuid(qevercloud::Guid guid) const
{
    auto f = m_underlyingLocalStorage->findSavedSearchByGuid(std::move(guid));
    waitForFuture(f);
    return f;
}

QFuture<std::optional<qevercloud::SavedSearch>>
SynchronousLocalStorage::findSavedSearchByName(QString name) const
{
    auto f = m_underlyingLocalStorage->findSavedSearchByName(std::move(name));
    waitForFuture(f);
    return f;
}

QFuture<QList<qevercloud::SavedSearch>> SynchronousLocalStorage::listSavedSearches(
    ListSavedSearchesOptions options) const
{
    auto f = m_underlyingLocalStorage->listSavedSearches(std::move(options));
    waitForFuture(f);
    return f;
}

QFuture<QSet<qevercloud::Guid>> SynchronousLocalStorage::listSavedSearchGuids(
    ListGuidsFilters filters) const
{
    auto f = m_underlyingLocalStorage->listSavedSearchGuids(std::move(filters));
    waitForFuture(f);
    return f;
}

QFuture<void> SynchronousLocalStorage::expungeSavedSearchByLocalId(
    QString savedSearchLocalId)
{
    auto f = m_underlyingLocalStorage->expungeSavedSearchByLocalId(
        savedSearchLocalId);
    waitForFuture(f);
    if (checkFuture(f)) {
        m_notifier->notifySavedSearchExpunged(std::move(savedSearchLocalId));
    }
    return f;
}

QFuture<void> SynchronousLocalStorage::expungeSavedSearchByGuid(
    qevercloud::Guid guid)
{
    QString savedSearchLocalId;
    {
        auto f = m_underlyingLocalStorage->findSavedSearchByGuid(guid);
        waitForFuture(f);

        try {
            Q_ASSERT(f.resultCount() == 1);
            const auto savedSearch = f.result();
            if (savedSearch) {
                savedSearchLocalId = savedSearch->localId();
            }
        }
        catch (...) {
        }
    }

    auto f =
        m_underlyingLocalStorage->expungeSavedSearchByGuid(std::move(guid));
    waitForFuture(f);
    if (!savedSearchLocalId.isEmpty() && checkFuture(f)) {
        m_notifier->notifySavedSearchExpunged(std::move(savedSearchLocalId));
    }
    return f;
}

QFuture<qint32> SynchronousLocalStorage::highestUpdateSequenceNumber(
    const HighestUsnOption option) const
{
    auto f = m_underlyingLocalStorage->highestUpdateSequenceNumber(option);
    waitForFuture(f);
    return f;
}

QFuture<qint32> SynchronousLocalStorage::highestUpdateSequenceNumber(
    qevercloud::Guid linkedNotebookGuid) const
{
    auto f = m_underlyingLocalStorage->highestUpdateSequenceNumber(
        std::move(linkedNotebookGuid));
    waitForFuture(f);
    return f;
}

local_storage::ILocalStorageNotifier * SynchronousLocalStorage::notifier() const noexcept
{
    return m_notifier;
}

} // namespace quentier
