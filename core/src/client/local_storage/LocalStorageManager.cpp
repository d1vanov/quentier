#include "LocalStorageManager.h"
#include "DatabaseOpeningException.h"
#include "DatabaseSqlErrorException.h"
#include "../Serialization.h"
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
#include <Limits_constants.h>

using namespace evernote::edam;

namespace qute_note {

#define QUTE_NOTE_DATABASE_NAME "qn.storage.sqlite"

LocalStorageManager::LocalStorageManager(const QString & username, const UserID userId) :
    // NOTE: don't initialize these! Otherwise SwitchUser won't work right
    m_currentUsername(),
    m_currentUserId(),
    m_applicationPersistenceStoragePath()
{
    SwitchUser(username, userId);
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
        errorDescription += query.lastError().text(); \
        return false; \
    }

bool LocalStorageManager::AddUser(const IUser & user, QString & errorDescription)
{
    QNDEBUG("LocalStorageManager::AddUser: " << user);

    errorDescription = QObject::tr("Can't add user into local storage database: ");

    int rowId = GetRowId("Users", "id", QVariant(QString::number(user.GetEnUser().id)));
    if (rowId >= 0) {
        errorDescription += QObject::tr("user with the same id already exists");
        return false;
    }

    QString error;
    bool res = InsertOrReplaceUser(user, error);
    if (!res) {
        errorDescription += error;
        return false;
    }

    return true;
}

bool LocalStorageManager::UpdateUser(const IUser & user, QString & errorDescription)
{
    QNDEBUG("LocalStorageManager::UpdateUser: " << user);

    errorDescription = QObject::tr("Can't update user in local storage database: ");

    int rowId = GetRowId("Users", "id", QVariant(QString::number(user.GetEnUser().id)));
    if (rowId < 0) {
        errorDescription += QObject::tr("user id was not found");
        return false;
    }

    QString error;
    bool res = InsertOrReplaceUser(user, error);
    if (!res) {
        errorDescription += error;
        return false;
    }

    return true;
}

bool LocalStorageManager::FindUser(const UserID id, IUser & user, QString & errorDescription) const
{
    QNDEBUG("LocalStorageManager::FindUser: id = " << id);

    errorDescription = QObject::tr("Can't find user in local storage database: ");

    QString idStr = QString::number(id);
    int rowId = GetRowId("Users", "id", QVariant(idStr));
    if (rowId < 0) {
        errorDescription += QObject::tr("user id was not found");
        return false;
    }

    user.Clear();

    QSqlQuery query(m_sqlDatabase);
    query.prepare("SELECT id, username, name, timezone, privilege, creationTimestamp, "
                  "modificationTimestamp, isDirty, isLocal, isDeleted, deletionTimestamp, "
                  "isActive FROM Users WHERE rowid = ?");
    query.addBindValue(idStr);

    bool res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("can't select user from \"Users\" table in SQL database");

    if (!query.next()) {
        errorDescription += QObject::tr("Internal error: SQL query result is empty");
        return false;
    }

    QSqlRecord rec = query.record();

#define CHECK_AND_SET_USER_PROPERTY(property, counterProperty) \
    if (rec.contains("is " #property)) { \
        int property = (qvariant_cast<int>(rec.value(#property)) != 0); \
        if (property > 0) { \
            user.Set##property(); \
        } \
        else { \
            user.Set##counterProperty(); \
        } \
    } \
    else { \
        errorDescription += QObject::tr("Internal error: no " #property \
                                        " field in the result of SQL query "); \
        return false; \
    }

    CHECK_AND_SET_USER_PROPERTY(Dirty, Clean);
    CHECK_AND_SET_USER_PROPERTY(Local, NonLocal);

#undef CHECK_AND_SET_USER_PROPERTY

    evernote::edam::User & enUser = user.GetEnUser();
    enUser.id = id;
    enUser.__isset.id = true;

#define CHECK_AND_SET_EN_USER_PROPERTY(holder, propertyLocalName, propertyEnName, \
                                       type, true_type, is_required, ...) \
    if (rec.contains(#propertyLocalName)) { \
        holder.propertyEnName = static_cast<true_type>((qvariant_cast<type>(rec.value(#propertyLocalName)))__VA_ARGS__); \
        holder.__isset.propertyEnName = true; \
    } \
    else if (is_required) { \
        errorDescription += QObject::tr("Internal error: no " #propertyLocalName \
                                        " field in the result of SQL query"); \
        return false; \
    }

    bool isRequired = true;
    CHECK_AND_SET_EN_USER_PROPERTY(enUser, username, username, QString, std::string,
                                   isRequired, .toStdString());
    CHECK_AND_SET_EN_USER_PROPERTY(enUser, name, name, QString, std::string,
                                   isRequired, .toStdString());
    CHECK_AND_SET_EN_USER_PROPERTY(enUser, timezone, timezone, QString, std::string,
                                   /* isRequired = */ false, .toStdString());
    CHECK_AND_SET_EN_USER_PROPERTY(enUser, privilege, privilege, int,
                                   evernote::edam::PrivilegeLevel::type, isRequired);
    CHECK_AND_SET_EN_USER_PROPERTY(enUser, creationTimestamp, created, int,
                                   evernote::edam::Timestamp, isRequired);
    CHECK_AND_SET_EN_USER_PROPERTY(enUser, modificationTimestamp, updated, int,
                                   evernote::edam::Timestamp, isRequired);
    CHECK_AND_SET_EN_USER_PROPERTY(enUser, deletionTimestamp, deleted, int,
                                   evernote::edam::Timestamp, /* isRequired = */ false);
    CHECK_AND_SET_EN_USER_PROPERTY(enUser, isActive, active, int, bool, isRequired);   // NOTE: int to bool cast

#undef CHECK_AND_SET_EN_USER_PROPERTY

#define CHECK_SQL_DATABASE_STATE_AND_SET_ERROR \
    if (!res) \
    { \
        QString sqlError = m_sqlDatabase.lastError().text(); \
        if (!sqlError.isEmpty()) { \
            errorDescription += QObject::tr("Internal error: "); \
            errorDescription += error; \
            return false; \
        } \
    }

    QString error;
    res = FindUserAttributes(id, enUser.attributes, error);
    enUser.__isset.attributes = res;
    CHECK_SQL_DATABASE_STATE_AND_SET_ERROR

    error.clear();
    res = FindAccounting(id, enUser.accounting, error);
    enUser.__isset.accounting = res;
    CHECK_SQL_DATABASE_STATE_AND_SET_ERROR

    error.clear();
    res = FindPremiumInfo(id, enUser.premiumInfo, error);
    enUser.__isset.premiumInfo = res;
    CHECK_SQL_DATABASE_STATE_AND_SET_ERROR

    error.clear();
    res = FindBusinessUserInfo(id, enUser.businessUserInfo, error);
    enUser.__isset.businessUserInfo = res;
    CHECK_SQL_DATABASE_STATE_AND_SET_ERROR

#undef CHECK_SQL_DATABASE_STATE_AND_SET_ERROR

    return true;
}

#define FIND_OPTIONAL_USER_PROPERTIES(which) \
    bool LocalStorageManager::Find##which(const UserID id, evernote::edam::which & prop, \
                                          QString & errorDescription) const \
    { \
        errorDescription = QObject::tr("Can't find " #which " in local storage database: "); \
        \
        prop = evernote::edam::which(); \
        QString idStr = QString::number(id); \
        int rowId = GetRowId(#which, "id", QVariant(idStr)); \
        if (rowId < 0) { \
            errorDescription += QObject::tr(#which " for specified user id was not found"); \
            return false; \
        } \
        \
        QSqlQuery query(m_sqlDatabase); \
        query.prepare("SELECT data FROM " #which " WHERE rowid = ?"); \
        query.addBindValue(idStr); \
        \
        bool res = query.exec(); \
        DATABASE_CHECK_AND_SET_ERROR("can't select data from \"BusinessUserInfo\" table in SQL database"); \
        \
        if (!query.next()) { \
            errorDescription += QObject::tr("Internal error: SQL query result is empty"); \
            return false; \
        } \
        \
        QByteArray data = qvariant_cast<QByteArray>(query.value(0)); \
        prop = GetDeserialized##which(data); \
        \
        return true; \
    }

FIND_OPTIONAL_USER_PROPERTIES(UserAttributes)
FIND_OPTIONAL_USER_PROPERTIES(Accounting)
FIND_OPTIONAL_USER_PROPERTIES(PremiumInfo)
FIND_OPTIONAL_USER_PROPERTIES(BusinessUserInfo)

#undef FIND_OPTIONAL_USER_PROPERTIES

bool LocalStorageManager::DeleteUser(const IUser & user, QString & errorDescription)
{
    if (user.IsLocal()) {
        return ExpungeUser(user, errorDescription);
    }

    errorDescription = QObject::tr("Can't delete user from local storage database: ");

    const auto & enUser = user.GetEnUser();

    if (!enUser.deleted) {
        errorDescription += QObject::tr("deletion timestamp is not set");
        return false;
    }

    if (!enUser.__isset.id) {
        errorDescription += QObject::tr("user id is not set");
        return false;
    }

    QSqlQuery query(m_sqlDatabase);
    query.prepare("UPDATE Users SET deletionTimestamp = ? WHERE id = ?");
    query.addBindValue(static_cast<qint64>(enUser.deleted));
    query.addBindValue(QString::number(enUser.id));

    bool res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("can't update deletion timestamp in \"Users\" table in SQL database");

    return true;
}

bool LocalStorageManager::ExpungeUser(const IUser & user, QString & errorDescription)
{
    errorDescription = QObject::tr("Can't expunge user from local storage database: ");

    if (!user.IsLocal()) {
        errorDescription += QObject::tr("user is not local, expunge is disallowed");
        return false;
    }

    const auto & enUser = user.GetEnUser();

    if (!enUser.__isset.id) {
        errorDescription += QObject::tr("user id is not set");
        return false;
    }

    QSqlQuery query(m_sqlDatabase);
    query.prepare("DELETE FROM Users WHERE id = ?");
    query.addBindValue(QString::number(enUser.id));

    bool res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("can't delete entry from \"Users\" table in SQL database");

    return true;
}

void LocalStorageManager::SwitchUser(const QString & username, const UserID userId)
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

    m_sqlDatabase = QSqlDatabase::addDatabase(sqlDriverName);

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

bool LocalStorageManager::AddNotebook(const Notebook & notebook, QString & errorDescription)
{
    errorDescription = QObject::tr("Can't add notebook to local storage database: ");
    QString error;

    bool res = notebook.checkParameters(error);
    if (!res) {
        errorDescription += error;
        return false;
    }

    int rowId = GetRowId("Notebooks", "guid", QVariant(notebook.guid()));
    if (rowId >= 0) {
        errorDescription += QObject::tr("notebook with specified guid already exists");
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
        return false;
    }

    int rowId = GetRowId("Notebooks", "guid", QVariant(notebook.guid()));
    if (rowId < 0) {
        errorDescription += QObject::tr("notebook was not found by guid");
        return false;
    }

    return InsertOrReplaceNotebook(notebook, errorDescription);
}

bool LocalStorageManager::FindNotebook(const QString & notebookGuid, Notebook & notebook,
                                       QString & errorDescription)
{
    errorDescription = QObject::tr("Can't find notebook in local storage database: ");

    if (!CheckGuid(notebookGuid)) {
        errorDescription += QObject::tr("requested guid is invalid");
        return false;
    }

    notebook = Notebook();

    QSqlQuery query(m_sqlDatabase);
    query.prepare("SELECT guid, updateSequenceNumber, name, creationTimestamp, "
                  "modificationTimestamp, isDirty, isLocal, isDefault, isLastUsed, "
                  "publishingUri, publishingNoteSortOrder, publishingAscendingOrder, "
                  "publicDescription, isPublished, stack, businessNotebookDescription, "
                  "businessNotebookPrivilegeLevel, businessNotebookIsRecommended, "
                  "contactId FROM Notebooks WHERE guid=?");
    query.addBindValue(notebookGuid);

    bool res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("can't find notebook in SQL database by guid");

    if (!query.next()) {
        errorDescription += QObject::tr("Internal error: SQL query result is empty");
        return false;
    }

    QSqlRecord rec = query.record();

    return FillNotebookFromSqlRecord(rec, notebook, errorDescription);
}

bool LocalStorageManager::ListAllNotebooks(std::vector<Notebook> & notebooks,
                                           QString & errorDescription) const
{
    QNDEBUG("LocalStorageManager::ListAllNotebooks");

    notebooks.clear();
    errorDescription = QObject::tr("Can't list all notebooks in local storage database: ");

    QSqlQuery query(m_sqlDatabase);
    bool res = query.exec("SELECT * FROM Notebooks");
    DATABASE_CHECK_AND_SET_ERROR("can't select all notebooks from SQL database");

    size_t numRows = query.size();
    notebooks.reserve(numRows);

    while(query.next())
    {
        QSqlRecord rec = query.record();

        notebooks.push_back(Notebook());
        auto & notebook = notebooks.back();

        res = FillNotebookFromSqlRecord(rec, notebook, errorDescription);
        if (!res) {
            return false;
        } 
    }

    QNDEBUG("found " << notebooks.size() << " notebooks");

    return true;
}

bool LocalStorageManager::ListAllSharedNotebooks(std::vector<SharedNotebookWrapper> & sharedNotebooks,
                                                 QString & errorDescription) const
{
    QNDEBUG("LocalStorageManager::ListAllSharedNotebooks");

    sharedNotebooks.clear();
    errorDescription = QObject::tr("Can't list all shared notebooks: ");

    QSqlQuery query(m_sqlDatabase);
    bool res = query.exec("SELECT * FROM SharedNotebooks");
    DATABASE_CHECK_AND_SET_ERROR("Can't select shared notebooks from SQL database");

    sharedNotebooks.reserve(std::max(query.size(), 0));

    while(query.next())
    {
        QSqlRecord record = query.record();

        sharedNotebooks.push_back(SharedNotebookWrapper());
        SharedNotebookWrapper & sharedNotebook = sharedNotebooks.back();

        res = FillSharedNotebookFromSqlRecord(record, sharedNotebook, errorDescription);
        if (!res) {
            return false;
        }
    }

    QNDEBUG("found " << sharedNotebooks.size() << " shared notebooks");

    return true;
}

bool LocalStorageManager::ListSharedNotebooksPerNotebookGuid(const QString & notebookGuid,
                                                             std::vector<SharedNotebookWrapper> & sharedNotebooks,
                                                             QString & errorDescription) const
{
    std::vector<SharedNotebook> enSharedNotebooks;
    bool res = ListSharedNotebooksPerNotebookGuid(notebookGuid, enSharedNotebooks,
                                                  errorDescription);
    if (!res) {
        return res;
    }

    size_t numFoundSharedNotebooks = enSharedNotebooks.size();
    sharedNotebooks.clear();
    sharedNotebooks.reserve(numFoundSharedNotebooks);

    for(size_t i = 0; i < numFoundSharedNotebooks; ++i) {
        sharedNotebooks.push_back(SharedNotebookWrapper(enSharedNotebooks[i]));
    }

    return true;
}

bool LocalStorageManager::ListSharedNotebooksPerNotebookGuid(const QString & notebookGuid,
                                                             std::vector<SharedNotebookAdapter> & sharedNotebooks,
                                                             QString & errorDescription) const
{
    std::vector<SharedNotebook> enSharedNotebooks;
    bool res = ListSharedNotebooksPerNotebookGuid(notebookGuid, enSharedNotebooks,
                                                  errorDescription);
    if (!res) {
        return res;
    }

    size_t numFoundSharedNotebooks = enSharedNotebooks.size();
    sharedNotebooks.clear();
    sharedNotebooks.reserve(numFoundSharedNotebooks);

    for(size_t i = 0; i < numFoundSharedNotebooks; ++i) {
        sharedNotebooks.push_back(SharedNotebookAdapter(enSharedNotebooks[i]));
    }

    return true;
}

bool LocalStorageManager::ListSharedNotebooksPerNotebookGuid(const QString & notebookGuid,
                                                             std::vector<SharedNotebook> & sharedNotebooks,
                                                             QString & errorDescription) const
{
    QNDEBUG("LocalStorageManager::ListSharedNotebooksPerNotebookGuid: guid = " << notebookGuid);

    sharedNotebooks.clear();
    errorDescription = QObject::tr("Can't list shared notebooks per notebook guid: ");

    if (!CheckGuid(notebookGuid)) {
        errorDescription += QObject::tr("notebook guid is invalid");
        return false;
    }

    QSqlQuery query(m_sqlDatabase);
    query.prepare("SELECT shareId, userId, email, creationTimestamp, modificationTimestamp, "
                  "shareKey, username, sharedNotebookPrivilegeLevel, allowPreview, "
                  "recipientReminderNotifyEmail, recipientReminderNotifyInApp "
                  "FROM SharedNotebooks WHERE notebookGuid=?");
    query.addBindValue(notebookGuid);

    bool res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("can't select shared notebooks for given "
                                 "notebook guid from SQL database");

    sharedNotebooks.reserve(std::max(query.size(), 0));

    while(query.next())
    {
        QSqlRecord record = query.record();

        sharedNotebooks.push_back(SharedNotebook());
        SharedNotebook & sharedNotebook = sharedNotebooks.back();
        SharedNotebookAdapter sharedNotebookAdapter(sharedNotebook);

        res = FillSharedNotebookFromSqlRecord(record, sharedNotebookAdapter, errorDescription);
        if (!res) {
            return false;
        }
    }

    QNDEBUG("found " << sharedNotebooks.size() << " shared notebooks");

    return true;
}

bool LocalStorageManager::ExpungeNotebook(const Notebook & notebook, QString & errorDescription)
{
    errorDescription = QObject::tr("Can't expunge notebook from local storage database: ");

    if (!notebook.hasGuid()) {
        errorDescription += QObject::tr("notebook's guid is not set");
        return false;
    }

    const QString notebookGuid = notebook.guid();
    if (!CheckGuid(notebookGuid)) {
        errorDescription += QObject::tr("notebook's guid is invalid");
        return false;
    }

    int rowId = GetRowId("Notebooks", "guid", QVariant(notebookGuid));
    if (rowId < 0) {
        errorDescription += QObject::tr("can't determine row id of notebook "
                                        "to be expunged in \"Notebooks\" table in SQL database");
        return false;
    }

    QSqlQuery query(m_sqlDatabase);
    query.prepare("DELETE FROM Notebooks WHERE rowid=?");
    query.addBindValue(QVariant(rowId));

    bool res = query.exec();
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
        return false;
    }

    int rowId = GetRowId("LinkedNotebooks", "guid",
                         QVariant(linkedNotebook.guid()));
    if (rowId >= 0) {
        errorDescription += QObject::tr("linked notebook with specified guid already exists");
        return false;
    }

    return InsertOrReplaceLinkedNotebook(linkedNotebook, errorDescription);
}

bool LocalStorageManager::UpdateLinkedNotebook(const LinkedNotebook & linkedNotebook, QString &errorDescription)
{
    errorDescription = QObject::tr("Can't update linked notebook in local storage database: ");
    QString error;

    bool res = linkedNotebook.checkParameters(error);
    if (!res) {
        errorDescription += error;
        return false;
    }

    int rowId = GetRowId("LinkedNotebooks", "guid",
                         QVariant(linkedNotebook.guid()));
    if (rowId < 0) {
        errorDescription += QObject::tr("linked notebook with specified guid "
                                        "was not found in local storage database");
        return false;
    }

    return InsertOrReplaceLinkedNotebook(linkedNotebook, errorDescription);
}

bool LocalStorageManager::FindLinkedNotebook(const QString & notebookGuid,
                                             LinkedNotebook & linkedNotebook,
                                             QString & errorDescription) const
{
    QNDEBUG("LocalStorageManager::FindLinkedNotebook: notebookGuid: " << notebookGuid);

    errorDescription = QObject::tr("Can't find linked notebook in local storage database: ");

    if (!CheckGuid(notebookGuid)) {
        errorDescription += QObject::tr("requested guid is invalid");
        return false;
    }

    linkedNotebook.clear();

    QSqlQuery query(m_sqlDatabase);
    query.prepare("SELECT guid, updateSequenceNumber, isDirty, shareName, username, shardId, "
                  "shareKey, uri, noteStoreUrl, webApiUrlPrefix, stack, businessId WHERE guid=?");
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

bool LocalStorageManager::ListAllLinkedNotebooks(std::vector<LinkedNotebook> & notebooks,
                                                 QString & errorDescription) const
{
    QNDEBUG("LocalStorageManager::ListAllLinkedNotebooks");

    notebooks.clear();
    errorDescription = QObject::tr("Can't list all linked notebooks in local storage database: ");

    QSqlQuery query(m_sqlDatabase);
    bool res = query.exec("SELECT * FROM LinkedNotebooks");
    DATABASE_CHECK_AND_SET_ERROR("can't select all linked notebooks from SQL database");

    size_t numRows = query.size();
    notebooks.reserve(numRows);

    while(query.next())
    {
        QSqlRecord rec = query.record();

        notebooks.push_back(LinkedNotebook());
        auto & notebook = notebooks.back();

        res = FillLinkedNotebookFromSqlRecord(rec, notebook, errorDescription);
        if (!res) {
            return false;
        }
    }

    QNDEBUG("found " << notebooks.size() << " notebooks");

    return true;
}

bool LocalStorageManager::ExpungeLinkedNotebook(const LinkedNotebook & linkedNotebook,
                                                QString & errorDescription)
{
    QNDEBUG("LocalStorageManager::ExpungeLinkedNotebook: " << linkedNotebook);

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

    int rowId = GetRowId("LinkedNotebooks", "guid", QVariant(linkedNotebookGuid));
    if (rowId < 0) {
        errorDescription += QObject::tr("can't determine row id of linked notebook "
                                        "to be expunged in \"LinkedNotebooks\" table in SQL database");
        return false;
    }

    QSqlQuery query(m_sqlDatabase);
    query.prepare("DELETE FROM LinkedNotebooks WHERE rowid=?");
    query.addBindValue(QVariant(rowId));

    bool res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("can't delete entry from \"LinkedNotebooks\" table in SQL database");

    return true;
}

bool LocalStorageManager::AddNote(const Note & note, QString & errorDescription)
{
    errorDescription = QObject::tr("Can't add note to local storage database: ");
    QString error;

    bool res = note.checkParameters(error);
    if (!res) {
        errorDescription += error;
        return false;
    }

    int rowId = GetRowId("Notes", "guid", QVariant(note.guid()));
    if (rowId >= 0) {
        errorDescription += QObject::tr("note with specified guid already exists");
        return false;
    }

    return InsertOrReplaceNote(note, errorDescription);
}

bool LocalStorageManager::UpdateNote(const Note & note, QString & errorDescription)
{
    errorDescription = QObject::tr("Can't update note in local storage database: ");
    QString error;

    bool res = note.checkParameters(error);
    if (!res) {
        errorDescription += error;
        return false;
    }

    int rowId = GetRowId("Notes", "guid", QVariant(note.guid()));
    if (rowId < 0) {
        errorDescription += QObject::tr("note with specified guid was not found");
        return false;
    }

    return InsertOrReplaceNote(note, errorDescription);
}

bool LocalStorageManager::FindNote(const QString & noteGuid, Note & note,
                                   QString & errorDescription) const
{
    errorDescription = QObject::tr("Can't find note in local storage database: ");

    if (!CheckGuid(noteGuid)) {
        errorDescription += QObject::tr("requested note guid is invalid");
        return false;
    }

    note = Note();

    QSqlQuery query(m_sqlDatabase);
    query.prepare("SELECT guid, updateSequenceNumber, title, isDirty, isLocal, content, "
                  "creationTimestamp, modificationTimestamp, isDeleted, deletionTimestap, "
                  "notebook FROM Notes WHERE guid=?");
    query.addBindValue(noteGuid);

    bool res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("can't select note from \"Notes\" table in SQL database");

    if (!query.next()) {
        errorDescription += QObject::tr("Internal error: SQL query result is empty");
        return false;
    }

    QSqlRecord rec = query.record();

#define CHECK_AND_SET_NOTE_PROPERTY(propertyLocalName, setter, type) \
    if (rec.contains(#propertyLocalName)) { \
        note.setter(qvariant_cast<type>(rec.value(#propertyLocalName))); \
    } \
    else { \
        errorDescription += QObject::tr("Internal error: no " #propertyLocalName " field " \
                                        "in the result of SQL query"); \
        return false; \
    }

    CHECK_AND_SET_NOTE_PROPERTY(isDirty, setDirty, bool);
    CHECK_AND_SET_NOTE_PROPERTY(isLocal, setLocal, bool);
    CHECK_AND_SET_NOTE_PROPERTY(isDeleted, setDeleted, bool);

    CHECK_AND_SET_NOTE_PROPERTY(guid, setGuid, QString);
    CHECK_AND_SET_NOTE_PROPERTY(updateSequenceNumber, setUpdateSequenceNumber, qint32);

    CHECK_AND_SET_NOTE_PROPERTY(notebookGuid, setNotebookGuid, QString);
    CHECK_AND_SET_NOTE_PROPERTY(title, setTitle, QString);
    CHECK_AND_SET_NOTE_PROPERTY(content, setContent, QString);

    // NOTE: omitting content hash and content length as it will be set by the service

    CHECK_AND_SET_NOTE_PROPERTY(creationTimestamp, setCreationTimestamp, qint32);
    CHECK_AND_SET_NOTE_PROPERTY(modificationTimestamp, setModificationTimestamp, qint32);

    if (note.isDeleted()) {
        CHECK_AND_SET_NOTE_PROPERTY(deletionTimestamp, setDeletionTimestamp, qint32);
    }

    CHECK_AND_SET_NOTE_PROPERTY(isActive, setActive, bool);

    QString error;
    res = FindAndSetTagGuidsPerNote(note, error);
    if (!res) {
        errorDescription += error;
        return false;
    }

    error.clear();
    res = FindAndSetResourcesPerNote(note, error);
    if (!res) {
        errorDescription += error;
        return false;
    }

    if (rec.contains("hasAttributes"))
    {
        int hasAttributes = qvariant_cast<int>(rec.value("hasAttributes"));
        if (hasAttributes == 0) {
            return true;
        }
    }
    else {
        errorDescription += QObject::tr("Internal error: no \"hasAttributes\" field " \
                                        "in the result of SQL query");
        return false;
    }

    error.clear();
    res = FindAndSetNoteAttributesPerNote(note, error);
    if (!res) {
        errorDescription += error;
        return false;
    }

    // NOTE: no validity check of note here: it may be incomplete, for example, due to
    // not yet complete synchronization procedure
    return true;
}

bool LocalStorageManager::FindAllNotesPerNotebook(const QString & notebookGuid, std::vector<Note> & notes,
                                                  QString & errorDescription) const
{
    errorDescription = QObject::tr("Can't find all notes per notebook: ");

    notes.clear();

    if (!CheckGuid(notebookGuid)) {
        errorDescription += QObject::tr("notebook guid is invalid");
        return false;
    }

    QSqlQuery query(m_sqlDatabase);
    query.prepare("SELECT * FROM Notes WHERE notebookGuid = ?");
    query.addBindValue(notebookGuid);

    bool res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("can't select notes per notebook guid from SQL database");

    size_t numRows = query.size();
    notes.reserve(numRows);

    while(query.next())
    {
        QSqlRecord rec = query.record();
        Note note;

        CHECK_AND_SET_NOTE_PROPERTY(isDirty, setDirty, bool);
        CHECK_AND_SET_NOTE_PROPERTY(isLocal, setLocal, bool);
        CHECK_AND_SET_NOTE_PROPERTY(isDeleted, setDeleted, bool);

        note.setNotebookGuid(notebookGuid);

        CHECK_AND_SET_NOTE_PROPERTY(updateSequenceNumber, setUpdateSequenceNumber, qint32);
        CHECK_AND_SET_NOTE_PROPERTY(guid, setGuid, QString);
        CHECK_AND_SET_NOTE_PROPERTY(title, setTitle, QString);
        CHECK_AND_SET_NOTE_PROPERTY(content, setContent, QString);

        // NOTE: omitting content hash and content length as it will be set by the service

        CHECK_AND_SET_NOTE_PROPERTY(creationTimestamp, setCreationTimestamp, qint32);
        CHECK_AND_SET_NOTE_PROPERTY(modificationTimestamp, setModificationTimestamp, qint32);

        if (note.isDeleted()) {
            CHECK_AND_SET_NOTE_PROPERTY(deletionTimestamp, setDeletionTimestamp, qint32);
        }

        CHECK_AND_SET_NOTE_PROPERTY(isActive, setActive, bool);

        notes.push_back(note);
    }

    if (notes.empty()) {
        // NOTE: negative result is a result too, return empty vector and true
        return true;
    }

    for(Note & note: notes)
    {
        const QString guid = note.guid();
        if (!CheckGuid(guid)) {
            errorDescription += QObject::tr("found note with invalid guid");
            return false;
        }

        QString error;
        res = FindAndSetTagGuidsPerNote(note, error);
        if (!res) {
            errorDescription += error;
            return false;
        }

        error.clear();
        res = FindAndSetResourcesPerNote(note, error);
        if (!res) {
            errorDescription += error;
            return false;
        }

        error.clear();
        res = FindAndSetNoteAttributesPerNote(note, error);
        if (!res) {
            errorDescription += error;
            return false;
        }
    }

    return true;
}

bool LocalStorageManager::DeleteNote(const Note & note, QString & errorDescription)
{
    if (note.isLocal()) {
        return ExpungeNote(note, errorDescription);
    }

    errorDescription = QObject::tr("Can't delete note from local storage database: ");

    const QString guid = note.guid();
    if (!CheckGuid(guid)) {
        errorDescription += QObject::tr("note guid is invalid");
        return false;
    }

    if (!note.isDeleted()) {
        errorDescription += QObject::tr("note to be deleted is not marked as "
                                        "deleted one, rejecting to delete it");
        return false;
    }

    QSqlQuery query(m_sqlDatabase);
    query.prepare("UPDATE Notes SET isDeleted=1, isDirty=1 WHERE guid=?");
    query.addBindValue(guid);
    bool res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("can't delete entry from \"Notes\" table in SQL database");

    return true;
}

bool LocalStorageManager::ExpungeNote(const Note & note, QString & errorDescription)
{
    errorDescription = QObject::tr("Can't expunge note from local storage database: ");

    const QString guid = note.guid();
    if (!CheckGuid(guid)) {
        errorDescription += QObject::tr("note guid is invalid");
        return false;
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

    QSqlQuery query(m_sqlDatabase);
    query.prepare("SELECT rowid FROM Notes WHERE guid=?");
    query.addBindValue(guid);
    bool res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("can't find note to be expunged SQL database");

    int rowId = -1;
    bool conversionResult = false;
    while(query.next()) {
        rowId = query.value(0).toInt(&conversionResult);
    }

    if (!conversionResult || (rowId < 0)) {
        errorDescription += QObject::tr("can't find row id of note to be expunged in \"Notes\" table in SQL database");
        return false;
    }

    query.clear();
    query.prepare("DELETE FROM Notes WHERE rowid=?");
    query.addBindValue(QVariant(rowId));

    res = query.exec();
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
        return false;
    }

    QString guid = tag.guid();
    int rowId = GetRowId("Tags", "guid", QVariant(guid));
    if (rowId >= 0) {
        errorDescription += QObject::tr("tag with the same guid already exists");
        return false;
    }

    QString nameUpper = tag.name().toUpper();
    rowId = GetRowId("Tags", "nameUpper", QVariant(nameUpper));
    if (rowId >= 0) {
        errorDescription += QObject::tr("tag with similar name (case insensitive) "
                                       "already exists");
        return false;
    }

    return InsertOrReplaceTag(tag, errorDescription);
}

bool LocalStorageManager::UpdateTag(const Tag &tag, QString &errorDescription)
{
    errorDescription = QObject::tr("Can't update tag in local storage database: ");
    QString error;

    bool res = tag.checkParameters(error);
    if (!res) {
        errorDescription += error;
        return false;
    }

    QString guid = tag.guid();
    int rowId = GetRowId("Tags", "guid", QVariant(guid));
    if (rowId < 0) {
        errorDescription += QObject::tr("tag with specified guid was not found");
        return false;
    }

    QString nameUpper = tag.name().toUpper();
    rowId = GetRowId("Tags", "nameUpper", QVariant(nameUpper));
    if (rowId < 0) {
        errorDescription += QObject::tr("tag with similar name (case insensitive) "
                                        "was not found");
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

    QSqlQuery query(m_sqlDatabase);
    query.prepare("INSERT OR REPLACE INTO NoteTags (note, tag) VALUES(?, ?)");
    query.addBindValue(note.guid());
    query.addBindValue(tag.guid());

    bool res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("can't insert or replace note guid + tag guid in "
                                 "\"NoteTags\" table in SQL storage database");

    return true;
}

bool LocalStorageManager::FindTag(const QString & tagGuid, Tag & tag, QString & errorDescription) const
{
    errorDescription = QObject::tr("Can't find tag in local storage database: ");

    if (!CheckGuid(tagGuid)) {
        errorDescription += QObject::tr("requested tag guid is invalid");
        return false;
    }

    tag.clear();

    QSqlQuery query(m_sqlDatabase);

    query.prepare("SELECT updateSequenceNumber, name, parentGuid, isDirty, isLocal, "
                  "isDeleted FROM Tags WHERE guid = ?");
    query.addBindValue(tagGuid);

    bool res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("can't select tag from \"Tags\" table in SQL database: ");

    if (!query.next()) {
        errorDescription += QObject::tr("Internal error: SQL query result is empty");
        return false;
    }

    tag.setGuid(tagGuid);

    bool conversionResult = false;
    qint32 updateSequenceNumber = static_cast<qint32>(query.value(0).toInt(&conversionResult));
    if (updateSequenceNumber < 0 || !conversionResult) {
        errorDescription += QObject::tr("No tag's update sequence number was found in SQL query result");
        return false;
    }
    tag.setUpdateSequenceNumber(updateSequenceNumber);

    QString name = query.value(1).toString();
    size_t nameSize = name.size();
    if ( (nameSize < evernote::limits::g_Limits_constants.EDAM_TAG_NAME_LEN_MIN) ||
         (nameSize > evernote::limits::g_Limits_constants.EDAM_TAG_NAME_LEN_MAX) )
    {
        errorDescription += QObject::tr("Internal error: found tag with invalid "
                                        "name size from SQL query result");
        return false;
    }
    tag.setName(name);

    QString parentGuid = query.value(2).toString();
    if (parentGuid.isEmpty() || CheckGuid(parentGuid)) {
        tag.setParentGuid(parentGuid);
    }
    else {
        errorDescription += QObject::tr("Internal error: found tag with invalid parent guid "
                                        "in SQL query result");
        return false;
    }

    conversionResult = false;
    int isDirtyInt = query.value(3).toInt(&conversionResult);
    if (isDirtyInt < 0 || !conversionResult) {
        errorDescription += QObject::tr("No tag's \"isDirty\" flag was found in SQL result");
        return false;
    }
    tag.setDirty(static_cast<bool>(isDirtyInt));

    conversionResult = false;
    int isLocalInt = query.value(4).toInt(&conversionResult);
    if (isLocalInt < 0 || !conversionResult) {
        errorDescription += QObject::tr("No tag's \"isLocal\" flag was found in SQL result");
        return false;
    }
    tag.setLocal(static_cast<bool>(isLocalInt));

    conversionResult = false;
    int isDeletedInt = query.value(5).toInt(&conversionResult);
    if (isDeletedInt < 0 || !conversionResult) {
        errorDescription += QObject::tr("No tag's \"isDeleted\" flag was found in SQL result");
        return false;
    }
    tag.setDeleted(static_cast<bool>(isDeletedInt));

    return tag.checkParameters(errorDescription);
}

bool LocalStorageManager::FindAllTagsPerNote(const QString & noteGuid, std::vector<Tag> & tags,
                                             QString & errorDescription) const
{
    QNDEBUG("LocalStorageManager::FindAllTagsPerNote: note guid = " << noteGuid);

    tags.clear();
    errorDescription = QObject::tr("Can't find all tags per note in local storage database: ");

    if (!CheckGuid(noteGuid)) {
        errorDescription += QObject::tr("note's guid is invalid");
        return false;
    }

    QSqlQuery query(m_sqlDatabase);
    query.prepare("SELECT tag FROM NoteTags WHERE note=?");
    query.addBindValue(noteGuid);

    bool res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("can't select tag guid from \"NoteTags\" table in SQL database");

    res = FillTagsFromSqlQuery(query, tags, errorDescription);
    if (!res) {
        return false;
    }

    QNDEBUG("found " << tags.size() << " tags");

    return true;
}

bool LocalStorageManager::ListAllTags(std::vector<Tag> & tags, QString & errorDescription) const
{
    QNDEBUG("LocalStorageManager::ListAllTags");

    tags.clear();

    errorDescription = QObject::tr("Can't list all tags in local storage database: ");

    QSqlQuery query(m_sqlDatabase);
    bool res = query.exec("SELECT guid FROM Tags");
    DATABASE_CHECK_AND_SET_ERROR("can't select tag guids from SQL database");

    res = FillTagsFromSqlQuery(query, tags, errorDescription);
    if (!res) {
        return false;
    }

    QNDEBUG("found " << tags.size() << " tags");

    return true;
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

    int rowId = GetRowId("Tags", "guid", QVariant(tag.guid()));
    if (rowId < 0) {
        errorDescription += QObject::tr("tag to be expunged was not found by guid");
        return false;
    }

    QSqlQuery query(m_sqlDatabase);
    query.prepare("DELETE FROM Tags WHERE rowid = ?");
    query.addBindValue(rowId);

    bool res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("can't delete tag from \"Tags\" table in SQL database");

    return true;
}

bool LocalStorageManager::FindResource(const QString & resourceGuid, IResource & resource,
                                       QString & errorDescription, const bool withBinaryData) const
{
    errorDescription = QObject::tr("Can't find resource in local storage database: ");

    if (!CheckGuid(resourceGuid)) {
        errorDescription += QObject::tr("requested resource guid is invalid");
        return false;
    }

    resource.clear();
    resource.setGuid(resourceGuid);

    QSqlQuery query(m_sqlDatabase);
    query.prepare("SELECT noteGuid, updateSequenceNumber, isDirty, dataSize, dataHash, "
                  "mime, width, height, recognitionSize, recognitionHash :binaryColumns "
                  "FROM Resources WHERE guid = ?");
    if (withBinaryData) {
        query.bindValue("binaryColumns", ", dataBody, recognitionBody");
    }
    else {
        query.bindValue("binaryColumns", "");
    }

    bool res = query.exec();
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
    CHECK_AND_SET_RESOURCE_PROPERTY(isDirty, int, bool, setDirty, isRequired);
    CHECK_AND_SET_RESOURCE_PROPERTY(noteGuid, QString, QString, setNoteGuid, isRequired);
    CHECK_AND_SET_RESOURCE_PROPERTY(updateSequenceNumber, int, qint32, setUpdateSequenceNumber, isRequired);
    CHECK_AND_SET_RESOURCE_PROPERTY(dataSize, int, qint32, setDataSize, isRequired);
    CHECK_AND_SET_RESOURCE_PROPERTY(dataHash, QString, QString, setDataHash, isRequired);
    CHECK_AND_SET_RESOURCE_PROPERTY(mime, QString, QString, setMime, isRequired);

    isRequired = false;

    CHECK_AND_SET_RESOURCE_PROPERTY(width, int, qint32, setWidth, isRequired);
    CHECK_AND_SET_RESOURCE_PROPERTY(height, int, qint32, setHeight, isRequired);
    CHECK_AND_SET_RESOURCE_PROPERTY(recognitionDataSize, int, qint32, setRecognitionDataSize, isRequired);
    CHECK_AND_SET_RESOURCE_PROPERTY(recognitionDataHash, QString, QString, setRecognitionDataHash, isRequired);

    if (withBinaryData) {
        CHECK_AND_SET_RESOURCE_PROPERTY(recognitionDataBody, QString, QString, setRecognitionDataBody, isRequired);
        CHECK_AND_SET_RESOURCE_PROPERTY(dataBody, QString, QString, setDataBody, isRequired);
    }

    query.prepare("SELECT data FROM ResourceAttributes WHERE guid = ?");
    query.addBindValue(resourceGuid);

    res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("can't select data from \"ResourceAttributes\" table in SQL database");

    if (!query.next()) {
        return true;
    }

    QByteArray serializedResourceAttributes = query.value(0).toByteArray();
    resource.setResourceAttributes(serializedResourceAttributes);

    return true;
}

bool LocalStorageManager::ExpungeResource(const IResource & resource, QString & errorDescription)
{
    errorDescription = QObject::tr("Can't expunge resource from local storage database: ");

    int rowId = GetRowId("Resources", "guid", QVariant(resource.guid()));
    if (rowId < 0) {
        errorDescription += QObject::tr("resource to be expunged was not found by guid");
        return false;
    }

    QSqlQuery query(m_sqlDatabase);
    query.prepare("DELETE FROM Resources WHERE rowid = ?");
    query.addBindValue(rowId);

    bool res = query.exec();
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
        return false;
    }

    QString guid = search.guid();
    int rowId = GetRowId("SavedSearches", "guid", QVariant(guid));
    if (rowId >= 0) {
        errorDescription += QObject::tr("saved search with the same guid already exists");
        return false;
    }

    return InsertOrReplaceSavedSearch(search, errorDescription);
}

bool LocalStorageManager::UpdateSavedSearch(const SavedSearch & search, QString & errorDescription)
{
    errorDescription = QObject::tr("Can't update saved search in local storage database: ");
    QString error;

    bool res = search.checkParameters(error);
    if (!res) {
        errorDescription += error;
        return false;
    }

    QString guid = search.guid();
    int rowId = GetRowId("SavedSearches", "guid", QVariant(guid));
    if (rowId < 0) {
        errorDescription += QObject::tr("saved search with specified guid was not found");
        return false;
    }

    return InsertOrReplaceSavedSearch(search, errorDescription);
}

bool LocalStorageManager::FindSavedSearch(const QString & searchGuid, SavedSearch & search,
                                          QString & errorDescription) const
{
    errorDescription = QObject::tr("Can't find saved search in local storage database: ");

    if (!CheckGuid(searchGuid)) {
        errorDescription += QObject::tr("requested saved search guid is invalid");
        return false;
    }

    search.clear();

    QSqlQuery query(m_sqlDatabase);

    query.prepare("SELECT guid, name, query, format, updateSequenceNumber, includeAccount, "
                  "includePersonalLinkedNotebooks, includeBusinessLinkedNotebooks "
                  "FROM SavedSearches WHERE guid = ?");
    query.addBindValue(searchGuid);

    bool res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("can't find saved search in \"SavedSearches\" table in SQL database");

    if (!query.next()) {
        errorDescription += QObject::tr("Internal error: SQL query result is empty");
        return false;
    }

    QSqlRecord rec = query.record();
    return FillSavedSearchFromSqlRecord(rec, search, errorDescription);
}

bool LocalStorageManager::ListAllSavedSearches(std::vector<SavedSearch> & searches,
                                               QString & errorDescription) const
{
    QNDEBUG("LocalStorageManager::ListAllSavedSearches");

    searches.clear();

    errorDescription = QObject::tr("Can't find all saved searches in local storage database");

    QSqlQuery query(m_sqlDatabase);
    bool res = query.exec("SELECT * FROM SavedSearches");
    DATABASE_CHECK_AND_SET_ERROR("can't select all saved searches from \"SavedSearches\" "
                                 "table in SQL database");

    size_t numRows = query.size();
    if (numRows == 0) {
        QNDEBUG("No saved searches were found");
        return true;
    }

    searches.reserve(numRows);

    while(query.next())
    {
        QSqlRecord rec = query.record();

        searches.push_back(SavedSearch());
        auto & search = searches.back();

        res = FillSavedSearchFromSqlRecord(rec, search, errorDescription);
        if (!res) {
            return false;
        }
    }

    QNDEBUG("found " << searches.size() << " saved searches");

    return true;
}

bool LocalStorageManager::ExpungeSavedSearch(const SavedSearch & search,
                                             QString & errorDescription)
{
    errorDescription = QObject::tr("Can't expunge saved search from local storage database: ");

    if (!search.hasGuid()) {
        errorDescription += QObject::tr("saved search's guid is not set");
        return false;
    }
    else if (!CheckGuid(search.guid())) {
        errorDescription += QObject::tr("saved search's guid is invalid");
        return false;
    }

    int rowId = GetRowId("SavedSearches", "guid", QVariant(search.guid()));
    if (rowId < 0) {
        errorDescription += QObject::tr("saved search to be expunged was not found");
        return false;
    }

    QSqlQuery query(m_sqlDatabase);
    query.prepare("DELETE FROM SavedSearches WHERE rowid = ?");
    query.addBindValue(rowId);

    bool res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("can't delete saved search from \"SavedSearches\" table in SQL database");

    return true;
}

bool LocalStorageManager::AddResource(const IResource & resource, QString & errorDescription)
{
    errorDescription = QObject::tr("Can't add resource to local storage database: ");

    QString error;
    if (!resource.checkParameters(error)) {
        errorDescription += error;
        return false;
    }

    int rowId = GetRowId("Resources", "guid", QVariant(resource.guid()));
    if (rowId >= 0) {
        errorDescription += QObject::tr("resource with the same guid already exists");
        return false;
    }

    return InsertOrReplaceResource(resource, errorDescription);
}

bool LocalStorageManager::UpdateResource(const IResource & resource, QString & errorDescription)
{
    errorDescription = QObject::tr("Can't update resource in local storage database: ");

    QString error;
    if (!resource.checkParameters(error)) {
        errorDescription += error;
        return false;
    }

    int rowId = GetRowId("Resources", "guid", QVariant(resource.guid()));
    if (rowId < 0) {
        errorDescription += QObject::tr("resource to be updated was not found by guid");
        return false;
    }

    return InsertOrReplaceResource(resource, errorDescription);
}

bool LocalStorageManager::CreateTables(QString & errorDescription)
{
    QSqlQuery query(m_sqlDatabase);
    bool res;

    res = query.exec("CREATE TABLE IF NOT EXISTS Users("
                     "  id                      INTEGER PRIMARY KEY     NOT NULL UNIQUE, "
                     "  username                TEXT                    NOT NULL, "
                     "  name                    TEXT                    NOT NULL, "
                     "  timezone                TEXT                    DEFAULT NULL, "
                     "  privilege               INTEGER                 NOT NULL, "
                     "  creationTimestamp       INTEGER                 NOT NULL, "
                     "  modificationTimestamp   INTEGER                 NOT NULL, "
                     "  isDirty                 INTEGER                 NOT NULL, "
                     "  isLocal                 INTEGER                 NOT NULL, "
                     "  deletionTimestamp       INTEGER                 DEFAULT 0, "
                     "  isActive                INTEGER                 NOT NULL"
                     ")");
    DATABASE_CHECK_AND_SET_ERROR("can't create Users table");

    res = query.exec("CREATE TABLE IF NOT EXISTS UserAttributes("
                     "  id REFERENCES Users(id) ON DELETE CASCADE ON UPDATE CASCADE, "
                     "  data                    BLOB                    DEFAULT NULL)");
    DATABASE_CHECK_AND_SET_ERROR("can't create UserAttributes table");

    res = query.exec("CREATE TABLE IF NOT EXISTS Accounting("
                     "  id REFERENCES Users(id) ON DELETE CASCADE ON UPDATE CASCADE, "
                     "  data                    BLOB                    DEFAULT NULL)");
    DATABASE_CHECK_AND_SET_ERROR("can't create Accounting table");

    res = query.exec("CREATE TABLE IF NOT EXISTS PremiumInfo("
                     "  id REFERENCES Users(id) ON DELETE CASCADE ON UPDATE CASCADE, "
                     "  data                    BLOB                    DEFAULT NULL)");
    DATABASE_CHECK_AND_SET_ERROR("can't create PremiumInfo table");

    res = query.exec("CREATE TABLE IF NOT EXISTS BusinessUserInfo("
                     "  id REFERENCES Users(id) ON DELETE CASCADE ON UPDATE CASCADE, "
                     "  data                    BLOB                    DEFAULT NULL)");
    DATABASE_CHECK_AND_SET_ERROR("can't create BusinessUserInfo table");

    res = query.exec("CREATE TABLE IF NOT EXISTS Notebooks("
                     "  guid                            TEXT PRIMARY KEY  NOT NULL UNIQUE, "
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
                     "  contactId                       INTEGER           DEFAULT 0"
                     ")");
    DATABASE_CHECK_AND_SET_ERROR("can't create Notebooks table");

    res = query.exec("CREATE TABLE IF NOT EXISTS NotebookRestrictions("
                     "  guid REFERENCES Notebooks(guid) ON DELETE CASCADE ON UPDATE CASCADE, "
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
                     "  UNIQUE(shareId, notebookGuid) ON CONFLICT REPLACE"
                     ")");
    DATABASE_CHECK_AND_SET_ERROR("can't create SharedNotebooks table");

    res = query.exec("CREATE VIRTUAL TABLE IF NOT EXISTS NoteText USING fts3("
                     "  guid              TEXT PRIMARY KEY     NOT NULL UNIQUE, "
                     "  title             TEXT, "
                     "  content           TEXT, "
                     "  notebookGuid REFERENCES Notebooks(guid) ON DELETE CASCADE ON UPDATE CASCADE"
                     ")");
    DATABASE_CHECK_AND_SET_ERROR("can't create virtual table NoteText");

    res = query.exec("CREATE TABLE IF NOT EXISTS Notes("
                     "  guid                    TEXT PRIMARY KEY     NOT NULL UNIQUE, "
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
                     "  notebookGuid REFERENCES Notebooks(guid) ON DELETE CASCADE ON UPDATE CASCADE"
                     ")");
    DATABASE_CHECK_AND_SET_ERROR("can't create Notes table");

    res = query.exec("CREATE TABLE IF NOT EXISTS NoteAttributes("
                     "  noteGuid REFERENCES Notes(guid) ON DELETE CASCADE ON UPDATE CASCADE, "
                     "  data                    BLOB                DEFAULT NULL"
                     ")");
    DATABASE_CHECK_AND_SET_ERROR("can't create NoteAttributes table");

    res = query.exec("CREATE INDEX IF NOT EXISTS NotesNotebooks ON Notes(notebookGuid)");
    DATABASE_CHECK_AND_SET_ERROR("can't create index NotesNotebooks");

    res = query.exec("CREATE TABLE IF NOT EXISTS Resources("
                     "  guid                    TEXT PRIMARY KEY     NOT NULL UNIQUE, "
                     "  noteGuid                TEXT                 NOT NULL, "
                     "  updateSequenceNumber    INTEGER              NOT NULL, "
                     "  isDirty                 INTEGER              NOT NULL, "
                     "  dataBody                TEXT                 NOT NULL, "
                     "  dataSize                INTEGER              NOT NULL, "
                     "  dataHash                TEXT                 NOT NULL, "
                     "  mime                    TEXT                 NOT NULL, "
                     "  width                   INTEGER              DEFAULT 0, "
                     "  height                  INTEGER              DEFAULT 0, "
                     "  recognitionBody         TEXT                 DEFAULT NULL, "
                     "  recognitionSize         INTEGER              DEFAULT 0, "
                     "  recognitionHash         TEXT                 DEFAULT NULL"
                     ")");
    DATABASE_CHECK_AND_SET_ERROR("can't create Resources table");

    res = query.exec("CREATE INDEX IF NOT EXISTS ResourceNote ON Resources(noteGuid)");
    DATABASE_CHECK_AND_SET_ERROR("can't create ResourceNote index");

    res = query.exec("CREATE TABLE IF NOT EXISTS ResourceAttributes("
                     "  guid REFERENCES Resource(guid) ON DELETE CASCADE ON UPDATE CASCADE, "
                     "  data                  BLOB                 DEFAULT NULL)");
    DATABASE_CHECK_AND_SET_ERROR("can't create ResourceAttributes table");

    res = query.exec("CREATE TABLE IF NOT EXISTS Tags("
                     "  guid                  TEXT PRIMARY KEY     NOT NULL UNIQUE, "
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
                     "  note REFERENCES Notes(guid) ON DELETE CASCADE ON UPDATE CASCADE, "
                     "  tag  REFERENCES Tags(guid)  ON DELETE CASCADE ON UPDATE CASCADE, "
                     "  UNIQUE(note, tag) ON CONFLICT REPLACE"
                     ")");
    DATABASE_CHECK_AND_SET_ERROR("can't create NoteTags table");

    res = query.exec("CREATE INDEX IF NOT EXISTS NoteTagsNote ON NoteTags(note)");
    DATABASE_CHECK_AND_SET_ERROR("can't create NoteTagsNote index");

    res = query.exec("CREATE TABLE IF NOT EXISTS SavedSearches("
                     "  guid                            TEXT PRIMARY KEY    NOT NULL UNIQUE, "
                     "  name                            TEXT                NOT NULL, "
                     "  query                           TEXT                NOT NULL, "
                     "  format                          INTEGER             NOT NULL, "
                     "  updateSequenceNumber            INTEGER             NOT NULL, "
                     "  includeAccount                  INTEGER             NOT NULL, "
                     "  includePersonalLinkedNotebooks  INTEGER             NOT NULL, "
                     "  includeBusinessLinkedNotebooks  INTEGER             NOT NULL)");
    DATABASE_CHECK_AND_SET_ERROR("can't create SavedSearches table");

    return true;
}

bool LocalStorageManager::SetNoteAttributes(const Note & note, QString & errorDescription)
{
    errorDescription += QObject::tr("can't set note attributes: ");

    const QString guid = note.guid();
    if (!CheckGuid(guid.toStdString())) {
        errorDescription += QObject::tr("note guid is invalid");
        return false;
    }

    if (!note.hasNoteAttributes()) {
        return true;
    }

    QSqlQuery query(m_sqlDatabase);
    query.prepare("INSERT OR REPLACE INTO NoteAttributes(noteGuid, data) VALUES(?, ?)");
    query.addBindValue(guid);
    query.addBindValue(note.noteAttributes());

    bool res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("can't insert or replace lastEditorId into \"NoteAttributes\" table in SQL database");

    return true;
}

bool LocalStorageManager::SetNotebookAdditionalAttributes(const Notebook & notebook,
                                                          QString & errorDescription)
{
    QSqlQuery query(m_sqlDatabase);

    bool hasAdditionalAttributes = false;
    query.prepare("INSERT OR REPLACE INTO Notebooks (:notebookGuid, :columns) VALUES(:values)");
    query.bindValue("notebookGuid", QVariant(notebook.guid()));

    QString columns;
    QString values;

    if (notebook.hasPublished())
    {
        hasAdditionalAttributes = true;

        bool notebookIsPublished = notebook.isPublished();

        columns.append("isPublished");
        values.append(QString::number(notebookIsPublished ? 1 : 0));

        if (notebookIsPublished)
        {

#define APPEND_SEPARATORS \
    if (!columns.isEmpty()) { columns.append(", "); } \
    if (!values.isEmpty()) { values.append(", "); }

            if (notebook.hasPublishingUri()) {
                APPEND_SEPARATORS;
                columns.append("publishingUri");
                values.append(notebook.publishingUri());
            }

            if (notebook.hasPublishingOrder()) {
                APPEND_SEPARATORS;
                columns.append("publishingNoteSortOrder");
                values.append(notebook.publishingOrder());
            }

            if (notebook.hasPublishingAscending()) {
                APPEND_SEPARATORS;
                columns.append("publishingAscendingSort");
                values.append(notebook.isPublishingAscending() ? 1 : 0);
            }

            if (notebook.hasPublishingPublicDescription()) {
                APPEND_SEPARATORS;
                columns.append("publicDescription");
                values.append(notebook.publishingPublicDescription());
            }
        }
    }

    if (notebook.hasBusinessNotebookDescription()) {
        hasAdditionalAttributes = true;
        APPEND_SEPARATORS;
        columns.append("businessNotebookDescription");
        values.append(notebook.businessNotebookDescription());
    }

    if (notebook.hasBusinessNotebookPrivilegeLevel()) {
        hasAdditionalAttributes = true;
        APPEND_SEPARATORS;
        columns.append("businessNotebookPrivilegeLevel");
        values.append(notebook.businessNotebookPrivilegeLevel());
    }

    if (notebook.hasBusinessNotebookRecommended()) {
        hasAdditionalAttributes = true;
        APPEND_SEPARATORS;
        columns.append("businessNotebookIsRecommended");
        values.append(notebook.isBusinessNotebookRecommended() ? 1 : 0);
    }

    if (notebook.hasContact())
    {
        hasAdditionalAttributes = true;
        APPEND_SEPARATORS;
        columns.append("contactId");
        const UserAdapter user = notebook.contact();
        values.append(user.GetEnUser().id);

        QString error;
        bool res = user.CheckParameters(error);
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

    if (hasAdditionalAttributes) {
        query.bindValue("columns", columns);
        query.bindValue("values", values);
        bool res = query.exec();
        DATABASE_CHECK_AND_SET_ERROR("can't insert or replace additional notebook attributes "
                                     "into \"Notebooks\" table in SQL database");
    }

    if (notebook.hasRestrictions())
    {
        QString error;
        bool res = SetNotebookRestrictions(notebook.restrictions(), notebook.guid(), error);
        if (!res) {
            errorDescription += error;
            return res;
        }
    }

    std::vector<SharedNotebookAdapter> sharedNotebooks;
    notebook.sharedNotebooks(sharedNotebooks);

    for(const auto & sharedNotebook: sharedNotebooks)
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

bool LocalStorageManager::SetNotebookRestrictions(const evernote::edam::NotebookRestrictions & notebookRestrictions,
                                                  const QString & notebookGuid, QString & errorDescription)
{
    errorDescription = QObject::tr("Can't set notebook restrictions: ");

    QSqlQuery query(m_sqlDatabase);
    query.prepare("INSERT OR REPLACE INTO NotebookRestrictions (:notebookGuid, :columns) "
                  "VALUES(:vals)");
    query.bindValue("notebookGuid", notebookGuid);

    bool hasAnyRestriction = false;

    QString columns, values;

#define CHECK_AND_SET_NOTEBOOK_RESTRICTION(restriction, value) \
    if (notebookRestrictions.__isset.restriction) \
    { \
        hasAnyRestriction = true; \
        \
        if (!columns.isEmpty()) { \
            columns.append(", "); \
        } \
        columns.append(#restriction); \
        if (!values.isEmpty()) { \
            values.append(", "); \
        } \
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
        query.bindValue("columns", columns);
        query.bindValue("vals", values);
        bool res = query.exec();
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

    QSqlQuery query(m_sqlDatabase);
    query.prepare("INSERT OR REPLACE INTO SharedNotebooks(:columns) VALUES(:vals)");

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
    CHECK_AND_SET_SHARED_NOTEBOOK_ATTRIBUTE(hasNotebookGuid, notebookGuid, sharedNotebook.notebookGuid());
    CHECK_AND_SET_SHARED_NOTEBOOK_ATTRIBUTE(hasEmail, email, sharedNotebook.email());
    CHECK_AND_SET_SHARED_NOTEBOOK_ATTRIBUTE(hasCreationTimestamp,creationTimestamp,
                                            QString::number(sharedNotebook.creationTimestamp()));
    CHECK_AND_SET_SHARED_NOTEBOOK_ATTRIBUTE(hasModificationTimestamp, modificationTimestamp,
                                            QString::number(sharedNotebook.modificationTimestamp()));
    CHECK_AND_SET_SHARED_NOTEBOOK_ATTRIBUTE(hasShareKey, shareKey, sharedNotebook.shareKey());
    CHECK_AND_SET_SHARED_NOTEBOOK_ATTRIBUTE(hasUsername, username, sharedNotebook.username());
    CHECK_AND_SET_SHARED_NOTEBOOK_ATTRIBUTE(hasPrivilegeLevel, sharedNotebookPrivilegeLevel,
                                            QString::number(sharedNotebook.privilegeLevel()));
    CHECK_AND_SET_SHARED_NOTEBOOK_ATTRIBUTE(hasAllowPreview, allowPreview,
                                            QString::number(sharedNotebook.allowPreview() ? 1 : 0));
    CHECK_AND_SET_SHARED_NOTEBOOK_ATTRIBUTE(hasReminderNotifyEmail, recipientReminderNotifyEmail,
                                            QString::number(sharedNotebook.reminderNotifyEmail() ? 1 : 0));
    CHECK_AND_SET_SHARED_NOTEBOOK_ATTRIBUTE(hasReminderNotifyApp, recipientRemindrNotifyInApp,
                                            QString::number(sharedNotebook.reminderNotifyApp() ? 1 : 0));

#undef CHECK_AND_SET_SHARED_NOTEBOOK_ATTRIBUTE

    if (hasAnyProperty) {
        query.bindValue("columns", columns);
        query.bindValue("vals", values);
        bool res = query.exec();
        DATABASE_CHECK_AND_SET_ERROR("can't insert or replace shared notebook attributes "
                                     "into SQL database");
    }

    return true;
}

int LocalStorageManager::GetRowId(const QString & tableName, const QString & uniqueKeyName,
                                  const QVariant & uniqueKeyValue) const
{
    int rowId = -1;

    QSqlQuery query(m_sqlDatabase);
    query.prepare("SELECT rowid FROM ? WHERE ? = ?");
    query.addBindValue(tableName);
    query.addBindValue(uniqueKeyName);
    query.addBindValue(uniqueKeyValue);

    query.exec();

    while(query.next()) {
        rowId = query.value(0).toInt();
    }

    return rowId;
}

bool LocalStorageManager::InsertOrReplaceUser(const IUser & user, QString & errorDescription)
{
    errorDescription += QObject::tr("Can't insert or replace User into local storage database: ");

    QSqlQuery query(m_sqlDatabase);
    query.prepare("INSERT OR REPLACE INTO Users (id, username, name, timezone, privilege, "
                  "  creationTimestamp, modificationTimestamp, isDirty, isLocal, "
                  "  deletionTimestamp, isActive) VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?)");

    const evernote::edam::User & enUser = user.GetEnUser();

#define CHECK_AND_SET_USER_VALUE(column, error, prep) \
    if (!enUser.__isset.column) { \
        errorDescription += QObject::tr(error); \
        return false; \
    } \
    else { \
        query.addBindValue(prep(enUser.column)); \
    }

    CHECK_AND_SET_USER_VALUE(id, "User ID is not set", QString::number);
    CHECK_AND_SET_USER_VALUE(username, "Username is not set", QString::fromStdString);
    CHECK_AND_SET_USER_VALUE(name, "User's name is not set", QString::fromStdString);
    CHECK_AND_SET_USER_VALUE(timezone, "User's timezone is not set", QString::fromStdString);
    CHECK_AND_SET_USER_VALUE(created, "User's creation timestamp is not set", QString::number);
    CHECK_AND_SET_USER_VALUE(updated, "User's modification timestamp is not set", QString::number);

    query.addBindValue(QString::number((user.IsDirty() ? 1 : 0)));
    query.addBindValue(QString::number((user.IsLocal() ? 1 : 0)));

    // Process deletionTimestamp properties specifically
    if (enUser.__isset.deleted) {
        query.addBindValue(QString::number(enUser.deleted));
    }
    else {
        query.addBindValue(QString::number(0));
    }

    CHECK_AND_SET_USER_VALUE(active, "User's active field is not set (should be true)", static_cast<int>);

#undef CHECK_AND_SET_USER_VALUE

    bool res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("can't insert or replace user into \"Users\" table in SQL database");

    if (enUser.__isset.attributes)
    {
        query.clear();
        query.prepare("INSERT OR REPLACE INTO UserAttributes (id, data) VALUES(?, ?)");
        query.addBindValue(QString::number(enUser.id));
        QByteArray serializedUserAttributes = GetSerializedUserAttributes(enUser.attributes);
        query.addBindValue(serializedUserAttributes);

        res = query.exec();
        DATABASE_CHECK_AND_SET_ERROR("can't add user attributes into \"UserAttributes\" table in SQL database");
    }

    if (enUser.__isset.accounting)
    {
        query.clear();
        query.prepare("INSERT OR REPLACE INTO Accounting (id, data) VALUES(?, ?)");
        query.addBindValue(QString::number(enUser.id));
        QByteArray serializedAccounting = GetSerializedAccounting(enUser.accounting);
        query.addBindValue(serializedAccounting);

        res = query.exec();
        DATABASE_CHECK_AND_SET_ERROR("can't add user's accounting info into \"Accounting\" table in SQL database");
    }

    if (enUser.__isset.premiumInfo)
    {
        query.clear();
        query.prepare("INSERT OR REPLACE INTO PremiumInfo (id, data) VALUES(?, ?)");
        query.addBindValue(QString::number(enUser.id));
        QByteArray serializedPremiumInfo = GetSerializedPremiumInfo(enUser.premiumInfo);
        query.addBindValue(serializedPremiumInfo);

        res = query.exec();
        DATABASE_CHECK_AND_SET_ERROR("can't add user's premium info into \"PremiumInfo\" table in SQL database");
    }

    if (enUser.__isset.businessUserInfo)
    {
        query.clear();
        query.prepare("INSERT OR REPLACE INTO BusinessUserInfo (id, data) VALUES(?, ?)");
        query.addBindValue(QString::number(enUser.id));
        QByteArray serializedBusinessUserInfo = GetSerializedBusinessUserInfo(enUser.businessUserInfo);
        query.addBindValue(serializedBusinessUserInfo);

        res = query.exec();
        DATABASE_CHECK_AND_SET_ERROR("can't add user's business info into \"BusinessUserInfo\" table in SQL database");
    }

    return true;
}

bool LocalStorageManager::InsertOrReplaceNotebook(const Notebook & notebook,
                                                  QString & errorDescription)
{
    // NOTE: this method expects to be called after notebook is already checked
    // for sanity of its parameters!

    errorDescription = QObject::tr("Can't insert or replace notebook into local storage database: ");

    QSqlQuery query(m_sqlDatabase);
    query.prepare("INSERT OR REPLACE INTO Notebooks (guid, updateSequenceNumber, name, "
                  "creationTimestamp, modificationTimestamp, isDirty, isLocal, "
                  "isDefault, isLastUsed, isPublished, stack) VALUES(?, ?, ?, ?, "
                  "?, ?, ?, ?, ?, ?, ?)");
    query.addBindValue(notebook.guid());
    query.addBindValue(notebook.updateSequenceNumber());
    query.addBindValue(notebook.name());
    query.addBindValue(notebook.creationTimestamp());
    query.addBindValue(notebook.modificationTimestamp());
    query.addBindValue((notebook.isDirty() ? 1 : 0));
    query.addBindValue((notebook.isLocal() ? 1 : 0));
    query.addBindValue((notebook.isDefaultNotebook() ? 1 : 0));
    query.addBindValue((notebook.isLastUsed() ? 1 : 0));
    query.addBindValue((notebook.isPublished() ? 1 : 0));
    query.addBindValue(notebook.stack());

    bool res = query.exec();
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

    QSqlQuery query(m_sqlDatabase);
    query.prepare("INSERT OR REPLACE INTO LinkedNotebooks(:columns) VALUES(:values)");

    QString columns, values;
    bool hasAnyProperty = false;

#define CHECK_AND_INSERT_LINKED_NOTEBOOK_PROPERTY(property, checker) \
    if (linkedNotebook.checker()) \
    { \
        if (!columns.isEmpty()) { \
            columns.append(", "); \
        } \
        columns.append(#property); \
        \
        if (!values.isEmpty()) { \
            values.append(", "); \
        } \
        values.append(linkedNotebook.property()); \
    }

    CHECK_AND_INSERT_LINKED_NOTEBOOK_PROPERTY(guid, hasGuid);
    CHECK_AND_INSERT_LINKED_NOTEBOOK_PROPERTY(updateSequenceNumber, hasUpdateSequenceNumber);
    CHECK_AND_INSERT_LINKED_NOTEBOOK_PROPERTY(shareName, hasShareName);
    CHECK_AND_INSERT_LINKED_NOTEBOOK_PROPERTY(username, hasUsername);
    CHECK_AND_INSERT_LINKED_NOTEBOOK_PROPERTY(shardId, hasShardId);
    CHECK_AND_INSERT_LINKED_NOTEBOOK_PROPERTY(shareKey, hasShareKey);
    CHECK_AND_INSERT_LINKED_NOTEBOOK_PROPERTY(uri, hasUri);
    CHECK_AND_INSERT_LINKED_NOTEBOOK_PROPERTY(noteStoreUrl, hasNoteStoreUrl);
    CHECK_AND_INSERT_LINKED_NOTEBOOK_PROPERTY(webApiUrlPrefix, hasWebApiUrlPrefix);
    CHECK_AND_INSERT_LINKED_NOTEBOOK_PROPERTY(stack, hasStack);
    CHECK_AND_INSERT_LINKED_NOTEBOOK_PROPERTY(businessId, hasBusinessId);

#undef CHECK_AND_INSERT_LINKED_NOTEBOOK_PROPERTY

    if (hasAnyProperty)
    {
        columns.append(", isDirty");
        values.append(QString::number(linkedNotebook.isDirty() ? 1 : 0));

        query.bindValue("columns", columns);
        query.bindValue("values", values);

        bool res = query.exec();
        DATABASE_CHECK_AND_SET_ERROR("can't insert or replace notebook into \"LinkedNotebooks\" table in SQL database");
    }

    return true;
}

bool LocalStorageManager::InsertOrReplaceNote(const Note & note, QString & errorDescription)
{
    // NOTE: this method expects to be called after the note is already checked
    // for sanity of its parameters!

    errorDescription += QObject::tr("can't insert or replace note into local storage database: ");

    const QString guid = note.guid();
    const QString content = note.content();
    const QString title = note.title();
    const QString notebookGuid = note.notebookGuid();

    QSqlQuery query(m_sqlDatabase);

    query.prepare("INSERT OR REPLACE INTO Notes (guid, updateSequenceNumber, title, isDirty, "
                  "isLocal, content, creationTimestamp, modificationTimestamp, "
                  "notebookGuid) VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?)");
    query.addBindValue(guid);
    query.addBindValue(note.updateSequenceNumber());
    query.addBindValue(title);
    query.addBindValue((note.isDirty() ? 1 : 0));
    query.addBindValue((note.isLocal() ? 1 : 0));
    query.addBindValue(content);
    query.addBindValue(note.creationTimestamp());
    query.addBindValue(note.modificationTimestamp());
    query.addBindValue(notebookGuid);

    bool res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("can't insert or replace note into \"Notes\" table in SQL database");

    query.prepare("INSERT OR REPLACE INTO NoteText (guid, title, content, notebookGuid) "
                  "VALUES(?, ?, ?, ?)");
    query.addBindValue(guid);
    query.addBindValue(title);
    query.addBindValue(content);
    query.addBindValue(notebookGuid);
    res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("can't insert or replace note into \"NoteText\" table in SQL database");

    if (note.hasTagGuids())
    {
        QString error;

        std::vector<QString> tagGuids;
        note.tagGuids(tagGuids);

        for(const auto & tagGuid: tagGuids)
        {
            // NOTE: the behavior expressed here is valid since tags are synchronized before notes
            // so they must exist within local storage database; if they don't then something went really wrong

            Tag tag;
            bool res = FindTag(tagGuid, tag, error);
            if (!res) {
                errorDescription += QObject::tr("failed to find one of note tags by guid: ");
                errorDescription += error;
                return false;
            }

            query.prepare("INSERT OR REPLACE INTO NoteTags(note, tag) VALUES(?, ?)");
            query.addBindValue(guid);
            query.addBindValue(tagGuid);

            res = query.exec();
            DATABASE_CHECK_AND_SET_ERROR("can't insert or replace information into \"NoteTags\" table in SQL database");
        }
    }

    // NOTE: don't even attempt fo find tags by their names because evernote::edam::Note.tagNames
    // has the only purpose to provide tag names alternatively to guids to NoteStore::createNote method

    if (note.hasResources())
    {
        std::vector<ResourceAdapter> resources;
        note.resources(resources);
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
            res = InsertOrReplaceResource(resource, error);
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

    QString guid = tag.guid();
    QString name = tag.name();
    QString nameUpper = name.toUpper();

    QString parentGuid = tag.parentGuid();

    QSqlQuery query(m_sqlDatabase);

    query.prepare("INSERT OR REPLACE INTO Tags (guid, updateSequenceNumber, name, "
                  "nameUpper, parentGuid, isDirty, isLocal, isDeleted) "
                  "VALUES(?, ?, ?, ?, ?, ?, ?, ?");
    query.addBindValue(guid);
    query.addBindValue(tag.updateSequenceNumber());
    query.addBindValue(name);
    query.addBindValue(nameUpper);
    query.addBindValue(parentGuid);
    query.addBindValue(tag.isDirty() ? 1 : 0);
    query.addBindValue(tag.isLocal() ? 1 : 0);
    query.addBindValue(tag.isDeleted() ? 1 : 0);

    bool res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("can't insert or replace tag into \"Tags\" table in SQL database");

    return true;
}

bool LocalStorageManager::InsertOrReplaceResource(const IResource & resource,
                                                  QString & errorDescription)
{
    // NOTE: this method expects to be called after resource is already checked
    // for sanity of its parameters!

    errorDescription = QObject::tr("Can't insert or replace resource into local storage database: ");

    QSqlQuery query(m_sqlDatabase);
    query.prepare("INSERT OR REPLACE INTO Resources (:columns) VALUES(:vals)");

    QString columns, values;

#define CHECK_AND_INSERT_RESOURCE_PROPERTY(property, checker) \
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
        values.append(resource.property()); \
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
        CHECK_AND_INSERT_RESOURCE_PROPERTY(dataSize, hasDataSize);

        const auto & dataHash = (hasData ? resource.dataHash() : resource.alternateDataHash());
        CHECK_AND_INSERT_RESOURCE_PROPERTY(dataHash, hasDataHash);
    }

    CHECK_AND_INSERT_RESOURCE_PROPERTY(mime, hasMime);
    CHECK_AND_INSERT_RESOURCE_PROPERTY(width, hasWidth);
    CHECK_AND_INSERT_RESOURCE_PROPERTY(height, hasHeight);

    if (resource.hasRecognitionData()) {
        CHECK_AND_INSERT_RESOURCE_PROPERTY(recognitionDataBody, hasRecognitionDataBody);
        CHECK_AND_INSERT_RESOURCE_PROPERTY(recognitionDataSize, hasRecognitionDataSize);
        CHECK_AND_INSERT_RESOURCE_PROPERTY(recognitionDataHash, hasRecognitionDataHash);
    }

    CHECK_AND_INSERT_RESOURCE_PROPERTY(updateSequenceNumber, hasUpdateSequenceNumber);

#undef CHECK_AND_INSERT_RESOURCE_PROPERTY

    if (!columns.isEmpty()) {
        columns.append(", ");
    }
    columns.append("isDirty");

    if (!values.isEmpty()) {
        values.append(", ");
    }
    values.append(QString::number(resource.isDirty() ? 1 : 0));

    query.bindValue("columns", columns);
    query.bindValue("vals", values);

    bool res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("can't insert or replace data into \"Resources\" table in SQL database");

    if (resource.hasResourceAttributes())
    {
        query.prepare("INSERT OR REPLACE INTO ResourceAttributes (guid, data) VALUES(?, ?)");
        query.addBindValue(resource.guid());
        query.addBindValue(resource.resourceAttributes());

        res = query.exec();
        DATABASE_CHECK_AND_SET_ERROR("can't insert or replace data into \"ResourceAttributes\" table in SQL database");
    }

    return true;
}

bool LocalStorageManager::InsertOrReplaceSavedSearch(const SavedSearch & search,
                                                     QString & errorDescription)
{
    // NOTE: this method expects to be called after the search is already checked
    // for sanity of its parameters!

    errorDescription = QObject::tr("Can't insert or replace saved search into local storage database: ");

    QSqlQuery query(m_sqlDatabase);
    query.prepare("INSERT OR REPLACE INTO SavedSearches (guid, name, query, format, "
                  "updateSequenceNumber, includeAccount, includePersonalLinkedNotebooks, "
                  "includeBusinessLinkedNotebooks) VALUES(:guid, :name, :query, "
                  ":format, :updateSequenceNumber, :includeAccount, :includePersonalLinkedNotebooks, "
                  ":includeBusinessLinkedNotebooks)");
    query.bindValue("guid", search.guid());
    query.bindValue("name", search.name());
    query.bindValue("query", search.query());
    query.bindValue("format", search.queryFormat());
    query.bindValue("updateSequenceNumber", search.updateSequenceNumber());
    query.bindValue("includeAccount", (search.includeAccount() ? 1 : 0));
    query.bindValue("includePersonalLinkedNotebooks", (search.includePersonalLinkedNotebooks() ? 1 : 0));
    query.bindValue("includeBusinessLinkedNotebooks", (search.includeBusinessLinkedNotebooks() ? 1 : 0));

    bool res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("can't insert or replace saved search into \"SavedSearches\" table in SQL database");

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
        CHECK_AND_SET_NOTEBOOK_ATTRIBUTE(publishingOrder, setPublishingOrder,
                                         int, qint8, isRequired);
        CHECK_AND_SET_NOTEBOOK_ATTRIBUTE(publishingAscending, setPublishingAscending,
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

    if (record.contains("contactId")) {
        UserAdapter user = notebook.contact();
        auto & enUser = user.GetEnUser();
        enUser.id = qvariant_cast<int32_t>(record.value("contactId"));
        enUser.__isset.id = true;
        notebook.setContact(user);

        if (notebook.hasContact())
        {
            QString error;
            bool res = FindUser(enUser.id, user, error);
            if (!res) {
                errorDescription += error;
                return false;
            }
        }
    }
    else {
        errorDescription += QObject::tr("Internal error: No contactId field "
                                        "in the result of SQL query");
        return false;
    }

    QSqlQuery query(m_sqlDatabase);
    query.prepare("SELECT noReadNotes, noCreateNotes, noUpdateNotes, noExpungeNotes, "
                  "noShareNotes, noEmailNotes, noSendMessageToRecipients, noUpdateNotebook, "
                  "noExpungeNotebook, noSetDefaultNotebook, noSetNotebookStack, noPublishToPublic, "
                  "noPublishToBusinessLibrary, noCreateTags, noUpdateTags, noExpungeTags, "
                  "noSetParentTag, noCreateSharedNotebooks, updateWhichSharedNotebookRestrictions, "
                  "expungeWhichSharedNotebookRestrictions FROM NotebookRestrictions "
                  "WHERE guid=?");
    query.addBindValue(notebook.guid());

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
    std::vector<SharedNotebookAdapter> sharedNotebooks;
    notebook.sharedNotebooks(sharedNotebooks);

    res = ListSharedNotebooksPerNotebookGuid(notebook.guid(), sharedNotebooks, error);
    if (!res) {
        errorDescription += error;
        return false;
    }

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

#undef CHECK_AND_SET_SHARED_NOTEBOOK_PROPERTY

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
        search.setter(static_cast<localType>(qvariant_cast<type>(rec.value(#property)))); \
    } \
    else if (isRequired) { \
        errorDescription += QObject::tr("no " #property " field in the result of SQL query"); \
        return false; \
    }

    bool isRequired = true;
    CHECK_AND_SET_SAVED_SEARCH_PROPERTY(guid, QString, QString, setGuid, isRequired);
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

bool LocalStorageManager::FillTagsFromSqlQuery(QSqlQuery & query, std::vector<Tag> & tags,
                                               QString & errorDescription) const
{
    tags.clear();

    size_t numRows = query.size();
    if (numRows == 0) {
        QNDEBUG("No tags were found in SQL query");
        return true;
    }

    tags.reserve(numRows);

    while(query.next())
    {
        QString tagGuid = query.value(0).toString();
        if (!CheckGuid(tagGuid)) {
            errorDescription += QObject::tr("Internal error: found invalid tag guid in \"NoteTags\" table");
            return false;
        }

        tags.push_back(Tag());

        QString error;
        bool res = FindTag(tagGuid, tags.back(), error);
        if (!res) {
            errorDescription += error;
            return false;
        }
    }

    return true;
}

bool LocalStorageManager::FindAndSetTagGuidsPerNote(Note & note, QString & errorDescription) const
{
    errorDescription += QObject::tr("can't find tag guids per note: ");

    if (!CheckGuid(note.guid())) {
        errorDescription += QObject::tr("note's guid is invalid");
        return false;
    }

    QSqlQuery query(m_sqlDatabase);
    query.prepare("SELECT tag FROM NoteTags WHERE note = ?");
    query.addBindValue(note.guid());

    bool res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("can't select note tags from \"NoteTags\" table in SQL database");

    while (query.next())
    {
        QString tagGuid = query.value(0).toString();
        if (!CheckGuid(tagGuid)) {
            errorDescription += QObject::tr("found invalid tag guid for requested note");
            return false;
        }

        note.addTagGuid(tagGuid);
    }

    return true;
}

bool LocalStorageManager::FindAndSetResourcesPerNote(Note & note, QString & errorDescription,
                                                     const bool withBinaryData) const
{
    errorDescription += QObject::tr("can't find resources for note: ");

    const QString guid = note.guid();
    if (!CheckGuid(guid)) {
        errorDescription += QObject::tr("note's guid is invalid");
        return false;
    }

    QSqlQuery query(m_sqlDatabase);
    query.prepare("SELECT :columns FROM Resources WHERE noteGuid = :noteGuid");

    QString columns = "guid, updateSequenceNumber, dataSize, dataHash, mime, width, height, "
                      "recognitionBody, recognitionSize, recognitionHash";
    if (withBinaryData) {
        columns.append(", dataBody");
    }

    query.bindValue("columns", columns);
    query.bindValue("noteGuid", guid);

    bool res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("can't select resources' guids per note's guid");

    int numResources = query.size();
    if (numResources < 0) {
        // No resources were found
        return true;
    }

    size_t k = 0;
    while(query.next())
    {
        QSqlRecord rec = query.record();

        ResourceWrapper resource;

        CHECK_AND_SET_RESOURCE_PROPERTY(guid, QString, QString, setGuid, /* is required = */ true);
        CHECK_AND_SET_RESOURCE_PROPERTY(updateSequenceNumber, int, qint32, setUpdateSequenceNumber,
                                        /* is required = */ true);
        CHECK_AND_SET_RESOURCE_PROPERTY(dataSize, int, qint32, setDataSize, /* is required = */ true);
        CHECK_AND_SET_RESOURCE_PROPERTY(dataHash, QString, QString, setDataHash, /* is required = */ true);

        if (withBinaryData) {
            CHECK_AND_SET_RESOURCE_PROPERTY(dataBody, QString, QString, setDataBody,
                                            /* is required = */ true);
        }

        CHECK_AND_SET_RESOURCE_PROPERTY(mime, QString, QString, setMime, /* is required = */ true);
        CHECK_AND_SET_RESOURCE_PROPERTY(width, int, qint32, setWidth, /* is required = */ true);
        CHECK_AND_SET_RESOURCE_PROPERTY(height, int, qint32, setHeight, /* is required = */ true);

        CHECK_AND_SET_RESOURCE_PROPERTY(recognitionDataBody, QString, QString,
                                        setRecognitionDataBody, /* is required = */ false);
        CHECK_AND_SET_RESOURCE_PROPERTY(recognitionDataSize, int, qint32,
                                        setRecognitionDataSize, /* is required = */ false);
        CHECK_AND_SET_RESOURCE_PROPERTY(recognitionDataHash, QString, QString,
                                        setRecognitionDataHash, /* is required = */ false);

        // Retrieve optional resource attributes for each resource
        QSqlQuery resourceAttributeQuery(m_sqlDatabase);
        resourceAttributeQuery.prepare("SELECT data FROM ResourceAttributes WHERE guid = ?");
        resourceAttributeQuery.addBindValue(guid);

        res = resourceAttributeQuery.exec();
        DATABASE_CHECK_AND_SET_ERROR("can't select serialized resource attributes from "
                                     "\"ResourceAttributes\" table in SQL database");

        if (!resourceAttributeQuery.next()) {
            // No attributes for this resource
            continue;
        }

        resource.setResourceAttributes(query.value(0).toByteArray());

        note.addResource(resource);
    }

    return true;
}

bool LocalStorageManager::FindAndSetNoteAttributesPerNote(Note & note, QString & errorDescription) const
{
    errorDescription += QObject::tr("can't find note attributes: ");

    if (!note.hasNoteAttributes()) {
        return true;
    }

    const QString guid = note.guid();
    if (!CheckGuid(guid)) {
        errorDescription += QObject::tr("note's guid is invalid");
        return false;
    }

    QSqlQuery query(m_sqlDatabase);
    query.prepare("SELECT data FROM NoteAttributes WHERE noteGuid=?");
    query.addBindValue(guid);

    bool res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("can't select note's attributes from \"NoteAttributes\" table in SQL database");

    if (query.next()) {
        note.setNoteAttributes(query.value(0).toByteArray());
    }
    else {
        errorDescription += QObject::tr("Internal error: SQL query result is empty");
        return false;
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
