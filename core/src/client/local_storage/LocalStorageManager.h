#ifndef __QUTE_NOTE__EVERNOTE_CLIENT__LOCAL_STORAGE__LOCAL_STORAGE_MANAGER
#define __QUTE_NOTE__EVERNOTE_CLIENT__LOCAL_STORAGE__LOCAL_STORAGE_MANAGER

#include <QString>
#include <QtSql>
#include <QSharedPointer>

namespace evernote {
namespace edam {

typedef std::string Guid;
QT_FORWARD_DECLARE_CLASS(User)
QT_FORWARD_DECLARE_CLASS(UserAttributes)
QT_FORWARD_DECLARE_CLASS(Accounting)
QT_FORWARD_DECLARE_CLASS(PremiumInfo)
QT_FORWARD_DECLARE_CLASS(BusinessUserInfo)
QT_FORWARD_DECLARE_CLASS(NoteStoreClient)
QT_FORWARD_DECLARE_CLASS(Note)
QT_FORWARD_DECLARE_CLASS(Notebook)
QT_FORWARD_DECLARE_CLASS(SharedNotebook)
typedef int32_t UserID;

}
}

namespace qute_note {

QT_FORWARD_DECLARE_STRUCT(Notebook)
QT_FORWARD_DECLARE_STRUCT(Note)
QT_FORWARD_DECLARE_STRUCT(Tag)
QT_FORWARD_DECLARE_CLASS(IResource)
QT_FORWARD_DECLARE_STRUCT(SavedSearch)
QT_FORWARD_DECLARE_STRUCT(User)
QT_FORWARD_DECLARE_CLASS(IUser)
typedef evernote::edam::UserID UserID;

// TODO: implement all the necessary functionality
class LocalStorageManager
{
public:
    LocalStorageManager(const QString & username, const UserID userId,
                        const QString & authenticationToken,
                        QSharedPointer<evernote::edam::NoteStoreClient> & pNoteStore);
    ~LocalStorageManager();

    void SetNewAuthenticationToken(const QString & authenticationToken);

    void SwitchUser(const QString & username, const UserID userId);

    bool AddUser(const IUser & user, QString & errorDescription);
    bool UpdateUser(const IUser & user, QString & errorDescription);
    bool FindUser(const UserID id, IUser & user, QString & errorDescription) const;
    bool FindUserAttributes(const UserID id, evernote::edam::UserAttributes & attributes,
                            QString & errorDescription) const;
    bool FindAccounting(const UserID id, evernote::edam::Accounting & accounting,
                        QString & errorDescription) const;
    bool FindPremiumInfo(const UserID id, evernote::edam::PremiumInfo & info,
                         QString & errorDescription) const;
    bool FindBusinessUserInfo(const UserID id, evernote::edam::BusinessUserInfo & info,
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

    bool AddNotebook(const Notebook & notebook, QString & errorDescription);
    bool UpdateNotebook(const Notebook & notebook, QString & errorDescription);

    bool FindNotebook(const evernote::edam::Guid & notebookGuid, Notebook & notebook,
                      QString & errorDescription);

    // TODO: implement
    bool ListAllNotebooks(std::vector<Notebook> & notebooks, QString & errorDescription) const;

    // TODO: implement
    bool ListAllSharedNotebooks(std::vector<Notebook> & notebooks, QString & errorDescription) const;

    // TODO: implement
    bool LIstAllLinkedNotebooks(std::vector<Notebook> & notebooks, QString & errorDescription) const;

    /**
     * @brief ExpungeNotebook - deletes specified notebook from local storage.
     * Evernote API doesn't allow to delete notebooks from remote storage, it can
     * only be done by official desktop client or web GUI. So this method should be called
     * only during synchronization with remote database, when some notebook is found to be
     * deleted via either official desktop client or web GUI
     * @param notebook - notebook to be expunged
     * @param errorDescription - error description if notebook could not be expunged
     * @return true if notebook was expunged successfully, false otherwise
     */
    bool ExpungeNotebook(const Notebook & notebook, QString & errorDescription);

    bool AddNote(const Note & note, QString & errorDescription);
    bool UpdateNote(const Note & note, QString & errorDescription);

    bool FindNote(const evernote::edam::Guid & noteGuid, Note & note,
                  QString & errorDescription) const;

    bool FindAllNotesPerNotebook(const evernote::edam::Guid & notebookGuid,
                                 std::vector<Note> & notes, QString & errorDescription) const;


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

    bool AddTag(const Tag & tag, QString & errorDescription);
    bool UpdateTag(const Tag & tag, QString & errorDescription);

    bool LinkTagWithNote(const Tag & tag, const Note & note, QString & errorDescription);

    bool FindTag(const evernote::edam::Guid & tagGuid, Tag & tag, QString & errorDescription) const;

    // TODO: implement
    bool FindAllTagsPerNotebook(const evernote::edam::Guid & notebook, std::vector<Tag> & tags,
                                QString & errorDescription) const;

    // TODO: implement
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

    bool AddResource(const IResource & resource, QString & errorDescription);
    bool UpdateResource(const IResource & resource, QString & errorDescription);

    bool FindResource(const evernote::edam::Guid & resourceGuid, IResource & resource,
                      QString & errorDescription, const bool withBinaryData = true) const;

    // NOTE: there is no 'DeleteResource' method for a reason: resources are deleted automatically
    // in remote storage so there's no need to mark some resource as deleted for synchronization procedure.

    /**
     * @brief ExpungeResource - permanently deletes resource from local storage completely,
     * without the possibility to restore it
     * @param resource - resource to be expunged
     * @param errorDescription - error description if resource could not be expunged
     * @return true if resource was expunged successfully, false otherwise
     */
    bool ExpungeResource(const IResource & resource, QString & errorDescription);

    bool AddSavedSearch(const SavedSearch & search, QString & errorDescription);
    bool UpdateSavedSearch(const SavedSearch & search, QString & errorDescription);

    bool FindSavedSearch(const evernote::edam::Guid & searchGuid, SavedSearch & search,
                         QString & errorDescription) const;

    // TODO: implement
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
    bool SetNoteAttributes(const evernote::edam::Note & note, QString & errorDescription);
    bool SetNotebookAdditionalAttributes(const evernote::edam::Notebook & notebook,
                                         QString & errorDescription);
    bool SetNotebookRestrictions(const evernote::edam::Notebook & notebook,
                                 QString & errorDescription);
    bool SetSharedNotebookAttributes(const evernote::edam::SharedNotebook & sharedNotebook,
                                     QString & errorDescription);

    int GetRowId(const QString & tableName, const QString & uniqueKeyName,
                 const QVariant & uniqueKeyValue) const;

    bool InsertOrReplaceUser(const IUser & user, QString & errorDescription);
    bool InsertOrReplaceNotebook(const Notebook & notebook, QString & errorDescription);
    bool InsertOrReplaceNote(const Note & note, QString & errorDescription);
    bool InsertOrReplaceTag(const Tag & tag, QString & errorDescription);
    bool InsertOrReplaceResource(const IResource & resource, QString & errorDescription);
    bool InsertOrReplaceSavedSearch(const SavedSearch & search, QString & errorDescription);

    bool FindAndSetTagGuidsPerNote(evernote::edam::Note & enNote, QString & errorDescription) const;
    bool FindAndSetResourcesPerNote(evernote::edam::Note & enNote, QString & errorDescription,
                                    const bool withBinaryData = true) const;
    bool FindAndSetNoteAttributesPerNote(evernote::edam::Note & enNote, QString & errorDescription) const;

    LocalStorageManager() = delete;
    LocalStorageManager(const LocalStorageManager & other) = delete;
    LocalStorageManager & operator=(const LocalStorageManager & other) = delete;

    QString m_authenticationToken;
    QSharedPointer<evernote::edam::NoteStoreClient> m_pNoteStore;
    QString m_currentUsername;
    UserID m_currentUserId;
    QString m_applicationPersistenceStoragePath;

    QSqlDatabase m_sqlDatabase;
};

}

#endif // __QUTE_NOTE__EVERNOTE_CLIENT__LOCAL_STORAGE__LOCAL_STORAGE_MANAGER
