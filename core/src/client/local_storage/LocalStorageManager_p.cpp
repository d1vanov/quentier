#include "LocalStorageManager_p.h"
#include "DatabaseOpeningException.h"
#include "DatabaseSqlErrorException.h"
#include <client/Utility.h>
#include <logging/QuteNoteLogger.h>
#include <tools/QuteNoteNullPtrException.h>
#include <tools/ApplicationStoragePersistencePath.h>

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
    if (m_sqlDatabase.open()) {
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
    if (user.isLocal()) {
        return ExpungeUser(user, errorDescription);
    }

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

    if (!user.isLocal()) {
        // TRANSLATOR explaining the reason of error
        errorDescription += QT_TR_NOOP("user is not local, expunge is disallowed");
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

    QString queryString = QString("DELETE FROM Notebooks WHERE %1 = \"%2\"").arg(column).arg(guid);
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

    QString queryString = QString("SELECT * FROM Notes LEFT OUTER JOIN NoteAttributes "
                                  "ON Notes.localGuid = NoteAttributes.noteLocalGuid "
                                  "LEFT OUTER JOIN NoteAttributesApplicationDataKeysOnly "
                                  "ON Notes.localGuid = NoteAttributesApplicationDataKeysOnly.noteLocalGuid "
                                  "LEFT OUTER JOIN NoteAttributesApplicationDataFullMap "
                                  "ON Notes.localGuid = NoteAttributesApplicationDataFullMap.noteLocalGuid "
                                  "LEFT OUTER JOIN NoteAttributesClassifications "
                                  "ON Notes.localGuid = NoteAttributesClassifications.noteLocalGuid "
                                  "WHERE %1 = '%2'").arg(column).arg(guid);
    QSqlQuery query(m_sqlDatabase);
    bool res = query.exec(queryString);
    DATABASE_CHECK_AND_SET_ERROR("can't select note from \"Notes\" table in SQL database");

    size_t counter = 0;
    while(query.next()) {
        QSqlRecord rec = query.record();
        FillNoteFromSqlRecord(rec, note);
        ++counter;
    }

    if (!counter) {
        errorDescription += QT_TR_NOOP("no note was found in local storage");
        QNDEBUG(errorDescription);
        return false;
    }

    QString error;
    res = FindAndSetTagGuidsPerNote(note, error);
    if (!res) {
        errorDescription += error;
        QNWARNING(errorDescription);
        return false;
    }

    error.clear();
    res = FindAndSetResourcesPerNote(note, error, withResourceBinaryData);
    if (!res) {
        errorDescription += error;
        QNWARNING(errorDescription);
        return false;
    }

    error.clear();
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

    QString queryString = QString("SELECT * FROM Notes LEFT OUTER JOIN NoteAttributes "
                                  "ON Notes.localGuid = NoteAttributes.noteLocalGuid "
                                  "LEFT OUTER JOIN NoteAttributesApplicationDataKeysOnly "
                                  "ON Notes.localGuid = NoteAttributesApplicationDataKeysOnly.noteLocalGuid "
                                  "LEFT OUTER JOIN NoteAttributesApplicationDataFullMap "
                                  "ON Notes.localGuid = NoteAttributesApplicationDataFullMap.noteLocalGuid "
                                  "LEFT OUTER JOIN NoteAttributesClassifications "
                                  "ON Notes.localGuid = NoteAttributesClassifications.noteLocalGuid "
                                  "WHERE %1 = '%2'").arg(column).arg(guid);
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
    if (note.isLocal()) {
        return ExpungeNote(note, errorDescription);
    }

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

    if (!note.isDeleted()) {
        errorDescription += QT_TR_NOOP("note to be marked as deleted in local storage "
                                       "does not have corresponding mark, rejecting to mark it deleted");
        QNWARNING(errorDescription);
        return false;
    }

    if (!note.hasDeletionTimestamp()) {
        errorDescription += QT_TR_NOOP("note to be marked as deleted in local storage "
                                       "does not have deletion timestamp set, rejecting to mark it deleted");
        QNWARNING(errorDescription);
        return false;
    }

    QString queryString = QString("UPDATE Notes SET isDeleted=1, isDirty=1, deletionTimestamp=%1 WHERE %2 = '%3'")
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

    if (!note.isLocal()) {
        errorDescription += QT_TR_NOOP("note to be expunged from local storage is not marked local, "
                                       "expunging non-local notes is not allowed");
        QNWARNING(errorDescription);
        return false;
    }

    if (!note.isDeleted()) {
        errorDescription += QT_TR_NOOP("note to be expunged from local storage is not marked deleted, "
                                       "expunging non-deleted notes is not allowed");
        QNWARNING(errorDescription);
        return false;
    }

    bool exists = RowExists("Notes", column, QVariant(guid));
    if (!exists) {
        errorDescription += QT_TR_NOOP("can't find note to be expunged in \"Notes\" table in SQL database");
        QNWARNING(errorDescription << ", " << column << ": " << guid);
        return false;
    }

    QString queryString = QString("DELETE FROM Notes WHERE %1 = \"%2\"").arg(column).arg(guid);
    QSqlQuery query(m_sqlDatabase);
    bool res = query.exec(queryString);
    DATABASE_CHECK_AND_SET_ERROR("can't delete entry from \"Notes\" table in SQL database");

    return true;
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

    QString columns = "localNote, localTag";
    bool tagHasGuid = tag.hasGuid();
    if (tagHasGuid) {
        columns.append(", tag");
    }

    bool noteHasGuid = note.hasGuid();
    if (noteHasGuid) {
        columns.append(", note");
    }

    QString valuesString = ":localNote, :localTag";
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
                                  "isDeleted FROM Tags WHERE %1 = '%2'").arg(column).arg(guid);
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
    if (tag.isLocal()) {
        return ExpungeTag(tag, errorDescription);
    }

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

    if (!tag.isLocal()) {
        errorDescription += QT_TR_NOOP("tag to be expunged is not marked local, "
                                       "expunging non-local tags is not allowed");
        QNWARNING(errorDescription);
        return false;
    }

    if (!tag.isDeleted()) {
        errorDescription += QT_TR_NOOP("tag to be expunged is not marked deleted, "
                                       "expunging non-deleted tags is not allowed");
        QNWARNING(errorDescription);
        return false;
    }

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

    QString queryString = QString("SELECT Resources.resourceLocalGuid, resourceGuid, noteGuid, noteLocalGuid, "
                                  "resourceUpdateSequenceNumber, resourceIsDirty, dataSize, dataHash, "
                                  "mime, width, height, recognitionDataSize, recognitionDataHash, "
                                  "resourceIndexInNote, ResourceAttributes.resourceSourceURL, "
                                  "ResourceAttributes.timestamp, ResourceAttributes.resourceLatitude, "
                                  "ResourceAttributes.resourceLongitude, ResourceAttributes.resourceAltitude, "
                                  "ResourceAttributes.cameraMake, ResourceAttributes.cameraModel, "
                                  "ResourceAttributes.clientWillIndex, ResourceAttributes.recoType, "
                                  "ResourceAttributes.fileName, ResourceAttributes.attachment, "
                                  "ResourceAttributesApplicationDataKeysOnly.resourceKey, "
                                  "ResourceAttributesApplicationDataFullMap.resourceMapKey, "
                                  "ResourceAttributesApplicationDataFullMap.resourceValue "
                                  "%1 FROM Resources "
                                  "LEFT OUTER JOIN ResourceAttributes "
                                  "ON Resources.resourceLocalGuid = ResourceAttributes.resourceLocalGuid "
                                  "LEFT OUTER JOIN ResourceAttributesApplicationDataKeysOnly "
                                  "ON Resources.resourceLocalGuid = ResourceAttributesApplicationDataKeysOnly.resourceLocalGuid "
                                  "LEFT OUTER JOIN ResourceAttributesApplicationDataFullMap "
                                  "ON Resources.resourceLocalGuid = ResourceAttributesApplicationDataFullMap.resourceLocalGuid "
                                  "WHERE Resources.%2 = '%3'")
                                 .arg(withBinaryData ? ", dataBody, recognitionDataBody" : " ")
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

    QString queryString = QString("SELECT localGuid, guid, name, query, format, updateSequenceNumber, includeAccount, "
                                  "includePersonalLinkedNotebooks, includeBusinessLinkedNotebooks "
                                  "FROM SavedSearches WHERE %1 = '%2'").arg(column).arg(guid);
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
                     "  userIsDirty                 INTEGER                 DEFAULT NULL, "
                     "  userIsLocal                 INTEGER                 DEFAULT NULL, "
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
                     "  updateSequenceNumber            INTEGER           NOT NULL, "
                     "  notebookName                    TEXT              NOT NULL, "
                     "  creationTimestamp               INTEGER           NOT NULL, "
                     "  modificationTimestamp           INTEGER           NOT NULL, "
                     "  isDirty                         INTEGER           NOT NULL, "
                     "  isLocal                         INTEGER           NOT NULL, "
                     "  isDefault                       INTEGER           DEFAULT NULL UNIQUE, "
                     "  isLastUsed                      INTEGER           DEFAULT NULL UNIQUE, "
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

    res = query.exec("CREATE TABLE IF NOT EXISTS NotebookRestrictions("
                     "  localGuid REFERENCES Notebooks(localGuid) ON DELETE CASCADE ON UPDATE CASCADE, "
                     "  noReadNotes                 INTEGER     DEFAULT NULL, "
                     "  noCreateNotes               INTEGER     DEFAULT NULL, "
                     "  noUpdateNotes               INTEGER     DEFAULT NULL, "
                     "  noExpungeNotes              INTEGER     DEFAULT NULL, "
                     "  noShareNotes                INTEGER     DEFAULT NULL, "
                     "  noEmailNotes                INTEGER     DEFAULT NULL, "
                     "  noSendMessageToRecipients   INTEGER     DEFAULT NULL, "
                     "  noUpdateNotebook            INTEGER     DEFAULT NULL, "
                     "  noExpungeNotebook           INTEGER     DEFAULT NULL, "
                     "  noSetDefaultNotebook        INTEGER     DEFAULT NULL, "
                     "  noSetNotebookStack          INTEGER     DEFAULT NULL, "
                     "  noPublishToPublic           INTEGER     DEFAULT NULL, "
                     "  noPublishToBusinessLibrary  INTEGER     DEFAULT NULL, "
                     "  noCreateTags                INTEGER     DEFAULT NULL, "
                     "  noUpdateTags                INTEGER     DEFAULT NULL, "
                     "  noExpungeTags               INTEGER     DEFAULT NULL, "
                     "  noSetParentTag              INTEGER     DEFAULT NULL, "
                     "  noCreateSharedNotebooks     INTEGER     DEFAULT NULL, "
                     "  updateWhichSharedNotebookRestrictions    INTEGER     DEFAULT NULL, "
                     "  expungeWhichSharedNotebookRestrictions   INTEGER     DEFAULT NULL "
                     ")");
    DATABASE_CHECK_AND_SET_ERROR("can't create NotebookRestrictions table");

    res = query.exec("CREATE TABLE IF NOT EXISTS LinkedNotebooks("
                     "  guid                            TEXT PRIMARY KEY  NOT NULL UNIQUE, "
                     "  updateSequenceNumber            INTEGER           NOT NULL, "
                     "  isDirty                         INTEGER           NOT NULL, "
                     "  shareName                       TEXT              NOT NULL, "
                     "  username                        TEXT              NOT NULL, "
                     "  shardId                         TEXT              NOT NULL, "
                     "  shareKey                        TEXT              NOT NULL, "
                     "  uri                             TEXT              NOT NULL, "
                     "  noteStoreUrl                    TEXT              NOT NULL, "
                     "  webApiUrlPrefix                 TEXT              NOT NULL, "
                     "  stack                           TEXT              DEFAULT NULL, "
                     "  businessId                      INTEGER           DEFAULT NULL"
                     ")");
    DATABASE_CHECK_AND_SET_ERROR("can't create LinkedNotebooks table");

    res = query.exec("CREATE TABLE IF NOT EXISTS SharedNotebooks("
                     "  shareId                             INTEGER    NOT NULL, "
                     "  userId                              INTEGER    NOT NULL, "
                     "  notebookGuid REFERENCES Notebooks(guid) ON DELETE CASCADE ON UPDATE CASCADE, "
                     "  sharedNotebookEmail                 TEXT       NOT NULL, "
                     "  sharedNotebookCreationTimestamp     INTEGER    NOT NULL, "
                     "  sharedNotebookModificationTimestamp INTEGER    NOT NULL, "
                     "  shareKey                            TEXT       NOT NULL, "
                     "  SharedNotebookUsername              TEXT       NOT NULL, "
                     "  sharedNotebookPrivilegeLevel        INTEGER    NOT NULL, "
                     "  allowPreview                        INTEGER    DEFAULT NULL, "
                     "  recipientReminderNotifyEmail        INTEGER    DEFAULT NULL, "
                     "  recipientReminderNotifyInApp        INTEGER    DEFAULT NULL, "
                     "  indexInNotebook                     INTEGER    DEFAULT NULL, "
                     "  UNIQUE(shareId, notebookGuid) ON CONFLICT REPLACE"
                     ")");
    DATABASE_CHECK_AND_SET_ERROR("can't create SharedNotebooks table");

    res = query.exec("CREATE VIRTUAL TABLE IF NOT EXISTS NoteText USING fts3("
                     "  localGuid         TEXT PRIMARY KEY     NOT NULL UNIQUE, "
                     "  guid              TEXT                 DEFAULT NULL UNIQUE, "
                     "  title             TEXT, "
                     "  content           TEXT, "
                     "  notebookLocalGuid REFERENCES Notebooks(localGuid) ON DELETE CASCADE ON UPDATE CASCADE, "
                     "  notebookGuid REFERENCES Notebooks(guid) ON DELETE CASCADE ON UPDATE CASCADE, "
                     "  UNIQUE(localGuid, guid)"
                     ")");
    DATABASE_CHECK_AND_SET_ERROR("can't create virtual table NoteText");

    res = query.exec("CREATE TABLE IF NOT EXISTS Notes("
                     "  localGuid               TEXT PRIMARY KEY     NOT NULL UNIQUE, "
                     "  guid                    TEXT                 DEFAULT NULL UNIQUE, "
                     "  updateSequenceNumber    INTEGER              NOT NULL, "
                     "  isDirty                 INTEGER              NOT NULL, "
                     "  isLocal                 INTEGER              NOT NULL, "
                     "  isDeleted               INTEGER              DEFAULT NULL, "
                     "  title                   TEXT                 NOT NULL, "
                     "  content                 TEXT                 NOT NULL, "
                     "  creationTimestamp       INTEGER              NOT NULL, "
                     "  modificationTimestamp   INTEGER              NOT NULL, "
                     "  deletionTimestamp       INTEGER              DEFAULT NULL, "
                     "  isActive                INTEGER              DEFAULT NULL, "
                     "  hasAttributes           INTEGER              DEFAULT NULL, "
                     "  thumbnail               BLOB                 DEFAULT NULL, "
                     "  notebookLocalGuid REFERENCES Notebooks(localGuid) ON DELETE CASCADE ON UPDATE CASCADE, "
                     "  notebookGuid REFERENCES Notebooks(guid) ON DELETE CASCADE ON UPDATE CASCADE, "
                     "  UNIQUE(localGuid, guid)"
                     ")");
    DATABASE_CHECK_AND_SET_ERROR("can't create Notes table");

    res = query.exec("CREATE TABLE IF NOT EXISTS NoteAttributes("
                     "  noteLocalGuid REFERENCES Notes(localGuid) ON DELETE CASCADE ON UPDATE CASCADE, "
                     "  subjectDate             INTEGER             DEFAULT NULL, "
                     "  latitude                REAL                DEFAULT NULL, "
                     "  longitude               REAL                DEFAULT NULL, "
                     "  altitude                REAL                DEFAULT NULL, "
                     "  author                  TEXT                DEFAULT NULL, "
                     "  source                  TEXT                DEFAULT NULL, "
                     "  sourceURL               TEXT                DEFAULT NULL, "
                     "  sourceApplication       TEXT                DEFAULT NULL, "
                     "  shareDate               INTEGER             DEFAULT NULL, "
                     "  reminderOrder           INTEGER             DEFAULT NULL, "
                     "  reminderDoneTime        INTEGER             DEFAULT NULL, "
                     "  reminderTime            INTEGER             DEFAULT NULL, "
                     "  placeName               TEXT                DEFAULT NULL, "
                     "  contentClass            TEXT                DEFAULT NULL, "
                     "  lastEditedBy            TEXT                DEFAULT NULL, "
                     "  creatorId               INTEGER             DEFAULT NULL, "
                     "  lastEditorId            INTEGER             DEFAULT NULL)");
    DATABASE_CHECK_AND_SET_ERROR("can't create NoteAttributes table");

    res = query.exec("CREATE TABLE IF NOT EXISTS NoteAttributesApplicationDataKeysOnly("
                     "  noteLocalGuid REFERENCES Notes(localGuid) ON DELETE CASCADE ON UPDATE CASCADE, "
                     "  noteKey                     TEXT                DEFAULT NULL UNIQUE)");
    DATABASE_CHECK_AND_SET_ERROR("can't create NoteAttributesApplicationDataKeysOnly table");

    res = query.exec("CREATE TABLE IF NOT EXISTS NoteAttributesApplicationDataFullMap("
                     "  noteLocalGuid REFERENCES Notes(localGuid) ON DELETE CASCADE ON UPDATE CASCADE, "
                     "  noteMapKey                  TEXT                DEFAULT NULL UNIQUE, "
                     "  noteValue                   TEXT                DEFAULT NULL)");
    DATABASE_CHECK_AND_SET_ERROR("can't create NoteAttributesApplicationDataFullMap table");

    res = query.exec("CREATE TABLE IF NOT EXISTS NoteAttributesClassifications("
                     "  noteLocalGuid REFERENCES Notes(localGuid) ON DELETE CASCADE ON UPDATE CASCADE, "
                     "  noteClassificationKey       TEXT                DEFAULT NULL UNIQUE, "
                     "  noteClassificationValue     TEXT                DEFAULT NULL)");
    DATABASE_CHECK_AND_SET_ERROR("can't create NoteAttributesClassifications table");

    res = query.exec("CREATE INDEX IF NOT EXISTS NotesNotebooks ON Notes(notebookLocalGuid)");
    DATABASE_CHECK_AND_SET_ERROR("can't create index NotesNotebooks");

    res = query.exec("CREATE TABLE IF NOT EXISTS Resources("
                     "  resourceLocalGuid               TEXT PRIMARY KEY     NOT NULL UNIQUE, "
                     "  resourceGuid                    TEXT                 DEFAULT NULL UNIQUE, "
                     "  noteLocalGuid REFERENCES Notes(localGuid) ON DELETE CASCADE ON UPDATE CASCADE, "
                     "  noteGuid REFERENCES Notes(guid) ON DELETE CASCADE ON UPDATE CASCADE, "
                     "  resourceUpdateSequenceNumber    INTEGER              NOT NULL, "
                     "  resourceIsDirty                 INTEGER              NOT NULL, "
                     "  dataBody                        TEXT                 NOT NULL, "
                     "  dataSize                        INTEGER              NOT NULL, "
                     "  dataHash                        TEXT                 NOT NULL, "
                     "  mime                            TEXT                 NOT NULL, "
                     "  width                           INTEGER              DEFAULT NULL, "
                     "  height                          INTEGER              DEFAULT NULL, "
                     "  recognitionDataBody             TEXT                 DEFAULT NULL, "
                     "  recognitionDataSize             INTEGER              DEFAULT NULL, "
                     "  recognitionDataHash             TEXT                 DEFAULT NULL, "
                     "  resourceIndexInNote             INTEGER              DEFAULT NULL, "
                     "  UNIQUE(resourceLocalGuid, resourceGuid)"
                     ")");
    DATABASE_CHECK_AND_SET_ERROR("can't create Resources table");

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
                     "  recoType                TEXT                DEFAULT NULL, "
                     "  fileName                TEXT                DEFAULT NULL, "
                     "  attachment              INTEGER             DEFAULT NULL)");
    DATABASE_CHECK_AND_SET_ERROR("can't create ResourceAttributes table");

    res = query.exec("CREATE TABLE IF NOT EXISTS ResourceAttributesApplicationDataKeysOnly("
                     "  resourceLocalGuid REFERENCES Resources(resourceLocalGuid) ON DELETE CASCADE ON UPDATE CASCADE, "
                     "  resourceKey             TEXT                DEFAULT NULL UNIQUE)");
    DATABASE_CHECK_AND_SET_ERROR("can't create ResourceAttributesApplicationDataKeysOnly table");

    res = query.exec("CREATE TABLE IF NOT EXISTS ResourceAttributesApplicationDataFullMap("
                     "  resourceLocalGuid REFERENCES Resources(resourceLocalGuid) ON DELETE CASCADE ON UPDATE CASCADE, "
                     "  resourceMapKey          TEXT                DEFAULT NULL UNIQUE, "
                     "  resourcevalue           TEXT                DEFAULT NULL)");
    DATABASE_CHECK_AND_SET_ERROR("can't create ResourceAttributesApplicationDataFullMap table");

    res = query.exec("CREATE TABLE IF NOT EXISTS Tags("
                     "  localGuid             TEXT PRIMARY KEY     NOT NULL UNIQUE, "
                     "  guid                  TEXT                 DEFAULT NULL UNIQUE, "
                     "  updateSequenceNumber  INTEGER              NOT NULL, "
                     "  name                  TEXT                 NOT NULL, "
                     "  nameUpper             TEXT                 NOT NULL, "
                     "  parentGuid            TEXT                 DEFAULT NULL, "
                     "  isDirty               INTEGER              NOT NULL, "
                     "  isLocal               INTEGER              NOT NULL, "
                     "  isDeleted             INTEGER              DEFAULT NULL"
                     ")");
    DATABASE_CHECK_AND_SET_ERROR("can't create Tags table");

    res = query.exec("CREATE INDEX IF NOT EXISTS TagsSearchName ON Tags(nameUpper)");
    DATABASE_CHECK_AND_SET_ERROR("can't create TagsSearchName index");

    res = query.exec("CREATE TABLE IF NOT EXISTS NoteTags("
                     "  localNote REFERENCES Notes(localGuid) ON DELETE CASCADE ON UPDATE CASCADE, "
                     "  note REFERENCES Notes(guid) ON DELETE CASCADE ON UPDATE CASCADE, "
                     "  localTag REFERENCES Tags(localGuid) ON DELETE CASCADE ON UPDATE CASCADE, "
                     "  tag  REFERENCES Tags(guid) ON DELETE CASCADE ON UPDATE CASCADE, "
                     "  indexInNote           INTEGER               DEFAULT NULL, "
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
                     "  name                            TEXT                NOT NULL, "
                     "  nameUpper                       TEXT                NOT NULL UNIQUE, "
                     "  query                           TEXT                NOT NULL, "
                     "  format                          INTEGER             NOT NULL, "
                     "  updateSequenceNumber            INTEGER             NOT NULL, "
                     "  includeAccount                  INTEGER             NOT NULL, "
                     "  includePersonalLinkedNotebooks  INTEGER             NOT NULL, "
                     "  includeBusinessLinkedNotebooks  INTEGER             NOT NULL, "
                     "  UNIQUE(localGuid, guid))");
    DATABASE_CHECK_AND_SET_ERROR("can't create SavedSearches table");

    return true;
}

bool LocalStorageManagerPrivate::InsertOrReplaceNoteAttributes(const Note & note,
                                                        const QString & overrideLocalGuid,
                                                        QString & errorDescription)
{
    QNDEBUG("LocalStorageManagerPrivate::InsertOrReplaceNoteAttributes");

    if (!note.hasNoteAttributes()) {
        return true;
    }

    const QString localGuid = (overrideLocalGuid.isEmpty() ? note.localGuid() : overrideLocalGuid);

    QString columns = "noteLocalGuid";
    QString valuesString = ":noteLocalGuid";

    const qevercloud::NoteAttributes & attributes = note.noteAttributes();

#define CHECK_AND_ADD_COLUMN_AND_VALUE(name) \
    bool has##name = attributes.name.isSet(); \
    if (has##name) { \
        columns.append(", " #name); \
        valuesString.append(", :" #name); \
    }

    CHECK_AND_ADD_COLUMN_AND_VALUE(subjectDate);
    CHECK_AND_ADD_COLUMN_AND_VALUE(latitude);
    CHECK_AND_ADD_COLUMN_AND_VALUE(longitude);
    CHECK_AND_ADD_COLUMN_AND_VALUE(altitude);
    CHECK_AND_ADD_COLUMN_AND_VALUE(author);
    CHECK_AND_ADD_COLUMN_AND_VALUE(source);
    CHECK_AND_ADD_COLUMN_AND_VALUE(sourceURL);
    CHECK_AND_ADD_COLUMN_AND_VALUE(sourceApplication);
    CHECK_AND_ADD_COLUMN_AND_VALUE(shareDate);
    CHECK_AND_ADD_COLUMN_AND_VALUE(reminderOrder);
    CHECK_AND_ADD_COLUMN_AND_VALUE(reminderTime);
    CHECK_AND_ADD_COLUMN_AND_VALUE(placeName);
    CHECK_AND_ADD_COLUMN_AND_VALUE(contentClass);
    CHECK_AND_ADD_COLUMN_AND_VALUE(lastEditedBy);
    CHECK_AND_ADD_COLUMN_AND_VALUE(creatorId);
    CHECK_AND_ADD_COLUMN_AND_VALUE(lastEditorId);

#undef CHECK_AND_ADD_COLUMN_AND_VALUE

    QString queryString = QString("INSERT OR REPLACE INTO NoteAttributes(%1) VALUES(%2)")
                                  .arg(columns).arg(valuesString);
    QSqlQuery query(m_sqlDatabase);
    bool res = query.prepare(queryString);
    DATABASE_CHECK_AND_SET_ERROR("can't prepare SQL query");

#define CHECK_AND_BIND_VALUE(name) \
    if (has##name) { \
        query.bindValue(":" #name, attributes.name.ref()); \
    }

    CHECK_AND_BIND_VALUE(subjectDate);
    CHECK_AND_BIND_VALUE(latitude);
    CHECK_AND_BIND_VALUE(longitude);
    CHECK_AND_BIND_VALUE(altitude);
    CHECK_AND_BIND_VALUE(author);
    CHECK_AND_BIND_VALUE(source);
    CHECK_AND_BIND_VALUE(sourceURL);
    CHECK_AND_BIND_VALUE(sourceApplication);
    CHECK_AND_BIND_VALUE(shareDate);
    CHECK_AND_BIND_VALUE(reminderOrder);
    CHECK_AND_BIND_VALUE(reminderTime);
    CHECK_AND_BIND_VALUE(placeName);
    CHECK_AND_BIND_VALUE(contentClass);
    CHECK_AND_BIND_VALUE(lastEditedBy);
    CHECK_AND_BIND_VALUE(creatorId);
    CHECK_AND_BIND_VALUE(lastEditorId);

#undef CHECK_AND_BIND_VALUE

    query.bindValue(":noteLocalGuid", localGuid);

    res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("can't insert or replace data into \"NoteAttributes\" table in SQL database");

    // Special treatment for NoteAttributes.applicationData: keysOnly + fullMap and also NoteAttributes.classifications

    if (attributes.applicationData.isSet())
    {
        if (attributes.applicationData->keysOnly.isSet())
        {
            const QSet<QString> & keysOnly = attributes.applicationData->keysOnly.ref();
            foreach(const QString & key, keysOnly) {
                queryString = QString("INSERT OR REPLACE INTO NoteAttributesApplicationDataKeysOnly"
                                      "(noteLocalGuid, noteKey) VALUES('%1', '%2')")
                                      .arg(localGuid).arg(key);
                res = query.exec();
                DATABASE_CHECK_AND_SET_ERROR("can't insert or replace data into \"NoteAttributesApplicationDataKeysOnly\" table in SQL database");
            }
        }

        if (attributes.applicationData->fullMap.isSet())
        {
            const QMap<QString, QString> & fullMap = attributes.applicationData->fullMap.ref();
            foreach(const QString & key, fullMap.keys()) {
                queryString = QString("INSERT OR REPLACE INTO NoteAttributesApplicationDataFullMap"
                                      "(noteLocalGuid, noteMapKey, noteValue) VALUES('%1', '%2', '%3')")
                                      .arg(localGuid).arg(key).arg(fullMap.value(key));
                res = query.exec();
                DATABASE_CHECK_AND_SET_ERROR("can't insert or replace data into \"NoteAttributesApplicationDataFullMap\" table in SQL database");
            }
        }
    }

    if (attributes.classifications.isSet())
    {
        const QMap<QString, QString> & map = attributes.classifications.ref();
        foreach(const QString & key, map.keys()) {
            queryString = QString("INSERT OR REPLACE INTO NoteAttributesClassifications"
                                  "(noteLocalGuid, noteClassificationKey, noteClassificationValue) VALUES('%1', '%2', '%3')")
                                  .arg(localGuid).arg(key).arg(map.value(key));
            res = query.exec();
            DATABASE_CHECK_AND_SET_ERROR("can't insert or replace data into \"NoteAttributesClassifications\" table in SQL database");
        }
    }

    return true;
}

bool LocalStorageManagerPrivate::InsertOrReplaceNotebookAdditionalAttributes(const Notebook & notebook,
                                                                      const QString & overrideLocalGuid,
                                                                      QString & errorDescription)
{
    bool hasAdditionalAttributes = false;
    QString queryString;

    if (notebook.hasPublished())
    {
        hasAdditionalAttributes = true;

        bool notebookIsPublished = notebook.isPublished();

#define APPEND_SEPARATOR \
    if (!queryString.isEmpty()) { queryString.append(", "); }

        queryString.append("isPublished = ");
        queryString.append(QString::number(notebookIsPublished ? 1 : 0));

        if (notebookIsPublished)
        {
            if (notebook.hasPublishingUri()) {
                APPEND_SEPARATOR;
                queryString.append("publishingUri = ");
                queryString.append("\"" + notebook.publishingUri() + "\"");
            }

            if (notebook.hasPublishingOrder()) {
                APPEND_SEPARATOR;
                queryString.append("publishingNoteSortOrder = ");
                queryString.append(QString::number(notebook.publishingOrder()));
            }

            if (notebook.hasPublishingAscending()) {
                APPEND_SEPARATOR;
                queryString.append("publishingAscendingSort = ");
                queryString.append(QString::number(notebook.isPublishingAscending() ? 1 : 0));
            }

            if (notebook.hasPublishingPublicDescription()) {
                APPEND_SEPARATOR;
                queryString.append("publicDescription = ");
                queryString.append("\"" + notebook.publishingPublicDescription() + "\"");
            }
        }
    }

    if (notebook.hasBusinessNotebookDescription()) {
        hasAdditionalAttributes = true;
        APPEND_SEPARATOR;
        queryString.append("businessNotebookDescription = ");
        queryString.append("\"" + notebook.businessNotebookDescription() + "\"");
    }

    if (notebook.hasBusinessNotebookPrivilegeLevel()) {
        hasAdditionalAttributes = true;
        APPEND_SEPARATOR;
        queryString.append("businessNotebookPrivilegeLevel = ");
        queryString.append(QString::number(notebook.businessNotebookPrivilegeLevel()));
    }

    if (notebook.hasBusinessNotebookRecommended()) {
        hasAdditionalAttributes = true;
        APPEND_SEPARATOR;
        queryString.append("businessNotebookIsRecommended = ");
        queryString.append(QString::number(notebook.isBusinessNotebookRecommended() ? 1 : 0));
    }

    if (notebook.hasContact())
    {
        hasAdditionalAttributes = true;
        APPEND_SEPARATOR;
        queryString.append("contactId");
        const UserAdapter user = notebook.contact();
        queryString.append(QString::number(user.id()));

        QString error;
        bool res = user.checkParameters(error);
        if (!res) {
            errorDescription += QT_TR_NOOP("notebook's contact is invalid: ");
            errorDescription += error;
            return false;
        }

        error.clear();
        res = InsertOrReplaceUser(user, error);
        if (!res) {
            errorDescription += QT_TR_NOOP("can't insert or replace notebook's contact: ");
            errorDescription += error;
            return false;
        }
    }

#undef APPEND_SEPARATOR

    QString localGuid = (overrideLocalGuid.isEmpty() ? notebook.localGuid() : overrideLocalGuid);

    if (hasAdditionalAttributes)
    {
        queryString.prepend("UPDATE Notebooks SET ");
        queryString += QString(" WHERE localGuid = '%1'").arg(localGuid);

        QSqlQuery query(m_sqlDatabase);
        bool res = query.exec(queryString);
        DATABASE_CHECK_AND_SET_ERROR("can't insert or replace additional notebook attributes "
                                     "into \"Notebooks\" table in SQL database");
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

    QList<SharedNotebookAdapter> sharedNotebooks = notebook.sharedNotebooks();
    foreach(const SharedNotebookAdapter & sharedNotebook, sharedNotebooks)
    {
        QString error;
        bool res = SetSharedNotebookAttributes(sharedNotebook, error);
        if (!res) {
            errorDescription += error;
            return res;
        }
    }

    return true;
}

bool LocalStorageManagerPrivate::InsertOrReplaceNotebookRestrictions(const qevercloud::NotebookRestrictions & notebookRestrictions,
                                                              const QString & localGuid, QString & errorDescription)
{
    errorDescription = QT_TR_NOOP("Can't insert or replace notebook restrictions: ");

    bool hasAnyRestriction = false;

    QString columns = "localGuid";
    QString values = QString("'%1'").arg(localGuid);

#define CHECK_AND_SET_NOTEBOOK_RESTRICTION(restriction, value) \
    if (notebookRestrictions.restriction.isSet()) \
    { \
        hasAnyRestriction = true; \
        \
        columns.append(", "); \
        columns.append(#restriction); \
        \
        values.append(", "); \
        values.append(value); \
    }

    CHECK_AND_SET_NOTEBOOK_RESTRICTION(noReadNotes, QString::number(notebookRestrictions.noReadNotes ? 1 : 0));
    CHECK_AND_SET_NOTEBOOK_RESTRICTION(noCreateNotes, QString::number(notebookRestrictions.noCreateNotes ? 1 : 0));
    CHECK_AND_SET_NOTEBOOK_RESTRICTION(noUpdateNotes, QString::number(notebookRestrictions.noUpdateNotes ? 1 : 0));
    CHECK_AND_SET_NOTEBOOK_RESTRICTION(noExpungeNotes, QString::number(notebookRestrictions.noExpungeNotes ? 1 : 0));
    CHECK_AND_SET_NOTEBOOK_RESTRICTION(noShareNotes, QString::number(notebookRestrictions.noShareNotes ? 1 : 0));
    CHECK_AND_SET_NOTEBOOK_RESTRICTION(noEmailNotes, QString::number(notebookRestrictions.noEmailNotes ? 1 : 0));
    CHECK_AND_SET_NOTEBOOK_RESTRICTION(noSendMessageToRecipients, QString::number(notebookRestrictions.noSendMessageToRecipients ? 1 : 0));
    CHECK_AND_SET_NOTEBOOK_RESTRICTION(noUpdateNotebook, QString::number(notebookRestrictions.noUpdateNotebook ? 1 : 0));
    CHECK_AND_SET_NOTEBOOK_RESTRICTION(noExpungeNotebook, QString::number(notebookRestrictions.noExpungeNotebook ? 1 : 0));
    CHECK_AND_SET_NOTEBOOK_RESTRICTION(noSetDefaultNotebook, QString::number(notebookRestrictions.noSetDefaultNotebook ? 1 : 0));
    CHECK_AND_SET_NOTEBOOK_RESTRICTION(noSetNotebookStack, QString::number(notebookRestrictions.noSetNotebookStack ? 1 : 0));
    CHECK_AND_SET_NOTEBOOK_RESTRICTION(noPublishToPublic, QString::number(notebookRestrictions.noPublishToPublic ? 1 : 0));
    CHECK_AND_SET_NOTEBOOK_RESTRICTION(noPublishToBusinessLibrary, QString::number(notebookRestrictions.noPublishToBusinessLibrary ? 1 : 0));
    CHECK_AND_SET_NOTEBOOK_RESTRICTION(noCreateTags, QString::number(notebookRestrictions.noCreateTags ? 1 : 0));
    CHECK_AND_SET_NOTEBOOK_RESTRICTION(noUpdateTags, QString::number(notebookRestrictions.noUpdateTags ? 1 : 0));
    CHECK_AND_SET_NOTEBOOK_RESTRICTION(noExpungeTags, QString::number(notebookRestrictions.noExpungeTags ? 1 : 0));
    CHECK_AND_SET_NOTEBOOK_RESTRICTION(noSetParentTag, QString::number(notebookRestrictions.noSetParentTag ? 1 : 0));
    CHECK_AND_SET_NOTEBOOK_RESTRICTION(noCreateSharedNotebooks, QString::number(notebookRestrictions.noCreateSharedNotebooks ? 1 : 0));
    CHECK_AND_SET_NOTEBOOK_RESTRICTION(updateWhichSharedNotebookRestrictions,
                                       QString::number(notebookRestrictions.updateWhichSharedNotebookRestrictions));
    CHECK_AND_SET_NOTEBOOK_RESTRICTION(expungeWhichSharedNotebookRestrictions,
                                       QString::number(notebookRestrictions.expungeWhichSharedNotebookRestrictions));

#undef CHECK_AND_SET_NOTEBOOK_RESTRICTION

    if (hasAnyRestriction) {
        QString queryString = QString("INSERT OR REPLACE INTO NotebookRestrictions (%1) VALUES(%2)")
                                     .arg(columns).arg(values);
        QSqlQuery query(m_sqlDatabase);
        bool res = query.exec(queryString);
        DATABASE_CHECK_AND_SET_ERROR("can't insert or replace notebook restrictions "
                                     "into SQL database");
    }

    return true;
}

bool LocalStorageManagerPrivate::SetSharedNotebookAttributes(const ISharedNotebook & sharedNotebook,
                                                      QString & errorDescription)
{
    errorDescription = QT_TR_NOOP("Can't set shared notebook attributes: ");

    if (!sharedNotebook.hasId()) {
        errorDescription += QT_TR_NOOP("shared notebook's share id is not set");
        QNWARNING(errorDescription);
        return false;
    }

    if (!sharedNotebook.hasNotebookGuid()) {
        errorDescription += QT_TR_NOOP("shared notebook's notebook guid is not set");
        QNWARNING(errorDescription);
        return false;
    }
    else if (!CheckGuid(sharedNotebook.notebookGuid())) {
        errorDescription += QT_TR_NOOP("shared notebook's notebook guid is invalid");
        QNWARNING(errorDescription);
        return false;
    }

    QString columns, values;
    bool hasAnyProperty = false;

#define CHECK_AND_SET_SHARED_NOTEBOOK_ATTRIBUTE(checker, columnName, valueName) \
    if (sharedNotebook.checker()) \
    { \
        hasAnyProperty = true; \
        \
        if (!columns.isEmpty()) { \
            columns.append(", "); \
        } \
        columns.append(#columnName); \
        \
        if (!values.isEmpty()) { \
            values.append(", "); \
        } \
        values.append(valueName); \
    }

    CHECK_AND_SET_SHARED_NOTEBOOK_ATTRIBUTE(hasId, shareId, QString::number(sharedNotebook.id()));
    CHECK_AND_SET_SHARED_NOTEBOOK_ATTRIBUTE(hasUserId, userId, QString::number(sharedNotebook.userId()));
    CHECK_AND_SET_SHARED_NOTEBOOK_ATTRIBUTE(hasNotebookGuid, notebookGuid, "\"" + sharedNotebook.notebookGuid() + "\"");
    CHECK_AND_SET_SHARED_NOTEBOOK_ATTRIBUTE(hasEmail, sharedNotebookEmail, "\"" + sharedNotebook.email() + "\"");
    CHECK_AND_SET_SHARED_NOTEBOOK_ATTRIBUTE(hasCreationTimestamp, sharedNotebookCreationTimestamp,
                                            QString::number(sharedNotebook.creationTimestamp()));
    CHECK_AND_SET_SHARED_NOTEBOOK_ATTRIBUTE(hasModificationTimestamp, sharedNotebookModificationTimestamp,
                                            QString::number(sharedNotebook.modificationTimestamp()));
    CHECK_AND_SET_SHARED_NOTEBOOK_ATTRIBUTE(hasShareKey, shareKey, "\"" + sharedNotebook.shareKey() + "\"");
    CHECK_AND_SET_SHARED_NOTEBOOK_ATTRIBUTE(hasUsername, sharedNotebookUsername, "\"" + sharedNotebook.username() + "\"");
    CHECK_AND_SET_SHARED_NOTEBOOK_ATTRIBUTE(hasPrivilegeLevel, sharedNotebookPrivilegeLevel,
                                            QString::number(sharedNotebook.privilegeLevel()));
    CHECK_AND_SET_SHARED_NOTEBOOK_ATTRIBUTE(hasAllowPreview, allowPreview,
                                            QString::number(sharedNotebook.allowPreview() ? 1 : 0));
    CHECK_AND_SET_SHARED_NOTEBOOK_ATTRIBUTE(hasReminderNotifyEmail, recipientReminderNotifyEmail,
                                            QString::number(sharedNotebook.reminderNotifyEmail() ? 1 : 0));
    CHECK_AND_SET_SHARED_NOTEBOOK_ATTRIBUTE(hasReminderNotifyApp, recipientReminderNotifyInApp,
                                            QString::number(sharedNotebook.reminderNotifyApp() ? 1 : 0));

#undef CHECK_AND_SET_SHARED_NOTEBOOK_ATTRIBUTE

    if (hasAnyProperty) {
        int indexInNotebook = sharedNotebook.indexInNotebook();
        if (indexInNotebook >= 0) {
            columns.append(", indexInNotebook");
            values.append(", " + QString::number(indexInNotebook));
        }

        QString queryString = QString("INSERT OR REPLACE INTO SharedNotebooks(%1) VALUES(%2)")
                                     .arg(columns).arg(values);
        QSqlQuery query(m_sqlDatabase);
        bool res = query.exec(queryString);
        DATABASE_CHECK_AND_SET_ERROR("can't insert or replace shared notebook attributes "
                                     "into SQL database");
    }

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
    errorDescription += QT_TR_NOOP("Can't insert or replace User into local storage database: ");

    QString columns = "id, username, email, name, timezone, privilege, userCreationTimestamp, "
                      "userModificationTimestamp, userIsDirty, userIsLocal, userIsActive";
    QString values;

#define CHECK_AND_SET_USER_VALUE(checker, property, error, use_quotation, ...) \
    if (!user.checker()) { \
        errorDescription += QT_TR_NOOP(error); \
        return false; \
    } \
    else { \
        if (!values.isEmpty()) { values.append(", "); } \
        if (use_quotation) { \
            values.append("'" + __VA_ARGS__(user.property()) + "'"); \
        } \
        else { \
            values.append(__VA_ARGS__(user.property())); \
        } \
    }

    CHECK_AND_SET_USER_VALUE(hasId, id, "User ID is not set", false, QString::number);
    CHECK_AND_SET_USER_VALUE(hasUsername, username, "Username is not set", true);
    CHECK_AND_SET_USER_VALUE(hasEmail, email, "User's email is not set", true);
    CHECK_AND_SET_USER_VALUE(hasName, name, "User's name is not set", true);
    CHECK_AND_SET_USER_VALUE(hasTimezone, timezone, "User's timezone is not set", true);
    CHECK_AND_SET_USER_VALUE(hasPrivilegeLevel, privilegeLevel, "User's privilege level is not set", false, QString::number);
    CHECK_AND_SET_USER_VALUE(hasCreationTimestamp, creationTimestamp, "User's creation timestamp is not set", false, QString::number);
    CHECK_AND_SET_USER_VALUE(hasModificationTimestamp, modificationTimestamp, "User's modification timestamp is not set", false, QString::number);
    CHECK_AND_SET_USER_VALUE(hasActive, active, "User's active field is not set", false, QString::number);

#undef CHECK_AND_SET_USER_VALUE

    values.append(", '" + QString::number((user.isDirty() ? 1 : 0)) + "'");
    values.append(", '" + QString::number((user.isLocal() ? 1 : 0)) + "'");

    // Process deletionTimestamp properties specifically
    if (user.hasDeletionTimestamp()) {
        columns.append(", userDeletionTimestamp");
        values.append(", '" + QString::number(user.deletionTimestamp()) + "'");
    }

    QString queryString = QString("INSERT OR REPLACE INTO Users (%1) VALUES(%2)").arg(columns).arg(values);
    QSqlQuery query(m_sqlDatabase);
    bool res = query.exec(queryString);
    DATABASE_CHECK_AND_SET_ERROR("can't insert or replace user into \"Users\" table in SQL database");

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

    return true;
}

bool LocalStorageManagerPrivate::InsertOrReplaceBusinesUserInfo(const UserID id, const qevercloud::BusinessUserInfo & info,
                                                         QString & errorDescription)
{
    QString columns = "id, ";
    QString valuesString = ":id, ";

    bool hasBusinessId = info.businessId.isSet();
    if (hasBusinessId) {
        columns.append("businessId, ");
        valuesString.append(":businessId, ");
    }

    bool hasBusinessName = info.businessName.isSet();
    if (hasBusinessName) {
        columns.append("businessName, ");
        valuesString.append(":businessName, ");
    }

    bool hasRole = info.role.isSet();
    if (hasRole) {
        columns.append("role, ");
        valuesString.append(":role, ");
    }

    bool hasEmail = info.email.isSet();
    if (hasEmail) {
        columns.append("businessInfoEmail");
        valuesString.append(":businessInfoEmail");
    }

    QString queryString = QString("INSERT OR REPLACE INTO BusinessUserInfo (%1) VALUES(%2)")
                                  .arg(columns).arg(valuesString);
    QSqlQuery query(m_sqlDatabase);
    bool res = query.prepare(queryString);
    DATABASE_CHECK_AND_SET_ERROR("can't set user's business info into \"BusinessUserInfo\" table in SQL database");

    query.bindValue(":id", id);

    if (hasBusinessId) {
        query.bindValue(":businessId", info.businessId.ref());
    }

    if (hasBusinessName) {
        query.bindValue(":businessName", info.businessName.ref());
    }

    if (hasRole) {
        query.bindValue(":role", static_cast<int>(info.role.ref()));
    }

    if (hasEmail) {
        query.bindValue(":businessInfoEmail", info.email.ref());
    }

    res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("can't set user's business info into \"BusinessUserInfo\" table in SQL database");

    return true;
}

bool LocalStorageManagerPrivate::InsertOrReplacePremiumInfo(const UserID id, const qevercloud::PremiumInfo &info, QString & errorDescription)
{
    QString columns = "id, currentTime, premium, premiumRecurring, premiumExtendable, "
                      "premiumPending, premiumCancellationPending, canPurchaseUploadAllowance";
    QString valuesString = ":id, :currentTime, :premium, :premiumRecurring, :premiumExtendable, "
                           ":premiumPending, :premiumCancellationPending, :canPurchaseUploadAllowance";

    bool hasPremiumExpirationDate = info.premiumExpirationDate.isSet();
    if (hasPremiumExpirationDate) {
        columns.append(", premiumExpirationDate");
        valuesString.append(", :premiumExpirationDate");
    }

    bool hasSponsoredGroupName = info.sponsoredGroupName.isSet();
    if (hasSponsoredGroupName) {
        columns.append(", sponsoredGroupName");
        valuesString.append(", :sponsoredGroupName");
    }

    bool hasSponsoredGroupRole = info.sponsoredGroupRole.isSet();
    if (hasSponsoredGroupRole) {
        columns.append(", sponsoredGroupRole");
        valuesString.append(", :sponsoredGroupRole");
    }

    bool hasPremiumUpgradable = info.premiumUpgradable.isSet();
    if (hasPremiumUpgradable) {
        columns.append(", premiumUpgradable");
        valuesString.append(", :premiumUpgradable");
    }

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

    if (hasPremiumExpirationDate) {
        query.bindValue(":premiumExpirationDate", info.premiumExpirationDate.ref());
    }

    if (hasSponsoredGroupName) {
        query.bindValue(":sponsoredGroupName", info.sponsoredGroupName.ref());
    }

    if (hasSponsoredGroupRole) {
        query.bindValue(":sponsoredGroupRole", static_cast<int>(info.sponsoredGroupRole.ref()));
    }

    if (hasPremiumUpgradable) {
        query.bindValue(":premiumUpgradable", (info.premiumUpgradable.ref() ? 1 : 0));
    }

    res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("can't set user's premium info into \"PremiumInfo\" table in SQL database");

    return true;
}

bool LocalStorageManagerPrivate::InsertOrReplaceAccounting(const UserID id, const qevercloud::Accounting & accounting,
                                                    QString & errorDescription)
{
    QString columns = "id";
    QString valuesString = ":id";

#define CHECK_AND_ADD_COLUMN_AND_VALUE(name) \
    bool has##name = accounting.name.isSet(); \
    if (has##name) { \
        columns.append(", " #name); \
        valuesString.append(", :" #name); \
    }

    CHECK_AND_ADD_COLUMN_AND_VALUE(uploadLimit);
    CHECK_AND_ADD_COLUMN_AND_VALUE(uploadLimitEnd);
    CHECK_AND_ADD_COLUMN_AND_VALUE(uploadLimitNextMonth);
    CHECK_AND_ADD_COLUMN_AND_VALUE(premiumServiceStatus);
    CHECK_AND_ADD_COLUMN_AND_VALUE(premiumOrderNumber);
    CHECK_AND_ADD_COLUMN_AND_VALUE(premiumCommerceService);
    CHECK_AND_ADD_COLUMN_AND_VALUE(premiumServiceStart);
    CHECK_AND_ADD_COLUMN_AND_VALUE(premiumServiceSKU);
    CHECK_AND_ADD_COLUMN_AND_VALUE(lastSuccessfulCharge);
    CHECK_AND_ADD_COLUMN_AND_VALUE(lastFailedCharge);
    CHECK_AND_ADD_COLUMN_AND_VALUE(lastFailedChargeReason);
    CHECK_AND_ADD_COLUMN_AND_VALUE(nextPaymentDue);
    CHECK_AND_ADD_COLUMN_AND_VALUE(premiumLockUntil);
    CHECK_AND_ADD_COLUMN_AND_VALUE(updated);
    CHECK_AND_ADD_COLUMN_AND_VALUE(premiumSubscriptionNumber);
    CHECK_AND_ADD_COLUMN_AND_VALUE(lastRequestedCharge);
    CHECK_AND_ADD_COLUMN_AND_VALUE(currency);
    CHECK_AND_ADD_COLUMN_AND_VALUE(unitPrice);
    CHECK_AND_ADD_COLUMN_AND_VALUE(unitDiscount);
    CHECK_AND_ADD_COLUMN_AND_VALUE(nextChargeDate);

#undef CHECK_AND_ADD_COLUMN_AND_VALUE

#define CHECK_AND_ADD_COLUMN_AND_VALUE(name, columnName) \
    bool has##name = accounting.name.isSet(); \
    if (has##name) { \
        columns.append(", " #columnName); \
        valuesString.append(", :" #columnName); \
    }

    CHECK_AND_ADD_COLUMN_AND_VALUE(businessId, accountingBusinessId);
    CHECK_AND_ADD_COLUMN_AND_VALUE(businessName, accountingBusinessName);
    CHECK_AND_ADD_COLUMN_AND_VALUE(businessRole, accountingBusinessRole);

#undef CHECK_AND_ADD_COLUMN_AND_VALUE

    QString queryString = QString("INSERT OR REPLACE INTO Accounting (%1) VALUES(%2)")
                                  .arg(columns).arg(valuesString);
    QSqlQuery query(m_sqlDatabase);
    bool res = query.prepare(queryString);
    DATABASE_CHECK_AND_SET_ERROR("can't set user's acconting into \"Accounting\" table in SQL database");

    query.bindValue(":id", id);

#define CHECK_AND_BIND_VALUE(name) \
    if (has##name) { \
        query.bindValue(":" #name, accounting.name.ref()); \
    }

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
    if (has##name) { \
        query.bindValue(":" #columnName, accounting.name.ref()); \
    }

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
    QString columns = "id";
    QString valuesString = ":id";

#define CHECK_AND_ADD_COLUMN_AND_VALUE(name) \
    bool has##name = attributes.name.isSet(); \
    if (has##name) { \
        columns.append(", " #name); \
        valuesString.append(", :" #name); \
    }

    CHECK_AND_ADD_COLUMN_AND_VALUE(defaultLocationName);
    CHECK_AND_ADD_COLUMN_AND_VALUE(defaultLatitude);
    CHECK_AND_ADD_COLUMN_AND_VALUE(defaultLongitude);
    CHECK_AND_ADD_COLUMN_AND_VALUE(preactivation);
    CHECK_AND_ADD_COLUMN_AND_VALUE(incomingEmailAddress);
    CHECK_AND_ADD_COLUMN_AND_VALUE(comments);
    CHECK_AND_ADD_COLUMN_AND_VALUE(dateAgreedToTermsOfService);
    CHECK_AND_ADD_COLUMN_AND_VALUE(maxReferrals);
    CHECK_AND_ADD_COLUMN_AND_VALUE(referralCount);
    CHECK_AND_ADD_COLUMN_AND_VALUE(refererCode);
    CHECK_AND_ADD_COLUMN_AND_VALUE(sentEmailDate);
    CHECK_AND_ADD_COLUMN_AND_VALUE(sentEmailCount);
    CHECK_AND_ADD_COLUMN_AND_VALUE(dailyEmailLimit);
    CHECK_AND_ADD_COLUMN_AND_VALUE(emailOptOutDate);
    CHECK_AND_ADD_COLUMN_AND_VALUE(partnerEmailOptInDate);
    CHECK_AND_ADD_COLUMN_AND_VALUE(preferredLanguage);
    CHECK_AND_ADD_COLUMN_AND_VALUE(preferredCountry);
    CHECK_AND_ADD_COLUMN_AND_VALUE(clipFullPage);
    CHECK_AND_ADD_COLUMN_AND_VALUE(twitterUserName);
    CHECK_AND_ADD_COLUMN_AND_VALUE(twitterId);
    CHECK_AND_ADD_COLUMN_AND_VALUE(groupName);
    CHECK_AND_ADD_COLUMN_AND_VALUE(recognitionLanguage);
    CHECK_AND_ADD_COLUMN_AND_VALUE(referralProof);
    CHECK_AND_ADD_COLUMN_AND_VALUE(educationalDiscount);
    CHECK_AND_ADD_COLUMN_AND_VALUE(businessAddress);
    CHECK_AND_ADD_COLUMN_AND_VALUE(hideSponsorBilling);
    CHECK_AND_ADD_COLUMN_AND_VALUE(taxExempt);
    CHECK_AND_ADD_COLUMN_AND_VALUE(useEmailAutoFiling);
    CHECK_AND_ADD_COLUMN_AND_VALUE(reminderEmailConfig);

#undef CHECK_AND_ADD_COLUMN_AND_VALUE

    QString queryString = QString("INSERT OR REPLACE INTO UserAttributes (%1) VALUES(%2)")
                                  .arg(columns).arg(valuesString);
    QSqlQuery query(m_sqlDatabase);
    bool res = query.prepare(queryString);
    DATABASE_CHECK_AND_SET_ERROR("can't set user's attributes into \"UserAttributes\" table in SQL database");

    query.bindValue(":id", id);

#define CHECK_AND_BIND_VALUE(name) \
    if (has##name) { \
        query.bindValue(":" #name, attributes.name.ref()); \
    }

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
    if (has##name) { \
        query.bindValue(":" #name, (attributes.name.ref() ? 1 : 0)); \
    }

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

    if (attributes.recentMailedAddresses.isSet()) {
        foreach(const QString & address, attributes.recentMailedAddresses.ref()) {
            queryString = QString("INSERT OR REPLACE INTO UserAttributesRecentMailedAddresses (id, address) VALUES('%1', '%2')")
                                  .arg(id).arg(address);
            res = query.exec(queryString);
            DATABASE_CHECK_AND_SET_ERROR("can't set user's attributes into \"UserAttributesRecentMailedAddresses\" table in SQL database");
        }
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

    QString columns, values;

#define APPEND_COLUMN_VALUE(checker, name, ...) \
    if (notebook.checker()) { \
        if (!columns.isEmpty()) { \
            columns += ", "; \
        } \
        columns += #name; \
        if (!values.isEmpty()) { \
            values += ", "; \
        } \
        values += "'" + __VA_ARGS__ (notebook.name()) + "'"; \
    }

    APPEND_COLUMN_VALUE(hasGuid, guid);
    APPEND_COLUMN_VALUE(hasUpdateSequenceNumber, updateSequenceNumber, QString::number);
    APPEND_COLUMN_VALUE(hasCreationTimestamp, creationTimestamp, QString::number);
    APPEND_COLUMN_VALUE(hasModificationTimestamp, modificationTimestamp, QString::number);
    APPEND_COLUMN_VALUE(hasPublished, isPublished, QString::number);
    APPEND_COLUMN_VALUE(hasStack, stack);

#undef APPEND_COLUMN_VALUE

    if (notebook.hasName()) {
        if (!columns.isEmpty()) {
            columns += ", ";
        }
        columns += "notebookName";
        if (!values.isEmpty()) {
            values += ", ";
        }
        values += "'" + notebook.name() + "'";
    }

    if (notebook.hasContact()) {
        UserAdapter contact = notebook.contact();
        if (!columns.isEmpty()) {
            columns += ", ";
        }
        columns += "contact";
        if (!values.isEmpty()) {
            values += ", ";
        }
        values += QString::number(contact.id());
    }

    if (notebook.isDefaultNotebook()) {
        if (!columns.isEmpty()) {
            columns += ", ";
        }
        columns += "isDefault";
        if (!values.isEmpty()) {
            values += ", ";
        }
        values += QString::number(1);
    }

    if (notebook.isLastUsed()) {
        if (!columns.isEmpty()) {
            columns += ", ";
        }
        columns += "isLastUsed";
        if (!values.isEmpty()) {
            values += ", ";
        }
        values += QString::number(1);
    }

    QString localGuid = (overrideLocalGuid.isEmpty() ? notebook.localGuid() : overrideLocalGuid);
    QString isDirty = QString::number(notebook.isDirty() ? 1 : 0);
    QString isLocal = QString::number(notebook.isLocal() ? 1 : 0);

    QString queryString = QString("INSERT OR REPLACE INTO Notebooks (%1, localGuid, "
                                  "isDirty, isLocal) VALUES(%2, '%3', %4, %5)")
                                  .arg(columns).arg(values).arg(localGuid).arg(isDirty).arg(isLocal);

    QSqlQuery query(m_sqlDatabase);
    bool res = query.exec(queryString);
    DATABASE_CHECK_AND_SET_ERROR("can't insert or replace notebook into \"Notebooks\" table in SQL database");

    return InsertOrReplaceNotebookAdditionalAttributes(notebook, overrideLocalGuid, errorDescription);
}

bool LocalStorageManagerPrivate::InsertOrReplaceLinkedNotebook(const LinkedNotebook & linkedNotebook,
                                                        QString & errorDescription)
{
    // NOTE: this method expects to be called after the linked notebook
    // is already checked for sanity ot its parameters

    errorDescription += QT_TR_NOOP("can't insert or replace linked notebook into "
                                   "local storage database");

    QString columns, values;
    bool hasAnyProperty = false;

#define CHECK_AND_INSERT_LINKED_NOTEBOOK_PROPERTY(property, checker, ...) \
    if (linkedNotebook.checker()) \
    { \
        hasAnyProperty = true; \
        \
        if (!columns.isEmpty()) { \
            columns.append(", "); \
        } \
        columns.append(#property); \
        \
        if (!values.isEmpty()) { \
            values.append(", "); \
        } \
        values.append("\"" + __VA_ARGS__ (linkedNotebook.property()) + "\""); \
    }

    CHECK_AND_INSERT_LINKED_NOTEBOOK_PROPERTY(guid, hasGuid, QString);
    CHECK_AND_INSERT_LINKED_NOTEBOOK_PROPERTY(updateSequenceNumber, hasUpdateSequenceNumber, QString::number);
    CHECK_AND_INSERT_LINKED_NOTEBOOK_PROPERTY(shareName, hasShareName);
    CHECK_AND_INSERT_LINKED_NOTEBOOK_PROPERTY(username, hasUsername);
    CHECK_AND_INSERT_LINKED_NOTEBOOK_PROPERTY(shardId, hasShardId);
    CHECK_AND_INSERT_LINKED_NOTEBOOK_PROPERTY(shareKey, hasShareKey);
    CHECK_AND_INSERT_LINKED_NOTEBOOK_PROPERTY(uri, hasUri);
    CHECK_AND_INSERT_LINKED_NOTEBOOK_PROPERTY(noteStoreUrl, hasNoteStoreUrl);
    CHECK_AND_INSERT_LINKED_NOTEBOOK_PROPERTY(webApiUrlPrefix, hasWebApiUrlPrefix);
    CHECK_AND_INSERT_LINKED_NOTEBOOK_PROPERTY(stack, hasStack);
    CHECK_AND_INSERT_LINKED_NOTEBOOK_PROPERTY(businessId, hasBusinessId, QString::number);

#undef CHECK_AND_INSERT_LINKED_NOTEBOOK_PROPERTY

    if (hasAnyProperty)
    {
        columns.append(", isDirty");
        values.append(", " + QString::number(linkedNotebook.isDirty() ? 1 : 0));

        QString queryString = QString("INSERT OR REPLACE INTO LinkedNotebooks (%1) VALUES(%2)").arg(columns).arg(values);
        QSqlQuery query(m_sqlDatabase);

        bool res = query.exec(queryString);
        DATABASE_CHECK_AND_SET_ERROR("can't insert or replace notebook into \"LinkedNotebooks\" table in SQL database");
    }

    return true;
}

bool LocalStorageManagerPrivate::InsertOrReplaceNote(const Note & note, const Notebook & notebook,
                                              const QString & overrideLocalGuid, QString & errorDescription)
{
    // NOTE: this method expects to be called after the note is already checked
    // for sanity of its parameters!

    errorDescription += QT_TR_NOOP("can't insert or replace note into local storage database: ");

    QImage thumbnail = note.getThumbnail();
    bool thumbnailIsNull = thumbnail.isNull();

    // ========= Creating and executing "insert or replace" query for Notes table
    QString columns = "localGuid, updateSequenceNumber, title, isDirty, isLocal, "
                      "isDeleted, content, creationTimestamp, modificationTimestamp";
    bool noteHasGuid = note.hasGuid();
    if (noteHasGuid) {
        columns.append(", guid");
    }

    bool notebookHasGuid = notebook.hasGuid();
    if (notebookHasGuid) {
        columns.append(", notebookGuid");
    }

    if (!thumbnailIsNull) {
        columns.append(", thumbnail");
    }

    bool noteHasActive = note.hasActive();
    if (noteHasActive) {
        columns.append(", isActive");
    }

    const QString notebookLocalGuid = notebook.localGuid();
    bool notebookHasLocalGuid = !notebookLocalGuid.isEmpty();
    if (notebookHasLocalGuid) {
        columns.append(", notebookLocalGuid");
    }

    QString valuesString = ":localGuid, :updateSequenceNumber, :title, :isDirty, :isLocal, "
                           ":isDeleted, :content, :creationTimestamp, :modificationTimestamp";
    if (noteHasGuid) {
        valuesString.append(", :guid");
    }

    if (notebookHasGuid) {
        valuesString.append(", :notebookGuid");
    }

    if (!thumbnailIsNull) {
        valuesString.append(", :thumbnail");
    }

    if (noteHasActive) {
        valuesString.append(", :isActive");
    }

    if (notebookHasLocalGuid) {
        valuesString.append(", :notebookLocalGuid");
    }

    QString queryString = QString("INSERT OR REPLACE INTO Notes (%1) VALUES(%2)").arg(columns).arg(valuesString);
    QSqlQuery query(m_sqlDatabase);
    bool res = query.prepare(queryString);
    DATABASE_CHECK_AND_SET_ERROR("can't find local notebook's guid for its remote guid: "
                                 "can't prepare SQL query for \"Notes\" table");

    const QString localGuid = (overrideLocalGuid.isEmpty() ? note.localGuid() : overrideLocalGuid);
    const QString content = note.content();
    const QString title = note.title();

    query.bindValue(":localGuid", localGuid);
    query.bindValue(":updateSequenceNumber", note.updateSequenceNumber());
    query.bindValue(":title", title);
    query.bindValue(":isDirty", (note.isDirty() ? 1 : 0));
    query.bindValue(":isLocal", (note.isLocal() ? 1 : 0));
    query.bindValue(":isDeleted", (note.isDeleted() ? 1 : 0));
    query.bindValue(":content", content);
    query.bindValue(":creationTimestamp", note.creationTimestamp());
    query.bindValue(":modificationTimestamp", note.modificationTimestamp());

    if (notebookHasLocalGuid) {
        query.bindValue(":notebookLocalGuid", notebookLocalGuid);
    }

    if (noteHasGuid) {
        query.bindValue(":guid", note.guid());
    }

    if (notebookHasGuid) {
        query.bindValue(":notebookGuid", notebook.guid());
    }

    if (!thumbnailIsNull) {
        query.bindValue(":thumbnail", thumbnail);
    }

    if (noteHasActive) {
        query.bindValue(":isActive", (note.active() ? 1 : 0));
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

            columns = "localNote, localTag, tag, indexInNote";
            if (noteHasGuid) {
                columns.append(", note");
            }

            valuesString = ":localNote, :localTag, :tag, :indexInNote";
            if (noteHasGuid) {
                valuesString.append(", :note");
            }

            queryString = QString("INSERT OR REPLACE INTO NoteTags(%1) VALUES(%2)").arg(columns).arg(valuesString);

            res = query.prepare(queryString);
            DATABASE_CHECK_AND_SET_ERROR("can't insert or replace data into \"NoteTags\" table: can't prepare SQL query");

            query.bindValue(":localNote", localGuid);
            query.bindValue(":localTag", tag.localGuid());
            query.bindValue(":tag", tagGuid);
            query.bindValue(":indexInNote", i);

            if (noteHasGuid) {
                query.bindValue(":note", note.guid());
            }

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
            res = InsertOrReplaceResource(resource, QString(), note, localGuid, error);
            if (!res) {
                errorDescription += QT_TR_NOOP("can't add or update one of note's "
                                               "attached resources: ");
                errorDescription += error;
                return false;
            }
        }
    }

    if (!note.hasNoteAttributes()) {
        return true;
    }

    return InsertOrReplaceNoteAttributes(note, localGuid, errorDescription);
}

bool LocalStorageManagerPrivate::InsertOrReplaceTag(const Tag & tag, const QString & overrideLocalGuid,
                                             QString & errorDescription)
{
    // NOTE: this method expects to be called after tag is already checked
    // for sanity of its parameters!

    errorDescription = QT_TR_NOOP("Can't insert or replace tag into local storage database: ");

    QString localGuid = (overrideLocalGuid.isEmpty() ? tag.localGuid() : overrideLocalGuid);
    QString guid = tag.guid();
    QString name = tag.name();
    QString nameUpper = name.toUpper();

    QString columns = "localGuid, guid, updateSequenceNumber, name, nameUpper, parentGuid, "
                      "isDirty, isLocal, isDeleted";

    QString valuesString = ":localGuid, :guid, :updateSequenceNumber, :name, :nameUpper, :parentGuid, "
                           ":isDirty, :isLocal, :isDeleted";

    QString queryString = QString("INSERT OR REPLACE INTO Tags (%1) VALUES(%2)").arg(columns).arg(valuesString);

    QSqlQuery query(m_sqlDatabase);

    bool res = query.prepare(queryString);
    DATABASE_CHECK_AND_SET_ERROR("can't prepare SQL query to insert or replace tag into \"Tags\" table in SQL database");

    query.bindValue(":localGuid", localGuid);
    query.bindValue(":guid", guid);
    query.bindValue(":updateSequenceNumber", tag.updateSequenceNumber());
    query.bindValue(":name", name);
    query.bindValue(":nameUpper", nameUpper);
    query.bindValue(":parentGuid", tag.hasParentGuid() ? tag.parentGuid() : QString());
    query.bindValue(":isDirty", tag.isDirty() ? 1 : 0);
    query.bindValue(":isLocal", tag.isLocal() ? 1 : 0);
    query.bindValue(":isDeleted", tag.isDeleted() ? 1 : 0);

    res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("can't insert or replace tag into \"Tags\" table in SQL database");

    return true;
}

bool LocalStorageManagerPrivate::InsertOrReplaceResource(const IResource & resource, const QString overrideResourceLocalGuid,
                                                  const Note & note, const QString & overrideNoteLocalGuid,
                                                  QString & errorDescription)
{
    // NOTE: this method expects to be called after resource is already checked
    // for sanity of its parameters!

    errorDescription = QT_TR_NOOP("Can't insert or replace resource into local storage database: ");

    QString columns, values;

#define CHECK_AND_INSERT_RESOURCE_PROPERTY(property, checker, getter, ...) \
    if (resource.checker()) \
    { \
        if (!columns.isEmpty()) { \
            columns.append(", "); \
        } \
        columns.append(#property); \
        \
        if (!values.isEmpty()) { \
            values.append(", "); \
        } \
        values.append("\"" + __VA_ARGS__ (resource.getter()) + "\""); \
    }

    CHECK_AND_INSERT_RESOURCE_PROPERTY(resourceGuid, hasGuid, guid);
    CHECK_AND_INSERT_RESOURCE_PROPERTY(noteGuid, hasNoteGuid, noteGuid);

    bool hasData = resource.hasData();
    bool hasAnyData = (hasData || resource.hasAlternateData());
    if (hasAnyData)
    {
        const auto & dataBody = (hasData ? resource.dataBody() : resource.alternateDataBody());
        CHECK_AND_INSERT_RESOURCE_PROPERTY(dataBody, hasDataBody, dataBody);

        const auto & dataSize = (hasData ? resource.dataSize() : resource.alternateDataSize());
        CHECK_AND_INSERT_RESOURCE_PROPERTY(dataSize, hasDataSize, dataSize, QString::number);

        const auto & dataHash = (hasData ? resource.dataHash() : resource.alternateDataHash());
        CHECK_AND_INSERT_RESOURCE_PROPERTY(dataHash, hasDataHash, dataHash);
    }

    CHECK_AND_INSERT_RESOURCE_PROPERTY(mime, hasMime, mime);
    CHECK_AND_INSERT_RESOURCE_PROPERTY(width, hasWidth, width, QString::number);
    CHECK_AND_INSERT_RESOURCE_PROPERTY(height, hasHeight, height, QString::number);

    if (resource.hasRecognitionData()) {
        CHECK_AND_INSERT_RESOURCE_PROPERTY(recognitionDataBody, hasRecognitionDataBody, recognitionDataBody);
        CHECK_AND_INSERT_RESOURCE_PROPERTY(recognitionDataSize, hasRecognitionDataSize, recognitionDataSize, QString::number);
        CHECK_AND_INSERT_RESOURCE_PROPERTY(recognitionDataHash, hasRecognitionDataHash, recognitionDataHash);
    }

    CHECK_AND_INSERT_RESOURCE_PROPERTY(resourceUpdateSequenceNumber, hasUpdateSequenceNumber, updateSequenceNumber, QString::number);

#undef CHECK_AND_INSERT_RESOURCE_PROPERTY

    if (!columns.isEmpty()) {
        columns.append(", ");
    }
    columns.append("resourceIsDirty");

    if (!values.isEmpty()) {
        values.append(", ");
    }
    values.append(QString::number(resource.isDirty() ? 1 : 0));

    int indexInNote = resource.indexInNote();
    if (indexInNote >= 0) {
        columns.append(", resourceIndexInNote");
        values.append(", " + QString::number(indexInNote));
    }

    columns.append(", resourceLocalGuid");
    QString resourceLocalGuid = (overrideResourceLocalGuid.isEmpty()
                                 ? resource.localGuid() : overrideResourceLocalGuid);
    values.append(", \"" + resourceLocalGuid + "\"");

    QSqlQuery query(m_sqlDatabase);
    QString queryString = QString("INSERT OR REPLACE INTO Resources (%1) VALUES(%2)").arg(columns).arg(values);
    bool res = query.exec(queryString);
    DATABASE_CHECK_AND_SET_ERROR("can't insert or replace data into \"Resources\" table in SQL database");

    QString noteLocalGuid = (overrideNoteLocalGuid.isEmpty()
                             ? note.localGuid() : overrideNoteLocalGuid);

    QString noteColumns, noteValues;
    if (!noteLocalGuid.isEmpty()) {
        noteColumns += "localNote";
        noteValues += "'" + noteLocalGuid + "'";
    }

    if (note.hasGuid()) {
        if (!noteColumns.isEmpty()) {
            noteColumns += ", ";
            noteValues += ", ";
        }

        noteColumns += "note";
        noteValues += "'" + note.guid() + "'";
    }

    QString resourceColumns = "localResource";
    QString resourceValues = "'" + resourceLocalGuid + "'";
    if (resource.hasGuid()) {
        resourceColumns += ", resource";
        resourceValues += ", '" + resource.guid() + "'";
    }

    queryString = QString("INSERT OR REPLACE INTO NoteResources (%1, %2) VALUES(%3, %4)")
                          .arg(noteColumns).arg(resourceColumns).arg(noteValues).arg(resourceValues);
    res = query.exec(queryString);
    DATABASE_CHECK_AND_SET_ERROR("can't insert or replace data into \"NoteResources\" table in SQL database");

    if (resource.hasResourceAttributes())
    {
        const qevercloud::ResourceAttributes & attributes = resource.resourceAttributes();
        res = InsertOrReplaceResourceAttributes(resourceLocalGuid, attributes, errorDescription);
        if (!res) {
            return false;
        }
    }

    return true;
}

bool LocalStorageManagerPrivate::InsertOrReplaceResourceAttributes(const QString & localGuid,
                                                            const qevercloud::ResourceAttributes & attributes,
                                                            QString & errorDescription)
{
    QString columns = "resourceLocalGuid";
    QString valuesString = ":resourceLocalGuid";

#define CHECK_AND_ADD_COLUMN_AND_VALUE(name, property) \
    bool has##name = attributes.property.isSet(); \
    if (has##name) { \
        columns.append(", " #name); \
        valuesString.append(", :" #name); \
    }

    CHECK_AND_ADD_COLUMN_AND_VALUE(resourceSourceURL, sourceURL);
    CHECK_AND_ADD_COLUMN_AND_VALUE(timestamp, timestamp);
    CHECK_AND_ADD_COLUMN_AND_VALUE(resourceLatitude, latitude);
    CHECK_AND_ADD_COLUMN_AND_VALUE(resourceLongitude, longitude);
    CHECK_AND_ADD_COLUMN_AND_VALUE(resourceAltitude, altitude);
    CHECK_AND_ADD_COLUMN_AND_VALUE(cameraMake, cameraMake);
    CHECK_AND_ADD_COLUMN_AND_VALUE(cameraModel, cameraModel);
    CHECK_AND_ADD_COLUMN_AND_VALUE(clientWillIndex, clientWillIndex);
    CHECK_AND_ADD_COLUMN_AND_VALUE(recoType, recoType);
    CHECK_AND_ADD_COLUMN_AND_VALUE(fileName, fileName);
    CHECK_AND_ADD_COLUMN_AND_VALUE(attachment, attachment);

#undef CHECK_AND_ADD_COLUMN_AND_VALUE

    QString queryString = QString("INSERT OR REPLACE INTO ResourceAttributes(%1) VALUES(%2)")
                                  .arg(columns).arg(valuesString);
    QSqlQuery query(m_sqlDatabase);
    bool res = query.prepare(queryString);
    DATABASE_CHECK_AND_SET_ERROR("can't prepare SQL query");

#define CHECK_AND_BIND_VALUE(name, property) \
    if (has##name) { \
        query.bindValue(":" #name, attributes.property.ref()); \
    }

    CHECK_AND_BIND_VALUE(resourceSourceURL, sourceURL);
    CHECK_AND_BIND_VALUE(timestamp, timestamp);
    CHECK_AND_BIND_VALUE(resourceLatitude, latitude);
    CHECK_AND_BIND_VALUE(resourceLongitude, longitude);
    CHECK_AND_BIND_VALUE(resourceAltitude, altitude);
    CHECK_AND_BIND_VALUE(cameraMake, cameraMake);
    CHECK_AND_BIND_VALUE(cameraModel, cameraModel);
    CHECK_AND_BIND_VALUE(recoType, recoType);
    CHECK_AND_BIND_VALUE(fileName, fileName);

#undef CHECK_AND_BIND_VALUE

    if (hasattachment) {
        query.bindValue(":attachment", (attributes.attachment.ref() ? 1 : 0));
    }

    if (hasclientWillIndex) {
        query.bindValue(":clientWillIndex", (attributes.clientWillIndex.ref() ? 1 : 0));
    }

    query.bindValue(":resourceLocalGuid", localGuid);

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
                res = query.exec();
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
                res = query.exec();
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

    QString columns = "localGuid, guid, name, nameUpper, query, format, updateSequenceNumber, "
                      "includeAccount, includePersonalLinkedNotebooks, includeBusinessLinkedNotebooks";

    QString valuesNames = ":localGuid, :guid, :name, :nameUpper, :query, :format, :updateSequenceNumber, "
                          ":includeAccount, :includePersonalLinkedNotebooks, :includeBusinessLinkedNotebooks";

    QString queryString = QString("INSERT OR REPLACE INTO SavedSearches (%1) VALUES(%2)").arg(columns).arg(valuesNames);

    QSqlQuery query(m_sqlDatabase);
    bool res = query.prepare(queryString);
    if (!res) {
        errorDescription += QT_TR_NOOP("failed to prepare SQL query: ");
        QNWARNING(errorDescription << query.lastError());
        errorDescription += query.lastError().text();
        return false;
    }

    query.bindValue(":localGuid", (overrideLocalGuid.isEmpty()
                                   ? search.localGuid()
                                   : overrideLocalGuid));
    query.bindValue(":guid", search.guid());

    QString searchName = search.name();
    query.bindValue(":name", searchName);
    query.bindValue(":nameUpper", searchName.toUpper());

    query.bindValue(":query", search.query());
    query.bindValue(":format", search.queryFormat());
    query.bindValue(":updateSequenceNumber", search.updateSequenceNumber());
    query.bindValue(":includeAccount", (search.includeAccount() ? 1 : 0));
    query.bindValue(":includePersonalLinkedNotebooks", (search.includePersonalLinkedNotebooks() ? 1 : 0));
    query.bindValue(":includeBusinessLinkedNotebooks", (search.includeBusinessLinkedNotebooks() ? 1 : 0));

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

    if (withBinaryData) {
        CHECK_AND_SET_RESOURCE_PROPERTY(recognitionDataBody, QByteArray, QByteArray, setRecognitionDataBody);
        CHECK_AND_SET_RESOURCE_PROPERTY(dataBody, QByteArray, QByteArray, setDataBody);
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
    CHECK_AND_SET_RESOURCE_ATTRIBUTE(timestamp, timestamp, int, qevercloud::Timestamp);
    CHECK_AND_SET_RESOURCE_ATTRIBUTE(resourceLatitude, latitude, double, double);
    CHECK_AND_SET_RESOURCE_ATTRIBUTE(resourceLongitude, longitude, double, double);
    CHECK_AND_SET_RESOURCE_ATTRIBUTE(resourceAltitude, altitude, double, double);
    CHECK_AND_SET_RESOURCE_ATTRIBUTE(cameraMake, cameraMake, QString, QString);
    CHECK_AND_SET_RESOURCE_ATTRIBUTE(cameraModel, cameraModel, QString, QString);
    CHECK_AND_SET_RESOURCE_ATTRIBUTE(clientWillIndex, clientWillIndex, int, bool);
    CHECK_AND_SET_RESOURCE_ATTRIBUTE(recoType, recoType, QString, QString);
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

bool LocalStorageManagerPrivate::FillNoteAttributesFromSqlRecord(const QSqlRecord & rec, qevercloud::NoteAttributes & attributes) const
{
    bool hasSomething = false;

#define CHECK_AND_SET_NOTE_ATTRIBUTE(property, type, localType) \
    { \
        int index = rec.indexOf(#property); \
        if (index >= 0) { \
            QVariant value = rec.value(index); \
            if (!value.isNull()) { \
                attributes.property = static_cast<localType>(qvariant_cast<type>(value)); \
                hasSomething = true; \
            } \
        } \
    }

    CHECK_AND_SET_NOTE_ATTRIBUTE(subjectDate, int, qevercloud::Timestamp);
    CHECK_AND_SET_NOTE_ATTRIBUTE(latitude, double, double);
    CHECK_AND_SET_NOTE_ATTRIBUTE(longitude, double, double);
    CHECK_AND_SET_NOTE_ATTRIBUTE(altitude, double, double);
    CHECK_AND_SET_NOTE_ATTRIBUTE(author, QString, QString);
    CHECK_AND_SET_NOTE_ATTRIBUTE(source, QString, QString);
    CHECK_AND_SET_NOTE_ATTRIBUTE(sourceURL, QString, QString);
    CHECK_AND_SET_NOTE_ATTRIBUTE(sourceApplication, QString, QString);
    CHECK_AND_SET_NOTE_ATTRIBUTE(shareDate, int, qevercloud::Timestamp);
    CHECK_AND_SET_NOTE_ATTRIBUTE(reminderOrder, int, qint64);
    CHECK_AND_SET_NOTE_ATTRIBUTE(reminderDoneTime, int, qevercloud::Timestamp);
    CHECK_AND_SET_NOTE_ATTRIBUTE(reminderTime, int, qevercloud::Timestamp);
    CHECK_AND_SET_NOTE_ATTRIBUTE(placeName, QString, QString);
    CHECK_AND_SET_NOTE_ATTRIBUTE(contentClass, QString, QString);
    CHECK_AND_SET_NOTE_ATTRIBUTE(lastEditedBy, QString, QString);
    CHECK_AND_SET_NOTE_ATTRIBUTE(creatorId, int, UserID);
    CHECK_AND_SET_NOTE_ATTRIBUTE(lastEditorId, int, UserID);

#undef CHECK_AND_SET_NOTE_ATTRIBUTE

    return hasSomething;
}

bool LocalStorageManagerPrivate::FillNoteAttributesApplicationDataKeysOnlyFromSqlRecord(const QSqlRecord & rec,
                                                                                 qevercloud::NoteAttributes & attributes) const
{
    bool hasSomething = false;

    int index = rec.indexOf("noteKey");
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

bool LocalStorageManagerPrivate::FillNoteAttributesApplicationDataFullMapFromSqlRecord(const QSqlRecord & rec,
                                                                                qevercloud::NoteAttributes & attributes) const
{
    bool hasSomething = false;

    int keyIndex = rec.indexOf("noteMapKey");
    int valueIndex = rec.indexOf("noteValue");
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

bool LocalStorageManagerPrivate::FillNoteAttributesClassificationsFromSqlRecord(const QSqlRecord & rec,
                                                                         qevercloud::NoteAttributes & attributes) const
{
    bool hasSomething = false;

    int keyIndex = rec.indexOf("noteClassificationKey");
    int valueIndex = rec.indexOf("noteClassificationValue");
    if ((keyIndex >= 0) && (valueIndex >= 0)) {
        QVariant key = rec.value(keyIndex);
        QVariant value = rec.value(valueIndex);
        if (!key.isNull() && !value.isNull()) {
            if (!attributes.classifications.isSet()) {
                attributes.classifications = QMap<QString, QString>();
            }
            attributes.classifications.ref().insert(key.toString(), value.toString());
            hasSomething = true;
        }
    }

    return hasSomething;
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
                               int, qint64, !isRequired);
    FIND_AND_SET_USER_PROPERTY(userModificationTimestamp, setModificationTimestamp,
                               int, qint64, !isRequired);
    FIND_AND_SET_USER_PROPERTY(userDeletionTimestamp, setDeletionTimestamp,
                               int, qint64, !isRequired);
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
    FIND_AND_SET_USER_ATTRIBUTE(dateAgreedToTermsOfService, dateAgreedToTermsOfService, int, qevercloud::Timestamp);
    FIND_AND_SET_USER_ATTRIBUTE(maxReferrals, maxReferrals, int, qint32);
    FIND_AND_SET_USER_ATTRIBUTE(referralCount, referralCount, int, qint32);
    FIND_AND_SET_USER_ATTRIBUTE(refererCode, refererCode, QString, QString);
    FIND_AND_SET_USER_ATTRIBUTE(sentEmailDate, sentEmailDate, int, qevercloud::Timestamp);
    FIND_AND_SET_USER_ATTRIBUTE(sentEmailCount, sentEmailCount, int, qint32);
    FIND_AND_SET_USER_ATTRIBUTE(dailyEmailLimit, dailyEmailLimit, int, qint32);
    FIND_AND_SET_USER_ATTRIBUTE(emailOptOutDate, emailOptOutDate, int, qevercloud::Timestamp);
    FIND_AND_SET_USER_ATTRIBUTE(partnerEmailOptInDate, partnerEmailOptInDate, int, qevercloud::Timestamp);
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

    FIND_AND_SET_ACCOUNTING_PROPERTY(uploadLimit, uploadLimit, int, qint64);
    FIND_AND_SET_ACCOUNTING_PROPERTY(uploadLimitEnd, uploadLimitEnd, int, qevercloud::Timestamp);
    FIND_AND_SET_ACCOUNTING_PROPERTY(uploadLimitNextMonth, uploadLimitNextMonth, int, qint64);
    FIND_AND_SET_ACCOUNTING_PROPERTY(premiumServiceStatus, premiumServiceStatus,
                                     int, qevercloud::PremiumOrderStatus::type);
    FIND_AND_SET_ACCOUNTING_PROPERTY(premiumOrderNumber, premiumOrderNumber, QString, QString);
    FIND_AND_SET_ACCOUNTING_PROPERTY(premiumCommerceService, premiumCommerceService, QString, QString);
    FIND_AND_SET_ACCOUNTING_PROPERTY(premiumServiceStart, premiumServiceStart,
                                     int, qevercloud::Timestamp);
    FIND_AND_SET_ACCOUNTING_PROPERTY(premiumServiceSKU, premiumServiceSKU, QString, QString);
    FIND_AND_SET_ACCOUNTING_PROPERTY(lastSuccessfulCharge, lastSuccessfulCharge,
                                     int, qevercloud::Timestamp);
    FIND_AND_SET_ACCOUNTING_PROPERTY(lastFailedCharge, lastFailedCharge, int, qevercloud::Timestamp);
    FIND_AND_SET_ACCOUNTING_PROPERTY(lastFailedChargeReason, lastFailedChargeReason, QString, QString);
    FIND_AND_SET_ACCOUNTING_PROPERTY(nextPaymentDue, nextPaymentDue, int, qevercloud::Timestamp);
    FIND_AND_SET_ACCOUNTING_PROPERTY(premiumLockUntil, premiumLockUntil, int, qevercloud::Timestamp);
    FIND_AND_SET_ACCOUNTING_PROPERTY(updated, updated, int, qevercloud::Timestamp);
    FIND_AND_SET_ACCOUNTING_PROPERTY(premiumSubscriptionNumber, premiumSubscriptionNumber,
                                     QString, QString);
    FIND_AND_SET_ACCOUNTING_PROPERTY(lastRequestedCharge, lastRequestedCharge, int, qevercloud::Timestamp);
    FIND_AND_SET_ACCOUNTING_PROPERTY(currency, currency, QString, QString);
    FIND_AND_SET_ACCOUNTING_PROPERTY(accountingBusinessId, businessId, int, qint32);
    FIND_AND_SET_ACCOUNTING_PROPERTY(accountingBusinessName, businessName, QString, QString);
    FIND_AND_SET_ACCOUNTING_PROPERTY(accountingBusinessRole, businessRole, int, qevercloud::BusinessUserRole::type);
    FIND_AND_SET_ACCOUNTING_PROPERTY(unitPrice, unitPrice, int, qint32);
    FIND_AND_SET_ACCOUNTING_PROPERTY(unitDiscount, unitDiscount, int, qint32);
    FIND_AND_SET_ACCOUNTING_PROPERTY(nextChargeDate, nextChargeDate, int, qevercloud::Timestamp);

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

    FIND_AND_SET_PREMIUM_INFO_PROPERTY(currentTime, currentTime, int, qevercloud::Timestamp);
    FIND_AND_SET_PREMIUM_INFO_PROPERTY(premium, premium, int, bool);
    FIND_AND_SET_PREMIUM_INFO_PROPERTY(premiumRecurring, premiumRecurring, int, bool);
    FIND_AND_SET_PREMIUM_INFO_PROPERTY(premiumExtendable, premiumExtendable, int, bool);
    FIND_AND_SET_PREMIUM_INFO_PROPERTY(premiumPending, premiumPending, int, bool);
    FIND_AND_SET_PREMIUM_INFO_PROPERTY(premiumCancellationPending, premiumCancellationPending, int, bool);
    FIND_AND_SET_PREMIUM_INFO_PROPERTY(canPurchaseUploadAllowance, canPurchaseUploadAllowance, int, bool);
    FIND_AND_SET_PREMIUM_INFO_PROPERTY(premiumExpirationDate, premiumExpirationDate, int, qevercloud::Timestamp);
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

    FIND_AND_SET_BUSINESS_USER_INFO_PROPERTY(businessId, businessId, int, qint32);
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
    CHECK_AND_SET_NOTE_PROPERTY(isDeleted, setDeleted, int, bool);
    CHECK_AND_SET_NOTE_PROPERTY(localGuid, setLocalGuid, QString, QString);

    CHECK_AND_SET_NOTE_PROPERTY(guid, setGuid, QString, QString);
    CHECK_AND_SET_NOTE_PROPERTY(updateSequenceNumber, setUpdateSequenceNumber, int, qint32);

    CHECK_AND_SET_NOTE_PROPERTY(notebookGuid, setNotebookGuid, QString, QString);
    CHECK_AND_SET_NOTE_PROPERTY(title, setTitle, QString, QString);
    CHECK_AND_SET_NOTE_PROPERTY(content, setContent, QString, QString);

    // NOTE: omitting content hash and content length as it will be set by the service

    CHECK_AND_SET_NOTE_PROPERTY(creationTimestamp, setCreationTimestamp, int, qint32);
    CHECK_AND_SET_NOTE_PROPERTY(modificationTimestamp, setModificationTimestamp, int, qint32);

    if (note.isDeleted()) {
        CHECK_AND_SET_NOTE_PROPERTY(deletionTimestamp, setDeletionTimestamp, int, qint32);
    }

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

    qevercloud::NoteAttributes localAttributes;
    qevercloud::NoteAttributes & attributes = (note.hasNoteAttributes()
                                               ? note.noteAttributes()
                                               : localAttributes);

    bool hasAttributes = FillNoteAttributesFromSqlRecord(rec, attributes);
    hasAttributes |= FillNoteAttributesApplicationDataKeysOnlyFromSqlRecord(rec, attributes);
    hasAttributes |= FillNoteAttributesApplicationDataFullMapFromSqlRecord(rec, attributes);
    hasAttributes |= FillNoteAttributesClassificationsFromSqlRecord(rec, attributes);

    if (hasAttributes && !note.hasNoteAttributes()) {
        note.noteAttributes() = attributes;
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
                                     int, qint32, isRequired);
    CHECK_AND_SET_NOTEBOOK_ATTRIBUTE(notebookName, setName, QString, QString, isRequired);
    CHECK_AND_SET_NOTEBOOK_ATTRIBUTE(creationTimestamp, setCreationTimestamp,
                                     int, qint64, isRequired);
    CHECK_AND_SET_NOTEBOOK_ATTRIBUTE(modificationTimestamp, setModificationTimestamp,
                                     int, qint64, isRequired);

    isRequired = false;

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
                notebook.setter(!(static_cast<bool>((qvariant_cast<int>(value))))); \
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

    CHECK_AND_SET_SHARED_NOTEBOOK_PROPERTY(shareId, int, qint64, setId);
    CHECK_AND_SET_SHARED_NOTEBOOK_PROPERTY(userId, int, qint32, setUserId);
    CHECK_AND_SET_SHARED_NOTEBOOK_PROPERTY(sharedNotebookEmail, QString, QString, setEmail);
    CHECK_AND_SET_SHARED_NOTEBOOK_PROPERTY(sharedNotebookCreationTimestamp, int,
                                           qint64, setCreationTimestamp);
    CHECK_AND_SET_SHARED_NOTEBOOK_PROPERTY(sharedNotebookModificationTimestamp, int,
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
    CHECK_AND_SET_LINKED_NOTEBOOK_PROPERTY(updateSequenceNumber, int, qint32,
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
    CHECK_AND_SET_LINKED_NOTEBOOK_PROPERTY(businessId, int, qint32, setBusinessId, isRequired);

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

    isRequired = true;
    CHECK_AND_SET_SAVED_SEARCH_PROPERTY(localGuid, QString, QString, setLocalGuid, isRequired);
    CHECK_AND_SET_SAVED_SEARCH_PROPERTY(name, QString, QString, setName, isRequired);
    CHECK_AND_SET_SAVED_SEARCH_PROPERTY(query, QString, QString, setQuery, isRequired);
    CHECK_AND_SET_SAVED_SEARCH_PROPERTY(format, int, qint8, setQueryFormat, isRequired);
    CHECK_AND_SET_SAVED_SEARCH_PROPERTY(updateSequenceNumber, int, int32_t,
                                        setUpdateSequenceNumber, isRequired);

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

    isRequired = true;
    CHECK_AND_SET_TAG_PROPERTY(localGuid, QString, QString, setLocalGuid, isRequired);
    CHECK_AND_SET_TAG_PROPERTY(updateSequenceNumber, int, qint32, setUpdateSequenceNumber, isRequired);
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

    if (!CheckGuid(note.guid())) {
        errorDescription += QT_TR_NOOP("note's guid is invalid");
        return false;
    }

    QSqlQuery query(m_sqlDatabase);
    query.prepare("SELECT tag, indexInNote FROM NoteTags WHERE localNote = ?");
    query.addBindValue(note.localGuid());

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

        QNDEBUG("Found tag guid " << tagGuid << " for note with guid " << note.guid());

        int indexInNote = -1;
        int recordIndex = rec.indexOf("indexInNote");
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

    const QString noteGuid = note.localGuid();

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

            QString foundNoteGuid = value.toString();
            int resourceIndex = rec.indexOf("localResource");
            if ((foundNoteGuid == noteGuid) && (resourceIndex >= 0))
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
                << " for note with local guid " << note.localGuid());
    }

    qSort(resources.begin(), resources.end(), [](const ResourceWrapper & lhs, const ResourceWrapper & rhs) { return lhs.indexInNote() < rhs.indexInNote(); });
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

#undef CHECK_AND_SET_RESOURCE_PROPERTY
#undef CHECK_AND_SET_NOTEBOOK_ATTRIBUTE
#undef CHECK_AND_SET_EN_NOTEBOOK_ATTRIBUTE
#undef SET_IS_FREE_ACCOUNT_FLAG
#undef CHECK_AND_SET_EN_RESOURCE_PROPERTY
#undef CHECK_AND_SET_NOTE_PROPERTY
#undef DATABASE_CHECK_AND_SET_ERROR

} // namespace qute_note

