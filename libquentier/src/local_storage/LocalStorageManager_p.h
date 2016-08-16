#ifndef LIB_QUENTIER_LOCAL_STORAGE_LOCAL_STORAGE_MANAGER_PRIVATE_H
#define LIB_QUENTIER_LOCAL_STORAGE_LOCAL_STORAGE_MANAGER_PRIVATE_H

#include <quentier/local_storage/Lists.h>
#include <quentier/local_storage/LocalStorageManager.h>
#include <quentier/types/IUser.h>
#include <quentier/types/UserAdapter.h>
#include <quentier/types/UserWrapper.h>
#include <quentier/types/Notebook.h>
#include <quentier/types/ISharedNotebook.h>
#include <quentier/types/SharedNotebookAdapter.h>
#include <quentier/types/SharedNotebookWrapper.h>
#include <quentier/types/Note.h>
#include <quentier/types/Tag.h>
#include <quentier/types/IResource.h>
#include <quentier/types/ResourceAdapter.h>
#include <quentier/types/ResourceWrapper.h>
#include <quentier/types/LinkedNotebook.h>
#include <quentier/types/SavedSearch.h>
#include <quentier/utility/StringUtils.h>
#include <QtSql>

// Prevent boost::interprocess from automatic linkage to boost::datetime
#define BOOST_DATE_TIME_NO_LIB

// NOTE: Workaround a bug in Qt4 which may prevent building with some boost versions
#ifndef Q_MOC_RUN
#include <boost/interprocess/sync/file_lock.hpp>
#endif

namespace quentier {

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
    int userCount(QNLocalizedString & errorDescription) const;
    bool addUser(const IUser & user, QNLocalizedString & errorDescription);
    bool updateUser(const IUser & user, QNLocalizedString & errorDescription);
    bool findUser(IUser & user, QNLocalizedString & errorDescription) const;
    bool deleteUser(const IUser & user, QNLocalizedString & errorDescription);
    bool expungeUser(const IUser & user, QNLocalizedString & errorDescription);

    int notebookCount(QNLocalizedString & errorDescription) const;
    bool addNotebook(Notebook & notebook, QNLocalizedString & errorDescription);
    bool updateNotebook(Notebook & notebook, QNLocalizedString & errorDescription);
    bool findNotebook(Notebook & notebook, QNLocalizedString & errorDescription) const;
    bool findDefaultNotebook(Notebook & notebook, QNLocalizedString & errorDescription) const;
    bool findLastUsedNotebook(Notebook & notebook, QNLocalizedString & errorDescription) const;
    bool findDefaultOrLastUsedNotebook(Notebook & notebook, QNLocalizedString & errorDescription) const;
    QList<Notebook> listAllNotebooks(QNLocalizedString & errorDescription, const size_t limit, const size_t offset,
                                     const LocalStorageManager::ListNotebooksOrder::type & order,
                                     const LocalStorageManager::OrderDirection::type & orderDirection,
                                     const QString & linkedNotebookGuid) const;
    QList<Notebook> listNotebooks(const LocalStorageManager::ListObjectsOptions flag,
                                  QNLocalizedString & errorDescription, const size_t limit, const size_t offset,
                                  const LocalStorageManager::ListNotebooksOrder::type & order,
                                  const LocalStorageManager::OrderDirection::type & orderDirection,
                                  const QString & linkedNotebookGuid) const;
    QList<SharedNotebookWrapper> listAllSharedNotebooks(QNLocalizedString & errorDescription) const;
    QList<SharedNotebookWrapper> listSharedNotebooksPerNotebookGuid(const QString & notebookGuid,
                                                                    QNLocalizedString & errorDescription) const;
    bool expungeNotebook(Notebook & notebook, QNLocalizedString & errorDescription);

    int linkedNotebookCount(QNLocalizedString & errorDescription) const;
    bool addLinkedNotebook(const LinkedNotebook & linkedNotebook, QNLocalizedString & errorDescription);
    bool updateLinkedNotebook(const LinkedNotebook & linkedNotebook, QNLocalizedString & errorDescription);
    bool findLinkedNotebook(LinkedNotebook & linkedNotebook, QNLocalizedString & errorDescription) const;
    QList<LinkedNotebook> listAllLinkedNotebooks(QNLocalizedString & errorDescription, const size_t limit, const size_t offset,
                                                 const LocalStorageManager::ListLinkedNotebooksOrder::type order,
                                                 const LocalStorageManager::OrderDirection::type & orderDirection) const;
    QList<LinkedNotebook> listLinkedNotebooks(const LocalStorageManager::ListObjectsOptions flag,
                                              QNLocalizedString & errorDescription, const size_t limit, const size_t offset,
                                              const LocalStorageManager::ListLinkedNotebooksOrder::type & order,
                                              const LocalStorageManager::OrderDirection::type & orderDirection) const;
    bool expungeLinkedNotebook(const LinkedNotebook & linkedNotebook, QNLocalizedString & errorDescription);

    int noteCount(QNLocalizedString & errorDescription) const;
    int noteCountPerNotebook(const Notebook & notebook, QNLocalizedString & errorDescription) const;
    int noteCountPerTag(const Tag & tag, QNLocalizedString & errorDescription) const;
    bool addNote(Note & note, QNLocalizedString & errorDescription);
    bool updateNote(Note & note, const bool updateResources, const bool updateTags, QNLocalizedString & errorDescription);
    bool findNote(Note & note, QNLocalizedString & errorDescription,
                  const bool withResourceBinaryData = true) const;
    QList<Note> listNotesPerNotebook(const Notebook & notebook, QNLocalizedString & errorDescription,
                                     const bool withResourceBinaryData,
                                     const LocalStorageManager::ListObjectsOptions & flag,
                                     const size_t limit, const size_t offset,
                                     const LocalStorageManager::ListNotesOrder::type & order,
                                     const LocalStorageManager::OrderDirection::type & orderDirection) const;
    QList<Note> listNotesPerTag(const Tag & tag, QNLocalizedString & errorDescription,
                                const bool withResourceBinaryData,
                                const LocalStorageManager::ListObjectsOptions & flag,
                                const size_t limit, const size_t offset,
                                const LocalStorageManager::ListNotesOrder::type & order,
                                const LocalStorageManager::OrderDirection::type & orderDirection) const;
    QList<Note> listNotes(const LocalStorageManager::ListObjectsOptions flag, QNLocalizedString & errorDescription,
                          const bool withResourceBinaryData, const size_t limit,
                          const size_t offset, const LocalStorageManager::ListNotesOrder::type & order,
                          const LocalStorageManager::OrderDirection::type & orderDirection) const;
    bool expungeNote(Note & note, QNLocalizedString & errorDescription);

    QStringList findNoteLocalUidsWithSearchQuery(const NoteSearchQuery & noteSearchQuery,
                                                 QNLocalizedString & errorDescription) const;
    NoteList findNotesWithSearchQuery(const NoteSearchQuery & noteSearchQuery,
                                      QNLocalizedString & errorDescription,
                                      const bool withResourceBinaryData = true) const;

    int tagCount(QNLocalizedString & errorDescription) const;
    bool addTag(Tag & tag, QNLocalizedString & errorDescription);
    bool updateTag(Tag & tag, QNLocalizedString & errorDescription);
    bool findTag(Tag & tag, QNLocalizedString & errorDescription) const;
    QList<Tag> listAllTagsPerNote(const Note & note, QNLocalizedString & errorDescription,
                                  const LocalStorageManager::ListObjectsOptions & flag,
                                  const size_t limit, const size_t offset,
                                  const LocalStorageManager::ListTagsOrder::type & order,
                                  const LocalStorageManager::OrderDirection::type & orderDirection) const;
    QList<Tag> listAllTags(QNLocalizedString & errorDescription, const size_t limit,
                           const size_t offset, const LocalStorageManager::ListTagsOrder::type & order,
                           const LocalStorageManager::OrderDirection::type & orderDirection,
                           const QString & linkedNotebookGuid) const;
    QList<Tag> listTags(const LocalStorageManager::ListObjectsOptions flag, QNLocalizedString & errorDescription,
                        const size_t limit, const size_t offset, const LocalStorageManager::ListTagsOrder::type & order,
                        const LocalStorageManager::OrderDirection::type & orderDirection,
                        const QString & linkedNotebookGuid) const;
    bool expungeTag(Tag & tag, QNLocalizedString & errorDescription);
    bool expungeNotelessTagsFromLinkedNotebooks(QNLocalizedString & errorDescription);

    int enResourceCount(QNLocalizedString & errorDescription) const;
    bool addEnResource(IResource & resource, QNLocalizedString & errorDescription);
    bool updateEnResource(IResource & resource, QNLocalizedString & errorDescription);
    bool findEnResource(IResource & resource, QNLocalizedString & errorDescription, const bool withBinaryData = true) const;
    bool expungeEnResource(IResource & resource, QNLocalizedString & errorDescription);

    int savedSearchCount(QNLocalizedString & errorDescription) const;
    bool addSavedSearch(SavedSearch & search, QNLocalizedString & errorDescription);
    bool updateSavedSearch(SavedSearch & search, QNLocalizedString & errorDescription);
    bool findSavedSearch(SavedSearch & search, QNLocalizedString & errorDescription) const;
    QList<SavedSearch> listAllSavedSearches(QNLocalizedString & errorDescription, const size_t limit, const size_t offset,
                                            const LocalStorageManager::ListSavedSearchesOrder::type & order,
                                            const LocalStorageManager::OrderDirection::type & orderDirection) const;
    QList<SavedSearch> listSavedSearches(const LocalStorageManager::ListObjectsOptions flag,
                                         QNLocalizedString & errorDescription, const size_t limit,
                                         const size_t offset, const LocalStorageManager::ListSavedSearchesOrder::type & order,
                                         const LocalStorageManager::OrderDirection::type & orderDirection) const;
    bool expungeSavedSearch(SavedSearch & search, QNLocalizedString & errorDescription);

public Q_SLOTS:
    void processPostTransactionException(QNLocalizedString message, QSqlError error) const;

private:
    LocalStorageManagerPrivate() Q_DECL_EQ_DELETE;
    Q_DISABLE_COPY(LocalStorageManagerPrivate)

    void unlockDatabaseFile();

    bool createTables(QNLocalizedString & errorDescription);
    bool insertOrReplaceNotebookRestrictions(const qevercloud::NotebookRestrictions & notebookRestrictions,
                                             const QString & localUid, QNLocalizedString & errorDescription);
    bool insertOrReplaceSharedNotebook(const ISharedNotebook & sharedNotebook,
                                       QNLocalizedString & errorDescription);

    bool rowExists(const QString & tableName, const QString & uniqueKeyName,
                   const QVariant & uniqueKeyValue) const;

    bool insertOrReplaceUser(const IUser & user, QNLocalizedString & errorDescription);
    bool insertOrReplaceBusinessUserInfo(const UserID id, const qevercloud::BusinessUserInfo & info,
                                         QNLocalizedString & errorDescription);
    bool insertOrReplacePremiumInfo(const UserID id, const qevercloud::PremiumInfo & info,
                                    QNLocalizedString & errorDescription);
    bool insertOrReplaceAccounting(const UserID id, const qevercloud::Accounting & accounting,
                                   QNLocalizedString & errorDescription);
    bool insertOrReplaceUserAttributes(const UserID id, const qevercloud::UserAttributes & attributes,
                                       QNLocalizedString & errorDescription);
    bool checkAndPrepareUserCountQuery() const;
    bool checkAndPrepareInsertOrReplaceUserQuery();
    bool checkAndPrepareInsertOrReplaceAccountingQuery();
    bool checkAndPrepareInsertOrReplacePremiumUserInfoQuery();
    bool checkAndPrepareInsertOrReplaceBusinessUserInfoQuery();
    bool checkAndPrepareInsertOrReplaceUserAttributesQuery();
    bool checkAndPrepareInsertOrReplaceUserAttributesViewedPromotionsQuery();
    bool checkAndPrepareInsertOrReplaceUserAttributesRecentMailedAddressesQuery();
    bool checkAndPrepareDeleteUserQuery();

    bool insertOrReplaceNotebook(const Notebook & notebook, QNLocalizedString & errorDescription);
    bool checkAndPrepareNotebookCountQuery() const;
    bool checkAndPrepareInsertOrReplaceNotebookQuery();
    bool checkAndPrepareInsertOrReplaceNotebookRestrictionsQuery();
    bool checkAndPrepareInsertOrReplaceSharedNotebokQuery();

    bool insertOrReplaceLinkedNotebook(const LinkedNotebook & linkedNotebook, QNLocalizedString & errorDescription);
    bool checkAndPrepareGetLinkedNotebookCountQuery() const;
    bool checkAndPrepareInsertOrReplaceLinkedNotebookQuery();

    bool getNoteLocalUidFromResource(const IResource & resource, QString & noteLocalUid, QNLocalizedString & errorDescription);
    bool getNotebookLocalUidFromNote(const Note & note, QString & notebookLocalUid, QNLocalizedString & errorDescription);
    bool getNotebookGuidForNote(const Note & note, QString & notebookGuid, QNLocalizedString & errorDescription);
    bool getNotebookLocalUidForGuid(const QString & notebookGuid, QString & notebookLocalUid, QNLocalizedString & errorDescription);
    bool getNoteLocalUidForGuid(const QString & noteGuid, QString & noteLocalUid, QNLocalizedString & errorDescription);
    bool getTagLocalUidForGuid(const QString & tagGuid, QString & tagLocalUid, QNLocalizedString & errorDescription);
    bool getResourceLocalUidForGuid(const QString & resourceGuid, QString & resourceLocalUid, QNLocalizedString & errorDescription);
    bool getSavedSearchLocalUidForGuid(const QString & savedSearchGuid, QString & savedSearchLocalUid, QNLocalizedString & errorDescription);

    bool insertOrReplaceNote(const Note & note, const bool updateResources, const bool updateTags, QNLocalizedString & errorDescription);
    bool canAddNoteToNotebook(const QString & notebookLocalUid, QNLocalizedString & errorDescription);
    bool canUpdateNoteInNotebook(const QString & notebookLocalUid, QNLocalizedString & errorDescription);
    bool canExpungeNoteInNotebook(const QString & notebookLocalUid, QNLocalizedString & errorDescription);

    bool checkAndPrepareNoteCountQuery() const;
    bool checkAndPrepareInsertOrReplaceNoteQuery();
    bool checkAndPrepareCanAddNoteToNotebookQuery() const;
    bool checkAndPrepareCanUpdateNoteInNotebookQuery() const;
    bool checkAndPrepareCanExpungeNoteInNotebookQuery() const;
    bool checkAndPrepareInsertOrReplaceNoteIntoNoteTagsQuery();

    bool insertOrReplaceTag(const Tag & tag, QNLocalizedString & errorDescription);
    bool checkAndPrepareTagCountQuery() const;
    bool checkAndPrepareInsertOrReplaceTagQuery();
    bool checkAndPrepareDeleteTagQuery();

    bool insertOrReplaceResource(const IResource & resource, QNLocalizedString & errorDescription,
                                 const bool useSeparateTransaction = true);
    bool insertOrReplaceResourceAttributes(const QString & localUid,
                                           const qevercloud::ResourceAttributes & attributes,
                                           QNLocalizedString & errorDescription);
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

    bool insertOrReplaceSavedSearch(const SavedSearch & search, QNLocalizedString & errorDescription);
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
    bool fillUserFromSqlRecord(const QSqlRecord & rec, IUser & user, QNLocalizedString & errorDescription) const;
    void fillNoteFromSqlRecord(const QSqlRecord & record, Note & note) const;
    bool fillNoteTagIdFromSqlRecord(const QSqlRecord & record, const QString & column, QList<QPair<QString, int> > & tagIdsAndIndices,
                                    QHash<QString, int> & tagIndexPerId, QNLocalizedString & errorDescription) const;
    bool fillNotebookFromSqlRecord(const QSqlRecord & record, Notebook & notebook, QNLocalizedString & errorDescription) const;
    bool fillSharedNotebookFromSqlRecord(const QSqlRecord & record, ISharedNotebook & sharedNotebook,
                                         QNLocalizedString & errorDescription) const;
    bool fillLinkedNotebookFromSqlRecord(const QSqlRecord & record, LinkedNotebook & linkedNotebook,
                                         QNLocalizedString & errorDescription) const;
    bool fillSavedSearchFromSqlRecord(const QSqlRecord & rec, SavedSearch & search,
                                      QNLocalizedString & errorDescription) const;
    bool fillTagFromSqlRecord(const QSqlRecord & rec, Tag & tag,
                              QNLocalizedString & errorDescription) const;
    QList<Tag> fillTagsFromSqlQuery(QSqlQuery & query, QNLocalizedString & errorDescription) const;

    bool findAndSetTagIdsPerNote(Note & note, QNLocalizedString & errorDescription) const;
    bool findAndSetResourcesPerNote(Note & note, QNLocalizedString & errorDescription,
                                    const bool withBinaryData = true) const;
    void sortSharedNotebooks(Notebook & notebook) const;

    QList<qevercloud::SharedNotebook> listEnSharedNotebooksPerNotebookGuid(const QString & notebookGuid,
                                                                           QNLocalizedString & errorDescription) const;

    bool noteSearchQueryToSQL(const NoteSearchQuery & noteSearchQuery, QString & sql,
                              QNLocalizedString & errorDescription) const;

    bool noteSearchQueryContentSearchTermsToSQL(const NoteSearchQuery & noteSearchQuery,
                                                QString & sql, QNLocalizedString & errorDescription) const;

    void contentSearchTermToSQLQueryPart(QString & frontSearchTermModifier, QString & searchTerm,
                                         QString & backSearchTermModifier, QString & matchStatement) const;

    bool tagNamesToTagLocalUids(const QStringList & tagNames, QStringList & tagLocalUids,
                                QNLocalizedString & errorDescription) const;
    bool resourceMimeTypesToResourceLocalUids(const QStringList & resourceMimeTypes,
                                              QStringList & resourceLocalUids,
                                              QNLocalizedString & errorDescription) const;

    bool complementResourceNoteIds(IResource & resource, QNLocalizedString & errorDescription) const;

    bool partialUpdateNoteResources(const QString & noteLocalUid, const QList<ResourceAdapter> & updatedNoteResources,
                                    QNLocalizedString & errorDescription);

    template <class T>
    QString listObjectsOptionsToSqlQueryConditions(const LocalStorageManager::ListObjectsOptions & flag,
                                                   QNLocalizedString & errorDescription) const;

    template <class T, class TOrderBy>
    QList<T> listObjects(const LocalStorageManager::ListObjectsOptions & flag,
                         QNLocalizedString & errorDescription, const size_t limit,
                         const size_t offset, const TOrderBy & orderBy,
                         const LocalStorageManager::OrderDirection::type & orderDirection,
                         const QString & additionalSqlQueryCondition = QString()) const;

    template <class T>
    QString listObjectsGenericSqlQuery() const;

    template <class TOrderBy>
    QString orderByToSqlTableColumn(const TOrderBy & orderBy) const;

    template <class T>
    bool fillObjectsFromSqlQuery(QSqlQuery query, QList<T> & objects, QNLocalizedString & errorDescription) const;

    template <class T>
    bool fillObjectFromSqlRecord(const QSqlRecord & record, T & object, QNLocalizedString & errorDescription) const;

    void clearDatabaseFile();

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

    mutable QSqlQuery   m_getNoteCountQuery;
    mutable bool        m_getNoteCountQueryPrepared;

    QSqlQuery           m_insertOrReplaceNoteQuery;
    bool                m_insertOrReplaceNoteQueryPrepared;

    mutable QSqlQuery   m_canAddNoteToNotebookQuery;
    mutable bool        m_canAddNoteToNotebookQueryPrepared;

    mutable QSqlQuery   m_canUpdateNoteInNotebookQuery;
    mutable bool        m_canUpdateNoteInNotebookQueryPrepared;

    mutable QSqlQuery   m_canExpungeNoteInNotebookQuery;
    mutable bool        m_canExpungeNoteInNotebookQueryPrepared;

    QSqlQuery           m_insertOrReplaceNoteIntoNoteTagsQuery;
    bool                m_insertOrReplaceNoteIntoNoteTagsQueryPrepared;

    mutable QSqlQuery   m_getLinkedNotebookCountQuery;
    mutable bool        m_getLinkedNotebookCountQueryPrepared;

    QSqlQuery           m_insertOrReplaceLinkedNotebookQuery;
    bool                m_insertOrReplaceLinkedNotebookQueryPrepared;

    mutable QSqlQuery   m_getNotebookCountQuery;
    mutable bool        m_getNotebookCountQueryPrepared;

    QSqlQuery           m_insertOrReplaceNotebookQuery;
    bool                m_insertOrReplaceNotebookQueryPrepared;

    QSqlQuery           m_insertOrReplaceNotebookRestrictionsQuery;
    bool                m_insertOrReplaceNotebookRestrictionsQueryPrepared;

    QSqlQuery		    m_insertOrReplaceSharedNotebookQuery;
    bool		        m_insertOrReplaceSharedNotebookQueryPrepared;

    mutable QSqlQuery   m_getUserCountQuery;
    mutable bool        m_getUserCountQueryPrepared;

    QSqlQuery           m_insertOrReplaceUserQuery;
    bool                m_insertOrReplaceUserQueryPrepared;

    QSqlQuery           m_insertOrReplaceUserAttributesQuery;
    bool                m_insertOrReplaceUserAttributesQueryPrepared;

    QSqlQuery           m_insertOrReplaceAccountingQuery;
    bool                m_insertOrReplaceAccountingQueryPrepared;

    QSqlQuery           m_insertOrReplacePremiumUserInfoQuery;
    bool                m_insertOrReplacePremiumUserInfoQueryPrepared;

    QSqlQuery           m_insertOrReplaceBusinessUserInfoQuery;
    bool                m_insertOrReplaceBusinessUserInfoQueryPrepared;

    QSqlQuery           m_insertOrReplaceUserAttributesViewedPromotionsQuery;
    bool                m_insertOrReplaceUserAttributesViewedPromotionsQueryPrepared;

    QSqlQuery           m_insertOrReplaceUserAttributesRecentMailedAddressesQuery;
    bool                m_insertOrReplaceUserAttributesRecentMailedAddressesQueryPrepared;

    QSqlQuery           m_deleteUserQuery;
    bool                m_deleteUserQueryPrepared;

    StringUtils         m_stringUtils;
    QVector<QChar>      m_preservedAsterisk;
};

} // namespace quentier

#endif // LIB_QUENTIER_LOCAL_STORAGE_LOCAL_STORAGE_MANAGER_PRIVATE_H
