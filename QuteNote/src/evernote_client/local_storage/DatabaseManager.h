#ifndef __QUTE_NOTE__EVERNOTE_CLIENT__LOCAL_STORAGE__DATABASE_MANAGER
#define __QUTE_NOTE__EVERNOTE_CLIENT__LOCAL_STORAGE__DATABASE_MANAGER

#include <QString>
#include <QtSql>

namespace qute_note {

class Note;

// TODO: implement all the necessary functionality
class DatabaseManager
{
public:
    DatabaseManager(const QString & username);
    ~DatabaseManager();

    bool AddNote(const Note & note, QString & errorDescription);
    bool ReplaceNote(const Note & note, QString & errorDescription);

private:
    bool CreateTables(QString & errorDescription);
    void NoteAttributesToQueryString(const Note & note, QString & queryString);

    DatabaseManager() = delete;
    DatabaseManager(const DatabaseManager & other) = delete;
    DatabaseManager & operator=(const DatabaseManager & other) = delete;

    QString m_applicationPersistenceStoragePath;
    QSqlDatabase m_sqlDatabase;
};

}

#endif // __QUTE_NOTE__EVERNOTE_CLIENT__LOCAL_STORAGE__DATABASE_MANAGER
