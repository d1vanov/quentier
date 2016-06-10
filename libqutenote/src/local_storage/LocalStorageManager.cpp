#include <qute_note/local_storage/LocalStorageManager.h>
#include "LocalStorageManager_p.h"
#include <qute_note/local_storage/NoteSearchQuery.h>

namespace qute_note {

#define QUTE_NOTE_DATABASE_NAME "qn.storage.sqlite"

LocalStorageManager::LocalStorageManager(const QString & username, const UserID userId,
                                         const bool startFromScratch, const bool overrideLock) :
    d_ptr(new LocalStorageManagerPrivate(username, userId, startFromScratch, overrideLock))
{
    QObject::connect(d_ptr.data(), QNSIGNAL(LocalStorageManagerPrivate,upgradeProgress,double),
                     this, QNSIGNAL(LocalStorageManager,upgradeProgress,double));
}

LocalStorageManager::~LocalStorageManager()
{}

bool LocalStorageManager::addUser(const IUser & user, QString & errorDescription)
{
    Q_D(LocalStorageManager);
    return d->addUser(user, errorDescription);
}

bool LocalStorageManager::updateUser(const IUser & user, QString & errorDescription)
{
    Q_D(LocalStorageManager);
    return d->updateUser(user, errorDescription);
}

bool LocalStorageManager::findUser(IUser & user, QString & errorDescription) const
{
    Q_D(const LocalStorageManager);
    return d->findUser(user, errorDescription);
}

bool LocalStorageManager::deleteUser(const IUser & user, QString & errorDescription)
{
    Q_D(LocalStorageManager);
    return d->deleteUser(user, errorDescription);
}

bool LocalStorageManager::expungeUser(const IUser & user, QString & errorDescription)
{
    Q_D(LocalStorageManager);
    return d->expungeUser(user, errorDescription);
}

int LocalStorageManager::notebookCount(QString & errorDescription) const
{
    Q_D(const LocalStorageManager);
    return d->notebookCount(errorDescription);
}

void LocalStorageManager::switchUser(const QString & username, const UserID userId,
                                     const bool startFromScratch, const bool overrideLock)
{
    Q_D(LocalStorageManager);
    return d->switchUser(username, userId, startFromScratch, overrideLock);
}

int LocalStorageManager::userCount(QString & errorDescription) const
{
    Q_D(const LocalStorageManager);
    return d->userCount(errorDescription);
}

bool LocalStorageManager::addNotebook(Notebook & notebook, QString & errorDescription)
{
    Q_D(LocalStorageManager);
    return d->addNotebook(notebook, errorDescription);
}

bool LocalStorageManager::updateNotebook(Notebook & notebook, QString & errorDescription)
{
    Q_D(LocalStorageManager);
    return d->updateNotebook(notebook, errorDescription);
}

bool LocalStorageManager::findNotebook(Notebook & notebook, QString & errorDescription) const
{
    Q_D(const LocalStorageManager);
    return d->findNotebook(notebook, errorDescription);
}

bool LocalStorageManager::findDefaultNotebook(Notebook & notebook, QString & errorDescription) const
{
    Q_D(const LocalStorageManager);
    return d->findDefaultNotebook(notebook, errorDescription);
}

bool LocalStorageManager::findLastUsedNotebook(Notebook & notebook, QString & errorDescription) const
{
    Q_D(const LocalStorageManager);
    return d->findLastUsedNotebook(notebook, errorDescription);
}

bool LocalStorageManager::findDefaultOrLastUsedNotebook(Notebook & notebook, QString & errorDescription) const
{
    Q_D(const LocalStorageManager);
    return d->findDefaultOrLastUsedNotebook(notebook, errorDescription);
}

QList<Notebook> LocalStorageManager::listAllNotebooks(QString & errorDescription,
                                                      const size_t limit, const size_t offset,
                                                      const ListNotebooksOrder::type order,
                                                      const OrderDirection::type orderDirection,
                                                      const QString & linkedNotebookGuid) const
{
    Q_D(const LocalStorageManager);
    return d->listAllNotebooks(errorDescription, limit, offset, order, orderDirection, linkedNotebookGuid);
}

QList<Notebook> LocalStorageManager::listNotebooks(const ListObjectsOptions flag, QString & errorDescription,
                                                   const size_t limit, const size_t offset,
                                                   const ListNotebooksOrder::type order,
                                                   const OrderDirection::type orderDirection,
                                                   const QString & linkedNotebookGuid) const
{
    Q_D(const LocalStorageManager);
    return d->listNotebooks(flag, errorDescription, limit, offset, order, orderDirection, linkedNotebookGuid);
}

QList<SharedNotebookWrapper> LocalStorageManager::listAllSharedNotebooks(QString & errorDescription) const
{
    Q_D(const LocalStorageManager);
    return d->listAllSharedNotebooks(errorDescription);
}

QList<SharedNotebookWrapper> LocalStorageManager::listSharedNotebooksPerNotebookGuid(const QString & notebookGuid,
                                                                                     QString & errorDescription) const
{
    Q_D(const LocalStorageManager);
    return d->listSharedNotebooksPerNotebookGuid(notebookGuid, errorDescription);
}

bool LocalStorageManager::expungeNotebook(Notebook & notebook, QString & errorDescription)
{
    Q_D(LocalStorageManager);
    return d->expungeNotebook(notebook, errorDescription);
}

int LocalStorageManager::linkedNotebookCount(QString & errorDescription) const
{
    Q_D(const LocalStorageManager);
    return d->linkedNotebookCount(errorDescription);
}

bool LocalStorageManager::addLinkedNotebook(const LinkedNotebook & linkedNotebook,
                                            QString & errorDescription)
{
    Q_D(LocalStorageManager);
    return d->addLinkedNotebook(linkedNotebook, errorDescription);
}

bool LocalStorageManager::updateLinkedNotebook(const LinkedNotebook & linkedNotebook,
                                               QString & errorDescription)
{
    Q_D(LocalStorageManager);
    return d->updateLinkedNotebook(linkedNotebook, errorDescription);
}

bool LocalStorageManager::findLinkedNotebook(LinkedNotebook & linkedNotebook, QString & errorDescription) const
{
    Q_D(const LocalStorageManager);
    return d->findLinkedNotebook(linkedNotebook, errorDescription);
}

QList<LinkedNotebook> LocalStorageManager::listAllLinkedNotebooks(QString & errorDescription, const size_t limit,
                                                                  const size_t offset, const ListLinkedNotebooksOrder::type order,
                                                                  const OrderDirection::type orderDirection) const
{
    Q_D(const LocalStorageManager);
    return d->listAllLinkedNotebooks(errorDescription, limit, offset, order, orderDirection);
}

QList<LinkedNotebook> LocalStorageManager::listLinkedNotebooks(const ListObjectsOptions flag, QString & errorDescription, const size_t limit,
                                                               const size_t offset, const ListLinkedNotebooksOrder::type order,
                                                               const OrderDirection::type orderDirection) const
{
    Q_D(const LocalStorageManager);
    return d->listLinkedNotebooks(flag, errorDescription, limit, offset, order, orderDirection);
}

bool LocalStorageManager::expungeLinkedNotebook(const LinkedNotebook & linkedNotebook,
                                                QString & errorDescription)
{
    Q_D(LocalStorageManager);
    return d->expungeLinkedNotebook(linkedNotebook, errorDescription);
}

int LocalStorageManager::noteCount(QString & errorDescription) const
{
    Q_D(const LocalStorageManager);
    return d->noteCount(errorDescription);
}

int LocalStorageManager::noteCountPerNotebook(const Notebook & notebook, QString & errorDescription) const
{
    Q_D(const LocalStorageManager);
    return d->noteCountPerNotebook(notebook, errorDescription);
}

int LocalStorageManager::noteCountPerTag(const Tag & tag, QString & errorDescription) const
{
    Q_D(const LocalStorageManager);
    return d->noteCountPerTag(tag, errorDescription);
}

bool LocalStorageManager::addNote(Note & note, QString & errorDescription)
{
    Q_D(LocalStorageManager);
    return d->addNote(note, errorDescription);
}

bool LocalStorageManager::updateNote(Note & note, const bool updateResources,
                                     const bool updateTags, QString & errorDescription)
{
    Q_D(LocalStorageManager);
    return d->updateNote(note, updateResources, updateTags, errorDescription);
}

bool LocalStorageManager::findNote(Note & note, QString & errorDescription,
                                   const bool withResourceBinaryData) const
{
    Q_D(const LocalStorageManager);
    return d->findNote(note, errorDescription, withResourceBinaryData);
}

QList<Note> LocalStorageManager::listNotesPerNotebook(const Notebook & notebook,
                                                      QString & errorDescription,
                                                      const bool withResourceBinaryData,
                                                      const LocalStorageManager::ListObjectsOptions & flag,
                                                      const size_t limit, const size_t offset,
                                                      const LocalStorageManager::ListNotesOrder::type & order,
                                                      const LocalStorageManager::OrderDirection::type & orderDirection) const
{
    Q_D(const LocalStorageManager);
    return d->listNotesPerNotebook(notebook, errorDescription, withResourceBinaryData,
                                   flag, limit, offset, order, orderDirection);
}

QList<Note> LocalStorageManager::listNotesPerTag(const Tag & tag, QString & errorDescription,
                                                 const bool withResourceBinaryData,
                                                 const LocalStorageManager::ListObjectsOptions & flag,
                                                 const size_t limit, const size_t offset,
                                                 const LocalStorageManager::ListNotesOrder::type & order,
                                                 const LocalStorageManager::OrderDirection::type & orderDirection) const
{
    Q_D(const LocalStorageManager);
    return d->listNotesPerTag(tag, errorDescription, withResourceBinaryData, flag, limit, offset, order, orderDirection);
}

QList<Note> LocalStorageManager::listNotes(const ListObjectsOptions flag, QString & errorDescription,
                                           const bool withResourceBinaryData, const size_t limit,
                                           const size_t offset, const ListNotesOrder::type order,
                                           const OrderDirection::type orderDirection) const
{
    Q_D(const LocalStorageManager);
    return d->listNotes(flag, errorDescription, withResourceBinaryData, limit, offset, order, orderDirection);
}

NoteList LocalStorageManager::findNotesWithSearchQuery(const NoteSearchQuery & noteSearchQuery,
                                                       QString & errorDescription,
                                                       const bool withResourceBinaryData) const
{
    Q_D(const LocalStorageManager);
    return d->findNotesWithSearchQuery(noteSearchQuery, errorDescription, withResourceBinaryData);
}

bool LocalStorageManager::expungeNote(Note & note, QString & errorDescription)
{
    Q_D(LocalStorageManager);
    return d->expungeNote(note, errorDescription);
}

int LocalStorageManager::tagCount(QString & errorDescription) const
{
    Q_D(const LocalStorageManager);
    return d->tagCount(errorDescription);
}

bool LocalStorageManager::addTag(Tag & tag, QString & errorDescription)
{
    Q_D(LocalStorageManager);
    return d->addTag(tag, errorDescription);
}

bool LocalStorageManager::updateTag(Tag & tag, QString & errorDescription)
{
    Q_D(LocalStorageManager);
    return d->updateTag(tag, errorDescription);
}

bool LocalStorageManager::findTag(Tag & tag, QString & errorDescription) const
{
    Q_D(const LocalStorageManager);
    return d->findTag(tag, errorDescription);
}

QList<Tag> LocalStorageManager::listAllTagsPerNote(const Note & note, QString & errorDescription,
                                                   const ListObjectsOptions & flag, const size_t limit,
                                                   const size_t offset, const ListTagsOrder::type & order,
                                                   const OrderDirection::type & orderDirection) const
{
    Q_D(const LocalStorageManager);
    return d->listAllTagsPerNote(note, errorDescription, flag, limit, offset, order, orderDirection);
}

QList<Tag> LocalStorageManager::listAllTags(QString & errorDescription, const size_t limit,
                                            const size_t offset, const ListTagsOrder::type order,
                                            const OrderDirection::type orderDirection,
                                            const QString & linkedNotebookGuid) const
{
    Q_D(const LocalStorageManager);
    return d->listAllTags(errorDescription, limit, offset, order, orderDirection, linkedNotebookGuid);
}

QList<Tag> LocalStorageManager::listTags(const ListObjectsOptions flag, QString & errorDescription,
                                         const size_t limit, const size_t offset,
                                         const ListTagsOrder::type & order,
                                         const OrderDirection::type orderDirection,
                                         const QString & linkedNotebookGuid) const
{
    Q_D(const LocalStorageManager);
    return d->listTags(flag, errorDescription, limit, offset, order, orderDirection, linkedNotebookGuid);
}

bool LocalStorageManager::expungeTag(Tag & tag, QString & errorDescription)
{
    Q_D(LocalStorageManager);
    return d->expungeTag(tag, errorDescription);
}

bool LocalStorageManager::expungeNotelessTagsFromLinkedNotebooks(QString & errorDescription)
{
    Q_D(LocalStorageManager);
    return d->expungeNotelessTagsFromLinkedNotebooks(errorDescription);
}

int LocalStorageManager::enResourceCount(QString & errorDescription) const
{
    Q_D(const LocalStorageManager);
    return d->enResourceCount(errorDescription);
}

bool LocalStorageManager::addEnResource(IResource & resource, QString & errorDescription)
{
    Q_D(LocalStorageManager);
    return d->addEnResource(resource, errorDescription);
}

bool LocalStorageManager::updateEnResource(IResource & resource, QString & errorDescription)
{
    Q_D(LocalStorageManager);
    return d->updateEnResource(resource, errorDescription);
}

bool LocalStorageManager::findEnResource(IResource & resource, QString & errorDescription,
                                         const bool withBinaryData) const
{
    Q_D(const LocalStorageManager);
    return d->findEnResource(resource, errorDescription, withBinaryData);
}

bool LocalStorageManager::expungeEnResource(IResource & resource, QString & errorDescription)
{
    Q_D(LocalStorageManager);
    return d->expungeEnResource(resource, errorDescription);
}

int LocalStorageManager::savedSearchCount(QString & errorDescription) const
{
    Q_D(const LocalStorageManager);
    return d->savedSearchCount(errorDescription);
}

bool LocalStorageManager::addSavedSearch(SavedSearch & search, QString & errorDescription)
{
    Q_D(LocalStorageManager);
    return d->addSavedSearch(search, errorDescription);
}

bool LocalStorageManager::updateSavedSearch(SavedSearch & search, QString & errorDescription)
{
    Q_D(LocalStorageManager);
    return d->updateSavedSearch(search, errorDescription);
}

bool LocalStorageManager::findSavedSearch(SavedSearch & search, QString & errorDescription) const
{
    Q_D(const LocalStorageManager);
    return d->findSavedSearch(search, errorDescription);
}

QList<SavedSearch> LocalStorageManager::listAllSavedSearches(QString & errorDescription, const size_t limit, const size_t offset,
                                                             const ListSavedSearchesOrder::type order,
                                                             const OrderDirection::type orderDirection) const
{
    Q_D(const LocalStorageManager);
    return d->listAllSavedSearches(errorDescription, limit, offset, order, orderDirection);
}

QList<SavedSearch> LocalStorageManager::listSavedSearches(const ListObjectsOptions flag, QString & errorDescription,
                                                          const size_t limit, const size_t offset,
                                                          const ListSavedSearchesOrder::type order,
                                                          const OrderDirection::type orderDirection) const
{
    Q_D(const LocalStorageManager);
    return d->listSavedSearches(flag, errorDescription, limit, offset, order, orderDirection);
}

bool LocalStorageManager::expungeSavedSearch(SavedSearch & search, QString & errorDescription)
{
    Q_D(LocalStorageManager);
    return d->expungeSavedSearch(search, errorDescription);
}

} // namespace qute_note
