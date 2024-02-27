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
#include "TestUtils.h"

#include <quentier/exception/InvalidArgument.h>

namespace quentier {

SynchronousLocalStorage::SynchronousLocalStorage(
    local_storage::ILocalStoragePtr underlyingLocalStorage) :
    m_underlyingLocalStorage{std::move(underlyingLocalStorage)}
{
    if (Q_UNLIKELY(!m_underlyingLocalStorage)) {
        throw InvalidArgument{ErrorString{
            "SynchronousLocalStorage ctor: underlying local storage is null"}};
    }
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
    auto f = m_underlyingLocalStorage->putUser(std::move(user));
    waitForFuture(f);
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
    qevercloud::UserID userId)
{
    auto f = m_underlyingLocalStorage->expungeUserById(userId);
    waitForFuture(f);
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
    auto f = m_underlyingLocalStorage->putNotebook(std::move(notebook));
    waitForFuture(f);
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
        std::move(notebookLocalId));
    waitForFuture(f);
    return f;
}

QFuture<void> SynchronousLocalStorage::expungeNotebookByGuid(
    qevercloud::Guid notebookGuid)
{
    auto f = m_underlyingLocalStorage->expungeNotebookByGuid(
        std::move(notebookGuid));
    waitForFuture(f);
    return f;
}

QFuture<void> SynchronousLocalStorage::expungeNotebookByName(
    QString name,
    std::optional<qevercloud::Guid> linkedNotebookGuid)
{
    auto f = m_underlyingLocalStorage->expungeNotebookByName(
        std::move(name), std::move(linkedNotebookGuid));
    waitForFuture(f);
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
    auto f =
        m_underlyingLocalStorage->putLinkedNotebook(std::move(linkedNotebook));
    waitForFuture(f);
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
    auto f =
        m_underlyingLocalStorage->expungeLinkedNotebookByGuid(std::move(guid));
    waitForFuture(f);
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
    auto f = m_underlyingLocalStorage->putNote(std::move(note));
    waitForFuture(f);
    return f;
}

QFuture<void> SynchronousLocalStorage::updateNote(
    qevercloud::Note note, const UpdateNoteOptions options)
{
    auto f = m_underlyingLocalStorage->updateNote(std::move(note), options);
    waitForFuture(f);
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

QFuture<std::optional<qevercloud::Note>> SynchronousLocalStorage::findNoteByGuid(
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

QFuture<QList<qevercloud::Note>> SynchronousLocalStorage::listNotesPerNotebookAndTagLocalIds(
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
    auto f =
        m_underlyingLocalStorage->expungeNoteByLocalId(std::move(noteLocalId));
    waitForFuture(f);
    return f;
}

QFuture<void> SynchronousLocalStorage::expungeNoteByGuid(
    qevercloud::Guid noteGuid)
{
    auto f = m_underlyingLocalStorage->expungeNoteByGuid(std::move(noteGuid));
    waitForFuture(f);
    return f;
}

} // namespace quentier
