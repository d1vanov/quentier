#ifndef LIB_QUENTIER_LOCAL_STORAGE_LOCAL_STORAGE_MANAGER_PRIVATE_H
#define LIB_QUENTIER_LOCAL_STORAGE_LOCAL_STORAGE_MANAGER_PRIVATE_H

#include <quentier/local_storage/Lists.h>
#include <quentier/local_storage/LocalStorageManager.h>
#include <quentier/types/User.h>
#include <quentier/types/Notebook.h>
#include <quentier/types/SharedNotebook.h>
#include <quentier/types/Note.h>
#include <quentier/types/Tag.h>
#include <quentier/types/Resource.h>
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

QT_FORWARD_DECLARE_CLASS(NoteSearchQuery)

class LocalStorageManagerPrivate: public QObject
{
    Q_OBJECT
public:
    LocalStorageManagerPrivate(const Account & account, const bool startFromScratch, const bool overrideLock);
    ~LocalStorageManagerPrivate();

Q_SIGNALS:
    void upgradeProgress(double progress);

public:
    void switchUser(const Account & account, const bool startFromScratch = false,
                    const bool overrideLock = false);
    int userCount(QNLocalizedString & errorDescription) const;
    bool addUser(const User & user, QNLocalizedString & errorDescription);
    bool updateUser(const User & user, QNLocalizedString & errorDescription);
    bool findUser(User & user, QNLocalizedString & errorDescription) const;
    bool deleteUser(const User & user, QNLocalizedString & errorDescription);
    bool expungeUser(const User & user, QNLocalizedString & errorDescription);

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
    QList<SharedNotebook> listAllSharedNotebooks(QNLocalizedString & errorDescription) const;
    QList<SharedNotebook> listSharedNotebooksPerNotebookGuid(const QString & notebookGuid,
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
    bool addEnResource(Resource & resource, QNLocalizedString & errorDescription);
    bool updateEnResource(Resource & resource, QNLocalizedString & errorDescription);
    bool findEnResource(Resource & resource, QNLocalizedString & errorDescription, const bool withBinaryData = true) const;
    bool expungeEnResource(Resource & resource, QNLocalizedString & errorDescription);

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
    bool insertOrReplaceNotebookRestrictions(const QString & localUid, const qevercloud::NotebookRestrictions & notebookRestrictions,
                                             QNLocalizedString & errorDescription);
    bool insertOrReplaceSharedNotebook(const SharedNotebook & sharedNotebook,
                                       QNLocalizedString & errorDescription);

    bool rowExists(const QString & tableName, const QString & uniqueKeyName,
                   const QVariant & uniqueKeyValue) const;

    bool insertOrReplaceUser(const User & user, QNLocalizedString & errorDescription);
    bool insertOrReplaceBusinessUserInfo(const qevercloud::UserID id, const qevercloud::BusinessUserInfo & info,
                                         QNLocalizedString & errorDescription);
    bool insertOrReplaceAccounting(const qevercloud::UserID id, const qevercloud::Accounting & accounting,
                                   QNLocalizedString & errorDescription);
    bool insertOrReplaceAccountLimits(const qevercloud::UserID id, const qevercloud::AccountLimits & accountLimits,
                                      QNLocalizedString & errorDescription);
    bool insertOrReplaceUserAttributes(const qevercloud::UserID id, const qevercloud::UserAttributes & attributes,
                                       QNLocalizedString & errorDescription);
    bool checkAndPrepareUserCountQuery() const;
    bool checkAndPrepareInsertOrReplaceUserQuery();
    bool checkAndPrepareInsertOrReplaceAccountingQuery();
    bool checkAndPrepareInsertOrReplaceAccountLimitsQuery();
    bool checkAndPrepareInsertOrReplaceBusinessUserInfoQuery();
    bool checkAndPrepareInsertOrReplaceUserAttributesQuery();
    bool checkAndPrepareInsertOrReplaceUserAttributesViewedPromotionsQuery();
    bool checkAndPrepareInsertOrReplaceUserAttributesRecentMailedAddressesQuery();
    bool checkAndPrepareDeleteUserQuery();

    bool insertOrReplaceNotebook(const Notebook & notebook, QNLocalizedString & errorDescription);
    bool checkAndPrepareNotebookCountQuery() const;
    bool checkAndPrepareInsertOrReplaceNotebookQuery();
    bool checkAndPrepareInsertOrReplaceNotebookRestrictionsQuery();
    bool checkAndPrepareInsertOrReplaceSharedNotebookQuery();

    bool insertOrReplaceLinkedNotebook(const LinkedNotebook & linkedNotebook, QNLocalizedString & errorDescription);
    bool checkAndPrepareGetLinkedNotebookCountQuery() const;
    bool checkAndPrepareInsertOrReplaceLinkedNotebookQuery();

    bool getNoteLocalUidFromResource(const Resource & resource, QString & noteLocalUid, QNLocalizedString & errorDescription);
    bool getNotebookLocalUidFromNote(const Note & note, QString & notebookLocalUid, QNLocalizedString & errorDescription);
    bool getNotebookGuidForNote(const Note & note, QString & notebookGuid, QNLocalizedString & errorDescription);
    bool getNotebookLocalUidForGuid(const QString & notebookGuid, QString & notebookLocalUid, QNLocalizedString & errorDescription);
    bool getNoteLocalUidForGuid(const QString & noteGuid, QString & noteLocalUid, QNLocalizedString & errorDescription);
    bool getTagLocalUidForGuid(const QString & tagGuid, QString & tagLocalUid, QNLocalizedString & errorDescription);
    bool getResourceLocalUidForGuid(const QString & resourceGuid, QString & resourceLocalUid, QNLocalizedString & errorDescription);
    bool getSavedSearchLocalUidForGuid(const QString & savedSearchGuid, QString & savedSearchLocalUid, QNLocalizedString & errorDescription);

    bool insertOrReplaceNote(const Note & note, const bool updateResources, const bool updateTags, QNLocalizedString & errorDescription);
    bool insertOrReplaceSharedNote(const SharedNote & sharedNote, QNLocalizedString & errorDescription);
    bool insertOrReplaceNoteRestrictions(const QString & noteLocalUid, const qevercloud::NoteRestrictions & noteRestrictions,
                                         QNLocalizedString & errorDescription);
    bool insertOrReplaceNoteLimits(const QString & noteLocalUid, const qevercloud::NoteLimits & noteLimits,
                                   QNLocalizedString & errorDescription);
    bool canAddNoteToNotebook(const QString & notebookLocalUid, QNLocalizedString & errorDescription);
    bool canUpdateNoteInNotebook(const QString & notebookLocalUid, QNLocalizedString & errorDescription);
    bool canExpungeNoteInNotebook(const QString & notebookLocalUid, QNLocalizedString & errorDescription);

    bool checkAndPrepareNoteCountQuery() const;
    bool checkAndPrepareInsertOrReplaceNoteQuery();
    bool checkAndPrepareInsertOrReplaceSharedNoteQuery();
    bool checkAndPrepareInsertOrReplaceNoteRestrictionsQuery();
    bool checkAndPrepareInsertOrReplaceNoteLimitsQuery();
    bool checkAndPrepareCanAddNoteToNotebookQuery() const;
    bool checkAndPrepareCanUpdateNoteInNotebookQuery() const;
    bool checkAndPrepareCanExpungeNoteInNotebookQuery() const;
    bool checkAndPrepareInsertOrReplaceNoteIntoNoteTagsQuery();

    bool insertOrReplaceTag(const Tag & tag, QNLocalizedString & errorDescription);
    bool checkAndPrepareTagCountQuery() const;
    bool checkAndPrepareInsertOrReplaceTagQuery();
    bool checkAndPrepareDeleteTagQuery();

    bool insertOrReplaceResource(const Resource & resource, QNLocalizedString & errorDescription,
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

    void fillResourceFromSqlRecord(const QSqlRecord & rec, const bool withBinaryData, Resource & resource) const;
    bool fillResourceAttributesFromSqlRecord(const QSqlRecord & rec, qevercloud::ResourceAttributes & attributes) const;
    bool fillResourceAttributesApplicationDataKeysOnlyFromSqlRecord(const QSqlRecord & rec, qevercloud::ResourceAttributes & attributes) const;
    bool fillResourceAttributesApplicationDataFullMapFromSqlRecord(const QSqlRecord & rec, qevercloud::ResourceAttributes & attributes) const;
    void fillNoteAttributesFromSqlRecord(const QSqlRecord & rec, qevercloud::NoteAttributes & attributes) const;
    void fillNoteAttributesApplicationDataKeysOnlyFromSqlRecord(const QSqlRecord & rec, qevercloud::NoteAttributes & attributes) const;
    void fillNoteAttributesApplicationDataFullMapFromSqlRecord(const QSqlRecord & rec, qevercloud::NoteAttributes & attributes) const;
    void fillNoteAttributesClassificationsFromSqlRecord(const QSqlRecord & rec, qevercloud::NoteAttributes & attributes) const;
    bool fillUserFromSqlRecord(const QSqlRecord & rec, User & user, QNLocalizedString & errorDescription) const;
    bool fillNoteFromSqlRecord(const QSqlRecord & record, Note & note, QNLocalizedString & errorDescription) const;
    bool fillSharedNoteFromSqlRecord(const QSqlRecord & record, SharedNote & sharedNote, QNLocalizedString & errorDescription) const;
    bool fillNoteTagIdFromSqlRecord(const QSqlRecord & record, const QString & column, QList<QPair<QString, int> > & tagIdsAndIndices,
                                    QHash<QString, int> & tagIndexPerId, QNLocalizedString & errorDescription) const;
    bool fillNotebookFromSqlRecord(const QSqlRecord & record, Notebook & notebook, QNLocalizedString & errorDescription) const;
    bool fillSharedNotebookFromSqlRecord(const QSqlRecord & record, SharedNotebook & sharedNotebook,
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
    void sortSharedNotes(Note & note) const;

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

    bool complementResourceNoteIds(Resource & resource, QNLocalizedString & errorDescription) const;

    bool partialUpdateNoteResources(const QString & noteLocalUid, const QList<Resource> & updatedNoteResources,
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

    struct SharedNotebookCompareByIndex
    {
        bool operator()(const SharedNotebook & lhs, const SharedNotebook & rhs) const;
    };

    struct SharedNoteCompareByIndex
    {
        bool operator()(const SharedNote & lhs, const SharedNote & rhs) const;
    };

    struct ResourceCompareByIndex
    {
        bool operator()(const Resource & lhs, const Resource & rhs) const;
    };

    struct QStringIntPairCompareByInt
    {
        bool operator()(const QPair<QString, int> & lhs, const QPair<QString, int> & rhs) const;
    };

    Account             m_currentAccount;
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

    QSqlQuery           m_insertOrReplaceSharedNoteQuery;
    bool                m_insertOrReplaceSharedNoteQueryPrepared;

    QSqlQuery           m_insertOrReplaceNoteRestrictionsQuery;
    bool                m_insertOrReplaceNoteRestrictionsQueryPrepared;

    QSqlQuery           m_insertOrReplaceNoteLimitsQuery;
    bool                m_insertOrReplaceNoteLimitsQueryPrepared;

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

    QSqlQuery           m_insertOrReplaceSharedNotebookQuery;
    bool                m_insertOrReplaceSharedNotebookQueryPrepared;

    mutable QSqlQuery   m_getUserCountQuery;
    mutable bool        m_getUserCountQueryPrepared;

    QSqlQuery           m_insertOrReplaceUserQuery;
    bool                m_insertOrReplaceUserQueryPrepared;

    QSqlQuery           m_insertOrReplaceUserAttributesQuery;
    bool                m_insertOrReplaceUserAttributesQueryPrepared;

    QSqlQuery           m_insertOrReplaceAccountingQuery;
    bool                m_insertOrReplaceAccountingQueryPrepared;

    QSqlQuery           m_insertOrReplaceAccountLimitsQuery;
    bool                m_insertOrReplaceAccountLimitsQueryPrepared;

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
