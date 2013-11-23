#include "DatabaseManager.h"
#include "DatabaseOpeningException.h"
#include "DatabaseSqlErrorException.h"

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

bool DatabaseManager::CreateTables(QString & errorDescription)
{
    QSqlQuery query(m_sqlDatabase);
    bool res;

#define DATABASE_CHECK_AND_SET_ERROR() \
    if (!res) { \
        errorDescription = m_sqlDatabase.lastError().text(); \
        return false; \
    }

    res = query.exec("CREATE TABLE IF NOT EXISTS Notebooks (guid PRIMARY KEY, usn, "
                     "name, creationDate, modificationDate, isDirty, "
                     "updateCount DEFAULT 0, isDefault DEFAULT 0, isLastUsed DEFAULT 0)");
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

    query.exec("CREATE TABLE IF NOT EXISTS Notes (guid PRIMARY KEY, usn, title, "
               "isDirty, body, creationDate, modificationDate, subjectDate, "
               "altitude, latitude, longitude, author, source, sourceUrl, "
               "sourceApplication, isDeleted DEFAULT 0, "
               "notebook REFERENCES Notebooks(guid), ON DELETE CASCADE "
               "ON UPDATE CASCADE)");
    DATABASE_CHECK_AND_SET_ERROR()

    res = query.exec("CREATE INDEX NotesNotebooks ON Notes(notebook)");
    DATABASE_CHECK_AND_SET_ERROR()

    // TODO: proceed with other tables

#undef DATABASE_CHECK_AND_SET_ERROR

    return true;
}

}
