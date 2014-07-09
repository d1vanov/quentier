#include "LocalStorageManager.h"
#include "LocalStorageManager_p.h"

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

bool LocalStorageManager::AddUser(const IUser & user, QString & errorDescription)
{
    Q_D(LocalStorageManager);
    return d->AddUser(user, errorDescription);
}

bool LocalStorageManager::UpdateUser(const IUser & user, QString & errorDescription)
{
    Q_D(LocalStorageManager);
    return d->UpdateUser(user, errorDescription);
}

bool LocalStorageManager::FindUser(IUser & user, QString & errorDescription) const
{
    Q_D(const LocalStorageManager);
    return d->FindUser(user, errorDescription);
}

bool LocalStorageManager::DeleteUser(const IUser & user, QString & errorDescription)
{
    Q_D(LocalStorageManager);
    return d->DeleteUser(user, errorDescription);
}

bool LocalStorageManager::ExpungeUser(const IUser & user, QString & errorDescription)
{
    Q_D(LocalStorageManager);
    return d->ExpungeUser(user, errorDescription);
}

int LocalStorageManager::GetNotebookCount(QString & errorDescription) const
{
    Q_D(const LocalStorageManager);
    return d->GetNotebookCount(errorDescription);
}

void LocalStorageManager::SwitchUser(const QString & username, const UserID userId,
                                     const bool startFromScratch)
{
    Q_D(LocalStorageManager);
    return d->SwitchUser(username, userId, startFromScratch);
}

int LocalStorageManager::GetUserCount(QString & errorDescription) const
{
    Q_D(const LocalStorageManager);
    return d->GetUserCount(errorDescription);
}

bool LocalStorageManager::AddNotebook(const Notebook & notebook, QString & errorDescription)
{
    Q_D(LocalStorageManager);
    return d->AddNotebook(notebook, errorDescription);
}

bool LocalStorageManager::UpdateNotebook(const Notebook & notebook, QString & errorDescription)
{
    Q_D(LocalStorageManager);
    return d->UpdateNotebook(notebook, errorDescription);
}

bool LocalStorageManager::FindNotebook(Notebook & notebook, QString & errorDescription) const
{
    Q_D(const LocalStorageManager);
    return d->FindNotebook(notebook, errorDescription);
}

bool LocalStorageManager::FindDefaultNotebook(Notebook & notebook, QString & errorDescription) const
{
    Q_D(const LocalStorageManager);
    return d->FindDefaultNotebook(notebook, errorDescription);
}

bool LocalStorageManager::FindLastUsedNotebook(Notebook & notebook, QString & errorDescription) const
{
    Q_D(const LocalStorageManager);
    return d->FindLastUsedNotebook(notebook, errorDescription);
}

bool LocalStorageManager::FindDefaultOrLastUsedNotebook(Notebook & notebook, QString & errorDescription) const
{
    Q_D(const LocalStorageManager);
    return d->FindDefaultOrLastUsedNotebook(notebook, errorDescription);
}

QList<Notebook> LocalStorageManager::ListAllNotebooks(QString & errorDescription) const
{
    Q_D(const LocalStorageManager);
    return d->ListAllNotebooks(errorDescription);
}

QList<SharedNotebookWrapper> LocalStorageManager::ListAllSharedNotebooks(QString & errorDescription) const
{
    Q_D(const LocalStorageManager);
    return d->ListAllSharedNotebooks(errorDescription);
}

QList<SharedNotebookWrapper> LocalStorageManager::ListSharedNotebooksPerNotebookGuid(const QString & notebookGuid,
                                                                                     QString & errorDescription) const
{
    Q_D(const LocalStorageManager);
    return d->ListSharedNotebooksPerNotebookGuid(notebookGuid, errorDescription);
}

bool LocalStorageManager::ExpungeNotebook(const Notebook & notebook, QString & errorDescription)
{
    Q_D(LocalStorageManager);
    return d->ExpungeNotebook(notebook, errorDescription);
}

int LocalStorageManager::GetLinkedNotebookCount(QString & errorDescription) const
{
    Q_D(const LocalStorageManager);
    return d->GetLinkedNotebookCount(errorDescription);
}

bool LocalStorageManager::AddLinkedNotebook(const LinkedNotebook & linkedNotebook,
                                            QString & errorDescription)
{
    Q_D(LocalStorageManager);
    return d->AddLinkedNotebook(linkedNotebook, errorDescription);
}

bool LocalStorageManager::UpdateLinkedNotebook(const LinkedNotebook & linkedNotebook,
                                               QString & errorDescription)
{
    Q_D(LocalStorageManager);
    return d->UpdateLinkedNotebook(linkedNotebook, errorDescription);
}

bool LocalStorageManager::FindLinkedNotebook(LinkedNotebook & linkedNotebook, QString & errorDescription) const
{
    Q_D(const LocalStorageManager);
    return d->FindLinkedNotebook(linkedNotebook, errorDescription);
}

QList<LinkedNotebook> LocalStorageManager::ListAllLinkedNotebooks(QString & errorDescription) const
{
    Q_D(const LocalStorageManager);
    return d->ListAllLinkedNotebooks(errorDescription);
}

bool LocalStorageManager::ExpungeLinkedNotebook(const LinkedNotebook & linkedNotebook,
                                                QString & errorDescription)
{
    Q_D(LocalStorageManager);
    return d->ExpungeLinkedNotebook(linkedNotebook, errorDescription);
}

int LocalStorageManager::GetNoteCount(QString & errorDescription) const
{
    Q_D(const LocalStorageManager);
    return d->GetNoteCount(errorDescription);
}

bool LocalStorageManager::AddNote(const Note & note, const Notebook & notebook, QString & errorDescription)
{
    Q_D(LocalStorageManager);
    return d->AddNote(note, notebook, errorDescription);
}

bool LocalStorageManager::UpdateNote(const Note & note, const Notebook & notebook, QString & errorDescription)
{
    Q_D(LocalStorageManager);
    return d->UpdateNote(note, notebook, errorDescription);
}

bool LocalStorageManager::FindNote(Note & note, QString & errorDescription,
                                   const bool withResourceBinaryData) const
{
    Q_D(const LocalStorageManager);
    return d->FindNote(note, errorDescription, withResourceBinaryData);
}

QList<Note> LocalStorageManager::ListAllNotesPerNotebook(const Notebook & notebook,
                                                         QString & errorDescription,
                                                         const bool withResourceBinaryData) const
{
    Q_D(const LocalStorageManager);
    return d->ListAllNotesPerNotebook(notebook, errorDescription, withResourceBinaryData);
}

bool LocalStorageManager::DeleteNote(const Note & note, QString & errorDescription)
{
    Q_D(LocalStorageManager);
    return d->DeleteNote(note, errorDescription);
}

bool LocalStorageManager::ExpungeNote(const Note & note, QString & errorDescription)
{
    Q_D(LocalStorageManager);
    return d->ExpungeNote(note, errorDescription);
}

int LocalStorageManager::GetTagCount(QString & errorDescription) const
{
    Q_D(const LocalStorageManager);
    return d->GetTagCount(errorDescription);
}

bool LocalStorageManager::AddTag(const Tag & tag, QString & errorDescription)
{
    Q_D(LocalStorageManager);
    return d->AddTag(tag, errorDescription);
}

bool LocalStorageManager::UpdateTag(const Tag & tag, QString & errorDescription)
{
    Q_D(LocalStorageManager);
    return d->UpdateTag(tag, errorDescription);
}

bool LocalStorageManager::LinkTagWithNote(const Tag & tag, const Note & note,
                                          QString & errorDescription)
{
    Q_D(LocalStorageManager);
    return d->LinkTagWithNote(tag, note, errorDescription);
}

bool LocalStorageManager::FindTag(Tag & tag, QString & errorDescription) const
{
    Q_D(const LocalStorageManager);
    return d->FindTag(tag, errorDescription);
}

QList<Tag> LocalStorageManager::ListAllTagsPerNote(const Note & note, QString & errorDescription) const
{
    Q_D(const LocalStorageManager);
    return d->ListAllTagsPerNote(note, errorDescription);
}

QList<Tag> LocalStorageManager::ListAllTags(QString & errorDescription) const
{
    Q_D(const LocalStorageManager);
    return d->ListAllTags(errorDescription);
}

bool LocalStorageManager::DeleteTag(const Tag & tag, QString & errorDescription)
{
    Q_D(LocalStorageManager);
    return d->DeleteTag(tag, errorDescription);
}

bool LocalStorageManager::ExpungeTag(const Tag & tag, QString & errorDescription)
{
    Q_D(LocalStorageManager);
    return d->ExpungeTag(tag, errorDescription);
}

int LocalStorageManager::GetEnResourceCount(QString & errorDescription) const
{
    Q_D(const LocalStorageManager);
    return d->GetEnResourceCount(errorDescription);
}

bool LocalStorageManager::AddEnResource(const IResource & resource, const Note & note, QString & errorDescription)
{
    Q_D(LocalStorageManager);
    return d->AddEnResource(resource, note, errorDescription);
}

bool LocalStorageManager::UpdateEnResource(const IResource & resource, const Note &note, QString & errorDescription)
{
    Q_D(LocalStorageManager);
    return d->UpdateEnResource(resource, note, errorDescription);
}

bool LocalStorageManager::FindEnResource(IResource & resource, QString & errorDescription,
                                         const bool withBinaryData) const
{
    Q_D(const LocalStorageManager);
    return d->FindEnResource(resource, errorDescription, withBinaryData);
}

bool LocalStorageManager::ExpungeEnResource(const IResource & resource, QString & errorDescription)
{
    Q_D(LocalStorageManager);
    return d->ExpungeEnResource(resource, errorDescription);
}

int LocalStorageManager::GetSavedSearchCount(QString & errorDescription) const
{
    Q_D(const LocalStorageManager);
    return d->GetSavedSearchCount(errorDescription);
}

bool LocalStorageManager::AddSavedSearch(const SavedSearch & search, QString & errorDescription)
{
    Q_D(LocalStorageManager);
    return d->AddSavedSearch(search, errorDescription);
}

bool LocalStorageManager::UpdateSavedSearch(const SavedSearch & search,
                                            QString & errorDescription)
{
    Q_D(LocalStorageManager);
    return d->UpdateSavedSearch(search, errorDescription);
}

bool LocalStorageManager::FindSavedSearch(SavedSearch & search, QString & errorDescription) const
{
    Q_D(const LocalStorageManager);
    return d->FindSavedSearch(search, errorDescription);
}

QList<SavedSearch> LocalStorageManager::ListAllSavedSearches(QString & errorDescription) const
{
    Q_D(const LocalStorageManager);
    return d->ListAllSavedSearches(errorDescription);
}

bool LocalStorageManager::ExpungeSavedSearch(const SavedSearch & search,
                                             QString & errorDescription)
{
    Q_D(LocalStorageManager);
    return d->ExpungeSavedSearch(search, errorDescription);
}

} // namespace qute_note
