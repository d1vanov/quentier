#ifndef __LIB_QUTE_NOTE__LOCAL_STORAGE__LOCAL_STORAGE_MANAGER_PRIVATE_H
#define __LIB_QUTE_NOTE__LOCAL_STORAGE__LOCAL_STORAGE_MANAGER_PRIVATE_H

#include <qute_note/local_storage/Lists.h>
#include <qute_note/local_storage/LocalStorageManager.h>
#include <qute_note/types/IUser.h>
#include <qute_note/types/UserAdapter.h>
#include <qute_note/types/UserWrapper.h>
#include <qute_note/types/Notebook.h>
#include <qute_note/types/ISharedNotebook.h>
#include <qute_note/types/SharedNotebookAdapter.h>
#include <qute_note/types/SharedNotebookWrapper.h>
#include <qute_note/types/Note.h>
#include <qute_note/types/Tag.h>
#include <qute_note/types/IResource.h>
#include <qute_note/types/ResourceAdapter.h>
#include <qute_note/types/ResourceWrapper.h>
#include <qute_note/types/LinkedNotebook.h>
#include <qute_note/types/SavedSearch.h>
#include <qute_note/utility/StringUtils.h>
#include <QtSql>

// Prevent boost::interprocess from automatic linkage to boost::datetime
#define BOOST_DATE_TIME_NO_LIB

// NOTE: Workaround a bug in Qt4 which may prevent building with some boost versions
#ifndef Q_MOC_RUN
#include <boost/interprocess/sync/file_lock.hpp>
#endif

namespace qute_note {

typedef qevercloud::UserID UserID;
QT_FORWARD_DECLARE_CLASS(NoteSearchQuery)

class LocalStorageManagerPrivate: public QObject
{
    Q_OBJECT
public:
    LocalStorageManagerPrivate(const QString & username, const UserID userId,
                               const bool startFromScratch, const bool overrideLock);
    ~LocalStorageManagerPrivate();

Q_SIGNALS:
    void upgradeProgress(double progress);

public:
    void switchUser(const QString & username, const UserID userId, const bool startFromScratch = false,
                    const bool overrideLock = false);
    int userCount(QString & errorDescription) const;
    bool addUser(const IUser & user, QString & errorDescription);
    bool updateUser(const IUser & user, QString & errorDescription);
    bool findUser(IUser & user, QString & errorDescription) const;
    bool deleteUser(const IUser & user, QString & errorDescription);
    bool expungeUser(const IUser & user, QString & errorDescription);

    int notebookCount(QString & errorDescription) const;
    bool addNotebook(const Notebook & notebook, QString & errorDescription);
    bool updateNotebook(const Notebook & notebook, QString & errorDescription);
    bool findNotebook(Notebook & notebook, QString & errorDescription) const;
    bool findDefaultNotebook(Notebook & notebook, QString & errorDescription) const;
    bool findLastUsedNotebook(Notebook & notebook, QString & errorDescription) const;
    bool findDefaultOrLastUsedNotebook(Notebook & notebook, QString & errorDescription) const;
    QList<Notebook> listAllNotebooks(QString & errorDescription, const size_t limit, const size_t offset,
                                     const LocalStorageManager::ListNotebooksOrder::type & order,
                                     const LocalStorageManager::OrderDirection::type & orderDirection,
                                     const QString & linkedNotebookGuid) const;
    QList<Notebook> listNotebooks(const LocalStorageManager::ListObjectsOptions flag,
                                  QString & errorDescription, const size_t limit, const size_t offset,
                                  const LocalStorageManager::ListNotebooksOrder::type & order,
                                  const LocalStorageManager::OrderDirection::type & orderDirection,
                                  const QString & linkedNotebookGuid) const;
    QList<SharedNotebookWrapper> listAllSharedNotebooks(QString & errorDescription) const;
    QList<SharedNotebookWrapper> listSharedNotebooksPerNotebookGuid(const QString & notebookGuid,
                                                                    QString & errorDescription) const;
    bool expungeNotebook(const Notebook & notebook, QString & errorDescription);

    int linkedNotebookCount(QString & errorDescription) const;
    bool addLinkedNotebook(const LinkedNotebook & linkedNotebook, QString & errorDescription);
    bool updateLinkedNotebook(const LinkedNotebook & linkedNotebook, QString & errorDescription);
    bool findLinkedNotebook(LinkedNotebook & linkedNotebook, QString & errorDescription) const;
    QList<LinkedNotebook> listAllLinkedNotebooks(QString & errorDescription, const size_t limit, const size_t offset,
                                                 const LocalStorageManager::ListLinkedNotebooksOrder::type order,
                                                 const LocalStorageManager::OrderDirection::type & orderDirection) const;
    QList<LinkedNotebook> listLinkedNotebooks(const LocalStorageManager::ListObjectsOptions flag,
                                              QString & errorDescription, const size_t limit, const size_t offset,
                                              const LocalStorageManager::ListLinkedNotebooksOrder::type & order,
                                              const LocalStorageManager::OrderDirection::type & orderDirection) const;
    bool expungeLinkedNotebook(const LinkedNotebook & linkedNotebook, QString & errorDescription);

    int noteCount(QString & errorDescription) const;
    bool addNote(const Note & note, QString & errorDescription);
    bool updateNote(const Note & note, const bool updateResources, const bool updateTags, QString & errorDescription);
    bool findNote(Note & note, QString & errorDescription,
                  const bool withResourceBinaryData = true) const;
    QList<Note> listAllNotesPerNotebook(const Notebook & notebook, QString & errorDescription,
                                        const bool withResourceBinaryData,
                                        const LocalStorageManager::ListObjectsOptions & flag,
                                        const size_t limit, const size_t offset,
                                        const LocalStorageManager::ListNotesOrder::type & order,
                                        const LocalStorageManager::OrderDirection::type & orderDirection) const;
    QList<Note> listNotes(const LocalStorageManager::ListObjectsOptions flag, QString & errorDescription,
                          const bool withResourceBinaryData, const size_t limit,
                          const size_t offset, const LocalStorageManager::ListNotesOrder::type & order,
                          const LocalStorageManager::OrderDirection::type & orderDirection) const;
    bool deleteNote(const Note & note, QString & errorDescription);
    bool expungeNote(const Note & note, QString & errorDescription);

    NoteList findNotesWithSearchQuery(const NoteSearchQuery & noteSearchQuery,
                                      QString & errorDescription,
                                      const bool withResourceBinaryData = true) const;

    int tagCount(QString & errorDescription) const;
    bool addTag(const Tag & tag, QString & errorDescription);
    bool updateTag(const Tag & tag, QString & errorDescription);
    bool linkTagWithNote(const Tag & tag, const Note & note, QString & errorDescription);
    bool findTag(Tag & tag, QString & errorDescription) const;
    QList<Tag> listAllTagsPerNote(const Note & note, QString & errorDescription,
                                  const LocalStorageManager::ListObjectsOptions & flag,
                                  const size_t limit, const size_t offset,
                                  const LocalStorageManager::ListTagsOrder::type & order,
                                  const LocalStorageManager::OrderDirection::type & orderDirection) const;
    QList<Tag> listAllTags(QString & errorDescription, const size_t limit,
                           const size_t offset, const LocalStorageManager::ListTagsOrder::type & order,
                           const LocalStorageManager::OrderDirection::type & orderDirection,
                           const QString & linkedNotebookGuid) const;
    QList<Tag> listTags(const LocalStorageManager::ListObjectsOptions flag, QString & errorDescription,
                        const size_t limit, const size_t offset, const LocalStorageManager::ListTagsOrder::type & order,
                        const LocalStorageManager::OrderDirection::type & orderDirection,
                        const QString & linkedNotebookGuid) const;
    bool deleteTag(const Tag & tag, QString & errorDescription);
    bool expungeTag(const Tag & tag, QString & errorDescription);
    bool expungeNotelessTagsFromLinkedNotebooks(QString & errorDescription);

    int enResourceCount(QString & errorDescription) const;
    bool addEnResource(const IResource & resource, const Note & note, QString & errorDescription);
    bool updateEnResource(const IResource & resource, const Note & note, QString & errorDescription);
    bool findEnResource(IResource & resource, QString & errorDescription, const bool withBinaryData = true) const;
    bool expungeEnResource(const IResource & resource, QString & errorDescription);

    int savedSearchCount(QString & errorDescription) const;
    bool addSavedSearch(const SavedSearch & search, QString & errorDescription);
    bool updateSavedSearch(const SavedSearch & search, QString & errorDescription);
    bool findSavedSearch(SavedSearch & search, QString & errorDescription) const;
    QList<SavedSearch> listAllSavedSearches(QString & errorDescription, const size_t limit, const size_t offset,
                                            const LocalStorageManager::ListSavedSearchesOrder::type & order,
                                            const LocalStorageManager::OrderDirection::type & orderDirection) const;
    QList<SavedSearch> listSavedSearches(const LocalStorageManager::ListObjectsOptions flag,
                                         QString & errorDescription, const size_t limit,
                                         const size_t offset, const LocalStorageManager::ListSavedSearchesOrder::type & order,
                                         const LocalStorageManager::OrderDirection::type & orderDirection) const;
    bool expungeSavedSearch(const SavedSearch & search, QString & errorDescription);

public Q_SLOTS:
    void processPostTransactionException(QString message, QSqlError error) const;

private:
    LocalStorageManagerPrivate() Q_DECL_DELETE;
    Q_DISABLE_COPY(LocalStorageManagerPrivate)

    void unlockDatabaseFile();

    bool createTables(QString & errorDescription);
    bool insertOrReplaceNotebookRestrictions(const qevercloud::NotebookRestrictions & notebookRestrictions,
                                             const QString & localUid, QString & errorDescription);
    bool insertOrReplaceSharedNotebook(const ISharedNotebook & sharedNotebook,
                                       QString & errorDescription);

    bool rowExists(const QString & tableName, const QString & uniqueKeyName,
                   const QVariant & uniqueKeyValue) const;

    bool insertOrReplaceUser(const IUser & user, QString & errorDescription);
    bool insertOrReplaceBusinesUserInfo(const UserID id, const qevercloud::BusinessUserInfo & info,
                                        QString & errorDescription);
    bool insertOrReplacePremiumInfo(const UserID id, const qevercloud::PremiumInfo & info,
                                    QString & errorDescription);
    bool insertOrReplaceAccounting(const UserID id, const qevercloud::Accounting & accounting,
                                   QString & errorDescription);
    bool insertOrReplaceUserAttributes(const UserID id, const qevercloud::UserAttributes & attributes,
                                       QString & errorDescription);
    bool checkAndPrepareUserCountQuery() const;
    bool checkAndPrepareInsertOrReplaceUserQuery();
    bool checkAndPrepareExpungeAccountingQuery();
    bool checkAndPrepareInsertOrReplaceAccountingQuery();
    bool checkAndPrepareExpungePremiumUserInfoQuery();
    bool checkAndPrepareInsertOrReplacePremiumUserInfoQuery();
    bool checkAndPrepareExpungeBusinessUserInfoQuery();
    bool checkAndPrepareInsertOrReplaceBusinessUserInfoQuery();
    bool checkAndPrepareExpungeUserAttributesQuery();
    bool checkAndPrepareInsertOrReplaceUserAttributesQuery();
    bool checkAndPrepareExpungeUserAttributesViewedPromotionsQuery();
    bool checkAndPrepareInsertOrReplaceUserAttributesViewedPromotionsQuery();
    bool checkAndPrepareExpungeUserAttributesRecentMailedAddressesQuery();
    bool checkAndPrepareInsertOrReplaceUserAttributesRecentMailedAddressesQuery();
    bool checkAndPrepareDeleteUserQuery();
    bool checkAndPrepareExpungeUserQuery();

    bool insertOrReplaceNotebook(const Notebook & notebook, const QString & overrideLocalUid, QString & errorDescription);
    bool checkAndPrepareNotebookCountQuery() const;
    bool checkAndPrepareInsertOrReplaceNotebookQuery();
    bool checkAndPrepareExpungeNotebookFromNotebookRestrictionsQuery();
    bool checkAndPrepareInsertOrReplaceNotebookRestrictionsQuery();
    bool checkAndPrepareExpungeSharedNotebooksQuery();
    bool checkAndPrepareInsertOrReplaceSharedNotebokQuery();

    bool insertOrReplaceLinkedNotebook(const LinkedNotebook & linkedNotebook, QString & errorDescription);
    bool checkAndPrepareGetLinkedNotebookCountQuery() const;
    bool checkAndPrepareInsertOrReplaceLinkedNotebookQuery();
    bool checkAndPrepareExpungeLinkedNotebookQuery();

    bool getNotebookLocalUidFromNote(const Note & note, QString & notebookLocalUid, QString & errorDescription);
    bool insertOrReplaceNote(const Note & note, const QString & overrideLocalUid, const QString & overrideNotebookLocalUid,
                             const bool updateResources, const bool updateTags, QString & errorDescription);
    bool canAddNoteToNotebook(const QString & notebookLocalUid, QString & errorDescription);
    bool canUpdateNoteInNotebook(const QString & notebookLocalUid, QString & errorDescription);

    bool checkAndPrepareNoteCountQuery() const;
    bool checkAndPrepareInsertOrReplaceNoteQuery();
    bool checkAndPrepareCanAddNoteToNotebookQuery() const;
    bool checkAndPrepareCanUpdateNoteInNotebookQuery() const;
    bool checkAndPrepareExpungeNoteFromNoteTagsQuery();
    bool checkAndPrepareInsertOrReplaceNoteIntoNoteTagsQuery();
    bool checkAndPrepareExpungeResourcesByNoteQuery();

    bool insertOrReplaceTag(const Tag & tag, const QString & overrideLocalUid, QString & errorDescription);
    bool checkAndPrepareTagCountQuery() const;
    bool checkAndPrepareInsertOrReplaceTagQuery();
    bool checkAndPrepareDeleteTagQuery();
    bool checkAndPrepareExpungeTagQuery();

    bool insertOrReplaceResource(const IResource & resource, const QString overrideResourceLocalUid,
                                 const Note & note, const QString & overrideNoteLocalUid,
                                 QString & errorDescription, const bool useSeparateTransaction = true);
    bool insertOrReplaceResourceAttributes(const QString & localUid,
                                           const qevercloud::ResourceAttributes & attributes,
                                           QString & errorDescription);
    bool checkAndPrepareInsertOrReplaceResourceQuery();
    bool checkAndPrepareInsertOrReplaceNoteResourceQuery();
    bool checkAndPrepareDeleteResourceFromResourceRecognitionTypesQuery();
    bool checkAndPrepareInsertOrReplaceIntoResourceRecognitionDataQuery();
    bool checkAndPrepareDeleteResourceFromResourceAttributesQuery();
    bool checkAndPrepareDeleteResourceFromResourceAttributesApplicationDataKeysOnlyQuery();
    bool checkAndPrepareDeleteResourceFromResourceAttributesApplicationDataFullMapQuery();
    bool checkAndPrepareInsertOrReplaceResourceAttributesQuery();
    bool checkAndPrepareInsertOrReplaceResourceAttributesApplicationDataKeysOnlyQuery();
    bool checkAndPrepareInsertOrReplaceResourceAttributesApplicationDataFullMapQuery();
    bool checkAndPrepareResourceCountQuery() const;

    bool insertOrReplaceSavedSearch(const SavedSearch & search, const QString & overrideLocalUid, QString & errorDescription);
    bool checkAndPrepareInsertOrReplaceSavedSearchQuery();
    bool checkAndPrepareGetSavedSearchCountQuery() const;
    bool checkAndPrepareExpungeSavedSearchQuery();

    void fillResourceFromSqlRecord(const QSqlRecord & rec, const bool withBinaryData, IResource & resource) const;
    bool fillResourceAttributesFromSqlRecord(const QSqlRecord & rec, qevercloud::ResourceAttributes & attributes) const;
    bool fillResourceAttributesApplicationDataKeysOnlyFromSqlRecord(const QSqlRecord & rec, qevercloud::ResourceAttributes & attributes) const;
    bool fillResourceAttributesApplicationDataFullMapFromSqlRecord(const QSqlRecord & rec, qevercloud::ResourceAttributes & attributes) const;
    void fillNoteAttributesFromSqlRecord(const QSqlRecord & rec, qevercloud::NoteAttributes & attributes) const;
    void fillNoteAttributesApplicationDataKeysOnlyFromSqlRecord(const QSqlRecord & rec, qevercloud::NoteAttributes & attributes) const;
    void fillNoteAttributesApplicationDataFullMapFromSqlRecord(const QSqlRecord & rec, qevercloud::NoteAttributes & attributes) const;
    void fillNoteAttributesClassificationsFromSqlRecord(const QSqlRecord & rec, qevercloud::NoteAttributes & attributes) const;
    bool fillUserFromSqlRecord(const QSqlRecord & rec, IUser & user, QString &errorDescription) const;
    void fillNoteFromSqlRecord(const QSqlRecord & record, Note & note) const;
    bool fillNoteTagIdFromSqlRecord(const QSqlRecord & record, const QString & column, QList<QPair<QString, int> > & tagIdsAndIndices,
                                    QHash<QString, int> & tagIndexPerId, QString & errorDescription) const;
    bool fillNotebookFromSqlRecord(const QSqlRecord & record, Notebook & notebook, QString & errorDescription) const;
    bool fillSharedNotebookFromSqlRecord(const QSqlRecord & record, ISharedNotebook & sharedNotebook,
                                         QString & errorDescription) const;
    bool fillLinkedNotebookFromSqlRecord(const QSqlRecord & record, LinkedNotebook & linkedNotebook,
                                         QString & errorDescription) const;
    bool fillSavedSearchFromSqlRecord(const QSqlRecord & rec, SavedSearch & search,
                                      QString & errorDescription) const;
    bool fillTagFromSqlRecord(const QSqlRecord & rec, Tag & tag,
                              QString & errorDescription) const;
    QList<Tag> fillTagsFromSqlQuery(QSqlQuery & query, QString & errorDescription) const;

    bool findAndSetTagIdsPerNote(Note & note, QString & errorDescription) const;
    bool findAndSetResourcesPerNote(Note & note, QString & errorDescription,
                                    const bool withBinaryData = true) const;
    void sortSharedNotebooks(Notebook & notebook) const;

    QList<qevercloud::SharedNotebook> listEnSharedNotebooksPerNotebookGuid(const QString & notebookGuid,
                                                                           QString & errorDescription) const;

    bool noteSearchQueryToSQL(const NoteSearchQuery & noteSearchQuery, QString & sql,
                              QString & errorDescription) const;

    bool noteSearchQueryContentSearchTermsToSQL(const NoteSearchQuery & noteSearchQuery,
                                                QString & sql, QString & errorDescription) const;

    void contentSearchTermToSQLQueryPart(QString & frontSearchTermModifier, QString & searchTerm,
                                         QString & backSearchTermModifier, QString & matchStatement) const;

    bool tagNamesToTagLocalUids(const QStringList & tagNames, QStringList & tagLocalUids,
                                QString & errorDescription) const;
    bool resourceMimeTypesToResourceLocalUids(const QStringList & resourceMimeTypes,
                                              QStringList & resourceLocalUids,
                                              QString & errorDescription) const;
    bool resourceRecognitionTypesToResourceLocalUids(const QStringList & resourceRecognitionTypes,
                                                     QStringList & resourceLocalUids,
                                                     QString & errorDescription) const;

    template <class T>
    QString listObjectsOptionsToSqlQueryConditions(const LocalStorageManager::ListObjectsOptions & flag,
                                                   QString & errorDescription) const;

    template <class T, class TOrderBy>
    QList<T> listObjects(const LocalStorageManager::ListObjectsOptions & flag,
                         QString & errorDescription, const size_t limit,
                         const size_t offset, const TOrderBy & orderBy,
                         const LocalStorageManager::OrderDirection::type & orderDirection,
                         const QString & additionalSqlQueryCondition = QString()) const;

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
    QString             m_databaseFilePath;
    QSqlDatabase        m_sqlDatabase;
    boost::interprocess::file_lock  m_databaseFileLock;

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

    QSqlQuery           m_insertOrReplaceIntoResourceRecognitionDataQuery;
    bool                m_insertOrReplaceIntoResourceRecognitionDataQueryPrepared;

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

    mutable QSqlQuery   m_canAddNoteToNotebookQuery;
    mutable bool        m_canAddNoteToNotebookQueryPrepared;

    mutable QSqlQuery   m_canUpdateNoteInNotebookQuery;
    mutable bool        m_canUpdateNoteInNotebookQueryPrepared;

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

    QSqlQuery		    m_expungeSharedNotebooksQuery;
    bool		        m_expungeSharedNotebooksQueryPrepared;

    QSqlQuery		    m_insertOrReplaceSharedNotebookQuery;
    bool		        m_insertOrReplaceSharedNotebookQueryPrepared;

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

    StringUtils         m_stringUtils;
    QVector<QChar>      m_preservedAsterisk;
};

} // namespace qute_note

#endif // __LIB_QUTE_NOTE__LOCAL_STORAGE__LOCAL_STORAGE_MANAGER_PRIVATE_H
