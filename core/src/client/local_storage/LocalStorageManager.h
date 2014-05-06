#ifndef __QUTE_NOTE__EVERNOTE_CLIENT__LOCAL_STORAGE__LOCAL_STORAGE_MANAGER
#define __QUTE_NOTE__EVERNOTE_CLIENT__LOCAL_STORAGE__LOCAL_STORAGE_MANAGER

#include <QString>
#include <QtSql>
#include <QSharedPointer>

namespace qevercloud {
QT_FORWARD_DECLARE_STRUCT(UserAttributes)
QT_FORWARD_DECLARE_STRUCT(Accounting)
QT_FORWARD_DECLARE_STRUCT(PremiumInfo)
QT_FORWARD_DECLARE_STRUCT(BusinessUserInfo)
QT_FORWARD_DECLARE_STRUCT(SharedNotebook)
QT_FORWARD_DECLARE_STRUCT(NotebookRestrictions)
typedef int32_t UserID;
}

namespace qute_note {

QT_FORWARD_DECLARE_CLASS(ISharedNotebook)
QT_FORWARD_DECLARE_CLASS(SharedNotebookWrapper)
QT_FORWARD_DECLARE_CLASS(SharedNotebookAdapter)
QT_FORWARD_DECLARE_CLASS(LinkedNotebook)
QT_FORWARD_DECLARE_CLASS(Notebook)
QT_FORWARD_DECLARE_CLASS(Note)
QT_FORWARD_DECLARE_CLASS(Tag)
QT_FORWARD_DECLARE_CLASS(IResource)
QT_FORWARD_DECLARE_CLASS(SavedSearch)
QT_FORWARD_DECLARE_CLASS(IUser)
typedef qevercloud::UserID UserID;

struct WhichGuid {
    enum type {
        LocalGuid,
        EverCloudGuid
    };
};

class LocalStorageManager
{
public:
    LocalStorageManager(const QString & username, const UserID userId, const bool startFromScratch);
    ~LocalStorageManager();

    void SwitchUser(const QString & username, const UserID userId, const bool startFromScratch = false);

    bool AddUser(const IUser & user, QString & errorDescription);
    bool UpdateUser(const IUser & user, QString & errorDescription);
    bool FindUser(const UserID id, IUser & user, QString & errorDescription) const;
    bool FindUserAttributes(const UserID id, qevercloud::UserAttributes & attributes,
                            QString & errorDescription) const;
    bool FindAccounting(const UserID id, qevercloud::Accounting & accounting,
                        QString & errorDescription) const;
    bool FindPremiumInfo(const UserID id, qevercloud::PremiumInfo & info,
                         QString & errorDescription) const;
    bool FindBusinessUserInfo(const UserID id, qevercloud::BusinessUserInfo & info,
                              QString & errorDescription) const;

    /**
     * @brief DeleteUser - either expunges the local user (i.e. deletes it from
     * local storage completely, without the possibility to restore)
     * if it has not been synchronized with Evernote service yet or marks
     * user as deleted otherwise. Evernote API doesn't allow thirdparty
     * applications to expunge users which have ever been synchronized
     * with remote data store at least once
     * @param user - user to be deleted
     * @param errorDescription - error description if user could not be deleted
     * @return true if user was deleted successfully, false otherwise
     */
    bool DeleteUser(const IUser & user, QString & errorDescription);

    /**
     * @brief ExpungeUser - permanently deletes local user i.e. user which
     * has not yet been synchronized with Evernote service. This method deletes
     * user from local storage completely, without the possibility to restore
     * @param user - user to be expunged
     * @param errorDescription - error description if user could not be expunged
     * @return true if user was expunged successfully, false otherwise
     */
    bool ExpungeUser(const IUser & user, QString & errorDescription);

    /**
     * @brief AddNotebook - adds passed in Notebook to the local storage database;
     * if Notebook has "remote" Evernote service's guid set, it is identified by this guid
     * in local storage database. Otherwise it is identified by its local guid.
     * @param notebook - notebook to be added to the local storage database
     * @param errorDescription - error description if notebook could not be added
     * @return true if notebook was added successfully, false otherwise
     */
    bool AddNotebook(const Notebook & notebook, QString & errorDescription);

    /**
     * @brief UpdateNotebook - updates passed in Notebook in the local storage database;
     * if Notebook has "remote" Evernote service's guid set, it is identified by this guid
     * in local storage database. Otherwise it is identified by its local guid.
     * @param notebook - notebook to be updated in the local storage database
     * @param errorDescription - error description if notebook could not be updated
     * @return true if notebook was updated successfully, false otherwise
     */
    bool UpdateNotebook(const Notebook & notebook, QString & errorDescription);

    /**
     * @brief FindNotebook - attempts to find and set all found fields for passed in
     * by reference Notebook object. If "remote" Evernote service's guid for Notebook is set,
     * it is used to identify the Notebook in local storage database. Otherwise it is
     * identified by its local guid.
     * @param notebook - notebook to be found. Must have either "remote" or local guid set
     * @param errorDescription - error description if notebook could not be found
     * @return true if notebook was found, false otherwise
     */
    bool FindNotebook(Notebook & notebook, QString & errorDescription);

    QList<Notebook> ListAllNotebooks(QString & errorDescription) const;
    QList<SharedNotebookWrapper> ListAllSharedNotebooks(QString & errorDescription) const;

    QList<SharedNotebookWrapper> ListSharedNotebooksPerNotebookGuid(const QString & notebookGuid,
                                                                    QString & errorDescription) const;

    /**
     * @brief ExpungeNotebook - deletes specified notebook from local storage.
     * Evernote API doesn't allow to delete notebooks from remote storage, it can
     * only be done by official desktop client or web GUI. So this method should be called
     * only during synchronization with remote database, when some notebook is found to be
     * deleted via either official desktop client or web GUI
     * @param notebook - notebook to be expunged. Must have either "remote" or local guid set
     * @param errorDescription - error description if notebook could not be expunged
     * @return true if notebook was expunged successfully, false otherwise
     */
    bool ExpungeNotebook(const Notebook & notebook, QString & errorDescription);

    /**
     * @brief AddLinkedNotebook - adds passed in LinkedNotebook to the local storage database;
     * LinkedNotebook must have "remote" Evernote service's guid set. It is not possible
     * to add a linked notebook in offline mode so it doesn't make sense for LinkedNotebook
     * objects to not have guid.
     * @param linkedNotebook - LinkedNotebook to be added to the local storage database
     * @param errorDescription - error description if linked notebook could not be added
     * @return true if linked notebook was added successfully, false otherwise
     */
    bool AddLinkedNotebook(const LinkedNotebook & linkedNotebook, QString & errorDescription);

    /**
     * @brief UpdateLinkedNotebook - updates passd in LinkedNotebook in the local storage database;
     * LinkedNotebook must have "remote" Evernote service's guid set.
     * @param linkedNotebook - LinkedNotebook to be updated in the local storage database
     * @param errorDescription - error description if linked notebook could not be updated
     * @return true if linked notebook was updated successfully, false otherwise
     */
    bool UpdateLinkedNotebook(const LinkedNotebook & linkedNotebook, QString & errorDescription);

    bool FindLinkedNotebook(const QString & notebookGuid, LinkedNotebook & linkedNotebook,
                            QString & errorDescription) const;

    bool ListAllLinkedNotebooks(std::vector<LinkedNotebook> & notebooks, QString & errorDescription) const;

    bool ExpungeLinkedNotebook(const LinkedNotebook & linkedNotebook,
                               QString & errorDescription);

    bool AddNote(const Note & note, const Notebook & notebook, QString & errorDescription);
    bool UpdateNote(const Note & note, const Notebook & notebook, QString & errorDescription);

    bool FindNote(Note & note, QString & errorDescription,
                  const bool withResourceBinaryData = true) const;

    bool ListAllNotesPerNotebook(const QString & notebookGuid, std::vector<Note> & notes,
                                 QString & errorDescription, const bool withResourceBinaryData = true) const;


    /**
     * @brief DeleteNote - either expunges the local note (i.e. deletes it from
     * local storage completely, without the possibility to restore)
     * if it has not been synchronized with Evernote service yet or marks
     * the note as deleted otherwise. Evernote API doesn't allow thirdparty
     * applications to expunge notes which have ever been synchronized
     * with remote data store at least once
     * @param note - note to be deleted
     * @param errorDescription - error description if note could not be deleted
     * @return true if note was deleted successfully, false otherwise
     */
    bool DeleteNote(const Note & note, QString & errorDescription);

    /**
     * @brief ExpungeNote - permanently deletes local notes i.e. notes which
     * have not yet been synchronized with Evernote service. This method deletes
     * the note from local storage completely, without the possibility to restore it
     * @param note - note to be expunged
     * @param errorDescription - error description if note could not be expunged
     * @return true if note was expunged successfully, false otherwise
     */
    bool ExpungeNote(const Note & note, QString & errorDescription);

    /**
     * @brief AddTag - adds passed in Tag to the local storage database. If tag has
     * "remote" Evernote service's guid set, it is identified in the database by this guid.
     * Otherwise it is identified by local guid.
     * @param tag - tag to be added to the local storage
     * @param errorDescription - error description if Tag could not be added
     * @return true if Tag was added successfully, false otherwise
     */
    bool AddTag(const Tag & tag, QString & errorDescription);

    /**
     * @brief UpdateTag - updates passed in Tag in the local storage database. If tag has
     * "remote" Evernote service's guid set, it is identified in the database by this guid.
     * Otherwise it is identified by local guid.
     * @param tag - Tag filled with values to be updated in the local storage database
     * @param errorDescription - error description if Tag could not be updated
     * @return true if Tag was updated successfully, false otherwise
     */
    bool UpdateTag(const Tag & tag, QString & errorDescription);

    bool LinkTagWithNote(const Tag & tag, const Note & note, QString & errorDescription);

    bool FindTag(const QString & tagGuid, const WhichGuid::type whichGuid, Tag & tag,
                 QString & errorDescription) const;

    bool ListAllTagsPerNote(const QString & noteGuid, std::vector<Tag> & tags,
                            QString & errorDescription) const;

    bool ListAllTags(std::vector<Tag> & tags, QString & errorDescription) const;

    /**
     * @brief DeleteTag - either expunges the local tag (i.e. deletes it from
     * local storage completely, without the possibility to restore)
     * if it has not been synchronized with Evernote service yet or marks the tag
     * as deleted otherwise. Evernote API doesn't allow thirdparty applications
     * to expunge tags which have ever been synchronized with remote data store at least once
     * @param tag - tag to be deleted
     * @param errorDescription - error description if tag could not be deleted
     * @return true if tag was deleted successfully, false otherwise
     */
    bool DeleteTag(const Tag & tag, QString & errorDescription);

    /**
     * @brief ExpungeTag - permanently deletes local tags i.e. tags which have not yet been
     * sychronized with Evernote service. This method deletes the tag from local storage
     * completely, without the possibility to restore it
     * @param tag - tag to be expunged
     * @param errorDescription - error description if tag could not be expunged
     * @return true if tag was expunged successfully, false otherwise
     */
    bool ExpungeTag(const Tag & tag, QString & errorDescription);

    bool AddEnResource(const IResource & resource, const Note & note, QString & errorDescription);
    bool UpdateEnResource(const IResource & resource, const Note & note, QString & errorDescription);

    bool FindEnResource(IResource & resource, QString & errorDescription, const bool withBinaryData = true) const;

    // NOTE: there is no 'DeleteEnResource' method for a reason: resources are deleted automatically
    // in remote storage so there's no need to mark some resource as deleted for synchronization procedure.

    /**
     * @brief ExpungeResource - permanently deletes resource from local storage completely,
     * without the possibility to restore it
     * @param resource - resource to be expunged
     * @param errorDescription - error description if resource could not be expunged
     * @return true if resource was expunged successfully, false otherwise
     */
    bool ExpungeEnResource(const IResource & resource, QString & errorDescription);

    /**
     * @brief AddSavedSearch - adds passed in SavedSearch to the local storage database;
     * If search has "remote" Evernote service's guid set, it is identified in the database
     * by this guid. Otherwise it is identified by local guid.
     * @param search - SavedSearch to be added to the local storage
     * @param errorDescription - error description if SavedSearch could not be added
     * @return true if SavedSearch was added successfully, false otherwise
     */
    bool AddSavedSearch(const SavedSearch & search, QString & errorDescription);

    /**
     * @brief UpdateSavedSearch - updates passed in SavedSearch in th local storage database.
     * If search has "remote" Evernote service's guid set, it is identified in the database
     * by this guid. Otherwise it is identified by local guid.
     * @param search - SavedSearch filled with values to be updated in the local storage database
     * @param errorDescription - error description if SavedSearch could not be updated
     * @return true if SavedSearch was updated successfully, false otherwise
     */
    bool UpdateSavedSearch(const SavedSearch & search, QString & errorDescription);

    bool FindSavedSearch(const QString & searchGuid, const WhichGuid::type whichGuid,
                         SavedSearch & search, QString & errorDescription) const;

    bool ListAllSavedSearches(std::vector<SavedSearch> & searches, QString & errorDescription) const;

    // NOTE: there is no 'DeleteSearch' method for a reason: saved searches are deleted automatically
    // in remote storage so there's no need to mark some saved search as deleted for synchronization procedure.

    /**
     * @brief ExpungeSavedSearch - permanently deletes saved search from local storage completely,
     * without the possibility to restore it
     * @param search - saved search to be expunged
     * @param errorDescription - error description if saved search could not be expunged
     * @return true if saved search was expunged succesfully, false otherwise
     */
    bool ExpungeSavedSearch(const SavedSearch & search, QString & errorDescription);

private:
    bool CreateTables(QString & errorDescription);
    bool SetNoteAttributes(const Note & note, QString & errorDescription);
    bool SetNotebookAdditionalAttributes(const Notebook & notebook, QString & errorDescription);
    bool SetNotebookRestrictions(const qevercloud::NotebookRestrictions & notebookRestrictions,
                                 const QString & notebookLocalGuid, QString & errorDescription);
    bool SetSharedNotebookAttributes(const ISharedNotebook &sharedNotebook,
                                     QString & errorDescription);

    bool RowExists(const QString & tableName, const QString & uniqueKeyName,
                   const QVariant & uniqueKeyValue) const;

    bool InsertOrReplaceUser(const IUser & user, QString & errorDescription);
    bool InsertOrReplaceNotebook(const Notebook & notebook, QString & errorDescription);
    bool InsertOrReplaceLinkedNotebook(const LinkedNotebook & linkedNotebook, QString & errorDescription);
    bool InsertOrReplaceNote(const Note & note, const Notebook & notebook, QString & errorDescription);
    bool InsertOrReplaceTag(const Tag & tag, QString & errorDescription);
    bool InsertOrReplaceResource(const IResource & resource, const Note & note, QString & errorDescription);
    bool InsertOrReplaceSavedSearch(const SavedSearch & search, QString & errorDescription);

    bool FillNoteFromSqlRecord(const QSqlRecord & record, Note & note, QString & errorDescription,
                               const bool withResourceBinaryData) const;
    bool FillNotebookFromSqlRecord(const QSqlRecord & record, Notebook & notebook, QString & errorDescription) const;
    bool FillSharedNotebookFromSqlRecord(const QSqlRecord & record, ISharedNotebook & sharedNotebook,
                                         QString & errorDescription) const;
    bool FillLinkedNotebookFromSqlRecord(const QSqlRecord & record, LinkedNotebook & linkedNotebook,
                                         QString & errorDescription) const;
    bool FillSavedSearchFromSqlRecord(const QSqlRecord & rec, SavedSearch & search,
                                      QString & errorDescription) const;
    bool FillTagFromSqlRecord(const QSqlRecord & rec, Tag & tag,
                              QString & errorDescription) const;
    bool FillTagsFromSqlQuery(QSqlQuery & query, std::vector<Tag> & tags,
                              QString & errorDescription) const;

    bool FindAndSetTagGuidsPerNote(Note & note, QString & errorDescription) const;
    bool FindAndSetResourcesPerNote(Note & note, QString & errorDescription,
                                    const bool withBinaryData = true) const;
    bool FindAndSetNoteAttributesPerNote(Note & note, QString & errorDescription) const;

    QList<qevercloud::SharedNotebook> ListEnSharedNotebooksPerNotebookGuid(const QString & notebookGuid,
                                                                           QString & errorDescription) const;

    LocalStorageManager() = delete;
    LocalStorageManager(const LocalStorageManager & other) = delete;
    LocalStorageManager & operator=(const LocalStorageManager & other) = delete;

    QString m_currentUsername;
    UserID m_currentUserId;
    QString m_applicationPersistenceStoragePath;

    QSqlDatabase m_sqlDatabase;
};

}

#endif // __QUTE_NOTE__EVERNOTE_CLIENT__LOCAL_STORAGE__LOCAL_STORAGE_MANAGER
