#ifndef __QUTE_NOTE__EVERNOTE_CLIENT__LOCAL_STORAGE__DATABASE_MANAGER
#define __QUTE_NOTE__EVERNOTE_CLIENT__LOCAL_STORAGE__DATABASE_MANAGER

#include <QString>
#include <QtSql>

namespace qute_note {

class Notebook;
class Note;

// TODO: implement all the necessary functionality
class DatabaseManager
{
public:
    DatabaseManager(const QString & username);
    ~DatabaseManager();

    bool AddNotebook(const Notebook & notebook, QString & errorDescription);
    bool ReplaceNotebook(const Notebook & notebook, QString & errorDescription);

    /**
     * @brief DeleteNotebook - either expunges the local notebook (i.e. deleted it from
     * local storage completely, without the possibility to restore)
     * if it has not been synchronized with Evernote service yet or marks
     * the note as deleted otherwise. Evernote API doesn't allow thirdparty applications
     * to expunge notebooks which have ever been synchronized with remote
     * data store at least once
     * @param notebook - notebook to be deleted
     * @param errorDescription - error description if notebook could not be deleted
     * @return true if notebook was deleted successfully, false otherwise
     */
    bool DeleteNotebook(const Notebook & notebook, QString & errorDescription);

    /**
     * @brief ExpungeNotebook- permanently deletes local notebooks i.e. notebooks which
     * have not yet been synchronized with Evernote service. This method deletes
     * the notebook from local storage completely, without the possibility to restore it
     * @param notebook - notebook to be expunged
     * @param errorDescription - error description if notebook could not be expunged
     * @return true if notebook was expunged successfully, false otherwise
     */
    bool ExpungeNotebook(const Notebook & notebook, QString & errorDescription);

    bool AddNote(const Note & note, QString & errorDescription);
    bool ReplaceNote(const Note & note, QString & errorDescription);

    /**
     * @brief DeleteNote - either expunges the local note (i.e. deletes it from
     * local storage completely, without the possibility to restore)
     * if it has not been synchronized with Evernote service yet or marks
     * the note as deleted otherwise. Evernote API doesn't allow thirdparty
     * applications to expunge notes which have ever been synchronized
     * with remote data store at least once
     * @param note - note to be deleted
     * @param errorDescription - error description if note could not be deleted
     * @return true if note was deleted successfully, false otherwise
     */
    bool DeleteNote(const Note & note, QString & errorDescription);

    /**
     * @brief ExpungeNote - permanently deletes local notes i.e. notes which
     * have not yet been synchronized with Evernote service. This method deletes
     * the note from local storage completely, without the possibility to restore it
     * @param note - note to be expunged
     * @param errorDescription - error description if note could not be expunged
     * @return true if note was expunged successfully, false otherwise
     */
    bool ExpungeNote(const Note & note, QString & errorDescription);

private:
    bool CreateTables(QString & errorDescription);
    bool ReplaceNotebookAtRowId(const int rowId, const Notebook & notebook,
                                QString & errorDescription);
    void NoteAttributesToQueryString(const Note & note, QString & queryString);

    DatabaseManager() = delete;
    DatabaseManager(const DatabaseManager & other) = delete;
    DatabaseManager & operator=(const DatabaseManager & other) = delete;

    QString m_applicationPersistenceStoragePath;
    QSqlDatabase m_sqlDatabase;
};

}

#endif // __QUTE_NOTE__EVERNOTE_CLIENT__LOCAL_STORAGE__DATABASE_MANAGER
