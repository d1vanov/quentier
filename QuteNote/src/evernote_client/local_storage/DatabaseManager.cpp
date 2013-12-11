#include "DatabaseManager.h"
#include "DatabaseOpeningException.h"
#include "DatabaseSqlErrorException.h"
#include "../Note.h"
#include "../Notebook.h"
#include "../Tag.h"

#ifdef USE_QT5
#include <QStandardPaths>
#else
#include <QDesktopServices>
#endif

namespace qute_note {

#define QUTE_NOTE_DATABASE_NAME "qn.storage.sqlite"

DatabaseManager::DatabaseManager(const QString & username) :
    m_applicationPersistenceStoragePath(QDesktopServices::storageLocation(QDesktopServices::DataLocation))
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

DatabaseManager::~DatabaseManager()
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

#define CHECK_GUID(object, objectName) \
    {\
        const Guid guid = object.guid(); \
        if (guid.isEmpty()) { \
            errorDescription = QObject::tr(objectName " guid is empty"); \
            return false; \
        } \
    }

bool DatabaseManager::AddNotebook(const Notebook & notebook, QString & errorDescription)
{
    CHECK_GUID(notebook, "Notebook");

    // Check the existence of the notebook prior to attempting to add it
    bool res = ReplaceNotebook(notebook, errorDescription);
    if (res) {
        return true;
    }
    errorDescription.clear();

    QSqlQuery query(m_sqlDatabase);
    query.prepare("INSERT INTO Notebooks (guid, usn, name, creationTimestamp, "
                  "modificationTimestamp, isDirty, isLocal, isDefault, isLastUsed) "
                  "VALUES(?, ?, ?, ?, ?, ?, ?, 0, 0");
    query.addBindValue(notebook.guid().ToQString());
    query.addBindValue(notebook.getUpdateSequenceNumber());
    query.addBindValue(notebook.name());
    query.addBindValue(QVariant(static_cast<int>(notebook.createdTimestamp())));
    query.addBindValue(QVariant(static_cast<int>(notebook.updatedTimestamp())));
    query.addBindValue(notebook.isDirty());
    query.addBindValue(notebook.isLocal());

    res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("Can't insert new notebook into local storage database: ");

    return true;
}

bool DatabaseManager::ReplaceNotebook(const Notebook & notebook, QString & errorDescription)
{
    CHECK_GUID(notebook, "Notebook");

    QSqlQuery query(m_sqlDatabase);
    query.prepare("SELECT rowid FROM Notebooks WHERE guid = ?");
    query.addBindValue(notebook.guid().ToQString());

    bool res = query.exec();
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
                  "modificationTimestamp=?, isDirty=?, isLocal=?, isDefault=?, isLastUsed=?");
    query.addBindValue(notebook.guid().ToQString());
    query.addBindValue(notebook.getUpdateSequenceNumber());
    query.addBindValue(notebook.name());
    query.addBindValue(QVariant(static_cast<int>(notebook.createdTimestamp())));
    query.addBindValue(QVariant(static_cast<int>(notebook.updatedTimestamp())));
    query.addBindValue((notebook.isDirty() ? 1 : 0));
    query.addBindValue((notebook.isLocal() ? 1 : 0));
    query.addBindValue((notebook.isDefault() ? 1 : 0));
    query.addBindValue((notebook.isLastUsed() ? 1 : 0));

    res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("Can't update Notebooks table: ");

    return true;
}

bool DatabaseManager::DeleteNotebook(const Notebook & notebook, QString & errorDescription)
{
    if (notebook.isLocal()) {
        return ExpungeNotebook(notebook, errorDescription);
    }

    if (!notebook.isDeleted()) {
        errorDescription = QObject::tr("Notebook to be deleted from local storage "
                                       "is not marked as deleted one, rejecting "
                                       "to mark it deleted in local database");
        return false;
    }

    QSqlQuery query(m_sqlDatabase);
    query.prepare("UPDATE Notebooks SET isDeleted = 1, isDirty = 1 WHERE guid = ?");
    query.addBindValue(notebook.guid().ToQString());
    bool res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("Can't delete notebook in local storage database: ");

    return true;
}

bool DatabaseManager::ExpungeNotebook(const Notebook & notebook, QString & errorDescription)
{
    if (!notebook.isLocal()) {
        errorDescription = QObject::tr("Can't expunge non-local notebook");
        return false;
    }

    if (!notebook.isDeleted()) {
        errorDescription = QObject::tr("Local notebook to be expunged is not marked as "
                                       "deleted one, rejecting to delete it from "
                                       "local database");
        return false;
    }

    QSqlQuery query(m_sqlDatabase);
    query.prepare("SELECT rowid FROM Notebooks WHERE guid = ?");
    query.addBindValue(notebook.guid().ToQString());
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

#define CHECK_NOTE_ATTRIBUTES(note) \
    const Guid notebookGuid = note.notebookGuid(); \
    if (notebookGuid.isEmpty()) { \
        errorDescription = QObject::tr("Notebook guid is empty"); \
        return false; \
    } \
    CHECK_GUID(note, "Note")

bool DatabaseManager::AddNote(const Note & note, QString & errorDescription)
{
    CHECK_NOTE_ATTRIBUTES(note);

    // Check the existence of the note prior to attempting to add it
    bool res = ReplaceNote(note, errorDescription);
    if (res) {
        return true;
    }
    errorDescription.clear();

    Guid noteGuid = note.guid();

    QSqlQuery query(m_sqlDatabase);
    query.prepare("INSERT INTO NoteText (guid, title, body, notebook) "
                  "VALUES(?, ?, ?, ?)");
    query.addBindValue(noteGuid.ToQString());
    query.addBindValue(note.title());
    query.addBindValue(note.content());
    query.addBindValue(notebookGuid.ToQString());
    res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("Can't add note to NoteText table in local storage database: ");

    query.clear();
    query.prepare("INSERT INTO Notes (guid, usn, title, isDirty, "
                  "isLocal, body, creationTimestamp, modificationTimestamp, "
                  "subjectDate, altitude, latitude, longitude, author, source, "
                  "sourceUrl, sourceApplication, isDeleted, notebook) "
                  "VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, "
                  "?, ?, ?, ?, ?, ?, ?, ?)");
    NoteAttributesToQuery(note, query);

    res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("Can't add note to Notes table in local storage database: ");

    return true;
}

bool DatabaseManager::ReplaceNote(const Note & note, QString & errorDescription)
{
    CHECK_NOTE_ATTRIBUTES(note);

    Guid noteGuid = note.guid();

    QSqlQuery query(m_sqlDatabase);
    query.prepare("SELECT rowid FROM Notes WHERE guid = ?");
    query.addBindValue(noteGuid.ToQString());
    bool res = query.exec();
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
                  "body, notebook) VALUES(?, ?, ?, ?)");
    query.addBindValue(noteGuid.ToQString());
    query.addBindValue(note.title());
    query.addBindValue(note.content());
    query.addBindValue(note.notebookGuid().ToQString());

    res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("Can't replace note in NotesText table in local "
                                 "storage database: ");

    query.clear();
    query.prepare("INSERT OR REPLACE INTO Notes (guid, usn, title, "
                  "isDirty, isLocal, body, creationTimestamp, "
                  "modificationTimestamp, subjectDate, altitude, "
                  "latitude, longitude, author, source, sourceUrl, "
                  "sourceApplication, isDeleted, notebook) "
                  "VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, "
                  "?, ?, ?, ?, ?, ?, ?, ?)");
    NoteAttributesToQuery(note, query);

    res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("Can't replace note in Notes table in local "
                                 "storage database: ");

    return true;
}

bool DatabaseManager::DeleteNote(const Note & note, QString & errorDescription)
{
    if (note.isLocal()) {
        return ExpungeNote(note, errorDescription);
    }

    if (!note.isDeleted()) {
        errorDescription = QObject::tr("Note to be deleted from local storage "
                                       "is not marked as deleted one, rejecting "
                                       "to mark it deleted in local database");
        return false;
    }

    QSqlQuery query(m_sqlDatabase);
    query.prepare("UPDATE Notes SET isDeleted = 1, isDirty = 1 WHERE guid = ?");
    query.addBindValue(note.guid().ToQString());
    bool res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("Can't delete note in local storage database: ");

    return true;
}

bool DatabaseManager::ExpungeNote(const Note &note, QString &errorDescription)
{
    if (!note.isLocal()) {
        errorDescription = QObject::tr("Can't expunge non-local note");
        return false;
    }

    if (!note.isDeleted()) {
        errorDescription = QObject::tr("Local note to be expunged is not marked as "
                                       "deleted one, rejecting to delete it from "
                                       "local database");
        return false;
    }

    QSqlQuery query(m_sqlDatabase);
    query.prepare("SELECT rowid FROM Notes WHERE guid = ?");
    query.addBindValue(note.guid().ToQString());
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

#define CHECK_TAG_ATTRIBUTES(tag) \
    CHECK_GUID(tag, "Tag"); \
    if (tag.isEmpty()) { \
        errorDescription = QObject::tr("Tag is empty"); \
        return false; \
    }

bool DatabaseManager::AddTag(const Tag & tag, QString & errorDescription)
{
    CHECK_TAG_ATTRIBUTES(tag);

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

bool DatabaseManager::AddTagToNote(const Tag & tag, const Note & note,
                                   QString & errorDescription)
{
    CHECK_TAG_ATTRIBUTES(tag);
    CHECK_NOTE_ATTRIBUTES(note);

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

bool DatabaseManager::ReplaceTag(const Tag & tag, QString & errorDescription)
{
    CHECK_TAG_ATTRIBUTES(tag);

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

#undef CHECK_TAG_ATTRIBUTES
#undef CHECK_NOTE_ATTRIBUTES
#undef CHECK_GUID

bool DatabaseManager::CreateTables(QString & errorDescription)
{
    QSqlQuery query(m_sqlDatabase);
    bool res;

    res = query.exec("CREATE TABLE IF NOT EXISTS Notebooks("
                     "  guid                  TEXT PRIMARY KEY   NOT NULL, "
                     "  usn                   INTEGER            NOT NULL, "
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
                     "  SET    usn = NEW.usn, name = NEW.name, isDirty = NEW.isDirty, "
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
                     "  body              TEXT, "
                     "  notebook REFERENCES Notebooks(guid) ON DELETE CASCADE ON UPDATE CASCADE"
                     ")");
    DATABASE_CHECK_AND_SET_ERROR("Can't create virtual table NoteText: ");

    res = query.exec("CREATE TABLE IF NOT EXISTS Notes("
                     "  guid                    TEXT PRIMARY KEY     NOT NULL, "
                     "  usn                     INTEGER              NOT NULL, "
                     "  title                   TEXT, "
                     "  isDirty                 INTEGER              NOT NULL, "
                     "  isLocal                 INTEGER              NOT NULL, "
                     "  body                    TEXT, "
                     "  creationTimestamp       INTEGER              NOT NULL, "
                     "  modificationTimestamp   INTEGER              NOT NULL, "
                     "  subjectDate             INTEGER              NOT NULL, "
                     "  altitude                REAL, "
                     "  latitude                REAL, "
                     "  longitude               REAL, "
                     "  author                  TEXT                 NOT NULL, "
                     "  source                  TEXT, "
                     "  sourceUrl,              TEXT, "
                     "  sourceApplication       TEXT, "
                     "  isDeleted               INTEGER              DEFAULT 0, "
                     "  notebook REFERENCES Notebooks(guid) ON DELETE CASCADE ON UPDATE CASCADE"
                     ")");
    DATABASE_CHECK_AND_SET_ERROR("Can't create Notes table: ");

    res = query.exec("CREATE INDEX NotesNotebooks ON Notes(notebook)");
    DATABASE_CHECK_AND_SET_ERROR("Can't create index NotesNotebooks: ");

    res = query.exec("CREATE TABLE IF NOT EXISTS Resources("
                     "  guid              TEXT PRIMARY KEY     NOT NULL, "
                     "  hash              TEXT UNIQUE          NOT NULL, "
                     "  data              TEXT, "
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
                     "  guid              TEXT PRIMARY KEY     NOT NULL, "
                     "  usn               INTEGER              NOT NULL, "
                     "  name              TEXT                 NOT NULL, "
                     "  parentGuid        TEXT"
                     "  searchName        TEXT UNIQUE          NOT NULL, "
                     "  isDirty           INTEGER              NOT NULL, "
                     "  isLocal           INTEGER              NOT NULL, "
                     "  isDeleted         INTEGER              DEFAULT 0, "
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

void DatabaseManager::NoteAttributesToQuery(const Note & note, QSqlQuery & query)
{
    query.addBindValue(note.guid().ToQString());
    query.addBindValue(note.getUpdateSequenceNumber());
    query.addBindValue(note.title());
    query.addBindValue(QVariant(static_cast<int>(note.isDirty() ? 1 : 0)));
    query.addBindValue(QVariant(static_cast<int>(note.isLocal() ? 1 : 0)));
    query.addBindValue(note.content());
    query.addBindValue(QVariant(static_cast<int>(note.createdTimestamp())));
    query.addBindValue(QVariant(static_cast<int>(note.updatedTimestamp())));
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

#undef DATABASE_CHECK_AND_SET_ERROR

}
