#include "LocalStorageManager.h"
#include "DatabaseOpeningException.h"
#include "DatabaseSqlErrorException.h"
#include "../EnWrappers.h"
#include "../../tools/QuteNoteNullPtrException.h"
#include <Limits_constants.h>

using namespace evernote::edam;

#ifdef USE_QT5
#include <QStandardPaths>
#else
#include <QDesktopServices>
#endif

namespace qute_note {

#define QUTE_NOTE_DATABASE_NAME "qn.storage.sqlite"

LocalStorageManager::LocalStorageManager(const QString & username, const UserID userId,
                                         const QString & authenticationToken,
                                         QSharedPointer<evernote::edam::NoteStoreClient> & pNoteStore) :
    m_authenticationToken(authenticationToken),
    m_pNoteStore(pNoteStore),
    // NOTE: don't initialize these! Otherwise SwitchUser won't work right
    m_currentUsername(),
    m_currentUserId(),
    m_applicationPersistenceStoragePath()
{
    if (m_pNoteStore == nullptr) {
        throw QuteNoteNullPtrException("Found null pointer to NoteStoreClient while "
                                       "trying to create LocalStorageManager instance");
    }

    SwitchUser(username, userId);
}

LocalStorageManager::~LocalStorageManager()
{
    if (m_sqlDatabase.open()) {
        m_sqlDatabase.close();
    }
}

void LocalStorageManager::SetNewAuthenticationToken(const QString & authenticationToken)
{
    m_authenticationToken = authenticationToken;
}

#define DATABASE_CHECK_AND_SET_ERROR(errorPrefix) \
    if (!res) { \
        errorDescription += QObject::tr("Internal error: "); \
        errorDescription += QObject::tr(errorPrefix); \
        errorDescription += ": "; \
        errorDescription.append(m_sqlDatabase.lastError().text()); \
        return false; \
    }

bool LocalStorageManager::AddUser(const User & user, QString & errorDescription)
{
    errorDescription = QObject::tr("Can't add user into local storage database: ");

    int rowId = GetRowId("Users", "id", QVariant(QString::number(user.en_user.id)));
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

bool LocalStorageManager::UpdateUser(const User & user, QString & errorDescription)
{
    errorDescription = QObject::tr("Can't update user in local storage database: ");

    int rowId = GetRowId("Users", "id", QVariant(QString::number(user.en_user.id)));
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

bool LocalStorageManager::FindUser(const UserID id, User & user, QString & errorDescription) const
{
    errorDescription = QObject::tr("Can't find user in local storage database: ");

    QString idStr = QString::number(id);
    int rowId = GetRowId("Users", "id", QVariant(idStr));
    if (rowId < 0) {
        errorDescription += QObject::tr("user id was not found");
        return false;
    }

    user = User();

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

#define CHECK_AND_SET_USER_PROPERTY(property) \
    if (rec.contains(#property)) { \
        user.property = (qvariant_cast<int>(rec.value(#property)) != 0); \
    } \
    else { \
        errorDescription += QObject::tr("Internal error: no " #property \
                                        " field in the result of SQL query "); \
        return false; \
    }

    CHECK_AND_SET_USER_PROPERTY(isDirty);
    CHECK_AND_SET_USER_PROPERTY(isLocal);

#undef CHECK_AND_SET_USER_PROPERTY

    evernote::edam::User & enUser = user.en_user;
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
    CHECK_AND_SET_EN_USER_PROPERTY(enUser, isActive, active, int, bool, isRequired);   // NOTE: cast int to bool

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

bool LocalStorageManager::DeleteUser(const User & user, QString & errorDescription)
{
    if (user.isLocal) {
        return ExpungeUser(user, errorDescription);
    }

    errorDescription = QObject::tr("Can't delete user from local storage database: ");

    if (!user.en_user.deleted) {
        errorDescription += QObject::tr("deletion timestamp is not set");
        return false;
    }

    if (!user.en_user.__isset.id) {
        errorDescription += QObject::tr("user id is not set");
        return false;
    }

    QSqlQuery query(m_sqlDatabase);
    query.prepare("UPDATE Users SET deletionTimestamp = ? WHERE id = ?");
    query.addBindValue(static_cast<qint64>(user.en_user.deleted));
    query.addBindValue(QString::number(user.en_user.id));

    bool res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("can't update deletion timestamp in \"Users\" table in SQL database");

    return true;
}

bool LocalStorageManager::ExpungeUser(const User & user, QString & errorDescription)
{
    errorDescription = QObject::tr("Can't expunge user from local storage database: ");

    if (!user.isLocal) {
        errorDescription += QObject::tr("user is not local, expunge is disallowed");
        return false;
    }

    if (!user.en_user.__isset.id) {
        errorDescription += QObject::tr("user id is not set");
        return false;
    }

    QSqlQuery query(m_sqlDatabase);
    query.prepare("DELETE FROM Users WHERE id = ?");
    query.addBindValue(QString::number(user.en_user.id));

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

#if QT_VERSION >= 0x050000
    m_applicationPersistenceStoragePath = QStandardPaths::writableLocation(QStandardPaths::DataLocation);
#else
    m_applicationPersistenceStoragePath = QDesktopServices::storageLocation(QDesktopServices::DataLocation);
#endif

    m_applicationPersistenceStoragePath.append("/" + m_currentUsername + m_currentUserId);

    bool needsInitializing = false;

    QDir databaseFolder(m_applicationPersistenceStoragePath);
    if (!databaseFolder.exists())
    {
        needsInitializing = true;
        if (!databaseFolder.mkpath(m_applicationPersistenceStoragePath)) {
            throw DatabaseOpeningException(QObject::tr("Cannot create folder "
                                                       "to store local storage  database."));
        }
    }

    m_sqlDatabase.addDatabase("QSQLITE");
    QString databaseFileName = m_applicationPersistenceStoragePath + QString("/") +
                               QString(QUTE_NOTE_DATABASE_NAME);
    QFile databaseFile(databaseFileName);

    if (!databaseFile.exists())
    {
        needsInitializing = true;
        if (!databaseFile.open(QIODevice::ReadWrite)) {
            throw DatabaseOpeningException(QObject::tr("Cannot create local storage database: ") +
                                           databaseFile.errorString());
        }
    }

    m_sqlDatabase.setDatabaseName(databaseFileName);
    if (!m_sqlDatabase.open())
    {
        QString lastErrorText = m_sqlDatabase.lastError().text();
        throw DatabaseOpeningException(QObject::tr("Cannot open local storage database: ") +
                                       lastErrorText);
    }

    QSqlQuery query(m_sqlDatabase);
    if (!query.exec("PRAGMA foreign_keys = ON")) {
        throw DatabaseSqlErrorException(QObject::tr("Cannot set foreign_keys = ON pragma "
                                                    "for SQL local storage database"));
    }

    if (needsInitializing)
    {
        QString errorDescription;
        if (!CreateTables(errorDescription)) {
            throw DatabaseSqlErrorException(QObject::tr("Cannot initialize tables in SQL database: ") +
                                            errorDescription);
        }
    }
}

bool LocalStorageManager::AddNotebook(const Notebook & notebook, QString & errorDescription)
{
    errorDescription = QObject::tr("Can't add notebook to local storage database: ");
    QString error;

    bool res = notebook.CheckParameters(error);
    if (!res) {
        errorDescription += error;
        return false;
    }

    int rowId = GetRowId("Notebooks", "guid", QVariant(QString::fromStdString(notebook.en_notebook.guid)));
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

    bool res = notebook.CheckParameters(error);
    if (!res) {
        errorDescription += error;
        return false;
    }

    int rowId = GetRowId("Notebooks", "guid", QVariant(QString::fromStdString(notebook.en_notebook.guid)));
    if (rowId < 0) {
        errorDescription += QObject::tr("notebook was not found by guid");
        return false;
    }

    return InsertOrReplaceNotebook(notebook, errorDescription);
}

bool LocalStorageManager::FindNotebook(const Guid & notebookGuid, Notebook & notebook,
                                       QString & errorDescription)
{
    errorDescription = QObject::tr("Can't find notebook in local storage database: ");

    if (!CheckGuid(notebookGuid)) {
        errorDescription += QObject::tr("requested guid is invalid");
        return false;
    }

    notebook = Notebook();

    QSqlQuery query(m_sqlDatabase);
    query.prepare("SELECT updateSequenceNumber, name, creationTimestamp, "
                  "modificationTimestamp, isDirty, isLocal, isDefault, isLastUsed, "
                  "publishingUri, publishingNoteSortOrder, publishingAscendingOrder, "
                  "publicDescription, isPublished, stack, businessNotebookDescription, "
                  "businessNotebookPrivilegeLevel, businessNotebookIsRecommended, "
                  "contactId FROM Notebooks WHERE guid=?");
    query.addBindValue(QString::fromStdString(notebookGuid));
    bool res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("can't find notebook in SQL database by guid");

    if (!query.next()) {
        errorDescription += QObject::tr("Internal error: SQL query result is empty");
        return false;
    }

    QSqlRecord rec = query.record();

#define CHECK_AND_SET_NOTEBOOK_ATTRIBUTE(attribute) \
    if (rec.contains(#attribute)) { \
        notebook.attribute = (qvariant_cast<int>(rec.value(#attribute)) != 0); \
    } \
    else { \
        errorDescription += QObject::tr("Internal error: No " #attribute " field " \
                                        "in the result of SQL query"); \
        return false; \
    }

    CHECK_AND_SET_NOTEBOOK_ATTRIBUTE(isDirty);
    CHECK_AND_SET_NOTEBOOK_ATTRIBUTE(isLocal);
    CHECK_AND_SET_NOTEBOOK_ATTRIBUTE(isLastUsed);

#undef CHECK_AND_SET_NOTEBOOK_ATTRIBUTE

    evernote::edam::Notebook & enNotebook = notebook.en_notebook;
    enNotebook.guid = notebookGuid;
    enNotebook.__isset.guid = true;

#define CHECK_AND_SET_EN_NOTEBOOK_ATTRIBUTE(holder, attributeLocalName, attributeEnName, \
                                            type, true_type, is_required, ...) \
    if (rec.contains(#attributeLocalName)) { \
        holder.attributeEnName = static_cast<true_type>((qvariant_cast<type>(rec.value(#attributeLocalName)))__VA_ARGS__); \
        holder.__isset.attributeEnName = true; \
    } \
    else if (is_required) { \
        errorDescription += QObject::tr("Internal error: no " #attributeLocalName " field " \
                                        "in the result of SQL query"); \
        return false; \
    }

    bool isRequired = true;
    CHECK_AND_SET_EN_NOTEBOOK_ATTRIBUTE(enNotebook, updateSequenceNumber, updateSequenceNum,
                                        int, int32_t, isRequired);
    CHECK_AND_SET_EN_NOTEBOOK_ATTRIBUTE(enNotebook, name, name, QString, std::string,
                                        isRequired, .toStdString());
    CHECK_AND_SET_EN_NOTEBOOK_ATTRIBUTE(enNotebook, isDefault, defaultNotebook,
                                        int, bool, isRequired);   // NOTE: int to bool cast
    CHECK_AND_SET_EN_NOTEBOOK_ATTRIBUTE(enNotebook, creationTimestamp, serviceCreated,
                                        int, Timestamp, isRequired);
    CHECK_AND_SET_EN_NOTEBOOK_ATTRIBUTE(enNotebook, modificationTimestamp, serviceUpdated,
                                        int, Timestamp, isRequired);
    CHECK_AND_SET_EN_NOTEBOOK_ATTRIBUTE(enNotebook, isPublished, published, int,
                                        bool, isRequired);   // NOTE: int to bool cast

    CHECK_AND_SET_EN_NOTEBOOK_ATTRIBUTE(enNotebook, stack, stack, QString, std::string,
                                        isRequired, .toStdString());
    if (enNotebook.stack.empty()) {
        enNotebook.__isset.stack = false;
    }

    if (enNotebook.published) {
        CHECK_AND_SET_EN_NOTEBOOK_ATTRIBUTE(enNotebook.publishing, publishingUri, uri,
                                            QString, std::string, isRequired, .toStdString());
        CHECK_AND_SET_EN_NOTEBOOK_ATTRIBUTE(enNotebook.publishing, publishingNoteSortOrder,
                                            order, int, NoteSortOrder::type, isRequired);
    }

    CHECK_AND_SET_EN_NOTEBOOK_ATTRIBUTE(enNotebook.businessNotebook, businessNotebookDescription,
                                        notebookDescription, QString, std::string,
                                        isRequired, .toStdString());
    CHECK_AND_SET_EN_NOTEBOOK_ATTRIBUTE(enNotebook.businessNotebook, businessNotebookPrivilegeLevel,
                                        privilege, int, SharedNotebookPrivilegeLevel::type, isRequired);
    CHECK_AND_SET_EN_NOTEBOOK_ATTRIBUTE(enNotebook.businessNotebook, businessNotebookIsRecommended,
                                        recommended, int, bool, isRequired);  // NOTE: int to bool cast

    BusinessNotebook & businessNotebook = enNotebook.businessNotebook;
    if (businessNotebook.notebookDescription.empty() &&
        (businessNotebook.privilege == 0) &&
        !businessNotebook.recommended)
    {
        businessNotebook.__isset.notebookDescription = false;
        businessNotebook.__isset.privilege = false;
        businessNotebook.__isset.recommended = false;
        enNotebook.__isset.businessNotebook = false;
    }

    if (rec.contains("contactId")) {
        enNotebook.contact.id = qvariant_cast<int32_t>(rec.value("contactId"));
        enNotebook.contact.__isset.id = true;
        enNotebook.__isset.contact = true;
    }
    else {
        errorDescription += QObject::tr("Internal error: No contactId field "
                                        "in the result of SQL query");
        return false;
    }

    if (enNotebook.contact.id == 0) {
        enNotebook.__isset.contact = false;
        enNotebook.contact.__isset.id = false;
    }
    else {
        User user;
        QString error;
        res = FindUser(enNotebook.contact.id, user, error);
        if (!res) {
            errorDescription += error;
            return false;
        }

        enNotebook.contact = user.en_user;
    }

    query.clear();
    query.prepare("SELECT noReadNotes, noCreateNotes, noUpdateNotes, noExpungeNotes, "
                  "noShareNotes, noEmailNotes, noSendMessageToRecipients, noUpdateNotebook, "
                  "noExpungeNotebook, noSetDefaultNotebook, noSetNotebookStack, noPublishToPublic, "
                  "noPublishToBusinessLibrary, noCreateTags, noUpdateTags, noExpungeTags, "
                  "noSetParentTag, noCreateSharedNotebooks, updateWhichSharedNotebookRestrictions, "
                  "expungeWhichSharedNotebookRestrictions FROM NotebookRestrictions "
                  "WHERE guid=?");
    query.addBindValue(QString::fromStdString(notebookGuid));

    res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("can't find notebook restrictions for specified "
                                 "notebook guid from SQL database");

    if (query.next())
    {
        QSqlRecord rec = query.record();

        enNotebook.__isset.restrictions = true;
        NotebookRestrictions & restrictions = enNotebook.restrictions;

        isRequired = false;
#define SET_EN_NOTEBOOK_RESTRICTION(notebook_restriction) \
    CHECK_AND_SET_EN_NOTEBOOK_ATTRIBUTE(restrictions, notebook_restriction, \
                                        notebook_restriction, int, bool, isRequired);

        SET_EN_NOTEBOOK_RESTRICTION(noReadNotes);
        SET_EN_NOTEBOOK_RESTRICTION(noCreateNotes);
        SET_EN_NOTEBOOK_RESTRICTION(noUpdateNotes);
        SET_EN_NOTEBOOK_RESTRICTION(noExpungeNotes);
        SET_EN_NOTEBOOK_RESTRICTION(noShareNotes);
        SET_EN_NOTEBOOK_RESTRICTION(noEmailNotes);
        SET_EN_NOTEBOOK_RESTRICTION(noSendMessageToRecipients);
        SET_EN_NOTEBOOK_RESTRICTION(noUpdateNotebook);
        SET_EN_NOTEBOOK_RESTRICTION(noExpungeNotebook);
        SET_EN_NOTEBOOK_RESTRICTION(noSetDefaultNotebook);
        SET_EN_NOTEBOOK_RESTRICTION(noSetNotebookStack);
        SET_EN_NOTEBOOK_RESTRICTION(noPublishToPublic);
        SET_EN_NOTEBOOK_RESTRICTION(noPublishToBusinessLibrary);
        SET_EN_NOTEBOOK_RESTRICTION(noCreateTags);
        SET_EN_NOTEBOOK_RESTRICTION(noUpdateTags);
        SET_EN_NOTEBOOK_RESTRICTION(noExpungeTags);
        SET_EN_NOTEBOOK_RESTRICTION(noSetParentTag);
        SET_EN_NOTEBOOK_RESTRICTION(noCreateSharedNotebooks);

#undef SET_EN_NOTEBOOK_RESTRICTION

        CHECK_AND_SET_EN_NOTEBOOK_ATTRIBUTE(restrictions, updateWhichSharedNotebookRestrictions,
                                            updateWhichSharedNotebookRestrictions,
                                            int, SharedNotebookInstanceRestrictions::type,
                                            isRequired);

        CHECK_AND_SET_EN_NOTEBOOK_ATTRIBUTE(restrictions, expungeWhichSharedNotebookRestrictions,
                                            expungeWhichSharedNotebookRestrictions,
                                            int, SharedNotebookInstanceRestrictions::type,
                                            isRequired);

    }
    else {
        // No restrictions are set
        enNotebook.__isset.restrictions = false;
    }

    query.clear();
    query.prepare("SELECT shareId, userId, email, creationTimestamp, modificationTimestamp, "
                  "shareKey, username, sharedNotebookPrivilegeLevel, allowPreview, "
                  "recipientReminderNotifyEmail, recipientReminderNotifyInApp "
                  "FROM SharedNotebooks WHERE notebookGuid=?");
    query.addBindValue(QString::fromStdString(notebookGuid));

    res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("can't select shared notebooks for given "
                                 "notebook guid from SQL database");

    std::vector<SharedNotebook> sharedNotebooks;
    sharedNotebooks.reserve(std::max(query.size(), 0));

    while(query.next())
    {
        QSqlRecord rec = query.record();

        SharedNotebook sharedNotebook;
        isRequired = true;

        CHECK_AND_SET_EN_NOTEBOOK_ATTRIBUTE(sharedNotebook, shareId, id, int, int64_t, isRequired);
        CHECK_AND_SET_EN_NOTEBOOK_ATTRIBUTE(sharedNotebook, userId, userId, int, int32_t, isRequired);
        CHECK_AND_SET_EN_NOTEBOOK_ATTRIBUTE(sharedNotebook, email, email, QString,
                                            std::string, isRequired, .toStdString());
        CHECK_AND_SET_EN_NOTEBOOK_ATTRIBUTE(sharedNotebook, creationTimestamp,
                                            serviceCreated, int, Timestamp, isRequired);
        CHECK_AND_SET_EN_NOTEBOOK_ATTRIBUTE(sharedNotebook, modificationTimestamp,
                                            serviceUpdated, int, Timestamp, isRequired);
        CHECK_AND_SET_EN_NOTEBOOK_ATTRIBUTE(sharedNotebook, shareKey, shareKey,
                                            QString, std::string, isRequired, .toStdString());
        CHECK_AND_SET_EN_NOTEBOOK_ATTRIBUTE(sharedNotebook, username, username,
                                            QString, std::string, isRequired, .toStdString());
        CHECK_AND_SET_EN_NOTEBOOK_ATTRIBUTE(sharedNotebook, sharedNotebookPrivilegeLevel,
                                            privilege, int, SharedNotebookPrivilegeLevel::type, isRequired);
        CHECK_AND_SET_EN_NOTEBOOK_ATTRIBUTE(sharedNotebook, allowPreview, allowPreview,
                                            int, bool, isRequired);   // NOTE: int to bool cast
        CHECK_AND_SET_EN_NOTEBOOK_ATTRIBUTE(sharedNotebook.recipientSettings,
                                            recipientReminderNotifyEmail,
                                            reminderNotifyEmail, int, bool, isRequired);  // NOTE: int to bool cast
        CHECK_AND_SET_EN_NOTEBOOK_ATTRIBUTE(sharedNotebook.recipientSettings,
                                            recipientReminderNotifyInApp,
                                            reminderNotifyInApp, int, bool, isRequired);  // NOTE: int to bool cast

#undef CHECK_AND_SET_EN_NOTEBOOK_ATTRIBUTE

        sharedNotebooks.push_back(sharedNotebook);
    }

    enNotebook.sharedNotebooks = sharedNotebooks;
    return true;
}

bool LocalStorageManager::ExpungeNotebook(const Notebook & notebook, QString & errorDescription)
{
    errorDescription = QObject::tr("Can't expunge notebook from local storage database: ");

    const evernote::edam::Notebook & enNotebook = notebook.en_notebook;

    if (!CheckGuid(enNotebook.guid)) {
        errorDescription += QObject::tr("notebook's guid is invalid");
        return false;
    }

    QSqlQuery query(m_sqlDatabase);
    query.prepare("SELECT rowid FROM Notebooks WHERE guid=?");
    query.addBindValue(QString::fromStdString(notebook.en_notebook.guid));
    bool res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("can't find notebook to be expunged by guid in SQL database");

    int rowId = -1;
    bool conversionResult = false;
    while(query.next()) {
        rowId = query.value(0).toInt(&conversionResult);
    }

    if (!conversionResult || (rowId < 0)) {
        errorDescription += QObject::tr("can't determine row id of notebook "
                                        "to be expunged in \"Notebooks\" table in SQL database");
        return false;
    }

    query.clear();
    query.prepare("DELETE FROM Notebooks WHERE rowid=?");
    query.addBindValue(QVariant(rowId));

    res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("can't delete entry from \"Notebooks\" table in SQL database");

    return true;
}

bool LocalStorageManager::AddNote(const Note & note, QString & errorDescription)
{
    errorDescription = QObject::tr("Can't add note to local storage database: ");
    QString error;

    bool res = note.CheckParameters(error);
    if (!res) {
        errorDescription += error;
        return false;
    }

    int rowId = GetRowId("Notes", "guid", QVariant(QString::fromStdString(note.en_note.guid)));
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

    bool res = note.CheckParameters(error);
    if (!res) {
        errorDescription += error;
        return false;
    }

    int rowId = GetRowId("Notes", "guid", QVariant(QString::fromStdString(note.en_note.guid)));
    if (rowId < 0) {
        errorDescription += QObject::tr("note with specified guid was not found");
        return false;
    }

    return InsertOrReplaceNote(note, errorDescription);
}

bool LocalStorageManager::FindNote(const Guid & noteGuid, Note & note, QString & errorDescription) const
{
    errorDescription = QObject::tr("Can't find note in local storage database: ");

    if (!CheckGuid(noteGuid)) {
        errorDescription += QObject::tr("requested note guid is invalid");
        return false;
    }

    note = Note();

    QSqlQuery query(m_sqlDatabase);
    query.prepare("SELECT updateSequenceNumber, title, isDirty, isLocal, content, "
                  "creationTimestamp, modificationTimestamp, isDeleted, deletionTimestap, "
                  "notebook FROM Notes WHERE guid=?");
    query.addBindValue(QString::fromStdString(noteGuid));

    bool res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("can't select note from \"Notes\" table in SQL database");

    if (!query.next()) {
        errorDescription += QObject::tr("Internal error: SQL query result is empty");
        return false;
    }

    QSqlRecord rec = query.record();

#define CHECK_AND_SET_NOTE_PROPERTY(property) \
    if (rec.contains(#property)) { \
        note.property = (qvariant_cast<int>(rec.value(#property)) != 0); \
    } \
    else { \
        errorDescription += QObject::tr("Internal error: no " #property " field " \
                                        "in the result of SQL query"); \
        return false; \
    }

    CHECK_AND_SET_NOTE_PROPERTY(isDirty);
    CHECK_AND_SET_NOTE_PROPERTY(isLocal);
    CHECK_AND_SET_NOTE_PROPERTY(isDeleted);

    evernote::edam::Note & enNote = note.en_note;
    enNote.guid = noteGuid;
    enNote.__isset.guid = true;

#define CHECK_AND_SET_EN_NOTE_PROPERTY(propertyLocalName, propertyEnName, type, ...) \
    if (rec.contains(#propertyLocalName)) { \
        enNote.propertyEnName = (qvariant_cast<type>(rec.value(#propertyLocalName)))__VA_ARGS__; \
        enNote.__isset.propertyEnName = true; \
    } \
    else { \
        errorDescription += QObject::tr("Internal error: no " #propertyLocalName " field " \
                                        "in the result of SQL query"); \
        return false; \
    }

    CHECK_AND_SET_EN_NOTE_PROPERTY(updateSequenceNumber, updateSequenceNum, uint32_t);
    CHECK_AND_SET_EN_NOTE_PROPERTY(notebookGuid, notebookGuid, QString, .toStdString());
    CHECK_AND_SET_EN_NOTE_PROPERTY(title, title, QString, .toStdString());
    CHECK_AND_SET_EN_NOTE_PROPERTY(content, content, QString, .toStdString());

    // NOTE: omitting content hash and content length as it will be set by the service

    CHECK_AND_SET_EN_NOTE_PROPERTY(creationTimestamp, created, Timestamp);
    CHECK_AND_SET_EN_NOTE_PROPERTY(modificationTimestamp, updated, Timestamp);

    if (note.isDeleted) {
        CHECK_AND_SET_EN_NOTE_PROPERTY(deletionTimestamp, deleted, Timestamp);
    }

    CHECK_AND_SET_EN_NOTE_PROPERTY(isActive, active, int);

    if (rec.contains("hasAttributes"))
    {
        int hasAttributes = qvariant_cast<int>(rec.value("hasAttributes"));
        if (hasAttributes == 0) {
            return true;
        }
        else {
            enNote.__isset.attributes = true;
        }
    }
    else {
        errorDescription += QObject::tr("Internal error: no \"hasAttributes\" field " \
                                        "in the result of SQL query");
        return false;
    }

    QString error;
    res = FindAndSetTagGuidsPerNote(enNote, error);
    if (!res) {
        errorDescription += error;
        return false;
    }

    error.clear();
    res = FindAndSetResourcesPerNote(enNote, error);
    if (!res) {
        errorDescription += error;
        return false;
    }

    error.clear();
    res = FindAndSetNoteAttributesPerNote(enNote, error);
    if (!res) {
        errorDescription += error;
        return false;
    }

    // NOTE: no validity check of note here: it may be incomplete, for example, due to
    // not yet complete synchronization procedure
    return true;
}

bool LocalStorageManager::FindAllNotesPerNotebook(const Guid & notebookGuid, std::vector<Note> & notes,
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
    query.addBindValue(QString::fromStdString(notebookGuid));

    bool res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("can't select notes per notebook guid from SQL database");

    size_t numRows = query.size();
    notes.reserve(numRows);

    while(query.next())
    {
        QSqlRecord rec = query.record();
        Note note;

        CHECK_AND_SET_NOTE_PROPERTY(isDirty);
        CHECK_AND_SET_NOTE_PROPERTY(isLocal);
        CHECK_AND_SET_NOTE_PROPERTY(isDeleted);

        evernote::edam::Note & enNote = note.en_note;
        enNote.notebookGuid = notebookGuid;
        enNote.__isset.notebookGuid = true;

        CHECK_AND_SET_EN_NOTE_PROPERTY(updateSequenceNumber, updateSequenceNum, uint32_t);
        CHECK_AND_SET_EN_NOTE_PROPERTY(guid, guid, QString, .toStdString());
        CHECK_AND_SET_EN_NOTE_PROPERTY(title, title, QString, .toStdString());
        CHECK_AND_SET_EN_NOTE_PROPERTY(content, content, QString, .toStdString());

        // NOTE: omitting content hash and content length as it will be set by the service

        CHECK_AND_SET_EN_NOTE_PROPERTY(creationTimestamp, created, Timestamp);
        CHECK_AND_SET_EN_NOTE_PROPERTY(modificationTimestamp, updated, Timestamp);

        if (note.isDeleted) {
            CHECK_AND_SET_EN_NOTE_PROPERTY(deletionTimestamp, deleted, Timestamp);
        }

        CHECK_AND_SET_EN_NOTE_PROPERTY(isActive, active, int);

        if (rec.contains("hasAttributes"))
        {
            int hasAttributes = qvariant_cast<int>(rec.value("hasAttributes"));
            if (hasAttributes == 0) {
                return true;
            }
            else {
                enNote.__isset.attributes = true;
            }
        }
        else {
            errorDescription += QObject::tr("Internal error: no \"hasAttributes\" field "
                                            "in the result of SQL query");
            return false;
        }

        notes.push_back(note);
    }

    if (notes.empty()) {
        // NOTE: negative result is a result too, return empty vector and true
        return true;
    }

    for(Note & note: notes)
    {
        evernote::edam::Note & enNote = note.en_note;

        const Guid & guid = enNote.guid;
        if (!CheckGuid(guid)) {
            errorDescription += QObject::tr("found note with invalid guid");
            return false;
        }

        QString error;
        res = FindAndSetTagGuidsPerNote(enNote, error);
        if (!res) {
            errorDescription += error;
            return false;
        }

        error.clear();
        res = FindAndSetResourcesPerNote(enNote, error);
        if (!res) {
            errorDescription += error;
            return false;
        }

        error.clear();
        res = FindAndSetNoteAttributesPerNote(enNote, error);
        if (!res) {
            errorDescription += error;
            return false;
        }
    }

    return true;
}

bool LocalStorageManager::DeleteNote(const Note & note, QString & errorDescription)
{
    if (note.isLocal) {
        return ExpungeNote(note, errorDescription);
    }

    errorDescription = QObject::tr("Can't delete note from local storage database: ");

    const evernote::edam::Note & enNote = note.en_note;
    if (!CheckGuid(enNote.guid)) {
        errorDescription += QObject::tr("note guid is invalid");
        return false;
    }

    if (!note.isDeleted) {
        errorDescription += QObject::tr("note to be deleted is not marked as "
                                        "deleted one, rejecting to delete it");
        return false;
    }

    QSqlQuery query(m_sqlDatabase);
    query.prepare("UPDATE Notes SET isDeleted=1, isDirty=1 WHERE guid=?");
    query.addBindValue(QString::fromStdString(enNote.guid));
    bool res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("can't delete entry from \"Notes\" table in SQL database");

    return true;
}

bool LocalStorageManager::ExpungeNote(const Note & note, QString & errorDescription)
{
    errorDescription = QObject::tr("Can't expunge note from local storage database: ");

    const evernote::edam::Note & enNote = note.en_note;
    if (!CheckGuid(enNote.guid)) {
        errorDescription += QObject::tr("note guid is invalid");
        return false;
    }

    if (!note.isLocal) {
        errorDescription += QObject::tr("note to be expunged is not local");
        return false;
    }

    if (!note.isDeleted) {
        errorDescription += QObject::tr("note to be expunged is not marked as "
                                        "deleted one, rejecting to expunge it");
        return false;
    }

    QSqlQuery query(m_sqlDatabase);
    query.prepare("SELECT rowid FROM Notes WHERE guid=?");
    query.addBindValue(QString::fromStdString(note.en_note.guid));
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

    bool res = tag.CheckParameters(error);
    if (!res) {
        errorDescription += error;
        return false;
    }

    QString guid = QString::fromStdString(tag.en_tag.guid);
    int rowId = GetRowId("Tags", "guid", QVariant(guid));
    if (rowId >= 0) {
        errorDescription += QObject::tr("tag with the same guid already exists");
        return false;
    }

    QString nameUpper = QString::fromStdString(tag.en_tag.name).toUpper();
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

    bool res = tag.CheckParameters(error);
    if (!res) {
        errorDescription += error;
        return false;
    }

    QString guid = QString::fromStdString(tag.en_tag.guid);
    int rowId = GetRowId("Tags", "guid", QVariant(guid));
    if (rowId < 0) {
        errorDescription += QObject::tr("tag with specified guid was not found");
        return false;
    }

    QString nameUpper = QString::fromStdString(tag.en_tag.name).toUpper();
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

    if (!tag.CheckParameters(error)) {
        errorDescription += QObject::tr("tag is invalid: ");
        errorDescription += error;
        return false;
    }

    if (!note.CheckParameters(errorDescription)) {
        errorDescription += QObject::tr("note is invalid: ");
        errorDescription += error;
        return false;
    }

    QSqlQuery query(m_sqlDatabase);
    query.prepare("INSERT OR REPLACE INTO NoteTags (note, tag) VALUES(?, ?)");
    query.addBindValue(QString::fromStdString(note.en_note.guid));
    query.addBindValue(QString::fromStdString(tag.en_tag.guid));

    bool res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("can't insert or replace note guid + tag guid in "
                                 "\"NoteTags\" table in SQL storage database");

    return true;
}

bool LocalStorageManager::FindTag(const Guid & tagGuid, Tag & tag, QString & errorDescription) const
{
    errorDescription = QObject::tr("Can't find tag in local storage database: ");

    if (!CheckGuid(tagGuid)) {
        errorDescription += QObject::tr("requested tag guid is invalid");
        return false;
    }

    tag = Tag();

    QSqlQuery query(m_sqlDatabase);

    query.prepare("SELECT updateSequenceNumber, name, parentGuid, isDirty, isLocal, "
                  "isDeleted FROM Tags WHERE guid = ?");
    query.addBindValue(QString::fromStdString(tagGuid));

    bool res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("can't select tag from \"Tags\" table in SQL database: ");

    if (!query.next()) {
        errorDescription += QObject::tr("Internal error: SQL query result is empty");
        return false;
    }

    evernote::edam::Tag & enTag = tag.en_tag;
    enTag.guid = tagGuid;
    enTag.__isset.guid = true;

    bool conversionResult = false;
    enTag.updateSequenceNum = static_cast<int32_t>(query.value(0).toInt(&conversionResult));
    if (enTag.updateSequenceNum < 0 || !conversionResult) {
        errorDescription += QObject::tr("No tag's update sequence number was found in SQL query result");
        return false;
    }
    enTag.__isset.updateSequenceNum = true;

    enTag.name = query.value(1).toString().toStdString();
    size_t nameSize = enTag.name.size();
    if ( (nameSize < evernote::limits::g_Limits_constants.EDAM_TAG_NAME_LEN_MIN) ||
         (nameSize > evernote::limits::g_Limits_constants.EDAM_TAG_NAME_LEN_MAX) )
    {
        errorDescription += QObject::tr("Internal error: found tag with invalid "
                                        "name size from SQL query result");
        return false;
    }
    enTag.__isset.name = true;

    enTag.parentGuid = query.value(2).toString().toStdString();
    if (enTag.parentGuid.empty()) {
        enTag.__isset.parentGuid = false;
    }
    else if (!CheckGuid(enTag.parentGuid)) {
        errorDescription += QObject::tr("Internal error: found tag with invalid parent guid "
                                        "in SQL query result");
        return false;
    }
    enTag.__isset.parentGuid = true;

    conversionResult = false;
    int isDirtyInt = query.value(3).toInt(&conversionResult);
    if (isDirtyInt < 0 || !conversionResult) {
        errorDescription += QObject::tr("No tag's \"isDirty\" flag was found in SQL result");
        return false;
    }
    tag.isDirty = static_cast<bool>(isDirtyInt);

    conversionResult = false;
    int isLocalInt = query.value(4).toInt(&conversionResult);
    if (isLocalInt < 0 || !conversionResult) {
        errorDescription += QObject::tr("No tag's \"isLocal\" flag was found in SQL result");
        return false;
    }
    tag.isLocal = static_cast<bool>(isLocalInt);

    conversionResult = false;
    int isDeletedInt = query.value(5).toInt(&conversionResult);
    if (isDeletedInt < 0 || !conversionResult) {
        errorDescription += QObject::tr("No tag's \"isDeleted\" flag was found in SQL result");
        return false;
    }
    tag.isDeleted = static_cast<bool>(isDeletedInt);

    return tag.CheckParameters(errorDescription);
}

bool LocalStorageManager::DeleteTag(const Tag & tag, QString & errorDescription)
{
    if (tag.isLocal) {
        return ExpungeTag(tag, errorDescription);
    }

    errorDescription = QObject::tr("Can't delete tag from local storage database: ");

    if (!tag.isDeleted) {
        errorDescription += QObject::tr("tag to be deleted is not marked as deleted one");
        return false;
    }

    QSqlQuery query(m_sqlDatabase);
    query.prepare("UPDATE Tags SET isDeleted = 1, isDirty = 1 WHERE guid = ?");
    query.addBindValue(QString::fromStdString(tag.en_tag.guid));
    bool res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("can't mark tag deleted in \"Tags\" table in SQL database");

    return true;
}

bool LocalStorageManager::ExpungeTag(const Tag & tag, QString & errorDescription)
{
    errorDescription = QObject::tr("Can't expunge tag from local storage database: ");

    if (!tag.isLocal) {
        errorDescription += QObject::tr("tag to be expunged is not local");
        return false;
    }

    if (!tag.isDeleted) {
        errorDescription += QObject::tr("tag to be expunged is not marked as deleted one");
        return false;
    }

    if (!tag.en_tag.__isset.guid) {
        errorDescription += QObject::tr("tag's guid is not set");
        return false;
    }
    else if (!CheckGuid(tag.en_tag.guid)) {
        errorDescription += QObject::tr("tag's guid is invalid");
        return false;
    }

    int rowId = GetRowId("Tags", "guid", QVariant(QString::fromStdString(tag.en_tag.guid)));
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

bool LocalStorageManager::FindResource(const Guid & resourceGuid, Resource & resource,
                                       QString & errorDescription, const bool withBinaryData) const
{
    errorDescription = QObject::tr("Can't find resource in local storage database: ");

    if (!CheckGuid(resourceGuid)) {
        errorDescription += QObject::tr("requested resource guid is invalid");
        return false;
    }

    resource = Resource();

    resource.en_resource.guid = resourceGuid;
    resource.en_resource.__isset.guid = true;

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

    if (rec.contains("isDirty")) {
        resource.isDirty = (qvariant_cast<int>(rec.value("isDirty")) != 0);
    }
    else {
        errorDescription += QObject::tr("no \"isDirty\" field in the result of SQL query");
        return false;
    }

#define CHECK_AND_SET_EN_RESOURCE_PROPERTY(holder, localPropertyName, enPropertyName, \
                                           localType, trueType, isRequired, ...) \
    if (rec.contains(#localPropertyName)) { \
        holder.enPropertyName = static_cast<trueType>((qvariant_cast<localType>(rec.value(#localPropertyName)))__VA_ARGS__); \
        holder.__isset.enPropertyName = true; \
    } \
    else if (isRequired) { \
        errorDescription += QObject::tr("no " #localPropertyName \
                                        " field in the result of SQL query"); \
        return false; \
    }

    auto & enResource = resource.en_resource;
    bool isRequired = true;
    CHECK_AND_SET_EN_RESOURCE_PROPERTY(enResource, noteGuid, noteGuid, QString, std::string, isRequired, .toStdString());
    CHECK_AND_SET_EN_RESOURCE_PROPERTY(enResource, updateSequenceNumber, updateSequenceNum,
                                       int, int32_t, isRequired);
    CHECK_AND_SET_EN_RESOURCE_PROPERTY(enResource.data, dataSize, size, int, int32_t, isRequired);
    CHECK_AND_SET_EN_RESOURCE_PROPERTY(enResource.data, dataHash, bodyHash, QString,
                                       std::string, isRequired, .toStdString());
    CHECK_AND_SET_EN_RESOURCE_PROPERTY(enResource, mime, mime, QString, std::string,
                                       isRequired, .toStdString());

    isRequired = false;
    CHECK_AND_SET_EN_RESOURCE_PROPERTY(enResource, width, width, int, int32_t, isRequired);
    CHECK_AND_SET_EN_RESOURCE_PROPERTY(enResource, height, height, int, int32_t, isRequired);
    CHECK_AND_SET_EN_RESOURCE_PROPERTY(enResource.recognition, dataSize, size, int, int32_t, isRequired);
    CHECK_AND_SET_EN_RESOURCE_PROPERTY(enResource.recognition, dataHash, bodyHash,
                                       QString, std::string, isRequired, .toStdString());

    if (withBinaryData) {
        CHECK_AND_SET_EN_RESOURCE_PROPERTY(enResource.recognition, dataBody, body, QString,
                                           std::string, isRequired, .toStdString());
        CHECK_AND_SET_EN_RESOURCE_PROPERTY(enResource.data, dataBody, body, QString,
                                           std::string, isRequired, .toStdString());
    }

    if (enResource.data.__isset.size && enResource.data.__isset.bodyHash)
    {
        if (withBinaryData && enResource.data.__isset.body) {
            enResource.__isset.data = true;
        }
        else if (!withBinaryData) {
            enResource.__isset.data = true;
        }
        else {
            enResource.__isset.data = false;
        }
    }
    else
    {
        enResource.__isset.data = false;
    }

    if (enResource.recognition.__isset.size && enResource.recognition.__isset.bodyHash)
    {
        if (withBinaryData && enResource.recognition.__isset.body) {
            enResource.__isset.recognition = true;
        }
        else if (!withBinaryData) {
            enResource.__isset.recognition = true;
        }
        else {
            enResource.__isset.recognition = false;
        }
    }
    else
    {
        enResource.__isset.recognition = false;
    }

    query.prepare("SELECT data FROM ResourceAttributes WHERE guid = ?");
    query.addBindValue(QString::fromStdString(resourceGuid));

    res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("can't select data from \"ResourceAttributes\" table in SQL database");

    if (!query.next()) {
        enResource.__isset.attributes = false;
        return true;
    }

    QByteArray serializedResourceAttributes = query.value(0).toByteArray();
    enResource.attributes = GetDeserializedResourceAttributes(serializedResourceAttributes);
    enResource.__isset.attributes = true;

    return true;
}

bool LocalStorageManager::ExpungeResource(const Resource & resource, QString & errorDescription)
{
    errorDescription = QObject::tr("Can't expunge resource from local storage database: ");

    int rowId = GetRowId("Resources", "guid", QVariant(QString::fromStdString(resource.en_resource.guid)));
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

    bool res = search.CheckParameters(error);
    if (!res) {
        errorDescription += error;
        return false;
    }

    QString guid = QString::fromStdString(search.en_search.guid);
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

    bool res = search.CheckParameters(error);
    if (!res) {
        errorDescription += error;
        return false;
    }

    QString guid = QString::fromStdString(search.en_search.guid);
    int rowId = GetRowId("SavedSearches", "guid", QVariant(guid));
    if (rowId < 0) {
        errorDescription += QObject::tr("saved search with specified guid was not found");
        return false;
    }

    return InsertOrReplaceSavedSearch(search, errorDescription);
}

bool LocalStorageManager::FindSavedSearch(const Guid & searchGuid, SavedSearch & search,
                                          QString & errorDescription) const
{
    errorDescription = QObject::tr("Can't find saved search in local storage database: ");

    if (!CheckGuid(searchGuid)) {
        errorDescription += QObject::tr("requested saved search guid is invalid");
        return false;
    }

    search = SavedSearch();

    QSqlQuery query(m_sqlDatabase);

    query.prepare("SELECT name, query, format, updateSequenceNumber, includeAccount, "
                  "includePersonalLinkedNotebooks, includeBusinessLinkedNotebooks "
                  "FROM SavedSearches WHERE guid = ?");
    query.addBindValue(QString::fromStdString(searchGuid));

    bool res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("can't find saved search in \"SavedSearches\" table in SQL database: ");

    if (!query.next()) {
        errorDescription += QObject::tr("Internal error: SQL query result is empty");
        return false;
    }

    evernote::edam::SavedSearch & enSearch = search.en_search;
    enSearch.guid = searchGuid;
    enSearch.__isset.guid = true;

    QSqlRecord rec = query.record();

#define CHECK_AND_SET_EN_SEARCH_PROPERTY(holder, localPropertyName, enPropertyName, \
                                         localType, trueType, isRequired, ...) \
    if (rec.contains(#localPropertyName)) { \
        holder.enPropertyName = static_cast<trueType>((qvariant_cast<localType>(rec.value(#localPropertyName)))__VA_ARGS__); \
        holder.__isset.enPropertyName = true; \
    } \
    else if (isRequired) { \
        errorDescription += QObject::tr("no " #localPropertyName " field" \
                                        "in the result of SQL query"); \
        return false; \
    }

    bool isRequired = true;
    CHECK_AND_SET_EN_SEARCH_PROPERTY(enSearch, name, name, QString, std::string,
                                     isRequired, .toStdString());
    CHECK_AND_SET_EN_SEARCH_PROPERTY(enSearch, query, query, QString, std::string,
                                     isRequired, .toStdString());
    CHECK_AND_SET_EN_SEARCH_PROPERTY(enSearch, format, format, int,
                                     evernote::edam::QueryFormat::type, isRequired);
    CHECK_AND_SET_EN_SEARCH_PROPERTY(enSearch, updateSequenceNumber, updateSequenceNum,
                                     int, int32_t, isRequired);

    auto & scope = enSearch.scope;
    CHECK_AND_SET_EN_SEARCH_PROPERTY(scope, includeAccount, includeAccount,
                                     int, bool, isRequired, ? true : false);
    CHECK_AND_SET_EN_SEARCH_PROPERTY(scope, includePersonalLinkedNotebooks,
                                     includePersonalLinkedNotebooks, int, bool,
                                     isRequired, ? true : false);
    CHECK_AND_SET_EN_SEARCH_PROPERTY(scope, includeBusinessLinkedNotebooks,
                                     includeBusinessLinkedNotebooks, int, bool,
                                     isRequired, ? true : false);

#undef CHECK_AND_SET_EN_SEARCH_PROPERTY

    return true;
}

bool LocalStorageManager::ExpungeSavedSearch(const SavedSearch & search,
                                             QString & errorDescription)
{
    errorDescription = QObject::tr("Can't expunge saved search from local storage database: ");

    if (!search.en_search.__isset.guid) {
        errorDescription += QObject::tr("saved search's guid is not set");
        return false;
    }
    else if (!CheckGuid(search.en_search.guid)) {
        errorDescription += QObject::tr("saved search's guid is invalid");
        return false;
    }

    int rowId = GetRowId("SavedSearches", "guid", QVariant(QString::fromStdString(search.en_search.guid)));
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

#define SET_IS_FREE_ACCOUNT_FLAG \
    User currentUser; \
    QString error; \
    bool res = FindUser(m_currentUserId, currentUser, error); \
    if (!res) { \
        errorDescription += QObject::tr("can't find current user to " \
                                        "determine max allowed size of the resource: "); \
        errorDescription += error; \
        return false; \
    } \
    bool isFreeAccount = (currentUser.en_user.privilege == evernote::edam::PrivilegeLevel::NORMAL);

bool LocalStorageManager::AddResource(const Resource & resource, QString & errorDescription)
{
    errorDescription = QObject::tr("Can't add resource to local storage database: ");

    SET_IS_FREE_ACCOUNT_FLAG

    if (!resource.CheckParameters(error, isFreeAccount)) {
        errorDescription += error;
        return false;
    }

    int rowId = GetRowId("Resources", "guid", QVariant(QString::fromStdString(resource.en_resource.guid)));
    if (rowId >= 0) {
        errorDescription += QObject::tr("resource with the same guid already exists");
        return false;
    }

    return InsertOrReplaceResource(resource, errorDescription);
}

bool LocalStorageManager::UpdateResource(const Resource & resource, QString & errorDescription)
{
    errorDescription = QObject::tr("Can't update resource in local storage database: ");

    SET_IS_FREE_ACCOUNT_FLAG

    if (!resource.CheckParameters(error)) {
        errorDescription += error;
        return false;
    }

    int rowId = GetRowId("Resources", "guid", QVariant(QString::fromStdString(resource.en_resource.guid)));
    if (rowId < 0) {
        errorDescription += QObject::tr("resource to be updated was not found by guid");
        return false;
    }

    return InsertOrReplaceResource(resource, errorDescription);
}

#undef SET_IS_FREE_ACCOUNT_FLAG

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
                     "  isActive                INTEGER                 NOT NULL, "
                     ")");
    DATABASE_CHECK_AND_SET_ERROR("Can't create Users table: ");

    res = query.exec("CREATE TABLE IF NOT EXISTS UserAttributes("
                     "  id REFERENCES Users(id) ON DELETE CASCADE ON UPDATE CASCADE, "
                     "  data                    BLOB                    DEFAULT NULL)");
    DATABASE_CHECK_AND_SET_ERROR("Can't create UserAttributes table: ");

    res = query.exec("CREATE TABLE IF NOT EXITS Accounting("
                     "  id REFERENCES Users(id) ON DELETE CASCADE ON UPDATE CASCADE, "
                     "  data                    BLOB                    DEFAULT NULL)");
    DATABASE_CHECK_AND_SET_ERROR("Can't create Accounting table: ");

    res = query.exec("CREATE TABLE IF NOT EXISTS PremiumInfo("
                     "  id REFERENCES Users(id) ON DELETE CASCADE ON UPDATE CASCADE, "
                     "  data                    BLOB                    DEFAULT NULL)");
    DATABASE_CHECK_AND_SET_ERROR("Can't create PremiumInfo table: ");

    res = query.exec("CREATE TABLE IF NOT EXISTS BusinessUserInfo("
                     "  id REFERENCES Users(id) ON DELETE CASCADE ON UPDATE CASCADE, "
                     "  data                    BLOB                    DEFAULT NULL)");
    DATABASE_CHECK_AND_SET_ERROR("Can't create BusinessUserInfo table: ");

    res = query.exec("CREATE TABLE IF NOT EXISTS Notebooks("
                     "  guid                            TEXT PRIMARY KEY  NOT NULL UNIQUE, "
                     "  updateSequenceNumber            INTEGER           NOT NULL, "
                     "  name                            TEXT              NOT NULL, "
                     "  creationTimestamp               INTEGER           NOT NULL, "
                     "  modificationTimestamp           INTEGER           NOT NULL, "
                     "  isDirty                         INTEGER           NOT NULL, "
                     "  isLocal                         INTEGER           NOT NULL, "
                     "  isDeleted                       INTEGER           NOT NULL, "
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
    DATABASE_CHECK_AND_SET_ERROR("Can't create Notebooks table: ");

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
    DATABASE_CHECK_AND_SET_ERROR("Can't create NotebookRestrictions table: ");

    res = query.exec("CREATE TABLE IF NOT EXISTS SharedNotebooks("
                     "  shareId                         INTEGER    NOT NULL, "
                     "  userId                          INTEGER    NOT NULL, "
                     "  notebookGuid REFERENCES Notebooks(guid) ON DELETE CASCADE ON UPDATE CASCADE"
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
    DATABASE_CHECK_AND_SET_ERROR("Can't create SharedNotebooks table: ");

    res = query.exec("CREATE VIRTUAL TABLE NoteText USING fts3("
                     "  guid              TEXT PRIMARY KEY     NOT NULL UNIQUE, "
                     "  title             TEXT, "
                     "  content           TEXT, "
                     "  notebookGuid REFERENCES Notebooks(guid) ON DELETE CASCADE ON UPDATE CASCADE"
                     ")");
    DATABASE_CHECK_AND_SET_ERROR("Can't create virtual table NoteText: ");

    res = query.exec("CREATE TABLE IF NOT EXISTS Notes("
                     "  guid                    TEXT PRIMARY KEY     NOT NULL UNIQUE, "
                     "  updateSequenceNumber    INTEGER              NOT NULL, "
                     "  isDirty                 INTEGER              NOT NULL, "
                     "  isLocal                 INTEGER              NOT NULL, "
                     "  isDeleted               INTEGER              DEFAULT 0, "
                     "  title                   TEXT,                NOT NULL, "
                     "  content                 TEXT,                NOT NULL, "
                     "  creationTimestamp       INTEGER              NOT NULL, "
                     "  modificationTimestamp   INTEGER              NOT NULL, "
                     "  deletionTimestamp       INTEGER              DEFAULT 0, "
                     "  isActive                INTEGER              DEFAULT 1, "
                     "  hasAttributes           INTEGER              DEFAULT 0, "
                     "  notebookGuid REFERENCES Notebooks(guid) ON DELETE CASCADE ON UPDATE CASCADE"
                     ")");
    DATABASE_CHECK_AND_SET_ERROR("Can't create Notes table: ");

    res = query.exec("CREATE TABLE IF NOT EXISTS NoteAttributes("
                     "  noteGuid REFERENCES Notes(guid) ON DELETE CASCADE ON UPDATE CASCADE, "
                     "  data                    BLOB                DEFAULT NULL"
                     ")");
    DATABASE_CHECK_AND_SET_ERROR("Can't create NoteAttributes table: ");

    res = query.exec("CREATE INDEX NotesNotebooks ON Notes(notebookGuid)");
    DATABASE_CHECK_AND_SET_ERROR("Can't create index NotesNotebooks: ");

    res = query.exec("CREATE TABLE IF NOT EXISTS Resources("
                     "  guid                    TEXT PRIMARY KEY     NOT NULL UNIQUE, "
                     "  noteGuid                TEXT                 NOT NULL, "
                     "  updateSequenceNumber    INTEGER              NOT NULL, "
                     "  isDirty                 INTEGER              NOT NULL, "
                     "  dataBody                TEXT,                NOT NULL, "
                     "  dataSize                INTEGER              NOT NULL, "
                     "  dataHash                TEXT                 NOT NULL, "
                     "  mime                    TEXT                 NOT NULL, "
                     "  width                   INTEGER              DEFAULT 0, "
                     "  height                  INTEGER              DEFAULT 0, "
                     "  recognitionBody         TEXT                 DEFAULT NULL, "
                     "  recognitionSize         INTEGER              DEFAULT 0, "
                     "  recognitionHash         TEXT                 DEFAULT NULL, "
                     ")");
    DATABASE_CHECK_AND_SET_ERROR("Can't create Resources table: ");

    res = query.exec("CREATE INDEX ResourceNote ON Resources(noteGuid)");
    DATABASE_CHECK_AND_SET_ERROR("Can't create ResourceNote index: ");

    res = query.exec("CREATE TABLE IF NOT EXISTS ResourceAttributes("
                     "  guid REFERENCES Resource(guid) ON DELETE CASCADE ON UPDATE CASCADE, "
                     "  data                  BLOB                 DEFAULT NULL");
    DATABASE_CHECK_AND_SET_ERROR("Can't create ResourceAttributes table: ");

    res = query.exec("CREATE TABLE IF NOT EXISTS Tags("
                     "  guid                  TEXT PRIMARY KEY     NOT NULL UNIQUE, "
                     "  updateSequenceNumber  INTEGER              NOT NULL, "
                     "  name                  TEXT                 NOT NULL, "
                     "  nameUpper             TEXT                 NOT NULL, "
                     "  parentGuid            TEXT                 DEFAULT NULL, "
                     "  isDirty               INTEGER              NOT NULL, "
                     "  isLocal               INTEGER              NOT NULL, "
                     "  isDeleted             INTEGER              DEFAULT 0, "
                     ")");
    DATABASE_CHECK_AND_SET_ERROR("Can't create Tags table: ");

    res = query.exec("CREATE INDEX TagsSearchName ON Tags(nameUpper)");
    DATABASE_CHECK_AND_SET_ERROR("Can't create TagsSearchName index: ");

    res = query.exec("CREATE TABLE IF NOT EXISTS NoteTags("
                     "  note REFERENCES Notes(guid) ON DELETE CASCADE ON UPDATE CASCADE, "
                     "  tag  REFERENCES Tags(guid)  ON DELETE CASCADE ON UPDATE CASCADE, "
                     "  UNIQUE(note, tag) ON CONFLICT REPLACE"
                     ")");
    DATABASE_CHECK_AND_SET_ERROR("Can't create NoteTags table: ");

    res = query.exec("CREATE INDEX NoteTagsNote ON NoteTags(note)");
    DATABASE_CHECK_AND_SET_ERROR("Can't create NoteTagsNote index: ");

    res = query.exec("CREATE TABLE IF NOT EXISTS SavedSearches("
                     "  guid                            TEXT PRIMARY KEY    NOT NULL UNIQUE, "
                     "  name                            TEXT                NOT NULL, "
                     "  query                           TEXT                NOT NULL, "
                     "  format                          INTEGER             NOT NULL, "
                     "  updateSequenceNumber            INTEGER             NOT NULL, "
                     "  includeAccount                  INTEGER             NOT NULL, "
                     "  includePersonalLinkedNotebooks  INTEGER             NOT NULL, "
                     "  includeBusinessLinkedNotebooks  INTEGER             NOT NULL)");
    DATABASE_CHECK_AND_SET_ERROR("Can't create SavedSearches table: ");

    return true;
}

bool LocalStorageManager::SetNoteAttributes(const evernote::edam::Note & note,
                                            QString & errorDescription)
{
    errorDescription += QObject::tr("can't set note attributes: ");

    if (!CheckGuid(note.guid)) {
        errorDescription += QObject::tr("note guid is invalid");
        return false;
    }

    if (!note.__isset.attributes) {
        return true;
    }

    const evernote::edam::NoteAttributes & attributes = note.attributes;
    QString noteGuid = QString::fromStdString(note.guid);

    QSqlQuery query(m_sqlDatabase);
    query.prepare("INSERT OR REPLACE INTO NoteAttributes(noteGuid, data) VALUES(?, ?)");
    query.addBindValue(noteGuid);
    QByteArray data = GetSerializedNoteAttributes(attributes);
    query.addBindValue(data);

    bool res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("can't insert or replace lastEditorId into \"NoteAttributes\" table in SQL database");

    return true;
}

bool LocalStorageManager::SetNotebookAdditionalAttributes(const evernote::edam::Notebook & notebook,
                                                          QString & errorDescription)
{
    QSqlQuery query(m_sqlDatabase);

    bool hasAdditionalAttributes = false;
    query.prepare("INSERT OR REPLACE INTO Notebooks (:notebookGuid, :columns) VALUES(:values)");
    query.bindValue("notebookGuid", QVariant(QString::fromStdString(notebook.guid)));

    QString columns;
    QString values;

    if (notebook.__isset.published)
    {
        hasAdditionalAttributes = true;

        columns.append("isPublished");
        values.append(QString::number(notebook.published ? 1 : 0));
    }

    if (notebook.published && notebook.__isset.published)
    {
        hasAdditionalAttributes = true;

        const Publishing & publishing = notebook.publishing;

        if (publishing.__isset.uri)
        {
            if (!columns.isEmpty()) { columns.append(", "); }
            if (!values.isEmpty()) { values.append(", "); }

            columns.append("publishingUri");
            values.append(QString::fromStdString(publishing.uri));
        }

        if (publishing.__isset.order)
        {
            if (!columns.isEmpty()) { columns.append(", "); }
            if (!values.isEmpty()) { values.append(", "); }

            columns.append("publishingNoteSortOrder");
            values.append(QString::number(publishing.order));
        }

        if (publishing.__isset.ascending)
        {
            if (!columns.isEmpty()) { columns.append(", "); }
            if (!values.isEmpty()) { values.append(", "); }

            columns.append("publishingAscendingSort");
            values.append(QString::number((publishing.ascending ? 1 : 0)));
        }

        if (publishing.__isset.publicDescription)
        {
            if (!columns.isEmpty()) { columns.append(", "); }
            if (!values.isEmpty()) { values.append(", "); }

            columns.append("publicDescription");
            values.append(QString::fromStdString(publishing.publicDescription));
        }
    }

    if (notebook.__isset.businessNotebook)
    {
        hasAdditionalAttributes = true;

        const BusinessNotebook & businessNotebook = notebook.businessNotebook;

        if (businessNotebook.__isset.notebookDescription)
        {
            if (!columns.isEmpty()) { columns.append(", "); }
            if (!values.isEmpty()) { values.append(", "); }

            columns.append("businessNotebookDescription");
            values.append(QString::fromStdString(businessNotebook.notebookDescription));
        }

        if (businessNotebook.__isset.privilege)
        {
            if (!columns.isEmpty()) { columns.append(", "); }
            if (!values.isEmpty()) { values.append(", "); }

            columns.append("businessNotebookPrivilegeLevel");
            values.append(QString::number(businessNotebook.privilege));
        }

        if (businessNotebook.__isset.recommended)
        {
            if (!columns.isEmpty()) { columns.append(", "); }
            if (!values.isEmpty()) { values.append(", "); }

            columns.append("businessNotebookIsRecommended");
            values.append(QString::number((businessNotebook.recommended ? 1 : 0)));
        }
    }

    if (notebook.__isset.contact)
    {
        hasAdditionalAttributes = true;

        if (!columns.isEmpty()) { columns.append(", "); }
        if (!values.isEmpty()) { values.append(", "); }

        columns.append("contactId");
        values.append(QString::number(notebook.contact.id));

        // TODO: check whether such id is present in Users table, if not, add it
    }

    if (hasAdditionalAttributes) {
        query.bindValue("columns", columns);
        query.bindValue("values", values);
        bool res = query.exec();
        DATABASE_CHECK_AND_SET_ERROR("can't insert or replace additional notebook attributes "
                                     "into \"Notebooks\" table in SQL database");
    }

    QString error;
    bool res = SetNotebookRestrictions(notebook, error);
    if (!res) {
        errorDescription += error;
        return res;
    }

    const auto & sharedNotebooks = notebook.sharedNotebooks;
    for(const auto & sharedNotebook: sharedNotebooks)
    {
        error.clear();
        res = SetSharedNotebookAttributes(sharedNotebook, error);
        if (!res) {
            errorDescription += error;
            return res;
        }
    }

    return true;
}

bool LocalStorageManager::SetNotebookRestrictions(const evernote::edam::Notebook & notebook,
                                                  QString & errorDescription)
{
    if (!notebook.__isset.restrictions) {
        // Nothing to do
        return true;
    }

    errorDescription = QObject::tr("Can't set notebook restrictions: ");

    QSqlQuery query(m_sqlDatabase);
    query.prepare("INSERT OR REPLACE INTO NotebookRestrictions (:notebookGuid, :columns) "
                  "VALUES(:vals)");
    query.bindValue("notebookGuid", QString::fromStdString(notebook.guid));

    bool hasAnyRestriction = false;

    QString columns, values;

    const evernote::edam::NotebookRestrictions & notebookRestrictions = notebook.restrictions;

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

bool LocalStorageManager::SetSharedNotebookAttributes(const evernote::edam::SharedNotebook & sharedNotebook,
                                                      QString & errorDescription)
{
    errorDescription = QObject::tr("Can't set shared notebook attributes: ");

    if (!sharedNotebook.__isset.id) {
        errorDescription += QObject::tr("shared notebook's share id is not set");
        return false;
    }

    if (!sharedNotebook.__isset.notebookGuid) {
        errorDescription += QObject::tr("shared notebook's notebook guid is not set");
        return false;
    }
    else if (!CheckGuid(sharedNotebook.notebookGuid)) {
        errorDescription += QObject::tr("shared notebook's notebook guid is invalid");
        return false;
    }

    QSqlQuery query(m_sqlDatabase);
    query.prepare("INSERT OR REPLACE INTO SharedNotebooks(:columns) VALUES(:vals)");

    QString columns, values;
    bool hasAnyProperty = false;

#define CHECK_AND_SET_SHARED_NOTEBOOK_ATTRIBUTE(holder, isSetName, columnName, valueName) \
    if (holder.__isset.isSetName) \
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

    CHECK_AND_SET_SHARED_NOTEBOOK_ATTRIBUTE(sharedNotebook, id, shareId,
                                            QString::number(sharedNotebook.id));
    CHECK_AND_SET_SHARED_NOTEBOOK_ATTRIBUTE(sharedNotebook, userId, userId,
                                            QString::number(sharedNotebook.userId));
    CHECK_AND_SET_SHARED_NOTEBOOK_ATTRIBUTE(sharedNotebook, notebookGuid, notebookGuid,
                                            QString::fromStdString(sharedNotebook.notebookGuid));
    CHECK_AND_SET_SHARED_NOTEBOOK_ATTRIBUTE(sharedNotebook, email, email,
                                            QString::fromStdString(sharedNotebook.email));
    CHECK_AND_SET_SHARED_NOTEBOOK_ATTRIBUTE(sharedNotebook, serviceCreated, creationTimestamp,
                                            QString::number(sharedNotebook.serviceCreated));
    CHECK_AND_SET_SHARED_NOTEBOOK_ATTRIBUTE(sharedNotebook, serviceUpdated, modificationTimestamp,
                                            QString::number(sharedNotebook.serviceUpdated));
    CHECK_AND_SET_SHARED_NOTEBOOK_ATTRIBUTE(sharedNotebook, shareKey, shareKey,
                                            QString::fromStdString(sharedNotebook.shareKey));
    CHECK_AND_SET_SHARED_NOTEBOOK_ATTRIBUTE(sharedNotebook, username, username,
                                            QString::fromStdString(sharedNotebook.username));
    CHECK_AND_SET_SHARED_NOTEBOOK_ATTRIBUTE(sharedNotebook, privilege, sharedNotebookPrivilegeLevel,
                                            QString::number(sharedNotebook.privilege));
    CHECK_AND_SET_SHARED_NOTEBOOK_ATTRIBUTE(sharedNotebook, allowPreview, allowPreview,
                                            QString::number(sharedNotebook.allowPreview ? 1 : 0));

    if (sharedNotebook.__isset.recipientSettings)
    {
        const auto & recipientSettings = sharedNotebook.recipientSettings;

        CHECK_AND_SET_SHARED_NOTEBOOK_ATTRIBUTE(recipientSettings, reminderNotifyEmail,
                                                recipientReminderNotifyEmail,
                                                QString::number(recipientSettings.reminderNotifyEmail ? 1 : 0));
        CHECK_AND_SET_SHARED_NOTEBOOK_ATTRIBUTE(recipientSettings, reminderNotifyInApp,
                                                recipientRemindrNotifyInApp,
                                                QString::number(recipientSettings.reminderNotifyInApp ? 1 : 0));
    }

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

bool LocalStorageManager::InsertOrReplaceUser(const User & user, QString & errorDescription)
{
    errorDescription += QObject::tr("Can't insert or replace User into local storage database: ");

    QSqlQuery query(m_sqlDatabase);
    query.prepare("INSERT OR REPLACE INTO Users (id, username, name, timezone, privilege, "
                  "  creationTimestamp, modificationTimestamp, isDirty, isLocal, "
                  "  deletionTimestamp, isActive) VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?)");

    const evernote::edam::User & en_user = user.en_user;

#define CHECK_AND_SET_USER_VALUE(column, error, prep) \
    if (!en_user.__isset.column) { \
        errorDescription += QObject::tr(error); \
        return false; \
    } \
    else { \
        query.addBindValue(prep(en_user.column)); \
    }

    CHECK_AND_SET_USER_VALUE(id, "User ID is not set", QString::number);
    CHECK_AND_SET_USER_VALUE(username, "Username is not set", QString::fromStdString);
    CHECK_AND_SET_USER_VALUE(name, "User's name is not set", QString::fromStdString);
    CHECK_AND_SET_USER_VALUE(timezone, "User's timezone is not set", QString::fromStdString);
    CHECK_AND_SET_USER_VALUE(created, "User's creation timestamp is not set", QString::number);
    CHECK_AND_SET_USER_VALUE(updated, "User's modification timestamp is not set", QString::number);

    query.addBindValue(QString::number((user.isDirty ? 1 : 0)));
    query.addBindValue(QString::number((user.isLocal ? 1 : 0)));

    // Process deletionTimestamp properties specifically
    if (en_user.__isset.deleted) {
        query.addBindValue(QString::number(en_user.deleted));
    }
    else {
        query.addBindValue(QString::number(0));
    }

    CHECK_AND_SET_USER_VALUE(active, "User's active field is not set (should be true)", static_cast<int>);

#undef CHECK_AND_SET_USER_VALUE

    bool res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("can't insert or replace user into \"Users\" table in SQL database");

    if (en_user.__isset.attributes)
    {
        query.clear();
        query.prepare("INSERT OR REPLACE INTO UserAttributes (id, data) VALUES(?, ?)");
        query.addBindValue(QString::number(en_user.id));
        QByteArray serializedUserAttributes = GetSerializedUserAttributes(en_user.attributes);
        query.addBindValue(serializedUserAttributes);

        res = query.exec();
        DATABASE_CHECK_AND_SET_ERROR("can't add user attributes into \"UserAttributes\" table in SQL database");
    }

    if (en_user.__isset.accounting)
    {
        query.clear();
        query.prepare("INSERT OR REPLACE INTO Accounting (id, data) VALUES(?, ?)");
        query.addBindValue(QString::number(en_user.id));
        QByteArray serializedAccounting = GetSerializedAccounting(en_user.accounting);
        query.addBindValue(serializedAccounting);

        res = query.exec();
        DATABASE_CHECK_AND_SET_ERROR("can't add user's accounting info into \"Accounting\" table in SQL database");
    }

    if (en_user.__isset.premiumInfo)
    {
        query.clear();
        query.prepare("INSERT OR REPLACE INTO PremiumInfo (id, data) VALUES(?, ?)");
        query.addBindValue(QString::number(en_user.id));
        QByteArray serializedPremiumInfo = GetSerializedPremiumInfo(en_user.premiumInfo);
        query.addBindValue(serializedPremiumInfo);

        res = query.exec();
        DATABASE_CHECK_AND_SET_ERROR("can't add user's premium info into \"PremiumInfo\" table in SQL database");
    }

    if (en_user.__isset.businessUserInfo)
    {
        query.clear();
        query.prepare("INSERT OR REPLACE INTO BusinessUserInfo (id, data) VALUES(?, ?)");
        query.addBindValue(QString::number(en_user.id));
        QByteArray serializedBusinessUserInfo = GetSerializedBusinessUserInfo(en_user.businessUserInfo);
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

    const evernote::edam::Notebook & enNotebook = notebook.en_notebook;

    QSqlQuery query(m_sqlDatabase);
    query.prepare("INSERT OR REPLACE INTO Notebooks (guid, updateSequenceNumber, name, "
                  "creationTimestamp, modificationTimestamp, isDirty, isLocal, "
                  "isDefault, isLastUsed, isPublished, stack) VALUES(?, ?, ?, ?, "
                  "?, ?, ?, ?, ?, ?, ?");
    query.addBindValue(QString::fromStdString(enNotebook.guid));
    query.addBindValue(enNotebook.updateSequenceNum);
    query.addBindValue(QString::fromStdString(enNotebook.name));
    query.addBindValue(QVariant(static_cast<int>(enNotebook.serviceCreated)));
    query.addBindValue(QVariant(static_cast<int>(enNotebook.serviceUpdated)));
    query.addBindValue(notebook.isDirty);
    query.addBindValue(notebook.isLocal);
    query.addBindValue(enNotebook.defaultNotebook);
    query.addBindValue(notebook.isLastUsed);
    query.addBindValue(QVariant(enNotebook.published ? 1 : 0));
    query.addBindValue(QString::fromStdString(enNotebook.stack));

    bool res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("can't insert or replace notebook into \"Notebooks\" table in SQL database");

    return SetNotebookAdditionalAttributes(enNotebook, errorDescription);
}

bool LocalStorageManager::InsertOrReplaceNote(const Note & note, QString & errorDescription)
{
    // NOTE: this method expects to be called after note is already checked
    // for sanity of its parameters!

    errorDescription = QObject::tr("Can't insert or replace note into local storage database: ");

    const evernote::edam::Note & enNote = note.en_note;

    QString guid = QString::fromStdString(enNote.guid);
    QString title = QString::fromStdString(enNote.title);
    QString content = QString::fromStdString(enNote.content);
    QString notebookGuid = QString::fromStdString(enNote.notebookGuid);

    QSqlQuery query(m_sqlDatabase);

    query.prepare("INSERT OR REPLACE INTO Notes (guid, updateSequenceNumber, title, isDirty, "
                  "isLocal, content, creationTimestamp, modificationTimestamp, "
                  "notebookGuid) VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?)");
    query.addBindValue(guid);
    query.addBindValue(enNote.updateSequenceNum);
    query.addBindValue(title);
    query.addBindValue((note.isDirty ? 1 : 0));
    query.addBindValue((note.isLocal ? 1 : 0));
    query.addBindValue(content);
    query.addBindValue(static_cast<qint64>(enNote.created));
    query.addBindValue(static_cast<qint64>(enNote.updated));
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

    if (enNote.__isset.tagGuids)
    {
        QString error;

        const auto & tagGuids = enNote.tagGuids;
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
            query.addBindValue(QString::fromStdString(tagGuid));

            res = query.exec();
            DATABASE_CHECK_AND_SET_ERROR("can't insert or replace information into \"NoteTags\" table in SQL database");
        }
    }

    // NOTE: don't even attempt fo find tags by their names because evernote::edam::Note.tagNames
    // has the only purpose to provide tag names alternatively to guids to NoteStore::createNote method

    if (enNote.__isset.resources)
    {
        const std::vector<evernote::edam::Resource> & resources = enNote.resources;
        size_t numResources = resources.size();
        for(size_t i = 0; i < numResources; ++i)
        {
            // const evernote::edam::Resource & resource = resources[i];
            // TODO: check parameters of this resource

            // TODO: try to find this resource within local storage database,
            // in case of failure report error
        }
    }

    if (!enNote.__isset.attributes) {
        return true;
    }

    return SetNoteAttributes(enNote, errorDescription);
}

bool LocalStorageManager::InsertOrReplaceTag(const Tag & tag, QString & errorDescription)
{
    // NOTE: this method expects to be called after tag is already checked
    // for sanity of its parameters!

    errorDescription = QObject::tr("Can't insert or replace tag into local storage database: ");

    const evernote::edam::Tag & enTag = tag.en_tag;

    QString guid = QString::fromStdString(enTag.guid);
    QString name = QString::fromStdString(enTag.name);
    QString nameUpper = name.toUpper();

    QString parentGuid = (enTag.__isset.parentGuid ? QString::fromStdString(enTag.parentGuid) : QString());

    QSqlQuery query(m_sqlDatabase);

    query.prepare("INSERT OR REPLACE INTO Tags (guid, updateSequenceNumber, name, "
                  "nameUpper, parentGuid, isDirty, isLocal, isDeleted) "
                  "VALUES(?, ?, ?, ?, ?, ?, ?, ?");
    query.addBindValue(guid);
    query.addBindValue(enTag.updateSequenceNum);
    query.addBindValue(name);
    query.addBindValue(nameUpper);
    query.addBindValue(parentGuid);
    query.addBindValue(tag.isDirty ? 1 : 0);
    query.addBindValue(tag.isLocal ? 1 : 0);
    query.addBindValue(tag.isDeleted ? 1 : 0);

    bool res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("can't insert or replace tag into \"Tags\" table in SQL database");

    return true;
}

bool LocalStorageManager::InsertOrReplaceResource(const Resource & resource, QString & errorDescription)
{
    // NOTE: this method expects to be called after resource is already checked
    // for sanity of its parameters!

    errorDescription = QObject::tr("Can't insert or replace resource into local storage database: ");

    const evernote::edam::Resource & enResource = resource.en_resource;

    QSqlQuery query(m_sqlDatabase);
    query.prepare("INSERT OR REPLACE INTO Resources (:columns) VALUES(:vals)");

    QString columns, values;

#define CHECK_AND_SET_RESOURCE_PROPERTY(holder, isSetName, columnName, valueName) \
    if (holder.__isset.isSetName) \
    { \
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

    CHECK_AND_SET_RESOURCE_PROPERTY(enResource, guid, guid, QString::fromStdString(enResource.guid));
    CHECK_AND_SET_RESOURCE_PROPERTY(enResource, noteGuid, noteGuid, QString::fromStdString(enResource.noteGuid));

    bool hasAnyData = enResource.__isset.data || enResource.__isset.alternateData;
    if (hasAnyData)
    {
        const auto & data = (enResource.__isset.alternateData
                             ? enResource.alternateData
                             : enResource.data);
        CHECK_AND_SET_RESOURCE_PROPERTY(data, body, dataBody, QString::fromStdString(data.body));
        CHECK_AND_SET_RESOURCE_PROPERTY(data, size, dataSize, QString::number(data.size));
        CHECK_AND_SET_RESOURCE_PROPERTY(data, bodyHash, dataHash, QString::fromStdString(data.bodyHash));
    }

    CHECK_AND_SET_RESOURCE_PROPERTY(enResource, mime, mime, QString::fromStdString(enResource.mime));
    CHECK_AND_SET_RESOURCE_PROPERTY(enResource, width, width, QString::number(enResource.width));
    CHECK_AND_SET_RESOURCE_PROPERTY(enResource, height, height, QString::number(enResource.height));

    if (enResource.__isset.recognition) {
        CHECK_AND_SET_RESOURCE_PROPERTY(enResource.recognition, body, recognitionBody, QString::fromStdString(enResource.recognition.body));
        CHECK_AND_SET_RESOURCE_PROPERTY(enResource.recognition, size, recognitionSize, QString::number(enResource.recognition.size));
        CHECK_AND_SET_RESOURCE_PROPERTY(enResource.recognition, bodyHash, recognitionHash, QString::fromStdString(enResource.recognition.bodyHash));
    }

    CHECK_AND_SET_RESOURCE_PROPERTY(enResource, updateSequenceNum, updateSequenceNumber, QString::number(enResource.updateSequenceNum));

#undef CHECK_AND_SET_RESOURCE_PROPERTY

    if (!columns.isEmpty()) {
        columns.append(", ");
    }
    columns.append("isDirty");

    if (!values.isEmpty()) {
        values.append(", ");
    }
    values.append(QString::number(resource.isDirty ? 1 : 0));

    query.bindValue("columns", columns);
    query.bindValue("vals", values);

    bool res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("can't insert or replace data into \"Resources\" table in SQL database");

    if (enResource.__isset.attributes)
    {
        QByteArray serializedResourceAttributes = GetSerializedResourceAttributes(enResource.attributes);
        query.prepare("INSERT OR REPLACE INTO ResourceAttributes (guid, data) VALUES(?, ?)");
        query.addBindValue(QString::fromStdString(enResource.guid));
        query.addBindValue(serializedResourceAttributes);

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

    const evernote::edam::SavedSearch & enSearch = search.en_search;

    QSqlQuery query(m_sqlDatabase);
    query.prepare("INSERT OR REPLACE INTO SavedSearches (guid, name, query, format, "
                  "updateSequenceNumber, includeAccount, includePersonalLinkedNotebooks, "
                  "includeBusinessLinkedNotebooks) VALUES(:guid, :name, :query, "
                  ":format, :updateSequenceNumber, :includeAccount, :includePersonalLinkedNotebooks, "
                  ":includeBusinessLinkedNotebooks)");
    query.bindValue("guid", QString::fromStdString(enSearch.guid));
    query.bindValue("name", QString::fromStdString(enSearch.name));
    query.bindValue("query", QString::fromStdString(enSearch.query));
    query.bindValue("format", static_cast<int>(enSearch.format));
    query.bindValue("updateSequenceNumber", static_cast<qint32>(enSearch.updateSequenceNum));
    query.bindValue("includeAccount", (enSearch.scope.includeAccount ? 1 : 0));
    query.bindValue("includePersonalLinkedNotebooks", (enSearch.scope.includePersonalLinkedNotebooks ? 1 : 0));
    query.bindValue("includeBusinessLinkedNotebooks", (enSearch.scope.includeBusinessLinkedNotebooks ? 1 : 0));

    bool res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("can't insert or replace saved search into \"SavedSearches\" table in SQL database");

    return true;
}

bool LocalStorageManager::FindAndSetTagGuidsPerNote(evernote::edam::Note & enNote,
                                                    QString & errorDescription) const
{
    errorDescription += QObject::tr("can't find tag guids per note: ");

    const Guid & guid = enNote.guid;
    if (!CheckGuid(guid)) {
        errorDescription += QObject::tr("note's guid is invalid");
        return false;
    }

    QSqlQuery query(m_sqlDatabase);
    query.prepare("SELECT tag FROM NoteTags WHERE note = ?");
    query.addBindValue(QString::fromStdString(guid));

    bool res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("can't select note tags from \"NoteTags\" table in SQL database");

    auto & tagGuids = enNote.tagGuids;
    tagGuids.clear();
    size_t numRows = query.size();
    tagGuids.reserve(numRows);

    while (query.next())
    {
        Guid tagGuid = query.value(0).toString().toStdString();
        if (!CheckGuid(tagGuid)) {
            errorDescription += QObject::tr("found invalid tag guid for requested note");
            return false;
        }

        tagGuids.push_back(tagGuid);
    }

    return true;
}

bool LocalStorageManager::FindAndSetResourcesPerNote(evernote::edam::Note & enNote,
                                                     QString & errorDescription,
                                                     const bool withBinaryData) const
{
    errorDescription += QObject::tr("can't find resources for note: ");

    const Guid & guid = enNote.guid;
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
    query.bindValue("noteGuid", QString::fromStdString(guid));

    bool res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("can't select resources' guids per note's guid");

    int numResources = query.size();
    auto & resources = enNote.resources;
    resources.resize(numResources);

    if (numResources < 0) {
        // No resources were found
        return true;
    }

    size_t k = 0;
    while(query.next())
    {
        QSqlRecord rec = query.record();

        evernote::edam::Resource & resource = resources[k++];

        CHECK_AND_SET_EN_RESOURCE_PROPERTY(resource, guid, guid, QString, std::string,
                                           /* is required = */ true, .toStdString());
        CHECK_AND_SET_EN_RESOURCE_PROPERTY(resource, updateSequenceNumber, updateSequenceNum,
                                           int, int32_t, /* is required = */ true);
        auto & data = resource.data;

        CHECK_AND_SET_EN_RESOURCE_PROPERTY(data, dataSize, size, int, int32_t,
                                           /* is required = */ true);
        CHECK_AND_SET_EN_RESOURCE_PROPERTY(data, dataHash, bodyHash, QString, std::string,
                                           /* is required = */ true, .toStdString());
        CHECK_AND_SET_EN_RESOURCE_PROPERTY(resource, mime, mime, QString, std::string,
                                           /* is required = */ true, .toStdString());
        CHECK_AND_SET_EN_RESOURCE_PROPERTY(resource, width, width, int, int32_t,
                                           /* is required = */ false);
        CHECK_AND_SET_EN_RESOURCE_PROPERTY(resource, height, height, int, int32_t,
                                           /* is required = */ false);

        auto & recognition = resource.recognition;

        CHECK_AND_SET_EN_RESOURCE_PROPERTY(recognition, recognitionBody, body,
                                           QString, std::string, /* is required = */ false,
                                           .toStdString());
        CHECK_AND_SET_EN_RESOURCE_PROPERTY(recognition, recognitionSize, size,
                                           int, int32_t, /* is required = */ false);
        CHECK_AND_SET_EN_RESOURCE_PROPERTY(recognition, recognitionHash, bodyHash,
                                           QString, std::string, /* is required = */ false,
                                           .toStdString());

        if (withBinaryData) {
            CHECK_AND_SET_EN_RESOURCE_PROPERTY(data, dataBody, body, QString, std::string,
                                               /* is required = */ true, .toStdString());
        }

        // Retrieve optional resource attributes for each resource
        QSqlQuery resourceAttributeQuery(m_sqlDatabase);
        resourceAttributeQuery.prepare("SELECT data FROM ResourceAttributes WHERE guid = ?");
        resourceAttributeQuery.addBindValue(QString::fromStdString(guid));

        res = resourceAttributeQuery.exec();
        DATABASE_CHECK_AND_SET_ERROR("can't select serialized resource attributes from "
                                     "\"ResourceAttributes\" table in SQL database");

        if (!resourceAttributeQuery.next()) {
            // No attributes for this resource
            continue;
        }

        QByteArray serializedResourceAttributes = query.value(0).toByteArray();
        resource.attributes = GetDeserializedResourceAttributes(serializedResourceAttributes);
        resource.__isset.attributes = true;
    }

    return true;
}

bool LocalStorageManager::FindAndSetNoteAttributesPerNote(evernote::edam::Note & enNote,
                                                          QString & errorDescription) const
{
    errorDescription += QObject::tr("can't find note attributes: ");

    if (!enNote.__isset.attributes) {
        return true;
    }

    const Guid & guid = enNote.guid;
    if (!CheckGuid(guid)) {
        errorDescription += QObject::tr("note's guid is invalid");
        return false;
    }

    QSqlQuery query(m_sqlDatabase);
    query.prepare("SELECT data FROM NoteAttributes WHERE noteGuid=?");
    query.addBindValue(QString::fromStdString(guid));

    bool res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("can't select note's attributes from \"NoteAttributes\" table in SQL database");

    if (query.next()) {
        QByteArray data = query.value(0).toByteArray();
        enNote.attributes = std::move(GetDeserializedNoteAttributes(data));
    }
    else {
        errorDescription += QObject::tr("Internal error: SQL query result is empty");
        return false;
    }

    return true;
}

#undef CHECK_AND_SET_EN_RESOURCE_PROPERTY
#undef CHECK_AND_SET_NOTE_PROPERTY
#undef CHECK_AND_SET_EN_NOTE_PROPERTY
#undef DATABASE_CHECK_AND_SET_ERROR

}
