#include "DatabaseManager.h"
#include "DatabaseOpeningException.h"
#include "DatabaseSqlErrorException.h"
#include "../Note.h"
#include "../Notebook.h"

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
    const Guid guid = object.guid(); \
    if (guid.isEmpty()) { \
        errorDescription = QObject::tr(objectName " guid is empty"); \
        return false; \
    }

bool DatabaseManager::AddNotebook(const Notebook & notebook, QString & errorDescription)
{
    CHECK_GUID(notebook, "Notebook");

    // Check the existence of the notebook prior to attempt to add it
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

    if (query.next())
    {
        int rowId = query.value(0).toInt(&res);
        if (!res || (rowId < 0)) {
            errorDescription = QObject::tr("Can't get rowId from SQL selection query result");
            return false;
        }

        return ReplaceNotebookAtRowId(rowId, notebook, errorDescription);
    }
    else {
        errorDescription = QObject::tr("Can't replace notebook: can't find existing notebook");
        return false;
    }
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

    Guid noteGuid = note.guid();
    QSqlQuery query(m_sqlDatabase);

    QString noteExistenceCheckQueryStr("SELECT rowid FROM Notes WHERE guid = %1");
    noteExistenceCheckQueryStr = noteExistenceCheckQueryStr.arg("%1", noteGuid.ToQString());
    bool res = query.exec(noteExistenceCheckQueryStr);
    if (res)
    {
        return ReplaceNote(note, errorDescription);
    }
    else
    {
        QString noteAddQueryStr("INSERT INTO NoteText (guid, title, body, notebook) "
                                "VALUES(%1, %2, %3, %4)");
        noteAddQueryStr = noteAddQueryStr.arg("%1", noteGuid.ToQString());
        noteAddQueryStr = noteAddQueryStr.arg("%2", note.title());
        noteAddQueryStr = noteAddQueryStr.arg("%3", note.content());
        noteAddQueryStr = noteAddQueryStr.arg("%4", notebookGuid.ToQString());

        res = query.exec(noteAddQueryStr);
        if (!res)
        {
            errorDescription = QObject::tr("Can't add note to NoteText table in "
                                           "local storage database: ");
            errorDescription.append(m_sqlDatabase.lastError().text());
            return false;
        }

        noteAddQueryStr = QString("INSERT INTO Notes (guid, usn, title, isDirty, "
                                  "isLocal, body, creationTimestamp, modificationTimestamp, "
                                  "subjectDate, altitude, latitude, longitude, author, source, "
                                  "sourceUrl, sourceApplication, isDeleted, notebook) "
                                  "VALUES(%1, %2, %3, %4, %5, %6, %7, %8, %9, %10, "
                                  "%11, %12, %13, %14, %15, %16, %17, %18)");
        NoteAttributesToQueryString(note, noteAddQueryStr);

        res = query.exec(noteAddQueryStr);
        DATABASE_CHECK_AND_SET_ERROR("Can't add note to Notes table in local storage database: ");
    }

    return true;
}

bool DatabaseManager::ReplaceNote(const Note & note, QString & errorDescription)
{
    CHECK_NOTE_ATTRIBUTES(note);

    Guid noteGuid = note.guid();
    QSqlQuery query(m_sqlDatabase);

    QString noteExistenceCheckQueryStr("SELECT rowid FROM Notes WHERE guid = %1");
    noteExistenceCheckQueryStr = noteExistenceCheckQueryStr.arg("%1", noteGuid.ToQString());
    bool res = query.exec(noteExistenceCheckQueryStr);
    if (res)
    {
        QString noteTextReplaceQueryStr("INSERT OR REPLACE INTO NotesText (guid, title, "
                                        "body, notebook) VALUES(%1, %2, %3, %4)");
        noteTextReplaceQueryStr = noteTextReplaceQueryStr.arg("%1", note.guid().ToQString());
        noteTextReplaceQueryStr = noteTextReplaceQueryStr.arg("%2", note.title());
        noteTextReplaceQueryStr = noteTextReplaceQueryStr.arg("%3", note.content());
        noteTextReplaceQueryStr = noteTextReplaceQueryStr.arg("%4", note.notebookGuid().ToQString());

        res = query.exec(noteTextReplaceQueryStr);
        DATABASE_CHECK_AND_SET_ERROR("Can't replace note in NotesText table in local "
                                     "storage database: ");

        QString noteReplaceQueryStr("INSERT OR REPLACE INTO Notes (guid, usn, title, "
                                    "isDirty, isLocal, body, creationTimestamp, "
                                    "modificationTimestamp, subjectDate, altitude, "
                                    "latitude, longitude, author, source, sourceUrl, "
                                    "sourceApplication, isDeleted, notebook) "
                                    "VALUES(%1, %2, %3, %4, %5, %6, %7, %8, %9, %10, "
                                    "%11, %12, %13, %14, %15, %16, %17, %18)");
        NoteAttributesToQueryString(note, noteReplaceQueryStr);

        res = query.exec(noteReplaceQueryStr);
        DATABASE_CHECK_AND_SET_ERROR("Can't replace note in Notes table in local "
                                     "storage database: ");
        return true;
    }
    else
    {
        errorDescription = QObject::tr("Can't replace note: it doesn't exist yet");
        return false;
    }
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

    QString deleteNoteQueryStr("UPDATE Notes SET isDeleted = 1, isDirty = 1 WHERE guid = %1");
    deleteNoteQueryStr = deleteNoteQueryStr.arg("%1", note.guid().ToQString());

    QSqlQuery query(m_sqlDatabase);
    bool res = query.exec(deleteNoteQueryStr);
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

    QString findNoteToBeExpungedQueryStr("SELECT rowid FROM Notes WHERE guid = %1");
    findNoteToBeExpungedQueryStr = findNoteToBeExpungedQueryStr.arg("%1", note.guid().ToQString());

    QSqlQuery findNoteToBeExpungedQuery(m_sqlDatabase);
    bool res = findNoteToBeExpungedQuery.exec(findNoteToBeExpungedQueryStr);
    DATABASE_CHECK_AND_SET_ERROR("Can't find note to be expunged in local storage database: ");

    int rowId = -1;
    bool conversionResult = false;
    while(findNoteToBeExpungedQuery.next()) {
        rowId = findNoteToBeExpungedQuery.value(0).toInt(&conversionResult);
    }

    if (!conversionResult || (rowId < 0)) {
        errorDescription = QObject::tr("Can't get rowId of note to be expunged in Notes table");
        return false;
    }

    QString expungeNoteQueryStr("DELETE FROM Notes WHERE rowid = %1");
    expungeNoteQueryStr = expungeNoteQueryStr.arg("%1", QString::number(rowId));

    QSqlQuery expungeNoteQuery(m_sqlDatabase);
    res = expungeNoteQuery.exec(expungeNoteQueryStr);
    DATABASE_CHECK_AND_SET_ERROR("Can't expunge note from local storage database: ");

    return true;
}

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
                     "  isDefault             INTEGER            DEFAULT 0, "
                     "  isLastUsed            INTEGER            DEFAULT 0"
                     ")");
    DATABASE_CHECK_AND_SET_ERROR("Can't create Notebooks table: ");

    res = query.exec("CREATE TRIGGER ReplaceNotebook BEFORE INSERT ON Notebooks"
                     "  BEGIN"
                     "  UPDATE Notebooks"
                     "  SET    usn = NEW.usn, name = NEW.name, isDirty = NEW.isDirty, "
                     "         isLocal = NEW.isLocal, isDefault = NEW.isDefault, "
                     "         isLastUsed = NEW.isLastUsed, "
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
                     "  isDirty           INTEGER              NOT NULL"
                     ")");
    DATABASE_CHECK_AND_SET_ERROR("Can't create Tags table: ");

    res = query.exec("CREATE INDEX TagsSearchName ON Tags(searchName)");
    DATABASE_CHECK_AND_SET_ERROR("Can't create TassSearchName index: ");

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

bool DatabaseManager::ReplaceNotebookAtRowId(const int rowId, const Notebook & notebook,
                                             QString & errorDescription)
{
    CHECK_GUID(notebook, "Notebook");

    if (rowId < 0) {
        errorDescription = QObject::tr("Can't replace notebook: row id is negative");
        return false;
    }

    QSqlQuery query(m_sqlDatabase);
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

    bool res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("Can't update Notebooks table: ");

    return true;
}

void DatabaseManager::NoteAttributesToQueryString(const Note & note, QString & queryString)
{
    queryString = queryString.arg("%1", note.guid().ToQString());
    queryString = queryString.arg("%2", note.getUpdateSequenceNumber());
    queryString = queryString.arg("%3", note.title());
    queryString = queryString.arg("%4", (note.isDirty() ? QString::number(1) : QString::number(0)));
    queryString = queryString.arg("%5", (note.isLocal() ? QString::number(1) : QString::number(0)));
    queryString = queryString.arg("%6", note.content());
    queryString = queryString.arg("%7", note.createdTimestamp());
    queryString = queryString.arg("%8", note.updatedTimestamp());
    queryString = queryString.arg("%9", note.subjectDateTimestamp());

    if (note.hasValidLocation())
    {
        queryString = queryString.arg("%10", note.altitude());
        queryString = queryString.arg("%11", note.latitude());
        queryString = queryString.arg("%12", note.longitude());
    }
    else
    {
        queryString = queryString.arg("%10", QString());
        queryString = queryString.arg("%11", QString());
        queryString = queryString.arg("%12", QString());
    }

    queryString = queryString.arg("%13", note.author());
    queryString = queryString.arg("%14", note.source());
    queryString = queryString.arg("%15", note.sourceUrl());
    queryString = queryString.arg("%16", note.sourceApplication());
    queryString = queryString.arg("%17", (note.isDeleted()
                                                  ? QString::number(1)
                                                  : QString::number(0)));
    queryString = queryString.arg("%18", note.notebookGuid().ToQString());
}

#undef DATABASE_CHECK_AND_SET_ERROR

}
