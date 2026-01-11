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

#pragma once

#include <quentier/local_storage/Fwd.h>
#include <quentier/local_storage/ILocalStorage.h>

namespace quentier {

class SynchronousLocalStorageNotifier;

class SynchronousLocalStorage : public local_storage::ILocalStorage
{
public:
    explicit SynchronousLocalStorage(
        local_storage::ILocalStoragePtr underlyingLocalStorage);

    ~SynchronousLocalStorage() override;

public: // local_storage::ILocalStorage
    // Versions/upgrade API
    [[nodiscard]] QFuture<bool> isVersionTooHigh() const override;
    [[nodiscard]] QFuture<bool> requiresUpgrade() const override;
    [[nodiscard]] QFuture<QList<local_storage::IPatchPtr>> requiredPatches()
        const override;

    [[nodiscard]] QFuture<qint32> version() const override;
    [[nodiscard]] QFuture<qint32> highestSupportedVersion() const override;

    // Users API
    [[nodiscard]] QFuture<quint32> userCount() const override;
    [[nodiscard]] QFuture<void> putUser(qevercloud::User user) override;

    [[nodiscard]] QFuture<std::optional<qevercloud::User>> findUserById(
        qevercloud::UserID userId) const override;

    [[nodiscard]] QFuture<void> expungeUserById(
        qevercloud::UserID userId) override;

    // Notebooks API
    [[nodiscard]] QFuture<quint32> notebookCount() const override;

    [[nodiscard]] QFuture<void> putNotebook(
        qevercloud::Notebook notebook) override;

    [[nodiscard]] QFuture<std::optional<qevercloud::Notebook>>
        findNotebookByLocalId(QString notebookLocalId) const override;

    [[nodiscard]] QFuture<std::optional<qevercloud::Notebook>>
        findNotebookByGuid(qevercloud::Guid guid) const override;

    [[nodiscard]] QFuture<std::optional<qevercloud::Notebook>>
        findNotebookByName(
            QString notebookName,
            std::optional<qevercloud::Guid> linkedNotebookGuid =
                std::nullopt) const override;

    [[nodiscard]] QFuture<std::optional<qevercloud::Notebook>>
        findDefaultNotebook() const override;

    [[nodiscard]] QFuture<void> expungeNotebookByLocalId(
        QString notebookLocalId) override;

    [[nodiscard]] QFuture<void> expungeNotebookByGuid(
        qevercloud::Guid notebookGuid) override;

    [[nodiscard]] QFuture<void> expungeNotebookByName(
        QString name,
        std::optional<qevercloud::Guid> linkedNotebookGuid =
            std::nullopt) override;

    [[nodiscard]] QFuture<QList<qevercloud::Notebook>> listNotebooks(
        ListNotebooksOptions options = {}) const override;

    [[nodiscard]] QFuture<QList<qevercloud::SharedNotebook>>
        listSharedNotebooks(qevercloud::Guid notebookGuid = {}) const override;

    [[nodiscard]] QFuture<QSet<qevercloud::Guid>> listNotebookGuids(
        ListGuidsFilters filters,
        std::optional<qevercloud::Guid> linkedNotebookGuid = {}) const override;

    // Linked notebooks API
    [[nodiscard]] QFuture<quint32> linkedNotebookCount() const override;

    [[nodiscard]] QFuture<void> putLinkedNotebook(
        qevercloud::LinkedNotebook linkedNotebook) override;

    [[nodiscard]] QFuture<std::optional<qevercloud::LinkedNotebook>>
        findLinkedNotebookByGuid(qevercloud::Guid guid) const override;

    [[nodiscard]] QFuture<void> expungeLinkedNotebookByGuid(
        qevercloud::Guid guid) override;

    [[nodiscard]] QFuture<QList<qevercloud::LinkedNotebook>>
        listLinkedNotebooks(
            ListLinkedNotebooksOptions options = {}) const override;

    // Notes API
    [[nodiscard]] QFuture<quint32> noteCount(
        NoteCountOptions options = NoteCountOptions(
            NoteCountOption::IncludeNonDeletedNotes)) const override;

    [[nodiscard]] QFuture<quint32> noteCountPerNotebookLocalId(
        QString notebookLocalId,
        NoteCountOptions options = NoteCountOptions(
            NoteCountOption::IncludeNonDeletedNotes)) const override;

    [[nodiscard]] QFuture<quint32> noteCountPerTagLocalId(
        QString tagLocalId,
        NoteCountOptions options = NoteCountOptions(
            NoteCountOption::IncludeNonDeletedNotes)) const override;

    [[nodiscard]] QFuture<QHash<QString, quint32>> noteCountsPerTags(
        ListTagsOptions listTagsOptions = {},
        NoteCountOptions options = NoteCountOptions(
            NoteCountOption::IncludeNonDeletedNotes)) const override;

    [[nodiscard]] QFuture<quint32> noteCountPerNotebookAndTagLocalIds(
        QStringList notebookLocalIds, QStringList tagLocalIds,
        NoteCountOptions options = NoteCountOptions(
            NoteCountOption::IncludeNonDeletedNotes)) const override;

    [[nodiscard]] QFuture<void> putNote(qevercloud::Note note) override;

    [[nodiscard]] QFuture<void> updateNote(
        qevercloud::Note note, UpdateNoteOptions options) override;

    [[nodiscard]] QFuture<std::optional<qevercloud::Note>> findNoteByLocalId(
        QString noteLocalId, FetchNoteOptions options) const override;

    [[nodiscard]] QFuture<std::optional<qevercloud::Note>> findNoteByGuid(
        qevercloud::Guid noteGuid, FetchNoteOptions options) const override;

    [[nodiscard]] QFuture<QList<qevercloud::Note>> listNotes(
        FetchNoteOptions fetchOptions,
        ListNotesOptions listOptions = {}) const override;

    [[nodiscard]] QFuture<QList<qevercloud::Note>> listNotesPerNotebookLocalId(
        QString notebookLocalId, FetchNoteOptions fetchOptions,
        ListNotesOptions listOptions = {}) const override;

    [[nodiscard]] QFuture<QList<qevercloud::Note>> listNotesPerTagLocalId(
        QString tagLocalId, FetchNoteOptions fetchOptions,
        ListNotesOptions listOptions = {}) const override;

    [[nodiscard]] QFuture<QList<qevercloud::Note>>
        listNotesPerNotebookAndTagLocalIds(
            QStringList notebookLocalIds, QStringList tagLocalIds,
            FetchNoteOptions fetchOptions,
            ListNotesOptions listOptions = {}) const override;

    [[nodiscard]] QFuture<QList<qevercloud::Note>> listNotesByLocalIds(
        QStringList noteLocalIds, FetchNoteOptions fetchOptions,
        ListNotesOptions listOptions = {}) const override;

    [[nodiscard]] QFuture<QSet<qevercloud::Guid>> listNoteGuids(
        ListGuidsFilters filters,
        std::optional<qevercloud::Guid> linkedNotebookGuid = {}) const override;

    [[nodiscard]] QFuture<QList<qevercloud::Note>> queryNotes(
        local_storage::NoteSearchQuery query,
        FetchNoteOptions fetchOptions) const override;

    [[nodiscard]] QFuture<QStringList> queryNoteLocalIds(
        local_storage::NoteSearchQuery query) const override;

    [[nodiscard]] QFuture<void> expungeNoteByLocalId(
        QString noteLocalId) override;

    [[nodiscard]] QFuture<void> expungeNoteByGuid(
        qevercloud::Guid noteGuid) override;

    // Tags API
    [[nodiscard]] QFuture<quint32> tagCount() const override;
    [[nodiscard]] QFuture<void> putTag(qevercloud::Tag tag) override;

    [[nodiscard]] QFuture<std::optional<qevercloud::Tag>> findTagByLocalId(
        QString tagLocalId) const override;

    [[nodiscard]] QFuture<std::optional<qevercloud::Tag>> findTagByGuid(
        qevercloud::Guid tagGuid) const override;

    [[nodiscard]] QFuture<std::optional<qevercloud::Tag>> findTagByName(
        QString tagName,
        std::optional<qevercloud::Guid> linkedNotebookGuid =
            std::nullopt) const override;

    [[nodiscard]] QFuture<QList<qevercloud::Tag>> listTags(
        ListTagsOptions options = {}) const override;

    [[nodiscard]] QFuture<QList<qevercloud::Tag>> listTagsPerNoteLocalId(
        QString noteLocalId, ListTagsOptions options = {}) const override;

    [[nodiscard]] QFuture<QSet<qevercloud::Guid>> listTagGuids(
        ListGuidsFilters filters,
        std::optional<qevercloud::Guid> linkedNotebookGuid = {}) const override;

    [[nodiscard]] QFuture<void> expungeTagByLocalId(
        QString tagLocalId) override;

    [[nodiscard]] QFuture<void> expungeTagByGuid(
        qevercloud::Guid tagGuid) override;

    [[nodiscard]] QFuture<void> expungeTagByName(
        QString name,
        std::optional<qevercloud::Guid> linkedNotebookGuid =
            std::nullopt) override;

    // Resources API
    [[nodiscard]] QFuture<quint32> resourceCount(
        NoteCountOptions options = NoteCountOptions(
            NoteCountOption::IncludeNonDeletedNotes)) const override;

    [[nodiscard]] QFuture<quint32> resourceCountPerNoteLocalId(
        QString noteLocalId) const override;

    [[nodiscard]] QFuture<void> putResource(
        qevercloud::Resource resource) override;

    [[nodiscard]] QFuture<std::optional<qevercloud::Resource>>
        findResourceByLocalId(
            QString resourceLocalId,
            FetchResourceOptions options = {}) const override;

    [[nodiscard]] QFuture<std::optional<qevercloud::Resource>>
        findResourceByGuid(
            qevercloud::Guid resourceGuid,
            FetchResourceOptions options = {}) const override;

    [[nodiscard]] QFuture<void> expungeResourceByLocalId(
        QString resourceLocalId) override;

    [[nodiscard]] QFuture<void> expungeResourceByGuid(
        qevercloud::Guid resourceGuid) override;

    // Saved searches API
    [[nodiscard]] QFuture<quint32> savedSearchCount() const override;

    [[nodiscard]] QFuture<void> putSavedSearch(
        qevercloud::SavedSearch search) override;

    [[nodiscard]] QFuture<std::optional<qevercloud::SavedSearch>>
        findSavedSearchByLocalId(QString savedSearchLocalId) const override;

    [[nodiscard]] QFuture<std::optional<qevercloud::SavedSearch>>
        findSavedSearchByGuid(qevercloud::Guid guid) const override;

    [[nodiscard]] QFuture<std::optional<qevercloud::SavedSearch>>
        findSavedSearchByName(QString name) const override;

    [[nodiscard]] QFuture<QList<qevercloud::SavedSearch>> listSavedSearches(
        ListSavedSearchesOptions options = {}) const override;

    [[nodiscard]] QFuture<QSet<qevercloud::Guid>> listSavedSearchGuids(
        ListGuidsFilters filters) const override;

    [[nodiscard]] QFuture<void> expungeSavedSearchByLocalId(
        QString savedSearchLocalId) override;

    [[nodiscard]] QFuture<void> expungeSavedSearchByGuid(
        qevercloud::Guid guid) override;

    // Synchronization API
    [[nodiscard]] QFuture<qint32> highestUpdateSequenceNumber(
        HighestUsnOption option) const override;

    [[nodiscard]] QFuture<qint32> highestUpdateSequenceNumber(
        qevercloud::Guid linkedNotebookGuid) const override;

    // Notifications about events occurring in local storage are done via
    // signals emitted by ILocalStorageNotifier.
    // ILocalStorageNotifier must be alive for at least as much as ILocalStorage
    // itself.
    [[nodiscard]] local_storage::ILocalStorageNotifier * notifier() const
        noexcept override;

private:
    const local_storage::ILocalStoragePtr m_underlyingLocalStorage;
    SynchronousLocalStorageNotifier * m_notifier;
};

} // namespace quentier
