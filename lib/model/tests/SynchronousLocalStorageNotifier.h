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

#include <quentier/local_storage/ILocalStorageNotifier.h>

namespace quentier {

class SynchronousLocalStorageNotifier :
    public local_storage::ILocalStorageNotifier
{
    Q_OBJECT
public:
    void notifyUserPut(qevercloud::User user);
    void notifyUserExpunged(qevercloud::UserID userId);

    void notifyNotebookPut(qevercloud::Notebook notebook);
    void notifyNotebookExpunged(QString notebookLocalId);

    void notifyLinkedNotebookPut(qevercloud::LinkedNotebook linkedNotebook);
    void notifyLinkedNotebookExpunged(qevercloud::Guid linkedNotebookGuid);

    void notifyNotePut(qevercloud::Note note);
    void notifyNoteUpdated(
        qevercloud::Note note,
        local_storage::ILocalStorage::UpdateNoteOptions options);

    void notifyNoteExpunged(QString noteLocalId);

    void notifyTagPut(qevercloud::Tag tag);
    void notifyTagExpunged(
        QString tagLocalId, QStringList expungedChildTagLocalIds);

    void notifyResourcePut(qevercloud::Resource resource);
    void notifyResourceMetadataPut(qevercloud::Resource resource);
    void notifyResourceExpunged(QString resourceLocalId);

    void notifySavedSearchPut(qevercloud::SavedSearch savedSearch);
    void notifySavedSearchExpunged(QString savedSearchLocalId);
};

} // namespace quentier
