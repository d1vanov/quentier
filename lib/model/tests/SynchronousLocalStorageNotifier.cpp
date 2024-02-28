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

#include "SynchronousLocalStorageNotifier.h"

namespace quentier {

void SynchronousLocalStorageNotifier::notifyUserPut(qevercloud::User user)
{
    Q_EMIT userPut(std::move(user));
}

void SynchronousLocalStorageNotifier::notifyUserExpunged(
    const qevercloud::UserID userId)
{
    Q_EMIT userExpunged(userId);
}

void SynchronousLocalStorageNotifier::notifyNotebookPut(
    qevercloud::Notebook notebook)
{
    Q_EMIT notebookPut(std::move(notebook));
}

void SynchronousLocalStorageNotifier::notifyNotebookExpunged(
    QString notebookLocalId)
{
    Q_EMIT notebookExpunged(std::move(notebookLocalId));
}

void SynchronousLocalStorageNotifier::notifyLinkedNotebookPut(
    qevercloud::LinkedNotebook linkedNotebook)
{
    Q_EMIT linkedNotebookPut(std::move(linkedNotebook));
}

void SynchronousLocalStorageNotifier::notifyLinkedNotebookExpunged(
    qevercloud::Guid linkedNotebookGuid)
{
    Q_EMIT linkedNotebookExpunged(std::move(linkedNotebookGuid));
}

void SynchronousLocalStorageNotifier::notifyNotePut(qevercloud::Note note)
{
    Q_EMIT notePut(std::move(note));
}

void SynchronousLocalStorageNotifier::notifyNoteUpdated(
    qevercloud::Note note,
    const local_storage::ILocalStorage::UpdateNoteOptions options)
{
    Q_EMIT noteUpdated(std::move(note), options);
}

void SynchronousLocalStorageNotifier::notifyNoteExpunged(QString noteLocalId)
{
    Q_EMIT noteExpunged(std::move(noteLocalId));
}

void SynchronousLocalStorageNotifier::notifyTagPut(qevercloud::Tag tag)
{
    Q_EMIT tagPut(std::move(tag));
}

void SynchronousLocalStorageNotifier::notifyTagExpunged(
    QString tagLocalId, QStringList expungedChildTagLocalIds)
{
    Q_EMIT tagExpunged(
        std::move(tagLocalId), std::move(expungedChildTagLocalIds));
}

void SynchronousLocalStorageNotifier::notifyResourcePut(
    qevercloud::Resource resource)
{
    Q_EMIT resourcePut(std::move(resource));
}

void SynchronousLocalStorageNotifier::notifyResourceMetadataPut(
    qevercloud::Resource resource)
{
    Q_EMIT resourceMetadataPut(std::move(resource));
}

void SynchronousLocalStorageNotifier::notifyResourceExpunged(
    QString resourceLocalId)
{
    Q_EMIT resourceExpunged(std::move(resourceLocalId));
}

void SynchronousLocalStorageNotifier::notifySavedSearchPut(
    qevercloud::SavedSearch savedSearch)
{
    Q_EMIT savedSearchPut(std::move(savedSearch));
}

void SynchronousLocalStorageNotifier::notifySavedSearchExpunged(
    QString savedSearchLocalId)
{
    Q_EMIT savedSearchExpunged(std::move(savedSearchLocalId));
}

} // namespace quentier
