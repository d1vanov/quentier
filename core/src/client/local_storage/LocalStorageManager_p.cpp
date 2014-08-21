#include "LocalStorageManager_p.h"
#include "DatabaseOpeningException.h"
#include "DatabaseSqlErrorException.h"
#include "Transaction.h"
#include "NoteSearchQuery.h"
#include <client/Utility.h>
#include <logging/QuteNoteLogger.h>
#include <tools/QuteNoteNullPtrException.h>
#include <tools/ApplicationStoragePersistencePath.h>
#include <tools/SysInfo.h>

namespace qute_note {

#define QUTE_NOTE_DATABASE_NAME "qn.storage.sqlite"

LocalStorageManagerPrivate::LocalStorageManagerPrivate(const QString & username, const UserID userId,
                                         const bool startFromScratch) :
    // NOTE: don't initialize these! Otherwise SwitchUser won't work right
    m_currentUsername(),
    m_currentUserId(),
    m_applicationPersistenceStoragePath()
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

    error.clear();
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

    error.clear();
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

    QSqlQuery query(m_sqlDatabase);
    query.prepare("UPDATE Users SET userDeletionTimestamp = ?, userIsLocal = ? WHERE id = ?");
    query.addBindValue(user.deletionTimestamp());
    query.addBindValue(user.isLocal() ? 1 : 0);
    query.addBindValue(user.id());

    bool res = query.exec();
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

    QSqlQuery query(m_sqlDatabase);
    query.prepare("DELETE FROM Users WHERE id = ?");
    query.addBindValue(QString::number(user.id()));

    bool res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("can't delete entry from \"Users\" table in SQL database");

    return true;
}

int LocalStorageManagerPrivate::GetNotebookCount(QString & errorDescription) const
{
    QSqlQuery query(m_sqlDatabase);
    bool res = query.exec("SELECT COUNT(*) FROM Notebooks");
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

    m_applicationPersistenceStoragePath = GetApplicationPersistentStoragePath();
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
        for(const QString & driver: drivers) {
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
    QSqlQuery query(m_sqlDatabase);
    bool res = query.exec("SELECT COUNT(*) FROM Users WHERE userDeletionTimestamp IS NULL");
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

    QString column, guid;
    bool notebookHasGuid = notebook.hasGuid();
    if (notebookHasGuid) {
        column = "guid";
        guid = notebook.guid();
        if (!CheckGuid(guid)) {
            // TRANSLATOR explaining the reason of error
            errorDescription += QT_TR_NOOP("requested guid is invalid");
            QNWARNING(errorDescription);
            return false;
        }
    }
    else { // whichGuid == WhichGuid::EverCloudGuid
        column = "localGuid";
        guid = notebook.localGuid();
    }

    notebook = Notebook();

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
                                  "WHERE Notebooks.%1 = '%2'").arg(column).arg(guid);
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

QList<Notebook> LocalStorageManagerPrivate::ListAllNotebooks(QString & errorDescription) const
{
    QNDEBUG("LocalStorageManagerPrivate::ListAllNotebooks");

    QList<Notebook> notebooks;
    errorDescription = QT_TR_NOOP("Can't list all notebooks in local storage database: ");

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
                          "LEFT OUTER JOIN BusinessUserInfo ON Notebooks.contactId = BusinessUserInfo.id");
    if (!res) {
        // TRANSLATOR explaining the reason of error
        errorDescription += QT_TR_NOOP("can't select all notebooks from SQL database: ");
        QNCRITICAL(errorDescription << "last error = " << query.lastError() << ", last query = " << query.lastQuery());
        errorDescription += query.lastError().text();
        return notebooks;
    }

    QMap<QString, int> indexForLocalGuid;

    while(query.next())
    {
        QSqlRecord rec = query.record();

        int localGuidIndex = rec.indexOf("localGuid");
        if (localGuidIndex < 0) {
            errorDescription += QT_TR_NOOP("Internal error: no localGuid field in SQL record");
            QNWARNING(errorDescription);
            notebooks.clear();
            return notebooks;
        }

        QVariant localGuidValue = rec.value(localGuidIndex);
        QString localGuid = localGuidValue.toString();
        if (localGuid.isEmpty()) {
            errorDescription += QT_TR_NOOP("Internal error: found empty localGuid field for Notebook");
            QNWARNING(errorDescription);
            notebooks.clear();
            return notebooks;
        }

        auto it = indexForLocalGuid.find(localGuid);
        bool notFound = (it == indexForLocalGuid.end());
        if (notFound) {
            indexForLocalGuid[localGuid] = notebooks.size();
            notebooks << Notebook();
        }

        Notebook & notebook = (notFound ? notebooks.back() : notebooks[it.value()]);

        res = FillNotebookFromSqlRecord(rec, notebook, errorDescription);
        if (!res) {
            notebooks.clear();
            return notebooks;
        }

        SortSharedNotebooks(notebook);
    }

    int numNotebooks = notebooks.size();
    QNDEBUG("found " << numNotebooks << " notebooks");

    if (numNotebooks <= 0) {
        // TRANSLATOR explaining the reason of error
        errorDescription += QT_TR_NOOP("no notebooks exist in local storage");
        QNDEBUG(errorDescription);
    }

    return notebooks;
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

    qSort(sharedNotebookAdapters.begin(), sharedNotebookAdapters.end(),
          [](const SharedNotebookAdapter & lhs, const SharedNotebookAdapter & rhs)
          { return lhs.indexInNotebook() < rhs.indexInNotebook(); });

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
    QSqlQuery query(m_sqlDatabase);
    bool res = query.exec("SELECT COUNT(*) FROM LinkedNotebooks");
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

QList<LinkedNotebook> LocalStorageManagerPrivate::ListAllLinkedNotebooks(QString & errorDescription) const
{
    QNDEBUG("LocalStorageManagerPrivate::ListAllLinkedNotebooks");

    QList<LinkedNotebook> notebooks;
    QString errorPrefix = QT_TR_NOOP("Can't list all linked notebooks in local storage database: ");

    QSqlQuery query(m_sqlDatabase);
    bool res = query.exec("SELECT * FROM LinkedNotebooks");
    if (!res) {
        // TRANSLATOR explaining the reason of error
        errorDescription = errorPrefix + QT_TR_NOOP("can't select all linked notebooks from SQL database: ");
        QNCRITICAL(errorDescription << "last error = " << query.lastError() << ", last query = " << query.lastQuery());
        errorDescription += query.lastError().text();
        return notebooks;
    }

    notebooks.reserve(qMax(query.size(), 0));
    while(query.next())
    {
        QSqlRecord rec = query.record();

        notebooks << LinkedNotebook();
        LinkedNotebook & notebook = notebooks.back();

        res = FillLinkedNotebookFromSqlRecord(rec, notebook, errorDescription);
        if (!res) {
            errorDescription.prepend(errorPrefix);
            notebooks.clear();
            return notebooks;
        }
    }

    QNDEBUG("found " << notebooks.size() << " notebooks");

    return notebooks;
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

    QSqlQuery query(m_sqlDatabase);
    query.prepare("DELETE FROM LinkedNotebooks WHERE guid = ?");
    query.addBindValue(QVariant(linkedNotebookGuid));

    bool res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("can't delete entry from \"LinkedNotebooks\" table in SQL database");

    return true;
}

int LocalStorageManagerPrivate::GetNoteCount(QString & errorDescription) const
{
    QSqlQuery query(m_sqlDatabase);
    bool res = query.exec("SELECT COUNT(*) FROM Notes WHERE deletionTimestamp IS NULL");
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
        qSort(resources.begin(), resources.end(), [](const ResourceWrapper & lhs, const ResourceWrapper & rhs)
                                                    { return lhs.indexInNote() < rhs.indexInNote(); });
        note.setResources(resources);
    }

    int numTags = tagGuidsAndIndices.size();
    if (numTags > 0) {
        qSort(tagGuidsAndIndices.begin(), tagGuidsAndIndices.end(), [](const QPair<QString, int> & lhs, const QPair<QString, int> & rhs)
                                                                       { return lhs.second < rhs.second; });
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
                                                                const bool withResourceBinaryData) const
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

    QString queryString = QString("SELECT * FROM Notes WHERE %1 = '%2'").arg(column).arg(guid);
    QSqlQuery query(m_sqlDatabase);
    bool res = query.exec(queryString);
    if (!res) {
        errorDescription = errorPrefix + QT_TR_NOOP("can't select notes per notebook guid from SQL database: ");
        QNCRITICAL(errorDescription << "last error = " << query.lastError() << ", last query = " << query.lastQuery());
        errorDescription += query.lastError().text();
        return notes;
    }

    notes.reserve(qMax(query.size(), 0));
    QString error;

    while(query.next())
    {
        notes << Note();
        Note & note = notes.back();

        QSqlRecord rec = query.record();
        FillNoteFromSqlRecord(rec, note);

        res = FindAndSetTagGuidsPerNote(note, error);
        if (!res) {
            errorDescription += error;
            QNWARNING(errorDescription);
            notes.clear();
            return notes;
        }

        error.clear();
        res = FindAndSetResourcesPerNote(note, error, withResourceBinaryData);
        if (!res) {
            errorDescription += error;
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

    QString queryString;

    // Will run all the queries from this method and its sub-methods within a single transaction
    // to prevent multiple drops and re-obtainings of shared lock
    Transaction transaction(m_sqlDatabase, Transaction::Selection);
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
        notes.push_back(QSharedPointer<Note>(new Note));
        Note & note = *(notes.back());
        note.setLocalGuid(QString());

        QSqlRecord rec = query.record();
        FillNoteFromSqlRecord(rec, note);

        res = FindAndSetTagGuidsPerNote(note, error);
        if (!res) {
            errorDescription += error;
            QNWARNING(errorDescription);
            return NoteList();
        }

        error.clear();
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

    errorDescription.clear();
    return notes;
}

int LocalStorageManagerPrivate::GetTagCount(QString & errorDescription) const
{
    QSqlQuery query(m_sqlDatabase);
    bool res = query.exec("SELECT COUNT(*) FROM Tags WHERE isDeleted = 0");
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

    QString column, guid;
    bool tagHasGuid = tag.hasGuid();
    if (tagHasGuid) {
        column = "guid";
        guid = tag.guid();

        if (!CheckGuid(guid)) {
            // TRANSLATION explaining why tag cannot be found in local storage
            errorDescription = errorPrefix + QT_TR_NOOP("requested tag guid is invalid");
            QNWARNING(errorDescription);
            return false;
        }
    }
    else {
        column = "localGuid";
        guid = tag.localGuid();
    }

    tag.clear();

    QString queryString = QString("SELECT localGuid, guid, updateSequenceNumber, name, parentGuid, isDirty, isLocal, "
                                  "isDeleted, hasShortcut FROM Tags WHERE %1 = '%2'").arg(column).arg(guid);
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

QList<Tag> LocalStorageManagerPrivate::ListAllTagsPerNote(const Note & note, QString & errorDescription) const
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

    QString queryString = QString("SELECT localTag FROM NoteTags WHERE %1 = '%2'").arg(column).arg(guid);
    QSqlQuery query(m_sqlDatabase);
    bool res = query.exec(queryString);
    if (!res) {
        errorDescription = errorPrefix + QT_TR_NOOP("can't select tag guids from \"NoteTags\" table in SQL database: ");
        QNWARNING(errorDescription << "last error = " << query.lastError() << ", last query = " << query.lastQuery());
        errorDescription += query.lastError().text();
        return tags;
    }

    tags = FillTagsFromSqlQuery(query, errorDescription);
    if (tags.isEmpty() && !errorDescription.isEmpty()) {
        errorDescription.prepend(errorPrefix);
        QNWARNING(errorDescription);
    }

    QNDEBUG("found " << tags.size() << " tags");

    return tags;
}

QList<Tag> LocalStorageManagerPrivate::ListAllTags(QString & errorDescription) const
{
    QNDEBUG("LocalStorageManagerPrivate::ListAllTags");

    QList<Tag> tags;
    QString errorPrefix = QT_TR_NOOP("Can't list all tags from local storage database: ");

    QSqlQuery query(m_sqlDatabase);
    bool res = query.exec("SELECT localGuid FROM Tags");
    if (!res) {
        // TRANSLATOR explaining why all tag from local storage cannot be listed
        errorDescription = errorPrefix + QT_TR_NOOP("can't select tag guids from SQL database: ");
        QNWARNING(errorDescription << "last error = " << query.lastError() << ", last query = " << query.lastQuery());
        errorDescription += query.lastError().text();
        return tags;
    }

    tags = FillTagsFromSqlQuery(query, errorDescription);
    if (tags.isEmpty() && !errorDescription.isEmpty()) {
        errorDescription.prepend(errorPrefix);
        QNWARNING(errorDescription);
    }

    QNDEBUG("found " << tags.size() << " tags");

    return tags;
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

    QSqlQuery query(m_sqlDatabase);
    query.prepare("UPDATE Tags SET isDeleted = 1, isDirty = 1 WHERE localGuid = ?");
    query.addBindValue(tag.localGuid());
    bool res = query.exec();
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

    QSqlQuery query(m_sqlDatabase);
    query.prepare("DELETE FROM Tags WHERE localGuid = ?");
    query.addBindValue(tag.localGuid());

    bool res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("can't delete tag from \"Tags\" table in SQL database");

    return true;
}

int LocalStorageManagerPrivate::GetEnResourceCount(QString & errorDescription) const
{
    QSqlQuery query(m_sqlDatabase);
    bool res = query.exec("SELECT COUNT(*) FROM Resources");
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
    QSqlQuery query(m_sqlDatabase);
    bool res = query.exec("SELECT COUNT(*) FROM SavedSearches");
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

    QString column, guid;
    bool searchHasGuid = search.hasGuid();
    if (searchHasGuid) {
        column = "guid";
        guid = search.guid();

        if (!CheckGuid(guid)) {
            errorDescription += QT_TR_NOOP("requested saved search guid is invalid");
            return false;
        }
    }
    else {
        column = "localGuid";
        guid = search.localGuid();
    }

    search.clear();

    QString queryString = QString("SELECT localGuid, guid, name, query, format, updateSequenceNumber, isDirty, "
                                  "includeAccount, includePersonalLinkedNotebooks, includeBusinessLinkedNotebooks, "
                                  "hasShortcut FROM SavedSearches WHERE %1 = '%2'").arg(column).arg(guid);
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

QList<SavedSearch> LocalStorageManagerPrivate::ListAllSavedSearches(QString & errorDescription) const
{
    QNDEBUG("LocalStorageManagerPrivate::ListAllSavedSearches");

    QList<SavedSearch> searches;

    QString errorPrefix = QT_TR_NOOP("Can't list all saved searches in local storage database");

    QSqlQuery query(m_sqlDatabase);
    bool res = query.exec("SELECT * FROM SavedSearches");
    if (!res) {
        errorDescription = errorPrefix + QT_TR_NOOP("can't select all saved searches from "
                                                    "\"SavedSearches\" table in SQL database: ");
        QNCRITICAL(errorDescription << "last error = " << query.lastError() << ", last error = " << query.lastError());
        errorDescription += query.lastError().text();
        return searches;
    }

    searches.reserve(qMax(query.size(), 0));

    while(query.next())
    {
        QSqlRecord rec = query.record();

        searches << SavedSearch();
        SavedSearch & search = searches.back();

        res = FillSavedSearchFromSqlRecord(rec, search, errorDescription);
        if (!res) {
            errorDescription.prepend(errorPrefix);
            searches.clear();
            return searches;
        }
    }

    QNDEBUG("found " << searches.size() << " saved searches");

    return searches;
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

    QSqlQuery query(m_sqlDatabase);
    query.prepare("DELETE FROM SavedSearches WHERE localGuid = ?");
    query.addBindValue(localGuid);

    bool res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("can't delete saved search from \"SavedSearches\" table in SQL database");

    return true;
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

    res = query.exec("CREATE TABLE IF NOT EXISTS Notebooks("
                     "  localGuid                       TEXT PRIMARY KEY  NOT NULL UNIQUE, "
                     "  guid                            TEXT              DEFAULT NULL UNIQUE, "
                     "  updateSequenceNumber            INTEGER           DEFAULT NULL, "
                     "  notebookName                    TEXT              DEFAULT NULL UNIQUE, "
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
                     "  UNIQUE(localGuid, guid) "
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

    res = query.exec("CREATE VIRTUAL TABLE IF NOT EXISTS NoteText USING FTS4 "
                     "(localGuid, guid, title, content, contentPlainText, contentListOfWords)");
    DATABASE_CHECK_AND_SET_ERROR("can't create virtual table NoteText");

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

    res = query.exec("CREATE TABLE IF NOT EXISTS ResourceRecognitionTypes("
                     "  resourceLocalGuid REFERENCES Resources(resourceLocalGuid) ON DELETE CASCADE ON UPDATE CASCADE, "
                     "  recognitionType                 TEXT                DEFAULT NULL)");
    DATABASE_CHECK_AND_SET_ERROR("can't create ResourceRecognitionTypes table");

    res = query.exec("CREATE VIRTUAL TABLE ResourceRecoTypesFTS USING FTS4(content=\"ResourceRecognitionTypes\", recognitionType)");
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

    res = query.exec("CREATE VIRTUAL TABLE ResourceMimeFTS USING FTS4(content=\"Resources\", mime)");
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
                     "  updateSequenceNumber  INTEGER              DEFAULT NULL, "
                     "  name                  TEXT                 DEFAULT NULL, "
                     "  nameUpper             TEXT                 DEFAULT NULL UNIQUE, "
                     "  parentGuid            TEXT                 DEFAULT NULL, "
                     "  isDirty               INTEGER              NOT NULL, "
                     "  isLocal               INTEGER              NOT NULL, "
                     "  isDeleted             INTEGER              NOT NULL, "
                     "  hasShortcut           INTEGER              NOT NULL "
                     ")");
    DATABASE_CHECK_AND_SET_ERROR("can't create Tags table");

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
                     "  includeAccount                  INTEGER             NOT NULL, "
                     "  includePersonalLinkedNotebooks  INTEGER             NOT NULL, "
                     "  includeBusinessLinkedNotebooks  INTEGER             NOT NULL, "
                     "  hasShortcut                     INTEGER             NOT NULL, "
                     "  UNIQUE(localGuid, guid))");
    DATABASE_CHECK_AND_SET_ERROR("can't create SavedSearches table");

    return true;
}

bool LocalStorageManagerPrivate::InsertOrReplaceNotebookRestrictions(const qevercloud::NotebookRestrictions & notebookRestrictions,
                                                                     const QString & localGuid, QString & errorDescription)
{
    errorDescription = QT_TR_NOOP("Can't insert or replace notebook restrictions: ");

    QString columns = "localGuid, noReadNotes, noCreateNotes, noUpdateNotes, "
                      "noExpungeNotes, noShareNotes, noEmailNotes, noSendMessageToRecipients, "
                      "noUpdateNotebook, noExpungeNotebook, noSetDefaultNotebook, "
                      "noSetNotebookStack, noPublishToPublic, noPublishToBusinessLibrary, "
                      "noCreateTags, noUpdateTags, noExpungeTags, noSetParentTag, "
                      "noCreateSharedNotebooks, updateWhichSharedNotebookRestrictions, "
                      "expungeWhichSharedNotebookRestrictions";

    QString values = ":localGuid, :noReadNotes, :noCreateNotes, :noUpdateNotes, "
                     ":noExpungeNotes, :noShareNotes, :noEmailNotes, :noSendMessageToRecipients, "
                     ":noUpdateNotebook, :noExpungeNotebook, :noSetDefaultNotebook, "
                     ":noSetNotebookStack, :noPublishToPublic, :noPublishToBusinessLibrary, "
                     ":noCreateTags, :noUpdateTags, :noExpungeTags, :noSetParentTag, "
                     ":noCreateSharedNotebooks, :updateWhichSharedNotebookRestrictions, "
                     ":expungeWhichSharedNotebookRestrictions";

    QString queryString = QString("INSERT OR REPLACE INTO NotebookRestrictions(%1) VALUES(%2)").arg(columns).arg(values);
    QSqlQuery query(m_sqlDatabase);
    bool res = query.prepare(queryString);
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

    QString columns = "shareId, userId, notebookGuid, sharedNotebookEmail, sharedNotebookCreationTimestamp, "
                      "sharedNotebookModificationTimestamp, shareKey, sharedNotebookUsername, "
                      "sharedNotebookPrivilegeLevel, allowPreview, recipientReminderNotifyEmail, "
                      "recipientReminderNotifyInApp, indexInNotebook";

    QString values = ":shareId, :userId, :notebookGuid, :sharedNotebookEmail, :sharedNotebookCreationTimestamp, "
                     ":sharedNotebookModificationTimestamp, :shareKey, :sharedNotebookUsername, "
                     ":sharedNotebookPrivilegeLevel, :allowPreview, :recipientReminderNotifyEmail, "
                     ":recipientReminderNotifyInApp, :indexInNotebook";

    QString queryString = QString("INSERT OR REPLACE INTO SharedNotebooks(%1) VALUES(%2)").arg(columns).arg(values);
    QSqlQuery query(m_sqlDatabase);
    bool res = query.prepare(queryString);
    DATABASE_CHECK_AND_SET_ERROR("can't insert or replace data into \"SharedNotebooks\" table in SQL database: "
                                 "can't prepare SQL query");

    QVariant nullValue;

    query.bindValue(":shareId", sharedNotebook.id());
    query.bindValue(":userId", (sharedNotebook.hasUserId() ? sharedNotebook.userId() : nullValue));
    query.bindValue(":notebookGuid", (sharedNotebook.hasNotebookGuid() ? sharedNotebook.notebookGuid() : nullValue));
    query.bindValue(":sharedNotebookEmail", (sharedNotebook.hasEmail() ? sharedNotebook.email() : nullValue));
    query.bindValue(":sharedNotebookCreationTimestamp", (sharedNotebook.hasCreationTimestamp() ? sharedNotebook.creationTimestamp() : nullValue));
    query.bindValue(":sharedNotebokModificationTimestamp", (sharedNotebook.hasModificationTimestamp() ? sharedNotebook.modificationTimestamp() : nullValue));
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

    QString columns = "id, username, email, name, timezone, privilege, userCreationTimestamp, "
                      "userModificationTimestamp, userIsDirty, userIsLocal, userIsActive, userDeletionTimestamp";
    QString values = ":id, :username, :email, :name, :timezone, :privilege, :userCreationTimestamp, "
                     ":userModificationTimestamp, :userIsDirty, :userIsLocal, :userIsActive, :userDeletionTimestamp";

    Transaction transaction(m_sqlDatabase, Transaction::Exclusive);

    QString queryString = QString("INSERT OR REPLACE INTO Users(%1) VALUES(%2)").arg(columns).arg(values);
    QSqlQuery query(m_sqlDatabase);
    bool res = query.prepare(queryString);
    DATABASE_CHECK_AND_SET_ERROR("can't insert or replace data into \"Users\" table in SQL database: "
                                 "can't prepare SQL query");

    QVariant nullValue;

    query.bindValue(":id", user.id());
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

    if (user.hasUserAttributes())
    {
        res = InsertOrReplaceUserAttributes(user.id(), user.userAttributes(), errorDescription);
        if (!res) {
            return false;
        }
    }

    if (user.hasAccounting())
    {
        res = InsertOrReplaceAccounting(user.id(), user.accounting(), errorDescription);
        if (!res) {
            return false;
        }
    }

    if (user.hasPremiumInfo())
    {
        res = InsertOrReplacePremiumInfo(user.id(), user.premiumInfo(), errorDescription);
        if (!res) {
            return false;
        }
    }

    if (user.hasBusinessUserInfo()) {
        res = InsertOrReplaceBusinesUserInfo(user.id(), user.businessUserInfo(), errorDescription);
        if (!res) {
            return false;
        }
    }

    return transaction.commit(errorDescription);
}

bool LocalStorageManagerPrivate::InsertOrReplaceBusinesUserInfo(const UserID id, const qevercloud::BusinessUserInfo & info,
                                                                QString & errorDescription)
{
    QString columns = "id, businessId, businessName, role, businessInfoEmail";
    QString values = ":id, :businessId, :businessName, :role, :businessInfoEmail";

    QString queryString = QString("INSERT OR REPLACE INTO BusinessUserInfo (%1) VALUES(%2)")
                                  .arg(columns).arg(values);
    QSqlQuery query(m_sqlDatabase);
    bool res = query.prepare(queryString);
    DATABASE_CHECK_AND_SET_ERROR("can't set user's business info into \"BusinessUserInfo\" table in SQL database");

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
    QString columns = "id, currentTime, premium, premiumRecurring, premiumExtendable, "
                      "premiumPending, premiumCancellationPending, canPurchaseUploadAllowance, "
                      "premiumExpirationDate, sponsoredGroupName, sponsoredGroupRole, premiumUpgradable";
    QString valuesString = ":id, :currentTime, :premium, :premiumRecurring, :premiumExtendable, "
                           ":premiumPending, :premiumCancellationPending, :canPurchaseUploadAllowance, "
                           ":premiumExpirationDate, :sponsoredGroupName, :sponsoredGroupRole, :premiumUpgradable";

    QString queryString = QString("INSERT OR REPLACE INTO PremiumInfo (%1) VALUES(%2)")
                                  .arg(columns).arg(valuesString);
    QSqlQuery query(m_sqlDatabase);
    bool res = query.prepare(queryString);
    DATABASE_CHECK_AND_SET_ERROR("can't set user's premium info into \"PremiumInfo\" table in SQL database");

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
    QString columns = "id, uploadLimit, uploadLimitEnd, uploadLimitNextMonth, "
                      "premiumServiceStatus, premiumOrderNumber, premiumCommerceService, "
                      "premiumServiceStart, premiumServiceSKU, lastSuccessfulCharge, "
                      "lastFailedCharge, lastFailedChargeReason, nextPaymentDue, premiumLockUntil, "
                      "updated, premiumSubscriptionNumber, lastRequestedCharge, currency, "
                      "unitPrice, unitDiscount, nextChargeDate, accountingBusinessId, "
                      "accountingBusinessName, accountingBusinessRole";

    QString values = ":id, :uploadLimit, :uploadLimitEnd, :uploadLimitNextMonth, "
                     ":premiumServiceStatus, :premiumOrderNumber, :premiumCommerceService, "
                     ":premiumServiceStart, :premiumServiceSKU, :lastSuccessfulCharge, "
                     ":lastFailedCharge, :lastFailedChargeReason, :nextPaymentDue, :premiumLockUntil, "
                     ":updated, :premiumSubscriptionNumber, :lastRequestedCharge, :currency, "
                     ":unitPrice, :unitDiscount, :nextChargeDate, :accountingBusinessId, "
                     ":accountingBusinessName, :accountingBusinessRole";

    QString queryString = QString("INSERT OR REPLACE INTO Accounting (%1) VALUES(%2)")
                                  .arg(columns).arg(values);
    QSqlQuery query(m_sqlDatabase);
    bool res = query.prepare(queryString);
    DATABASE_CHECK_AND_SET_ERROR("can't set user's acconting into \"Accounting\" table in SQL database");

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
    QString columns = "id, defaultLocationName, defaultLatitude, defaultLongitude, preactivation, "
                      "incomingEmailAddress, comments, dateAgreedToTermsOfService, maxReferrals, "
                      "referralCount, refererCode, sentEmailDate, sentEmailCount, dailyEmailLimit, "
                      "emailOptOutDate, partnerEmailOptInDate, preferredLanguage, preferredCountry, "
                      "clipFullPage, twitterUserName, twitterId, groupName, recognitionLanguage, "
                      "referralProof, educationalDiscount, businessAddress, hideSponsorBilling, "
                      "taxExempt, useEmailAutoFiling, reminderEmailConfig";

    QString values = ":id, :defaultLocationName, :defaultLatitude, :defaultLongitude, :preactivation, "
                     ":incomingEmailAddress, :comments, :dateAgreedToTermsOfService, :maxReferrals, "
                     ":referralCount, :refererCode, :sentEmailDate, :sentEmailCount, :dailyEmailLimit, "
                     ":emailOptOutDate, :partnerEmailOptInDate, :preferredLanguage, :preferredCountry, "
                     ":clipFullPage, :twitterUserName, :twitterId, :groupName, :recognitionLanguage, "
                     ":referralProof, :educationalDiscount, :businessAddress, :hideSponsorBilling, "
                     ":taxExempt, :useEmailAutoFiling, :reminderEmailConfig";

    QString queryString = QString("INSERT OR REPLACE INTO UserAttributes (%1) VALUES(%2)")
                                  .arg(columns).arg(values);
    QSqlQuery query(m_sqlDatabase);
    bool res = query.prepare(queryString);
    DATABASE_CHECK_AND_SET_ERROR("can't set user's attributes into \"UserAttributes\" table in SQL database");

    query.bindValue(":id", id);

    QVariant nullValue;

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

    if (attributes.viewedPromotions.isSet()) {
        foreach(const QString & promotion, attributes.viewedPromotions.ref()) {
            queryString = QString("INSERT OR REPLACE INTO UserAttributesViewedPromotions (id, promotion) VALUES('%1', '%2')")
                                  .arg(id).arg(promotion);
            res = query.exec(queryString);
            DATABASE_CHECK_AND_SET_ERROR("can't set user's attributes into \"UserAttributesViewedPromotions\" table in SQL database");
        }
    }
    else {
        queryString = QString("DELETE FROM UserAttributesViewedPromotions WHERE id = %1").arg(QString::number(id));
        res = query.exec(queryString);
        DATABASE_CHECK_AND_SET_ERROR("can't set user's attributes into \"UserAttributesViewedPromotions\" table in SQL database");
    }

    if (attributes.recentMailedAddresses.isSet()) {
        foreach(const QString & address, attributes.recentMailedAddresses.ref()) {
            queryString = QString("INSERT OR REPLACE INTO UserAttributesRecentMailedAddresses (id, address) VALUES('%1', '%2')")
                                  .arg(id).arg(address);
            res = query.exec(queryString);
            DATABASE_CHECK_AND_SET_ERROR("can't set user's attributes into \"UserAttributesRecentMailedAddresses\" table in SQL database");
        }
    }
    else {
        queryString = QString("DELETE FROM UserAttributesRecentMailedAddresses WHERE id = %1").arg(QString::number(id));
        res = query.exec(queryString);
        DATABASE_CHECK_AND_SET_ERROR("can't set user's attributes into \"UserAttributesRecentMailedAddresses\" table in SQL database");
    }

    return true;
}

bool LocalStorageManagerPrivate::InsertOrReplaceNotebook(const Notebook & notebook,
                                                         const QString & overrideLocalGuid,
                                                         QString & errorDescription)
{
    // NOTE: this method expects to be called after notebook is already checked
    // for sanity of its parameters!

    errorDescription = QT_TR_NOOP("Can't insert or replace notebook into local storage database: ");

    QString columns = "localGuid, guid, updateSequenceNumber, notebookName, creationTimestamp, "
                      "modificationTimestamp, isDirty, isLocal, isDefault, isLastUsed, "
                      "hasShortcut, publishingUri, publishingNoteSortOrder, publishingAscendingSort, "
                      "publicDescription, isPublished, stack, businessNotebookDescription, "
                      "businessNotebookPrivilegeLevel, businessNotebookIsRecommended, contactId";

    QString values = ":localGuid, :guid, :updateSequenceNumber, :notebookName, :creationTimestamp, "
                     ":modificationTimestamp, :isDirty, :isLocal, :isDefault, :isLastUsed, "
                     ":hasShortcut, :publishingUri, :publishingNoteSortOrder, :publishingAscendingSort, "
                     ":publicDescription, :isPublished, :stack, :businessNotebookDescription, "
                     ":businessNotebookPrivilegeLevel, :businessNotebookIsRecommended, :contactId";

    Transaction transaction(m_sqlDatabase, Transaction::Exclusive);

    QString queryString = QString("INSERT OR REPLACE INTO Notebooks(%1) VALUES(%2)").arg(columns).arg(values);
    QSqlQuery query(m_sqlDatabase);
    bool res = query.prepare(queryString);
    DATABASE_CHECK_AND_SET_ERROR("can't insert or replace data into \"Notebooks\" table of SQL database: "
                                 "can't prepare SQL query");

    QString localGuid = (overrideLocalGuid.isEmpty() ? notebook.localGuid() : overrideLocalGuid);

    QVariant nullValue;

    query.bindValue(":localGuid", localGuid);
    query.bindValue(":guid", (notebook.hasGuid() ? notebook.guid() : nullValue));
    query.bindValue(":updateSequenceNumber", (notebook.hasUpdateSequenceNumber() ? notebook.updateSequenceNumber() : nullValue));
    query.bindValue(":notebookName", (notebook.hasName() ? notebook.name() : nullValue));
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

    if (notebook.hasRestrictions())
    {
        QString error;
        res = InsertOrReplaceNotebookRestrictions(notebook.restrictions(), localGuid, error);
        if (!res) {
            errorDescription += error;
            return res;
        }
    }

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

    return transaction.commit(errorDescription);
}

bool LocalStorageManagerPrivate::InsertOrReplaceLinkedNotebook(const LinkedNotebook & linkedNotebook,
                                                               QString & errorDescription)
{
    // NOTE: this method expects to be called after the linked notebook
    // is already checked for sanity ot its parameters

    errorDescription += QT_TR_NOOP("can't insert or replace linked notebook into "
                                   "local storage database");

    QString columns = "guid, updateSequenceNumber, shareName, username, shardId, shareKey, "
                      "uri, noteStoreUrl, webApiUrlPrefix, stack, businessId, isDirty";

    QString values = ":guid, :updateSequenceNumber, :shareName, :username, :shardId, :shareKey, "
                     ":uri, :noteStoreUrl, :webApiUrlPrefix, :stack, :businessId, :isDirty";

    QString queryString = QString("INSERT OR REPLACE INTO LinkedNotebooks (%1) VALUES(%2)").arg(columns).arg(values);
    QSqlQuery query(m_sqlDatabase);
    bool res = query.prepare(queryString);
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

bool LocalStorageManagerPrivate::InsertOrReplaceNote(const Note & note, const Notebook & notebook,
                                                     const QString & overrideLocalGuid, QString & errorDescription)
{
    // NOTE: this method expects to be called after the note is already checked
    // for sanity of its parameters!

    errorDescription += QT_TR_NOOP("can't insert or replace note into local storage database: ");

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

    Transaction transaction(m_sqlDatabase, Transaction::Exclusive);

    QString queryString = QString("INSERT OR REPLACE INTO Notes(%1) VALUES(%2)").arg(columns).arg(values);
    QSqlQuery query(m_sqlDatabase);
    bool res = query.prepare(queryString);
    DATABASE_CHECK_AND_SET_ERROR("Can't insert data into \"Notes\" table in SQL database: can't prepare SQL query");

    QString localGuid = (overrideLocalGuid.isEmpty() ? note.localGuid() : overrideLocalGuid);

    QVariant nullValue;

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

        error.clear();
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
                    keysOnlyString += "\0";
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
                    fullMapKeysString += "\0";

                    fullMapValuesString += "'";
                    fullMapValuesString += it.value();
                    fullMapValuesString += "'";
                    fullMapValuesString += "\0";
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
                classificationKeys += "\0";

                classificationValues += "'";
                classificationValues += it.value();
                classificationValues += "'";
                classificationValues += "\0";
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

    if (note.hasTagGuids())
    {
        QString error;

        QStringList tagGuids;
        note.tagGuids(tagGuids);
        int numTagGuids = tagGuids.size();

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

            columns = "localNote, note, localTag, tag, tagIndexInNote";
            values = ":localNote, :note, :localTag, :tag, :tagIndexInNote";

            queryString = QString("INSERT OR REPLACE INTO NoteTags(%1) VALUES(%2)").arg(columns).arg(values);

            res = query.prepare(queryString);
            DATABASE_CHECK_AND_SET_ERROR("can't insert or replace data into \"NoteTags\" table: can't prepare SQL query");

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

    if (note.hasResources())
    {
        QList<ResourceAdapter> resources = note.resourceAdapters();
        int numResources = resources.size();
        for(int i = 0; i < numResources; ++i)
        {
            const ResourceAdapter & resource = resources[i];

            QString error;
            res = resource.checkParameters(error);
            if (!res) {
                errorDescription += QT_TR_NOOP("found invalid resource linked with note: ");
                errorDescription += error;
                QNWARNING(errorDescription);
                return false;
            }

            error.clear();
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

bool LocalStorageManagerPrivate::InsertOrReplaceTag(const Tag & tag, const QString & overrideLocalGuid,
                                                    QString & errorDescription)
{
    // NOTE: this method expects to be called after tag is already checked
    // for sanity of its parameters!

    errorDescription = QT_TR_NOOP("Can't insert or replace tag into local storage database: ");

    QString localGuid = (overrideLocalGuid.isEmpty() ? tag.localGuid() : overrideLocalGuid);

    QString columns = "localGuid, guid, updateSequenceNumber, name, nameUpper, parentGuid, "
                      "isDirty, isLocal, isDeleted, hasShortcut";

    QString valuesString = ":localGuid, :guid, :updateSequenceNumber, :name, :nameUpper, :parentGuid, "
                           ":isDirty, :isLocal, :isDeleted, :hasShortcut";

    QString queryString = QString("INSERT OR REPLACE INTO Tags (%1) VALUES(%2)").arg(columns).arg(valuesString);

    QSqlQuery query(m_sqlDatabase);

    bool res = query.prepare(queryString);
    DATABASE_CHECK_AND_SET_ERROR("can't prepare SQL query to insert or replace tag into \"Tags\" table in SQL database");

    QVariant nullValue;

    query.bindValue(":localGuid", localGuid);
    query.bindValue(":guid", (tag.hasGuid() ? tag.guid() : nullValue));
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

bool LocalStorageManagerPrivate::InsertOrReplaceResource(const IResource & resource, const QString overrideResourceLocalGuid,
                                                         const Note & note, const QString & overrideNoteLocalGuid,
                                                         QString & errorDescription, const bool useSeparateTransaction)
{
    // NOTE: this method expects to be called after resource is already checked
    // for sanity of its parameters!

    errorDescription = QT_TR_NOOP("Can't insert or replace resource into local storage database: ");

    QString columns = "resourceGuid, noteGuid, dataBody, dataSize, dataHash, mime, "
                      "width, height, recognitionDataBody, recognitionDataSize, "
                      "recognitionDataHash, alternateDataBody, alternateDataSize, "
                      "alternateDataHash, resourceUpdateSequenceNumber, resourceIsDirty, "
                      "resourceIndexInNote, resourceLocalGuid";

    QString values = ":resourceGuid, :noteGuid, :dataBody, :dataSize, :dataHash, :mime, "
                     ":width, :height, :recognitionDataBody, :recognitionDataSize, "
                     ":recognitionDataHash, :alternateDataBody, :alternateDataSize, "
                     ":alternateDataHash, :resourceUpdateSequenceNumber, :resourceIsDirty, "
                     ":resourceIndexInNote, :resourceLocalGuid";

    QScopedPointer<Transaction> pTransaction;
    if (useSeparateTransaction) {
        pTransaction.reset(new Transaction(m_sqlDatabase, Transaction::Exclusive));
    }

    QSqlQuery query(m_sqlDatabase);
    QString queryString = QString("INSERT OR REPLACE INTO Resources (%1) VALUES(%2)").arg(columns).arg(values);
    bool res = query.prepare(queryString);
    DATABASE_CHECK_AND_SET_ERROR("can't insert or replace data into \"Resources\" table in SQL database, "
                                 "can't prepare SQL query");

    QString resourceLocalGuid = (overrideResourceLocalGuid.isEmpty()
                                 ? resource.localGuid() : overrideResourceLocalGuid);

    QVariant nullValue;

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

    // Updating connections between note and resource in NoteResources table

    columns = "localNote, note, localResource, resource";
    values = ":localNote, :note, :localResource, :resource";
    queryString = QString("INSERT OR REPLACE INTO NoteResources (%1) VALUES(%2)")
                          .arg(columns).arg(values);
    res = query.prepare(queryString);
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

    // Updating resource's recognition types

    queryString = QString("DELETE FROM ResourceRecognitionTypes WHERE resourceLocalGuid = '%1'").arg(resourceLocalGuid);
    res = query.exec(queryString);
    DATABASE_CHECK_AND_SET_ERROR("can't delete data from ResourceRecognitionTypes table");

    QStringList recognitionTypes = resource.recognitionTypes();
    if (!recognitionTypes.isEmpty())
    {
        foreach(const QString & recognitionType, recognitionTypes)
        {
            columns = "resourceLocalGuid, recognitionType";
            values = ":resourceLocalGuid, :recognitionType";

            queryString = QString("INSERT OR REPLACE INTO ResourceRecognitionTypes(%1) VALUES(%2)").arg(columns).arg(values);
            res = query.prepare(queryString);
            DATABASE_CHECK_AND_SET_ERROR("can't insert or replace data into \"ResourceRecognitionTypes\" table in SQL database, "
                                         "can't prepare SQL query");
            query.bindValue(":resourceLocalGuid", resourceLocalGuid);
            query.bindValue(":recognitionType", recognitionType);

            res = query.exec();
            DATABASE_CHECK_AND_SET_ERROR("can't insert or replace data into \"ResourceRecognitionTypes\" table in SQL database");
        }
    }

    if (resource.hasResourceAttributes())
    {
        const qevercloud::ResourceAttributes & attributes = resource.resourceAttributes();
        res = InsertOrReplaceResourceAttributes(resourceLocalGuid, attributes, errorDescription);
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
    QString columns = "resourceLocalGuid, resourceSourceURL, timestamp, resourceLatitude, "
                      "resourceLongitude, resourceAltitude, cameraMake, cameraModel, "
                      "clientWillIndex, fileName, attachment";

    QString values = ":resourceLocalGuid, :resourceSourceURL, :timestamp, :resourceLatitude, "
                     ":resourceLongitude, :resourceAltitude, :cameraMake, :cameraModel, "
                     ":clientWillIndex, :fileName, :attachment";

    QString queryString = QString("INSERT OR REPLACE INTO ResourceAttributes(%1) VALUES(%2)")
                                  .arg(columns).arg(values);
    QSqlQuery query(m_sqlDatabase);
    bool res = query.prepare(queryString);
    DATABASE_CHECK_AND_SET_ERROR("can't insert or replace data into \"ResourceAttributes\" table in SQL database, "
                                 "can't prepare SQL query");

    QVariant nullValue;

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

    // Special treatment for ResourceAttributes.applicationData: keysOnly + fullMap

    if (attributes.applicationData.isSet())
    {
        if (attributes.applicationData->keysOnly.isSet())
        {
            const QSet<QString> & keysOnly = attributes.applicationData->keysOnly.ref();
            foreach(const QString & key, keysOnly) {
                queryString = QString("INSERT OR REPLACE INTO ResourceAttributesApplicationDataKeysOnly"
                                      "(resourceLocalGuid, resourceKey) VALUES('%1', '%2')")
                                     .arg(localGuid).arg(key);
                res = query.exec(queryString);
                DATABASE_CHECK_AND_SET_ERROR("can't insert or replace data into \"ResourceAttributesApplicationDataKeysOnly\" table in SQL database");
            }
        }

        if (attributes.applicationData->fullMap.isSet())
        {
            const QMap<QString, QString> & fullMap = attributes.applicationData->fullMap.ref();
            foreach(const QString & key, fullMap.keys()) {
                queryString = QString("INSERT OR REPLACE INTO ResourceAttributesApplicationDataFullMap"
                                      "(resourceLocalGuid, resourceMapKey, resourceValue) VALUES('%1', '%2', '%3')")
                                     .arg(localGuid).arg(key).arg(fullMap.value(key));
                res = query.exec(queryString);
                DATABASE_CHECK_AND_SET_ERROR("can't insert or replace data into \"ResourceAttributesApplicationDataFullMap\" table in SQL database");

            }
        }
    }

    return true;
}

bool LocalStorageManagerPrivate::InsertOrReplaceSavedSearch(const SavedSearch & search,
                                                            const QString & overrideLocalGuid,
                                                            QString & errorDescription)
{
    // NOTE: this method expects to be called after the search is already checked
    // for sanity of its parameters!

    errorDescription = QT_TR_NOOP("Can't insert or replace saved search into local storage database: ");

    QString columns = "localGuid, guid, name, nameUpper, query, format, updateSequenceNumber, isDirty, "
                      "includeAccount, includePersonalLinkedNotebooks, includeBusinessLinkedNotebooks, "
                      "hasShortcut";

    QString valuesNames = ":localGuid, :guid, :name, :nameUpper, :query, :format, :updateSequenceNumber, :isDirty, "
                          ":includeAccount, :includePersonalLinkedNotebooks, :includeBusinessLinkedNotebooks, "
                          ":hasShortcut";

    QString queryString = QString("INSERT OR REPLACE INTO SavedSearches (%1) VALUES(%2)").arg(columns).arg(valuesNames);

    QSqlQuery query(m_sqlDatabase);
    bool res = query.prepare(queryString);
    if (!res) {
        errorDescription += QT_TR_NOOP("failed to prepare SQL query: ");
        QNWARNING(errorDescription << query.lastError());
        errorDescription += query.lastError().text();
        return false;
    }

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
    query.bindValue(":includeAccount", (search.includeAccount() ? 1 : 0));
    query.bindValue(":includePersonalLinkedNotebooks", (search.includePersonalLinkedNotebooks() ? 1 : 0));
    query.bindValue(":includeBusinessLinkedNotebooks", (search.includeBusinessLinkedNotebooks() ? 1 : 0));
    query.bindValue(":hasShortcut", (search.hasShortcut() ? 1 : 0));

    res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("can't insert or replace saved search into \"SavedSearches\" table in SQL database");

    return true;
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
    QChar sep('\0');
    for(int i = 0; i < (length - 1); ++i)
    {
        const QChar currentChar = keysOnlyString.at(i);
        const QChar nextChar = keysOnlyString.at(i+1);

        if (currentChar == wordSep)
        {
            insideQuotedText = !insideQuotedText;

            if (nextChar == sep) {
                keysOnly.insert(currentKey);
                currentKey.clear();
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
    QChar sep('\0');
    for(int i = 0; i < (keysLength - 1); ++i)
    {
        const QChar currentChar = keysString.at(i);
        const QChar nextChar = keysString.at(i+1);

        if (currentChar == wordSep)
        {
            insideQuotedText = !insideQuotedText;

            if (nextChar == sep) {
                keysList << currentKey;
                currentKey.clear();
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

            if (nextChar == sep) {
                valuesList << currentValue;
                currentValue.clear();
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
    QChar sep('\0');
    for(int i = 0; i < (keysLength - 1); ++i)
    {
        const QChar currentChar = keysString.at(i);
        const QChar nextChar = keysString.at(i+1);

        if (currentChar == wordSep)
        {
            insideQuotedText = !insideQuotedText;

            if (nextChar == sep) {
                keysList << currentKey;
                currentKey.clear();
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

            if (nextChar == sep) {
                valuesList << currentValue;
                currentValue.clear();
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

    CHECK_AND_SET_NOTEBOOK_ATTRIBUTE(guid, setGuid, QString, QString, isRequired);
    CHECK_AND_SET_NOTEBOOK_ATTRIBUTE(updateSequenceNumber, setUpdateSequenceNumber,
                                     qint32, qint32, isRequired);
    CHECK_AND_SET_NOTEBOOK_ATTRIBUTE(notebookName, setName, QString, QString, isRequired);
    CHECK_AND_SET_NOTEBOOK_ATTRIBUTE(creationTimestamp, setCreationTimestamp,
                                     qint64, qint64, isRequired);
    CHECK_AND_SET_NOTEBOOK_ATTRIBUTE(modificationTimestamp, setModificationTimestamp,
                                     qint64, qint64, isRequired);

    isRequired = false;

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

        notebook.addSharedNotebook(sharedNotebook);
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
    QStringList tagGuids;
    tagGuids.reserve(std::max(numTagGuids, 0));
    for(QMultiHash<int, QString>::ConstIterator it = tagGuidsAndIndices.constBegin();
        it != tagGuidsAndIndices.constEnd(); ++it)
    {
        tagGuids << it.value();
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

    qSort(resources.begin(), resources.end(), [](const ResourceWrapper & lhs, const ResourceWrapper & rhs)
                                              { return lhs.indexInNote() < rhs.indexInNote(); });
    note.setResources(resources);

    return true;
}

bool LocalStorageManagerPrivate::FindAndSetNoteAttributesPerNote(Note & note, QString & errorDescription) const
{
    QNDEBUG("LocalStorageManagerPrivate::FindAndSetNoteAttributesPerNote: note's local guid = " << note.localGuid());

    errorDescription += QT_TR_NOOP("can't find note attributes: ");

    const QString guid = note.localGuid();

    // ====== 1) Basic NoteAttributes =======
    QSqlQuery query(m_sqlDatabase);
    query.prepare("SELECT * FROM NoteAttributes WHERE noteLocalGuid=?");
    query.addBindValue(guid);

    bool res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("can't select note's attributes from \"NoteAttributes\" table in SQL database");

    if (!query.next()) {
        errorDescription += QT_TR_NOOP("no note attributes were found for provided note's local guid");
        return true;
    }

    qevercloud::NoteAttributes & attributes = note.noteAttributes();

    QSqlRecord rec = query.record();
    FillNoteAttributesFromSqlRecord(rec, attributes);

    // ======= 2) NoteAttributes.applicationData.keysOnly =======
    QString queryString = QString("SELECT * FROM NoteAttributesApplicationDataKeysOnly WHERE noteLocalGuid = '%1'").arg(guid);
    res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("can't select note's attributes from \"NoteAttributesApplicationDataKeysOnly\" table in SQL database");

    if (query.next()) {
        rec = query.record();
        FillNoteAttributesApplicationDataKeysOnlyFromSqlRecord(rec, attributes);
    }

    // ======= 3) NoteAttributes.applicationData.fullMap =======
    queryString = QString("SELECT * FROM NoteAttributesApplicationDataFullMap WHERE noteLocalGuid = '%1'").arg(guid);
    res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("can't select note's attributes from \"NoteAttributesApplicationDataFullMap\" table in SQL database");

    if (query.next()) {
        rec = query.record();
        FillNoteAttributesApplicationDataFullMapFromSqlRecord(rec, attributes);
    }

    // ======= 4) NoteAttributes.classifications =======
    queryString = QString("SELECT * FROM NoteAttributesClassifications WHERE noteLocalGuid = '%1'").arg(guid);
    res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("can't select note's attributes from \"NoteAttributesClassifications\" table in SQL database");

    if (query.next()) {
        rec = query.record();
        FillNoteAttributesApplicationDataKeysOnlyFromSqlRecord(rec, attributes);
    }

    return true;
}

void LocalStorageManagerPrivate::SortSharedNotebooks(Notebook & notebook) const
{
    if (!notebook.hasSharedNotebooks()) {
        return;
    }

    // Sort shared notebooks to ensure the correct order for proper work of comparison operators
    QList<SharedNotebookAdapter> sharedNotebookAdapters = notebook.sharedNotebooks();

    qSort(sharedNotebookAdapters.begin(), sharedNotebookAdapters.end(),
          [](const SharedNotebookAdapter & lhs, const SharedNotebookAdapter & rhs)
          { return lhs.indexInNotebook() < rhs.indexInNotebook(); });
}

bool LocalStorageManagerPrivate::noteSearchQueryToSQL(const NoteSearchQuery & noteSearchQuery,
                                                      QString & sql, QString & errorDescription) const
{
    errorDescription = QT_TR_NOOP("Can't convert note search string into SQL query: ");

    sql = "SELECT DISTINCT localGuid FROM NoteFTS, NoteTags, NoteResources WHERE ";   // initial template to add to

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
            sql += "(NoteTags.localTag IN '";
            sql += tagLocalGuids.join(", ");
            sql += "') ";
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
            sql += "(NoteTags.localTag NOT IN '";
            sql += tagNegatedLocalGuids.join(", ");
            sql += "') ";
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
            sql += "(NoteResources.localResource IN '";
            sql += resourceLocalGuidsPerMime.join(", ");
            sql += "') ";
            sql += uniteOperator;
            sql += " ";
        }

        const QStringList & negatedResourceMimeTypes = noteSearchQuery.negatedResourceMimeTypes();
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
            sql += "(NoteResources.localResource NOT IN '";
            sql += resourceNegatedLocalGuidsPerMime.join(", ");
            sql += "') ";
            sql += uniteOperator;
            sql += " ";
        }
    }

    // 5) ============== Processing resource recognition indices ===============

    if (noteSearchQuery.hasAnyRecognitionType())
    {
        sql += "(NoteResources.localResource IS NOT NULL) ";
        sql += uniteOperator;
        sql += " ";
    }
    else if (noteSearchQuery.hasNegatedAnyRecognitionType())
    {
        sql += "(NoteResources.localResource IS NULL) ";
        sql += uniteOperator;
        sql += " ";
    }
    else
    {
        QStringList resourceLocalGuidsPerRecognitionType;
        QStringList resourceNegatedLocalGuidsPerRecognitionType;

        const QStringList & resourceRecognitionTypes = noteSearchQuery.recognitionTypes();
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
            sql += "(NoteResources.localResource IN '";
            sql += resourceLocalGuidsPerRecognitionType.join(", ");
            sql += "') ";
            sql += uniteOperator;
            sql += " ";
        }

        const QStringList & negatedResourceRecognitionTypes = noteSearchQuery.negatedRecognitionTypes();
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
            sql += "(NoteResources.localGuid NOT IN '";
            sql += resourceNegatedLocalGuidsPerRecognitionType.join(", ");
            sql += "') ";
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
        if (negated) { \
            sql += "(NoteFTS." #column " NOT MATCH '"; \
        } \
        else { \
            sql += "(NoteFTS." #column " MATCH '"; \
        } \
        \
        bool firstItem = true; \
        foreach(const auto & item, noteSearchQuery##list##column) \
        { \
            if (!firstItem) { \
                sql += " OR "; \
            } \
            sql += __VA_ARGS__(item); \
            if (firstItem) { \
                firstItem = false; \
            } \
        } \
        sql += "') "; \
        sql += uniteOperator; \
        sql += " "; \
    }

#define CHECK_AND_PROCESS_ITEM(list, negatedList, hasAnyItem, hasNegatedAnyItem, column, ...) \
    CHECK_AND_PROCESS_ANY_ITEM(hasAnyItem, hasNegatedAnyItem, column) \
    else { \
        CHECK_AND_PROCESS_LIST(list, column, !negated, __VA_ARGS__); \
        CHECK_AND_PROCESS_LIST(negatedList, column, negated, __VA_ARGS__); \
    }

    bool negated = true;
    CHECK_AND_PROCESS_ITEM(titleNames, negatedTitleNames, hasAnyTitleName, hasNegatedAnyTitleName, title);
    CHECK_AND_PROCESS_ITEM(creationTimestamps, negatedCreationTimestamps, hasAnyCreationTimestamp,
                           hasNegatedAnyCreationTimestamp, creationTimestamp, QString::number);
    CHECK_AND_PROCESS_ITEM(modificationTimestamps, negatedModificationTimestamps,
                           hasAnyModificationTimestamp, hasNegatedAnyModificationTimestamp,
                           modificationTimestamp, QString::number);
    CHECK_AND_PROCESS_ITEM(subjectDateTimestamps, negatedSubjectDateTimestamps,
                           hasAnySubjectDateTimestamp, hasNegatedAnySubjectDateTimestamp,
                           subjectDate, QString::number);
    CHECK_AND_PROCESS_ITEM(latitudes, negatedLatitudes, hasAnyLatitude, hasNegatedAnyLatitude,
                           latitude, QString::number);
    CHECK_AND_PROCESS_ITEM(longitudes, negatedLongitudes, hasAnyLongitude, hasNegatedAnyLongitude,
                           longitude, QString::number);
    CHECK_AND_PROCESS_ITEM(altitudes, negatedAltitudes, hasAnyAltitude, hasNegatedAnyAltitude,
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
    CHECK_AND_PROCESS_ITEM(reminderOrders, negatedReminderOrders, hasAnyReminderOrder, hasNegatedAnyReminderOrder,
                           reminderOrder, QString::number);
    CHECK_AND_PROCESS_ITEM(reminderTimes, negatedReminderTimes, hasAnyReminderTime,
                           hasNegatedAnyReminderTime, reminderTime, QString::number);
    CHECK_AND_PROCESS_ITEM(reminderDoneTimes, negatedReminderDoneTimes, hasAnyReminderDoneTime,
                           hasNegatedAnyReminderDoneTime, reminderDoneTime, QString::number);

#undef CHECK_AND_PROCESS_ITEM
#undef CHECK_AND_PROCESS_LIST
#undef CHECK_AND_PROCESS_ANY_ITEM

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

    return true;
}

bool LocalStorageManagerPrivate::tagNamesToTagLocalGuids(const QStringList & tagNames,
                                                         QStringList & tagLocalGuids,
                                                         QString & errorDescription) const
{
    tagLocalGuids.clear();

    QSqlQuery query(m_sqlDatabase);

    if (tagNames.contains("*"))
    {
        bool res = query.exec("SELECT localGuid FROM Tags WHERE nameUpper IS NOT NULL");
        DATABASE_CHECK_AND_SET_ERROR("can't select tag local guids for tag names");
    }
    else
    {
        bool res = query.prepare("SELECT localGuid FROM Tags WHERE nameUpper MATCH ':names'");
        DATABASE_CHECK_AND_SET_ERROR("can't select tag local guids for tag names: can't prepare SQL query");

        QString names = tagNames.join(", ").toUpper();
        query.bindValue(":names", names);

        res = query.exec();
        DATABASE_CHECK_AND_SET_ERROR("can't select tag local guids for tag names");
    }

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

    if (resourceMimeTypes.contains("*"))
    {
        bool res = query.exec("SELECT resourceLocalGuid WHERE mime IS NOT NULL");
        DATABASE_CHECK_AND_SET_ERROR("can't select resource local guids for resource mime types");
    }
    else
    {
        bool res = query.prepare("SELECT resourceLocalGuid WHERE mime MATCH ':mimeTypes'");
        DATABASE_CHECK_AND_SET_ERROR("can't select resource local guids for resource mime types: "
                                     "can't prepare SQL query");

        QString mimeTypes = resourceMimeTypes.join(" OR ");
        query.bindValue(":mimeTypes", mimeTypes);

        res = query.exec();
        DATABASE_CHECK_AND_SET_ERROR("can't select resource local guids for resource mime types");
    }

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
    if (resourceRecognitionTypes.contains("*"))
    {
        bool res = query.exec("SELECT resourceLocalGuid FROM ResourceRecoTypesFTS WHERE recognitionType IS NOT NULL");
        DATABASE_CHECK_AND_SET_ERROR("can't select resource local guids for resource recognition types");
    }
    else
    {
        bool res = query.prepare("SELECT resourceLocalGuid FROM ResourceRecoTypesFTS WHERE recognitionType MATCH ':recognitionTypes'");
        DATABASE_CHECK_AND_SET_ERROR("can't select resource local guids for resource recognition types: "
                                     "can't prepare SQL query");

        QString recognitionTypes = resourceRecognitionTypes.join(" OR ");
        query.bindValue(":recognitionTypes", recognitionTypes);

        res = query.exec();
        DATABASE_CHECK_AND_SET_ERROR("can't select resource local guids for resource recognition types");
    }

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

#undef CHECK_AND_SET_RESOURCE_PROPERTY
#undef CHECK_AND_SET_NOTEBOOK_ATTRIBUTE
#undef CHECK_AND_SET_EN_NOTEBOOK_ATTRIBUTE
#undef SET_IS_FREE_ACCOUNT_FLAG
#undef CHECK_AND_SET_EN_RESOURCE_PROPERTY
#undef CHECK_AND_SET_NOTE_PROPERTY
#undef DATABASE_CHECK_AND_SET_ERROR

} // namespace qute_note

