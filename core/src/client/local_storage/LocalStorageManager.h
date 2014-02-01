#ifndef __QUTE_NOTE__EVERNOTE_CLIENT__LOCAL_STORAGE__LOCAL_STORAGE_MANAGER
#define __QUTE_NOTE__EVERNOTE_CLIENT__LOCAL_STORAGE__LOCAL_STORAGE_MANAGER

#include <QString>
#include <QtSql>
#include <QSharedPointer>

namespace evernote {
namespace edam {

typedef std::string Guid;
QT_FORWARD_DECLARE_CLASS(User)
QT_FORWARD_DECLARE_CLASS(NoteStoreClient)
QT_FORWARD_DECLARE_CLASS(Note)
QT_FORWARD_DECLARE_CLASS(Notebook)
QT_FORWARD_DECLARE_CLASS(SharedNotebook)

}
}

namespace qute_note {

QT_FORWARD_DECLARE_STRUCT(Notebook)
QT_FORWARD_DECLARE_STRUCT(Note)
QT_FORWARD_DECLARE_STRUCT(Tag)
QT_FORWARD_DECLARE_STRUCT(Resource)

// TODO: implement all the necessary functionality
class LocalStorageManager
{
public:
    LocalStorageManager(const QString & username, const int32_t userId,
                        const QString & authenticationToken,
                        QSharedPointer<evernote::edam::NoteStoreClient> & pNoteStore);
    ~LocalStorageManager();

    void SetNewAuthenticationToken(const QString & authenticationToken);

    bool AddUser(const evernote::edam::User & user, QString & errorDescription);
    bool UpdateUser(const evernote::edam::User & user, QString & errorDescription);

    void SwitchUser(const QString & username, const int32_t userId);

    bool AddNotebook(const Notebook & notebook, QString & errorDescription);
    bool UpdateNotebook(const Notebook & notebook, QString & errorDescription);

    bool FindNotebook(const evernote::edam::Guid & notebookGuid, Notebook & notebook,
                      QString & errorDescription);

    /**
     * @brief ExpungeNotebook - deletes specified notebook from local storage.
     * Evernote API doesn't allow to delete notebooks from remote storage, it can
     * only be done by official desktop client or wed GUI. So this method should be called
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

    bool AddTag(const Tag & /* tag */, QString & /* errorDescription */) { return true; }
    bool AddTagToNote(const Tag & tag, const Note & note, QString & errorDescription);
    bool ReplaceTag(const Tag & tag, QString & errorDescription);

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

    bool AddResource(const Resource & resource, QString & errorDescription);
    bool ReplaceResource(const Resource & resource, QString & errorDescription);

    /**
     * @brief DeleteResource - either expunges the local resource (i.e. deletes it from
     * local storage completely, without the possibility to restore)
     * if it has not been synchronized with Evernote service yet or marks the resource
     * as deleted otherwise. Evernote API doesn't allow thirdparty applications
     * to expunge resources which have ever been synchronized with remote data store
     * at least once
     * @param resourceMetadata - metadata of the resource to be deleted
     * @param errorDescription - error description if resource could not be deleted
     * @return true if resource was deleted successfully, false otherwise
     */
    bool DeleteResource(const Resource & resource, QString & errorDescription);

    /**
     * @brief ExpungeResource - permanently deletes local resources i.e. resources which have not yet been
     * synchronized with Evernote service. This method deletes the resource local storage completely,
     * without the possibility to restore it
     * @param resourceMetadata - metadata of the resource to be deleted
     * @param errorDescription - error description if resource could not be expunged
     * @return true if resource was expunged successfully, false otherwise
     */
    bool ExpungeResource(const Resource & resource, QString & errorDescription);

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

    bool InsertOrReplaceUser(const evernote::edam::User & user, QString & errorDescription);

    LocalStorageManager() = delete;
    LocalStorageManager(const LocalStorageManager & other) = delete;
    LocalStorageManager & operator=(const LocalStorageManager & other) = delete;

    QString m_authenticationToken;
    QSharedPointer<evernote::edam::NoteStoreClient> m_pNoteStore;
    QString m_currentUsername;
    int32_t m_currentUserId;
    QString m_applicationPersistenceStoragePath;

    QSqlDatabase m_sqlDatabase;
};

}

#endif // __QUTE_NOTE__EVERNOTE_CLIENT__LOCAL_STORAGE__LOCAL_STORAGE_MANAGER
