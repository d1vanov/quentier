#include "LocalStorageManager.h"
#include "DatabaseOpeningException.h"
#include "DatabaseSqlErrorException.h"
#include "../NoteStore.h"

using namespace evernote::edam;

#ifdef USE_QT5
#include <QStandardPaths>
#else
#include <QDesktopServices>
#endif

namespace qute_note {

#define QUTE_NOTE_DATABASE_NAME "qn.storage.sqlite"

LocalStorageManager::LocalStorageManager(const QString & username) :
#ifdef USE_QT5
    m_applicationPersistenceStoragePath(QStandardPaths::writableLocation(QStandardPaths::DataLocation))
#else
    m_applicationPersistenceStoragePath(QDesktopServices::storageLocation(QDesktopServices::DataLocation))
#endif
{
    bool needsInitializing = false;

    m_applicationPersistenceStoragePath.append("/" + username);
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
                                                    "for Sql local storage database"));
    }

    if (needsInitializing)
    {
        QString errorDescription;
        if (!CreateTables(errorDescription)) {
            throw DatabaseSqlErrorException(QObject::tr("Cannot initialize tables in Sql database: ") +
                                            errorDescription);
        }
    }
}

LocalStorageManager::~LocalStorageManager()
{
    if (m_sqlDatabase.open()) {
        m_sqlDatabase.close();
    }
}

#define DATABASE_CHECK_AND_SET_ERROR(errorPrefix) \
    if (!res) { \
        errorDescription = QObject::tr(errorPrefix); \
        errorDescription.append(m_sqlDatabase.lastError().text()); \
        return false; \
    }

bool LocalStorageManager::AddNotebook(const Notebook & notebook, QString & errorDescription)
{
    bool res = notebook.CheckParameters(errorDescription);
    if (!res) {
        return false;
    }

    // Check the existence of the notebook prior to attempting to add it
    res = ReplaceNotebook(notebook, errorDescription);
    if (res) {
        return true;
    }
    errorDescription.clear();

    const evernote::edam::Notebook & enNotebook = notebook.en_notebook;

    QSqlQuery query(m_sqlDatabase);
    query.prepare("INSERT INTO Notebooks (guid, usn, name, creationTimestamp, "
                  "modificationTimestamp, isDirty, isLocal, isDefault, isLastUsed) "
                  "VALUES(?, ?, ?, ?, ?, ?, ?, 0, 0");
    query.addBindValue(QString::fromStdString(enNotebook.guid));
    query.addBindValue(enNotebook.updateSequenceNum);
    query.addBindValue(QString::fromStdString(enNotebook.name));
    query.addBindValue(QVariant(static_cast<int>(enNotebook.serviceCreated)));
    query.addBindValue(QVariant(static_cast<int>(enNotebook.serviceUpdated)));
    query.addBindValue(notebook.isDirty);
    query.addBindValue(notebook.isLocal);

    res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("Can't insert new notebook into local storage database: ");

    return true;
}

bool LocalStorageManager::ReplaceNotebook(const Notebook & notebook, QString & errorDescription)
{
    bool res = notebook.CheckParameters(errorDescription);
    if (!res) {
        return false;
    }

    const evernote::edam::Notebook & enNotebook = notebook.en_notebook;

    QSqlQuery query(m_sqlDatabase);
    query.prepare("SELECT rowid FROM Notebooks WHERE guid = ?");
    query.addBindValue(QString::fromStdString(enNotebook.guid));

    res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("Can't check the existence of notebook "
                                 "in local storage database");

    if (!query.next()) {
        errorDescription = QObject::tr("Can't replace notebook: can't find notebook "
                                       "in local storage database");
        return false;
    }

    int rowId = query.value(0).toInt(&res);
    if (!res || (rowId < 0)) {
        errorDescription = QObject::tr("Can't get rowId from SQL selection query result");
        return false;
    }

    query.clear();
    query.prepare("UPDATE Notebooks SET guid=?, usn=?, name=?, creationTimestamp=?, "
                  "modificationTimestamp=?, isDirty=?, isLocal=?, isDefault=?, isLastUsed=? "
                  "WHERE rowid = ?");
    query.addBindValue(QString::fromStdString(enNotebook.guid));
    query.addBindValue(enNotebook.updateSequenceNum);
    query.addBindValue(QString::fromStdString(enNotebook.name));
    query.addBindValue(QVariant(static_cast<int>(enNotebook.serviceCreated)));
    query.addBindValue(QVariant(static_cast<int>(enNotebook.serviceUpdated)));
    query.addBindValue((notebook.isDirty ? 1 : 0));
    query.addBindValue((notebook.isLocal ? 1 : 0));
    query.addBindValue((enNotebook.defaultNotebook ? 1 : 0));
    query.addBindValue((notebook.isLastUsed ? 1 : 0));
    query.addBindValue(rowId);

    res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("Can't update Notebooks table: ");

    return true;
}

#define CHECK_GUID(object, objectName) \
    {\
        if (!object.__isset.guid) { \
            errorDescription = QObject::tr(objectName " guid is not set"); \
            return false; \
        } \
        const Guid & guid = object.guid; \
        if (guid.empty()) { \
            errorDescription = QObject::tr(objectName " guid is empty"); \
            return false; \
        } \
    }

bool LocalStorageManager::ExpungeLocalNotebook(const Notebook & notebook, QString & errorDescription)
{
    const evernote::edam::Notebook & enNotebook = notebook.en_notebook;
    CHECK_GUID(enNotebook, "Notebook");

    if (!notebook.isLocal) {
        errorDescription = QObject::tr("Can't expunge non-local notebook");
        return false;
    }

    QSqlQuery query(m_sqlDatabase);
    query.prepare("SELECT rowid FROM Notebooks WHERE guid = ?");
    query.addBindValue(QString::fromStdString(notebook.en_notebook.guid));
    bool res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("Can't find notebook to be expunged in local storage database: ");

    int rowId = -1;
    bool conversionResult = false;
    while(query.next()) {
        rowId = query.value(0).toInt(&conversionResult);
    }

    if (!conversionResult || (rowId < 0)) {
        errorDescription = QObject::tr("Can't get rowId of notebook to be expunged in Notebooks table");
        return false;
    }

    query.clear();
    query.prepare("DELETE FROM Notebooks WHERE rowid = ?");
    query.addBindValue(QVariant(rowId));

    res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("Can't expunge notebook from local storage database: ");

    return true;
}

bool LocalStorageManager::AddNote(const Note & note, QString & errorDescription)
{
    bool res = note.CheckParameters(errorDescription);
    if (!res) {
        return false;
    }

    // Check the existence of the note prior to attempting to add it
    res = ReplaceNote(note, errorDescription);
    if (res) {
        return true;
    }
    errorDescription.clear();

    const evernote::edam::Note & enNote = note.en_note;

    QString guid = QString::fromStdString(enNote.guid);
    QString title = QString::fromStdString(enNote.title);
    QString content = QString::fromStdString(enNote.content);
    QString notebookGuid = QString::fromStdString(enNote.notebookGuid);

    QSqlQuery query(m_sqlDatabase);
    query.prepare("INSERT INTO NoteText (guid, title, content, notebookGuid) "
                  "VALUES(?, ?, ?, ?)");
    query.addBindValue(guid);
    query.addBindValue(title);
    query.addBindValue(content);
    query.addBindValue(notebookGuid);
    res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("Can't add note to NoteText table in local storage database: ");

    query.clear();
    query.prepare("INSERT INTO Notes (guid, usn, title, isDirty, isLocal, content, "
                  "creationTimestamp, modificationTimestamp, notebookGuid) "
                  "VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?)");
    query.addBindValue(guid);
    query.addBindValue(enNote.updateSequenceNum);
    query.addBindValue(title);
    query.addBindValue((note.isDirty ? 1 : 0));
    query.addBindValue((note.isLocal ? 1 : 0));
    query.addBindValue(content);
    query.addBindValue(static_cast<qint64>(enNote.created));
    query.addBindValue(static_cast<qint64>(enNote.updated));
    query.addBindValue(notebookGuid);

    res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("Can't add note to Notes table in local storage database: ");

    // TODO: also add or update note's tags and resources if need be
    // TODO: add note's attributes if these are set

    return true;
}

bool LocalStorageManager::ReplaceNote(const Note & note, QString & errorDescription)
{
    bool res = note.CheckParameters(errorDescription);
    if (!res) {
        return false;
    }

    const evernote::edam::Note & enNote = note.en_note;
    QString noteGuid = QString::fromStdString(enNote.guid);

    QSqlQuery query(m_sqlDatabase);
    query.prepare("SELECT rowid FROM Notes WHERE guid = ?");
    query.addBindValue(noteGuid);
    res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("Can't check note for existence in local storage database: ");

    if (!query.next()) {
        errorDescription = QObject::tr("Can't replace note: can't find note "
                                       "in local storage database");
        return false;
    }

    int rowId = query.value(0).toInt(&res);
    if (!res || (rowId < 0)) {
        errorDescription = QObject::tr("Can't replace note: can't get row id from "
                                       "SQL query result");
        return false;
    }

    query.clear();
    query.prepare("INSERT OR REPLACE INTO NotesText (guid, title, "
                  "content, notebookGuid) VALUES(?, ?, ?, ?)");
    query.addBindValue(noteGuid);
    query.addBindValue(QString::fromStdString(enNote.title));
    query.addBindValue(QString::fromStdString(enNote.content));
    query.addBindValue(QString::fromStdString(enNote.notebookGuid));

    res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("Can't replace note in NotesText table in local "
                                 "storage database: ");

    query.clear();
    query.prepare("INSERT OR REPLACE INTO Notes (guid, usn, title, "
                  "isDirty, isLocal, content, creationTimestamp, "
                  "modificationTimestamp, subjectDate, altitude, "
                  "latitude, longitude, author, source, sourceUrl, "
                  "sourceApplication, isDeleted, notebookGuid) "
                  "VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, "
                  "?, ?, ?, ?, ?, ?, ?, ?)");
    NoteAttributesToQuery(note, query);

    res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("Can't replace note in Notes table in local "
                                 "storage database: ");

    return true;
}

bool LocalStorageManager::FindNote(const Guid & noteGuid, Note & note, QString & errorDescription) const
{
    QSqlQuery query(m_sqlDatabase);

    query.prepare("SELECT updateSequenceNumber, title, isDirty, isLocal, body, creationTimestamp, "
                  "modificationTimestamp, isDeleted, notebook FROM Notes WHERE guid = ?");
    query.addBindValue(QString::fromStdString(noteGuid));

    bool res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("Can't select note from Notes table in local "
                                 "storage database: ");

    QSqlRecord rec = query.record();

    bool isDirty = false;
    int isDirtyInt = qvariant_cast<int>(rec.value("isDirty"));
    if (isDirtyInt != 0) {
        isDirty = true;
    }
    note.isDirty = isDirty;

    bool isLocal = false;
    int isLocalInt = qvariant_cast<int>(rec.value("isLocal"));
    if (isLocalInt != 0) {
        isLocal = true;
    }
    note.isLocal = isLocal;

    bool isDeleted = false;
    int isDeletedInt = qvariant_cast<int>(rec.value("isDeleted"));
    if (isDeletedInt != 0) {
        isDeleted = true;
    }
    note.isDeleted = isDeleted;

    evernote::edam::Note & enNote = note.en_note;

    enNote.guid = noteGuid;
    enNote.__isset.guid = true;

    uint32_t updateSequenceNumber = qvariant_cast<uint32_t>(rec.value("updateSequenceNumber"));
    enNote.updateSequenceNum = updateSequenceNumber;
    enNote.__isset.updateSequenceNum = true;

    QString notebookGuid = qvariant_cast<QString>(rec.value("notebook"));
    enNote.notebookGuid = notebookGuid.toStdString();
    enNote.__isset.notebookGuid = true;

    QString title = qvariant_cast<QString>(rec.value("title"));
    enNote.title = title.toStdString();
    enNote.__isset.title = true;

    QString content = qvariant_cast<QString>(rec.value("content"));
    enNote.content = content.toStdString();
    enNote.__isset.content = true;

#if QT_VERSION >= 0x050101
    QByteArray contentBinaryHash = QCryptographicHash::hash(content.toUtf8(), QCryptographicHash::Md5);
#else
    QByteArray contentBinaryHash = QCryptographicHash::hash(content.toAscii(), QCryptographicHash::Md5);
#endif
    QString contentHash(contentBinaryHash);
    enNote.contentHash = contentHash.toStdString();
    enNote.__isset.contentHash = true;

    enNote.contentLength = content.length();
    enNote.__isset.contentLength = true;

    Timestamp creationTimestamp = qvariant_cast<Timestamp>(rec.value("creationTimestamp"));
    enNote.created = creationTimestamp;
    enNote.__isset.created = true;

    Timestamp modificationTimestamp = qvariant_cast<Timestamp>(rec.value("modificationTimestamp"));
    enNote.updated = modificationTimestamp;
    enNote.__isset.updated = true;

    if (isDeleted) {
        Timestamp deletionTimestamp = qvariant_cast<Timestamp>(rec.value("deletionTimestamp"));
        enNote.deleted = deletionTimestamp;
        enNote.__isset.deleted = true;
    }

    int isActive = qvariant_cast<int>(rec.value("isActive"));
    if (isActive != 0) {
        enNote.active = true;
    }
    else {
        enNote.active = false;
    }
    enNote.__isset.active = true;

    // TODO: retrieve tag and resource guids from tags and resources tables

    int hasAttributes = qvariant_cast<int>(rec.value("hasAttributes"));
    if (hasAttributes == 0) {
        return true;
    }
    else {
        enNote.__isset.attributes = true;
    }

    query.clear();
    query.prepare("SELECT subjectDate, isSetSubjectDate, latitude, isSetLatitude, "
                  "longitude, isSetLongitude, altitude, isSetAltitude, author, "
                  "isSetAuthor, source, isSetSource, sourceUrl, isSetSourceUrl, "
                  "sourceApplication, isSetSourceApplication, shareDate, isSetShareDate, "
                  "reminderOrder, isSetReminderOrder, reminderDoneTime, isSetReminderDoneTime, "
                  "reminderTime, isSetReminderTime, placeName, isSetPlaceName, "
                  "contentClass, isSetContentClass, lastEditedBy, isSetLastEditedBy, "
                  "creatorId, isSetCreatorId, lastEditorId, isSetLastEditorId WHERE noteGuid = ?");
    query.addBindValue(QString::fromStdString(noteGuid));
    res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("Can't select note attributes from NoteAttributes table: ");

    int isSetSubjectDate = qvariant_cast<int>(rec.value("isSetSubjectDate"));
    if (isSetSubjectDate != 0) {
        Timestamp subjectDate = qvariant_cast<Timestamp>(rec.value("subjectDate"));
        enNote.attributes.subjectDate = subjectDate;
        enNote.attributes.__isset.subjectDate = true;
    }
    else {
        enNote.attributes.__isset.subjectDate = false;
    }

    int isSetLatitude = qvariant_cast<int>(rec.value("isSetLatitude"));
    if (isSetLatitude != 0) {
        double latitude = qvariant_cast<double>(rec.value("latitude"));
        enNote.attributes.latitude = latitude;
        enNote.attributes.__isset.latitude = true;
    }
    else {
        enNote.attributes.__isset.latitude = false;
    }

    int isSetAltitude = qvariant_cast<int>(rec.value("isSetAltitude"));
    if (isSetAltitude != 0) {
        double altitude = qvariant_cast<double>(rec.value("altitude"));
        enNote.attributes.altitude = altitude;
        enNote.attributes.__isset.altitude = true;
    }
    else {
        enNote.attributes.__isset.altitude = false;
    }

    int isSetLongitude = qvariant_cast<int>(rec.value("isSetLongitude"));
    if (isSetLongitude != 0) {
        double longitude = qvariant_cast<double>(rec.value("longitude"));
        enNote.attributes.longitude = longitude;
        enNote.attributes.__isset.longitude = true;
    }
    else {
        enNote.attributes.__isset.longitude = false;
    }

    int isSetAuthor = qvariant_cast<int>(rec.value("isSetAuthor"));
    if (isSetAuthor != 0) {
        QString author = qvariant_cast<QString>(rec.value("author"));
        enNote.attributes.author = author.toStdString();
        enNote.attributes.__isset.author = true;
    }
    else {
        enNote.attributes.__isset.author = false;
    }

    int isSetSource = qvariant_cast<int>(rec.value("isSetSource"));
    if (isSetSource != 0) {
        QString source = qvariant_cast<QString>(rec.value("source"));
        enNote.attributes.source = source.toStdString();
        enNote.attributes.__isset.source = true;
    }
    else {
        enNote.attributes.__isset.source = false;
    }

    int isSetSourceUrl = qvariant_cast<int>(rec.value("isSetSourceUrl"));
    if (isSetSourceUrl != 0) {
        QString sourceUrl = qvariant_cast<QString>(rec.value("sourceUrl"));
        enNote.attributes.sourceURL = sourceUrl.toStdString();
        enNote.attributes.__isset.sourceURL = true;
    }
    else {
        enNote.attributes.__isset.sourceURL = false;
    }

    int isSetSourceApplication = qvariant_cast<int>(rec.value("isSetSourceApplication"));
    if (isSetSourceApplication != 0) {
        QString sourceApplication = qvariant_cast<QString>(rec.value("sourceApplication"));
        enNote.attributes.sourceApplication = sourceApplication.toStdString();
        enNote.attributes.__isset.sourceApplication = true;
    }
    else {
        enNote.attributes.__isset.sourceApplication = false;
    }

    int isSetShareDate = qvariant_cast<int>(rec.value("isSetShareDate"));
    if (isSetShareDate != 0) {
        Timestamp shareDate = qvariant_cast<Timestamp>(rec.value("shareDate"));
        enNote.attributes.shareDate = shareDate;
        enNote.attributes.__isset.shareDate = true;
    }
    else {
        enNote.attributes.__isset.shareDate = false;
    }

    int isSetReminderOrder = qvariant_cast<int>(rec.value("isSetReminderOrder"));
    if (isSetReminderOrder != 0) {
        int64_t reminderOrder = qvariant_cast<int64_t>(rec.value("reminderOrder"));
        enNote.attributes.reminderOrder = reminderOrder;
        enNote.attributes.__isset.reminderOrder = true;
    }
    else {
        enNote.attributes.__isset.reminderOrder = false;
    }

    int isSetReminderDoneTime = qvariant_cast<int>(rec.value("isSetReminderDoneTime"));
    if (isSetReminderDoneTime != 0) {
        Timestamp reminderDoneTime = qvariant_cast<Timestamp>(rec.value("reminderDoneTime"));
        enNote.attributes.reminderDoneTime = reminderDoneTime;
        enNote.attributes.__isset.reminderDoneTime = true;
    }
    else {
        enNote.attributes.__isset.reminderDoneTime = false;
    }

    int isSetReminderTime = qvariant_cast<int>(rec.value("isSetReminderTime"));
    if (isSetReminderTime != 0) {
        Timestamp reminderTime = qvariant_cast<Timestamp>(rec.value("reminderTime"));
        enNote.attributes.reminderTime = reminderTime;
        enNote.attributes.__isset.reminderTime = true;
    }
    else {
        enNote.attributes.__isset.reminderTime = false;
    }

    int isSetPlaceName = qvariant_cast<int>(rec.value("isSetPlaceName"));
    if (isSetPlaceName != 0) {
        QString placeName = qvariant_cast<QString>(rec.value("placeName"));
        enNote.attributes.placeName = placeName.toStdString();
        enNote.attributes.__isset.placeName = true;
    }
    else {
        enNote.attributes.__isset.placeName = false;
    }

    int isSetContentClass = qvariant_cast<int>(rec.value("isSetContentClass"));
    if (isSetContentClass != 0) {
        QString contentClass = qvariant_cast<QString>(rec.value("contentClass"));
        enNote.attributes.contentClass = contentClass.toStdString();
        enNote.attributes.__isset.contentClass = true;
    }
    else {
        enNote.attributes.__isset.contentClass = false;
    }

    int isSetLastEditedBy = qvariant_cast<int>(rec.value("isSetLastEditedBy"));
    if (isSetLastEditedBy != 0) {
        QString lastEditedBy = qvariant_cast<QString>(rec.value("lastEditedBy"));
        enNote.attributes.lastEditedBy = lastEditedBy.toStdString();
        enNote.attributes.__isset.lastEditedBy = true;
    }
    else {
        enNote.attributes.__isset.lastEditedBy = false;
    }

    int isSetCreatorId = qvariant_cast<int>(rec.value("isSetCreatorId"));
    if (isSetCreatorId != 0) {
        UserID creatorId = qvariant_cast<UserID>(rec.value("creatorId"));
        enNote.attributes.creatorId = creatorId;
        enNote.attributes.__isset.creatorId = true;
    }
    else {
        enNote.attributes.__isset.creatorId = false;
    }

    int isSetLastEditorId = qvariant_cast<int>(rec.value("isSetLastEditorId"));
    if (isSetLastEditorId != 0) {
        UserID lastEditorId = qvariant_cast<UserID>(rec.value("lastEditorId"));
        enNote.attributes.lastEditorId = lastEditorId;
        enNote.attributes.__isset.lastEditorId = true;
    }
    else {
        enNote.attributes.__isset.lastEditorId = false;
    }

    return note.CheckParameters(errorDescription);
}

bool LocalStorageManager::DeleteNote(const Note & note, QString & errorDescription)
{
    if (note.isLocal) {
        return ExpungeNote(note, errorDescription);
    }

    if (!note.isDeleted) {
        errorDescription = QObject::tr("Note to be deleted from local storage "
                                       "is not marked as deleted one, rejecting "
                                       "to mark it deleted in local database");
        return false;
    }

    QSqlQuery query(m_sqlDatabase);
    query.prepare("UPDATE Notes SET isDeleted = 1, isDirty = 1 WHERE guid = ?");
    query.addBindValue(QString::fromStdString(note.en_note.guid));
    bool res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("Can't delete note in local storage database: ");

    return true;
}

bool LocalStorageManager::ExpungeNote(const Note &note, QString &errorDescription)
{
    if (!note.isLocal) {
        errorDescription = QObject::tr("Can't expunge non-local note");
        return false;
    }

    if (!note.isDeleted) {
        errorDescription = QObject::tr("Local note to be expunged is not marked as "
                                       "deleted one, rejecting to delete it from "
                                       "local database");
        return false;
    }

    QSqlQuery query(m_sqlDatabase);
    query.prepare("SELECT rowid FROM Notes WHERE guid = ?");
    query.addBindValue(QString::fromStdString(note.en_note.guid));
    bool res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("Can't find note to be expunged in local storage database: ");

    int rowId = -1;
    bool conversionResult = false;
    while(query.next()) {
        rowId = query.value(0).toInt(&conversionResult);
    }

    if (!conversionResult || (rowId < 0)) {
        errorDescription = QObject::tr("Can't get rowId of note to be expunged in Notes table");
        return false;
    }

    query.clear();
    query.prepare("DELETE FROM Notes WHERE rowid = ?");
    query.addBindValue(QVariant(rowId));

    res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("Can't expunge note from local storage database: ");

    return true;
}

#define CHECK_TAG(tag) \
    CHECK_GUID(tag.en_tag, "Tag"); \
    if (tag.en_tag.name.empty()) { \
        errorDescription = QObject::tr("Tag name is empty"); \
        return false; \
    }

// FIXME: reimplement all methods below to comply with new Evernote objects' signature

/*
bool LocalStorageManager::AddTag(const Tag & tag, QString & errorDescription)
{
    CHECK_TAG(tag);

    // Check the existence of tag prior to attempting to add it
    bool res = ReplaceTag(tag, errorDescription);
    if (res) {
        return true;
    }
    errorDescription.clear();

    Guid tagGuid = tag.guid();

    QSqlQuery query(m_sqlDatabase);
    query.prepare("INSERT INTO Tags (guid, usn, name, parentGuid, searchName, isDirty, "
                  "isLocal, isDeleted) VALUES(?, ?, ?, ?, ?, ?, ?, ?)");
    query.addBindValue(tagGuid.ToQString());
    query.addBindValue(tag.getUpdateSequenceNumber());
    query.addBindValue(tag.name());
    query.addBindValue((tag.parentGuid().isEmpty() ? QString() : tag.parentGuid().ToQString()));
    query.addBindValue(tag.name().toUpper());
    query.addBindValue(tag.isDirty());
    query.addBindValue(tag.isLocal());
    query.addBindValue(tag.isDeleted());

    res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("Can't add tag to local storage database: ");

    return true;
}

bool LocalStorageManager::AddTagToNote(const Tag & tag, const Note & note,
                                   QString & errorDescription)
{
    CHECK_TAG(tag);
    CHECK_NOTE(note);

    if (!note.labeledByTag(tag)) {
        errorDescription = QObject::tr("Can't add tag to note: note is not labeled by this tag");
        return false;
    }

    QSqlQuery query(m_sqlDatabase);
    query.prepare("INSERT OR REPLACE INTO NoteTags (note, tag) "
                  "    SELECT ?, tag FROM Tags WHERE tag = ?");
    query.addBindValue(note.guid().ToQString());
    query.addBindValue(tag.guid().ToQString());

    bool res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("Can't link tag to note in NoteTags table "
                                 "in local storage database: ");

    return true;
}

bool LocalStorageManager::ReplaceTag(const Tag & tag, QString & errorDescription)
{
    CHECK_TAG(tag);

    QSqlQuery query(m_sqlDatabase);
    query.prepare("SELECT rowid FROM Tags WHERE guid = ?");
    query.addBindValue(tag.guid().ToQString());
    bool res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("Can't check tag for existence in local storage database: ");

    if (!query.next()) {
        errorDescription = QObject::tr("Can't replace tag: can't find tag in local storage database");
        return false;
    }

    int rowId = query.value(0).toInt(&res);
    if (!res || (rowId < 0)) {
        errorDescription = QObject::tr("Can't replace tag: can't get row id from SQL query result");
        return false;
    }

    query.clear();
    query.prepare("INSERT OR REPLACE INTO Tags(guid, usn, name, parentGuid, searchName, "
                  "isDirty, isLocal, isDeleted) VALUES(?, ?, ?, ?, ?, ?, ?, ?)");
    query.addBindValue(tag.guid().ToQString());
    query.addBindValue(tag.getUpdateSequenceNumber());
    query.addBindValue(tag.name());
    query.addBindValue((tag.parentGuid().isEmpty() ? QString() : tag.parentGuid().ToQString()));
    query.addBindValue(tag.isDirty());
    query.addBindValue(tag.isLocal());
    query.addBindValue(tag.isDeleted());

    res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("Can't replace tag in local storage database: ");

    return true;
}

bool LocalStorageManager::DeleteTag(const Tag & tag, QString & errorDescription)
{
    if (tag.isLocal()) {
        return ExpungeTag(tag, errorDescription);
    }

    if (!tag.isDeleted()) {
        errorDescription = QObject::tr("Tag to be deleted from local storage "
                                       "is not marked as deleted one, rejecting "
                                       "to mark it deleted in local database");
        return false;
    }

    QSqlQuery query(m_sqlDatabase);
    query.prepare("UPDATE Tags SET isDeleted = 1, isDirty = 1 WHERE guid = ?");
    query.addBindValue(tag.guid().ToQString());
    bool res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("Can't delete tag in local storage database: ");

    return true;
}

bool LocalStorageManager::ExpungeTag(const Tag & tag, QString & errorDescription)
{
    if (!tag.isLocal()) {
        errorDescription = QObject::tr("Can't expunge non-local tag");
        return false;
    }

    if (!tag.isDeleted()) {
        errorDescription = QObject::tr("Local tag to be expunged is not marked as "
                                       "deleted one, rejecting to delete it from "
                                       "local database");
        return false;
    }

    QSqlQuery query(m_sqlDatabase);
    query.prepare("SELECT rowid FROM Tags WHERE guid = ?");
    query.addBindValue(tag.guid().ToQString());
    bool res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("Can't find tag to be expunged in local storage database: ");

    int rowId = -1;
    bool conversionResult = false;
    while(query.next()) {
        rowId = query.value(0).toInt(&conversionResult);
    }

    if (!conversionResult || (rowId < 0)) {
        errorDescription = QObject::tr("Can't get rowId of tag to be expunged in Tags table");
        return false;
    }

    query.clear();
    query.prepare("DELETE FROM Tags WHERE rowid = ?");
    query.addBindValue(QVariant(rowId));

    res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("Can't expunge tag from local storage database: ");

    return true;
}

#define CHECK_RESOURCE_ATTRIBUTES(resourceMetadata, resourceBinaryData) \
    CHECK_GUID(resourceMetadata, "ResourceMetadata"); \
    if (resourceMetadata.isEmpty()) { \
        errorDescription = QObject::tr("Resource metadata is empty"); \
        return false; \
    } \
    if (resourceBinaryData.isEmpty()) { \
        errorDescription = QObject::tr("Resource binary data is empty"); \
        return false; \
    }

bool LocalStorageManager::AddResource(const evernote::edam::Resource &resource,
                                  QString & errorDescription)
{
    CHECK_RESOURCE_ATTRIBUTES(resource, resourceBinaryData);

    // Check the existence of the resource prior to attempting to add it
    bool res = ReplaceResource(resource, resourceBinaryData, errorDescription);
    if (res) {
        return true;
    }
    errorDescription.clear();

    Guid resourceGuid = resource.guid();
    QSqlQuery query(m_sqlDatabase);
    query.prepare("INSERT INTO Resources (guid, hash, data, mime, width, height, "
                  "sourceUrl, timestamp, altitude, latitude, longitude, fileName, "
                  "isAttachment, noteGuid) VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, "
                  "?, ?, ?)");
    query.addBindValue(resourceGuid.ToQString());
    query.addBindValue(resource.dataHash());
    query.addBindValue(resourceBinaryData);
    query.addBindValue(resource.mimeType());
    query.addBindValue(QVariant(static_cast<int>(resource.width())));
    query.addBindValue(QVariant(static_cast<int>(resource.height())));
    query.addBindValue(resource.sourceUrl());
    query.addBindValue(QVariant(static_cast<int>(resource.timestamp())));
    query.addBindValue(resource.altitude());
    query.addBindValue(resource.latitude());
    query.addBindValue(resource.longitude());
    query.addBindValue(resource.filename());
    query.addBindValue(QVariant((resource.isAttachment() ? 1 : 0)));
    query.addBindValue(resource.noteGuid().ToQString());

    res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("Can't add resource to local storage database: ");

    return true;
}

bool LocalStorageManager::ReplaceResource(const evernote::edam::Resource &resource,
                                      QString & errorDescription)
{
    CHECK_RESOURCE_ATTRIBUTES(resource, resourceBinaryData);

    QSqlQuery query(m_sqlDatabase);
    query.prepare("SELECT rowid FROM Resources WHERE guid = ?");
    query.addBindValue(resource.guid().ToQString());
    bool res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("Can't check resource for existence in local storage database: ");

    if (!query.next()) {
        errorDescription = QObject::tr("Can't replace resource: can't find resource in local storage database");
        return false;
    }

    int rowId = query.value(0).toInt(&res);
    if (!res || (rowId < 0)) {
        errorDescription = QObject::tr("Can't replace resource: can't get row id from SQL query result");
        return false;
    }

    query.clear();
    query.prepare("INSERT OR REPLACE INTO Resources(guid, hash, data, mime, width, height, "
                  "sourceUrl, timestamp, altitude, latitude, longitude, fileName, "
                  "isAttachment, noteGuid) VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, "
                  "?, ?, ?)");
    query.addBindValue(resource.guid().ToQString());
    query.addBindValue(resource.dataHash());
    query.addBindValue(resourceBinaryData);
    query.addBindValue(resource.mimeType());
    query.addBindValue(QVariant(static_cast<int>(resource.width())));
    query.addBindValue(QVariant(static_cast<int>(resource.height())));
    query.addBindValue(resource.sourceUrl());
    query.addBindValue(QVariant(static_cast<int>(resource.timestamp())));
    query.addBindValue(resource.altitude());
    query.addBindValue(resource.latitude());
    query.addBindValue(resource.longitude());
    query.addBindValue(resource.filename());
    query.addBindValue(QVariant((resource.isAttachment() ? 1 : 0)));
    query.addBindValue(resource.noteGuid().ToQString());

    res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("Can't replace resource in local storage database: ");

    return true;
}

bool LocalStorageManager::DeleteResource(const evernote::edam::Resource &resource,
                                     QString & errorDescription)
{
    if (resource.isLocal()) {
        return ExpungeResource(resource, errorDescription);
    }

    if (!resource.isDeleted()) {
        errorDescription = QObject::tr("Metadata of the resource to be deleted from local storage "
                                       "is not marked deleted, rejecting to mark it deleted "
                                       "in local database");
        return false;
    }

    QSqlQuery query(m_sqlDatabase);
    query.prepare("UPDATE Resources SET isDeleted = 1, isDirty = 1 WHERE guid = ?");
    query.addBindValue(resource.guid().ToQString());
    bool res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("Can't delete resource in local storage database: ");

    return true;
}

bool LocalStorageManager::ExpungeResource(const evernote::edam::Resource &resource,
                                      QString & errorDescription)
{
    if (!resource.isLocal()) {
        errorDescription = QObject::tr("Can't expunge non-local resource");
        return false;
    }

    if (!resource.isDeleted()) {
        errorDescription = QObject::tr("Metadata of local resource to be deleted "
                                       "is not marked deleted, rejecting to delete it from "
                                       "local database");
        return false;
    }

    QSqlQuery query(m_sqlDatabase);
    query.prepare("SELECT rowid FROM Resources WHERE guid = ?");
    query.addBindValue(resource.guid().ToQString());
    bool res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("Can't find resource to be expunged in local storage database");

    // TODO: generalize this procedure as it's used in multiple places in code
    int rowId = -1;
    bool conversionResult = false;
    while(query.next()) {
        rowId = query.value(0).toInt(&conversionResult);
    }

    if (!conversionResult || (rowId < 0)) {
        errorDescription = QObject::tr("Can't get rowId of resource to be expunged in Resource table");
        return false;
    }

    query.clear();
    query.prepare("DELETE FROM Resources WHERE rowid = ?");
    query.addBindValue(QVariant(rowId));

    res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("Can't expunge resource from local storage database");

    return true;
}

#undef CHECK_RESOURCE_ATTRIBUTES
#undef CHECK_TAG_ATTRIBUTES
#undef CHECK_NOTE_ATTRIBUTES
#undef CHECK_GUID

bool LocalStorageManager::CreateTables(QString & errorDescription)
{
    QSqlQuery query(m_sqlDatabase);
    bool res;

    res = query.exec("CREATE TABLE IF NOT EXISTS Notebooks("
                     "  guid                  TEXT PRIMARY KEY   NOT NULL, "
                     "  updateSequenceNumber  INTEGER            NOT NULL, "
                     "  name                  TEXT               NOT NULL, "
                     "  creationTimestamp     INTEGER            NOT NULL, "
                     "  modificationTimestamp INTEGER            NOT NULL, "
                     "  isDirty               INTEGER            NOT NULL, "
                     "  isLocal               INTEGER            NOT NULL, "
                     "  isDeleted             INTEGER            NOT NULL, "
                     "  isDefault             INTEGER            DEFAULT 0, "
                     "  isLastUsed            INTEGER            DEFAULT 0"
                     ")");
    DATABASE_CHECK_AND_SET_ERROR("Can't create Notebooks table: ");

    res = query.exec("CREATE TRIGGER ReplaceNotebook BEFORE INSERT ON Notebooks"
                     "  BEGIN"
                     "  UPDATE Notebooks"
                     "  SET    updateSequenceNumber = NEW.updateSequenceNumber,
                     "         name = NEW.name, isDirty = NEW.isDirty, "
                     "         isLocal = NEW.isLocal, isDefault = NEW.isDefault, "
                     "         isDeleted = NEW.isDeleted, isLastUsed = NEW.isLastUsed, "
                     "         creationTimestamp = NEW.creationTimestamp, "
                     "         modificationTimestamp = NEW.modificationTimestamp"
                     "  WHERE  guid = NEW.guid;"
                     "  END");
    DATABASE_CHECK_AND_SET_ERROR("Can't create ReplaceNotebooks trigger: ");

    res = query.exec("CREATE VIRTUAL TABLE NoteText USING fts3("
                     "  guid              TEXT PRIMARY KEY     NOT NULL, "
                     "  title             TEXT, "
                     "  content           TEXT, "
                     "  notebookGuid REFERENCES Notebooks(guid) ON DELETE CASCADE ON UPDATE CASCADE"
                     ")");
    DATABASE_CHECK_AND_SET_ERROR("Can't create virtual table NoteText: ");

    res = query.exec("CREATE TABLE IF NOT EXISTS Notes("
                     "  guid                    TEXT PRIMARY KEY     NOT NULL, "
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

    // TODO: support applicationData (lazyMap) and classifications (string to string map) in this table
    res = query.exec("CREATE TABLE IF NOT EXISTS NoteAttributes("
                     "  subjectDate             INTEGER              DEFAULT 0, "
                     "  isSetSubjectDate        INTEGER              DEFAULT 0, "
                     "  latitude                REAL                 DEFAULT NULL, "
                     "  isSetLatitude           INTEGER              DEFAULT 0, "
                     "  longitude               REAL                 DEFAULT NULL, "
                     "  isSetLongitude          INTEGER              DEFAULT 0, "
                     "  altitude                REAL                 DEFUALT NULL, "
                     "  isSetAltitude           INTEGER              DEFAULT 0, "
                     "  author                  TEXT                 DEFAULT NULL, "
                     "  isSetAuthor             INTEGER              DEFAULT 0, "
                     "  source                  TEXT                 DEFAULT NULL, "
                     "  isSetSource             INTEGER              DEFAULT 0, "
                     "  sourceUrl               TEXT                 DEFAULT NULL, "
                     "  isSetSourceUrl          INTEGER              DEFAULT 0, "
                     "  sourceApplication       TEXT                 DEFAULT NULL, "
                     "  isSetSourceApplication  INTEGER              DEFAULT 0, "
                     "  shareDate               INTEGER              DEFAULT 0, "
                     "  isSetShareDate          INTEGER              DEFAULT 0, "
                     "  reminderOrder           INTEGER              DEFAULT 0, "
                     "  isSetReminderOrder      INTEGER              DEFAULT 0, "
                     "  reminderDoneTime        INTEGER              DEFAULT 0, "
                     "  isSetReminderDoneTime   INTEGER              DEFAULT 0, "
                     "  reminderTime            INTEGER              DEFAULT 0, "
                     "  isSetReminderTime       INTEGER              DEFAULT 0, "
                     "  placeName               TEXT                 DEFAULT NULL, "
                     "  isSetPlaceName          INTEGER              DEFAULT 0, "
                     "  contentClass            TEXT                 DEFAULT NULL, "
                     "  isSetContentClass       INTEGER              DEFAULT 0, "
                     "  lastEditedBy            TEXT                 DEFAULT NULL, "
                     "  isSetLastEditedBy       INTEGER              DEFAULT 0, "
                     "  creatorId               INTEGER              DEFAULT 0, "
                     "  isSetCreatorId          INTEGER              DEFAULT 0, "
                     "  lastEditorId            INTEGER              DEFAULT 0, "
                     "  isSetLastEditorId       INTEGER              DEFAULT 0, "
                     "  noteGuid REFERENCES Notes(guid) ON DELETE CASCADE ON UPDATE CASCADE"
                     ")");
    DATABASE_CHECK_AND_SET_ERROR("Can't create NoteAttributes table: ");

    res = query.exec("CREATE INDEX NotesNotebooks ON Notes(notebook)");
    DATABASE_CHECK_AND_SET_ERROR("Can't create index NotesNotebooks: ");

    res = query.exec("CREATE TABLE IF NOT EXISTS Resources("
                     "  guid              TEXT PRIMARY KEY     NOT NULL, "
                     "  hash              TEXT UNIQUE          NOT NULL, "
                     "  data              TEXT,                NOT NULL, "
                     "  mime              TEXT                 NOT NULL, "
                     "  width             INTEGER              NOT NULL, "
                     "  height            INTEGER              NOT NULL, "
                     "  sourceUrl         TEXT, "
                     "  timestamp         INTEGER              NOT NULL, "
                     "  altitude          REAL, "
                     "  latitude          REAL, "
                     "  longitude         REAL, "
                     "  fileName          TEXT, "
                     "  isAttachment      INTEGER              NOT NULL, "
                     "  note REFERENCES Notes(guid) ON DELETE CASCADE ON UPDATE CASCADE"
                     ")");
    DATABASE_CHECK_AND_SET_ERROR("Can't create Resources table: ");

    res = query.exec("CREATE INDEX ResourcesNote ON Resources(note)");
    DATABASE_CHECK_AND_SET_ERROR("Can't create ResourcesNote index: ");

    res = query.exec("CREATE TABLE IF NOT EXISTS Tags("
                     "  guid                  TEXT PRIMARY KEY     NOT NULL, "
                     "  updateSequenceNumber  INTEGER              NOT NULL, "
                     "  name                  TEXT                 NOT NULL, "
                     "  parentGuid            TEXT                 DEFAULT NULL, "
                     "  searchName            TEXT UNIQUE          NOT NULL, "
                     "  isDirty               INTEGER              NOT NULL, "
                     "  isLocal               INTEGER              NOT NULL, "
                     "  isDeleted             INTEGER              DEFAULT 0, "
                     ")");
    DATABASE_CHECK_AND_SET_ERROR("Can't create Tags table: ");

    res = query.exec("CREATE INDEX TagsSearchName ON Tags(searchName)");
    DATABASE_CHECK_AND_SET_ERROR("Can't create TagsSearchName index: ");

    res = query.exec("CREATE TABLE IF NOT EXISTS NoteTags("
                     "  note REFERENCES Notes(guid) ON DELETE CASCADE ON UPDATE CASCADE, "
                     "  tag  REFERENCES Tags(guid)  ON DELETE CASCADE ON UPDATE CASCADE, "
                     "  UNIQUE(note, tag) ON CONFLICT REPLACE"
                     ")");
    DATABASE_CHECK_AND_SET_ERROR("Can't create NoteTags table: ");

    res = query.exec("CREATE INDEX NoteTagsNote ON NoteTags(note)");
    DATABASE_CHECK_AND_SET_ERROR("Can't create NoteTagsNote index: ");

    return true;
}

void LocalStorageManager::NoteAttributesToQuery(const Note & note, QSqlQuery & query)
{
    query.addBindValue(note.guid().ToQString());
    query.addBindValue(note.getUpdateSequenceNumber());
    query.addBindValue(note.title());
    query.addBindValue(QVariant(static_cast<int>(note.isDirty() ? 1 : 0)));
    query.addBindValue(QVariant(static_cast<int>(note.isLocal() ? 1 : 0)));
    query.addBindValue(note.content());
    query.addBindValue(QVariant(static_cast<int>(note.creationTimestamp())));
    query.addBindValue(QVariant(static_cast<int>(note.modificationTimestamp())));
    query.addBindValue(QVariant(static_cast<int>(note.subjectDateTimestamp())));

    if (note.hasValidLocation())
    {
        query.addBindValue(note.altitude());
        query.addBindValue(note.latitude());
        query.addBindValue(note.longitude());
    }
    else
    {
        query.addBindValue(0.0);
        query.addBindValue(0.0);
        query.addBindValue(0.0);
    }

    query.addBindValue(note.author());
    query.addBindValue(note.source());
    query.addBindValue(note.sourceUrl());
    query.addBindValue(note.sourceApplication());
    query.addBindValue(QVariant(static_cast<int>(note.isDeleted() ? 1 : 0)));
    query.addBindValue(note.notebookGuid().ToQString());
}
*/

#undef DATABASE_CHECK_AND_SET_ERROR

}
