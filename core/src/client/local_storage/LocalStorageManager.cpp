#include "LocalStorageManager.h"
#include "DatabaseOpeningException.h"
#include "DatabaseSqlErrorException.h"
#include <client/Utility.h>
#include <logging/QuteNoteLogger.h>
#include <client/types/IResource.h>
#include <client/types/ResourceAdapter.h>
#include <client/types/ResourceWrapper.h>
#include <client/types/IUser.h>
#include <client/types/UserAdapter.h>
#include <client/types/UserWrapper.h>
#include <client/types/Notebook.h>
#include <client/types/LinkedNotebook.h>
#include <client/types/ISharedNotebook.h>
#include <client/types/SharedNotebookAdapter.h>
#include <client/types/SharedNotebookWrapper.h>
#include <client/types/SavedSearch.h>
#include <client/types/Tag.h>
#include <tools/QuteNoteNullPtrException.h>
#include <tools/ApplicationStoragePersistencePath.h>

namespace qute_note {

#define QUTE_NOTE_DATABASE_NAME "qn.storage.sqlite"

LocalStorageManager::LocalStorageManager(const QString & username, const UserID userId,
                                         const bool startFromScratch) :
    // NOTE: don't initialize these! Otherwise SwitchUser won't work right
    m_currentUsername(),
    m_currentUserId(),
    m_applicationPersistenceStoragePath()
{
    SwitchUser(username, userId, startFromScratch);
}

LocalStorageManager::~LocalStorageManager()
{
    if (m_sqlDatabase.open()) {
        m_sqlDatabase.close();
    }
}

#define DATABASE_CHECK_AND_SET_ERROR(errorPrefix) \
    if (!res) { \
        errorDescription += QObject::tr("Internal error: "); \
        errorDescription += QObject::tr(errorPrefix); \
        errorDescription += ": "; \
        QNCRITICAL(errorDescription << query.lastError() << ", last executed query: " << query.lastQuery()); \
        errorDescription += query.lastError().text(); \
        return false; \
    }

bool LocalStorageManager::AddUser(const IUser & user, QString & errorDescription)
{
    errorDescription = QObject::tr("Can't add user into local storage database: ");
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
        errorDescription += QObject::tr("user with the same id already exists");
        QNWARNING(errorDescription << ", id: " << userId);
        return false;
    }

    error.clear();
    res = InsertOrReplaceUser(user, error);
    if (!res) {
        errorDescription += error;
        return false;
    }

    return true;
}

bool LocalStorageManager::UpdateUser(const IUser & user, QString & errorDescription)
{
    errorDescription = QObject::tr("Can't add user into local storage database: ");
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
        errorDescription += QObject::tr("user id was not found");
        QNWARNING(errorDescription << ", id: " << userId);
        return false;
    }

    error.clear();
    res = InsertOrReplaceUser(user, error);
    if (!res) {
        errorDescription += error;
        return false;
    }

    return true;
}

bool LocalStorageManager::FindUser(IUser & user, QString & errorDescription) const
{
    QNDEBUG("LocalStorageManager::FindUser");

    errorDescription = QObject::tr("Can't find user in local storage database: ");

    if (!user.hasId()) {
        errorDescription += QObject::tr("user id is not set");
        return false;
    }

    qint32 id = user.id();
    QString userId = QString::number(id);
    QNDEBUG("Looking for user with id = " << userId);

    bool exists = RowExists("Users", "id", QVariant(userId));
    if (!exists) {
        errorDescription += QObject::tr("user id was not found");
        QNDEBUG(errorDescription << ", id: " << userId);
        return false;
    }

    user.clear();
    user.setId(id);

    QSqlQuery query(m_sqlDatabase);
    query.prepare("SELECT username, email, name, timezone, privilege, creationTimestamp, "
                  "modificationTimestamp, isDirty, isLocal, deletionTimestamp, isActive "
                  "FROM Users WHERE id=?");
    query.addBindValue(userId);

    bool res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("can't select user from \"Users\" table in SQL database");

    if (!query.next()) {
        errorDescription += QObject::tr("Internal error: SQL query result is empty");
        return false;
    }

    QSqlRecord rec = query.record();

#define CHECK_AND_SET_USER_PROPERTY(property) \
    if (rec.contains("is"#property)) { \
        int property = (qvariant_cast<int>(rec.value("is"#property)) != 0); \
        user.set##property(static_cast<bool>(property)); \
    } \
    else { \
        errorDescription += QObject::tr("Internal error: no " #property \
                                        " field in the result of SQL query "); \
        return false; \
    }

    CHECK_AND_SET_USER_PROPERTY(Dirty);
    CHECK_AND_SET_USER_PROPERTY(Local);

#undef CHECK_AND_SET_USER_PROPERTY

#define CHECK_AND_SET_EN_USER_PROPERTY(propertyLocalName, setter, \
                                       type, true_type, isRequired) \
    if (rec.contains(#propertyLocalName)) { \
        user.setter(static_cast<true_type>((qvariant_cast<type>(rec.value(#propertyLocalName))))); \
    } \
    else if (isRequired) { \
        errorDescription += QObject::tr("Internal error: no " #propertyLocalName \
                                        " field in the result of SQL query"); \
        return false; \
    }

    bool isRequired = true;
    CHECK_AND_SET_EN_USER_PROPERTY(username, setUsername, QString, QString, isRequired);
    CHECK_AND_SET_EN_USER_PROPERTY(email, setEmail, QString, QString, isRequired);
    CHECK_AND_SET_EN_USER_PROPERTY(name, setName, QString, QString, isRequired);
    CHECK_AND_SET_EN_USER_PROPERTY(timezone, setTimezone, QString, QString,
                                   /* isRequired = */ false);
    CHECK_AND_SET_EN_USER_PROPERTY(privilege, setPrivilegeLevel, int, qint8, isRequired);
    CHECK_AND_SET_EN_USER_PROPERTY(creationTimestamp, setCreationTimestamp,
                                   int, qint64, isRequired);
    CHECK_AND_SET_EN_USER_PROPERTY(modificationTimestamp, setModificationTimestamp,
                                   int, qint64, isRequired);
    CHECK_AND_SET_EN_USER_PROPERTY(deletionTimestamp, setDeletionTimestamp,
                                   int, qint64, /* isRequired = */ false);
    CHECK_AND_SET_EN_USER_PROPERTY(isActive, setActive, int, bool, isRequired);   // NOTE: int to bool cast

#undef CHECK_AND_SET_EN_USER_PROPERTY

    QString error;
    qevercloud::UserAttributes userAttributes;
    res = FindUserAttributes(id, userAttributes, error);
    if (res) {
        user.setUserAttributes(std::move(userAttributes));
    }

    error.clear();
    qevercloud::Accounting accounting;
    res = FindAccounting(id, accounting, error);
    if (res) {
        user.setAccounting(std::move(accounting));
    }

    error.clear();
    qevercloud::PremiumInfo premiumInfo;
    res = FindPremiumInfo(id, premiumInfo, error);
    if (res) {
        user.setPremiumInfo(std::move(premiumInfo));
    }

    error.clear();
    qevercloud::BusinessUserInfo businessUserInfo;
    res = FindBusinessUserInfo(id, businessUserInfo, error);
    if (res) {
        user.setBusinessUserInfo(std::move(businessUserInfo));
    }

    return true;
}

bool LocalStorageManager::DeleteUser(const IUser & user, QString & errorDescription)
{
    if (user.isLocal()) {
        return ExpungeUser(user, errorDescription);
    }

    errorDescription = QObject::tr("Can't delete user from local storage database: ");

    if (!user.hasDeletionTimestamp()) {
        errorDescription += QObject::tr("deletion timestamp is not set");
        return false;
    }

    if (!user.hasId()) {
        errorDescription += QObject::tr("user id is not set");
        return false;
    }

    QSqlQuery query(m_sqlDatabase);
    query.prepare("UPDATE Users SET deletionTimestamp = ?, isLocal = ? WHERE id = ?");
    query.addBindValue(user.deletionTimestamp());
    query.addBindValue(user.isLocal() ? 1 : 0);
    query.addBindValue(user.id());

    bool res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("can't update deletion timestamp in \"Users\" table in SQL database");

    return true;
}

bool LocalStorageManager::ExpungeUser(const IUser & user, QString & errorDescription)
{
    errorDescription = QObject::tr("Can't expunge user from local storage database: ");

    if (!user.isLocal()) {
        errorDescription += QObject::tr("user is not local, expunge is disallowed");
        return false;
    }

    if (!user.hasId()) {
        errorDescription += QObject::tr("user id is not set");
        return false;
    }

    QSqlQuery query(m_sqlDatabase);
    query.prepare("DELETE FROM Users WHERE id = ?");
    query.addBindValue(QString::number(user.id()));

    bool res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("can't delete entry from \"Users\" table in SQL database");

    return true;
}

void LocalStorageManager::SwitchUser(const QString & username, const UserID userId,
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
            throw DatabaseOpeningException(QObject::tr("Cannot create folder "
                                                       "to store local storage  database."));
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
            throw DatabaseOpeningException(QObject::tr("Cannot create local storage database: ") +
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
        QNDEBUG("Unable to open QSqlDatabase corresponding to file " + databaseFileName);

        QString lastErrorText = m_sqlDatabase.lastError().text();
        throw DatabaseOpeningException(QObject::tr("Cannot open local storage database: ") +
                                       lastErrorText);
    }

    QSqlQuery query(m_sqlDatabase);
    if (!query.exec("PRAGMA foreign_keys = ON")) {
        throw DatabaseSqlErrorException(QObject::tr("Cannot set foreign_keys = ON pragma "
                                                    "for SQL local storage database"));
    }

    QString errorDescription;
    if (!CreateTables(errorDescription)) {
        throw DatabaseSqlErrorException(QObject::tr("Cannot initialize tables in SQL database: ") +
                                        errorDescription);
    }
}

int LocalStorageManager::GetUserCount(QString & errorDescription) const
{
    QSqlQuery query(m_sqlDatabase);
    bool res = query.exec("SELECT COUNT(*) FROM Users WHERE deletionTimestamp = -1");
    if (!res) {
        errorDescription = "Internal error: can't get number of users in local storage database: ";
        QNCRITICAL(errorDescription << query.lastError() << ", last query: " << query.lastQuery());
        errorDescription = QObject::tr(qPrintable(errorDescription));
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
        errorDescription = "Internal error: can't convert number of users to int";
        QNCRITICAL(errorDescription << ": " << query.value(0));
        errorDescription = QObject::tr(qPrintable(errorDescription));
        return -1;
    }

    return count;
}

bool LocalStorageManager::AddNotebook(const Notebook & notebook, QString & errorDescription)
{
    errorDescription = QObject::tr("Can't add notebook to local storage database: ");
    QString error;

    bool res = notebook.checkParameters(error);
    if (!res) {
        errorDescription += error;
        QNWARNING("Found invalid notebook: " << notebook << "\nError: " << error);
        return false;
    }

    QString column, guid;
    bool notebookHasGuid = notebook.hasGuid();
    if (notebookHasGuid) {
        column = "guid";
        guid = notebook.guid();
    }
    else {
        column = "localGuid";
        guid = notebook.localGuid();
    }

    bool exists = RowExists("Notebooks", column, QVariant(guid));
    if (exists) {
        errorDescription += QObject::tr("notebook with specified ");
        errorDescription += (notebookHasGuid ? QObject::tr("guid") : QObject::tr("local guid"));
        errorDescription += QObject::tr(" already exists");
        QNWARNING(errorDescription << ", " << column << ": " << guid);
        return false;
    }

    return InsertOrReplaceNotebook(notebook, errorDescription);
}

bool LocalStorageManager::UpdateNotebook(const Notebook & notebook, QString & errorDescription)
{
    errorDescription = QObject::tr("Can't update notebook in local storage database: ");
    QString error;

    bool res = notebook.checkParameters(error);
    if (!res) {
        errorDescription += error;
        QNWARNING("Found invalid notebook: " << notebook << "\nError: " << error);
        return false;
    }

    QString column, guid;
    bool notebookHasGuid = notebook.hasGuid();
    if (notebookHasGuid) {
        column = "guid";
        guid = notebook.guid();
    }
    else {
        column = "localGuid";
        guid = notebook.localGuid();
    }

    bool exists = RowExists("Notebooks", column, QVariant(guid));
    if (!exists) {
        errorDescription += QObject::tr("notebook with specified ");
        errorDescription += (notebookHasGuid ? QObject::tr("guid") : QObject::tr("local guid"));
        errorDescription += QObject::tr(" was not found");
        QNWARNING(errorDescription << ", " << column << ": " << guid);
        return false;
    }

    return InsertOrReplaceNotebook(notebook, errorDescription);
}

bool LocalStorageManager::FindNotebook(Notebook & notebook, QString & errorDescription)
{
    errorDescription = QObject::tr("Can't find notebook in local storage database: ");

    QString column, guid;
    bool notebookHasGuid = notebook.hasGuid();
    if (notebookHasGuid) {
        column = "guid";
        guid = notebook.guid();
        if (!CheckGuid(guid)) {
            errorDescription += QObject::tr("requested guid is invalid");
            return false;
        }
    }
    else { // whichGuid == WhichGuid::EverCloudGuid
        column = "localGuid";
        guid = notebook.localGuid();
    }

    notebook = Notebook();

    QString queryString = QString("SELECT localGuid, guid, updateSequenceNumber, name, creationTimestamp, "
                                  "modificationTimestamp, isDirty, isLocal, isDefault, isLastUsed, "
                                  "publishingUri, publishingNoteSortOrder, publishingAscendingSort, "
                                  "publicDescription, isPublished, stack, businessNotebookDescription, "
                                  "businessNotebookPrivilegeLevel, businessNotebookIsRecommended, "
                                  "contactId FROM Notebooks WHERE %1 = '%2'").arg(column).arg(guid);
    QSqlQuery query(m_sqlDatabase);
    bool res = query.exec(queryString);
    DATABASE_CHECK_AND_SET_ERROR("can't find notebook in SQL database by guid");

    if (!query.next()) {
        errorDescription += QObject::tr("Internal error: SQL query result is empty");
        return false;
    }

    QSqlRecord rec = query.record();

    return FillNotebookFromSqlRecord(rec, notebook, errorDescription);
}

QList<Notebook> LocalStorageManager::ListAllNotebooks(QString & errorDescription) const
{
    QNDEBUG("LocalStorageManager::ListAllNotebooks");

    QList<Notebook> notebooks;
    errorDescription = QObject::tr("Can't list all notebooks in local storage database: ");

    QSqlQuery query(m_sqlDatabase);
    bool res = query.exec("SELECT * FROM Notebooks");
    if (!res) {
        errorDescription += QObject::tr("can't select all notebooks from SQL database: ");
        QNCRITICAL(errorDescription << "last error = " << query.lastError() << ", last query = " << query.lastQuery());
        errorDescription += query.lastError().text();
        return notebooks;
    }

    notebooks.reserve(qMax(query.size(), 0));

    while(query.next())
    {
        QSqlRecord rec = query.record();

        notebooks << Notebook();
        Notebook & notebook = notebooks.back();
        res = FillNotebookFromSqlRecord(rec, notebook, errorDescription);
        if (!res) {
            notebooks.clear();
            return notebooks;
        }
    }

    int numNotebooks = notebooks.size();
    QNDEBUG("found " << numNotebooks << " notebooks");

    if (numNotebooks <= 0) {
        errorDescription += QObject::tr("found no notebooks");
    }

    return notebooks;
}

QList<SharedNotebookWrapper> LocalStorageManager::ListAllSharedNotebooks(QString & errorDescription) const
{
    QNDEBUG("LocalStorageManager::ListAllSharedNotebooks");

    QList<SharedNotebookWrapper> sharedNotebooks;
    errorDescription = QObject::tr("Can't list all shared notebooks: ");

    QSqlQuery query(m_sqlDatabase);
    bool res = query.exec("SELECT * FROM SharedNotebooks");
    if (!res) {
        errorDescription += QObject::tr("can't select shared notebooks from SQL database: ");
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
        errorDescription += QObject::tr("found no shared notebooks");
    }

    return sharedNotebooks;
}

QList<SharedNotebookWrapper> LocalStorageManager::ListSharedNotebooksPerNotebookGuid(const QString & notebookGuid,
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

QList<qevercloud::SharedNotebook> LocalStorageManager::ListEnSharedNotebooksPerNotebookGuid(const QString & notebookGuid,
                                                                                            QString & errorDescription) const
{
    QNDEBUG("LocalStorageManager::ListSharedNotebooksPerNotebookGuid: guid = " << notebookGuid);

    QList<qevercloud::SharedNotebook> sharedNotebooks;
    QString errorPrefix = QObject::tr("Can't list shared notebooks per notebook guid: ");

    if (!CheckGuid(notebookGuid)) {
        errorDescription = errorPrefix + QObject::tr("notebook guid is invalid");
        return sharedNotebooks;
    }

    QSqlQuery query(m_sqlDatabase);
    query.prepare("SELECT shareId, userId, email, creationTimestamp, modificationTimestamp, "
                  "shareKey, username, sharedNotebookPrivilegeLevel, allowPreview, indexInNotebook, "
                  "recipientReminderNotifyEmail, recipientReminderNotifyInApp, notebookGuid "
                  "FROM SharedNotebooks WHERE notebookGuid=?");
    query.addBindValue(notebookGuid);

    bool res = query.exec();
    if (!res) {
        errorDescription = errorPrefix + QObject::tr("can't select shared notebooks for given "
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

bool LocalStorageManager::ExpungeNotebook(const Notebook & notebook, QString & errorDescription)
{
    errorDescription = QObject::tr("Can't expunge notebook from local storage database: ");

    QString column, guid;
    bool notebookHasGuid = notebook.hasGuid();
    if (notebookHasGuid) {
        column = "guid";
        guid = notebook.guid();

        if (!CheckGuid(guid)) {
            errorDescription += QObject::tr("notebook's guid is invalid");
            return false;
        }
    }
    else {
        column = "localGuid";
        guid = notebook.localGuid();
    }

    bool exists = RowExists("Notebooks", column, QVariant(guid));
    if (!exists) {
        errorDescription += QObject::tr("notebook to be expunged from \"Notebooks\""
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

bool LocalStorageManager::AddLinkedNotebook(const LinkedNotebook & linkedNotebook,
                                            QString & errorDescription)
{
    errorDescription = QObject::tr("Can't add linked notebook to local storage database: ");
    QString error;

    bool res = linkedNotebook.checkParameters(error);
    if (!res) {
        errorDescription += error;
        QNWARNING("Found invalid LinkedNotebook: " << linkedNotebook << "\nError: " << error);
        return false;
    }

    bool exists = RowExists("LinkedNotebooks", "guid", QVariant(linkedNotebook.guid()));
    if (exists) {
        errorDescription += QObject::tr("linked notebook with specified guid already exists");
        QNWARNING(errorDescription << ", guid: " << linkedNotebook.guid());
        return false;
    }

    return InsertOrReplaceLinkedNotebook(linkedNotebook, errorDescription);
}

bool LocalStorageManager::UpdateLinkedNotebook(const LinkedNotebook & linkedNotebook,
                                               QString & errorDescription)
{
    errorDescription = QObject::tr("Can't update linked notebook in local storage database: ");
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
        errorDescription += QObject::tr("linked notebook with specified guid was not found");
        QNWARNING(errorDescription << ", guid: " << guid);
        return false;
    }

    return InsertOrReplaceLinkedNotebook(linkedNotebook, errorDescription);
}

bool LocalStorageManager::FindLinkedNotebook(LinkedNotebook & linkedNotebook, QString & errorDescription) const
{
    QNDEBUG("LocalStorageManager::FindLinkedNotebook");

    errorDescription = QObject::tr("Can't find linked notebook in local storage database: ");

    if (!linkedNotebook.hasGuid()) {
        errorDescription += QObject::tr("guid is not set");
        return false;
    }

    QString notebookGuid = linkedNotebook.guid();
    QNDEBUG("guid = " << notebookGuid);
    if (!CheckGuid(notebookGuid)) {
        errorDescription += QObject::tr("guid is invalid");
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
        errorDescription += QObject::tr("Internal error: SQL query result is empty");
        return false;
    }

    QSqlRecord rec = query.record();

    return FillLinkedNotebookFromSqlRecord(rec, linkedNotebook, errorDescription);
}

QList<LinkedNotebook> LocalStorageManager::ListAllLinkedNotebooks(QString & errorDescription) const
{
    QNDEBUG("LocalStorageManager::ListAllLinkedNotebooks");

    QList<LinkedNotebook> notebooks;
    QString errorPrefix = QObject::tr("Can't list all linked notebooks in local storage database: ");

    QSqlQuery query(m_sqlDatabase);
    bool res = query.exec("SELECT * FROM LinkedNotebooks");
    if (!res) {
        errorDescription = errorPrefix + QObject::tr("can't select all linked notebooks from SQL database: ");
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

bool LocalStorageManager::ExpungeLinkedNotebook(const LinkedNotebook & linkedNotebook,
                                                QString & errorDescription)
{
    errorDescription = QObject::tr("Can't expunge linked notebook from local storage database: ");

    if (!linkedNotebook.hasGuid()) {
        errorDescription += QObject::tr("linked notebook's guid is not set");
        return false;
    }

    const QString linkedNotebookGuid = linkedNotebook.guid();

    if (!CheckGuid(linkedNotebookGuid)) {
        errorDescription += QObject::tr("linked notebook's guid is invalid");
        return false;
    }

    bool exists = RowExists("LinkedNotebooks", "guid", QVariant(linkedNotebookGuid));
    if (!exists) {
        errorDescription += QObject::tr("can't determine row id of linked notebook "
                                        "to be expunged in \"LinkedNotebooks\" table in SQL database");
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

bool LocalStorageManager::AddNote(const Note & note, const Notebook & notebook, QString & errorDescription)
{
    errorDescription = QObject::tr("Can't add note to local storage database: ");
    QString error;

    if (!notebook.canCreateNotes()) {
        errorDescription += QObject::tr("notebook's restrictions forbid notes creation");
        return false;
    }

    bool res = note.checkParameters(error);
    if (!res) {
        errorDescription += error;
        QNWARNING("Found invalid note: " << note);
        return false;
    }

    QString column, guid;
    bool noteHasGuid = note.hasGuid();
    if (noteHasGuid) {
        column = "guid";
        guid = note.guid();
    }
    else {
        column = "localGuid";
        guid = note.localGuid();
    }

    bool exists = RowExists("Notes", column, QVariant(guid));
    if (exists) {
        errorDescription += QObject::tr("note with specified ");
        errorDescription += (noteHasGuid ? QObject::tr("guid") : QObject::tr("local guid"));
        errorDescription += QObject::tr(" already exists");
        QNWARNING(errorDescription << ", " << column << ": " << guid);
        return false;
    }

    return InsertOrReplaceNote(note, notebook, errorDescription);
}

bool LocalStorageManager::UpdateNote(const Note & note, const Notebook & notebook, QString & errorDescription)
{
    errorDescription = QObject::tr("Can't update note in local storage database: ");
    QString error;

    if (!notebook.canUpdateNotes()) {
        errorDescription += QObject::tr("notebook's restrictions forbid update of notes");
        return false;
    }

    bool res = note.checkParameters(error);
    if (!res) {
        errorDescription += error;
        QNWARNING("Found invalid note: " << note);
        return false;
    }

    QString column, guid;
    bool noteHasGuid = note.hasGuid();
    if (noteHasGuid) {
        column = "guid";
        guid = note.guid();
    }
    else {
        column = "localGuid";
        guid = note.localGuid();
    }

    bool exists = RowExists("Notes", column, QVariant(guid));
    if (!exists) {
        errorDescription += QObject::tr("note with specified ");
        errorDescription += (noteHasGuid ? QObject::tr("guid") : QObject::tr("local guid"));
        errorDescription += QObject::tr(" was not found");
        QNWARNING(errorDescription << ", " << column << ": " << guid);
        return false;
    }

    return InsertOrReplaceNote(note, notebook, errorDescription);
}

bool LocalStorageManager::FindNote(Note & note, QString & errorDescription,
                                   const bool withResourceBinaryData) const
{
    QNDEBUG("LocalStorageManager::FindNote");

    errorDescription = QObject::tr("Can't find note in local storage database: ");

    QString column, guid;
    bool noteHasGuid = note.hasGuid();
    if (noteHasGuid) {
        column = "guid";
        guid = note.guid();

        if (!CheckGuid(guid)) {
            errorDescription += QObject::tr("requested note guid is invalid");
            return false;
        }
    }
    else {
        column = "local guid";
        guid = note.localGuid();
    }

    note = Note();

    QString queryString = QString("SELECT localGuid, guid, updateSequenceNumber, title, isDirty, isLocal, content, "
                                  "creationTimestamp, modificationTimestamp, isActive, isDeleted, deletionTimestamp, "
                                  "hasAttributes, notebookGuid FROM Notes WHERE %1 = \"%2\"").arg(column).arg(guid);
    QSqlQuery query(m_sqlDatabase);
    bool res = query.exec(queryString);
    DATABASE_CHECK_AND_SET_ERROR("can't select note from \"Notes\" table in SQL database");

    if (!query.next()) {
        errorDescription += QObject::tr("Internal error: SQL query result is empty");
        return false;
    }

    QSqlRecord rec = query.record();
    res = FillNoteFromSqlRecord(rec, note, errorDescription, withResourceBinaryData);
    if (!res) {
        return false;
    }

    QString error;
    res = FindAndSetTagGuidsPerNote(note, error);
    if (!res) {
        errorDescription += error;
        return false;
    }

    error.clear();
    res = FindAndSetResourcesPerNote(note, error, withResourceBinaryData);
    if (!res) {
        errorDescription += error;
        return false;
    }

    error.clear();
    res = note.checkParameters(error);
    if (!res) {
        errorDescription = QObject::tr("Found note is invalid: ");
        errorDescription += error;
        return false;
    }

    return true;
}

QList<Note> LocalStorageManager::ListAllNotesPerNotebook(const Notebook & notebook,
                                                         QString & errorDescription,
                                                         const bool withResourceBinaryData) const
{
    QNDEBUG("LocalStorageManager::ListAllNotesPerNotebook: notebookGuid = " << notebook);

    QString errorPrefix = QObject::tr("Can't find all notes per notebook: ");

    QList<Note> notes;

    QString column, guid;
    bool notebookHasGuid = notebook.hasGuid();
    if (notebookHasGuid) {
        column = "notebookGuid";
        guid = notebook.guid();

        if (!CheckGuid(guid)) {
            errorDescription = errorPrefix + QObject::tr("notebook guid is invalid");
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
        errorDescription = errorPrefix + QObject::tr("can't select notes per notebook guid from SQL database: ");
        QNCRITICAL(errorDescription << "last error = " << query.lastError() << ", last query = " << query.lastQuery());
        errorDescription += query.lastError().text();
        return notes;
    }

    notes.reserve(qMax(query.size(), 0));
    while(query.next())
    {
        notes << Note();
        Note & note = notes.back();

        QSqlRecord rec = query.record();

        res = FillNoteFromSqlRecord(rec, note, errorDescription, withResourceBinaryData);
        if (!res) {
            errorDescription.prepend(errorPrefix);
            notes.clear();
            return notes;
        }

        QString error;
        res = note.checkParameters(error);
        if (!res) {
            errorDescription = errorPrefix + QObject::tr("found note is invalid: ");
            errorDescription += error;
            notes.clear();
            return notes;
        }
    }

    return notes;
}

bool LocalStorageManager::DeleteNote(const Note & note, QString & errorDescription)
{
    if (note.isLocal()) {
        return ExpungeNote(note, errorDescription);
    }

    errorDescription = QObject::tr("Can't delete note from local storage database: ");

    QString column, guid;
    bool noteHasGuid = note.hasGuid();
    if(noteHasGuid) {
        column = "guid";
        guid = note.guid();

        if (!CheckGuid(guid)) {
            errorDescription += QObject::tr("note guid is invalid");
            return false;
        }
    }
    else {
        column = "localGuid";
        guid = note.localGuid();
    }

    if (!note.isDeleted()) {
        errorDescription += QObject::tr("note to be deleted is not marked as "
                                        "deleted one, rejecting to delete it");
        return false;
    }

    QString queryString = QString("UPDATE Notes SET isDeleted=1, isDirty=1 WHERE %1 = \"%2\"")
                                  .arg(column).arg(guid);
    QSqlQuery query(m_sqlDatabase);
    bool res = query.exec(queryString);
    DATABASE_CHECK_AND_SET_ERROR("can't delete entry from \"Notes\" table in SQL database");

    return true;
}

bool LocalStorageManager::ExpungeNote(const Note & note, QString & errorDescription)
{
    errorDescription = QObject::tr("Can't expunge note from local storage database: ");

    QString column, guid;
    bool noteHasGuid = note.hasGuid();
    if(noteHasGuid) {
        column = "guid";
        guid = note.guid();

        if (!CheckGuid(guid)) {
            errorDescription += QObject::tr("note guid is invalid");
            return false;
        }
    }
    else {
        column = "localGuid";
        guid = note.localGuid();
    }

    if (!note.isLocal()) {
        errorDescription += QObject::tr("note to be expunged is not local");
        return false;
    }

    if (!note.isDeleted()) {
        errorDescription += QObject::tr("note to be expunged is not marked as "
                                        "deleted one, rejecting to expunge it");
        return false;
    }

    bool exists = RowExists("Notes", column, QVariant(guid));
    if (!exists) {
        errorDescription += QObject::tr("can't note to be expunged in \"Notes\" table in SQL database");
        QNWARNING(errorDescription << ", " << column << ": " << guid);
        return false;
    }

    QString queryString = QString("DELETE FROM Notes WHERE %1 = \"%2\"").arg(column).arg(guid);
    QSqlQuery query(m_sqlDatabase);
    bool res = query.exec(queryString);
    DATABASE_CHECK_AND_SET_ERROR("can't delete entry from \"Notes\" table in SQL database");

    return true;
}

bool LocalStorageManager::AddTag(const Tag & tag, QString & errorDescription)
{
    errorDescription = QObject::tr("Can't add tag to local storage database: ");
    QString error;

    bool res = tag.checkParameters(error);
    if (!res) {
        errorDescription += error;
        QNWARNING("Found invalid tag: " << tag);
        return false;
    }

    QString column, guid;
    bool tagHasGuid = tag.hasGuid();
    if (tagHasGuid) {
        column = "guid";
        guid = tag.guid();
    }
    else {
        column = "localGuid";
        guid = tag.localGuid();
    }

    bool exists = RowExists("Tags", column, QVariant(guid));
    if (exists) {
        errorDescription += QObject::tr("tag with the same ");
        errorDescription += (tagHasGuid ? QObject::tr("guid") : QObject::tr("local guid"));
        errorDescription += QObject::tr(" already exists");
        QNWARNING(errorDescription << ", " << column << ": " << guid);
        return false;
    }

    QString nameUpper = tag.name().toUpper();
    exists = RowExists("Tags", "nameUpper", QVariant(nameUpper));
    if (exists) {
        errorDescription += QObject::tr("tag with similar name (case insensitive) "
                                        "already exists");
        QNWARNING(errorDescription << ", nameUpper: " << nameUpper);
        return false;
    }

    return InsertOrReplaceTag(tag, errorDescription);
}

bool LocalStorageManager::UpdateTag(const Tag & tag, QString & errorDescription)
{
    errorDescription = QObject::tr("Can't update tag in local storage database: ");
    QString error;

    bool res = tag.checkParameters(error);
    if (!res) {
        errorDescription += error;
        QNWARNING("Found invalid tag: " << tag);
        return false;
    }

    QString column, guid;
    bool tagHasGuid = tag.hasGuid();
    if (tagHasGuid) {
        column = "guid";
        guid = tag.guid();
    }
    else {
        column = "localGuid";
        guid = tag.localGuid();
    }

    bool exists = RowExists("Tags", column, QVariant(guid));
    if (!exists) {
        errorDescription += QObject::tr("tag with specified ");
        errorDescription += (tagHasGuid ? QObject::tr("guid") : QObject::tr("local guid"));
        errorDescription += QObject::tr(" was not found");
        QNWARNING(errorDescription << ", " << column << ": " << guid);
        return false;
    }

    return InsertOrReplaceTag(tag, errorDescription);
}

bool LocalStorageManager::LinkTagWithNote(const Tag & tag, const Note & note,
                                          QString & errorDescription)
{
    errorDescription = QObject::tr("Can't link tag with note: ");
    QString error;

    if (!tag.checkParameters(error)) {
        errorDescription += QObject::tr("tag is invalid: ");
        errorDescription += error;
        return false;
    }

    if (!note.checkParameters(errorDescription)) {
        errorDescription += QObject::tr("note is invalid: ");
        errorDescription += error;
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

bool LocalStorageManager::FindTag(Tag & tag, QString & errorDescription) const
{
    QNDEBUG("LocalStorageManager::FindTag");

    QString errorPrefix = QObject::tr("Can't find tag in local storage database: ");

    QString column, guid;
    bool tagHasGuid = tag.hasGuid();
    if (tagHasGuid) {
        column = "guid";
        guid = tag.guid();

        if (!CheckGuid(guid)) {
            errorDescription = errorPrefix + QObject::tr("requested tag guid is invalid");
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
        errorDescription = errorPrefix + QObject::tr("Internal error: SQL query result is empty");
        return false;
    }

    QSqlRecord record = query.record();
    return FillTagFromSqlRecord(record, tag, errorDescription);
}

QList<Tag> LocalStorageManager::ListAllTagsPerNote(const Note & note, QString & errorDescription) const
{
    QNDEBUG("LocalStorageManager::ListAllTagsPerNote");

    QList<Tag> tags;
    QString errorPrefix = QObject::tr("Can't find all tags per note in local storage database: ");

    QString column, guid;
    bool noteHasGuid = note.hasGuid();
    if (noteHasGuid) {
        column = "note";
        guid = note.guid();

        if (!CheckGuid(guid)) {
            errorDescription = errorPrefix + QObject::tr("note's guid is invalid");
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
        errorDescription = errorPrefix + QObject::tr("can't select tag guids from \"NoteTags\" table in SQL database: ");
        QNWARNING(errorDescription << "last error = " << query.lastError() << ", last query = " << query.lastQuery());
        errorDescription += query.lastError().text();
        return tags;
    }

    tags = FillTagsFromSqlQuery(query, errorDescription);
    if (tags.isEmpty() && !errorDescription.isEmpty()) {
        errorDescription.prepend(errorPrefix);
    }

    QNDEBUG("found " << tags.size() << " tags");

    return tags;
}

QList<Tag> LocalStorageManager::ListAllTags(QString & errorDescription) const
{
    QNDEBUG("LocalStorageManager::ListAllTags");

    QList<Tag> tags;
    QString errorPrefix = QObject::tr("Can't list all tags in local storage database: ");

    QSqlQuery query(m_sqlDatabase);
    bool res = query.exec("SELECT localGuid FROM Tags");
    if (!res) {
        errorDescription = errorPrefix + QObject::tr("can't select tag guids from SQL database: ");
        QNWARNING(errorDescription << "last error = " << query.lastError() << ", last query = " << query.lastQuery());
        errorDescription += query.lastError().text();
        return tags;
    }

    tags = FillTagsFromSqlQuery(query, errorDescription);
    if (tags.isEmpty() && !errorDescription.isEmpty()) {
        errorDescription.prepend(errorPrefix);
    }

    QNDEBUG("found " << tags.size() << " tags");

    return tags;
}

bool LocalStorageManager::DeleteTag(const Tag & tag, QString & errorDescription)
{
    if (tag.isLocal()) {
        return ExpungeTag(tag, errorDescription);
    }

    errorDescription = QObject::tr("Can't delete tag from local storage database: ");

    if (!tag.isDeleted()) {
        errorDescription += QObject::tr("tag to be deleted is not marked as deleted one");
        return false;
    }

    QSqlQuery query(m_sqlDatabase);
    query.prepare("UPDATE Tags SET isDeleted = 1, isDirty = 1 WHERE guid = ?");
    query.addBindValue(tag.guid());
    bool res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("can't mark tag deleted in \"Tags\" table in SQL database");

    return true;
}

bool LocalStorageManager::ExpungeTag(const Tag & tag, QString & errorDescription)
{
    errorDescription = QObject::tr("Can't expunge tag from local storage database: ");

    if (!tag.isLocal()) {
        errorDescription += QObject::tr("tag to be expunged is not local");
        return false;
    }

    if (!tag.isDeleted()) {
        errorDescription += QObject::tr("tag to be expunged is not marked as deleted one");
        return false;
    }

    if (!tag.hasGuid()) {
        errorDescription += QObject::tr("tag's guid is not set");
        return false;
    }
    else if (!CheckGuid(tag.guid())) {
        errorDescription += QObject::tr("tag's guid is invalid");
        return false;
    }

    bool exists = RowExists("Tags", "guid", QVariant(tag.guid()));
    if (!exists) {
        errorDescription += QObject::tr("tag to be expunged was not found by guid");
        QNWARNING(errorDescription << ", guid: " << tag.guid());
        return false;
    }

    QSqlQuery query(m_sqlDatabase);
    query.prepare("DELETE FROM Tags WHERE guid = ?");
    query.addBindValue(tag.guid());

    bool res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("can't delete tag from \"Tags\" table in SQL database");

    return true;
}

bool LocalStorageManager::FindEnResource(IResource & resource, QString & errorDescription,
                                         const bool withBinaryData) const
{
    QNDEBUG("LocalStorageManager::FindEnResource");

    errorDescription = QObject::tr("Can't find resource in local storage database: ");

    QString column, guid;
    bool resourceHasGuid = resource.hasGuid();
    if (resourceHasGuid) {
        column = "guid";
        guid = resource.guid();

        if (!CheckGuid(guid)) {
            errorDescription += QObject::tr("requested resource guid is invalid");
            return false;
        }
    }
    else {
        column = "localGuid";
        guid = resource.localGuid();
    }

    resource.clear();

    QString queryString = QString("SELECT localGuid, guid, noteGuid, noteLocalGuid, updateSequenceNumber, "
                                  "isDirty, dataSize, dataHash, mime, width, height, recognitionDataSize, "
                                  "recognitionDataHash, indexInNote %1 FROM Resources WHERE %2 = \"%3\"")
                                 .arg(withBinaryData ? ", dataBody, recognitionDataBody" : " ")
                                 .arg(column).arg(guid);

    QSqlQuery query(m_sqlDatabase);
    bool res = query.exec(queryString);
    DATABASE_CHECK_AND_SET_ERROR("can't find resource in \"Resources\" table in SQL database");

    if (!query.next()) {
        errorDescription += QObject::tr("Internal error: SQL query result is empty");
        return false;
    }

    QSqlRecord rec = query.record();

#define CHECK_AND_SET_RESOURCE_PROPERTY(property, type, localType, setter, isRequired) \
    if (rec.contains(#property)) { \
        resource.setter(static_cast<localType>(qvariant_cast<type>(rec.value(#property)))); \
    } \
    else if (isRequired) { \
        errorDescription += QObject::tr("no " #property " field in the result of SQL query"); \
        return false; \
    }

    bool isRequired = true;
    CHECK_AND_SET_RESOURCE_PROPERTY(localGuid, QString, QString, setLocalGuid, isRequired);
    CHECK_AND_SET_RESOURCE_PROPERTY(isDirty, int, bool, setDirty, isRequired);
    CHECK_AND_SET_RESOURCE_PROPERTY(noteGuid, QString, QString, setNoteGuid, isRequired);
    CHECK_AND_SET_RESOURCE_PROPERTY(updateSequenceNumber, int, qint32, setUpdateSequenceNumber, isRequired);
    CHECK_AND_SET_RESOURCE_PROPERTY(dataSize, int, qint32, setDataSize, isRequired);
    CHECK_AND_SET_RESOURCE_PROPERTY(dataHash, QByteArray, QByteArray, setDataHash, isRequired);
    CHECK_AND_SET_RESOURCE_PROPERTY(mime, QString, QString, setMime, isRequired);

    isRequired = false;

    CHECK_AND_SET_RESOURCE_PROPERTY(guid, QString, QString, setGuid, isRequired);
    CHECK_AND_SET_RESOURCE_PROPERTY(width, int, qint32, setWidth, isRequired);
    CHECK_AND_SET_RESOURCE_PROPERTY(height, int, qint32, setHeight, isRequired);
    CHECK_AND_SET_RESOURCE_PROPERTY(recognitionDataSize, int, qint32, setRecognitionDataSize, isRequired);
    CHECK_AND_SET_RESOURCE_PROPERTY(recognitionDataHash, QByteArray, QByteArray, setRecognitionDataHash, isRequired);
    CHECK_AND_SET_RESOURCE_PROPERTY(indexInNote, int, int, setIndexInNote, isRequired);

    if (withBinaryData) {
        CHECK_AND_SET_RESOURCE_PROPERTY(recognitionDataBody, QByteArray, QByteArray, setRecognitionDataBody, isRequired);
        CHECK_AND_SET_RESOURCE_PROPERTY(dataBody, QByteArray, QByteArray, setDataBody, isRequired);
    }

    // ======= 1) Fetch the common part of resource attributes =======
    qevercloud::ResourceAttributes attributes;
    bool hasAttributes = false;

    queryString = QString("SELECT * FROM ResourceAttributes WHERE localGuid = '%1'").arg(resource.localGuid());
    res = query.exec(queryString);
    DATABASE_CHECK_AND_SET_ERROR("can't select data from \"ResourceAttributes\" table in SQL database");

    while(query.next()) {
        QSqlRecord rec = query.record();
        hasAttributes |= FillResourceAttributesFromSqlRecord(rec, attributes);
    }

    // ======== 2) Fetch applicationData.keysOnly =======
    queryString = QString("SELECT * FROM ResourceAttributesApplicationDataKeysOnly WHERE localGuid = '%1'").arg(resource.localGuid());
    res = query.exec(queryString);
    DATABASE_CHECK_AND_SET_ERROR("can't select data from \"ResourceAttributesApplicationDataKeysOnly\" table in SQL database");

    while(query.next()) {
        QSqlRecord rec = query.record();
        hasAttributes |= FillResourceAttributesApplicationDataKeysOnlyFromSqlRecord(rec, attributes);
    }

    // ======== 3) Fetch applicationData.fullMap =======
    queryString = QString("SELECT * FROM ResourceAttributesApplicationDataFullMap WHERE localGuid = '%1'").arg(resource.localGuid());
    res = query.exec(queryString);
    DATABASE_CHECK_AND_SET_ERROR("can't select data from \"ResourceAttributesApplicationFullMapOnly\" table in SQL database");

    while(query.next()) {
        QSqlRecord rec = query.record();
        hasAttributes |= FillResourceAttributesApplicationDataFullMapFromSqlRecord(rec, attributes);
    }

    if (hasAttributes) {
        resource.setResourceAttributes(attributes);
    }

    return true;
}

bool LocalStorageManager::ExpungeEnResource(const IResource & resource, QString & errorDescription)
{
    errorDescription = QObject::tr("Can't expunge resource from local storage database: ");

    QString column, guid;
    bool resourceHasGuid = resource.hasGuid();
    if (resourceHasGuid) {
        column = "guid";
        guid = resource.guid();

        if (!CheckGuid(guid)) {
            errorDescription += QObject::tr("requested resource guid is invalid");
            return false;
        }
    }
    else {
        column = "localGuid";
        guid = resource.localGuid();
    }

    bool exists = RowExists("Resources", column, QVariant(guid));
    if (!exists) {
        errorDescription += QObject::tr("resource to be expunged was not found by guid");
        QNWARNING(errorDescription << ", " << column << ": " << guid);
        return false;
    }

    QString queryString = QString("DELETE FROM Resources WHERE %1 = \"%2\"").arg(column).arg(guid);
    QSqlQuery query(m_sqlDatabase);
    bool res = query.exec(queryString);
    DATABASE_CHECK_AND_SET_ERROR("can't delete resource from \"Resources\" table in SQL database");

    return true;
}

bool LocalStorageManager::AddSavedSearch(const SavedSearch & search, QString & errorDescription)
{
    errorDescription = QObject::tr("Can't add saved search to local storage database: ");
    QString error;

    bool res = search.checkParameters(error);
    if (!res) {
        errorDescription += error;
        QNWARNING("Found invalid SavedSearch: " << search << "\nError: " << error);
        return false;
    }

    QString column, guid;
    bool searchHasGuid = search.hasGuid();
    if (searchHasGuid) {
        column = "guid";
        guid = search.guid();
    }
    else {
        column = "localGuid";
        guid = search.localGuid();
    }

    bool exists = RowExists("SavedSearches", column, QVariant(guid));
    if (exists) {
        errorDescription += QObject::tr("saved search with the same ");
        errorDescription += (searchHasGuid ? QObject::tr("guid") : QObject::tr("local guid"));
        errorDescription += QObject::tr(" already exists");
        QNWARNING(errorDescription << ", " << column << ": " << guid);
        return false;
    }

    return InsertOrReplaceSavedSearch(search, errorDescription);
}

bool LocalStorageManager::UpdateSavedSearch(const SavedSearch & search,
                                            QString & errorDescription)
{
    errorDescription = QObject::tr("Can't update saved search in local storage database: ");
    QString error;

    bool res = search.checkParameters(error);
    if (!res) {
        errorDescription += error;
        QNWARNING("Found invalid SavedSearch: " << search << "\nError: " << error);
        return false;
    }

    QString column, guid;
    bool searchHasGuid = search.hasGuid();
    if (searchHasGuid) {
        column = "guid";
        guid = search.guid();
    }
    else {
        column = "localGuid";
        guid = search.localGuid();
    }

    bool exists = RowExists("SavedSearches", column, QVariant(guid));
    if (!exists) {
        errorDescription += QObject::tr("saved search with specified ");
        errorDescription += (searchHasGuid ? QObject::tr("guid") : QObject::tr("local guid"));
        errorDescription += QObject::tr(" was not found");
        QNWARNING(errorDescription << ", " << column << ": " << guid);
        return false;
    }

    return InsertOrReplaceSavedSearch(search, errorDescription);
}

bool LocalStorageManager::FindSavedSearch(SavedSearch & search, QString & errorDescription) const
{
    QNDEBUG("LocalStorageManager::FindSavedSearch");

    errorDescription = QObject::tr("Can't find saved search in local storage database: ");

    QString column, guid;
    bool searchHasGuid = search.hasGuid();
    if (searchHasGuid) {
        column = "guid";
        guid = search.guid();

        if (!CheckGuid(guid)) {
            errorDescription += QObject::tr("requested saved search guid is invalid");
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
        errorDescription += QObject::tr("Internal error: SQL query result is empty");
        return false;
    }

    QSqlRecord rec = query.record();
    return FillSavedSearchFromSqlRecord(rec, search, errorDescription);
}

QList<SavedSearch> LocalStorageManager::ListAllSavedSearches(QString & errorDescription) const
{
    QNDEBUG("LocalStorageManager::ListAllSavedSearches");

    QList<SavedSearch> searches;

    QString errorPrefix = QObject::tr("Can't find all saved searches in local storage database");

    QSqlQuery query(m_sqlDatabase);
    bool res = query.exec("SELECT * FROM SavedSearches");
    if (!res) {
        errorDescription = errorPrefix + QObject::tr("can't select all saved searches from "
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

bool LocalStorageManager::ExpungeSavedSearch(const SavedSearch & search,
                                             QString & errorDescription)
{
    errorDescription = QObject::tr("Can't expunge saved search from local storage database: ");

    QString localGuid = search.localGuid();
    if (localGuid.isEmpty()) {
        errorDescription += QObject::tr("saved search's local guid is not set");
        return false;
    }

    bool exists = RowExists("SavedSearches", "localGuid", QVariant(localGuid));
    if (!exists) {
        errorDescription += QObject::tr("saved search to be expunged was not found");
        return false;
    }

    QSqlQuery query(m_sqlDatabase);
    query.prepare("DELETE FROM SavedSearches WHERE localGuid = ?");
    query.addBindValue(localGuid);

    bool res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("can't delete saved search from \"SavedSearches\" table in SQL database");

    return true;
}

bool LocalStorageManager::AddEnResource(const IResource & resource, const Note & note, QString & errorDescription)
{
    errorDescription = QObject::tr("Can't add resource to local storage database: ");

    QString error;
    if (!resource.checkParameters(error)) {
        errorDescription += error;
        QNWARNING("Found invalid resource: " << resource);
        return false;
    }

    QString column, guid;
    bool resourceHasGuid = resource.hasGuid();
    if (resourceHasGuid) {
        column = "guid";
        guid = resource.guid();
    }
    else {
        column = "localGuid";
        guid = resource.localGuid();
    }

    bool exists = RowExists("Resources", column, QVariant(guid));
    if (exists) {
        errorDescription += QObject::tr("resource with the same ");
        errorDescription += (resourceHasGuid ? QObject::tr("guid") : QObject::tr("local guid"));
        errorDescription += QObject::tr(" already exists");
        QNWARNING(errorDescription << ", " << column << ": " << guid);
        return false;
    }

    return InsertOrReplaceResource(resource, note, errorDescription);
}

bool LocalStorageManager::UpdateEnResource(const IResource & resource, const Note &note, QString & errorDescription)
{
    errorDescription = QObject::tr("Can't update resource in local storage database: ");

    QString error;
    if (!resource.checkParameters(error)) {
        errorDescription += error;
        QNWARNING("Found invalid resource: " << resource);
        return false;
    }

    QString column, guid;
    bool resourceHasGuid = resource.hasGuid();
    if (resourceHasGuid) {
        column = "guid";
        guid = resource.guid();
    }
    else {
        column = "localGuid";
        guid = resource.localGuid();
    }

    bool exists = RowExists("Resources", column, QVariant(guid));
    if (!exists) {
        errorDescription += QObject::tr("resource to be updated was not found");
        QNWARNING(errorDescription << ", " << column << ": " << guid);
        return false;
    }

    return InsertOrReplaceResource(resource, note, errorDescription);
}

bool LocalStorageManager::CreateTables(QString & errorDescription)
{
    QSqlQuery query(m_sqlDatabase);
    bool res;

    res = query.exec("CREATE TABLE IF NOT EXISTS Users("
                     "  id                      INTEGER PRIMARY KEY     NOT NULL UNIQUE, "
                     "  username                TEXT                    NOT NULL, "
                     "  email                   TEXT                    NOT NULL, "
                     "  name                    TEXT                    NOT NULL, "
                     "  timezone                TEXT                    DEFAULT NULL, "
                     "  privilege               INTEGER                 NOT NULL, "
                     "  creationTimestamp       INTEGER                 NOT NULL, "
                     "  modificationTimestamp   INTEGER                 NOT NULL, "
                     "  isDirty                 INTEGER                 NOT NULL, "
                     "  isLocal                 INTEGER                 NOT NULL, "
                     "  deletionTimestamp       INTEGER                 DEFAULT -1, "
                     "  isActive                INTEGER                 NOT NULL"
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
                     "  useEmailAutoFiling         INTEGER                 DEFAULT NULL, "
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
                     "  businessId                  INTEGER             DEFAULT NULL, "
                     "  businessName                TEXT                DEFAULT NULL, "
                     "  businessRole                INTEGER             DEFAULT NULL, "
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
                     "  email                   TEXT                    DEFAULT NULL)");
    DATABASE_CHECK_AND_SET_ERROR("can't create BusinessUserInfo table");

    res = query.exec("CREATE TABLE IF NOT EXISTS Notebooks("
                     "  localGuid                       TEXT PRIMARY KEY  NOT NULL UNIQUE, "
                     "  guid                            TEXT              DEFAULT NULL UNIQUE, "
                     "  updateSequenceNumber            INTEGER           NOT NULL, "
                     "  name                            TEXT              NOT NULL, "
                     "  creationTimestamp               INTEGER           NOT NULL, "
                     "  modificationTimestamp           INTEGER           NOT NULL, "
                     "  isDirty                         INTEGER           NOT NULL, "
                     "  isLocal                         INTEGER           NOT NULL, "
                     "  isDefault                       INTEGER           DEFAULT 0, "
                     "  isLastUsed                      INTEGER           DEFAULT 0, "
                     "  publishingUri                   TEXT              DEFAULT NULL, "
                     "  publishingNoteSortOrder         INTEGER           DEFAULT 0, "
                     "  publishingAscendingSort         INTEGER           DEFAULT 0, "
                     "  publicDescription               TEXT              DEFAULT NULL, "
                     "  isPublished                     INTEGER           DEFAULT 0, "
                     "  stack                           TEXT              DEFAULT NULL, "
                     "  businessNotebookDescription     TEXT              DEFAULT NULL, "
                     "  businessNotebookPrivilegeLevel  INTEGER           DEFAULT 0, "
                     "  businessNotebookIsRecommended   INTEGER           DEFAULT 0, "
                     "  contactId                       INTEGER           DEFAULT NULL, "
                     "  UNIQUE(localGuid, guid)"
                     ")");
    DATABASE_CHECK_AND_SET_ERROR("can't create Notebooks table");

    res = query.exec("CREATE TABLE IF NOT EXISTS NotebookRestrictions("
                     "  localGuid REFERENCES Notebooks(localGuid) ON DELETE CASCADE ON UPDATE CASCADE, "
                     "  noReadNotes                 INTEGER     DEFAULT 0, "
                     "  noCreateNotes               INTEGER     DEFAULT 0, "
                     "  noUpdateNotes               INTEGER     DEFAULT 0, "
                     "  noExpungeNotes              INTEGER     DEFAULT 0, "
                     "  noShareNotes                INTEGER     DEFAULT 0, "
                     "  noEmailNotes                INTEGER     DEFAULT 0, "
                     "  noSendMessageToRecipients   INTEGER     DEFAULT 0, "
                     "  noUpdateNotebook            INTEGER     DEFAULT 0, "
                     "  noExpungeNotebook           INTEGER     DEFAULT 0, "
                     "  noSetDefaultNotebook        INTEGER     DEFAULT 0, "
                     "  noSetNotebookStack          INTEGER     DEFAULT 0, "
                     "  noPublishToPublic           INTEGER     DEFAULT 0, "
                     "  noPublishToBusinessLibrary  INTEGER     DEFAULT 0, "
                     "  noCreateTags                INTEGER     DEFAULT 0, "
                     "  noUpdateTags                INTEGER     DEFAULT 0, "
                     "  noExpungeTags               INTEGER     DEFAULT 0, "
                     "  noSetParentTag              INTEGER     DEFAULT 0, "
                     "  noCreateSharedNotebooks     INTEGER     DEFAULT 0, "
                     "  updateWhichSharedNotebookRestrictions    INTEGER     DEFAULT 0, "
                     "  expungeWhichSharedNotebookRestrictions   INTEGER     DEFAULT 0"
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
                     "  businessId                      INTEGER           DEFAULT 0"
                     ")");
    DATABASE_CHECK_AND_SET_ERROR("can't create LinkedNotebooks table");

    res = query.exec("CREATE TABLE IF NOT EXISTS SharedNotebooks("
                     "  shareId                         INTEGER    NOT NULL, "
                     "  userId                          INTEGER    NOT NULL, "
                     "  notebookGuid REFERENCES Notebooks(guid) ON DELETE CASCADE ON UPDATE CASCADE, "
                     "  email                           TEXT       NOT NULL, "
                     "  creationTimestamp               INTEGER    NOT NULL, "
                     "  modificationTimestamp           INTEGER    NOT NULL, "
                     "  shareKey                        TEXT       NOT NULL, "
                     "  username                        TEXT       NOT NULL, "
                     "  sharedNotebookPrivilegeLevel    INTEGER    NOT NULL, "
                     "  allowPreview                    INTEGER    DEFAULT 0, "
                     "  recipientReminderNotifyEmail    INTEGER    DEFAULT 0, "
                     "  recipientReminderNotifyInApp    INTEGER    DEFAULT 0, "
                     "  indexInNotebook                 INTEGER    DEFAULT NULL, "
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
                     "  isDeleted               INTEGER              DEFAULT 0, "
                     "  title                   TEXT                 NOT NULL, "
                     "  content                 TEXT                 NOT NULL, "
                     "  creationTimestamp       INTEGER              NOT NULL, "
                     "  modificationTimestamp   INTEGER              NOT NULL, "
                     "  deletionTimestamp       INTEGER              DEFAULT 0, "
                     "  isActive                INTEGER              DEFAULT 1, "
                     "  hasAttributes           INTEGER              DEFAULT 0, "
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
                     "  key                     TEXT                DEFAULT NULL UNIQUE)");
    DATABASE_CHECK_AND_SET_ERROR("can't create NoteAttributesApplicationDataKeysOnly table");

    res = query.exec("CREATE TABLE IF NOT EXISTS NoteAttributesApplicationDataFullMap("
                     "  noteLocalGuid REFERENCES Notes(localGuid) ON DELETE CASCADE ON UPDATE CASCADE, "
                     "  key                     TEXT                DEFAULT NULL UNIQUE, "
                     "  value                   TEXT                DEFAULT NULL)");
    DATABASE_CHECK_AND_SET_ERROR("can't create NoteAttributesApplicationDataFullMap table");

    res = query.exec("CREATE TABLE IF NOT EXISTS NoteAttributesClassifications("
                     "  noteLocalGuid REFERENCES Notes(localGuid) ON DELETE CASCADE ON UPDATE CASCADE, "
                     "  key                     TEXT                DEFAULT NULL UNIQUE, "
                     "  value                   TEXT                DEFAULT NULL)");
    DATABASE_CHECK_AND_SET_ERROR("can't create NoteAttributesClassifications table");

    res = query.exec("CREATE INDEX IF NOT EXISTS NotesNotebooks ON Notes(notebookLocalGuid)");
    DATABASE_CHECK_AND_SET_ERROR("can't create index NotesNotebooks");

    res = query.exec("CREATE TABLE IF NOT EXISTS Resources("
                     "  localGuid               TEXT PRIMARY KEY     NOT NULL UNIQUE, "
                     "  guid                    TEXT                 DEFAULT NULL UNIQUE, "
                     "  noteLocalGuid REFERENCES Notes(localGuid) ON DELETE CASCADE ON UPDATE CASCADE, "
                     "  noteGuid REFERENCES Notes(guid) ON DELETE CASCADE ON UPDATE CASCADE, "
                     "  updateSequenceNumber    INTEGER              NOT NULL, "
                     "  isDirty                 INTEGER              NOT NULL, "
                     "  dataBody                TEXT                 NOT NULL, "
                     "  dataSize                INTEGER              NOT NULL, "
                     "  dataHash                TEXT                 NOT NULL, "
                     "  mime                    TEXT                 NOT NULL, "
                     "  width                   INTEGER              DEFAULT 0, "
                     "  height                  INTEGER              DEFAULT 0, "
                     "  recognitionDataBody     TEXT                 DEFAULT NULL, "
                     "  recognitionDataSize     INTEGER              DEFAULT 0, "
                     "  recognitionDataHash     TEXT                 DEFAULT NULL, "
                     "  indexInNote             INTEGER              DEFAULT NULL, "
                     "  UNIQUE(localGuid, guid)"
                     ")");
    DATABASE_CHECK_AND_SET_ERROR("can't create Resources table");

    res = query.exec("CREATE INDEX IF NOT EXISTS ResourceNote ON Resources(noteLocalGuid)");
    DATABASE_CHECK_AND_SET_ERROR("can't create ResourceNote index");

    res = query.exec("CREATE TABLE IF NOT EXISTS ResourceAttributes("
                     "  localGuid REFERENCES Resources(localGuid) ON DELETE CASCADE ON UPDATE CASCADE, "
                     "  sourceURL               TEXT                DEFAULT NULL, "
                     "  timestamp               INTEGER             DEFAULT NULL, "
                     "  latitude                REAL                DEFAULT NULL, "
                     "  longitude               REAL                DEFAULT NULL, "
                     "  altitude                REAL                DEFAULT NULL, "
                     "  cameraMake              TEXT                DEFAULT NULL, "
                     "  cameraModel             TEXT                DEFAULT NULL, "
                     "  clientWillIndex         INTEGER             DEFAULT NULL, "
                     "  recoType                TEXT                DEFAULT NULL, "
                     "  fileName                TEXT                DEFAULT NULL, "
                     "  attachment              INTEGER             DEFAULT NULL)");
    DATABASE_CHECK_AND_SET_ERROR("can't create ResourceAttributes table");

    res = query.exec("CREATE TABLE IF NOT EXISTS ResourceAttributesApplicationDataKeysOnly("
                     "  localGuid REFERENCES Resources(localGuid) ON DELETE CASCADE ON UPDATE CASCADE, "
                     "  key                     TEXT                DEFAULT NULL UNIQUE)");
    DATABASE_CHECK_AND_SET_ERROR("can't create ResourceAttributesApplicationDataKeysOnly table");

    res = query.exec("CREATE TABLE IF NOT EXISTS ResourceAttributesApplicationDataFullMap("
                     "  localGuid REFERENCES Resources(localGuid) ON DELETE CASCADE ON UPDATE CASCADE, "
                     "  key                     TEXT                DEFAULT NULL UNIQUE, "
                     "  value                   TEXT                DEFAULT NULL)");
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
                     "  isDeleted             INTEGER              DEFAULT 0"
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
                     "  note     REFERENCES Notes(localGuid)     ON DELETE CASCADE ON UPDATE CASCADE, "
                     "  resource REFERENCES Resources(localGuid) ON DELETE CASCADE ON UPDATE CASCADE, "
                     "  UNIQUE(note, resource) ON CONFLICT REPLACE)");
    DATABASE_CHECK_AND_SET_ERROR("can't create NoteResources table");

    res = query.exec("CREATE INDEX IF NOT EXISTS NoteResourcesNote ON NoteResources(note)");
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

bool LocalStorageManager::SetNoteAttributes(const Note & note, QString & errorDescription)
{
    QNDEBUG("LocalStorageManager::SetNoteAttributes");

    if (!note.hasNoteAttributes()) {
        return true;
    }

    const QString localGuid = note.localGuid();

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
                queryString = QString("INSERT OR REPLACE INTO NoteAttributesApplicationDataKeysOnly(noteLocalGuid, key) VALUES('%1', '%2')")
                                      .arg(localGuid).arg(key);
                res = query.exec();
                DATABASE_CHECK_AND_SET_ERROR("can't insert or replace data into \"NoteAttributesApplicationDataKeysOnly\" table in SQL database");
            }
        }

        if (attributes.applicationData->fullMap.isSet())
        {
            const QMap<QString, QString> & fullMap = attributes.applicationData->fullMap.ref();
            foreach(const QString & key, fullMap.keys()) {
                queryString = QString("INSERT OR REPLACE INTO NoteAttributesApplicationDataFullMap(noteLocalGuid, key, value) VALUES('%1', '%2', '%3')")
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
            queryString = QString("INSERT OR REPLACE INTO NoteAttributesClassifications(noteLocalGuid, key, value) VALUES('%1', '%2', '%3')")
                                  .arg(localGuid).arg(key).arg(map.value(key));
            res = query.exec();
            DATABASE_CHECK_AND_SET_ERROR("can't insert or replace data into \"NoteAttributesClassifications\" table in SQL database");
        }
    }

    return true;
}

bool LocalStorageManager::SetNotebookAdditionalAttributes(const Notebook & notebook,
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
            errorDescription += QObject::tr("notebook's contact is invalid: ");
            errorDescription += error;
            return false;
        }

        error.clear();
        res = InsertOrReplaceUser(user, error);
        if (!res) {
            errorDescription += QObject::tr("can't insert or replace notebook's contact: ");
            errorDescription += error;
            return false;
        }
    }

#undef APPEND_SEPARATOR

    if (hasAdditionalAttributes)
    {
        queryString.prepend("UPDATE Notebooks SET ");
        queryString.append(" WHERE localGuid = ");
        queryString.append("\"" + notebook.localGuid() + "\"");

        QSqlQuery query(m_sqlDatabase);
        bool res = query.exec(queryString);
        DATABASE_CHECK_AND_SET_ERROR("can't insert or replace additional notebook attributes "
                                     "into \"Notebooks\" table in SQL database");
    }

    if (notebook.hasRestrictions())
    {
        QString error;
        bool res = SetNotebookRestrictions(notebook.restrictions(), notebook.localGuid(), error);
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

bool LocalStorageManager::SetNotebookRestrictions(const qevercloud::NotebookRestrictions & notebookRestrictions,
                                                  const QString & notebookLocalGuid, QString & errorDescription)
{
    errorDescription = QObject::tr("Can't set notebook restrictions: ");

    bool hasAnyRestriction = false;

    QString columns = "localGuid";
    QString values = QString("\"%1\"").arg(notebookLocalGuid);

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

bool LocalStorageManager::SetSharedNotebookAttributes(const ISharedNotebook & sharedNotebook,
                                                      QString & errorDescription)
{
    errorDescription = QObject::tr("Can't set shared notebook attributes: ");

    if (!sharedNotebook.hasId()) {
        errorDescription += QObject::tr("shared notebook's share id is not set");
        return false;
    }

    if (!sharedNotebook.hasNotebookGuid()) {
        errorDescription += QObject::tr("shared notebook's notebook guid is not set");
        return false;
    }
    else if (!CheckGuid(sharedNotebook.notebookGuid())) {
        errorDescription += QObject::tr("shared notebook's notebook guid is invalid");
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

    CHECK_AND_SET_SHARED_NOTEBOOK_ATTRIBUTE(hasId, shareId,
                                            QString::number(sharedNotebook.id()));
    CHECK_AND_SET_SHARED_NOTEBOOK_ATTRIBUTE(hasUserId, userId,
                                            QString::number(sharedNotebook.userId()));
    CHECK_AND_SET_SHARED_NOTEBOOK_ATTRIBUTE(hasNotebookGuid, notebookGuid, "\"" + sharedNotebook.notebookGuid() + "\"");
    CHECK_AND_SET_SHARED_NOTEBOOK_ATTRIBUTE(hasEmail, email, "\"" + sharedNotebook.email() + "\"");
    CHECK_AND_SET_SHARED_NOTEBOOK_ATTRIBUTE(hasCreationTimestamp,creationTimestamp,
                                            QString::number(sharedNotebook.creationTimestamp()));
    CHECK_AND_SET_SHARED_NOTEBOOK_ATTRIBUTE(hasModificationTimestamp, modificationTimestamp,
                                            QString::number(sharedNotebook.modificationTimestamp()));
    CHECK_AND_SET_SHARED_NOTEBOOK_ATTRIBUTE(hasShareKey, shareKey, "\"" + sharedNotebook.shareKey() + "\"");
    CHECK_AND_SET_SHARED_NOTEBOOK_ATTRIBUTE(hasUsername, username, "\"" + sharedNotebook.username() + "\"");
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

bool LocalStorageManager::RowExists(const QString & tableName, const QString & uniqueKeyName,
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

bool LocalStorageManager::InsertOrReplaceUser(const IUser & user, QString & errorDescription)
{
    errorDescription += QObject::tr("Can't insert or replace User into local storage database: ");

    QString columns = "id, username, email, name, timezone, privilege, creationTimestamp, "
                      "modificationTimestamp, isDirty, isLocal, isActive";
    QString values;

#define CHECK_AND_SET_USER_VALUE(checker, property, error, use_quotation, ...) \
    if (!user.checker()) { \
        errorDescription += QObject::tr(error); \
        return false; \
    } \
    else { \
        if (!values.isEmpty()) { values.append(", "); } \
        if (use_quotation) { \
            values.append("\'" + __VA_ARGS__(user.property()) + "\'"); \
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

    values.append(", \"" + QString::number((user.isDirty() ? 1 : 0)) + "\"");
    values.append(", \"" + QString::number((user.isLocal() ? 1 : 0)) + "\"");

    // Process deletionTimestamp properties specifically
    if (user.hasDeletionTimestamp()) {
        columns.append(", deletionTimestamp");
        values.append(", \"" + QString::number(user.deletionTimestamp()) + "\"");
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

bool LocalStorageManager::InsertOrReplaceBusinesUserInfo(const UserID id, const qevercloud::BusinessUserInfo & info,
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
        columns.append("email");
        valuesString.append(":email");
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
        query.bindValue(":email", info.email.ref());
    }

    res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("can't set user's business info into \"BusinessUserInfo\" table in SQL database");

    return true;
}

bool LocalStorageManager::InsertOrReplacePremiumInfo(const UserID id, const qevercloud::PremiumInfo &info, QString & errorDescription)
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

bool LocalStorageManager::InsertOrReplaceAccounting(const UserID id, const qevercloud::Accounting & accounting,
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
    CHECK_AND_ADD_COLUMN_AND_VALUE(businessId);
    CHECK_AND_ADD_COLUMN_AND_VALUE(businessName);
    CHECK_AND_ADD_COLUMN_AND_VALUE(businessRole);
    CHECK_AND_ADD_COLUMN_AND_VALUE(unitDiscount);
    CHECK_AND_ADD_COLUMN_AND_VALUE(nextChargeDate);

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
    CHECK_AND_BIND_VALUE(businessId);
    CHECK_AND_BIND_VALUE(businessName);
    CHECK_AND_BIND_VALUE(businessRole);
    CHECK_AND_BIND_VALUE(unitDiscount);
    CHECK_AND_BIND_VALUE(nextChargeDate);

#undef CHECK_AND_BIND_VALUE

    res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("can't set user's acconting into \"Accounting\" table in SQL database");

    return true;
}

bool LocalStorageManager::InsertOrReplaceUserAttributes(const UserID id, const qevercloud::UserAttributes & attributes,
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

bool LocalStorageManager::InsertOrReplaceNotebook(const Notebook & notebook,
                                                  QString & errorDescription)
{
    // NOTE: this method expects to be called after notebook is already checked
    // for sanity of its parameters!

    errorDescription = QObject::tr("Can't insert or replace notebook into local storage database: ");

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
        values += "\"" + __VA_ARGS__ (notebook.name()) + "\""; \
    }

    APPEND_COLUMN_VALUE(hasGuid, guid);
    APPEND_COLUMN_VALUE(hasUpdateSequenceNumber, updateSequenceNumber, QString::number);
    APPEND_COLUMN_VALUE(hasName, name);
    APPEND_COLUMN_VALUE(hasCreationTimestamp, creationTimestamp, QString::number);
    APPEND_COLUMN_VALUE(hasModificationTimestamp, modificationTimestamp, QString::number);
    APPEND_COLUMN_VALUE(hasPublished, isPublished, QString::number);
    APPEND_COLUMN_VALUE(hasStack, stack);

#undef APPEND_COLUMN_VALUE

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

    QString localGuid = notebook.localGuid();
    QString isDirty = QString::number(notebook.isDirty() ? 1 : 0);
    QString isLocal = QString::number(notebook.isLocal() ? 1 : 0);
    QString isDefault = QString::number(notebook.isDefaultNotebook() ? 1 : 0);
    QString isLastUsed = QString::number(notebook.isLastUsed() ? 1 : 0);

    QString queryString = QString("INSERT OR REPLACE INTO Notebooks (%1, localGuid, isDirty, isLocal, isDefault, "
                                  "isLastUsed) VALUES(%2, \"%3\", %4, %5, %6, %7)")
                                  .arg(columns).arg(values).arg(localGuid).arg(isDirty)
                                  .arg(isLocal).arg(isDefault).arg(isLastUsed);

    QSqlQuery query(m_sqlDatabase);
    bool res = query.exec(queryString);
    DATABASE_CHECK_AND_SET_ERROR("can't insert or replace notebook into \"Notebooks\" table in SQL database");

    return SetNotebookAdditionalAttributes(notebook, errorDescription);
}

bool LocalStorageManager::InsertOrReplaceLinkedNotebook(const LinkedNotebook & linkedNotebook,
                                                        QString & errorDescription)
{
    // NOTE: this method expects to be called after the linked notebook
    // is already checked for sanity ot its parameters

    errorDescription += QObject::tr("can't insert or replace linked notebook into "
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

bool LocalStorageManager::InsertOrReplaceNote(const Note & note, const Notebook & notebook, QString & errorDescription)
{
    // NOTE: this method expects to be called after the note is already checked
    // for sanity of its parameters!

    errorDescription += QObject::tr("can't insert or replace note into local storage database: ");

    // ========= Creating and executing "insert or replace" query for Notes table
    QString columns = "localGuid, updateSequenceNumber, title, isDirty, "
                      "isLocal, content, creationTimestamp, modificationTimestamp, "
                      "notebookLocalGuid, hasAttributes";
    bool noteHasGuid = note.hasGuid();
    if (noteHasGuid) {
        columns.append(", guid");
    }

    bool notebookHasGuid = notebook.hasGuid();
    if (notebookHasGuid) {
        columns.append(", notebookGuid");
    }

    QString valuesString = ":localGuid, :updateSequenceNumber, :title, :isDirty, "
                           ":isLocal, :content, :creationTimestamp, :modificationTimestamp, "
                           ":notebookLocalGuid, :hasAttributes";
    if (noteHasGuid) {
        valuesString.append(", :guid");
    }

    if (notebookHasGuid) {
        valuesString.append(", :notebookGuid");
    }

    QString queryString = QString("INSERT OR REPLACE INTO Notes (%1) VALUES(%2)").arg(columns).arg(valuesString);
    QSqlQuery query(m_sqlDatabase);
    bool res = query.prepare(queryString);
    DATABASE_CHECK_AND_SET_ERROR("can't find local notebook's guid for its remote guid: "
                                 "can't prepare SQL query for \"Notes\" table");

    const QString localGuid = note.localGuid();
    const QString content = note.content();
    const QString title = note.title();
    const QString notebookLocalGuid = notebook.localGuid();

    query.bindValue(":localGuid", localGuid);
    query.bindValue(":updateSequenceNumber", note.updateSequenceNumber());
    query.bindValue(":title", title);
    query.bindValue(":isDirty", (note.isDirty() ? 1 : 0));
    query.bindValue(":isLocal", (note.isLocal() ? 1 : 0));
    query.bindValue(":content", content);
    query.bindValue(":creationTimestamp", note.creationTimestamp());
    query.bindValue(":modificationTimestamp", note.modificationTimestamp());
    query.bindValue(":notebookLocalGuid", notebookLocalGuid);
    query.bindValue(":hasAttributes", (note.hasNoteAttributes() ? 1 : 0));

    if (noteHasGuid) {
        query.bindValue(":guid", note.guid());
    }

    if (notebookHasGuid) {
        query.bindValue(":notebookGuid", notebook.guid());
    }

    res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("can't insert or replace note into \"Notes\" table in SQL database");

    columns = "localGuid, title, content, notebookLocalGuid";
    if (noteHasGuid) {
        columns.append(", guid");
    }

    if (notebookHasGuid) {
        columns.append(", notebookGuid");
    }

    valuesString = ":localGuid, :title, :content, :notebookLocalGuid";
    if (noteHasGuid) {
        valuesString.append(", :guid");
    }

    if (notebookHasGuid) {
        valuesString.append(", :notebookGuid");
    }

    queryString = QString("INSERT OR REPLACE INTO NoteText (%1) VALUES(%2)").arg(columns).arg(valuesString);
    res = query.prepare(queryString);
    DATABASE_CHECK_AND_SET_ERROR("can't prepare SQL query to insert or replace note into \"NoteText\" table");

    query.bindValue(":localGuid", localGuid);
    query.bindValue(":title", title);
    query.bindValue(":content", content);
    query.bindValue(":notebookLocalGuid", notebookLocalGuid);
    query.bindValue(":hasAttributes", (note.hasNoteAttributes() ? 1 : 0));

    if (noteHasGuid) {
        query.bindValue(":guid", note.guid());
    }

    if (notebookHasGuid) {
        query.bindValue(":notebookGuid", notebook.guid());
    }

    res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("can't insert or replace note into \"NoteText\" table in SQL database");

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
                errorDescription += QObject::tr("failed to find one of note tags by guid: ");
                errorDescription += error;
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
        size_t numResources = resources.size();
        for(size_t i = 0; i < numResources; ++i)
        {
            const ResourceAdapter & resource = resources[i];

            QString error;
            res = resource.checkParameters(error);
            if (!res) {
                errorDescription += QObject::tr("found invalid resource linked with note: ");
                errorDescription += error;
                return false;
            }

            error.clear();
            res = InsertOrReplaceResource(resource, note, error);
            if (!res) {
                errorDescription += QObject::tr("can't add or update one of note's "
                                                "attached resources: ");
                errorDescription += error;
                return false;
            }
        }
    }

    if (!note.hasNoteAttributes()) {
        return true;
    }

    return SetNoteAttributes(note, errorDescription);
}

bool LocalStorageManager::InsertOrReplaceTag(const Tag & tag, QString & errorDescription)
{
    // NOTE: this method expects to be called after tag is already checked
    // for sanity of its parameters!

    errorDescription = QObject::tr("Can't insert or replace tag into local storage database: ");

    QString localGuid = tag.localGuid();
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

bool LocalStorageManager::InsertOrReplaceResource(const IResource & resource,
                                                  const Note & note, QString & errorDescription)
{
    // NOTE: this method expects to be called after resource is already checked
    // for sanity of its parameters!

    errorDescription = QObject::tr("Can't insert or replace resource into local storage database: ");

    QString columns, values;

#define CHECK_AND_INSERT_RESOURCE_PROPERTY(property, checker, ...) \
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
        values.append("\"" + __VA_ARGS__ (resource.property()) + "\""); \
    }

    CHECK_AND_INSERT_RESOURCE_PROPERTY(guid, hasGuid);
    CHECK_AND_INSERT_RESOURCE_PROPERTY(noteGuid, hasNoteGuid);

    bool hasData = resource.hasData();
    bool hasAnyData = (hasData || resource.hasAlternateData());
    if (hasAnyData)
    {
        const auto & dataBody = (hasData ? resource.dataBody() : resource.alternateDataBody());
        CHECK_AND_INSERT_RESOURCE_PROPERTY(dataBody, hasDataBody);

        const auto & dataSize = (hasData ? resource.dataSize() : resource.alternateDataSize());
        CHECK_AND_INSERT_RESOURCE_PROPERTY(dataSize, hasDataSize, QString::number);

        const auto & dataHash = (hasData ? resource.dataHash() : resource.alternateDataHash());
        CHECK_AND_INSERT_RESOURCE_PROPERTY(dataHash, hasDataHash);
    }

    CHECK_AND_INSERT_RESOURCE_PROPERTY(mime, hasMime);
    CHECK_AND_INSERT_RESOURCE_PROPERTY(width, hasWidth, QString::number);
    CHECK_AND_INSERT_RESOURCE_PROPERTY(height, hasHeight, QString::number);

    if (resource.hasRecognitionData()) {
        CHECK_AND_INSERT_RESOURCE_PROPERTY(recognitionDataBody, hasRecognitionDataBody);
        CHECK_AND_INSERT_RESOURCE_PROPERTY(recognitionDataSize, hasRecognitionDataSize, QString::number);
        CHECK_AND_INSERT_RESOURCE_PROPERTY(recognitionDataHash, hasRecognitionDataHash);
    }

    CHECK_AND_INSERT_RESOURCE_PROPERTY(updateSequenceNumber, hasUpdateSequenceNumber, QString::number);

#undef CHECK_AND_INSERT_RESOURCE_PROPERTY

    if (!columns.isEmpty()) {
        columns.append(", ");
    }
    columns.append("isDirty");

    if (!values.isEmpty()) {
        values.append(", ");
    }
    values.append(QString::number(resource.isDirty() ? 1 : 0));

    int indexInNote = resource.indexInNote();
    if (indexInNote >= 0) {
        columns.append(", indexInNote");
        values.append(", " + QString::number(indexInNote));
    }

    columns.append(", localGuid");
    QString resourceLocalGuid = resource.localGuid();
    values.append(", \"" + resourceLocalGuid + "\"");

    QSqlQuery query(m_sqlDatabase);
    QString queryString = QString("INSERT OR REPLACE INTO Resources (%1) VALUES(%2)").arg(columns).arg(values);
    bool res = query.exec(queryString);
    DATABASE_CHECK_AND_SET_ERROR("can't insert or replace data into \"Resources\" table in SQL database");

    QString noteLocalGuid = note.localGuid();

    queryString = QString("INSERT OR REPLACE INTO NoteResources (note, resource) VALUES(\"%1\", \"%2\")")
                          .arg(noteLocalGuid).arg(resource.localGuid());
    res = query.exec(queryString);
    DATABASE_CHECK_AND_SET_ERROR("can't insert or replace data into \"NoteResources\" table in SQL database");

    if (resource.hasResourceAttributes())
    {
        const qevercloud::ResourceAttributes & attributes = resource.resourceAttributes();
        res = InsertOrReplaceResourceAttributes(resource.localGuid(), attributes, errorDescription);
        if (!res) {
            return false;
        }
    }

    return true;
}

bool LocalStorageManager::InsertOrReplaceResourceAttributes(const QString & localGuid,
                                                            const qevercloud::ResourceAttributes & attributes,
                                                            QString & errorDescription)
{
    QString columns = "localGuid";
    QString valuesString = ":localGuid";

#define CHECK_AND_ADD_COLUMN_AND_VALUE(name) \
    bool has##name = attributes.name.isSet(); \
    if (has##name) { \
        columns.append(", " #name); \
        valuesString.append(", :" #name); \
    }

    CHECK_AND_ADD_COLUMN_AND_VALUE(sourceURL);
    CHECK_AND_ADD_COLUMN_AND_VALUE(timestamp);
    CHECK_AND_ADD_COLUMN_AND_VALUE(latitude);
    CHECK_AND_ADD_COLUMN_AND_VALUE(longitude);
    CHECK_AND_ADD_COLUMN_AND_VALUE(altitude);
    CHECK_AND_ADD_COLUMN_AND_VALUE(cameraMake);
    CHECK_AND_ADD_COLUMN_AND_VALUE(cameraModel);
    CHECK_AND_ADD_COLUMN_AND_VALUE(clientWillIndex);
    CHECK_AND_ADD_COLUMN_AND_VALUE(recoType);
    CHECK_AND_ADD_COLUMN_AND_VALUE(fileName);
    CHECK_AND_ADD_COLUMN_AND_VALUE(attachment);

#undef CHECK_AND_ADD_COLUMN_AND_VALUE

    QString queryString = QString("INSERT OR REPLACE INTO ResourceAttributes(%1) VALUES(%2)")
                                  .arg(columns).arg(valuesString);
    QSqlQuery query(m_sqlDatabase);
    bool res = query.prepare(queryString);
    DATABASE_CHECK_AND_SET_ERROR("can't prepare SQL query");

#define CHECK_AND_BIND_VALUE(name) \
    if (has##name) { \
        query.bindValue(":" #name, attributes.name.ref()); \
    }

    CHECK_AND_BIND_VALUE(sourceURL);
    CHECK_AND_BIND_VALUE(timestamp);
    CHECK_AND_BIND_VALUE(latitude);
    CHECK_AND_BIND_VALUE(longitude);
    CHECK_AND_BIND_VALUE(altitude);
    CHECK_AND_BIND_VALUE(cameraMake);
    CHECK_AND_BIND_VALUE(cameraModel);
    CHECK_AND_BIND_VALUE(recoType);
    CHECK_AND_BIND_VALUE(fileName);

#undef CHECK_AND_BIND_VALUE

    if (hasattachment) {
        query.bindValue(":attachment", (attributes.attachment.ref() ? 1 : 0));
    }

    if (hasclientWillIndex) {
        query.bindValue(":clientWillIndex", (attributes.clientWillIndex.ref() ? 1 : 0));
    }

    query.bindValue(":localGuid", localGuid);

    res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("can't insert or replace data into \"ResourceAttributes\" table in SQL database");

    // Special treatment for ResourceAttributes.applicationData: keysOnly + fullMap

    if (attributes.applicationData.isSet())
    {
        if (attributes.applicationData->keysOnly.isSet())
        {
            const QSet<QString> & keysOnly = attributes.applicationData->keysOnly.ref();
            foreach(const QString & key, keysOnly) {
                queryString = QString("INSERT OR REPLACE INTO ResourceAttributesApplicationDataKeysOnly(localGuid, key) VALUES('%1', '%2')")
                                        .arg(localGuid).arg(key);
                res = query.exec();
                DATABASE_CHECK_AND_SET_ERROR("can't insert or replace data into \"ResourceAttributesApplicationDataKeysOnly\" table in SQL database");
            }
        }

        if (attributes.applicationData->fullMap.isSet())
        {
            const QMap<QString, QString> & fullMap = attributes.applicationData->fullMap.ref();
            foreach(const QString & key, fullMap.keys()) {
                queryString = QString("INSERT OR REPLACE INTO ResourceAttributesApplicationDataFullMap(localGuid, key, value) VALUES('%1', '%2', '%3')")
                                      .arg(localGuid).arg(key).arg(fullMap.value(key));
                res = query.exec();
                DATABASE_CHECK_AND_SET_ERROR("can't insert or replace data into \"ResourceAttributesApplicationDataFullMap\" table in SQL database");

            }
        }
    }

    return true;
}

bool LocalStorageManager::InsertOrReplaceSavedSearch(const SavedSearch & search,
                                                     QString & errorDescription)
{
    // NOTE: this method expects to be called after the search is already checked
    // for sanity of its parameters!

    errorDescription = QObject::tr("Can't insert or replace saved search into local storage database: ");

    QString columns = "localGuid, guid, name, nameUpper, query, format, updateSequenceNumber, "
                      "includeAccount, includePersonalLinkedNotebooks, includeBusinessLinkedNotebooks";

    QString valuesNames = ":localGuid, :guid, :name, :nameUpper, :query, :format, :updateSequenceNumber, "
                          ":includeAccount, :includePersonalLinkedNotebooks, :includeBusinessLinkedNotebooks";

    QString queryString = QString("INSERT OR REPLACE INTO SavedSearches (%1) VALUES(%2)").arg(columns).arg(valuesNames);

    QSqlQuery query(m_sqlDatabase);
    bool res = query.prepare(queryString);
    if (!res) {
        errorDescription += QObject::tr("failed to prepare SQL query: ");
        QNDEBUG(errorDescription << query.lastError());
        errorDescription += query.lastError().text();
        return false;
    }

    query.bindValue(":localGuid", search.localGuid());
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

bool LocalStorageManager::FillResourceAttributesFromSqlRecord(const QSqlRecord & rec, qevercloud::ResourceAttributes & attributes) const
{
    bool hasSomething = false;

#define CHECK_AND_SET_RESOURCE_ATTRIBUTE(property, type, localType) \
    if (rec.contains(#property)) { \
        QVariant value = rec.value(#property); \
        if (!value.isNull()) { \
            attributes.property = static_cast<localType>(qvariant_cast<type>(value)); \
            hasSomething = true; \
        } \
    }

    CHECK_AND_SET_RESOURCE_ATTRIBUTE(sourceURL, QString, QString);
    CHECK_AND_SET_RESOURCE_ATTRIBUTE(timestamp, int, qevercloud::Timestamp);
    CHECK_AND_SET_RESOURCE_ATTRIBUTE(latitude, double, double);
    CHECK_AND_SET_RESOURCE_ATTRIBUTE(longitude, double, double);
    CHECK_AND_SET_RESOURCE_ATTRIBUTE(altitude, double, double);
    CHECK_AND_SET_RESOURCE_ATTRIBUTE(cameraMake, QString, QString);
    CHECK_AND_SET_RESOURCE_ATTRIBUTE(cameraModel, QString, QString);
    CHECK_AND_SET_RESOURCE_ATTRIBUTE(clientWillIndex, int, bool);
    CHECK_AND_SET_RESOURCE_ATTRIBUTE(recoType, QString, QString);
    CHECK_AND_SET_RESOURCE_ATTRIBUTE(fileName, QString, QString);
    CHECK_AND_SET_RESOURCE_ATTRIBUTE(attachment, int, bool);

#undef CHECK_AND_SET_RESOURCE_ATTRIBUTE

    return hasSomething;
}

bool LocalStorageManager::FillResourceAttributesApplicationDataKeysOnlyFromSqlRecord(const QSqlRecord & rec,
                                                                                     qevercloud::ResourceAttributes & attributes) const
{
    bool hasSomething = false;

    if (rec.contains("key")) {
        QVariant value = rec.value("key");
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

bool LocalStorageManager::FillResourceAttributesApplicationDataFullMapFromSqlRecord(const QSqlRecord & rec,
                                                                                    qevercloud::ResourceAttributes & attributes) const
{
    bool hasSomething = false;

    if (rec.contains("key") && rec.contains("value")) {
        QVariant key = rec.value("key");
        QVariant value = rec.value("value");
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

void LocalStorageManager::FillNoteAttributesFromSqlRecord(const QSqlRecord & rec, qevercloud::NoteAttributes & attributes) const
{
#define CHECK_AND_SET_NOTE_ATTRIBUTE(property, type, localType) \
    if (rec.contains(#property)) { \
        QVariant value = rec.value(#property); \
        if (!value.isNull()) { \
            attributes.property = static_cast<localType>(qvariant_cast<type>(value)); \
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
}

void LocalStorageManager::FillNoteAttributesApplicationDataKeysOnlyFromSqlRecord(const QSqlRecord & rec,
                                                                                 qevercloud::NoteAttributes & attributes) const
{
    if (rec.contains("key")) {
        QVariant value = rec.value("key");
        if (!value.isNull()) {
            if (!attributes.applicationData.isSet()) {
                attributes.applicationData = qevercloud::LazyMap();
            }
            if (!attributes.applicationData->keysOnly.isSet()) {
                attributes.applicationData->keysOnly = QSet<QString>();
            }
            attributes.applicationData->keysOnly.ref().insert(value.toString());
        }
    }
}

void LocalStorageManager::FillNoteAttributesApplicationDataFullMapFromSqlRecord(const QSqlRecord & rec,
                                                                                qevercloud::NoteAttributes & attributes) const
{
    if (rec.contains("key") && rec.contains("value")) {
        QVariant key = rec.value("key");
        QVariant value = rec.value("value");
        if (!key.isNull() && !value.isNull()) {
            if (!attributes.applicationData.isSet()) {
                attributes.applicationData = qevercloud::LazyMap();
            }
            if (!attributes.applicationData->fullMap.isSet()) {
                attributes.applicationData->fullMap = QMap<QString, QString>();
            }
            attributes.applicationData->fullMap.ref().insert(key.toString(), value.toString());
        }
    }
}

void LocalStorageManager::FillNoteAttributesClassificationsFromSqlRecord(const QSqlRecord & rec,
                                                                         qevercloud::NoteAttributes & attributes) const
{
    if (rec.contains("key") && rec.contains("value")) {
        QVariant key = rec.value("key");
        QVariant value = rec.value("value");
        if (!key.isNull() && !value.isNull()) {
            if (!attributes.classifications.isSet()) {
                attributes.classifications = QMap<QString, QString>();
            }
            attributes.classifications.ref().insert(key.toString(), value.toString());
        }
    }
}

void LocalStorageManager::FillUserAttributesFromSqlRecord(const QSqlRecord & rec, qevercloud::UserAttributes & attributes) const
{
    if (rec.contains("promotion")) {
        QVariant value = rec.value("promotion");
        if (!value.isNull()) {
            if (!attributes.viewedPromotions.isSet()) {
                attributes.viewedPromotions = QStringList();
            }
            attributes.viewedPromotions.ref() << value.toString();
        }

        return;  // NOTE: the presence of "promotion" field means the method is called after
                 // the most part of UserAttributes is already processed so we can simply return here
    }

    if (rec.contains("address")) {
        QVariant value = rec.value("address");
        if (!value.isNull()) {
            if (!attributes.recentMailedAddresses.isSet()) {
                attributes.recentMailedAddresses = QStringList();
            }
            attributes.recentMailedAddresses.ref() << value.toString();
        }

        return;   // NOTE: the presence of "address" field means the method is called after
                  // the most part of UserAttributes is already processed so we can simply return here
    }

#define CHECK_AND_SET_USER_ATTRIBUTES_PROPERTY(property, type, localType) \
    if (rec.contains(#property)) { \
        QVariant value = rec.value(#property); \
        if (!value.isNull()) { \
            attributes.property = static_cast<localType>(qvariant_cast<type>(value)); \
        } \
    }

    CHECK_AND_SET_USER_ATTRIBUTES_PROPERTY(defaultLocationName, QString, QString);
    CHECK_AND_SET_USER_ATTRIBUTES_PROPERTY(defaultLatitude, double, double);
    CHECK_AND_SET_USER_ATTRIBUTES_PROPERTY(defaultLongitude, double, double);
    CHECK_AND_SET_USER_ATTRIBUTES_PROPERTY(preactivation, int, bool);
    CHECK_AND_SET_USER_ATTRIBUTES_PROPERTY(incomingEmailAddress, QString, QString);
    CHECK_AND_SET_USER_ATTRIBUTES_PROPERTY(comments, QString, QString);
    CHECK_AND_SET_USER_ATTRIBUTES_PROPERTY(dateAgreedToTermsOfService, int, qevercloud::Timestamp);
    CHECK_AND_SET_USER_ATTRIBUTES_PROPERTY(maxReferrals, int, qint32);
    CHECK_AND_SET_USER_ATTRIBUTES_PROPERTY(referralCount, int, qint32);
    CHECK_AND_SET_USER_ATTRIBUTES_PROPERTY(refererCode, QString, QString);
    CHECK_AND_SET_USER_ATTRIBUTES_PROPERTY(sentEmailDate, int, qevercloud::Timestamp);
    CHECK_AND_SET_USER_ATTRIBUTES_PROPERTY(sentEmailCount, int, qint32);
    CHECK_AND_SET_USER_ATTRIBUTES_PROPERTY(dailyEmailLimit, int, qint32);
    CHECK_AND_SET_USER_ATTRIBUTES_PROPERTY(emailOptOutDate, int, qevercloud::Timestamp);
    CHECK_AND_SET_USER_ATTRIBUTES_PROPERTY(partnerEmailOptInDate, int, qevercloud::Timestamp);
    CHECK_AND_SET_USER_ATTRIBUTES_PROPERTY(preferredLanguage, QString, QString);
    CHECK_AND_SET_USER_ATTRIBUTES_PROPERTY(preferredCountry, QString, QString);
    CHECK_AND_SET_USER_ATTRIBUTES_PROPERTY(clipFullPage, int, bool);
    CHECK_AND_SET_USER_ATTRIBUTES_PROPERTY(twitterUserName, QString, QString);
    CHECK_AND_SET_USER_ATTRIBUTES_PROPERTY(twitterId, QString, QString);
    CHECK_AND_SET_USER_ATTRIBUTES_PROPERTY(groupName, QString, QString);
    CHECK_AND_SET_USER_ATTRIBUTES_PROPERTY(recognitionLanguage, QString, QString);
    CHECK_AND_SET_USER_ATTRIBUTES_PROPERTY(referralProof, QString, QString);
    CHECK_AND_SET_USER_ATTRIBUTES_PROPERTY(educationalDiscount, int, bool);
    CHECK_AND_SET_USER_ATTRIBUTES_PROPERTY(businessAddress, QString, QString);
    CHECK_AND_SET_USER_ATTRIBUTES_PROPERTY(hideSponsorBilling, int, bool);
    CHECK_AND_SET_USER_ATTRIBUTES_PROPERTY(taxExempt, int, bool);
    CHECK_AND_SET_USER_ATTRIBUTES_PROPERTY(useEmailAutoFiling, int, bool);
    CHECK_AND_SET_USER_ATTRIBUTES_PROPERTY(reminderEmailConfig, int, qevercloud::ReminderEmailConfig::type);

#undef CHECK_AND_SET_USER_ATTRIBUTES_PROPERTY
}

void LocalStorageManager::FillAccountingFromSqlRecord(const QSqlRecord & rec, qevercloud::Accounting & accounting) const
{
#define CHECK_AND_SET_ACCOUNTING_PROPERTY(property, type, localType) \
    if (rec.contains(#property)) { \
        QVariant value = rec.value(#property); \
        if (!value.isNull()) { \
            accounting.property = static_cast<localType>(qvariant_cast<type>(value)); \
        } \
    }

    CHECK_AND_SET_ACCOUNTING_PROPERTY(uploadLimit, int, qint64);
    CHECK_AND_SET_ACCOUNTING_PROPERTY(uploadLimitEnd, int, qevercloud::Timestamp);
    CHECK_AND_SET_ACCOUNTING_PROPERTY(uploadLimitNextMonth, int, qint64);
    CHECK_AND_SET_ACCOUNTING_PROPERTY(premiumServiceStatus, int, qevercloud::PremiumOrderStatus::type);
    CHECK_AND_SET_ACCOUNTING_PROPERTY(premiumOrderNumber, QString, QString);
    CHECK_AND_SET_ACCOUNTING_PROPERTY(premiumCommerceService, QString, QString);
    CHECK_AND_SET_ACCOUNTING_PROPERTY(premiumServiceStart, int, qevercloud::Timestamp);
    CHECK_AND_SET_ACCOUNTING_PROPERTY(premiumServiceSKU, QString, QString);
    CHECK_AND_SET_ACCOUNTING_PROPERTY(lastSuccessfulCharge, int, qevercloud::Timestamp);
    CHECK_AND_SET_ACCOUNTING_PROPERTY(lastFailedCharge, int, qevercloud::Timestamp);
    CHECK_AND_SET_ACCOUNTING_PROPERTY(lastFailedChargeReason, QString, QString);
    CHECK_AND_SET_ACCOUNTING_PROPERTY(nextPaymentDue, int, qevercloud::Timestamp);
    CHECK_AND_SET_ACCOUNTING_PROPERTY(premiumLockUntil, int, qevercloud::Timestamp);
    CHECK_AND_SET_ACCOUNTING_PROPERTY(updated, int, qevercloud::Timestamp);
    CHECK_AND_SET_ACCOUNTING_PROPERTY(premiumSubscriptionNumber, QString, QString);
    CHECK_AND_SET_ACCOUNTING_PROPERTY(lastRequestedCharge, int, qevercloud::Timestamp);
    CHECK_AND_SET_ACCOUNTING_PROPERTY(currency, QString, QString);
    CHECK_AND_SET_ACCOUNTING_PROPERTY(unitPrice, int, qint32);
    CHECK_AND_SET_ACCOUNTING_PROPERTY(businessId, int, qint32);
    CHECK_AND_SET_ACCOUNTING_PROPERTY(businessName, QString, QString);
    CHECK_AND_SET_ACCOUNTING_PROPERTY(businessRole, int, qevercloud::BusinessUserRole::type);
    CHECK_AND_SET_ACCOUNTING_PROPERTY(unitDiscount, int, qint32);
    CHECK_AND_SET_ACCOUNTING_PROPERTY(nextChargeDate, int, qevercloud::Timestamp);

#undef CHECK_AND_SET_ACCOUNTING_PROPERTY
}

bool LocalStorageManager::FillPremiumInfoFromSqlRecord(const QSqlRecord & rec, qevercloud::PremiumInfo & info,
                                                       QString & errorDescription) const
{
#define CHECK_AND_SET_PREMIUM_INFO_PROPERTY(property, type, localType) \
    { \
        bool isNull = true; \
        if (rec.contains(#property)) { \
            QVariant value = rec.value(#property); \
            isNull = value.isNull(); \
            if (!isNull) { \
                info.property = static_cast<localType>(qvariant_cast<type>(value)); \
            } \
        } \
        \
        if (isNull && isRequired) { \
            errorDescription += QObject::tr("Internal error: no " #property " field " \
                                            "in the result of SQL query"); \
            return false; \
        } \
    }

    bool isRequired = true;
    CHECK_AND_SET_PREMIUM_INFO_PROPERTY(currentTime, int, qevercloud::Timestamp);
    CHECK_AND_SET_PREMIUM_INFO_PROPERTY(premium, int, bool);
    CHECK_AND_SET_PREMIUM_INFO_PROPERTY(premiumRecurring, int, bool);
    CHECK_AND_SET_PREMIUM_INFO_PROPERTY(premiumExtendable, int, bool);
    CHECK_AND_SET_PREMIUM_INFO_PROPERTY(premiumPending, int, bool);
    CHECK_AND_SET_PREMIUM_INFO_PROPERTY(premiumCancellationPending, int, bool);
    CHECK_AND_SET_PREMIUM_INFO_PROPERTY(canPurchaseUploadAllowance, int, bool);

    isRequired = false;
    CHECK_AND_SET_PREMIUM_INFO_PROPERTY(premiumExpirationDate, int, qevercloud::Timestamp);
    CHECK_AND_SET_PREMIUM_INFO_PROPERTY(sponsoredGroupName, QString, QString);
    CHECK_AND_SET_PREMIUM_INFO_PROPERTY(sponsoredGroupRole, int, qevercloud::SponsoredGroupRole::type);
    CHECK_AND_SET_PREMIUM_INFO_PROPERTY(premiumUpgradable, int, bool);

#undef CHECK_AND_SET_PREMIUM_INFO_PROPERTY

    return true;
}

bool LocalStorageManager::FillBusinessUserInfoFromSqlRecord(const QSqlRecord & rec, qevercloud::BusinessUserInfo & info,
                                                            QString & errorDescription) const
{
    QVariant businessIdValue = rec.value("businessId");
    if (!businessIdValue.isNull())
    {
        bool conversionResult = false;
        int businessId = businessIdValue.toInt(&conversionResult);
        if (!conversionResult) {
            errorDescription = QObject::tr("Internal error: can't convert business id to int");
            QNCRITICAL(errorDescription);
            return false;
        }

        info.businessId = businessId;
    }

    QVariant businessNameValue = rec.value("businessName");
    if (!businessNameValue.isNull()) {
        info.businessName = businessNameValue.toString();
    }

    QVariant roleValue = rec.value("role");
    if (!roleValue.isNull())
    {
        bool conversionResult = false;
        int role = roleValue.toInt(&conversionResult);
        if (!conversionResult) {
            errorDescription = QObject::tr("Internal error: can't convert role to int");
            QNCRITICAL(errorDescription);
            return false;
        }

        if ((role < 0) || (role > static_cast<int>(qevercloud::BusinessUserRole::NORMAL))) {
            errorDescription = QObject::tr("Internal error: found invalid role for BusinessUserInfo");
            QNCRITICAL(errorDescription);
            return false;
        }

        info.role = static_cast<qevercloud::BusinessUserRole::type>(role);
    }

    QVariant emailValue = rec.value("email");
    if (!emailValue.isNull()) {
        info.email = emailValue.toString();
    }

    return true;
}

bool LocalStorageManager::FillNoteFromSqlRecord(const QSqlRecord & rec, Note & note,
                                                QString & errorDescription, const bool withResourceBinaryData) const
{
#define CHECK_AND_SET_NOTE_PROPERTY(propertyLocalName, setter, type, localType) \
    if (rec.contains(#propertyLocalName)) { \
        note.setter(static_cast<localType>(qvariant_cast<type>(rec.value(#propertyLocalName)))); \
    } \
    else { \
        errorDescription += QObject::tr("Internal error: no " #propertyLocalName " field " \
                                        "in the result of SQL query"); \
        return false; \
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

    if (rec.contains("hasAttributes"))
    {
        int hasAttributes = qvariant_cast<int>(rec.value("hasAttributes"));
        if (hasAttributes == 0) {
            QNDEBUG("Note has no optional attributes");
            return true;
        }
    }
    else {
        errorDescription += QObject::tr("Internal error: no \"hasAttributes\" field " \
                                        "in the result of SQL query");
        return false;
    }

    QString error;
    bool res = FindAndSetNoteAttributesPerNote(note, error);
    if (!res) {
        errorDescription += error;
        return false;
    }

    return true;
}

bool LocalStorageManager::FillNotebookFromSqlRecord(const QSqlRecord & record, Notebook & notebook, 
                                                    QString & errorDescription) const
{
#define CHECK_AND_SET_NOTEBOOK_ATTRIBUTE(attribute, setter, dbType, trueType, isRequired) \
    if (record.contains(#attribute)) { \
        notebook.setter(static_cast<trueType>((qvariant_cast<dbType>(record.value(#attribute))))); \
    } \
    else if (isRequired) { \
        errorDescription += QObject::tr("Internal error: No " #attribute " field " \
                                        "in the result of SQL query"); \
        return false; \
    }

    bool isRequired = true;

    CHECK_AND_SET_NOTEBOOK_ATTRIBUTE(isDirty, setDirty, int, bool, isRequired);
    CHECK_AND_SET_NOTEBOOK_ATTRIBUTE(isLocal, setLocal, int, bool, isRequired);
    CHECK_AND_SET_NOTEBOOK_ATTRIBUTE(isLastUsed, setLastUsed, int, bool, isRequired);
    CHECK_AND_SET_NOTEBOOK_ATTRIBUTE(localGuid, setLocalGuid, QString, QString, isRequired);

    CHECK_AND_SET_NOTEBOOK_ATTRIBUTE(guid, setGuid, QString, QString, isRequired);
    CHECK_AND_SET_NOTEBOOK_ATTRIBUTE(updateSequenceNumber, setUpdateSequenceNumber,
                                     int, qint32, isRequired);
    CHECK_AND_SET_NOTEBOOK_ATTRIBUTE(name, setName, QString, QString, isRequired);
    CHECK_AND_SET_NOTEBOOK_ATTRIBUTE(isDefault, setDefaultNotebook, int, bool, isRequired);
    CHECK_AND_SET_NOTEBOOK_ATTRIBUTE(creationTimestamp, setCreationTimestamp,
                                     int, qint64, isRequired);
    CHECK_AND_SET_NOTEBOOK_ATTRIBUTE(modificationTimestamp, setModificationTimestamp,
                                     int, qint64, isRequired);
    CHECK_AND_SET_NOTEBOOK_ATTRIBUTE(isPublished, setPublished, int, bool, isRequired);
    CHECK_AND_SET_NOTEBOOK_ATTRIBUTE(stack, setStack, QString, QString, isRequired);

    if (notebook.hasPublished() && notebook.isPublished())
    {
        CHECK_AND_SET_NOTEBOOK_ATTRIBUTE(publishingUri, setPublishingUri,
                                         QString, QString, isRequired);
        CHECK_AND_SET_NOTEBOOK_ATTRIBUTE(publishingNoteSortOrder, setPublishingOrder,
                                         int, qint8, isRequired);
        CHECK_AND_SET_NOTEBOOK_ATTRIBUTE(publishingAscendingSort, setPublishingAscending,
                                         int, bool, isRequired);
        CHECK_AND_SET_NOTEBOOK_ATTRIBUTE(publicDescription,
                                         setPublishingPublicDescription,
                                         QString, QString, isRequired);
    }

    CHECK_AND_SET_NOTEBOOK_ATTRIBUTE(businessNotebookDescription, setBusinessNotebookDescription,
                                     QString, QString, isRequired);
    CHECK_AND_SET_NOTEBOOK_ATTRIBUTE(businessNotebookPrivilegeLevel, setBusinessNotebookPrivilegeLevel,
                                     int, qint8, isRequired);
    CHECK_AND_SET_NOTEBOOK_ATTRIBUTE(businessNotebookIsRecommended, setBusinessNotebookRecommended,
                                     int, bool, isRequired);

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
        bool res = FindUser(user, error);
        if (!res) {
            errorDescription += error;
            return false;
        }
    }

    QSqlQuery query(m_sqlDatabase);
    query.prepare("SELECT noReadNotes, noCreateNotes, noUpdateNotes, noExpungeNotes, "
                  "noShareNotes, noEmailNotes, noSendMessageToRecipients, noUpdateNotebook, "
                  "noExpungeNotebook, noSetDefaultNotebook, noSetNotebookStack, noPublishToPublic, "
                  "noPublishToBusinessLibrary, noCreateTags, noUpdateTags, noExpungeTags, "
                  "noSetParentTag, noCreateSharedNotebooks, updateWhichSharedNotebookRestrictions, "
                  "expungeWhichSharedNotebookRestrictions FROM NotebookRestrictions "
                  "WHERE localGuid=?");
    query.addBindValue(notebook.localGuid());

    bool res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("can't find notebook restrictions for specified "
                                 "notebook guid from SQL database");

    if (query.next())
    {
        QSqlRecord record = query.record();   // NOTE: intentionally hide the method parameter in this scope to reuse macro

#define SET_EN_NOTEBOOK_RESTRICTION(notebook_restriction, setter) \
    if (record.contains(#notebook_restriction)) { \
        notebook.setter(!(static_cast<bool>((qvariant_cast<int>(record.value(#notebook_restriction)))))); \
    } \

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

        isRequired = false;

        CHECK_AND_SET_NOTEBOOK_ATTRIBUTE(updateWhichSharedNotebookRestrictions,
                                         setUpdateWhichSharedNotebookRestrictions,
                                         int, qint8, isRequired);

        CHECK_AND_SET_NOTEBOOK_ATTRIBUTE(expungeWhichSharedNotebookRestrictions,
                                         setExpungeWhichSharedNotebookRestrictions,
                                         int, qint8, isRequired);

    }

    QString error;
    QList<qevercloud::SharedNotebook> sharedNotebooks = ListEnSharedNotebooksPerNotebookGuid(notebook.guid(), error);
    if (!error.isEmpty()) {
        errorDescription += error;
        return false;
    }

    notebook.setSharedNotebooks(sharedNotebooks);

    return true;
}

bool LocalStorageManager::FillSharedNotebookFromSqlRecord(const QSqlRecord & rec,
                                                          ISharedNotebook & sharedNotebook,
                                                          QString & errorDescription) const
{
#define CHECK_AND_SET_SHARED_NOTEBOOK_PROPERTY(property, type, localType, setter, isRequired) \
    if (rec.contains(#property)) { \
        sharedNotebook.setter(static_cast<localType>(qvariant_cast<type>(rec.value(#property)))); \
    } \
    else if (isRequired) { \
        errorDescription += QObject::tr("no " #property " field in the result of SQL query"); \
        return false; \
    }

    bool isRequired = true;

    CHECK_AND_SET_SHARED_NOTEBOOK_PROPERTY(shareId, int, qint64, setId, isRequired);
    CHECK_AND_SET_SHARED_NOTEBOOK_PROPERTY(userId, int, qint32, setUserId, isRequired);
    CHECK_AND_SET_SHARED_NOTEBOOK_PROPERTY(email, QString, QString, setEmail, isRequired);
    CHECK_AND_SET_SHARED_NOTEBOOK_PROPERTY(creationTimestamp, int, qint64,
                                           setCreationTimestamp, isRequired);
    CHECK_AND_SET_SHARED_NOTEBOOK_PROPERTY(modificationTimestamp, int, qint64,
                                           setModificationTimestamp, isRequired);
    CHECK_AND_SET_SHARED_NOTEBOOK_PROPERTY(shareKey, QString, QString, setShareKey, isRequired);
    CHECK_AND_SET_SHARED_NOTEBOOK_PROPERTY(username, QString, QString, setUsername, isRequired);
    CHECK_AND_SET_SHARED_NOTEBOOK_PROPERTY(sharedNotebookPrivilegeLevel, int, qint8,
                                           setPrivilegeLevel, isRequired);
    CHECK_AND_SET_SHARED_NOTEBOOK_PROPERTY(allowPreview, int, bool, setAllowPreview,
                                           isRequired);   // NOTE: int to bool cast
    CHECK_AND_SET_SHARED_NOTEBOOK_PROPERTY(recipientReminderNotifyEmail, int, bool,
                                           setReminderNotifyEmail, isRequired);  // NOTE: int to bool cast
    CHECK_AND_SET_SHARED_NOTEBOOK_PROPERTY(recipientReminderNotifyInApp, int, bool,
                                           setReminderNotifyApp, isRequired);  // NOTE: int to bool cast
    CHECK_AND_SET_SHARED_NOTEBOOK_PROPERTY(notebookGuid, QString, QString, setNotebookGuid, isRequired);

#undef CHECK_AND_SET_SHARED_NOTEBOOK_PROPERTY

    int recordIndex = rec.indexOf("indexInNotebook");
    if (recordIndex >= 0)
    {
        QVariant indexInNotebookVariant = rec.value(recordIndex);
        if (!indexInNotebookVariant.isNull())
        {
            bool conversionResult = false;
            int indexInNotebook = indexInNotebookVariant.toInt(&conversionResult);
            if (!conversionResult) {
                errorDescription += QObject::tr("Internal error: can't convert shared notebook's index in notebook to int");
                return false;
            }
            sharedNotebook.setIndexInNotebook(indexInNotebook);
        }
    }

    return true;
}

bool LocalStorageManager::FillLinkedNotebookFromSqlRecord(const QSqlRecord & rec,
                                                          LinkedNotebook & linkedNotebook,
                                                          QString & errorDescription) const
{
#define CHECK_AND_SET_LINKED_NOTEBOOK_PROPERTY(property, type, localType, setter, isRequired) \
    if (rec.contains(#property)) { \
        linkedNotebook.setter(static_cast<localType>(qvariant_cast<type>(rec.value(#property)))); \
    } \
    else if (isRequired) { \
        errorDescription += QObject::tr("no " #property " field in the result of SQL query"); \
        return false; \
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

bool LocalStorageManager::FillSavedSearchFromSqlRecord(const QSqlRecord & rec,
                                                       SavedSearch & search,
                                                       QString & errorDescription) const
{
#define CHECK_AND_SET_SAVED_SEARCH_PROPERTY(property, type, localType, setter, isRequired) \
    if (rec.contains(#property)) { \
        QVariant value = rec.value(#property); \
        if (value.isNull() && isRequired) { \
            errorDescription += QObject::tr("required parameter " #property " is null in the result of SQL query"); \
            return false; \
        } \
        else if (!value.isNull()) { \
            search.setter(static_cast<localType>(qvariant_cast<type>(value))); \
        } \
    } \
    else { \
        errorDescription += QObject::tr("no " #property " field in the result of SQL query"); \
        return false; \
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

bool LocalStorageManager::FillTagFromSqlRecord(const QSqlRecord & rec, Tag & tag,
                                               QString & errorDescription) const
{
#define CHECK_AND_SET_TAG_PROPERTY(property, type, localType, setter, isRequired) \
    if (rec.contains(#property)) { \
        QVariant value = rec.value(#property); \
        if (value.isNull() && isRequired) { \
            errorDescription += QObject::tr("required parameter " #property " is null in the result of SQL query"); \
            return false; \
        } \
        else if (!value.isNull()) { \
            tag.setter(static_cast<localType>(qvariant_cast<type>(value))); \
        } \
    } \
    else { \
        errorDescription += QObject::tr("no " #property " field in the result of SQL query"); \
        return false; \
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

QList<Tag> LocalStorageManager::FillTagsFromSqlQuery(QSqlQuery & query, QString & errorDescription) const
{
    QList<Tag> tags;
    tags.reserve(qMax(query.size(), 0));

    while(query.next())
    {
        tags << Tag();
        Tag & tag = tags.back();

        QString tagLocalGuid = query.value(0).toString();
        if (tagLocalGuid.isEmpty()) {
            errorDescription = QObject::tr("Internal error: no tag's local guid in the result of SQL query");
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

bool LocalStorageManager::FindUserAttributes(const UserID id, qevercloud::UserAttributes & attributes,
                                             QString & errorDescription) const
{
    QNDEBUG("LocalStorageManager::FindUserAttributes: user id = " << id);

    QString errorPrefix = QObject::tr("Can't find UserAttributes: ");

    QString queryString = "SELECT * FROM UserAttributes WHERE id = :id";
    QSqlQuery query(m_sqlDatabase);
    bool res = query.prepare(queryString);

#define CHECK_AND_SET_QUERY_PREPARE_ERROR() \
    if (!res) { \
        errorDescription = errorPrefix + QObject::tr("can't prepare SQL query: "); \
        QNCRITICAL(errorDescription << "last error = " << query.lastError() << ", last query = " << query.lastQuery()); \
        errorDescription += query.lastError().text(); \
        return false; \
    }

    CHECK_AND_SET_QUERY_PREPARE_ERROR();

    query.bindValue(":id", id);

    res = query.exec();

#define CHECK_AND_SET_QUERY_EXEC_ERROR() \
    if (!res) { \
        errorDescription = errorPrefix + QObject::tr("can't execute SQL query: "); \
        QNCRITICAL(errorDescription << "last error = " << query.lastError() << ", last query = " << query.lastQuery()); \
        errorDescription += query.lastError().text(); \
        return false; \
    }

    CHECK_AND_SET_QUERY_EXEC_ERROR();

    if (!query.next()) {
        errorDescription = QObject::tr("No attributes were found for user id = ");
        errorDescription += QString::number(id);
        return false;
    }

    QSqlRecord rec = query.record();
    FillUserAttributesFromSqlRecord(rec, attributes);

    // Special treatment for viewedPromotions field of UserAttributes
    queryString = QString("SELECT * FROM UserAttributesViewedPromotions WHERE id = '%1'").arg(QString::number(id));
    res = query.exec(queryString);
    CHECK_AND_SET_QUERY_EXEC_ERROR();

    while(query.next()) {
        rec = query.record();
        FillUserAttributesFromSqlRecord(rec, attributes);
    }

    // Special treatment for recentMailedAddresses field of UserAttributes
    queryString = QString("SELECT * FROM UserAttributesRecentMailedAddresses WHERE id = '%1'").arg(QString::number(id));
    res = query.exec(queryString);
    CHECK_AND_SET_QUERY_EXEC_ERROR();

    while(query.next()) {
        rec = query.record();
        FillUserAttributesFromSqlRecord(rec, attributes);
    }

    return true;
}

bool LocalStorageManager::FindAccounting(const UserID id, qevercloud::Accounting & accounting,
                                         QString & errorDescription) const
{
    QNDEBUG("LocalStorageManager::FindAccounting: user id = " << id);

    QString errorPrefix = QObject::tr("Can't find Accounting: ");

    QString queryString = "SELECT * FROM Accounting WHERE id = :id";
    QSqlQuery query(m_sqlDatabase);
    bool res = query.prepare(queryString);
    CHECK_AND_SET_QUERY_PREPARE_ERROR();

    query.bindValue(":id", id);

    res = query.exec();
    CHECK_AND_SET_QUERY_EXEC_ERROR();

    if (!query.next()) {
        errorDescription = QObject::tr("No accounting was found for user id = ");
        errorDescription += QString::number(id);
        return false;
    }

    QSqlRecord rec = query.record();

    FillAccountingFromSqlRecord(rec, accounting);

    return true;
}

bool LocalStorageManager::FindPremiumInfo(const UserID id, qevercloud::PremiumInfo & info,
                                          QString & errorDescription) const
{
    QNDEBUG("LocalStorageManager::FindPremiumInfo: user id = " << id);

    QString errorPrefix = QObject::tr("Can't find PremiumInfo: ");

    QString queryString = QString("SELECT currentTime, premium, premiumRecurring, premiumExpirationDate, "
                                  "premiumExtendable, premiumPending, premiumCancellationPending, "
                                  "canPurchaseUploadAllowance, sponsoredGroupName, sponsoredGroupRole, "
                                  "premiumUpgradable FROM PremiumInfo WHERE id = :id");
    QSqlQuery query(m_sqlDatabase);
    bool res = query.prepare(queryString);
    CHECK_AND_SET_QUERY_PREPARE_ERROR();

    query.bindValue(":id", id);
    res = query.exec();
    CHECK_AND_SET_QUERY_EXEC_ERROR();

    if (!query.next()) {
        errorDescription = QObject::tr("No premium info was found for user id = ");
        errorDescription += QString::number(id);
        return false;
    }

    QSqlRecord rec = query.record();

    res = FillPremiumInfoFromSqlRecord(rec, info, errorDescription);
    if (!res) {
        errorDescription.prepend(errorPrefix);
        return false;
    }

    return true;
}

bool LocalStorageManager::FindBusinessUserInfo(const UserID id, qevercloud::BusinessUserInfo & info,
                                               QString & errorDescription) const
{
    QNDEBUG("LocalStorageManager::FindBusinessUserInfo: user id = " << id);

    QString errorPrefix = QObject::tr("Can't find BusinessUserInfo: ");

    QString queryString = QString("SELECT businessId, businessName, role, email "
                                  "FROM BusinessUserInfo WHERE id = :id");
    QSqlQuery query(m_sqlDatabase);
    bool res = query.prepare(queryString);
    CHECK_AND_SET_QUERY_PREPARE_ERROR();

    query.bindValue(":id", id);
    res = query.exec();
    CHECK_AND_SET_QUERY_EXEC_ERROR();

    if (!query.next()) {
        errorDescription = QObject::tr("No business user info was found for user id = ");
        errorDescription += QString::number(id);
        return false;
    }

    QSqlRecord rec = query.record();

    res = FillBusinessUserInfoFromSqlRecord(rec, info, errorDescription);
    if (!res) {
        errorDescription.prepend(errorPrefix);
        return false;
    }

    return true;
}

#undef CHECK_AND_SET_QUERY_PREPARE_ERROR
#undef CHECK_AND_SET_QUERY_EXEC_ERROR

bool LocalStorageManager::FindAndSetTagGuidsPerNote(Note & note, QString & errorDescription) const
{
    errorDescription += QObject::tr("can't find tag guids per note: ");

    if (!CheckGuid(note.guid())) {
        errorDescription += QObject::tr("note's guid is invalid");
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
        if (rec.contains("tag")) {
            tagGuid = rec.value("tag").toString();
        }
        else {
            errorDescription += QObject::tr("Internal error: can't find tag guid in the result of SQL query");
            return false;
        }

        if (!CheckGuid(tagGuid)) {
            errorDescription += QObject::tr("found invalid tag guid for requested note");
            return false;
        }

        QNDEBUG("Found tag guid " << tagGuid << " for note with guid " << note.guid());

        int indexInNote = -1;
        int recordIndex = rec.indexOf("indexInNote");
        if (recordIndex >= 0)
        {
            QVariant indexVariant = rec.value(recordIndex);
            if (!indexVariant.isNull())
            {
                bool conversionResult = false;
                indexInNote = indexVariant.toInt(&conversionResult);
                if (!conversionResult) {
                    QNWARNING("Can't convert QVariant to int: " << rec.value(recordIndex));
                    errorDescription += QObject::tr("Internal error: unable to convert to int "
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

bool LocalStorageManager::FindAndSetResourcesPerNote(Note & note, QString & errorDescription,
                                                     const bool withBinaryData) const
{
    errorDescription += QObject::tr("can't find resources for note: ");

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
        if (rec.contains("note"))
        {
            QString foundNoteGuid = rec.value("note").toString();
            if ((foundNoteGuid == noteGuid) && (rec.contains("resource"))) {
                QString resourceGuid = rec.value("resource").toString();
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

bool LocalStorageManager::FindAndSetNoteAttributesPerNote(Note & note, QString & errorDescription) const
{
    QNDEBUG("LocalStorageManager::FindAndSetNoteAttributesPerNote: note's local guid = " << note.localGuid());

    errorDescription += QObject::tr("can't find note attributes: ");

    const QString guid = note.localGuid();

    // ====== 1) Basic NoteAttributes =======
    QSqlQuery query(m_sqlDatabase);
    query.prepare("SELECT * FROM NoteAttributes WHERE noteLocalGuid=?");
    query.addBindValue(guid);

    bool res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("can't select note's attributes from \"NoteAttributes\" table in SQL database");

    qevercloud::NoteAttributes & attributes = note.noteAttributes();

    if (!query.next()) {
        errorDescription += QObject::tr("Note attributes were not found for provided note's local guid");
        return false;
    }

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

#undef CHECK_AND_SET_RESOURCE_PROPERTY
#undef CHECK_AND_SET_NOTEBOOK_ATTRIBUTE
#undef CHECK_AND_SET_EN_NOTEBOOK_ATTRIBUTE
#undef SET_IS_FREE_ACCOUNT_FLAG
#undef CHECK_AND_SET_EN_RESOURCE_PROPERTY
#undef CHECK_AND_SET_NOTE_PROPERTY
#undef DATABASE_CHECK_AND_SET_ERROR

}
