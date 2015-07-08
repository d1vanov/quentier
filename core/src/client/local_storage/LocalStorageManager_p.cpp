#include "LocalStorageManager_p.h"
#include "DatabaseOpeningException.h"
#include "DatabaseSqlErrorException.h"
#include "Transaction.h"
#include "NoteSearchQuery.h"
#include <client/Utility.h>
#include <logging/QuteNoteLogger.h>
#include <tools/DesktopServices.h>
#include <tools/SysInfo.h>
#include <algorithm>

namespace qute_note {

#define QUTE_NOTE_DATABASE_NAME "qn.storage.sqlite"

LocalStorageManagerPrivate::LocalStorageManagerPrivate(const QString & username, const UserID userId,
                                                       const bool startFromScratch) :
    QObject(),
    // NOTE: don't initialize these! Otherwise SwitchUser won't work right
    m_currentUsername(),
    m_currentUserId(),
    m_applicationPersistenceStoragePath(),
    m_sqlDatabase(),
    m_insertOrReplaceSavedSearchQuery(),
    m_insertOrReplaceSavedSearchQueryPrepared(false),
    m_getSavedSearchCountQuery(),
    m_getSavedSearchCountQueryPrepared(false),
    m_expungeSavedSearchQuery(),
    m_expungeSavedSearchQueryPrepared(false),
    m_insertOrReplaceResourceQuery(),
    m_insertOrReplaceResourceQueryPrepared(false),
    m_insertOrReplaceNoteResourceQuery(),
    m_insertOrReplaceNoteResourceQueryPrepared(false),
    m_deleteResourceFromResourceRecognitionTypesQuery(),
    m_deleteResourceFromResourceRecognitionTypesQueryPrepared(false),
    m_insertOrReplaceIntoResourceRecognitionTypesQuery(),
    m_insertOrReplaceIntoResourceRecognitionTypesQueryPrepared(false),
    m_deleteResourceFromResourceAttributesQuery(),
    m_deleteResourceFromResourceAttributesQueryPrepared(false),
    m_deleteResourceFromResourceAttributesApplicationDataKeysOnlyQuery(),
    m_deleteResourceFromResourceAttributesApplicationDataKeysOnlyQueryPrepared(false),
    m_deleteResourceFromResourceAttributesApplicationDataFullMapQuery(),
    m_deleteResourceFromResourceAttributesApplicationDataFullMapQueryPrepared(false),
    m_insertOrReplaceResourceAttributesQuery(),
    m_insertOrReplaceResourceAttributesQueryPrepared(false),
    m_insertOrReplaceResourceAttributeApplicationDataKeysOnlyQuery(),
    m_insertOrReplaceResourceAttributeApplicationDataKeysOnlyQueryPrepared(false),
    m_insertOrReplaceResourceAttributeApplicationDataFullMapQuery(),
    m_insertOrReplaceResourceAttributeApplicationDataFullMapQueryPrepared(false),
    m_getResourceCountQuery(),
    m_getResourceCountQueryPrepared(false),
    m_getTagCountQuery(),
    m_getTagCountQueryPrepared(false),
    m_insertOrReplaceTagQuery(),
    m_insertOrReplaceTagQueryPrepared(false),
    m_deleteTagQuery(),
    m_deleteTagQueryPrepared(false),
    m_expungeTagQuery(),
    m_expungeTagQueryPrepared(false),
    m_getNoteCountQuery(),
    m_getNoteCountQueryPrepared(false),
    m_insertOrReplaceNoteQuery(),
    m_insertOrReplaceNoteQueryPrepared(false),
    m_expungeNoteFromNoteTagsQuery(),
    m_expungeNoteFromNoteTagsQueryPrepared(false),
    m_insertOrReplaceNoteIntoNoteTagsQuery(),
    m_insertOrReplaceNoteIntoNoteTagsQueryPrepared(false),
    m_expungeResourceByNoteQuery(),
    m_expungeResourceByNoteQueryPrepared(false),
    m_getLinkedNotebookCountQuery(),
    m_getLinkedNotebookCountQueryPrepared(false),
    m_insertOrReplaceLinkedNotebookQuery(),
    m_insertOrReplaceLinkedNotebookQueryPrepared(false),
    m_expungeLinkedNotebookQuery(),
    m_expungeLinkedNotebookQueryPrepared(false),
    m_getNotebookCountQuery(),
    m_getNotebookCountQueryPrepared(false),
    m_insertOrReplaceNotebookQuery(),
    m_insertOrReplaceNotebookQueryPrepared(false),
    m_expungeNotebookFromNotebookRestrictionsQuery(),
    m_expungeNotebookFromNotebookRestrictionsQueryPrepared(false),
    m_insertOrReplaceNotebookRestrictionsQuery(),
    m_insertOrReplaceNotebookRestrictionsQueryPrepared(false),
    m_expungeSharedNotebooksQuery(),
    m_expungeSharedNotebooksQueryPrepared(false),
    m_insertOrReplaceSharedNotebookQuery(),
    m_insertOrReplaceSharedNotebookQueryPrepared(false),
    m_getUserCountQuery(),
    m_getUserCountQueryPrepared(false),
    m_insertOrReplaceUserQuery(),
    m_insertOrReplaceUserQueryPrepared(false),
    m_expungeUserAttributesQuery(),
    m_expungeUserAttributesQueryPrepared(false),
    m_insertOrReplaceUserAttributesQuery(),
    m_insertOrReplaceUserAttributesQueryPrepared(false),
    m_expungeAccountingQuery(),
    m_expungeAccountingQueryPrepared(false),
    m_insertOrReplaceAccountingQuery(),
    m_insertOrReplaceAccountingQueryPrepared(false),
    m_expungePremiumUserInfoQuery(),
    m_expungePremiumUserInfoQueryPrepared(false),
    m_insertOrReplacePremiumUserInfoQuery(),
    m_insertOrReplacePremiumUserInfoQueryPrepared(false),
    m_expungeBusinessUserInfoQuery(),
    m_expungeBusinessUserInfoQueryPrepared(false),
    m_insertOrReplaceBusinessUserInfoQuery(),
    m_insertOrReplaceBusinessUserInfoQueryPrepared(false),
    m_expungeUserAttributesViewedPromotionsQuery(),
    m_expungeUserAttributesViewedPromotionsQueryPrepared(false),
    m_insertOrReplaceUserAttributesViewedPromotionsQuery(),
    m_insertOrReplaceUserAttributesViewedPromotionsQueryPrepared(false),
    m_expungeUserAttributesRecentMailedAddressesQuery(),
    m_expungeUserAttributesRecentMailedAddressesQueryPrepared(false),
    m_insertOrReplaceUserAttributesRecentMailedAddressesQuery(),
    m_insertOrReplaceUserAttributesRecentMailedAddressesQueryPrepared(false),
    m_deleteUserQuery(),
    m_deleteUserQueryPrepared(false),
    m_expungeUserQuery(),
    m_expungeUserQueryPrepared(false)
{
    SwitchUser(username, userId, startFromScratch);
}

LocalStorageManagerPrivate::~LocalStorageManagerPrivate()
{
    if (m_sqlDatabase.isOpen()) {
        m_sqlDatabase.close();
    }
}

#define DATABASE_CHECK_AND_SET_ERROR(errorPrefix) \
    if (!res) { \
        errorDescription += QT_TR_NOOP("Internal error: "); \
        errorDescription += errorPrefix; \
        errorDescription += ": "; \
        QNCRITICAL(errorDescription << query.lastError() << ", last executed query: " << query.lastQuery()); \
        errorDescription += query.lastError().text(); \
        return false; \
    }

bool LocalStorageManagerPrivate::AddUser(const IUser & user, QString & errorDescription)
{
    errorDescription = QT_TR_NOOP("Can't add user into local storage database: ");
    QString error;

    bool res = user.checkParameters(error);
    if (!res) {
        errorDescription += error;
        QNWARNING("Found invalid user: " << user << "\nError: " << error);
        return false;
    }

    QString userId = QString::number(user.id());
    bool exists = RowExists("Users", "id", QVariant(userId));
    if (exists) {
        errorDescription += QT_TR_NOOP("user with the same id already exists");
        QNWARNING(errorDescription << ", id: " << userId);
        return false;
    }

    error.resize(0);
    res = InsertOrReplaceUser(user, error);
    if (!res) {
        errorDescription += error;
        QNWARNING(errorDescription);
        return false;
    }

    return true;
}

bool LocalStorageManagerPrivate::UpdateUser(const IUser & user, QString & errorDescription)
{
    errorDescription = QT_TR_NOOP("Can't update user in local storage database: ");
    QString error;

    bool res = user.checkParameters(error);
    if (!res) {
        errorDescription += error;
        QNWARNING("Found invalid user: " << user << "\nError: " << error);
        return false;
    }

    QString userId = QString::number(user.id());
    bool exists = RowExists("Users", "id", QVariant(userId));
    if (!exists) {
        // TRANSLATOR explaining the reason of error
        errorDescription += QT_TR_NOOP("user id was not found");
        QNWARNING(errorDescription << ", id: " << userId);
        return false;
    }

    error.resize(0);
    res = InsertOrReplaceUser(user, error);
    if (!res) {
        errorDescription += error;
        QNWARNING(errorDescription);
        return false;
    }

    return true;
}

bool LocalStorageManagerPrivate::FindUser(IUser & user, QString & errorDescription) const
{
    QNDEBUG("LocalStorageManagerPrivate::FindUser");

    errorDescription = QT_TR_NOOP("Can't find user in local storage database: ");

    if (!user.hasId()) {
        // TRANSLATOR explaining the reason of error
        errorDescription += QT_TR_NOOP("user id is not set");
        QNWARNING(errorDescription);
        return false;
    }

    qint32 id = user.id();
    QString userId = QString::number(id);
    QNDEBUG("Looking for user with id = " << userId);

    QString queryString = QString("SELECT * FROM Users LEFT OUTER JOIN UserAttributes "
                                  "ON Users.id = UserAttributes.id "
                                  "LEFT OUTER JOIN UserAttributesViewedPromotions "
                                  "ON Users.id = UserAttributesViewedPromotions.id "
                                  "LEFT OUTER JOIN UserAttributesRecentMailedAddresses "
                                  "ON Users.id = UserAttributesRecentMailedAddresses.id "
                                  "LEFT OUTER JOIN Accounting ON Users.id = Accounting.id "
                                  "LEFT OUTER JOIN PremiumInfo ON Users.id = PremiumInfo.id "
                                  "LEFT OUTER JOIN BusinessUserInfo ON Users.id = BusinessUserInfo.id "
                                  "WHERE Users.id = %1").arg(userId);
    QSqlQuery query(m_sqlDatabase);
    bool res = query.exec(queryString);
    DATABASE_CHECK_AND_SET_ERROR("can't select user from SQL database");

    size_t counter = 0;
    while(query.next()) {
        QSqlRecord rec = query.record();
        res = FillUserFromSqlRecord(rec, user, errorDescription);
        if (!res) {
            return false;
        }
        ++counter;
    }

    if (!counter) {
        errorDescription += QT_TR_NOOP("no user was found in local storage");
        QNDEBUG(errorDescription);
        return false;
    }

    return true;
}

bool LocalStorageManagerPrivate::DeleteUser(const IUser & user, QString & errorDescription)
{
    errorDescription = QT_TR_NOOP("Can't delete user from local storage database: ");

    if (!user.hasDeletionTimestamp()) {
        // TRANSLATOR explaining the reason of error
        errorDescription += QT_TR_NOOP("deletion timestamp is not set");
        QNWARNING(errorDescription);
        return false;
    }

    if (!user.hasId()) {
        // TRANSLATOR explaining the reason of error
        errorDescription += QT_TR_NOOP("user id is not set");
        QNWARNING(errorDescription);
        return false;
    }

    bool res = CheckAndPrepareDeleteUserQuery();
    QSqlQuery & query = m_deleteUserQuery;
    DATABASE_CHECK_AND_SET_ERROR("can't update deletion timestamp in \"Users\" table in SQL database: "
                                 "can't prepare SQL query");

    query.bindValue(":userDeletionTimestamp", user.deletionTimestamp());
    query.bindValue(":userIsLocal", (user.isLocal() ? 1 : 0));
    query.bindValue(":id", user.id());

    res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("can't update deletion timestamp in \"Users\" table in SQL database");

    return true;
}

bool LocalStorageManagerPrivate::ExpungeUser(const IUser & user, QString & errorDescription)
{
    errorDescription = QT_TR_NOOP("Can't expunge user from local storage database: ");

    if (!user.hasId()) {
        // TRANSLATOR explaining the reason of error
        errorDescription += QT_TR_NOOP("user id is not set");
        QNWARNING(errorDescription);
        return false;
    }

    bool res = CheckAndPrepareExpungeUserQuery();
    QSqlQuery & query = m_expungeUserQuery;
    DATABASE_CHECK_AND_SET_ERROR("can't delete entry from \"Users\" table in SQL database: can't prepare SQL query");

    query.bindValue(":id", user.id());

    res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("can't delete entry from \"Users\" table in SQL database");

    return true;
}

int LocalStorageManagerPrivate::GetNotebookCount(QString & errorDescription) const
{
    bool res = CheckAndPrepareGetNotebookCountQuery();
    QSqlQuery & query = m_getNotebookCountQuery;
    if (!res) {
        errorDescription = QT_TR_NOOP("Internal error: can't get number of notebooks: "
                                      "can't prepare SQL query");
        QNCRITICAL(errorDescription << query.lastError() << ", last query: " << query.lastQuery());
        errorDescription += query.lastError().text();
        return -1;
    }

    res = query.exec();
    if (!res) {
        errorDescription = QT_TR_NOOP("Internal error: can't get number of notebooks in local storage database: ");
        QNCRITICAL(errorDescription << query.lastError() << ", last query: " << query.lastQuery());
        errorDescription += query.lastError().text();
        return -1;
    }

    if (!query.next()) {
        QNDEBUG("Found no notebooks in local storage database");
        return 0;
    }

    bool conversionResult = false;
    int count = query.value(0).toInt(&conversionResult);
    if (!conversionResult) {
        errorDescription = QT_TR_NOOP("Internal error: can't convert number of notebooks to int");
        QNCRITICAL(errorDescription << ": " << query.value(0));
        return -1;
    }

    return count;
}

void LocalStorageManagerPrivate::SwitchUser(const QString & username, const UserID userId,
                                     const bool startFromScratch)
{
    if ( (username == m_currentUsername) &&
         (userId == m_currentUserId) )
    {
        return;
    }

    m_currentUsername = username;
    m_currentUserId = userId;

    m_applicationPersistenceStoragePath = applicationPersistentStoragePath();
    m_applicationPersistenceStoragePath.append("/" + m_currentUsername + QString::number(m_currentUserId));

    QDir databaseFolder(m_applicationPersistenceStoragePath);
    if (!databaseFolder.exists())
    {
        if (!databaseFolder.mkpath(m_applicationPersistenceStoragePath)) {
            throw DatabaseOpeningException(QT_TR_NOOP("Cannot create folder to store local storage database."));
        }
    }

    QString sqlDriverName = "QSQLITE";
    bool isSqlDriverAvailable = QSqlDatabase::isDriverAvailable(sqlDriverName);
    if (!isSqlDriverAvailable)
    {
        QString error = "SQL driver " + sqlDriverName + " is not available. Available drivers: ";
        const QStringList drivers = QSqlDatabase::drivers();
        foreach(const QString & driver, drivers) {
            error += "{" + driver + "} ";
        }
        throw DatabaseSqlErrorException(error);
    }

    QString sqlDatabaseConnectionName = "qute_note_sqlite_connection";
    if (!QSqlDatabase::contains(sqlDatabaseConnectionName)) {
        m_sqlDatabase = QSqlDatabase::addDatabase(sqlDriverName, sqlDatabaseConnectionName);
    }
    else {
        m_sqlDatabase = QSqlDatabase::database(sqlDatabaseConnectionName);
    }

    QString databaseFileName = m_applicationPersistenceStoragePath + QString("/") +
                               QString(QUTE_NOTE_DATABASE_NAME);
    QNDEBUG("Attempting to open or create database file: " + databaseFileName);

    QFile databaseFile(databaseFileName);

    if (!databaseFile.exists())
    {
        if (!databaseFile.open(QIODevice::ReadWrite)) {
            throw DatabaseOpeningException(QT_TR_NOOP("Cannot create local storage database: ") +
                                           databaseFile.errorString());
        }
    }
    else if (startFromScratch) {
        QNDEBUG("Cleaning up the whole database for user with name " << m_currentUsername
                << " and id " << QString::number(m_currentUserId));
        databaseFile.resize(0);
        databaseFile.flush();
    }

    m_sqlDatabase.setHostName("localhost");
    m_sqlDatabase.setUserName(username);
    m_sqlDatabase.setPassword(username);
    m_sqlDatabase.setDatabaseName(databaseFileName);

    if (!m_sqlDatabase.open())
    {
        QString lastErrorText = m_sqlDatabase.lastError().text();
        throw DatabaseOpeningException(QT_TR_NOOP("Cannot open local storage database: ") +
                                       lastErrorText);
    }

    QSqlQuery query(m_sqlDatabase);
    if (!query.exec("PRAGMA foreign_keys = ON")) {
        throw DatabaseSqlErrorException(QT_TR_NOOP("Cannot set foreign_keys = ON pragma "
                                                   "for SQL local storage database"));
    }

    qint64 pageSize = SysInfo::GetSingleton().GetPageSize();
    QString pageSizeQuery = QString("PRAGMA page_size = %1").arg(QString::number(pageSize));
    if (!query.exec(pageSizeQuery)) {
        throw DatabaseSqlErrorException(QT_TR_NOOP("Cannot set page_size pragma "
                                                   "for SQL local storage database"));
    }

    QString errorDescription;
    if (!CreateTables(errorDescription)) {
        throw DatabaseSqlErrorException(QT_TR_NOOP("Cannot initialize tables in SQL database: ") +
                                        errorDescription);
    }
}

int LocalStorageManagerPrivate::GetUserCount(QString & errorDescription) const
{
    bool res = CheckAndPrepareGetUserCountQuery();
    QSqlQuery & query = m_getUserCountQuery;
    if (!res) {
        errorDescription = QT_TR_NOOP("Internal error: can't get number of users: "
                                      "can't prepare SQL query");
        QNCRITICAL(errorDescription << query.lastError() << ", last query: " << query.lastQuery());
        errorDescription += query.lastError().text();
        return -1;
    }

    res = query.exec();
    if (!res) {
        errorDescription = QT_TR_NOOP("Internal error: can't get number of users in local storage database: ");
        QNCRITICAL(errorDescription << query.lastError() << ", last query: " << query.lastQuery());
        errorDescription += query.lastError().text();
        return -1;
    }

    if (!query.next()) {
        QNDEBUG("Found no users in local storage database");
        return 0;
    }

    bool conversionResult = false;
    int count = query.value(0).toInt(&conversionResult);
    if (!conversionResult) {
        errorDescription = QT_TR_NOOP("Internal error: can't convert number of users to int");
        QNCRITICAL(errorDescription << ": " << query.value(0));
        return -1;
    }

    return count;
}

bool LocalStorageManagerPrivate::AddNotebook(const Notebook & notebook, QString & errorDescription)
{
    errorDescription = QT_TR_NOOP("Can't add notebook to local storage database: ");
    QString error;

    bool res = notebook.checkParameters(error);
    if (!res) {
        errorDescription += error;
        QNWARNING("Found invalid notebook: " << notebook << "\nError: " << error);
        return false;
    }

    QString localGuid = notebook.localGuid();

    QString column, guid;
    bool shouldCheckRowExistence = true;

    bool notebookHasGuid = notebook.hasGuid();
    if (notebookHasGuid)
    {
        column = "guid";
        guid = notebook.guid();

        if (localGuid.isEmpty()) {
            QString queryString = QString("SELECT localGuid FROM Notebooks WHERE guid = '%1'").arg(guid);
            QSqlQuery query(m_sqlDatabase);
            res = query.exec(queryString);
            DATABASE_CHECK_AND_SET_ERROR("can't find local guid corresponding to Notebook's guid");

            if (query.next()) {
                localGuid = query.record().value("localGuid").toString();
            }

            if (!localGuid.isEmpty()) {
                errorDescription += QT_TR_NOOP("found existing local guid corresponding to Notebook's guid");
                QNCRITICAL(errorDescription << ", guid: " << guid);
                return false;
            }

            localGuid = QUuid::createUuid().toString();
            shouldCheckRowExistence = false;
        }
    }
    else {
        column = "localGuid";
        guid = localGuid;
    }

    if (shouldCheckRowExistence && RowExists("Notebooks", column, QVariant(guid))) {
        // TRANSLATOR explaining the reason of error
        errorDescription += QT_TR_NOOP("notebook with specified ");
        errorDescription += column;
        // TRANSLATOR previous part of the phrase was "notebook with specified "
        errorDescription += QT_TR_NOOP(" already exists in local storage");
        QNWARNING(errorDescription << ", " << column << ": " << guid);
        return false;
    }

    return InsertOrReplaceNotebook(notebook, localGuid, errorDescription);
}

bool LocalStorageManagerPrivate::UpdateNotebook(const Notebook & notebook, QString & errorDescription)
{
    errorDescription = QT_TR_NOOP("Can't update notebook in local storage database: ");
    QString error;

    bool res = notebook.checkParameters(error);
    if (!res) {
        errorDescription += error;
        QNWARNING("Found invalid notebook: " << notebook << "\nError: " << error);
        return false;
    }

    QString localGuid = notebook.localGuid();

    QString column, guid;
    bool shouldCheckRowExistence = true;

    bool notebookHasGuid = notebook.hasGuid();
    if (notebookHasGuid)
    {
        column = "guid";
        guid = notebook.guid();

        if (localGuid.isEmpty()) {
            QString queryString = QString("SELECT localGuid FROM Notebooks WHERE guid = '%1'").arg(guid);
            QSqlQuery query(m_sqlDatabase);
            res = query.exec(queryString);
            DATABASE_CHECK_AND_SET_ERROR("can't find local guid corresponding to Notebook's guid");

            if (query.next()) {
                localGuid = query.record().value("localGuid").toString();
            }

            if (localGuid.isEmpty()) {
                errorDescription += QT_TR_NOOP("no existing local guid corresponding to Notebook's guid was found in local storage");
                QNCRITICAL(errorDescription << ", guid: " << guid);
                return false;
            }

            shouldCheckRowExistence = false;
        }
    }
    else {
        column = "localGuid";
        guid = localGuid;
    }

    if (shouldCheckRowExistence && !RowExists("Notebooks", column, QVariant(guid))) {
        // TRANSLATOR explaining the reason of error
        errorDescription += QT_TR_NOOP("notebook with specified ");
        errorDescription += column;
        // TRANSLATOR previous part of the phrase was "notebook with specified "
        errorDescription += QT_TR_NOOP(" was not found in local storage");
        QNWARNING(errorDescription << ", " << column << ": " << guid);
        return false;
    }

    return InsertOrReplaceNotebook(notebook, localGuid, errorDescription);
}

bool LocalStorageManagerPrivate::FindNotebook(Notebook & notebook, QString & errorDescription) const
{
    errorDescription = QT_TR_NOOP("Can't find notebook in local storage database: ");

    QString column, value;
    bool notebookHasGuid = notebook.hasGuid();
    if (notebookHasGuid)
    {
        column = "guid";
        value = notebook.guid();
        if (!CheckGuid(value)) {
            // TRANSLATOR explaining the reason of error
            errorDescription += QT_TR_NOOP("requested guid is invalid");
            QNWARNING(errorDescription);
            return false;
        }
    }
    else if (notebook.localGuid().isEmpty())
    {
        if (!notebook.hasName()) {
            errorDescription += QT_TR_NOOP("can't find notebook: need either guid "
                                           "or local guid or name as search criteria");
            QNWARNING(errorDescription);
            return false;
        }

        column = "notebookNameUpper";
        value = notebook.name().toUpper();
    }
    else
    {
        column = "localGuid";
        value = notebook.localGuid();
    }

    QString queryString = QString("SELECT * FROM Notebooks LEFT OUTER JOIN NotebookRestrictions "
                                  "ON Notebooks.localGuid = NotebookRestrictions.localGuid "
                                  "LEFT OUTER JOIN SharedNotebooks ON Notebooks.guid = SharedNotebooks.notebookGuid "
                                  "LEFT OUTER JOIN Users ON Notebooks.contactId = Users.id "
                                  "LEFT OUTER JOIN UserAttributes ON Notebooks.contactId = UserAttributes.id "
                                  "LEFT OUTER JOIN UserAttributesViewedPromotions ON Notebooks.contactId = UserAttributesViewedPromotions.id "
                                  "LEFT OUTER JOIN UserAttributesRecentMailedAddresses ON Notebooks.contactId = UserAttributesRecentMailedAddresses.id "
                                  "LEFT OUTER JOIN Accounting ON Notebooks.contactId = Accounting.id "
                                  "LEFT OUTER JOIN PremiumInfo ON Notebooks.contactId = PremiumInfo.id "
                                  "LEFT OUTER JOIN BusinessUserInfo ON Notebooks.contactId = BusinessUserInfo.id "
                                  "WHERE (Notebooks.%1 = '%2'").arg(column).arg(value);

    if (notebook.hasLinkedNotebookGuid()) {
        queryString += QString(" AND Notebooks.linkedNotebookGuid = '%1')").arg(notebook.linkedNotebookGuid());
    }
    else {
        queryString += " AND Notebooks.linkedNotebookGuid IS NULL)";
    }

    notebook = Notebook();

    QSqlQuery query(m_sqlDatabase);
    bool res = query.exec(queryString);
    DATABASE_CHECK_AND_SET_ERROR("can't find notebook in SQL database by guid");

    size_t counter = 0;
    while(query.next())
    {
        QSqlRecord rec = query.record();
        QString error;
        res = FillNotebookFromSqlRecord(rec, notebook, error);
        if (!res) {
            errorDescription += error;
            QNWARNING(errorDescription);
            return false;
        }

        ++counter;
    }

    if (!counter) {
        errorDescription += QT_TR_NOOP("Notebook with specified local guid was not found");
        QNDEBUG(errorDescription);
        return false;
    }

    SortSharedNotebooks(notebook);
    return true;
}

bool LocalStorageManagerPrivate::FindDefaultNotebook(Notebook & notebook, QString & errorDescription) const
{
    errorDescription = QT_TR_NOOP("Can't find default notebook in local storage database: ");

    notebook = Notebook();
    QSqlQuery query(m_sqlDatabase);
    bool res = query.exec("SELECT * FROM Notebooks LEFT OUTER JOIN NotebookRestrictions "
                          "ON Notebooks.localGuid = NotebookRestrictions.localGuid "
                          "LEFT OUTER JOIN SharedNotebooks ON Notebooks.guid = SharedNotebooks.notebookGuid "
                          "LEFT OUTER JOIN Users ON Notebooks.contactId = Users.id "
                          "LEFT OUTER JOIN UserAttributes ON Notebooks.contactId = UserAttributes.id "
                          "LEFT OUTER JOIN UserAttributesViewedPromotions ON Notebooks.contactId = UserAttributesViewedPromotions.id "
                          "LEFT OUTER JOIN UserAttributesRecentMailedAddresses ON Notebooks.contactId = UserAttributesRecentMailedAddresses.id "
                          "LEFT OUTER JOIN Accounting ON Notebooks.contactId = Accounting.id "
                          "LEFT OUTER JOIN PremiumInfo ON Notebooks.contactId = PremiumInfo.id "
                          "LEFT OUTER JOIN BusinessUserInfo ON Notebooks.contactId = BusinessUserInfo.id "
                          "WHERE isDefault = 1 LIMIT 1");
    DATABASE_CHECK_AND_SET_ERROR("can't find default notebook in SQL database");

    if (!query.next()) {
        // TRANSLATOR explaining the reason of error
        errorDescription += QT_TR_NOOP("no default notebook was found");
        QNDEBUG(errorDescription);
        return false;
    }

    QSqlRecord rec = query.record();
    res = FillNotebookFromSqlRecord(rec, notebook, errorDescription);
    if (!res) {
        return false;
    }

    SortSharedNotebooks(notebook);
    return true;
}

bool LocalStorageManagerPrivate::FindLastUsedNotebook(Notebook & notebook, QString & errorDescription) const
{
    errorDescription = QT_TR_NOOP("Can't find last used notebook in local storage database: ");

    notebook = Notebook();
    QSqlQuery query(m_sqlDatabase);
    bool res = query.exec("SELECT * FROM Notebooks LEFT OUTER JOIN NotebookRestrictions "
                          "ON Notebooks.localGuid = NotebookRestrictions.localGuid "
                          "LEFT OUTER JOIN SharedNotebooks ON Notebooks.guid = SharedNotebooks.notebookGuid "
                          "LEFT OUTER JOIN Users ON Notebooks.contactId = Users.id "
                          "LEFT OUTER JOIN UserAttributes ON Notebooks.contactId = UserAttributes.id "
                          "LEFT OUTER JOIN UserAttributesViewedPromotions ON Notebooks.contactId = UserAttributesViewedPromotions.id "
                          "LEFT OUTER JOIN UserAttributesRecentMailedAddresses ON Notebooks.contactId = UserAttributesRecentMailedAddresses.id "
                          "LEFT OUTER JOIN Accounting ON Notebooks.contactId = Accounting.id "
                          "LEFT OUTER JOIN PremiumInfo ON Notebooks.contactId = PremiumInfo.id "
                          "LEFT OUTER JOIN BusinessUserInfo ON Notebooks.contactId = BusinessUserInfo.id "
                          "WHERE isLastUsed = 1 LIMIT 1");
    DATABASE_CHECK_AND_SET_ERROR("can't find default notebook in SQL database");

    if (!query.next()) {
        // TRANSLATOR explaining the reason of error
        errorDescription += QT_TR_NOOP("no last used notebook exists in local storage");
        QNDEBUG(errorDescription);
        return false;
    }

    QSqlRecord rec = query.record();
    res = FillNotebookFromSqlRecord(rec, notebook, errorDescription);
    if (!res) {
        return false;
    }

    SortSharedNotebooks(notebook);
    return true;
}

bool LocalStorageManagerPrivate::FindDefaultOrLastUsedNotebook(Notebook & notebook, QString & errorDescription) const
{
    bool res = FindDefaultNotebook(notebook, errorDescription);
    if (res) {
        return true;
    }

    return FindLastUsedNotebook(notebook, errorDescription);
}

QList<Notebook> LocalStorageManagerPrivate::ListAllNotebooks(QString & errorDescription,
                                                             const size_t limit, const size_t offset,
                                                             const LocalStorageManager::ListNotebooksOrder::type & order,
                                                             const LocalStorageManager::OrderDirection::type & orderDirection,
                                                             const QString & linkedNotebookGuid) const
{
    QNDEBUG("LocalStorageManagerPrivate::ListAllNotebooks");
    return ListNotebooks(LocalStorageManager::ListAll, errorDescription, limit,
                         offset, order, orderDirection, linkedNotebookGuid);
}

QList<Notebook> LocalStorageManagerPrivate::ListNotebooks(const LocalStorageManager::ListObjectsOptions flag,
                                                          QString & errorDescription, const size_t limit,
                                                          const size_t offset, const LocalStorageManager::ListNotebooksOrder::type & order,
                                                          const LocalStorageManager::OrderDirection::type & orderDirection,
                                                          const QString & linkedNotebookGuid) const
{
    QNDEBUG("LocalStorageManagerPrivate::ListNotebooks: flag = " << flag);

    QString linkedNotebookGuidSqlQueryCondition = (linkedNotebookGuid.isEmpty()
                                                   ? "linkedNotebookGuid IS NULL"
                                                   : QString("linkedNotebookGuid = '%1'").arg(linkedNotebookGuid));

    return listObjects<Notebook, LocalStorageManager::ListNotebooksOrder::type>(flag, errorDescription, limit, offset, order, orderDirection,
                                                                                linkedNotebookGuidSqlQueryCondition);
}

QList<SharedNotebookWrapper> LocalStorageManagerPrivate::ListAllSharedNotebooks(QString & errorDescription) const
{
    QNDEBUG("LocalStorageManagerPrivate::ListAllSharedNotebooks");

    QList<SharedNotebookWrapper> sharedNotebooks;
    errorDescription = QT_TR_NOOP("Can't list all shared notebooks: ");

    QSqlQuery query(m_sqlDatabase);
    bool res = query.exec("SELECT * FROM SharedNotebooks");
    if (!res) {
        // TRANSLATOR explaining the reason of error
        errorDescription += QT_TR_NOOP("can't select shared notebooks from SQL database: ");
        QNCRITICAL(errorDescription << "last error = " << query.lastError() << ", last query = " << query.lastQuery());
        errorDescription += query.lastError().text();
        return sharedNotebooks;
    }

    sharedNotebooks.reserve(qMax(query.size(), 0));

    while(query.next())
    {
        QSqlRecord record = query.record();

        sharedNotebooks << SharedNotebookWrapper();
        SharedNotebookWrapper & sharedNotebook = sharedNotebooks.back();

        res = FillSharedNotebookFromSqlRecord(record, sharedNotebook, errorDescription);
        if (!res) {
            sharedNotebooks.clear();
            return sharedNotebooks;
        }
    }

    int numSharedNotebooks = sharedNotebooks.size();
    QNDEBUG("found " << numSharedNotebooks << " shared notebooks");

    if (numSharedNotebooks <= 0) {
        errorDescription += QT_TR_NOOP("no shared notebooks exist in local storage");
        QNDEBUG(errorDescription);
    }

    return sharedNotebooks;
}

QList<SharedNotebookWrapper> LocalStorageManagerPrivate::ListSharedNotebooksPerNotebookGuid(const QString & notebookGuid,
                                                                                     QString & errorDescription) const
{
    QList<SharedNotebookWrapper> sharedNotebooks;

    QList<qevercloud::SharedNotebook> enSharedNotebooks = ListEnSharedNotebooksPerNotebookGuid(notebookGuid, errorDescription);
    if (enSharedNotebooks.isEmpty()) {
        return sharedNotebooks;
    }

    sharedNotebooks.reserve(qMax(enSharedNotebooks.size(), 0));

    foreach(const qevercloud::SharedNotebook & sharedNotebook, enSharedNotebooks) {
        sharedNotebooks << sharedNotebook;
    }

    return sharedNotebooks;
}

QList<qevercloud::SharedNotebook> LocalStorageManagerPrivate::ListEnSharedNotebooksPerNotebookGuid(const QString & notebookGuid,
                                                                                            QString & errorDescription) const
{
    QNDEBUG("LocalStorageManagerPrivate::ListSharedNotebooksPerNotebookGuid: guid = " << notebookGuid);

    QList<qevercloud::SharedNotebook> sharedNotebooks;
    QString errorPrefix = QT_TR_NOOP("Can't list shared notebooks per notebook guid: ");

    if (!CheckGuid(notebookGuid)) {
        errorDescription = errorPrefix + QT_TR_NOOP("notebook guid is invalid");
        QNWARNING(errorDescription);
        return sharedNotebooks;
    }

    QSqlQuery query(m_sqlDatabase);
    query.prepare("SELECT * FROM SharedNotebooks WHERE notebookGuid=?");
    query.addBindValue(notebookGuid);

    bool res = query.exec();
    if (!res) {
        errorDescription = errorPrefix + QT_TR_NOOP("can't select shared notebooks for given "
                                                    "notebook guid from SQL database: ");
        QNCRITICAL(errorDescription << ", last error = " << query.lastError() << ", last query = " << query.lastQuery());
        errorDescription += query.lastError().text();
        return sharedNotebooks;
    }

    int numSharedNotebooks = query.size();
    sharedNotebooks.reserve(qMax(numSharedNotebooks, 0));
    QList<qevercloud::SharedNotebook> unsortedSharedNotebooks;
    unsortedSharedNotebooks.reserve(qMax(numSharedNotebooks, 0));
    QList<SharedNotebookAdapter> sharedNotebookAdapters;
    sharedNotebookAdapters.reserve(qMax(numSharedNotebooks, 0));

    while(query.next())
    {
        QSqlRecord record = query.record();

        unsortedSharedNotebooks << qevercloud::SharedNotebook();
        qevercloud::SharedNotebook & sharedNotebook = unsortedSharedNotebooks.back();
        sharedNotebookAdapters << SharedNotebookAdapter(sharedNotebook);
        SharedNotebookAdapter & sharedNotebookAdapter = sharedNotebookAdapters.back();

        res = FillSharedNotebookFromSqlRecord(record, sharedNotebookAdapter, errorDescription);
        if (!res) {
            sharedNotebooks.clear();
            return sharedNotebooks;
        }
    }

    qSort(sharedNotebookAdapters.begin(), sharedNotebookAdapters.end(), SharedNotebookAdapterCompareByIndex());

    foreach(const SharedNotebookAdapter & sharedNotebookAdapter, sharedNotebookAdapters) {
        sharedNotebooks << sharedNotebookAdapter.GetEnSharedNotebook();
    }

    numSharedNotebooks = sharedNotebooks.size();
    QNDEBUG("found " << numSharedNotebooks << " shared notebooks");

    return sharedNotebooks;
}

bool LocalStorageManagerPrivate::ExpungeNotebook(const Notebook & notebook, QString & errorDescription)
{
    errorDescription = QT_TR_NOOP("Can't expunge notebook from local storage database: ");

    QString column, guid;
    bool notebookHasGuid = notebook.hasGuid();
    if (notebookHasGuid) {
        column = "guid";
        guid = notebook.guid();

        if (!CheckGuid(guid)) {
            // TRANSLATOR explaining the reason of error
            errorDescription += QT_TR_NOOP("notebook's guid is invalid");
            QNWARNING(errorDescription);
            return false;
        }
    }
    else {
        column = "localGuid";
        guid = notebook.localGuid();
    }

    bool exists = RowExists("Notebooks", column, QVariant(guid));
    if (!exists) {
        errorDescription += QT_TR_NOOP("notebook to be expunged from \"Notebooks\""
                                       "table in SQL database was not found");
        QNWARNING(errorDescription << ", " << column << ": " << guid);
        return false;
    }

    QString queryString = QString("DELETE FROM Notebooks WHERE %1 = '%2'").arg(column).arg(guid);
    QSqlQuery query(m_sqlDatabase);
    bool res = query.exec(queryString);
    DATABASE_CHECK_AND_SET_ERROR("can't delete entry from \"Notebooks\" table in SQL database");

    return true;
}

int LocalStorageManagerPrivate::GetLinkedNotebookCount(QString & errorDescription) const
{
    bool res = CheckAndPrepareGetLinkedNotebookCountQuery();
    QSqlQuery & query = m_getLinkedNotebookCountQuery;
    if (!res) {
        errorDescription = QT_TR_NOOP("Internal error: can't get number of linked notebooks: "
                                      "can't prepare SQL query");
        QNCRITICAL(errorDescription << query.lastError() << ", last query: " << query.lastQuery());
        errorDescription += query.lastError().text();
        return -1;
    }

    res = query.exec();
    if (!res) {
        errorDescription = QT_TR_NOOP("Internal error: can't get number of linked notebooks in local storage database: ");
        QNCRITICAL(errorDescription << query.lastError() << ", last query: " << query.lastQuery());
        errorDescription += query.lastError().text();
        return -1;
    }

    if (!query.next()) {
        QNDEBUG("Found no linked notebooks in local storage database");
        return 0;
    }

    bool conversionResult = false;
    int count = query.value(0).toInt(&conversionResult);
    if (!conversionResult) {
        errorDescription = QT_TR_NOOP("Internal error: can't convert number of linked notebooks to int");
        QNCRITICAL(errorDescription << ": " << query.value(0));
        return -1;
    }

    return count;
}

bool LocalStorageManagerPrivate::AddLinkedNotebook(const LinkedNotebook & linkedNotebook,
                                            QString & errorDescription)
{
    errorDescription = QT_TR_NOOP("Can't add linked notebook to local storage database: ");
    QString error;

    bool res = linkedNotebook.checkParameters(error);
    if (!res) {
        errorDescription += error;
        QNWARNING("Found invalid LinkedNotebook: " << linkedNotebook << "\nError: " << error);
        return false;
    }

    bool exists = RowExists("LinkedNotebooks", "guid", QVariant(linkedNotebook.guid()));
    if (exists) {
        // TRANSLATOR explaining the reason of error
        errorDescription += QT_TR_NOOP("linked notebook with specified guid already exists");
        QNWARNING(errorDescription << ", guid: " << linkedNotebook.guid());
        return false;
    }

    return InsertOrReplaceLinkedNotebook(linkedNotebook, errorDescription);
}

bool LocalStorageManagerPrivate::UpdateLinkedNotebook(const LinkedNotebook & linkedNotebook,
                                               QString & errorDescription)
{
    errorDescription = QT_TR_NOOP("Can't update linked notebook in local storage database: ");
    QString error;

    bool res = linkedNotebook.checkParameters(error);
    if (!res) {
        errorDescription += error;
        QNWARNING("Found invalid LinkedNotebook: " << linkedNotebook << "\nError: " << error);
        return false;
    }

    QString guid = linkedNotebook.guid();

    bool exists = RowExists("LinkedNotebooks", "guid", QVariant(guid));
    if (!exists) {
        // TRANSLATOR explaining the reason of error
        errorDescription += QT_TR_NOOP("linked notebook with specified guid was not found");
        QNWARNING(errorDescription << ", guid: " << guid);
        return false;
    }

    return InsertOrReplaceLinkedNotebook(linkedNotebook, errorDescription);
}

bool LocalStorageManagerPrivate::FindLinkedNotebook(LinkedNotebook & linkedNotebook, QString & errorDescription) const
{
    QNDEBUG("LocalStorageManagerPrivate::FindLinkedNotebook");

    errorDescription = QT_TR_NOOP("Can't find linked notebook in local storage database: ");

    if (!linkedNotebook.hasGuid()) {
        // TRANSLATOR explaining the reason of error
        errorDescription += QT_TR_NOOP("guid is not set");
        QNWARNING(errorDescription);
        return false;
    }

    QString notebookGuid = linkedNotebook.guid();
    QNDEBUG("guid = " << notebookGuid);
    if (!CheckGuid(notebookGuid)) {
        // TRANSLATOR explaining the reason of error
        errorDescription += QT_TR_NOOP("guid is invalid");
        QNWARNING(errorDescription);
        return false;
    }

    linkedNotebook.clear();

    QSqlQuery query(m_sqlDatabase);
    query.prepare("SELECT guid, updateSequenceNumber, isDirty, shareName, username, shardId, "
                  "shareKey, uri, noteStoreUrl, webApiUrlPrefix, stack, businessId "
                  "FROM LinkedNotebooks WHERE guid = ?");
    query.addBindValue(notebookGuid);

    bool res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("can't find linked notebook in Sql database by guid");

    if (!query.next()) {
        errorDescription += QT_TR_NOOP("no linked notebook was found in local storage");
        QNDEBUG(errorDescription);
        return false;
    }

    QSqlRecord rec = query.record();

    return FillLinkedNotebookFromSqlRecord(rec, linkedNotebook, errorDescription);
}

QList<LinkedNotebook> LocalStorageManagerPrivate::ListAllLinkedNotebooks(QString & errorDescription, const size_t limit, const size_t offset,
                                                                         const LocalStorageManager::ListLinkedNotebooksOrder::type order,
                                                                         const LocalStorageManager::OrderDirection::type & orderDirection) const
{
    QNDEBUG("LocalStorageManagerPrivate::ListAllLinkedNotebooks");
    return ListLinkedNotebooks(LocalStorageManager::ListAll, errorDescription, limit, offset, order, orderDirection);
}

QList<LinkedNotebook> LocalStorageManagerPrivate::ListLinkedNotebooks(const LocalStorageManager::ListObjectsOptions flag,
                                                                      QString & errorDescription, const size_t limit, const size_t offset,
                                                                      const LocalStorageManager::ListLinkedNotebooksOrder::type & order,
                                                                      const LocalStorageManager::OrderDirection::type & orderDirection) const
{
    QNDEBUG("LocalStorageManagerPrivate::ListLinkedNotebooks: flag = " << flag);
    return listObjects<LinkedNotebook, LocalStorageManager::ListLinkedNotebooksOrder::type>(flag, errorDescription, limit, offset, order, orderDirection);
}

bool LocalStorageManagerPrivate::ExpungeLinkedNotebook(const LinkedNotebook & linkedNotebook,
                                                       QString & errorDescription)
{
    errorDescription = QT_TR_NOOP("Can't expunge linked notebook from local storage database: ");

    if (!linkedNotebook.hasGuid()) {
        // TRANSLATOR explaining the reason of error
        errorDescription += QT_TR_NOOP("linked notebook's guid is not set");
        QNWARNING(errorDescription);
        return false;
    }

    const QString linkedNotebookGuid = linkedNotebook.guid();

    if (!CheckGuid(linkedNotebookGuid)) {
        // TRANSLATOR explaining the reason of error
        errorDescription += QT_TR_NOOP("linked notebook's guid is invalid");
        QNWARNING(errorDescription);
        return false;
    }

    bool exists = RowExists("LinkedNotebooks", "guid", QVariant(linkedNotebookGuid));
    if (!exists) {
        errorDescription += QT_TR_NOOP("can't find linked notebook to be expunged from "
                                       "\"LinkedNotebooks\" table in SQL database");
        QNWARNING(errorDescription << ", guid: " << linkedNotebookGuid);
        return false;
    }

    bool res = CheckAndPrepareExpungeLinkedNotebookQuery();
    QSqlQuery & query = m_expungeLinkedNotebookQuery;
    DATABASE_CHECK_AND_SET_ERROR("can't delete entry from \"LinkedNotebooks\" table: "
                                 "can't prepare SQL query");

    query.bindValue(":guid", linkedNotebookGuid);

    res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("can't delete entry from \"LinkedNotebooks\" table in SQL database");

    return true;
}

int LocalStorageManagerPrivate::GetNoteCount(QString & errorDescription) const
{
    bool res = CheckAndPrepareGetNoteCountQuery();
    QSqlQuery & query = m_getNoteCountQuery;
    if (!res) {
        errorDescription = QT_TR_NOOP("Internal error: can't get number of notes in local storage database: "
                                      "can't prepare SQL query");
        QNCRITICAL(errorDescription << query.lastError() << ", last query: " << query.lastQuery());
        errorDescription += query.lastError().text();
        return -1;
    }

    res = query.exec();
    if (!res) {
        errorDescription = QT_TR_NOOP("Internal error: can't get number of notes in local storage database: ");
        QNCRITICAL(errorDescription << query.lastError() << ", last query: " << query.lastQuery());
        errorDescription += query.lastError().text();
        return -1;
    }

    if (!query.next()) {
        QNDEBUG("Found no notes in local storage database");
        return 0;
    }

    bool conversionResult = false;
    int count = query.value(0).toInt(&conversionResult);
    if (!conversionResult) {
        errorDescription = QT_TR_NOOP("Internal error: can't convert number of notes to int");
        QNCRITICAL(errorDescription << ": " << query.value(0));
        return -1;
    }

    return count;
}

bool LocalStorageManagerPrivate::AddNote(const Note & note, const Notebook & notebook, QString & errorDescription)
{
    errorDescription = QT_TR_NOOP("Can't add note to local storage database: ");
    QString error;

    if (!notebook.hasGuid() && notebook.localGuid().isEmpty()) {
        errorDescription += QT_TR_NOOP("both local and remote notebook's guids are empty");
        QNWARNING(errorDescription << ", notebook: " << notebook);
        return false;
    }

    if (!notebook.canCreateNotes()) {
        // TRANSLATOR explaining why note cannot be added to notebook
        errorDescription += QT_TR_NOOP("notebook's restrictions forbid notes creation");
        QNWARNING(errorDescription);
        return false;
    }

    bool res = note.checkParameters(error);
    if (!res) {
        errorDescription += error;
        QNWARNING("Found invalid note: " << note);
        return false;
    }

    QString localGuid = note.localGuid();

    QString column, guid;
    bool shouldCheckNoteExistence = true;

    bool noteHasGuid = note.hasGuid();
    if (noteHasGuid)
    {
        column = "guid";
        guid = note.guid();

        if (localGuid.isEmpty()) {
            QString queryString = QString("SELECT localGuid FROM Notes WHERE guid = '%1'").arg(guid);
            QSqlQuery query(m_sqlDatabase);
            res = query.exec(queryString);
            DATABASE_CHECK_AND_SET_ERROR("can't find local guid corresponding to Note's guid");

            if (query.next()) {
                localGuid = query.record().value("localGuid").toString();
            }

            if (!localGuid.isEmpty()) {
                errorDescription += QT_TR_NOOP("found existing local guid corresponding to Note's guid");
                QNCRITICAL(errorDescription << ", guid: " << guid);
                return false;
            }

            localGuid = QUuid::createUuid().toString();
            shouldCheckNoteExistence = false;
        }
    }
    else {
        column = "localGuid";
        guid = localGuid;
    }

    if (shouldCheckNoteExistence && RowExists("Notes", column, QVariant(guid))) {
        // TRANSLATOR explaining the reason of error
        errorDescription += QT_TR_NOOP("note with specified ");
        errorDescription += column;
        // TRANSLATOR previous part of the phrase was "note with specified guid|localGuid "
        errorDescription += QT_TR_NOOP(" already exists in local storage");
        QNWARNING(errorDescription << ", " << column << ": " << guid);
        return false;
    }

    return InsertOrReplaceNote(note, notebook, localGuid, errorDescription);
}

bool LocalStorageManagerPrivate::UpdateNote(const Note & note, const Notebook & notebook, QString & errorDescription)
{
    errorDescription = QT_TR_NOOP("Can't update note in local storage database: ");
    QString error;

    if (!notebook.hasGuid() && notebook.localGuid().isEmpty()) {
        errorDescription += QT_TR_NOOP("both local and remote notebook's guids are empty");
        QNWARNING(errorDescription << ", notebook: " << notebook);
        return false;
    }

    if (!notebook.canUpdateNotes()) {
        // TRANSLATOR explaining why note cannot be updated
        errorDescription += QT_TR_NOOP("notebook's restrictions forbid update of notes");
        return false;
    }

    bool res = note.checkParameters(error);
    if (!res) {
        errorDescription += error;
        QNWARNING("Found invalid note: " << note);
        return false;
    }

    QString localGuid = note.localGuid();

    QString column, guid;
    bool shouldCheckNoteExistence = true;

    bool noteHasGuid = note.hasGuid();
    if (noteHasGuid)
    {
        column = "guid";
        guid = note.guid();

        if (localGuid.isEmpty())
        {
            QString queryString = QString("SELECT localGuid FROM Notes WHERE guid = '%1'").arg(guid);
            QSqlQuery query(m_sqlDatabase);
            res = query.exec(queryString);
            DATABASE_CHECK_AND_SET_ERROR("can't find local guid corresponding to Note's guid");

            if (query.next()) {
                localGuid = query.record().value("localGuid").toString();
            }

            if (localGuid.isEmpty()) {
                errorDescription += QT_TR_NOOP("no existing local guid corresponding to Note's guid was found in local storage");
                QNCRITICAL(errorDescription << ", guid: " << guid);
                return false;
            }

            shouldCheckNoteExistence = false;
        }
    }
    else {
        column = "localGuid";
        guid = localGuid;
    }

    if (shouldCheckNoteExistence && !RowExists("Notes", column, QVariant(guid))) {
        // TRANSLATOR explaining the reason of error
        errorDescription += QT_TR_NOOP("note with specified ");
        errorDescription += column;
        // TRANSLATOR previous part of the phrase was "note with specified guid|localGuid"
        errorDescription += QT_TR_NOOP(" was not found in local storage");
        QNWARNING(errorDescription << ", " << column << ": " << guid);
        return false;
    }

    return InsertOrReplaceNote(note, notebook, localGuid, errorDescription);
}

bool LocalStorageManagerPrivate::FindNote(Note & note, QString & errorDescription,
                                   const bool withResourceBinaryData) const
{
    QNDEBUG("LocalStorageManagerPrivate::FindNote");

    errorDescription = QT_TR_NOOP("Can't find note in local storage database: ");

    QString column, guid;
    bool noteHasGuid = note.hasGuid();
    if (noteHasGuid) {
        column = "guid";
        guid = note.guid();

        if (!CheckGuid(guid)) {
            // TRANSLATOR explaining why note cannot be found
            errorDescription += QT_TR_NOOP("requested note guid is invalid");
            QNWARNING(errorDescription);
            return false;
        }
    }
    else {
        column = "localGuid";
        guid = note.localGuid();
    }

    note = Note();

    QString resourcesTable = "Resources";
    if (!withResourceBinaryData) {
        resourcesTable += "WithoutBinaryData";
    }

    QString resourceIndexColumn = (column == "localGuid" ? "noteLocalGuid" : "noteGuid");

    QString queryString = QString("SELECT * FROM Notes "
                                  "LEFT OUTER JOIN %1 ON Notes.%3 = %1.%2 "
                                  "LEFT OUTER JOIN ResourceAttributes "
                                  "ON %1.resourceLocalGuid = ResourceAttributes.resourceLocalGuid "
                                  "LEFT OUTER JOIN ResourceAttributesApplicationDataKeysOnly "
                                  "ON %1.resourceLocalGuid = ResourceAttributesApplicationDataKeysOnly.resourceLocalGuid "
                                  "LEFT OUTER JOIN ResourceAttributesApplicationDataFullMap "
                                  "ON %1.resourceLocalGuid = ResourceAttributesApplicationDataFullMap.resourceLocalGuid "
                                  "LEFT OUTER JOIN NoteTags ON Notes.localGuid = NoteTags.localNote "
                                  "WHERE %3 = '%4'")
                                 .arg(resourcesTable).arg(resourceIndexColumn)
                                 .arg(column).arg(guid);
    QSqlQuery query(m_sqlDatabase);
    bool res = query.exec(queryString);
    DATABASE_CHECK_AND_SET_ERROR("can't select note from \"Notes\" table in SQL database");

    QList<ResourceWrapper> resources;
    QHash<QString, int> resourceIndexPerLocalGuid;

    QList<QPair<QString, int> > tagGuidsAndIndices;
    QHash<QString, int> tagGuidIndexPerGuid;

    size_t counter = 0;
    while(query.next())
    {
        QSqlRecord rec = query.record();
        FillNoteFromSqlRecord(rec, note);
        ++counter;

        int resourceLocalGuidIndex = rec.indexOf("resourceLocalGuid");
        if (resourceLocalGuidIndex >= 0)
        {
            QVariant value = rec.value(resourceLocalGuidIndex);
            if (!value.isNull())
            {
                QString resourceLocalGuid = value.toString();
                auto it = resourceIndexPerLocalGuid.find(resourceLocalGuid);
                bool resourceIndexNotFound = (it == resourceIndexPerLocalGuid.end());
                if (resourceIndexNotFound) {
                    int resourceIndexInList = resources.size();
                    resourceIndexPerLocalGuid[resourceLocalGuid] = resourceIndexInList;
                    resources << ResourceWrapper();
                }

                ResourceWrapper & resource = (resourceIndexNotFound
                                              ? resources.back()
                                              : resources[it.value()]);
                FillResourceFromSqlRecord(rec, withResourceBinaryData, resource);
            }
        }

        int tagGuidIndex = rec.indexOf("tag");
        if (tagGuidIndex >= 0)
        {
            QVariant value = rec.value(tagGuidIndex);
            if (!value.isNull())
            {
                QVariant tagGuidIndexInNoteValue = rec.value("tagIndexInNote");
                if (tagGuidIndexInNoteValue.isNull()) {
                    QNWARNING("tag index in note was not found in the result of SQL query");
                    continue;
                }

                bool conversionResult = false;
                int tagGuidIndexInNote = tagGuidIndexInNoteValue.toInt(&conversionResult);
                if (!conversionResult) {
                    errorDescription += QT_TR_NOOP("Internal error: can't convert tag guid's index in note to int");
                    return false;
                }

                QString tagGuid = value.toString();
                auto it = tagGuidIndexPerGuid.find(tagGuid);
                bool tagGuidIndexNotFound = (it == tagGuidIndexPerGuid.end());
                if (tagGuidIndexNotFound) {
                    int tagGuidIndexInList = tagGuidsAndIndices.size();
                    tagGuidIndexPerGuid[tagGuid] = tagGuidIndexInList;
                    tagGuidsAndIndices << QPair<QString, int>(tagGuid, tagGuidIndexInNote);
                    continue;
                }

                QPair<QString, int> & tagGuidAndIndexInNote = tagGuidsAndIndices[it.value()];
                tagGuidAndIndexInNote.first = tagGuid;
                tagGuidAndIndexInNote.second = tagGuidIndexInNote;
            }
        }
    }

    if (!counter) {
        errorDescription += QT_TR_NOOP("no note was found in local storage");
        QNDEBUG(errorDescription);
        return false;
    }

    int numResources = resources.size();
    if (numResources > 0) {
        qSort(resources.begin(), resources.end(), ResourceWrapperCompareByIndex());
        note.setResources(resources);
    }

    int numTags = tagGuidsAndIndices.size();
    if (numTags > 0) {
        qSort(tagGuidsAndIndices.begin(), tagGuidsAndIndices.end(), QStringIntPairCompareByInt());
        QStringList tagGuids;
        tagGuids.reserve(numTags);
        for(int i = 0; i < numTags; ++i) {
            tagGuids << tagGuidsAndIndices[i].first;
        }

        note.setTagGuids(tagGuids);
    }

    QString error;
    res = note.checkParameters(error);
    if (!res) {
        errorDescription = QT_TR_NOOP("Found note is invalid: ");
        errorDescription += error;
        QNWARNING(errorDescription);
        return false;
    }

    return true;
}

QList<Note> LocalStorageManagerPrivate::ListAllNotesPerNotebook(const Notebook & notebook,
                                                                QString & errorDescription,
                                                                const bool withResourceBinaryData,
                                                                const LocalStorageManager::ListObjectsOptions & flag,
                                                                const size_t limit, const size_t offset,
                                                                const LocalStorageManager::ListNotesOrder::type & order,
                                                                const LocalStorageManager::OrderDirection::type & orderDirection) const
{
    QNDEBUG("LocalStorageManagerPrivate::ListAllNotesPerNotebook: notebookGuid = " << notebook);

    QString errorPrefix = QT_TR_NOOP("Can't find all notes per notebook: ");

    QList<Note> notes;

    QString column, guid;
    bool notebookHasGuid = notebook.hasGuid();
    if (notebookHasGuid) {
        column = "notebookGuid";
        guid = notebook.guid();

        if (!CheckGuid(guid)) {
            // TRANSLATOR explaining why notes per notebook cannot be listed
            errorDescription = errorPrefix + QT_TR_NOOP("notebook guid is invalid");
            QNWARNING(errorDescription);
            return notes;
        }
    }
    else {
        column = "notebookLocalGuid";
        guid = notebook.localGuid();
    }

    // Will run all the queries from this method and its sub-methods within a single transaction
    // to prevent multiple drops and re-obtainings of shared lock
    Transaction transaction(m_sqlDatabase, *this, Transaction::Selection);
    Q_UNUSED(transaction)

    QString error;
    QString notebookGuidSqlQueryCondition = QString("%1 = '%2'").arg(column).arg(guid);
    notes = listObjects<Note, LocalStorageManager::ListNotesOrder::type>(flag, error, limit, offset, order,
                                                                         orderDirection, notebookGuidSqlQueryCondition);
    const int numNotes = notes.size();
    if ((numNotes == 0) && !error.isEmpty()) {
        errorDescription = errorPrefix + error;
        QNWARNING(errorDescription);
        return notes;
    }

    for(int i = 0; i < numNotes; ++i)
    {
        Note & note = notes[i];

        error.resize(0);
        bool res = FindAndSetTagGuidsPerNote(note, error);
        if (!res) {
            errorDescription = errorPrefix + error;
            QNWARNING(errorDescription);
            notes.clear();
            return notes;
        }

        error.resize(0);
        res = FindAndSetResourcesPerNote(note, error, withResourceBinaryData);
        if (!res) {
            errorDescription = errorPrefix + error;
            QNWARNING(errorDescription);
            notes.clear();
            return notes;
        }

        res = note.checkParameters(error);
        if (!res) {
            errorDescription = errorPrefix + QT_TR_NOOP("found note is invalid: ");
            errorDescription += error;
            QNWARNING(errorDescription);
            notes.clear();
            return notes;
        }
    }

    return notes;
}

QList<Note> LocalStorageManagerPrivate::ListNotes(const LocalStorageManager::ListObjectsOptions flag,
                                                  QString & errorDescription, const bool withResourceBinaryData,
                                                  const size_t limit, const size_t offset,
                                                  const LocalStorageManager::ListNotesOrder::type & order,
                                                  const LocalStorageManager::OrderDirection::type & orderDirection) const
{
    QNDEBUG("LocalStorageManagerPrivate::ListNotes: flag = " << flag << ", withResourceBinaryData = "
            << (withResourceBinaryData ? "true" : "false"));

    // Will run all the queries from this method and its sub-methods within a single transaction
    // to prevent multiple drops and re-obtainings of shared lock
    Transaction transaction(m_sqlDatabase, *this, Transaction::Selection);
    Q_UNUSED(transaction)

    QList<Note> notes = listObjects<Note, LocalStorageManager::ListNotesOrder::type>(flag, errorDescription, limit, offset, order, orderDirection);
    if (notes.isEmpty()) {
        return notes;
    }

    const int numNotes = notes.size();
    QString error;
    for(int i = 0; i < numNotes; ++i)
    {
        Note & note = notes[i];

        bool res = FindAndSetTagGuidsPerNote(note, error);
        if (!res) {
            errorDescription += error;
            QNWARNING(errorDescription);
            notes.clear();
            return notes;
        }

        error.resize(0);
        res = FindAndSetResourcesPerNote(note, error, withResourceBinaryData);
        if (!res) {
            errorDescription += error;
            QNWARNING(errorDescription);
            notes.clear();
            return notes;
        }

        res = note.checkParameters(error);
        if (!res) {
            errorDescription += QT_TR_NOOP("found note is invalid: ");
            errorDescription += error;
            QNWARNING(errorDescription);
            notes.clear();
            return notes;
        }
    }

    return notes;
}

bool LocalStorageManagerPrivate::DeleteNote(const Note & note, QString & errorDescription)
{
    errorDescription = QT_TR_NOOP("Can't delete note from local storage database: ");

    QString column, guid;
    bool noteHasGuid = note.hasGuid();
    if(noteHasGuid) {
        column = "guid";
        guid = note.guid();

        if (!CheckGuid(guid)) {
            // TRANSLATOR explaining why note cannot be marked as deleted in local storage
            errorDescription += QT_TR_NOOP("requested note guid is invalid");
            QNWARNING(errorDescription);
            return false;
        }
    }
    else {
        column = "localGuid";
        guid = note.localGuid();
    }

    if (!note.hasActive() || note.active()) {
        errorDescription += QT_TR_NOOP("note to be marked as deleted in local storage "
                                       "does not have corresponding \"active\" mark set to false, "
                                       "rejecting to mark it deleted");
        QNWARNING(errorDescription);
        return false;
    }

    if (!note.hasDeletionTimestamp()) {
        errorDescription += QT_TR_NOOP("note to be marked as deleted in local storage "
                                       "does not have deletion timestamp set, rejecting to mark it deleted");
        QNWARNING(errorDescription);
        return false;
    }

    QString queryString = QString("UPDATE Notes SET isActive=0, isDirty=1, deletionTimestamp=%1 WHERE %2 = '%3'")
                                  .arg(note.deletionTimestamp()).arg(column).arg(guid);
    QSqlQuery query(m_sqlDatabase);
    bool res = query.exec(queryString);
    DATABASE_CHECK_AND_SET_ERROR("can't delete entry from \"Notes\" table in SQL database");

    return true;
}

bool LocalStorageManagerPrivate::ExpungeNote(const Note & note, QString & errorDescription)
{
    errorDescription = QT_TR_NOOP("Can't expunge note from local storage database: ");

    QString column, guid;
    bool noteHasGuid = note.hasGuid();
    if(noteHasGuid) {
        column = "guid";
        guid = note.guid();

        if (!CheckGuid(guid)) {
            // TRANSLATOR explaining why note cannot be expunged from local storage
            errorDescription += QT_TR_NOOP("requested note guid is invalid");
            QNWARNING(errorDescription);
            return false;
        }
    }
    else {
        column = "localGuid";
        guid = note.localGuid();
    }

    bool exists = RowExists("Notes", column, QVariant(guid));
    if (!exists) {
        errorDescription += QT_TR_NOOP("can't find note to be expunged in \"Notes\" table in SQL database");
        QNWARNING(errorDescription << ", " << column << ": " << guid);
        return false;
    }

    QString queryString = QString("DELETE FROM Notes WHERE %1 = '%2'").arg(column).arg(guid);
    QSqlQuery query(m_sqlDatabase);
    bool res = query.exec(queryString);
    DATABASE_CHECK_AND_SET_ERROR("can't delete entry from \"Notes\" table in SQL database");

    return true;
}

NoteList LocalStorageManagerPrivate::FindNotesWithSearchQuery(const NoteSearchQuery & noteSearchQuery,
                                                              QString & errorDescription,
                                                              const bool withResourceBinaryData) const
{
    QNDEBUG("LocalStorageManagerPrivate::FindNotesWithSearchQuery: " << noteSearchQuery);

    if (!noteSearchQuery.isMatcheable()) {
        return NoteList();
    }

    QString queryString;

    // Will run all the queries from this method and its sub-methods within a single transaction
    // to prevent multiple drops and re-obtainings of shared lock
    Transaction transaction(m_sqlDatabase, *this, Transaction::Selection);
    Q_UNUSED(transaction)

    errorDescription = QT_TR_NOOP("Can't convert note search query string into SQL query string: ");
    bool res = noteSearchQueryToSQL(noteSearchQuery, queryString, errorDescription);
    if (!res) {
        return NoteList();
    }

    errorDescription = QT_TR_NOOP("Can't execute SQL to find notes local guids: ");
    QSqlQuery query(m_sqlDatabase);
    res = query.exec(queryString);
    if (!res) {
        errorDescription += query.lastError().text();
        errorDescription += QT_TR_NOOP("; full executed SQL query: ");
        errorDescription += queryString;
        QNCRITICAL(errorDescription << ", full error: " << query.lastError());
        return NoteList();
    }

    QSet<QString> foundLocalGuids;
    while(query.next())
    {
        QSqlRecord rec = query.record();
        int index = rec.indexOf("localGuid");   // one way of selecting notes
        if (index < 0) {
            continue;
        }

        QString value = rec.value(index).toString();
        if (value.isEmpty() || foundLocalGuids.contains(value)) {
            continue;
        }

        foundLocalGuids.insert(value);
    }

    QString joinedLocalGuids;
    foreach(const QString & item, foundLocalGuids)
    {
        if (!joinedLocalGuids.isEmpty()) {
            joinedLocalGuids += ", ";
        }

        joinedLocalGuids += "'";
        joinedLocalGuids += item;
        joinedLocalGuids += "'";
    }

    queryString = QString("SELECT * FROM Notes WHERE localGuid IN (%1)").arg(joinedLocalGuids);
    errorDescription = QT_TR_NOOP("Can't execute SQL to find notes per local guids: ");
    res = query.exec(queryString);
    if (!res) {
        errorDescription += query.lastError().text();
        QNCRITICAL(errorDescription << ", full error: " << query.lastError());
        return NoteList();
    }

    NoteList notes;
    notes.reserve(qMax(query.size(), 0));
    QString error;

    errorDescription = QT_TR_NOOP("Can't perform post-processing when fetching notes per note search query: ");
    while(query.next())
    {
        notes.push_back(Note());
        Note & note = notes.back();
        note.setLocalGuid(QString());

        QSqlRecord rec = query.record();
        FillNoteFromSqlRecord(rec, note);

        res = FindAndSetTagGuidsPerNote(note, error);
        if (!res) {
            errorDescription += error;
            QNWARNING(errorDescription);
            return NoteList();
        }

        error.resize(0);
        res = FindAndSetResourcesPerNote(note, error, withResourceBinaryData);
        if (!res) {
            errorDescription += error;
            QNWARNING(errorDescription);
            return NoteList();
        }

        res = note.checkParameters(error);
        if (!res) {
            errorDescription += QT_TR_NOOP("found note is invalid: ");
            errorDescription += error;
            QNWARNING(errorDescription);
            return NoteList();
        }
    }

    errorDescription.resize(0);
    return notes;
}

int LocalStorageManagerPrivate::GetTagCount(QString & errorDescription) const
{
    bool res = CheckAndPrepareGetTagCountQuery();
    QSqlQuery & query = m_getTagCountQuery;
    if (!res) {
        errorDescription = QT_TR_NOOP("Internal error: can't get number of tags in local storage database, "
                                      "can't prepare SQL query: ");
        QNCRITICAL(errorDescription << query.lastError() << ", last query: " << query.lastQuery());
        errorDescription += query.lastError().text();
        return -1;
    }

    res = query.exec();
    if (!res) {
        errorDescription = QT_TR_NOOP("Internal error: can't get number of tags in local storage database: ");
        QNCRITICAL(errorDescription << query.lastError() << ", last query: " << query.lastQuery());
        errorDescription += query.lastError().text();
        return -1;
    }

    if (!query.next()) {
        QNDEBUG("Found no tags in local storage database");
        return 0;
    }

    bool conversionResult = false;
    int count = query.value(0).toInt(&conversionResult);
    if (!conversionResult) {
        errorDescription = QT_TR_NOOP("Internal error: can't convert number of tags to int");
        QNCRITICAL(errorDescription << ": " << query.value(0));
        return -1;
    }

    return count;
}

bool LocalStorageManagerPrivate::AddTag(const Tag & tag, QString & errorDescription)
{
    errorDescription = QT_TR_NOOP("Can't add tag to local storage database: ");
    QString error;

    bool res = tag.checkParameters(error);
    if (!res) {
        errorDescription += error;
        QNWARNING("Found invalid tag: " << tag);
        return false;
    }

    QString localGuid = tag.localGuid();

    QString column, guid;
    bool shouldCheckTagExistence = true;

    bool tagHasGuid = tag.hasGuid();
    if (tagHasGuid) {
        column = "guid";
        guid = tag.guid();

        if (localGuid.isEmpty()) {
            QString queryString = QString("SELECT localGuid FROM Tags WHERE guid = '%1'").arg(guid);
            QSqlQuery query(m_sqlDatabase);
            res = query.exec(queryString);
            DATABASE_CHECK_AND_SET_ERROR("can't find local guid corresponding to Tag's guid");

            if (query.next()) {
                localGuid = query.record().value("localGuid").toString();
            }

            if (!localGuid.isEmpty()) {
                errorDescription += QT_TR_NOOP("found existing local guid corresponding to Tag's guid");
                QNCRITICAL(errorDescription << ", guid: " << guid);
                return false;
            }

            localGuid = QUuid::createUuid().toString();
            shouldCheckTagExistence = false;
        }
    }
    else {
        column = "localGuid";
        guid = tag.localGuid();
    }

    if (shouldCheckTagExistence && RowExists("Tags", column, QVariant(guid))) {
        // TRANSLATOR explaining the reason of error
        errorDescription += QT_TR_NOOP("tag with specified ");
        errorDescription += column;
        // TRANSATOR previous part of the phrase was "tag with specified guid|localGuid "
        errorDescription += QT_TR_NOOP(" already exists in local storage");
        QNWARNING(errorDescription << ", " << column << ": " << guid);
        return false;
    }

    return InsertOrReplaceTag(tag, localGuid, errorDescription);
}

bool LocalStorageManagerPrivate::UpdateTag(const Tag & tag, QString & errorDescription)
{
    errorDescription = QT_TR_NOOP("Can't update tag in local storage database: ");
    QString error;

    bool res = tag.checkParameters(error);
    if (!res) {
        errorDescription += error;
        QNWARNING("Found invalid tag: " << tag);
        return false;
    }

    QString localGuid = tag.localGuid();

    QString column, guid;
    bool shouldCheckTagExistence = true;

    bool tagHasGuid = tag.hasGuid();
    if (tagHasGuid) {
        column = "guid";
        guid = tag.guid();

        if (localGuid.isEmpty()) {
            QString queryString = QString("SELECT localGuid FROM Tags WHERE guid = '%1'").arg(guid);
            QSqlQuery query(m_sqlDatabase);
            res = query.exec(queryString);
            DATABASE_CHECK_AND_SET_ERROR("can't find local guid corresponding to Tag's guid");

            if (query.next()) {
                localGuid = query.record().value("localGuid").toString();
            }

            if (localGuid.isEmpty()) {
                errorDescription += QT_TR_NOOP("no existing local guid corresponding to "
                                               "Tag's guid was found in local storage");
                QNCRITICAL(errorDescription << ", guid: " << guid);
                return false;
            }

            shouldCheckTagExistence = false;
        }
    }
    else {
        column = "localGuid";
        guid = tag.localGuid();
    }

    if (shouldCheckTagExistence && !RowExists("Tags", column, QVariant(guid))) {
        // TRANSLATOR explaining the reason of error
        errorDescription += QT_TR_NOOP("tag with specified ");
        errorDescription += column;
        // TRANSLATOR previous part of the phrase was "tag with specified guid|localGuid"
        errorDescription += QT_TR_NOOP(" was not found in local storage");
        QNWARNING(errorDescription << ", " << column << ": " << guid);
        return false;
    }

    return InsertOrReplaceTag(tag, localGuid, errorDescription);
}

bool LocalStorageManagerPrivate::LinkTagWithNote(const Tag & tag, const Note & note,
                                                 QString & errorDescription)
{
    errorDescription = QT_TR_NOOP("Can't link tag with note: ");
    QString error;

    if (!tag.checkParameters(error)) {
        errorDescription += QT_TR_NOOP("tag is invalid: ");
        errorDescription += error;
        QNWARNING(errorDescription);
        return false;
    }

    if (!note.checkParameters(errorDescription)) {
        errorDescription += QT_TR_NOOP("note is invalid: ");
        errorDescription += error;
        QNWARNING(errorDescription);
        return false;
    }

    QString columns = "localNote, localTag, tagIndexInNote";
    bool tagHasGuid = tag.hasGuid();
    if (tagHasGuid) {
        columns.append(", tag");
    }

    bool noteHasGuid = note.hasGuid();
    if (noteHasGuid) {
        columns.append(", note");
    }

    QString valuesString = ":localNote, :localTag, :tagIndexInNote";
    if (tagHasGuid) {
        valuesString.append(", :tag");
    }

    if (noteHasGuid) {
        valuesString.append(", :note");
    }

    QString queryString = QString("INSERT OR REPLACE INTO NoteTags (%1) VALUES(%2)").arg(columns).arg(valuesString);
    QSqlQuery query(m_sqlDatabase);
    bool res = query.prepare(queryString);
    DATABASE_CHECK_AND_SET_ERROR("can't insert or replace data into \"NoteTags\" table: can't prepare SQL query");

    query.bindValue(":localNote", note.localGuid());
    query.bindValue(":localTag", tag.localGuid());

    QStringList tagGuids;
    note.tagGuids(tagGuids);
    int numTagGuids = tagGuids.size();

    bool foundAndBindedGuidIndex = false;
    if (tagHasGuid) {
        int tagGuidIndexInNote = tagGuids.indexOf(tag.guid());
        if (tagGuidIndexInNote >= 0) {
            query.bindValue(":tagIndexInNote", tagGuidIndexInNote);
            foundAndBindedGuidIndex = true;
        }
    }

    if (!foundAndBindedGuidIndex) {
        query.bindValue(":tagIndexInNote", numTagGuids);
    }

    if (tagHasGuid) {
        query.bindValue(":tag", tag.guid());
    }

    if (noteHasGuid) {
        query.bindValue(":note", note.guid());
    }

    res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("can't insert or replace data into \"NoteTags\" table "
                                 "in SQL database");

    return true;
}

bool LocalStorageManagerPrivate::FindTag(Tag & tag, QString & errorDescription) const
{
    QNDEBUG("LocalStorageManagerPrivate::FindTag");

    QString errorPrefix = QT_TR_NOOP("Can't find tag in local storage database: ");

    QString column, value;
    bool tagHasGuid = tag.hasGuid();
    if (tagHasGuid)
    {
        column = "guid";
        value = tag.guid();

        if (!CheckGuid(value)) {
            // TRANSLATION explaining why tag cannot be found in local storage
            errorDescription = errorPrefix + QT_TR_NOOP("requested tag guid is invalid");
            QNWARNING(errorDescription);
            return false;
        }
    }
    else if (tag.localGuid().isEmpty())
    {
        if (!tag.hasName()) {
            errorDescription = errorPrefix + QT_TR_NOOP("can't find tag: need either guid "
                                                        "or local guid or name as search criteria");
            QNWARNING(errorDescription);
            return false;
        }

        column = "nameUpper";
        value = tag.name().toUpper();
    }
    else
    {
        column = "localGuid";
        value = tag.localGuid();
    }

    QString queryString = QString("SELECT localGuid, guid, linkedNotebookGuid, updateSequenceNumber, "
                                  "name, parentGuid, isDirty, isLocal, isLocal, isDeleted, hasShortcut "
                                  "FROM Tags WHERE (%1 = '%2'").arg(column).arg(value);

    if (tag.hasLinkedNotebookGuid()) {
        queryString += QString(" AND linkedNotebookGuid = '%1')").arg(tag.linkedNotebookGuid());
    }
    else {
        queryString += " AND linkedNotebookGuid IS NULL)";
    }

    tag.clear();

    QSqlQuery query(m_sqlDatabase);
    bool res = query.exec(queryString);
    DATABASE_CHECK_AND_SET_ERROR("can't select tag from \"Tags\" table in SQL database: ");

    if (!query.next()) {
        errorDescription = errorPrefix + QT_TR_NOOP("no tag was found in local storage");
        QNDEBUG(errorDescription);
        return false;
    }

    QSqlRecord record = query.record();
    return FillTagFromSqlRecord(record, tag, errorDescription);
}

QList<Tag> LocalStorageManagerPrivate::ListAllTagsPerNote(const Note & note, QString & errorDescription,
                                                          const LocalStorageManager::ListObjectsOptions & flag,
                                                          const size_t limit, const size_t offset,
                                                          const LocalStorageManager::ListTagsOrder::type & order,
                                                          const LocalStorageManager::OrderDirection::type & orderDirection) const
{
    QNDEBUG("LocalStorageManagerPrivate::ListAllTagsPerNote");

    QList<Tag> tags;
    QString errorPrefix = QT_TR_NOOP("Can't list all tags per note in local storage database: ");

    QString column, guid;
    bool noteHasGuid = note.hasGuid();
    if (noteHasGuid) {
        column = "note";
        guid = note.guid();

        if (!CheckGuid(guid)) {
            // TRANSLATOR explaining why all tags per note cannot be listed
            errorDescription = errorPrefix + QT_TR_NOOP("note's guid is invalid");
            QNWARNING(errorDescription);
            return tags;
        }
    }
    else {
        column = "localNote";
        guid = note.localGuid();
    }

    // Will run all the queries from this method and its sub-methods within a single transaction
    // to prevent multiple drops and re-obtainings of shared lock
    Transaction transaction(m_sqlDatabase, *this, Transaction::Selection);
    Q_UNUSED(transaction)

    QString queryString = QString("SELECT localTag FROM NoteTags WHERE %1 = '%2'").arg(column).arg(guid);
    QSqlQuery query(m_sqlDatabase);
    bool res = query.exec(queryString);
    if (!res) {
        errorDescription = errorPrefix + QT_TR_NOOP("can't select tag guids from \"NoteTags\" table in SQL database: ");
        QNWARNING(errorDescription << "last error = " << query.lastError() << ", last query = " << query.lastQuery());
        errorDescription += query.lastError().text();
        return tags;
    }

    if (query.size() == 0) {
        QNDEBUG("No tags for this note were found");
        return tags;
    }

    QStringList tagLocalGuids;
    tagLocalGuids.reserve(std::max(query.size(), 0));

    while(query.next())
    {
        tagLocalGuids << QString();
        QString & tagLocalGuid = tagLocalGuids.back();
        tagLocalGuid = query.value(0).toString();

        if (tagLocalGuid.isEmpty()) {
            errorDescription = QT_TR_NOOP("Internal error: no tag's local guid in the result of SQL query");
            tags.clear();
            return tags;
        }
    }

    QString noteGuidSqlQueryCondition = "localGuid IN (";
    const int numTagLocalGuids = tagLocalGuids.size();
    for(int i = 0; i < numTagLocalGuids; ++i)
    {
        noteGuidSqlQueryCondition += QString("'%1'").arg(tagLocalGuids[i]);
        if (i != (numTagLocalGuids - 1)) {
            noteGuidSqlQueryCondition += ", ";
        }
    }
    noteGuidSqlQueryCondition += ")";

    return listObjects<Tag, LocalStorageManager::ListTagsOrder::type>(flag, errorDescription, limit, offset, order,
                                                                      orderDirection, noteGuidSqlQueryCondition);
}

QList<Tag> LocalStorageManagerPrivate::ListAllTags(QString & errorDescription, const size_t limit, const size_t offset,
                                                   const LocalStorageManager::ListTagsOrder::type & order,
                                                   const LocalStorageManager::OrderDirection::type & orderDirection,
                                                   const QString & linkedNotebookGuid) const
{
    QNDEBUG("LocalStorageManagerPrivate::ListAllTags");
    return ListTags(LocalStorageManager::ListAll, errorDescription, limit, offset, order, orderDirection, linkedNotebookGuid);
}

QList<Tag> LocalStorageManagerPrivate::ListTags(const LocalStorageManager::ListObjectsOptions flag,
                                                QString & errorDescription, const size_t limit, const size_t offset,
                                                const LocalStorageManager::ListTagsOrder::type & order,
                                                const LocalStorageManager::OrderDirection::type & orderDirection,
                                                const QString & linkedNotebookGuid) const
{
    QNDEBUG("LocalStorageManagerPrivate::ListTags: flag = " << flag);

    QString linkedNotebookGuidSqlQueryCondition = (linkedNotebookGuid.isEmpty()
                                                   ? "linkedNotebookGuid IS NULL"
                                                   : QString("linkedNotebookGuid = '%1'").arg(linkedNotebookGuid));

    return listObjects<Tag, LocalStorageManager::ListTagsOrder::type>(flag, errorDescription, limit, offset, order, orderDirection,
                                                                      linkedNotebookGuidSqlQueryCondition);
}

bool LocalStorageManagerPrivate::DeleteTag(const Tag & tag, QString & errorDescription)
{
    errorDescription = QT_TR_NOOP("Can't delete tag from local storage database: ");

    if (!tag.isDeleted()) {
        // TRANSLATOR explaining why tag cannot be marked as deleted in local storage
        errorDescription += QT_TR_NOOP("tag to be deleted is not marked correspondingly");
        QNWARNING(errorDescription);
        return false;
    }

    bool res = CheckAndPrepareDeleteTagQuery();
    QSqlQuery & query = m_deleteTagQuery;
    DATABASE_CHECK_AND_SET_ERROR("can't mark tag deleted in \"Tags\" table: "
                                 "can't prepare SQL query");

    query.bindValue(":localGuid", tag.localGuid());
    res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("can't mark tag deleted in \"Tags\" table in SQL database");

    return true;
}

bool LocalStorageManagerPrivate::ExpungeTag(const Tag & tag, QString & errorDescription)
{
    errorDescription = QT_TR_NOOP("Can't expunge tag from local storage database: ");

    bool exists = RowExists("Tags", "localGuid", QVariant(tag.localGuid()));
    if (!exists) {
        errorDescription += QT_TR_NOOP("tag to be expunged was not found by local guid");
        QNWARNING(errorDescription << ", guid: " << tag.localGuid());
        return false;
    }

    bool res = CheckAndPrepareExpungeTagQuery();
    QSqlQuery & query = m_expungeTagQuery;
    DATABASE_CHECK_AND_SET_ERROR("can't delete tag from \"Tags\" table: "
                                 "can't prepare SQL query");

    query.bindValue(":localGuid", tag.localGuid());

    res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("can't delete tag from \"Tags\" table in SQL database");

    return true;
}

bool LocalStorageManagerPrivate::ExpungeNotelessTagsFromLinkedNotebooks(QString & errorDescription)
{
    errorDescription = QT_TR_NOOP("Can't expunge tags from linked notebooks not connected to any notes: ");

    QString queryString = "DELETE FROM Tags WHERE ((linkedNotebookGuid IS NOT NULL) AND "
                          "(localGuid NOT IN (SELECT localTag FROM NoteTags)))";
    QSqlQuery query(m_sqlDatabase);
    bool res = query.exec(queryString);
    DATABASE_CHECK_AND_SET_ERROR("can't delete tags from \"Tags\" table in SQL database");

    return true;
}

int LocalStorageManagerPrivate::GetEnResourceCount(QString & errorDescription) const
{
    bool res = CheckAndPrepareGetResourceCountQuery();
    QSqlQuery & query = m_getResourceCountQuery;
    if (!res) {
        errorDescription = QT_TR_NOOP("Internal error: can't get number of resources in local storage database: "
                                      "can't prepare SQL query: ");
        QNCRITICAL(errorDescription << query.lastError() << ", last query: " << query.lastQuery());
        errorDescription += query.lastError().text();
        return -1;
    }

    res = query.exec();
    if (!res) {
        errorDescription = QT_TR_NOOP("Internal error: can't get number of resources in local storage database: ");
        QNCRITICAL(errorDescription << query.lastError() << ", last query: " << query.lastQuery());
        errorDescription += query.lastError().text();
        return -1;
    }

    if (!query.next()) {
        QNDEBUG("Found no resources in local storage database");
        return 0;
    }

    bool conversionResult = false;
    int count = query.value(0).toInt(&conversionResult);
    if (!conversionResult) {
        errorDescription = QT_TR_NOOP("Internal error: can't convert number of resources to int");
        QNCRITICAL(errorDescription << ": " << query.value(0));
        return -1;
    }

    return count;
}

bool LocalStorageManagerPrivate::FindEnResource(IResource & resource, QString & errorDescription,
                                                const bool withBinaryData) const
{
    QNDEBUG("LocalStorageManagerPrivate::FindEnResource");

    errorDescription = QT_TR_NOOP("Can't find resource in local storage database: ");

    QString column, guid;
    bool resourceHasGuid = resource.hasGuid();
    if (resourceHasGuid) {
        column = "resourceGuid";
        guid = resource.guid();

        if (!CheckGuid(guid)) {
            // TRANSLATOR explaining why resource cannot be found in local storage
            errorDescription += QT_TR_NOOP("requested resource guid is invalid");
            QNWARNING(errorDescription);
            return false;
        }
    }
    else {
        column = "resourceLocalGuid";
        guid = resource.localGuid();
    }

    resource.clear();

    QString queryString = QString("SELECT * FROM %1 "
                                  "LEFT OUTER JOIN ResourceAttributes "
                                  "ON %1.resourceLocalGuid = ResourceAttributes.resourceLocalGuid "
                                  "LEFT OUTER JOIN ResourceAttributesApplicationDataKeysOnly "
                                  "ON %1.resourceLocalGuid = ResourceAttributesApplicationDataKeysOnly.resourceLocalGuid "
                                  "LEFT OUTER JOIN ResourceAttributesApplicationDataFullMap "
                                  "ON %1.resourceLocalGuid = ResourceAttributesApplicationDataFullMap.resourceLocalGuid "
                                  "LEFT OUTER JOIN NoteResources ON %1.resourceLocalGuid = NoteResources.localResource "
                                  "WHERE %1.%2 = '%3'")
                                 .arg(withBinaryData ? "Resources" : "ResourcesWithoutBinaryData")
                                 .arg(column).arg(guid);

    QSqlQuery query(m_sqlDatabase);
    bool res = query.exec(queryString);
    DATABASE_CHECK_AND_SET_ERROR("can't find resource in \"Resources\" table in SQL database");

    size_t counter = 0;
    while(query.next()) {
        QSqlRecord rec = query.record();
        FillResourceFromSqlRecord(rec, withBinaryData, resource);
        ++counter;
    }

    if (!counter) {
        errorDescription += QT_TR_NOOP("requested resource was found in local storage");
        QNDEBUG(errorDescription);
        return false;
    }

    return true;
}

bool LocalStorageManagerPrivate::ExpungeEnResource(const IResource & resource, QString & errorDescription)
{
    errorDescription = QT_TR_NOOP("Can't expunge resource from local storage database: ");

    QString column, guid;
    bool resourceHasGuid = resource.hasGuid();
    if (resourceHasGuid) {
        column = "resourceGuid";
        guid = resource.guid();

        if (!CheckGuid(guid)) {
            // TRANSLATOR explaining why resource could not be expunged from local storage
            errorDescription += QT_TR_NOOP("requested resource guid is invalid");
            QNWARNING(errorDescription);
            return false;
        }
    }
    else {
        column = "resourceLocalGuid";
        guid = resource.localGuid();
    }

    bool exists = RowExists("Resources", column, QVariant(guid));
    if (!exists) {
        errorDescription += QT_TR_NOOP("resource to be expunged was not found");
        QNWARNING(errorDescription << ", " << column << ": " << guid);
        return false;
    }

    QString queryString = QString("DELETE FROM Resources WHERE %1 = \"%2\"").arg(column).arg(guid);
    QSqlQuery query(m_sqlDatabase);
    bool res = query.exec(queryString);
    DATABASE_CHECK_AND_SET_ERROR("can't delete resource from \"Resources\" table in SQL database");

    return true;
}

int LocalStorageManagerPrivate::GetSavedSearchCount(QString & errorDescription) const
{
    QSqlQuery & query = m_getSavedSearchCountQuery;
    bool res = CheckAndPrepareGetSavedSearchCountQuery();
    if (!res) {
        errorDescription = QT_TR_NOOP("Internal error: can't prepare SQL query to get the number "
                                      "of saved searches in local storage database: ");
        QNCRITICAL(errorDescription << query.lastError() << ", last query: " << query.lastQuery());
        errorDescription += query.lastError().text();
        return -1;
    }

    res = query.exec();
    if (!res) {
        errorDescription = QT_TR_NOOP("Internal error: can't get number of saved searches in local storage database: ");
        QNCRITICAL(errorDescription << query.lastError() << ", last query: " << query.lastQuery());
        errorDescription += query.lastError().text();
        return -1;
    }

    if (!query.next()) {
        QNDEBUG("Found no saved searches in local storage database");
        return 0;
    }

    bool conversionResult = false;
    int count = query.value(0).toInt(&conversionResult);
    if (!conversionResult) {
        errorDescription = QT_TR_NOOP("Internal error: can't convert number of saved searches to int");
        QNCRITICAL(errorDescription << ": " << query.value(0));
        return -1;
    }

    return count;
}

bool LocalStorageManagerPrivate::AddSavedSearch(const SavedSearch & search, QString & errorDescription)
{
    errorDescription = QT_TR_NOOP("Can't add saved search to local storage database: ");
    QString error;

    bool res = search.checkParameters(error);
    if (!res) {
        errorDescription += error;
        QNWARNING("Found invalid SavedSearch: " << search << "\nError: " << error);
        return false;
    }

    QString localGuid = search.localGuid();

    QString column, guid;
    bool shouldCheckSearchExistence = true;

    bool searchHasGuid = search.hasGuid();
    if (searchHasGuid) {
        column = "guid";
        guid = search.guid();

        if (localGuid.isEmpty()) {
            QString queryString = QString("SELECT localGuid FROM SavedSearches WHERE guid = '%1'").arg(guid);
            QSqlQuery query(m_sqlDatabase);
            res = query.exec(queryString);
            DATABASE_CHECK_AND_SET_ERROR("can't find local guid corresponding to SavedSearch's guid");

            if (query.next()) {
                localGuid = query.record().value("localGuid").toString();
            }

            if (!localGuid.isEmpty()) {
                errorDescription += QT_TR_NOOP("found existing local guid corresponding to Resource's guid");
                QNCRITICAL(errorDescription << ", guid: " << guid);
                return false;
            }

            localGuid = QUuid::createUuid().toString();
            shouldCheckSearchExistence = false;
        }
    }
    else {
        column = "localGuid";
        guid = search.localGuid();
    }

    if (shouldCheckSearchExistence && RowExists("SavedSearches", column, QVariant(guid))) {
        // TRANSLATOR explaining why saved search could not be added to local storage
        errorDescription += QT_TR_NOOP("saved search with specified ");
        errorDescription += column;
        // TRANSLATOR prevous part of the phrase was "saved search with specified guid|localGuid "
        errorDescription += QT_TR_NOOP(" already exists in local storage");
        QNWARNING(errorDescription << ", " << column << ": " << guid);
        return false;
    }

    return InsertOrReplaceSavedSearch(search, localGuid, errorDescription);
}

bool LocalStorageManagerPrivate::UpdateSavedSearch(const SavedSearch & search,
                                            QString & errorDescription)
{
    errorDescription = QT_TR_NOOP("Can't update saved search in local storage database: ");
    QString error;

    bool res = search.checkParameters(error);
    if (!res) {
        errorDescription += error;
        QNWARNING("Found invalid SavedSearch: " << search << "\nError: " << error);
        return false;
    }

    QString localGuid = search.localGuid();

    QString column, guid;
    bool shouldCheckSearchExistence = true;

    bool searchHasGuid = search.hasGuid();
    if (searchHasGuid) {
        column = "guid";
        guid = search.guid();

        if (localGuid.isEmpty()) {
            QString queryString = QString("SELECT localGuid FROM SavedSearches WHERE guid = '%1'").arg(guid);
            QSqlQuery query(m_sqlDatabase);
            res = query.exec(queryString);
            DATABASE_CHECK_AND_SET_ERROR("can't find local guid correspinding to SavedSearch's guid");

            if (query.next()) {
                localGuid = query.record().value("localGuid").toString();
            }

            if (localGuid.isEmpty()) {
                errorDescription += QT_TR_NOOP("no existing local guid corresponding to Resource's guid was found in local storage");
                QNCRITICAL(errorDescription << ", guid: " << guid);
                return false;
            }

            shouldCheckSearchExistence = false;
        }
    }
    else {
        column = "localGuid";
        guid = search.localGuid();
    }

    if (shouldCheckSearchExistence && !RowExists("SavedSearches", column, QVariant(guid))) {
        // TRANSLATOR explaining why saved search could not be added to local storage
        errorDescription += QT_TR_NOOP("saved search with specified ");
        errorDescription += column;
        // TRANSLATOR prevous part of the phrase was "saved search with specified guid|localGuid "
        errorDescription += QT_TR_NOOP(" was not found in local storage");
        QNWARNING(errorDescription << ", " << column << ": " << guid);
        return false;
    }

    return InsertOrReplaceSavedSearch(search, localGuid, errorDescription);
}

bool LocalStorageManagerPrivate::FindSavedSearch(SavedSearch & search, QString & errorDescription) const
{
    QNDEBUG("LocalStorageManagerPrivate::FindSavedSearch");

    errorDescription = QT_TR_NOOP("Can't find saved search in local storage database: ");

    QString column, value;
    bool searchHasGuid = search.hasGuid();
    if (searchHasGuid)
    {
        column = "guid";
        value = search.guid();

        if (!CheckGuid(value)) {
            errorDescription += QT_TR_NOOP("requested saved search guid is invalid");
            return false;
        }
    }
    else if (search.localGuid().isEmpty())
    {
        if (!search.hasName()) {
            errorDescription += QT_TR_NOOP("can't find saved search: need either guid "
                                           "or local guid or name as search criteria");
            QNWARNING(errorDescription);
            return false;
        }

        column = "nameUpper";
        value = search.name().toUpper();
    }
    else
    {
        column = "localGuid";
        value = search.localGuid();
    }

    search.clear();

    QString queryString = QString("SELECT localGuid, guid, name, query, format, updateSequenceNumber, isDirty, isLocal, "
                                  "includeAccount, includePersonalLinkedNotebooks, includeBusinessLinkedNotebooks, "
                                  "hasShortcut FROM SavedSearches WHERE %1 = '%2'").arg(column).arg(value);
    QSqlQuery query(m_sqlDatabase);
    bool res = query.exec(queryString);
    DATABASE_CHECK_AND_SET_ERROR("can't find saved search in \"SavedSearches\" table in SQL database");

    if (!query.next()) {
        errorDescription += QT_TR_NOOP("no saved search was found in local storage");
        QNDEBUG(errorDescription);
        return false;
    }

    QSqlRecord rec = query.record();
    return FillSavedSearchFromSqlRecord(rec, search, errorDescription);
}

QList<SavedSearch> LocalStorageManagerPrivate::ListAllSavedSearches(QString & errorDescription, const size_t limit, const size_t offset,
                                                                    const LocalStorageManager::ListSavedSearchesOrder::type & order,
                                                                    const LocalStorageManager::OrderDirection::type & orderDirection) const
{
    QNDEBUG("LocalStorageManagerPrivate::ListAllSavedSearches");
    return ListSavedSearches(LocalStorageManager::ListAll, errorDescription, limit, offset, order, orderDirection);
}

QList<SavedSearch> LocalStorageManagerPrivate::ListSavedSearches(const LocalStorageManager::ListObjectsOptions flag,
                                                                 QString & errorDescription, const size_t limit, const size_t offset,
                                                                 const LocalStorageManager::ListSavedSearchesOrder::type & order,
                                                                 const LocalStorageManager::OrderDirection::type & orderDirection) const
{
    QNDEBUG("LocalStorageManagerPrivate::ListSavedSearches: flag = " << flag);
    return listObjects<SavedSearch, LocalStorageManager::ListSavedSearchesOrder::type>(flag, errorDescription, limit, offset, order, orderDirection);
}

bool LocalStorageManagerPrivate::ExpungeSavedSearch(const SavedSearch & search,
                                                    QString & errorDescription)
{
    errorDescription = QT_TR_NOOP("Can't expunge saved search from local storage database: ");

    QString localGuid = search.localGuid();
    if (localGuid.isEmpty()) {
        errorDescription += QT_TR_NOOP("saved search's local guid is not set");
        QNWARNING(errorDescription);
        return false;
    }

    bool exists = RowExists("SavedSearches", "localGuid", QVariant(localGuid));
    if (!exists) {
        errorDescription += QT_TR_NOOP("saved search to be expunged was not found");
        QNWARNING(errorDescription);
        return false;
    }

    bool res = CheckAndPrepareExpungeSavedSearchQuery();
    QSqlQuery & query = m_expungeSavedSearchQuery;
    DATABASE_CHECK_AND_SET_ERROR("can't delete saved search from SQL database: can't prepare SQL query");

    query.bindValue(":localGuid", localGuid);
    res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("can't delete saved search from \"SavedSearches\" table in SQL database");

    return true;
}

void LocalStorageManagerPrivate::ProcessPostTransactionException(QString message, QSqlError error) const
{
    QNCRITICAL(message << ": " << error);
    message += ", last error: ";
    message += error.text();
    throw DatabaseSqlErrorException(message);
}

bool LocalStorageManagerPrivate::AddEnResource(const IResource & resource, const Note & note, QString & errorDescription)
{
    errorDescription = QT_TR_NOOP("Can't add resource to local storage database: ");

    QString error;
    if (!resource.checkParameters(error)) {
        errorDescription += error;
        QNWARNING("Found invalid resource: " << resource);
        return false;
    }

    if (!note.hasGuid() && note.localGuid().isEmpty()) {
        errorDescription += QT_TR_NOOP("both local and remote note's guids are empty");
        QNWARNING(errorDescription << ", note: " << note);
    }

    QString resourceLocalGuid = resource.localGuid();

    QString column, guid;
    bool shouldCheckResourceExistence = true;

    bool resourceHasGuid = resource.hasGuid();
    if (resourceHasGuid) {
        column = "resourceGuid";
        guid = resource.guid();

        if (resourceLocalGuid.isEmpty()) {
            QString queryString = QString("SELECT resourceLocalGuid FROM Resources WHERE resourceGuid = '%1'").arg(guid);
            QSqlQuery query(m_sqlDatabase);
            bool res = query.exec(queryString);
            DATABASE_CHECK_AND_SET_ERROR("can't find local guid corresponding to Resource's guid");

            if (query.next()) {
                resourceLocalGuid = query.record().value("resourceLocalGuid").toString();
            }

            if (!resourceLocalGuid.isEmpty()) {
                errorDescription += QT_TR_NOOP("found existing local guid corresponding to Resource's guid");
                QNCRITICAL(errorDescription << ", guid: " << guid);
            }

            resourceLocalGuid = QUuid::createUuid().toString();
            shouldCheckResourceExistence = false;
        }
    }
    else {
        column = "resourceLocalGuid";
        guid = resource.localGuid();
    }

    if (shouldCheckResourceExistence && RowExists("Resources", column, QVariant(guid))) {
        // TRANSLATOR explaining the reason of error
        errorDescription += QT_TR_NOOP("resource with specified ");
        errorDescription += column;
        // TRANSLATOR previous part of the phrase was "resource with specified guid|localGuid "
        errorDescription += QT_TR_NOOP(" already exists in local storage");
        QNWARNING(errorDescription << ", " << column << ": " << guid);
        return false;
    }

    return InsertOrReplaceResource(resource, resourceLocalGuid, note, QString(), errorDescription);
}

bool LocalStorageManagerPrivate::UpdateEnResource(const IResource & resource, const Note &note, QString & errorDescription)
{
    errorDescription = QT_TR_NOOP("Can't update resource in local storage database: ");

    QString error;
    if (!resource.checkParameters(error)) {
        errorDescription += error;
        QNWARNING("Found invalid resource: " << resource);
        return false;
    }

    if (!note.hasGuid() && note.localGuid().isEmpty()) {
        errorDescription += QT_TR_NOOP("both local and remote note's guids are empty");
        QNWARNING(errorDescription << ", note: " << note);
    }

    QString resourceLocalGuid = resource.localGuid();

    QString column, guid;
    bool shouldCheckResourceExistence = true;

    bool resourceHasGuid = resource.hasGuid();
    if (resourceHasGuid) {
        column = "resourceGuid";
        guid = resource.guid();

        if (resourceLocalGuid.isEmpty()) {
            QString queryString = QString("SELECT resourceLocalGuid FROM Resources WHERE resourceGuid = '%1'").arg(guid);
            QSqlQuery query(m_sqlDatabase);
            bool res = query.exec(queryString);
            DATABASE_CHECK_AND_SET_ERROR("can't find local guid corresponding to Resource's guid");

            if (query.next()) {
                resourceLocalGuid = query.record().value("resourceLocalGuid").toString();
            }

            if (resourceLocalGuid.isEmpty()) {
                errorDescription += QT_TR_NOOP("no existing local guid corresponding to Resource's guid was found in local storage");
                QNCRITICAL(errorDescription << ", guid: " << guid);
                return false;
            }

            shouldCheckResourceExistence = false;
        }
    }
    else {
        column = "resourceLocalGuid";
        guid = resource.localGuid();
    }

    if (shouldCheckResourceExistence && !RowExists("Resources", column, QVariant(guid))) {
        // TRANSLATOR explaining why resource cannot be updated in local storage
        errorDescription += QT_TR_NOOP("resource with specified ");
        errorDescription += column;
        // TRANSLATOR previous part of the phrase was "resource with specified guid|localGuid"
        errorDescription += QT_TR_NOOP(" was not found in local storage");
        QNWARNING(errorDescription << ", " << column << ": " << guid);
        return false;
    }

    return InsertOrReplaceResource(resource, resourceLocalGuid, note, QString(), errorDescription);
}

bool LocalStorageManagerPrivate::CreateTables(QString & errorDescription)
{
    QSqlQuery query(m_sqlDatabase);
    bool res;

    res = query.exec("CREATE TABLE IF NOT EXISTS Users("
                     "  id                          INTEGER PRIMARY KEY     NOT NULL UNIQUE, "
                     "  username                    TEXT                    DEFAULT NULL, "
                     "  email                       TEXT                    DEFAULT NULL, "
                     "  name                        TEXT                    DEFAULT NULL, "
                     "  timezone                    TEXT                    DEFAULT NULL, "
                     "  privilege                   INTEGER                 DEFAULT NULL, "
                     "  userCreationTimestamp       INTEGER                 DEFAULT NULL, "
                     "  userModificationTimestamp   INTEGER                 DEFAULT NULL, "
                     "  userIsDirty                 INTEGER                 NOT NULL, "
                     "  userIsLocal                 INTEGER                 NOT NULL, "
                     "  userDeletionTimestamp       INTEGER                 DEFAULT NULL, "
                     "  userIsActive                INTEGER                 DEFAULT NULL"
                     ")");
    DATABASE_CHECK_AND_SET_ERROR("can't create Users table");

    res = query.exec("CREATE TABLE IF NOT EXISTS UserAttributes("
                     "  id REFERENCES Users(id) ON DELETE CASCADE ON UPDATE CASCADE, "
                     "  defaultLocationName         TEXT                    DEFAULT NULL, "
                     "  defaultLatitude             REAL                    DEFAULT NULL, "
                     "  defaultLongitude            REAL                    DEFAULT NULL, "
                     "  preactivation               INTEGER                 DEFAULT NULL, "
                     "  incomingEmailAddress        TEXT                    DEFAULT NULL, "
                     "  comments                    TEXT                    DEFAULT NULL, "
                     "  dateAgreedToTermsOfService  INTEGER                 DEFAULT NULL, "
                     "  maxReferrals                INTEGER                 DEFAULT NULL, "
                     "  referralCount               INTEGER                 DEFAULT NULL, "
                     "  refererCode                 TEXT                    DEFAULT NULL, "
                     "  sentEmailDate               INTEGER                 DEFAULT NULL, "
                     "  sentEmailCount              INTEGER                 DEFAULT NULL, "
                     "  dailyEmailLimit             INTEGER                 DEFAULT NULL, "
                     "  emailOptOutDate             INTEGER                 DEFAULT NULL, "
                     "  partnerEmailOptInDate       INTEGER                 DEFAULT NULL, "
                     "  preferredLanguage           TEXT                    DEFAULT NULL, "
                     "  preferredCountry            TEXT                    DEFAULT NULL, "
                     "  clipFullPage                INTEGER                 DEFAULT NULL, "
                     "  twitterUserName             TEXT                    DEFAULT NULL, "
                     "  twitterId                   TEXT                    DEFAULT NULL, "
                     "  groupName                   TEXT                    DEFAULT NULL, "
                     "  recognitionLanguage         TEXT                    DEFAULT NULL, "
                     "  referralProof               TEXT                    DEFAULT NULL, "
                     "  educationalDiscount         INTEGER                 DEFAULT NULL, "
                     "  businessAddress             TEXT                    DEFAULT NULL, "
                     "  hideSponsorBilling          INTEGER                 DEFAULT NULL, "
                     "  taxExempt                   INTEGER                 DEFAULT NULL, "
                     "  useEmailAutoFiling          INTEGER                 DEFAULT NULL, "
                     "  reminderEmailConfig         INTEGER                 DEFAULT NULL)");
    DATABASE_CHECK_AND_SET_ERROR("can't create UserAttributes table");

    res = query.exec("CREATE TABLE IF NOT EXISTS UserAttributesViewedPromotions("
                     "  id REFERENCES Users(id) ON DELETE CASCADE ON UPDATE CASCADE, "
                     "  promotion               TEXT                    DEFAULT NULL)");
    DATABASE_CHECK_AND_SET_ERROR("can't create UserAttributesViewedPromotions table");

    res = query.exec("CREATE TABLE IF NOT EXISTS UserAttributesRecentMailedAddresses("
                     "  id REFERENCES Users(id) ON DELETE CASCADE ON UPDATE CASCADE, "
                     "  address                 TEXT                    DEFAULT NULL)");
    DATABASE_CHECK_AND_SET_ERROR("can't create UserAttributesRecentMailedAddresses table");

    res = query.exec("CREATE TABLE IF NOT EXISTS Accounting("
                     "  id REFERENCES Users(id) ON DELETE CASCADE ON UPDATE CASCADE, "
                     "  uploadLimit                 INTEGER             DEFAULT NULL, "
                     "  uploadLimitEnd              INTEGER             DEFAULT NULL, "
                     "  uploadLimitNextMonth        INTEGER             DEFAULT NULL, "
                     "  premiumServiceStatus        INTEGER             DEFAULT NULL, "
                     "  premiumOrderNumber          TEXT                DEFAULT NULL, "
                     "  premiumCommerceService      TEXT                DEFAULT NULL, "
                     "  premiumServiceStart         INTEGER             DEFAULT NULL, "
                     "  premiumServiceSKU           TEXT                DEFAULT NULL, "
                     "  lastSuccessfulCharge        INTEGER             DEFAULT NULL, "
                     "  lastFailedCharge            INTEGER             DEFAULT NULL, "
                     "  lastFailedChargeReason      TEXT                DEFAULT NULL, "
                     "  nextPaymentDue              INTEGER             DEFAULT NULL, "
                     "  premiumLockUntil            INTEGER             DEFAULT NULL, "
                     "  updated                     INTEGER             DEFAULT NULL, "
                     "  premiumSubscriptionNumber   TEXT                DEFAULT NULL, "
                     "  lastRequestedCharge         INTEGER             DEFAULT NULL, "
                     "  currency                    TEXT                DEFAULT NULL, "
                     "  unitPrice                   INTEGER             DEFAULT NULL, "
                     "  accountingBusinessId        INTEGER             DEFAULT NULL, "
                     "  accountingBusinessName      TEXT                DEFAULT NULL, "
                     "  accountingBusinessRole      INTEGER             DEFAULT NULL, "
                     "  unitDiscount                INTEGER             DEFAULT NULL, "
                     "  nextChargeDate              INTEGER             DEFAULT NULL)");
    DATABASE_CHECK_AND_SET_ERROR("can't create Accounting table");

    res = query.exec("CREATE TABLE IF NOT EXISTS PremiumInfo("
                     "  id REFERENCES Users(id) ON DELETE CASCADE ON UPDATE CASCADE, "
                     "  currentTime                 INTEGER                 DEFAULT NULL, "
                     "  premium                     INTEGER                 DEFAULT NULL, "
                     "  premiumRecurring            INTEGER                 DEFAULT NULL, "
                     "  premiumExpirationDate       INTEGER                 DEFAULT NULL, "
                     "  premiumExtendable           INTEGER                 DEFAULT NULL, "
                     "  premiumPending              INTEGER                 DEFAULT NULL, "
                     "  premiumCancellationPending  INTEGER                 DEFAULT NULL, "
                     "  canPurchaseUploadAllowance  INTEGER                 DEFAULT NULL, "
                     "  sponsoredGroupName          INTEGER                 DEFAULT NULL, "
                     "  sponsoredGroupRole          INTEGER                 DEFAULT NULL, "
                     "  premiumUpgradable           INTEGER                 DEFAULT NULL)");
    DATABASE_CHECK_AND_SET_ERROR("can't create PremiumInfo table");

    res = query.exec("CREATE TABLE IF NOT EXISTS BusinessUserInfo("
                     "  id REFERENCES Users(id) ON DELETE CASCADE ON UPDATE CASCADE, "
                     "  businessId              INTEGER                 DEFAULT NULL, "
                     "  businessName            TEXT                    DEFAULT NULL, "
                     "  role                    INTEGER                 DEFAULT NULL, "
                     "  businessInfoEmail       TEXT                    DEFAULT NULL)");
    DATABASE_CHECK_AND_SET_ERROR("can't create BusinessUserInfo table");

    res = query.exec("CREATE TABLE IF NOT EXISTS LinkedNotebooks("
                     "  guid                            TEXT PRIMARY KEY  NOT NULL UNIQUE, "
                     "  updateSequenceNumber            INTEGER           DEFAULT NULL, "
                     "  isDirty                         INTEGER           DEFAULT NULL, "
                     "  shareName                       TEXT              DEFAULT NULL, "
                     "  username                        TEXT              DEFAULT NULL, "
                     "  shardId                         TEXT              DEFAULT NULL, "
                     "  shareKey                        TEXT              DEFAULT NULL, "
                     "  uri                             TEXT              DEFAULT NULL, "
                     "  noteStoreUrl                    TEXT              DEFAULT NULL, "
                     "  webApiUrlPrefix                 TEXT              DEFAULT NULL, "
                     "  stack                           TEXT              DEFAULT NULL, "
                     "  businessId                      INTEGER           DEFAULT NULL"
                     ")");
    DATABASE_CHECK_AND_SET_ERROR("can't create LinkedNotebooks table");

    res = query.exec("CREATE TABLE IF NOT EXISTS Notebooks("
                     "  localGuid                       TEXT PRIMARY KEY  NOT NULL UNIQUE, "
                     "  guid                            TEXT              DEFAULT NULL UNIQUE, "
                     "  linkedNotebookGuid REFERENCES LinkedNotebooks(guid) ON DELETE CASCADE ON UPDATE CASCADE, "
                     "  updateSequenceNumber            INTEGER           DEFAULT NULL, "
                     "  notebookName                    TEXT              DEFAULT NULL, "
                     "  notebookNameUpper               TEXT              DEFAULT NULL, "
                     "  creationTimestamp               INTEGER           DEFAULT NULL, "
                     "  modificationTimestamp           INTEGER           DEFAULT NULL, "
                     "  isDirty                         INTEGER           NOT NULL, "
                     "  isLocal                         INTEGER           NOT NULL, "
                     "  isDefault                       INTEGER           DEFAULT NULL UNIQUE, "
                     "  isLastUsed                      INTEGER           DEFAULT NULL UNIQUE, "
                     "  hasShortcut                     INTEGER           DEFAULT NULL, "
                     "  publishingUri                   TEXT              DEFAULT NULL, "
                     "  publishingNoteSortOrder         INTEGER           DEFAULT NULL, "
                     "  publishingAscendingSort         INTEGER           DEFAULT NULL, "
                     "  publicDescription               TEXT              DEFAULT NULL, "
                     "  isPublished                     INTEGER           DEFAULT NULL, "
                     "  stack                           TEXT              DEFAULT NULL, "
                     "  businessNotebookDescription     TEXT              DEFAULT NULL, "
                     "  businessNotebookPrivilegeLevel  INTEGER           DEFAULT NULL, "
                     "  businessNotebookIsRecommended   INTEGER           DEFAULT NULL, "
                     "  contactId                       INTEGER           DEFAULT NULL, "
                     "  UNIQUE(localGuid, guid), "
                     "  UNIQUE(linkedNotebookGuid, notebookNameUpper) "
                     ")");
    DATABASE_CHECK_AND_SET_ERROR("can't create Notebooks table");

    res = query.exec("CREATE VIRTUAL TABLE NotebookFTS USING FTS4(content=\"Notebooks\", "
                     "localGuid, guid, notebookName)");
    DATABASE_CHECK_AND_SET_ERROR("can't create virtual FTS4 NotebookFTS table");

    res = query.exec("CREATE TRIGGER NotebookFTS_BeforeDeleteTrigger BEFORE DELETE ON Notebooks "
                     "BEGIN "
                     "DELETE FROM NotebookFTS WHERE localGuid=old.localGuid; "
                     "END");
    DATABASE_CHECK_AND_SET_ERROR("can't create NotebookFTS_BeforeDeleteTrigger");

    res = query.exec("CREATE TRIGGER NotebookFTS_AfterInsertTrigger AFTER INSERT ON Notebooks "
                     "BEGIN "
                     "INSERT INTO NotebookFTS(NotebookFTS) VALUES('rebuild'); "
                     "END");
    DATABASE_CHECK_AND_SET_ERROR("can't create NotebookFTS_AfterInsertTrigger");

    res = query.exec("CREATE TABLE IF NOT EXISTS NotebookRestrictions("
                     "  localGuid REFERENCES Notebooks(localGuid) ON DELETE CASCADE ON UPDATE CASCADE, "
                     "  noReadNotes                 INTEGER      DEFAULT NULL, "
                     "  noCreateNotes               INTEGER      DEFAULT NULL, "
                     "  noUpdateNotes               INTEGER      DEFAULT NULL, "
                     "  noExpungeNotes              INTEGER      DEFAULT NULL, "
                     "  noShareNotes                INTEGER      DEFAULT NULL, "
                     "  noEmailNotes                INTEGER      DEFAULT NULL, "
                     "  noSendMessageToRecipients   INTEGER      DEFAULT NULL, "
                     "  noUpdateNotebook            INTEGER      DEFAULT NULL, "
                     "  noExpungeNotebook           INTEGER      DEFAULT NULL, "
                     "  noSetDefaultNotebook        INTEGER      DEFAULT NULL, "
                     "  noSetNotebookStack          INTEGER      DEFAULT NULL, "
                     "  noPublishToPublic           INTEGER      DEFAULT NULL, "
                     "  noPublishToBusinessLibrary  INTEGER      DEFAULT NULL, "
                     "  noCreateTags                INTEGER      DEFAULT NULL, "
                     "  noUpdateTags                INTEGER      DEFAULT NULL, "
                     "  noExpungeTags               INTEGER      DEFAULT NULL, "
                     "  noSetParentTag              INTEGER      DEFAULT NULL, "
                     "  noCreateSharedNotebooks     INTEGER      DEFAULT NULL, "
                     "  updateWhichSharedNotebookRestrictions    INTEGER     DEFAULT NULL, "
                     "  expungeWhichSharedNotebookRestrictions   INTEGER     DEFAULT NULL "
                     ")");
    DATABASE_CHECK_AND_SET_ERROR("can't create NotebookRestrictions table");

    res = query.exec("CREATE TABLE IF NOT EXISTS SharedNotebooks("
                     "  shareId                             INTEGER PRIMARY KEY   NOT NULL UNIQUE, "
                     "  userId                              INTEGER    DEFAULT NULL, "
                     "  notebookGuid REFERENCES Notebooks(guid) ON DELETE CASCADE ON UPDATE CASCADE, "
                     "  sharedNotebookEmail                 TEXT       DEFAULT NULL, "
                     "  sharedNotebookCreationTimestamp     INTEGER    DEFAULT NULL, "
                     "  sharedNotebookModificationTimestamp INTEGER    DEFAULT NULL, "
                     "  shareKey                            TEXT       DEFAULT NULL, "
                     "  sharedNotebookUsername              TEXT       DEFAULT NULL, "
                     "  sharedNotebookPrivilegeLevel        INTEGER    DEFAULT NULL, "
                     "  allowPreview                        INTEGER    DEFAULT NULL, "
                     "  recipientReminderNotifyEmail        INTEGER    DEFAULT NULL, "
                     "  recipientReminderNotifyInApp        INTEGER    DEFAULT NULL, "
                     "  indexInNotebook                     INTEGER    DEFAULT NULL, "
                     "  UNIQUE(shareId, notebookGuid) ON CONFLICT REPLACE"
                     ")");
    DATABASE_CHECK_AND_SET_ERROR("can't create SharedNotebooks table");

    res = query.exec("CREATE TABLE IF NOT EXISTS Notes("
                     "  localGuid                       TEXT PRIMARY KEY     NOT NULL UNIQUE, "
                     "  guid                            TEXT                 DEFAULT NULL UNIQUE, "
                     "  updateSequenceNumber            INTEGER              DEFAULT NULL, "
                     "  isDirty                         INTEGER              NOT NULL, "
                     "  isLocal                         INTEGER              NOT NULL, "
                     "  hasShortcut                     INTEGER              NOT NULL, "
                     "  title                           TEXT                 DEFAULT NULL, "
                     "  content                         TEXT                 DEFAULT NULL, "
                     "  contentLength                   INTEGER              DEFAULT NULL, "
                     "  contentHash                     TEXT                 DEFAULT NULL, "
                     "  contentPlainText                TEXT                 DEFAULT NULL, "
                     "  contentListOfWords              TEXT                 DEFAULT NULL, "
                     "  contentContainsFinishedToDo     INTEGER              DEFAULT NULL, "
                     "  contentContainsUnfinishedToDo   INTEGER              DEFAULT NULL, "
                     "  contentContainsEncryption       INTEGER              DEFAULT NULL, "
                     "  creationTimestamp               INTEGER              DEFAULT NULL, "
                     "  modificationTimestamp           INTEGER              DEFAULT NULL, "
                     "  deletionTimestamp               INTEGER              DEFAULT NULL, "
                     "  isActive                        INTEGER              DEFAULT NULL, "
                     "  hasAttributes                   INTEGER              NOT NULL, "
                     "  thumbnail                       BLOB                 DEFAULT NULL, "
                     "  notebookLocalGuid REFERENCES Notebooks(localGuid) ON DELETE CASCADE ON UPDATE CASCADE, "
                     "  notebookGuid REFERENCES Notebooks(guid) ON DELETE CASCADE ON UPDATE CASCADE, "
                     "  subjectDate                     INTEGER              DEFAULT NULL, "
                     "  latitude                        REAL                 DEFAULT NULL, "
                     "  longitude                       REAL                 DEFAULT NULL, "
                     "  altitude                        REAL                 DEFAULT NULL, "
                     "  author                          TEXT                 DEFAULT NULL, "
                     "  source                          TEXT                 DEFAULT NULL, "
                     "  sourceURL                       TEXT                 DEFAULT NULL, "
                     "  sourceApplication               TEXT                 DEFAULT NULL, "
                     "  shareDate                       INTEGER              DEFAULT NULL, "
                     "  reminderOrder                   INTEGER              DEFAULT NULL, "
                     "  reminderDoneTime                INTEGER              DEFAULT NULL, "
                     "  reminderTime                    INTEGER              DEFAULT NULL, "
                     "  placeName                       TEXT                 DEFAULT NULL, "
                     "  contentClass                    TEXT                 DEFAULT NULL, "
                     "  lastEditedBy                    TEXT                 DEFAULT NULL, "
                     "  creatorId                       INTEGER              DEFAULT NULL, "
                     "  lastEditorId                    INTEGER              DEFAULT NULL, "
                     "  applicationDataKeysOnly         TEXT                 DEFAULT NULL, "
                     "  applicationDataKeysMap          TEXT                 DEFAULT NULL, "
                     "  applicationDataValues           TEXT                 DEFAULT NULL, "
                     "  classificationKeys              TEXT                 DEFAULT NULL, "
                     "  classificationValues            TEXT                 DEFAULT NULL, "
                     "  UNIQUE(localGuid, guid)"
                     ")");
    DATABASE_CHECK_AND_SET_ERROR("can't create Notes table");

    res = query.exec("CREATE INDEX IF NOT EXISTS NotesNotebooks ON Notes(notebookLocalGuid)");
    DATABASE_CHECK_AND_SET_ERROR("can't create index NotesNotebooks");

    res = query.exec("CREATE VIRTUAL TABLE NoteFTS USING FTS4(content=\"Notes\", localGuid, title, "
                     "contentPlainText, contentContainsFinishedToDo, contentContainsUnfinishedToDo, "
                     "contentContainsEncryption, creationTimestamp, modificationTimestamp, "
                     "isActive, notebookLocalGuid, notebookGuid, subjectDate, latitude, longitude, "
                     "altitude, author, source, sourceApplication, reminderOrder, reminderDoneTime, "
                     "reminderTime, placeName, contentClass, applicationDataKeysOnly, "
                     "applicationDataKeysMap, applicationDataValues)");
    DATABASE_CHECK_AND_SET_ERROR("can't create virtual FTS4 table NoteFTS");

    res = query.exec("CREATE TRIGGER NoteFTS_BeforeDeleteTrigger BEFORE DELETE ON Notes "
                     "BEGIN "
                     "DELETE FROM NoteFTS WHERE localGuid=old.localGuid; "
                     "END");
    DATABASE_CHECK_AND_SET_ERROR("can't create trigger NoteFTS_BeforeDeleteTrigger");

    res = query.exec("CREATE TRIGGER NoteFTS_AfterInsertTrigger AFTER INSERT ON Notes "
                     "BEGIN "
                     "INSERT INTO NoteFTS(NoteFTS) VALUES('rebuild'); "
                     "END");
    DATABASE_CHECK_AND_SET_ERROR("can't create trigger NoteFTS_AfterInsertTrigger");

    res = query.exec("CREATE TABLE IF NOT EXISTS Resources("
                     "  resourceLocalGuid               TEXT PRIMARY KEY     NOT NULL UNIQUE, "
                     "  resourceGuid                    TEXT                 DEFAULT NULL UNIQUE, "
                     "  noteLocalGuid REFERENCES Notes(localGuid) ON DELETE CASCADE ON UPDATE CASCADE, "
                     "  noteGuid REFERENCES Notes(guid) ON DELETE CASCADE ON UPDATE CASCADE, "
                     "  resourceUpdateSequenceNumber    INTEGER              DEFAULT NULL, "
                     "  resourceIsDirty                 INTEGER              NOT NULL, "
                     "  dataBody                        TEXT                 DEFAULT NULL, "
                     "  dataSize                        INTEGER              DEFAULT NULL, "
                     "  dataHash                        TEXT                 DEFAULT NULL, "
                     "  mime                            TEXT                 DEFAULT NULL, "
                     "  width                           INTEGER              DEFAULT NULL, "
                     "  height                          INTEGER              DEFAULT NULL, "
                     "  recognitionDataBody             TEXT                 DEFAULT NULL, "
                     "  recognitionDataSize             INTEGER              DEFAULT NULL, "
                     "  recognitionDataHash             TEXT                 DEFAULT NULL, "
                     "  alternateDataBody               TEXT                 DEFAULT NULL, "
                     "  alternateDataSize               INTEGER              DEFAULT NULL, "
                     "  alternateDataHash               TEXT                 DEFAULT NULL, "
                     "  resourceIndexInNote             INTEGER              DEFAULT NULL, "
                     "  UNIQUE(resourceLocalGuid, resourceGuid)"
                     ")");
    DATABASE_CHECK_AND_SET_ERROR("can't create Resources table");

    res = query.exec("CREATE INDEX IF NOT EXISTS ResourceMimeIndex ON Resources(mime)");
    DATABASE_CHECK_AND_SET_ERROR("can't create ResourceMimeIndex index");

    res = query.exec("CREATE TABLE IF NOT EXISTS ResourceRecognitionTypes("
                     "  resourceLocalGuid REFERENCES Resources(resourceLocalGuid) ON DELETE CASCADE ON UPDATE CASCADE, "
                     "  recognitionType                 TEXT                DEFAULT NULL)");
    DATABASE_CHECK_AND_SET_ERROR("can't create ResourceRecognitionTypes table");

    res = query.exec("CREATE INDEX IF NOT EXISTS ResourceRecognitionTypeIndex ON ResourceRecognitionTypes(recognitionType)");
    DATABASE_CHECK_AND_SET_ERROR("can't create ResourceRecognitionTypeIndex index");

    res = query.exec("CREATE VIRTUAL TABLE ResourceRecoTypesFTS USING FTS4(content=\"ResourceRecognitionTypes\", resourceLocalGuid, recognitionType)");
    DATABASE_CHECK_AND_SET_ERROR("can't create virtual FTS4 ResourceRecoTypesFTS table");

    res = query.exec("CREATE TRIGGER ResourceRecoTypesFTS_BeforeDeleteTrigger BEFORE DELETE ON ResourceRecognitionTypes "
                     "BEGIN "
                     "DELETE FROM ResourceRecoTypesFTS WHERE recognitionType=old.recognitionType; "
                     "END");
    DATABASE_CHECK_AND_SET_ERROR("can't create trigger ResourceRecoTypesFTS_BeforeDeleteTrigger");

    res = query.exec("CREATE TRIGGER ResourceRecoTypesFTS_AfterInsertTrigger AFTER INSERT ON ResourceRecognitionTypes "
                     "BEGIN "
                     "INSERT INTO ResourceRecoTypesFTS(ResourceRecoTypesFTS) VALUES('rebuild'); "
                     "END");
    DATABASE_CHECK_AND_SET_ERROR("can't create trigger ResourceRecoTypesFTS_AfterInsertTrigger");

    res = query.exec("CREATE VIRTUAL TABLE ResourceMimeFTS USING FTS4(content=\"Resources\", resourceLocalGuid, mime)");
    DATABASE_CHECK_AND_SET_ERROR("can't create virtual FTS4 ResourceMimeFTS table");

    res = query.exec("CREATE TRIGGER ResourceMimeFTS_BeforeDeleteTrigger BEFORE DELETE ON Resources "
                     "BEGIN "
                     "DELETE FROM ResourceMimeFTS WHERE mime=old.mime; "
                     "END");
    DATABASE_CHECK_AND_SET_ERROR("can't create trigger ResourceMimeFTS_BeforeDeleteTrigger");

    res = query.exec("CREATE TRIGGER ResourceMimeFTS_AfterInsertTrigger AFTER INSERT ON Resources "
                     "BEGIN "
                     "INSERT INTO ResourceMimeFTS(ResourceMimeFTS) VALUES('rebuild'); "
                     "END");
    DATABASE_CHECK_AND_SET_ERROR("can't create trigger ResourceMimeFTS_AfterInsertTrigger");

    res = query.exec("CREATE VIEW IF NOT EXISTS ResourcesWithoutBinaryData "
                     "AS SELECT resourceLocalGuid, resourceGuid, noteLocalGuid, noteGuid, "
                     "resourceUpdateSequenceNumber, resourceIsDirty, dataSize, dataHash, "
                     "mime, width, height, recognitionDataSize, recognitionDataHash, "
                     "alternateDataSize, alternateDataHash, resourceIndexInNote FROM Resources");
    DATABASE_CHECK_AND_SET_ERROR("can't create ResourcesWithoutBinaryData view");

    res = query.exec("CREATE INDEX IF NOT EXISTS ResourceNote ON Resources(noteLocalGuid)");
    DATABASE_CHECK_AND_SET_ERROR("can't create ResourceNote index");

    res = query.exec("CREATE TABLE IF NOT EXISTS ResourceAttributes("
                     "  resourceLocalGuid REFERENCES Resources(resourceLocalGuid) ON DELETE CASCADE ON UPDATE CASCADE, "
                     "  resourceSourceURL       TEXT                DEFAULT NULL, "
                     "  timestamp               INTEGER             DEFAULT NULL, "
                     "  resourceLatitude        REAL                DEFAULT NULL, "
                     "  resourceLongitude       REAL                DEFAULT NULL, "
                     "  resourceAltitude        REAL                DEFAULT NULL, "
                     "  cameraMake              TEXT                DEFAULT NULL, "
                     "  cameraModel             TEXT                DEFAULT NULL, "
                     "  clientWillIndex         INTEGER             DEFAULT NULL, "
                     "  fileName                TEXT                DEFAULT NULL, "
                     "  attachment              INTEGER             DEFAULT NULL, "
                     "  UNIQUE(resourceLocalGuid) "
                     ")");
    DATABASE_CHECK_AND_SET_ERROR("can't create ResourceAttributes table");

    res = query.exec("CREATE TABLE IF NOT EXISTS ResourceAttributesApplicationDataKeysOnly("
                     "  resourceLocalGuid REFERENCES Resources(resourceLocalGuid) ON DELETE CASCADE ON UPDATE CASCADE, "
                     "  resourceKey             TEXT                DEFAULT NULL, "
                     "  UNIQUE(resourceLocalGuid, resourceKey)"
                     ")");
    DATABASE_CHECK_AND_SET_ERROR("can't create ResourceAttributesApplicationDataKeysOnly table");

    res = query.exec("CREATE TABLE IF NOT EXISTS ResourceAttributesApplicationDataFullMap("
                     "  resourceLocalGuid REFERENCES Resources(resourceLocalGuid) ON DELETE CASCADE ON UPDATE CASCADE, "
                     "  resourceMapKey          TEXT                DEFAULT NULL, "
                     "  resourcevalue           TEXT                DEFAULT NULL, "
                     "  UNIQUE(resourceLocalGuid, resourceMapKey) ON CONFLICT REPLACE"
                     ")");
    DATABASE_CHECK_AND_SET_ERROR("can't create ResourceAttributesApplicationDataFullMap table");

    res = query.exec("CREATE TABLE IF NOT EXISTS Tags("
                     "  localGuid             TEXT PRIMARY KEY     NOT NULL UNIQUE, "
                     "  guid                  TEXT                 DEFAULT NULL UNIQUE, "
                     "  linkedNotebookGuid REFERENCES LinkedNotebooks(guid) ON DELETE CASCADE ON UPDATE CASCADE, "
                     "  updateSequenceNumber  INTEGER              DEFAULT NULL, "
                     "  name                  TEXT                 DEFAULT NULL, "
                     "  nameUpper             TEXT                 DEFAULT NULL, "
                     "  parentGuid            TEXT                 DEFAULT NULL, "
                     "  isDirty               INTEGER              NOT NULL, "
                     "  isLocal               INTEGER              NOT NULL, "
                     "  isDeleted             INTEGER              NOT NULL, "
                     "  hasShortcut           INTEGER              NOT NULL, "
                     "  UNIQUE(linkedNotebookGuid, nameUpper) "
                     ")");
    DATABASE_CHECK_AND_SET_ERROR("can't create Tags table");

    res = query.exec("CREATE INDEX IF NOT EXISTS TagNameUpperIndex ON Tags(nameUpper)");
    DATABASE_CHECK_AND_SET_ERROR("can't create TagNameUpperIndex index");

    res = query.exec("CREATE VIRTUAL TABLE TagFTS USING FTS4(content=\"Tags\", localGuid, guid, nameUpper)");
    DATABASE_CHECK_AND_SET_ERROR("can't create virtual FTS4 table TagFTS");

    res = query.exec("CREATE TRIGGER TagFTS_BeforeDeleteTrigger BEFORE DELETE ON Tags "
                     "BEGIN "
                     "DELETE FROM TagFTS WHERE localGuid=old.localGuid; "
                     "END");
    DATABASE_CHECK_AND_SET_ERROR("can't create trigger TagFTS_BeforeDeleteTrigger");

    res = query.exec("CREATE TRIGGER TagFTS_AfterInsertTrigger AFTER INSERT ON Tags "
                     "BEGIN "
                     "INSERT INTO TagFTS(TagFTS) VALUES('rebuild'); "
                     "END");
    DATABASE_CHECK_AND_SET_ERROR("can't create trigger TagFTS_AfterInsertTrigger");

    res = query.exec("CREATE INDEX IF NOT EXISTS TagsSearchName ON Tags(nameUpper)");
    DATABASE_CHECK_AND_SET_ERROR("can't create TagsSearchName index");

    res = query.exec("CREATE TABLE IF NOT EXISTS NoteTags("
                     "  localNote REFERENCES Notes(localGuid) ON DELETE CASCADE ON UPDATE CASCADE, "
                     "  note REFERENCES Notes(guid) ON DELETE CASCADE ON UPDATE CASCADE, "
                     "  localTag REFERENCES Tags(localGuid) ON DELETE CASCADE ON UPDATE CASCADE, "
                     "  tag  REFERENCES Tags(guid) ON DELETE CASCADE ON UPDATE CASCADE, "
                     "  tagIndexInNote        INTEGER               DEFAULT NULL, "
                     "  UNIQUE(localNote, localTag) ON CONFLICT REPLACE"
                     ")");
    DATABASE_CHECK_AND_SET_ERROR("can't create NoteTags table");

    res = query.exec("CREATE INDEX IF NOT EXISTS NoteTagsNote ON NoteTags(localNote)");
    DATABASE_CHECK_AND_SET_ERROR("can't create NoteTagsNote index");

    res = query.exec("CREATE TABLE IF NOT EXISTS NoteResources("
                     "  localNote     REFERENCES Notes(localGuid)     ON DELETE CASCADE ON UPDATE CASCADE, "
                     "  note          REFERENCES Notes(guid)          ON DELETE CASCADE ON UPDATE CASCADE, "
                     "  localResource REFERENCES Resources(resourceLocalGuid) ON DELETE CASCADE ON UPDATE CASCADE, "
                     "  resource      REFERENCES Resources(resourceGuid)      ON DELETE CASCADE ON UPDATE CASCADE, "
                     "  UNIQUE(localNote, localResource) ON CONFLICT REPLACE)");
    DATABASE_CHECK_AND_SET_ERROR("can't create NoteResources table");

    res = query.exec("CREATE INDEX IF NOT EXISTS NoteResourcesNote ON NoteResources(localNote)");
    DATABASE_CHECK_AND_SET_ERROR("can't create NoteResourcesNote index");

    // NOTE: reasoning for existence and unique constraint for nameUpper, citing Evernote API reference:
    // "The account may only contain one search with a given name (case-insensitive compare)"

    res = query.exec("CREATE TABLE IF NOT EXISTS SavedSearches("
                     "  localGuid                       TEXT PRIMARY KEY    NOT NULL UNIQUE, "
                     "  guid                            TEXT                DEFAULT NULL UNIQUE, "
                     "  name                            TEXT                DEFAULT NULL, "
                     "  nameUpper                       TEXT                DEFAULT NULL UNIQUE, "
                     "  query                           TEXT                DEFAULT NULL, "
                     "  format                          INTEGER             DEFAULT NULL, "
                     "  updateSequenceNumber            INTEGER             DEFAULT NULL, "
                     "  isDirty                         INTEGER             NOT NULL, "
                     "  isLocal                         INTEGER             NOT NULL, "
                     "  includeAccount                  INTEGER             DEFAULT NULL, "
                     "  includePersonalLinkedNotebooks  INTEGER             DEFAULT NULL, "
                     "  includeBusinessLinkedNotebooks  INTEGER             DEFAULT NULL, "
                     "  hasShortcut                     INTEGER             NOT NULL, "
                     "  UNIQUE(localGuid, guid))");
    DATABASE_CHECK_AND_SET_ERROR("can't create SavedSearches table");

    return true;
}

bool LocalStorageManagerPrivate::InsertOrReplaceNotebookRestrictions(const qevercloud::NotebookRestrictions & notebookRestrictions,
                                                                     const QString & localGuid, QString & errorDescription)
{
    errorDescription = QT_TR_NOOP("Can't insert or replace notebook restrictions: ");

    bool res = CheckAndPrepareInsertOrReplaceNotebookRestrictionsQuery();
    QSqlQuery & query = m_insertOrReplaceNotebookRestrictionsQuery;
    DATABASE_CHECK_AND_SET_ERROR("can't insert or replace data into \"NotebookRestrictions\" table in SQL database: "
                                 "can't prepare SQL query");

    QVariant nullValue;

    query.bindValue(":localGuid", localGuid);

#define BIND_RESTRICTION(name) \
    query.bindValue(":" #name, (notebookRestrictions.name.isSet() ? (notebookRestrictions.name.ref() ? 1 : 0) : nullValue))

    BIND_RESTRICTION(noReadNotes);
    BIND_RESTRICTION(noCreateNotes);
    BIND_RESTRICTION(noUpdateNotes);
    BIND_RESTRICTION(noExpungeNotes);
    BIND_RESTRICTION(noShareNotes);
    BIND_RESTRICTION(noEmailNotes);
    BIND_RESTRICTION(noSendMessageToRecipients);
    BIND_RESTRICTION(noUpdateNotebook);
    BIND_RESTRICTION(noExpungeNotebook);
    BIND_RESTRICTION(noSetDefaultNotebook);
    BIND_RESTRICTION(noSetNotebookStack);
    BIND_RESTRICTION(noPublishToPublic);
    BIND_RESTRICTION(noPublishToBusinessLibrary);
    BIND_RESTRICTION(noCreateTags);
    BIND_RESTRICTION(noUpdateTags);
    BIND_RESTRICTION(noExpungeTags);
    BIND_RESTRICTION(noSetParentTag);
    BIND_RESTRICTION(noCreateSharedNotebooks);

#undef BIND_RESTRICTION

    query.bindValue(":updateWhichSharedNotebookRestrictions", notebookRestrictions.updateWhichSharedNotebookRestrictions.isSet()
                    ? notebookRestrictions.updateWhichSharedNotebookRestrictions.ref() : nullValue);
    query.bindValue(":expungeWhichSharedNotebookRestrictions", notebookRestrictions.expungeWhichSharedNotebookRestrictions.isSet()
                    ? notebookRestrictions.expungeWhichSharedNotebookRestrictions.ref() : nullValue);

    res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("can't insert or replace data into \"NotebookRestrictions\" table in SQL database");

    return true;
}

bool LocalStorageManagerPrivate::InsertOrReplaceSharedNotebook(const ISharedNotebook & sharedNotebook,
                                                               QString & errorDescription)
{
    // NOTE: this method is expected to be called after the sanity check of sharedNotebook object!

    bool res = CheckAndPrepareInsertOrReplaceSharedNotebokQuery();
    QSqlQuery & query = m_insertOrReplaceSharedNotebookQuery;
    DATABASE_CHECK_AND_SET_ERROR("can't insert or replace data into \"SharedNotebooks\" table in SQL database: "
                                 "can't prepare SQL query");

    QVariant nullValue;

    query.bindValue(":shareId", sharedNotebook.id());
    query.bindValue(":userId", (sharedNotebook.hasUserId() ? sharedNotebook.userId() : nullValue));
    query.bindValue(":notebookGuid", (sharedNotebook.hasNotebookGuid() ? sharedNotebook.notebookGuid() : nullValue));
    query.bindValue(":sharedNotebookEmail", (sharedNotebook.hasEmail() ? sharedNotebook.email() : nullValue));
    query.bindValue(":sharedNotebookCreationTimestamp", (sharedNotebook.hasCreationTimestamp() ? sharedNotebook.creationTimestamp() : nullValue));
    query.bindValue(":sharedNotebookModificationTimestamp", (sharedNotebook.hasModificationTimestamp() ? sharedNotebook.modificationTimestamp() : nullValue));
    query.bindValue(":shareKey", (sharedNotebook.hasShareKey() ? sharedNotebook.shareKey() : nullValue));
    query.bindValue(":sharedNotebookUsername", (sharedNotebook.hasUsername() ? sharedNotebook.username() : nullValue));
    query.bindValue(":sharedNotebookPrivilegeLevel", (sharedNotebook.hasPrivilegeLevel() ? sharedNotebook.privilegeLevel() : nullValue));
    query.bindValue(":allowPreview", (sharedNotebook.hasAllowPreview() ? (sharedNotebook.allowPreview() ? 1 : 0) : nullValue));
    query.bindValue(":recipientReminderNotifyEmail", (sharedNotebook.hasReminderNotifyEmail() ? (sharedNotebook.reminderNotifyEmail() ? 1 : 0) : nullValue));
    query.bindValue(":recipientReminderNotifyInApp", (sharedNotebook.hasReminderNotifyApp() ? (sharedNotebook.reminderNotifyApp() ? 1 : 0) : nullValue));
    query.bindValue(":indexInNotebook", (sharedNotebook.indexInNotebook() >= 0 ? sharedNotebook.indexInNotebook() : nullValue));

    res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("can't insert or replace data into \"SharedNotebooks\" table in SQL database");

    return true;
}

bool LocalStorageManagerPrivate::RowExists(const QString & tableName, const QString & uniqueKeyName,
                                           const QVariant & uniqueKeyValue) const
{
    QSqlQuery query(m_sqlDatabase);
    const QString & queryString = QString("SELECT count(*) FROM %1 WHERE %2=\'%3\'")
                                  .arg(tableName)
                                  .arg(uniqueKeyName)
                                  .arg(uniqueKeyValue.toString());

    bool res = query.exec(queryString);
    if (!res) {
        QNWARNING("Unable to check the existence of row with key name " << uniqueKeyName
                  << ", value = " << uniqueKeyValue << " in table " << tableName
                  << ": unalble to execute SQL statement: " << query.lastError().text()
                  << "; assuming no such row exists");
        return false;
    }

    while(query.next() && query.isValid()) {
        int count = query.value(0).toInt();
        return (count != 0);
    }

    return false;
}

bool LocalStorageManagerPrivate::InsertOrReplaceUser(const IUser & user, QString & errorDescription)
{
    // NOTE: this method is expected to be called after the check of user object for sanity of its parameters!

    errorDescription += QT_TR_NOOP("Can't insert or replace User into local storage database: ");

    Transaction transaction(m_sqlDatabase, *this, Transaction::Exclusive);

    QString userId = QString::number(user.id());
    QVariant nullValue;

    // Insert or replace common user data
    {
        bool res = CheckAndPrepareInsertOrReplaceUserQuery();
        QSqlQuery & query = m_insertOrReplaceUserQuery;
        DATABASE_CHECK_AND_SET_ERROR("can't insert or replace data into \"Users\" table in SQL database: "
                                     "can't prepare SQL query");

        query.bindValue(":id", userId);
        query.bindValue(":username", (user.hasUsername() ? user.username() : nullValue));
        query.bindValue(":email", (user.hasEmail() ? user.email() : nullValue));
        query.bindValue(":name", (user.hasName() ? user.name() : nullValue));
        query.bindValue(":timezone", (user.hasTimezone() ? user.timezone() : nullValue));
        query.bindValue(":privilege", (user.hasPrivilegeLevel() ? user.privilegeLevel() : nullValue));
        query.bindValue(":userCreationTimestamp", (user.hasCreationTimestamp() ? user.creationTimestamp() : nullValue));
        query.bindValue(":userModificationTimestamp", (user.hasModificationTimestamp() ? user.modificationTimestamp() : nullValue));
        query.bindValue(":userIsDirty", (user.isDirty() ? 1 : 0));
        query.bindValue(":userIsLocal", (user.isLocal() ? 1 : 0));
        query.bindValue(":userIsActive", (user.hasActive() ? (user.active() ? 1 : 0) : nullValue));
        query.bindValue(":userDeletionTimestamp", (user.hasDeletionTimestamp() ? user.deletionTimestamp() : nullValue));

        res = query.exec();
        DATABASE_CHECK_AND_SET_ERROR("can't insert or replace data into \"Users\" table in SQL database");
    }

    if (user.hasUserAttributes())
    {
        bool res = InsertOrReplaceUserAttributes(user.id(), user.userAttributes(), errorDescription);
        if (!res) {
            return false;
        }
    }
    else
    {
        // Clean entries from UserAttributesViewedPromotions table
        {
            bool res = CheckAndPrepareExpungeUserAttributesViewedPromotionsQuery();
            QSqlQuery & query = m_expungeUserAttributesViewedPromotionsQuery;
            DATABASE_CHECK_AND_SET_ERROR("can't clear UserAttributesViewedPromotions when updating user: "
                                         "can't prepare SQL query");

            query.bindValue(":id", userId);

            res = query.exec();
            DATABASE_CHECK_AND_SET_ERROR("can't clear UserAttributesViewedPromotions when updating user");
        }

        // Clean entries from UserAttributesRecentMailedAddresses table
        {
            bool res = CheckAndPrepareExpungeUserAttributesRecentMailedAddressesQuery();
            QSqlQuery & query = m_expungeUserAttributesRecentMailedAddressesQuery;
            DATABASE_CHECK_AND_SET_ERROR("can't clear UserAttributesRecentMailedAddresses when updating user: "
                                         "can't prepare SQL query");

            query.bindValue(":id", userId);

            res = query.exec();
            DATABASE_CHECK_AND_SET_ERROR("can't clear UserAttributesRecentMailedAddresses when updating user");
        }

        // Clean entries from UserAttributes table
        {
            bool res = CheckAndPrepareExpungeUserAttributesQuery();
            QSqlQuery & query = m_expungeUserAttributesQuery;
            DATABASE_CHECK_AND_SET_ERROR("can't clear UserAttributes when updating user: "
                                         "can't prepare SQL query");

            query.bindValue(":id", userId);

            res = query.exec();
            DATABASE_CHECK_AND_SET_ERROR("can't clear UserAttributes when updating user");
        }
    }

    if (user.hasAccounting())
    {
        bool res = InsertOrReplaceAccounting(user.id(), user.accounting(), errorDescription);
        if (!res) {
            return false;
        }
    }
    else
    {
        bool res = CheckAndPrepareExpungeAccountingQuery();
        QSqlQuery & query = m_expungeAccountingQuery;
        DATABASE_CHECK_AND_SET_ERROR("can't clear Accounting when updating user: "
                                     "can't prepare SQL query");

        query.bindValue(":id", userId);

        res = query.exec();
        DATABASE_CHECK_AND_SET_ERROR("can't clear Accounting when updating user");
    }

    if (user.hasPremiumInfo())
    {
        bool res = InsertOrReplacePremiumInfo(user.id(), user.premiumInfo(), errorDescription);
        if (!res) {
            return false;
        }
    }
    else
    {
        bool res = CheckAndPrepareExpungePremiumUserInfoQuery();
        QSqlQuery & query = m_expungePremiumUserInfoQuery;
        DATABASE_CHECK_AND_SET_ERROR("can't clear PremiumInfo when updating user: "
                                     "can't prepare SQL query");

        query.bindValue(":id", userId);

        res = query.exec();
        DATABASE_CHECK_AND_SET_ERROR("can't clear PremiumInfo when updating user");
    }

    if (user.hasBusinessUserInfo())
    {
        bool res = InsertOrReplaceBusinesUserInfo(user.id(), user.businessUserInfo(), errorDescription);
        if (!res) {
            return false;
        }
    }
    else
    {
        bool res = CheckAndPrepareExpungeBusinessUserInfoQuery();
        QSqlQuery & query = m_expungeBusinessUserInfoQuery;
        DATABASE_CHECK_AND_SET_ERROR("can't clear BusinesUserInfo when updating user: "
                                     "can't prepare SQL query");

        query.bindValue(":id", userId);

        res = query.exec();
        DATABASE_CHECK_AND_SET_ERROR("can't clear BusinesUserInfo when updating user");
    }

    return transaction.commit(errorDescription);
}

bool LocalStorageManagerPrivate::InsertOrReplaceBusinesUserInfo(const UserID id, const qevercloud::BusinessUserInfo & info,
                                                                QString & errorDescription)
{
    bool res = CheckAndPrepareInsertOrReplaceBusinessUserInfoQuery();
    QSqlQuery & query = m_insertOrReplaceBusinessUserInfoQuery;
    DATABASE_CHECK_AND_SET_ERROR("can't set user's business info into \"BusinessUserInfo\" table in SQL database: "
                                 "can't prepare SQL query");

    QVariant nullValue;

    query.bindValue(":id", id);
    query.bindValue(":businessId", (info.businessId.isSet() ? info.businessId.ref() : nullValue));
    query.bindValue(":businessName", (info.businessName.isSet() ? info.businessName.ref() : nullValue));
    query.bindValue(":role", (info.role.isSet() ? info.role.ref() : nullValue));
    query.bindValue(":businessInfoEmail", (info.email.isSet() ? info.email.ref() : nullValue));

    res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("can't set user's business info into \"BusinessUserInfo\" table in SQL database");

    return true;
}

bool LocalStorageManagerPrivate::InsertOrReplacePremiumInfo(const UserID id, const qevercloud::PremiumInfo & info,
                                                            QString & errorDescription)
{
    bool res = CheckAndPrepareInsertOrReplacePremiumUserInfoQuery();
    QSqlQuery & query = m_insertOrReplacePremiumUserInfoQuery;
    DATABASE_CHECK_AND_SET_ERROR("can't set user's premium info into \"PremiumInfo\" table in SQL database: "
                                 "can't prepare SQL query");

    query.bindValue(":id", id);
    query.bindValue(":currentTime", info.currentTime);
    query.bindValue(":premium", (info.premium ? 1 : 0));
    query.bindValue(":premiumRecurring", (info.premiumRecurring ? 1 : 0));
    query.bindValue(":premiumExtendable", (info.premiumExtendable ? 1 : 0));
    query.bindValue(":premiumPending", (info.premiumPending ? 1 : 0));
    query.bindValue(":premiumCancellationPending", (info.premiumCancellationPending ? 1 : 0));
    query.bindValue(":canPurchaseUploadAllowance", (info.canPurchaseUploadAllowance ? 1 : 0));

    QVariant nullValue;

    query.bindValue(":premiumExpirationDate", (info.premiumExpirationDate.isSet() ? info.premiumExpirationDate.ref() : nullValue));
    query.bindValue(":sponsoredGroupName", (info.sponsoredGroupName.isSet() ? info.sponsoredGroupName.ref() : nullValue));
    query.bindValue(":sponsoredGroupRole", (info.sponsoredGroupRole.isSet() ? info.sponsoredGroupRole.ref() : nullValue));
    query.bindValue(":premiumUpgradable", (info.premiumUpgradable.isSet() ? (info.premiumUpgradable.ref() ? 1 : 0) : nullValue));

    res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("can't set user's premium info into \"PremiumInfo\" table in SQL database");

    return true;
}

bool LocalStorageManagerPrivate::InsertOrReplaceAccounting(const UserID id, const qevercloud::Accounting & accounting,
                                                           QString & errorDescription)
{
    bool res = CheckAndPrepareInsertOrReplaceAccountingQuery();
    QSqlQuery & query = m_insertOrReplaceAccountingQuery;
    DATABASE_CHECK_AND_SET_ERROR("can't set user's acconting into \"Accounting\" table in SQL database: "
                                 "can't prepare SQL query");

    query.bindValue(":id", id);

    QVariant nullValue;

#define CHECK_AND_BIND_VALUE(name) \
    query.bindValue(":" #name, accounting.name.isSet() ? accounting.name.ref() : nullValue)

    CHECK_AND_BIND_VALUE(uploadLimit);
    CHECK_AND_BIND_VALUE(uploadLimitEnd);
    CHECK_AND_BIND_VALUE(uploadLimitNextMonth);
    CHECK_AND_BIND_VALUE(premiumServiceStatus);
    CHECK_AND_BIND_VALUE(premiumOrderNumber);
    CHECK_AND_BIND_VALUE(premiumCommerceService);
    CHECK_AND_BIND_VALUE(premiumServiceStart);
    CHECK_AND_BIND_VALUE(premiumServiceSKU);
    CHECK_AND_BIND_VALUE(lastSuccessfulCharge);
    CHECK_AND_BIND_VALUE(lastFailedCharge);
    CHECK_AND_BIND_VALUE(lastFailedChargeReason);
    CHECK_AND_BIND_VALUE(nextPaymentDue);
    CHECK_AND_BIND_VALUE(premiumLockUntil);
    CHECK_AND_BIND_VALUE(updated);
    CHECK_AND_BIND_VALUE(premiumSubscriptionNumber);
    CHECK_AND_BIND_VALUE(lastRequestedCharge);
    CHECK_AND_BIND_VALUE(currency);
    CHECK_AND_BIND_VALUE(unitPrice);
    CHECK_AND_BIND_VALUE(unitDiscount);
    CHECK_AND_BIND_VALUE(nextChargeDate);

#undef CHECK_AND_BIND_VALUE

#define CHECK_AND_BIND_VALUE(name, columnName) \
    query.bindValue(":" #columnName, accounting.name.isSet() ? accounting.name.ref() : nullValue)

    CHECK_AND_BIND_VALUE(businessId, accountingBusinessId);
    CHECK_AND_BIND_VALUE(businessName, accountingBusinessName);
    CHECK_AND_BIND_VALUE(businessRole, accountingBusinessRole);

#undef CHECK_AND_BIND_VALUE

    res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("can't set user's acconting into \"Accounting\" table in SQL database");

    return true;
}

bool LocalStorageManagerPrivate::InsertOrReplaceUserAttributes(const UserID id, const qevercloud::UserAttributes & attributes,
                                                               QString & errorDescription)
{
    QVariant nullValue;

    // Insert or replace common user attributes data
    {
        bool res = CheckAndPrepareInsertOrReplaceUserAttributesQuery();
        QSqlQuery & query = m_insertOrReplaceUserAttributesQuery;
        DATABASE_CHECK_AND_SET_ERROR("can't set user's attributes into \"UserAttributes\" table in SQL database: "
                                     "can't prepare SQL query");

        query.bindValue(":id", id);

#define CHECK_AND_BIND_VALUE(name) \
    query.bindValue(":" #name, (attributes.name.isSet() ? attributes.name.ref() : nullValue))

        CHECK_AND_BIND_VALUE(defaultLocationName);
        CHECK_AND_BIND_VALUE(defaultLatitude);
        CHECK_AND_BIND_VALUE(defaultLongitude);
        CHECK_AND_BIND_VALUE(incomingEmailAddress);
        CHECK_AND_BIND_VALUE(comments);
        CHECK_AND_BIND_VALUE(dateAgreedToTermsOfService);
        CHECK_AND_BIND_VALUE(maxReferrals);
        CHECK_AND_BIND_VALUE(referralCount);
        CHECK_AND_BIND_VALUE(refererCode);
        CHECK_AND_BIND_VALUE(sentEmailDate);
        CHECK_AND_BIND_VALUE(sentEmailCount);
        CHECK_AND_BIND_VALUE(dailyEmailLimit);
        CHECK_AND_BIND_VALUE(emailOptOutDate);
        CHECK_AND_BIND_VALUE(partnerEmailOptInDate);
        CHECK_AND_BIND_VALUE(preferredLanguage);
        CHECK_AND_BIND_VALUE(preferredCountry);
        CHECK_AND_BIND_VALUE(twitterUserName);
        CHECK_AND_BIND_VALUE(twitterId);
        CHECK_AND_BIND_VALUE(groupName);
        CHECK_AND_BIND_VALUE(recognitionLanguage);
        CHECK_AND_BIND_VALUE(referralProof);
        CHECK_AND_BIND_VALUE(businessAddress);
        CHECK_AND_BIND_VALUE(reminderEmailConfig);

#undef CHECK_AND_BIND_VALUE

#define CHECK_AND_BIND_BOOLEAN_VALUE(name) \
    query.bindValue(":" #name, (attributes.name.isSet() ? (attributes.name.ref() ? 1 : 0) : nullValue))

        CHECK_AND_BIND_BOOLEAN_VALUE(preactivation);
        CHECK_AND_BIND_BOOLEAN_VALUE(clipFullPage);
        CHECK_AND_BIND_BOOLEAN_VALUE(educationalDiscount);
        CHECK_AND_BIND_BOOLEAN_VALUE(hideSponsorBilling);
        CHECK_AND_BIND_BOOLEAN_VALUE(taxExempt);
        CHECK_AND_BIND_BOOLEAN_VALUE(useEmailAutoFiling);

#undef CHECK_AND_BIND_BOOLEAN_VALUE

        res = query.exec();
        DATABASE_CHECK_AND_SET_ERROR("can't set user's attributes into \"UserAttributes\" table in SQL database");
    }

    // Clean viewed promotions first, then re-insert
    {
        bool res = CheckAndPrepareExpungeUserAttributesViewedPromotionsQuery();
        QSqlQuery & query = m_expungeUserAttributesViewedPromotionsQuery;
        DATABASE_CHECK_AND_SET_ERROR("can't set user's attributes into \"UserAttributesViewedPromotions\" "
                                     "table in SQL database: can't prepare SQL query");

        query.bindValue(":id", id);

        res = query.exec();
        DATABASE_CHECK_AND_SET_ERROR("can't set user's attributes into \"UserAttributesViewedPromotions\" table in SQL database");
    }

    if (attributes.viewedPromotions.isSet())
    {
        bool res = CheckAndPrepareInsertOrReplaceUserAttributesViewedPromotionsQuery();
        QSqlQuery & query = m_insertOrReplaceUserAttributesViewedPromotionsQuery;
        DATABASE_CHECK_AND_SET_ERROR("can't set user's attributes into \"UserAttributesViewedPromotions\" "
                                     "table in SQL database: can't prepare SQL query");

        query.bindValue(":id", id);

        foreach(const QString & promotion, attributes.viewedPromotions.ref()) {
            query.bindValue(":promotion", promotion);
            res = query.exec();
            DATABASE_CHECK_AND_SET_ERROR("can't set user's attributes into \"UserAttributesViewedPromotions\" table in SQL database");
        }
    }

    // Clean recent mailed addresses first, then re-insert
    {
        bool res = CheckAndPrepareExpungeUserAttributesRecentMailedAddressesQuery();
        QSqlQuery & query = m_expungeUserAttributesRecentMailedAddressesQuery;
        DATABASE_CHECK_AND_SET_ERROR("can't set user's attributes into \"UserAttributesRecentMailedAddresses\" table in SQL database: "
                                     "can't prepare SQL query");

        query.bindValue(":id", id);

        res = query.exec();
        DATABASE_CHECK_AND_SET_ERROR("can't set user's attributes into \"UserAttributesRecentMailedAddresses\" table in SQL database");
    }

    if (attributes.recentMailedAddresses.isSet())
    {
        bool res = CheckAndPrepareInsertOrReplaceUserAttributesRecentMailedAddressesQuery();
        QSqlQuery & query = m_insertOrReplaceUserAttributesRecentMailedAddressesQuery;
        DATABASE_CHECK_AND_SET_ERROR("can't set user's attributes into \"UserAttributesRecentMailedAddresses\" table in SQL database: "
                                     "can't prepare SQL query");

        query.bindValue(":id", id);

        foreach(const QString & address, attributes.recentMailedAddresses.ref()) {
            query.bindValue(":address", address);
            res = query.exec();
            DATABASE_CHECK_AND_SET_ERROR("can't set user's attributes into \"UserAttributesRecentMailedAddresses\" table in SQL database");
        }
    }

    return true;
}

bool LocalStorageManagerPrivate::CheckAndPrepareGetUserCountQuery() const
{
    if (!m_getUserCountQueryPrepared)
    {
        QNDEBUG("Preparing SQL query to get the count of users");

        m_getUserCountQuery = QSqlQuery(m_sqlDatabase);
        bool res = m_getUserCountQuery.prepare("SELECT COUNT(*) FROM Users WHERE userDeletionTimestamp IS NULL");
        if (res) {
            m_getUserCountQueryPrepared = true;
        }

        return res;
    }
    else
    {
        return true;
    }
}

bool LocalStorageManagerPrivate::CheckAndPrepareInsertOrReplaceUserQuery()
{
    if (!m_insertOrReplaceUserQueryPrepared)
    {
        QNDEBUG("Preparing SQL query to insert or replace user");

        m_insertOrReplaceUserQuery = QSqlQuery(m_sqlDatabase);
        bool res = m_insertOrReplaceUserQuery.prepare("INSERT OR REPLACE INTO Users"
                                                      "(id, username, email, name, timezone, "
                                                      "privilege, userCreationTimestamp, "
                                                      "userModificationTimestamp, userIsDirty, "
                                                      "userIsLocal, userIsActive, userDeletionTimestamp)"
                                                      "VALUES(:id, :username, :email, :name, :timezone, "
                                                      ":privilege, :userCreationTimestamp, "
                                                      ":userModificationTimestamp, :userIsDirty, "
                                                      ":userIsLocal, :userIsActive, :userDeletionTimestamp)");
        if (res) {
            m_insertOrReplaceUserQueryPrepared = true;
        }

        return res;
    }
    else
    {
        return true;
    }
}

bool LocalStorageManagerPrivate::CheckAndPrepareExpungeAccountingQuery()
{
    if (!m_expungeAccountingQueryPrepared)
    {
        QNDEBUG("Preparing SQL query to expunge accounting");

        m_expungeAccountingQuery = QSqlQuery(m_sqlDatabase);
        bool res = m_expungeAccountingQuery.prepare("DELETE FROM Accounting WHERE id = :id");
        if (res) {
            m_expungeAccountingQueryPrepared = true;
        }

        return res;
    }
    else
    {
        return true;
    }
}

bool LocalStorageManagerPrivate::CheckAndPrepareInsertOrReplaceAccountingQuery()
{
    if (!m_insertOrReplaceAccountingQueryPrepared)
    {
        QNDEBUG("Preparing SQL query to insert or replace accounting");

        m_insertOrReplaceAccountingQuery = QSqlQuery(m_sqlDatabase);
        bool res = m_insertOrReplaceAccountingQuery.prepare("INSERT OR REPLACE INTO Accounting"
                                                            "(id, uploadLimit, uploadLimitEnd, uploadLimitNextMonth, "
                                                            "premiumServiceStatus, premiumOrderNumber, premiumCommerceService, "
                                                            "premiumServiceStart, premiumServiceSKU, lastSuccessfulCharge, "
                                                            "lastFailedCharge, lastFailedChargeReason, nextPaymentDue, premiumLockUntil, "
                                                            "updated, premiumSubscriptionNumber, lastRequestedCharge, currency, "
                                                            "unitPrice, unitDiscount, nextChargeDate, accountingBusinessId, "
                                                            "accountingBusinessName, accountingBusinessRole) "
                                                            "VALUES(:id, :uploadLimit, :uploadLimitEnd, :uploadLimitNextMonth, "
                                                            ":premiumServiceStatus, :premiumOrderNumber, :premiumCommerceService, "
                                                            ":premiumServiceStart, :premiumServiceSKU, :lastSuccessfulCharge, "
                                                            ":lastFailedCharge, :lastFailedChargeReason, :nextPaymentDue, :premiumLockUntil, "
                                                            ":updated, :premiumSubscriptionNumber, :lastRequestedCharge, :currency, "
                                                            ":unitPrice, :unitDiscount, :nextChargeDate, :accountingBusinessId, "
                                                            ":accountingBusinessName, :accountingBusinessRole)");
        if (res) {
            m_insertOrReplaceAccountingQueryPrepared = true;
        }

        return res;
    }
    else
    {
        return true;
    }
}

bool LocalStorageManagerPrivate::CheckAndPrepareExpungePremiumUserInfoQuery()
{
    if (!m_expungePremiumUserInfoQueryPrepared)
    {
        QNDEBUG("Preparing SQL query to expunge premium user info");

        m_expungePremiumUserInfoQuery = QSqlQuery(m_sqlDatabase);
        bool res = m_expungePremiumUserInfoQuery.prepare("DELETE FROM PremiumInfo WHERE id = :id");
        if (res) {
            m_expungePremiumUserInfoQueryPrepared = true;
        }

        return res;
    }
    else
    {
        return true;
    }
}

bool LocalStorageManagerPrivate::CheckAndPrepareInsertOrReplacePremiumUserInfoQuery()
{
    if (!m_insertOrReplacePremiumUserInfoQueryPrepared)
    {
        QNDEBUG("Preparing SQL query to insert or replace premium user info");

        m_insertOrReplacePremiumUserInfoQuery = QSqlQuery(m_sqlDatabase);
        bool res = m_insertOrReplacePremiumUserInfoQuery.prepare("INSERT OR REPLACE INTO PremiumInfo"
                                                                 "(id, currentTime, premium, premiumRecurring, premiumExtendable, "
                                                                 "premiumPending, premiumCancellationPending, canPurchaseUploadAllowance, "
                                                                 "premiumExpirationDate, sponsoredGroupName, sponsoredGroupRole, premiumUpgradable) "
                                                                 "VALUES(:id, :currentTime, :premium, :premiumRecurring, :premiumExtendable, "
                                                                 ":premiumPending, :premiumCancellationPending, :canPurchaseUploadAllowance, "
                                                                 ":premiumExpirationDate, :sponsoredGroupName, :sponsoredGroupRole, :premiumUpgradable)");
        if (res) {
            m_insertOrReplacePremiumUserInfoQueryPrepared = true;
        }

        return res;
    }
    else
    {
        return true;
    }
}

bool LocalStorageManagerPrivate::CheckAndPrepareExpungeBusinessUserInfoQuery()
{
    if (!m_expungeBusinessUserInfoQueryPrepared)
    {
        QNDEBUG("Preparing SQL query to expunge business user info");

        m_expungeBusinessUserInfoQuery = QSqlQuery(m_sqlDatabase);
        bool res = m_expungeBusinessUserInfoQuery.prepare("DELETE FROM BusinessUserInfo WHERE id = :id");
        if (res) {
            m_expungeBusinessUserInfoQueryPrepared = true;
        }

        return res;
    }
    else
    {
        return true;
    }
}

bool LocalStorageManagerPrivate::CheckAndPrepareInsertOrReplaceBusinessUserInfoQuery()
{
    if (!m_insertOrReplaceBusinessUserInfoQueryPrepared)
    {
        QNDEBUG("Preparing SQl query to insert or replace business user info");

        m_insertOrReplaceBusinessUserInfoQuery = QSqlQuery(m_sqlDatabase);
        bool res = m_insertOrReplaceBusinessUserInfoQuery.prepare("INSERT OR REPLACE INTO BusinessUserInfo"
                                                                  "(id, businessId, businessName, role, businessInfoEmail) "
                                                                  "VALUES(:id, :businessId, :businessName, :role, :businessInfoEmail)");
        if (res) {
            m_insertOrReplaceBusinessUserInfoQueryPrepared = true;
        }

        return res;
    }
    else
    {
        return true;
    }
}

bool LocalStorageManagerPrivate::CheckAndPrepareExpungeUserAttributesQuery()
{
    if (!m_expungeUserAttributesQueryPrepared)
    {
        QNDEBUG("Preparing SQL query to expunge user attributes");

        m_expungeUserAttributesQuery = QSqlQuery(m_sqlDatabase);
        bool res = m_expungeUserAttributesQuery.prepare("DELETE FROM UserAttributes WHERE id = :id");
        if (res) {
            m_expungeUserAttributesQueryPrepared = true;
        }

        return res;
    }
    else
    {
        return true;
    }
}

bool LocalStorageManagerPrivate::CheckAndPrepareInsertOrReplaceUserAttributesQuery()
{
    if (!m_insertOrReplaceUserAttributesQueryPrepared)
    {
        QNDEBUG("Preparing SQL query to insert or replace user attributes");

        m_insertOrReplaceUserAttributesQuery = QSqlQuery(m_sqlDatabase);
        bool res = m_insertOrReplaceUserAttributesQuery.prepare("INSERT OR REPLACE INTO UserAttributes"
                                                                "(id, defaultLocationName, defaultLatitude, "
                                                                "defaultLongitude, preactivation, "
                                                                "incomingEmailAddress, comments, "
                                                                "dateAgreedToTermsOfService, maxReferrals, "
                                                                "referralCount, refererCode, sentEmailDate, "
                                                                "sentEmailCount, dailyEmailLimit, "
                                                                "emailOptOutDate, partnerEmailOptInDate, "
                                                                "preferredLanguage, preferredCountry, "
                                                                "clipFullPage, twitterUserName, twitterId, "
                                                                "groupName, recognitionLanguage, "
                                                                "referralProof, educationalDiscount, "
                                                                "businessAddress, hideSponsorBilling, "
                                                                "taxExempt, useEmailAutoFiling, reminderEmailConfig) "
                                                                "VALUES(:id, :defaultLocationName, :defaultLatitude, "
                                                                ":defaultLongitude, :preactivation, "
                                                                ":incomingEmailAddress, :comments, "
                                                                ":dateAgreedToTermsOfService, :maxReferrals, "
                                                                ":referralCount, :refererCode, :sentEmailDate, "
                                                                ":sentEmailCount, :dailyEmailLimit, "
                                                                ":emailOptOutDate, :partnerEmailOptInDate, "
                                                                ":preferredLanguage, :preferredCountry, "
                                                                ":clipFullPage, :twitterUserName, :twitterId, "
                                                                ":groupName, :recognitionLanguage, "
                                                                ":referralProof, :educationalDiscount, "
                                                                ":businessAddress, :hideSponsorBilling, "
                                                                ":taxExempt, :useEmailAutoFiling, :reminderEmailConfig)");
        if (res) {
            m_insertOrReplaceUserAttributesQueryPrepared = true;
        }

        return res;
    }
    else
    {
        return true;
    }
}

bool LocalStorageManagerPrivate::CheckAndPrepareExpungeUserAttributesViewedPromotionsQuery()
{
    if (!m_expungeUserAttributesViewedPromotionsQueryPrepared)
    {
        QNDEBUG("Preparing SQL query to expunge user attributes viewed promotions");

        m_expungeUserAttributesViewedPromotionsQuery = QSqlQuery(m_sqlDatabase);
        bool res = m_expungeUserAttributesViewedPromotionsQuery.prepare("DELETE FROM UserAttributesViewedPromotions WHERE id = :id");
        if (res) {
            m_expungeUserAttributesViewedPromotionsQueryPrepared = true;
        }

        return res;
    }
    else
    {
        return true;
    }
}

bool LocalStorageManagerPrivate::CheckAndPrepareInsertOrReplaceUserAttributesViewedPromotionsQuery()
{
    if (!m_insertOrReplaceUserAttributesViewedPromotionsQueryPrepared)
    {
        QNDEBUG("Preparing SQL query to insert or replace user attributes viewed promotions");

        m_insertOrReplaceUserAttributesViewedPromotionsQuery = QSqlQuery(m_sqlDatabase);
        bool res = m_insertOrReplaceUserAttributesViewedPromotionsQuery.prepare("INSERT OR REPLACE INTO UserAttributesViewedPromotions"
                                                                                "(id, promotion) VALUES(:id, :promotion)");
        if (res) {
            m_insertOrReplaceUserAttributesViewedPromotionsQueryPrepared = true;
        }

        return res;
    }
    else
    {
        return true;
    }
}

bool LocalStorageManagerPrivate::CheckAndPrepareExpungeUserAttributesRecentMailedAddressesQuery()
{
    if (!m_expungeUserAttributesRecentMailedAddressesQueryPrepared)
    {
        QNDEBUG("Preparing SQL query to expunge user attributes recent mailed addresses");

        m_expungeUserAttributesRecentMailedAddressesQuery = QSqlQuery(m_sqlDatabase);
        bool res = m_expungeUserAttributesRecentMailedAddressesQuery.prepare("DELETE FROM UserAttributesRecentMailedAddresses WHERE id = :id");
        if (res) {
            m_expungeUserAttributesRecentMailedAddressesQueryPrepared = true;
        }

        return res;
    }
    else
    {
        return true;
    }
}

bool LocalStorageManagerPrivate::CheckAndPrepareInsertOrReplaceUserAttributesRecentMailedAddressesQuery()
{
    if (!m_insertOrReplaceUserAttributesRecentMailedAddressesQueryPrepared)
    {
        QNDEBUG("Preparing SQL query to insert or replace user attributes recent mailed addresses");

        m_insertOrReplaceUserAttributesRecentMailedAddressesQuery = QSqlQuery(m_sqlDatabase);
        bool res = m_insertOrReplaceUserAttributesRecentMailedAddressesQuery.prepare("INSERT OR REPLACE INTO UserAttributesRecentMailedAddresses"
                                                                                     "(id, address) VALUES(:id, :address)");
        if (res) {
            m_insertOrReplaceUserAttributesRecentMailedAddressesQueryPrepared = true;
        }

        return res;
    }
    else
    {
        return true;
    }
}

bool LocalStorageManagerPrivate::CheckAndPrepareDeleteUserQuery()
{
    if (!m_deleteUserQueryPrepared)
    {
        QNDEBUG("Preparing SQL query to mark user deleted");

        m_deleteUserQuery = QSqlQuery(m_sqlDatabase);
        bool res = m_deleteUserQuery.prepare("UPDATE Users SET userDeletionTimestamp = :userDeletionTimestamp, "
                                             "userIsLocal = :userIsLocal WHERE id = :id");
        if (res) {
            m_deleteUserQueryPrepared = true;
        }

        return res;
    }
    else
    {
        return true;
    }
}

bool LocalStorageManagerPrivate::CheckAndPrepareExpungeUserQuery()
{
    if (!m_expungeUserQueryPrepared)
    {
        QNDEBUG("Preparing SQL query to expunge user");

        m_expungeUserQuery = QSqlQuery(m_sqlDatabase);
        bool res = m_expungeUserQuery.prepare("DELETE FROM Users WHERE id = :id");
        if (res) {
            m_expungeUserQueryPrepared = true;
        }

        return res;
    }
    else
    {
        return true;
    }
}

bool LocalStorageManagerPrivate::InsertOrReplaceNotebook(const Notebook & notebook,
                                                         const QString & overrideLocalGuid,
                                                         QString & errorDescription)
{
    // NOTE: this method expects to be called after notebook is already checked
    // for sanity of its parameters!

    errorDescription = QT_TR_NOOP("Can't insert or replace notebook into local storage database: ");

    Transaction transaction(m_sqlDatabase, *this, Transaction::Exclusive);

    QString localGuid = (overrideLocalGuid.isEmpty() ? notebook.localGuid() : overrideLocalGuid);
    QVariant nullValue;

    // Insert or replace common Notebook data
    {
        bool res = CheckAndPrepareInsertOrReplaceNotebookQuery();
        QSqlQuery & query = m_insertOrReplaceNotebookQuery;
        DATABASE_CHECK_AND_SET_ERROR("can't insert or replace data into \"Notebooks\" table of SQL database: "
                                     "can't prepare SQL query");

        query.bindValue(":localGuid", localGuid);
        query.bindValue(":guid", (notebook.hasGuid() ? notebook.guid() : nullValue));
        query.bindValue(":linkedNotebookGuid", (notebook.hasLinkedNotebookGuid() ? notebook.linkedNotebookGuid() : nullValue));
        query.bindValue(":updateSequenceNumber", (notebook.hasUpdateSequenceNumber() ? notebook.updateSequenceNumber() : nullValue));
        query.bindValue(":notebookName", (notebook.hasName() ? notebook.name() : nullValue));
        query.bindValue(":notebookNameUpper", (notebook.hasName() ? notebook.name().toUpper() : nullValue));
        query.bindValue(":creationTimestamp", (notebook.hasCreationTimestamp() ? notebook.creationTimestamp() : nullValue));
        query.bindValue(":modificationTimestamp", (notebook.hasModificationTimestamp() ? notebook.modificationTimestamp() : nullValue));
        query.bindValue(":isDirty", (notebook.isDirty() ? 1 : 0));
        query.bindValue(":isLocal", (notebook.isLocal() ? 1 : 0));
        query.bindValue(":isDefault", (notebook.isDefaultNotebook() ? 1 : nullValue));
        query.bindValue(":isLastUsed", (notebook.isLastUsed() ? 1 : nullValue));
        query.bindValue(":hasShortcut", (notebook.hasShortcut() ? 1 : 0));
        query.bindValue(":publishingUri", (notebook.hasPublishingUri() ? notebook.publishingUri() : nullValue));
        query.bindValue(":publishingNoteSortOrder", (notebook.hasPublishingOrder() ? notebook.publishingOrder() : nullValue));
        query.bindValue(":publishingAscendingSort", (notebook.hasPublishingAscending() ? (notebook.isPublishingAscending() ? 1 : 0) : nullValue));
        query.bindValue(":publicDescription", (notebook.hasPublishingPublicDescription() ? notebook.publishingPublicDescription() : nullValue));
        query.bindValue(":isPublished", (notebook.hasPublished() ? (notebook.isPublished() ? 1 : 0) : nullValue));
        query.bindValue(":stack", (notebook.hasStack() ? notebook.stack() : nullValue));
        query.bindValue(":businessNotebookDescription", (notebook.hasBusinessNotebookDescription() ? notebook.businessNotebookDescription() : nullValue));
        query.bindValue(":businessNotebookPrivilegeLevel", (notebook.hasBusinessNotebookPrivilegeLevel() ? notebook.businessNotebookPrivilegeLevel() : nullValue));
        query.bindValue(":businessNotebookIsRecommended", (notebook.hasBusinessNotebookRecommended() ? (notebook.isBusinessNotebookRecommended() ? 1 : 0) : nullValue));
        query.bindValue(":contactId", (notebook.hasContact() && notebook.contact().hasId() ? notebook.contact().id() : nullValue));

        res = query.exec();
        DATABASE_CHECK_AND_SET_ERROR("can't insert or replace data into \"Notebooks\" table in SQL database");
    }

    if (notebook.hasRestrictions())
    {
        QString error;
        bool res = InsertOrReplaceNotebookRestrictions(notebook.restrictions(), localGuid, error);
        if (!res) {
            errorDescription += error;
            return res;
        }
    }
    else
    {
        bool res = CheckAndPrepareExpungeNotebookFromNotebookRestrictionsQuery();
        QSqlQuery & query = m_expungeNotebookFromNotebookRestrictionsQuery;
        DATABASE_CHECK_AND_SET_ERROR("can't clear notebook restrictions when updating notebook: "
                                     "can't prepare SQL query");

        query.bindValue(":localGuid", localGuid);

        res = query.exec();
        DATABASE_CHECK_AND_SET_ERROR("can't clear notebook restrictions when updating notebook");
    }

    if (notebook.hasGuid())
    {
        bool res = CheckAndPrepareExpungeSharedNotebooksQuery();
        QSqlQuery & query = m_expungeSharedNotebooksQuery;
        DATABASE_CHECK_AND_SET_ERROR("can't clear shared notebooks before the insertion: can't prepare SQL query");

        query.bindValue(":notebookGuid", notebook.guid());

        res = query.exec();
        DATABASE_CHECK_AND_SET_ERROR("can't clear shared notebooks before the insertion");

        QList<SharedNotebookAdapter> sharedNotebooks = notebook.sharedNotebooks();
        foreach(const SharedNotebookAdapter & sharedNotebook, sharedNotebooks)
        {
            if (!sharedNotebook.hasId()) {
                QNWARNING("Found shared notebook without primary identifier of the share set, skipping it");
                continue;
            }

            QString error;
            res = InsertOrReplaceSharedNotebook(sharedNotebook, error);
            if (!res) {
                errorDescription += error;
                return res;
            }
        }
    }

    return transaction.commit(errorDescription);
}

bool LocalStorageManagerPrivate::CheckAndPrepareGetNotebookCountQuery() const
{
    if (!m_getNotebookCountQueryPrepared)
    {
        QNDEBUG("Preparing SQL query to get the count of notebooks");

        m_getNotebookCountQuery = QSqlQuery(m_sqlDatabase);
        bool res = m_getNotebookCountQuery.prepare("SELECT COUNT(*) FROM Notebooks");
        if (res) {
            m_getNotebookCountQueryPrepared = true;
        }

        return res;
    }
    else
    {
        return true;
    }
}

bool LocalStorageManagerPrivate::CheckAndPrepareInsertOrReplaceNotebookQuery()
{
    if (!m_insertOrReplaceNotebookQueryPrepared)
    {
        QNDEBUG("Preparing SQL query to insert or replace notebook");

        m_insertOrReplaceNotebookQuery = QSqlQuery(m_sqlDatabase);
        bool res = m_insertOrReplaceNotebookQuery.prepare("INSERT OR REPLACE INTO Notebooks"
                                                          "(localGuid, guid, linkedNotebookGuid, updateSequenceNumber, "
                                                          "notebookName, notebookNameUpper, creationTimestamp, "
                                                          "modificationTimestamp, isDirty, isLocal, "
                                                          "isDefault, isLastUsed, hasShortcut, publishingUri, "
                                                          "publishingNoteSortOrder, publishingAscendingSort, "
                                                          "publicDescription, isPublished, stack, businessNotebookDescription, "
                                                          "businessNotebookPrivilegeLevel, businessNotebookIsRecommended, contactId) "
                                                          "VALUES(:localGuid, :guid, :linkedNotebookGuid, :updateSequenceNumber, "
                                                          ":notebookName, :notebookNameUpper, :creationTimestamp, "
                                                          ":modificationTimestamp, :isDirty, :isLocal, :isDefault, :isLastUsed, "
                                                          ":hasShortcut, :publishingUri, :publishingNoteSortOrder, :publishingAscendingSort, "
                                                          ":publicDescription, :isPublished, :stack, :businessNotebookDescription, "
                                                          ":businessNotebookPrivilegeLevel, :businessNotebookIsRecommended, :contactId)");
        if (res) {
            m_insertOrReplaceNotebookQueryPrepared = true;
        }

        return res;
    }
    else
    {
        return true;
    }
}

bool LocalStorageManagerPrivate::CheckAndPrepareExpungeNotebookFromNotebookRestrictionsQuery()
{
    if (!m_expungeNotebookFromNotebookRestrictionsQueryPrepared)
    {
        QNDEBUG("Preparing SQL query to expunge notebook from notebook restrictions table");

        m_expungeNotebookFromNotebookRestrictionsQuery = QSqlQuery(m_sqlDatabase);
        bool res = m_expungeNotebookFromNotebookRestrictionsQuery.prepare("DELETE FROM NotebookRestrictions WHERE localGuid = :localGuid");
        if (res) {
            m_expungeNotebookFromNotebookRestrictionsQueryPrepared = true;
        }

        return res;
    }
    else
    {
        return true;
    }
}

bool LocalStorageManagerPrivate::CheckAndPrepareInsertOrReplaceNotebookRestrictionsQuery()
{
    if (!m_insertOrReplaceNotebookRestrictionsQueryPrepared)
    {
        QNDEBUG("Preparing SQL query to insert or replace notebook restrictions");

        m_insertOrReplaceNotebookRestrictionsQuery = QSqlQuery(m_sqlDatabase);
        bool res = m_insertOrReplaceNotebookRestrictionsQuery.prepare("INSERT OR REPLACE INTO NotebookRestrictions"
                                                                      "(localGuid, noReadNotes, noCreateNotes, noUpdateNotes, "
                                                                      "noExpungeNotes, noShareNotes, noEmailNotes, noSendMessageToRecipients, "
                                                                      "noUpdateNotebook, noExpungeNotebook, noSetDefaultNotebook, "
                                                                      "noSetNotebookStack, noPublishToPublic, noPublishToBusinessLibrary, "
                                                                      "noCreateTags, noUpdateTags, noExpungeTags, noSetParentTag, "
                                                                      "noCreateSharedNotebooks, updateWhichSharedNotebookRestrictions, "
                                                                      "expungeWhichSharedNotebookRestrictions) "
                                                                      "VALUES(:localGuid, :noReadNotes, :noCreateNotes, :noUpdateNotes, "
                                                                      ":noExpungeNotes, :noShareNotes, :noEmailNotes, :noSendMessageToRecipients, "
                                                                      ":noUpdateNotebook, :noExpungeNotebook, :noSetDefaultNotebook, "
                                                                      ":noSetNotebookStack, :noPublishToPublic, :noPublishToBusinessLibrary, "
                                                                      ":noCreateTags, :noUpdateTags, :noExpungeTags, :noSetParentTag, "
                                                                      ":noCreateSharedNotebooks, :updateWhichSharedNotebookRestrictions, "
                                                                      ":expungeWhichSharedNotebookRestrictions)");
        if (res) {
            m_insertOrReplaceNotebookRestrictionsQueryPrepared = true;
        }

        return res;
    }
    else
    {
        return true;
    }
}

bool LocalStorageManagerPrivate::CheckAndPrepareExpungeSharedNotebooksQuery()
{
    if (!m_expungeSharedNotebooksQueryPrepared)
    {
        QNDEBUG("Preparing SQL query to expunge shared notebooks per notebook guid");

        m_expungeSharedNotebooksQuery = QSqlQuery(m_sqlDatabase);
        bool res = m_expungeSharedNotebooksQuery.prepare("DELETE FROM SharedNotebooks WHERE notebookGuid = :notebookGuid");
        if (res) {
            m_expungeSharedNotebooksQueryPrepared = true;
        }

        return res;
    }
    else
    {
        return true;
    }
}

bool LocalStorageManagerPrivate::CheckAndPrepareInsertOrReplaceSharedNotebokQuery()
{
    if (!m_insertOrReplaceSharedNotebookQueryPrepared)
    {
        QNDEBUG("Preparing SQL query to insert or replace shared notebook");

        m_insertOrReplaceSharedNotebookQuery = QSqlQuery(m_sqlDatabase);
        bool res = m_insertOrReplaceSharedNotebookQuery.prepare("INSERT OR REPLACE INTO SharedNotebooks"
                                                                "(shareId, userId, notebookGuid, sharedNotebookEmail, "
                                                                "sharedNotebookCreationTimestamp, sharedNotebookModificationTimestamp, "
                                                                "shareKey, sharedNotebookUsername, sharedNotebookPrivilegeLevel, "
                                                                "allowPreview, recipientReminderNotifyEmail, "
                                                                "recipientReminderNotifyInApp, indexInNotebook)"
                                                                "VALUES(:shareId, :userId, :notebookGuid, :sharedNotebookEmail, "
                                                                ":sharedNotebookCreationTimestamp, :sharedNotebookModificationTimestamp, "
                                                                ":shareKey, :sharedNotebookUsername, :sharedNotebookPrivilegeLevel, :allowPreview, "
                                                                ":recipientReminderNotifyEmail, :recipientReminderNotifyInApp, :indexInNotebook)");
        if (res) {
            m_insertOrReplaceSharedNotebookQueryPrepared = true;
        }

        return res;
    }
    else
    {
        return true;
    }
}

bool LocalStorageManagerPrivate::InsertOrReplaceLinkedNotebook(const LinkedNotebook & linkedNotebook,
                                                               QString & errorDescription)
{
    // NOTE: this method expects to be called after the linked notebook
    // is already checked for sanity ot its parameters

    errorDescription += QT_TR_NOOP("can't insert or replace linked notebook into "
                                   "local storage database");

    bool res = CheckAndPrepareInsertOrReplaceLinkedNotebookQuery();
    QSqlQuery & query = m_insertOrReplaceLinkedNotebookQuery;
    DATABASE_CHECK_AND_SET_ERROR("can't insert or replace data into \"LinkedNotebooks\" table in SQL database, "
                                 "can't prepare SQL query");

    QVariant nullValue;

    query.bindValue(":guid", (linkedNotebook.hasGuid() ? linkedNotebook.guid() : nullValue));
    query.bindValue(":updateSequenceNumber", (linkedNotebook.hasUpdateSequenceNumber() ? linkedNotebook.updateSequenceNumber() : nullValue));
    query.bindValue(":shareName", (linkedNotebook.hasShareName() ? linkedNotebook.shareName() : nullValue));
    query.bindValue(":username", (linkedNotebook.hasUsername() ? linkedNotebook.username() : nullValue));
    query.bindValue(":shardId", (linkedNotebook.hasShardId() ? linkedNotebook.shardId() : nullValue));
    query.bindValue(":shareKey", (linkedNotebook.hasShareKey() ? linkedNotebook.shareKey() : nullValue));
    query.bindValue(":uri", (linkedNotebook.hasUri() ? linkedNotebook.uri() : nullValue));
    query.bindValue(":noteStoreUrl", (linkedNotebook.hasNoteStoreUrl() ? linkedNotebook.noteStoreUrl() : nullValue));
    query.bindValue(":webApiUrlPrefix", (linkedNotebook.hasWebApiUrlPrefix() ? linkedNotebook.webApiUrlPrefix() : nullValue));
    query.bindValue(":stack", (linkedNotebook.hasStack() ? linkedNotebook.stack() : nullValue));
    query.bindValue(":businessId", (linkedNotebook.hasBusinessId() ? linkedNotebook.businessId() : nullValue));
    query.bindValue(":isDirty", (linkedNotebook.isDirty() ? 1 : 0));

    res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("can't insert or replace notebook into \"LinkedNotebooks\" table in SQL database");

    return true;
}

bool LocalStorageManagerPrivate::CheckAndPrepareGetLinkedNotebookCountQuery() const
{
    if (!m_getLinkedNotebookCountQueryPrepared)
    {
        QNDEBUG("Preparing SQL query to get the count of linked notebooks");

        m_getLinkedNotebookCountQuery = QSqlQuery(m_sqlDatabase);
        bool res = m_getLinkedNotebookCountQuery.prepare("SELECT COUNT(*) FROM LinkedNotebooks");
        if (res) {
            m_getLinkedNotebookCountQueryPrepared = true;
        }

        return res;
    }
    else
    {
        return true;
    }
}

bool LocalStorageManagerPrivate::CheckAndPrepareInsertOrReplaceLinkedNotebookQuery()
{
    if (!m_insertOrReplaceLinkedNotebookQueryPrepared)
    {
        QNDEBUG("Preparing SQL query to insert or replace linked notebook");

        QString columns = "guid, updateSequenceNumber, shareName, username, shardId, shareKey, "
                          "uri, noteStoreUrl, webApiUrlPrefix, stack, businessId, isDirty";

        QString values = ":guid, :updateSequenceNumber, :shareName, :username, :shardId, :shareKey, "
                         ":uri, :noteStoreUrl, :webApiUrlPrefix, :stack, :businessId, :isDirty";

        QSqlQuery query(m_sqlDatabase);

        m_insertOrReplaceLinkedNotebookQuery = QSqlQuery(m_sqlDatabase);
        bool res = m_insertOrReplaceLinkedNotebookQuery.prepare("INSERT OR REPLACE INTO LinkedNotebooks "
                                                                "(guid, updateSequenceNumber, shareName, username, shardId, shareKey, "
                                                                "uri, noteStoreUrl, webApiUrlPrefix, stack, businessId, isDirty) "
                                                                "VALUES(:guid, :updateSequenceNumber, :shareName, :username, :shardId, :shareKey, "
                                                                ":uri, :noteStoreUrl, :webApiUrlPrefix, :stack, :businessId, :isDirty)");
        if (res) {
            m_insertOrReplaceLinkedNotebookQueryPrepared = true;
        }

        return res;
    }
    else
    {
        return true;
    }
}

bool LocalStorageManagerPrivate::CheckAndPrepareExpungeLinkedNotebookQuery()
{
    if (!m_expungeLinkedNotebookQueryPrepared)
    {
        QNDEBUG("Preparing SQL query to expunge linked notebook");

        m_expungeLinkedNotebookQuery = QSqlQuery(m_sqlDatabase);
        bool res = m_expungeLinkedNotebookQuery.prepare("DELETE FROM LinkedNotebooks WHERE guid = :guid");
        if (res) {
            m_expungeLinkedNotebookQueryPrepared = true;
        }

        return res;
    }
    else
    {
        return true;
    }
}

bool LocalStorageManagerPrivate::InsertOrReplaceNote(const Note & note, const Notebook & notebook,
                                                     const QString & overrideLocalGuid, QString & errorDescription)
{
    // NOTE: this method expects to be called after the note is already checked
    // for sanity of its parameters!

    errorDescription += QT_TR_NOOP("can't insert or replace note into local storage database: ");

    Transaction transaction(m_sqlDatabase, *this, Transaction::Exclusive);

    QVariant nullValue;
    QString localGuid = (overrideLocalGuid.isEmpty() ? note.localGuid() : overrideLocalGuid);

    // Update common table with Note properties
    {
        bool res = CheckAndPrepareInsertOrReplaceNoteQuery();
        QSqlQuery & query = m_insertOrReplaceNoteQuery;
        DATABASE_CHECK_AND_SET_ERROR("Can't insert data into \"Notes\" table in SQL database: can't prepare SQL query");

        query.bindValue(":localGuid", localGuid);
        query.bindValue(":guid", (note.hasGuid() ? note.guid() : nullValue));
        query.bindValue(":updateSequenceNumber", (note.hasUpdateSequenceNumber() ? note.updateSequenceNumber() : nullValue));
        query.bindValue(":isDirty", (note.isDirty() ? 1 : 0));
        query.bindValue(":isLocal", (note.isLocal() ? 1 : 0));
        query.bindValue(":hasShortcut", (note.hasShortcut() ? 1 : 0));
        query.bindValue(":title", (note.hasTitle() ? note.title() : nullValue));
        query.bindValue(":content", (note.hasContent() ? note.content() : nullValue));
        query.bindValue(":contentContainsUnfinishedToDo", (note.containsUncheckedTodo() ? 1 : nullValue));
        query.bindValue(":contentContainsFinishedToDo", (note.containsCheckedTodo() ? 1 : nullValue));
        query.bindValue(":contentContainsEncryption", (note.containsEncryption() ? 1 : nullValue));
        query.bindValue(":contentLength", (note.hasContentLength() ? note.contentLength() : nullValue));
        query.bindValue(":contentHash", (note.hasContentHash() ? note.contentHash() : nullValue));

        if (note.hasContent())
        {
            QString error;
            QString contentPlainText = note.plainText(&error);
            if (!error.isEmpty()) {
                errorDescription += QT_TR_NOOP("can't get note's plain text: ") + error;
                QNWARNING(errorDescription);
                return false;
            }

            error.resize(0);
            QStringList contentListOfWords = note.listOfWords(&error);
            if (!error.isEmpty()) {
                errorDescription += QT_TR_NOOP("can't get note's list of words: ") + error;
                QNWARNING(errorDescription);
                return false;
            }
            QString listOfWords = contentListOfWords.join(" ");

            query.bindValue(":contentPlainText", (contentPlainText.isEmpty() ? nullValue : contentPlainText));
            query.bindValue(":contentListOfWords", (listOfWords.isEmpty() ? nullValue : listOfWords));
        }
        else {
            query.bindValue(":contentPlainText", nullValue);
            query.bindValue(":contentListOfWords", nullValue);
        }

        query.bindValue(":creationTimestamp", (note.hasCreationTimestamp() ? note.creationTimestamp() : nullValue));
        query.bindValue(":modificationTimestamp", (note.hasModificationTimestamp() ? note.modificationTimestamp() : nullValue));
        query.bindValue(":deletionTimestamp", (note.hasDeletionTimestamp() ? note.deletionTimestamp() : nullValue));
        query.bindValue(":isActive", (note.hasActive() ? (note.active() ? 1 : 0) : nullValue));
        query.bindValue(":hasAttributes", (note.hasNoteAttributes() ? 1 : 0));

        QImage thumbnail = note.thumbnail();
        bool thumbnailIsNull = thumbnail.isNull();

        query.bindValue(":thumbnail", (thumbnailIsNull ? nullValue : thumbnail));
        query.bindValue(":notebookLocalGuid", (notebook.localGuid().isEmpty() ? nullValue : notebook.localGuid()));
        query.bindValue(":notebookGuid", (note.hasNotebookGuid() ? note.notebookGuid() : nullValue));

        if (note.hasNoteAttributes())
        {
            const qevercloud::NoteAttributes & attributes = note.noteAttributes();

#define BIND_ATTRIBUTE(name) \
    query.bindValue(":" #name, (attributes.name.isSet() ? attributes.name.ref() : nullValue))

            BIND_ATTRIBUTE(subjectDate);
            BIND_ATTRIBUTE(latitude);
            BIND_ATTRIBUTE(longitude);
            BIND_ATTRIBUTE(altitude);
            BIND_ATTRIBUTE(author);
            BIND_ATTRIBUTE(source);
            BIND_ATTRIBUTE(sourceURL);
            BIND_ATTRIBUTE(sourceApplication);
            BIND_ATTRIBUTE(shareDate);
            BIND_ATTRIBUTE(reminderOrder);
            BIND_ATTRIBUTE(reminderDoneTime);
            BIND_ATTRIBUTE(reminderTime);
            BIND_ATTRIBUTE(placeName);
            BIND_ATTRIBUTE(contentClass);
            BIND_ATTRIBUTE(lastEditedBy);
            BIND_ATTRIBUTE(creatorId);
            BIND_ATTRIBUTE(lastEditorId);

#undef BIND_ATTRIBUTE

            if (attributes.applicationData.isSet())
            {
                const qevercloud::LazyMap & lazyMap = attributes.applicationData.ref();

                if (lazyMap.keysOnly.isSet())
                {
                    const QSet<QString> & keysOnly = lazyMap.keysOnly.ref();
                    QString keysOnlyString;

                    typedef QSet<QString>::const_iterator CIter;
                    CIter keysOnlyEnd = keysOnly.constEnd();
                    for(CIter it = keysOnly.constBegin(); it != keysOnlyEnd; ++it) {
                        keysOnlyString += "'";
                        keysOnlyString += *it;
                        keysOnlyString += "'";
                    }

                    QNDEBUG("Application data keys only string: " << keysOnlyString);

                    query.bindValue(":applicationDataKeysOnly", keysOnlyString);
                }
                else
                {
                    query.bindValue(":applicationDataKeysOnly", nullValue);
                }

                if (lazyMap.fullMap.isSet())
                {
                    const QMap<QString, QString> & fullMap = lazyMap.fullMap.ref();
                    QString fullMapKeysString;
                    QString fullMapValuesString;

                    typedef QMap<QString, QString>::const_iterator CIter;
                    CIter fullMapEnd = fullMap.constEnd();
                    for(CIter it = fullMap.constBegin(); it != fullMapEnd; ++it)
                    {
                        fullMapKeysString += "'";
                        fullMapKeysString += it.key();
                        fullMapKeysString += "'";

                        fullMapValuesString += "'";
                        fullMapValuesString += it.value();
                        fullMapValuesString += "'";
                    }

                    QNDEBUG("Application data map keys: " << fullMapKeysString
                            << ", application data map values: " << fullMapValuesString);

                    query.bindValue(":applicationDataKeysMap", fullMapKeysString);
                    query.bindValue(":applicationDataValues", fullMapValuesString);
                }
                else
                {
                    query.bindValue(":applicationDataKeysMap", nullValue);
                    query.bindValue(":applicationDataValues", nullValue);
                }
            }
            else
            {
                query.bindValue(":applicationDataKeysOnly", nullValue);
                query.bindValue(":applicationDataKeysMap", nullValue);
                query.bindValue(":applicationDataValues", nullValue);
            }

            if (attributes.classifications.isSet())
            {
                const QMap<QString, QString> & classifications = attributes.classifications.ref();
                QString classificationKeys, classificationValues;
                typedef QMap<QString, QString>::const_iterator CIter;
                CIter classificationsEnd = classifications.constEnd();
                for(CIter it = classifications.constBegin(); it != classificationsEnd; ++it)
                {
                    classificationKeys += "'";
                    classificationKeys += it.key();
                    classificationKeys += "'";

                    classificationValues += "'";
                    classificationValues += it.value();
                    classificationValues += "'";
                }

                QNDEBUG("Classification keys: " << classificationKeys << ", classification values"
                        << classificationValues);

                query.bindValue(":classificationKeys", classificationKeys);
                query.bindValue(":classificationValues", classificationValues);
            }
            else
            {
                query.bindValue(":classificationKeys", nullValue);
                query.bindValue(":classificationValues", nullValue);
            }
        }
        else
        {
#define BIND_NULL_ATTRIBUTE(name) \
    query.bindValue(":" #name, nullValue)

            BIND_NULL_ATTRIBUTE(subjectDate);
            BIND_NULL_ATTRIBUTE(latitude);
            BIND_NULL_ATTRIBUTE(longitude);
            BIND_NULL_ATTRIBUTE(altitude);
            BIND_NULL_ATTRIBUTE(author);
            BIND_NULL_ATTRIBUTE(source);
            BIND_NULL_ATTRIBUTE(sourceURL);
            BIND_NULL_ATTRIBUTE(sourceApplication);
            BIND_NULL_ATTRIBUTE(shareDate);
            BIND_NULL_ATTRIBUTE(reminderOrder);
            BIND_NULL_ATTRIBUTE(reminderDoneTime);
            BIND_NULL_ATTRIBUTE(reminderTime);
            BIND_NULL_ATTRIBUTE(placeName);
            BIND_NULL_ATTRIBUTE(contentClass);
            BIND_NULL_ATTRIBUTE(lastEditedBy);
            BIND_NULL_ATTRIBUTE(creatorId);
            BIND_NULL_ATTRIBUTE(lastEditorId);
            BIND_NULL_ATTRIBUTE(applicationDataKeysOnly);
            BIND_NULL_ATTRIBUTE(applicationDataKeysMap);
            BIND_NULL_ATTRIBUTE(applicationDataValues);
            BIND_NULL_ATTRIBUTE(classificationKeys);
            BIND_NULL_ATTRIBUTE(classificationValues);

#undef BIND_NULL_ATTRIBUTE
        }

        res = query.exec();
        DATABASE_CHECK_AND_SET_ERROR("can't insert or replace note into \"Notes\" table in SQL database");
    }

    // Clear note-to-tag binding first, update them second
    {
        bool res = CheckAndPrepareExpungeNoteFromNoteTagsQuery();
        QSqlQuery & query = m_expungeNoteFromNoteTagsQuery;
        DATABASE_CHECK_AND_SET_ERROR("can't clear note's tags when updating note: can't prepare SQL query");

        query.bindValue(":localNote", localGuid);

        res = query.exec();
        DATABASE_CHECK_AND_SET_ERROR("can't clear note's tags when updating note");
    }

    if (note.hasTagGuids())
    {
        QString error;

        QStringList tagGuids;
        note.tagGuids(tagGuids);
        int numTagGuids = tagGuids.size();

        bool res = CheckAndPrepareInsertOrReplaceNoteIntoNoteTagsQuery();
        QSqlQuery & query = m_insertOrReplaceNoteIntoNoteTagsQuery;
        DATABASE_CHECK_AND_SET_ERROR("can't insert or replace data into \"NoteTags\" table: "
                                     "can't prepare SQL query");

        for(int i = 0; i < numTagGuids; ++i)
        {
            // NOTE: the behavior expressed here is valid since tags are synchronized before notes
            // so they must exist within local storage database; if they don't then something went really wrong

            const QString & tagGuid = tagGuids[i];

            Tag tag;
            tag.setGuid(tagGuid);
            bool res = FindTag(tag, error);
            if (!res) {
                errorDescription += QT_TR_NOOP("failed to find one of note's tags: ");
                errorDescription += error;
                QNCRITICAL(errorDescription);
                return false;
            }

            query.bindValue(":localNote", localGuid);
            query.bindValue(":note", (note.hasGuid() ? note.guid() : nullValue));
            query.bindValue(":localTag", tag.localGuid());
            query.bindValue(":tag", tagGuid);
            query.bindValue(":tagIndexInNote", i);

            res = query.exec();
            DATABASE_CHECK_AND_SET_ERROR("can't insert or replace data into \"NoteTags\" table in SQL database");
        }
    }

    // NOTE: don't even attempt fo find tags by their names because qevercloud::Note.tagNames
    // has the only purpose to provide tag names alternatively to guids to NoteStore::createNote method

    // Clear note's resources first and insert new ones second
    // FIXME: it can be inefficient; need to figure out some partial update approach
    {
        bool res = CheckAndPrepareExpungeResourcesByNoteQuery();
        QSqlQuery & query = m_expungeResourceByNoteQuery;
        DATABASE_CHECK_AND_SET_ERROR("can't clear resources when updating note: "
                                     "can't prepare SQL query");

        query.bindValue(":noteLocalGuid", localGuid);

        res = query.exec();
        DATABASE_CHECK_AND_SET_ERROR("can't clear resources when updating note");
    }

    if (note.hasResources())
    {
        QList<ResourceAdapter> resources = note.resourceAdapters();
        int numResources = resources.size();
        for(int i = 0; i < numResources; ++i)
        {
            const ResourceAdapter & resource = resources[i];

            QString error;
            bool res = resource.checkParameters(error);
            if (!res) {
                errorDescription += QT_TR_NOOP("found invalid resource linked with note: ");
                errorDescription += error;
                QNWARNING(errorDescription);
                return false;
            }

            error.resize(0);
            res = InsertOrReplaceResource(resource, QString(), note, localGuid, error,
                                          /* useSeparateTransaction = */ false);
            if (!res) {
                errorDescription += QT_TR_NOOP("can't add or update one of note's "
                                               "attached resources: ");
                errorDescription += error;
                return false;
            }
        }
    }

    return transaction.commit(errorDescription);
}

bool LocalStorageManagerPrivate::CheckAndPrepareGetNoteCountQuery() const
{
    if (!m_getNoteCountQueryPrepared)
    {
        QNDEBUG("Preparing SQL query to get the count of notes");

        m_getNoteCountQuery = QSqlQuery(m_sqlDatabase);
        bool res = m_getNoteCountQuery.prepare("SELECT COUNT(*) FROM Notes WHERE deletionTimestamp IS NULL");
        if (res) {
            m_getNoteCountQueryPrepared = true;
        }

        return res;
    }
    else
    {
        return true;
    }
}

bool LocalStorageManagerPrivate::CheckAndPrepareInsertOrReplaceNoteQuery()
{
    if (!m_insertOrReplaceNoteQueryPrepared)
    {
        QNDEBUG("Preparing SQL query to insert or replace note");

        m_insertOrReplaceNoteQuery = QSqlQuery(m_sqlDatabase);

        QString columns = "localGuid, guid, updateSequenceNumber, isDirty, isLocal, "
                          "hasShortcut, title, content, contentLength, contentHash, "
                          "contentPlainText, contentListOfWords, contentContainsFinishedToDo, "
                          "contentContainsUnfinishedToDo, contentContainsEncryption, creationTimestamp, "
                          "modificationTimestamp, deletionTimestamp, isActive, hasAttributes, "
                          "thumbnail, notebookLocalGuid, notebookGuid, subjectDate, latitude, "
                          "longitude, altitude, author, source, sourceURL, sourceApplication, "
                          "shareDate, reminderOrder, reminderDoneTime, reminderTime, placeName, "
                          "contentClass, lastEditedBy, creatorId, lastEditorId, "
                          "applicationDataKeysOnly, applicationDataKeysMap, "
                          "applicationDataValues, classificationKeys, classificationValues";

        QString values = ":localGuid, :guid, :updateSequenceNumber, :isDirty, :isLocal, "
                         ":hasShortcut, :title, :content, :contentLength, :contentHash, "
                         ":contentPlainText, :contentListOfWords, :contentContainsFinishedToDo, "
                         ":contentContainsUnfinishedToDo, :contentContainsEncryption, :creationTimestamp, "
                         ":modificationTimestamp, :deletionTimestamp, :isActive, :hasAttributes, "
                         ":thumbnail, :notebookLocalGuid, :notebookGuid, :subjectDate, :latitude, "
                         ":longitude, :altitude, :author, :source, :sourceURL, :sourceApplication, "
                         ":shareDate, :reminderOrder, :reminderDoneTime, :reminderTime, :placeName, "
                         ":contentClass, :lastEditedBy, :creatorId, :lastEditorId, "
                         ":applicationDataKeysOnly, :applicationDataKeysMap, "
                         ":applicationDataValues, :classificationKeys, :classificationValues";

        QString queryString = QString("INSERT OR REPLACE INTO Notes(%1) VALUES(%2)").arg(columns).arg(values);

        bool res = m_insertOrReplaceNoteQuery.prepare(queryString);
        if (res) {
            m_insertOrReplaceNoteQueryPrepared = true;
        }

        return res;
    }
    else
    {
        return true;
    }
}

bool LocalStorageManagerPrivate::CheckAndPrepareExpungeNoteFromNoteTagsQuery()
{
    if (!m_expungeNoteFromNoteTagsQueryPrepared)
    {
        QNDEBUG("Preparing SQL query to expunge note from NoteTags table");

        m_expungeNoteFromNoteTagsQuery = QSqlQuery(m_sqlDatabase);
        bool res = m_expungeNoteFromNoteTagsQuery.prepare("DELETE From NoteTags WHERE localNote = :localNote");
        if (res) {
            m_expungeNoteFromNoteTagsQueryPrepared = true;
        }

        return res;
    }
    else
    {
        return true;
    }
}

bool LocalStorageManagerPrivate::CheckAndPrepareInsertOrReplaceNoteIntoNoteTagsQuery()
{
    if (!m_insertOrReplaceNoteIntoNoteTagsQueryPrepared)
    {
        QNDEBUG("Preparing SQL query to insert or replace note into NoteTags table");

        m_insertOrReplaceNoteIntoNoteTagsQuery = QSqlQuery(m_sqlDatabase);
        bool res = m_insertOrReplaceNoteIntoNoteTagsQuery.prepare("INSERT OR REPLACE INTO NoteTags"
                                                                  "(localNote, note, localTag, tag, tagIndexInNote) "
                                                                  "VALUES(:localNote, :note, :localTag, :tag, :tagIndexInNote)");
        if (res) {
            m_insertOrReplaceNoteIntoNoteTagsQueryPrepared = true;
        }

        return res;
    }
    else
    {
        return true;
    }
}

bool LocalStorageManagerPrivate::CheckAndPrepareExpungeResourcesByNoteQuery()
{
    if (!m_expungeResourceByNoteQueryPrepared)
    {
        QNDEBUG("Preparing SQL query to expunge resource by note");

        m_expungeResourceByNoteQuery = QSqlQuery(m_sqlDatabase);
        bool res = m_expungeResourceByNoteQuery.prepare("DELETE FROM Resources WHERE noteLocalGuid = :noteLocalGuid");
        if (res) {
            m_expungeResourceByNoteQueryPrepared = true;
        }

        return res;
    }
    else
    {
        return true;
    }
}

bool LocalStorageManagerPrivate::InsertOrReplaceTag(const Tag & tag, const QString & overrideLocalGuid,
                                                    QString & errorDescription)
{
    // NOTE: this method expects to be called after tag is already checked
    // for sanity of its parameters!

    errorDescription = QT_TR_NOOP("Can't insert or replace tag into local storage database: ");

    QString localGuid = (overrideLocalGuid.isEmpty() ? tag.localGuid() : overrideLocalGuid);

    bool res = CheckAndPrepareInsertOrReplaceTagQuery();
    QSqlQuery & query = m_insertOrReplaceTagQuery;
    DATABASE_CHECK_AND_SET_ERROR("can't prepare SQL query to insert or replace tag into \"Tags\" table in SQL database");

    QVariant nullValue;

    query.bindValue(":localGuid", localGuid);
    query.bindValue(":guid", (tag.hasGuid() ? tag.guid() : nullValue));
    query.bindValue(":linkedNotebookGuid", tag.hasLinkedNotebookGuid() ? tag.linkedNotebookGuid() : nullValue);
    query.bindValue(":updateSequenceNumber", (tag.hasUpdateSequenceNumber() ? tag.updateSequenceNumber() : nullValue));
    query.bindValue(":name", (tag.hasName() ? tag.name() : nullValue));
    query.bindValue(":nameUpper", (tag.hasName() ? tag.name().toUpper() : nullValue));
    query.bindValue(":parentGuid", (tag.hasParentGuid() ? tag.parentGuid() : nullValue));
    query.bindValue(":isDirty", (tag.isDirty() ? 1 : 0));
    query.bindValue(":isLocal", (tag.isLocal() ? 1 : 0));
    query.bindValue(":isDeleted", (tag.isDeleted() ? 1 : 0));
    query.bindValue(":hasShortcut", (tag.hasShortcut() ? 1 : 0));

    res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("can't insert or replace tag into \"Tags\" table in SQL database");

    return true;
}

bool LocalStorageManagerPrivate::CheckAndPrepareGetTagCountQuery() const
{
    if (!m_getTagCountQueryPrepared)
    {
        m_getTagCountQuery = QSqlQuery(m_sqlDatabase);
        bool res = m_getTagCountQuery.prepare("SELECT COUNT(*) FROM Tags WHERE isDeleted = 0");
        if (res) {
            m_getTagCountQueryPrepared = true;
        }

        return res;
    }
    else
    {
        return true;
    }
}

bool LocalStorageManagerPrivate::CheckAndPrepareInsertOrReplaceTagQuery()
{
    if (!m_insertOrReplaceTagQueryPrepared)
    {
        m_insertOrReplaceTagQuery = QSqlQuery(m_sqlDatabase);
        bool res = m_insertOrReplaceTagQuery.prepare("INSERT OR REPLACE INTO Tags "
                                                     "(localGuid, guid, linkedNotebookGuid, updateSequenceNumber, "
                                                     "name, nameUpper, parentGuid, isDirty, "
                                                     "isLocal, isDeleted, hasShortcut) "
                                                     "VALUES(:localGuid, :guid, :linkedNotebookGuid, "
                                                     ":updateSequenceNumber, :name, :nameUpper, "
                                                     ":parentGuid, :isDirty, :isLocal, "
                                                     ":isDeleted, :hasShortcut)");
        if (res) {
            m_insertOrReplaceTagQueryPrepared = true;
        }

        return res;
    }
    else
    {
        return true;
    }
}

bool LocalStorageManagerPrivate::CheckAndPrepareDeleteTagQuery()
{
    if (!m_deleteTagQueryPrepared)
    {
        QNDEBUG("Preparing SQL query to delete tag");

        m_deleteTagQuery = QSqlQuery(m_sqlDatabase);
        bool res = m_deleteTagQuery.prepare("UPDATE Tags SET isDeleted = 1, isDirty = 1 WHERE localGuid = :localGuid");
        if (res) {
            m_deleteTagQueryPrepared = true;
        }

        return res;
    }
    else
    {
        return true;
    }
}

bool LocalStorageManagerPrivate::CheckAndPrepareExpungeTagQuery()
{
    if (!m_expungeTagQueryPrepared)
    {
        m_expungeTagQuery = QSqlQuery(m_sqlDatabase);
        bool res = m_expungeTagQuery.prepare("DELETE FROM Tags WHERE localGuid = :localGuid");
        if (res) {
            m_expungeTagQueryPrepared = true;
        }

        return res;
    }
    else
    {
        return true;
    }
}

bool LocalStorageManagerPrivate::InsertOrReplaceResource(const IResource & resource, const QString overrideResourceLocalGuid,
                                                         const Note & note, const QString & overrideNoteLocalGuid,
                                                         QString & errorDescription, const bool useSeparateTransaction)
{
    // NOTE: this method expects to be called after resource is already checked
    // for sanity of its parameters!

    errorDescription = QT_TR_NOOP("Can't insert or replace resource into local storage database: ");

    QScopedPointer<Transaction> pTransaction;
    if (useSeparateTransaction) {
        pTransaction.reset(new Transaction(m_sqlDatabase, *this, Transaction::Exclusive));
    }

    QVariant nullValue;
    QString resourceLocalGuid = (overrideResourceLocalGuid.isEmpty()
                                 ? resource.localGuid()
                                 : overrideResourceLocalGuid);

    // Updating common resource information in Resources table
    {
        bool res = CheckAndPrepareInsertOrReplaceResourceQuery();
        QSqlQuery & query = m_insertOrReplaceResourceQuery;
        DATABASE_CHECK_AND_SET_ERROR("can't insert or replace data into \"Resources\" table in SQL database, "
                                     "can't prepare SQL query");

        query.bindValue(":resourceGuid", (resource.hasGuid() ? resource.guid() : nullValue));
        query.bindValue(":noteGuid", (resource.hasNoteGuid() ? resource.noteGuid() : nullValue));
        query.bindValue(":dataBody", (resource.hasDataBody() ? resource.dataBody() : nullValue));
        query.bindValue(":dataSize", (resource.hasDataSize() ? resource.dataSize() : nullValue));
        query.bindValue(":dataHash", (resource.hasDataHash() ? resource.dataHash() : nullValue));
        query.bindValue(":mime", (resource.hasMime() ? resource.mime() : nullValue));
        query.bindValue(":width", (resource.hasWidth() ? resource.width() : nullValue));
        query.bindValue(":height", (resource.hasHeight() ? resource.height() : nullValue));
        query.bindValue(":recognitionDataBody", (resource.hasRecognitionDataBody() ? resource.recognitionDataBody() : nullValue));
        query.bindValue(":recognitionDataSize", (resource.hasRecognitionDataSize() ? resource.recognitionDataSize() : nullValue));
        query.bindValue(":recognitionDataHash", (resource.hasRecognitionDataHash() ? resource.recognitionDataHash() : nullValue));
        query.bindValue(":alternateDataBody", (resource.hasAlternateDataBody() ? resource.alternateDataBody() : nullValue));
        query.bindValue(":alternateDataSize", (resource.hasAlternateDataSize() ? resource.alternateDataSize() : nullValue));
        query.bindValue(":alternateDataHash", (resource.hasAlternateDataHash() ? resource.alternateDataHash() : nullValue));
        query.bindValue(":resourceUpdateSequenceNumber", (resource.hasUpdateSequenceNumber() ? resource.updateSequenceNumber() : nullValue));
        query.bindValue(":resourceIsDirty", (resource.isDirty() ? 1 : 0));
        query.bindValue(":resourceIndexInNote", resource.indexInNote());
        query.bindValue(":resourceLocalGuid", resourceLocalGuid);

        res = query.exec();
        DATABASE_CHECK_AND_SET_ERROR("can't insert or replace data into \"Resources\" table in SQL database");
    }

    // Updating connections between note and resource in NoteResources table
    {
        bool res = CheckAndPrepareInsertOrReplaceNoteResourceQuery();
        QSqlQuery & query = m_insertOrReplaceNoteResourceQuery;
        DATABASE_CHECK_AND_SET_ERROR("can't insert or replace data into \"NoteResources\" table in SQL database, "
                                     "can't prepare SQL query");

        QString noteLocalGuid = (overrideNoteLocalGuid.isEmpty()
                                 ? note.localGuid() : overrideNoteLocalGuid);

        query.bindValue(":localNote", (noteLocalGuid.isEmpty() ? nullValue : noteLocalGuid));
        query.bindValue(":note", (note.hasGuid() ? note.guid() : nullValue));
        query.bindValue(":localResource", resourceLocalGuid);
        query.bindValue(":resource", (resource.hasGuid() ? resource.guid() : nullValue));

        res = query.exec();
        DATABASE_CHECK_AND_SET_ERROR("can't insert or replace data into \"NoteResources\" table in SQL database");
    }

    // Removing resource's local guid from ResourceRecognitionTypes table
    {
        bool res = CheckAndPrepareDeleteResourceFromResourceRecognitionTypesQuery();
        QSqlQuery & query = m_deleteResourceFromResourceRecognitionTypesQuery;
        DATABASE_CHECK_AND_SET_ERROR("can't delete data from ResourceRecognitionTypes table: "
                                     "can't prepare SQL query");

        query.bindValue(":resourceLocalGuid", resourceLocalGuid);

        res = query.exec();
        DATABASE_CHECK_AND_SET_ERROR("can't delete data from ResourceRecognitionTypes table");
    }

    QStringList recognitionTypes = resource.recognitionTypes();
    if (!recognitionTypes.isEmpty())
    {
        // Updating resource's recognition types

        bool res = CheckAndPrepareInsertOrReplaceIntoResourceRecognitionTypesQuery();
        QSqlQuery & query = m_insertOrReplaceIntoResourceRecognitionTypesQuery;
        DATABASE_CHECK_AND_SET_ERROR("can't insert or replace data into \"ResourceRecognitionTypes\" table in SQL database, "
                                     "can't prepare SQL query");

        foreach(const QString & recognitionType, recognitionTypes)
        {
            query.bindValue(":resourceLocalGuid", resourceLocalGuid);
            query.bindValue(":recognitionType", recognitionType);

            res = query.exec();
            DATABASE_CHECK_AND_SET_ERROR("can't insert or replace data into \"ResourceRecognitionTypes\" table in SQL database");
        }
    }

    // Removing resource from ResourceAttributes table
    {
        bool res = CheckAndPrepareDeleteResourceFromResourceAttributesQuery();
        QSqlQuery & query = m_deleteResourceFromResourceAttributesQuery;
        DATABASE_CHECK_AND_SET_ERROR("can't delete data from ResourceAttributes table: "
                                     "can't prepare SQL query");

        query.bindValue(":resourceLocalGuid", resourceLocalGuid);

        res = query.exec();
        DATABASE_CHECK_AND_SET_ERROR("can't delete data from ResourceAttributes table");
    }

    // Removing resource from ResourceAttributesApplicationDataKeysOnly table
    {
        bool res = CheckAndPrepareDeleteResourceFromResourceAttributesApplicationDataKeysOnlyQuery();
        QSqlQuery & query = m_deleteResourceFromResourceAttributesApplicationDataKeysOnlyQuery;
        DATABASE_CHECK_AND_SET_ERROR("can't delete data from ResourceAttributesApplicationDataKeysOnly table: "
                                     "can't prepare SQL query");

        query.bindValue(":resourceLocalGuid", resourceLocalGuid);

        res = query.exec();
        DATABASE_CHECK_AND_SET_ERROR("can't delete data from ResourceAttributesApplicationDataKeysOnly table");
    }

    // Removing resource from ResourceAttributesApplicationDataFullMap table
    {
        bool res = CheckAndPrepareDeleteResourceFromResourceAttributesApplicationDataFullMapQuery();
        QSqlQuery & query = m_deleteResourceFromResourceAttributesApplicationDataFullMapQuery;
        DATABASE_CHECK_AND_SET_ERROR("can't delete data from ResourceAttributesApplicationDataFullMap: "
                                     "can't prepare SQL query");

        query.bindValue(":resourceLocalGuid", resourceLocalGuid);

        res = query.exec();
        DATABASE_CHECK_AND_SET_ERROR("can't delete data from ResourceAttributesApplicationDataFullMap");
    }

    if (resource.hasResourceAttributes())
    {
        const qevercloud::ResourceAttributes & attributes = resource.resourceAttributes();
        bool res = InsertOrReplaceResourceAttributes(resourceLocalGuid, attributes, errorDescription);
        if (!res) {
            return false;
        }
    }

    if (!pTransaction.isNull()) {
        return pTransaction->commit(errorDescription);
    }
    else {
        return true;
    }
}

bool LocalStorageManagerPrivate::InsertOrReplaceResourceAttributes(const QString & localGuid,
                                                                   const qevercloud::ResourceAttributes & attributes,
                                                                   QString & errorDescription)
{
    QVariant nullValue;

    // Insert or replace attributes into ResourceAttributes table
    {
        bool res = CheckAndPrepareInsertOrReplaceResourceAttributesQuery();
        QSqlQuery & query = m_insertOrReplaceResourceAttributesQuery;
        DATABASE_CHECK_AND_SET_ERROR("can't insert or replace data into \"ResourceAttributes\" table in SQL database, "
                                     "can't prepare SQL query");

        query.bindValue(":resourceLocalGuid", localGuid);
        query.bindValue(":resourceSourceURL", (attributes.sourceURL.isSet() ? attributes.sourceURL.ref() : nullValue));
        query.bindValue(":timestamp", (attributes.timestamp.isSet() ? attributes.timestamp.ref() : nullValue));
        query.bindValue(":resourceLatitude", (attributes.latitude.isSet() ? attributes.latitude.ref() : nullValue));
        query.bindValue(":resourceLongitude", (attributes.longitude.isSet() ? attributes.longitude.ref() : nullValue));
        query.bindValue(":resourceAltitude", (attributes.altitude.isSet() ? attributes.altitude.ref() : nullValue));
        query.bindValue(":cameraMake", (attributes.cameraMake.isSet() ? attributes.cameraMake.ref() : nullValue));
        query.bindValue(":cameraModel", (attributes.cameraModel.isSet() ? attributes.cameraModel.ref() : nullValue));
        query.bindValue(":clientWillIndex", (attributes.clientWillIndex.isSet() ? (attributes.clientWillIndex.ref() ? 1 : 0) : nullValue));
        query.bindValue(":fileName", (attributes.fileName.isSet() ? attributes.fileName.ref() : nullValue));
        query.bindValue(":attachment", (attributes.attachment.isSet() ? (attributes.attachment.ref() ? 1 : 0) : nullValue));

        res = query.exec();
        DATABASE_CHECK_AND_SET_ERROR("can't insert or replace data into \"ResourceAttributes\" table in SQL database");
    }

    // Special treatment for ResourceAttributes.applicationData: keysOnly + fullMap

    if (attributes.applicationData.isSet())
    {
        if (attributes.applicationData->keysOnly.isSet())
        {
            bool res = CheckAndPrepareInsertOrReplaceResourceAttributesApplicationDataKeysOnlyQuery();
            QSqlQuery & query = m_insertOrReplaceResourceAttributeApplicationDataKeysOnlyQuery;
            DATABASE_CHECK_AND_SET_ERROR("can't insert or replace data into \"ResourceAttributesApplicationDataKeysOnly\" "
                                         "table in SQL database: can't prepare SQL query");

            query.bindValue(":resourceLocalGuid", localGuid);

            const QSet<QString> & keysOnly = attributes.applicationData->keysOnly.ref();
            foreach(const QString & key, keysOnly) {
                query.bindValue(":resourceKey", key);
                res = query.exec();
                DATABASE_CHECK_AND_SET_ERROR("can't insert or replace data into \"ResourceAttributesApplicationDataKeysOnly\" table in SQL database");
            }
        }

        if (attributes.applicationData->fullMap.isSet())
        {
            bool res = CheckAndPrepareInsertOrReplaceResourceAttributesApplicationDataFullMapQuery();
            QSqlQuery & query = m_insertOrReplaceResourceAttributeApplicationDataFullMapQuery;
            DATABASE_CHECK_AND_SET_ERROR("can't insert or replace data into \"ResourceAttributesApplicationDataFullMap\" "
                                         "table in SQL database: can't prepare SQL query");

            query.bindValue(":resourceLocalGuid", localGuid);

            const QMap<QString, QString> & fullMap = attributes.applicationData->fullMap.ref();
            foreach(const QString & key, fullMap.keys()) {
                query.bindValue(":resourceMapKey", key);
                query.bindValue(":resourceValue", fullMap.value(key));
                res = query.exec();
                DATABASE_CHECK_AND_SET_ERROR("can't insert or replace data into \"ResourceAttributesApplicationDataFullMap\" table in SQL database");
            }
        }
    }

    return true;
}

bool LocalStorageManagerPrivate::CheckAndPrepareInsertOrReplaceResourceQuery()
{
    if (!m_insertOrReplaceResourceQueryPrepared)
    {
        QNDEBUG("Preparing SQL query to insert or replace the resource");

        m_insertOrReplaceResourceQuery = QSqlQuery(m_sqlDatabase);
        bool res = m_insertOrReplaceResourceQuery.prepare("INSERT OR REPLACE INTO Resources (resourceGuid, "
                                                          "noteGuid, dataBody, dataSize, dataHash, mime, "
                                                          "width, height, recognitionDataBody, recognitionDataSize, "
                                                          "recognitionDataHash, alternateDataBody, alternateDataSize, "
                                                          "alternateDataHash, resourceUpdateSequenceNumber, "
                                                          "resourceIsDirty, resourceIndexInNote, resourceLocalGuid) "
                                                          "VALUES(:resourceGuid, :noteGuid, :dataBody, :dataSize, :dataHash, :mime, "
                                                          ":width, :height, :recognitionDataBody, :recognitionDataSize, "
                                                          ":recognitionDataHash, :alternateDataBody, :alternateDataSize, "
                                                          ":alternateDataHash, :resourceUpdateSequenceNumber, :resourceIsDirty, "
                                                          ":resourceIndexInNote, :resourceLocalGuid)");
        if (res) {
            m_insertOrReplaceResourceQueryPrepared = true;
        }

        return res;
    }
    else
    {
        return true;
    }
}

bool LocalStorageManagerPrivate::CheckAndPrepareInsertOrReplaceNoteResourceQuery()
{
    if (!m_insertOrReplaceNoteResourceQueryPrepared)
    {
        QNDEBUG("Preparing SQL query to insert or replace resource into NoteResources table");

        m_insertOrReplaceNoteResourceQuery = QSqlQuery(m_sqlDatabase);
        bool res = m_insertOrReplaceNoteResourceQuery.prepare("INSERT OR REPLACE INTO NoteResources "
                                                              "(localNote, note, localResource, resource) "
                                                              "VALUES(:localNote, :note, :localResource, :resource)");
        if (res) {
            m_insertOrReplaceNoteResourceQueryPrepared = true;
        }

        return res;
    }
    else
    {
        return true;
    }

}

bool LocalStorageManagerPrivate::CheckAndPrepareDeleteResourceFromResourceRecognitionTypesQuery()
{
    if (!m_deleteResourceFromResourceRecognitionTypesQueryPrepared)
    {
        QNDEBUG("Preparing SQL query to delete resource from ResourceRecognitionTypes table");

        m_deleteResourceFromResourceRecognitionTypesQuery = QSqlQuery(m_sqlDatabase);
        bool res = m_deleteResourceFromResourceRecognitionTypesQuery.prepare("DELETE FROM ResourceRecognitionTypes "
                                                                             "WHERE resourceLocalGuid = :resourceLocalGuid");
        if (res) {
            m_deleteResourceFromResourceRecognitionTypesQueryPrepared = true;
        }

        return res;
    }
    else
    {
        return true;
    }
}

bool LocalStorageManagerPrivate::CheckAndPrepareInsertOrReplaceIntoResourceRecognitionTypesQuery()
{
    if (!m_insertOrReplaceIntoResourceRecognitionTypesQueryPrepared)
    {
        QNDEBUG("Preparing SQL query to insert or replace resource into ResourceRecognitionTypes table");

        m_insertOrReplaceIntoResourceRecognitionTypesQuery = QSqlQuery(m_sqlDatabase);
        bool res = m_insertOrReplaceIntoResourceRecognitionTypesQuery.prepare("INSERT OR REPLACE INTO ResourceRecognitionTypes"
                                                                              "(resourceLocalGuid, recognitionType) "
                                                                              "VALUES(:resourceLocalGuid, :recognitionType)");

        if (res) {
            m_insertOrReplaceIntoResourceRecognitionTypesQueryPrepared = true;
        }

        return res;
    }
    else
    {
        return true;
    }
}

bool LocalStorageManagerPrivate::CheckAndPrepareDeleteResourceFromResourceAttributesQuery()
{
    if (!m_deleteResourceFromResourceAttributesQueryPrepared)
    {
        QNDEBUG("Preparing SQL query to delete resource from ResourceAttributes table");

        m_deleteResourceFromResourceAttributesQuery = QSqlQuery(m_sqlDatabase);
        bool res = m_deleteResourceFromResourceAttributesQuery.prepare("DELETE FROM ResourceAttributes WHERE resourceLocalGuid = :resourceLocalGuid");

        if (res) {
            m_deleteResourceFromResourceAttributesQueryPrepared = true;
        }

        return res;
    }
    else
    {
        return true;
    }
}

bool LocalStorageManagerPrivate::CheckAndPrepareDeleteResourceFromResourceAttributesApplicationDataKeysOnlyQuery()
{
    if (!m_deleteResourceFromResourceAttributesApplicationDataKeysOnlyQueryPrepared)
    {
        QNDEBUG("Preparing SQL query to delete Resource from ResourceAttributesApplicationDataKeysOnly table");

        m_deleteResourceFromResourceAttributesApplicationDataKeysOnlyQuery = QSqlQuery(m_sqlDatabase);
        bool res = m_deleteResourceFromResourceAttributesApplicationDataKeysOnlyQuery.prepare("DELETE FROM ResourceAttributesApplicationDataKeysOnly "
                                                                                              "WHERE resourceLocalGuid = :resourceLocalGuid");
        if (res) {
            m_deleteResourceFromResourceAttributesApplicationDataKeysOnlyQueryPrepared = true;
        }

        return res;
    }
    else
    {
        return true;
    }
}

bool LocalStorageManagerPrivate::CheckAndPrepareDeleteResourceFromResourceAttributesApplicationDataFullMapQuery()
{
    if (!m_deleteResourceFromResourceAttributesApplicationDataFullMapQueryPrepared)
    {
        QNDEBUG("Preparing SQL query to delete Resource from ResourceAttributesApplicationDataFullMap table");

        m_deleteResourceFromResourceAttributesApplicationDataFullMapQuery = QSqlQuery(m_sqlDatabase);
        bool res = m_deleteResourceFromResourceAttributesApplicationDataFullMapQuery.prepare("DELETE FROM ResourceAttributesApplicationDataFullMap "
                                                                                             "WHERE resourceLocalGuid = :resourceLocalGuid");
        if (res) {
            m_deleteResourceFromResourceAttributesApplicationDataFullMapQueryPrepared = true;
        }

        return res;
    }
    else
    {
        return true;
    }
}

bool LocalStorageManagerPrivate::CheckAndPrepareInsertOrReplaceResourceAttributesQuery()
{
    if (!m_insertOrReplaceResourceAttributesQueryPrepared)
    {
        QNDEBUG("Preparing SQL query to insert or replace ResourceAttributes");

        m_insertOrReplaceResourceAttributesQuery = QSqlQuery(m_sqlDatabase);
        bool res = m_insertOrReplaceResourceAttributesQuery.prepare("INSERT OR REPLACE INTO ResourceAttributes"
                                                                    "(resourceLocalGuid, resourceSourceURL, timestamp, resourceLatitude, "
                                                                    "resourceLongitude, resourceAltitude, cameraMake, cameraModel, "
                                                                    "clientWillIndex, fileName, attachment) "
                                                                    "VALUES(:resourceLocalGuid, :resourceSourceURL, :timestamp, :resourceLatitude, "
                                                                    ":resourceLongitude, :resourceAltitude, :cameraMake, :cameraModel, "
                                                                    ":clientWillIndex, :fileName, :attachment)");
        if (res) {
            m_insertOrReplaceResourceAttributesQueryPrepared = true;
        }

        return res;
    }
    else
    {
        return true;
    }
}

bool LocalStorageManagerPrivate::CheckAndPrepareInsertOrReplaceResourceAttributesApplicationDataKeysOnlyQuery()
{
    if (!m_insertOrReplaceResourceAttributeApplicationDataKeysOnlyQueryPrepared)
    {
        QNDEBUG("Preparing SQL query to insert or replace resource attribute application data (keys only)");

        m_insertOrReplaceResourceAttributeApplicationDataKeysOnlyQuery = QSqlQuery(m_sqlDatabase);
        bool res = m_insertOrReplaceResourceAttributeApplicationDataKeysOnlyQuery.prepare("INSERT OR REPLACE INTO ResourceAttributesApplicationDataKeysOnly"
                                                                                          "(resourceLocalGuid, resourceKey) VALUES(:resourceLocalGuid, :resourceKey)");
        if (res) {
            m_insertOrReplaceResourceAttributeApplicationDataKeysOnlyQueryPrepared = true;
        }

        return res;
    }
    else
    {
        return true;
    }
}

bool LocalStorageManagerPrivate::CheckAndPrepareInsertOrReplaceResourceAttributesApplicationDataFullMapQuery()
{
    if (!m_insertOrReplaceResourceAttributeApplicationDataFullMapQueryPrepared)
    {
        QNDEBUG("Preparing SQL query to insert or replace resource attributes application data (full map)");

        m_insertOrReplaceResourceAttributeApplicationDataFullMapQuery = QSqlQuery(m_sqlDatabase);
        bool res = m_insertOrReplaceResourceAttributeApplicationDataFullMapQuery.prepare("INSERT OR REPLACE INTO ResourceAttributesApplicationDataFullMap"
                                                                                         "(resourceLocalGuid, resourceMapKey, resourceValue) "
                                                                                         "VALUES(:resourceLocalGuid, :resourceMapKey, :resourceValue)");
        if (res) {
            m_insertOrReplaceResourceAttributeApplicationDataFullMapQueryPrepared = true;
        }

        return res;
    }
    else
    {
        return true;
    }
}

bool LocalStorageManagerPrivate::CheckAndPrepareGetResourceCountQuery() const
{
    if (!m_getResourceCountQueryPrepared)
    {
        QNDEBUG("Preparing SQL query to get the count of Resources");

        m_getResourceCountQuery = QSqlQuery(m_sqlDatabase);
        bool res = m_getResourceCountQuery.prepare("SELECT COUNT(*) FROM Resources");

        if (res) {
            m_getResourceCountQueryPrepared = res;
        }

        return res;
    }
    else
    {
        return true;
    }
}

bool LocalStorageManagerPrivate::InsertOrReplaceSavedSearch(const SavedSearch & search,
                                                            const QString & overrideLocalGuid,
                                                            QString & errorDescription)
{
    // NOTE: this method expects to be called after the search is already checked
    // for sanity of its parameters!

    errorDescription = QT_TR_NOOP("Can't insert or replace saved search into local storage database: ");

    bool res = CheckAndPrepareInsertOrReplaceSavedSearchQuery();
    if (!res) {
        errorDescription += QT_TR_NOOP("failed to prepare SQL query: ");
        QNWARNING(errorDescription << m_insertOrReplaceSavedSearchQuery.lastError());
        errorDescription += m_insertOrReplaceSavedSearchQuery.lastError().text();
        return false;
    }

    QSqlQuery & query = m_insertOrReplaceSavedSearchQuery;
    QVariant nullValue;

    query.bindValue(":localGuid", (overrideLocalGuid.isEmpty()
                                   ? search.localGuid()
                                   : overrideLocalGuid));
    query.bindValue(":guid", (search.hasGuid() ? search.guid() : nullValue));
    query.bindValue(":name", (search.hasName() ? search.name() : nullValue));
    query.bindValue(":nameUpper", (search.hasName() ? search.name().toUpper() : nullValue));

    query.bindValue(":query", (search.hasQuery() ? search.query() : nullValue));
    query.bindValue(":format", (search.hasQueryFormat() ? search.queryFormat() : nullValue));
    query.bindValue(":updateSequenceNumber", (search.hasUpdateSequenceNumber()
                                              ? search.updateSequenceNumber()
                                              : nullValue));
    query.bindValue(":isDirty", (search.isDirty() ? 1 : 0));
    query.bindValue(":isLocal", (search.isLocal() ? 1 : 0));
    query.bindValue(":includeAccount", (search.hasIncludeAccount()
                                        ? (search.includeAccount() ? 1 : 0)
                                        : nullValue));
    query.bindValue(":includePersonalLinkedNotebooks", (search.hasIncludePersonalLinkedNotebooks()
                                                        ? (search.includePersonalLinkedNotebooks() ? 1 : 0)
                                                        : nullValue));
    query.bindValue(":includeBusinessLinkedNotebooks", (search.hasIncludeBusinessLinkedNotebooks()
                                                        ? (search.includeBusinessLinkedNotebooks() ? 1 : 0)
                                                        : nullValue));
    query.bindValue(":hasShortcut", (search.hasShortcut() ? 1 : 0));

    res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("can't insert or replace saved search into \"SavedSearches\" table in SQL database");

    return true;
}

bool LocalStorageManagerPrivate::CheckAndPrepareInsertOrReplaceSavedSearchQuery()
{
    if (!m_insertOrReplaceSavedSearchQueryPrepared)
    {
        QNDEBUG("Preparing SQL query to insert or replace SavedSearch");

        QString columns = "localGuid, guid, name, nameUpper, query, format, updateSequenceNumber, isDirty, "
                          "isLocal, includeAccount, includePersonalLinkedNotebooks, "
                          "includeBusinessLinkedNotebooks, hasShortcut";

        QString valuesNames = ":localGuid, :guid, :name, :nameUpper, :query, :format, :updateSequenceNumber, :isDirty, "
                              ":isLocal, :includeAccount, :includePersonalLinkedNotebooks, "
                              ":includeBusinessLinkedNotebooks, :hasShortcut";

        QString queryString = QString("INSERT OR REPLACE INTO SavedSearches (%1) VALUES(%2)").arg(columns).arg(valuesNames);

        m_insertOrReplaceSavedSearchQuery = QSqlQuery(m_sqlDatabase);
        bool res = m_insertOrReplaceSavedSearchQuery.prepare(queryString);

        if (res) {
            m_insertOrReplaceSavedSearchQueryPrepared = true;
        }

        return res;
    }
    else {
        return true;
    }
}

bool LocalStorageManagerPrivate::CheckAndPrepareGetSavedSearchCountQuery() const
{
    if (!m_getSavedSearchCountQueryPrepared)
    {
        QNDEBUG("Preparing SQL query to get the count of SavedSearches");

        m_getSavedSearchCountQuery = QSqlQuery(m_sqlDatabase);
        bool res = m_getSavedSearchCountQuery.prepare("SELECT COUNT(*) FROM SavedSearches");

        if (res) {
            m_getSavedSearchCountQueryPrepared = true;
        }

        return res;
    }
    else
    {
        return true;
    }
}

bool LocalStorageManagerPrivate::CheckAndPrepareExpungeSavedSearchQuery()
{
    if (!m_expungeSavedSearchQueryPrepared)
    {
        QNDEBUG("Preparing SQL query to expunge SavedSearch");

        m_expungeSavedSearchQuery = QSqlQuery(m_sqlDatabase);
        bool res = m_expungeSavedSearchQuery.prepare("DELETE FROM SavedSearches WHERE localGuid = :localGuid");

        if (res) {
            m_expungeSavedSearchQueryPrepared = res;
        }

        return res;
    }
    else
    {
        return true;
    }
}

void LocalStorageManagerPrivate::FillResourceFromSqlRecord(const QSqlRecord & rec, const bool withBinaryData,
                                                           IResource & resource) const
{
#define CHECK_AND_SET_RESOURCE_PROPERTY(property, type, localType, setter) \
    { \
        int index = rec.indexOf(#property); \
        if (index >= 0) { \
            QVariant value = rec.value(index); \
            if (!value.isNull()) { \
                resource.setter(static_cast<localType>(qvariant_cast<type>(value))); \
            } \
        } \
    }

    CHECK_AND_SET_RESOURCE_PROPERTY(resourceLocalGuid, QString, QString, setLocalGuid);
    CHECK_AND_SET_RESOURCE_PROPERTY(resourceIsDirty, int, bool, setDirty);
    CHECK_AND_SET_RESOURCE_PROPERTY(noteGuid, QString, QString, setNoteGuid);
    CHECK_AND_SET_RESOURCE_PROPERTY(localNote, QString, QString, setNoteLocalGuid);
    CHECK_AND_SET_RESOURCE_PROPERTY(resourceUpdateSequenceNumber, int, qint32, setUpdateSequenceNumber);
    CHECK_AND_SET_RESOURCE_PROPERTY(dataSize, int, qint32, setDataSize);
    CHECK_AND_SET_RESOURCE_PROPERTY(dataHash, QByteArray, QByteArray, setDataHash);
    CHECK_AND_SET_RESOURCE_PROPERTY(mime, QString, QString, setMime);

    CHECK_AND_SET_RESOURCE_PROPERTY(resourceGuid, QString, QString, setGuid);
    CHECK_AND_SET_RESOURCE_PROPERTY(width, int, qint32, setWidth);
    CHECK_AND_SET_RESOURCE_PROPERTY(height, int, qint32, setHeight);
    CHECK_AND_SET_RESOURCE_PROPERTY(recognitionDataSize, int, qint32, setRecognitionDataSize);
    CHECK_AND_SET_RESOURCE_PROPERTY(recognitionDataHash, QByteArray, QByteArray, setRecognitionDataHash);
    CHECK_AND_SET_RESOURCE_PROPERTY(resourceIndexInNote, int, int, setIndexInNote);
    CHECK_AND_SET_RESOURCE_PROPERTY(alternateDataSize, int, qint32, setAlternateDataSize);
    CHECK_AND_SET_RESOURCE_PROPERTY(alternateDataHash, QByteArray, QByteArray, setAlternateDataHash);

    if (withBinaryData) {
        CHECK_AND_SET_RESOURCE_PROPERTY(recognitionDataBody, QByteArray, QByteArray, setRecognitionDataBody);
        CHECK_AND_SET_RESOURCE_PROPERTY(dataBody, QByteArray, QByteArray, setDataBody);
        CHECK_AND_SET_RESOURCE_PROPERTY(alternateDataBody, QByteArray, QByteArray, setAlternateDataBody);
    }

    qevercloud::ResourceAttributes localAttributes;
    qevercloud::ResourceAttributes & attributes = (resource.hasResourceAttributes()
                                                   ? resource.resourceAttributes()
                                                   : localAttributes);

    bool hasAttributes = FillResourceAttributesFromSqlRecord(rec, attributes);
    hasAttributes |= FillResourceAttributesApplicationDataKeysOnlyFromSqlRecord(rec, attributes);
    hasAttributes |= FillResourceAttributesApplicationDataFullMapFromSqlRecord(rec, attributes);

    if (hasAttributes && !resource.hasResourceAttributes()) {
        resource.setResourceAttributes(attributes);
    }
}

bool LocalStorageManagerPrivate::FillResourceAttributesFromSqlRecord(const QSqlRecord & rec, qevercloud::ResourceAttributes & attributes) const
{
    bool hasSomething = false;

#define CHECK_AND_SET_RESOURCE_ATTRIBUTE(name, property, type, localType) \
    { \
        int index = rec.indexOf(#name); \
        if (index >= 0) { \
            QVariant value = rec.value(index); \
            if (!value.isNull()) { \
                attributes.property = static_cast<localType>(qvariant_cast<type>(value)); \
                hasSomething = true; \
            } \
        } \
    }

    CHECK_AND_SET_RESOURCE_ATTRIBUTE(resourceSourceURL, sourceURL, QString, QString);
    CHECK_AND_SET_RESOURCE_ATTRIBUTE(timestamp, timestamp, qint64, qevercloud::Timestamp);
    CHECK_AND_SET_RESOURCE_ATTRIBUTE(resourceLatitude, latitude, double, double);
    CHECK_AND_SET_RESOURCE_ATTRIBUTE(resourceLongitude, longitude, double, double);
    CHECK_AND_SET_RESOURCE_ATTRIBUTE(resourceAltitude, altitude, double, double);
    CHECK_AND_SET_RESOURCE_ATTRIBUTE(cameraMake, cameraMake, QString, QString);
    CHECK_AND_SET_RESOURCE_ATTRIBUTE(cameraModel, cameraModel, QString, QString);
    CHECK_AND_SET_RESOURCE_ATTRIBUTE(clientWillIndex, clientWillIndex, int, bool);
    CHECK_AND_SET_RESOURCE_ATTRIBUTE(fileName, fileName, QString, QString);
    CHECK_AND_SET_RESOURCE_ATTRIBUTE(attachment, attachment, int, bool);

#undef CHECK_AND_SET_RESOURCE_ATTRIBUTE

    return hasSomething;
}

bool LocalStorageManagerPrivate::FillResourceAttributesApplicationDataKeysOnlyFromSqlRecord(const QSqlRecord & rec,
                                                                                     qevercloud::ResourceAttributes & attributes) const
{
    bool hasSomething = false;

    int index = rec.indexOf("resourceKey");
    if (index >= 0) {
        QVariant value = rec.value(index);
        if (!value.isNull()) {
            if (!attributes.applicationData.isSet()) {
                attributes.applicationData = qevercloud::LazyMap();
            }
            if (!attributes.applicationData->keysOnly.isSet()) {
                attributes.applicationData->keysOnly = QSet<QString>();
            }
            attributes.applicationData->keysOnly.ref().insert(value.toString());
            hasSomething = true;
        }
    }

    return hasSomething;
}

bool LocalStorageManagerPrivate::FillResourceAttributesApplicationDataFullMapFromSqlRecord(const QSqlRecord & rec,
                                                                                    qevercloud::ResourceAttributes & attributes) const
{
    bool hasSomething = false;

    int keyIndex = rec.indexOf("resourceMapKey");
    int valueIndex = rec.indexOf("resourceValue");
    if ((keyIndex >= 0) && (valueIndex >= 0)) {
        QVariant key = rec.value(keyIndex);
        QVariant value = rec.value(valueIndex);
        if (!key.isNull() && !value.isNull()) {
            if (!attributes.applicationData.isSet()) {
                attributes.applicationData = qevercloud::LazyMap();
            }
            if (!attributes.applicationData->fullMap.isSet()) {
                attributes.applicationData->fullMap = QMap<QString, QString>();
            }
            attributes.applicationData->fullMap.ref().insert(key.toString(), value.toString());
            hasSomething = true;
        }
    }

    return hasSomething;
}

void LocalStorageManagerPrivate::FillNoteAttributesFromSqlRecord(const QSqlRecord & rec, qevercloud::NoteAttributes & attributes) const
{
#define CHECK_AND_SET_NOTE_ATTRIBUTE(property, type, localType) \
    { \
        int index = rec.indexOf(#property); \
        if (index >= 0) { \
            QVariant value = rec.value(index); \
            if (!value.isNull()) { \
                attributes.property = static_cast<localType>(qvariant_cast<type>(value)); \
            } \
        } \
    }

    CHECK_AND_SET_NOTE_ATTRIBUTE(subjectDate, qint64, qevercloud::Timestamp);
    CHECK_AND_SET_NOTE_ATTRIBUTE(latitude, double, double);
    CHECK_AND_SET_NOTE_ATTRIBUTE(longitude, double, double);
    CHECK_AND_SET_NOTE_ATTRIBUTE(altitude, double, double);
    CHECK_AND_SET_NOTE_ATTRIBUTE(author, QString, QString);
    CHECK_AND_SET_NOTE_ATTRIBUTE(source, QString, QString);
    CHECK_AND_SET_NOTE_ATTRIBUTE(sourceURL, QString, QString);
    CHECK_AND_SET_NOTE_ATTRIBUTE(sourceApplication, QString, QString);
    CHECK_AND_SET_NOTE_ATTRIBUTE(shareDate, qint64, qevercloud::Timestamp);
    CHECK_AND_SET_NOTE_ATTRIBUTE(reminderOrder, qint64, qint64);
    CHECK_AND_SET_NOTE_ATTRIBUTE(reminderDoneTime, qint64, qevercloud::Timestamp);
    CHECK_AND_SET_NOTE_ATTRIBUTE(reminderTime, qint64, qevercloud::Timestamp);
    CHECK_AND_SET_NOTE_ATTRIBUTE(placeName, QString, QString);
    CHECK_AND_SET_NOTE_ATTRIBUTE(contentClass, QString, QString);
    CHECK_AND_SET_NOTE_ATTRIBUTE(lastEditedBy, QString, QString);
    CHECK_AND_SET_NOTE_ATTRIBUTE(creatorId, qint32, UserID);
    CHECK_AND_SET_NOTE_ATTRIBUTE(lastEditorId, qint32, UserID);

#undef CHECK_AND_SET_NOTE_ATTRIBUTE
}

void LocalStorageManagerPrivate::FillNoteAttributesApplicationDataKeysOnlyFromSqlRecord(const QSqlRecord & rec,
                                                                                        qevercloud::NoteAttributes & attributes) const
{
    int keysOnlyIndex = rec.indexOf("applicationDataKeysOnly");
    if (keysOnlyIndex < 0) {
        return;
    }

    QVariant value = rec.value(keysOnlyIndex);
    if (value.isNull()) {
        return;
    }

    bool applicationDataWasEmpty = !attributes.applicationData.isSet();
    if (applicationDataWasEmpty) {
        attributes.applicationData = qevercloud::LazyMap();
    }

    if (!attributes.applicationData->keysOnly.isSet()) {
        attributes.applicationData->keysOnly = QSet<QString>();
    }

    QSet<QString> & keysOnly = attributes.applicationData->keysOnly.ref();

    QString keysOnlyString = value.toString();
    int length = keysOnlyString.length();
    bool insideQuotedText = false;
    QString currentKey;
    QChar wordSep('\'');
    for(int i = 0; i < (length - 1); ++i)
    {
        const QChar currentChar = keysOnlyString.at(i);
        const QChar nextChar = keysOnlyString.at(i+1);

        if (currentChar == wordSep)
        {
            insideQuotedText = !insideQuotedText;

            if (nextChar == wordSep) {
                keysOnly.insert(currentKey);
                currentKey.resize(0);
            }
        }
        else if (insideQuotedText)
        {
            currentKey.append(currentChar);
        }
    }

    if (!currentKey.isEmpty()) {
        keysOnly.insert(currentKey);
    }

    if (keysOnly.isEmpty())
    {
        if (applicationDataWasEmpty) {
            attributes.applicationData.clear();
        }
        else {
            attributes.applicationData->keysOnly.clear();
        }
    }
}

void LocalStorageManagerPrivate::FillNoteAttributesApplicationDataFullMapFromSqlRecord(const QSqlRecord & rec,
                                                                                       qevercloud::NoteAttributes & attributes) const
{
    int keyIndex = rec.indexOf("applicationDataKeysMap");
    int valueIndex = rec.indexOf("applicationDataValues");

    if ((keyIndex < 0) || (valueIndex < 0)) {
        return;
    }

    QVariant keys = rec.value(keyIndex);
    QVariant values = rec.value(valueIndex);

    if (keys.isNull() || values.isNull()) {
        return;
    }

    bool applicationDataWasEmpty = !attributes.applicationData.isSet();
    if (applicationDataWasEmpty) {
        attributes.applicationData = qevercloud::LazyMap();
    }

    if (!attributes.applicationData->fullMap.isSet()) {
        attributes.applicationData->fullMap = QMap<QString, QString>();
    }

    QMap<QString, QString> & fullMap = attributes.applicationData->fullMap.ref();
    QStringList keysList, valuesList;

    QString keysString = keys.toString();
    int keysLength = keysString.length();
    bool insideQuotedText = false;
    QString currentKey;
    QChar wordSep('\'');
    for(int i = 0; i < (keysLength - 1); ++i)
    {
        const QChar currentChar = keysString.at(i);
        const QChar nextChar = keysString.at(i+1);

        if (currentChar == wordSep)
        {
            insideQuotedText = !insideQuotedText;

            if (nextChar == wordSep) {
                keysList << currentKey;
                currentKey.resize(0);
            }
        }
        else if (insideQuotedText)
        {
            currentKey.append(currentChar);
        }
    }

    if (!currentKey.isEmpty()) {
        keysList << currentKey;
    }

    QString valuesString = values.toString();
    int valuesLength = valuesString.length();
    insideQuotedText = false;
    QString currentValue;
    for(int i = 0; i < (valuesLength - 1); ++i)
    {
        const QChar currentChar = valuesString.at(i);
        const QChar nextChar = valuesString.at(i+1);

        if (currentChar == wordSep)
        {
            insideQuotedText = !insideQuotedText;

            if (nextChar == wordSep) {
                valuesList << currentValue;
                currentValue.resize(0);
            }
        }
        else if (insideQuotedText)
        {
            currentValue.append(currentChar);
        }
    }

    if (!currentValue.isEmpty()) {
        valuesList << currentValue;
    }

    int numKeys = keysList.size();
    for(int i = 0; i < numKeys; ++i) {
        fullMap.insert(keysList.at(i), valuesList.at(i));
    }

    if (fullMap.isEmpty())
    {
        if (applicationDataWasEmpty) {
            attributes.applicationData.clear();
        }
        else {
            attributes.applicationData->fullMap.clear();
        }
    }
}

void LocalStorageManagerPrivate::FillNoteAttributesClassificationsFromSqlRecord(const QSqlRecord & rec,
                                                                                qevercloud::NoteAttributes & attributes) const
{
    int keyIndex = rec.indexOf("classificationKeys");
    int valueIndex = rec.indexOf("classificationValues");

    if ((keyIndex < 0) || (valueIndex < 0)) {
        return;
    }

    QVariant keys = rec.value(keyIndex);
    QVariant values = rec.value(valueIndex);

    if (keys.isNull() || values.isNull()) {
        return;
    }

    bool classificationsWereEmpty = !attributes.classifications.isSet();
    if (classificationsWereEmpty) {
        attributes.classifications = QMap<QString, QString>();
    }

    QMap<QString, QString> & classifications = attributes.classifications.ref();
    QStringList keysList, valuesList;

    QString keysString = keys.toString();
    int keysLength = keysString.length();
    bool insideQuotedText = false;
    QString currentKey;
    QChar wordSep('\'');
    for(int i = 0; i < (keysLength - 1); ++i)
    {
        const QChar currentChar = keysString.at(i);
        const QChar nextChar = keysString.at(i+1);

        if (currentChar == wordSep)
        {
            insideQuotedText = !insideQuotedText;

            if (nextChar == wordSep) {
                keysList << currentKey;
                currentKey.resize(0);
            }
        }
        else if (insideQuotedText)
        {
            currentKey.append(currentChar);
        }
    }

    QString valuesString = values.toString();
    int valuesLength = valuesString.length();
    insideQuotedText = false;
    QString currentValue;
    for(int i = 0; i < (valuesLength - 1); ++i)
    {
        const QChar currentChar = valuesString.at(i);
        const QChar nextChar = valuesString.at(i+1);

        if (currentChar == wordSep)
        {
            insideQuotedText = !insideQuotedText;

            if (nextChar == wordSep) {
                valuesList << currentValue;
                currentValue.resize(0);
            }
        }
        else if (insideQuotedText)
        {
            currentValue.append(currentChar);
        }
    }

    int numKeys = keysList.size();
    for(int i = 0; i < numKeys; ++i) {
        classifications[keysList.at(i)] = valuesList.at(i);
    }

    if (classifications.isEmpty() && classificationsWereEmpty) {
        attributes.classifications.clear();
    }
}

bool LocalStorageManagerPrivate::FillUserFromSqlRecord(const QSqlRecord & rec, IUser & user, QString & errorDescription) const
{
#define FIND_AND_SET_USER_PROPERTY(column, setter, type, localType, isRequired) \
    { \
        bool valueFound = false; \
        int index = rec.indexOf(#column); \
        if (index >= 0) { \
            QVariant value = rec.value(#column); \
            if (!value.isNull()) { \
                user.setter(static_cast<localType>(qvariant_cast<type>(value))); \
                valueFound = true; \
            } \
        } \
        \
        if (!valueFound && isRequired) { \
            errorDescription += QT_TR_NOOP("Internal error no " #column " field in the result of SQL query"); \
            QNCRITICAL(errorDescription); \
            return false; \
        } \
    }

    bool isRequired = true;

    FIND_AND_SET_USER_PROPERTY(userIsDirty, setDirty, int, bool, isRequired);
    FIND_AND_SET_USER_PROPERTY(userIsLocal, setLocal, int, bool, isRequired);
    FIND_AND_SET_USER_PROPERTY(username, setUsername, QString, QString, !isRequired);
    FIND_AND_SET_USER_PROPERTY(email, setEmail, QString, QString, !isRequired);
    FIND_AND_SET_USER_PROPERTY(name, setName, QString, QString, !isRequired);
    FIND_AND_SET_USER_PROPERTY(timezone, setTimezone, QString, QString, !isRequired);
    FIND_AND_SET_USER_PROPERTY(privilege, setPrivilegeLevel, int, qint8, !isRequired);
    FIND_AND_SET_USER_PROPERTY(userCreationTimestamp, setCreationTimestamp,
                               qint64, qint64, !isRequired);
    FIND_AND_SET_USER_PROPERTY(userModificationTimestamp, setModificationTimestamp,
                               qint64, qint64, !isRequired);
    FIND_AND_SET_USER_PROPERTY(userDeletionTimestamp, setDeletionTimestamp,
                               qint64, qint64, !isRequired);
    FIND_AND_SET_USER_PROPERTY(userIsActive, setActive, int, bool, !isRequired);

#undef FIND_AND_SET_USER_PROPERTY

    bool foundSomeUserAttribute = false;
    qevercloud::UserAttributes attributes;
    if (user.hasUserAttributes()) {
        const qevercloud::UserAttributes & userAttributes = user.userAttributes();
        attributes.viewedPromotions = userAttributes.viewedPromotions;
        attributes.recentMailedAddresses = userAttributes.recentMailedAddresses;
    }

    int promotionIndex = rec.indexOf("promotion");
    if (promotionIndex >= 0) {
        QVariant value = rec.value(promotionIndex);
        if (!value.isNull()) {
            if (!attributes.viewedPromotions.isSet()) {
                attributes.viewedPromotions = QStringList();
            }
            QString valueString = value.toString();
            // FIXME: it is workaround but not the reliable solution, need to find a way to fix it
            if (!attributes.viewedPromotions.ref().contains(valueString)) {
                attributes.viewedPromotions.ref() << valueString;
            }
            foundSomeUserAttribute = true;
        }
    }

    int addressIndex = rec.indexOf("address");
    if (addressIndex >= 0) {
        QVariant value = rec.value(addressIndex);
        if (!value.isNull()) {
            if (!attributes.recentMailedAddresses.isSet()) {
                attributes.recentMailedAddresses = QStringList();
            }
            QString valueString = value.toString();
            // FIXME: it is workaround but not the reliable solution, need to find a way to fix it
            if (!attributes.recentMailedAddresses.ref().contains(valueString)) {
                attributes.recentMailedAddresses.ref() << valueString;
            }
            foundSomeUserAttribute = true;
        }
    }

#define FIND_AND_SET_USER_ATTRIBUTE(column, property, type, localType) \
    { \
        int index = rec.indexOf(#column); \
        if (index >= 0) { \
            QVariant value = rec.value(#column); \
            if (!value.isNull()) { \
                attributes.property = static_cast<localType>(qvariant_cast<type>(value)); \
                foundSomeUserAttribute = true; \
            } \
        } \
    }

    FIND_AND_SET_USER_ATTRIBUTE(defaultLocationName, defaultLocationName, QString, QString);
    FIND_AND_SET_USER_ATTRIBUTE(defaultLatitude, defaultLatitude, double, double);
    FIND_AND_SET_USER_ATTRIBUTE(defaultLongitude, defaultLongitude, double, double);
    FIND_AND_SET_USER_ATTRIBUTE(preactivation, preactivation, int, bool);
    FIND_AND_SET_USER_ATTRIBUTE(incomingEmailAddress, incomingEmailAddress, QString, QString);
    FIND_AND_SET_USER_ATTRIBUTE(comments, comments, QString, QString);
    FIND_AND_SET_USER_ATTRIBUTE(dateAgreedToTermsOfService, dateAgreedToTermsOfService, qint64, qevercloud::Timestamp);
    FIND_AND_SET_USER_ATTRIBUTE(maxReferrals, maxReferrals, qint32, qint32);
    FIND_AND_SET_USER_ATTRIBUTE(referralCount, referralCount, qint32, qint32);
    FIND_AND_SET_USER_ATTRIBUTE(refererCode, refererCode, QString, QString);
    FIND_AND_SET_USER_ATTRIBUTE(sentEmailDate, sentEmailDate, qint64, qevercloud::Timestamp);
    FIND_AND_SET_USER_ATTRIBUTE(sentEmailCount, sentEmailCount, qint32, qint32);
    FIND_AND_SET_USER_ATTRIBUTE(dailyEmailLimit, dailyEmailLimit, qint32, qint32);
    FIND_AND_SET_USER_ATTRIBUTE(emailOptOutDate, emailOptOutDate, qint64, qevercloud::Timestamp);
    FIND_AND_SET_USER_ATTRIBUTE(partnerEmailOptInDate, partnerEmailOptInDate, qint64, qevercloud::Timestamp);
    FIND_AND_SET_USER_ATTRIBUTE(preferredLanguage, preferredLanguage, QString, QString);
    FIND_AND_SET_USER_ATTRIBUTE(preferredCountry, preferredCountry, QString, QString);
    FIND_AND_SET_USER_ATTRIBUTE(clipFullPage, clipFullPage, int, bool);
    FIND_AND_SET_USER_ATTRIBUTE(twitterUserName, twitterUserName, QString, QString);
    FIND_AND_SET_USER_ATTRIBUTE(twitterId, twitterId, QString, QString);
    FIND_AND_SET_USER_ATTRIBUTE(groupName, groupName, QString, QString);
    FIND_AND_SET_USER_ATTRIBUTE(recognitionLanguage, recognitionLanguage, QString, QString);
    FIND_AND_SET_USER_ATTRIBUTE(referralProof, referralProof, QString, QString);
    FIND_AND_SET_USER_ATTRIBUTE(educationalDiscount, educationalDiscount, int, bool);
    FIND_AND_SET_USER_ATTRIBUTE(businessAddress, businessAddress, QString, QString);
    FIND_AND_SET_USER_ATTRIBUTE(hideSponsorBilling, hideSponsorBilling, int, bool);
    FIND_AND_SET_USER_ATTRIBUTE(taxExempt, taxExempt, int, bool);
    FIND_AND_SET_USER_ATTRIBUTE(useEmailAutoFiling, useEmailAutoFiling, int, bool);
    FIND_AND_SET_USER_ATTRIBUTE(reminderEmailConfig, reminderEmailConfig, int, qevercloud::ReminderEmailConfig::type);

#undef FIND_AND_SET_USER_ATTRIBUTE

    if (foundSomeUserAttribute) {
        user.setUserAttributes(std::move(attributes));
    }

    bool foundSomeAccountingProperty = false;
    qevercloud::Accounting accounting;

#define FIND_AND_SET_ACCOUNTING_PROPERTY(column, property, type, localType) \
    { \
        int index = rec.indexOf(#column); \
        if (index >= 0) { \
            QVariant value = rec.value(#column); \
            if (!value.isNull()) { \
                accounting.property = static_cast<localType>(qvariant_cast<type>(value)); \
                foundSomeAccountingProperty = true; \
            } \
        } \
    }

    FIND_AND_SET_ACCOUNTING_PROPERTY(uploadLimit, uploadLimit, qint64, qint64);
    FIND_AND_SET_ACCOUNTING_PROPERTY(uploadLimitEnd, uploadLimitEnd, qint64, qevercloud::Timestamp);
    FIND_AND_SET_ACCOUNTING_PROPERTY(uploadLimitNextMonth, uploadLimitNextMonth, qint64, qint64);
    FIND_AND_SET_ACCOUNTING_PROPERTY(premiumServiceStatus, premiumServiceStatus,
                                     int, qevercloud::PremiumOrderStatus::type);
    FIND_AND_SET_ACCOUNTING_PROPERTY(premiumOrderNumber, premiumOrderNumber, QString, QString);
    FIND_AND_SET_ACCOUNTING_PROPERTY(premiumCommerceService, premiumCommerceService, QString, QString);
    FIND_AND_SET_ACCOUNTING_PROPERTY(premiumServiceStart, premiumServiceStart,
                                     qint64, qevercloud::Timestamp);
    FIND_AND_SET_ACCOUNTING_PROPERTY(premiumServiceSKU, premiumServiceSKU, QString, QString);
    FIND_AND_SET_ACCOUNTING_PROPERTY(lastSuccessfulCharge, lastSuccessfulCharge,
                                     qint64, qevercloud::Timestamp);
    FIND_AND_SET_ACCOUNTING_PROPERTY(lastFailedCharge, lastFailedCharge, qint64, qevercloud::Timestamp);
    FIND_AND_SET_ACCOUNTING_PROPERTY(lastFailedChargeReason, lastFailedChargeReason, QString, QString);
    FIND_AND_SET_ACCOUNTING_PROPERTY(nextPaymentDue, nextPaymentDue, qint64, qevercloud::Timestamp);
    FIND_AND_SET_ACCOUNTING_PROPERTY(premiumLockUntil, premiumLockUntil, qint64, qevercloud::Timestamp);
    FIND_AND_SET_ACCOUNTING_PROPERTY(updated, updated, qint64, qevercloud::Timestamp);
    FIND_AND_SET_ACCOUNTING_PROPERTY(premiumSubscriptionNumber, premiumSubscriptionNumber,
                                     QString, QString);
    FIND_AND_SET_ACCOUNTING_PROPERTY(lastRequestedCharge, lastRequestedCharge, qint64, qevercloud::Timestamp);
    FIND_AND_SET_ACCOUNTING_PROPERTY(currency, currency, QString, QString);
    FIND_AND_SET_ACCOUNTING_PROPERTY(accountingBusinessId, businessId, int, qint32);
    FIND_AND_SET_ACCOUNTING_PROPERTY(accountingBusinessName, businessName, QString, QString);
    FIND_AND_SET_ACCOUNTING_PROPERTY(accountingBusinessRole, businessRole, int, qevercloud::BusinessUserRole::type);
    FIND_AND_SET_ACCOUNTING_PROPERTY(unitPrice, unitPrice, int, qint32);
    FIND_AND_SET_ACCOUNTING_PROPERTY(unitDiscount, unitDiscount, int, qint32);
    FIND_AND_SET_ACCOUNTING_PROPERTY(nextChargeDate, nextChargeDate, qint64, qevercloud::Timestamp);

#undef FIND_AND_SET_ACCOUNTING_PROPERTY

    if (foundSomeAccountingProperty) {
        user.setAccounting(std::move(accounting));
    }

    bool foundSomePremiumInfoProperty = false;
    qevercloud::PremiumInfo premiumInfo;

#define FIND_AND_SET_PREMIUM_INFO_PROPERTY(column, property, type, localType) \
    { \
        int index = rec.indexOf(#column); \
        if (index >= 0) { \
            QVariant value = rec.value(#column); \
            if (!value.isNull()) { \
                premiumInfo.property = static_cast<localType>(qvariant_cast<type>(value)); \
                foundSomePremiumInfoProperty = true; \
            } \
        } \
    }

    FIND_AND_SET_PREMIUM_INFO_PROPERTY(currentTime, currentTime, qint64, qevercloud::Timestamp);
    FIND_AND_SET_PREMIUM_INFO_PROPERTY(premium, premium, int, bool);
    FIND_AND_SET_PREMIUM_INFO_PROPERTY(premiumRecurring, premiumRecurring, int, bool);
    FIND_AND_SET_PREMIUM_INFO_PROPERTY(premiumExtendable, premiumExtendable, int, bool);
    FIND_AND_SET_PREMIUM_INFO_PROPERTY(premiumPending, premiumPending, int, bool);
    FIND_AND_SET_PREMIUM_INFO_PROPERTY(premiumCancellationPending, premiumCancellationPending, int, bool);
    FIND_AND_SET_PREMIUM_INFO_PROPERTY(canPurchaseUploadAllowance, canPurchaseUploadAllowance, int, bool);
    FIND_AND_SET_PREMIUM_INFO_PROPERTY(premiumExpirationDate, premiumExpirationDate, qint64, qevercloud::Timestamp);
    FIND_AND_SET_PREMIUM_INFO_PROPERTY(sponsoredGroupName, sponsoredGroupName, QString, QString);
    FIND_AND_SET_PREMIUM_INFO_PROPERTY(sponsoredGroupRole, sponsoredGroupRole, int, qevercloud::SponsoredGroupRole::type);
    FIND_AND_SET_PREMIUM_INFO_PROPERTY(premiumUpgradable, premiumUpgradable, int, bool);

#undef FIND_AND_SET_PREMIUM_INFO_PROPERTY

    if (foundSomePremiumInfoProperty) {
        user.setPremiumInfo(std::move(premiumInfo));
    }

    bool foundSomeBusinessUserInfoProperty = false;
    qevercloud::BusinessUserInfo businessUserInfo;

#define FIND_AND_SET_BUSINESS_USER_INFO_PROPERTY(column, property, type, localType) \
    { \
        int index = rec.indexOf(#column); \
        if (index >= 0) { \
            QVariant value = rec.value(#column); \
            if (!value.isNull()) { \
                businessUserInfo.property = static_cast<localType>(qvariant_cast<type>(value)); \
                foundSomeBusinessUserInfoProperty = true; \
            } \
        } \
    }

    FIND_AND_SET_BUSINESS_USER_INFO_PROPERTY(businessId, businessId, qint32, qint32);
    FIND_AND_SET_BUSINESS_USER_INFO_PROPERTY(businessName, businessName, QString, QString);
    FIND_AND_SET_BUSINESS_USER_INFO_PROPERTY(role, role, int, qevercloud::BusinessUserRole::type);
    FIND_AND_SET_BUSINESS_USER_INFO_PROPERTY(businessInfoEmail, email, QString, QString);

#undef FIND_AND_SET_BUSINESS_USER_INFO_PROPERTY

    if (foundSomeBusinessUserInfoProperty) {
        user.setBusinessUserInfo(std::move(businessUserInfo));
    }

    return true;
}

void LocalStorageManagerPrivate::FillNoteFromSqlRecord(const QSqlRecord & rec, Note & note) const
{
#define CHECK_AND_SET_NOTE_PROPERTY(propertyLocalName, setter, type, localType) \
    int propertyLocalName##index = rec.indexOf(#propertyLocalName); \
    if (propertyLocalName##index >= 0) { \
        QVariant value = rec.value(propertyLocalName##index); \
        if (!value.isNull()) { \
            note.setter(static_cast<localType>(qvariant_cast<type>(value))); \
        } \
    }

    CHECK_AND_SET_NOTE_PROPERTY(isDirty, setDirty, int, bool);
    CHECK_AND_SET_NOTE_PROPERTY(isLocal, setLocal, int, bool);
    CHECK_AND_SET_NOTE_PROPERTY(hasShortcut, setShortcut, int, bool);
    CHECK_AND_SET_NOTE_PROPERTY(localGuid, setLocalGuid, QString, QString);

    CHECK_AND_SET_NOTE_PROPERTY(guid, setGuid, QString, QString);
    CHECK_AND_SET_NOTE_PROPERTY(updateSequenceNumber, setUpdateSequenceNumber, qint32, qint32);

    CHECK_AND_SET_NOTE_PROPERTY(notebookGuid, setNotebookGuid, QString, QString);
    CHECK_AND_SET_NOTE_PROPERTY(title, setTitle, QString, QString);
    CHECK_AND_SET_NOTE_PROPERTY(content, setContent, QString, QString);
    CHECK_AND_SET_NOTE_PROPERTY(contentLength, setContentLength, qint32, qint32);
    CHECK_AND_SET_NOTE_PROPERTY(contentHash, setContentHash, QByteArray, QByteArray);

    CHECK_AND_SET_NOTE_PROPERTY(creationTimestamp, setCreationTimestamp, qint64, qint64);
    CHECK_AND_SET_NOTE_PROPERTY(modificationTimestamp, setModificationTimestamp, qint64, qint64);
    CHECK_AND_SET_NOTE_PROPERTY(deletionTimestamp, setDeletionTimestamp, qint64, qint64);
    CHECK_AND_SET_NOTE_PROPERTY(isActive, setActive, int, bool);

#undef CHECK_AND_SET_NOTE_PROPERTY

    int indexOfThumbnail = rec.indexOf("thumbnail");
    if (indexOfThumbnail >= 0) {
        QVariant thumbnailValue = rec.value(indexOfThumbnail);
        if (!thumbnailValue.isNull()) {
            QImage thumbnail = thumbnailValue.value<QImage>();
            note.setThumbnail(thumbnail);
        }
    }

    int hasAttributesIndex = rec.indexOf("hasAttributes");
    if (hasAttributesIndex >= 0)
    {
        QVariant hasAttributesValue = rec.value(hasAttributesIndex);
        if (!hasAttributesValue.isNull())
        {
            bool hasAttributes = static_cast<bool>(qvariant_cast<int>(hasAttributesValue));
            if (hasAttributes)
            {
                qevercloud::NoteAttributes & attributes = note.noteAttributes();

                FillNoteAttributesFromSqlRecord(rec, attributes);
                FillNoteAttributesApplicationDataKeysOnlyFromSqlRecord(rec, attributes);
                FillNoteAttributesApplicationDataFullMapFromSqlRecord(rec, attributes);
                FillNoteAttributesClassificationsFromSqlRecord(rec, attributes);
            }
        }
    }
}

bool LocalStorageManagerPrivate::FillNotebookFromSqlRecord(const QSqlRecord & record, Notebook & notebook,
                                                           QString & errorDescription) const
{
#define CHECK_AND_SET_NOTEBOOK_ATTRIBUTE(attribute, setter, dbType, trueType, isRequired) { \
        bool valueFound = false; \
        int index = record.indexOf(#attribute); \
        if (index >= 0) { \
            QVariant value = record.value(index); \
            if (!value.isNull()) { \
                notebook.setter(static_cast<trueType>((qvariant_cast<dbType>(value)))); \
                valueFound = true; \
            } \
        } \
        \
        if (!valueFound && isRequired) { \
            errorDescription += QT_TR_NOOP("Internal error: No " #attribute " field " \
                                           "in the result of SQL query"); \
            QNCRITICAL(errorDescription); \
            return false; \
        } \
    }

    bool isRequired = true;

    CHECK_AND_SET_NOTEBOOK_ATTRIBUTE(isDirty, setDirty, int, bool, isRequired);
    CHECK_AND_SET_NOTEBOOK_ATTRIBUTE(isLocal, setLocal, int, bool, isRequired);
    CHECK_AND_SET_NOTEBOOK_ATTRIBUTE(localGuid, setLocalGuid, QString, QString, isRequired);

    CHECK_AND_SET_NOTEBOOK_ATTRIBUTE(updateSequenceNumber, setUpdateSequenceNumber,
                                     qint32, qint32, isRequired);
    CHECK_AND_SET_NOTEBOOK_ATTRIBUTE(notebookName, setName, QString, QString, isRequired);
    CHECK_AND_SET_NOTEBOOK_ATTRIBUTE(creationTimestamp, setCreationTimestamp,
                                     qint64, qint64, isRequired);
    CHECK_AND_SET_NOTEBOOK_ATTRIBUTE(modificationTimestamp, setModificationTimestamp,
                                     qint64, qint64, isRequired);

    isRequired = false;

    CHECK_AND_SET_NOTEBOOK_ATTRIBUTE(guid, setGuid, QString, QString, isRequired);
    CHECK_AND_SET_NOTEBOOK_ATTRIBUTE(linkedNotebookGuid, setLinkedNotebookGuid, QString, QString, isRequired);
    CHECK_AND_SET_NOTEBOOK_ATTRIBUTE(hasShortcut, setShortcut, int, bool, isRequired);
    CHECK_AND_SET_NOTEBOOK_ATTRIBUTE(stack, setStack, QString, QString, isRequired);

    CHECK_AND_SET_NOTEBOOK_ATTRIBUTE(isPublished, setPublished, int, bool, isRequired);
    if (notebook.hasPublished() && notebook.isPublished())
    {
        CHECK_AND_SET_NOTEBOOK_ATTRIBUTE(publishingUri, setPublishingUri, QString, QString, isRequired);
        CHECK_AND_SET_NOTEBOOK_ATTRIBUTE(publishingNoteSortOrder, setPublishingOrder,
                                         int, qint8, isRequired);
        CHECK_AND_SET_NOTEBOOK_ATTRIBUTE(publishingAscendingSort, setPublishingAscending,
                                         int, bool, isRequired);
        CHECK_AND_SET_NOTEBOOK_ATTRIBUTE(publicDescription, setPublishingPublicDescription,
                                         QString, QString, isRequired);
    }

    CHECK_AND_SET_NOTEBOOK_ATTRIBUTE(businessNotebookDescription, setBusinessNotebookDescription,
                                     QString, QString, isRequired);
    CHECK_AND_SET_NOTEBOOK_ATTRIBUTE(businessNotebookPrivilegeLevel, setBusinessNotebookPrivilegeLevel,
                                     int, qint8, isRequired);
    CHECK_AND_SET_NOTEBOOK_ATTRIBUTE(businessNotebookIsRecommended, setBusinessNotebookRecommended,
                                     int, bool, isRequired);

    CHECK_AND_SET_NOTEBOOK_ATTRIBUTE(isLastUsed, setLastUsed, int, bool, isRequired);
    CHECK_AND_SET_NOTEBOOK_ATTRIBUTE(isDefault, setDefaultNotebook, int, bool, isRequired);

    // NOTE: workarounding unset isDefaultNotebook and isLastUsed
    if (!notebook.isDefaultNotebook()) {
        notebook.setDefaultNotebook(false);
    }

    if (!notebook.isLastUsed()) {
        notebook.setLastUsed(false);
    }

    if (record.contains("contactId") && !record.isNull("contactId"))
    {
        if (notebook.hasContact()) {
            UserAdapter contact = notebook.contact();
            contact.setId(qvariant_cast<qint32>(record.value("contactId")));
            notebook.setContact(contact);
        }
        else {
            UserWrapper contact;
            contact.setId(qvariant_cast<qint32>(record.value("contactId")));
            notebook.setContact(contact);
        }

        UserAdapter user = notebook.contact();
        QString error;
        bool res = FillUserFromSqlRecord(record, user, error);
        if (!res) {
            errorDescription += error;
            return false;
        }
    }

#define SET_EN_NOTEBOOK_RESTRICTION(notebook_restriction, setter) \
    { \
        int index = record.indexOf(#notebook_restriction); \
        if (index >= 0) { \
            QVariant value = record.value(index); \
            if (!value.isNull()) { \
                notebook.setter(qvariant_cast<int>(value) > 0 ? false : true); \
            } \
        } \
    }

    SET_EN_NOTEBOOK_RESTRICTION(noReadNotes, setCanReadNotes);
    SET_EN_NOTEBOOK_RESTRICTION(noCreateNotes, setCanCreateNotes);
    SET_EN_NOTEBOOK_RESTRICTION(noUpdateNotes, setCanUpdateNotes);
    SET_EN_NOTEBOOK_RESTRICTION(noExpungeNotes, setCanExpungeNotes);
    SET_EN_NOTEBOOK_RESTRICTION(noShareNotes, setCanShareNotes);
    SET_EN_NOTEBOOK_RESTRICTION(noEmailNotes, setCanEmailNotes);
    SET_EN_NOTEBOOK_RESTRICTION(noSendMessageToRecipients, setCanSendMessageToRecipients);
    SET_EN_NOTEBOOK_RESTRICTION(noUpdateNotebook, setCanUpdateNotebook);
    SET_EN_NOTEBOOK_RESTRICTION(noExpungeNotebook, setCanExpungeNotebook);
    SET_EN_NOTEBOOK_RESTRICTION(noSetDefaultNotebook, setCanSetDefaultNotebook);
    SET_EN_NOTEBOOK_RESTRICTION(noSetNotebookStack, setCanSetNotebookStack);
    SET_EN_NOTEBOOK_RESTRICTION(noPublishToPublic, setCanPublishToPublic);
    SET_EN_NOTEBOOK_RESTRICTION(noPublishToBusinessLibrary, setCanPublishToBusinessLibrary);
    SET_EN_NOTEBOOK_RESTRICTION(noCreateTags, setCanCreateTags);
    SET_EN_NOTEBOOK_RESTRICTION(noUpdateTags, setCanUpdateTags);
    SET_EN_NOTEBOOK_RESTRICTION(noExpungeTags, setCanExpungeTags);
    SET_EN_NOTEBOOK_RESTRICTION(noSetParentTag, setCanSetParentTag);
    SET_EN_NOTEBOOK_RESTRICTION(noCreateSharedNotebooks, setCanCreateSharedNotebooks);

#undef SET_EN_NOTEBOOK_RESTRICTION

    CHECK_AND_SET_NOTEBOOK_ATTRIBUTE(updateWhichSharedNotebookRestrictions,
                                     setUpdateWhichSharedNotebookRestrictions,
                                     int, qint8, isRequired);

    CHECK_AND_SET_NOTEBOOK_ATTRIBUTE(expungeWhichSharedNotebookRestrictions,
                                     setExpungeWhichSharedNotebookRestrictions,
                                     int, qint8, isRequired);

    if (notebook.hasGuid())
    {
        QString error;
        SharedNotebookWrapper sharedNotebook;
        bool res = FillSharedNotebookFromSqlRecord(record, sharedNotebook, error);
        if (!res) {
            errorDescription += error;
            return false;
        }

        if (sharedNotebook.hasNotebookGuid()) {
            notebook.addSharedNotebook(sharedNotebook);
        }
    }

    return true;
}

bool LocalStorageManagerPrivate::FillSharedNotebookFromSqlRecord(const QSqlRecord & rec,
                                                          ISharedNotebook & sharedNotebook,
                                                          QString & errorDescription) const
{
#define CHECK_AND_SET_SHARED_NOTEBOOK_PROPERTY(property, type, localType, setter) \
    { \
        int index = rec.indexOf(#property); \
        if (index >= 0) { \
            QVariant value = rec.value(index); \
            if (!value.isNull()) { \
                sharedNotebook.setter(static_cast<localType>(qvariant_cast<type>(value))); \
            } \
        } \
    }

    CHECK_AND_SET_SHARED_NOTEBOOK_PROPERTY(shareId, qint64, qint64, setId);
    CHECK_AND_SET_SHARED_NOTEBOOK_PROPERTY(userId, qint32, qint32, setUserId);
    CHECK_AND_SET_SHARED_NOTEBOOK_PROPERTY(sharedNotebookEmail, QString, QString, setEmail);
    CHECK_AND_SET_SHARED_NOTEBOOK_PROPERTY(sharedNotebookCreationTimestamp, qint64,
                                           qint64, setCreationTimestamp);
    CHECK_AND_SET_SHARED_NOTEBOOK_PROPERTY(sharedNotebookModificationTimestamp, qint64,
                                           qint64, setModificationTimestamp);
    CHECK_AND_SET_SHARED_NOTEBOOK_PROPERTY(shareKey, QString, QString, setShareKey);
    CHECK_AND_SET_SHARED_NOTEBOOK_PROPERTY(sharedNotebookUsername, QString, QString, setUsername);
    CHECK_AND_SET_SHARED_NOTEBOOK_PROPERTY(sharedNotebookPrivilegeLevel, int, qint8, setPrivilegeLevel);
    CHECK_AND_SET_SHARED_NOTEBOOK_PROPERTY(allowPreview, int, bool, setAllowPreview);   // NOTE: int to bool cast
    CHECK_AND_SET_SHARED_NOTEBOOK_PROPERTY(recipientReminderNotifyEmail, int, bool, setReminderNotifyEmail);  // NOTE: int to bool cast
    CHECK_AND_SET_SHARED_NOTEBOOK_PROPERTY(recipientReminderNotifyInApp, int, bool, setReminderNotifyApp);  // NOTE: int to bool cast
    CHECK_AND_SET_SHARED_NOTEBOOK_PROPERTY(notebookGuid, QString, QString, setNotebookGuid);

#undef CHECK_AND_SET_SHARED_NOTEBOOK_PROPERTY

    int recordIndex = rec.indexOf("indexInNotebook");
    if (recordIndex >= 0)
    {
        QVariant value = rec.value(recordIndex);
        if (!value.isNull())
        {
            bool conversionResult = false;
            int indexInNotebook = value.toInt(&conversionResult);
            if (!conversionResult) {
                errorDescription += QT_TR_NOOP("Internal error: can't convert shared notebook's index in notebook to int");
                QNCRITICAL(errorDescription);
                return false;
            }
            sharedNotebook.setIndexInNotebook(indexInNotebook);
        }
    }

    return true;
}

bool LocalStorageManagerPrivate::FillLinkedNotebookFromSqlRecord(const QSqlRecord & rec,
                                                                 LinkedNotebook & linkedNotebook,
                                                                 QString & errorDescription) const
{
#define CHECK_AND_SET_LINKED_NOTEBOOK_PROPERTY(property, type, localType, setter, isRequired) \
    { \
        bool valueFound = false; \
        int index = rec.indexOf(#property); \
        if (index >= 0) { \
            QVariant value = rec.value(index); \
            if (!value.isNull()) { \
                linkedNotebook.setter(static_cast<localType>(qvariant_cast<type>(value))); \
                valueFound = true; \
            } \
        } \
        \
        if (!valueFound && isRequired) { \
            errorDescription += QT_TR_NOOP("no " #property " field in the result of SQL query"); \
            QNCRITICAL(errorDescription); \
            return false; \
        } \
    }

    bool isRequired = true;
    CHECK_AND_SET_LINKED_NOTEBOOK_PROPERTY(isDirty, int, bool, setDirty, isRequired);
    CHECK_AND_SET_LINKED_NOTEBOOK_PROPERTY(guid, QString, QString, setGuid, isRequired);
    CHECK_AND_SET_LINKED_NOTEBOOK_PROPERTY(updateSequenceNumber, qint32, qint32,
                                           setUpdateSequenceNumber, isRequired);
    CHECK_AND_SET_LINKED_NOTEBOOK_PROPERTY(shareName, QString, QString, setShareName, isRequired);
    CHECK_AND_SET_LINKED_NOTEBOOK_PROPERTY(username, QString, QString, setUsername, isRequired);
    CHECK_AND_SET_LINKED_NOTEBOOK_PROPERTY(shardId, QString, QString, setShardId, isRequired);
    CHECK_AND_SET_LINKED_NOTEBOOK_PROPERTY(shareKey, QString, QString, setShareKey, isRequired);
    CHECK_AND_SET_LINKED_NOTEBOOK_PROPERTY(uri, QString, QString, setUri, isRequired);
    CHECK_AND_SET_LINKED_NOTEBOOK_PROPERTY(noteStoreUrl, QString, QString,
                                           setNoteStoreUrl, isRequired);
    CHECK_AND_SET_LINKED_NOTEBOOK_PROPERTY(webApiUrlPrefix, QString, QString,
                                           setWebApiUrlPrefix, isRequired);

    isRequired = false;
    CHECK_AND_SET_LINKED_NOTEBOOK_PROPERTY(stack, QString, QString, setStack, isRequired);
    CHECK_AND_SET_LINKED_NOTEBOOK_PROPERTY(businessId, qint32, qint32, setBusinessId, isRequired);

#undef CHECK_AND_SET_LINKED_NOTEBOOK_PROPERTY

    return true;
}

bool LocalStorageManagerPrivate::FillSavedSearchFromSqlRecord(const QSqlRecord & rec,
                                                              SavedSearch & search,
                                                              QString & errorDescription) const
{
#define CHECK_AND_SET_SAVED_SEARCH_PROPERTY(property, type, localType, setter, isRequired) \
    { \
        bool valueFound = false; \
        int index = rec.indexOf(#property); \
        if (index >= 0) { \
            QVariant value = rec.value(index); \
            if (!value.isNull()) { \
                search.setter(static_cast<localType>(qvariant_cast<type>(value))); \
                valueFound = true; \
            } \
        } \
        \
        if (!valueFound && isRequired) { \
            errorDescription += QT_TR_NOOP("no " #property " field in the result of SQL query"); \
            QNCRITICAL(errorDescription); \
            return false; \
        } \
    }

    bool isRequired = false;
    CHECK_AND_SET_SAVED_SEARCH_PROPERTY(guid, QString, QString, setGuid, isRequired);
    CHECK_AND_SET_SAVED_SEARCH_PROPERTY(hasShortcut, int, bool, setShortcut, isRequired);

    isRequired = true;
    CHECK_AND_SET_SAVED_SEARCH_PROPERTY(localGuid, QString, QString, setLocalGuid, isRequired);
    CHECK_AND_SET_SAVED_SEARCH_PROPERTY(name, QString, QString, setName, isRequired);
    CHECK_AND_SET_SAVED_SEARCH_PROPERTY(query, QString, QString, setQuery, isRequired);
    CHECK_AND_SET_SAVED_SEARCH_PROPERTY(format, int, qint8, setQueryFormat, isRequired);
    CHECK_AND_SET_SAVED_SEARCH_PROPERTY(updateSequenceNumber, qint32, qint32,
                                        setUpdateSequenceNumber, isRequired);
    CHECK_AND_SET_SAVED_SEARCH_PROPERTY(isDirty, int, bool, setDirty, isRequired);
    CHECK_AND_SET_SAVED_SEARCH_PROPERTY(isLocal, int, bool, setLocal, isRequired);

    CHECK_AND_SET_SAVED_SEARCH_PROPERTY(includeAccount, int, bool, setIncludeAccount,
                                        isRequired);
    CHECK_AND_SET_SAVED_SEARCH_PROPERTY(includePersonalLinkedNotebooks, int, bool,
                                        setIncludePersonalLinkedNotebooks, isRequired);
    CHECK_AND_SET_SAVED_SEARCH_PROPERTY(includeBusinessLinkedNotebooks, int, bool,
                                        setIncludeBusinessLinkedNotebooks, isRequired);

#undef CHECK_AND_SET_SAVED_SEARCH_PROPERTY

    return true;
}

bool LocalStorageManagerPrivate::FillTagFromSqlRecord(const QSqlRecord & rec, Tag & tag,
                                                      QString & errorDescription) const
{
#define CHECK_AND_SET_TAG_PROPERTY(property, type, localType, setter, isRequired) \
    { \
        bool valueFound = false; \
        int index = rec.indexOf(#property); \
        if (index >= 0) { \
            QVariant value = rec.value(index); \
            if (!value.isNull()) { \
                tag.setter(static_cast<localType>(qvariant_cast<type>(value))); \
                valueFound = true; \
            } \
        } \
        \
        if (!valueFound && isRequired) { \
            errorDescription += QT_TR_NOOP("no " #property " field in the result of SQL query"); \
            QNCRITICAL(errorDescription); \
            return false; \
        } \
    }

    bool isRequired = false;
    CHECK_AND_SET_TAG_PROPERTY(guid, QString, QString, setGuid, isRequired);
    CHECK_AND_SET_TAG_PROPERTY(linkedNotebookGuid, QString, QString, setLinkedNotebookGuid, isRequired);
    CHECK_AND_SET_TAG_PROPERTY(parentGuid, QString, QString, setParentGuid, isRequired);
    CHECK_AND_SET_TAG_PROPERTY(hasShortcut, int, bool, setShortcut, isRequired);

    isRequired = true;
    CHECK_AND_SET_TAG_PROPERTY(localGuid, QString, QString, setLocalGuid, isRequired);
    CHECK_AND_SET_TAG_PROPERTY(updateSequenceNumber, qint32, qint32, setUpdateSequenceNumber, isRequired);
    CHECK_AND_SET_TAG_PROPERTY(name, QString, QString, setName, isRequired);
    CHECK_AND_SET_TAG_PROPERTY(isDirty, int, bool, setDirty, isRequired);
    CHECK_AND_SET_TAG_PROPERTY(isLocal, int, bool, setLocal, isRequired);
    CHECK_AND_SET_TAG_PROPERTY(isDeleted, int, bool, setDeleted, isRequired);

#undef CHECK_AND_SET_TAG_PROPERTY

    return true;
}

QList<Tag> LocalStorageManagerPrivate::FillTagsFromSqlQuery(QSqlQuery & query, QString & errorDescription) const
{
    QList<Tag> tags;
    tags.reserve(qMax(query.size(), 0));

    while(query.next())
    {
        tags << Tag();
        Tag & tag = tags.back();

        QString tagLocalGuid = query.value(0).toString();
        if (tagLocalGuid.isEmpty()) {
            errorDescription = QT_TR_NOOP("Internal error: no tag's local guid in the result of SQL query");
            tags.clear();
            return tags;
        }

        tag.setLocalGuid(tagLocalGuid);

        bool res = FindTag(tag, errorDescription);
        if (!res) {
            tags.clear();
            return tags;
        }
    }

    return tags;
}

bool LocalStorageManagerPrivate::FindAndSetTagGuidsPerNote(Note & note, QString & errorDescription) const
{
    errorDescription += QT_TR_NOOP("can't find tag guids per note: ");

    const QString noteLocalGuid = note.localGuid();

    QSqlQuery query(m_sqlDatabase);
    query.prepare("SELECT tag, tagIndexInNote FROM NoteTags WHERE localNote = ?");
    query.addBindValue(noteLocalGuid);

    bool res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("can't select note tags from \"NoteTags\" table in SQL database");

    QMultiHash<int, QString> tagGuidsAndIndices;
    while (query.next())
    {
        QSqlRecord rec = query.record();

        QString tagGuid;
        bool tagFound = false;
        int tagIndex = rec.indexOf("tag");
        if (tagIndex >= 0) {
            QVariant value = rec.value(tagIndex);
            if (!value.isNull()) {
                tagGuid = value.toString();
                tagFound = true;
            }
        }

        if (!tagFound) {
            errorDescription += QT_TR_NOOP("Internal error: can't find tag guid in the result of SQL query");
            return false;
        }

        if (!CheckGuid(tagGuid)) {
            errorDescription += QT_TR_NOOP("found invalid tag guid for requested note");
            return false;
        }

        QNDEBUG("Found tag guid " << tagGuid << " for note with local guid " << noteLocalGuid);

        int indexInNote = -1;
        int recordIndex = rec.indexOf("tagIndexInNote");
        if (recordIndex >= 0)
        {
            QVariant value = rec.value(recordIndex);
            if (!value.isNull())
            {
                bool conversionResult = false;
                indexInNote = value.toInt(&conversionResult);
                if (!conversionResult) {
                    errorDescription += QT_TR_NOOP("Internal error: unable to convert to int "
                                                   "while processing the result of SQL query");
                    return false;
                }
            }
        }

        tagGuidsAndIndices.insertMulti(indexInNote, tagGuid);
    }

    int numTagGuids = tagGuidsAndIndices.size();
    QList<QPair<QString, int> > tagGuidIndexPairs;
    tagGuidIndexPairs.reserve(std::max(numTagGuids, 0));
    for(QMultiHash<int, QString>::ConstIterator it = tagGuidsAndIndices.constBegin();
        it != tagGuidsAndIndices.constEnd(); ++it)
    {
        tagGuidIndexPairs << QPair<QString, int>(it.value(), it.key());
    }

    qSort(tagGuidIndexPairs.begin(), tagGuidIndexPairs.end(), QStringIntPairCompareByInt());
    QStringList tagGuids;
    tagGuids.reserve(std::max(numTagGuids, 0));
    for(int i = 0; i < numTagGuids; ++i) {
        tagGuids << tagGuidIndexPairs[i].first;
    }

    note.setTagGuids(tagGuids);

    return true;
}

bool LocalStorageManagerPrivate::FindAndSetResourcesPerNote(Note & note, QString & errorDescription,
                                                            const bool withBinaryData) const
{
    errorDescription += QT_TR_NOOP("can't find resources for note: ");

    const QString noteLocalGuid = note.localGuid();

    // NOTE: it's weird but I can only get this query work as intended,
    // any more specific ones trying to pick the resource for note's local guid fail miserably.
    // I've just spent some hours of my life trying to figure out what the hell is going on here
    // but the best I was able to do is this. Please be very careful if you think you can do better here...
    QString queryString = QString("SELECT * FROM NoteResources");
    QSqlQuery query(m_sqlDatabase);
    bool res = query.exec(queryString);
    DATABASE_CHECK_AND_SET_ERROR("can't select resources' guids per note's guid");

    QStringList resourceGuids;
    while(query.next())
    {
        QSqlRecord rec = query.record();
        int index = rec.indexOf("localNote");
        if (index >= 0)
        {
            QVariant value = rec.value(index);
            if (value.isNull()) {
                continue;
            }

            QString foundNoteLocalGuid = value.toString();

            int resourceIndex = rec.indexOf("localResource");
            if ((foundNoteLocalGuid == noteLocalGuid) && (resourceIndex >= 0))
            {
                value = rec.value(resourceIndex);
                if (value.isNull()) {
                    continue;
                }

                QString resourceGuid = value.toString();
                resourceGuids << resourceGuid;
                QNDEBUG("Found resource's local guid: " << resourceGuid);
            }
        }
    }

    int numResources = resourceGuids.size();
    QNDEBUG("Found " << numResources << " resources");

    QList<ResourceWrapper> resources;
    foreach(const QString & resourceGuid, resourceGuids)
    {
        resources << ResourceWrapper();
        ResourceWrapper & resource = resources.back();
        resource.setLocalGuid(resourceGuid);

        bool res = FindEnResource(resource, errorDescription, withBinaryData);
        DATABASE_CHECK_AND_SET_ERROR("can't find resource per local guid");

        QNDEBUG("Found resource with local guid " << resource.localGuid()
                << " for note with local guid " << noteLocalGuid);
    }

    qSort(resources.begin(), resources.end(), ResourceWrapperCompareByIndex());
    note.setResources(resources);

    return true;
}

void LocalStorageManagerPrivate::SortSharedNotebooks(Notebook & notebook) const
{
    if (!notebook.hasSharedNotebooks()) {
        return;
    }

    // Sort shared notebooks to ensure the correct order for proper work of comparison operators
    QList<SharedNotebookAdapter> sharedNotebookAdapters = notebook.sharedNotebooks();

    qSort(sharedNotebookAdapters.begin(), sharedNotebookAdapters.end(), SharedNotebookAdapterCompareByIndex());
}

bool LocalStorageManagerPrivate::noteSearchQueryToSQL(const NoteSearchQuery & noteSearchQuery,
                                                      QString & sql, QString & errorDescription) const
{
    errorDescription = QT_TR_NOOP("Can't convert note search string into SQL query: ");

    // Setting up initial templates
    QString sqlPrefix = "SELECT DISTINCT localGuid ";
    sql.resize(0);

    bool queryHasAnyModifier = noteSearchQuery.hasAnyModifier();

    // ========== 1) Processing notebook modifier (if present) ==============

    QString notebookName = noteSearchQuery.notebookModifier();
    QString notebookLocalGuid;
    if (!notebookName.isEmpty())
    {
        QSqlQuery query(m_sqlDatabase);
        QString notebookQueryString = QString("SELECT localGuid FROM NotebookFTS WHERE notebookName MATCH '%1' LIMIT 1").arg(notebookName);
        bool res = query.exec(notebookQueryString);
        DATABASE_CHECK_AND_SET_ERROR("can't select notebook's local guid by notebook's name");

        if (!query.next()) {
            errorDescription += QT_TR_NOOP("notebook with provided name was not found");
            return false;
        }

        QSqlRecord rec = query.record();
        int index = rec.indexOf("localGuid");
        if (index < 0) {
            errorDescription += QT_TR_NOOP("can't find notebook's local guid by notebook name: "
                                           "SQL query record doesn't contain the requested item");
            return false;
        }

        QVariant value = rec.value(index);
        if (value.isNull()) {
            errorDescription += QT_TR_NOOP("Internal error: found null notebook's local guid "
                                           "corresponding to notebook's name");
            return false;
        }

        notebookLocalGuid = value.toString();
        if (notebookLocalGuid.isEmpty()) {
            errorDescription += QT_TR_NOOP("Internal error: found empty notebook's local guid "
                                           "corresponding to notebook's name");
            return false;
        }
    }

    if (!notebookLocalGuid.isEmpty()) {
        sql += "(notebookLocalGuid = '";
        sql += notebookLocalGuid;
        sql += "') AND ";
    }

    // 2) ============ Determining whether "any:" modifier takes effect ==============

    QString uniteOperator = (noteSearchQuery.hasAnyModifier() ? "OR" : "AND");

    // 3) ============ Processing tag names and negated tag names, if any =============

    if (noteSearchQuery.hasAnyTag())
    {
        sql += "(NoteTags.localTag IS NOT NULL) ";
        sql += uniteOperator;
        sql += " ";
    }
    else if (noteSearchQuery.hasNegatedAnyTag())
    {
        sql += "(NoteTags.localTag IS NULL) ";
        sql += uniteOperator;
        sql += " ";
    }
    else
    {
        QStringList tagLocalGuids;
        QStringList tagNegatedLocalGuids;

        const QStringList & tagNames = noteSearchQuery.tagNames();
        if (!tagNames.isEmpty())
        {
            bool res = tagNamesToTagLocalGuids(tagNames, tagLocalGuids, errorDescription);
            if (!res) {
                return false;
            }
        }

        if (!tagLocalGuids.isEmpty())
        {
            if (!queryHasAnyModifier)
            {
                // In successful note search query there are exactly as many tag local guids
                // as there are tag names; therefore, when the search is for notes with
                // some particular tags, we need to ensure that each note's local guid
                // in the sub-query result is present there exactly as many times
                // as there are tag local guids in the query which the note is labeled with

                const int numTagLocalGuids = tagLocalGuids.size();
                sql += "(NoteTags.localNote IN (SELECT localNote FROM (SELECT localNote, localTag, COUNT(*) "
                       "FROM NoteTags WHERE NoteTags.localTag IN ('";
                foreach(const QString & tagLocalGuid, tagLocalGuids) {
                    sql += tagLocalGuid;
                    sql += "', '";
                }
                sql.chop(3);    // remove trailing comma, whitespace and single quotation mark

                sql += ") GROUP BY localNote HAVING COUNT(*)=";
                sql += QString::number(numTagLocalGuids);
                sql += "))) ";
            }
            else
            {
                // With "any:" modifier the search doesn't care about the exactness of tag-to-note map,
                // it would instead pick just any note corresponding to any of requested tags at least once

                sql += "(NoteTags.localNote IN (SELECT localNote FROM (SELECT localNote, localTag "
                       "FROM NoteTags WHERE NoteTags.localTag IN ('";
                foreach(const QString & tagLocalGuid, tagLocalGuids) {
                    sql += tagLocalGuid;
                    sql += "', '";
                }
                sql.chop(3);    // remove trailing comma, whitespace and single quotation mark

                sql += ")))) ";
            }

            sql += uniteOperator;
            sql += " ";
        }

        const QStringList & negatedTagNames = noteSearchQuery.negatedTagNames();
        if (!negatedTagNames.isEmpty())
        {
            bool res = tagNamesToTagLocalGuids(negatedTagNames, tagNegatedLocalGuids, errorDescription);
            if (!res) {
                return false;
            }
        }

        if (!tagNegatedLocalGuids.isEmpty())
        {
            if (!queryHasAnyModifier)
            {
                // First find all notes' local guids which actually correspond to negated tags' local guids;
                // then simply negate that condition

                const int numTagNegatedLocalGuids = tagNegatedLocalGuids.size();
                sql += "(NoteTags.localNote NOT IN (SELECT localNote FROM (SELECT localNote, localTag, COUNT(*) "
                       "FROM NoteTags WHERE NoteTags.localTag IN ('";
                foreach(const QString & tagNegatedLocalGuid, tagNegatedLocalGuids) {
                    sql += tagNegatedLocalGuid;
                    sql += "', '";
                }
                sql.chop(3);    // remove trailing comma, whitespace and single quotation mark

                sql += ") GROUP BY localNote HAVING COUNT(*)=";
                sql += QString::number(numTagNegatedLocalGuids);

                // Don't forget to account for the case of no tags used for note
                // so it's not even present in NoteTags table

                sql += ")) OR (NoteTags.localNote IS NULL)) ";
            }
            else
            {
                // With "any:" modifier the search doesn't care about the exactness of tag-to-note map,
                // it would instead pick just any note not from the list of notes corresponding to
                // any of requested tags at least once

                sql += "(NoteTags.localNote NOT IN (SELECT localNote FROM (SELECT localNote, localTag "
                       "FROM NoteTags WHERE NoteTags.localTag IN ('";
                foreach(const QString & tagNegatedLocalGuid, tagNegatedLocalGuids) {
                    sql += tagNegatedLocalGuid;
                    sql += "', '";
                }
                sql.chop(3);    // remove trailing comma, whitespace and single quotation mark

                // Don't forget to account for the case of no tags used for note
                // so it's not even present in NoteTags table

                sql += "))) OR (NoteTags.localNote IS NULL)) ";
            }

            sql += uniteOperator;
            sql += " ";
        }
    }

    // 4) ============== Processing resource mime types ===============

    if (noteSearchQuery.hasAnyResourceMimeType())
    {
        sql += "(NoteResources.localResource IS NOT NULL) ";
        sql += uniteOperator;
        sql += " ";
    }
    else if (noteSearchQuery.hasNegatedAnyResourceMimeType())
    {
        sql += "(NoteResources.localResource IS NULL) ";
        sql += uniteOperator;
        sql += " ";
    }
    else
    {
        QStringList resourceLocalGuidsPerMime;
        QStringList resourceNegatedLocalGuidsPerMime;

        const QStringList & resourceMimeTypes = noteSearchQuery.resourceMimeTypes();
        const int numResourceMimeTypes = resourceMimeTypes.size();
        if (!resourceMimeTypes.isEmpty())
        {
            bool res = resourceMimeTypesToResourceLocalGuids(resourceMimeTypes, resourceLocalGuidsPerMime,
                                                             errorDescription);
            if (!res) {
                return false;
            }
        }

        if (!resourceLocalGuidsPerMime.isEmpty())
        {
            if (!queryHasAnyModifier)
            {
                // Need to find notes which each have all the found resource local guids

                // One resource mime type can correspond to multiple resources. However,
                // one resource corresponds to exactly one note. When searching for notes
                // which resources have particular mime type, we need to ensure that each note's
                // local guid in the sub-query result is present there exactly as many times
                // as there are resource mime types in the query

                sql += "(NoteResources.localNote IN (SELECT localNote FROM (SELECT localNote, localResource, COUNT(*) "
                       "FROM NoteResources WHERE NoteResources.localResource IN ('";
                foreach(const QString & resourceLocalGuid, resourceLocalGuidsPerMime) {
                    sql += resourceLocalGuid;
                    sql += "', '";
                }
                sql.chop(3);    // remove trailing comma, whitespace and single quotation mark

                sql += ") GROUP BY localNote HAVING COUNT(*)=";
                sql += QString::number(numResourceMimeTypes);
                sql += "))) ";
            }
            else
            {
                // With "any:" modifier the search doesn't care about the exactness of resource mime type-to-note map,
                // it would instead pick just any note having at least one resource with requested mime type

                sql += "(NoteResources.localNote IN (SELECT localNote FROM (SELECT localNote, localResource "
                       "FROM NoteResources WHERE NoteResources.localResource IN ('";
                foreach(const QString & resourceLocalGuid, resourceLocalGuidsPerMime) {
                    sql += resourceLocalGuid;
                    sql += "', '";
                }
                sql.chop(3);    // remove trailing comma, whitespace and single quotation mark

                sql += ")))) ";
            }

            sql += uniteOperator;
            sql += " ";
        }

        const QStringList & negatedResourceMimeTypes = noteSearchQuery.negatedResourceMimeTypes();
        const int numNegatedResourceMimeTypes = negatedResourceMimeTypes.size();
        if (!negatedResourceMimeTypes.isEmpty())
        {
            bool res = resourceMimeTypesToResourceLocalGuids(negatedResourceMimeTypes,
                                                             resourceNegatedLocalGuidsPerMime,
                                                             errorDescription);
            if (!res) {
                return false;
            }
        }

        if (!resourceNegatedLocalGuidsPerMime.isEmpty())
        {
            if (!queryHasAnyModifier)
            {
                sql += "(NoteResources.localNote NOT IN (SELECT localNote FROM (SELECT localNote, localResource, COUNT(*) "
                       "FROM NoteResources WHERE NoteResources.localResource IN ('";
                foreach(const QString & resourceNegatedLocalGuid, resourceNegatedLocalGuidsPerMime) {
                    sql += resourceNegatedLocalGuid;
                    sql += "', '";
                }
                sql.chop(3);    // remove trailing comma, whitespace and single quotation mark

                sql += ") GROUP BY localNote HAVING COUNT(*)=";
                sql += QString::number(numNegatedResourceMimeTypes);

                // Don't forget to account for the case of no resources existing in the note
                // so it's not even present in NoteResources table

                sql += ")) OR (NoteResources.localNote IS NULL)) ";
            }
            else
            {
                sql += "(NoteResources.localNote NOT IN (SELECT localNote FROM (SELECT localNote, localResource "
                       "FROM NoteResources WHERE NoteResources.localResource IN ('";
                foreach(const QString & resourceNegatedLocalGuid, resourceNegatedLocalGuidsPerMime) {
                    sql += resourceNegatedLocalGuid;
                    sql += "', '";
                }
                sql.chop(3);    // remove trailing comma, whitespace and single quotation mark

                // Don't forget to account for the case of no resources existing in the note
                // so it's not even present in NoteResources table

                sql += "))) OR (NoteResources.localNote IS NULL)) ";
            }

            sql += uniteOperator;
            sql += " ";
        }
    }

    // 5) ============== Processing resource recognition indices ===============

    if (noteSearchQuery.hasAnyRecognitionType())
    {
        sql += "(NoteResources.localResource IN (SELECT resourceLocalGuid FROM ResourceRecoTypesFTS)) ";
        sql += uniteOperator;
        sql += " ";
    }
    else if (noteSearchQuery.hasNegatedAnyRecognitionType())
    {
        sql += "((NoteResources.localResource NOT IN (SELECT resourceLocalGuid FROM ResourceRecoTypesFTS)) OR ";
        sql += "(NoteResources.localResource IS NULL)) ";
        sql += uniteOperator;
        sql += " ";
    }
    else
    {
        QStringList resourceLocalGuidsPerRecognitionType;
        QStringList resourceNegatedLocalGuidsPerRecognitionType;

        const QStringList & resourceRecognitionTypes = noteSearchQuery.recognitionTypes();
        const int numResourceRecognitionTypes = resourceRecognitionTypes.size();
        if (!resourceRecognitionTypes.isEmpty())
        {
            bool res = resourceRecognitionTypesToResourceLocalGuids(resourceRecognitionTypes,
                                                                    resourceLocalGuidsPerRecognitionType,
                                                                    errorDescription);
            if (!res) {
                return false;
            }
        }

        if (!resourceLocalGuidsPerRecognitionType.isEmpty())
        {
            if (!queryHasAnyModifier)
            {
                // Need to find notes which each have all the found resource local guids

                // One resource recognition type can correspond to multiple resources. However,
                // one resource corresponds to exactly one note. When searching for notes
                // which resources have particular recognition type, we need to ensure that each note's
                // local guid in the sub-query result is present there exactly as many times
                // as there are resource recognition types in the query

                sql += "(NoteResources.localNote IN (SELECT localNote FROM (SELECT localNote, localResource, COUNT(*) "
                       "FROM NoteResources WHERE NoteResources.localResource IN ('";
                foreach(const QString & resourceLocalGuid, resourceLocalGuidsPerRecognitionType) {
                    sql += resourceLocalGuid;
                    sql += "', '";
                }
                sql.chop(3);    // remove trailing comma, whitespace and single quotation mark

                sql += ") GROUP BY localNote HAVING COUNT(*)=";
                sql += QString::number(numResourceRecognitionTypes);
                sql += "))) ";
            }
            else
            {
                // With "any:" modifier the search doesn't care about the exactness of resource recognition type-to-note map,
                // it would instead pick just any note having at least one resource with requested recognition type

                sql += "(NoteResources.localNote IN (SELECT localNote FROM (SELECT localNote, localResource "
                       "FROM NoteResources WHERE NoteResources.localResource IN ('";
                foreach(const QString & resourceLocalGuid, resourceLocalGuidsPerRecognitionType) {
                    sql += resourceLocalGuid;
                    sql += "', '";
                }
                sql.chop(3);    // remove trailing comma, whitespace and single quotation mark

                sql += ")))) ";
            }

            sql += uniteOperator;
            sql += " ";
        }

        const QStringList & negatedResourceRecognitionTypes = noteSearchQuery.negatedRecognitionTypes();
        const int numNegatedResourceRecognitionTypes = negatedResourceRecognitionTypes.size();
        if (!negatedResourceRecognitionTypes.isEmpty())
        {
            bool res = resourceRecognitionTypesToResourceLocalGuids(negatedResourceRecognitionTypes,
                                                                    resourceNegatedLocalGuidsPerRecognitionType,
                                                                    errorDescription);
            if (!res) {
                return false;
            }
        }

        if (!resourceNegatedLocalGuidsPerRecognitionType.isEmpty())
        {
            if (!queryHasAnyModifier)
            {
                sql += "(NoteResources.localNote NOT IN (SELECT localNote FROM (SELECT localNote, localResource, COUNT(*) "
                       "FROM NoteResources WHERE NoteResources.localResource IN ('";
                foreach(const QString & resourceNegatedLocalGuid, resourceNegatedLocalGuidsPerRecognitionType) {
                    sql += resourceNegatedLocalGuid;
                    sql += "', '";
                }
                sql.chop(3);    // remove trailing comma, whitespace and single quotation mark

                sql += ") GROUP BY localNote HAVING COUNT(*)=";
                sql += QString::number(numNegatedResourceRecognitionTypes);

                // Don't forget to account for the case of no resources existing in the note
                // so it's not even present in NoteResources table

                sql += ")) OR (NoteResources.localNote IS NULL)) ";
            }
            else
            {
                sql += "(NoteResources.localNote NOT IN (SELECT localNote FROM (SELECT localNote, localResource "
                       "FROM NoteResources WHERE NoteResources.localResource IN ('";
                foreach(const QString & resourceNegatedLocalGuid, resourceNegatedLocalGuidsPerRecognitionType) {
                    sql += resourceNegatedLocalGuid;
                    sql += "', '";
                }
                sql.chop(3);    // remove trailing comma, whitespace and single quotation mark

                // Don't forget to account for the case of no resources existing in the note
                // so it's not even present in NoteResources table

                sql += "))) OR (NoteResources.localNote IS NULL)) ";
            }

            sql += uniteOperator;
            sql += " ";
        }
    }

    // 6) ============== Processing other better generalizable filters =============

#define CHECK_AND_PROCESS_ANY_ITEM(hasAnyItem, hasNegatedAnyItem, column) \
    if (noteSearchQuery.hasAnyItem()) { \
        sql += "(NoteFTS." #column " IS NOT NULL) "; \
        sql += uniteOperator; \
        sql += " "; \
    } \
    else if (noteSearchQuery.hasNegatedAnyItem()) { \
        sql += "(NoteFTS." #column " IS NULL) "; \
        sql += uniteOperator; \
        sql += " "; \
    }

#define CHECK_AND_PROCESS_LIST(list, column, negated, ...) \
    const auto & noteSearchQuery##list##column = noteSearchQuery.list(); \
    if (!noteSearchQuery##list##column.isEmpty()) \
    { \
        sql += "("; \
        foreach(const auto & item, noteSearchQuery##list##column) \
        { \
            if (negated) { \
                sql += "(localGuid NOT IN "; \
            } \
            else { \
                sql += "(localGuid IN "; \
            } \
            \
            sql += "(SELECT localGuid FROM NoteFTS WHERE NoteFTS." #column " MATCH \'"; \
            sql += __VA_ARGS__(item); \
            sql += "\')) "; \
            sql += uniteOperator; \
            sql += " "; \
        } \
        sql.chop(uniteOperator.length() + 1); \
        sql += ")"; \
        sql += uniteOperator; \
        sql += " "; \
    }

#define CHECK_AND_PROCESS_NUMERIC_LIST(list, column, negated, ...) \
    const auto & noteSearchQuery##list##column = noteSearchQuery.list(); \
    if (!noteSearchQuery##list##column.isEmpty()) \
    { \
        auto it = noteSearchQuery##list##column.constEnd(); \
        if (queryHasAnyModifier) \
        { \
            if (negated) { \
                it = std::max_element(noteSearchQuery##list##column.constBegin(), \
                                      noteSearchQuery##list##column.constEnd()); \
            } \
            else { \
                it = std::min_element(noteSearchQuery##list##column.constBegin(), \
                                      noteSearchQuery##list##column.constEnd()); \
            } \
        } \
        else \
        { \
            if (negated) { \
                it = std::min_element(noteSearchQuery##list##column.constBegin(), \
                                      noteSearchQuery##list##column.constEnd()); \
            } \
            else { \
                it = std::max_element(noteSearchQuery##list##column.constBegin(), \
                                      noteSearchQuery##list##column.constEnd()); \
            } \
        } \
        \
        if (it != noteSearchQuery##list##column.constEnd()) \
        { \
            sql += "(localGuid IN (SELECT localGuid FROM Notes WHERE Notes." #column; \
            if (negated) { \
                sql += " < "; \
            } \
            else { \
                sql += " >= "; \
            } \
            sql += __VA_ARGS__(*it); \
            sql += ")) "; \
            sql += uniteOperator; \
            sql += " "; \
        } \
    }

#define CHECK_AND_PROCESS_ITEM(list, negatedList, hasAnyItem, hasNegatedAnyItem, column, ...) \
    CHECK_AND_PROCESS_ANY_ITEM(hasAnyItem, hasNegatedAnyItem, column) \
    else { \
        CHECK_AND_PROCESS_LIST(list, column, !negated, __VA_ARGS__); \
        CHECK_AND_PROCESS_LIST(negatedList, column, negated, __VA_ARGS__); \
    }

#define CHECK_AND_PROCESS_NUMERIC_ITEM(list, negatedList, hasAnyItem, hasNegatedAnyItem, column, ...) \
    CHECK_AND_PROCESS_ANY_ITEM(hasAnyItem, hasNegatedAnyItem, column) \
    else { \
        CHECK_AND_PROCESS_NUMERIC_LIST(list, column, !negated, __VA_ARGS__); \
        CHECK_AND_PROCESS_NUMERIC_LIST(negatedList, column, negated, __VA_ARGS__); \
    }

    bool negated = true;
    CHECK_AND_PROCESS_ITEM(titleNames, negatedTitleNames, hasAnyTitleName, hasNegatedAnyTitleName, title);
    CHECK_AND_PROCESS_NUMERIC_ITEM(creationTimestamps, negatedCreationTimestamps, hasAnyCreationTimestamp,
                                   hasNegatedAnyCreationTimestamp, creationTimestamp, QString::number);
    CHECK_AND_PROCESS_NUMERIC_ITEM(modificationTimestamps, negatedModificationTimestamps,
                                   hasAnyModificationTimestamp, hasNegatedAnyModificationTimestamp,
                                   modificationTimestamp, QString::number);
    CHECK_AND_PROCESS_NUMERIC_ITEM(subjectDateTimestamps, negatedSubjectDateTimestamps,
                                   hasAnySubjectDateTimestamp, hasNegatedAnySubjectDateTimestamp,
                                   subjectDate, QString::number);
    CHECK_AND_PROCESS_NUMERIC_ITEM(latitudes, negatedLatitudes, hasAnyLatitude, hasNegatedAnyLatitude,
                                   latitude, QString::number);
    CHECK_AND_PROCESS_NUMERIC_ITEM(longitudes, negatedLongitudes, hasAnyLongitude, hasNegatedAnyLongitude,
                                   longitude, QString::number);
    CHECK_AND_PROCESS_NUMERIC_ITEM(altitudes, negatedAltitudes, hasAnyAltitude, hasNegatedAnyAltitude,
                                   altitude, QString::number);
    CHECK_AND_PROCESS_ITEM(authors, negatedAuthors, hasAnyAuthor, hasNegatedAnyAuthor, author);
    CHECK_AND_PROCESS_ITEM(sources, negatedSources, hasAnySource, hasNegatedAnySource, source);
    CHECK_AND_PROCESS_ITEM(sourceApplications, negatedSourceApplications, hasAnySourceApplication,
                           hasNegatedAnySourceApplication, sourceApplication);
    CHECK_AND_PROCESS_ITEM(contentClasses, negatedContentClasses, hasAnyContentClass,
                           hasNegatedAnyContentClass, contentClass);
    CHECK_AND_PROCESS_ITEM(placeNames, negatedPlaceNames, hasAnyPlaceName, hasNegatedAnyPlaceName, placeName);
    CHECK_AND_PROCESS_ITEM(applicationData, negatedApplicationData, hasAnyApplicationData,
                           hasNegatedAnyApplicationData, applicationDataKeysOnly);
    CHECK_AND_PROCESS_ITEM(applicationData, negatedApplicationData, hasAnyApplicationData,
                           hasNegatedAnyApplicationData, applicationDataKeysMap);
    CHECK_AND_PROCESS_NUMERIC_ITEM(reminderOrders, negatedReminderOrders, hasAnyReminderOrder,
                                   hasNegatedAnyReminderOrder, reminderOrder, QString::number);
    CHECK_AND_PROCESS_NUMERIC_ITEM(reminderTimes, negatedReminderTimes, hasAnyReminderTime,
                                   hasNegatedAnyReminderTime, reminderTime, QString::number);
    CHECK_AND_PROCESS_NUMERIC_ITEM(reminderDoneTimes, negatedReminderDoneTimes, hasAnyReminderDoneTime,
                                   hasNegatedAnyReminderDoneTime, reminderDoneTime, QString::number);

#undef CHECK_AND_PROCESS_ITEM
#undef CHECK_AND_PROCESS_LIST
#undef CHECK_AND_PROCESS_ANY_ITEM
#undef CHECK_AND_PROCESS_NUMERIC_ITEM

    // 7) ============== Processing ToDo items ================

    if (noteSearchQuery.hasAnyToDo())
    {
        sql += "((NoteFTS.contentContainsFinishedToDo IS 1) OR (NoteFTS.contentContainsUnfinishedToDo IS 1)) ";
        sql += uniteOperator;
        sql += " ";
    }
    else if (noteSearchQuery.hasNegatedAnyToDo())
    {
        sql += "((NoteFTS.contentContainsFinishedToDo IS 0) OR (NoteFTS.contentContainsFinishedToDo IS NULL)) AND "
               "((NoteFTS.contentContainsUnfinishedToDo IS 0) OR (NoteFTS.contentContainsUnfinishedToDo IS NULL)) ";
        sql += uniteOperator;
        sql += " ";
    }
    else
    {
        if (noteSearchQuery.hasFinishedToDo()) {
            sql += "(NoteFTS.contentContainsFinishedToDo IS 1) " ;
            sql += uniteOperator;
            sql += " ";
        }
        else if (noteSearchQuery.hasNegatedFinishedToDo()) {
            sql += "((NoteFTS.contentContainsFinishedToDo IS 0) OR (NoteFTS.contentContainsFinishedToDo IS NULL)) " ;
            sql += uniteOperator;
            sql += " ";
        }

        if (noteSearchQuery.hasUnfinishedToDo()) {
            sql += "(NoteFTS.contentContainsUnfinishedToDo IS 1) " ;
            sql += uniteOperator;
            sql += " ";
        }
        else if (noteSearchQuery.hasNegatedUnfinishedToDo()) {
            sql += "((NoteFTS.contentContainsUnfinishedToDo IS 0) OR (NoteFTS.contentContainsUnfinishedToDo IS NULL)) " ;
            sql += uniteOperator;
            sql += " ";
        }
    }

    // 8) ============== Processing encryption item =================

    if (noteSearchQuery.hasNegatedEncryption()) {
        sql += "((NoteFTS.contentContainsEncryption IS 0) OR (NoteFTS.contentContainsEncryption IS NULL)) ";
        sql += uniteOperator;
        sql += " ";
    }
    else if (noteSearchQuery.hasEncryption()) {
        sql += "(NoteFTS.contentContainsEncryption IS 1) ";
        sql += uniteOperator;
        sql += " ";
    }

    // 9) ============== Removing trailing unite operator from the SQL string =============

    QString spareEnd = uniteOperator + QString(" ");
    if (sql.endsWith(spareEnd)) {
        sql.chop(spareEnd.size());
    }

    // 10) ============= See whether we should bother anything regarding tags or resources ============

    QString sqlPostfix = "FROM NoteFTS ";
    if (sql.contains("NoteTags")) {
        sqlPrefix += ", NoteTags.localTag ";
        sqlPostfix += "LEFT OUTER JOIN NoteTags ON NoteFTS.localGuid = NoteTags.localNote ";
    }

    if (sql.contains("NoteResources")) {
        sqlPrefix += ", NoteResources.localResource ";
        sqlPostfix += "LEFT OUTER JOIN NoteResources ON NoteFTS.localGuid = NoteResources.localNote ";
    }

    // 11) ============== Finalize the query composed of parts ===============

    sqlPrefix += sqlPostfix;
    sqlPrefix += "WHERE ";
    sql.prepend(sqlPrefix);

    return true;
}

bool LocalStorageManagerPrivate::tagNamesToTagLocalGuids(const QStringList & tagNames,
                                                         QStringList & tagLocalGuids,
                                                         QString & errorDescription) const
{
    tagLocalGuids.clear();

    QSqlQuery query(m_sqlDatabase);

    QString queryString;

    bool singleTagName = (tagNames.size() == 1);
    if (singleTagName)
    {
        bool res = query.prepare("SELECT localGuid FROM TagFTS WHERE nameUpper MATCH :names");
        DATABASE_CHECK_AND_SET_ERROR("can't select tag local guids for tag names: can't prepare SQL query");

        QString names = tagNames.at(0).toUpper();
        names.prepend("\'");
        names.append("\'");
        query.bindValue(":names", names);
    }
    else
    {
        bool someTagNameHasWhitespace = false;
        foreach(const QString & tagName, tagNames)
        {
            if (tagName.contains(" ")) {
                someTagNameHasWhitespace = true;
                break;
            }
        }

        if (someTagNameHasWhitespace)
        {
            // Unfortunately, stardard SQLite at least from Qt 4.x has standard query syntax for FTS
            // which does not support whitespaces in search terms and therefore MATCH function is simply inapplicable here,
            // have to use brute-force "equal to X1 or equal to X2 or ... equal to XN
            queryString = "SELECT localGuid FROM Tags WHERE ";

            foreach(const QString & tagName, tagNames) {
                queryString += "(nameUpper = \'";
                queryString += tagName.toUpper();
                queryString += "\') OR ";
            }
            queryString.chop(4);    // remove trailing OR and two whitespaces
        }
        else
        {
            queryString = "SELECT localGuid FROM TagFTS WHERE nameUpper MATCH \'";

            foreach(const QString & tagName, tagNames) {
                queryString += tagName.toUpper();
                queryString += " ";
            }
            queryString.chop(1);    // remove trailing whitespace
            queryString += "\'";
        }
    }

    bool res = false;
    if (queryString.isEmpty()) {
        res = query.exec();
    }
    else {
        res = query.exec(queryString);
    }
    DATABASE_CHECK_AND_SET_ERROR("can't select tag local guids for tag names " + tagNames.join(", "));

    while (query.next())
    {
        QSqlRecord rec = query.record();
        int index = rec.indexOf("localGuid");
        if (index < 0) {
            errorDescription += QT_TR_NOOP("Internal error: tag's local guid is not present in the result of SQL query");
            return false;
        }

        QVariant value = rec.value(index);
        tagLocalGuids << value.toString();
    }

    return true;
}

bool LocalStorageManagerPrivate::resourceMimeTypesToResourceLocalGuids(const QStringList & resourceMimeTypes,
                                                                       QStringList & resourceLocalGuids,
                                                                       QString & errorDescription) const
{
    resourceLocalGuids.clear();

    QSqlQuery query(m_sqlDatabase);

    QString queryString;

    bool singleMimeType = (resourceMimeTypes.size() == 1);
    if (singleMimeType)
    {
        bool res = query.prepare("SELECT resourceLocalGuid FROM ResourceMimeFTS WHERE mime MATCH :mimeTypes");
        DATABASE_CHECK_AND_SET_ERROR("can't select resource local guids for resource mime types: "
                                     "can't prepare SQL query");

        QString mimeTypes = resourceMimeTypes.at(0);
        mimeTypes.prepend("\'");
        mimeTypes.append("\'");
        query.bindValue(":mimeTypes", mimeTypes);
    }
    else
    {
        bool someMimeTypeHasWhitespace = false;
        foreach(const QString & mimeType, resourceMimeTypes)
        {
            if (mimeType.contains(" ")) {
                someMimeTypeHasWhitespace = true;
                break;
            }
        }

        if (someMimeTypeHasWhitespace)
        {
            // Unfortunately, stardard SQLite at least from Qt 4.x has standard query syntax for FTS
            // which does not support whitespaces in search terms and therefore MATCH function is simply inapplicable here,
            // have to use brute-force "equal to X1 or equal to X2 or ... equal to XN
            queryString = "SELECT resourceLocalGuid FROM Resources WHERE ";

            foreach(const QString & mimeType, resourceMimeTypes) {
                queryString += "(mime = \'";
                queryString += mimeType;
                queryString += "\') OR ";
            }
            queryString.chop(4);    // remove trailing OR and two whitespaces
        }
        else
        {
            // For unknown reason statements like "MATCH 'x OR y'" don't work for me while
            // "SELECT ... MATCH 'x' UNION SELECT ... MATCH 'y'" does work.

            foreach(const QString & mimeType, resourceMimeTypes) {
                queryString += "SELECT resourceLocalGuid FROM ResourceMimeFTS WHERE mime MATCH \'";
                queryString += mimeType;
                queryString += "\' UNION ";
            }
            queryString.chop(7);    // remove trailing characters
        }
    }

    bool res = false;
    if (queryString.isEmpty()) {
        res = query.exec();
    }
    else {
        res = query.exec(queryString);
    }
    DATABASE_CHECK_AND_SET_ERROR("can't select resource local guids for resource mime types");

    while (query.next())
    {
        QSqlRecord rec = query.record();
        int index = rec.indexOf("resourceLocalGuid");
        if (index < 0) {
            errorDescription += QT_TR_NOOP("Internal error: resource's local guid is not present in the result of SQL query");
            return false;
        }

        QVariant value = rec.value(index);
        resourceLocalGuids << value.toString();
    }

    return true;
}

bool LocalStorageManagerPrivate::resourceRecognitionTypesToResourceLocalGuids(const QStringList & resourceRecognitionTypes,
                                                                              QStringList & resourceLocalGuids,
                                                                              QString & errorDescription) const
{
    resourceLocalGuids.clear();

    QSqlQuery query(m_sqlDatabase);

    QString queryString;

    bool singleRecognitionType = (resourceRecognitionTypes.size() == 1);
    if (singleRecognitionType)
    {
        bool res = query.prepare("SELECT resourceLocalGuid FROM ResourceRecoTypesFTS WHERE recognitionType MATCH :recognitionType");
        DATABASE_CHECK_AND_SET_ERROR("can't select resource local guids for resource recognition type: "
                                     "can't prepare SQL query");

        QString recognitionType = resourceRecognitionTypes.at(0);
        recognitionType.prepend("\'");
        recognitionType.append("\'");
        query.bindValue(":recognitionType", recognitionType);
    }
    else
    {
        bool someRecognitionTypeHasWhitespace = false;
        foreach(const QString & recognitionType, resourceRecognitionTypes)
        {
            if (recognitionType.contains(" ")) {
                someRecognitionTypeHasWhitespace = true;
                break;
            }
        }

        if (someRecognitionTypeHasWhitespace)
        {
            // Unfortunately, stardard SQLite at least from Qt 4.x has standard query syntax for FTS
            // which does not support whitespaces in search terms and therefore MATCH function is simply inapplicable here,
            // have to use brute-force "equal to X1 or equal to X2 or ... equal to XN
            queryString = "SELECT resourceLocalGuid FROM ResourceRecognitionTypes WHERE ";

            foreach(const QString & recognitionType, resourceRecognitionTypes) {
                queryString += "(recognitionType = \'";
                queryString += recognitionType;
                queryString += "\') OR ";
            }
            queryString.chop(4);    // remove trailing OR and two whitespaces
        }
        else
        {
            // For unknown reason statements like "MATCH 'x OR y'" don't work for me while
            // "SELECT ... MATCH 'x' UNION SELECT ... MATCH 'y'" does work.

            foreach(const QString & recognitionType, resourceRecognitionTypes) {
                queryString += "SELECT resourceLocalGuid FROM ResourceRecoTypesFTS WHERE recognitionType MATCH \'";
                queryString += recognitionType;
                queryString += "\' UNION ";
            }
            queryString.chop(7);    // remove trailing characters
        }
    }

    bool res = false;
    if (queryString.isEmpty()) {
        res = query.exec();
    }
    else {
        res = query.exec(queryString);
    }
    DATABASE_CHECK_AND_SET_ERROR("can't select resource local guids for resource recognition types");

    while(query.next())
    {
        QSqlRecord rec = query.record();
        int index = rec.indexOf("resourceLocalGuid");
        if (index < 0) {
            errorDescription += QT_TR_NOOP("Internal error: resource's local guid is not present in the result of SQL query");
            return false;
        }

        QVariant value = rec.value(index);
        resourceLocalGuids << value.toString();
    }

    return true;
}

template <class T>
QString LocalStorageManagerPrivate::listObjectsOptionsToSqlQueryConditions(const LocalStorageManager::ListObjectsOptions & options,
                                                                           QString & errorDescription) const
{
    QString result;
    errorDescription.resize(0);

    bool listAll = options.testFlag(LocalStorageManager::ListAll);

    bool listDirty = options.testFlag(LocalStorageManager::ListDirty);
    bool listNonDirty = options.testFlag(LocalStorageManager::ListNonDirty);

    bool listElementsWithoutGuid = options.testFlag(LocalStorageManager::ListElementsWithoutGuid);
    bool listElementsWithGuid = options.testFlag(LocalStorageManager::ListElementsWithGuid);

    bool listLocal = options.testFlag(LocalStorageManager::ListLocal);
    bool listNonLocal = options.testFlag(LocalStorageManager::ListNonLocal);

    bool listElementsWithShortcuts = options.testFlag(LocalStorageManager::ListElementsWithShortcuts);
    bool listElementsWithoutShortcuts = options.testFlag(LocalStorageManager::ListElementsWithoutShortcuts);

    if (!listAll && !listDirty && !listNonDirty && !listElementsWithoutGuid && !listElementsWithGuid &&
        !listLocal && !listNonLocal && !listElementsWithShortcuts && !listElementsWithoutShortcuts)
    {
        errorDescription = QT_TR_NOOP("can't list objects by filter: detected incorrect filter flag: ");
        errorDescription += QString::number(static_cast<int>(options));
        return result;
    }

    if (!(listDirty && listNonDirty))
    {
        if (listDirty) {
            result += "(isDirty=1) AND ";
        }

        if (listNonDirty) {
            result += "(isDirty=0) AND ";
        }
    }

    if (!(listElementsWithoutGuid && listElementsWithGuid))
    {
        if (listElementsWithoutGuid) {
            result += "(guid IS NULL) AND ";
        }

        if (listElementsWithGuid) {
            result += "(guid IS NOT NULL) AND ";
        }
    }

    if (!(listLocal && listNonLocal))
    {
        if (listLocal) {
            result += "(isLocal=1) AND ";
        }

        if (listNonLocal) {
            result += "(isLocal=0) AND ";
        }
    }

    if (!(listElementsWithShortcuts && listElementsWithoutShortcuts))
    {
        if (listElementsWithShortcuts) {
            result += "(hasShortcut=1) AND ";
        }

        if (listElementsWithoutShortcuts) {
            result += "(hasShortcut=0) AND ";
        }
    }

    return result;
}

template<>
QString LocalStorageManagerPrivate::listObjectsOptionsToSqlQueryConditions<LinkedNotebook>(const LocalStorageManager::ListObjectsOptions & flag,
                                                                                           QString & errorDescription) const
{
    QString result;
    errorDescription.resize(0);

    bool listAll = flag.testFlag(LocalStorageManager::ListAll);
    bool listDirty = flag.testFlag(LocalStorageManager::ListDirty);
    bool listNonDirty = flag.testFlag(LocalStorageManager::ListNonDirty);

    if (!listAll && !listDirty && !listNonDirty) {
        errorDescription = QT_TR_NOOP("can't list linked notebooks by filter: detected incorrect filter flag: ");
        errorDescription += QString::number(static_cast<int>(flag));
        return result;
    }

    if (!(listDirty && listNonDirty))
    {
        if (listDirty) {
            result += "(isDirty=1)";
        }

        if (listNonDirty) {
            result += "(isDirty=0)";
        }
    }

    return result;
}

template <>
QString LocalStorageManagerPrivate::listObjectsGenericSqlQuery<SavedSearch>() const
{
    QString result = "SELECT * FROM SavedSearches";
    return result;
}

template <>
QString LocalStorageManagerPrivate::listObjectsGenericSqlQuery<Tag>() const
{
    QString result = "SELECT * FROM Tags";
    return result;
}

template <>
QString LocalStorageManagerPrivate::listObjectsGenericSqlQuery<LinkedNotebook>() const
{
    QString result = "SELECT * FROM LinkedNotebooks";
    return result;
}

template <>
QString LocalStorageManagerPrivate::listObjectsGenericSqlQuery<Notebook>() const
{
    QString result = "SELECT * FROM Notebooks LEFT OUTER JOIN NotebookRestrictions "
                     "ON Notebooks.localGuid = NotebookRestrictions.localGuid "
                     "LEFT OUTER JOIN SharedNotebooks ON ((Notebooks.guid IS NOT NULL) AND (Notebooks.guid = SharedNotebooks.notebookGuid)) "
                     "LEFT OUTER JOIN Users ON Notebooks.contactId = Users.id "
                     "LEFT OUTER JOIN UserAttributes ON Notebooks.contactId = UserAttributes.id "
                     "LEFT OUTER JOIN UserAttributesViewedPromotions ON Notebooks.contactId = UserAttributesViewedPromotions.id "
                     "LEFT OUTER JOIN UserAttributesRecentMailedAddresses ON Notebooks.contactId = UserAttributesRecentMailedAddresses.id "
                     "LEFT OUTER JOIN Accounting ON Notebooks.contactId = Accounting.id "
                     "LEFT OUTER JOIN PremiumInfo ON Notebooks.contactId = PremiumInfo.id "
                     "LEFT OUTER JOIN BusinessUserInfo ON Notebooks.contactId = BusinessUserInfo.id";
    return result;
}

template <>
QString LocalStorageManagerPrivate::listObjectsGenericSqlQuery<Note>() const
{
    QString result = "SELECT * FROM Notes";
    return result;
}

template <>
QString LocalStorageManagerPrivate::orderByToSqlTableColumn<LocalStorageManager::ListNotesOrder::type>(const LocalStorageManager::ListNotesOrder::type & order) const
{
    QString result;

    switch(order)
    {
    case LocalStorageManager::ListNotesOrder::ByUpdateSequenceNumber:
        result = "updateSequenceNumber";
        break;
    case LocalStorageManager::ListNotesOrder::ByTitle:
        result = "title";
        break;
    case LocalStorageManager::ListNotesOrder::ByCreationTimestamp:
        result = "creationTimestamp";
        break;
    case LocalStorageManager::ListNotesOrder::ByModificationTimestamp:
        result = "modificationTimestamp";
        break;
    case LocalStorageManager::ListNotesOrder::ByDeletionTimestamp:
        result = "deletionTimestamp";
        break;
    case LocalStorageManager::ListNotesOrder::ByAuthor:
        result = "author";
        break;
    case LocalStorageManager::ListNotesOrder::BySource:
        result = "source";
        break;
    case LocalStorageManager::ListNotesOrder::BySourceApplication:
        result = "sourceApplication";
        break;
    case LocalStorageManager::ListNotesOrder::ByReminderTime:
        result = "reminderTime";
        break;
    case LocalStorageManager::ListNotesOrder::ByPlaceName:
        result = "placeName";
        break;
    default:
        break;
    }

    return result;
}

template <>
QString LocalStorageManagerPrivate::orderByToSqlTableColumn<LocalStorageManager::ListNotebooksOrder::type>(const LocalStorageManager::ListNotebooksOrder::type & order) const
{
    QString result;

    switch(order)
    {
    case LocalStorageManager::ListNotebooksOrder::ByUpdateSequenceNumber:
        result = "updateSequenceNumber";
        break;
    case LocalStorageManager::ListNotebooksOrder::ByNotebookName:
        result = "notebookNameUpper";
        break;
    case LocalStorageManager::ListNotebooksOrder::ByCreationTimestamp:
        result = "creationTimestamp";
        break;
    case LocalStorageManager::ListNotebooksOrder::ByModificationTimestamp:
        result = "modificationTimestamp";
        break;
    default:
        break;
    }

    return result;
}

template <>
QString LocalStorageManagerPrivate::orderByToSqlTableColumn<LocalStorageManager::ListLinkedNotebooksOrder::type>(const LocalStorageManager::ListLinkedNotebooksOrder::type & order) const
{
    QString result;

    switch(order)
    {
    case LocalStorageManager::ListLinkedNotebooksOrder::ByUpdateSequenceNumber:
        result = "updateSequenceNumber";
        break;
    case LocalStorageManager::ListLinkedNotebooksOrder::ByShareName:
        result = "shareName";
        break;
    case LocalStorageManager::ListLinkedNotebooksOrder::ByUsername:
        result = "username";
        break;
    default:
        break;
    }

    return result;
}

template <>
QString LocalStorageManagerPrivate::orderByToSqlTableColumn<LocalStorageManager::ListTagsOrder::type>(const LocalStorageManager::ListTagsOrder::type & order) const
{
    QString result;

    switch(order)
    {
    case LocalStorageManager::ListTagsOrder::ByUpdateSequenceNumber:
        result = "updateSequenceNumber";
        break;
    case LocalStorageManager::ListTagsOrder::ByName:
        result = "nameUpper";
        break;
    default:
        break;
    }

    return result;
}

template <>
QString LocalStorageManagerPrivate::orderByToSqlTableColumn<LocalStorageManager::ListSavedSearchesOrder::type>(const LocalStorageManager::ListSavedSearchesOrder::type & order) const
{
    QString result;

    switch(order)
    {
    case LocalStorageManager::ListSavedSearchesOrder::ByUpdateSequenceNumber:
        result = "updateSequenceNumber";
        break;
    case LocalStorageManager::ListSavedSearchesOrder::ByName:
        result = "nameUpper";
        break;
    case LocalStorageManager::ListSavedSearchesOrder::ByFormat:
        result = "format";
        break;
    default:
        break;
    }

    return result;
}

template <class T>
bool LocalStorageManagerPrivate::fillObjectsFromSqlQuery(QSqlQuery query, QList<T> & objects, QString & errorDescription) const
{
    objects.reserve(std::max(query.size(), 0));

    while(query.next())
    {
        QSqlRecord rec = query.record();

        objects << T();
        T & object = objects.back();

        bool res = fillObjectFromSqlRecord(rec, object, errorDescription);
        if (!res) {
            return false;
        }
    }

    return true;
}

template <>
bool LocalStorageManagerPrivate::fillObjectsFromSqlQuery<Notebook>(QSqlQuery query, QList<Notebook> & notebooks,
                                                                   QString & errorDescription) const
{
    QMap<QString, int> indexForLocalGuid;

    while(query.next())
    {
        QSqlRecord rec = query.record();

        int localGuidIndex = rec.indexOf("localGuid");
        if (localGuidIndex < 0) {
            errorDescription += QT_TR_NOOP("Internal error: no localGuid field in SQL record");
            QNWARNING(errorDescription);
            return false;
        }

        QVariant localGuidValue = rec.value(localGuidIndex);
        QString localGuid = localGuidValue.toString();
        if (localGuid.isEmpty()) {
            errorDescription += QT_TR_NOOP("Internal error: found empty localGuid field for Notebook");
            QNWARNING(errorDescription);
            return false;
        }

        auto it = indexForLocalGuid.find(localGuid);
        bool notFound = (it == indexForLocalGuid.end());
        if (notFound) {
            indexForLocalGuid[localGuid] = notebooks.size();
            notebooks << Notebook();
        }

        Notebook & notebook = (notFound ? notebooks.back() : notebooks[it.value()]);

        bool res = FillNotebookFromSqlRecord(rec, notebook, errorDescription);
        if (!res) {
            return false;
        }

        SortSharedNotebooks(notebook);
    }

    return true;
}

template<>
bool LocalStorageManagerPrivate::fillObjectFromSqlRecord<SavedSearch>(const QSqlRecord & rec, SavedSearch & search,
                                                                      QString & errorDescription) const
{
    return FillSavedSearchFromSqlRecord(rec, search, errorDescription);
}

template<>
bool LocalStorageManagerPrivate::fillObjectFromSqlRecord<Tag>(const QSqlRecord & rec, Tag & tag,
                                                              QString & errorDescription) const
{
    return FillTagFromSqlRecord(rec, tag, errorDescription);
}

template<>
bool LocalStorageManagerPrivate::fillObjectFromSqlRecord<LinkedNotebook>(const QSqlRecord & rec, LinkedNotebook & linkedNotebook,
                                                                         QString & errorDescription) const
{
    return FillLinkedNotebookFromSqlRecord(rec, linkedNotebook, errorDescription);
}

template <>
bool LocalStorageManagerPrivate::fillObjectFromSqlRecord<Notebook>(const QSqlRecord & rec, Notebook & notebook,
                                                                   QString & errorDescription) const
{
    return FillNotebookFromSqlRecord(rec, notebook, errorDescription);
}

template<>
bool LocalStorageManagerPrivate::fillObjectFromSqlRecord<Note>(const QSqlRecord & rec, Note & note,
                                                               QString & errorDescription) const
{
    Q_UNUSED(errorDescription);
    FillNoteFromSqlRecord(rec, note);
    return true;
}

template <class T, class TOrderBy>
QList<T> LocalStorageManagerPrivate::listObjects(const LocalStorageManager::ListObjectsOptions & flag,
                                                 QString & errorDescription, const size_t limit,
                                                 const size_t offset, const TOrderBy & orderBy,
                                                 const LocalStorageManager::OrderDirection::type & orderDirection,
                                                 const QString & additionalSqlQueryCondition) const
{
    QString flagError;
    QString sqlQueryConditions = listObjectsOptionsToSqlQueryConditions<T>(flag, flagError);
    if (sqlQueryConditions.isEmpty() && !flagError.isEmpty()) {
        errorDescription = flagError;
        return QList<T>();
    }

    QString sumSqlQueryConditions;
    if (!sqlQueryConditions.isEmpty()) {
        sumSqlQueryConditions += sqlQueryConditions;
    }

    if (!additionalSqlQueryCondition.isEmpty())
    {
        if (!sumSqlQueryConditions.isEmpty() && !sumSqlQueryConditions.endsWith(" AND ")) {
            sumSqlQueryConditions += " AND ";
        }

        sumSqlQueryConditions += additionalSqlQueryCondition;
    }

    if (sumSqlQueryConditions.endsWith(" AND ")) {
        sumSqlQueryConditions.chop(5);
    }

    QString queryString = listObjectsGenericSqlQuery<T>();
    if (!sumSqlQueryConditions.isEmpty()) {
        sumSqlQueryConditions.prepend("(");
        sumSqlQueryConditions.append(")");
        queryString += " WHERE ";
        queryString += sumSqlQueryConditions;
    }

    QString orderByColumn = orderByToSqlTableColumn<TOrderBy>(orderBy);
    if (!orderByColumn.isEmpty())
    {
        queryString += " ORDER BY ";
        queryString += orderByColumn;

        switch(orderDirection)
        {
            case LocalStorageManager::OrderDirection::Descending:
                queryString += " DESC";
                break;
            case LocalStorageManager::OrderDirection::Ascending:    // NOTE: intentional fall-through
            default:
                queryString += " ASC";
                break;
        }
    }

    if (limit != 0) {
        queryString += " LIMIT " + QString::number(limit);
    }

    if (offset != 0) {
        queryString += " OFFSET " + QString::number(offset);
    }

    QNDEBUG("SQL query string: " << queryString);

    QList<T> objects;

    QString errorPrefix = QT_TR_NOOP("Can't list objects in local storage by filter: ");
    QSqlQuery query(m_sqlDatabase);
    bool res = query.exec(queryString);
    if (!res) {
        errorDescription = errorPrefix + QT_TR_NOOP("can't list objects by filter from SQL database: ");
        QNCRITICAL(errorDescription << "last query = " << query.lastQuery() << ", last error = " << query.lastError());
        errorDescription += query.lastError().text();
        return objects;
    }

    res = fillObjectsFromSqlQuery(query, objects, errorDescription);
    if (!res) {
        errorDescription.prepend(errorPrefix);
        objects.clear();
        return objects;
    }

    QNDEBUG("found " << objects.size() << " objects");

    return objects;
}

bool LocalStorageManagerPrivate::SharedNotebookAdapterCompareByIndex::operator()(const SharedNotebookAdapter & lhs,
                                                                                 const SharedNotebookAdapter & rhs) const
{
    return (lhs.indexInNotebook() < rhs.indexInNotebook());
}

bool LocalStorageManagerPrivate::ResourceWrapperCompareByIndex::operator()(const ResourceWrapper & lhs,
                                                                           const ResourceWrapper & rhs) const
{
    return (lhs.indexInNote() < rhs.indexInNote());
}

bool LocalStorageManagerPrivate::QStringIntPairCompareByInt::operator()(const QPair<QString, int> & lhs,
                                                                        const QPair<QString, int> & rhs) const
{
    return (lhs.second < rhs.second);
}

#undef CHECK_AND_SET_RESOURCE_PROPERTY
#undef CHECK_AND_SET_NOTEBOOK_ATTRIBUTE
#undef CHECK_AND_SET_EN_NOTEBOOK_ATTRIBUTE
#undef SET_IS_FREE_ACCOUNT_FLAG
#undef CHECK_AND_SET_EN_RESOURCE_PROPERTY
#undef CHECK_AND_SET_NOTE_PROPERTY
#undef DATABASE_CHECK_AND_SET_ERROR

} // namespace qute_note

