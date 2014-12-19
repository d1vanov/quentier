#ifndef __QUTE_NOTE__CLIENT__LOCAL_STORAGE__LOCAL_STORAGE_MANAGER_PRIVATE_H
#define __QUTE_NOTE__CLIENT__LOCAL_STORAGE__LOCAL_STORAGE_MANAGER_PRIVATE_H

#include "Lists.h"
#include "LocalStorageManager.h"
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
QT_FORWARD_DECLARE_CLASS(NoteSearchQuery)

class LocalStorageManagerPrivate: public QObject
{
    Q_OBJECT
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
    QList<Notebook> ListNotebooks(const LocalStorageManager::ListObjectsOptions flag,
                                  QString & errorDescription, const size_t limit, const size_t offset,
                                  const LocalStorageManager::ListNotebooksOrder::type & order) const;
    QList<SharedNotebookWrapper> ListAllSharedNotebooks(QString & errorDescription) const;
    QList<SharedNotebookWrapper> ListSharedNotebooksPerNotebookGuid(const QString & notebookGuid,
                                                                    QString & errorDescription) const;
    bool ExpungeNotebook(const Notebook & notebook, QString & errorDescription);

    int GetLinkedNotebookCount(QString & errorDescription) const;
    bool AddLinkedNotebook(const LinkedNotebook & linkedNotebook, QString & errorDescription);
    bool UpdateLinkedNotebook(const LinkedNotebook & linkedNotebook, QString & errorDescription);
    bool FindLinkedNotebook(LinkedNotebook & linkedNotebook, QString & errorDescription) const;
    QList<LinkedNotebook> ListAllLinkedNotebooks(QString & errorDescription) const;
    QList<LinkedNotebook> ListLinkedNotebooks(const LocalStorageManager::ListObjectsOptions flag,
                                              QString & errorDescription, const size_t limit = 0,
                                              const size_t offset = 0, const QString & orderBy = QString()) const;
    bool ExpungeLinkedNotebook(const LinkedNotebook & linkedNotebook, QString & errorDescription);

    int GetNoteCount(QString & errorDescription) const;
    bool AddNote(const Note & note, const Notebook & notebook, QString & errorDescription);
    bool UpdateNote(const Note & note, const Notebook & notebook, QString & errorDescription);
    bool FindNote(Note & note, QString & errorDescription,
                  const bool withResourceBinaryData = true) const;
    QList<Note> ListAllNotesPerNotebook(const Notebook & notebook, QString & errorDescription,
                                        const bool withResourceBinaryData = true) const;
    QList<Note> ListNotes(const LocalStorageManager::ListObjectsOptions flag, QString & errorDescription,
                          const bool withResourceBinaryData, const size_t limit,
                          const size_t offset, const LocalStorageManager::ListNotesOrder::type & order) const;
    bool DeleteNote(const Note & note, QString & errorDescription);
    bool ExpungeNote(const Note & note, QString & errorDescription);

    NoteList FindNotesWithSearchQuery(const NoteSearchQuery & noteSearchQuery,
                                      QString & errorDescription,
                                      const bool withResourceBinaryData = true) const;

    int GetTagCount(QString & errorDescription) const;
    bool AddTag(const Tag & tag, QString & errorDescription);
    bool UpdateTag(const Tag & tag, QString & errorDescription);
    bool LinkTagWithNote(const Tag & tag, const Note & note, QString & errorDescription);
    bool FindTag(Tag & tag, QString & errorDescription) const;
    QList<Tag> ListAllTagsPerNote(const Note & note, QString & errorDescription) const;
    QList<Tag> ListAllTags(QString & errorDescription) const;
    QList<Tag> ListTags(const LocalStorageManager::ListObjectsOptions flag, QString & errorDescription,
                        const size_t limit = 0, const size_t offset = 0, const QString & orderBy = QString()) const;
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
    QList<SavedSearch> ListSavedSearches(const LocalStorageManager::ListObjectsOptions flag,
                                         QString & errorDescription, const size_t limit = 0,
                                         const size_t offset = 0, const QString & orderBy = QString()) const;
    bool ExpungeSavedSearch(const SavedSearch & search, QString & errorDescription);

public Q_SLOTS:
    void ProcessPostTransactionException(QString message, QSqlError error) const;

private:
    LocalStorageManagerPrivate() Q_DECL_DELETE;
    LocalStorageManagerPrivate(const LocalStorageManagerPrivate & other) Q_DECL_DELETE;
    LocalStorageManagerPrivate & operator=(const LocalStorageManagerPrivate & other) Q_DECL_DELETE;

    bool CreateTables(QString & errorDescription);
    bool InsertOrReplaceNotebookRestrictions(const qevercloud::NotebookRestrictions & notebookRestrictions,
                                             const QString & localGuid, QString & errorDescription);
    bool InsertOrReplaceSharedNotebook(const ISharedNotebook & sharedNotebook,
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
    bool CheckAndPrepareGetUserCountQuery() const;
    bool CheckAndPrepareInsertOrReplaceUserQuery();
    bool CheckAndPrepareExpungeAccountingQuery();
    bool CheckAndPrepareInsertOrReplaceAccountingQuery();
    bool CheckAndPrepareExpungePremiumUserInfoQuery();
    bool CheckAndPrepareInsertOrReplacePremiumUserInfoQuery();
    bool CheckAndPrepareExpungeBusinessUserInfoQuery();
    bool CheckAndPrepareInsertOrReplaceBusinessUserInfoQuery();
    bool CheckAndPrepareExpungeUserAttributesQuery();
    bool CheckAndPrepareInsertOrReplaceUserAttributesQuery();
    bool CheckAndPrepareExpungeUserAttributesViewedPromotionsQuery();
    bool CheckAndPrepareInsertOrReplaceUserAttributesViewedPromotionsQuery();
    bool CheckAndPrepareExpungeUserAttributesRecentMailedAddressesQuery();
    bool CheckAndPrepareInsertOrReplaceUserAttributesRecentMailedAddressesQuery();
    bool CheckAndPrepareDeleteUserQuery();
    bool CheckAndPrepareExpungeUserQuery();

    bool InsertOrReplaceNotebook(const Notebook & notebook, const QString & overrideLocalGuid, QString & errorDescription);
    bool CheckAndPrepareGetNotebookCountQuery() const;
    bool CheckAndPrepareInsertOrReplaceNotebookQuery();
    bool CheckAndPrepareExpungeNotebookFromNotebookRestrictionsQuery();
    bool CheckAndPrepareInsertOrReplaceNotebookRestrictionsQuery();
    bool CheckAndPrepareExpungeSharedNotebooksQuery();
    bool CheckAndPrepareInsertOrReplaceSharedNotebokQuery();

    bool InsertOrReplaceLinkedNotebook(const LinkedNotebook & linkedNotebook, QString & errorDescription);
    bool CheckAndPrepareGetLinkedNotebookCountQuery() const;
    bool CheckAndPrepareInsertOrReplaceLinkedNotebookQuery();
    bool CheckAndPrepareExpungeLinkedNotebookQuery();

    bool InsertOrReplaceNote(const Note & note, const Notebook & notebook,
                             const QString & overrideLocalGuid, QString & errorDescription);
    bool CheckAndPrepareGetNoteCountQuery() const;
    bool CheckAndPrepareInsertOrReplaceNoteQuery();
    bool CheckAndPrepareExpungeNoteFromNoteTagsQuery();
    bool CheckAndPrepareInsertOrReplaceNoteIntoNoteTagsQuery();
    bool CheckAndPrepareExpungeResourcesByNoteQuery();

    bool InsertOrReplaceTag(const Tag & tag, const QString & overrideLocalGuid, QString & errorDescription);
    bool CheckAndPrepareGetTagCountQuery() const;
    bool CheckAndPrepareInsertOrReplaceTagQuery();
    bool CheckAndPrepareDeleteTagQuery();
    bool CheckAndPrepareExpungeTagQuery();

    bool InsertOrReplaceResource(const IResource & resource, const QString overrideResourceLocalGuid,
                                 const Note & note, const QString & overrideNoteLocalGuid,
                                 QString & errorDescription, const bool useSeparateTransaction = true);
    bool InsertOrReplaceResourceAttributes(const QString & localGuid, 
                                           const qevercloud::ResourceAttributes & attributes,
                                           QString & errorDescription);
    bool CheckAndPrepareInsertOrReplaceResourceQuery();
    bool CheckAndPrepareInsertOrReplaceNoteResourceQuery();
    bool CheckAndPrepareDeleteResourceFromResourceRecognitionTypesQuery();
    bool CheckAndPrepareInsertOrReplaceIntoResourceRecognitionTypesQuery();
    bool CheckAndPrepareDeleteResourceFromResourceAttributesQuery();
    bool CheckAndPrepareDeleteResourceFromResourceAttributesApplicationDataKeysOnlyQuery();
    bool CheckAndPrepareDeleteResourceFromResourceAttributesApplicationDataFullMapQuery();
    bool CheckAndPrepareInsertOrReplaceResourceAttributesQuery();
    bool CheckAndPrepareInsertOrReplaceResourceAttributesApplicationDataKeysOnlyQuery();
    bool CheckAndPrepareInsertOrReplaceResourceAttributesApplicationDataFullMapQuery();
    bool CheckAndPrepareGetResourceCountQuery() const;

    bool InsertOrReplaceSavedSearch(const SavedSearch & search, const QString & overrideLocalGuid, QString & errorDescription);
    bool CheckAndPrepareInsertOrReplaceSavedSearchQuery();
    bool CheckAndPrepareGetSavedSearchCountQuery() const;
    bool CheckAndPrepareExpungeSavedSearchQuery();

    void FillResourceFromSqlRecord(const QSqlRecord & rec, const bool withBinaryData, IResource & resource) const;
    bool FillResourceAttributesFromSqlRecord(const QSqlRecord & rec, qevercloud::ResourceAttributes & attributes) const;
    bool FillResourceAttributesApplicationDataKeysOnlyFromSqlRecord(const QSqlRecord & rec, qevercloud::ResourceAttributes & attributes) const;
    bool FillResourceAttributesApplicationDataFullMapFromSqlRecord(const QSqlRecord & rec, qevercloud::ResourceAttributes & attributes) const;
    void FillNoteAttributesFromSqlRecord(const QSqlRecord & rec, qevercloud::NoteAttributes & attributes) const;
    void FillNoteAttributesApplicationDataKeysOnlyFromSqlRecord(const QSqlRecord & rec, qevercloud::NoteAttributes & attributes) const;
    void FillNoteAttributesApplicationDataFullMapFromSqlRecord(const QSqlRecord & rec, qevercloud::NoteAttributes & attributes) const;
    void FillNoteAttributesClassificationsFromSqlRecord(const QSqlRecord & rec, qevercloud::NoteAttributes & attributes) const;
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
    void SortSharedNotebooks(Notebook & notebook) const;

    QList<qevercloud::SharedNotebook> ListEnSharedNotebooksPerNotebookGuid(const QString & notebookGuid,
                                                                           QString & errorDescription) const;

    bool noteSearchQueryToSQL(const NoteSearchQuery & noteSearchQuery, QString & sql,
                              QString & errorDescription) const;

    bool tagNamesToTagLocalGuids(const QStringList & tagNames, QStringList & tagLocalGuids,
                                 QString & errorDescription) const;
    bool resourceMimeTypesToResourceLocalGuids(const QStringList & resourceMimeTypes,
                                               QStringList & resourceLocalGuids,
                                               QString & errorDescription) const;
    bool resourceRecognitionTypesToResourceLocalGuids(const QStringList & resourceRecognitionTypes,
                                                      QStringList & resourceLocalGuids,
                                                      QString & errorDescription) const;

    template <class T>
    QString listObjectsOptionsToSqlQueryConditions(const LocalStorageManager::ListObjectsOptions & flag,
                                                   QString & errorDescription) const;

    template <class T, class TOrderBy>
    QList<T> listObjects(const LocalStorageManager::ListObjectsOptions & flag,
                         QString & errorDescription, const size_t limit,
                         const size_t offset, const TOrderBy & orderBy) const;

    template <class T>
    QString listObjectsGenericSqlQuery() const;

    template <class TOrderBy>
    QString orderByToSqlTableColumn(const TOrderBy & orderBy) const;

    template <class T>
    bool fillObjectsFromSqlQuery(QSqlQuery query, QList<T> & objects, QString & errorDescription) const;

    template <class T>
    bool fillObjectFromSqlRecord(const QSqlRecord & record, T & object, QString & errorDescription) const;

    struct SharedNotebookAdapterCompareByIndex
    {
        bool operator()(const SharedNotebookAdapter & lhs, const SharedNotebookAdapter & rhs) const;
    };

    struct ResourceWrapperCompareByIndex
    {
        bool operator()(const ResourceWrapper & lhs, const ResourceWrapper & rhs) const;
    };

    struct QStringIntPairCompareByInt
    {
        bool operator()(const QPair<QString, int> & lhs, const QPair<QString, int> & rhs) const;
    };

    QString             m_currentUsername;
    qevercloud::UserID  m_currentUserId;
    QString             m_applicationPersistenceStoragePath;
    QSqlDatabase        m_sqlDatabase;

    QSqlQuery           m_insertOrReplaceSavedSearchQuery;
    bool                m_insertOrReplaceSavedSearchQueryPrepared;

    mutable QSqlQuery   m_getSavedSearchCountQuery;
    mutable bool        m_getSavedSearchCountQueryPrepared;

    QSqlQuery           m_expungeSavedSearchQuery;
    bool                m_expungeSavedSearchQueryPrepared;

    QSqlQuery           m_insertOrReplaceResourceQuery;
    bool                m_insertOrReplaceResourceQueryPrepared;

    QSqlQuery           m_insertOrReplaceNoteResourceQuery;
    bool                m_insertOrReplaceNoteResourceQueryPrepared;

    QSqlQuery           m_deleteResourceFromResourceRecognitionTypesQuery;
    bool                m_deleteResourceFromResourceRecognitionTypesQueryPrepared;

    QSqlQuery           m_insertOrReplaceIntoResourceRecognitionTypesQuery;
    bool                m_insertOrReplaceIntoResourceRecognitionTypesQueryPrepared;

    QSqlQuery           m_deleteResourceFromResourceAttributesQuery;
    bool                m_deleteResourceFromResourceAttributesQueryPrepared;

    QSqlQuery           m_deleteResourceFromResourceAttributesApplicationDataKeysOnlyQuery;
    bool                m_deleteResourceFromResourceAttributesApplicationDataKeysOnlyQueryPrepared;

    QSqlQuery           m_deleteResourceFromResourceAttributesApplicationDataFullMapQuery;
    bool                m_deleteResourceFromResourceAttributesApplicationDataFullMapQueryPrepared;

    QSqlQuery           m_insertOrReplaceResourceAttributesQuery;
    bool                m_insertOrReplaceResourceAttributesQueryPrepared;

    QSqlQuery           m_insertOrReplaceResourceAttributeApplicationDataKeysOnlyQuery;
    bool                m_insertOrReplaceResourceAttributeApplicationDataKeysOnlyQueryPrepared;

    QSqlQuery           m_insertOrReplaceResourceAttributeApplicationDataFullMapQuery;
    bool                m_insertOrReplaceResourceAttributeApplicationDataFullMapQueryPrepared;

    mutable QSqlQuery   m_getResourceCountQuery;
    mutable bool        m_getResourceCountQueryPrepared;

    mutable QSqlQuery   m_getTagCountQuery;
    mutable bool        m_getTagCountQueryPrepared;

    QSqlQuery           m_insertOrReplaceTagQuery;
    bool                m_insertOrReplaceTagQueryPrepared;

    QSqlQuery           m_deleteTagQuery;
    bool                m_deleteTagQueryPrepared;

    QSqlQuery           m_expungeTagQuery;
    bool                m_expungeTagQueryPrepared;

    mutable QSqlQuery   m_getNoteCountQuery;
    mutable bool        m_getNoteCountQueryPrepared;

    QSqlQuery           m_insertOrReplaceNoteQuery;
    bool                m_insertOrReplaceNoteQueryPrepared;

    QSqlQuery           m_expungeNoteFromNoteTagsQuery;
    bool                m_expungeNoteFromNoteTagsQueryPrepared;

    QSqlQuery           m_insertOrReplaceNoteIntoNoteTagsQuery;
    bool                m_insertOrReplaceNoteIntoNoteTagsQueryPrepared;

    QSqlQuery           m_expungeResourceByNoteQuery;
    bool                m_expungeResourceByNoteQueryPrepared;

    mutable QSqlQuery   m_getLinkedNotebookCountQuery;
    mutable bool        m_getLinkedNotebookCountQueryPrepared;

    QSqlQuery           m_insertOrReplaceLinkedNotebookQuery;
    bool                m_insertOrReplaceLinkedNotebookQueryPrepared;

    QSqlQuery           m_expungeLinkedNotebookQuery;
    bool                m_expungeLinkedNotebookQueryPrepared;

    mutable QSqlQuery   m_getNotebookCountQuery;
    mutable bool        m_getNotebookCountQueryPrepared;

    QSqlQuery           m_insertOrReplaceNotebookQuery;
    bool                m_insertOrReplaceNotebookQueryPrepared;

    QSqlQuery           m_expungeNotebookFromNotebookRestrictionsQuery;
    bool                m_expungeNotebookFromNotebookRestrictionsQueryPrepared;

    QSqlQuery           m_insertOrReplaceNotebookRestrictionsQuery;
    bool                m_insertOrReplaceNotebookRestrictionsQueryPrepared;

    QSqlQuery		m_expungeSharedNotebooksQuery;
    bool		m_expungeSharedNotebooksQueryPrepared;

    QSqlQuery		m_insertOrReplaceSharedNotebookQuery;
    bool		m_insertOrReplaceSharedNotebookQueryPrepared;

    mutable QSqlQuery   m_getUserCountQuery;
    mutable bool        m_getUserCountQueryPrepared;

    QSqlQuery           m_insertOrReplaceUserQuery;
    bool                m_insertOrReplaceUserQueryPrepared;

    QSqlQuery           m_expungeUserAttributesQuery;
    bool                m_expungeUserAttributesQueryPrepared;

    QSqlQuery           m_insertOrReplaceUserAttributesQuery;
    bool                m_insertOrReplaceUserAttributesQueryPrepared;

    QSqlQuery           m_expungeAccountingQuery;
    bool                m_expungeAccountingQueryPrepared;

    QSqlQuery           m_insertOrReplaceAccountingQuery;
    bool                m_insertOrReplaceAccountingQueryPrepared;

    QSqlQuery           m_expungePremiumUserInfoQuery;
    bool                m_expungePremiumUserInfoQueryPrepared;

    QSqlQuery           m_insertOrReplacePremiumUserInfoQuery;
    bool                m_insertOrReplacePremiumUserInfoQueryPrepared;

    QSqlQuery           m_expungeBusinessUserInfoQuery;
    bool                m_expungeBusinessUserInfoQueryPrepared;

    QSqlQuery           m_insertOrReplaceBusinessUserInfoQuery;
    bool                m_insertOrReplaceBusinessUserInfoQueryPrepared;

    QSqlQuery           m_expungeUserAttributesViewedPromotionsQuery;
    bool                m_expungeUserAttributesViewedPromotionsQueryPrepared;

    QSqlQuery           m_insertOrReplaceUserAttributesViewedPromotionsQuery;
    bool                m_insertOrReplaceUserAttributesViewedPromotionsQueryPrepared;

    QSqlQuery           m_expungeUserAttributesRecentMailedAddressesQuery;
    bool                m_expungeUserAttributesRecentMailedAddressesQueryPrepared;

    QSqlQuery           m_insertOrReplaceUserAttributesRecentMailedAddressesQuery;
    bool                m_insertOrReplaceUserAttributesRecentMailedAddressesQueryPrepared;

    QSqlQuery           m_deleteUserQuery;
    bool                m_deleteUserQueryPrepared;

    QSqlQuery           m_expungeUserQuery;
    bool                m_expungeUserQueryPrepared;
};

} // namespace qute_note

#endif // __QUTE_NOTE__CLIENT__LOCAL_STORAGE__LOCAL_STORAGE_MANAGER_PRIVATE_H
