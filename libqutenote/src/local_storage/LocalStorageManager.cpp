#include <qute_note/local_storage/LocalStorageManager.h>
#include "LocalStorageManager_p.h"
#include <qute_note/local_storage/NoteSearchQuery.h>

namespace qute_note {

#define QUTE_NOTE_DATABASE_NAME "qn.storage.sqlite"

LocalStorageManager::LocalStorageManager(const QString & username, const UserID userId,
                                         const bool startFromScratch) :
    d_ptr(new LocalStorageManagerPrivate(username, userId, startFromScratch))
{}

LocalStorageManager::~LocalStorageManager()
{
    if (d_ptr) {
        delete d_ptr;
    }
}

bool LocalStorageManager::addUser(const IUser & user, QString & errorDescription)
{
    Q_D(LocalStorageManager);
    return d->AddUser(user, errorDescription);
}

bool LocalStorageManager::updateUser(const IUser & user, QString & errorDescription)
{
    Q_D(LocalStorageManager);
    return d->UpdateUser(user, errorDescription);
}

bool LocalStorageManager::findUser(IUser & user, QString & errorDescription) const
{
    Q_D(const LocalStorageManager);
    return d->FindUser(user, errorDescription);
}

bool LocalStorageManager::deleteUser(const IUser & user, QString & errorDescription)
{
    Q_D(LocalStorageManager);
    return d->DeleteUser(user, errorDescription);
}

bool LocalStorageManager::expungeUser(const IUser & user, QString & errorDescription)
{
    Q_D(LocalStorageManager);
    return d->ExpungeUser(user, errorDescription);
}

int LocalStorageManager::notebookCount(QString & errorDescription) const
{
    Q_D(const LocalStorageManager);
    return d->GetNotebookCount(errorDescription);
}

void LocalStorageManager::switchUser(const QString & username, const UserID userId,
                                     const bool startFromScratch)
{
    Q_D(LocalStorageManager);
    return d->SwitchUser(username, userId, startFromScratch);
}

int LocalStorageManager::userCount(QString & errorDescription) const
{
    Q_D(const LocalStorageManager);
    return d->GetUserCount(errorDescription);
}

bool LocalStorageManager::addNotebook(const Notebook & notebook, QString & errorDescription)
{
    Q_D(LocalStorageManager);
    return d->AddNotebook(notebook, errorDescription);
}

bool LocalStorageManager::updateNotebook(const Notebook & notebook, QString & errorDescription)
{
    Q_D(LocalStorageManager);
    return d->UpdateNotebook(notebook, errorDescription);
}

bool LocalStorageManager::findNotebook(Notebook & notebook, QString & errorDescription) const
{
    Q_D(const LocalStorageManager);
    return d->FindNotebook(notebook, errorDescription);
}

bool LocalStorageManager::findDefaultNotebook(Notebook & notebook, QString & errorDescription) const
{
    Q_D(const LocalStorageManager);
    return d->FindDefaultNotebook(notebook, errorDescription);
}

bool LocalStorageManager::findLastUsedNotebook(Notebook & notebook, QString & errorDescription) const
{
    Q_D(const LocalStorageManager);
    return d->FindLastUsedNotebook(notebook, errorDescription);
}

bool LocalStorageManager::findDefaultOrLastUsedNotebook(Notebook & notebook, QString & errorDescription) const
{
    Q_D(const LocalStorageManager);
    return d->FindDefaultOrLastUsedNotebook(notebook, errorDescription);
}

QList<Notebook> LocalStorageManager::listAllNotebooks(QString & errorDescription,
                                                      const size_t limit, const size_t offset,
                                                      const ListNotebooksOrder::type order,
                                                      const OrderDirection::type orderDirection,
                                                      const QString & linkedNotebookGuid) const
{
    Q_D(const LocalStorageManager);
    return d->ListAllNotebooks(errorDescription, limit, offset, order, orderDirection, linkedNotebookGuid);
}

QList<Notebook> LocalStorageManager::listNotebooks(const ListObjectsOptions flag, QString & errorDescription,
                                                   const size_t limit, const size_t offset,
                                                   const ListNotebooksOrder::type order,
                                                   const OrderDirection::type orderDirection,
                                                   const QString & linkedNotebookGuid) const
{
    Q_D(const LocalStorageManager);
    return d->ListNotebooks(flag, errorDescription, limit, offset, order, orderDirection, linkedNotebookGuid);
}

QList<SharedNotebookWrapper> LocalStorageManager::listAllSharedNotebooks(QString & errorDescription) const
{
    Q_D(const LocalStorageManager);
    return d->ListAllSharedNotebooks(errorDescription);
}

QList<SharedNotebookWrapper> LocalStorageManager::listSharedNotebooksPerNotebookGuid(const QString & notebookGuid,
                                                                                     QString & errorDescription) const
{
    Q_D(const LocalStorageManager);
    return d->ListSharedNotebooksPerNotebookGuid(notebookGuid, errorDescription);
}

bool LocalStorageManager::expungeNotebook(const Notebook & notebook, QString & errorDescription)
{
    Q_D(LocalStorageManager);
    return d->ExpungeNotebook(notebook, errorDescription);
}

int LocalStorageManager::linkedNotebookCount(QString & errorDescription) const
{
    Q_D(const LocalStorageManager);
    return d->GetLinkedNotebookCount(errorDescription);
}

bool LocalStorageManager::addLinkedNotebook(const LinkedNotebook & linkedNotebook,
                                            QString & errorDescription)
{
    Q_D(LocalStorageManager);
    return d->AddLinkedNotebook(linkedNotebook, errorDescription);
}

bool LocalStorageManager::updateLinkedNotebook(const LinkedNotebook & linkedNotebook,
                                               QString & errorDescription)
{
    Q_D(LocalStorageManager);
    return d->UpdateLinkedNotebook(linkedNotebook, errorDescription);
}

bool LocalStorageManager::findLinkedNotebook(LinkedNotebook & linkedNotebook, QString & errorDescription) const
{
    Q_D(const LocalStorageManager);
    return d->FindLinkedNotebook(linkedNotebook, errorDescription);
}

QList<LinkedNotebook> LocalStorageManager::listAllLinkedNotebooks(QString & errorDescription, const size_t limit,
                                                                  const size_t offset, const ListLinkedNotebooksOrder::type order,
                                                                  const OrderDirection::type orderDirection) const
{
    Q_D(const LocalStorageManager);
    return d->ListAllLinkedNotebooks(errorDescription, limit, offset, order, orderDirection);
}

QList<LinkedNotebook> LocalStorageManager::listLinkedNotebooks(const ListObjectsOptions flag, QString & errorDescription, const size_t limit,
                                                               const size_t offset, const ListLinkedNotebooksOrder::type order,
                                                               const OrderDirection::type orderDirection) const
{
    Q_D(const LocalStorageManager);
    return d->ListLinkedNotebooks(flag, errorDescription, limit, offset, order, orderDirection);
}

bool LocalStorageManager::expungeLinkedNotebook(const LinkedNotebook & linkedNotebook,
                                                QString & errorDescription)
{
    Q_D(LocalStorageManager);
    return d->ExpungeLinkedNotebook(linkedNotebook, errorDescription);
}

int LocalStorageManager::noteCount(QString & errorDescription) const
{
    Q_D(const LocalStorageManager);
    return d->GetNoteCount(errorDescription);
}

bool LocalStorageManager::addNote(const Note & note, const Notebook & notebook, QString & errorDescription)
{
    Q_D(LocalStorageManager);
    return d->AddNote(note, notebook, errorDescription);
}

bool LocalStorageManager::updateNote(const Note & note, const Notebook & notebook, QString & errorDescription)
{
    Q_D(LocalStorageManager);
    return d->UpdateNote(note, notebook, errorDescription);
}

bool LocalStorageManager::findNote(Note & note, QString & errorDescription,
                                   const bool withResourceBinaryData) const
{
    Q_D(const LocalStorageManager);
    return d->FindNote(note, errorDescription, withResourceBinaryData);
}

QList<Note> LocalStorageManager::listAllNotesPerNotebook(const Notebook & notebook,
                                                         QString & errorDescription,
                                                         const bool withResourceBinaryData,
                                                         const LocalStorageManager::ListObjectsOptions & flag,
                                                         const size_t limit, const size_t offset,
                                                         const LocalStorageManager::ListNotesOrder::type & order,
                                                         const LocalStorageManager::OrderDirection::type & orderDirection) const
{
    Q_D(const LocalStorageManager);
    return d->ListAllNotesPerNotebook(notebook, errorDescription, withResourceBinaryData,
                                      flag, limit, offset, order, orderDirection);
}

QList<Note> LocalStorageManager::listNotes(const ListObjectsOptions flag, QString & errorDescription,
                                           const bool withResourceBinaryData, const size_t limit,
                                           const size_t offset, const ListNotesOrder::type order,
                                           const OrderDirection::type orderDirection) const
{
    Q_D(const LocalStorageManager);
    return d->ListNotes(flag, errorDescription, withResourceBinaryData, limit, offset, order, orderDirection);
}

NoteList LocalStorageManager::findNotesWithSearchQuery(const NoteSearchQuery & noteSearchQuery,
                                                       QString & errorDescription,
                                                       const bool withResourceBinaryData) const
{
    Q_D(const LocalStorageManager);
    return d->FindNotesWithSearchQuery(noteSearchQuery, errorDescription, withResourceBinaryData);
}

bool LocalStorageManager::deleteNote(const Note & note, QString & errorDescription)
{
    Q_D(LocalStorageManager);
    return d->DeleteNote(note, errorDescription);
}

bool LocalStorageManager::expungeNote(const Note & note, QString & errorDescription)
{
    Q_D(LocalStorageManager);
    return d->ExpungeNote(note, errorDescription);
}

int LocalStorageManager::tagCount(QString & errorDescription) const
{
    Q_D(const LocalStorageManager);
    return d->GetTagCount(errorDescription);
}

bool LocalStorageManager::addTag(const Tag & tag, QString & errorDescription)
{
    Q_D(LocalStorageManager);
    return d->AddTag(tag, errorDescription);
}

bool LocalStorageManager::updateTag(const Tag & tag, QString & errorDescription)
{
    Q_D(LocalStorageManager);
    return d->UpdateTag(tag, errorDescription);
}

bool LocalStorageManager::linkTagWithNote(const Tag & tag, const Note & note,
                                          QString & errorDescription)
{
    Q_D(LocalStorageManager);
    return d->LinkTagWithNote(tag, note, errorDescription);
}

bool LocalStorageManager::findTag(Tag & tag, QString & errorDescription) const
{
    Q_D(const LocalStorageManager);
    return d->FindTag(tag, errorDescription);
}

QList<Tag> LocalStorageManager::listAllTagsPerNote(const Note & note, QString & errorDescription,
                                                   const ListObjectsOptions & flag, const size_t limit,
                                                   const size_t offset, const ListTagsOrder::type & order,
                                                   const OrderDirection::type & orderDirection) const
{
    Q_D(const LocalStorageManager);
    return d->ListAllTagsPerNote(note, errorDescription, flag, limit, offset, order, orderDirection);
}

QList<Tag> LocalStorageManager::listAllTags(QString & errorDescription, const size_t limit,
                                            const size_t offset, const ListTagsOrder::type order,
                                            const OrderDirection::type orderDirection,
                                            const QString & linkedNotebookGuid) const
{
    Q_D(const LocalStorageManager);
    return d->ListAllTags(errorDescription, limit, offset, order, orderDirection, linkedNotebookGuid);
}

QList<Tag> LocalStorageManager::listTags(const ListObjectsOptions flag, QString & errorDescription,
                                         const size_t limit, const size_t offset,
                                         const ListTagsOrder::type & order,
                                         const OrderDirection::type orderDirection,
                                         const QString & linkedNotebookGuid) const
{
    Q_D(const LocalStorageManager);
    return d->ListTags(flag, errorDescription, limit, offset, order, orderDirection, linkedNotebookGuid);
}

bool LocalStorageManager::deleteTag(const Tag & tag, QString & errorDescription)
{
    Q_D(LocalStorageManager);
    return d->DeleteTag(tag, errorDescription);
}

bool LocalStorageManager::expungeTag(const Tag & tag, QString & errorDescription)
{
    Q_D(LocalStorageManager);
    return d->ExpungeTag(tag, errorDescription);
}

bool LocalStorageManager::expungeNotelessTagsFromLinkedNotebooks(QString & errorDescription)
{
    Q_D(LocalStorageManager);
    return d->ExpungeNotelessTagsFromLinkedNotebooks(errorDescription);
}

int LocalStorageManager::enResourceCount(QString & errorDescription) const
{
    Q_D(const LocalStorageManager);
    return d->GetEnResourceCount(errorDescription);
}

bool LocalStorageManager::addEnResource(const IResource & resource, const Note & note, QString & errorDescription)
{
    Q_D(LocalStorageManager);
    return d->AddEnResource(resource, note, errorDescription);
}

bool LocalStorageManager::updateEnResource(const IResource & resource, const Note &note, QString & errorDescription)
{
    Q_D(LocalStorageManager);
    return d->UpdateEnResource(resource, note, errorDescription);
}

bool LocalStorageManager::findEnResource(IResource & resource, QString & errorDescription,
                                         const bool withBinaryData) const
{
    Q_D(const LocalStorageManager);
    return d->FindEnResource(resource, errorDescription, withBinaryData);
}

bool LocalStorageManager::expungeEnResource(const IResource & resource, QString & errorDescription)
{
    Q_D(LocalStorageManager);
    return d->ExpungeEnResource(resource, errorDescription);
}

int LocalStorageManager::savedSearchCount(QString & errorDescription) const
{
    Q_D(const LocalStorageManager);
    return d->GetSavedSearchCount(errorDescription);
}

bool LocalStorageManager::addSavedSearch(const SavedSearch & search, QString & errorDescription)
{
    Q_D(LocalStorageManager);
    return d->AddSavedSearch(search, errorDescription);
}

bool LocalStorageManager::updateSavedSearch(const SavedSearch & search,
                                            QString & errorDescription)
{
    Q_D(LocalStorageManager);
    return d->UpdateSavedSearch(search, errorDescription);
}

bool LocalStorageManager::findSavedSearch(SavedSearch & search, QString & errorDescription) const
{
    Q_D(const LocalStorageManager);
    return d->FindSavedSearch(search, errorDescription);
}

QList<SavedSearch> LocalStorageManager::listAllSavedSearches(QString & errorDescription, const size_t limit, const size_t offset,
                                                             const ListSavedSearchesOrder::type order,
                                                             const OrderDirection::type orderDirection) const
{
    Q_D(const LocalStorageManager);
    return d->ListAllSavedSearches(errorDescription, limit, offset, order, orderDirection);
}

QList<SavedSearch> LocalStorageManager::listSavedSearches(const ListObjectsOptions flag, QString & errorDescription,
                                                          const size_t limit, const size_t offset,
                                                          const ListSavedSearchesOrder::type order,
                                                          const OrderDirection::type orderDirection) const
{
    Q_D(const LocalStorageManager);
    return d->ListSavedSearches(flag, errorDescription, limit, offset, order, orderDirection);
}

bool LocalStorageManager::expungeSavedSearch(const SavedSearch & search,
                                             QString & errorDescription)
{
    Q_D(LocalStorageManager);
    return d->ExpungeSavedSearch(search, errorDescription);
}

} // namespace qute_note
