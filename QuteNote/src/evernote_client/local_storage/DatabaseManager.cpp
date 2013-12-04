#include "DatabaseManager.h"
#include "DatabaseOpeningException.h"
#include "DatabaseSqlErrorException.h"
#include "../Note.h"

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

bool DatabaseManager::AddNote(const Note & note, QString & errorDescription)
{
    const Guid notebookGuid = note.notebookGuid();
    if (notebookGuid.isEmpty()) {
        errorDescription = QObject::tr("Notebook guid is empty");
        return false;
    }

    const Guid noteGuid = note.guid();
    if (noteGuid.isEmpty()) {
        errorDescription = QObject::tr("Note guid is empty");
        return false;
    }

    QSqlQuery query(m_sqlDatabase);

    QString noteExistenceCheckQueryStr("SELECT ROWID FROM Notes WHERE guid = %1");
    noteExistenceCheckQueryStr = noteExistenceCheckQueryStr.arg("%1", noteGuid.ToQString());
    bool res = query.exec(noteExistenceCheckQueryStr);
    if (res)
    {
        // TODO: Replace note
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
                                  "body, creationDate, modificationDate, subjectDate, "
                                  "altitude, latitude, longitude, author, source, "
                                  "sourceUrl, sourceApplication, isDeleted, notebook) "
                                  "VALUES(%1, %2, %3, %4, %5, %6, %7, %8, %9, %10, "
                                  "%11, %12, %13, %14, %15, %16, %17)");
        noteAddQueryStr = noteAddQueryStr.arg("%1", noteGuid.ToQString());
        noteAddQueryStr = noteAddQueryStr.arg("%2", note.getUpdateSequenceNumber());
        noteAddQueryStr = noteAddQueryStr.arg("%3", note.title());
        noteAddQueryStr = noteAddQueryStr.arg("%4", (note.isDirty() ? QString::number(1) : QString::number(0)));
        noteAddQueryStr = noteAddQueryStr.arg("%5", note.content());
        noteAddQueryStr = noteAddQueryStr.arg("%6", note.createdTimestamp());
        noteAddQueryStr = noteAddQueryStr.arg("%7", note.updatedTimestamp());
        noteAddQueryStr = noteAddQueryStr.arg("%8", note.subjectDateTimestamp());

        if (note.hasValidLocation())
        {
            noteAddQueryStr = noteAddQueryStr.arg("%9", note.altitude());
            noteAddQueryStr = noteAddQueryStr.arg("%10", note.latitude());
            noteAddQueryStr = noteAddQueryStr.arg("%11", note.longitude());
        }
        else
        {
            noteAddQueryStr = noteAddQueryStr.arg("%9", QString());
            noteAddQueryStr = noteAddQueryStr.arg("%10", QString());
            noteAddQueryStr = noteAddQueryStr.arg("%11", QString());
        }

        noteAddQueryStr = noteAddQueryStr.arg("%12", note.author());
        noteAddQueryStr = noteAddQueryStr.arg("%13", note.source());
        noteAddQueryStr = noteAddQueryStr.arg("%14", note.sourceUrl());
        noteAddQueryStr = noteAddQueryStr.arg("%15", note.sourceApplication());
        noteAddQueryStr = noteAddQueryStr.arg("%16", (note.isDeleted()
                                                      ? QString::number(1)
                                                      : QString::number(0)));
        noteAddQueryStr = noteAddQueryStr.arg("%17", note.notebookGuid().ToQString());

        res = query.exec(noteAddQueryStr);
        if (!res)
        {
            errorDescription = QObject::tr("Can't add note to Notes table in "
                                           "local storage database: ");
            errorDescription.append(m_sqlDatabase.lastError().text());
            return false;
        }
    }

    return true;
}

bool DatabaseManager::CreateTables(QString & errorDescription)
{
    QSqlQuery query(m_sqlDatabase);
    bool res;

#define DATABASE_CHECK_AND_SET_ERROR() \
    if (!res) { \
        errorDescription = m_sqlDatabase.lastError().text(); \
        return false; \
    }

    res = query.exec("CREATE TABLE IF NOT EXISTS Notebooks("
                     "  guid             TEXT PRIMARY KEY   NOT NULL, "
                     "  usn              INTEGER            NOT NULL, "
                     "  name             TEXT               NOT NULL, "
                     "  creationDate     INTEGER            NOT NULL, "
                     "  modificationDate INTEGER            NOT NULL, "
                     "  isDirty          INTEGER            NOT NULL, "
                     "  updateCount      INTEGER            DEFAULT 0, "
                     "  isDefault        INTEGER            DEFAULT 0, "
                     "  isLastUsed       INTEGER            DEFAULT 0"
                     ")");
    DATABASE_CHECK_AND_SET_ERROR()

    res = query.exec("CREATE TRIGGER ReplaceNotebook BEFORE INSERT ON Notebooks"
                     "  BEGIN"
                     "  UPDATE Notebooks"
                     "  SET    usn = NEW.usn, name = NEW.name, updateCount = NEW.updateCount,"
                     "         isDirty = NEW.isDirty, isDefault = NEW.isDefault,"
                     "         isLastUsed = NEW.isLastUsed, creationDate = NEW.creationDate,"
                     "         modificationDate = NEW.modificationDate"
                     "  WHERE  guid = NEW.guid;"
                     "  END");
    DATABASE_CHECK_AND_SET_ERROR()

    res = query.exec("CREATE VIRTUAL TABLE NoteText USING fts3("
                     "  guid              TEXT PRIMARY KEY     NOT NULL, "
                     "  title             TEXT, "
                     "  body              TEXT, "
                     "  notebook REFERENCES Notebooks(guid) ON DELETE CASCADE ON UPDATE CASCADE"
                     ")");
    DATABASE_CHECK_AND_SET_ERROR()

    res = query.exec("CREATE TABLE IF NOT EXISTS Notes("
                     "  guid              TEXT PRIMARY KEY     NOT NULL, "
                     "  usn               INTEGER              NOT NULL, "
                     "  title             TEXT, "
                     "  isDirty           INTEGER              NOT NULL, "
                     "  body              TEXT, "
                     "  creationDate      TEXT                 NOT NULL, "
                     "  modificationDate  TEXT                 NOT NULL, "
                     "  subjectDate       TEXT                 NOT NULL, "
                     "  altitude          REAL, "
                     "  latitude          REAL, "
                     "  longitude         REAL, "
                     "  author            TEXT                 NOT NULL, "
                     "  source            TEXT, "
                     "  sourceUrl,        TEXT, "
                     "  sourceApplication TEXT, "
                     "  isDeleted         INTEGER              DEFAULT 0, "
                     "  notebook REFERENCES Notebooks(guid) ON DELETE CASCADE ON UPDATE CASCADE"
                     ")");
    DATABASE_CHECK_AND_SET_ERROR()

    res = query.exec("CREATE INDEX NotesNotebooks ON Notes(notebook)");
    DATABASE_CHECK_AND_SET_ERROR()

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
    DATABASE_CHECK_AND_SET_ERROR()

    res = query.exec("CREATE INDEX ResourcesNote ON Resources(note)");
    DATABASE_CHECK_AND_SET_ERROR()

    res = query.exec("CREATE TABLE IF NOT EXISTS Tags("
                     "  guid              TEXT PRIMARY KEY     NOT NULL, "
                     "  usn               INTEGER              NOT NULL, "
                     "  name              TEXT                 NOT NULL, "
                     "  parentGuid        TEXT"
                     "  searchName        TEXT UNIQUE          NOT NULL, "
                     "  isDirty           INTEGER              NOT NULL"
                     ")");
    DATABASE_CHECK_AND_SET_ERROR()

    res = query.exec("CREATE INDEX TagsSearchName ON Tags(searchName)");
    DATABASE_CHECK_AND_SET_ERROR()

    res = query.exec("CREATE TABLE IF NOT EXISTS NoteTags("
                     "  note REFERENCES Notes(guid) ON DELETE CASCADE ON UPDATE CASCADE, "
                     "  tag  REFERENCES Tags(guid)  ON DELETE CASCADE ON UPDATE CASCADE, "
                     "  UNIQUE(note, tag) ON CONFLICT REPLACE"
                     ")");
    DATABASE_CHECK_AND_SET_ERROR()

    res = query.exec("CREATE INDEX NoteTagsNote ON NoteTags(note)");
    DATABASE_CHECK_AND_SET_ERROR()

#undef DATABASE_CHECK_AND_SET_ERROR

    return true;
}

}
