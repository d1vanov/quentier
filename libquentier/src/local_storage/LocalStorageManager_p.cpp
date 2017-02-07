/*
 * Copyright 2016 Dmitry Ivanov
 *
 * This file is part of libquentier
 *
 * libquentier is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * libquentier is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with libquentier. If not, see <http://www.gnu.org/licenses/>.
 */

#include "LocalStorageManager_p.h"
#include <quentier/exception/DatabaseLockedException.h>
#include <quentier/exception/DatabaseLockFailedException.h>
#include <quentier/exception/DatabaseOpeningException.h>
#include <quentier/exception/DatabaseSqlErrorException.h>
#include "Transaction.h"
#include <quentier/local_storage/NoteSearchQuery.h>
#include <quentier/utility/StringUtils.h>
#include <quentier/utility/Utility.h>
#include <quentier/logging/QuentierLogger.h>
#include <quentier/utility/DesktopServices.h>
#include <quentier/utility/SysInfo.h>
#include <quentier/utility/UidGenerator.h>
#include <quentier/types/ResourceRecognitionIndices.h>
#include <algorithm>

namespace quentier {

#define QUENTIER_DATABASE_NAME "qn.storage.sqlite"

LocalStorageManagerPrivate::LocalStorageManagerPrivate(const Account & account, const bool startFromScratch, const bool overrideLock) :
    QObject(),
    // NOTE: don't initialize these! Otherwise SwitchUser won't work right
    m_currentAccount(account),
    m_databaseFilePath(),
    m_sqlDatabase(),
    m_databaseFileLock(),
    m_insertOrReplaceSavedSearchQuery(),
    m_insertOrReplaceSavedSearchQueryPrepared(false),
    m_getSavedSearchCountQuery(),
    m_getSavedSearchCountQueryPrepared(false),
    m_insertOrReplaceResourceQuery(),
    m_insertOrReplaceResourceQueryPrepared(false),
    m_insertOrReplaceNoteResourceQuery(),
    m_insertOrReplaceNoteResourceQueryPrepared(false),
    m_deleteResourceFromResourceRecognitionTypesQuery(),
    m_deleteResourceFromResourceRecognitionTypesQueryPrepared(false),
    m_insertOrReplaceIntoResourceRecognitionDataQuery(),
    m_insertOrReplaceIntoResourceRecognitionDataQueryPrepared(false),
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
    m_getNoteCountQuery(),
    m_getNoteCountQueryPrepared(false),
    m_insertOrReplaceNoteQuery(),
    m_insertOrReplaceNoteQueryPrepared(false),
    m_insertOrReplaceSharedNoteQuery(),
    m_insertOrReplaceSharedNoteQueryPrepared(false),
    m_insertOrReplaceNoteRestrictionsQuery(),
    m_insertOrReplaceNoteRestrictionsQueryPrepared(false),
    m_insertOrReplaceNoteLimitsQuery(),
    m_insertOrReplaceNoteLimitsQueryPrepared(false),
    m_canAddNoteToNotebookQuery(),
    m_canAddNoteToNotebookQueryPrepared(false),
    m_canUpdateNoteInNotebookQuery(),
    m_canUpdateNoteInNotebookQueryPrepared(false),
    m_canExpungeNoteInNotebookQuery(),
    m_canExpungeNoteInNotebookQueryPrepared(false),
    m_insertOrReplaceNoteIntoNoteTagsQuery(),
    m_insertOrReplaceNoteIntoNoteTagsQueryPrepared(false),
    m_getLinkedNotebookCountQuery(),
    m_getLinkedNotebookCountQueryPrepared(false),
    m_insertOrReplaceLinkedNotebookQuery(),
    m_insertOrReplaceLinkedNotebookQueryPrepared(false),
    m_getNotebookCountQuery(),
    m_getNotebookCountQueryPrepared(false),
    m_insertOrReplaceNotebookQuery(),
    m_insertOrReplaceNotebookQueryPrepared(false),
    m_insertOrReplaceNotebookRestrictionsQuery(),
    m_insertOrReplaceNotebookRestrictionsQueryPrepared(false),
    m_insertOrReplaceSharedNotebookQuery(),
    m_insertOrReplaceSharedNotebookQueryPrepared(false),
    m_getUserCountQuery(),
    m_getUserCountQueryPrepared(false),
    m_insertOrReplaceUserQuery(),
    m_insertOrReplaceUserQueryPrepared(false),
    m_insertOrReplaceUserAttributesQuery(),
    m_insertOrReplaceUserAttributesQueryPrepared(false),
    m_insertOrReplaceAccountingQuery(),
    m_insertOrReplaceAccountingQueryPrepared(false),
    m_insertOrReplaceAccountLimitsQuery(),
    m_insertOrReplaceAccountLimitsQueryPrepared(false),
    m_insertOrReplaceBusinessUserInfoQuery(),
    m_insertOrReplaceBusinessUserInfoQueryPrepared(false),
    m_insertOrReplaceUserAttributesViewedPromotionsQuery(),
    m_insertOrReplaceUserAttributesViewedPromotionsQueryPrepared(false),
    m_insertOrReplaceUserAttributesRecentMailedAddressesQuery(),
    m_insertOrReplaceUserAttributesRecentMailedAddressesQueryPrepared(false),
    m_deleteUserQuery(),
    m_deleteUserQueryPrepared(false),
    m_stringUtils(),
    m_preservedAsterisk()
{
    m_preservedAsterisk.reserve(1);
    m_preservedAsterisk.push_back('*');

    switchUser(account, startFromScratch, overrideLock);
}

LocalStorageManagerPrivate::~LocalStorageManagerPrivate()
{
    if (m_sqlDatabase.isOpen()) {
        m_sqlDatabase.close();
    }

    unlockDatabaseFile();
}

#define DATABASE_CHECK_AND_SET_ERROR() \
    if (!res) { \
        errorDescription.base() = errorPrefix.base(); \
        errorDescription.details() = query.lastError().text(); \
        QNCRITICAL(errorDescription << QStringLiteral(", last executed query: ") << lastExecutedQuery(query)); \
        query.finish(); \
        return false; \
    }

bool LocalStorageManagerPrivate::addUser(const User & user, ErrorString & errorDescription)
{
    ErrorString errorPrefix(QT_TRANSLATE_NOOP("", "Can't insert user data into the local storage database"));

    ErrorString error;
    bool res = user.checkParameters(error);
    if (!res) {
        errorDescription.base() = errorPrefix.base();
        errorDescription.additionalBases().append(error.base());
        errorDescription.additionalBases().append(error.additionalBases());
        errorDescription.details() = error.details();
        QNWARNING(QStringLiteral("Found invalid user: ") << user << QStringLiteral("\nError: ") << error);
        return false;
    }

    QString userId = QString::number(user.id());
    bool exists = rowExists(QStringLiteral("Users"), QStringLiteral("id"), userId);
    if (exists) {
        errorDescription.base() = errorPrefix.base()
        errorDescription.additionalBases().append(QT_TRANSLATE_NOOP("", "user with the same id already exists"));
        errorDescription.details() = userId;
        QNWARNING(errorDescription << QStringLiteral(", id: ") << userId);
        return false;
    }

    error.clear();
    res = insertOrReplaceUser(user, error);
    if (!res) {
        errorDescription.base() = errorPrefix.base();
        errorDescription.additionalBases().append(error.base());
        errorDescription.additionalBases().append(error.additionalBases());
        errorDescription.details() = error.details();
        QNWARNING(errorDescription);
        return false;
    }

    return true;
}

bool LocalStorageManagerPrivate::updateUser(const User & user, ErrorString & errorDescription)
{
    ErrorString errorPrefix(QT_TRANSLATE_NOOP("", "Can't update user data in the local storage database"));

    ErrorString error;
    bool res = user.checkParameters(error);
    if (!res) {
        errorDescription.base() = errorPrefix.base();
        errorDescription.additionalBases().append(error.base());
        errorDescription.additionalBases().append(error.additionalBases());
        errorDescription.details() = error.details();
        QNWARNING(QStringLiteral("Found invalid user: ") << user << QStringLiteral("\nError: ") << error);
        return false;
    }

    QString userId = QString::number(user.id());
    bool exists = rowExists(QStringLiteral("Users"), QStringLiteral("id"), userId);
    if (!exists) {
        errorDescription.base() = errorPrefix.base();
        errorDescription.additionalBases().append(QT_TRANSLATE_NOOP("", "user with the specified id was not found"));
        errorDescription.details() = userId;
        QNWARNING(errorDescription << QStringLiteral(", id: ") << userId);
        return false;
    }

    error.clear();
    res = insertOrReplaceUser(user, error);
    if (!res) {
        errorDescription.base() = errorPrefix.base();
        errorDescription.additionalBases().append(error.base());
        errorDescription.additionalBases().append(error.additionalBases());
        errorDescription.details() = error.details();
        QNWARNING(errorDescription);
        return false;
    }

    return true;
}

bool LocalStorageManagerPrivate::findUser(User & user, ErrorString & errorDescription) const
{
    QNDEBUG(QStringLiteral("LocalStorageManagerPrivate::findUser"));

    ErrorString errorPrefix(QT_TRANSLATE_NOOP("", "Can't find user data in the local storage database"));

    if (!user.hasId()) {
        errorDescription.base() = errorPrefix.base();
        errorDescription.additionalBases().append(QT_TRANSLATE_NOOP("", "user id is not set"));;
        QNWARNING(errorDescription);
        return false;
    }

    qint32 id = user.id();
    QString userId = QString::number(id);
    QNDEBUG(QStringLiteral("Looking for user with id = ") << userId);

    QString queryString = QStringLiteral("SELECT * FROM Users LEFT OUTER JOIN UserAttributes "
                                         "ON Users.id = UserAttributes.id "
                                         "LEFT OUTER JOIN UserAttributesViewedPromotions "
                                         "ON Users.id = UserAttributesViewedPromotions.id "
                                         "LEFT OUTER JOIN UserAttributesRecentMailedAddresses "
                                         "ON Users.id = UserAttributesRecentMailedAddresses.id "
                                         "LEFT OUTER JOIN Accounting ON Users.id = Accounting.id "
                                         "LEFT OUTER JOIN AccountLimits ON Users.id = AccountLimits.id "
                                         "LEFT OUTER JOIN BusinessUserInfo ON Users.id = BusinessUserInfo.id "
                                         "WHERE Users.id = :id");
    QSqlQuery query(m_sqlDatabase);
    bool res = query.prepare(queryString);
    DATABASE_CHECK_AND_SET_ERROR();

    query.bindValue(QStringLiteral(":id"), userId);

    res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR();

    size_t counter = 0;
    while(query.next()) {
        QSqlRecord rec = query.record();
        res = fillUserFromSqlRecord(rec, user, errorDescription);
        if (!res) {
            return false;
        }
        ++counter;
    }

    if (!counter) {
        QNDEBUG(errorDescription);
        return false;
    }

    return true;
}

bool LocalStorageManagerPrivate::deleteUser(const User & user, ErrorString & errorDescription)
{
    ErrorString errorPrefix(QT_TRANSLATE_NOOP("", "Can't mark the user data as deleted in the local storage database"));

    if (!user.hasDeletionTimestamp()) {
        errorDescription.base() = errorPrefix.base();
        errorDescription.additionalBases().append(QT_TRANSLATE_NOOP("", "deletion timestamp is not set"));
        QNWARNING(errorDescription);
        return false;
    }

    if (!user.hasId()) {
        errorDescription.base() = errorPrefix.base();
        errorDescription.additionalBases().append(QT_TRANSLATE_NOOP("", "user id is not set"));
        QNWARNING(errorDescription);
        return false;
    }

    bool res = checkAndPrepareDeleteUserQuery();
    QSqlQuery & query = m_deleteUserQuery;
    DATABASE_CHECK_AND_SET_ERROR();

    query.bindValue(QStringLiteral(":userDeletionTimestamp"), user.deletionTimestamp());
    query.bindValue(QStringLiteral(":userIsLocal"), (user.isLocal() ? 1 : 0));
    query.bindValue(QStringLiteral(":id"), user.id());

    res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR();

    query.finish();
    return true;
}

bool LocalStorageManagerPrivate::expungeUser(const User & user, ErrorString & errorDescription)
{
    ErrorString errorPrefix(QT_TRANSLATE_NOOP("", "Can't expunge user data from the local storage database"));

    if (!user.hasId()) {
        errorDescription.base() = errorPrefix.base();
        errorDescription.additionalBases().append(QT_TRANSLATE_NOOP("", "user id is not set"));
        QNWARNING(errorDescription);
        return false;
    }

    QString queryString = QStringLiteral("DELETE FROM Users WHERE id=:id");
    QSqlQuery query(m_sqlDatabase);
    bool res = query.prepare(queryString);
    DATABASE_CHECK_AND_SET_ERROR();

    qevercloud::UserID id = user.id();
    QString userId = QString::number(id);
    query.bindValue(QStringLiteral(":id"), userId);

    res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR();

    return true;
}

#define SET_ERROR() \
    errorDescription.base() = errorPrefix.base(); \
    errorDescription.details() = query.lastError().text(); \
    QNCRITICAL(errorDescription << QStringLiteral(", last query: ") << lastExecutedQuery(query))

#define SET_INT_CONVERSION_ERROR() \
    errorDescription.base() = errorPrefix.base(); \
    errorDescription.additionalBases().append(QT_TRANSLATE_NOOP("", "can't convert the fetched data to int")); \
    QNCRITICAL(errorDescription << QStringLiteral(": ") << query.value(0))

#define SET_NO_DATA_FOUND() \
    errorDescription.base() = errorPrefix.base(); \
    errorDescription.additionalBases().append(QT_TRANSLATE_NOOP("", "no data found")); \
    QNWARNING(errorDescription)

int LocalStorageManagerPrivate::notebookCount(ErrorString & errorDescription) const
{
    ErrorString errorPrefix(QT_TRANSLATE_NOOP("", "Can't get the number of notebooks from the local storage database"));

    bool res = checkAndPrepareNotebookCountQuery();
    QSqlQuery & query = m_getNotebookCountQuery;
    if (!res) {
        SET_ERROR();
        query.finish();
        return -1;
    }

    res = query.exec();
    if (!res) {
        SET_ERROR();
        query.finish();
        return -1;
    }

    if (!query.next()) {
        QNDEBUG(QStringLiteral("Found no notebooks in local storage database"));
        query.finish();
        return 0;
    }

    bool conversionResult = false;
    int count = query.value(0).toInt(&conversionResult);
    query.finish();

    if (!conversionResult) {
        SET_INT_CONVERSION_ERROR();
        return -1;
    }

    return count;
}

void LocalStorageManagerPrivate::switchUser(const Account & account,
                                            const bool startFromScratch, const bool overrideLock)
{
    QNDEBUG(QStringLiteral("LocalStorageManagerPrivate::switchUser: start from scratch = ")
            << (startFromScratch ? QStringLiteral("true") : QStringLiteral("false"))
            << QStringLiteral(", override lock = ") << (overrideLock ? QStringLiteral("true") : QStringLiteral("false"))
            << QStringLiteral(", account: ") << account);

    if (!m_databaseFilePath.isEmpty() &&
        (m_currentAccount.type() == account.type()) &&
        (m_currentAccount.name() == account.name()) &&
        (m_currentAccount.id() == account.id()))
    {
        QNDEBUG(QStringLiteral("The same account and it has already been initialized once, skipping"));
        return;
    }

    // Unlocking the previous database file, if any
    unlockDatabaseFile();

    m_currentAccount = account;

    QString sqlDriverName = QStringLiteral("QSQLITE");
    bool isSqlDriverAvailable = QSqlDatabase::isDriverAvailable(sqlDriverName);
    if (!isSqlDriverAvailable)
    {
        ErrorString error(QT_TRANSLATE_NOOP("", "SQLite driver is not available"));
        error.details() = QStringLiteral("Available SQL drivers: ");
        for(auto it = drivers.begin(), end = drivers.end(); it != end; ++it) {
            const QString & driver = *it;
            error.details() += QString(QStringLiteral("{") + driver + QStringLiteral("} "));
        }
        throw DatabaseSqlErrorException(error);
    }

    QString sqlDatabaseConnectionName = QStringLiteral("quentier_sqlite_connection");
    if (!QSqlDatabase::contains(sqlDatabaseConnectionName)) {
        m_sqlDatabase = QSqlDatabase::addDatabase(sqlDriverName, sqlDatabaseConnectionName);
    }
    else {
        m_sqlDatabase = QSqlDatabase::database(sqlDatabaseConnectionName);
    }

    QString accountName = account.name();
    if (Q_UNLIKELY(accountName.isEmpty())) {
        ErrorString error(QT_TRANSLATE_NOOP("", "Can't initialize local storage: account name is empty"));
        throw DatabaseOpeningException(error);
    }

    m_databaseFilePath = applicationPersistentStoragePath();
    if (account.type() == Account::Type::Local)
    {
        m_databaseFilePath += QStringLiteral("/LocalAccounts/");
        m_databaseFilePath += accountName;
    }
    else
    {
        m_databaseFilePath += QStringLiteral("/EvernoteAccounts/");
        m_databaseFilePath += accountName;
        m_databaseFilePath += QStringLiteral("_");
        m_databaseFilePath += account.evernoteHost();
        m_databaseFilePath += QStringLiteral("_");
        m_databaseFilePath += QString::number(account.id());
    }

    m_databaseFilePath += QStringLiteral("/") + QStringLiteral(QUENTIER_DATABASE_NAME);
    QNDEBUG(QStringLiteral("Attempting to open or create database file: ") + m_databaseFilePath);

    QFileInfo databaseFileInfo(m_databaseFilePath);

    QDir databaseFileDir = databaseFileInfo.absoluteDir();
    if (Q_UNLIKELY(!databaseFileDir.exists()))
    {
        bool res = databaseFileDir.mkpath(databaseFileDir.absolutePath());
        if (!res) {
            ErrorString error(QT_TRANSLATE_NOOP("", "Can't create the folder for local storage database file"));
            throw DatabaseOpeningException(error);
        }
    }

    if (databaseFileInfo.exists())
    {
        if (Q_UNLIKELY(!databaseFileInfo.isReadable())) {
            ErrorString error(QT_TRANSLATE_NOOP("", "Local storage database file is not readable"));
            error.details() = m_databaseFilePath;
            throw DatabaseOpeningException(error);
        }

        if (Q_UNLIKELY(!databaseFileInfo.isWritable())) {
            ErrorString error(QT_TRANSLATE_NOOP("", "Local storage database file is not writable"));
            error.details() = m_databaseFilePath;
            throw DatabaseOpeningException(error);
        }
    }
    else
    {
        // The file needs to exist in order to lock it
        clearDatabaseFile();
    }

    // NOTE: it appears boost::interprocess::file_lock applied to the database file on Windows
    // causes the inability to properly open the database. The reason for this is not clear
    // so for now just disable the use of boost::interprocess::file_lock on Windows.
    // It's not a major problem because Windows won't let another process to open the file
    // being worked with by another process
#ifndef Q_OS_WIN
    // WARNING: something strange is going on here: if no call is made to the below method,
    // boost::interprocess::file_lock occasionally and sporadically thinks "there is no such file
    // or directory"; that's what its exception message says
    bool databaseFileExists = databaseFileInfo.exists();
    QNDEBUG(QStringLiteral("Database file exists before locking: ")
            << (databaseFileExists ? QStringLiteral("true") : QStringLiteral("false")));

    bool lockResult = false;

    try {
        boost::interprocess::file_lock databaseLock(databaseFileInfo.canonicalFilePath().toUtf8().constData());
        m_databaseFileLock.swap(databaseLock);
        lockResult = m_databaseFileLock.try_lock();
    }
    catch(boost::interprocess::interprocess_exception & exc) {
        ErrorString error(QT_TRANSLATE_NOOP("", "Can't lock the database file"));
        error.details() = QStringLiteral("error code ");
        error.details() += QString::number(exc.get_error_code());
        error.details() += QStringLiteral("; ");
        error.details() += QString::fromUtf8(exc.what());
        throw DatabaseLockFailedException(error);
    }

    if (!lockResult)
    {
        if (!overrideLock) {
            ErrorString error(QT_TRANSLATE_NOOP("", "Local storage database file is locked"));
            error.details() = m_databaseFilePath;
            throw DatabaseLockedException(error);
        }
        else {
            QNINFO(QStringLiteral("Local storage database file ") << m_databaseFilePath
                   << QStringLiteral(" is locked but nobody cares"));
        }
    }
#endif

    if (startFromScratch) {
        QNDEBUG(QStringLiteral("Cleaning up the whole database for account: ") << m_currentAccount);
        clearDatabaseFile();
    }

    m_sqlDatabase.setHostName(QStringLiteral("localhost"));
    m_sqlDatabase.setUserName(accountName);
    m_sqlDatabase.setPassword(accountName);
    m_sqlDatabase.setDatabaseName(m_databaseFilePath);

    if (!m_sqlDatabase.open()) {
        QString lastErrorText = m_sqlDatabase.lastError().text();
        ErrorString error(QT_TRANSLATE_NOOP("", "Can't connect to the local storage database"));
        error.details() = lastErrorText;
        throw DatabaseOpeningException(error);
    }

    QSqlQuery query(m_sqlDatabase);
    if (!query.exec(QStringLiteral("PRAGMA foreign_keys = ON"))) {
        QString lastErrorText = m_sqlDatabase.lastError().text();
        ErrorString error(QT_TRANSLATE_NOOP("", "Can't set foreign_keys = ON pragma for the local storage database"));
        error.details() = lastErrorText;
        throw DatabaseSqlErrorException(error);
    }

    SysInfo sysInfo;
    qint64 pageSize = sysInfo.pageSize();
    QString pageSizeQuery = QString("PRAGMA page_size = %1").arg(QString::number(pageSize));
    if (!query.exec(pageSizeQuery)) {
        QString lastErrorText = m_sqlDatabase.lastError().text();
        ErrorString error(QT_TRANSLATE_NOOP("", "Can't set page_size pragma for the local storage database"));
        error.details() = lastErrorText;
        throw DatabaseSqlErrorException(error);
    }

    QString writeAheadLoggingQuery = QStringLiteral("PRAGMA journal_mode=WAL");
    if (!query.exec(writeAheadLoggingQuery)) {
        QString lastErrorText = m_sqlDatabase.lastError().text();
        ErrorString error(QT_TRANSLATE_NOOP("", "Can't set journal_mode pragma to WAL for the local storage database"));
        error.details() = lastErrorText;
        throw DatabaseSqlErrorException(error);
    }

    ErrorString errorDescription;
    if (!createTables(errorDescription)) {
        ErrorString error(QT_TRANSLATE_NOOP("", "Can't initialize tables in the local storage database"));
        error.additionalBases().append(errorDescription.base());
        error.additionalBases().append(errorDescription.additionalBases());
        error.details() = errorDescription.details();
        throw DatabaseSqlErrorException(error);
    }

    // TODO: in future should check whether the upgrade from previous database version is necessary
}

int LocalStorageManagerPrivate::userCount(ErrorString & errorDescription) const
{
    ErrorString errorPrefix(QT_TRANSLATE_NOOP("", "Can't get the number of users within the local storage database"));

    bool res = checkAndPrepareUserCountQuery();
    QSqlQuery & query = m_getUserCountQuery;
    if (!res) {
        SET_ERROR();
        query.finish();
        return -1;
    }

    res = query.exec();
    if (!res) {
        SET_ERROR();
        query.finish();
        return -1;
    }

    if (!query.next()) {
        QNDEBUG(QStringLiteral("Found no users in local storage database"));
        query.finish();
        return 0;
    }

    bool conversionResult = false;
    int count = query.value(0).toInt(&conversionResult);
    query.finish();

    if (!conversionResult) {
        SET_INT_CONVERSION_ERROR();
        return -1;
    }

    return count;
}

bool LocalStorageManagerPrivate::addNotebook(Notebook & notebook, ErrorString & errorDescription)
{
    ErrorString errorPrefix(QT_TRANSLATE_NOOP("", "Can't insert notebook data into the local storage database"));

    ErrorString error;
    bool res = notebook.checkParameters(error);
    if (!res) {
        errorDescription.base() = errorPrefix.base();
        errorDescription.additionalBases().append(error.base());
        errorDescription.additionalBases().append(error.additionalBases());
        errorDescription.details() = error.details();
        QNWARNING(QStringLiteral("Found invalid notebook: ") << notebook << QStringLiteral("\nError: ") << error);
        return false;
    }

    QString localUid = notebook.localUid();

    QString column, uid;
    bool shouldCheckRowExistence = true;

    bool notebookHasGuid = notebook.hasGuid();
    if (notebookHasGuid)
    {
        column = QStringLiteral("guid");
        uid = notebook.guid();

        if (!checkGuid(uid)) {
            errorDescription.base() = errorPrefix.base();
            errorDescription.additionalBases().append(QT_TRANSLATE_NOOP("", "notebook guid is invalid"));
            errorDescription.details() = uid;
            QNWARNING(errorDescription);
            return false;
        }

        if (localUid.isEmpty())
        {
            ErrorString error;
            bool res = getNotebookLocalUidForGuid(uid, localUid, error);
            if (res || !localUid.isEmpty()) {
                errorDescription.base() = errorPrefix.base();
                errorDescription.additionalBases().append(QT_TRANSLATE_NOOP("", "found existing notebook "
                                                                            "corresponding to the added notebook by guid"));
                errorDescription.details() = uid;
                QNWARNING(errorDescription);
                return false;
            }

            localUid = UidGenerator::Generate();
            notebook.setLocalUid(localUid);
            shouldCheckRowExistence = false;
        }
    }
    else
    {
        column = QStringLiteral("localUid");
        uid = localUid;
    }

    if (shouldCheckRowExistence && rowExists(QStringLiteral("Notebooks"), column, QVariant(uid))) {
        errorDescription.base() = errorPrefix.base();
        errorDescription.additionalBases().append(QT_TRANSLATE_NOOP("", "notebook already exists"));
        errorDescription.details() = column;
        errorDescription.details() += QStringLiteral(" = ");
        errorDescription.details() += uid;
        QNWARNING(errorDescription);
        return false;
    }

    error.clear();
    res = insertOrReplaceNotebook(notebook, error);
    if (!res) {
        errorDescription.base() = errorPrefix.base();
        errorDescription.additionalBases().append(error.base());
        errorDescription.additionalBases().append(error.additionalBases());
        errorDescription.details() = error.details();
        QNWARNING(errorDescription);
        return false;
    }

    return true;
}

bool LocalStorageManagerPrivate::updateNotebook(Notebook & notebook, ErrorString & errorDescription)
{
    ErrorString errorPrefix(QT_TRANSLATE_NOOP("", "Can't update notebook data in the local storage database"));

    ErrorString error;
    bool res = notebook.checkParameters(error);
    if (!res) {
        errorDescription.base() = errorPrefix.base();
        errorDescription.additionalBases().append(error.base());
        errorDescription.additionalBases().append(error.additionalBases());
        errorDescription.details() = error.details();
        QNWARNING(QStringLiteral("Found invalid notebook: ") << notebook << QStringLiteral("\nError: ") << error);
        return false;
    }

    QString localUid = notebook.localUid();

    QString column, uid;
    bool shouldCheckRowExistence = true;

    bool notebookHasGuid = notebook.hasGuid();
    if (notebookHasGuid)
    {
        column = QStringLiteral("guid");
        uid = notebook.guid();

        if (!checkGuid(uid)) {
            errorDescription.base() = errorPrefix.base();
            errorDescription.additionalBases().append(QT_TRANSLATE_NOOP("", "notebook guid is invalid"));
            errorDescription.details() = uid;
            QNWARNING(errorDescription);
            return false;
        }

        if (localUid.isEmpty())
        {
            ErrorString error;
            res = getNotebookLocalUidForGuid(uid, localUid, error);
            if (!res || localUid.isEmpty()) {
                errorDescription.base() = errorPrefix.base();
                errorDescription.additionalBases().append(error.base());
                errorDescription.additionalBases().append(error.additionalBases());
                errorDescription.details() = error.details();
                QNWARNING(errorDescription);
                return false;
            }

            notebook.setLocalUid(localUid);
            shouldCheckRowExistence = false;
        }
    }
    else
    {
        column = QStringLiteral("localUid");
        uid = localUid;
    }

    if (shouldCheckRowExistence && !rowExists(QStringLiteral("Notebooks"), column, uid))
    {
        errorDescription.base() = errorPrefix.base();
        errorDescription.additionalBases().append(QT_TRANSLATE_NOOP("", "notebook to be updated was not found in local storage"));
        errorDescription.details() = column;
        errorDescription.details() += QStringLiteral(" = ");
        errorDescription.details() += uid;
        QNWARNING(errorDescription);
        return false;
    }

    error.clear();
    res = insertOrReplaceNotebook(notebook, error);
    if (!res) {
        errorDescription.base() = errorPrefix.base();
        errorDescription.additionalBases().append(error.base());
        errorDescription.additionalBases().append(error.additionalBases());
        errorDescription.details() = error.details();
        QNWARNING(errorDescription);
        return false;
    }

    return true;
}

bool LocalStorageManagerPrivate::findNotebook(Notebook & notebook, ErrorString & errorDescription) const
{
    ErrorString errorPrefix(QT_TRANSLATE_NOOP("", "Can't find notebook in the local storage database"));

    QString column, value;
    bool notebookHasGuid = notebook.hasGuid();
    if (notebookHasGuid)
    {
        column = QStringLiteral("guid");
        value = notebook.guid();

        if (!checkGuid(value)) {
            errorDescription.base() = errorPrefix.base();
            errorDescription.additionalBases().append(QT_TRANSLATE_NOOP("", "notebook guid is invalid"));
            QNWARNING(errorDescription);
            return false;
        }
    }
    else if (notebook.localUid().isEmpty())
    {
        if (!notebook.hasName()) {
            errorDescription.base() = errorPrefix.base();
            errorDescription.additionalBases().append(QT_TRANSLATE_NOOP("", "need either guid or local uid or name as search criteria"));
            QNWARNING(errorDescription);
            return false;
        }

        column = QStringLiteral("notebookNameUpper");
        value = notebook.name().toUpper();
    }
    else
    {
        column = QStringLiteral("localUid");
        value = notebook.localUid();
    }

    value = sqlEscapeString(value);

    QString queryString = QString("SELECT * FROM Notebooks LEFT OUTER JOIN NotebookRestrictions "
                                  "ON Notebooks.localUid = NotebookRestrictions.localUid "
                                  "LEFT OUTER JOIN SharedNotebooks ON Notebooks.guid = SharedNotebooks.sharedNotebookNotebookGuid "
                                  "LEFT OUTER JOIN Users ON Notebooks.contactId = Users.id "
                                  "LEFT OUTER JOIN UserAttributes ON Notebooks.contactId = UserAttributes.id "
                                  "LEFT OUTER JOIN UserAttributesViewedPromotions ON Notebooks.contactId = UserAttributesViewedPromotions.id "
                                  "LEFT OUTER JOIN UserAttributesRecentMailedAddresses ON Notebooks.contactId = UserAttributesRecentMailedAddresses.id "
                                  "LEFT OUTER JOIN Accounting ON Notebooks.contactId = Accounting.id "
                                  "LEFT OUTER JOIN AccountLimits ON Notebooks.contactId = AccountLimits.id "
                                  "LEFT OUTER JOIN BusinessUserInfo ON Notebooks.contactId = BusinessUserInfo.id "
                                  "WHERE (Notebooks.%1 = '%2'").arg(column,value);

    if (notebook.hasLinkedNotebookGuid()) {
        QString linkedNotebookGuid = sqlEscapeString(notebook.linkedNotebookGuid());
        queryString += QString(" AND Notebooks.linkedNotebookGuid = '%1')").arg(linkedNotebookGuid);
    }
    else {
        queryString += QStringLiteral(" AND Notebooks.linkedNotebookGuid IS NULL)");
    }

    notebook = Notebook();

    QSqlQuery query(m_sqlDatabase);
    bool res = query.exec(queryString);
    DATABASE_CHECK_AND_SET_ERROR();

    size_t counter = 0;
    while(query.next())
    {
        QSqlRecord rec = query.record();
        ErrorString error;
        res = fillNotebookFromSqlRecord(rec, notebook, error);
        if (!res) {
            errorDescription.base() = errorPrefix.base();
            errorDescription.additionalBases().append(error.base());
            errorDescription.additionalBases().append(error.additionalBases());
            errorDescription.details() = error.details();
            QNWARNING(errorDescription);
            return false;
        }

        ++counter;
    }

    if (!counter) {
        QNDEBUG(errorDescription);
        return false;
    }

    sortSharedNotebooks(notebook);
    return true;
}

bool LocalStorageManagerPrivate::findDefaultNotebook(Notebook & notebook, ErrorString & errorDescription) const
{
    ErrorString errorPrefix(QT_TRANSLATE_NOOP("", "Can't find the default notebook in the local storage database"));

    notebook = Notebook();
    QSqlQuery query(m_sqlDatabase);
    bool res = query.exec(QStringLiteral("SELECT * FROM Notebooks LEFT OUTER JOIN NotebookRestrictions "
                                         "ON Notebooks.localUid = NotebookRestrictions.localUid "
                                         "LEFT OUTER JOIN SharedNotebooks ON Notebooks.guid = SharedNotebooks.sharedNotebookNotebookGuid "
                                         "LEFT OUTER JOIN Users ON Notebooks.contactId = Users.id "
                                         "LEFT OUTER JOIN UserAttributes ON Notebooks.contactId = UserAttributes.id "
                                         "LEFT OUTER JOIN UserAttributesViewedPromotions ON Notebooks.contactId = UserAttributesViewedPromotions.id "
                                         "LEFT OUTER JOIN UserAttributesRecentMailedAddresses ON Notebooks.contactId = UserAttributesRecentMailedAddresses.id "
                                         "LEFT OUTER JOIN Accounting ON Notebooks.contactId = Accounting.id "
                                         "LEFT OUTER JOIN AccountLimits ON Notebooks.contactId = AccountLimits.id "
                                         "LEFT OUTER JOIN BusinessUserInfo ON Notebooks.contactId = BusinessUserInfo.id "
                                         "WHERE isDefault = 1 LIMIT 1"));
    DATABASE_CHECK_AND_SET_ERROR();

    if (!query.next()) {
        errorDescription.base() = errorPrefix.base();
        errorDescription.additionalBases().append(QT_TRANSLATE_NOOP("", "no default notebook was found"));
        QNDEBUG(errorDescription);
        return false;
    }

    QSqlRecord rec = query.record();
    ErrorString error;
    res = fillNotebookFromSqlRecord(rec, notebook, error);
    if (!res) {
        errorDescription.base() = errorPrefix.base();
        errorDescription.additionalBases().append(error.base());
        errorDescription.additionalBases().append(error.additionalBases());
        errorDescription.details() = error.details();
        QNWARNING(errorDescription);
        return false;
    }

    sortSharedNotebooks(notebook);
    return true;
}

bool LocalStorageManagerPrivate::findLastUsedNotebook(Notebook & notebook, ErrorString & errorDescription) const
{
    ErrorString errorPrefix(QT_TRANSLATE_NOOP("", "Can't find the last used notebook in the local storage database"));

    notebook = Notebook();
    QSqlQuery query(m_sqlDatabase);
    bool res = query.exec(QStringLiteral("SELECT * FROM Notebooks LEFT OUTER JOIN NotebookRestrictions "
                                         "ON Notebooks.localUid = NotebookRestrictions.localUid "
                                         "LEFT OUTER JOIN SharedNotebooks ON Notebooks.guid = SharedNotebooks.sharedNotebookNotebookGuid "
                                         "LEFT OUTER JOIN Users ON Notebooks.contactId = Users.id "
                                         "LEFT OUTER JOIN UserAttributes ON Notebooks.contactId = UserAttributes.id "
                                         "LEFT OUTER JOIN UserAttributesViewedPromotions ON Notebooks.contactId = UserAttributesViewedPromotions.id "
                                         "LEFT OUTER JOIN UserAttributesRecentMailedAddresses ON Notebooks.contactId = UserAttributesRecentMailedAddresses.id "
                                         "LEFT OUTER JOIN Accounting ON Notebooks.contactId = Accounting.id "
                                         "LEFT OUTER JOIN AccountLimits ON Notebooks.contactId = AccountLimits.id "
                                         "LEFT OUTER JOIN BusinessUserInfo ON Notebooks.contactId = BusinessUserInfo.id "
                                         "WHERE isLastUsed = 1 LIMIT 1"));
    DATABASE_CHECK_AND_SET_ERROR();

    if (!query.next()) {
        errorDescription.base() = errorPrefix.base();
        errorDescription.additionalBases().append(QT_TRANSLATE_NOOP("", "no last used notebook exists in local storage"));
        QNDEBUG(errorDescription);
        return false;
    }

    QSqlRecord rec = query.record();
    ErrorString error;
    res = fillNotebookFromSqlRecord(rec, notebook, error);
    if (!res) {
        errorDescription.base() = errorPrefix.base();
        errorDescription.additionalBases().append(error.base());
        errorDescription.additionalBases().append(error.additionalBases());
        errorDescription.details() = error.details();
        QNWARNING(errorDescription);
        return false;
    }

    sortSharedNotebooks(notebook);
    return true;
}

bool LocalStorageManagerPrivate::findDefaultOrLastUsedNotebook(Notebook & notebook, ErrorString & errorDescription) const
{
    bool res = findDefaultNotebook(notebook, errorDescription);
    if (res) {
        return true;
    }

    return findLastUsedNotebook(notebook, errorDescription);
}

QList<Notebook> LocalStorageManagerPrivate::listAllNotebooks(ErrorString & errorDescription,
                                                             const size_t limit, const size_t offset,
                                                             const LocalStorageManager::ListNotebooksOrder::type & order,
                                                             const LocalStorageManager::OrderDirection::type & orderDirection,
                                                             const QString & linkedNotebookGuid) const
{
    QNDEBUG(QStringLiteral("LocalStorageManagerPrivate::listAllNotebooks"));
    return listNotebooks(LocalStorageManager::ListAll, errorDescription, limit,
                         offset, order, orderDirection, linkedNotebookGuid);
}

QList<Notebook> LocalStorageManagerPrivate::listNotebooks(const LocalStorageManager::ListObjectsOptions flag,
                                                          ErrorString & errorDescription, const size_t limit,
                                                          const size_t offset, const LocalStorageManager::ListNotebooksOrder::type & order,
                                                          const LocalStorageManager::OrderDirection::type & orderDirection,
                                                          const QString & linkedNotebookGuid) const
{
    QNDEBUG(QStringLiteral("LocalStorageManagerPrivate::listNotebooks: flag = ") << flag);

    QString linkedNotebookGuidSqlQueryCondition = (linkedNotebookGuid.isEmpty()
                                                   ? QStringLiteral("linkedNotebookGuid IS NULL")
                                                   : QString("linkedNotebookGuid = '%1'").arg(sqlEscapeString(linkedNotebookGuid)));

    return listObjects<Notebook, LocalStorageManager::ListNotebooksOrder::type>(flag, errorDescription, limit,
                                                                                offset, order, orderDirection,
                                                                                linkedNotebookGuidSqlQueryCondition);
}

QList<SharedNotebook> LocalStorageManagerPrivate::listAllSharedNotebooks(ErrorString & errorDescription) const
{
    QNDEBUG(QStringLiteral("LocalStorageManagerPrivate::listAllSharedNotebooks"));

    QList<SharedNotebook> sharedNotebooks;
    ErrorString errorPrefix(QT_TRANSLATE_NOOP("", "Can't list all shared notebooks"));

    QSqlQuery query(m_sqlDatabase);
    bool res = query.exec(QStringLiteral("SELECT * FROM SharedNotebooks"));
    if (!res) {
        errorDescription.base() = errorPrefix.base();
        QNCRITICAL(errorDescription << QStringLiteral("last error = ") << query.lastError()
                   << QStringLiteral(", last query = ") << query.lastQuery());
        errorDescription.details() += query.lastError().text();
        return sharedNotebooks;
    }

    sharedNotebooks.reserve(qMax(query.size(), 0));

    while(query.next())
    {
        QSqlRecord record = query.record();

        sharedNotebooks << SharedNotebook();
        SharedNotebook & sharedNotebook = sharedNotebooks.back();

        ErrorString error;
        res = fillSharedNotebookFromSqlRecord(record, sharedNotebook, error);
        if (!res) {
            errorDescription.base() = errorPrefix.base();
            errorDescription.additionalBases().append(error.base());
            errorDescription.additionalBases().append(error.additionalBases());
            errorDescription.details() = error.details();
            QNWARNING(errorDescription);
            sharedNotebooks.clear();
            return sharedNotebooks;
        }
    }

    int numSharedNotebooks = sharedNotebooks.size();
    QNDEBUG(QStringLiteral("found ") << numSharedNotebooks << QStringLiteral(" shared notebooks"));

    if (numSharedNotebooks <= 0) {
        errorDescription.base() = errorPrefix.base();
        errorDescription.additionalBases().append(QT_TRANSLATE_NOOP("", "no shared notebooks were found in the local storage database"));
        QNDEBUG(errorDescription);
    }

    return sharedNotebooks;
}

QList<SharedNotebook> LocalStorageManagerPrivate::listSharedNotebooksPerNotebookGuid(const QString & notebookGuid,
                                                                                     ErrorString & errorDescription) const
{
    QList<SharedNotebook> sharedNotebooks;

    QList<qevercloud::SharedNotebook> enSharedNotebooks = listEnSharedNotebooksPerNotebookGuid(notebookGuid, errorDescription);
    if (enSharedNotebooks.isEmpty()) {
        return sharedNotebooks;
    }

    sharedNotebooks.reserve(qMax(enSharedNotebooks.size(), 0));

    for(auto it = enSharedNotebooks.constBegin(), end = enSharedNotebooks.constEnd(); it != end; ++it) {
        sharedNotebooks << *it;
    }

    return sharedNotebooks;
}

QList<qevercloud::SharedNotebook> LocalStorageManagerPrivate::listEnSharedNotebooksPerNotebookGuid(const QString & notebookGuid,
                                                                                                   ErrorString & errorDescription) const
{
    QNDEBUG(QStringLiteral("LocalStorageManagerPrivate::listSharedNotebooksPerNotebookGuid: guid = ") << notebookGuid);

    QList<qevercloud::SharedNotebook> qecSharedNotebooks;

    ErrorString errorPrefix(QT_TRANSLATE_NOOP("", "Can't list shared notebooks per notebook guid"));

    if (!checkGuid(notebookGuid)) {
        errorDescription.base() = errorPrefix.base();
        errorDescription.additionalBases().append(QT_TRANSLATE_NOOP("", "notebook guid is invalid"));
        errorDescription.details() = notebookGuid;
        QNWARNING(errorDescription);
        return qecSharedNotebooks;
    }

    QSqlQuery query(m_sqlDatabase);
    query.prepare(QStringLiteral("SELECT * FROM SharedNotebooks WHERE sharedNotebookNotebookGuid=?"));
    query.addBindValue(notebookGuid);

    bool res = query.exec();
    if (!res) {
        SET_ERROR();
        return qecSharedNotebooks;
    }

    int numSharedNotebooks = query.size();
    qecSharedNotebooks.reserve(qMax(numSharedNotebooks, 0));
    QList<qevercloud::SharedNotebook> unsortedSharedNotebooks;
    unsortedSharedNotebooks.reserve(qMax(numSharedNotebooks, 0));
    QList<SharedNotebook> sharedNotebooks;
    sharedNotebooks.reserve(qMax(numSharedNotebooks, 0));

    while(query.next())
    {
        QSqlRecord record = query.record();

        unsortedSharedNotebooks << qevercloud::SharedNotebook();
        qevercloud::SharedNotebook & qecSharedNotebook = unsortedSharedNotebooks.back();
        sharedNotebooks << SharedNotebook(qecSharedNotebook);
        SharedNotebook & sharedNotebook = sharedNotebooks.back();

        ErrorString error;
        res = fillSharedNotebookFromSqlRecord(record, sharedNotebook, error);
        if (!res) {
            errorDescription.base() = errorPrefix.base();
            errorDescription.additionalBases().append(error.base());
            errorDescription.additionalBases().append(error.additionalBases());
            errorDescription.details() = error.details();
            qecSharedNotebooks.clear();
            return qecSharedNotebooks;
        }
    }

    qSort(sharedNotebooks.begin(), sharedNotebooks.end(), SharedNotebookCompareByIndex());

    for(auto it = sharedNotebooks.constBegin(), end = sharedNotebooks.constEnd(); it != end; ++it) {
        qecSharedNotebooks << static_cast<const qevercloud::SharedNotebook>(*it);
    }

    numSharedNotebooks = qecSharedNotebooks.size();
    QNDEBUG(QStringLiteral("found ") << numSharedNotebooks << QStringLiteral(" shared notebooks"));

    return qecSharedNotebooks;
}

bool LocalStorageManagerPrivate::expungeNotebook(Notebook & notebook, ErrorString & errorDescription)
{
    ErrorString errorPrefix(QT_TRANSLATE_NOOP("", "Can't expunge notebook from the local storage database"));

    QString localUid = notebook.localUid();

    QString column, uid;
    bool shouldCheckRowExistence = true;

    bool notebookHasGuid = notebook.hasGuid();
    if (notebookHasGuid)
    {
        column = QStringLiteral("guid");
        uid = notebook.guid();

        if (!checkGuid(uid)) {
            errorDescription.base() = errorPrefix.base();
            errorDescription.additionalBases().append(QT_TRANSLATE_NOOP("", "notebook's guid is invalid"));
            errorDescription.details() = uid;
            QNWARNING(errorDescription);
            return false;
        }

        if (localUid.isEmpty())
        {
            ErrorString error;
            bool res = getNotebookLocalUidForGuid(uid, localUid, error);
            if (!res) {
                errorDescription.base() = errorPrefix.base();
                errorDescription.additionalBases().append(error.base());
                errorDescription.additionalBases().append(error.additionalBases());
                errorDescription.details() = error.details();
                QNWARNING(errorDescription);
                return false;
            }

            notebook.setLocalUid(localUid);
            shouldCheckRowExistence = false;
        }
    }
    else
    {
        column = QStringLiteral("localUid");
        uid = notebook.localUid();
    }

    if (shouldCheckRowExistence && !rowExists(QStringLiteral("Notebooks"), column, uid)) {
        errorDescription.base() = errorPrefix.base();
        errorDescription.additionalBases().append(QT_TRANSLATE_NOOP("", "notebook to be expunged was not found"));
        errorDescription.details() = column;
        errorDescription.details() = QStringLiteral(" = ");
        errorDescription.details() = uid;
        QNWARNING(errorDescription);
        return false;
    }

    uid = sqlEscapeString(uid);

    QString queryString = QString("DELETE FROM Notebooks WHERE %1 = '%2'").arg(column,uid);
    QSqlQuery query(m_sqlDatabase);
    bool res = query.exec(queryString);
    DATABASE_CHECK_AND_SET_ERROR();

    return true;
}

int LocalStorageManagerPrivate::linkedNotebookCount(ErrorString & errorDescription) const
{
    ErrorString errorPrefix(QT_TRANSLATE_NOOP("", "Can't get the number of linked notebooks in the local storage database"));

    bool res = checkAndPrepareGetLinkedNotebookCountQuery();
    QSqlQuery & query = m_getLinkedNotebookCountQuery;
    if (!res) {
        SET_ERROR();
        query.finish();
        return -1;
    }

    res = query.exec();
    if (!res) {
        SET_ERROR();
        query.finish();
        return -1;
    }

    if (!query.next()) {
        QNDEBUG(QStringLiteral("Found no linked notebooks in local storage database"));
        query.finish();
        return 0;
    }

    bool conversionResult = false;
    int count = query.value(0).toInt(&conversionResult);
    query.finish();

    if (!conversionResult) {
        SET_INT_CONVERSION_ERROR();
        return -1;
    }

    return count;
}

bool LocalStorageManagerPrivate::addLinkedNotebook(const LinkedNotebook & linkedNotebook,
                                                   ErrorString & errorDescription)
{
    ErrorString errorPrefix(QT_TRANSLATE_NOOP("", "Can't add linked notebook to the local storage database"));

    ErrorString error;
    bool res = linkedNotebook.checkParameters(error);
    if (!res) {
        errorDescription.base() = errorPrefix.base();
        errorDescription.additionalBases().append(error.base());
        errorDescription.additionalBases().append(error.additionalBases());
        errorDescription.details() = error.details();
        QNWARNING(QStringLiteral("Found invalid LinkedNotebook: ") << linkedNotebook
                  << QStringLiteral("\nError: ") << error);
        return false;
    }

    bool exists = rowExists(QStringLiteral("LinkedNotebooks"), QStringLiteral("guid"), QVariant(linkedNotebook.guid()));
    if (exists) {
        errorDescription.base() = errorPrefix.base();
        errorDescription.additionalBases().append(QT_TRANSLATE_NOOP("", "linked notebook with specified guid already exists"));
        errorDescription.details() = linkedNotebook.guid();
        QNWARNING(errorDescription);
        return false;
    }

    error.clear();
    res = insertOrReplaceLinkedNotebook(linkedNotebook, error);
    if (!res) {
        errorDescription.base() = errorPrefix.base();
        errorDescription.additionalBases().append(error.base());
        errorDescription.additionalBases().append(error.additionalBases());
        errorDescription.details() = error.details();
        QNWARNING(errorDescription);
        return false;
    }

    return true;
}

bool LocalStorageManagerPrivate::updateLinkedNotebook(const LinkedNotebook & linkedNotebook,
                                                      ErrorString & errorDescription)
{
    ErrorString errorPrefix(QT_TRANSLATE_NOOP("", "Can't update linked notebook in the local storage database"));

    ErrorString error;
    bool res = linkedNotebook.checkParameters(error);
    if (!res) {
        errorDescription.base() = errorPrefix.base();
        errorDescription.additionalBases().append(error.base());
        errorDescription.additionalBases().append(error.additionalBases());
        errorDescription.details() = error.details();
        QNWARNING(QStringLiteral("Found invalid LinkedNotebook: ") << linkedNotebook
                  << QStringLiteral("\nError: ") << error);
        return false;
    }

    QString guid = linkedNotebook.guid();

    bool exists = rowExists(QStringLiteral("LinkedNotebooks"), QStringLiteral("guid"), QVariant(guid));
    if (!exists) {
        errorDescription.base() = errorPrefix.base();
        errorDescription.additionalBases().append(QT_TRANSLATE_NOOP("", "linked notebook to be updated was not found"));
        errorDescription.details() = guid;
        QNWARNING(errorDescription);
        return false;
    }

    error.clear();
    res = insertOrReplaceLinkedNotebook(linkedNotebook, error);
    if (!res) {
        errorDescription.base() = errorPrefix.base();
        errorDescription.additionalBases().append(error.base());
        errorDescription.additionalBases().append(error.additionalBases());
        errorDescription.details() = error.details();
        QNWARNING(errorDescription);
        return false;
    }

    return true;
}

bool LocalStorageManagerPrivate::findLinkedNotebook(LinkedNotebook & linkedNotebook, ErrorString & errorDescription) const
{
    QNDEBUG(QStringLiteral("LocalStorageManagerPrivate::findLinkedNotebook"));

    ErrorString errorPrefix(QT_TRANSLATE_NOOP("", "Can't find linked notebook in the local storage database"));

    if (!linkedNotebook.hasGuid()) {
        errorDescription.base() = errorPrefix.base();
        errorDescription.additionalBases().append(QT_TRANSLATE_NOOP("", "linked notebook's guid is not set"));
        QNWARNING(errorDescription);
        return false;
    }

    QString notebookGuid = linkedNotebook.guid();
    QNDEBUG(QStringLiteral("guid = ") << notebookGuid);
    if (!checkGuid(notebookGuid)) {
        errorDescription.base() = errorPrefix.base();
        errorDescription.additionalBases().append(QT_TRANSLATE_NOOP("", "linked notebook's guid is invalid"));
        errorDescription.details() = notebookGuid;
        QNWARNING(errorDescription);
        return false;
    }

    linkedNotebook.clear();

    QSqlQuery query(m_sqlDatabase);
    query.prepare(QStringLiteral("SELECT guid, updateSequenceNumber, isDirty, shareName, username, shardId, "
                                 "sharedNotebookGlobalId, uri, noteStoreUrl, webApiUrlPrefix, stack, businessId "
                                 "FROM LinkedNotebooks WHERE guid = ?"));
    query.addBindValue(notebookGuid);

    bool res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR();

    if (!query.next()) {
        QNDEBUG(errorDescription);
        return false;
    }

    QSqlRecord rec = query.record();

    ErrorString error;
    res = fillLinkedNotebookFromSqlRecord(rec, linkedNotebook, error);
    if (!res) {
        errorDescription.base() = errorPrefix.base();
        errorDescription.additionalBases().append(error.base());
        errorDescription.additionalBases().append(error.additionalBases());
        errorDescription.details() = error.details();
        QNWARNING(errorDescription);
        return false;
    }

    return true;
}

QList<LinkedNotebook> LocalStorageManagerPrivate::listAllLinkedNotebooks(ErrorString & errorDescription, const size_t limit, const size_t offset,
                                                                         const LocalStorageManager::ListLinkedNotebooksOrder::type order,
                                                                         const LocalStorageManager::OrderDirection::type & orderDirection) const
{
    QNDEBUG(QStringLiteral("LocalStorageManagerPrivate::listAllLinkedNotebooks"));
    return listLinkedNotebooks(LocalStorageManager::ListAll, errorDescription, limit, offset, order, orderDirection);
}

QList<LinkedNotebook> LocalStorageManagerPrivate::listLinkedNotebooks(const LocalStorageManager::ListObjectsOptions flag,
                                                                      ErrorString & errorDescription, const size_t limit, const size_t offset,
                                                                      const LocalStorageManager::ListLinkedNotebooksOrder::type & order,
                                                                      const LocalStorageManager::OrderDirection::type & orderDirection) const
{
    QNDEBUG(QStringLiteral("LocalStorageManagerPrivate::listLinkedNotebooks: flag = ") << flag);
    return listObjects<LinkedNotebook, LocalStorageManager::ListLinkedNotebooksOrder::type>(flag, errorDescription, limit, offset, order, orderDirection);
}

bool LocalStorageManagerPrivate::expungeLinkedNotebook(const LinkedNotebook & linkedNotebook,
                                                       ErrorString & errorDescription)
{
    ErrorString errorPrefix(QT_TRANSLATE_NOOP("", "Can't expunge linked notebook from the local storage database"));

    if (!linkedNotebook.hasGuid()) {
        errorDescription.base() = errorPrefix.base();
        errorDescription.additionalBases().append(QT_TRANSLATE_NOOP("", "linked notebook's guid is not set"));
        QNWARNING(errorDescription);
        return false;
    }

    QString linkedNotebookGuid = sqlEscapeString(linkedNotebook.guid());

    if (!checkGuid(linkedNotebookGuid)) {
        errorDescription.base() = errorPrefix.base();
        errorDescription.additionalBases().append(QT_TRANSLATE_NOOP("", "linked notebook's guid is invalid"));
        errorDescription.details() = linkedNotebookGuid;
        QNWARNING(errorDescription);
        return false;
    }

    bool exists = rowExists(QStringLiteral("LinkedNotebooks"), QStringLiteral("guid"), linkedNotebookGuid);
    if (!exists) {
        errorDescription.base() = errorPrefix.base();
        errorDescription.additionalBases().append(QT_TRANSLATE_NOOP("", "can't find the linked notebook to be expunged"));
        errorDescription.details() = linkedNotebookGuid;
        return false;
    }

    QString queryString = QString("DELETE FROM LinkedNotebooks WHERE guid='%1'").arg(linkedNotebookGuid);
    QSqlQuery query(m_sqlDatabase);
    bool res = query.exec(queryString);
    DATABASE_CHECK_AND_SET_ERROR();

    return true;
}

int LocalStorageManagerPrivate::noteCount(ErrorString & errorDescription) const
{
    ErrorString errorPrefix(QT_TRANSLATE_NOOP("", "Can't get the number of notes in the local storage database"));

    bool res = checkAndPrepareNoteCountQuery();
    QSqlQuery & query = m_getNoteCountQuery;
    if (!res) {
        SET_ERROR();
        query.finish();
        return -1;
    }

    res = query.exec();
    if (!res) {
        SET_ERROR();
        query.finish();
        return -1;
    }

    if (!query.next()) {
        QNDEBUG(QStringLiteral("Found no notes in local storage database"));
        query.finish();
        return 0;
    }

    bool conversionResult = false;
    int count = query.value(0).toInt(&conversionResult);
    query.finish();

    if (!conversionResult) {
        SET_INT_CONVERSION_ERROR();
        return -1;
    }

    return count;
}

int LocalStorageManagerPrivate::noteCountPerNotebook(const Notebook & notebook, ErrorString & errorDescription) const
{
    ErrorString errorPrefix(QT_TRANSLATE_NOOP("", "Can't get the number of notes per notebook in the local storage database"));

    ErrorString error;
    bool res = notebook.checkParameters(error);
    if (!res) {
        errorDescription.base() = errorPrefix.base();
        errorDescription.additionalBases().append(error.base());
        errorDescription.additionalBases().append(error.additionalBases());
        errorDescription.details() = error.details();
        QNWARNING(QStringLiteral("Found invalid notebook: ") << notebook << QStringLiteral("\nError: ") << error);
        return -1;
    }

    QString column, value;
    if (notebook.hasGuid()) {
        column = QStringLiteral("notebookGuid");
        value = notebook.guid();
    }
    else {
        column = QStringLiteral("notebookLocalUid");
        value = notebook.localUid();
    }

    value = sqlEscapeString(value);

    QString queryString = QString("SELECT COUNT(*) FROM Notes WHERE deletionTimestamp IS NULL AND %1 = '%2'").arg(column, value);
    QSqlQuery query(m_sqlDatabase);
    res = query.exec(queryString);

    if (!res) {
        SET_ERROR();
        return -1;
    }

    if (!query.next()) {
        QNDEBUG(QStringLiteral("Found no notes per given notebook in local storage database"));
        return 0;
    }

    bool conversionResult = false;
    int count = query.value(0).toInt(&conversionResult);
    if (!conversionResult) {
        SET_INT_CONVERSION_ERROR();
        return -1;
    }

    return count;
}

int LocalStorageManagerPrivate::noteCountPerTag(const Tag & tag, ErrorString & errorDescription) const
{
    ErrorString errorPrefix(QT_TRANSLATE_NOOP("", "Can't get number of notes per tag in the local storage database"));

    ErrorString error;
    bool res = tag.checkParameters(error);
    if (!res) {
        errorDescription.base() = errorPrefix.base();
        errorDescription.additionalBases().append(error.base());
        errorDescription.additionalBases().append(error.additionalBases());
        errorDescription.details() = error.details();
        QNWARNING(QStringLiteral("Found invalid tag: ") << tag << QStringLiteral("\nError: ") << error);
        return -1;
    }

    QString column, value;
    if (tag.hasGuid()) {
        column = QStringLiteral("tag");
        value = tag.guid();
    }
    else {
        column = QStringLiteral("localTag");
        value = tag.localUid();
    }

    value = sqlEscapeString(value);

    QString queryString = QString("SELECT COUNT(*) FROM Notes WHERE deletionTimestamp IS NULL AND (localUid IN"
                                  "(SELECT DISTINCT localNote FROM NoteTags WHERE %1 = '%2'))").arg(column,value);
    QSqlQuery query(m_sqlDatabase);
    res = query.exec(queryString);
    if (!res) {
        SET_ERROR();
        return -1;
    }

    if (!query.next()) {
        QNDEBUG(QStringLiteral("Found no notes per given tag in local storage database"));
        return 0;
    }

    bool conversionResult = false;
    int count = query.value(0).toInt(&conversionResult);
    if (!conversionResult) {
        SET_INT_CONVERSION_ERROR();
        return -1;
    }

    return count;
}

bool LocalStorageManagerPrivate::addNote(Note & note, ErrorString & errorDescription)
{
    ErrorString errorPrefix(QT_TRANSLATE_NOOP("", "Can't add note to the local storage database"));

    ErrorString error;
    QString notebookLocalUid;
    bool res = getNotebookLocalUidFromNote(note, notebookLocalUid, error);
    if (Q_UNLIKELY(!res)) {
        errorDescription.base() = errorPrefix.base();
        errorDescription.additionalBases().append(error.base());
        errorDescription.additionalBases().append(error.additionalBases());
        errorDescription.details() = error.details();
        QNWARNING(errorDescription);
        return false;
    }

    error.clear();
    res = canAddNoteToNotebook(notebookLocalUid, error);
    if (Q_UNLIKELY(!res)) {
        errorDescription.base() = errorPrefix.base();
        errorDescription.additionalBases().append(error.base());
        errorDescription.additionalBases().append(error.additionalBases());
        errorDescription.details() = error.details();
        QNWARNING(errorDescription);
        return false;
    }

    note.setNotebookLocalUid(notebookLocalUid);

    error.clear();
    QString notebookGuid;
    res = getNotebookGuidForNote(note, notebookGuid, error);
    if (Q_UNLIKELY(!res)) {
        errorDescription.base() = errorPrefix.base();
        errorDescription.additionalBases().append(error.base());
        errorDescription.additionalBases().append(error.additionalBases());
        errorDescription.details() = error.details();
        QNWARNING(errorDescription);
        return false;
    }

    note.setNotebookGuid(notebookGuid);

    error.clear();
    res = note.checkParameters(error);
    if (Q_UNLIKELY(!res)) {
        errorDescription.base() = errorPrefix.base();
        errorDescription.additionalBases().append(error.base());
        errorDescription.additionalBases().append(error.additionalBases());
        errorDescription.details() = error.details();
        QNWARNING(QStringLiteral("Found invalid note: ") << note);
        return false;
    }

    QString localUid = note.localUid();

    QString column, uid;
    bool shouldCheckNoteExistence = true;

    bool noteHasGuid = note.hasGuid();
    if (noteHasGuid)
    {
        column = QStringLiteral("guid");
        uid = note.guid();

        if (!checkGuid(uid)) {
            errorDescription.base() = errorPrefix.base();
            errorDescription.additionalBases().append(QT_TRANSLATE_NOOP("", "note's guid is invalid"));
            QNWARNING(errorDescription);
            return false;
        }

        if (localUid.isEmpty())
        {
            error.clear();
            res = getNoteLocalUidForGuid(uid, localUid, error);
            if (res || !localUid.isEmpty()) {
                errorDescription.base() = errorPrefix.base();
                errorDescription.additionalBases().append(QT_TRANSLATE_NOOP("", "found already existing note with the same guid"));
                QNWARNING(errorDescription << QStringLiteral(", guid: ") << uid);
                return false;
            }

            localUid = UidGenerator::Generate();
            note.setLocalUid(localUid);
            shouldCheckNoteExistence = false;
        }
    }
    else
    {
        column = QStringLiteral("localUid");
        uid = localUid;
    }

    if (shouldCheckNoteExistence && rowExists(QStringLiteral("Notes"), column, uid))
    {
        errorDescription.base() = errorPrefix.base();
        errorDescription.additionalBases().append(QT_TRANSLATE_NOOP("", "note already exists"));
        errorDescription.details() = column;
        errorDescription.details() += QStringLiteral(" = ");
        errorDescription.details() += uid;
        QNWARNING(errorDescription);
        return false;
    }

    return insertOrReplaceNote(note, /* update resources = */ true, /* update tags = */ true, errorDescription);
}

bool LocalStorageManagerPrivate::updateNote(Note & note, const bool updateResources, const bool updateTags,
                                            ErrorString & errorDescription)
{
    ErrorString errorPrefix(QT_TRANSLATE_NOOP("", "Can't update note in the local storage database"));

    ErrorString error;
    QString notebookLocalUid;
    bool res = getNotebookLocalUidFromNote(note, notebookLocalUid, error);
    if (Q_UNLIKELY(!res)) {
        errorDescription.base() = errorPrefix.base();
        errorDescription.additionalBases().append(error.base());
        errorDescription.additionalBases().append(error.additionalBases());
        errorDescription.details() = error.details();
        QNWARNING(errorDescription);
        return false;
    }

    error.clear();
    res = canUpdateNoteInNotebook(notebookLocalUid, error);
    if (!res) {
        errorDescription.base() = errorPrefix.base();
        errorDescription.additionalBases().append(error.base());
        errorDescription.additionalBases().append(error.additionalBases());
        errorDescription.details() = error.details();
        QNWARNING(errorDescription);
        return false;
    }

    note.setNotebookLocalUid(notebookLocalUid);

    error.clear();
    QString notebookGuid;
    res = getNotebookGuidForNote(note, notebookGuid, error);
    if (Q_UNLIKELY(!res)) {
        errorDescription.base() = errorPrefix.base();
        errorDescription.additionalBases().append(error.base());
        errorDescription.additionalBases().append(error.additionalBases());
        errorDescription.details() = error.details();
        QNWARNING(errorDescription);
        return false;
    }

    note.setNotebookGuid(notebookGuid);

    error.clear();
    res = note.checkParameters(error);
    if (!res) {
        errorDescription.base() = errorPrefix.base();
        errorDescription.additionalBases().append(error.base());
        errorDescription.additionalBases().append(error.additionalBases());
        errorDescription.details() = error.details();
        QNWARNING(QStringLiteral("Found invalid note: ") << note);
        return false;
    }

    QString localUid = note.localUid();

    QString column, uid;
    bool shouldCheckNoteExistence = true;

    bool noteHasGuid = note.hasGuid();
    if (noteHasGuid)
    {
        column = QStringLiteral("guid");
        uid = note.guid();

        if (!checkGuid(uid)) {
            errorDescription.base() = errorPrefix.base();
            errorDescription.additionalBases().append(QT_TRANSLATE_NOOP("", "note's guid is invalid"));
            errorDescription.details() = uid;
            QNWARNING(errorDescription);
            return false;
        }

        if (localUid.isEmpty())
        {
            error.clear();
            res = getNoteLocalUidForGuid(uid, localUid, error);
            if (!res || localUid.isEmpty()) {
                errorDescription.base() = errorPrefix.base();
                errorDescription.additionalBases().append(error.base());
                errorDescription.additionalBases().append(error.additionalBases());
                errorDescription.details() = error.details();
                return false;
            }

            note.setLocalUid(localUid);
            shouldCheckNoteExistence = false;
        }
    }
    else
    {
        column = QStringLiteral("localUid");
        uid = localUid;
    }

    if (shouldCheckNoteExistence && !rowExists(QStringLiteral("Notes"), column, uid))
    {
        errorDescription.base() = errorPrefix.base();
        errorDescription.additionalBases().append(QT_TRANSLATE_NOOP("", "note was not found in the local storage database"));
        errorDescription.details() = column;
        errorDescription.details() += QStringLiteral(" = ");
        errorDescription.details() += uid;
        QNWARNING(errorDescription);
        return false;
    }

    return insertOrReplaceNote(note, updateResources, updateTags, errorDescription);
}

bool LocalStorageManagerPrivate::findNote(Note & note, ErrorString & errorDescription,
                                          const bool withResourceBinaryData) const
{
    QNDEBUG(QStringLiteral("LocalStorageManagerPrivate::findNote"));

    ErrorString errorPrefix(QT_TRANSLATE_NOOP("", "Can't find note in the local storage database"));

    QString column, uid;
    bool noteHasGuid = note.hasGuid();
    if (noteHasGuid)
    {
        column = QStringLiteral("guid");
        uid = note.guid();

        if (!checkGuid(uid)) {
            errorDescription.base() = errorPrefix.base();
            errorDescription.additionalBases().append(QT_TRANSLATE_NOOP("", "note's guid is invalid"));
            errorDescription.details() = uid;
            QNWARNING(errorDescription);
            return false;
        }
    }
    else
    {
        column = QStringLiteral("localUid");
        uid = note.localUid();
    }

    note = Note();

    QString resourcesTable = QStringLiteral("Resources");
    if (!withResourceBinaryData) {
        resourcesTable += QStringLiteral("WithoutBinaryData");
    }

    QString resourceIndexColumn = (column == QStringLiteral("localUid")
                                   ? QStringLiteral("noteLocalUid")
                                   : QStringLiteral("noteGuid"));

    uid = sqlEscapeString(uid);

    QString queryString = QString("SELECT * FROM Notes "
                                  "LEFT OUTER JOIN SharedNotes ON ((Notes.guid IS NOT NULL) AND (Notes.guid = SharedNotes.sharedNoteNoteGuid)) "
                                  "LEFT OUTER JOIN NoteRestrictions ON Notes.localUid = NoteRestrictions.noteLocalUid "
                                  "LEFT OUTER JOIN NoteLimits ON Notes.localUid = NoteLimits.noteLocalUid "
                                  "LEFT OUTER JOIN %1 ON Notes.%3 = %1.%2 "
                                  "LEFT OUTER JOIN ResourceAttributes "
                                  "ON %1.resourceLocalUid = ResourceAttributes.resourceLocalUid "
                                  "LEFT OUTER JOIN ResourceAttributesApplicationDataKeysOnly "
                                  "ON %1.resourceLocalUid = ResourceAttributesApplicationDataKeysOnly.resourceLocalUid "
                                  "LEFT OUTER JOIN ResourceAttributesApplicationDataFullMap "
                                  "ON %1.resourceLocalUid = ResourceAttributesApplicationDataFullMap.resourceLocalUid "
                                  "LEFT OUTER JOIN NoteTags ON Notes.localUid = NoteTags.localNote "
                                  "WHERE %3 = '%4'")
                                 .arg(resourcesTable,resourceIndexColumn,column,uid);
    QSqlQuery query(m_sqlDatabase);
    bool res = query.exec(queryString);
    DATABASE_CHECK_AND_SET_ERROR();

    QList<Resource> resources;
    QHash<QString, int> resourceIndexPerLocalUid;

    QList<QPair<QString, int> > tagGuidsAndIndices;
    QHash<QString, int> tagGuidIndexPerGuid;

    QList<QPair<QString, int> > tagLocalUidsAndIndices;
    QHash<QString, int> tagLocalUidIndexPerUid;

    size_t counter = 0;
    while(query.next())
    {
        QSqlRecord rec = query.record();

        ErrorString error;
        bool res = fillNoteFromSqlRecord(rec, note, error);
        if (!res) {
            errorDescription.base() = errorPrefix.base();
            errorDescription.additionalBases().append(error.base());
            errorDescription.additionalBases().append(error.additionalBases());
            errorDescription.details() = error.details();
            QNWARNING(errorDescription);
            return false;
        }

        ++counter;

        int resourceLocalUidIndex = rec.indexOf(QStringLiteral("resourceLocalUid"));
        if (resourceLocalUidIndex >= 0)
        {
            QVariant value = rec.value(resourceLocalUidIndex);
            if (!value.isNull())
            {
                QString resourceLocalUid = value.toString();
                auto it = resourceIndexPerLocalUid.find(resourceLocalUid);
                bool resourceIndexNotFound = (it == resourceIndexPerLocalUid.end());
                if (resourceIndexNotFound) {
                    int resourceIndexInList = resources.size();
                    resourceIndexPerLocalUid[resourceLocalUid] = resourceIndexInList;
                    resources << Resource();
                }

                Resource & resource = (resourceIndexNotFound
                                              ? resources.back()
                                              : resources[it.value()]);
                fillResourceFromSqlRecord(rec, withResourceBinaryData, resource);
            }
        }

        error.clear();
        res = fillNoteTagIdFromSqlRecord(rec, QStringLiteral("tag"), tagGuidsAndIndices, tagGuidIndexPerGuid, error);
        if (!res) {
            errorDescription.base() = errorPrefix.base();
            errorDescription.additionalBases().append(error.base());
            errorDescription.additionalBases().append(error.additionalBases());
            errorDescription.details() = error.details();
            QNWARNING(errorDescription);
            return false;
        }

        error.clear();
        res = fillNoteTagIdFromSqlRecord(rec, QStringLiteral("localTag"), tagLocalUidsAndIndices, tagLocalUidIndexPerUid, error);
        if (!res) {
            errorDescription.base() = errorPrefix.base();
            errorDescription.additionalBases().append(error.base());
            errorDescription.additionalBases().append(error.additionalBases());
            errorDescription.details() = error.details();
            QNWARNING(errorDescription);
            return false;
        }
    }

    if (!counter) {
        QNDEBUG(errorDescription);
        return false;
    }

    int numResources = resources.size();
    if (numResources > 0) {
        qSort(resources.begin(), resources.end(), ResourceCompareByIndex());
        note.setResources(resources);
    }

    int numTagGuids = tagGuidsAndIndices.size();
    if (numTagGuids > 0)
    {
        qSort(tagGuidsAndIndices.begin(), tagGuidsAndIndices.end(), QStringIntPairCompareByInt());
        QStringList tagGuids;
        tagGuids.reserve(numTagGuids);
        for(int i = 0; i < numTagGuids; ++i)
        {
            const QString & guid = tagGuidsAndIndices[i].first;
            if (guid.isEmpty()) {
                continue;
            }

            tagGuids << guid;
        }

        note.setTagGuids(tagGuids);
    }

    int numTagLocalUids = tagLocalUidsAndIndices.size();
    if (numTagLocalUids > 0)
    {
        qSort(tagLocalUidsAndIndices.begin(), tagLocalUidsAndIndices.end(), QStringIntPairCompareByInt());
        QStringList tagLocalUids;
        tagLocalUids.reserve(numTagLocalUids);
        for(int i = 0; i < numTagLocalUids; ++i) {
            tagLocalUids << tagLocalUidsAndIndices[i].first;
        }

        note.setTagLocalUids(tagLocalUids);
    }

    sortSharedNotes(note);

    ErrorString error;
    res = note.checkParameters(error);
    if (!res) {
        errorDescription.base() = errorPrefix.base();
        errorDescription.additionalBases().append(error.base());
        errorDescription.additionalBases().append(error.additionalBases());
        errorDescription.details() = error.details();
        QNWARNING(errorDescription);
        return false;
    }

    return true;
}

QList<Note> LocalStorageManagerPrivate::listNotesPerNotebook(const Notebook & notebook,
                                                             ErrorString & errorDescription,
                                                             const bool withResourceBinaryData,
                                                             const LocalStorageManager::ListObjectsOptions & flag,
                                                             const size_t limit, const size_t offset,
                                                             const LocalStorageManager::ListNotesOrder::type & order,
                                                             const LocalStorageManager::OrderDirection::type & orderDirection) const
{
    QNDEBUG(QStringLiteral("LocalStorageManagerPrivate::listNotesPerNotebook: notebook = ") << notebook
            << QStringLiteral("\nWith resource binary data = ") << (withResourceBinaryData ? QStringLiteral("true") : QStringLiteral("false"))
            << QStringLiteral(", flag = ") << flag << QStringLiteral(", limit = ") << limit << QStringLiteral(", offset = ")
            << offset << QStringLiteral(", order = ") << order << QStringLiteral(", order direction = ") << orderDirection);

    ErrorString errorPrefix(QT_TRANSLATE_NOOP("", "Can't list notes per notebook"));

    QList<Note> notes;

    QString column, uid;
    bool notebookHasGuid = notebook.hasGuid();
    if (notebookHasGuid)
    {
        column = QStringLiteral("notebookGuid");
        uid = notebook.guid();

        if (!checkGuid(uid))
        {
            errorDescription.base() = errorPrefix.base();
            errorDescription.additionalBases().append(QT_TRANSLATE_NOOP("", "notebook's guid is invalid"));
            QNWARNING(errorDescription);
            return notes;
        }
    }
    else
    {
        column = QStringLiteral("notebookLocalUid");
        uid = notebook.localUid();
    }

    // Will run all the queries from this method and its sub-methods within a single transaction
    // to prevent multiple drops and re-obtainings of shared lock
    Transaction transaction(m_sqlDatabase, *this, Transaction::Selection);
    Q_UNUSED(transaction)

    uid = sqlEscapeString(uid);

    ErrorString error;
    QString notebookGuidSqlQueryCondition = QString("%1 = '%2'").arg(column,uid);
    notes = listObjects<Note, LocalStorageManager::ListNotesOrder::type>(flag, error, limit, offset, order,
                                                                         orderDirection, notebookGuidSqlQueryCondition);
    if (notes.isEmpty() && !error.isEmpty()) {
        errorDescription.base() = errorPrefix.base();
        errorDescription.additionalBases().append(error.base());
        errorDescription.additionalBases().append(error.additionalBases());
        errorDescription.details() = error.details();
        QNWARNING(errorDescription);
        return notes;
    }

    const int numNotes = notes.size();
    for(int i = 0; i < numNotes; ++i)
    {
        Note & note = notes[i];

        error.clear();
        bool res = findAndSetTagIdsPerNote(note, error);
        if (!res) {
            errorDescription.base() = errorPrefix.base();
            errorDescription.additionalBases().append(error.base());
            errorDescription.additionalBases().append(error.additionalBases());
            errorDescription.details() = error.details();
            QNWARNING(errorDescription);
            notes.clear();
            return notes;
        }

        error.clear();
        res = findAndSetResourcesPerNote(note, error, withResourceBinaryData);
        if (!res) {
            errorDescription.base() = errorPrefix.base();
            errorDescription.additionalBases().append(error.base());
            errorDescription.additionalBases().append(error.additionalBases());
            errorDescription.details() = error.details();
            QNWARNING(errorDescription);
            notes.clear();
            return notes;
        }

        res = note.checkParameters(error);
        if (!res) {
            errorDescription.base() = errorPrefix.base();
            errorDescription.additionalBases().append(error.base());
            errorDescription.additionalBases().append(error.additionalBases());
            errorDescription.details() = error.details();
            QNWARNING(errorDescription);
            notes.clear();
            return notes;
        }
    }

    return notes;
}

QList<Note> LocalStorageManagerPrivate::listNotesPerTag(const Tag & tag, ErrorString & errorDescription,
                                                        const bool withResourceBinaryData,
                                                        const LocalStorageManager::ListObjectsOptions & flag,
                                                        const size_t limit, const size_t offset,
                                                        const LocalStorageManager::ListNotesOrder::type & order,
                                                        const LocalStorageManager::OrderDirection::type & orderDirection) const
{
    QNDEBUG(QStringLiteral("LocalStorageManagerPrivate::listNotesPerTag: tag = ") << tag
            << QStringLiteral("\nWith resource binary data = ") << (withResourceBinaryData ? QStringLiteral("true") : QStringLiteral("false"))
            << QStringLiteral(", flag = ") << flag << QStringLiteral(", limit = ") << limit << QStringLiteral(", offset = ")
            << offset << QStringLiteral(", order = ") << order << QStringLiteral(", order direction = ") << orderDirection);

    ErrorString errorPrefix(QT_TRANSLATE_NOOP("", "Can't list all notes with tag"));

    QList<Note> notes;

    QString column, uid;
    bool tagHasGuid = tag.hasGuid();
    if (tagHasGuid)
    {
        column = QStringLiteral("tag");
        uid = tag.guid();

        if (!checkGuid(uid)) {
            errorDescription.base() = errorPrefix.base();
            errorDescription.additionalBases().append(QT_TRANSLATE_NOOP("", "tag's guid is invalid"));
            errorDescription.details() = uid;
            QNWARNING(errorDescription);
            return notes;
        }
    }
    else
    {
        column = QStringLiteral("localTag");
        uid = tag.localUid();
    }

    // Will run all the queries from this method and its sub-methods within a single transaction
    // to prevent multiple drops and re-obtainings of shared lock
    Transaction transaction(m_sqlDatabase, *this, Transaction::Selection);
    Q_UNUSED(transaction)

    uid = sqlEscapeString(uid);

    ErrorString error;
    QString queryCondition = QString("localUid IN (SELECT DISTINCT localNote FROM NoteTags WHERE %1 = '%2')").arg(column, uid);

    notes = listObjects<Note, LocalStorageManager::ListNotesOrder::type>(flag, error, limit, offset, order,
                                                                         orderDirection, queryCondition);
    if (notes.isEmpty() && !error.isEmpty()) {
        errorDescription.base() = errorPrefix.base();
        errorDescription.additionalBases().append(error.base());
        errorDescription.additionalBases().append(error.additionalBases());
        errorDescription.details() = error.details();
        QNWARNING(errorDescription);
        return notes;
    }

    int numNotes = notes.size();
    for(int i = 0; i < numNotes; ++i)
    {
        Note & note = notes[i];

        error.clear();
        bool res = findAndSetTagIdsPerNote(note, error);
        if (!res) {
            errorDescription.base() = errorPrefix.base();
            errorDescription.additionalBases().append(error.base());
            errorDescription.additionalBases().append(error.additionalBases());
            errorDescription.details() = error.details();
            QNWARNING(errorDescription);
            notes.clear();
            return notes;
        }

        error.clear();
        res = findAndSetResourcesPerNote(note, error, withResourceBinaryData);
        if (!res) {
            errorDescription.base() = errorPrefix.base();
            errorDescription.additionalBases().append(error.base());
            errorDescription.additionalBases().append(error.additionalBases());
            errorDescription.details() = error.details();
            QNWARNING(errorDescription);
            notes.clear();
            return notes;
        }

        res = note.checkParameters(error);
        if (!res) {
            errorDescription.base() = errorPrefix.base();
            errorDescription.additionalBases().append(error.base());
            errorDescription.additionalBases().append(error.additionalBases());
            errorDescription.details() = error.details();
            QNWARNING(errorDescription);
            notes.clear();
            return notes;
        }
    }

    return notes;
}

QList<Note> LocalStorageManagerPrivate::listNotes(const LocalStorageManager::ListObjectsOptions flag,
                                                  ErrorString & errorDescription, const bool withResourceBinaryData,
                                                  const size_t limit, const size_t offset,
                                                  const LocalStorageManager::ListNotesOrder::type & order,
                                                  const LocalStorageManager::OrderDirection::type & orderDirection) const
{
    QNDEBUG(QStringLiteral("LocalStorageManagerPrivate::listNotes: flag = ") << flag << QStringLiteral(", withResourceBinaryData = ")
            << (withResourceBinaryData ? QStringLiteral("true") : QStringLiteral("false")));

    ErrorString errorPrefix(QT_TRANSLATE_NOOP("", "Can't list notes from the local storage database"));

    // Will run all the queries from this method and its sub-methods within a single transaction
    // to prevent multiple drops and re-obtainings of shared lock
    Transaction transaction(m_sqlDatabase, *this, Transaction::Selection);
    Q_UNUSED(transaction)

    ErrorString error;
    QList<Note> notes = listObjects<Note, LocalStorageManager::ListNotesOrder::type>(flag, error, limit,
                                                                                     offset, order, orderDirection);
    if (notes.isEmpty() && !error.isEmpty()) {
        errorDescription.base() = errorPrefix.base();
        errorDescription.additionalBases().append(error.base());
        errorDescription.additionalBases().append(error.additionalBases());
        errorDescription.details() = error.details();
        QNWARNING(errorDescription);
        return notes;
    }

    const int numNotes = notes.size();
    for(int i = 0; i < numNotes; ++i)
    {
        Note & note = notes[i];

        error.clear();
        bool res = findAndSetTagIdsPerNote(note, error);
        if (!res) {
            errorDescription.base() = errorPrefix.base();
            errorDescription.additionalBases().append(error.base());
            errorDescription.additionalBases().append(error.additionalBases());
            errorDescription.details() = error.details();
            QNWARNING(errorDescription);
            notes.clear();
            return notes;
        }

        error.clear();
        res = findAndSetResourcesPerNote(note, error, withResourceBinaryData);
        if (!res) {
            errorDescription.base() = errorPrefix.base();
            errorDescription.additionalBases().append(error.base());
            errorDescription.additionalBases().append(error.additionalBases());
            errorDescription.details() = error.details();
            QNWARNING(errorDescription);
            notes.clear();
            return notes;
        }

        res = note.checkParameters(error);
        if (!res) {
            errorDescription.base() = errorPrefix.base();
            errorDescription.additionalBases().append(error.base());
            errorDescription.additionalBases().append(error.additionalBases());
            errorDescription.details() = error.details();
            QNWARNING(errorDescription);
            notes.clear();
            return notes;
        }
    }

    return notes;
}

bool LocalStorageManagerPrivate::expungeNote(Note & note, ErrorString & errorDescription)
{
    ErrorString errorPrefix(QT_TRANSLATE_NOOP("", "Can't expunge note from the local storage database"));

    ErrorString error;
    QString notebookLocalUid;
    bool res = getNotebookLocalUidFromNote(note, notebookLocalUid, error);
    if (Q_UNLIKELY(!res)) {
        errorDescription.base() = errorPrefix.base();
        errorDescription.additionalBases().append(error.base());
        errorDescription.additionalBases().append(error.additionalBases());
        errorDescription.details() = error.details();
        QNWARNING(errorDescription);
        return false;
    }

    error.clear();
    res = canExpungeNoteInNotebook(notebookLocalUid, error);
    if (!res) {
        errorDescription.base() = errorPrefix.base();
        errorDescription.additionalBases().append(error.base());
        errorDescription.additionalBases().append(error.additionalBases());
        errorDescription.details() = error.details();
        QNWARNING(errorDescription);
        return false;
    }

    note.setNotebookLocalUid(notebookLocalUid);

    error.clear();
    QString notebookGuid;
    res = getNotebookGuidForNote(note, notebookGuid, error);
    if (Q_UNLIKELY(!res)) {
        errorDescription.base() = errorPrefix.base();
        errorDescription.additionalBases().append(error.base());
        errorDescription.additionalBases().append(error.additionalBases());
        errorDescription.details() = error.details();
        QNWARNING(errorDescription);
        return false;
    }

    note.setNotebookGuid(notebookGuid);

    QString localUid = note.localUid();

    QString column, uid;
    bool shouldCheckNoteExistence = true;

    bool noteHasGuid = note.hasGuid();
    if (noteHasGuid)
    {
        column = QStringLiteral("guid");
        uid = note.guid();

        if (!checkGuid(uid)) {
            errorDescription.base() = errorPrefix.base();
            errorDescription.additionalBases().append(QT_TRANSLATE_NOOP("", "note's guid is invalid"));
            errorDescription.details() = uid;
            QNWARNING(errorDescription);
            return false;
        }

        if (localUid.isEmpty())
        {
            error.clear();
            res = getNoteLocalUidForGuid(uid, localUid, error);
            if (!res || localUid.isEmpty()) {
                errorDescription.base() = errorPrefix.base();
                errorDescription.additionalBases().append(error.base());
                errorDescription.additionalBases().append(error.additionalBases());
                errorDescription.details() = error.details();
                QNWARNING(errorDescription);
                return false;
            }

            note.setLocalUid(localUid);
            shouldCheckNoteExistence = false;
        }
    }
    else
    {
        column = QStringLiteral("localUid");
        uid = note.localUid();
    }

    uid = sqlEscapeString(uid);

    if (shouldCheckNoteExistence && !rowExists(QStringLiteral("Notes"), column, uid)) {
        errorDescription.base() = errorPrefix.base();
        errorDescription.additionalBases().append(QT_TRANSLATE_NOOP("", "note to be expunged was not found"));
        errorDescription.details() = column;
        errorDescription.details() += QStringLiteral(" = ");
        errorDescription.details() += uid;
        QNWARNING(errorDescription);
        return false;
    }

    QString queryString = QString("DELETE FROM Notes WHERE %1 = '%2'").arg(column, uid);
    QSqlQuery query(m_sqlDatabase);
    res = query.exec(queryString);
    DATABASE_CHECK_AND_SET_ERROR();

    return true;
}

QStringList LocalStorageManagerPrivate::findNoteLocalUidsWithSearchQuery(const NoteSearchQuery & noteSearchQuery,
                                                                         ErrorString & errorDescription) const
{
    QNDEBUG(QStringLiteral("LocalStorageManagerPrivate::findNoteLocalUidsWithSearchQuery: ") << noteSearchQuery);

    if (!noteSearchQuery.isMatcheable()) {
        return QStringList();
    }

    QString queryString;

    // Will run all the queries from this method and its sub-methods within a single transaction
    // to prevent multiple drops and re-obtainings of shared lock
    Transaction transaction(m_sqlDatabase, *this, Transaction::Selection);
    Q_UNUSED(transaction)

    ErrorString errorPrefix(QT_TRANSLATE_NOOP("", "Can't find notes with the note search query"));

    ErrorString error;
    bool res = noteSearchQueryToSQL(noteSearchQuery, queryString, error);
    if (!res) {
        errorDescription.base() = errorPrefix.base();
        errorDescription.additionalBases().append(error.base());
        errorDescription.additionalBases().append(error.additionalBases());
        errorDescription.details() = error.details();
        QNWARNING(errorDescription);
        return QStringList();
    }

    QSqlQuery query(m_sqlDatabase);
    res = query.exec(queryString);
    if (!res) {
        SET_ERROR();
        QNWARNING(QStringLiteral("Full executed SQL query: ") << queryString);
        return QStringList();
    }

    QSet<QString> foundLocalUids;
    while(query.next())
    {
        QSqlRecord rec = query.record();
        int index = rec.indexOf(QStringLiteral("localUid"));   // one way of selecting notes
        if (index < 0) {
            continue;
        }

        QString value = rec.value(index).toString();
        if (value.isEmpty() || foundLocalUids.contains(value)) {
            continue;
        }

        foundLocalUids.insert(value);
    }

    QStringList result;
    result.reserve(foundLocalUids.size());
    for(auto it = foundLocalUids.begin(), end = foundLocalUids.end(); it != end; ++it) {
        result << *it;
    }

    return result;
}

NoteList LocalStorageManagerPrivate::findNotesWithSearchQuery(const NoteSearchQuery & noteSearchQuery,
                                                              ErrorString & errorDescription,
                                                              const bool withResourceBinaryData) const
{
    QNDEBUG(QStringLiteral("LocalStorageManagerPrivate::findNotesWithSearchQuery: ") << noteSearchQuery);

    QStringList foundLocalUids = findNoteLocalUidsWithSearchQuery(noteSearchQuery, errorDescription);
    if (foundLocalUids.isEmpty()) {
        return NoteList();
    }

    QString joinedLocalUids;
    for(auto it = foundLocalUids.begin(), end = foundLocalUids.end(); it != end; ++it)
    {
        const QString & item = *it;

        if (!joinedLocalUids.isEmpty()) {
            joinedLocalUids += QStringLiteral(", ");
        }

        joinedLocalUids += QStringLiteral("'");
        joinedLocalUids += sqlEscapeString(item);
        joinedLocalUids += QStringLiteral("'");
    }

    ErrorString errorPrefix(QT_TRANSLATE_NOOP("", "Can't find notes with the note search query"));

    QString queryString = QString("SELECT * FROM Notes WHERE localUid IN (%1)").arg(joinedLocalUids);
    QSqlQuery query(m_sqlDatabase);
    bool res = query.exec(queryString);
    if (Q_UNLIKELY(!res)) {
        SET_ERROR();
        return NoteList();
    }

    NoteList notes;
    notes.reserve(qMax(query.size(), 0));
    ErrorString error;

    while(query.next())
    {
        notes.push_back(Note());
        Note & note = notes.back();
        note.setLocalUid(QString());

        QSqlRecord rec = query.record();

        error.clear();
        res = fillNoteFromSqlRecord(rec, note, error);
        if (!res) {
            errorDescription.base() = errorPrefix.base();
            errorDescription.additionalBases().append(QT_TRANSLATE_NOOP("", "can't fetch note's tag ids"));
            errorDescription.additionalBases().append(error.base());
            errorDescription.additionalBases().append(error.additionalBases());
            errorDescription.details() = error.details();
            QNWARNING(errorDescription);
            return NoteList();
        }

        error.clear();
        res = findAndSetTagIdsPerNote(note, error);
        if (!res) {
            errorDescription.base() = errorPrefix.base();
            errorDescription.additionalBases().append(QT_TRANSLATE_NOOP("", "can't fetch note's tag ids"));
            errorDescription.additionalBases().append(error.base());
            errorDescription.additionalBases().append(error.additionalBases());
            errorDescription.details() = error.details();
            QNWARNING(errorDescription);
            return NoteList();
        }

        error.clear();
        res = findAndSetResourcesPerNote(note, error, withResourceBinaryData);
        if (!res) {
            errorDescription.base() = errorPrefix.base();
            errorDescription.additionalBases().append(QT_TRANSLATE_NOOP("", "can't fetch note's resources"));
            errorDescription.additionalBases().append(error.base());
            errorDescription.additionalBases().append(error.additionalBases());
            errorDescription.details() = error.details();
            QNWARNING(errorDescription);
            return NoteList();
        }

        error.clear();
        res = note.checkParameters(error);
        if (!res) {
            errorDescription.base() = errorPrefix.base();
            errorDescription.additionalBases().append(QT_TRANSLATE_NOOP("", "can't fetch note's resources"));
            errorDescription.additionalBases().append(error.base());
            errorDescription.additionalBases().append(error.additionalBases());
            errorDescription.details() = error.details();
            QNWARNING(errorDescription);
            return NoteList();
        }
    }

    errorDescription.clear();
    return notes;
}

int LocalStorageManagerPrivate::tagCount(ErrorString & errorDescription) const
{
    ErrorString errorPrefix(QT_TRANSLATE_NOOP("", "Can't get the number of tags in the local storage database"));

    bool res = checkAndPrepareTagCountQuery();
    QSqlQuery & query = m_getTagCountQuery;
    if (!res) {
        SET_ERROR();
        query.finish();
        return -1;
    }

    res = query.exec();
    if (!res) {
        SET_ERROR();
        query.finish();
        return -1;
    }

    if (!query.next()) {
        QNDEBUG(QStringLiteral("Found no tags in local storage database"));
        query.finish();
        return 0;
    }

    bool conversionResult = false;
    int count = query.value(0).toInt(&conversionResult);
    query.finish();

    if (!conversionResult) {
        SET_INT_CONVERSION_ERROR();
        return -1;
    }

    return count;
}

bool LocalStorageManagerPrivate::addTag(Tag & tag, ErrorString & errorDescription)
{
    ErrorString errorPrefix(QT_TRANSLATE_NOOP("", "Can't add tag to the local storage database"));

    ErrorString error;
    bool res = tag.checkParameters(error);
    if (!res) {
        errorDescription.base() = errorPrefix.base();
        errorDescription.additionalBases().append(error.base());
        errorDescription.additionalBases().append(error.additionalBases());
        errorDescription.details() = error.details();
        QNWARNING(QStringLiteral("Found invalid tag: ") << tag);
        return false;
    }

    QString localUid = tag.localUid();

    QString column, uid;
    bool shouldCheckTagExistence = true;

    bool tagHasGuid = tag.hasGuid();
    if (tagHasGuid)
    {
        column = QStringLiteral("guid");
        uid = tag.guid();

        if (localUid.isEmpty())
        {
            error.clear();
            res = getTagLocalUidForGuid(uid, localUid, error);
            if (res || !localUid.isEmpty()) {
                errorDescription.base() = errorPrefix.base();
                errorDescription.additionalBases().append(QT_TRANSLATE_NOOP("", "found already existing tag"));
                errorDescription.details() = QStringLiteral("guid = ");
                errorDescription.details() += uid;
                QNWARNING(errorDescription);
                return false;
            }

            localUid = UidGenerator::Generate();
            tag.setLocalUid(localUid);
            shouldCheckTagExistence = false;
        }
    }
    else
    {
        column = QStringLiteral("localUid");
        uid = tag.localUid();
    }

    if (shouldCheckTagExistence && rowExists(QStringLiteral("Tags"), column, uid)) {
        errorDescription.base() = errorPrefix.base();
        errorDescription.additionalBases().append(QT_TRANSLATE_NOOP("", "tag already exists"));
        errorDescription.details() = column;
        errorDescription.details() += QStringLiteral(" = ");
        errorDescription.details() += uid;
        QNWARNING(errorDescription);
        return false;
    }

    error.clear();
    res = insertOrReplaceTag(tag, error);
    if (!res) {
        errorDescription.base() = errorPrefix.base();
        errorDescription.additionalBases().append(error.base());
        errorDescription.additionalBases().append(error.additionalBases());
        errorDescription.details() = error.details();
        QNWARNING(errorDescription);
        return false;
    }

    return true;
}

bool LocalStorageManagerPrivate::updateTag(Tag & tag, ErrorString & errorDescription)
{
    ErrorString errorPrefix(QT_TRANSLATE_NOOP("", "Can't update tag in the local storage database"));

    ErrorString error;
    bool res = tag.checkParameters(error);
    if (!res) {
        errorDescription.base() = errorPrefix.base();
        errorDescription.additionalBases().append(error.base());
        errorDescription.additionalBases().append(error.additionalBases());
        errorDescription.details() = error.details();
        QNWARNING(QStringLiteral("Found invalid tag: ") << tag);
        return false;
    }

    QString localUid = tag.localUid();

    QString column, uid;
    bool shouldCheckTagExistence = true;

    bool tagHasGuid = tag.hasGuid();
    if (tagHasGuid)
    {
        column = QStringLiteral("guid");
        uid = tag.guid();

        if (localUid.isEmpty())
        {
            error.clear();
            res = getTagLocalUidForGuid(uid, localUid, error);
            if (!res || localUid.isEmpty()) {
                errorDescription.base() = errorPrefix.base();
                errorDescription.additionalBases().append(error.base());
                errorDescription.additionalBases().append(error.additionalBases());
                errorDescription.details() = error.details();
                return false;
            }

            tag.setLocalUid(localUid);
            shouldCheckTagExistence = false;
        }
    }
    else
    {
        column = QStringLiteral("localUid");
        uid = tag.localUid();
    }

    if (shouldCheckTagExistence && !rowExists(QStringLiteral("Tags"), column, uid)) {
        errorDescription.base() = errorPrefix.base();
        errorDescription.additionalBases().append(QT_TRANSLATE_NOOP("", "tag was not found in the local storage database"));
        errorDescription.details() = column;
        errorDescription.details() += QStringLiteral(" = ");
        errorDescription.details() += uid;
        QNWARNING(errorDescription);
        return false;
    }

    error.clear();
    res = insertOrReplaceTag(tag, error);
    if (!res) {
        errorDescription.base() = errorPrefix.base();
        errorDescription.additionalBases().append(error.base());
        errorDescription.additionalBases().append(error.additionalBases());
        errorDescription.details() = error.details();
        QNWARNING(errorDescription);
        return false;
    }

    return true;
}

bool LocalStorageManagerPrivate::findTag(Tag & tag, ErrorString & errorDescription) const
{
    QNDEBUG(QStringLiteral("LocalStorageManagerPrivate::findTag"));

    ErrorString errorPrefix(QT_TRANSLATE_NOOP("", "Can't find tag in the local storage database"));

    QString column, value;
    bool tagHasGuid = tag.hasGuid();
    if (tagHasGuid)
    {
        column = QStringLiteral("guid");
        value = tag.guid();

        if (!checkGuid(value)) {
            errorDescription.base() = errorPrefix.base();
            errorDescription.additionalBases().append(QT_TRANSLATE_NOOP("", "tag's guid is invalid"));
            errorDescription.details() = value;
            QNWARNING(errorDescription);
            return false;
        }
    }
    else if (tag.localUid().isEmpty())
    {
        if (!tag.hasName()) {
            errorDescription.base() = errorPrefix.base();
            errorDescription.additionalBases().append(QT_TRANSLATE_NOOP("", "need either guid or local uid or name as a search criteria"));
            QNWARNING(errorDescription);
            return false;
        }

        column = QStringLiteral("nameLower");
        value = tag.name().toLower();
    }
    else
    {
        column = QStringLiteral("localUid");
        value = tag.localUid();
    }

    value = sqlEscapeString(value);

    QString queryString = QString("SELECT localUid, guid, linkedNotebookGuid, updateSequenceNumber, name, parentGuid, "
                                  "parentLocalUid, isDirty, isLocal, isLocal, isFavorited "
                                  "FROM Tags WHERE (%1 = '%2'").arg(column,value);

    if (tag.hasLinkedNotebookGuid()) {
        QString linkedNotebookGuid = tag.linkedNotebookGuid();
        queryString += QString(" AND linkedNotebookGuid = '%1')").arg(sqlEscapeString(linkedNotebookGuid));
    }
    else {
        queryString += ")";
    }

    tag.clear();

    QSqlQuery query(m_sqlDatabase);
    bool res = query.exec(queryString);
    DATABASE_CHECK_AND_SET_ERROR();

    if (!query.next()) {
        QNDEBUG(errorDescription);
        return false;
    }

    QSqlRecord record = query.record();

    ErrorString error;
    res = fillTagFromSqlRecord(record, tag, error);
    if (!res) {
        errorDescription.base() = errorPrefix.base();
        errorDescription.additionalBases().append(error.base());
        errorDescription.additionalBases().append(error.additionalBases());
        errorDescription.details() = error.details();
        QNWARNING(errorDescription);
        return false;
    }

    return true;
}

QList<Tag> LocalStorageManagerPrivate::listAllTagsPerNote(const Note & note, ErrorString & errorDescription,
                                                          const LocalStorageManager::ListObjectsOptions & flag,
                                                          const size_t limit, const size_t offset,
                                                          const LocalStorageManager::ListTagsOrder::type & order,
                                                          const LocalStorageManager::OrderDirection::type & orderDirection) const
{
    QNDEBUG(QStringLiteral("LocalStorageManagerPrivate::listAllTagsPerNote"));

    QList<Tag> tags;
    ErrorString errorPrefix(QT_TRANSLATE_NOOP("", "Can't list all tags per note from the local storage database"));

    QString column, uid;
    bool noteHasGuid = note.hasGuid();
    if (noteHasGuid)
    {
        column = QStringLiteral("note");
        uid = note.guid();

        if (!checkGuid(uid)) {
            errorDescription.base() = errorPrefix.base();
            errorDescription.additionalBases().append(QT_TRANSLATE_NOOP("", "note's guid is invalid"));
            errorDescription.details() = uid;
            QNWARNING(errorDescription);
            return tags;
        }
    }
    else
    {
        column = QStringLiteral("localNote");
        uid = note.localUid();
    }

    // Will run all the queries from this method and its sub-methods within a single transaction
    // to prevent multiple drops and re-obtainings of shared lock
    Transaction transaction(m_sqlDatabase, *this, Transaction::Selection);
    Q_UNUSED(transaction)

    uid = sqlEscapeString(uid);

    QString queryString = QString("SELECT localTag FROM NoteTags WHERE %1 = '%2'").arg(column,uid);
    QSqlQuery query(m_sqlDatabase);
    bool res = query.exec(queryString);
    if (!res) {
        SET_ERROR();
        return tags;
    }

    if (query.size() == 0) {
        QNDEBUG(QStringLiteral("No tags for this note were found"));
        return tags;
    }

    QStringList tagLocalUids;
    tagLocalUids.reserve(std::max(query.size(), 0));

    while(query.next())
    {
        tagLocalUids << QString();
        QString & tagLocalUid = tagLocalUids.back();
        tagLocalUid = query.value(0).toString();

        if (tagLocalUid.isEmpty()) {
            errorDescription.base() = errorPrefix.base();
            errorDescription.additionalBases().append(QT_TRANSLATE_NOOP("", "internal error: no tag's local uid in the result of SQL query"));
            tags.clear();
            return tags;
        }
    }

    QString noteGuidSqlQueryCondition = QStringLiteral("localUid IN (");
    const int numTagLocalUids = tagLocalUids.size();
    for(int i = 0; i < numTagLocalUids; ++i)
    {
        noteGuidSqlQueryCondition += QString("'%1'").arg(sqlEscapeString(tagLocalUids[i]));
        if (i != (numTagLocalUids - 1)) {
            noteGuidSqlQueryCondition += QStringLiteral(", ");
        }
    }
    noteGuidSqlQueryCondition += QStringLiteral(")");

    ErrorString error;
    tags = listObjects<Tag, LocalStorageManager::ListTagsOrder::type>(flag, error, limit, offset, order,
                                                                      orderDirection, noteGuidSqlQueryCondition);
    if (tags.isEmpty() && !error.isEmpty()) {
        errorDescription.base() = errorPrefix.base();
        errorDescription.additionalBases().append(error.base());
        errorDescription.additionalBases().append(error.additionalBases());
        errorDescription.details() = error.details();
        QNWARNING(errorDescription);
    }

    return tags;
}

QList<Tag> LocalStorageManagerPrivate::listAllTags(ErrorString & errorDescription, const size_t limit, const size_t offset,
                                                   const LocalStorageManager::ListTagsOrder::type & order,
                                                   const LocalStorageManager::OrderDirection::type & orderDirection,
                                                   const QString & linkedNotebookGuid) const
{
    QNDEBUG(QStringLiteral("LocalStorageManagerPrivate::listAllTags"));
    return listTags(LocalStorageManager::ListAll, errorDescription, limit, offset, order, orderDirection, linkedNotebookGuid);
}

QList<Tag> LocalStorageManagerPrivate::listTags(const LocalStorageManager::ListObjectsOptions flag,
                                                ErrorString & errorDescription, const size_t limit, const size_t offset,
                                                const LocalStorageManager::ListTagsOrder::type & order,
                                                const LocalStorageManager::OrderDirection::type & orderDirection,
                                                const QString & linkedNotebookGuid) const
{
    QNDEBUG(QStringLiteral("LocalStorageManagerPrivate::listTags: flag = ") << flag);

    QString linkedNotebookGuidSqlQueryCondition;
    if (!linkedNotebookGuid.isNull()) {
       linkedNotebookGuidSqlQueryCondition = (linkedNotebookGuid.isEmpty()
                                              ? QStringLiteral("linkedNotebookGuid IS NULL")
                                              : QString("linkedNotebookGuid = '%1'").arg(sqlEscapeString(linkedNotebookGuid)));
    }

    return listObjects<Tag, LocalStorageManager::ListTagsOrder::type>(flag, errorDescription, limit, offset, order, orderDirection,
                                                                      linkedNotebookGuidSqlQueryCondition);
}

bool LocalStorageManagerPrivate::expungeTag(Tag & tag, ErrorString & errorDescription)
{
    ErrorString errorPrefix(QT_TRANSLATE_NOOP("", "Can't expunge tag from the local storage database"));

    QString localUid = tag.localUid();

    QString column, parentColumn, uid;
    bool shouldCheckTagExistence = true;

    bool tagHasGuid = tag.hasGuid();
    if (tagHasGuid)
    {
        column = QStringLiteral("guid");
        parentColumn = QStringLiteral("parentGuid");
        uid = tag.guid();

        if (!checkGuid(uid)) {
            errorDescription.base() = errorPrefix.base();
            errorDescription.additionalBases().append(QT_TRANSLATE_NOOP("", "tag's guid is invalid"));
            errorDescription.details() = uid;
            QNWARNING(errorDescription);
            return false;
        }

        if (localUid.isEmpty())
        {
            ErrorString error;
            bool res = getTagLocalUidForGuid(uid, localUid, error);
            if (!res || localUid.isEmpty()) {
                errorDescription.base() = errorPrefix.base();
                errorDescription.additionalBases().append(QT_TRANSLATE_NOOP("", "tag to be expunged was not found "
                                                                            "in the local storage database"));
                errorDescription.details() = QStringLiteral("local uid = ");
                errorDescription.details() += localUid;
                QNWARNING(errorDescription);
                return false;
            }

            tag.setLocalUid(localUid);
            shouldCheckTagExistence = false;
        }
    }
    else
    {
        column = QStringLiteral("localUid");
        parentColumn = QStringLiteral("parentLocalUid");
        uid = tag.localUid();
    }

    uid = sqlEscapeString(uid);

    if (shouldCheckTagExistence && !rowExists(QStringLiteral("Tags"), column, uid)) {
        errorDescription.base() = errorPrefix.base();
        errorDescription.additionalBases().append(QT_TRANSLATE_NOOP("", "tag to be expunged was not found "
                                                                    "in the local storage database"));
        errorDescription.details() = column;
        errorDescription.details() += QStringLiteral(" = ");
        errorDescription.details() += uid;
        QNWARNING(errorDescription);
        return false;
    }

    QSqlQuery query(m_sqlDatabase);

    // Removing child tags first
    QString queryString = QString("DELETE FROM Tags WHERE %1='%2'").arg(parentColumn,uid);
    bool res = query.exec(queryString);
    DATABASE_CHECK_AND_SET_ERROR();

    queryString = QString("DELETE FROM Tags WHERE %1='%2'").arg(column,uid);
    res = query.exec(queryString);
    DATABASE_CHECK_AND_SET_ERROR();

    return true;
}

bool LocalStorageManagerPrivate::expungeNotelessTagsFromLinkedNotebooks(ErrorString & errorDescription)
{
    ErrorString errorPrefix(QT_TRANSLATE_NOOP("", "Can't expunge tags from linked notebooks not connected to any notes"));

    QString queryString = QStringLiteral("DELETE FROM Tags WHERE ((linkedNotebookGuid IS NOT NULL) AND "
                                         "(localUid NOT IN (SELECT localTag FROM NoteTags)))");
    QSqlQuery query(m_sqlDatabase);
    bool res = query.exec(queryString);
    DATABASE_CHECK_AND_SET_ERROR();

    return true;
}

int LocalStorageManagerPrivate::enResourceCount(ErrorString & errorDescription) const
{
    ErrorString errorPrefix(QT_TRANSLATE_NOOP("", "Can't get the number of resources in the local storage database"));

    bool res = checkAndPrepareResourceCountQuery();
    QSqlQuery & query = m_getResourceCountQuery;
    if (!res) {
        SET_ERROR();
        query.finish();
        return -1;
    }

    res = query.exec();
    if (!res) {
        SET_ERROR();
        query.finish();
        return -1;
    }

    if (!query.next()) {
        QNDEBUG(QStringLiteral("Found no resources in local storage database"));
        query.finish();
        return 0;
    }

    bool conversionResult = false;
    int count = query.value(0).toInt(&conversionResult);
    query.finish();

    if (!conversionResult) {
        SET_INT_CONVERSION_ERROR();
        return -1;
    }

    return count;
}

bool LocalStorageManagerPrivate::findEnResource(Resource & resource, ErrorString & errorDescription,
                                                const bool withBinaryData) const
{
    QNDEBUG(QStringLiteral("LocalStorageManagerPrivate::findEnResource"));

    ErrorString errorPrefix(QT_TRANSLATE_NOOP("", "Can't find resource in the local storage database"));

    QString column, uid;
    bool resourceHasGuid = resource.hasGuid();
    if (resourceHasGuid)
    {
        column = QStringLiteral("resourceGuid");
        uid = resource.guid();

        if (!checkGuid(uid)) {
            errorDescription.base() = errorPrefix.base();
            errorDescription.additionalBases().append(QT_TRANSLATE_NOOP("", "resource's guid is invalid"));
            errorDescription.details() = uid;
            QNWARNING(errorDescription);
            return false;
        }
    }
    else
    {
        column = QStringLiteral("resourceLocalUid");
        uid = resource.localUid();
    }

    resource.clear();

    QString resourcesTable = (withBinaryData
                              ? QStringLiteral("Resources")
                              : QStringLiteral("ResourcesWithoutBinaryData"));

    uid = sqlEscapeString(uid);

    QString queryString = QString("SELECT * FROM %1 "
                                  "LEFT OUTER JOIN ResourceAttributes "
                                  "ON %1.resourceLocalUid = ResourceAttributes.resourceLocalUid "
                                  "LEFT OUTER JOIN ResourceAttributesApplicationDataKeysOnly "
                                  "ON %1.resourceLocalUid = ResourceAttributesApplicationDataKeysOnly.resourceLocalUid "
                                  "LEFT OUTER JOIN ResourceAttributesApplicationDataFullMap "
                                  "ON %1.resourceLocalUid = ResourceAttributesApplicationDataFullMap.resourceLocalUid "
                                  "LEFT OUTER JOIN NoteResources ON %1.resourceLocalUid = NoteResources.localResource "
                                  "WHERE %1.%2 = '%3'")
                                 .arg(resourcesTable,column,uid);

    QSqlQuery query(m_sqlDatabase);
    bool res = query.exec(queryString);
    DATABASE_CHECK_AND_SET_ERROR();

    size_t counter = 0;
    while(query.next()) {
        QSqlRecord rec = query.record();
        fillResourceFromSqlRecord(rec, withBinaryData, resource);
        ++counter;
    }

    if (!counter) {
        QNDEBUG(errorDescription);
        return false;
    }

    return true;
}

bool LocalStorageManagerPrivate::expungeEnResource(Resource & resource, ErrorString & errorDescription)
{
    ErrorString errorPrefix(QT_TRANSLATE_NOOP("", "Can't expunge resource from the local storage database"));

    ErrorString error;
    QString noteLocalUid;
    bool res = getNoteLocalUidFromResource(resource, noteLocalUid, error);
    if (Q_UNLIKELY(!res)) {
        errorDescription.base() = errorPrefix.base();
        errorDescription.additionalBases().append(error.base());
        errorDescription.additionalBases().append(error.additionalBases());
        errorDescription.details() = error.details();
        QNWARNING(errorDescription);
        return false;
    }

    if (Q_UNLIKELY(noteLocalUid.isEmpty())) {
        errorDescription.base() = errorPrefix.base();
        errorDescription.additionalBases().append(QT_TRANSLATE_NOOP("", "note's local uid corresponding to the resource is empty"));
        errorDescription.details() = QStringLiteral("local uid = ");
        errorDescription.details() += noteLocalUid;
        QNWARNING(errorDescription);
        return false;
    }

    // Expunging the resource is essentially the update of note, so need to check if we're allowed to do that
    error.clear();
    QString notebookLocalUid;
    Note dummyNote;
    dummyNote.setLocalUid(noteLocalUid);
    res = getNotebookLocalUidFromNote(dummyNote, notebookLocalUid, error);
    if (Q_UNLIKELY(!res)) {
        errorDescription.base() = errorPrefix.base();
        errorDescription.additionalBases().append(error.base());
        errorDescription.additionalBases().append(error.additionalBases());
        errorDescription.details() = error.details();
        QNWARNING(errorDescription);
        return false;
    }

    error.clear();
    res = canUpdateNoteInNotebook(notebookLocalUid, error);
    if (!res) {
        errorDescription.base() = errorPrefix.base();
        errorDescription.additionalBases().append(error.base());
        errorDescription.additionalBases().append(error.additionalBases());
        errorDescription.details() = error.details();
        QNWARNING(errorDescription);
        return false;
    }

    QString localUid = resource.localUid();

    QString column, uid;
    bool shouldCheckResourceExistence = true;

    bool resourceHasGuid = resource.hasGuid();
    if (resourceHasGuid)
    {
        column = QStringLiteral("resourceGuid");
        uid = resource.guid();

        if (!checkGuid(uid)) {
            errorDescription.base() = errorPrefix.base();
            errorDescription.additionalBases().append(QT_TRANSLATE_NOOP("", "resource's guid is invalid"));
            errorDescription.details() = uid;
            QNWARNING(errorDescription);
            return false;
        }

        if (localUid.isEmpty())
        {
            error.clear();
            res = getResourceLocalUidForGuid(uid, localUid, error);
            if (!res || localUid.isEmpty()) {
                errorDescription.base() = errorPrefix.base();
                errorDescription.additionalBases().append(QT_TRANSLATE_NOOP("", "resource to be updated was not found "
                                                                            "in the local storage database"));
                errorDescription.details() = QStringLiteral("guid = ");
                errorDescription.details() += uid;
                QNCRITICAL(errorDescription);
                return false;
            }

            resource.setLocalUid(localUid);
            shouldCheckResourceExistence = false;
        }
    }
    else
    {
        column = QStringLiteral("resourceLocalUid");
        uid = resource.localUid();
    }

    uid = sqlEscapeString(uid);

    if (shouldCheckResourceExistence && !rowExists(QStringLiteral("Resources"), column, uid)) {
        errorDescription.base() = errorPrefix.base()
        errorDescription.additionalBases().append(QT_TRANSLATE_NOOP("", "resource to be expunged was not found in the local storage database"));
        errorDescription.details() = column;
        errorDescription.details() += QStringLiteral(" = ");
        errorDescription.details() += uid;
        QNWARNING(errorDescription);
        return false;
    }

    QString queryString = QString("DELETE FROM Resources WHERE %1 = '%2'").arg(column,uid);
    QSqlQuery query(m_sqlDatabase);
    res = query.exec(queryString);
    DATABASE_CHECK_AND_SET_ERROR();

    return true;
}

int LocalStorageManagerPrivate::savedSearchCount(ErrorString & errorDescription) const
{
    ErrorString errorPrefix(QT_TRANSLATE_NOOP("", "Can't get the number of saved searches in the local storage database"));

    QSqlQuery & query = m_getSavedSearchCountQuery;
    bool res = checkAndPrepareGetSavedSearchCountQuery();
    if (!res) {
        SET_ERROR();
        query.finish();
        return -1;
    }

    res = query.exec();
    if (!res) {
        SET_ERROR();
        query.finish();
        return -1;
    }

    if (!query.next()) {
        QNDEBUG(QStringLiteral("Found no saved searches in local storage database"));
        query.finish();
        return 0;
    }

    bool conversionResult = false;
    int count = query.value(0).toInt(&conversionResult);
    query.finish();

    if (!conversionResult) {
        SET_INT_CONVERSION_ERROR();
        return -1;
    }

    return count;
}

bool LocalStorageManagerPrivate::addSavedSearch(SavedSearch & search, ErrorString & errorDescription)
{
    ErrorString errorPrefix(QT_TRANSLATE_NOOP("", "Can't add saved search to the local storage database"));

    ErrorString error;
    bool res = search.checkParameters(error);
    if (!res) {
        errorDescription.base() = errorPrefix.base();
        errorDescription.additionalBases().append(error.base());
        errorDescription.additionalBases().append(error.additionalBases());
        errorDescription.details() = error.details();
        QNWARNING(QStringLiteral("Found invalid SavedSearch: ") << search << QStringLiteral("\nError: ") << error);
        return false;
    }

    QString localUid = search.localUid();

    QString column, uid;
    bool shouldCheckSearchExistence = true;

    bool searchHasGuid = search.hasGuid();
    if (searchHasGuid)
    {
        column = QStringLiteral("guid");
        uid = search.guid();

        if (localUid.isEmpty())
        {
            error.clear();
            res = getSavedSearchLocalUidForGuid(uid, localUid, error);
            if (res || !localUid.isEmpty()) {
                errorDescription.base() = errorPrefix.base();
                errorDescription.additionalBases().append(QT_TRANSLATE_NOOP("", "saved search already exists"));
                errorDescription.details() = column;
                errorDescription.details() += QStringLiteral(" = ");
                errorDescription.details() += uid;
                QNWARNING(errorDescription);
                return false;
            }

            localUid = UidGenerator::Generate();
            search.setLocalUid(localUid);
            shouldCheckSearchExistence = false;
        }
    }
    else
    {
        column = QStringLiteral("localUid");
        uid = search.localUid();
    }

    if (shouldCheckSearchExistence && rowExists(QStringLiteral("SavedSearches"), column, uid)) {
        errorDescription.base() = errorPrefix.base();
        errorDescription.additionalBases().append(QT_TRANSLATE_NOOP("", "saved search already exists"));
        errorDescription.details() = column;
        errorDescription.details() += QStringLiteral(" = ");
        errorDescription.details() += uid;
        QNWARNING(errorDescription);
        return false;
    }

    error.clear();
    res = insertOrReplaceSavedSearch(search, error);
    if (!res) {
        errorDescription.base() = errorPrefix.base();
        errorDescription.additionalBases().append(error.base());
        errorDescription.additionalBases().append(error.additionalBases());
        errorDescription.details() = error.details();
        QNWARNING(error);
        return false;
    }

    return true;
}

bool LocalStorageManagerPrivate::updateSavedSearch(SavedSearch & search, ErrorString & errorDescription)
{
    ErrorString errorPrefix(QT_TRANSLATE_NOOP("", "Can't update saved search in the local storage database"));

    ErrorString error;
    bool res = search.checkParameters(error);
    if (!res) {
        errorDescription.base() = errorPrefix.base();
        errorDescription.additionalBases().append(error.base());
        errorDescription.additionalBases().append(error.additionalBases());
        errorDescription.details() = error.details();
        QNWARNING(QStringLiteral("Found invalid SavedSearch: ") << search << QStringLiteral("\nError: ") << error);
        return false;
    }

    QString localUid = search.localUid();

    QString column, uid;
    bool shouldCheckSearchExistence = true;

    bool searchHasGuid = search.hasGuid();
    if (searchHasGuid)
    {
        column = QStringLiteral("guid");
        uid = search.guid();

        if (localUid.isEmpty())
        {
            error.clear();
            res = getSavedSearchLocalUidForGuid(uid, localUid, error);
            if (!res || localUid.isEmpty()) {
                errorDescription.base() = errorPrefix.base();
                errorDescription.additionalBases().append(QT_TRANSLATE_NOOP("", "Saved search to be updated was not found "
                                                                            "in the local storage database"));
                errorDescription.details() = column;
                errorDescription.details() += QStringLiteral(" = ");
                errorDescription.details() += uid;
                QNWARNING(errorDescription);
                return false;
            }

            search.setLocalUid(localUid);
            shouldCheckSearchExistence = false;
        }
    }
    else
    {
        column = QStringLiteral("localUid");
        uid = search.localUid();
    }

    if (shouldCheckSearchExistence && !rowExists(QStringLiteral("SavedSearches"), column, uid)) {
        errorDescription.base() = errorPrefix.base();
        errorDescription.additionalBases().append(QT_TRANSLATE_NOOP("", "Saved search to be updated was not found "
                                                                    "in the local storage database"));
        errorDescription.details() = column;
        errorDescription.details() += QStringLiteral(" = ");
        errorDescription.details() += uid;
        QNWARNING(errorDescription);
        return false;
    }

    error.clear();
    res = insertOrReplaceSavedSearch(search, error);
    if (!res) {
        errorDescription.base() = errorPrefix.base();
        errorDescription.additionalBases().append(error.base());
        errorDescription.additionalBases().append(error.additionalBases());
        errorDescription.details() = error.details();
        QNWARNING(error);
        return false;
    }

    return true;
}

bool LocalStorageManagerPrivate::findSavedSearch(SavedSearch & search, ErrorString & errorDescription) const
{
    QNDEBUG(QStringLiteral("LocalStorageManagerPrivate::findSavedSearch"));

    ErrorString errorPrefix(QT_TRANSLATE_NOOP("", "Can't find saved search in the local storage database"));

    QString column, value;
    bool searchHasGuid = search.hasGuid();
    if (searchHasGuid)
    {
        column = QStringLiteral("guid");
        value = search.guid();

        if (!checkGuid(value)) {
            errorDescription.base() = errorPrefix.base();
            errorDescription.additionalBases().append(QT_TRANSLATE_NOOP("", "saved search's guid is invalid"));
            errorDescription.details() = value;
            return false;
        }
    }
    else if (search.localUid().isEmpty())
    {
        if (!search.hasName()) {
            errorDescription.base() = errorPrefix.base();
            errorDescription.additionalBases().append(QT_TRANSLATE_NOOP("", "need either guid or local uid or name as search criteria"));
            QNWARNING(errorDescription);
            return false;
        }

        column = QStringLiteral("nameLower");
        value = search.name().toLower();
    }
    else
    {
        column = QStringLiteral("localUid");
        value = search.localUid();
    }

    search.clear();

    value = sqlEscapeString(value);

    QString queryString = QString("SELECT localUid, guid, name, query, format, updateSequenceNumber, isDirty, isLocal, "
                                  "includeAccount, includePersonalLinkedNotebooks, includeBusinessLinkedNotebooks, "
                                  "isFavorited FROM SavedSearches WHERE %1 = '%2'").arg(column,value);
    QSqlQuery query(m_sqlDatabase);
    bool res = query.exec(queryString);
    DATABASE_CHECK_AND_SET_ERROR();

    if (!query.next()) {
        QNDEBUG(errorDescription);
        return false;
    }

    QSqlRecord rec = query.record();
    ErrorString error;
    res = fillSavedSearchFromSqlRecord(rec, search, error);
    if (!res) {
        errorDescription.base() = errorPrefix.base();
        errorDescription.additionalBases().append(error.base());
        errorDescription.additionalBases().append(error.additionalBases());
        errorDescription.details() = error.details();
        QNWARNING(errorDescription);
        return false;
    }

    return true;
}

QList<SavedSearch> LocalStorageManagerPrivate::listAllSavedSearches(ErrorString & errorDescription, const size_t limit, const size_t offset,
                                                                    const LocalStorageManager::ListSavedSearchesOrder::type & order,
                                                                    const LocalStorageManager::OrderDirection::type & orderDirection) const
{
    QNDEBUG(QStringLiteral("LocalStorageManagerPrivate::listAllSavedSearches"));
    return listSavedSearches(LocalStorageManager::ListAll, errorDescription, limit, offset, order, orderDirection);
}

QList<SavedSearch> LocalStorageManagerPrivate::listSavedSearches(const LocalStorageManager::ListObjectsOptions flag,
                                                                 ErrorString & errorDescription, const size_t limit, const size_t offset,
                                                                 const LocalStorageManager::ListSavedSearchesOrder::type & order,
                                                                 const LocalStorageManager::OrderDirection::type & orderDirection) const
{
    QNDEBUG(QStringLiteral("LocalStorageManagerPrivate::listSavedSearches: flag = ") << flag);
    return listObjects<SavedSearch, LocalStorageManager::ListSavedSearchesOrder::type>(flag, errorDescription, limit, offset, order, orderDirection);
}

bool LocalStorageManagerPrivate::expungeSavedSearch(SavedSearch & search, ErrorString & errorDescription)
{
    ErrorString errorPrefix(QT_TRANSLATE_NOOP("", "Can't expunge saved search from the local storage database"));

    ErrorString error;
    bool res = search.checkParameters(error);
    if (!res) {
        errorDescription.base() = errorPrefix.base();
        errorDescription.additionalBases().append(error.base());
        errorDescription.additionalBases().append(error.additionalBases());
        errorDescription.details() = error.details();
        QNWARNING(QStringLiteral("Found invalid SavedSearch: ") << search << QStringLiteral("\nError: ") << error);
        return false;
    }

    QString localUid = search.localUid();

    QString column, uid;
    bool shouldCheckSearchExistence = true;

    bool searchHasGuid = search.hasGuid();
    if (searchHasGuid)
    {
        column = QStringLiteral("guid");
        uid = search.guid();

        if (localUid.isEmpty())
        {
            error.clear();
            res = getSavedSearchLocalUidForGuid(uid, localUid, error);
            if (!res || localUid.isEmpty()) {
                errorDescription.base() = errorPrefix.base();
                errorDescription.additionalBases().append(QT_TRANSLATE_NOOP("", "saved search to be expunged was not found "
                                                                            "in the local storage database"));
                errorDescription.details() = column;
                errorDescription.details() += QStringLiteral(" = ");
                errorDescription.details() += uid;
                QNWARNING(errorDescription);
                return false;
            }

            search.setLocalUid(localUid);
            shouldCheckSearchExistence = false;
        }
    }
    else
    {
        column = QStringLiteral("localUid");
        uid = search.localUid();
    }

    uid = sqlEscapeString(uid);

    if (shouldCheckSearchExistence && !rowExists(QStringLiteral("SavedSearches"), column, uid)) {
        errorDescription.base() = errorPrefix.base();
        errorDescription.additionalBases().append(QT_TRANSLATE_NOOP("", "saved search to be expunged was not found "
                                                                    "in the local storage database"));
        errorDescription.details() = column;
        errorDescription.details() += QStringLiteral(" = ");
        errorDescription.details() += uid;
        QNWARNING(errorDescription);
        return false;
    }

    QString queryString = QString("DELETE FROM SavedSearches WHERE %1='%2'").arg(column,uid);
    QSqlQuery query(m_sqlDatabase);
    res = query.exec(queryString);
    DATABASE_CHECK_AND_SET_ERROR();

    return true;
}

void LocalStorageManagerPrivate::processPostTransactionException(ErrorString message, QSqlError error) const
{
    QNCRITICAL(message << QStringLiteral(": ") << error);
    message.details() += error.text();
    throw DatabaseSqlErrorException(message);
}

bool LocalStorageManagerPrivate::addEnResource(Resource & resource, ErrorString & errorDescription)
{
    ErrorString errorPrefix(QT_TRANSLATE_NOOP("", "Can't add resource to the local storage database"));

    ErrorString error;
    if (!resource.checkParameters(error)) {
        errorDescription.base() = errorPrefix.base();
        errorDescription.additionalBases().append(error.base());
        errorDescription.additionalBases().append(error.additionalBases());
        errorDescription.details() = error.details();
        QNWARNING(QStringLiteral("Found invalid resource: ") << resource);
        return false;
    }

    if (!resource.hasNoteGuid() && !resource.hasNoteLocalUid()) {
        errorDescription.base() = errorPrefix.base();
        errorDescription.additionalBases().append(QT_TRANSLATE_NOOP("", "both resource's note local uid and note guid are empty"));
        QNWARNING(errorDescription << QStringLiteral(", resource: ") << resource);
        return false;
    }

    // Adding the resource is essentially the update of note, so need to check if we're allowed to do that
    QString notebookLocalUid;
    Note dummyNote;
    if (resource.hasNoteGuid()) {
        dummyNote.setGuid(resource.noteGuid());
    }
    else {
        dummyNote.setLocalUid(resource.noteLocalUid());
    }

    error.clear();
    bool res = getNotebookLocalUidFromNote(dummyNote, notebookLocalUid, error);
    if (Q_UNLIKELY(!res)) {
        errorDescription.base() = errorPrefix.base();
        errorDescription.additionalBases().append(error.base());
        errorDescription.additionalBases().append(error.additionalBases());
        errorDescription.details() = error.details();
        QNWARNING(errorDescription);
        return false;
    }

    error.clear();
    res = canUpdateNoteInNotebook(notebookLocalUid, error);
    if (!res) {
        errorDescription.base() = errorPrefix.base();
        errorDescription.additionalBases().append(error.base());
        errorDescription.additionalBases().append(error.additionalBases());
        errorDescription.details() = error.details();
        QNWARNING(errorDescription);
        return false;
    }

    // Now can continue with adding the resource
    error.clear();
    res = complementResourceNoteIds(resource, error);
    if (!res) {
        return false;
    }

    int resourceIndexInNote = -1;

    QString noteLocalUid = resource.noteLocalUid();
    noteLocalUid = sqlEscapeString(noteLocalUid);

    QString queryString = QString("SELECT COUNT(*) FROM NoteResources WHERE localNote = '%1'").arg(noteLocalUid);
    QSqlQuery query(m_sqlDatabase);
    res = query.exec(queryString);
    DATABASE_CHECK_AND_SET_ERROR();

    if (query.next())
    {
        bool conversionResult = false;
        resourceIndexInNote = query.record().value(0).toInt(&conversionResult);
        if (!conversionResult) {
            SET_INT_CONVERSION_ERROR();
            return false;
        }
    }
    else
    {
        resourceIndexInNote = 0;
    }

    resource.setIndexInNote(resourceIndexInNote);

    QString resourceLocalUid = resource.localUid();

    QString column, uid;
    bool shouldCheckResourceExistence = true;

    bool resourceHasGuid = resource.hasGuid();
    if (resourceHasGuid)
    {
        column = QStringLiteral("resourceGuid");
        uid = resource.guid();

        if (resourceLocalUid.isEmpty())
        {
            error.clear();
            bool res = getResourceLocalUidForGuid(uid, resourceLocalUid, error);
            if (res || !resourceLocalUid.isEmpty()) {
                errorDescription.base() = errorPrefix.base();
                errorDescription.additionalBases().append(QT_TRANSLATE_NOOP("", "resource already exists"));
                errorDescription.details() = column;
                errorDescription.details() += QStringLiteral(" = ");
                errorDescription.details() += uid;
                QNWARNING(errorDescription);
                return false;
            }

            resourceLocalUid = UidGenerator::Generate();
            resource.setLocalUid(resourceLocalUid);
            shouldCheckResourceExistence = false;
        }
    }
    else
    {
        column = QStringLiteral("resourceLocalUid");
        uid = resource.localUid();
    }

    if (shouldCheckResourceExistence && rowExists(QStringLiteral("Resources"), column, uid)) {
        errorDescription.base() = errorPrefix.base();
        errorDescription.additionalBases().append(QT_TRANSLATE_NOOP("", "resource already exists"));
        errorDescription.details() = column;
        errorDescription.details() += QStringLiteral(" = ");
        errorDescription.details() += uid;
        QNWARNING(errorDescription);
        return false;
    }

    error.clear();
    res = insertOrReplaceResource(resource, error);
    if (!res) {
        errorDescription.base() = errorPrefix.base();
        errorDescription.additionalBases().append(error.base());
        errorDescription.additionalBases().append(error.additionalBases());
        errorDescription.details() = error.details();
        QNWARNING(errorDescription);
        return false;
    }

    return true;
}

bool LocalStorageManagerPrivate::updateEnResource(Resource & resource, ErrorString & errorDescription)
{
    ErrorString errorPrefix(QT_TRANSLATE_NOOP("", "Can't update resource in the local storage database"));

    ErrorString error;
    if (!resource.checkParameters(error)) {
        errorDescription.base() = errorPrefix.base();
        errorDescription.additionalBases().append(error.base());
        errorDescription.additionalBases().append(error.additionalBases());
        errorDescription.details() = error.details();
        QNWARNING(QStringLiteral("Found invalid resource: ") << resource);
        return false;
    }

    if (!resource.hasNoteGuid() && !resource.hasNoteLocalUid()) {
        errorDescription.base() = errorPrefix.base();
        errorDescription.additionalBases().append(QT_TRANSLATE_NOOP("", "both resource's note local uid and note guid are empty"));
        QNWARNING(errorDescription << QStringLiteral(", resource: ") << resource);
        return false;
    }

    // Updating the resource is essentially the update of note, so need to check if we're allowed to do that
    QString notebookLocalUid;
    Note dummyNote;
    if (resource.hasNoteGuid()) {
        dummyNote.setGuid(resource.noteGuid());
    }
    else {
        dummyNote.setLocalUid(resource.noteLocalUid());
    }

    error.clear();
    bool res = getNotebookLocalUidFromNote(dummyNote, notebookLocalUid, error);
    if (Q_UNLIKELY(!res)) {
        errorDescription.base() = errorPrefix.base();
        errorDescription.additionalBases().append(error.base());
        errorDescription.additionalBases().append(error.additionalBases());
        errorDescription.details() = error.details();
        QNWARNING(errorDescription);
        return false;
    }

    error.clear();
    res = canUpdateNoteInNotebook(notebookLocalUid, error);
    if (!res) {
        errorDescription.base() = errorPrefix.base();
        errorDescription.additionalBases().append(error.base());
        errorDescription.additionalBases().append(error.additionalBases());
        errorDescription.details() = error.details();
        QNWARNING(errorDescription);
        return false;
    }

    // Now can continue with updating the resource
    error.clear();
    res = complementResourceNoteIds(resource, error);
    if (!res) {
        errorDescription.base() = errorPrefix.base();
        errorDescription.additionalBases().append(error.base());
        errorDescription.additionalBases().append(error.additionalBases());
        errorDescription.details() = error.details();
        QNWARNING(errorDescription);
        return false;
    }

    QString resourceLocalUid = resource.localUid();

    QString column, uid;
    bool shouldCheckResourceExistence = true;

    bool resourceHasGuid = resource.hasGuid();
    if (resourceHasGuid)
    {
        column = QStringLiteral("resourceGuid");
        uid = resource.guid();

        if (resourceLocalUid.isEmpty())
        {
            error.clear();
            bool res = getResourceLocalUidForGuid(uid, resourceLocalUid, error);
            if (!res || resourceLocalUid.isEmpty()) {
                errorDescription.base() = errorPrefix.base();
                errorDescription.additionalBases().append(QT_TRANSLATE_NOOP("", "resource to be updated was not found "
                                                                            "in the local storage database"));
                errorDescription.details() = column;
                errorDescription.details() += QStringLiteral(" = ");
                errorDescription.details() += uid;
                QNWARNING(errorDescription);
                return false;
            }

            resource.setLocalUid(resourceLocalUid);
            shouldCheckResourceExistence = false;
        }
    }
    else
    {
        column = QStringLiteral("resourceLocalUid");
        uid = resource.localUid();
    }

    if (shouldCheckResourceExistence && !rowExists(QStringLiteral("Resources"), column, uid))
    {
        errorDescription.base() = errorPrefix.base();
        errorDescription.additionalBases().append(QT_TRANSLATE_NOOP("", "resource to be updated was not found "
                                                                    "in the local storage database"));
        errorDescription.details() = column;
        errorDescription.details() += QStringLiteral(" = ");
        errorDescription.details() += uid;
        QNWARNING(errorDescription);
        return false;
    }

    error.clear();
    res = insertOrReplaceResource(resource, error);
    if (!res) {
        errorDescription.base() = errorPrefix.base();
        errorDescription.additionalBases().append(error.base());
        errorDescription.additionalBases().append(error.additionalBases());
        errorDescription.details() = error.details();
        QNWARNING(errorDescription);
        return false;
    }

    return true;
}

void LocalStorageManagerPrivate::unlockDatabaseFile()
{
    QNDEBUG(QStringLiteral("LocalStorageManagerPrivate::unlockDatabaseFile: ") << m_databaseFilePath);

    if (m_databaseFilePath.isEmpty()) {
        QNDEBUG(QStringLiteral("No database file, nothing to do"));
        return;
    }

    try {
        m_databaseFileLock.unlock();
    }
    catch(boost::interprocess::interprocess_exception & exc) {
        QNWARNING(QStringLiteral("Caught exception trying to unlock the database file: error code = ")
                  << exc.get_error_code() << QStringLiteral(", error message = ") << exc.what()
                  << QStringLiteral("; native error = ") << exc.get_native_error());
    }
}

QString LocalStorageManagerPrivate::sqlEscapeString(const QString & str) const
{
    QString res = str;
    res.replace('\'', QStringLiteral("\'\'"));
    return res;
}

QString LocalStorageManagerPrivate::lastExecutedQuery(const QSqlQuery & query) const
{
    QString str = query.lastQuery();
    QMap<QString,QVariant> boundValues = query.boundValues();

    for(auto it = boundValues.constBegin(), end = boundValues.constEnd(); it != end; ++it) {
        str.replace(it.key(), it.value().toString());
    }

    return str;
}

bool LocalStorageManagerPrivate::createTables(ErrorString & errorDescription)
{
    QSqlQuery query(m_sqlDatabase);
    bool res;

    res = query.exec(QStringLiteral("CREATE TABLE IF NOT EXISTS Auxiliary("
                                    "  lock                        CHAR(1) PRIMARY KEY     NOT NULL DEFAULT 'X'    CHECK (lock='X'), "
                                    "  version                     INTEGER                 NOT NULL DEFAULT 1"
                                    ")"));
    ErrorString errorPrefix(QT_TRANSLATE_NOOP("", "Can't create Auxiliary table"));
    DATABASE_CHECK_AND_SET_ERROR();

    res = query.exec(QStringLiteral("CREATE TABLE IF NOT EXISTS Users("
                                    "  id                              INTEGER PRIMARY KEY     NOT NULL UNIQUE, "
                                    "  username                        TEXT                    DEFAULT NULL, "
                                    "  email                           TEXT                    DEFAULT NULL, "
                                    "  name                            TEXT                    DEFAULT NULL, "
                                    "  timezone                        TEXT                    DEFAULT NULL, "
                                    "  privilege                       INTEGER                 DEFAULT NULL, "
                                    "  serviceLevel                    INTEGER                 DEFAULT NULL, "
                                    "  userCreationTimestamp           INTEGER                 DEFAULT NULL, "
                                    "  userModificationTimestamp       INTEGER                 DEFAULT NULL, "
                                    "  userIsDirty                     INTEGER                 NOT NULL, "
                                    "  userIsLocal                     INTEGER                 NOT NULL, "
                                    "  userDeletionTimestamp           INTEGER                 DEFAULT NULL, "
                                    "  userIsActive                    INTEGER                 DEFAULT NULL, "
                                    "  userShardId                     TEXT                    DEFAULT NULL, "
                                    "  userPhotoUrl                    TEXT                    DEFAULT NULL, "
                                    "  userPhotoLastUpdateTimestamp    INTEGER                 DEFAULT NULL"
                                    ")"));
    errorPrefix.base() = QT_TRANSLATE_NOOP("", "Can't create Users table");
    DATABASE_CHECK_AND_SET_ERROR();

    res = query.exec(QStringLiteral("CREATE TABLE IF NOT EXISTS UserAttributes("
                                    "  id REFERENCES Users(id) ON UPDATE CASCADE, "
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
                                    "  useEmailAutoFiling          INTEGER                 DEFAULT NULL, "
                                    "  reminderEmailConfig         INTEGER                 DEFAULT NULL, "
                                    "  emailAddressLastConfirmed   INTEGER                 DEFAULT NULL, "
                                    "  passwordUpdated             INTEGER                 DEFAULT NULL, "
                                    "  salesforcePushEnabled       INTEGER                 DEFAULT NULL, "
                                    "  shouldLogClientEvent        INTEGER                 DEFAULT NULL)"));
    errorPrefix.base() = QT_TRANSLATE_NOOP("", "Can't create UserAttributes table");
    DATABASE_CHECK_AND_SET_ERROR();

    res = query.exec(QStringLiteral("CREATE TABLE IF NOT EXISTS UserAttributesViewedPromotions("
                                    "  id REFERENCES Users(id) ON UPDATE CASCADE, "
                                    "  promotion               TEXT                    DEFAULT NULL)"));
    errorPrefix.base() = QT_TRANSLATE_NOOP("", "Can't create UserAttributesViewedPromotions table");
    DATABASE_CHECK_AND_SET_ERROR();

    res = query.exec(QStringLiteral("CREATE TABLE IF NOT EXISTS UserAttributesRecentMailedAddresses("
                                    "  id REFERENCES Users(id) ON UPDATE CASCADE, "
                                    "  address                 TEXT                    DEFAULT NULL)"));
    errorPrefix.base() = QT_TRANSLATE_NOOP("", "Can't create UserAttributesRecentMailedAddresses table");
    DATABASE_CHECK_AND_SET_ERROR();

    res = query.exec(QStringLiteral("CREATE TABLE IF NOT EXISTS Accounting("
                                    "  id REFERENCES Users(id) ON UPDATE CASCADE, "
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
                                    "  unitDiscount                INTEGER             DEFAULT NULL, "
                                    "  nextChargeDate              INTEGER             DEFAULT NULL, "
                                    "  availablePoints             INTEGER             DEFAULT NULL)"));
    errorPrefix.base() = QT_TRANSLATE_NOOP("", "Can't create Accounting table");
    DATABASE_CHECK_AND_SET_ERROR();

    res = query.exec(QStringLiteral("CREATE TABLE IF NOT EXISTS AccountLimits("
                                    "  id REFERENCES Users(id) ON UPDATE CASCADE, "
                                    "  userMailLimitDaily          INTEGER             DEFAULT NULL, "
                                    "  noteSizeMax                 INTEGER             DEFAULT NULL, "
                                    "  resourceSizeMax             INTEGER             DEFAULT NULL, "
                                    "  userLinkedNotebookMax       INTEGER             DEFAULT NULL, "
                                    "  uploadLimit                 INTEGER             DEFAULT NULL, "
                                    "  userNoteCountMax            INTEGER             DEFAULT NULL, "
                                    "  userNotebookCountMax        INTEGER             DEFAULT NULL, "
                                    "  userTagCountMax             INTEGER             DEFAULT NULL, "
                                    "  noteTagCountMax             INTEGER             DEFAULT NULL, "
                                    "  userSavedSearchesMax        INTEGER             DEFAULT NULL, "
                                    "  noteResourceCountMax        INTEGER             DEFAULT NULL)"));
    errorPrefix.base() = QT_TRANSLATE_NOOP("", "Can't create AccountLimits table");
    DATABASE_CHECK_AND_SET_ERROR();

    res = query.exec(QStringLiteral("CREATE TABLE IF NOT EXISTS BusinessUserInfo("
                                    "  id REFERENCES Users(id) ON UPDATE CASCADE, "
                                    "  businessId              INTEGER                 DEFAULT NULL, "
                                    "  businessName            TEXT                    DEFAULT NULL, "
                                    "  role                    INTEGER                 DEFAULT NULL, "
                                    "  businessInfoEmail       TEXT                    DEFAULT NULL)"));
    errorPrefix.base() = QT_TRANSLATE_NOOP("", "Can't create BusinessUserInfo table");
    DATABASE_CHECK_AND_SET_ERROR();

    res = query.exec(QStringLiteral("CREATE TRIGGER IF NOT EXISTS on_user_delete_trigger "
                                    "BEFORE DELETE ON Users "
                                    "BEGIN "
                                    "DELETE FROM UserAttributes WHERE id=OLD.id; "
                                    "DELETE FROM UserAttributesViewedPromotions WHERE id=OLD.id; "
                                    "DELETE FROM UserAttributesRecentMailedAddresses WHERE id=OLD.id; "
                                    "DELETE FROM Accounting WHERE id=OLD.id; "
                                    "DELETE FROM AccountLimits WHERE id=OLD.id; "
                                    "DELETE FROM BusinessUserInfo WHERE id=OLD.id; "
                                    "END"));
    errorPrefix.base() = QT_TRANSLATE_NOOP("", "Can't create trigger to fire on deletion from users table");
    DATABASE_CHECK_AND_SET_ERROR();

    res = query.exec(QStringLiteral("CREATE TABLE IF NOT EXISTS LinkedNotebooks("
                                    "  guid                            TEXT PRIMARY KEY  NOT NULL UNIQUE, "
                                    "  updateSequenceNumber            INTEGER           DEFAULT NULL, "
                                    "  isDirty                         INTEGER           DEFAULT NULL, "
                                    "  shareName                       TEXT              DEFAULT NULL, "
                                    "  username                        TEXT              DEFAULT NULL, "
                                    "  shardId                         TEXT              DEFAULT NULL, "
                                    "  sharedNotebookGlobalId          TEXT              DEFAULT NULL, "
                                    "  uri                             TEXT              DEFAULT NULL, "
                                    "  noteStoreUrl                    TEXT              DEFAULT NULL, "
                                    "  webApiUrlPrefix                 TEXT              DEFAULT NULL, "
                                    "  stack                           TEXT              DEFAULT NULL, "
                                    "  businessId                      INTEGER           DEFAULT NULL"
                                    ")"));
    errorPrefix.base() = QT_TRANSLATE_NOOP("", "Can't create LinkedNotebooks table");
    DATABASE_CHECK_AND_SET_ERROR();

    res = query.exec(QStringLiteral("CREATE TABLE IF NOT EXISTS Notebooks("
                                    "  localUid                        TEXT PRIMARY KEY  NOT NULL UNIQUE, "
                                    "  guid                            TEXT              DEFAULT NULL UNIQUE, "
                                    "  linkedNotebookGuid REFERENCES LinkedNotebooks(guid) ON UPDATE CASCADE, "
                                    "  updateSequenceNumber            INTEGER           DEFAULT NULL, "
                                    "  notebookName                    TEXT              DEFAULT NULL, "
                                    "  notebookNameUpper               TEXT              DEFAULT NULL, "
                                    "  creationTimestamp               INTEGER           DEFAULT NULL, "
                                    "  modificationTimestamp           INTEGER           DEFAULT NULL, "
                                    "  isDirty                         INTEGER           NOT NULL, "
                                    "  isLocal                         INTEGER           NOT NULL, "
                                    "  isDefault                       INTEGER           DEFAULT NULL UNIQUE, "
                                    "  isLastUsed                      INTEGER           DEFAULT NULL UNIQUE, "
                                    "  isFavorited                     INTEGER           DEFAULT NULL, "
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
                                    "  recipientReminderNotifyEmail    INTEGER           DEFAULT NULL, "
                                    "  recipientReminderNotifyInApp    INTEGER           DEFAULT NULL, "
                                    "  recipientInMyList               INTEGER           DEFAULT NULL, "
                                    "  recipientStack                  TEXT              DEFAULT NULL, "
                                    "  UNIQUE(localUid, guid), "
                                    "  UNIQUE(linkedNotebookGuid), "
                                    "  UNIQUE(notebookNameUpper) "
                                    ")"));
    errorPrefix.base() = QT_TRANSLATE_NOOP("", "Can't create Notebooks table");
    DATABASE_CHECK_AND_SET_ERROR();

    res = query.exec(QStringLiteral("CREATE VIRTUAL TABLE IF NOT EXISTS NotebookFTS USING FTS4(content=\"Notebooks\", "
                                    "localUid, guid, notebookName)"));
    errorPrefix.base() = QT_TRANSLATE_NOOP("", "Can't create virtual FTS4 NotebookFTS table");
    DATABASE_CHECK_AND_SET_ERROR();

    res = query.exec(QStringLiteral("CREATE TRIGGER IF NOT EXISTS NotebookFTS_BeforeDeleteTrigger BEFORE DELETE ON Notebooks "
                                    "BEGIN "
                                    "DELETE FROM NotebookFTS WHERE localUid=old.localUid; "
                                    "END"));
    errorPrefix.base() = QT_TRANSLATE_NOOP("", "Can't create NotebookFTS_BeforeDeleteTrigger");
    DATABASE_CHECK_AND_SET_ERROR();

    res = query.exec(QStringLiteral("CREATE TRIGGER IF NOT EXISTS NotebookFTS_AfterInsertTrigger AFTER INSERT ON Notebooks "
                                    "BEGIN "
                                    "INSERT INTO NotebookFTS(NotebookFTS) VALUES('rebuild'); "
                                    "END"));
    errorPrefix.base() = QT_TRANSLATE_NOOP("", "Can't create NotebookFTS_AfterInsertTrigger");
    DATABASE_CHECK_AND_SET_ERROR();

    res = query.exec(QStringLiteral("CREATE TABLE IF NOT EXISTS NotebookRestrictions("
                                    "  localUid REFERENCES Notebooks(localUid) ON UPDATE CASCADE, "
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
                                    "  noShareNotesWithBusiness    INTEGER      DEFAULT NULL, "
                                    "  noRenameNotebook            INTEGER      DEFAULT NULL, "
                                    "  updateWhichSharedNotebookRestrictions    INTEGER     DEFAULT NULL, "
                                    "  expungeWhichSharedNotebookRestrictions   INTEGER     DEFAULT NULL "
                                    ")"));
    errorPrefix.base() = QT_TRANSLATE_NOOP("", "Can't create NotebookRestrictions table");
    DATABASE_CHECK_AND_SET_ERROR();

    res = query.exec(QStringLiteral("CREATE TABLE IF NOT EXISTS SharedNotebooks("
                                    "  sharedNotebookShareId                             INTEGER PRIMARY KEY   NOT NULL UNIQUE, "
                                    "  sharedNotebookUserId                              INTEGER    DEFAULT NULL, "
                                    "  sharedNotebookNotebookGuid REFERENCES Notebooks(guid) ON UPDATE CASCADE, "
                                    "  sharedNotebookEmail                               TEXT       DEFAULT NULL, "
                                    "  sharedNotebookIdentityId                          INTEGER    DEFAULT NULL, "
                                    "  sharedNotebookCreationTimestamp                   INTEGER    DEFAULT NULL, "
                                    "  sharedNotebookModificationTimestamp               INTEGER    DEFAULT NULL, "
                                    "  sharedNotebookGlobalId                            TEXT       DEFAULT NULL, "
                                    "  sharedNotebookUsername                            TEXT       DEFAULT NULL, "
                                    "  sharedNotebookPrivilegeLevel                      INTEGER    DEFAULT NULL, "
                                    "  sharedNotebookRecipientReminderNotifyEmail        INTEGER    DEFAULT NULL, "
                                    "  sharedNotebookRecipientReminderNotifyInApp        INTEGER    DEFAULT NULL, "
                                    "  sharedNotebookSharerUserId                        INTEGER    DEFAULT NULL, "
                                    "  sharedNotebookRecipientUsername                   TEXT       DEFAULT NULL, "
                                    "  sharedNotebookRecipientUserId                     INTEGER    DEFAULT NULL, "
                                    "  sharedNotebookRecipientIdentityId                 INTEGER    DEFAULT NULL, "
                                    "  sharedNotebookAssignmentTimestamp                 INTEGER    DEFAULT NULL, "
                                    "  indexInNotebook                                   INTEGER    DEFAULT NULL, "
                                    "  UNIQUE(sharedNotebookShareId, sharedNotebookNotebookGuid) ON CONFLICT REPLACE"
                                    ")"));
    errorPrefix.base() = QT_TRANSLATE_NOOP("", "Can't create SharedNotebooks table");
    DATABASE_CHECK_AND_SET_ERROR();

    res = query.exec(QStringLiteral("CREATE TABLE IF NOT EXISTS Notes("
                                    "  localUid                        TEXT PRIMARY KEY     NOT NULL UNIQUE, "
                                    "  guid                            TEXT                 DEFAULT NULL UNIQUE, "
                                    "  updateSequenceNumber            INTEGER              DEFAULT NULL, "
                                    "  isDirty                         INTEGER              NOT NULL, "
                                    "  isLocal                         INTEGER              NOT NULL, "
                                    "  isFavorited                     INTEGER              NOT NULL, "
                                    "  title                           TEXT                 DEFAULT NULL, "
                                    "  titleNormalized                 TEXT                 DEFAULT NULL, "
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
                                    "  notebookLocalUid REFERENCES Notebooks(localUid) ON UPDATE CASCADE, "
                                    "  notebookGuid REFERENCES Notebooks(guid) ON UPDATE CASCADE, "
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
                                    "  sharedWithBusiness              INTEGER              DEFAULT NULL, "
                                    "  conflictSourceNoteGuid          TEXT                 DEFAULT NULL, "
                                    "  noteTitleQuality                INTEGER              DEFAULT NULL, "
                                    "  applicationDataKeysOnly         TEXT                 DEFAULT NULL, "
                                    "  applicationDataKeysMap          TEXT                 DEFAULT NULL, "
                                    "  applicationDataValues           TEXT                 DEFAULT NULL, "
                                    "  classificationKeys              TEXT                 DEFAULT NULL, "
                                    "  classificationValues            TEXT                 DEFAULT NULL, "
                                    "  UNIQUE(localUid, guid)"
                                    ")"));
    errorPrefix.base() = QT_TRANSLATE_NOOP("", "Can't create Notes table");
    DATABASE_CHECK_AND_SET_ERROR();

    res = query.exec(QStringLiteral("CREATE TABLE IF NOT EXISTS SharedNotes("
                                    "  sharedNoteNoteGuid REFERENCES Notes(guid) ON UPDATE CASCADE, "
                                    "  sharedNoteSharerUserId                               INTEGER     DEFAULT NULL, "
                                    "  sharedNoteRecipientIdentityId                        INTEGER     DEFAULT NULL UNIQUE, "
                                    "  sharedNoteRecipientContactName                       TEXT        DEFAULT NULL, "
                                    "  sharedNoteRecipientContactId                         TEXT        DEFAULT NULL, "
                                    "  sharedNoteRecipientContactType                       INTEGER     DEFAULT NULL, "
                                    "  sharedNoteRecipientContactPhotoUrl                   TEXT        DEFAULT NULL, "
                                    "  sharedNoteRecipientContactPhotoLastUpdated           INTEGER     DEFAULT NULL, "
                                    "  sharedNoteRecipientContactMessagingPermit            BLOB        DEFAULT NULL, "
                                    "  sharedNoteRecipientContactMessagingPermitExpires     INTEGER     DEFAULT NULL, "
                                    "  sharedNoteRecipientUserId                            INTEGER     DEFAULT NULL, "
                                    "  sharedNoteRecipientDeactivated                       INTEGER     DEFAULT NULL, "
                                    "  sharedNoteRecipientSameBusiness                      INTEGER     DEFAULT NULL, "
                                    "  sharedNoteRecipientBlocked                           INTEGER     DEFAULT NULL, "
                                    "  sharedNoteRecipientUserConnected                     INTEGER     DEFAULT NULL, "
                                    "  sharedNoteRecipientEventId                           INTEGER     DEFAULT NULL, "
                                    "  sharedNotePrivilegeLevel                             INTEGER     DEFAULT NULL, "
                                    "  sharedNoteCreationTimestamp                          INTEGER     DEFAULT NULL, "
                                    "  sharedNoteModificationTimestamp                      INTEGER     DEFAULT NULL, "
                                    "  sharedNoteAssignmentTimestamp                        INTEGER     DEFAULT NULL, "
                                    "  indexInNote                                          INTEGER     DEFAULT NULL, "
                                    "  UNIQUE(sharedNoteNoteGuid, sharedNoteRecipientIdentityId) ON CONFLICT REPLACE)"));
    errorPrefix.base() = QT_TRANSLATE_NOOP("", "Can't create SharedNotes table");
    DATABASE_CHECK_AND_SET_ERROR();

    res = query.exec(QStringLiteral("CREATE TABLE IF NOT EXISTS NoteRestrictions("
                                    "  noteLocalUid REFERENCES Notes(localUid) ON UPDATE CASCADE, "
                                    "  noUpdateNoteTitle                INTEGER             DEFAULT NULL, "
                                    "  noUpdateNoteContent              INTEGER             DEFAULT NULL, "
                                    "  noEmailNote                      INTEGER             DEFAULT NULL, "
                                    "  noShareNote                      INTEGER             DEFAULT NULL, "
                                    "  noShareNotePublicly              INTEGER             DEFAULT NULL)"));
    errorPrefix.base() = QT_TRANSLATE_NOOP("", "Can't create NoteRestrictions table");
    DATABASE_CHECK_AND_SET_ERROR();

    res = query.exec(QStringLiteral("CREATE TABLE IF NOT EXISTS NoteLimits("
                                    "  noteLocalUid REFERENCES Notes(localUid) ON UPDATE CASCADE, "
                                    "  noteResourceCountMax             INTEGER             DEFAULT NULL, "
                                    "  uploadLimit                      INTEGER             DEFAULT NULL, "
                                    "  resourceSizeMax                  INTEGER             DEFAULT NULL, "
                                    "  noteSizeMax                      INTEGER             DEFAULT NULL, "
                                    "  uploaded                         INTEGER             DEFAULT NULL)"));
    errorPrefix.base() = QT_TRANSLATE_NOOP("", "Can't create NoteLimits table");
    DATABASE_CHECK_AND_SET_ERROR();

    res = query.exec(QStringLiteral("CREATE INDEX IF NOT EXISTS NotesNotebooks ON Notes(notebookLocalUid)"));
    errorPrefix.base() = QT_TRANSLATE_NOOP("", "Can't create index NotesNotebooks");
    DATABASE_CHECK_AND_SET_ERROR();

    res = query.exec(QStringLiteral("CREATE VIRTUAL TABLE IF NOT EXISTS NoteFTS USING FTS4(content=\"Notes\", localUid, titleNormalized, "
                                    "contentListOfWords, contentContainsFinishedToDo, contentContainsUnfinishedToDo, "
                                    "contentContainsEncryption, creationTimestamp, modificationTimestamp, "
                                    "isActive, notebookLocalUid, notebookGuid, subjectDate, latitude, longitude, "
                                    "altitude, author, source, sourceApplication, reminderOrder, reminderDoneTime, "
                                    "reminderTime, placeName, contentClass, applicationDataKeysOnly, "
                                    "applicationDataKeysMap, applicationDataValues)"));
    errorPrefix.base() = QT_TRANSLATE_NOOP("", "Can't create virtual FTS4 table NoteFTS");
    DATABASE_CHECK_AND_SET_ERROR();

    res = query.exec(QStringLiteral("CREATE TRIGGER IF NOT EXISTS NoteFTS_BeforeDeleteTrigger BEFORE DELETE ON Notes "
                                    "BEGIN "
                                    "DELETE FROM NoteFTS WHERE localUid=old.localUid; "
                                    "END"));
    errorPrefix.base() = QT_TRANSLATE_NOOP("", "Can't create trigger NoteFTS_BeforeDeleteTrigger");
    DATABASE_CHECK_AND_SET_ERROR();

    res = query.exec(QStringLiteral("CREATE TRIGGER IF NOT EXISTS NoteFTS_AfterInsertTrigger AFTER INSERT ON Notes "
                                    "BEGIN "
                                    "INSERT INTO NoteFTS(NoteFTS) VALUES('rebuild'); "
                                    "END"));
    errorPrefix.base() = QT_TRANSLATE_NOOP("", "Can't create trigger NoteFTS_AfterInsertTrigger");
    DATABASE_CHECK_AND_SET_ERROR();

    res = query.exec(QStringLiteral("CREATE TRIGGER IF NOT EXISTS on_notebook_delete_trigger BEFORE DELETE ON Notebooks "
                                    "BEGIN "
                                    "DELETE FROM NotebookRestrictions WHERE NotebookRestrictions.localUid=OLD.localUid; "
                                    "DELETE FROM SharedNotebooks WHERE SharedNotebooks.sharedNotebookNotebookGuid=OLD.guid; "
                                    "DELETE FROM Notes WHERE Notes.notebookLocalUid=OLD.localUid; "
                                    "END"));
    errorPrefix.base() = QT_TRANSLATE_NOOP("", "Can't create trigger to fire on notebook deletion");
    DATABASE_CHECK_AND_SET_ERROR();

    res = query.exec(QStringLiteral("CREATE TABLE IF NOT EXISTS Resources("
                                    "  resourceLocalUid                TEXT PRIMARY KEY     NOT NULL UNIQUE, "
                                    "  resourceGuid                    TEXT                 DEFAULT NULL UNIQUE, "
                                    "  noteLocalUid REFERENCES Notes(localUid) ON UPDATE CASCADE, "
                                    "  noteGuid REFERENCES Notes(guid) ON UPDATE CASCADE, "
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
                                    "  UNIQUE(resourceLocalUid, resourceGuid)"
                                    ")"));
    errorPrefix.base() = QT_TRANSLATE_NOOP("", "Can't create Resources table");
    DATABASE_CHECK_AND_SET_ERROR();

    res = query.exec(QStringLiteral("CREATE INDEX IF NOT EXISTS ResourceMimeIndex ON Resources(mime)"));
    errorPrefix.base() = QT_TRANSLATE_NOOP("", "Can't create ResourceMimeIndex index");
    DATABASE_CHECK_AND_SET_ERROR();

    res = query.exec(QStringLiteral("CREATE TABLE IF NOT EXISTS ResourceRecognitionData("
                                    "  resourceLocalUid REFERENCES Resources(resourceLocalUid)     ON UPDATE CASCADE, "
                                    "  noteLocalUid REFERENCES Notes(localUid)                     ON UPDATE CASCADE, "
                                    "  recognitionData                 TEXT                        DEFAULT NULL)"));
    errorPrefix.base() = QT_TRANSLATE_NOOP("", "Can't create ResourceRecognitionData table");
    DATABASE_CHECK_AND_SET_ERROR();

    res = query.exec(QStringLiteral("CREATE INDEX IF NOT EXISTS ResourceRecognitionDataIndex "
                                    "ON ResourceRecognitionData(recognitionData)"));
    errorPrefix.base() = QT_TRANSLATE_NOOP("", "Can't create ResourceRecognitionDataIndex index");
    DATABASE_CHECK_AND_SET_ERROR();

    res = query.exec(QStringLiteral("CREATE VIRTUAL TABLE IF NOT EXISTS ResourceRecognitionDataFTS USING FTS4"
                                    "(content=\"ResourceRecognitionData\", resourceLocalUid, noteLocalUid, recognitionData)"));
    errorPrefix.base() = QT_TRANSLATE_NOOP("", "Can't create virtual FTS4 ResourceRecognitionDataFTS table");
    DATABASE_CHECK_AND_SET_ERROR();

    res = query.exec(QStringLiteral("CREATE TRIGGER IF NOT EXISTS ResourceRecognitionDataFTS_BeforeDeleteTrigger BEFORE DELETE "
                                    "ON ResourceRecognitionData "
                                    "BEGIN "
                                    "DELETE FROM ResourceRecognitionDataFTS WHERE recognitionData=old.recognitionData; "
                                    "END"));
    errorPrefix.base() = QT_TRANSLATE_NOOP("", "Can't create trigger ResourceRecognitionDataFTS_BeforeDeleteTrigger");
    DATABASE_CHECK_AND_SET_ERROR();

    res = query.exec(QStringLiteral("CREATE TRIGGER IF NOT EXISTS ResourceRecognitionDataFTS_AfterInsertTrigger AFTER INSERT "
                                    "ON ResourceRecognitionData "
                                    "BEGIN "
                                    "INSERT INTO ResourceRecognitionDataFTS(ResourceRecognitionDataFTS) VALUES('rebuild'); "
                                    "END"));
    errorPrefix.base() = QT_TRANSLATE_NOOP("", "Can't create trigger ResourceRecognitionDataFTS_AfterInsertTrigger");
    DATABASE_CHECK_AND_SET_ERROR();

    res = query.exec(QStringLiteral("CREATE VIRTUAL TABLE IF NOT EXISTS ResourceMimeFTS USING FTS4(content=\"Resources\", resourceLocalUid, mime)"));
    errorPrefix.base() = QT_TRANSLATE_NOOP("", "Can't create virtual FTS4 ResourceMimeFTS table");
    DATABASE_CHECK_AND_SET_ERROR();

    res = query.exec(QStringLiteral("CREATE TRIGGER IF NOT EXISTS ResourceMimeFTS_BeforeDeleteTrigger BEFORE DELETE ON Resources "
                                    "BEGIN "
                                    "DELETE FROM ResourceMimeFTS WHERE mime=old.mime; "
                                    "END"));
    errorPrefix.base() = QT_TRANSLATE_NOOP("", "Can't create trigger ResourceMimeFTS_BeforeDeleteTrigger");
    DATABASE_CHECK_AND_SET_ERROR();

    res = query.exec(QStringLiteral("CREATE TRIGGER IF NOT EXISTS ResourceMimeFTS_AfterInsertTrigger AFTER INSERT ON Resources "
                                    "BEGIN "
                                    "INSERT INTO ResourceMimeFTS(ResourceMimeFTS) VALUES('rebuild'); "
                                    "END"));
    errorPrefix.base() = QT_TRANSLATE_NOOP("", "Can't create trigger ResourceMimeFTS_AfterInsertTrigger");
    DATABASE_CHECK_AND_SET_ERROR();

    res = query.exec(QStringLiteral("CREATE VIEW IF NOT EXISTS ResourcesWithoutBinaryData "
                                    "AS SELECT resourceLocalUid, resourceGuid, noteLocalUid, noteGuid, "
                                    "resourceUpdateSequenceNumber, resourceIsDirty, dataSize, dataHash, "
                                    "mime, width, height, recognitionDataSize, recognitionDataHash, "
                                    "alternateDataSize, alternateDataHash, resourceIndexInNote FROM Resources"));
    errorPrefix.base() = QT_TRANSLATE_NOOP("", "Can't create ResourcesWithoutBinaryData view");
    DATABASE_CHECK_AND_SET_ERROR();

    res = query.exec(QStringLiteral("CREATE INDEX IF NOT EXISTS ResourceNote ON Resources(noteLocalUid)"));
    errorPrefix.base() = QT_TRANSLATE_NOOP("", "Can't create ResourceNote index");
    DATABASE_CHECK_AND_SET_ERROR();

    res = query.exec(QStringLiteral("CREATE TABLE IF NOT EXISTS ResourceAttributes("
                                    "  resourceLocalUid REFERENCES Resources(resourceLocalUid) ON UPDATE CASCADE, "
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
                                    "  UNIQUE(resourceLocalUid) "
                                    ")"));
    errorPrefix.base() = QT_TRANSLATE_NOOP("", "Can't create ResourceAttributes table");
    DATABASE_CHECK_AND_SET_ERROR();

    res = query.exec(QStringLiteral("CREATE TABLE IF NOT EXISTS ResourceAttributesApplicationDataKeysOnly("
                                    "  resourceLocalUid REFERENCES Resources(resourceLocalUid) ON UPDATE CASCADE, "
                                    "  resourceKey             TEXT                DEFAULT NULL, "
                                    "  UNIQUE(resourceLocalUid, resourceKey)"
                                    ")"));
    errorPrefix.base() = QT_TRANSLATE_NOOP("", "Can't create ResourceAttributesApplicationDataKeysOnly table");
    DATABASE_CHECK_AND_SET_ERROR();

    res = query.exec(QStringLiteral("CREATE TABLE IF NOT EXISTS ResourceAttributesApplicationDataFullMap("
                                    "  resourceLocalUid REFERENCES Resources(resourceLocalUid) ON UPDATE CASCADE, "
                                    "  resourceMapKey          TEXT                DEFAULT NULL, "
                                    "  resourceValue           TEXT                DEFAULT NULL, "
                                    "  UNIQUE(resourceLocalUid, resourceMapKey) ON CONFLICT REPLACE"
                                    ")"));
    errorPrefix.base() = QT_TRANSLATE_NOOP("", "Can't create ResourceAttributesApplicationDataFullMap table");
    DATABASE_CHECK_AND_SET_ERROR();

    res = query.exec(QStringLiteral("CREATE TABLE IF NOT EXISTS Tags("
                                    "  localUid              TEXT PRIMARY KEY     NOT NULL UNIQUE, "
                                    "  guid                  TEXT                 DEFAULT NULL UNIQUE, "
                                    "  linkedNotebookGuid REFERENCES LinkedNotebooks(guid) ON UPDATE CASCADE, "
                                    "  updateSequenceNumber  INTEGER              DEFAULT NULL, "
                                    "  name                  TEXT                 DEFAULT NULL, "
                                    "  nameLower             TEXT                 DEFAULT NULL, "
                                    "  parentGuid REFERENCES Tags(guid)           ON UPDATE CASCADE DEFAULT NULL, "
                                    "  parentLocalUid REFERENCES Tags(localUid)   ON UPDATE CASCADE DEFAULT NULL, "
                                    "  isDirty               INTEGER              NOT NULL, "
                                    "  isLocal               INTEGER              NOT NULL, "
                                    "  isFavorited           INTEGER              NOT NULL, "
                                    "  UNIQUE(linkedNotebookGuid, nameLower) "
                                    ")"));
    errorPrefix.base() = QT_TRANSLATE_NOOP("", "Can't create Tags table");
    DATABASE_CHECK_AND_SET_ERROR();

    res = query.exec(QStringLiteral("CREATE INDEX IF NOT EXISTS TagNameUpperIndex ON Tags(nameLower)"));
    errorPrefix.base() = QT_TRANSLATE_NOOP("", "Can't create TagNameUpperIndex index");
    DATABASE_CHECK_AND_SET_ERROR();

    res = query.exec(QStringLiteral("CREATE VIRTUAL TABLE IF NOT EXISTS TagFTS USING FTS4(content=\"Tags\", localUid, guid, nameLower)"));
    errorPrefix.base() = QT_TRANSLATE_NOOP("", "Can't create virtual FTS4 table TagFTS");
    DATABASE_CHECK_AND_SET_ERROR();

    res = query.exec(QStringLiteral("CREATE TRIGGER IF NOT EXISTS TagFTS_BeforeDeleteTrigger BEFORE DELETE ON Tags "
                                    "BEGIN "
                                    "DELETE FROM TagFTS WHERE localUid=old.localUid; "
                                    "END"));
    errorPrefix.base() = QT_TRANSLATE_NOOP("", "Can't create trigger TagFTS_BeforeDeleteTrigger");
    DATABASE_CHECK_AND_SET_ERROR();

    res = query.exec(QStringLiteral("CREATE TRIGGER IF NOT EXISTS TagFTS_AfterInsertTrigger AFTER INSERT ON Tags "
                                    "BEGIN "
                                    "INSERT INTO TagFTS(TagFTS) VALUES('rebuild'); "
                                    "END"));
    errorPrefix.base() = QT_TRANSLATE_NOOP("", "Can't create trigger TagFTS_AfterInsertTrigger");
    DATABASE_CHECK_AND_SET_ERROR();

    res = query.exec(QStringLiteral("CREATE INDEX IF NOT EXISTS TagsSearchName ON Tags(nameLower)"));
    errorPrefix.base() = QT_TRANSLATE_NOOP("", "Can't create TagsSearchName index");
    DATABASE_CHECK_AND_SET_ERROR();

    res = query.exec(QStringLiteral("CREATE TABLE IF NOT EXISTS NoteTags("
                                    "  localNote REFERENCES Notes(localUid) ON UPDATE CASCADE, "
                                    "  note REFERENCES Notes(guid)          ON UPDATE CASCADE, "
                                    "  localTag REFERENCES Tags(localUid)   ON UPDATE CASCADE, "
                                    "  tag  REFERENCES Tags(guid)           ON UPDATE CASCADE, "
                                    "  tagIndexInNote        INTEGER        DEFAULT NULL, "
                                    "  UNIQUE(localNote, localTag) ON CONFLICT REPLACE"
                                    ")"));
    errorPrefix.base() = QT_TRANSLATE_NOOP("", "Can't create NoteTags table");
    DATABASE_CHECK_AND_SET_ERROR();

    res = query.exec(QStringLiteral("CREATE INDEX IF NOT EXISTS NoteTagsNote ON NoteTags(localNote)"));
    errorPrefix.base() = QT_TRANSLATE_NOOP("", "Can't create NoteTagsNote index");
    DATABASE_CHECK_AND_SET_ERROR();

    res = query.exec(QStringLiteral("CREATE TABLE IF NOT EXISTS NoteResources("
                                    "  localNote     REFERENCES Notes(localUid)             ON UPDATE CASCADE, "
                                    "  note          REFERENCES Notes(guid)                 ON UPDATE CASCADE, "
                                    "  localResource REFERENCES Resources(resourceLocalUid) ON UPDATE CASCADE, "
                                    "  resource      REFERENCES Resources(resourceGuid)     ON UPDATE CASCADE, "
                                    "  UNIQUE(localNote, localResource) ON CONFLICT REPLACE)"));
    errorPrefix.base() = QT_TRANSLATE_NOOP("", "Can't create NoteResources table");
    DATABASE_CHECK_AND_SET_ERROR();

    res = query.exec(QStringLiteral("CREATE INDEX IF NOT EXISTS NoteResourcesNote ON NoteResources(localNote)"));
    errorPrefix.base() = QT_TRANSLATE_NOOP("", "Can't create NoteResourcesNote index");
    DATABASE_CHECK_AND_SET_ERROR();

    // NOTE: reasoning for existence and unique constraint for nameLower, citing Evernote API reference:
    // "The account may only contain one search with a given name (case-insensitive compare)"

    res = query.exec(QStringLiteral("CREATE TRIGGER IF NOT EXISTS on_linked_notebook_delete_trigger "
                                    "BEFORE DELETE ON LinkedNotebooks "
                                    "BEGIN "
                                    "DELETE FROM Notebooks WHERE Notebooks.linkedNotebookGuid=OLD.guid; "
                                    "DELETE FROM Tags WHERE Tags.linkedNotebookGuid=OLD.guid; "
                                    "END"));
    errorPrefix.base() = QT_TRANSLATE_NOOP("", "Can't create trigger to fire on linked notebook deletion");
    DATABASE_CHECK_AND_SET_ERROR();

    res = query.exec(QStringLiteral("CREATE TRIGGER IF NOT EXISTS on_note_delete_trigger "
                                    "BEFORE DELETE ON Notes "
                                    "BEGIN "
                                    "DELETE FROM Resources WHERE Resources.noteLocalUid=OLD.localUid; "
                                    "DELETE FROM ResourceRecognitionData WHERE ResourceRecognitionData.noteLocalUid=OLD.localUid; "
                                    "DELETE FROM NoteTags WHERE NoteTags.localNote=OLD.localUid; "
                                    "DELETE FROM NoteResources WHERE NoteResources.localNote=OLD.localUid; "
                                    "DELETE FROM SharedNotes WHERE SharedNotes.sharedNoteNoteGuid=OLD.guid; "
                                    "DELETE FROM NoteRestrictions WHERE NoteRestrictions.noteLocalUid=OLD.localUid; "
                                    "DELETE FROM NoteLimits WHERE NoteLimits.noteLocalUid=OLD.localUid; "
                                    "END"));
    errorPrefix.base() = QT_TRANSLATE_NOOP("", "Can't create trigger to fire on note deletion");
    DATABASE_CHECK_AND_SET_ERROR();

    res = query.exec(QStringLiteral("CREATE TRIGGER IF NOT EXISTS on_resource_delete_trigger "
                                    "BEFORE DELETE ON Resources "
                                    "BEGIN "
                                    "DELETE FROM ResourceRecognitionData WHERE ResourceRecognitionData.resourceLocalUid=OLD.resourceLocalUid; "
                                    "DELETE FROM ResourceAttributes WHERE ResourceAttributes.resourceLocalUid=OLD.resourceLocalUid; "
                                    "DELETE FROM ResourceAttributesApplicationDataKeysOnly WHERE ResourceAttributesApplicationDataKeysOnly.resourceLocalUid=OLD.resourceLocalUid; "
                                    "DELETE FROM ResourceAttributesApplicationDataFullMap WHERE ResourceAttributesApplicationDataFullMap.resourceLocalUid=OLD.resourceLocalUid; "
                                    "DELETE FROM NoteResources WHERE NoteResources.localResource=OLD.resourceLocalUid; "
                                    "END"));
    errorPrefix.base() = QT_TRANSLATE_NOOP("", "Can't create trigger to fire on resource deletion");
    DATABASE_CHECK_AND_SET_ERROR();

    res = query.exec(QStringLiteral("CREATE TRIGGER IF NOT EXISTS on_tag_delete_trigger "
                                    "BEFORE DELETE ON Tags "
                                    "BEGIN "
                                    "DELETE FROM NoteTags WHERE NoteTags.localTag=OLD.localUid; "
                                    "END"));
    errorPrefix.base() = QT_TRANSLATE_NOOP("", "Can't create trigger to fire on tag deletion");
    DATABASE_CHECK_AND_SET_ERROR();

    res = query.exec(QStringLiteral("CREATE TABLE IF NOT EXISTS SavedSearches("
                                    "  localUid                        TEXT PRIMARY KEY    NOT NULL UNIQUE, "
                                    "  guid                            TEXT                DEFAULT NULL UNIQUE, "
                                    "  name                            TEXT                DEFAULT NULL, "
                                    "  nameLower                       TEXT                DEFAULT NULL UNIQUE, "
                                    "  query                           TEXT                DEFAULT NULL, "
                                    "  format                          INTEGER             DEFAULT NULL, "
                                    "  updateSequenceNumber            INTEGER             DEFAULT NULL, "
                                    "  isDirty                         INTEGER             NOT NULL, "
                                    "  isLocal                         INTEGER             NOT NULL, "
                                    "  includeAccount                  INTEGER             DEFAULT NULL, "
                                    "  includePersonalLinkedNotebooks  INTEGER             DEFAULT NULL, "
                                    "  includeBusinessLinkedNotebooks  INTEGER             DEFAULT NULL, "
                                    "  isFavorited                     INTEGER             NOT NULL, "
                                    "  UNIQUE(localUid, guid))"));
    errorPrefix.base() = QT_TRANSLATE_NOOP("", "Can't create SavedSearches table");
    DATABASE_CHECK_AND_SET_ERROR();

    return true;
}

bool LocalStorageManagerPrivate::insertOrReplaceNotebookRestrictions(const QString & localUid, const qevercloud::NotebookRestrictions & notebookRestrictions,
                                                                     ErrorString & errorDescription)
{
    ErrorString errorPrefix(QT_TRANSLATE_NOOP("", "can't insert or replace notebook restrictions"));

    bool res = checkAndPrepareInsertOrReplaceNotebookRestrictionsQuery();
    QSqlQuery & query = m_insertOrReplaceNotebookRestrictionsQuery;
    DATABASE_CHECK_AND_SET_ERROR();

    QVariant nullValue;

    query.bindValue(QStringLiteral(":localUid"), localUid);

#define BIND_RESTRICTION(name) \
    query.bindValue(QStringLiteral(":" #name), (notebookRestrictions.name.isSet() ? (notebookRestrictions.name.ref() ? 1 : 0) : nullValue))

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
    BIND_RESTRICTION(noShareNotesWithBusiness);
    BIND_RESTRICTION(noRenameNotebook);

#undef BIND_RESTRICTION

    query.bindValue(QStringLiteral(":updateWhichSharedNotebookRestrictions"),
                    notebookRestrictions.updateWhichSharedNotebookRestrictions.isSet()
                    ? notebookRestrictions.updateWhichSharedNotebookRestrictions.ref() : nullValue);
    query.bindValue(QStringLiteral(":expungeWhichSharedNotebookRestrictions"),
                    notebookRestrictions.expungeWhichSharedNotebookRestrictions.isSet()
                    ? notebookRestrictions.expungeWhichSharedNotebookRestrictions.ref() : nullValue);

    res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR();

    query.finish();
    return true;
}

bool LocalStorageManagerPrivate::insertOrReplaceSharedNotebook(const SharedNotebook & sharedNotebook,
                                                               ErrorString & errorDescription)
{
    // NOTE: this method is expected to be called after the sanity check of sharedNotebook object!

    ErrorString errorPrefix(QT_TRANSLATE_NOOP("", "can't insert or replace shared notebook"));

    bool res = checkAndPrepareInsertOrReplaceSharedNotebookQuery();
    QSqlQuery & query = m_insertOrReplaceSharedNotebookQuery;
    DATABASE_CHECK_AND_SET_ERROR();

    QVariant nullValue;

    query.bindValue(QStringLiteral(":sharedNotebookShareId"), sharedNotebook.id());
    query.bindValue(QStringLiteral(":sharedNotebookUserId"), (sharedNotebook.hasUserId() ? sharedNotebook.userId() : nullValue));
    query.bindValue(QStringLiteral(":sharedNotebookNotebookGuid"), (sharedNotebook.hasNotebookGuid() ? sharedNotebook.notebookGuid() : nullValue));
    query.bindValue(QStringLiteral(":sharedNotebookEmail"), (sharedNotebook.hasEmail() ? sharedNotebook.email() : nullValue));
    query.bindValue(QStringLiteral(":sharedNotebookCreationTimestamp"), (sharedNotebook.hasCreationTimestamp() ? sharedNotebook.creationTimestamp() : nullValue));
    query.bindValue(QStringLiteral(":sharedNotebookModificationTimestamp"), (sharedNotebook.hasModificationTimestamp() ? sharedNotebook.modificationTimestamp() : nullValue));
    query.bindValue(QStringLiteral(":sharedNotebookGlobalId"), (sharedNotebook.hasGlobalId() ? sharedNotebook.globalId() : nullValue));
    query.bindValue(QStringLiteral(":sharedNotebookUsername"), (sharedNotebook.hasUsername() ? sharedNotebook.username() : nullValue));
    query.bindValue(QStringLiteral(":sharedNotebookPrivilegeLevel"), (sharedNotebook.hasPrivilegeLevel() ? sharedNotebook.privilegeLevel() : nullValue));
    query.bindValue(QStringLiteral(":sharedNotebookRecipientReminderNotifyEmail"), (sharedNotebook.hasReminderNotifyEmail() ? (sharedNotebook.reminderNotifyEmail() ? 1 : 0) : nullValue));
    query.bindValue(QStringLiteral(":sharedNotebookRecipientReminderNotifyInApp"), (sharedNotebook.hasReminderNotifyApp() ? (sharedNotebook.reminderNotifyApp() ? 1 : 0) : nullValue));
    query.bindValue(QStringLiteral(":sharedNotebookSharerUserId"), (sharedNotebook.hasSharerUserId() ? sharedNotebook.sharerUserId() : nullValue));
    query.bindValue(QStringLiteral(":sharedNotebookRecipientUsername"), (sharedNotebook.hasRecipientUsername() ? sharedNotebook.recipientUsername() : nullValue));
    query.bindValue(QStringLiteral(":sharedNotebookRecipientUserId"), (sharedNotebook.hasRecipientUserId() ? sharedNotebook.recipientUserId() : nullValue));
    query.bindValue(QStringLiteral(":sharedNotebookRecipientIdentityId"), (sharedNotebook.hasRecipientIdentityId() ? sharedNotebook.recipientIdentityId() : nullValue));
    query.bindValue(QStringLiteral(":sharedNotebookAssignmentTimestamp"), (sharedNotebook.hasAssignmentTimestamp() ? sharedNotebook.assignmentTimestamp() : nullValue));
    query.bindValue(QStringLiteral(":indexInNotebook"), (sharedNotebook.indexInNotebook() >= 0 ? sharedNotebook.indexInNotebook() : nullValue));

    res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR();

    query.finish();
    return true;
}

bool LocalStorageManagerPrivate::rowExists(const QString & tableName, const QString & uniqueKeyName,
                                           const QVariant & uniqueKeyValue) const
{
    QString key = uniqueKeyValue.toString();
    key = sqlEscapeString(key);
    QString queryString = QString("SELECT count(*) FROM %1 WHERE %2='%3'").arg(tableName,uniqueKeyName,key);

    QSqlQuery query(m_sqlDatabase);
    bool res = query.exec(queryString);
    if (!res) {
        QNWARNING(QStringLiteral("Unable to check the existence of row with key name ") << uniqueKeyName
                  << QStringLiteral(", value = ") << key << QStringLiteral(" in table ") << tableName
                  << QStringLiteral(": unable to execute SQL statement: ") << query.lastError().text()
                  << QStringLiteral("; assuming no such row exists"));
        return false;
    }

    while(query.next() && query.isValid()) {
        int count = query.value(0).toInt();
        return (count != 0);
    }

    return false;
}

bool LocalStorageManagerPrivate::insertOrReplaceUser(const User & user, ErrorString & errorDescription)
{
    // NOTE: this method is expected to be called after the check of user object for sanity of its parameters!

    ErrorString errorPrefix(QT_TRANSLATE_NOOP("", "can't insert or replace User into local storage database"));

    Transaction transaction(m_sqlDatabase, *this, Transaction::Exclusive);

    QString userId = QString::number(user.id());
    QVariant nullValue;

    // Insert or replace common user data
    {
        bool res = checkAndPrepareInsertOrReplaceUserQuery();
        QSqlQuery & query = m_insertOrReplaceUserQuery;
        DATABASE_CHECK_AND_SET_ERROR();

        query.bindValue(QStringLiteral(":id"), userId);
        query.bindValue(QStringLiteral(":username"), (user.hasUsername() ? user.username() : nullValue));
        query.bindValue(QStringLiteral(":email"), (user.hasEmail() ? user.email() : nullValue));
        query.bindValue(QStringLiteral(":name"), (user.hasName() ? user.name() : nullValue));
        query.bindValue(QStringLiteral(":timezone"), (user.hasTimezone() ? user.timezone() : nullValue));
        query.bindValue(QStringLiteral(":privilege"), (user.hasPrivilegeLevel() ? user.privilegeLevel() : nullValue));
        query.bindValue(QStringLiteral(":serviceLevel"), (user.hasServiceLevel() ? user.serviceLevel() : nullValue));
        query.bindValue(QStringLiteral(":userCreationTimestamp"), (user.hasCreationTimestamp() ? user.creationTimestamp() : nullValue));
        query.bindValue(QStringLiteral(":userModificationTimestamp"), (user.hasModificationTimestamp() ? user.modificationTimestamp() : nullValue));
        query.bindValue(QStringLiteral(":userIsDirty"), (user.isDirty() ? 1 : 0));
        query.bindValue(QStringLiteral(":userIsLocal"), (user.isLocal() ? 1 : 0));
        query.bindValue(QStringLiteral(":userDeletionTimestamp"), (user.hasDeletionTimestamp() ? user.deletionTimestamp() : nullValue));
        query.bindValue(QStringLiteral(":userIsActive"), (user.hasActive() ? (user.active() ? 1 : 0) : nullValue));
        query.bindValue(QStringLiteral(":userShardId"), (user.hasShardId() ? user.shardId() : nullValue));
        query.bindValue(QStringLiteral(":userPhotoUrl"), (user.hasPhotoUrl() ? user.photoUrl() : nullValue));
        query.bindValue(QStringLiteral(":userPhotoLastUpdateTimestamp"), (user.hasPhotoLastUpdateTimestamp() ? user.photoLastUpdateTimestamp() : nullValue));

        res = query.exec();
        DATABASE_CHECK_AND_SET_ERROR();

        query.finish();
    }

    if (user.hasUserAttributes())
    {
        ErrorString error;
        bool res = insertOrReplaceUserAttributes(user.id(), user.userAttributes(), error);
        if (!res) {
            errorDescription.base() = errorPrefix.base();
            errorDescription.additionalBases().append(error.base());
            errorDescription.additionalBases().append(error.additionalBases());
            errorDescription.details() = error.details();
            return false;
        }
    }
    else
    {
        // Clean entries from UserAttributesViewedPromotions table
        {
            QString queryString = QString("DELETE FROM UserAttributesViewedPromotions WHERE id=%1").arg(userId);
            QSqlQuery query(m_sqlDatabase);
            bool res = query.exec(queryString);
            DATABASE_CHECK_AND_SET_ERROR();
        }

        // Clean entries from UserAttributesRecentMailedAddresses table
        {
            QString queryString = QString("DELETE FROM UserAttributesRecentMailedAddresses WHERE id=%1").arg(userId);
            QSqlQuery query(m_sqlDatabase);
            bool res = query.exec(queryString);
            DATABASE_CHECK_AND_SET_ERROR();
        }

        // Clean entries from UserAttributes table
        {
            QString queryString = QString("DELETE FROM UserAttributes WHERE id=%1").arg(userId);
            QSqlQuery query(m_sqlDatabase);
            bool res = query.exec(queryString);
            DATABASE_CHECK_AND_SET_ERROR();
        }
    }

    if (user.hasAccounting())
    {
        ErrorString error;
        bool res = insertOrReplaceAccounting(user.id(), user.accounting(), error);
        if (!res) {
            errorDescription.base() = errorPrefix.base();
            errorDescription.additionalBases().append(error.base());
            errorDescription.additionalBases().append(error.additionalBases());
            errorDescription.details() = error.details();
            return false;
        }
    }
    else
    {
        QString queryString = QString("DELETE FROM Accounting WHERE id=%1").arg(userId);
        QSqlQuery query(m_sqlDatabase);
        bool res = query.exec(queryString);
        DATABASE_CHECK_AND_SET_ERROR();
    }

    if (user.hasAccountLimits())
    {
        ErrorString error;
        bool res = insertOrReplaceAccountLimits(user.id(), user.accountLimits(), error);
        if (!res) {
            errorDescription.base() = errorPrefix.base();
            errorDescription.additionalBases().append(error.base());
            errorDescription.additionalBases().append(error.additionalBases());
            errorDescription.details() = error.details();
            return false;
        }
    }
    else
    {
        QString queryString = QString("DELETE FROM AccountLimits WHERE id=%1").arg(userId);
        QSqlQuery query(m_sqlDatabase);
        bool res = query.exec(queryString);
        DATABASE_CHECK_AND_SET_ERROR();
    }

    if (user.hasBusinessUserInfo())
    {
        ErrorString error;
        bool res = insertOrReplaceBusinessUserInfo(user.id(), user.businessUserInfo(), error);
        if (!res) {
            errorDescription.base() = errorPrefix.base();
            errorDescription.additionalBases().append(error.base());
            errorDescription.additionalBases().append(error.additionalBases());
            errorDescription.details() = error.details();
            return false;
        }
    }
    else
    {
        QString queryString = QString("DELETE FROM BusinessUserInfo WHERE id=%1").arg(userId);
        QSqlQuery query(m_sqlDatabase);
        bool res = query.exec(queryString);
        DATABASE_CHECK_AND_SET_ERROR();
    }

    return transaction.commit(errorDescription);
}

bool LocalStorageManagerPrivate::insertOrReplaceBusinessUserInfo(const qevercloud::UserID id, const qevercloud::BusinessUserInfo & info,
                                                                 ErrorString & errorDescription)
{
    ErrorString errorPrefix(QT_TRANSLATE_NOOP("", "can't insert or replace business user info"));

    bool res = checkAndPrepareInsertOrReplaceBusinessUserInfoQuery();
    QSqlQuery & query = m_insertOrReplaceBusinessUserInfoQuery;
    DATABASE_CHECK_AND_SET_ERROR();

    QVariant nullValue;

    query.bindValue(QStringLiteral(":id"), id);
    query.bindValue(QStringLiteral(":businessId"), (info.businessId.isSet() ? info.businessId.ref() : nullValue));
    query.bindValue(QStringLiteral(":businessName"), (info.businessName.isSet() ? info.businessName.ref() : nullValue));
    query.bindValue(QStringLiteral(":role"), (info.role.isSet() ? info.role.ref() : nullValue));
    query.bindValue(QStringLiteral(":businessInfoEmail"), (info.email.isSet() ? info.email.ref() : nullValue));

    res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR();

    query.finish();
    return true;
}

bool LocalStorageManagerPrivate::insertOrReplaceAccounting(const qevercloud::UserID id, const qevercloud::Accounting & accounting,
                                                           ErrorString & errorDescription)
{
    ErrorString errorPrefix(QT_TRANSLATE_NOOP("", "can't insert or replace accounting"));

    bool res = checkAndPrepareInsertOrReplaceAccountingQuery();
    QSqlQuery & query = m_insertOrReplaceAccountingQuery;
    DATABASE_CHECK_AND_SET_ERROR();

    query.bindValue(QStringLiteral(":id"), id);

    QVariant nullValue;

#define CHECK_AND_BIND_VALUE(name) \
    query.bindValue(QStringLiteral(":" #name), accounting.name.isSet() ? accounting.name.ref() : nullValue)

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
    CHECK_AND_BIND_VALUE(availablePoints);

#undef CHECK_AND_BIND_VALUE

    res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR();

    query.finish();
    return true;
}

bool LocalStorageManagerPrivate::insertOrReplaceAccountLimits(const qevercloud::UserID id, const qevercloud::AccountLimits & accountLimits,
                                                              ErrorString & errorDescription)
{
    ErrorString errorPrefix(QT_TRANSLATE_NOOP("", "can't insert or replace account limits"));

    bool res = checkAndPrepareInsertOrReplaceAccountLimitsQuery();
    QSqlQuery & query = m_insertOrReplaceAccountLimitsQuery;
    DATABASE_CHECK_AND_SET_ERROR();

    query.bindValue(QStringLiteral(":id"), id);

    QVariant nullValue;

#define CHECK_AND_BIND_VALUE(name) \
    query.bindValue(QStringLiteral(":" #name), accountLimits.name.isSet() ? accountLimits.name.ref() : nullValue)

    CHECK_AND_BIND_VALUE(userMailLimitDaily);
    CHECK_AND_BIND_VALUE(noteSizeMax);
    CHECK_AND_BIND_VALUE(resourceSizeMax);
    CHECK_AND_BIND_VALUE(userLinkedNotebookMax);
    CHECK_AND_BIND_VALUE(uploadLimit);
    CHECK_AND_BIND_VALUE(userNoteCountMax);
    CHECK_AND_BIND_VALUE(userNotebookCountMax);
    CHECK_AND_BIND_VALUE(userTagCountMax);
    CHECK_AND_BIND_VALUE(noteTagCountMax);
    CHECK_AND_BIND_VALUE(userSavedSearchesMax);
    CHECK_AND_BIND_VALUE(noteResourceCountMax);

#undef CHECK_AND_BIND_VALUE

    res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR();

    query.finish();
    return true;
}

bool LocalStorageManagerPrivate::insertOrReplaceUserAttributes(const qevercloud::UserID id, const qevercloud::UserAttributes & attributes,
                                                               ErrorString & errorDescription)
{
    ErrorString errorPrefix(QT_TRANSLATE_NOOP("", "can't insert or replace user attributes"));

    QVariant nullValue;

    // Insert or replace common user attributes data
    {
        bool res = checkAndPrepareInsertOrReplaceUserAttributesQuery();
        QSqlQuery & query = m_insertOrReplaceUserAttributesQuery;
        DATABASE_CHECK_AND_SET_ERROR();

        query.bindValue(QStringLiteral(":id"), id);

#define CHECK_AND_BIND_VALUE(name) \
    query.bindValue(QStringLiteral(":" #name), (attributes.name.isSet() ? attributes.name.ref() : nullValue))

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
        CHECK_AND_BIND_VALUE(emailAddressLastConfirmed);
        CHECK_AND_BIND_VALUE(passwordUpdated);

#undef CHECK_AND_BIND_VALUE

#define CHECK_AND_BIND_BOOLEAN_VALUE(name) \
    query.bindValue(QStringLiteral(":" #name), (attributes.name.isSet() ? (attributes.name.ref() ? 1 : 0) : nullValue))

        CHECK_AND_BIND_BOOLEAN_VALUE(preactivation);
        CHECK_AND_BIND_BOOLEAN_VALUE(clipFullPage);
        CHECK_AND_BIND_BOOLEAN_VALUE(educationalDiscount);
        CHECK_AND_BIND_BOOLEAN_VALUE(hideSponsorBilling);
        CHECK_AND_BIND_BOOLEAN_VALUE(useEmailAutoFiling);
        CHECK_AND_BIND_BOOLEAN_VALUE(salesforcePushEnabled);
        CHECK_AND_BIND_BOOLEAN_VALUE(shouldLogClientEvent);

#undef CHECK_AND_BIND_BOOLEAN_VALUE

        res = query.exec();
        DATABASE_CHECK_AND_SET_ERROR();

        query.finish();
    }

    // Clean viewed promotions first, then re-insert
    {
        QString queryString = QString("DELETE FROM UserAttributesViewedPromotions WHERE id=%1").arg(id);
        QSqlQuery query(m_sqlDatabase);
        bool res = query.exec(queryString);
        DATABASE_CHECK_AND_SET_ERROR();
    }

    if (attributes.viewedPromotions.isSet())
    {
        bool res = checkAndPrepareInsertOrReplaceUserAttributesViewedPromotionsQuery();
        QSqlQuery & query = m_insertOrReplaceUserAttributesViewedPromotionsQuery;
        DATABASE_CHECK_AND_SET_ERROR();

        query.bindValue(QStringLiteral(":id"), id);

        const auto & viewedPromotions = attributes.viewedPromotions.ref();
        for(auto it = viewedPromotions.begin(), end = viewedPromotions.end(); it != end; ++it) {
            query.bindValue(QStringLiteral(":promotion"), *it);
            res = query.exec();
            DATABASE_CHECK_AND_SET_ERROR();
            query.finish();
        }
    }

    // Clean recent mailed addresses first, then re-insert
    {
        QString queryString = QString("DELETE FROM UserAttributesRecentMailedAddresses WHERE id=%1").arg(id);
        QSqlQuery query(m_sqlDatabase);
        bool res = query.exec(queryString);
        DATABASE_CHECK_AND_SET_ERROR();
    }

    if (attributes.recentMailedAddresses.isSet())
    {
        bool res = checkAndPrepareInsertOrReplaceUserAttributesRecentMailedAddressesQuery();
        QSqlQuery & query = m_insertOrReplaceUserAttributesRecentMailedAddressesQuery;
        DATABASE_CHECK_AND_SET_ERROR();

        query.bindValue(QStringLiteral(":id"), id);

        const auto & recentMailedAddresses = attributes.recentMailedAddresses.ref();
        for(auto it = recentMailedAddresses.begin(), end = recentMailedAddresses.end(); it != end; ++it) {
            query.bindValue(QStringLiteral(":address"), *it);
            res = query.exec();
            DATABASE_CHECK_AND_SET_ERROR();
            query.finish();
        }
    }

    return true;
}

bool LocalStorageManagerPrivate::checkAndPrepareUserCountQuery() const
{
    if (Q_LIKELY(m_getUserCountQueryPrepared)) {
        return true;
    }

    QNDEBUG(QStringLiteral("Preparing SQL query to get the count of users"));

    m_getUserCountQuery = QSqlQuery(m_sqlDatabase);
    bool res = m_getUserCountQuery.prepare(QStringLiteral("SELECT COUNT(*) FROM Users WHERE userDeletionTimestamp IS NULL"));
    if (res) {
        m_getUserCountQueryPrepared = true;
    }

    return res;
}

bool LocalStorageManagerPrivate::checkAndPrepareInsertOrReplaceUserQuery()
{
    if (Q_LIKELY(m_insertOrReplaceUserQueryPrepared)) {
        return true;
    }

    QNDEBUG(QStringLiteral("Preparing SQL query to insert or replace user"));

    m_insertOrReplaceUserQuery = QSqlQuery(m_sqlDatabase);
    bool res = m_insertOrReplaceUserQuery.prepare(QStringLiteral("INSERT OR REPLACE INTO Users"
                                                                 "(id, username, email, name, timezone, privilege, serviceLevel, "
                                                                 "userCreationTimestamp, userModificationTimestamp, userIsDirty, "
                                                                 "userIsLocal, userDeletionTimestamp, userIsActive, userShardId, "
                                                                 "userPhotoUrl, userPhotoLastUpdateTimestamp) "
                                                                 "VALUES(:id, :username, :email, :name, :timezone, :privilege, "
                                                                 ":serviceLevel, :userCreationTimestamp, :userModificationTimestamp, "
                                                                 ":userIsDirty, :userIsLocal, :userDeletionTimestamp, :userIsActive, "
                                                                 ":userShardId, :userPhotoUrl, :userPhotoLastUpdateTimestamp)"));
    if (res) {
        m_insertOrReplaceUserQueryPrepared = true;
    }

    return res;
}

bool LocalStorageManagerPrivate::checkAndPrepareInsertOrReplaceAccountingQuery()
{
    if (Q_LIKELY(m_insertOrReplaceAccountingQueryPrepared)) {
        return true;
    }

    QNDEBUG(QStringLiteral("Preparing SQL query to insert or replace accounting"));

    m_insertOrReplaceAccountingQuery = QSqlQuery(m_sqlDatabase);
    bool res = m_insertOrReplaceAccountingQuery.prepare(QStringLiteral("INSERT OR REPLACE INTO Accounting"
                                                                       "(id, uploadLimitEnd, uploadLimitNextMonth, "
                                                                       "premiumServiceStatus, premiumOrderNumber, premiumCommerceService, "
                                                                       "premiumServiceStart, premiumServiceSKU, lastSuccessfulCharge, "
                                                                       "lastFailedCharge, lastFailedChargeReason, nextPaymentDue, premiumLockUntil, "
                                                                       "updated, premiumSubscriptionNumber, lastRequestedCharge, currency, "
                                                                       "unitPrice, unitDiscount, nextChargeDate, availablePoints) "
                                                                       "VALUES(:id, :uploadLimitEnd, :uploadLimitNextMonth, "
                                                                       ":premiumServiceStatus, :premiumOrderNumber, :premiumCommerceService, "
                                                                       ":premiumServiceStart, :premiumServiceSKU, :lastSuccessfulCharge, "
                                                                       ":lastFailedCharge, :lastFailedChargeReason, :nextPaymentDue, :premiumLockUntil, "
                                                                       ":updated, :premiumSubscriptionNumber, :lastRequestedCharge, :currency, "
                                                                       ":unitPrice, :unitDiscount, :nextChargeDate, :availablePoints)"));
    if (res) {
        m_insertOrReplaceAccountingQueryPrepared = true;
    }

    return res;
}

bool LocalStorageManagerPrivate::checkAndPrepareInsertOrReplaceAccountLimitsQuery()
{
    if (Q_LIKELY(m_insertOrReplaceAccountLimitsQueryPrepared)) {
        return true;
    }

    QNDEBUG(QStringLiteral("Preparing SQL query to insert or replace account limits"));

    m_insertOrReplaceAccountLimitsQuery = QSqlQuery(m_sqlDatabase);
    bool res = m_insertOrReplaceAccountLimitsQuery.prepare(QStringLiteral("INSERT OR REPLACE INTO AccountLimits"
                                                                          "(id, userMailLimitDaily, noteSizeMax, resourceSizeMax, "
                                                                          "userLinkedNotebookMax, uploadLimit, userNoteCountMax, "
                                                                          "userNotebookCountMax, userTagCountMax, noteTagCountMax, "
                                                                          "userSavedSearchesMax, noteResourceCountMax) "
                                                                          "VALUES(:id, :userMailLimitDaily, :noteSizeMax, :resourceSizeMax, "
                                                                          ":userLinkedNotebookMax, :uploadLimit, :userNoteCountMax, "
                                                                          ":userNotebookCountMax, :userTagCountMax, :noteTagCountMax, "
                                                                          ":userSavedSearchesMax, :noteResourceCountMax)"));
    if (res) {
        m_insertOrReplaceAccountLimitsQueryPrepared = true;
    }

    return true;
}

bool LocalStorageManagerPrivate::checkAndPrepareInsertOrReplaceBusinessUserInfoQuery()
{
    if (Q_LIKELY(m_insertOrReplaceBusinessUserInfoQueryPrepared)) {
        return true;
    }

    QNDEBUG(QStringLiteral("Preparing SQl query to insert or replace business user info"));

    m_insertOrReplaceBusinessUserInfoQuery = QSqlQuery(m_sqlDatabase);
    bool res = m_insertOrReplaceBusinessUserInfoQuery.prepare(QStringLiteral("INSERT OR REPLACE INTO BusinessUserInfo"
                                                                             "(id, businessId, businessName, role, businessInfoEmail) "
                                                                             "VALUES(:id, :businessId, :businessName, :role, :businessInfoEmail)"));
    if (res) {
        m_insertOrReplaceBusinessUserInfoQueryPrepared = true;
    }

    return res;
}

bool LocalStorageManagerPrivate::checkAndPrepareInsertOrReplaceUserAttributesQuery()
{
    if (Q_LIKELY(m_insertOrReplaceUserAttributesQueryPrepared)) {
        return true;
    }

    QNDEBUG(QStringLiteral("Preparing SQL query to insert or replace user attributes"));

    m_insertOrReplaceUserAttributesQuery = QSqlQuery(m_sqlDatabase);
    bool res = m_insertOrReplaceUserAttributesQuery.prepare(QStringLiteral("INSERT OR REPLACE INTO UserAttributes"
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
                                                                           "useEmailAutoFiling, reminderEmailConfig, "
                                                                           "emailAddressLastConfirmed, passwordUpdated, "
                                                                           "salesforcePushEnabled, shouldLogClientEvent) "
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
                                                                           ":useEmailAutoFiling, :reminderEmailConfig, "
                                                                           ":emailAddressLastConfirmed, :passwordUpdated, "
                                                                           ":salesforcePushEnabled, :shouldLogClientEvent)"));
    if (res) {
        m_insertOrReplaceUserAttributesQueryPrepared = true;
    }

    return res;
}

bool LocalStorageManagerPrivate::checkAndPrepareInsertOrReplaceUserAttributesViewedPromotionsQuery()
{
    if (Q_LIKELY(m_insertOrReplaceUserAttributesViewedPromotionsQueryPrepared)) {
        return true;
    }

    QNDEBUG(QStringLiteral("Preparing SQL query to insert or replace user attributes viewed promotions"));

    m_insertOrReplaceUserAttributesViewedPromotionsQuery = QSqlQuery(m_sqlDatabase);
    bool res = m_insertOrReplaceUserAttributesViewedPromotionsQuery.prepare(QStringLiteral("INSERT OR REPLACE INTO UserAttributesViewedPromotions"
                                                                                           "(id, promotion) VALUES(:id, :promotion)"));
    if (res) {
        m_insertOrReplaceUserAttributesViewedPromotionsQueryPrepared = true;
    }

    return res;
}

bool LocalStorageManagerPrivate::checkAndPrepareInsertOrReplaceUserAttributesRecentMailedAddressesQuery()
{
    if (Q_LIKELY(m_insertOrReplaceUserAttributesRecentMailedAddressesQueryPrepared)) {
        return true;
    }

    QNDEBUG(QStringLiteral("Preparing SQL query to insert or replace user attributes recent mailed addresses"));

    m_insertOrReplaceUserAttributesRecentMailedAddressesQuery = QSqlQuery(m_sqlDatabase);
    bool res = m_insertOrReplaceUserAttributesRecentMailedAddressesQuery.prepare(QStringLiteral("INSERT OR REPLACE INTO UserAttributesRecentMailedAddresses"
                                                                                                "(id, address) VALUES(:id, :address)"));
    if (res) {
        m_insertOrReplaceUserAttributesRecentMailedAddressesQueryPrepared = true;
    }

    return res;
}

bool LocalStorageManagerPrivate::checkAndPrepareDeleteUserQuery()
{
    if (Q_LIKELY(m_deleteUserQueryPrepared)) {
        return true;
    }

    QNDEBUG(QStringLiteral("Preparing SQL query to mark user deleted"));

    m_deleteUserQuery = QSqlQuery(m_sqlDatabase);
    bool res = m_deleteUserQuery.prepare(QStringLiteral("UPDATE Users SET userDeletionTimestamp = :userDeletionTimestamp, "
                                                        "userIsLocal = :userIsLocal WHERE id = :id"));
    if (res) {
        m_deleteUserQueryPrepared = true;
    }

    return res;
}

bool LocalStorageManagerPrivate::insertOrReplaceNotebook(const Notebook & notebook,
                                                         ErrorString & errorDescription)
{
    // NOTE: this method expects to be called after notebook is already checked
    // for sanity of its parameters!

    ErrorString errorPrefix(QT_TRANSLATE_NOOP("", "can't insert or replace notebook"));

    Transaction transaction(m_sqlDatabase, *this, Transaction::Exclusive);

    QString localUid = sqlEscapeString(notebook.localUid());
    QVariant nullValue;

    // Insert or replace common Notebook data
    {
        bool res = checkAndPrepareInsertOrReplaceNotebookQuery();
        QSqlQuery & query = m_insertOrReplaceNotebookQuery;
        DATABASE_CHECK_AND_SET_ERROR();

        query.bindValue(QStringLiteral(":localUid"), (localUid.isEmpty() ? nullValue : localUid));
        query.bindValue(QStringLiteral(":guid"), (notebook.hasGuid() ? notebook.guid() : nullValue));
        query.bindValue(QStringLiteral(":linkedNotebookGuid"), (notebook.hasLinkedNotebookGuid() ? notebook.linkedNotebookGuid() : nullValue));
        query.bindValue(QStringLiteral(":updateSequenceNumber"), (notebook.hasUpdateSequenceNumber() ? notebook.updateSequenceNumber() : nullValue));
        query.bindValue(QStringLiteral(":notebookName"), (notebook.hasName() ? notebook.name() : nullValue));
        query.bindValue(QStringLiteral(":notebookNameUpper"), (notebook.hasName() ? notebook.name().toUpper() : nullValue));
        query.bindValue(QStringLiteral(":creationTimestamp"), (notebook.hasCreationTimestamp() ? notebook.creationTimestamp() : nullValue));
        query.bindValue(QStringLiteral(":modificationTimestamp"), (notebook.hasModificationTimestamp() ? notebook.modificationTimestamp() : nullValue));
        query.bindValue(QStringLiteral(":isDirty"), (notebook.isDirty() ? 1 : 0));
        query.bindValue(QStringLiteral(":isLocal"), (notebook.isLocal() ? 1 : 0));
        query.bindValue(QStringLiteral(":isDefault"), (notebook.isDefaultNotebook() ? 1 : nullValue));
        query.bindValue(QStringLiteral(":isLastUsed"), (notebook.isLastUsed() ? 1 : nullValue));
        query.bindValue(QStringLiteral(":isFavorited"), (notebook.isFavorited() ? 1 : 0));
        query.bindValue(QStringLiteral(":publishingUri"), (notebook.hasPublishingUri() ? notebook.publishingUri() : nullValue));
        query.bindValue(QStringLiteral(":publishingNoteSortOrder"), (notebook.hasPublishingOrder() ? notebook.publishingOrder() : nullValue));
        query.bindValue(QStringLiteral(":publishingAscendingSort"), (notebook.hasPublishingAscending() ? (notebook.isPublishingAscending() ? 1 : 0) : nullValue));
        query.bindValue(QStringLiteral(":publicDescription"), (notebook.hasPublishingPublicDescription() ? notebook.publishingPublicDescription() : nullValue));
        query.bindValue(QStringLiteral(":isPublished"), (notebook.hasPublished() ? (notebook.isPublished() ? 1 : 0) : nullValue));
        query.bindValue(QStringLiteral(":stack"), (notebook.hasStack() ? notebook.stack() : nullValue));
        query.bindValue(QStringLiteral(":businessNotebookDescription"), (notebook.hasBusinessNotebookDescription() ? notebook.businessNotebookDescription() : nullValue));
        query.bindValue(QStringLiteral(":businessNotebookPrivilegeLevel"), (notebook.hasBusinessNotebookPrivilegeLevel() ? notebook.businessNotebookPrivilegeLevel() : nullValue));
        query.bindValue(QStringLiteral(":businessNotebookIsRecommended"), (notebook.hasBusinessNotebookRecommended() ? (notebook.isBusinessNotebookRecommended() ? 1 : 0) : nullValue));
        query.bindValue(QStringLiteral(":contactId"), (notebook.hasContact() && notebook.contact().hasId() ? notebook.contact().id() : nullValue));
        query.bindValue(QStringLiteral(":recipientReminderNotifyEmail"), (notebook.hasRecipientReminderNotifyEmail()
                                                                          ? (notebook.recipientReminderNotifyEmail() ? 1 : 0)
                                                                          : nullValue));
        query.bindValue(QStringLiteral(":recipientReminderNotifyInApp"), (notebook.hasRecipientReminderNotifyInApp()
                                                                          ? (notebook.recipientReminderNotifyInApp() ? 1 : 0)
                                                                          : nullValue));
        query.bindValue(QStringLiteral(":recipientInMyList"), (notebook.hasRecipientInMyList()
                                                               ? (notebook.recipientInMyList() ? 1 : 0)
                                                               : nullValue));
        query.bindValue(QStringLiteral(":recipientStack"), (notebook.hasRecipientStack() ? notebook.recipientStack() : nullValue));

        res = query.exec();
        DATABASE_CHECK_AND_SET_ERROR();

        query.finish();
    }

    if (notebook.hasRestrictions())
    {
        ErrorString error;
        bool res = insertOrReplaceNotebookRestrictions(localUid, notebook.restrictions(), error);
        if (!res) {
            errorDescription.base() = errorPrefix.base();
            errorDescription.additionalBases().append(error.base());
            errorDescription.additionalBases().append(error.additionalBases());
            errorDescription.details() = error.details();
            return res;
        }
    }
    else
    {
        QString queryString = QString("DELETE FROM NotebookRestrictions WHERE localUid='%1'").arg(localUid);
        QSqlQuery query(m_sqlDatabase);
        bool res = query.exec(queryString);
        DATABASE_CHECK_AND_SET_ERROR();
    }

    if (notebook.hasGuid())
    {
        QString guid = sqlEscapeString(notebook.guid());
        QString queryString = QString("DELETE FROM SharedNotebooks WHERE sharedNotebookNotebookGuid='%1'").arg(guid);
        QSqlQuery query(m_sqlDatabase);
        bool res = query.exec(queryString);
        DATABASE_CHECK_AND_SET_ERROR();

        QList<SharedNotebook> sharedNotebooks = notebook.sharedNotebooks();
        for(int i = 0, size = sharedNotebooks.size(); i < size; ++i)
        {
            const SharedNotebook & sharedNotebook = sharedNotebooks[i];
            if (!sharedNotebook.hasId()) {
                QNWARNING(QStringLiteral("Found shared notebook without primary identifier of the share set, skipping it: ")
                          << sharedNotebook);
                continue;
            }

            ErrorString error;
            res = insertOrReplaceSharedNotebook(sharedNotebook, error);
            if (!res) {
                errorDescription.base() = errorPrefix.base();
                errorDescription.additionalBases().append(error.base());
                errorDescription.additionalBases().append(error.additionalBases());
                errorDescription.details() = error.details();
                return res;
            }
        }
    }

    return transaction.commit(errorDescription);
}

bool LocalStorageManagerPrivate::checkAndPrepareNotebookCountQuery() const
{
    if (Q_LIKELY(m_getNotebookCountQueryPrepared)) {
        return true;
    }

    QNDEBUG(QStringLiteral("Preparing SQL query to get the count of notebooks"));

    m_getNotebookCountQuery = QSqlQuery(m_sqlDatabase);
    bool res = m_getNotebookCountQuery.prepare(QStringLiteral("SELECT COUNT(*) FROM Notebooks"));
    if (res) {
        m_getNotebookCountQueryPrepared = true;
    }

    return res;
}

bool LocalStorageManagerPrivate::checkAndPrepareInsertOrReplaceNotebookQuery()
{
    if (Q_LIKELY(m_insertOrReplaceNotebookQueryPrepared)) {
        return true;
    }

    QNDEBUG(QStringLiteral("Preparing SQL query to insert or replace notebook"));

    m_insertOrReplaceNotebookQuery = QSqlQuery(m_sqlDatabase);
    bool res = m_insertOrReplaceNotebookQuery.prepare(QStringLiteral("INSERT OR REPLACE INTO Notebooks"
                                                                     "(localUid, guid, linkedNotebookGuid, updateSequenceNumber, "
                                                                     "notebookName, notebookNameUpper, creationTimestamp, "
                                                                     "modificationTimestamp, isDirty, isLocal, "
                                                                     "isDefault, isLastUsed, isFavorited, publishingUri, "
                                                                     "publishingNoteSortOrder, publishingAscendingSort, "
                                                                     "publicDescription, isPublished, stack, businessNotebookDescription, "
                                                                     "businessNotebookPrivilegeLevel, businessNotebookIsRecommended, contactId, "
                                                                     "recipientReminderNotifyEmail, recipientReminderNotifyInApp, recipientInMyList, "
                                                                     "recipientStack) "
                                                                     "VALUES(:localUid, :guid, :linkedNotebookGuid, :updateSequenceNumber, "
                                                                     ":notebookName, :notebookNameUpper, :creationTimestamp, "
                                                                     ":modificationTimestamp, :isDirty, :isLocal, :isDefault, :isLastUsed, "
                                                                     ":isFavorited, :publishingUri, :publishingNoteSortOrder, :publishingAscendingSort, "
                                                                     ":publicDescription, :isPublished, :stack, :businessNotebookDescription, "
                                                                     ":businessNotebookPrivilegeLevel, :businessNotebookIsRecommended, :contactId, "
                                                                     ":recipientReminderNotifyEmail, :recipientReminderNotifyInApp, :recipientInMyList, "
                                                                     ":recipientStack)"));
    if (res) {
        m_insertOrReplaceNotebookQueryPrepared = true;
    }

    return res;
}

bool LocalStorageManagerPrivate::checkAndPrepareInsertOrReplaceNotebookRestrictionsQuery()
{
    if (Q_LIKELY(m_insertOrReplaceNotebookRestrictionsQueryPrepared)) {
        return true;
    }

    QNDEBUG(QStringLiteral("Preparing SQL query to insert or replace notebook restrictions"));

    m_insertOrReplaceNotebookRestrictionsQuery = QSqlQuery(m_sqlDatabase);
    bool res = m_insertOrReplaceNotebookRestrictionsQuery.prepare(QStringLiteral("INSERT OR REPLACE INTO NotebookRestrictions"
                                                                                 "(localUid, noReadNotes, noCreateNotes, noUpdateNotes, "
                                                                                 "noExpungeNotes, noShareNotes, noEmailNotes, noSendMessageToRecipients, "
                                                                                 "noUpdateNotebook, noExpungeNotebook, noSetDefaultNotebook, "
                                                                                 "noSetNotebookStack, noPublishToPublic, noPublishToBusinessLibrary, "
                                                                                 "noCreateTags, noUpdateTags, noExpungeTags, noSetParentTag, "
                                                                                 "noCreateSharedNotebooks, updateWhichSharedNotebookRestrictions, "
                                                                                 "expungeWhichSharedNotebookRestrictions) "
                                                                                 "VALUES(:localUid, :noReadNotes, :noCreateNotes, :noUpdateNotes, "
                                                                                 ":noExpungeNotes, :noShareNotes, :noEmailNotes, :noSendMessageToRecipients, "
                                                                                 ":noUpdateNotebook, :noExpungeNotebook, :noSetDefaultNotebook, "
                                                                                 ":noSetNotebookStack, :noPublishToPublic, :noPublishToBusinessLibrary, "
                                                                                 ":noCreateTags, :noUpdateTags, :noExpungeTags, :noSetParentTag, "
                                                                                 ":noCreateSharedNotebooks, :updateWhichSharedNotebookRestrictions, "
                                                                                 ":expungeWhichSharedNotebookRestrictions)"));
    if (res) {
        m_insertOrReplaceNotebookRestrictionsQueryPrepared = true;
    }

    return res;
}

bool LocalStorageManagerPrivate::checkAndPrepareInsertOrReplaceSharedNotebookQuery()
{
    if (Q_LIKELY(m_insertOrReplaceSharedNotebookQueryPrepared)) {
        return true;
    }

    QNDEBUG(QStringLiteral("Preparing SQL query to insert or replace shared notebook"));

    m_insertOrReplaceSharedNotebookQuery = QSqlQuery(m_sqlDatabase);
    bool res = m_insertOrReplaceSharedNotebookQuery.prepare(QStringLiteral("INSERT OR REPLACE INTO SharedNotebooks"
                                                                           "(sharedNotebookShareId, sharedNotebookUserId, sharedNotebookNotebookGuid, "
                                                                           "sharedNotebookEmail, sharedNotebookCreationTimestamp, sharedNotebookModificationTimestamp, "
                                                                           "sharedNotebookGlobalId, sharedNotebookUsername, sharedNotebookPrivilegeLevel, "
                                                                           "sharedNotebookRecipientReminderNotifyEmail, sharedNotebookRecipientReminderNotifyInApp, "
                                                                           "sharedNotebookSharerUserId, sharedNotebookRecipientUsername, sharedNotebookRecipientUserId, "
                                                                           "sharedNotebookRecipientIdentityId, sharedNotebookAssignmentTimestamp, indexInNotebook) "
                                                                           "VALUES(:sharedNotebookShareId, :sharedNotebookUserId, :sharedNotebookNotebookGuid, "
                                                                           ":sharedNotebookEmail, :sharedNotebookCreationTimestamp, :sharedNotebookModificationTimestamp, "
                                                                           ":sharedNotebookGlobalId, :sharedNotebookUsername, :sharedNotebookPrivilegeLevel, "
                                                                           ":sharedNotebookRecipientReminderNotifyEmail, :sharedNotebookRecipientReminderNotifyInApp, "
                                                                           ":sharedNotebookSharerUserId, :sharedNotebookRecipientUsername, :sharedNotebookRecipientUserId, "
                                                                           ":sharedNotebookRecipientIdentityId, :sharedNotebookAssignmentTimestamp, :indexInNotebook) "));
    if (res) {
        m_insertOrReplaceSharedNotebookQueryPrepared = true;
    }

    return res;
}

bool LocalStorageManagerPrivate::insertOrReplaceLinkedNotebook(const LinkedNotebook & linkedNotebook,
                                                               ErrorString & errorDescription)
{
    // NOTE: this method expects to be called after the linked notebook
    // is already checked for sanity ot its parameters

    ErrorString errorPrefix(QT_TRANSLATE_NOOP("", "can't insert or replace linked notebook"));

    bool res = checkAndPrepareInsertOrReplaceLinkedNotebookQuery();
    QSqlQuery & query = m_insertOrReplaceLinkedNotebookQuery;
    DATABASE_CHECK_AND_SET_ERROR();

    QVariant nullValue;

    query.bindValue(QStringLiteral(":guid"), (linkedNotebook.hasGuid() ? linkedNotebook.guid() : nullValue));
    query.bindValue(QStringLiteral(":updateSequenceNumber"), (linkedNotebook.hasUpdateSequenceNumber() ? linkedNotebook.updateSequenceNumber() : nullValue));
    query.bindValue(QStringLiteral(":shareName"), (linkedNotebook.hasShareName() ? linkedNotebook.shareName() : nullValue));
    query.bindValue(QStringLiteral(":username"), (linkedNotebook.hasUsername() ? linkedNotebook.username() : nullValue));
    query.bindValue(QStringLiteral(":shardId"), (linkedNotebook.hasShardId() ? linkedNotebook.shardId() : nullValue));
    query.bindValue(QStringLiteral(":sharedNotebookGlobalId"), (linkedNotebook.hasSharedNotebookGlobalId() ? linkedNotebook.sharedNotebookGlobalId() : nullValue));
    query.bindValue(QStringLiteral(":uri"), (linkedNotebook.hasUri() ? linkedNotebook.uri() : nullValue));
    query.bindValue(QStringLiteral(":noteStoreUrl"), (linkedNotebook.hasNoteStoreUrl() ? linkedNotebook.noteStoreUrl() : nullValue));
    query.bindValue(QStringLiteral(":webApiUrlPrefix"), (linkedNotebook.hasWebApiUrlPrefix() ? linkedNotebook.webApiUrlPrefix() : nullValue));
    query.bindValue(QStringLiteral(":stack"), (linkedNotebook.hasStack() ? linkedNotebook.stack() : nullValue));
    query.bindValue(QStringLiteral(":businessId"), (linkedNotebook.hasBusinessId() ? linkedNotebook.businessId() : nullValue));
    query.bindValue(QStringLiteral(":isDirty"), (linkedNotebook.isDirty() ? 1 : 0));

    res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR();

    query.finish();
    return true;
}

bool LocalStorageManagerPrivate::checkAndPrepareGetLinkedNotebookCountQuery() const
{
    if (Q_LIKELY(m_getLinkedNotebookCountQueryPrepared)) {
        return true;
    }

    QNDEBUG(QStringLiteral("Preparing SQL query to get the count of linked notebooks"));

    m_getLinkedNotebookCountQuery = QSqlQuery(m_sqlDatabase);
    bool res = m_getLinkedNotebookCountQuery.prepare(QStringLiteral("SELECT COUNT(*) FROM LinkedNotebooks"));
    if (res) {
        m_getLinkedNotebookCountQueryPrepared = true;
    }

    return res;
}

bool LocalStorageManagerPrivate::checkAndPrepareInsertOrReplaceLinkedNotebookQuery()
{
    if (Q_LIKELY(m_insertOrReplaceLinkedNotebookQueryPrepared)) {
        return true;
    }

    QNDEBUG(QStringLiteral("Preparing SQL query to insert or replace linked notebook"));

    QSqlQuery query(m_sqlDatabase);

    m_insertOrReplaceLinkedNotebookQuery = QSqlQuery(m_sqlDatabase);
    bool res = m_insertOrReplaceLinkedNotebookQuery.prepare(QStringLiteral("INSERT OR REPLACE INTO LinkedNotebooks "
                                                                           "(guid, updateSequenceNumber, shareName, "
                                                                           "username, shardId, sharedNotebookGlobalId, "
                                                                           "uri, noteStoreUrl, webApiUrlPrefix, stack, "
                                                                           "businessId, isDirty) VALUES(:guid, "
                                                                           ":updateSequenceNumber, :shareName, :username, "
                                                                           ":shardId, :sharedNotebookGlobalId, :uri, "
                                                                           ":noteStoreUrl, :webApiUrlPrefix, :stack, "
                                                                           ":businessId, :isDirty)"));
    if (res) {
        m_insertOrReplaceLinkedNotebookQueryPrepared = true;
    }

    return res;
}

bool LocalStorageManagerPrivate::getNoteLocalUidFromResource(const Resource & resource, QString & noteLocalUid,
                                                             ErrorString & errorDescription)
{
    QNDEBUG("LocalStorageManagerPrivate::getNoteLocalUidFromResource: resource = " << resource);

    ErrorString errorPrefix(QT_TRANSLATE_NOOP("", "can't get note local uid for resource"));

    noteLocalUid.resize(0);

    if (resource.hasNoteLocalUid()) {
        noteLocalUid = resource.noteLocalUid();
        return true;
    }

    QNTRACE(QStringLiteral("Resource doesn't have the note local uid, trying to deduce it from note-resource linkage"));

    QString column, uid;
    bool resourceHasGuid = resource.hasGuid();
    if (resourceHasGuid)
    {
        column = QStringLiteral("resource");
        uid = resource.guid();

        if (!checkGuid(uid)) {
            errorDescription.base() = errorPrefix.base();
            errorDescription.additionalBases().append(QT_TRANSLATE_NOOP("", "resource's guid is invalid"));
            errorDescription.details() = uid;
            QNWARNING(errorDescription);
            return false;
        }
    }
    else
    {
        column = QStringLiteral("localResource");
        uid = resource.localUid();

        if (uid.isEmpty()) {
            errorDescription.base() = errorPrefix.base();
            errorDescription.additionalBases().append(QT_TRANSLATE_NOOP("", "both resource's local uid and guid are empty"));
            QNWARNING(errorDescription);
            return false;
        }
    }

    uid = sqlEscapeString(uid);

    QString queryString = QString("SELECT localNote FROM NoteResources WHERE %1='%2'").arg(column,uid);
    QSqlQuery query(m_sqlDatabase);
    bool res = query.exec(queryString);
    DATABASE_CHECK_AND_SET_ERROR();

    res = query.next();
    if (!res) {
        SET_NO_DATA_FOUND();
        return false;
    }

    noteLocalUid = query.record().value(QStringLiteral("localNote")).toString();
    return true;
}

bool LocalStorageManagerPrivate::getNotebookLocalUidFromNote(const Note & note, QString & notebookLocalUid,
                                                             ErrorString & errorDescription)
{
    QNDEBUG(QStringLiteral("LocalStorageManagerPrivate::getNotebookLocalUidFromNote: note local uid = ")
            << note.localUid() << QStringLiteral(", note guid = ") << (note.hasGuid() ? note.guid() : QStringLiteral("<null>")));

    ErrorString errorPrefix(QT_TRANSLATE_NOOP("", "can't get notebook local uid for note"));

    notebookLocalUid.resize(0);

    if (note.hasNotebookLocalUid()) {
        notebookLocalUid = note.notebookLocalUid();
        return true;
    }

    QNTRACE(QStringLiteral("Note doesn't have the notebook local uid, trying to deduce it from guid"));

    if (note.hasNotebookGuid())
    {
        QString notebookGuid = sqlEscapeString(note.notebookGuid());
        QString queryString = QString("SELECT localUid FROM Notebooks WHERE guid = '%1'").arg(notebookGuid);
        QSqlQuery query(m_sqlDatabase);
        bool res = query.exec(queryString);
        DATABASE_CHECK_AND_SET_ERROR();

        res = query.next();
        if (!res) {
            SET_NO_DATA_FOUND();
            return false;
        }

        notebookLocalUid = query.record().value(QStringLiteral("localUid")).toString();
    }
    else
    {
        QString column, uid;
        if (note.hasGuid()) {
            column = QStringLiteral("guid");
            uid = note.guid();
        }
        else {
            column = QStringLiteral("localUid");
            uid = note.localUid();
        }

        uid = sqlEscapeString(uid);

        QString queryString = QString("SELECT notebookLocalUid FROM Notes WHERE %1='%2'").arg(column,uid);
        QSqlQuery query(m_sqlDatabase);
        bool res = query.exec(queryString);
        DATABASE_CHECK_AND_SET_ERROR();

        res = query.next();
        if (!res) {
            SET_NO_DATA_FOUND();
            return false;
        }

        notebookLocalUid = query.record().value(QStringLiteral("notebookLocalUid")).toString();
    }

    if (notebookLocalUid.isEmpty()) {
        errorDescription.base() = errorPrefix.base();
        errorDescription.additionalBases().append(QT_TRANSLATE_NOOP("", "found notebook local uid is empty"));
        QNDEBUG(errorDescription << QStringLiteral(", note: ") << note);
        return false;
    }

    return true;
}

bool LocalStorageManagerPrivate::getNotebookGuidForNote(const Note & note, QString & notebookGuid,
                                                        ErrorString & errorDescription)
{
    QNDEBUG(QStringLiteral("LocalStorageManagerPrivate::getNotebookGuidForNote: note local uid = ") << note.localUid()
            << QStringLiteral(", note guid = ") << (note.hasGuid() ? note.guid() : QStringLiteral("<null>")));

    ErrorString errorPrefix(QT_TRANSLATE_NOOP("", "can't get notebook guid for note"));

    notebookGuid.resize(0);

    if (note.hasNotebookGuid()) {
        notebookGuid = note.notebookGuid();
        return true;
    }

    QNTRACE(QStringLiteral("Note doesn't have the notebook guid, trying to deduce it from notebook local uid"));

    if (!note.hasNotebookLocalUid()) {
        errorDescription.base() = errorPrefix.base();
        errorDescription.additionalBases().append(QT_TRANSLATE_NOOP("", "note has neither notebook local uid nor notebook guid"));
        QNDEBUG(errorDescription << QStringLiteral(", note: ") << note);
        return false;
    }

    QString notebookLocalUid = note.notebookLocalUid();
    notebookLocalUid = sqlEscapeString(notebookLocalUid);

    QString queryString = QString("SELECT guid FROM Notebooks where localUid = '%1'").arg(notebookLocalUid);
    QSqlQuery query(m_sqlDatabase);
    bool res = query.exec(queryString);
    DATABASE_CHECK_AND_SET_ERROR();

    res = query.next();
    if (!res) {
        SET_NO_DATA_FOUND();
        return false;
    }

    notebookGuid = query.record().value(QStringLiteral("guid")).toString();
    return true;
}

bool LocalStorageManagerPrivate::getNotebookLocalUidForGuid(const QString & notebookGuid, QString & notebookLocalUid,
                                                            ErrorString & errorDescription)
{
    QNDEBUG(QStringLiteral("LocalStorageManagerPrivate::getNotebookLocalUidForGuid: notebook guid = ") << notebookGuid);

    ErrorString errorPrefix(QT_TRANSLATE_NOOP("", "can't get notebook local uid for guid"));

    QString queryString = QString("SELECT localUid FROM Notebooks WHERE guid = '%1'").arg(sqlEscapeString(notebookGuid));
    QSqlQuery query(m_sqlDatabase);
    bool res = query.exec(queryString);
    DATABASE_CHECK_AND_SET_ERROR();

    if (query.next()) {
        notebookLocalUid = query.record().value(QStringLiteral("localUid")).toString();
    }

    if (notebookLocalUid.isEmpty()) {
        errorDescription.base() = errorPrefix.base();
        errorDescription.additionalBases().append(QT_TRANSLATE_NOOP("", "no existing local uid corresponding to notebook's guid was found"));
        errorDescription.details() = notebookGuid;
        QNDEBUG(errorDescription);
        return false;
    }

    return true;
}

bool LocalStorageManagerPrivate::getNoteLocalUidForGuid(const QString & noteGuid, QString & noteLocalUid,
                                                        ErrorString & errorDescription)
{
    QNDEBUG(QStringLiteral("LocalStorageManagerPrivate::getNoteLocalUidForGuid: note guid = ") << noteGuid);

    ErrorString errorPrefix(QT_TRANSLATE_NOOP("", "can't get note local uid for guid"));

    QString queryString = QString("SELECT localUid FROM Notes WHERE guid = '%1'").arg(sqlEscapeString(noteGuid));
    QSqlQuery query(m_sqlDatabase);
    bool res = query.exec(queryString);
    DATABASE_CHECK_AND_SET_ERROR();

    if (query.next()) {
        noteLocalUid = query.record().value(QStringLiteral("localUid")).toString();
    }

    if (noteLocalUid.isEmpty()) {
        errorDescription.base() = errorPrefix.base();
        errorDescription.additionalBases().append(QT_TRANSLATE_NOOP("", "no existing local uid corresponding to note's guid was found in local storage"));
        errorDescription.details() = noteGuid;
        QNDEBUG(errorDescription);
        return false;
    }

    return true;
}

bool LocalStorageManagerPrivate::getTagLocalUidForGuid(const QString & tagGuid, QString & tagLocalUid,
                                                       ErrorString & errorDescription)
{
    QNDEBUG(QStringLiteral("LocalStorageManagerPrivate::getTagLocalUidForGuid: tag guid = ") << tagGuid);

    ErrorString errorPrefix(QT_TRANSLATE_NOOP("", "can't get tag local uid for guid"));

    QString queryString = QString("SELECT localUid FROM Tags WHERE guid = '%1'").arg(sqlEscapeString(tagGuid));
    QSqlQuery query(m_sqlDatabase);
    bool res = query.exec(queryString);
    DATABASE_CHECK_AND_SET_ERROR();

    if (query.next()) {
        tagLocalUid = query.record().value(QStringLiteral("localUid")).toString();
    }

    if (tagLocalUid.isEmpty()) {
        errorDescription.base() = errorPrefix.base();
        errorDescription.additionalBases().append(QT_TRANSLATE_NOOP("", "no existing local uid corresponding to tag's guid was found"));
        errorDescription.details() = tagGuid;
        QNDEBUG(errorDescription);
        return false;
    }

    return true;
}

bool LocalStorageManagerPrivate::getResourceLocalUidForGuid(const QString & resourceGuid, QString & resourceLocalUid,
                                                            ErrorString & errorDescription)
{
    QNDEBUG(QStringLiteral("LocalStorageManagerPrivate::getResourceLocalUidForGuid: resource guid = ") << resourceGuid);

    ErrorString errorPrefix(QT_TRANSLATE_NOOP("", "can't get resource local uid for guid"));

    QString queryString = QString("SELECT resourceLocalUid FROM Resources WHERE resourceGuid = '%1'").arg(sqlEscapeString(resourceGuid));
    QSqlQuery query(m_sqlDatabase);
    bool res = query.exec(queryString);
    DATABASE_CHECK_AND_SET_ERROR();

    if (query.next()) {
        resourceLocalUid = query.record().value(QStringLiteral("resourceLocalUid")).toString();
    }

    if (resourceLocalUid.isEmpty()) {
        errorDescription.base() = errorPrefix.base();
        errorDescription.additionalBases().append(QT_TRANSLATE_NOOP("", "no existing local uid corresponding to resource's guid was found"));
        errorDescription.details() = resourceGuid;
        QNDEBUG(errorDescription);
        return false;
    }

    return true;
}

bool LocalStorageManagerPrivate::getSavedSearchLocalUidForGuid(const QString & savedSearchGuid, QString & savedSearchLocalUid,
                                                               ErrorString & errorDescription)
{
    QNDEBUG(QStringLiteral("LocalStorageManagerPrivate::getSavedSearchLocalUidForGuid: saved search guid = ") << savedSearchGuid);

    ErrorString errorPrefix(QT_TRANSLATE_NOOP("", "can't get saved search local uid for guid"));

    QString queryString = QString("SELECT localUid FROM SavedSearches WHERE guid = '%1'").arg(sqlEscapeString(savedSearchGuid));
    QSqlQuery query(m_sqlDatabase);
    bool res = query.exec(queryString);
    DATABASE_CHECK_AND_SET_ERROR();

    if (query.next()) {
        savedSearchLocalUid = query.record().value(QStringLiteral("localUid")).toString();
    }

    if (savedSearchLocalUid.isEmpty()) {
        errorDescription.base() = errorPrefix.base();
        errorDescription.additionalBases().append(QT_TRANSLATE_NOOP("", "no existing local uid corresponding to saved search's guid was found"));
        errorDescription.details() = savedSearchGuid;
        QNDEBUG(errorDescription);
        return false;
    }

    return true;
}

bool LocalStorageManagerPrivate::insertOrReplaceNote(const Note & note, const bool updateResources,
                                                     const bool updateTags, ErrorString & errorDescription)
{
    // NOTE: this method expects to be called after the note is already checked
    // for sanity of its parameters!

    ErrorString errorPrefix(QT_TRANSLATE_NOOP("", "can't insert or replace note"));

    Transaction transaction(m_sqlDatabase, *this, Transaction::Exclusive);

    QVariant nullValue;
    QString localUid = sqlEscapeString(note.localUid());
    QString notebookLocalUid = (note.hasNotebookLocalUid() ? sqlEscapeString(note.notebookLocalUid()) : QString());

    // Update common table with Note properties
    {
        bool res = checkAndPrepareInsertOrReplaceNoteQuery();
        QSqlQuery & query = m_insertOrReplaceNoteQuery;
        DATABASE_CHECK_AND_SET_ERROR();

        QString titleNormalized;
        if (note.hasTitle()) {
            titleNormalized = note.title().toLower();
            m_stringUtils.removeDiacritics(titleNormalized);
        }

        query.bindValue(QStringLiteral(":localUid"), localUid);
        query.bindValue(QStringLiteral(":guid"), (note.hasGuid() ? note.guid() : nullValue));
        query.bindValue(QStringLiteral(":updateSequenceNumber"), (note.hasUpdateSequenceNumber() ? note.updateSequenceNumber() : nullValue));
        query.bindValue(QStringLiteral(":isDirty"), (note.isDirty() ? 1 : 0));
        query.bindValue(QStringLiteral(":isLocal"), (note.isLocal() ? 1 : 0));
        query.bindValue(QStringLiteral(":isFavorited"), (note.isFavorited() ? 1 : 0));
        query.bindValue(QStringLiteral(":title"), (note.hasTitle() ? note.title() : nullValue));
        query.bindValue(QStringLiteral(":titleNormalized"), (titleNormalized.isEmpty() ? nullValue : titleNormalized));
        query.bindValue(QStringLiteral(":content"), (note.hasContent() ? note.content() : nullValue));
        query.bindValue(QStringLiteral(":contentContainsUnfinishedToDo"), (note.containsUncheckedTodo() ? 1 : nullValue));
        query.bindValue(QStringLiteral(":contentContainsFinishedToDo"), (note.containsCheckedTodo() ? 1 : nullValue));
        query.bindValue(QStringLiteral(":contentContainsEncryption"), (note.containsEncryption() ? 1 : nullValue));
        query.bindValue(QStringLiteral(":contentLength"), (note.hasContentLength() ? note.contentLength() : nullValue));
        query.bindValue(QStringLiteral(":contentHash"), (note.hasContentHash() ? note.contentHash() : nullValue));

        if (note.hasContent())
        {
            ErrorString error;

            std::pair<QString, QStringList> plainTextAndListOfWords = note.plainTextAndListOfWords(&error);
            if (!error.isEmpty()) {
                errorDescription.base() = errorPrefix.base();
                errorDescription.additionalBases().append(QT_TRANSLATE_NOOP("", "can't get note's plain text and list of words"));
                errorDescription.additionalBases().append(error.base());
                errorDescription.additionalBases().append(error.additionalBases());
                errorDescription.details() = error.details();
                QNWARNING(errorDescription);
                query.finish();
                return false;
            }

            QString listOfWords = plainTextAndListOfWords.second.join(" ");
            m_stringUtils.removePunctuation(listOfWords);
            listOfWords = listOfWords.toLower();
            m_stringUtils.removeDiacritics(listOfWords);

            query.bindValue(QStringLiteral(":contentPlainText"), (plainTextAndListOfWords.first.isEmpty() ? nullValue : plainTextAndListOfWords.first));
            query.bindValue(QStringLiteral(":contentListOfWords"), (listOfWords.isEmpty() ? nullValue : listOfWords));
        }
        else {
            query.bindValue(QStringLiteral(":contentPlainText"), nullValue);
            query.bindValue(QStringLiteral(":contentListOfWords"), nullValue);
        }

        query.bindValue(QStringLiteral(":creationTimestamp"), (note.hasCreationTimestamp() ? note.creationTimestamp() : nullValue));
        query.bindValue(QStringLiteral(":modificationTimestamp"), (note.hasModificationTimestamp() ? note.modificationTimestamp() : nullValue));
        query.bindValue(QStringLiteral(":deletionTimestamp"), (note.hasDeletionTimestamp() ? note.deletionTimestamp() : nullValue));
        query.bindValue(QStringLiteral(":isActive"), (note.hasActive() ? (note.active() ? 1 : 0) : nullValue));
        query.bindValue(QStringLiteral(":hasAttributes"), (note.hasNoteAttributes() ? 1 : 0));

        QImage thumbnail = note.thumbnail();
        bool thumbnailIsNull = thumbnail.isNull();

        query.bindValue(QStringLiteral(":thumbnail"), (thumbnailIsNull ? nullValue : thumbnail));
        query.bindValue(QStringLiteral(":notebookLocalUid"), (notebookLocalUid.isEmpty() ? nullValue : notebookLocalUid));
        query.bindValue(QStringLiteral(":notebookGuid"), (note.hasNotebookGuid() ? note.notebookGuid() : nullValue));

        if (note.hasNoteAttributes())
        {
            const qevercloud::NoteAttributes & attributes = note.noteAttributes();

#define BIND_ATTRIBUTE(name) \
    query.bindValue(QStringLiteral(":" #name), (attributes.name.isSet() ? attributes.name.ref() : nullValue))

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
            BIND_ATTRIBUTE(sharedWithBusiness);
            BIND_ATTRIBUTE(conflictSourceNoteGuid);
            BIND_ATTRIBUTE(noteTitleQuality);

#undef BIND_ATTRIBUTE

            if (attributes.applicationData.isSet())
            {
                const qevercloud::LazyMap & lazyMap = attributes.applicationData.ref();

                if (lazyMap.keysOnly.isSet())
                {
                    const QSet<QString> & keysOnly = lazyMap.keysOnly.ref();
                    QString keysOnlyString;

                    for(auto it = keysOnly.begin(), end = keysOnly.end(); it != end; ++it) {
                        keysOnlyString += QStringLiteral("'");
                        keysOnlyString += *it;
                        keysOnlyString += QStringLiteral("'");
                    }

                    QNDEBUG(QStringLiteral("Application data keys only string: ") << keysOnlyString);

                    query.bindValue(QStringLiteral(":applicationDataKeysOnly"), keysOnlyString);
                }
                else
                {
                    query.bindValue(QStringLiteral(":applicationDataKeysOnly"), nullValue);
                }

                if (lazyMap.fullMap.isSet())
                {
                    const QMap<QString, QString> & fullMap = lazyMap.fullMap.ref();
                    QString fullMapKeysString;
                    QString fullMapValuesString;

                    for(auto it = fullMap.begin(), end = fullMap.end(); it != end; ++it)
                    {
                        fullMapKeysString += QStringLiteral("'");
                        fullMapKeysString += it.key();
                        fullMapKeysString += QStringLiteral("'");

                        fullMapValuesString += QStringLiteral("'");
                        fullMapValuesString += it.value();
                        fullMapValuesString += QStringLiteral("'");
                    }

                    QNDEBUG(QStringLiteral("Application data map keys: ") << fullMapKeysString
                            << QStringLiteral(", application data map values: ") << fullMapValuesString);

                    query.bindValue(QStringLiteral(":applicationDataKeysMap"), fullMapKeysString);
                    query.bindValue(QStringLiteral(":applicationDataValues"), fullMapValuesString);
                }
                else
                {
                    query.bindValue(QStringLiteral(":applicationDataKeysMap"), nullValue);
                    query.bindValue(QStringLiteral(":applicationDataValues"), nullValue);
                }
            }
            else
            {
                query.bindValue(QStringLiteral(":applicationDataKeysOnly"), nullValue);
                query.bindValue(QStringLiteral(":applicationDataKeysMap"), nullValue);
                query.bindValue(QStringLiteral(":applicationDataValues"), nullValue);
            }

            if (attributes.classifications.isSet())
            {
                const QMap<QString, QString> & classifications = attributes.classifications.ref();
                QString classificationKeys, classificationValues;
                for(auto it = classifications.begin(), end = classifications.end(); it != end; ++it)
                {
                    classificationKeys += QStringLiteral("'");
                    classificationKeys += it.key();
                    classificationKeys += QStringLiteral("'");

                    classificationValues += QStringLiteral("'");
                    classificationValues += it.value();
                    classificationValues += QStringLiteral("'");
                }

                QNDEBUG(QStringLiteral("Classification keys: ") << classificationKeys << QStringLiteral(", classification values")
                        << classificationValues);

                query.bindValue(QStringLiteral(":classificationKeys"), classificationKeys);
                query.bindValue(QStringLiteral(":classificationValues"), classificationValues);
            }
            else
            {
                query.bindValue(QStringLiteral(":classificationKeys"), nullValue);
                query.bindValue(QStringLiteral(":classificationValues"), nullValue);
            }
        }
        else
        {
#define BIND_NULL_ATTRIBUTE(name) \
    query.bindValue(QStringLiteral(":" #name), nullValue)

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
            BIND_NULL_ATTRIBUTE(sharedWithBusiness);
            BIND_NULL_ATTRIBUTE(conflictSourceNoteGuid);
            BIND_NULL_ATTRIBUTE(noteTitleQuality);
            BIND_NULL_ATTRIBUTE(applicationDataKeysOnly);
            BIND_NULL_ATTRIBUTE(applicationDataKeysMap);
            BIND_NULL_ATTRIBUTE(applicationDataValues);
            BIND_NULL_ATTRIBUTE(classificationKeys);
            BIND_NULL_ATTRIBUTE(classificationValues);

#undef BIND_NULL_ATTRIBUTE
        }

        res = query.exec();
        DATABASE_CHECK_AND_SET_ERROR();

        query.finish();
    }

    if (note.hasNoteRestrictions())
    {
        const qevercloud::NoteRestrictions & restrictions = note.noteRestrictions();
        bool res = insertOrReplaceNoteRestrictions(localUid, restrictions, errorDescription);
        if (!res) {
            return false;
        }
    }
    else
    {
        QString queryString = QString("DELETE FROM NoteRestrictions WHERE noteLocalUid='%1'").arg(localUid);
        QSqlQuery query(m_sqlDatabase);
        bool res = query.exec(queryString);
        DATABASE_CHECK_AND_SET_ERROR();
    }

    if (note.hasNoteLimits())
    {
        const qevercloud::NoteLimits & limits = note.noteLimits();
        bool res = insertOrReplaceNoteLimits(localUid, limits, errorDescription);
        if (!res) {
            return false;
        }
    }
    else
    {
        QString queryString = QString("DELETE FROM NoteLimits WHERE noteLocalUid='%1'").arg(localUid);
        QSqlQuery query(m_sqlDatabase);
        bool res = query.exec(queryString);
        DATABASE_CHECK_AND_SET_ERROR();
    }

    if (note.hasGuid())
    {
        // Clear shared notes for a given note first, update them (if any) second
        {
            QString noteGuid = sqlEscapeString(note.guid());
            QString queryString = QString("DELETE FROM SharedNotes WHERE sharedNoteNoteGuid='%1'").arg(noteGuid);
            QSqlQuery query(m_sqlDatabase);
            bool res = query.exec(queryString);
            DATABASE_CHECK_AND_SET_ERROR();
        }

        if (note.hasSharedNotes())
        {
            QList<SharedNote> sharedNotes = note.sharedNotes();
            for(auto it = sharedNotes.begin(), end = sharedNotes.end(); it != end; ++it)
            {
                bool res = insertOrReplaceSharedNote(*it, errorDescription);
                if (!res) {
                    return false;
                }
            }
        }
    }

    if (updateTags)
    {
        // Clear note-to-tag binding first, update them second
        {
            QString queryString = QString("DELETE From NoteTags WHERE localNote='%1'").arg(localUid);
            QSqlQuery query(m_sqlDatabase);
            bool res = query.exec(queryString);
            DATABASE_CHECK_AND_SET_ERROR();
        }

        bool hasTagLocalUids = note.hasTagLocalUids();
        bool hasTagGuids = note.hasTagGuids();

        if (hasTagLocalUids || hasTagGuids)
        {
            QStringList tagIds;
            if (hasTagLocalUids) {
                tagIds = note.tagLocalUids();
            }
            else {
                tagIds = note.tagGuids();
            }

            int numTagIds = tagIds.size();

            bool res = checkAndPrepareInsertOrReplaceNoteIntoNoteTagsQuery();
            QSqlQuery & query = m_insertOrReplaceNoteIntoNoteTagsQuery;
            DATABASE_CHECK_AND_SET_ERROR();

            ErrorString error;

            for(int i = 0; i < numTagIds; ++i)
            {
                // NOTE: the behavior expressed here is valid since tags are synchronized before notes
                // so they must exist within local storage database; if they don't then something went really wrong

                const QString & tagId = tagIds[i];

                Tag tag;
                if (hasTagLocalUids) {
                    tag.setLocalUid(tagId);
                }
                else {
                    tag.setGuid(tagId);
                }

                error.clear();
                bool res = findTag(tag, error);
                if (!res) {
                    errorDescription.base() = errorPrefix.base();
                    errorDescription.additionalBases().append(QT_TRANSLATE_NOOP("", "failed to find one of note's tags"));
                    errorDescription.additionalBases().append(error.base());
                    errorDescription.additionalBases().append(error.additionalBases());
                    errorDescription.details() = error.details();
                    QNWARNING(errorDescription);
                    return false;
                }

                query.bindValue(QStringLiteral(":localNote"), localUid);
                query.bindValue(QStringLiteral(":note"), (note.hasGuid() ? note.guid() : nullValue));
                query.bindValue(QStringLiteral(":localTag"), tag.localUid());
                query.bindValue(QStringLiteral(":tag"), (tag.hasGuid() ? tag.guid() : nullValue));
                query.bindValue(QStringLiteral(":tagIndexInNote"), i);

                res = query.exec();
                DATABASE_CHECK_AND_SET_ERROR();

                query.finish();
            }
        }

        // NOTE: don't even attempt fo find tags by their names because qevercloud::Note.tagNames
        // has the only purpose to provide tag names alternatively to guids to NoteStore::createNote method
    }

    if (updateResources)
    {
        if (!note.hasResources())
        {
            // Just clear any resources the note might have had then
            QString queryString = QString("DELETE FROM Resources WHERE noteLocalUid='%1'").arg(localUid);
            QSqlQuery query(m_sqlDatabase);
            bool res = query.exec(queryString);
            DATABASE_CHECK_AND_SET_ERROR();
        }
        else
        {
            bool res = partialUpdateNoteResources(localUid, note.resources(), errorDescription);
            if (!res) {
                return false;
            }
        }
    }

    return transaction.commit(errorDescription);
}

bool LocalStorageManagerPrivate::insertOrReplaceSharedNote(const SharedNote & sharedNote, ErrorString & errorDescription)
{
    QNDEBUG(QStringLiteral("LocalStorageManagerPrivate::insertOrReplaceSharedNote: ") << sharedNote);

    // NOTE: this method expects to be called after the shared note is already checked
    // for sanity of its parameters!

    ErrorString errorPrefix(QT_TRANSLATE_NOOP("", "can't insert or replace shared note"));

    bool res = checkAndPrepareInsertOrReplaceSharedNoteQuery();
    QSqlQuery & query = m_insertOrReplaceSharedNoteQuery;
    DATABASE_CHECK_AND_SET_ERROR();

    QVariant nullValue;

    query.bindValue(QStringLiteral(":sharedNoteNoteGuid"), sharedNote.noteGuid());
    query.bindValue(QStringLiteral(":sharedNoteSharerUserId"), (sharedNote.hasSharerUserId() ? sharedNote.sharerUserId() : nullValue));
    query.bindValue(QStringLiteral(":sharedNoteRecipientIdentityId"), (sharedNote.hasRecipientIdentityId()
                                                                       ? sharedNote.recipientIdentityId()
                                                                       : nullValue));

    query.bindValue(QStringLiteral(":sharedNoteRecipientContactName"), (sharedNote.hasRecipientIdentityContactName()
                                                                        ? sharedNote.recipientIdentityContactName()
                                                                        : nullValue));
    query.bindValue(QStringLiteral(":sharedNoteRecipientContactId"), (sharedNote.hasRecipientIdentityContactId()
                                                                      ? sharedNote.recipientIdentityContactId()
                                                                      : nullValue));
    query.bindValue(QStringLiteral(":sharedNoteRecipientContactType"), (sharedNote.hasRecipientIdentityContactType()
                                                                        ? sharedNote.recipientIdentityContactType()
                                                                        : nullValue));
    query.bindValue(QStringLiteral(":sharedNoteRecipientContactPhotoUrl"), (sharedNote.hasRecipientIdentityContactPhotoUrl()
                                                                            ? sharedNote.recipientIdentityContactPhotoUrl()
                                                                            : nullValue));
    query.bindValue(QStringLiteral(":sharedNoteRecipientContactPhotoLastUpdated"), (sharedNote.hasRecipientIdentityContactPhotoLastUpdated()
                                                                                    ? sharedNote.recipientIdentityContactPhotoLastUpdated()
                                                                                    : nullValue));
    query.bindValue(QStringLiteral(":sharedNoteRecipientContactMessagingPermit"), (sharedNote.hasRecipientIdentityContactMessagingPermit()
                                                                                   ? sharedNote.recipientIdentityContactMessagingPermit()
                                                                                   : nullValue));
    query.bindValue(QStringLiteral(":sharedNoteRecipientContactMessagingPermitExpires"), (sharedNote.hasRecipientIdentityContactMessagingPermitExpires()
                                                                                          ? sharedNote.recipientIdentityContactMessagingPermitExpires()
                                                                                          : nullValue));

    query.bindValue(QStringLiteral(":sharedNoteRecipientUserId"), (sharedNote.hasRecipientIdentityUserId() ? sharedNote.recipientIdentityUserId() : nullValue));
    query.bindValue(QStringLiteral(":sharedNoteRecipientDeactivated"), (sharedNote.hasRecipientIdentityDeactivated()
                                                                        ? (sharedNote.recipientIdentityDeactivated() ? 1 : 0)
                                                                        : nullValue));
    query.bindValue(QStringLiteral(":sharedNoteRecipientSameBusiness"), (sharedNote.hasRecipientIdentitySameBusiness()
                                                                         ? (sharedNote.recipientIdentitySameBusiness() ? 1 : 0)
                                                                         : nullValue));
    query.bindValue(QStringLiteral(":sharedNoteRecipientBlocked"), (sharedNote.hasRecipientIdentityBlocked()
                                                                    ? (sharedNote.recipientIdentityBlocked() ? 1 : 0)
                                                                    : nullValue));
    query.bindValue(QStringLiteral(":sharedNoteRecipientUserConnected"), (sharedNote.hasRecipientIdentityUserConnected()
                                                                          ? (sharedNote.recipientIdentityUserConnected() ? 1 : 0)
                                                                          : nullValue));
    query.bindValue(QStringLiteral(":sharedNoteRecipientEventId"), (sharedNote.hasRecipientIdentityEventId() ? sharedNote.recipientIdentityEventId() : nullValue));

    query.bindValue(QStringLiteral(":sharedNotePrivilegeLevel"), (sharedNote.hasPrivilegeLevel() ? sharedNote.privilegeLevel() : nullValue));
    query.bindValue(QStringLiteral(":sharedNoteCreationTimestamp"), (sharedNote.hasCreationTimestamp() ? sharedNote.creationTimestamp() : nullValue));
    query.bindValue(QStringLiteral(":sharedNoteModificationTimestamp"), (sharedNote.hasModificationTimestamp() ? sharedNote.modificationTimestamp() : nullValue));
    query.bindValue(QStringLiteral(":sharedNoteAssignmentTimestamp"), (sharedNote.hasAssignmentTimestamp() ? sharedNote.assignmentTimestamp() : nullValue));

    res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR();

    query.finish();
    return true;
}

bool LocalStorageManagerPrivate::insertOrReplaceNoteRestrictions(const QString & noteLocalUid,
                                                                 const qevercloud::NoteRestrictions & noteRestrictions,
                                                                 ErrorString & errorDescription)
{
    ErrorString errorPrefix(QT_TRANSLATE_NOOP("", "can't insert or replace note restrictions"));

    bool res = checkAndPrepareInsertOrReplaceNoteRestrictionsQuery();
    QSqlQuery & query = m_insertOrReplaceNoteRestrictionsQuery;
    DATABASE_CHECK_AND_SET_ERROR();

    QVariant nullValue;

    query.bindValue(QStringLiteral(":noteLocalUid"), noteLocalUid);

#define BIND_RESTRICTION(column, name) \
    query.bindValue(QStringLiteral(":" #column), (noteRestrictions.name.isSet() ? (noteRestrictions.name.ref() ? 1 : 0) : nullValue))

    BIND_RESTRICTION(noUpdateNoteTitle, noUpdateTitle);
    BIND_RESTRICTION(noUpdateNoteContent, noUpdateContent);
    BIND_RESTRICTION(noEmailNote, noEmail);
    BIND_RESTRICTION(noShareNote, noShare);
    BIND_RESTRICTION(noShareNotePublicly, noSharePublicly);

#undef BIND_RESTRICTION

    res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR();

    query.finish();
    return true;
}

bool LocalStorageManagerPrivate::insertOrReplaceNoteLimits(const QString & noteLocalUid, const qevercloud::NoteLimits & noteLimits,
                                                           ErrorString & errorDescription)
{
    ErrorString errorPrefix(QT_TRANSLATE_NOOP("", "can't insert or replace note limits"));

    bool res = checkAndPrepareInsertOrReplaceNoteLimitsQuery();
    QSqlQuery & query = m_insertOrReplaceNoteLimitsQuery;
    DATABASE_CHECK_AND_SET_ERROR();

    QVariant nullValue;

    query.bindValue(QStringLiteral(":noteLocalUid"), noteLocalUid);

#define BIND_LIMIT(limit) \
    query.bindValue(QStringLiteral(":" #limit), (noteLimits.limit.isSet() ? (noteLimits.limit.ref() ? 1 : 0) : nullValue))

    BIND_LIMIT(noteResourceCountMax);
    BIND_LIMIT(uploadLimit);
    BIND_LIMIT(resourceSizeMax);
    BIND_LIMIT(noteSizeMax);
    BIND_LIMIT(uploaded);

#undef BIND_LIMIT

    res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR();

    query.finish();
    return true;
}

bool LocalStorageManagerPrivate::canAddNoteToNotebook(const QString & notebookLocalUid, ErrorString & errorDescription)
{
    QNDEBUG(QStringLiteral("LocalStorageManagerPrivate::canAddNoteToNotebook: notebook local uid = ") << notebookLocalUid);

    ErrorString errorPrefix(QT_TRANSLATE_NOOP("", "can't check whether some note can be added to the notebook"));

    bool res = checkAndPrepareCanAddNoteToNotebookQuery();
    QSqlQuery & query = m_canAddNoteToNotebookQuery;
    DATABASE_CHECK_AND_SET_ERROR();

    query.bindValue(QStringLiteral(":notebookLocalUid"), notebookLocalUid);
    res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR();

    if (!query.next()) {
        QNDEBUG(QStringLiteral("Found no notebook restrictions for notebook with local uid ") << notebookLocalUid
                << QStringLiteral(", assuming it's possible to add the note to this notebook"));
        query.finish();
        return true;
    }

    res = !(query.value(0).toBool());
    if (!res) {
        errorDescription.base() = errorPrefix.base();
        errorDescription.additionalBases().append(QT_TRANSLATE_NOOP("", "notebook restrictions forbid adding the note to it"));
    }

    query.finish();
    return res;
}

bool LocalStorageManagerPrivate::canUpdateNoteInNotebook(const QString & notebookLocalUid, ErrorString & errorDescription)
{
    QNDEBUG(QStringLiteral("LocalStorageManagerPrivate::canUpdateNoteInNotebook: notebook local uid = ") << notebookLocalUid);

    ErrorString errorPrefix(QT_TRANSLATE_NOOP("", "can't check whether some note in the notebook can be updated"));

    bool res = checkAndPrepareCanUpdateNoteInNotebookQuery();
    QSqlQuery & query = m_canUpdateNoteInNotebookQuery;
    DATABASE_CHECK_AND_SET_ERROR();

    query.bindValue(QStringLiteral(":notebookLocalUid"), notebookLocalUid);
    res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR();

    if (!query.next()) {
        QNDEBUG(QStringLiteral("Found no notebook restrictions for notebook with local uid ") << notebookLocalUid
                << QStringLiteral(", assuming it's possible to update the note in this notebook"));
        query.finish();
        return true;
    }

    res = !(query.value(0).toBool());
    if (!res) {
        errorDescription.base() = QT_TRANSLATE_NOOP("", "notebook restrictions forbid updating its notes");
    }

    query.finish();
    return res;
}

bool LocalStorageManagerPrivate::canExpungeNoteInNotebook(const QString & notebookLocalUid, ErrorString & errorDescription)
{
    QNDEBUG(QStringLiteral("LocalStorageManagerPrivate::canExpungeNoteInNotebook: notebook local uid = ") << notebookLocalUid);

    ErrorString errorPrefix(QT_TRANSLATE_NOOP("", "can't check whether some note can be expunged from the notebook"));

    bool res = checkAndPrepareCanExpungeNoteInNotebookQuery();
    QSqlQuery & query = m_canExpungeNoteInNotebookQuery;
    DATABASE_CHECK_AND_SET_ERROR();

    query.bindValue(QStringLiteral(":notebookLocalUid"), notebookLocalUid);
    res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR();

    if (!query.next()) {
        QNDEBUG(QStringLiteral("Found no notebook restrictions for notebook with local uid ") << notebookLocalUid
                << QStringLiteral(", assuming it's possible to expunge the note from this notebook"));
        query.finish();
        return true;
    }

    res = !(query.value(0).toBool());
    if (!res) {
        errorDescription.base() = QT_TRANSLATE_NOOP("", "notebook restrictions forbid expunging its notes");
    }

    query.finish();
    return res;
}

bool LocalStorageManagerPrivate::checkAndPrepareNoteCountQuery() const
{
    if (Q_LIKELY(m_getNoteCountQueryPrepared)) {
        return true;
    }

    QNTRACE(QStringLiteral("Preparing SQL query to get the count of notes"));

    m_getNoteCountQuery = QSqlQuery(m_sqlDatabase);
    bool res = m_getNoteCountQuery.prepare(QStringLiteral("SELECT COUNT(*) FROM Notes WHERE deletionTimestamp IS NULL"));
    if (res) {
        m_getNoteCountQueryPrepared = true;
    }

    return res;
}

bool LocalStorageManagerPrivate::checkAndPrepareInsertOrReplaceNoteQuery()
{
    if (Q_LIKELY(m_insertOrReplaceNoteQueryPrepared)) {
        return true;
    }

    QNTRACE(QStringLiteral("Preparing SQL query to insert or replace note"));

    m_insertOrReplaceNoteQuery = QSqlQuery(m_sqlDatabase);

    QString columns = QStringLiteral("localUid, guid, updateSequenceNumber, isDirty, isLocal, "
                                     "isFavorited, title, titleNormalized, content, contentLength, contentHash, "
                                     "contentPlainText, contentListOfWords, contentContainsFinishedToDo, "
                                     "contentContainsUnfinishedToDo, contentContainsEncryption, creationTimestamp, "
                                     "modificationTimestamp, deletionTimestamp, isActive, hasAttributes, "
                                     "thumbnail, notebookLocalUid, notebookGuid, subjectDate, latitude, "
                                     "longitude, altitude, author, source, sourceURL, sourceApplication, "
                                     "shareDate, reminderOrder, reminderDoneTime, reminderTime, placeName, "
                                     "contentClass, lastEditedBy, creatorId, lastEditorId, "
                                     "sharedWithBusiness, conflictSourceNoteGuid, noteTitleQuality, "
                                     "applicationDataKeysOnly, applicationDataKeysMap, "
                                     "applicationDataValues, classificationKeys, classificationValues");

    QString values = QStringLiteral(":localUid, :guid, :updateSequenceNumber, :isDirty, :isLocal, "
                                    ":isFavorited, :title, :titleNormalized, :content, :contentLength, :contentHash, "
                                    ":contentPlainText, :contentListOfWords, :contentContainsFinishedToDo, "
                                    ":contentContainsUnfinishedToDo, :contentContainsEncryption, :creationTimestamp, "
                                    ":modificationTimestamp, :deletionTimestamp, :isActive, :hasAttributes, "
                                    ":thumbnail, :notebookLocalUid, :notebookGuid, :subjectDate, :latitude, "
                                    ":longitude, :altitude, :author, :source, :sourceURL, :sourceApplication, "
                                    ":shareDate, :reminderOrder, :reminderDoneTime, :reminderTime, :placeName, "
                                    ":contentClass, :lastEditedBy, :creatorId, :lastEditorId, "
                                    ":sharedWithBusiness, :conflictSourceNoteGuid, :noteTitleQuality, "
                                    ":applicationDataKeysOnly, :applicationDataKeysMap, "
                                    ":applicationDataValues, :classificationKeys, :classificationValues");

    QString queryString = QString("INSERT OR REPLACE INTO Notes(%1) VALUES(%2)").arg(columns,values);

    bool res = m_insertOrReplaceNoteQuery.prepare(queryString);
    if (res) {
        m_insertOrReplaceNoteQueryPrepared = true;
    }

    return res;
}

bool LocalStorageManagerPrivate::checkAndPrepareInsertOrReplaceSharedNoteQuery()
{
    if (Q_LIKELY(m_insertOrReplaceSharedNoteQueryPrepared)) {
        return true;
    }

    QNTRACE(QStringLiteral("Preparing SQL query to insert or replace the shared note"));

    m_insertOrReplaceSharedNoteQuery = QSqlQuery(m_sqlDatabase);
    bool res = m_insertOrReplaceSharedNoteQuery.prepare(QStringLiteral("INSERT OR REPLACE INTO SharedNotes "
                                                                       "(sharedNoteNoteGuid, sharedNoteSharerUserId, "
                                                                       "sharedNoteRecipientIdentity, sharedNotePrivilegeLevel, "
                                                                       "sharedNoteCreationTimestamp, sharedNoteModificationTimestamp, "
                                                                       "sharedNoteAssignmentTimestamp) "
                                                                       "VALUES(:sharedNoteNoteGuid, :sharedNoteSharerUserId, "
                                                                       ":sharedNoteRecipientIdentity, :sharedNotePrivilegeLevel, "
                                                                       ":sharedNoteCreationTimestamp, :sharedNoteModificationTimestamp, "
                                                                       ":sharedNoteAssignmentTimestamp)"));
    if (res) {
        m_insertOrReplaceSharedNoteQueryPrepared = true;
    }

    return res;
}

bool LocalStorageManagerPrivate::checkAndPrepareInsertOrReplaceNoteRestrictionsQuery()
{
    if (Q_LIKELY(m_insertOrReplaceNoteRestrictionsQueryPrepared)) {
        return true;
    }

    QNTRACE(QStringLiteral("Preparing SQL query to insert or replace note restrictions"));

    m_insertOrReplaceNoteRestrictionsQuery = QSqlQuery(m_sqlDatabase);
    bool res = m_insertOrReplaceNoteRestrictionsQuery.prepare(QStringLiteral("INSERT OR REPLACE INTO NoteRestrictions "
                                                                             "(noteLocalUid, noUpdateNoteTitle, noUpdateNoteContent, "
                                                                             "noEmailNote, noShareNote, noShareNotePublicly) "
                                                                             "VALUES(:noteLocalUid, :noUpdateNoteTitle, "
                                                                             ":noUpdateNoteContent, :noEmailNote, "
                                                                             ":noShareNote, :noShareNotePublicly)"));
    if (res) {
        m_insertOrReplaceNoteRestrictionsQueryPrepared = true;
    }

    return res;
}

bool LocalStorageManagerPrivate::checkAndPrepareInsertOrReplaceNoteLimitsQuery()
{
    if (Q_LIKELY(m_insertOrReplaceNoteLimitsQueryPrepared)) {
        return true;
    }

    QNTRACE(QStringLiteral("Preparing SQL query to insert or replace note limits"));

    m_insertOrReplaceNoteLimitsQuery = QSqlQuery(m_sqlDatabase);
    bool res = m_insertOrReplaceNoteLimitsQuery.prepare(QStringLiteral("INSERT OR REPLACE INTO NoteLimits "
                                                                       "(noteLocalUid, noteResourceCountMax, uploadLimit, "
                                                                       "resourceSizeMax, noteSizeMax, uploaded) "
                                                                       "VALUES(:noteLocalUid, :noteResourceCountMax, :uploadLimit, "
                                                                       ":resourceSizeMax, :noteSizeMax, :uploaded)"));
    if (res) {
        m_insertOrReplaceNoteLimitsQueryPrepared = true;
    }

    return res;
}

bool LocalStorageManagerPrivate::checkAndPrepareCanAddNoteToNotebookQuery() const
{
    if (Q_LIKELY(m_canAddNoteToNotebookQueryPrepared)) {
        return true;
    }

    QNTRACE(QStringLiteral("Preparing SQL query to get the noCreateNotes notebook restriction"));

    m_canAddNoteToNotebookQuery = QSqlQuery(m_sqlDatabase);
    bool res = m_canAddNoteToNotebookQuery.prepare(QStringLiteral("SELECT noCreateNotes FROM NotebookRestrictions "
                                                                  "WHERE localUid = :notebookLocalUid"));
    if (res) {
        m_canAddNoteToNotebookQueryPrepared = true;
    }

    return res;
}

bool LocalStorageManagerPrivate::checkAndPrepareCanUpdateNoteInNotebookQuery() const
{
    if (Q_LIKELY(m_canUpdateNoteInNotebookQueryPrepared)) {
        return true;
    }

    QNTRACE(QStringLiteral("Preparing SQL query to get the noUpdateNotes notebook restriction"));

    m_canUpdateNoteInNotebookQuery = QSqlQuery(m_sqlDatabase);
    bool res = m_canUpdateNoteInNotebookQuery.prepare(QStringLiteral("SELECT noUpdateNotes FROM NotebookRestrictions "
                                                                     "WHERE localUid = :notebookLocalUid"));
    if (res) {
        m_canUpdateNoteInNotebookQueryPrepared = true;
    }

    return true;
}

bool LocalStorageManagerPrivate::checkAndPrepareCanExpungeNoteInNotebookQuery() const
{
    if (Q_LIKELY(m_canExpungeNoteInNotebookQueryPrepared)) {
        return true;
    }

    QNTRACE(QStringLiteral("Preparing SQL query to get the noExpungeNotes notebook restriction"));

    m_canExpungeNoteInNotebookQuery = QSqlQuery(m_sqlDatabase);
    bool res = m_canExpungeNoteInNotebookQuery.prepare(QStringLiteral("SELECT noExpungeNotes FROM NotebookRestrictions "
                                                                      "WHERE localUid = :notebookLocalUid"));
    if (res) {
        m_canExpungeNoteInNotebookQueryPrepared = true;
    }

    return res;
}

bool LocalStorageManagerPrivate::checkAndPrepareInsertOrReplaceNoteIntoNoteTagsQuery()
{
    if (Q_LIKELY(m_insertOrReplaceNoteIntoNoteTagsQueryPrepared)) {
        return true;
    }

    QNDEBUG(QStringLiteral("Preparing SQL query to insert or replace note into NoteTags table"));

    m_insertOrReplaceNoteIntoNoteTagsQuery = QSqlQuery(m_sqlDatabase);
    bool res = m_insertOrReplaceNoteIntoNoteTagsQuery.prepare(QStringLiteral("INSERT OR REPLACE INTO NoteTags"
                                                                             "(localNote, note, localTag, tag, tagIndexInNote) "
                                                                             "VALUES(:localNote, :note, :localTag, :tag, :tagIndexInNote)"));
    if (res) {
        m_insertOrReplaceNoteIntoNoteTagsQueryPrepared = true;
    }

    return res;
}

bool LocalStorageManagerPrivate::insertOrReplaceTag(const Tag & tag, ErrorString & errorDescription)
{
    // NOTE: this method expects to be called after tag is already checked
    // for sanity of its parameters!

    ErrorString errorPrefix(QT_TRANSLATE_NOOP("", "can't insert or replace tag into the local storage database"));

    QString localUid = tag.localUid();

    bool res = checkAndPrepareInsertOrReplaceTagQuery();
    QSqlQuery & query = m_insertOrReplaceTagQuery;
    DATABASE_CHECK_AND_SET_ERROR();

    QVariant nullValue;

    QString tagNameNormalized;
    if (tag.hasName()) {
        tagNameNormalized = tag.name().toLower();
        m_stringUtils.removeDiacritics(tagNameNormalized);
    }

    query.bindValue(QStringLiteral(":localUid"), (localUid.isEmpty() ? nullValue : localUid));
    query.bindValue(QStringLiteral(":guid"), (tag.hasGuid() ? tag.guid() : nullValue));
    query.bindValue(QStringLiteral(":linkedNotebookGuid"), tag.hasLinkedNotebookGuid() ? tag.linkedNotebookGuid() : nullValue);
    query.bindValue(QStringLiteral(":updateSequenceNumber"), (tag.hasUpdateSequenceNumber() ? tag.updateSequenceNumber() : nullValue));
    query.bindValue(QStringLiteral(":name"), (tag.hasName() ? tag.name() : nullValue));
    query.bindValue(QStringLiteral(":nameLower"), (tag.hasName() ? tagNameNormalized : nullValue));
    query.bindValue(QStringLiteral(":parentGuid"), (tag.hasParentGuid() ? tag.parentGuid() : nullValue));
    query.bindValue(QStringLiteral(":parentLocalUid"), (tag.hasParentLocalUid() ? tag.parentLocalUid() : nullValue));
    query.bindValue(QStringLiteral(":isDirty"), (tag.isDirty() ? 1 : 0));
    query.bindValue(QStringLiteral(":isLocal"), (tag.isLocal() ? 1 : 0));
    query.bindValue(QStringLiteral(":isFavorited"), (tag.isFavorited() ? 1 : 0));

    res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR();

    query.finish();
    return true;
}

bool LocalStorageManagerPrivate::checkAndPrepareTagCountQuery() const
{
    if (Q_LIKELY(m_getTagCountQueryPrepared)) {
        return true;
    }

    m_getTagCountQuery = QSqlQuery(m_sqlDatabase);
    bool res = m_getTagCountQuery.prepare(QStringLiteral("SELECT COUNT(*) FROM Tags"));
    if (res) {
        m_getTagCountQueryPrepared = true;
    }

    return res;
}

bool LocalStorageManagerPrivate::checkAndPrepareInsertOrReplaceTagQuery()
{
    if (Q_LIKELY(m_insertOrReplaceTagQueryPrepared)) {
        return true;
    }

    m_insertOrReplaceTagQuery = QSqlQuery(m_sqlDatabase);
    bool res = m_insertOrReplaceTagQuery.prepare(QStringLiteral("INSERT OR REPLACE INTO Tags "
                                                                "(localUid, guid, linkedNotebookGuid, updateSequenceNumber, "
                                                                "name, nameLower, parentGuid, parentLocalUid, isDirty, "
                                                                "isLocal, isFavorited) "
                                                                "VALUES(:localUid, :guid, :linkedNotebookGuid, "
                                                                ":updateSequenceNumber, :name, :nameLower, "
                                                                ":parentGuid, :parentLocalUid, :isDirty, :isLocal, "
                                                                ":isFavorited)"));
    if (res) {
        m_insertOrReplaceTagQueryPrepared = true;
    }

    return res;
}

bool LocalStorageManagerPrivate::insertOrReplaceResource(const Resource & resource, ErrorString & errorDescription,
                                                         const bool useSeparateTransaction)
{
    // NOTE: this method expects to be called after resource is already checked
    // for sanity of its parameters!

    ErrorString errorPrefix(QT_TRANSLATE_NOOP("", "can't insert or replace resource into the local storage database"));

    QScopedPointer<Transaction> pTransaction;
    if (useSeparateTransaction) {
        pTransaction.reset(new Transaction(m_sqlDatabase, *this, Transaction::Exclusive));
    }

    QVariant nullValue;
    QString resourceLocalUid = resource.localUid();
    QString noteLocalUid = resource.noteLocalUid();

    // Updating common resource information in Resources table
    {
        bool res = checkAndPrepareInsertOrReplaceResourceQuery();
        QSqlQuery & query = m_insertOrReplaceResourceQuery;
        DATABASE_CHECK_AND_SET_ERROR();

        query.bindValue(QStringLiteral(":resourceGuid"), (resource.hasGuid() ? resource.guid() : nullValue));
        query.bindValue(QStringLiteral(":noteGuid"), (resource.hasNoteGuid() ? resource.noteGuid() : nullValue));
        query.bindValue(QStringLiteral(":noteLocalUid"), noteLocalUid);
        query.bindValue(QStringLiteral(":dataBody"), (resource.hasDataBody() ? resource.dataBody() : nullValue));
        query.bindValue(QStringLiteral(":dataSize"), (resource.hasDataSize() ? resource.dataSize() : nullValue));
        query.bindValue(QStringLiteral(":dataHash"), (resource.hasDataHash() ? resource.dataHash() : nullValue));
        query.bindValue(QStringLiteral(":mime"), (resource.hasMime() ? resource.mime() : nullValue));
        query.bindValue(QStringLiteral(":width"), (resource.hasWidth() ? resource.width() : nullValue));
        query.bindValue(QStringLiteral(":height"), (resource.hasHeight() ? resource.height() : nullValue));
        query.bindValue(QStringLiteral(":recognitionDataBody"), (resource.hasRecognitionDataBody() ? resource.recognitionDataBody() : nullValue));
        query.bindValue(QStringLiteral(":recognitionDataSize"), (resource.hasRecognitionDataSize() ? resource.recognitionDataSize() : nullValue));
        query.bindValue(QStringLiteral(":recognitionDataHash"), (resource.hasRecognitionDataHash() ? resource.recognitionDataHash() : nullValue));
        query.bindValue(QStringLiteral(":alternateDataBody"), (resource.hasAlternateDataBody() ? resource.alternateDataBody() : nullValue));
        query.bindValue(QStringLiteral(":alternateDataSize"), (resource.hasAlternateDataSize() ? resource.alternateDataSize() : nullValue));
        query.bindValue(QStringLiteral(":alternateDataHash"), (resource.hasAlternateDataHash() ? resource.alternateDataHash() : nullValue));
        query.bindValue(QStringLiteral(":resourceUpdateSequenceNumber"), (resource.hasUpdateSequenceNumber() ? resource.updateSequenceNumber() : nullValue));
        query.bindValue(QStringLiteral(":resourceIsDirty"), (resource.isDirty() ? 1 : 0));
        query.bindValue(QStringLiteral(":resourceIndexInNote"), resource.indexInNote());
        query.bindValue(QStringLiteral(":resourceLocalUid"), resourceLocalUid);

        res = query.exec();
        DATABASE_CHECK_AND_SET_ERROR();

        query.finish();
    }

    // Updating connections between note and resource in NoteResources table
    {
        bool res = checkAndPrepareInsertOrReplaceNoteResourceQuery();
        QSqlQuery & query = m_insertOrReplaceNoteResourceQuery;
        DATABASE_CHECK_AND_SET_ERROR();

        query.bindValue(QStringLiteral(":localNote"), noteLocalUid);
        query.bindValue(QStringLiteral(":note"), (resource.hasNoteGuid() ? resource.noteGuid() : nullValue));
        query.bindValue(QStringLiteral(":localResource"), resourceLocalUid);
        query.bindValue(QStringLiteral(":resource"), (resource.hasGuid() ? resource.guid() : nullValue));

        res = query.exec();
        DATABASE_CHECK_AND_SET_ERROR();

        query.finish();
    }

    // Removing resource's local uid from ResourceRecognitionData table
    {
        bool res = checkAndPrepareDeleteResourceFromResourceRecognitionTypesQuery();
        QSqlQuery & query = m_deleteResourceFromResourceRecognitionTypesQuery;
        DATABASE_CHECK_AND_SET_ERROR();

        query.bindValue(QStringLiteral(":resourceLocalUid"), resourceLocalUid);

        res = query.exec();
        DATABASE_CHECK_AND_SET_ERROR();

        query.finish();
    }

    if (resource.hasRecognitionDataBody())
    {
        ResourceRecognitionIndices recoIndices;
        bool res = recoIndices.setData(resource.recognitionDataBody());
        if (res && recoIndices.isValid())
        {
            QString recognitionData;

            QVector<ResourceRecognitionIndexItem> items = recoIndices.items();
            const int numItems = items.size();
            for(int i = 0; i < numItems; ++i)
            {
                const ResourceRecognitionIndexItem & item = items[i];

                QVector<ResourceRecognitionIndexItem::TextItem> textItems = item.textItems();
                const int numTextItems = textItems.size();
                for(int j = 0; j < numTextItems; ++j)
                {
                    const ResourceRecognitionIndexItem::TextItem & textItem = textItems[j];
                    recognitionData += textItem.m_text + QStringLiteral(" ");
                }
            }

            recognitionData.chop(1);    // Remove trailing whitespace
            m_stringUtils.removePunctuation(recognitionData);
            m_stringUtils.removeDiacritics(recognitionData);

            if (!recognitionData.isEmpty())
            {
                bool res = checkAndPrepareInsertOrReplaceIntoResourceRecognitionDataQuery();
                QSqlQuery & query = m_insertOrReplaceIntoResourceRecognitionDataQuery;
                DATABASE_CHECK_AND_SET_ERROR();

                query.bindValue(QStringLiteral(":resourceLocalUid"), resourceLocalUid);
                query.bindValue(QStringLiteral(":noteLocalUid"), noteLocalUid);
                query.bindValue(QStringLiteral(":recognitionData"), recognitionData);

                res = query.exec();
                DATABASE_CHECK_AND_SET_ERROR();

                query.finish();
            }
        }
    }

    // Removing resource from ResourceAttributes table
    {
        bool res = checkAndPrepareDeleteResourceFromResourceAttributesQuery();
        QSqlQuery & query = m_deleteResourceFromResourceAttributesQuery;
        DATABASE_CHECK_AND_SET_ERROR();

        query.bindValue(QStringLiteral(":resourceLocalUid"), resourceLocalUid);

        res = query.exec();
        DATABASE_CHECK_AND_SET_ERROR();

        query.finish();
    }

    // Removing resource from ResourceAttributesApplicationDataKeysOnly table
    {
        bool res = checkAndPrepareDeleteResourceFromResourceAttributesApplicationDataKeysOnlyQuery();
        QSqlQuery & query = m_deleteResourceFromResourceAttributesApplicationDataKeysOnlyQuery;
        DATABASE_CHECK_AND_SET_ERROR();

        query.bindValue(QStringLiteral(":resourceLocalUid"), resourceLocalUid);

        res = query.exec();
        DATABASE_CHECK_AND_SET_ERROR();

        query.finish();
    }

    // Removing resource from ResourceAttributesApplicationDataFullMap table
    {
        bool res = checkAndPrepareDeleteResourceFromResourceAttributesApplicationDataFullMapQuery();
        QSqlQuery & query = m_deleteResourceFromResourceAttributesApplicationDataFullMapQuery;
        DATABASE_CHECK_AND_SET_ERROR();

        query.bindValue(QStringLiteral(":resourceLocalUid"), resourceLocalUid);

        res = query.exec();
        DATABASE_CHECK_AND_SET_ERROR();

        query.finish();
    }

    if (resource.hasResourceAttributes())
    {
        const qevercloud::ResourceAttributes & attributes = resource.resourceAttributes();
        bool res = insertOrReplaceResourceAttributes(resourceLocalUid, attributes, errorDescription);
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

bool LocalStorageManagerPrivate::insertOrReplaceResourceAttributes(const QString & localUid,
                                                                   const qevercloud::ResourceAttributes & attributes,
                                                                   ErrorString & errorDescription)
{
    ErrorString errorPrefix(QT_TRANSLATE_NOOP("", "can't insert or replace resource attributes"));

    QVariant nullValue;

    // Insert or replace attributes into ResourceAttributes table
    {
        bool res = checkAndPrepareInsertOrReplaceResourceAttributesQuery();
        QSqlQuery & query = m_insertOrReplaceResourceAttributesQuery;
        DATABASE_CHECK_AND_SET_ERROR();

        query.bindValue(QStringLiteral(":resourceLocalUid"), localUid);
        query.bindValue(QStringLiteral(":resourceSourceURL"), (attributes.sourceURL.isSet() ? attributes.sourceURL.ref() : nullValue));
        query.bindValue(QStringLiteral(":timestamp"), (attributes.timestamp.isSet() ? attributes.timestamp.ref() : nullValue));
        query.bindValue(QStringLiteral(":resourceLatitude"), (attributes.latitude.isSet() ? attributes.latitude.ref() : nullValue));
        query.bindValue(QStringLiteral(":resourceLongitude"), (attributes.longitude.isSet() ? attributes.longitude.ref() : nullValue));
        query.bindValue(QStringLiteral(":resourceAltitude"), (attributes.altitude.isSet() ? attributes.altitude.ref() : nullValue));
        query.bindValue(QStringLiteral(":cameraMake"), (attributes.cameraMake.isSet() ? attributes.cameraMake.ref() : nullValue));
        query.bindValue(QStringLiteral(":cameraModel"), (attributes.cameraModel.isSet() ? attributes.cameraModel.ref() : nullValue));
        query.bindValue(QStringLiteral(":clientWillIndex"), (attributes.clientWillIndex.isSet() ? (attributes.clientWillIndex.ref() ? 1 : 0) : nullValue));
        query.bindValue(QStringLiteral(":fileName"), (attributes.fileName.isSet() ? attributes.fileName.ref() : nullValue));
        query.bindValue(QStringLiteral(":attachment"), (attributes.attachment.isSet() ? (attributes.attachment.ref() ? 1 : 0) : nullValue));

        res = query.exec();
        DATABASE_CHECK_AND_SET_ERROR();

        query.finish();
    }

    // Special treatment for ResourceAttributes.applicationData: keysOnly + fullMap

    if (attributes.applicationData.isSet())
    {
        if (attributes.applicationData->keysOnly.isSet())
        {
            bool res = checkAndPrepareInsertOrReplaceResourceAttributesApplicationDataKeysOnlyQuery();
            QSqlQuery & query = m_insertOrReplaceResourceAttributeApplicationDataKeysOnlyQuery;
            DATABASE_CHECK_AND_SET_ERROR();

            query.bindValue(QStringLiteral(":resourceLocalUid"), localUid);

            const QSet<QString> & keysOnly = attributes.applicationData->keysOnly.ref();
            for(auto it = keysOnly.begin(), end = keysOnly.end(); it != end; ++it) {
                const QString & key = *it;
                query.bindValue(QStringLiteral(":resourceKey"), key);
                res = query.exec();
                DATABASE_CHECK_AND_SET_ERROR();
                query.finish();
            }
        }

        if (attributes.applicationData->fullMap.isSet())
        {
            bool res = checkAndPrepareInsertOrReplaceResourceAttributesApplicationDataFullMapQuery();
            QSqlQuery & query = m_insertOrReplaceResourceAttributeApplicationDataFullMapQuery;
            DATABASE_CHECK_AND_SET_ERROR();

            query.bindValue(QStringLiteral(":resourceLocalUid"), localUid);

            const QMap<QString, QString> & fullMap = attributes.applicationData->fullMap.ref();
            for(auto it = fullMap.begin(), end = fullMap.end(); it != end; ++it) {
                const QString & key = it.key();
                const QString & value = it.value();
                query.bindValue(QStringLiteral(":resourceMapKey"), key);
                query.bindValue(QStringLiteral(":resourceValue"), value);
                res = query.exec();
                DATABASE_CHECK_AND_SET_ERROR();
                query.finish();
            }
        }
    }

    return true;
}

bool LocalStorageManagerPrivate::checkAndPrepareInsertOrReplaceResourceQuery()
{
    if (Q_LIKELY(m_insertOrReplaceResourceQueryPrepared)) {
        return true;
    }

    QNDEBUG(QStringLiteral("Preparing SQL query to insert or replace the resource"));

    m_insertOrReplaceResourceQuery = QSqlQuery(m_sqlDatabase);
    bool res = m_insertOrReplaceResourceQuery.prepare(QStringLiteral("INSERT OR REPLACE INTO Resources (resourceGuid, "
                                                                     "noteGuid, noteLocalUid, dataBody, dataSize, dataHash, mime, "
                                                                     "width, height, recognitionDataBody, recognitionDataSize, "
                                                                     "recognitionDataHash, alternateDataBody, alternateDataSize, "
                                                                     "alternateDataHash, resourceUpdateSequenceNumber, "
                                                                     "resourceIsDirty, resourceIndexInNote, resourceLocalUid) "
                                                                     "VALUES(:resourceGuid, :noteGuid, :noteLocalUid, :dataBody, "
                                                                     ":dataSize, :dataHash, :mime, :width, :height, "
                                                                     ":recognitionDataBody, :recognitionDataSize, "
                                                                     ":recognitionDataHash, :alternateDataBody, :alternateDataSize, "
                                                                     ":alternateDataHash, :resourceUpdateSequenceNumber, :resourceIsDirty, "
                                                                     ":resourceIndexInNote, :resourceLocalUid)"));
    if (res) {
        m_insertOrReplaceResourceQueryPrepared = true;
    }

    return res;
}

bool LocalStorageManagerPrivate::checkAndPrepareInsertOrReplaceNoteResourceQuery()
{
    if (Q_LIKELY(m_insertOrReplaceNoteResourceQueryPrepared)) {
        return true;
    }

    QNDEBUG(QStringLiteral("Preparing SQL query to insert or replace resource into NoteResources table"));

    m_insertOrReplaceNoteResourceQuery = QSqlQuery(m_sqlDatabase);
    bool res = m_insertOrReplaceNoteResourceQuery.prepare(QStringLiteral("INSERT OR REPLACE INTO NoteResources "
                                                                         "(localNote, note, localResource, resource) "
                                                                         "VALUES(:localNote, :note, :localResource, :resource)"));
    if (res) {
        m_insertOrReplaceNoteResourceQueryPrepared = true;
    }

    return res;
}

bool LocalStorageManagerPrivate::checkAndPrepareDeleteResourceFromResourceRecognitionTypesQuery()
{
    if (Q_LIKELY(m_deleteResourceFromResourceRecognitionTypesQueryPrepared)) {
        return true;
    }

    QNDEBUG(QStringLiteral("Preparing SQL query to delete resource from ResourceRecognitionData table"));

    m_deleteResourceFromResourceRecognitionTypesQuery = QSqlQuery(m_sqlDatabase);
    bool res = m_deleteResourceFromResourceRecognitionTypesQuery.prepare(QStringLiteral("DELETE FROM ResourceRecognitionData "
                                                                                        "WHERE resourceLocalUid = :resourceLocalUid"));
    if (res) {
        m_deleteResourceFromResourceRecognitionTypesQueryPrepared = true;
    }

    return res;
}

bool LocalStorageManagerPrivate::checkAndPrepareInsertOrReplaceIntoResourceRecognitionDataQuery()
{
    if (Q_LIKELY(m_insertOrReplaceIntoResourceRecognitionDataQueryPrepared)) {
        return true;
    }

    QNDEBUG(QStringLiteral("Preparing SQL query to insert or replace resource into ResourceRecognitionData table"));

    m_insertOrReplaceIntoResourceRecognitionDataQuery = QSqlQuery(m_sqlDatabase);
    bool res = m_insertOrReplaceIntoResourceRecognitionDataQuery.prepare(QStringLiteral("INSERT OR REPLACE INTO ResourceRecognitionData"
                                                                                        "(resourceLocalUid, noteLocalUid, recognitionData) "
                                                                                        "VALUES(:resourceLocalUid, :noteLocalUid, :recognitionData)"));

    if (res) {
        m_insertOrReplaceIntoResourceRecognitionDataQueryPrepared = true;
    }

    return res;
}

bool LocalStorageManagerPrivate::checkAndPrepareDeleteResourceFromResourceAttributesQuery()
{
    if (Q_LIKELY(m_deleteResourceFromResourceAttributesQueryPrepared)) {
        return true;
    }

    QNDEBUG(QStringLiteral("Preparing SQL query to delete resource from ResourceAttributes table"));

    m_deleteResourceFromResourceAttributesQuery = QSqlQuery(m_sqlDatabase);
    bool res = m_deleteResourceFromResourceAttributesQuery.prepare(QStringLiteral("DELETE FROM ResourceAttributes WHERE resourceLocalUid = :resourceLocalUid"));

    if (res) {
        m_deleteResourceFromResourceAttributesQueryPrepared = true;
    }

    return res;
}

bool LocalStorageManagerPrivate::checkAndPrepareDeleteResourceFromResourceAttributesApplicationDataKeysOnlyQuery()
{
    if (Q_LIKELY(m_deleteResourceFromResourceAttributesApplicationDataKeysOnlyQueryPrepared)) {
        return true;
    }

    QNDEBUG(QStringLiteral("Preparing SQL query to delete Resource from ResourceAttributesApplicationDataKeysOnly table"));

    m_deleteResourceFromResourceAttributesApplicationDataKeysOnlyQuery = QSqlQuery(m_sqlDatabase);
    bool res = m_deleteResourceFromResourceAttributesApplicationDataKeysOnlyQuery.prepare(QStringLiteral("DELETE FROM ResourceAttributesApplicationDataKeysOnly "
                                                                                                         "WHERE resourceLocalUid = :resourceLocalUid"));
    if (res) {
        m_deleteResourceFromResourceAttributesApplicationDataKeysOnlyQueryPrepared = true;
    }

    return res;
}

bool LocalStorageManagerPrivate::checkAndPrepareDeleteResourceFromResourceAttributesApplicationDataFullMapQuery()
{
    if (Q_LIKELY(m_deleteResourceFromResourceAttributesApplicationDataFullMapQueryPrepared)) {
        return true;
    }

    QNDEBUG(QStringLiteral("Preparing SQL query to delete Resource from ResourceAttributesApplicationDataFullMap table"));

    m_deleteResourceFromResourceAttributesApplicationDataFullMapQuery = QSqlQuery(m_sqlDatabase);
    bool res = m_deleteResourceFromResourceAttributesApplicationDataFullMapQuery.prepare(QStringLiteral("DELETE FROM ResourceAttributesApplicationDataFullMap "
                                                                                                        "WHERE resourceLocalUid = :resourceLocalUid"));
    if (res) {
        m_deleteResourceFromResourceAttributesApplicationDataFullMapQueryPrepared = true;
    }

    return res;
}

bool LocalStorageManagerPrivate::checkAndPrepareInsertOrReplaceResourceAttributesQuery()
{
    if (Q_LIKELY(m_insertOrReplaceResourceAttributesQueryPrepared)) {
        return true;
    }

    QNDEBUG(QStringLiteral("Preparing SQL query to insert or replace ResourceAttributes"));

    m_insertOrReplaceResourceAttributesQuery = QSqlQuery(m_sqlDatabase);
    bool res = m_insertOrReplaceResourceAttributesQuery.prepare(QStringLiteral("INSERT OR REPLACE INTO ResourceAttributes"
                                                                               "(resourceLocalUid, resourceSourceURL, timestamp, "
                                                                               "resourceLatitude, resourceLongitude, resourceAltitude, "
                                                                               "cameraMake, cameraModel, clientWillIndex, "
                                                                               "fileName, attachment) VALUES(:resourceLocalUid, "
                                                                               ":resourceSourceURL, :timestamp, :resourceLatitude, "
                                                                               ":resourceLongitude, :resourceAltitude, :cameraMake, "
                                                                               ":cameraModel, :clientWillIndex, :fileName, :attachment)"));
    if (res) {
        m_insertOrReplaceResourceAttributesQueryPrepared = true;
    }

    return res;
}

bool LocalStorageManagerPrivate::checkAndPrepareInsertOrReplaceResourceAttributesApplicationDataKeysOnlyQuery()
{
    if (Q_LIKELY(m_insertOrReplaceResourceAttributeApplicationDataKeysOnlyQueryPrepared)) {
        return true;
    }

    QNDEBUG(QStringLiteral("Preparing SQL query to insert or replace resource attribute application data (keys only)"));

    m_insertOrReplaceResourceAttributeApplicationDataKeysOnlyQuery = QSqlQuery(m_sqlDatabase);
    bool res = m_insertOrReplaceResourceAttributeApplicationDataKeysOnlyQuery.prepare(QStringLiteral("INSERT OR REPLACE INTO ResourceAttributesApplicationDataKeysOnly"
                                                                                                     "(resourceLocalUid, resourceKey) VALUES(:resourceLocalUid, :resourceKey)"));
    if (res) {
        m_insertOrReplaceResourceAttributeApplicationDataKeysOnlyQueryPrepared = true;
    }

    return res;
}

bool LocalStorageManagerPrivate::checkAndPrepareInsertOrReplaceResourceAttributesApplicationDataFullMapQuery()
{
    if (Q_LIKELY(m_insertOrReplaceResourceAttributeApplicationDataFullMapQueryPrepared)) {
        return true;
    }

    QNDEBUG(QStringLiteral("Preparing SQL query to insert or replace resource attributes application data (full map)"));

    m_insertOrReplaceResourceAttributeApplicationDataFullMapQuery = QSqlQuery(m_sqlDatabase);
    bool res = m_insertOrReplaceResourceAttributeApplicationDataFullMapQuery.prepare(QStringLiteral("INSERT OR REPLACE INTO ResourceAttributesApplicationDataFullMap"
                                                                                                    "(resourceLocalUid, resourceMapKey, resourceValue) "
                                                                                                    "VALUES(:resourceLocalUid, :resourceMapKey, :resourceValue)"));
    if (res) {
        m_insertOrReplaceResourceAttributeApplicationDataFullMapQueryPrepared = true;
    }

    return res;
}

bool LocalStorageManagerPrivate::checkAndPrepareResourceCountQuery() const
{
    if (Q_LIKELY(m_getResourceCountQueryPrepared)) {
        return true;
    }

    QNDEBUG(QStringLiteral("Preparing SQL query to get the count of Resources"));

    m_getResourceCountQuery = QSqlQuery(m_sqlDatabase);
    bool res = m_getResourceCountQuery.prepare(QStringLiteral("SELECT COUNT(*) FROM Resources"));

    if (res) {
        m_getResourceCountQueryPrepared = res;
    }

    return res;
}

bool LocalStorageManagerPrivate::insertOrReplaceSavedSearch(const SavedSearch & search, ErrorString & errorDescription)
{
    // NOTE: this method expects to be called after the search is already checked
    // for sanity of its parameters!

    ErrorString errorPrefix(QT_TRANSLATE_NOOP("", "can't insert or replace saved search into the local storage database"));

    bool res = checkAndPrepareInsertOrReplaceSavedSearchQuery();
    if (!res) {
        errorDescription.base() = errorPrefix.base();
        errorDescription.additionalBases().append(QT_TRANSLATE_NOOP("", "failed to prepare the SQL query"));
        QNWARNING(errorDescription << m_insertOrReplaceSavedSearchQuery.lastError());
        errorDescription.details() = m_insertOrReplaceSavedSearchQuery.lastError().text();
        return false;
    }

    QSqlQuery & query = m_insertOrReplaceSavedSearchQuery;
    QVariant nullValue;

    query.bindValue(QStringLiteral(":localUid"), search.localUid());
    query.bindValue(QStringLiteral(":guid"), (search.hasGuid() ? search.guid() : nullValue));
    query.bindValue(QStringLiteral(":name"), (search.hasName() ? search.name() : nullValue));
    query.bindValue(QStringLiteral(":nameLower"), (search.hasName() ? search.name().toLower() : nullValue));

    query.bindValue(QStringLiteral(":query"), (search.hasQuery() ? search.query() : nullValue));
    query.bindValue(QStringLiteral(":format"), (search.hasQueryFormat() ? search.queryFormat() : nullValue));
    query.bindValue(QStringLiteral(":updateSequenceNumber"), (search.hasUpdateSequenceNumber()
                                                              ? search.updateSequenceNumber()
                                                              : nullValue));
    query.bindValue(QStringLiteral(":isDirty"), (search.isDirty() ? 1 : 0));
    query.bindValue(QStringLiteral(":isLocal"), (search.isLocal() ? 1 : 0));
    query.bindValue(QStringLiteral(":includeAccount"), (search.hasIncludeAccount()
                                                        ? (search.includeAccount() ? 1 : 0)
                                                        : nullValue));
    query.bindValue(QStringLiteral(":includePersonalLinkedNotebooks"), (search.hasIncludePersonalLinkedNotebooks()
                                                                        ? (search.includePersonalLinkedNotebooks() ? 1 : 0)
                                                                        : nullValue));
    query.bindValue(QStringLiteral(":includeBusinessLinkedNotebooks"), (search.hasIncludeBusinessLinkedNotebooks()
                                                                        ? (search.includeBusinessLinkedNotebooks() ? 1 : 0)
                                                                        : nullValue));
    query.bindValue(QStringLiteral(":isFavorited"), (search.isFavorited() ? 1 : 0));

    res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR();

    query.finish();
    return true;
}

bool LocalStorageManagerPrivate::checkAndPrepareInsertOrReplaceSavedSearchQuery()
{
    if (Q_LIKELY(m_insertOrReplaceSavedSearchQueryPrepared)) {
        return true;
    }

    QNDEBUG(QStringLiteral("Preparing SQL query to insert or replace SavedSearch"));

    QString columns = QStringLiteral("localUid, guid, name, nameLower, query, format, updateSequenceNumber, isDirty, "
                                     "isLocal, includeAccount, includePersonalLinkedNotebooks, "
                                     "includeBusinessLinkedNotebooks, isFavorited");

    QString valuesNames = QStringLiteral(":localUid, :guid, :name, :nameLower, :query, :format, :updateSequenceNumber, :isDirty, "
                                         ":isLocal, :includeAccount, :includePersonalLinkedNotebooks, "
                                         ":includeBusinessLinkedNotebooks, :isFavorited");

    QString queryString = QString("INSERT OR REPLACE INTO SavedSearches (%1) VALUES(%2)").arg(columns,valuesNames);

    m_insertOrReplaceSavedSearchQuery = QSqlQuery(m_sqlDatabase);
    bool res = m_insertOrReplaceSavedSearchQuery.prepare(queryString);

    if (res) {
        m_insertOrReplaceSavedSearchQueryPrepared = true;
    }

    return res;
}

bool LocalStorageManagerPrivate::checkAndPrepareGetSavedSearchCountQuery() const
{
    if (Q_LIKELY(m_getSavedSearchCountQueryPrepared)) {
        return true;
    }

    QNDEBUG(QStringLiteral("Preparing SQL query to get the count of SavedSearches"));

    m_getSavedSearchCountQuery = QSqlQuery(m_sqlDatabase);
    bool res = m_getSavedSearchCountQuery.prepare(QStringLiteral("SELECT COUNT(*) FROM SavedSearches"));

    if (res) {
        m_getSavedSearchCountQueryPrepared = true;
    }

    return res;
}

void LocalStorageManagerPrivate::fillResourceFromSqlRecord(const QSqlRecord & rec, const bool withBinaryData,
                                                           Resource & resource) const
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

    CHECK_AND_SET_RESOURCE_PROPERTY(resourceLocalUid, QString, QString, setLocalUid);
    CHECK_AND_SET_RESOURCE_PROPERTY(resourceIsDirty, int, bool, setDirty);
    CHECK_AND_SET_RESOURCE_PROPERTY(noteGuid, QString, QString, setNoteGuid);
    CHECK_AND_SET_RESOURCE_PROPERTY(localNote, QString, QString, setNoteLocalUid);
    CHECK_AND_SET_RESOURCE_PROPERTY(resourceUpdateSequenceNumber, int, qint32, setUpdateSequenceNumber);
    CHECK_AND_SET_RESOURCE_PROPERTY(dataSize, int, qint32, setDataSize);
    CHECK_AND_SET_RESOURCE_PROPERTY(dataHash, QByteArray, QByteArray, setDataHash);
    CHECK_AND_SET_RESOURCE_PROPERTY(mime, QString, QString, setMime);

    CHECK_AND_SET_RESOURCE_PROPERTY(resourceGuid, QString, QString, setGuid);
    CHECK_AND_SET_RESOURCE_PROPERTY(width, int, qint16, setWidth);
    CHECK_AND_SET_RESOURCE_PROPERTY(height, int, qint16, setHeight);
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

    bool hasAttributes = fillResourceAttributesFromSqlRecord(rec, attributes);
    hasAttributes |= fillResourceAttributesApplicationDataKeysOnlyFromSqlRecord(rec, attributes);
    hasAttributes |= fillResourceAttributesApplicationDataFullMapFromSqlRecord(rec, attributes);

    if (hasAttributes && !resource.hasResourceAttributes()) {
        resource.setResourceAttributes(attributes);
    }
}

bool LocalStorageManagerPrivate::fillResourceAttributesFromSqlRecord(const QSqlRecord & rec, qevercloud::ResourceAttributes & attributes) const
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

bool LocalStorageManagerPrivate::fillResourceAttributesApplicationDataKeysOnlyFromSqlRecord(const QSqlRecord & rec,
                                                                                            qevercloud::ResourceAttributes & attributes) const
{
    bool hasSomething = false;

    int index = rec.indexOf(QStringLiteral("resourceKey"));
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

bool LocalStorageManagerPrivate::fillResourceAttributesApplicationDataFullMapFromSqlRecord(const QSqlRecord & rec,
                                                                                           qevercloud::ResourceAttributes & attributes) const
{
    bool hasSomething = false;

    int keyIndex = rec.indexOf(QStringLiteral("resourceMapKey"));
    int valueIndex = rec.indexOf(QStringLiteral("resourceValue"));
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

void LocalStorageManagerPrivate::fillNoteAttributesFromSqlRecord(const QSqlRecord & rec, qevercloud::NoteAttributes & attributes) const
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
    CHECK_AND_SET_NOTE_ATTRIBUTE(creatorId, qint32, qevercloud::UserID);
    CHECK_AND_SET_NOTE_ATTRIBUTE(lastEditorId, qint32, qevercloud::UserID);
    CHECK_AND_SET_NOTE_ATTRIBUTE(sharedWithBusiness, int, bool);
    CHECK_AND_SET_NOTE_ATTRIBUTE(conflictSourceNoteGuid, QString, QString);
    CHECK_AND_SET_NOTE_ATTRIBUTE(noteTitleQuality, qint32, qint32);

#undef CHECK_AND_SET_NOTE_ATTRIBUTE
}

void LocalStorageManagerPrivate::fillNoteAttributesApplicationDataKeysOnlyFromSqlRecord(const QSqlRecord & rec,
                                                                                        qevercloud::NoteAttributes & attributes) const
{
    int keysOnlyIndex = rec.indexOf(QStringLiteral("applicationDataKeysOnly"));
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

void LocalStorageManagerPrivate::fillNoteAttributesApplicationDataFullMapFromSqlRecord(const QSqlRecord & rec,
                                                                                       qevercloud::NoteAttributes & attributes) const
{
    int keyIndex = rec.indexOf(QStringLiteral("applicationDataKeysMap"));
    int valueIndex = rec.indexOf(QStringLiteral("applicationDataValues"));

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

void LocalStorageManagerPrivate::fillNoteAttributesClassificationsFromSqlRecord(const QSqlRecord & rec,
                                                                                qevercloud::NoteAttributes & attributes) const
{
    int keyIndex = rec.indexOf(QStringLiteral("classificationKeys"));
    int valueIndex = rec.indexOf(QStringLiteral("classificationValues"));

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

bool LocalStorageManagerPrivate::fillUserFromSqlRecord(const QSqlRecord & rec, User & user,
                                                       ErrorString & errorDescription) const
{
#define FIND_AND_SET_USER_PROPERTY(column, setter, type, localType, isRequired) \
    { \
        bool valueFound = false; \
        int index = rec.indexOf(QStringLiteral(#column)); \
        if (index >= 0) { \
            QVariant value = rec.value(QStringLiteral(#column)); \
            if (!value.isNull()) { \
                user.setter(static_cast<localType>(qvariant_cast<type>(value))); \
                valueFound = true; \
            } \
        } \
        \
        if (!valueFound && isRequired) { \
            errorDescription.base() = QT_TRANSLATE_NOOP("", "missing field in the result of SQL query"); \
            errorDescription.details() = QStringLiteral(#column); \
            QNCRITICAL(errorDescription); \
            return false; \
        } \
    }

    bool isRequired = true;

    FIND_AND_SET_USER_PROPERTY(userIsDirty, setDirty, int, bool, isRequired)
    FIND_AND_SET_USER_PROPERTY(userIsLocal, setLocal, int, bool, isRequired)
    FIND_AND_SET_USER_PROPERTY(username, setUsername, QString, QString, !isRequired)
    FIND_AND_SET_USER_PROPERTY(email, setEmail, QString, QString, !isRequired)
    FIND_AND_SET_USER_PROPERTY(name, setName, QString, QString, !isRequired)
    FIND_AND_SET_USER_PROPERTY(timezone, setTimezone, QString, QString, !isRequired)
    FIND_AND_SET_USER_PROPERTY(privilege, setPrivilegeLevel, int, qint8, !isRequired)
    FIND_AND_SET_USER_PROPERTY(userCreationTimestamp, setCreationTimestamp,
                               qint64, qint64, !isRequired)
    FIND_AND_SET_USER_PROPERTY(userModificationTimestamp, setModificationTimestamp,
                               qint64, qint64, !isRequired)
    FIND_AND_SET_USER_PROPERTY(userDeletionTimestamp, setDeletionTimestamp,
                               qint64, qint64, !isRequired)
    FIND_AND_SET_USER_PROPERTY(userIsActive, setActive, int, bool, !isRequired)
    FIND_AND_SET_USER_PROPERTY(userShardId, setShardId, QString, QString, !isRequired)
    FIND_AND_SET_USER_PROPERTY(photoUrl, setPhotoUrl, QString, QString, !isRequired)
    FIND_AND_SET_USER_PROPERTY(photoLastUpdated, setPhotoLastUpdateTimestamp, qint64, qint64, !isRequired)

#undef FIND_AND_SET_USER_PROPERTY

    bool foundSomeUserAttribute = false;
    qevercloud::UserAttributes attributes;
    if (user.hasUserAttributes()) {
        const qevercloud::UserAttributes & userAttributes = user.userAttributes();
        attributes.viewedPromotions = userAttributes.viewedPromotions;
        attributes.recentMailedAddresses = userAttributes.recentMailedAddresses;
    }

    int promotionIndex = rec.indexOf(QStringLiteral("promotion"));
    if (promotionIndex >= 0) {
        QVariant value = rec.value(promotionIndex);
        if (!value.isNull()) {
            if (!attributes.viewedPromotions.isSet()) {
                attributes.viewedPromotions = QStringList();
            }
            QString valueString = value.toString();
            if (!attributes.viewedPromotions.ref().contains(valueString)) {
                attributes.viewedPromotions.ref() << valueString;
            }
            foundSomeUserAttribute = true;
        }
    }

    int addressIndex = rec.indexOf(QStringLiteral("address"));
    if (addressIndex >= 0) {
        QVariant value = rec.value(addressIndex);
        if (!value.isNull()) {
            if (!attributes.recentMailedAddresses.isSet()) {
                attributes.recentMailedAddresses = QStringList();
            }
            QString valueString = value.toString();
            if (!attributes.recentMailedAddresses.ref().contains(valueString)) {
                attributes.recentMailedAddresses.ref() << valueString;
            }
            foundSomeUserAttribute = true;
        }
    }

#define FIND_AND_SET_USER_ATTRIBUTE(column, property, type, localType) \
    { \
        int index = rec.indexOf(QStringLiteral(#column)); \
        if (index >= 0) { \
            QVariant value = rec.value(QStringLiteral(#column)); \
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
    FIND_AND_SET_USER_ATTRIBUTE(useEmailAutoFiling, useEmailAutoFiling, int, bool);
    FIND_AND_SET_USER_ATTRIBUTE(reminderEmailConfig, reminderEmailConfig, int, qevercloud::ReminderEmailConfig::type);
    FIND_AND_SET_USER_ATTRIBUTE(emailAddressLastConfirmed, emailAddressLastConfirmed, qint64, qevercloud::Timestamp);
    FIND_AND_SET_USER_ATTRIBUTE(passwordUpdated, passwordUpdated, qint64, qevercloud::Timestamp)
    FIND_AND_SET_USER_ATTRIBUTE(salesforcePushEnabled, salesforcePushEnabled, int, bool)
    FIND_AND_SET_USER_ATTRIBUTE(shouldLogClientEvent, shouldLogClientEvent, int, bool)

#undef FIND_AND_SET_USER_ATTRIBUTE

    if (foundSomeUserAttribute) {
        user.setUserAttributes(std::move(attributes));
    }

    bool foundSomeAccountingProperty = false;
    qevercloud::Accounting accounting;

#define FIND_AND_SET_ACCOUNTING_PROPERTY(column, property, type, localType) \
    { \
        int index = rec.indexOf(QStringLiteral(#column)); \
        if (index >= 0) { \
            QVariant value = rec.value(QStringLiteral(#column)); \
            if (!value.isNull()) { \
                accounting.property = static_cast<localType>(qvariant_cast<type>(value)); \
                foundSomeAccountingProperty = true; \
            } \
        } \
    }

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
    FIND_AND_SET_ACCOUNTING_PROPERTY(unitPrice, unitPrice, int, qint32);
    FIND_AND_SET_ACCOUNTING_PROPERTY(unitDiscount, unitDiscount, int, qint32);
    FIND_AND_SET_ACCOUNTING_PROPERTY(nextChargeDate, nextChargeDate, qint64, qevercloud::Timestamp);

#undef FIND_AND_SET_ACCOUNTING_PROPERTY

    if (foundSomeAccountingProperty) {
        user.setAccounting(std::move(accounting));
    }

    bool foundSomeAccountLimit = false;
    qevercloud::AccountLimits accountLimits;

#define FIND_AND_SET_ACCOUNT_LIMIT(property, type, localType) \
    { \
        int index = rec.indexOf(QStringLiteral(#property)); \
        if (index >= 0) { \
            QVariant value = rec.value(QStringLiteral(#property)); \
            if (!value.isNull()) { \
                accountLimits.property = static_cast<localType>(qvariant_cast<type>(value)); \
                foundSomeAccountLimit = true; \
            } \
        } \
    }

    FIND_AND_SET_ACCOUNT_LIMIT(userMailLimitDaily, int, qint32)
    FIND_AND_SET_ACCOUNT_LIMIT(noteSizeMax, qint64, qint64)
    FIND_AND_SET_ACCOUNT_LIMIT(resourceSizeMax, qint64, qint64)
    FIND_AND_SET_ACCOUNT_LIMIT(userLinkedNotebookMax, int, qint32)
    FIND_AND_SET_ACCOUNT_LIMIT(uploadLimit, qint64, qint64)
    FIND_AND_SET_ACCOUNT_LIMIT(userNoteCountMax, int, qint32)
    FIND_AND_SET_ACCOUNT_LIMIT(userNotebookCountMax, int, qint32)
    FIND_AND_SET_ACCOUNT_LIMIT(userTagCountMax, int, qint32)
    FIND_AND_SET_ACCOUNT_LIMIT(noteTagCountMax, int, qint32)
    FIND_AND_SET_ACCOUNT_LIMIT(userSavedSearchesMax, int, qint32)
    FIND_AND_SET_ACCOUNT_LIMIT(noteResourceCountMax, int, qint32)

#undef FIND_AND_SET_ACCOUNT_LIMIT

    if (foundSomeAccountLimit) {
        user.setAccountLimits(std::move(accountLimits));
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

bool LocalStorageManagerPrivate::fillNoteFromSqlRecord(const QSqlRecord & rec, Note & note, ErrorString & errorDescription) const
{
#define CHECK_AND_SET_NOTE_PROPERTY(propertyLocalName, setter, type, localType) \
    int propertyLocalName##index = rec.indexOf(QStringLiteral(#propertyLocalName)); \
    if (propertyLocalName##index >= 0) { \
        QVariant value = rec.value(propertyLocalName##index); \
        if (!value.isNull()) { \
            note.setter(static_cast<localType>(qvariant_cast<type>(value))); \
        } \
    }

    CHECK_AND_SET_NOTE_PROPERTY(isDirty, setDirty, int, bool);
    CHECK_AND_SET_NOTE_PROPERTY(isLocal, setLocal, int, bool);
    CHECK_AND_SET_NOTE_PROPERTY(isFavorited, setFavorited, int, bool);
    CHECK_AND_SET_NOTE_PROPERTY(localUid, setLocalUid, QString, QString);

    CHECK_AND_SET_NOTE_PROPERTY(guid, setGuid, QString, QString);
    CHECK_AND_SET_NOTE_PROPERTY(updateSequenceNumber, setUpdateSequenceNumber, qint32, qint32);

    CHECK_AND_SET_NOTE_PROPERTY(notebookGuid, setNotebookGuid, QString, QString);
    CHECK_AND_SET_NOTE_PROPERTY(notebookLocalUid, setNotebookLocalUid, QString, QString);
    CHECK_AND_SET_NOTE_PROPERTY(title, setTitle, QString, QString);
    CHECK_AND_SET_NOTE_PROPERTY(content, setContent, QString, QString);
    CHECK_AND_SET_NOTE_PROPERTY(contentLength, setContentLength, qint32, qint32);
    CHECK_AND_SET_NOTE_PROPERTY(contentHash, setContentHash, QByteArray, QByteArray);

    CHECK_AND_SET_NOTE_PROPERTY(creationTimestamp, setCreationTimestamp, qint64, qint64);
    CHECK_AND_SET_NOTE_PROPERTY(modificationTimestamp, setModificationTimestamp, qint64, qint64);
    CHECK_AND_SET_NOTE_PROPERTY(deletionTimestamp, setDeletionTimestamp, qint64, qint64);
    CHECK_AND_SET_NOTE_PROPERTY(isActive, setActive, int, bool);

#undef CHECK_AND_SET_NOTE_PROPERTY

    int indexOfThumbnail = rec.indexOf(QStringLiteral("thumbnail"));
    if (indexOfThumbnail >= 0) {
        QVariant thumbnailValue = rec.value(indexOfThumbnail);
        if (!thumbnailValue.isNull()) {
            QImage thumbnail = thumbnailValue.value<QImage>();
            note.setThumbnail(thumbnail);
        }
    }

    int hasAttributesIndex = rec.indexOf(QStringLiteral("hasAttributes"));
    if (hasAttributesIndex >= 0)
    {
        QVariant hasAttributesValue = rec.value(hasAttributesIndex);
        if (!hasAttributesValue.isNull())
        {
            bool hasAttributes = static_cast<bool>(qvariant_cast<int>(hasAttributesValue));
            if (hasAttributes)
            {
                qevercloud::NoteAttributes & attributes = note.noteAttributes();

                fillNoteAttributesFromSqlRecord(rec, attributes);
                fillNoteAttributesApplicationDataKeysOnlyFromSqlRecord(rec, attributes);
                fillNoteAttributesApplicationDataFullMapFromSqlRecord(rec, attributes);
                fillNoteAttributesClassificationsFromSqlRecord(rec, attributes);
            }
        }
    }

    bool foundSomeNoteRestriction = false;
    qevercloud::NoteRestrictions restrictions;

#define CHECK_AND_SET_NOTE_RESTRICTION(column, restriction) \
    int restriction##Index = rec.indexOf(QStringLiteral(#column)); \
    if (restriction##Index >= 0) { \
        QVariant value = rec.value(restriction##Index); \
        if (!value.isNull()) { \
            restrictions.restriction = static_cast<bool>(qvariant_cast<qint32>(value)); \
            foundSomeNoteRestriction = true; \
        } \
    }

    CHECK_AND_SET_NOTE_RESTRICTION(noUpdateNoteTitle, noUpdateTitle)
    CHECK_AND_SET_NOTE_RESTRICTION(noUpdateNoteContent, noUpdateContent)
    CHECK_AND_SET_NOTE_RESTRICTION(noEmailNote, noEmail)
    CHECK_AND_SET_NOTE_RESTRICTION(noShareNote, noShare)
    CHECK_AND_SET_NOTE_RESTRICTION(noShareNotePublicly, noSharePublicly)

#undef CHECK_AND_SET_NOTE_RESTRICTION

    if (foundSomeNoteRestriction) {
        note.setNoteRestrictions(std::move(restrictions));
    }

    bool foundSomeNoteLimit = false;
    qevercloud::NoteLimits limits;

#define CHECK_AND_SET_NOTE_LIMIT(limit, columnType, type) \
    int limit##Index = rec.indexOf(QStringLiteral(#limit)); \
    if (limit##Index >= 0) { \
        QVariant value = rec.value(limit##Index); \
        if (!value.isNull()) { \
            limits.limit = static_cast<type>(qvariant_cast<columnType>(value)); \
            foundSomeNoteLimit = true; \
        } \
    }

    CHECK_AND_SET_NOTE_LIMIT(noteResourceCountMax, qint32, qint32)
    CHECK_AND_SET_NOTE_LIMIT(uploadLimit, qint64, qint64)
    CHECK_AND_SET_NOTE_LIMIT(resourceSizeMax, qint64, qint64)
    CHECK_AND_SET_NOTE_LIMIT(noteSizeMax, qint64, qint64)
    CHECK_AND_SET_NOTE_LIMIT(uploaded, qint64, qint64)

#undef CHECK_AND_SET_NOTE_LIMIT

    if (foundSomeNoteLimit) {
        note.setNoteLimits(std::move(limits));
    }

    if (note.hasGuid())
    {
        SharedNote sharedNote;
        bool res = fillSharedNoteFromSqlRecord(rec, sharedNote, errorDescription);
        if (!res) {
            return false;
        }

        if (!sharedNote.noteGuid().isEmpty()) {
            note.addSharedNote(sharedNote);
        }
    }

    return true;
}

bool LocalStorageManagerPrivate::fillSharedNoteFromSqlRecord(const QSqlRecord & record, SharedNote & sharedNote,
                                                             ErrorString & errorDescription) const
{
#define CHECK_AND_SET_SHARED_NOTE_PROPERTY(property, type, localType, setter) \
    { \
        int index = record.indexOf(QStringLiteral(#property)); \
        if (index >= 0) { \
            QVariant value = record.value(index); \
            if (!value.isNull()) { \
                sharedNote.setter(static_cast<localType>(qvariant_cast<type>(value))); \
            } \
        } \
    }

    CHECK_AND_SET_SHARED_NOTE_PROPERTY(sharedNoteNoteGuid, QString, QString, setNoteGuid)
    CHECK_AND_SET_SHARED_NOTE_PROPERTY(sharedNoteSharerUserId, qint32, qint32, setSharerUserId)
    CHECK_AND_SET_SHARED_NOTE_PROPERTY(sharedNoteRecipientIdentityId, qint64, qint64, setRecipientIdentityId)
    CHECK_AND_SET_SHARED_NOTE_PROPERTY(sharedNoteRecipientContactName, QString, QString, setRecipientIdentityContactName)
    CHECK_AND_SET_SHARED_NOTE_PROPERTY(sharedNoteRecipientContactId, QString, QString, setRecipientIdentityContactId)
    CHECK_AND_SET_SHARED_NOTE_PROPERTY(sharedNoteRecipientContactType, qint32, qint32, setRecipientIdentityContactType)
    CHECK_AND_SET_SHARED_NOTE_PROPERTY(sharedNoteRecipientContactPhotoUrl, QString, QString, setRecipientIdentityContactPhotoUrl)
    CHECK_AND_SET_SHARED_NOTE_PROPERTY(sharedNoteRecipientContactPhotoLastUpdated, qint64, qint64, setRecipientIdentityContactPhotoLastUpdated)
    CHECK_AND_SET_SHARED_NOTE_PROPERTY(sharedNoteRecipientContactMessagingPermit, QByteArray, QByteArray, setRecipientIdentityContactMessagingPermit)
    CHECK_AND_SET_SHARED_NOTE_PROPERTY(sharedNoteRecipientContactMessagingPermitExpires, qint64, qint64, setRecipientIdentityContactMessagingPermitExpires)
    CHECK_AND_SET_SHARED_NOTE_PROPERTY(sharedNoteRecipientUserId, qint32, qint32, setRecipientIdentityUserId)
    CHECK_AND_SET_SHARED_NOTE_PROPERTY(sharedNoteRecipientDeactivated, int, bool, setRecipientIdentityDeactivated)
    CHECK_AND_SET_SHARED_NOTE_PROPERTY(sharedNoteRecipientSameBusiness, int, bool, setRecipientIdentitySameBusiness)
    CHECK_AND_SET_SHARED_NOTE_PROPERTY(sharedNoteRecipientBlocked, int, bool, setRecipientIdentityBlocked)
    CHECK_AND_SET_SHARED_NOTE_PROPERTY(sharedNoteRecipientUserConnected, int, bool, setRecipientIdentityUserConnected)
    CHECK_AND_SET_SHARED_NOTE_PROPERTY(sharedNoteRecipientEventId, qint64, qint64, setRecipientIdentityEventId)
    CHECK_AND_SET_SHARED_NOTE_PROPERTY(sharedNotePrivileveLevel, qint8, qint8, setPrivilegeLevel)
    CHECK_AND_SET_SHARED_NOTE_PROPERTY(sharedNoteCreationTimestamp, qint64, qint64, setCreationTimestamp)
    CHECK_AND_SET_SHARED_NOTE_PROPERTY(sharedNoteModificationTimestamp, qint64, qint64, setModificationTimestamp)
    CHECK_AND_SET_SHARED_NOTE_PROPERTY(sharedNoteAssignmentTimestamp, qint64, qint64, setAssignmentTimestamp)

#undef CHECK_AND_SET_SHARED_NOTE_PROPERTY

    int recordIndex = record.indexOf(QStringLiteral("indexInNote"));
    if (recordIndex >= 0)
    {
        QVariant value = record.value(recordIndex);
        if (!value.isNull())
        {
            bool conversionResult = false;
            int indexInNote = value.toInt(&conversionResult);
            if (!conversionResult) {
                errorDescription.base() = QT_TRANSLATE_NOOP("", "can't convert shared note's index in note to int");
                QNCRITICAL(errorDescription);
                return false;
            }
            sharedNote.setIndexInNote(indexInNote);
        }
    }

    return true;
}

bool LocalStorageManagerPrivate::fillNoteTagIdFromSqlRecord(const QSqlRecord & record, const QString & column,
                                                            QList<QPair<QString, int> > & tagIdsAndIndices,
                                                            QHash<QString, int> & tagIndexPerId, ErrorString & errorDescription) const
{
    int tagIdIndex = record.indexOf(column);
    if (tagIdIndex < 0) {
        return true;
    }

    QVariant value = record.value(tagIdIndex);
    if (value.isNull()) {
        return true;
    }

    QVariant tagGuidIndexInNoteValue = record.value(QStringLiteral("tagIndexInNote"));
    if (tagGuidIndexInNoteValue.isNull()) {
        QNWARNING(QStringLiteral("tag index in note was not found in the result of SQL query"));
        return true;
    }

    bool conversionResult = false;
    int tagIndexInNote = tagGuidIndexInNoteValue.toInt(&conversionResult);
    if (!conversionResult) {
        errorDescription.base() = QT_TRANSLATE_NOOP("", "can't convert tag's index in note to int");
        return false;
    }

    QString tagId = value.toString();
    auto it = tagIndexPerId.find(tagId);
    bool tagIndexNotFound = (it == tagIndexPerId.end());
    if (tagIndexNotFound) {
        int tagIndexInList = tagIdsAndIndices.size();
        tagIndexPerId[tagId] = tagIndexInList;
        tagIdsAndIndices << QPair<QString, int>(tagId, tagIndexInNote);
        return true;
    }

    QPair<QString, int> & tagIdAndIndexInNote = tagIdsAndIndices[it.value()];
    tagIdAndIndexInNote.first = tagId;
    tagIdAndIndexInNote.second = tagIndexInNote;
    return true;
}

bool LocalStorageManagerPrivate::fillNotebookFromSqlRecord(const QSqlRecord & record, Notebook & notebook,
                                                           ErrorString & errorDescription) const
{
#define CHECK_AND_SET_NOTEBOOK_ATTRIBUTE(attribute, setter, dbType, trueType, isRequired) \
    { \
        bool valueFound = false; \
        int index = record.indexOf(QStringLiteral( #attribute )); \
        if (index >= 0) { \
            QVariant value = record.value(index); \
            if (!value.isNull()) { \
                notebook.setter(static_cast<trueType>((qvariant_cast<dbType>(value)))); \
                valueFound = true; \
            } \
        } \
        \
        if (!valueFound && isRequired) { \
            errorDescription.base() = QT_TRANSLATE_NOOP("", "missing field in the result of SQL query"); \
            errorDescription.details() = #attribute; \
            return false; \
        } \
    }

    bool isRequired = true;

    CHECK_AND_SET_NOTEBOOK_ATTRIBUTE(isDirty, setDirty, int, bool, isRequired);
    CHECK_AND_SET_NOTEBOOK_ATTRIBUTE(isLocal, setLocal, int, bool, isRequired);
    CHECK_AND_SET_NOTEBOOK_ATTRIBUTE(localUid, setLocalUid, QString, QString, isRequired);

    isRequired = false;

    CHECK_AND_SET_NOTEBOOK_ATTRIBUTE(updateSequenceNumber, setUpdateSequenceNumber,
                                     qint32, qint32, isRequired);
    CHECK_AND_SET_NOTEBOOK_ATTRIBUTE(notebookName, setName, QString, QString, isRequired);
    CHECK_AND_SET_NOTEBOOK_ATTRIBUTE(creationTimestamp, setCreationTimestamp,
                                     qint64, qint64, isRequired);
    CHECK_AND_SET_NOTEBOOK_ATTRIBUTE(modificationTimestamp, setModificationTimestamp,
                                     qint64, qint64, isRequired);
    CHECK_AND_SET_NOTEBOOK_ATTRIBUTE(guid, setGuid, QString, QString, isRequired);
    CHECK_AND_SET_NOTEBOOK_ATTRIBUTE(linkedNotebookGuid, setLinkedNotebookGuid, QString, QString, isRequired);
    CHECK_AND_SET_NOTEBOOK_ATTRIBUTE(isFavorited, setFavorited, int, bool, isRequired);
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

    CHECK_AND_SET_NOTEBOOK_ATTRIBUTE(recipientReminderNotifyEmail, setRecipientReminderNotifyEmail,
                                     int, bool, isRequired);
    CHECK_AND_SET_NOTEBOOK_ATTRIBUTE(recipientReminderNotifyInApp, setRecipientReminderNotifyInApp,
                                     int, bool, isRequired);
    CHECK_AND_SET_NOTEBOOK_ATTRIBUTE(recipientInMyList, setRecipientInMyList, int, bool, isRequired);
    CHECK_AND_SET_NOTEBOOK_ATTRIBUTE(recipientStack, setRecipientStack, QString, QString, isRequired);

    CHECK_AND_SET_NOTEBOOK_ATTRIBUTE(isLastUsed, setLastUsed, int, bool, isRequired);
    CHECK_AND_SET_NOTEBOOK_ATTRIBUTE(isDefault, setDefaultNotebook, int, bool, isRequired);

    // NOTE: workarounding unset isDefaultNotebook and isLastUsed
    if (!notebook.isDefaultNotebook()) {
        notebook.setDefaultNotebook(false);
    }

    if (!notebook.isLastUsed()) {
        notebook.setLastUsed(false);
    }

    if (record.contains(QStringLiteral("contactId")) && !record.isNull(QStringLiteral("contactId")))
    {
        if (notebook.hasContact()) {
            User contact = notebook.contact();
            contact.setId(qvariant_cast<qint32>(record.value(QStringLiteral("contactId"))));
            notebook.setContact(contact);
        }
        else {
            User contact;
            contact.setId(qvariant_cast<qint32>(record.value(QStringLiteral("contactId"))));
            notebook.setContact(contact);
        }

        User user = notebook.contact();
        bool res = fillUserFromSqlRecord(record, user, errorDescription);
        if (!res) {
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
    SET_EN_NOTEBOOK_RESTRICTION(noShareNotesWithBusiness, setCanShareNotesWithBusiness);
    SET_EN_NOTEBOOK_RESTRICTION(noRenameNotebook, setCanRenameNotebook);

#undef SET_EN_NOTEBOOK_RESTRICTION

    CHECK_AND_SET_NOTEBOOK_ATTRIBUTE(updateWhichSharedNotebookRestrictions,
                                     setUpdateWhichSharedNotebookRestrictions,
                                     int, qint8, isRequired);

    CHECK_AND_SET_NOTEBOOK_ATTRIBUTE(expungeWhichSharedNotebookRestrictions,
                                     setExpungeWhichSharedNotebookRestrictions,
                                     int, qint8, isRequired);

    if (notebook.hasGuid())
    {
        SharedNotebook sharedNotebook;
        bool res = fillSharedNotebookFromSqlRecord(record, sharedNotebook, errorDescription);
        if (!res) {
            return false;
        }

        if (sharedNotebook.hasNotebookGuid()) {
            notebook.addSharedNotebook(sharedNotebook);
        }
    }

    return true;
}

bool LocalStorageManagerPrivate::fillSharedNotebookFromSqlRecord(const QSqlRecord & rec,
                                                                 SharedNotebook & sharedNotebook,
                                                                 ErrorString & errorDescription) const
{
#define CHECK_AND_SET_SHARED_NOTEBOOK_PROPERTY(property, type, localType, setter) \
    { \
        int index = rec.indexOf(QStringLiteral(#property)); \
        if (index >= 0) { \
            QVariant value = rec.value(index); \
            if (!value.isNull()) { \
                sharedNotebook.setter(static_cast<localType>(qvariant_cast<type>(value))); \
            } \
        } \
    }

    CHECK_AND_SET_SHARED_NOTEBOOK_PROPERTY(sharedNotebookShareId, qint64, qint64, setId)
    CHECK_AND_SET_SHARED_NOTEBOOK_PROPERTY(sharedNotebookUserId, qint32, qint32, setUserId)
    CHECK_AND_SET_SHARED_NOTEBOOK_PROPERTY(sharedNotebookNotebookGuid, QString, QString, setNotebookGuid)
    CHECK_AND_SET_SHARED_NOTEBOOK_PROPERTY(sharedNotebookEmail, QString, QString, setEmail)
    CHECK_AND_SET_SHARED_NOTEBOOK_PROPERTY(sharedNotebookCreationTimestamp, qint64,
                                           qint64, setCreationTimestamp)
    CHECK_AND_SET_SHARED_NOTEBOOK_PROPERTY(sharedNotebookModificationTimestamp, qint64,
                                           qint64, setModificationTimestamp)
    CHECK_AND_SET_SHARED_NOTEBOOK_PROPERTY(sharedNotebookGlobalId, QString, QString, setGlobalId)
    CHECK_AND_SET_SHARED_NOTEBOOK_PROPERTY(sharedNotebookUsername, QString, QString, setUsername)
    CHECK_AND_SET_SHARED_NOTEBOOK_PROPERTY(sharedNotebookPrivilegeLevel, int, qint8, setPrivilegeLevel)
    CHECK_AND_SET_SHARED_NOTEBOOK_PROPERTY(sharedNotebookRecipientReminderNotifyEmail, int, bool, setReminderNotifyEmail)
    CHECK_AND_SET_SHARED_NOTEBOOK_PROPERTY(sharedNotebookRecipientReminderNotifyInApp, int, bool, setReminderNotifyApp)
    CHECK_AND_SET_SHARED_NOTEBOOK_PROPERTY(sharedNotebookSharerUserId, qint32, qint32, setSharerUserId)
    CHECK_AND_SET_SHARED_NOTEBOOK_PROPERTY(sharedNotebookRecipientUsername, QString, QString, setRecipientUsername)
    CHECK_AND_SET_SHARED_NOTEBOOK_PROPERTY(sharedNotebookRecipientUserId, qint32, qint32, setRecipientUserId)
    CHECK_AND_SET_SHARED_NOTEBOOK_PROPERTY(sharedNotebookRecipientIdentityId, qint64, qint64, setRecipientIdentityId)
    CHECK_AND_SET_SHARED_NOTEBOOK_PROPERTY(sharedNotebookAssignmentTimestamp, qint64, qint64, setAssignmentTimestamp)

#undef CHECK_AND_SET_SHARED_NOTEBOOK_PROPERTY

    int recordIndex = rec.indexOf(QStringLiteral("indexInNotebook"));
    if (recordIndex >= 0)
    {
        QVariant value = rec.value(recordIndex);
        if (!value.isNull())
        {
            bool conversionResult = false;
            int indexInNotebook = value.toInt(&conversionResult);
            if (!conversionResult) {
                errorDescription.base() = QT_TRANSLATE_NOOP("", "can't convert shared notebook's index in notebook to int");
                QNCRITICAL(errorDescription);
                return false;
            }
            sharedNotebook.setIndexInNotebook(indexInNotebook);
        }
    }

    return true;
}

bool LocalStorageManagerPrivate::fillLinkedNotebookFromSqlRecord(const QSqlRecord & rec,
                                                                 LinkedNotebook & linkedNotebook,
                                                                 ErrorString & errorDescription) const
{
#define CHECK_AND_SET_LINKED_NOTEBOOK_PROPERTY(property, type, localType, setter, isRequired) \
    { \
        bool valueFound = false; \
        int index = rec.indexOf(QStringLiteral(#property)); \
        if (index >= 0) { \
            QVariant value = rec.value(index); \
            if (!value.isNull()) { \
                linkedNotebook.setter(static_cast<localType>(qvariant_cast<type>(value))); \
                valueFound = true; \
            } \
        } \
        \
        if (!valueFound && isRequired) { \
            errorDescription.base() = QT_TRANSLATE_NOOP("", "missing field in the result of SQL query"); \
            errorDescription.details() = QStringLiteral(#property); \
            QNCRITICAL(errorDescription); \
            return false; \
        } \
    }

    bool isRequired = true;
    CHECK_AND_SET_LINKED_NOTEBOOK_PROPERTY(guid, QString, QString, setGuid, isRequired);

    isRequired = false;
    CHECK_AND_SET_LINKED_NOTEBOOK_PROPERTY(isDirty, int, bool, setDirty, isRequired);
    CHECK_AND_SET_LINKED_NOTEBOOK_PROPERTY(updateSequenceNumber, qint32, qint32,
                                           setUpdateSequenceNumber, isRequired);
    CHECK_AND_SET_LINKED_NOTEBOOK_PROPERTY(shareName, QString, QString, setShareName, isRequired);
    CHECK_AND_SET_LINKED_NOTEBOOK_PROPERTY(username, QString, QString, setUsername, isRequired);
    CHECK_AND_SET_LINKED_NOTEBOOK_PROPERTY(shardId, QString, QString, setShardId, isRequired);
    CHECK_AND_SET_LINKED_NOTEBOOK_PROPERTY(sharedNotebookGlobalId, QString, QString, setSharedNotebookGlobalId, isRequired);
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

bool LocalStorageManagerPrivate::fillSavedSearchFromSqlRecord(const QSqlRecord & rec,
                                                              SavedSearch & search,
                                                              ErrorString & errorDescription) const
{
#define CHECK_AND_SET_SAVED_SEARCH_PROPERTY(property, type, localType, setter, isRequired) \
    { \
        bool valueFound = false; \
        int index = rec.indexOf(QStringLiteral(#property)); \
        if (index >= 0) { \
            QVariant value = rec.value(index); \
            if (!value.isNull()) { \
                search.setter(static_cast<localType>(qvariant_cast<type>(value))); \
                valueFound = true; \
            } \
        } \
        \
        if (!valueFound && isRequired) { \
            errorDescription.base() = QT_TRANSLATE_NOOP("", "missing field in the result of SQL query"); \
            errorDescription.details() = QStringLiteral(#property); \
            QNCRITICAL(errorDescription); \
            return false; \
        } \
    }

    bool isRequired = false;
    CHECK_AND_SET_SAVED_SEARCH_PROPERTY(guid, QString, QString, setGuid, isRequired);
    CHECK_AND_SET_SAVED_SEARCH_PROPERTY(name, QString, QString, setName, isRequired);
    CHECK_AND_SET_SAVED_SEARCH_PROPERTY(query, QString, QString, setQuery, isRequired);
    CHECK_AND_SET_SAVED_SEARCH_PROPERTY(format, int, qint8, setQueryFormat, isRequired);
    CHECK_AND_SET_SAVED_SEARCH_PROPERTY(updateSequenceNumber, qint32, qint32,
                                        setUpdateSequenceNumber, isRequired);
    CHECK_AND_SET_SAVED_SEARCH_PROPERTY(includeAccount, int, bool, setIncludeAccount,
                                        isRequired);
    CHECK_AND_SET_SAVED_SEARCH_PROPERTY(includePersonalLinkedNotebooks, int, bool,
                                        setIncludePersonalLinkedNotebooks, isRequired);
    CHECK_AND_SET_SAVED_SEARCH_PROPERTY(includeBusinessLinkedNotebooks, int, bool,
                                        setIncludeBusinessLinkedNotebooks, isRequired);

    isRequired = true;
    CHECK_AND_SET_SAVED_SEARCH_PROPERTY(localUid, QString, QString, setLocalUid, isRequired);
    CHECK_AND_SET_SAVED_SEARCH_PROPERTY(isDirty, int, bool, setDirty, isRequired);
    CHECK_AND_SET_SAVED_SEARCH_PROPERTY(isLocal, int, bool, setLocal, isRequired);
    CHECK_AND_SET_SAVED_SEARCH_PROPERTY(isFavorited, int, bool, setFavorited, isRequired);

#undef CHECK_AND_SET_SAVED_SEARCH_PROPERTY

    return true;
}

bool LocalStorageManagerPrivate::fillTagFromSqlRecord(const QSqlRecord & rec, Tag & tag,
                                                      ErrorString & errorDescription) const
{
#define CHECK_AND_SET_TAG_PROPERTY(property, type, localType, setter, isRequired) \
    { \
        bool valueFound = false; \
        int index = rec.indexOf(QStringLiteral(#property)); \
        if (index >= 0) { \
            QVariant value = rec.value(index); \
            if (!value.isNull()) { \
                tag.setter(static_cast<localType>(qvariant_cast<type>(value))); \
                valueFound = true; \
            } \
        } \
        \
        if (!valueFound && isRequired) { \
            errorDescription.base() = QT_TRANSLATE_NOOP("", "missing field in the result of SQL query"); \
            errorDescription.details() = QStringLiteral(#property); \
            QNCRITICAL(errorDescription); \
            return false; \
        } \
    }

    bool isRequired = false;
    CHECK_AND_SET_TAG_PROPERTY(guid, QString, QString, setGuid, isRequired);
    CHECK_AND_SET_TAG_PROPERTY(updateSequenceNumber, qint32, qint32, setUpdateSequenceNumber, isRequired);
    CHECK_AND_SET_TAG_PROPERTY(name, QString, QString, setName, isRequired);
    CHECK_AND_SET_TAG_PROPERTY(linkedNotebookGuid, QString, QString, setLinkedNotebookGuid, isRequired);
    CHECK_AND_SET_TAG_PROPERTY(parentGuid, QString, QString, setParentGuid, isRequired);
    CHECK_AND_SET_TAG_PROPERTY(parentLocalUid, QString, QString, setParentLocalUid, isRequired);

    isRequired = true;
    CHECK_AND_SET_TAG_PROPERTY(localUid, QString, QString, setLocalUid, isRequired);
    CHECK_AND_SET_TAG_PROPERTY(isDirty, int, bool, setDirty, isRequired);
    CHECK_AND_SET_TAG_PROPERTY(isLocal, int, bool, setLocal, isRequired);
    CHECK_AND_SET_TAG_PROPERTY(isFavorited, int, bool, setFavorited, isRequired);

#undef CHECK_AND_SET_TAG_PROPERTY

    return true;
}

QList<Tag> LocalStorageManagerPrivate::fillTagsFromSqlQuery(QSqlQuery & query, ErrorString & errorDescription) const
{
    QList<Tag> tags;
    tags.reserve(qMax(query.size(), 0));

    while(query.next())
    {
        tags << Tag();
        Tag & tag = tags.back();

        QString tagLocalUid = query.value(0).toString();
        if (tagLocalUid.isEmpty()) {
            errorDescription.base() = QT_TRANSLATE_NOOP("", "no tag's local uid in the result of SQL query");
            tags.clear();
            return tags;
        }

        tag.setLocalUid(tagLocalUid);

        bool res = findTag(tag, errorDescription);
        if (!res) {
            tags.clear();
            return tags;
        }
    }

    return tags;
}

bool LocalStorageManagerPrivate::findAndSetTagIdsPerNote(Note & note, ErrorString & errorDescription) const
{
    ErrorString errorPrefix(QT_TRANSLATE_NOOP("", "can't find tag guids/local uids per note"));

    const QString noteLocalUid = note.localUid();

    QSqlQuery query(m_sqlDatabase);
    query.prepare(QStringLiteral("SELECT tag, localTag, tagIndexInNote FROM NoteTags WHERE localNote = ?"));
    query.addBindValue(noteLocalUid);

    bool res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR();

    QMultiHash<int, QString> tagGuidsAndIndices;
    QMultiHash<int, QString> tagLocalUidsAndIndices;

    while (query.next())
    {
        QSqlRecord rec = query.record();

        QString tagLocalUid;
        QString tagGuid;

        bool tagLocalUidFound = false;
        bool tagGuidFound = false;

        int tagGuidIndex = rec.indexOf(QStringLiteral("tag"));
        if (tagGuidIndex >= 0) {
            QVariant value = rec.value(tagGuidIndex);
            tagGuid = value.toString();
            tagGuidFound = true;
        }

        int tagLocalUidIndex = rec.indexOf(QStringLiteral("localTag"));
        if (tagLocalUidIndex >= 0)
        {
            QVariant value = rec.value(tagLocalUidIndex);
            if (!value.isNull()) {
                tagLocalUid = value.toString();
                tagLocalUidFound = true;
            }
        }

        if (!tagLocalUidFound) {
            errorDescription.base() = errorPrefix.base();
            errorDescription.additionalBases().append(QT_TRANSLATE_NOOP("", "no tag local uid in the result of SQL query"));
            return false;
        }

        if (!tagGuidFound) {
            errorDescription.base() = errorPrefix.base();
            errorDescription.additionalBases().append(QT_TRANSLATE_NOOP("", "no tag guid in the result of SQL query"));
            return false;
        }

        if (!tagGuid.isEmpty() && !checkGuid(tagGuid)) {
            errorDescription.base() = errorPrefix.base();
            errorDescription.additionalBases().append(QT_TRANSLATE_NOOP("", "found invalid tag guid for the requested note"));
            return false;
        }

        QNDEBUG(QStringLiteral("Found tag local uid ") << tagLocalUid << QStringLiteral(" and tag guid ") << tagGuid
                << QStringLiteral(" for note with local uid ") << noteLocalUid);

        int indexInNote = -1;
        int recordIndex = rec.indexOf(QStringLiteral("tagIndexInNote"));
        if (recordIndex >= 0)
        {
            QVariant value = rec.value(recordIndex);
            if (!value.isNull())
            {
                bool conversionResult = false;
                indexInNote = value.toInt(&conversionResult);
                if (!conversionResult) {
                    errorDescription.base() = errorPrefix.base();
                    errorDescription.additionalBases().append(QT_TRANSLATE_NOOP("", "can't convert tag index in note to int"));
                    return false;
                }
            }
        }

        tagLocalUidsAndIndices.insert(indexInNote, tagLocalUid);

        if (!tagGuid.isEmpty()) {
            tagGuidsAndIndices.insert(indexInNote, tagGuid);
        }
    }

    // Setting tag local uids

    int numTagLocalUids = tagLocalUidsAndIndices.size();
    QList<QPair<QString, int> > tagLocalUidIndexPairs;
    tagLocalUidIndexPairs.reserve(std::max(numTagLocalUids, 0));
    for(auto it = tagLocalUidsAndIndices.begin(), end = tagLocalUidsAndIndices.end(); it != end; ++it) {
        tagLocalUidIndexPairs << QPair<QString, int>(it.value(), it.key());
    }

    qSort(tagLocalUidIndexPairs.begin(), tagLocalUidIndexPairs.end(), QStringIntPairCompareByInt());
    QStringList tagLocalUids;
    tagLocalUids.reserve(std::max(numTagLocalUids, 0));
    for(int i = 0; i < numTagLocalUids; ++i) {
        tagLocalUids << tagLocalUidIndexPairs[i].first;
    }

    note.setTagLocalUids(tagLocalUids);

    // Setting tag guids

    int numTagGuids = tagGuidsAndIndices.size();
    QList<QPair<QString, int> > tagGuidIndexPairs;
    tagGuidIndexPairs.reserve(std::max(numTagGuids, 0));
    for(auto it = tagGuidsAndIndices.begin(), end = tagGuidsAndIndices.end(); it != end; ++it) {
        tagGuidIndexPairs << QPair<QString, int>(it.value(), it.key());
    }

    qSort(tagGuidIndexPairs.begin(), tagGuidIndexPairs.end(), QStringIntPairCompareByInt());
    QStringList tagGuids;
    tagGuids.reserve(std::max(numTagGuids, 0));
    for(int i = 0; i < numTagGuids; ++i)
    {
        const QString & guid = tagGuidIndexPairs[i].first;
        if (guid.isEmpty()) {
            continue;
        }

        tagGuids << guid;
    }

    note.setTagGuids(tagGuids);

    return true;
}

bool LocalStorageManagerPrivate::findAndSetResourcesPerNote(Note & note, ErrorString & errorDescription,
                                                            const bool withBinaryData) const
{
    ErrorString errorPrefix(QT_TRANSLATE_NOOP("", "can't find resources for note"));

    const QString noteLocalUid = note.localUid();

    // NOTE: it's weird but I can only get this query work as intended,
    // any more specific ones trying to pick the resource for note's local uid fail miserably.
    // I've just spent some hours of my life trying to figure out what the hell is going on here
    // but the best I was able to do is this. Please be very careful if you think you can do better here...
    QString queryString = QString(QStringLiteral("SELECT * FROM NoteResources"));
    QSqlQuery query(m_sqlDatabase);
    bool res = query.exec(queryString);
    DATABASE_CHECK_AND_SET_ERROR();

    QStringList resourceLocalUids;
    while(query.next())
    {
        QSqlRecord rec = query.record();
        int index = rec.indexOf(QStringLiteral("localNote"));
        if (index >= 0)
        {
            QVariant value = rec.value(index);
            if (value.isNull()) {
                continue;
            }

            QString foundNoteLocalUid = value.toString();

            int resourceIndex = rec.indexOf(QStringLiteral("localResource"));
            if ((foundNoteLocalUid == noteLocalUid) && (resourceIndex >= 0))
            {
                value = rec.value(resourceIndex);
                if (value.isNull()) {
                    continue;
                }

                QString resourceLocalUid = value.toString();
                resourceLocalUids << resourceLocalUid;
                QNDEBUG(QStringLiteral("Found resource's local uid: ") << resourceLocalUid);
            }
        }
    }

    int numResources = resourceLocalUids.size();
    QNDEBUG(QStringLiteral("Found ") << numResources << QStringLiteral(" resources"));

    ErrorString error;
    QList<Resource> resources;
    resources.reserve(std::max(numResources, 0));
    for(auto it = resourceLocalUids.begin(), end = resourceLocalUids.end(); it != end; ++it)
    {
        const QString & resourceLocalUid = *it;

        resources << Resource();
        Resource & resource = resources.back();
        resource.setLocalUid(resourceLocalUid);

        error.clear();
        bool res = findEnResource(resource, error, withBinaryData);
        if (Q_UNLIKELY(!res)) {
            errorDescription.base() = errorPrefix.base();
            errorDescription.additionalBases().append(error.base());
            errorDescription.additionalBases().append(error.additionalBases());
            errorDescription.details() = error.details();
            QNWARNING(errorDescription);
            return false;
        }

        QNDEBUG(QStringLiteral("Found resource with local uid ") << resource.localUid()
                << QStringLiteral(" for note with local uid ") << noteLocalUid);
    }

    qSort(resources.begin(), resources.end(), ResourceCompareByIndex());
    note.setResources(resources);

    return true;
}

void LocalStorageManagerPrivate::sortSharedNotebooks(Notebook & notebook) const
{
    if (!notebook.hasSharedNotebooks()) {
        return;
    }

    // Sort shared notebooks to ensure the correct order for proper work of comparison operators
    QList<SharedNotebook> sharedNotebooks = notebook.sharedNotebooks();

    qSort(sharedNotebooks.begin(), sharedNotebooks.end(), SharedNotebookCompareByIndex());
}

void LocalStorageManagerPrivate::sortSharedNotes(Note & note) const
{
    if (!note.hasSharedNotes()) {
        return;
    }

    // Sort shared notes to ensure the correct order for proper work of comparison oeprators
    QList<SharedNote> sharedNotes = note.sharedNotes();
    qSort(sharedNotes.begin(), sharedNotes.end(), SharedNoteCompareByIndex());
    note.setSharedNotes(sharedNotes);
}

bool LocalStorageManagerPrivate::noteSearchQueryToSQL(const NoteSearchQuery & noteSearchQuery,
                                                      QString & sql, ErrorString & errorDescription) const
{
    ErrorString errorPrefix(QT_TRANSLATE_NOOP("", "can't convert note search query string into SQL query"));

    // 1) ============ Setting up initial templates
    QString sqlPrefix = QStringLiteral("SELECT DISTINCT localUid ");
    sql.resize(0);

    // 2) ============ Determining whether "any:" modifier takes effect ==============

    bool queryHasAnyModifier = noteSearchQuery.hasAnyModifier();
    QString uniteOperator = (queryHasAnyModifier ? QStringLiteral("OR") : QStringLiteral("AND"));

    // 3) ============ Processing notebook modifier (if present) ==============

    QString notebookName = noteSearchQuery.notebookModifier();
    QString notebookLocalUid;
    if (!notebookName.isEmpty())
    {
        QSqlQuery query(m_sqlDatabase);
        QString notebookQueryString = QString("SELECT localUid FROM NotebookFTS WHERE notebookName MATCH '%1' LIMIT 1").arg(sqlEscapeString(notebookName));
        bool res = query.exec(notebookQueryString);
        DATABASE_CHECK_AND_SET_ERROR();

        if (Q_UNLIKELY(!query.next())) {
            errorDescription.base() = errorPrefix.base();
            errorDescription.additionalBases().append(QT_TRANSLATE_NOOP("", "notebook with the provided name was not found"));
            return false;
        }

        QSqlRecord rec = query.record();
        int index = rec.indexOf(QStringLiteral("localUid"));
        if (Q_UNLIKELY(index < 0)) {
            errorDescription.base() = errorPrefix.base();
            errorDescription.additionalBases().append(QT_TRANSLATE_NOOP("", "can't find notebook's local uid by notebook name: "
                                                                        "SQL query record doesn't contain the requested item"));
            return false;
        }

        QVariant value = rec.value(index);
        if (Q_UNLIKELY(value.isNull())) {
            errorDescription.base() = errorPrefix.base();
            errorDescription.additionalBases().append(QT_TRANSLATE_NOOP("", "found null notebook's local uid corresponding to notebook's name"));
            return false;
        }

        notebookLocalUid = value.toString();
        if (Q_UNLIKELY(notebookLocalUid.isEmpty())) {
            errorDescription.base() = errorPrefix.base();
            errorDescription.additionalBases().append(QT_TRANSLATE_NOOP("", "found empty notebook's local uid corresponding to notebook's name"));
            return false;
        }
    }

    if (!notebookLocalUid.isEmpty()) {
        sql += QStringLiteral("(notebookLocalUid = '");
        sql += sqlEscapeString(notebookLocalUid);
        sql += QStringLiteral("') AND ");
    }

    // 4) ============ Processing tag names and negated tag names, if any =============

    if (noteSearchQuery.hasAnyTag())
    {
        sql += QStringLiteral("(NoteTags.localTag IS NOT NULL) ");
        sql += uniteOperator;
        sql += QStringLiteral(" ");
    }
    else if (noteSearchQuery.hasNegatedAnyTag())
    {
        sql += QStringLiteral("(NoteTags.localTag IS NULL) ");
        sql += uniteOperator;
        sql += QStringLiteral(" ");
    }
    else
    {
        QStringList tagLocalUids;
        QStringList tagNegatedLocalUids;

        const QStringList & tagNames = noteSearchQuery.tagNames();
        if (!tagNames.isEmpty())
        {
            ErrorString error;
            bool res = tagNamesToTagLocalUids(tagNames, tagLocalUids, error);
            if (!res) {
                errorDescription.base() = errorPrefix.base();
                errorDescription.additionalBases().append(error.base());
                errorDescription.additionalBases().append(error.additionalBases());
                errorDescription.details() = error.details();
                QNWARNING(errorDescription);
                return false;
            }
        }

        if (!tagLocalUids.isEmpty())
        {
            if (!queryHasAnyModifier)
            {
                // In successful note search query there are exactly as many tag local uids
                // as there are tag names; therefore, when the search is for notes with
                // some particular tags, we need to ensure that each note's local uid
                // in the sub-query result is present there exactly as many times
                // as there are tag local uids in the query which the note is labeled with

                const int numTagLocalUids = tagLocalUids.size();
                sql += QStringLiteral("(NoteTags.localNote IN (SELECT localNote FROM (SELECT localNote, localTag, COUNT(*) "
                                      "FROM NoteTags WHERE NoteTags.localTag IN ('");
                for(auto it = tagLocalUids.constBegin(), end = tagLocalUids.constEnd(); it != end; ++it) {
                    sql += sqlEscapeString(*it);
                    sql += QStringLiteral("', '");
                }
                sql.chop(3);    // remove trailing comma, whitespace and single quotation mark

                sql += QStringLiteral(") GROUP BY localNote HAVING COUNT(*)=");
                sql += QString::number(numTagLocalUids);
                sql += QStringLiteral("))) ");
            }
            else
            {
                // With "any:" modifier the search doesn't care about the exactness of tag-to-note map,
                // it would instead pick just any note corresponding to any of requested tags at least once

                sql += QStringLiteral("(NoteTags.localNote IN (SELECT localNote FROM (SELECT localNote, localTag "
                                      "FROM NoteTags WHERE NoteTags.localTag IN ('");
                for(auto it = tagLocalUids.constBegin(), end = tagLocalUids.constEnd(); it != end; ++it) {
                    sql += sqlEscapeString(*it);
                    sql += QStringLiteral("', '");
                }
                sql.chop(3);    // remove trailing comma, whitespace and single quotation mark

                sql += QStringLiteral(")))) ");
            }

            sql += uniteOperator;
            sql += QStringLiteral(" ");
        }

        const QStringList & negatedTagNames = noteSearchQuery.negatedTagNames();
        if (!negatedTagNames.isEmpty())
        {
            ErrorString error;
            bool res = tagNamesToTagLocalUids(negatedTagNames, tagNegatedLocalUids, error);
            if (!res) {
                errorDescription.base() = errorPrefix.base();
                errorDescription.additionalBases().append(error.base());
                errorDescription.additionalBases().append(error.additionalBases());
                errorDescription.details() = error.details();
                QNWARNING(errorDescription);
                return false;
            }
        }

        if (!tagNegatedLocalUids.isEmpty())
        {
            if (!queryHasAnyModifier)
            {
                // First find all notes' local uids which actually correspond to negated tags' local uids;
                // then simply negate that condition

                const int numTagNegatedLocalUids = tagNegatedLocalUids.size();
                sql += QStringLiteral("(NoteTags.localNote NOT IN (SELECT localNote FROM (SELECT localNote, localTag, COUNT(*) "
                                      "FROM NoteTags WHERE NoteTags.localTag IN ('");
                for(auto it = tagNegatedLocalUids.constBegin(), end = tagNegatedLocalUids.constEnd(); it != end; ++it) {
                    sql += sqlEscapeString(*it);
                    sql += QStringLiteral("', '");
                }
                sql.chop(3);    // remove trailing comma, whitespace and single quotation mark

                sql += QStringLiteral(") GROUP BY localNote HAVING COUNT(*)=");
                sql += QString::number(numTagNegatedLocalUids);

                // Don't forget to account for the case of no tags used for note
                // so it's not even present in NoteTags table

                sql += QStringLiteral(")) OR (NoteTags.localNote IS NULL)) ");
            }
            else
            {
                // With "any:" modifier the search doesn't care about the exactness of tag-to-note map,
                // it would instead pick just any note not from the list of notes corresponding to
                // any of requested tags at least once

                sql += QStringLiteral("(NoteTags.localNote NOT IN (SELECT localNote FROM (SELECT localNote, localTag "
                                      "FROM NoteTags WHERE NoteTags.localTag IN ('");
                for(auto it = tagNegatedLocalUids.constBegin(), end = tagNegatedLocalUids.constEnd(); it != end; ++it) {
                    sql += sqlEscapeString(*it);
                    sql += QStringLiteral("', '");
                }
                sql.chop(3);    // remove trailing comma, whitespace and single quotation mark

                // Don't forget to account for the case of no tags used for note
                // so it's not even present in NoteTags table

                sql += QStringLiteral("))) OR (NoteTags.localNote IS NULL)) ");
            }

            sql += uniteOperator;
            sql += QStringLiteral(" ");
        }
    }

    // 5) ============== Processing resource mime types ===============

    if (noteSearchQuery.hasAnyResourceMimeType())
    {
        sql += QStringLiteral("(NoteResources.localResource IS NOT NULL) ");
        sql += uniteOperator;
        sql += QStringLiteral(" ");
    }
    else if (noteSearchQuery.hasNegatedAnyResourceMimeType())
    {
        sql += QStringLiteral("(NoteResources.localResource IS NULL) ");
        sql += uniteOperator;
        sql += QStringLiteral(" ");
    }
    else
    {
        QStringList resourceLocalUidsPerMime;
        QStringList resourceNegatedLocalUidsPerMime;

        const QStringList & resourceMimeTypes = noteSearchQuery.resourceMimeTypes();
        const int numResourceMimeTypes = resourceMimeTypes.size();
        if (!resourceMimeTypes.isEmpty())
        {
            ErrorString error;
            bool res = resourceMimeTypesToResourceLocalUids(resourceMimeTypes, resourceLocalUidsPerMime, error);
            if (!res) {
                errorDescription.base() = errorPrefix.base();
                errorDescription.additionalBases().append(error.base());
                errorDescription.additionalBases().append(error.additionalBases());
                errorDescription.details() = error.details();
                QNWARNING(errorDescription);
                return false;
            }
        }

        if (!resourceLocalUidsPerMime.isEmpty())
        {
            if (!queryHasAnyModifier)
            {
                // Need to find notes which each have all the found resource local uids

                // One resource mime type can correspond to multiple resources. However,
                // one resource corresponds to exactly one note. When searching for notes
                // which resources have particular mime type, we need to ensure that each note's
                // local uid in the sub-query result is present there exactly as many times
                // as there are resource mime types in the query

                sql += QStringLiteral("(NoteResources.localNote IN (SELECT localNote FROM (SELECT localNote, localResource, COUNT(*) "
                                      "FROM NoteResources WHERE NoteResources.localResource IN ('");
                for(auto it = resourceLocalUidsPerMime.constBegin(), end = resourceLocalUidsPerMime.constEnd(); it != end; ++it) {
                    sql += sqlEscapeString(*it);
                    sql += QStringLiteral("', '");
                }
                sql.chop(3);    // remove trailing comma, whitespace and single quotation mark

                sql += QStringLiteral(") GROUP BY localNote HAVING COUNT(*)=");
                sql += QString::number(numResourceMimeTypes);
                sql += QStringLiteral("))) ");
            }
            else
            {
                // With "any:" modifier the search doesn't care about the exactness of resource mime type-to-note map,
                // it would instead pick just any note having at least one resource with requested mime type

                sql += QStringLiteral("(NoteResources.localNote IN (SELECT localNote FROM (SELECT localNote, localResource "
                                      "FROM NoteResources WHERE NoteResources.localResource IN ('");
                for(auto it = resourceLocalUidsPerMime.constBegin(), end = resourceLocalUidsPerMime.constEnd(); it != end; ++it) {
                    sql += sqlEscapeString(*it);
                    sql += QStringLiteral("', '");
                }
                sql.chop(3);    // remove trailing comma, whitespace and single quotation mark

                sql += QStringLiteral(")))) ");
            }

            sql += uniteOperator;
            sql += QStringLiteral(" ");
        }

        const QStringList & negatedResourceMimeTypes = noteSearchQuery.negatedResourceMimeTypes();
        const int numNegatedResourceMimeTypes = negatedResourceMimeTypes.size();
        if (!negatedResourceMimeTypes.isEmpty())
        {
            ErrorString error;
            bool res = resourceMimeTypesToResourceLocalUids(negatedResourceMimeTypes, resourceNegatedLocalUidsPerMime, error);
            if (!res) {
                errorDescription.base() = errorPrefix.base();
                errorDescription.additionalBases().append(error.base());
                errorDescription.additionalBases().append(error.additionalBases());
                errorDescription.details() = error.details();
                QNWARNING(errorDescription);
                return false;
            }
        }

        if (!resourceNegatedLocalUidsPerMime.isEmpty())
        {
            if (!queryHasAnyModifier)
            {
                sql += QStringLiteral("(NoteResources.localNote NOT IN (SELECT localNote FROM (SELECT localNote, localResource, COUNT(*) "
                                      "FROM NoteResources WHERE NoteResources.localResource IN ('");
                for(auto it = resourceNegatedLocalUidsPerMime.constBegin(), end = resourceNegatedLocalUidsPerMime.constEnd(); it != end; ++it) {
                    sql += sqlEscapeString(*it);
                    sql += QStringLiteral("', '");
                }
                sql.chop(3);    // remove trailing comma, whitespace and single quotation mark

                sql += QStringLiteral(") GROUP BY localNote HAVING COUNT(*)=");
                sql += QString::number(numNegatedResourceMimeTypes);

                // Don't forget to account for the case of no resources existing in the note
                // so it's not even present in NoteResources table

                sql += QStringLiteral(")) OR (NoteResources.localNote IS NULL)) ");
            }
            else
            {
                sql += QStringLiteral("(NoteResources.localNote NOT IN (SELECT localNote FROM (SELECT localNote, localResource "
                                      "FROM NoteResources WHERE NoteResources.localResource IN ('");
                for(auto it = resourceNegatedLocalUidsPerMime.constBegin(), end = resourceNegatedLocalUidsPerMime.constEnd(); it != end; ++it) {
                    sql += sqlEscapeString(*it);
                    sql += QStringLiteral("', '");
                }
                sql.chop(3);    // remove trailing comma, whitespace and single quotation mark

                // Don't forget to account for the case of no resources existing in the note
                // so it's not even present in NoteResources table

                sql += QStringLiteral("))) OR (NoteResources.localNote IS NULL)) ");
            }

            sql += uniteOperator;
            sql += QStringLiteral(" ");
        }
    }

    // 6) ============== Processing other better generalizable filters =============

#define CHECK_AND_PROCESS_ANY_ITEM(hasAnyItem, hasNegatedAnyItem, column) \
    if (noteSearchQuery.hasAnyItem()) { \
        sql += QStringLiteral("(NoteFTS." #column " IS NOT NULL) "); \
        sql += uniteOperator; \
        sql += QStringLiteral(" "); \
    } \
    else if (noteSearchQuery.hasNegatedAnyItem()) { \
        sql += QStringLiteral("(NoteFTS." #column " IS NULL) "); \
        sql += uniteOperator; \
        sql += QStringLiteral(" "); \
    }

#define CHECK_AND_PROCESS_LIST(list, column, negated, ...) \
    const auto & noteSearchQuery##list##column = noteSearchQuery.list(); \
    if (!noteSearchQuery##list##column.isEmpty()) \
    { \
        sql += QStringLiteral("("); \
        for(auto it = noteSearchQuery##list##column.constBegin(), end = noteSearchQuery##list##column.constEnd(); it != end; ++it) \
        { \
            const auto & item = *it; \
            if (negated) { \
                sql += QStringLiteral("(localUid NOT IN "); \
            } \
            else { \
                sql += QStringLiteral("(localUid IN "); \
            } \
            \
            sql += QStringLiteral("(SELECT localUid FROM NoteFTS WHERE NoteFTS." #column " MATCH \'"); \
            sql += sqlEscapeString(__VA_ARGS__(item)); \
            sql += QStringLiteral("\')) "); \
            sql += uniteOperator; \
            sql += QStringLiteral(" "); \
        } \
        sql.chop(uniteOperator.length() + 1); \
        sql += QStringLiteral(")"); \
        sql += uniteOperator; \
        sql += QStringLiteral(" "); \
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
            sql += QStringLiteral("(localUid IN (SELECT localUid FROM Notes WHERE Notes." #column); \
            if (negated) { \
                sql += QStringLiteral(" < "); \
            } \
            else { \
                sql += QStringLiteral(" >= "); \
            } \
            sql += sqlEscapeString(__VA_ARGS__(*it)); \
            sql += QStringLiteral(")) "); \
            sql += uniteOperator; \
            sql += QStringLiteral(" "); \
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
        sql += QStringLiteral("((NoteFTS.contentContainsFinishedToDo IS 1) OR (NoteFTS.contentContainsUnfinishedToDo IS 1)) ");
        sql += uniteOperator;
        sql += QStringLiteral(" ");
    }
    else if (noteSearchQuery.hasNegatedAnyToDo())
    {
        sql += QStringLiteral("((NoteFTS.contentContainsFinishedToDo IS 0) OR (NoteFTS.contentContainsFinishedToDo IS NULL)) AND "
                              "((NoteFTS.contentContainsUnfinishedToDo IS 0) OR (NoteFTS.contentContainsUnfinishedToDo IS NULL)) ");
        sql += uniteOperator;
        sql += QStringLiteral(" ");
    }
    else
    {
        if (noteSearchQuery.hasFinishedToDo()) {
            sql += QStringLiteral("(NoteFTS.contentContainsFinishedToDo IS 1) ");
            sql += uniteOperator;
            sql += QStringLiteral(" ");
        }
        else if (noteSearchQuery.hasNegatedFinishedToDo()) {
            sql += QStringLiteral("((NoteFTS.contentContainsFinishedToDo IS 0) OR (NoteFTS.contentContainsFinishedToDo IS NULL)) ");
            sql += uniteOperator;
            sql += QStringLiteral(" ");
        }

        if (noteSearchQuery.hasUnfinishedToDo()) {
            sql += QStringLiteral("(NoteFTS.contentContainsUnfinishedToDo IS 1) ");
            sql += uniteOperator;
            sql += QStringLiteral(" ");
        }
        else if (noteSearchQuery.hasNegatedUnfinishedToDo()) {
            sql += QStringLiteral("((NoteFTS.contentContainsUnfinishedToDo IS 0) OR (NoteFTS.contentContainsUnfinishedToDo IS NULL)) ");
            sql += uniteOperator;
            sql += QStringLiteral(" ");
        }
    }

    // 8) ============== Processing encryption item =================

    if (noteSearchQuery.hasNegatedEncryption()) {
        sql += QStringLiteral("((NoteFTS.contentContainsEncryption IS 0) OR (NoteFTS.contentContainsEncryption IS NULL)) ");
        sql += uniteOperator;
        sql += QStringLiteral(" ");
    }
    else if (noteSearchQuery.hasEncryption()) {
        sql += QStringLiteral("(NoteFTS.contentContainsEncryption IS 1) ");
        sql += uniteOperator;
        sql += QStringLiteral(" ");
    }

    // 9) ============== Processing content search terms =================

    if (noteSearchQuery.hasAnyContentSearchTerms())
    {
        ErrorString error;
        QString contentSearchTermsSqlQueryPart;
        bool res = noteSearchQueryContentSearchTermsToSQL(noteSearchQuery, contentSearchTermsSqlQueryPart, error);
        if (!res) {
            errorDescription.base() = errorPrefix.base();
            errorDescription.additionalBases().append(error.base());
            errorDescription.additionalBases().append(error.additionalBases());
            errorDescription.details() = error.details();
            QNWARNING(errorDescription);
            return false;
        }

        sql += contentSearchTermsSqlQueryPart;
        sql += uniteOperator;
        sql += QStringLiteral(" ");
    }

    // 10) ============== Removing potentially left trailing unite operator from the SQL string =============

    QString spareEnd = uniteOperator + QStringLiteral(" ");
    if (sql.endsWith(spareEnd)) {
        sql.chop(spareEnd.size());
    }

    // 11) ============= See whether we should bother anything regarding tags or resources ============

    QString sqlPostfix = QStringLiteral("FROM NoteFTS ");
    if (sql.contains(QStringLiteral("NoteTags"))) {
        sqlPrefix += QStringLiteral(", NoteTags.localTag ");
        sqlPostfix += QStringLiteral("LEFT OUTER JOIN NoteTags ON NoteFTS.localUid = NoteTags.localNote ");
    }

    if (sql.contains(QStringLiteral("NoteResources"))) {
        sqlPrefix += QStringLiteral(", NoteResources.localResource ");
        sqlPostfix += QStringLiteral("LEFT OUTER JOIN NoteResources ON NoteFTS.localUid = NoteResources.localNote ");
    }

    // 12) ============== Finalize the query composed of parts ===============

    sqlPrefix += sqlPostfix;
    sqlPrefix += QStringLiteral("WHERE ");
    sql.prepend(sqlPrefix);

    QNTRACE(QStringLiteral("Prepared SQL query for note search: ") << sql);
    return true;
}

bool LocalStorageManagerPrivate::noteSearchQueryContentSearchTermsToSQL(const NoteSearchQuery & noteSearchQuery,
                                                                        QString & sql, ErrorString & errorDescription) const
{
    QNDEBUG(QStringLiteral("LocalStorageManagerPrivate::noteSearchQueryContentSearchTermsToSQL"));

    if (!noteSearchQuery.hasAnyContentSearchTerms()) {
        errorDescription.base() = QT_TRANSLATE_NOOP("", "note search query has no advanced search modifiers and no content search terms");
        errorDescription.details() = noteSearchQuery.queryString();
        QNWARNING(errorDescription);
        return false;
    }

    sql.resize(0);

    bool queryHasAnyModifier = noteSearchQuery.hasAnyModifier();
    QString uniteOperator = (queryHasAnyModifier ? QStringLiteral("OR") : QStringLiteral("AND"));

    QString positiveSqlPart;
    QString negatedSqlPart;

    QString matchStatement;
    matchStatement.reserve(5);

    QString frontSearchTermModifier;
    frontSearchTermModifier.reserve(1);

    QString backSearchTermModifier;
    backSearchTermModifier.reserve(1);

    QString currentSearchTerm;

    const QStringList & contentSearchTerms = noteSearchQuery.contentSearchTerms();
    if (!contentSearchTerms.isEmpty())
    {
        const int numContentSearchTerms = contentSearchTerms.size();
        for(int i = 0; i < numContentSearchTerms; ++i)
        {
            currentSearchTerm = contentSearchTerms[i];
            m_stringUtils.removePunctuation(currentSearchTerm, m_preservedAsterisk);
            if (currentSearchTerm.isEmpty()) {
                continue;
            }

            m_stringUtils.removeDiacritics(currentSearchTerm);

            positiveSqlPart += QStringLiteral("(");
            contentSearchTermToSQLQueryPart(frontSearchTermModifier, currentSearchTerm, backSearchTermModifier, matchStatement);
            currentSearchTerm = sqlEscapeString(currentSearchTerm);
            positiveSqlPart += QString("(localUid IN (SELECT localUid FROM NoteFTS WHERE contentListOfWords %1 '%2%3%4')) OR "
                                       "(localUid IN (SELECT localUid FROM NoteFTS WHERE titleNormalized %1 '%2%3%4')) OR "
                                       "(localUid IN (SELECT noteLocalUid FROM ResourceRecognitionDataFTS WHERE recognitionData %1 '%2%3%4')) OR "
                                       "(localUid IN (SELECT localNote FROM NoteTags LEFT OUTER JOIN TagFTS ON NoteTags.localTag=TagFTS.localUid WHERE "
                                       "(nameLower IN (SELECT nameLower FROM TagFTS WHERE nameLower %1 '%2%3%4'))))")
                               .arg(matchStatement, frontSearchTermModifier, currentSearchTerm, backSearchTermModifier);
            positiveSqlPart += QStringLiteral(")");

            if (i != (numContentSearchTerms - 1)) {
                positiveSqlPart += QStringLiteral(" ") + uniteOperator + QStringLiteral(" ");
            }
        }

        if (!positiveSqlPart.isEmpty()) {
            positiveSqlPart.prepend(QStringLiteral("("));
            positiveSqlPart += QStringLiteral(")");
        }
    }

    const QStringList & negatedContentSearchTerms = noteSearchQuery.negatedContentSearchTerms();
    if (!negatedContentSearchTerms.isEmpty())
    {
        const int numNegatedContentSearchTerms = negatedContentSearchTerms.size();
        for(int i = 0; i < numNegatedContentSearchTerms; ++i)
        {
            currentSearchTerm = negatedContentSearchTerms[i];
            m_stringUtils.removePunctuation(currentSearchTerm, m_preservedAsterisk);
            if (currentSearchTerm.isEmpty()) {
                continue;
            }

            m_stringUtils.removeDiacritics(currentSearchTerm);

            negatedSqlPart += QStringLiteral("(");
            contentSearchTermToSQLQueryPart(frontSearchTermModifier, currentSearchTerm, backSearchTermModifier, matchStatement);
            currentSearchTerm = sqlEscapeString(currentSearchTerm);
            negatedSqlPart += QString("(localUid NOT IN (SELECT localUid FROM NoteFTS WHERE contentListOfWords %1 '%2%3%4')) AND "
                                      "(localUid NOT IN (SELECT localUid FROM NoteFTS WHERE titleNormalized %1 '%2%3%4')) AND "
                                      "(localUid NOT IN (SELECT noteLocalUid FROM ResourceRecognitionDataFTS WHERE recognitionData %1 '%2%3%4')) AND "
                                      "(localUid NOT IN (SELECT localNote FROM NoteTags LEFT OUTER JOIN TagFTS on NoteTags.localTag=TagFTS.localUid WHERE "
                                      "(nameLower IN (SELECT nameLower FROM TagFTS WHERE nameLower %1 '%2%3%4'))))")
                              .arg(matchStatement, frontSearchTermModifier, currentSearchTerm, backSearchTermModifier);
            negatedSqlPart += QStringLiteral(")");

            if (i != (numNegatedContentSearchTerms - 1)) {
                negatedSqlPart += QStringLiteral(" ") + uniteOperator + QStringLiteral(" ");
            }
        }

        if (!negatedSqlPart.isEmpty()) {
            negatedSqlPart.prepend(QStringLiteral("("));
            negatedSqlPart += QStringLiteral(")");
        }
    }

    // ======= First append all positive terms of the query =======
    if (!positiveSqlPart.isEmpty()) {
        sql += QStringLiteral("(") + positiveSqlPart + QStringLiteral(") ");
    }

    // ========== Now append all negative parts of the query (if any) =========

    if (!negatedSqlPart.isEmpty())
    {
        if (!positiveSqlPart.isEmpty()) {
            sql += QStringLiteral(" ") + uniteOperator + QStringLiteral(" ");
        }

        sql += QStringLiteral("(") + negatedSqlPart + QStringLiteral(")");
    }

    return true;
}

void LocalStorageManagerPrivate::contentSearchTermToSQLQueryPart(QString & frontSearchTermModifier, QString & searchTerm,
                                                                 QString & backSearchTermModifier, QString & matchStatement) const
{
    QRegExp whitespaceRegex(QStringLiteral("\\p{Z}"));

    if ((whitespaceRegex.indexIn(searchTerm) >= 0) || (searchTerm.contains('*') && !searchTerm.endsWith('*')))
    {
        // FTS "MATCH" clause doesn't work for phrased search or search with asterisk somewhere but the end of the search term,
        // need to use the slow "LIKE" clause instead
        matchStatement = QStringLiteral("LIKE");

        while(searchTerm.startsWith('*')) {
            searchTerm.remove(0, 1);
        }

        while(searchTerm.endsWith('*')) {
            searchTerm.chop(1);
        }

        frontSearchTermModifier = QStringLiteral("%");
        backSearchTermModifier = QStringLiteral("%");

        int pos = -1;
        while((pos = searchTerm.indexOf('*')) >= 0) {
            searchTerm.replace(pos, 1, '%');
        }
    }
    else
    {
        matchStatement = QStringLiteral("MATCH");
        frontSearchTermModifier = QStringLiteral("");
        backSearchTermModifier = QStringLiteral("");
    }
}

bool LocalStorageManagerPrivate::tagNamesToTagLocalUids(const QStringList & tagNames,
                                                        QStringList & tagLocalUids,
                                                        ErrorString & errorDescription) const
{
    ErrorString errorPrefix(QT_TRANSLATE_NOOP("", "can't get tag local uids for tag names"));

    tagLocalUids.clear();

    QSqlQuery query(m_sqlDatabase);

    QString queryString;

    bool singleTagName = (tagNames.size() == 1);
    if (singleTagName)
    {
        bool res = query.prepare(QStringLiteral("SELECT localUid FROM TagFTS WHERE nameLower MATCH :names"));
        DATABASE_CHECK_AND_SET_ERROR();

        QString names = tagNames.at(0).toLower();
        names.prepend(QStringLiteral("\'"));
        names.append(QStringLiteral("\'"));
        query.bindValue(QStringLiteral(":names"), names);
    }
    else
    {
        bool someTagNameHasWhitespace = false;
        for(auto it = tagNames.constBegin(), end = tagNames.constEnd(); it != end; ++it)
        {
            const QString & tagName = *it;
            if (tagName.contains(QStringLiteral(" "))) {
                someTagNameHasWhitespace = true;
                break;
            }
        }

        if (someTagNameHasWhitespace)
        {
            // Unfortunately, stardard SQLite at least from Qt 4.x has standard query syntax for FTS
            // which does not support whitespaces in search terms and therefore MATCH function is simply inapplicable here,
            // have to use brute-force "equal to X1 or equal to X2 or ... equal to XN
            queryString = QStringLiteral("SELECT localUid FROM Tags WHERE ");

            for(auto it = tagNames.constBegin(), end = tagNames.constEnd(); it != end; ++it) {
                const QString & tagName = *it;
                queryString += QStringLiteral("(nameLower = \'");
                queryString += sqlEscapeString(tagName.toLower());
                queryString += QStringLiteral("\') OR ");
            }
            queryString.chop(4);    // remove trailing " OR "
        }
        else
        {
            queryString = QStringLiteral("SELECT localUid FROM TagFTS WHERE ");

            for(auto it = tagNames.constBegin(), end = tagNames.constEnd(); it != end; ++it) {
                const QString & tagName = *it;
                queryString += QStringLiteral("(localUid IN (SELECT localUid FROM TagFTS WHERE nameLower MATCH \'");
                queryString += sqlEscapeString(tagName.toLower());
                queryString += QStringLiteral("\')) OR ");
            }
            queryString.chop(4);    // remove trailing " OR "
        }
    }

    bool res = false;
    if (queryString.isEmpty()) {
        res = query.exec();
    }
    else {
        res = query.exec(queryString);
    }
    DATABASE_CHECK_AND_SET_ERROR();

    while (query.next())
    {
        QSqlRecord rec = query.record();
        int index = rec.indexOf(QStringLiteral("localUid"));
        if (Q_UNLIKELY(index < 0)) {
            errorDescription.base() = errorPrefix.base();
            errorDescription.additionalBases().append(QT_TRANSLATE_NOOP("", "tag's local uid is not present in the result of SQL query"));
            QNWARNING(errorDescription);
            return false;
        }

        QVariant value = rec.value(index);
        tagLocalUids << value.toString();
    }

    return true;
}

bool LocalStorageManagerPrivate::resourceMimeTypesToResourceLocalUids(const QStringList & resourceMimeTypes,
                                                                      QStringList & resourceLocalUids,
                                                                      ErrorString & errorDescription) const
{
    ErrorString errorPrefix(QT_TRANSLATE_NOOP("", "can't get resource mime types for resource local uids"));

    resourceLocalUids.clear();

    QSqlQuery query(m_sqlDatabase);

    QString queryString;

    bool singleMimeType = (resourceMimeTypes.size() == 1);
    if (singleMimeType)
    {
        bool res = query.prepare(QStringLiteral("SELECT resourceLocalUid FROM ResourceMimeFTS WHERE mime MATCH :mimeTypes"));
        DATABASE_CHECK_AND_SET_ERROR();

        QString mimeTypes = resourceMimeTypes.at(0);
        mimeTypes.prepend(QStringLiteral("\'"));
        mimeTypes.append(QStringLiteral("\'"));
        query.bindValue(QStringLiteral(":mimeTypes"), mimeTypes);
    }
    else
    {
        bool someMimeTypeHasWhitespace = false;
        for(auto it = resourceMimeTypes.constBegin(), end = resourceMimeTypes.constEnd(); it != end; ++it)
        {
            const QString & mimeType = *it;
            if (mimeType.contains(QStringLiteral(" "))) {
                someMimeTypeHasWhitespace = true;
                break;
            }
        }

        if (someMimeTypeHasWhitespace)
        {
            // Unfortunately, stardard SQLite at least from Qt 4.x has standard query syntax for FTS
            // which does not support whitespaces in search terms and therefore MATCH function is simply inapplicable here,
            // have to use brute-force "equal to X1 or equal to X2 or ... equal to XN
            queryString = QStringLiteral("SELECT resourceLocalUid FROM Resources WHERE ");

            for(auto it = resourceMimeTypes.constBegin(), end = resourceMimeTypes.constEnd(); it != end; ++it) {
                const QString & mimeType = *it;
                queryString += QStringLiteral("(mime = \'");
                queryString += sqlEscapeString(mimeType);
                queryString += QStringLiteral("\') OR ");
            }
            queryString.chop(4);    // remove trailing OR and two whitespaces
        }
        else
        {
            // For some reason statements like "MATCH 'x OR y'" don't work while
            // "SELECT ... MATCH 'x' UNION SELECT ... MATCH 'y'" work.

            for(auto it = resourceMimeTypes.begin(), end = resourceMimeTypes.end(); it != end; ++it) {
                const QString & mimeType = *it;
                queryString += QStringLiteral("SELECT resourceLocalUid FROM ResourceMimeFTS WHERE mime MATCH \'");
                queryString += sqlEscapeString(mimeType);
                queryString += QStringLiteral("\' UNION ");
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
    DATABASE_CHECK_AND_SET_ERROR();

    while (query.next())
    {
        QSqlRecord rec = query.record();
        int index = rec.indexOf(QStringLiteral("resourceLocalUid"));
        if (Q_UNLIKELY(index < 0)) {
            errorDescription.base() = errorPrefix.base();
            errorDescription.additionalBases().append(QT_TRANSLATE_NOOP("", "resource's local uid is not present in the result of SQL query"));
            return false;
        }

        QVariant value = rec.value(index);
        resourceLocalUids << value.toString();
    }

    return true;
}

bool LocalStorageManagerPrivate::complementResourceNoteIds(Resource & resource, ErrorString & errorDescription) const
{
    ErrorString errorPrefix(QT_TRANSLATE_NOOP("", "can't complement resource note ids"));

    if (!resource.hasNoteGuid())
    {
        QString noteLocalUid = sqlEscapeString(resource.noteLocalUid());
        QString queryString = QString("SELECT guid FROM Notes WHERE localUid = '%1'").arg(noteLocalUid);
        QSqlQuery query(m_sqlDatabase);
        bool res = query.exec(queryString);
        DATABASE_CHECK_AND_SET_ERROR();

        if (query.next()) {
            resource.setNoteGuid(query.record().value(QStringLiteral("guid")).toString());
        }
    }
    else if (!resource.hasNoteLocalUid())
    {
        QString noteGuid = sqlEscapeString(resource.noteGuid());
        QString queryString = QString("SELECT localUid FROM Notes WHERE guid = '%1'").arg(noteGuid);
        QSqlQuery query(m_sqlDatabase);
        bool res = query.exec(queryString);
        DATABASE_CHECK_AND_SET_ERROR();

        if (query.next()) {
            resource.setNoteLocalUid(query.record().value(QStringLiteral("localUid")).toString());
        }
    }

    return true;
}

bool LocalStorageManagerPrivate::partialUpdateNoteResources(const QString & noteLocalUid,
                                                            const QList<Resource> & updatedNoteResources,
                                                            ErrorString & errorDescription)
{
    QNDEBUG(QStringLiteral("LocalStorageManagerPrivate::partialUpdateNoteResources: note local uid = ") << noteLocalUid);

    ErrorString errorPrefix(QT_TRANSLATE_NOOP("", "can't do the partial update of note's resources"));

    QString listNoteResourcesQueryString = QString("SELECT Resources.resourceLocalUid, resourceGuid, noteLocalUid, noteGuid, "
                                                   "resourceUpdateSequenceNumber, resourceIsDirty, dataSize, dataHash, "
                                                   "mime, width, height, recognitionDataSize, recognitionDataHash, "
                                                   "alternateDataSize, alternateDataHash, resourceIndexInNote, "
                                                   "resourceSourceURL, timestamp, resourceLatitude, resourceLongitude, "
                                                   "resourceAltitude, cameraMake, cameraModel, clientWillIndex, "
                                                   "fileName, attachment, resourceKey, resourceMapKey, resourceValue "
                                                   "FROM Resources LEFT OUTER JOIN ResourceAttributes "
                                                   "ON Resources.resourceLocalUid = ResourceAttributes.resourceLocalUid "
                                                   "LEFT OUTER JOIN ResourceAttributesApplicationDataKeysOnly "
                                                   "ON Resources.resourceLocalUid = ResourceAttributesApplicationDataKeysOnly.resourceLocalUid "
                                                   "LEFT OUTER JOIN ResourceAttributesApplicationDataFullMap "
                                                   "ON Resources.resourceLocalUid = ResourceAttributesApplicationDataFullMap.resourceLocalUid "
                                                   "WHERE noteLocalUid='%1'").arg(sqlEscapeString(noteLocalUid));
    QSqlQuery query(m_sqlDatabase);
    bool res = query.exec(listNoteResourcesQueryString);
    DATABASE_CHECK_AND_SET_ERROR();

    QList<Resource> previousNoteResources;

    QString resourceLocalUidProperty = QStringLiteral("resourceLocalUid");
    while(query.next())
    {
        QSqlRecord record = query.record();

        int resourceLocalUidIndex = record.indexOf(resourceLocalUidProperty);
        if (resourceLocalUidIndex < 0) {
            errorDescription.base() = errorPrefix.base();
            errorDescription.additionalBases().append(QT_TRANSLATE_NOOP("", "can't retrieve the resource local uid from the query result"));
            QNWARNING(errorDescription);
            return false;
        }

        Resource resource;
        resource.setLocalUid(record.value(resourceLocalUidIndex).toString());

        fillResourceFromSqlRecord(record, /* with binary data = */ false, resource);
        previousNoteResources << resource;
    }

    // Now figure out which resources were removed from the note and which were added or updated
    QStringList localUidsForResourcesRemovedFromNote;
    QList<Resource> addedOrUpdatedResources;

    int numResources = updatedNoteResources.size();
    int numPreviousResources = previousNoteResources.size();
    for(int i = 0; i < numPreviousResources; ++i)
    {
        const Resource & previousNoteResource = previousNoteResources[i];

        bool foundResource = false;
        for(int j = 0; j < numResources; ++j)
        {
            const Resource & resource = updatedNoteResources[j];
            if (resource.localUid() != previousNoteResource.localUid()) {
                continue;
            }

            foundResource = true;

            bool changed = false;

#define COMPARE_RESOURCE_PROPERTY(hasProperty, property) \
            changed = changed || ( (resource.hasProperty() && previousNoteResource.hasProperty() && (resource.property() != previousNoteResource.property())) || \
                                   (resource.hasProperty() != previousNoteResource.hasProperty()) )

            COMPARE_RESOURCE_PROPERTY(hasGuid, guid);
            COMPARE_RESOURCE_PROPERTY(hasNoteGuid, noteGuid);
            COMPARE_RESOURCE_PROPERTY(hasNoteLocalUid, noteLocalUid);
            COMPARE_RESOURCE_PROPERTY(hasUpdateSequenceNumber, updateSequenceNumber);
            COMPARE_RESOURCE_PROPERTY(hasDataSize, dataSize);
            COMPARE_RESOURCE_PROPERTY(hasDataHash, dataHash);
            COMPARE_RESOURCE_PROPERTY(hasMime, mime);
            COMPARE_RESOURCE_PROPERTY(hasWidth, width);
            COMPARE_RESOURCE_PROPERTY(hasHeight, height);
            COMPARE_RESOURCE_PROPERTY(hasRecognitionDataSize, recognitionDataSize);
            COMPARE_RESOURCE_PROPERTY(hasRecognitionDataHash, recognitionDataHash);
            COMPARE_RESOURCE_PROPERTY(hasAlternateDataSize, alternateDataSize);
            COMPARE_RESOURCE_PROPERTY(hasAlternateDataHash, alternateDataHash);
            COMPARE_RESOURCE_PROPERTY(hasResourceAttributes, resourceAttributes);

#undef COMPARE_RESOURCE_PROPERTY

            changed |= (resource.isDirty() != previousNoteResource.isDirty());
            changed |= (resource.isLocal() != previousNoteResource.isLocal());
            changed |= (resource.indexInNote() != previousNoteResource.indexInNote());

            if (changed) {
                addedOrUpdatedResources << resource;
            }

            break;
        }

        if (!foundResource) {
            localUidsForResourcesRemovedFromNote << sqlEscapeString(previousNoteResource.localUid());
        }
    }

    for(int j = 0; j < numResources; ++j)
    {
        const Resource & resource = updatedNoteResources[j];

        bool foundResource = false;
        for(int i = 0; i < numPreviousResources; ++i)
        {
            const Resource & previousResource = previousNoteResources[i];
            if (resource.localUid() != previousResource.localUid()) {
                continue;
            }

            foundResource = true;
            break;
        }

        if (!foundResource) {
            addedOrUpdatedResources << resource;
        }
    }

    // Now delete the removed resources and add/update the added/updated ones

    if (!localUidsForResourcesRemovedFromNote.isEmpty()) {
        QString removeResourcesQueryString = QString("DELETE FROM Resources WHERE resourceLocalUid IN ('%1')")
                                             .arg(localUidsForResourcesRemovedFromNote.join(QStringLiteral(",")));
        res = query.exec(removeResourcesQueryString);
        DATABASE_CHECK_AND_SET_ERROR();
    }

    int numAddedOrUpdatedResources = addedOrUpdatedResources.size();
    for(int i = 0; i < numAddedOrUpdatedResources; ++i)
    {
        const Resource & resource = addedOrUpdatedResources[i];

        ErrorString error;
        bool res = resource.checkParameters(error);
        if (!res) {
            errorDescription.base() = errorPrefix.base();
            errorDescription.additionalBases().append(QT_TRANSLATE_NOOP("", "found invalid resource linked with note"));
            errorDescription.additionalBases().append(error.base());
            errorDescription.additionalBases().append(error.additionalBases());
            errorDescription.details() = error.details();
            QNWARNING(errorDescription);
            return false;
        }

        error.clear();
        res = insertOrReplaceResource(resource, error, /* useSeparateTransaction = */ false);
        if (!res) {
            errorDescription.base() = errorPrefix.base();
            errorDescription.additionalBases().append(QT_TRANSLATE_NOOP("", "can't add or update one of note's resources"));
            errorDescription.additionalBases().append(error.base());
            errorDescription.additionalBases().append(error.additionalBases());
            errorDescription.details() = error.details();
            return false;
        }
    }

    return true;
}

void LocalStorageManagerPrivate::clearDatabaseFile()
{
    QFile databaseFile(m_databaseFilePath);
    if (!databaseFile.open(QIODevice::ReadWrite)) {
        ErrorString errorDescription(QT_TRANSLATE_NOOP("", "Can't open the local storage database file for both reading and writing"));
        errorDescription.details() = databaseFile.errorString();
        throw DatabaseOpeningException(errorDescription);
    }

    databaseFile.resize(0);
    databaseFile.flush();
    databaseFile.close();
}

template <class T>
QString LocalStorageManagerPrivate::listObjectsOptionsToSqlQueryConditions(const LocalStorageManager::ListObjectsOptions & options,
                                                                           ErrorString & errorDescription) const
{
    QString result;
    errorDescription.clear();

    bool listAll = options.testFlag(LocalStorageManager::ListAll);

    bool listDirty = options.testFlag(LocalStorageManager::ListDirty);
    bool listNonDirty = options.testFlag(LocalStorageManager::ListNonDirty);

    bool listElementsWithoutGuid = options.testFlag(LocalStorageManager::ListElementsWithoutGuid);
    bool listElementsWithGuid = options.testFlag(LocalStorageManager::ListElementsWithGuid);

    bool listLocal = options.testFlag(LocalStorageManager::ListLocal);
    bool listNonLocal = options.testFlag(LocalStorageManager::ListNonLocal);

    bool listFavoritedElements = options.testFlag(LocalStorageManager::ListFavoritedElements);
    bool listNonFavoritedElements = options.testFlag(LocalStorageManager::ListNonFavoritedElements);

    if (!listAll && !listDirty && !listNonDirty && !listElementsWithoutGuid && !listElementsWithGuid &&
        !listLocal && !listNonLocal && !listFavoritedElements && !listNonFavoritedElements)
    {
        errorDescription.base() = QT_TRANSLATE_NOOP("", "Can't list objects by filter: detected incorrect filter flag");
        errorDescription.details() = QString::number(static_cast<int>(options));
        return result;
    }

    if (!(listDirty && listNonDirty))
    {
        if (listDirty) {
            result += QStringLiteral("(isDirty=1) AND ");
        }

        if (listNonDirty) {
            result += QStringLiteral("(isDirty=0) AND ");
        }
    }

    if (!(listElementsWithoutGuid && listElementsWithGuid))
    {
        if (listElementsWithoutGuid) {
            result += QStringLiteral("(guid IS NULL) AND ");
        }

        if (listElementsWithGuid) {
            result += QStringLiteral("(guid IS NOT NULL) AND ");
        }
    }

    if (!(listLocal && listNonLocal))
    {
        if (listLocal) {
            result += QStringLiteral("(isLocal=1) AND ");
        }

        if (listNonLocal) {
            result += QStringLiteral("(isLocal=0) AND ");
        }
    }

    if (!(listFavoritedElements && listNonFavoritedElements))
    {
        if (listFavoritedElements) {
            result += QStringLiteral("(isFavorited=1) AND ");
        }

        if (listNonFavoritedElements) {
            result += QStringLiteral("(isFavorited=0) AND ");
        }
    }

    return result;
}

template<>
QString LocalStorageManagerPrivate::listObjectsOptionsToSqlQueryConditions<LinkedNotebook>(const LocalStorageManager::ListObjectsOptions & flag,
                                                                                           ErrorString & errorDescription) const
{
    QString result;
    errorDescription.clear();

    bool listAll = flag.testFlag(LocalStorageManager::ListAll);
    bool listDirty = flag.testFlag(LocalStorageManager::ListDirty);
    bool listNonDirty = flag.testFlag(LocalStorageManager::ListNonDirty);

    if (!listAll && !listDirty && !listNonDirty) {
        errorDescription.base() = QT_TRANSLATE_NOOP("", "Can't list linked notebooks by filter: detected incorrect filter flag");
        errorDescription.details() = QString::number(static_cast<int>(flag));
        return result;
    }

    if (!(listDirty && listNonDirty))
    {
        if (listDirty) {
            result += QStringLiteral("(isDirty=1)");
        }

        if (listNonDirty) {
            result += QStringLiteral("(isDirty=0)");
        }
    }

    return result;
}

template <>
QString LocalStorageManagerPrivate::listObjectsGenericSqlQuery<SavedSearch>() const
{
    QString result = QStringLiteral("SELECT * FROM SavedSearches");
    return result;
}

template <>
QString LocalStorageManagerPrivate::listObjectsGenericSqlQuery<Tag>() const
{
    QString result = QStringLiteral("SELECT * FROM Tags");
    return result;
}

template <>
QString LocalStorageManagerPrivate::listObjectsGenericSqlQuery<LinkedNotebook>() const
{
    QString result = QStringLiteral("SELECT * FROM LinkedNotebooks");
    return result;
}

template <>
QString LocalStorageManagerPrivate::listObjectsGenericSqlQuery<Notebook>() const
{
    QString result = QStringLiteral("SELECT * FROM Notebooks LEFT OUTER JOIN NotebookRestrictions "
                                    "ON Notebooks.localUid = NotebookRestrictions.localUid "
                                    "LEFT OUTER JOIN SharedNotebooks ON ((Notebooks.guid IS NOT NULL) AND (Notebooks.guid = SharedNotebooks.sharedNotebookNotebookGuid)) "
                                    "LEFT OUTER JOIN Users ON Notebooks.contactId = Users.id "
                                    "LEFT OUTER JOIN UserAttributes ON Notebooks.contactId = UserAttributes.id "
                                    "LEFT OUTER JOIN UserAttributesViewedPromotions ON Notebooks.contactId = UserAttributesViewedPromotions.id "
                                    "LEFT OUTER JOIN UserAttributesRecentMailedAddresses ON Notebooks.contactId = UserAttributesRecentMailedAddresses.id "
                                    "LEFT OUTER JOIN Accounting ON Notebooks.contactId = Accounting.id "
                                    "LEFT OUTER JOIN AccountLimits ON Notebooks.contactId = AccountLimits.id "
                                    "LEFT OUTER JOIN BusinessUserInfo ON Notebooks.contactId = BusinessUserInfo.id");
    return result;
}

template <>
QString LocalStorageManagerPrivate::listObjectsGenericSqlQuery<Note>() const
{
    QString result = QStringLiteral("SELECT * FROM Notes LEFT OUTER JOIN SharedNotes "
                                    "ON ((Notes.guid IS NOT NULL) AND (Notes.guid = SharedNotes.sharedNoteNoteGuid)) "
                                    "LEFT OUTER JOIN NoteRestrictions on Notes.localUid = NoteRestrictions.noteLocalUid "
                                    "LEFT OUTER JOIN NoteLimits ON Notes.localUid = NoteLimits.noteLocalUid");
    return result;
}

template <>
QString LocalStorageManagerPrivate::orderByToSqlTableColumn<LocalStorageManager::ListNotesOrder::type>(const LocalStorageManager::ListNotesOrder::type & order) const
{
    QString result;

    switch(order)
    {
    case LocalStorageManager::ListNotesOrder::ByUpdateSequenceNumber:
        result = QStringLiteral("updateSequenceNumber");
        break;
    case LocalStorageManager::ListNotesOrder::ByTitle:
        result = QStringLiteral("title");
        break;
    case LocalStorageManager::ListNotesOrder::ByCreationTimestamp:
        result = QStringLiteral("creationTimestamp");
        break;
    case LocalStorageManager::ListNotesOrder::ByModificationTimestamp:
        result = QStringLiteral("modificationTimestamp");
        break;
    case LocalStorageManager::ListNotesOrder::ByDeletionTimestamp:
        result = QStringLiteral("deletionTimestamp");
        break;
    case LocalStorageManager::ListNotesOrder::ByAuthor:
        result = QStringLiteral("author");
        break;
    case LocalStorageManager::ListNotesOrder::BySource:
        result = QStringLiteral("source");
        break;
    case LocalStorageManager::ListNotesOrder::BySourceApplication:
        result = QStringLiteral("sourceApplication");
        break;
    case LocalStorageManager::ListNotesOrder::ByReminderTime:
        result = QStringLiteral("reminderTime");
        break;
    case LocalStorageManager::ListNotesOrder::ByPlaceName:
        result = QStringLiteral("placeName");
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
        result = QStringLiteral("updateSequenceNumber");
        break;
    case LocalStorageManager::ListNotebooksOrder::ByNotebookName:
        result = QStringLiteral("notebookNameUpper");
        break;
    case LocalStorageManager::ListNotebooksOrder::ByCreationTimestamp:
        result = QStringLiteral("creationTimestamp");
        break;
    case LocalStorageManager::ListNotebooksOrder::ByModificationTimestamp:
        result = QStringLiteral("modificationTimestamp");
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
        result = QStringLiteral("updateSequenceNumber");
        break;
    case LocalStorageManager::ListLinkedNotebooksOrder::ByShareName:
        result = QStringLiteral("shareName");
        break;
    case LocalStorageManager::ListLinkedNotebooksOrder::ByUsername:
        result = QStringLiteral("username");
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
        result = QStringLiteral("updateSequenceNumber");
        break;
    case LocalStorageManager::ListTagsOrder::ByName:
        result = QStringLiteral("nameLower");
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
        result = QStringLiteral("updateSequenceNumber");
        break;
    case LocalStorageManager::ListSavedSearchesOrder::ByName:
        result = QStringLiteral("nameLower");
        break;
    case LocalStorageManager::ListSavedSearchesOrder::ByFormat:
        result = QStringLiteral("format");
        break;
    default:
        break;
    }

    return result;
}

template <class T>
bool LocalStorageManagerPrivate::fillObjectsFromSqlQuery(QSqlQuery query, QList<T> & objects, ErrorString & errorDescription) const
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
bool LocalStorageManagerPrivate::fillObjectsFromSqlQuery(QSqlQuery query, QList<Note> & notes,
                                                         ErrorString & errorDescription) const
{
    QMap<QString, int> indexForLocalUid;

    while(query.next())
    {
        QSqlRecord rec = query.record();

        int localUidIndex = rec.indexOf(QStringLiteral("localUid"));
        if (localUidIndex < 0) {
            errorDescription.base() = QT_TRANSLATE_NOOP("", "no localUid field in SQL record for note");
            QNWARNING(errorDescription);
            return false;
        }

        QVariant localUidValue = rec.value(localUidIndex);
        QString localUid = localUidValue.toString();
        if (localUid.isEmpty()) {
            errorDescription.base() = QT_TRANSLATE_NOOP("", "found empty localUid field in SQL record for note");
            QNWARNING(errorDescription);
            return false;
        }

        auto it = indexForLocalUid.find(localUid);
        bool notFound = (it == indexForLocalUid.end());
        if (notFound) {
            indexForLocalUid[localUid] = notes.size();
            notes << Note();
        }

        Note & note = (notFound ? notes.back() : notes[it.value()]);

        bool res = fillNoteFromSqlRecord(rec, note, errorDescription);
        if (!res) {
            return false;
        }

        sortSharedNotes(note);
    }

    return true;
}

template <>
bool LocalStorageManagerPrivate::fillObjectsFromSqlQuery<Notebook>(QSqlQuery query, QList<Notebook> & notebooks,
                                                                   ErrorString & errorDescription) const
{
    QMap<QString, int> indexForLocalUid;

    while(query.next())
    {
        QSqlRecord rec = query.record();

        int localUidIndex = rec.indexOf(QStringLiteral("localUid"));
        if (localUidIndex < 0) {
            errorDescription.base() = QT_TRANSLATE_NOOP("", "no localUid field in SQL record for notebook");
            QNWARNING(errorDescription);
            return false;
        }

        QVariant localUidValue = rec.value(localUidIndex);
        QString localUid = localUidValue.toString();
        if (localUid.isEmpty()) {
            errorDescription.base() = QT_TRANSLATE_NOOP("", "found empty localUid field in SQL record for Notebook");
            QNWARNING(errorDescription);
            return false;
        }

        auto it = indexForLocalUid.find(localUid);
        bool notFound = (it == indexForLocalUid.end());
        if (notFound) {
            indexForLocalUid[localUid] = notebooks.size();
            notebooks << Notebook();
        }

        Notebook & notebook = (notFound ? notebooks.back() : notebooks[it.value()]);

        bool res = fillNotebookFromSqlRecord(rec, notebook, errorDescription);
        if (!res) {
            return false;
        }

        sortSharedNotebooks(notebook);
    }

    return true;
}

template<>
bool LocalStorageManagerPrivate::fillObjectFromSqlRecord<SavedSearch>(const QSqlRecord & rec, SavedSearch & search,
                                                                      ErrorString & errorDescription) const
{
    return fillSavedSearchFromSqlRecord(rec, search, errorDescription);
}

template<>
bool LocalStorageManagerPrivate::fillObjectFromSqlRecord<Tag>(const QSqlRecord & rec, Tag & tag,
                                                              ErrorString & errorDescription) const
{
    return fillTagFromSqlRecord(rec, tag, errorDescription);
}

template<>
bool LocalStorageManagerPrivate::fillObjectFromSqlRecord<LinkedNotebook>(const QSqlRecord & rec, LinkedNotebook & linkedNotebook,
                                                                         ErrorString & errorDescription) const
{
    return fillLinkedNotebookFromSqlRecord(rec, linkedNotebook, errorDescription);
}

template <>
bool LocalStorageManagerPrivate::fillObjectFromSqlRecord<Notebook>(const QSqlRecord & rec, Notebook & notebook,
                                                                   ErrorString & errorDescription) const
{
    bool res = fillNotebookFromSqlRecord(rec, notebook, errorDescription);
    if (!res) {
        return false;
    }

    sortSharedNotebooks(notebook);
    return true;
}

template<>
bool LocalStorageManagerPrivate::fillObjectFromSqlRecord<Note>(const QSqlRecord & rec, Note & note,
                                                               ErrorString & errorDescription) const
{
    bool res = fillNoteFromSqlRecord(rec, note, errorDescription);
    if (!res) {
        return false;
    }

    sortSharedNotes(note);
    return true;
}

template <class T, class TOrderBy>
QList<T> LocalStorageManagerPrivate::listObjects(const LocalStorageManager::ListObjectsOptions & flag,
                                                 ErrorString & errorDescription, const size_t limit,
                                                 const size_t offset, const TOrderBy & orderBy,
                                                 const LocalStorageManager::OrderDirection::type & orderDirection,
                                                 const QString & additionalSqlQueryCondition) const
{
    ErrorString flagError;
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
        if (!sumSqlQueryConditions.isEmpty() && !sumSqlQueryConditions.endsWith(QStringLiteral(" AND "))) {
            sumSqlQueryConditions += QStringLiteral(" AND ");
        }

        sumSqlQueryConditions += additionalSqlQueryCondition;
    }

    if (sumSqlQueryConditions.endsWith(QStringLiteral(" AND "))) {
        sumSqlQueryConditions.chop(5);
    }

    QString queryString = listObjectsGenericSqlQuery<T>();
    if (!sumSqlQueryConditions.isEmpty()) {
        sumSqlQueryConditions.prepend(QStringLiteral("("));
        sumSqlQueryConditions.append(QStringLiteral(")"));
        queryString += QStringLiteral(" WHERE ");
        queryString += sumSqlQueryConditions;
    }

    QString orderByColumn = orderByToSqlTableColumn<TOrderBy>(orderBy);
    if (!orderByColumn.isEmpty())
    {
        queryString += QStringLiteral(" ORDER BY ");
        queryString += orderByColumn;

        switch(orderDirection)
        {
            case LocalStorageManager::OrderDirection::Descending:
                queryString += QStringLiteral(" DESC");
                break;
            case LocalStorageManager::OrderDirection::Ascending:    // NOTE: intentional fall-through
            default:
                queryString += QStringLiteral(" ASC");
                break;
        }
    }

    if (limit != 0) {
        queryString += QStringLiteral(" LIMIT ") + QString::number(limit);
    }

    if (offset != 0) {
        queryString += QStringLiteral(" OFFSET ") + QString::number(offset);
    }

    QNDEBUG(QStringLiteral("SQL query string: ") << queryString);

    QList<T> objects;

    ErrorString errorPrefix(QT_TRANSLATE_NOOP("", "can't list objects from the local storage database by filter"));
    QSqlQuery query(m_sqlDatabase);
    bool res = query.exec(queryString);
    if (!res) {
        errorDescription.base() = errorPrefix.base();
        QNCRITICAL(errorDescription << QStringLiteral(", last query = ") << query.lastQuery()
                   << QStringLiteral(", last error = ") << query.lastError());
        errorDescription.details() = query.lastError().text();
        return objects;
    }

    ErrorString error;
    res = fillObjectsFromSqlQuery(query, objects, error);
    if (!res) {
        errorDescription.base() = errorPrefix.base();
        errorDescription.additionalBases().append(error.base());
        errorDescription.additionalBases().append(error.additionalBases());
        errorDescription.details() = error.details();
        QNWARNING(errorDescription);
        objects.clear();
        return objects;
    }

    QNDEBUG(QStringLiteral("found ") << objects.size() << QStringLiteral(" objects"));

    return objects;
}

bool LocalStorageManagerPrivate::SharedNotebookCompareByIndex::operator()(const SharedNotebook & lhs,
                                                                          const SharedNotebook & rhs) const
{
    return (lhs.indexInNotebook() < rhs.indexInNotebook());
}

bool LocalStorageManagerPrivate::SharedNoteCompareByIndex::operator()(const SharedNote & lhs, const SharedNote & rhs) const
{
    return (lhs.indexInNote() < rhs.indexInNote());
}

bool LocalStorageManagerPrivate::ResourceCompareByIndex::operator()(const Resource & lhs,
                                                                           const Resource & rhs) const
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

} // namespace quentier

