#ifndef __QUTE_NOTE__CLIENT__LOCAL_STORAGE__LOCAL_STORAGE_MANAGER_PRIVATE_H
#define __QUTE_NOTE__CLIENT__LOCAL_STORAGE__LOCAL_STORAGE_MANAGER_PRIVATE_H

#include <client/types/IUser.h>
#include <client/types/UserAdapter.h>
#include <client/types/UserWrapper.h>
#include <client/types/Notebook.h>
#include <client/types/ISharedNotebook.h>
#include <client/types/SharedNotebookAdapter.h>
#include <client/types/SharedNotebookWrapper.h>
#include <client/types/Note.h>
#include <client/types/Tag.h>
#include <client/types/IResource.h>
#include <client/types/ResourceAdapter.h>
#include <client/types/ResourceWrapper.h>
#include <client/types/LinkedNotebook.h>
#include <client/types/SavedSearch.h>
#include <QtSql>

namespace qute_note {

typedef qevercloud::UserID UserID;

class LocalStorageManagerPrivate
{
public:
    LocalStorageManagerPrivate(const QString & username, const UserID userId,
                               const bool startFromScratch);
    ~LocalStorageManagerPrivate();

    void SwitchUser(const QString & username, const UserID userId, const bool startFromScratch = false);
    int GetUserCount(QString & errorDescription) const;
    bool AddUser(const IUser & user, QString & errorDescription);
    bool UpdateUser(const IUser & user, QString & errorDescription);
    bool FindUser(IUser & user, QString & errorDescription) const;
    bool DeleteUser(const IUser & user, QString & errorDescription);
    bool ExpungeUser(const IUser & user, QString & errorDescription);

    int GetNotebookCount(QString & errorDescription) const;
    bool AddNotebook(const Notebook & notebook, QString & errorDescription);
    bool UpdateNotebook(const Notebook & notebook, QString & errorDescription);
    bool FindNotebook(Notebook & notebook, QString & errorDescription) const;
    bool FindDefaultNotebook(Notebook & notebook, QString & errorDescription) const;
    bool FindLastUsedNotebook(Notebook & notebook, QString & errorDescription) const;
    bool FindDefaultOrLastUsedNotebook(Notebook & notebook, QString & errorDescription) const;
    QList<Notebook> ListAllNotebooks(QString & errorDescription) const;
    QList<SharedNotebookWrapper> ListAllSharedNotebooks(QString & errorDescription) const;
    QList<SharedNotebookWrapper> ListSharedNotebooksPerNotebookGuid(const QString & notebookGuid,
                                                                    QString & errorDescription) const;
    bool ExpungeNotebook(const Notebook & notebook, QString & errorDescription);

    int GetLinkedNotebookCount(QString & errorDescription) const;
    bool AddLinkedNotebook(const LinkedNotebook & linkedNotebook, QString & errorDescription);
    bool UpdateLinkedNotebook(const LinkedNotebook & linkedNotebook, QString & errorDescription);
    bool FindLinkedNotebook(LinkedNotebook & linkedNotebook, QString & errorDescription) const;
    QList<LinkedNotebook> ListAllLinkedNotebooks(QString & errorDescription) const;
    bool ExpungeLinkedNotebook(const LinkedNotebook & linkedNotebook, QString & errorDescription);

    int GetNoteCount(QString & errorDescription) const;
    bool AddNote(const Note & note, const Notebook & notebook, QString & errorDescription);
    bool UpdateNote(const Note & note, const Notebook & notebook, QString & errorDescription);
    bool FindNote(Note & note, QString & errorDescription,
                  const bool withResourceBinaryData = true) const;
    QList<Note> ListAllNotesPerNotebook(const Notebook & notebook, QString & errorDescription,
                                        const bool withResourceBinaryData = true) const;
    bool DeleteNote(const Note & note, QString & errorDescription);
    bool ExpungeNote(const Note & note, QString & errorDescription);

    int GetTagCount(QString & errorDescription) const;
    bool AddTag(const Tag & tag, QString & errorDescription);
    bool UpdateTag(const Tag & tag, QString & errorDescription);
    bool LinkTagWithNote(const Tag & tag, const Note & note, QString & errorDescription);
    bool FindTag(Tag & tag, QString & errorDescription) const;
    QList<Tag> ListAllTagsPerNote(const Note & note, QString & errorDescription) const;
    QList<Tag> ListAllTags(QString & errorDescription) const;
    bool DeleteTag(const Tag & tag, QString & errorDescription);
    bool ExpungeTag(const Tag & tag, QString & errorDescription);

    int GetEnResourceCount(QString & errorDescription) const;
    bool AddEnResource(const IResource & resource, const Note & note, QString & errorDescription);
    bool UpdateEnResource(const IResource & resource, const Note & note, QString & errorDescription);
    bool FindEnResource(IResource & resource, QString & errorDescription, const bool withBinaryData = true) const;
    bool ExpungeEnResource(const IResource & resource, QString & errorDescription);

    int GetSavedSearchCount(QString & errorDescription) const;
    bool AddSavedSearch(const SavedSearch & search, QString & errorDescription);
    bool UpdateSavedSearch(const SavedSearch & search, QString & errorDescription);
    bool FindSavedSearch(SavedSearch & search, QString & errorDescription) const;
    QList<SavedSearch> ListAllSavedSearches(QString & errorDescription) const;
    bool ExpungeSavedSearch(const SavedSearch & search, QString & errorDescription);

private:
    LocalStorageManagerPrivate() = delete;
    LocalStorageManagerPrivate(const LocalStorageManagerPrivate & other) = delete;
    LocalStorageManagerPrivate & operator=(const LocalStorageManagerPrivate & other) = delete;

    bool CreateTables(QString & errorDescription);
    bool InsertOrReplaceNoteAttributes(const Note & note, const QString & overrideLocalGuid,
                                       QString & errorDescription);
    bool InsertOrReplaceNotebookAdditionalAttributes(const Notebook & notebook,
                                                     const QString & overrideLocalGuid,
                                                     QString & errorDescription);
    bool InsertOrReplaceNotebookRestrictions(const qevercloud::NotebookRestrictions & notebookRestrictions,
                                             const QString & localGuid, QString & errorDescription);
    bool SetSharedNotebookAttributes(const ISharedNotebook &sharedNotebook,
                                     QString & errorDescription);

    bool RowExists(const QString & tableName, const QString & uniqueKeyName,
                   const QVariant & uniqueKeyValue) const;

    bool InsertOrReplaceUser(const IUser & user, QString & errorDescription);
    bool InsertOrReplaceBusinesUserInfo(const UserID id, const qevercloud::BusinessUserInfo & info,
                                        QString & errorDescription);
    bool InsertOrReplacePremiumInfo(const UserID id, const qevercloud::PremiumInfo & info,
                                    QString & errorDescription);
    bool InsertOrReplaceAccounting(const UserID id, const qevercloud::Accounting & accounting,
                                   QString & errorDescription);
    bool InsertOrReplaceUserAttributes(const UserID id, const qevercloud::UserAttributes & attributes,
                                       QString & errorDescription);
    bool InsertOrReplaceNotebook(const Notebook & notebook, const QString & overrideLocalGuid, QString & errorDescription);
    bool InsertOrReplaceLinkedNotebook(const LinkedNotebook & linkedNotebook, QString & errorDescription);
    bool InsertOrReplaceNote(const Note & note, const Notebook & notebook,
                             const QString & overrideLocalGuid, QString & errorDescription);
    bool InsertOrReplaceTag(const Tag & tag, const QString & overrideLocalGuid, QString & errorDescription);
    bool InsertOrReplaceResource(const IResource & resource, const QString overrideResourceLocalGuid,
                                 const Note & note, const QString & overrideNoteLocalGuid, QString & errorDescription);
    bool InsertOrReplaceResourceAttributes(const QString & localGuid, const qevercloud::ResourceAttributes & attributes,
                                           QString & errorDescription);
    bool InsertOrReplaceSavedSearch(const SavedSearch & search, const QString & overrideLocalGuid, QString & errorDescription);

    void FillResourceFromSqlRecord(const QSqlRecord & rec, const bool withBinaryData, IResource & resource) const;
    bool FillResourceAttributesFromSqlRecord(const QSqlRecord & rec, qevercloud::ResourceAttributes & attributes) const;
    bool FillResourceAttributesApplicationDataKeysOnlyFromSqlRecord(const QSqlRecord & rec, qevercloud::ResourceAttributes & attributes) const;
    bool FillResourceAttributesApplicationDataFullMapFromSqlRecord(const QSqlRecord & rec, qevercloud::ResourceAttributes & attributes) const;
    bool FillNoteAttributesFromSqlRecord(const QSqlRecord & rec, qevercloud::NoteAttributes & attributes) const;
    bool FillNoteAttributesApplicationDataKeysOnlyFromSqlRecord(const QSqlRecord & rec, qevercloud::NoteAttributes & attributes) const;
    bool FillNoteAttributesApplicationDataFullMapFromSqlRecord(const QSqlRecord & rec, qevercloud::NoteAttributes & attributes) const;
    bool FillNoteAttributesClassificationsFromSqlRecord(const QSqlRecord & rec, qevercloud::NoteAttributes & attributes) const;
    bool FillUserFromSqlRecord(const QSqlRecord & rec, IUser & user, QString &errorDescription) const;
    void FillNoteFromSqlRecord(const QSqlRecord & record, Note & note) const;
    bool FillNotebookFromSqlRecord(const QSqlRecord & record, Notebook & notebook, QString & errorDescription) const;
    bool FillSharedNotebookFromSqlRecord(const QSqlRecord & record, ISharedNotebook & sharedNotebook,
                                         QString & errorDescription) const;
    bool FillLinkedNotebookFromSqlRecord(const QSqlRecord & record, LinkedNotebook & linkedNotebook,
                                         QString & errorDescription) const;
    bool FillSavedSearchFromSqlRecord(const QSqlRecord & rec, SavedSearch & search,
                                      QString & errorDescription) const;
    bool FillTagFromSqlRecord(const QSqlRecord & rec, Tag & tag,
                              QString & errorDescription) const;
    QList<Tag> FillTagsFromSqlQuery(QSqlQuery & query, QString & errorDescription) const;

    bool FindAndSetTagGuidsPerNote(Note & note, QString & errorDescription) const;
    bool FindAndSetResourcesPerNote(Note & note, QString & errorDescription,
                                    const bool withBinaryData = true) const;
    bool FindAndSetNoteAttributesPerNote(Note & note, QString & errorDescription) const;

    void SortSharedNotebooks(Notebook & notebook) const;

    QList<qevercloud::SharedNotebook> ListEnSharedNotebooksPerNotebookGuid(const QString & notebookGuid,
                                                                           QString & errorDescription) const;

    QString             m_currentUsername;
    qevercloud::UserID  m_currentUserId;
    QString             m_applicationPersistenceStoragePath;
    QSqlDatabase        m_sqlDatabase;
};

} // namespace qute_note

#endif // __QUTE_NOTE__CLIENT__LOCAL_STORAGE__LOCAL_STORAGE_MANAGER_PRIVATE_H
