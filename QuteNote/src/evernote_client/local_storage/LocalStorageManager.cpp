#include "LocalStorageManager.h"
#include "DatabaseOpeningException.h"
#include "DatabaseSqlErrorException.h"
#include "../NoteStore.h"
#include "../tools/QuteNoteNullPtrException.h"

using namespace evernote::edam;

#ifdef USE_QT5
#include <QStandardPaths>
#else
#include <QDesktopServices>
#endif

namespace qute_note {

#define QUTE_NOTE_DATABASE_NAME "qn.storage.sqlite"

LocalStorageManager::LocalStorageManager(const QString & username, const QString & authenticationToken,
                                         QSharedPointer<evernote::edam::NoteStoreClient> & pNoteStore) :
    m_authenticationToken(authenticationToken),
    m_pNoteStore(pNoteStore),
#ifdef USE_QT5
    m_applicationPersistenceStoragePath(QStandardPaths::writableLocation(QStandardPaths::DataLocation))
#else
    m_applicationPersistenceStoragePath(QDesktopServices::storageLocation(QDesktopServices::DataLocation))
#endif
{
    if (m_pNoteStore == nullptr) {
        throw QuteNoteNullPtrException("Found null pointer to NoteStoreClient while "
                                       "trying to create LocalStorageManager instance");
    }

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

void LocalStorageManager::SetNewAuthenticationToken(const QString & authenticationToken)
{
    m_authenticationToken = authenticationToken;
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
    res = UpdateNotebook(notebook, errorDescription);
    if (res) {
        return true;
    }
    errorDescription.clear();

    const evernote::edam::Notebook & enNotebook = notebook.en_notebook;

    QSqlQuery query(m_sqlDatabase);
    // TODO: need to support all other information available about objects of notebook class
    query.prepare("INSERT INTO Notebooks (guid, updateSequenceNumber, name, "
                  "creationTimestamp, modificationTimestamp, isDirty, isLocal, "
                  "isDefault, isLastUsed) VALUES(?, ?, ?, ?, ?, ?, ?, 0, 0");
    query.addBindValue(QString::fromStdString(enNotebook.guid));
    query.addBindValue(enNotebook.updateSequenceNum);
    query.addBindValue(QString::fromStdString(enNotebook.name));
    query.addBindValue(QVariant(static_cast<int>(enNotebook.serviceCreated)));
    query.addBindValue(QVariant(static_cast<int>(enNotebook.serviceUpdated)));
    query.addBindValue(notebook.isDirty);
    query.addBindValue(notebook.isLocal);

    res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("Can't insert new notebook into local storage database: ");

    // bool hasOptionalFields = false;  // TODO: uncomment
    query.prepare("UPDATE Notebooks SET (:values) WHERE guid = :notebookGuid");
    query.bindValue("notebookGuid", QVariant(QString::fromStdString(enNotebook.guid)));

    // TODO: examine enNotebook: if it contains any optional fields, these should be added to query
    // in the form columnName=value. If at least one optional field is present, the resulting
    // query should be executed but it shouldn't be if no optional fields are present
    //
    // Think this should be moved to a separate method since it's reusable from UpdateNotebook too

    return true;
}

bool LocalStorageManager::UpdateNotebook(const Notebook & notebook, QString & errorDescription)
{
    bool res = notebook.CheckParameters(errorDescription);
    if (!res) {
        return false;
    }

    const evernote::edam::Notebook & enNotebook = notebook.en_notebook;

    QSqlQuery query(m_sqlDatabase);
    query.prepare("SELECT rowid FROM Notebooks WHERE guid=?");
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
    // TODO: need to support all other information available about objects of notebook class
    query.prepare("UPDATE Notebooks SET guid=?, updateSequenceNumber=?, name=?, "
                  "creationTimestamp=?, modificationTimestamp=?, isDirty=?, isLocal=?, "
                  "isDefault=?, isLastUsed=? WHERE rowid=?");
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

bool LocalStorageManager::FindNotebook(const Guid & notebookGuid, Notebook & notebook,
                                       QString & errorDescription)
{
    if (notebookGuid.empty()) {
        errorDescription = QObject::tr("Notebook guid is empty");
        return false;
    }

    QSqlQuery query(m_sqlDatabase);
    // TODO: need to support all other information available about objects of notebook class
    query.prepare("SELECT updateSequenceNumber, name, creationTimestamp, "
                  "modificationTimestamp, isDirty, isLocal, isDefault, isLastUsed "
                  "FROM Notebooks WHERE guid=?");
    query.addBindValue(QString::fromStdString(notebookGuid));
    bool res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("Can't find notebook in local storage database: ");

    QSqlRecord rec = query.record();

    if (rec.contains("isDirty")) {
        notebook.isDirty = (qvariant_cast<int>(rec.value("isDirty")) != 0);
    }
    else {
        errorDescription = QObject::tr("No \"isDirty\" field in the result of SQL query "
                                       "from local storage database");
        return false;
    }

    if (rec.contains("isLocal")) {
        notebook.isLocal = (qvariant_cast<int>(rec.value("isLocal")) != 0);
    }
    else {
        errorDescription = QObject::tr("No \"isLocal\" field in the result of SQL query "
                                       "from local storage database");
        return false;
    }

    if (rec.contains("isLasyUsed")) {
        notebook.isLastUsed = (qvariant_cast<int>(rec.value("isLastUsed")) != 0);
    }
    else {
        errorDescription = QObject::tr("No \"isLastUsed\" field in the result of SQL query "
                                       "from local storage database");
        return false;
    }

    evernote::edam::Notebook & enNotebook = notebook.en_notebook;

    enNotebook.guid = notebookGuid;
    enNotebook.__isset.guid = true;

    if (rec.contains("updateSequenceNumber")) {
        enNotebook.updateSequenceNum = qvariant_cast<uint32_t>(rec.value("updateSequenceNumber"));
        enNotebook.__isset.updateSequenceNum = true;
    }
    else {
        errorDescription = QObject::tr("No \"updateSequenceNumber\" field in the result of "
                                       "SQL query from local storage database");
        return false;
    }

    if (rec.contains("name")) {
        enNotebook.name = qvariant_cast<QString>(rec.value("name")).toStdString();
        enNotebook.__isset.name = true;
    }
    else {
        errorDescription = QObject::tr("No \"name\" field in the result of SQL query "
                                       "from local storage database");
        return false;
    }

    if (rec.contains("isDefault")) {
        enNotebook.defaultNotebook = (qvariant_cast<int>(rec.value("isDefault")) != 0);
        enNotebook.__isset.defaultNotebook = true;
    }
    else {
        errorDescription = QObject::tr("No \"isDefault\" field in the result of SQL query "
                                       "from local storage database");
        return false;
    }

    if (rec.contains("creationTimestamp")) {
        enNotebook.serviceCreated = qvariant_cast<Timestamp>(rec.value("creationTimestamp"));
        enNotebook.__isset.serviceCreated = true;
    }
    else {
        errorDescription = QObject::tr("No \"creationTimestamp\" field in the result of SQL "
                                       "query from local storage database");
        return false;
    }

    if (rec.contains("modificationTimestamp")) {
        enNotebook.serviceUpdated = qvariant_cast<Timestamp>(rec.value("modificationTimestamp"));
        enNotebook.__isset.serviceUpdated = true;
    }
    else {
        errorDescription = QObject::tr("No \"modificationTimestamp\" field in the result of "
                                       "SQL query from local storage database");
        return false;
    }

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

bool LocalStorageManager::ExpungeNotebook(const Notebook & notebook, QString & errorDescription)
{
    const evernote::edam::Notebook & enNotebook = notebook.en_notebook;
    CHECK_GUID(enNotebook, "Notebook");

    QSqlQuery query(m_sqlDatabase);
    query.prepare("SELECT rowid FROM Notebooks WHERE guid=?");
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
    query.prepare("DELETE FROM Notebooks WHERE rowid=?");
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
    res = UpdateNote(note, errorDescription);
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
    query.prepare("INSERT INTO Notes (guid, updateSequenceNumber, title, isDirty, "
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

    res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("Can't add note to Notes table in local storage database: ");

    bool tagGuidsSet = enNote.__isset.tagGuids;
    bool tagNamesSet = enNote.__isset.tagNames;

    if (tagGuidsSet && tagNamesSet) {
        errorDescription = QObject::tr("Both tag guids and names are specified for note");
        return false;
    }

    if (tagGuidsSet)
    {
        const std::vector<Guid> & tagGuids = enNote.tagGuids;
        size_t numTagGuids = tagGuids.size();

        for(size_t i = 0; i < numTagGuids; ++i) {
            // TODO: try to find this tag within local storage database;
            // in case of failure find this tag via NoreStoreClient and add to local storage
        }
    }

    if (tagNamesSet)
    {
        const std::vector<std::string> & tagNames = enNote.tagNames;
        size_t numTagNames = tagNames.size();

        for(size_t i = 0; i < numTagNames; ++i)
        {
            // NOTE: don't optimize this out, tag needs to be re-created on each iteration
            Tag tag;
            tag.en_tag.__isset.name = true;
            tag.en_tag.name = tagNames[i];

            // TODO: try to find tag with such name in *local* storage database before
            // attempting to create it

            m_pNoteStore->createTag(tag.en_tag, m_authenticationToken.toStdString(),
                                    tag.en_tag);
            if (!tag.en_tag.__isset.guid) {
                errorDescription = QObject::tr("Can't create tag from name");
                return false;
            }
            tag.isDirty = true;
            tag.isLocal = true;

            res = AddTag(tag, errorDescription);
            if (!res) {
                return false;
            }
        }
    }

    if (enNote.__isset.resources)
    {
        const std::vector<evernote::edam::Resource> & resources = enNote.resources;
        size_t numResources = resources.size();
        for(size_t i = 0; i < numResources; ++i)
        {
            // const evernote::edam::Resource & resource = resources[i];
            // TODO: check parameters of this resource

            // TODO: try to find this resource within local storage database,
            // in case of failure add this resource to local storage database
        }
    }

    if (!enNote.__isset.attributes) {
        return true;
    }

    return SetNoteAttributes(enNote, errorDescription);
}

bool LocalStorageManager::UpdateNote(const Note & note, QString & errorDescription)
{
    bool res = note.CheckParameters(errorDescription);
    if (!res) {
        return false;
    }

    const evernote::edam::Note & enNote = note.en_note;
    QString noteGuid = QString::fromStdString(enNote.guid);
    QString title = QString::fromStdString(enNote.title);
    QString content = QString::fromStdString(enNote.content);
    QString notebookGuid = QString::fromStdString(enNote.notebookGuid);

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
    query.addBindValue(title);
    query.addBindValue(content);
    query.addBindValue(notebookGuid);

    res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("Can't replace note in NotesText table in local "
                                 "storage database: ");

    query.clear();
    query.prepare("INSERT OR REPLACE INTO Notes (guid, updateSequenceNumber, title, "
                  "isDirty, isLocal, content, creationTimestamp, modificationTimestamp, "
                  "isDeleted, notebookGuid) VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
    query.addBindValue(noteGuid);
    query.addBindValue(enNote.updateSequenceNum);
    query.addBindValue(title);
    query.addBindValue((note.isDirty ? 1 : 0));
    query.addBindValue((note.isLocal ? 1 : 0));
    query.addBindValue(content);
    query.addBindValue(static_cast<quint64>(enNote.created));
    query.addBindValue(static_cast<quint64>(enNote.updated));
    query.addBindValue((note.isDeleted ? 1 : 0));
    query.addBindValue(notebookGuid);

    res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("Can't replace note in Notes table in local "
                                 "storage database: ");

    // TODO: replace tags and resources
    return SetNoteAttributes(enNote, errorDescription);
}

bool LocalStorageManager::FindNote(const Guid & noteGuid, Note & note, QString & errorDescription) const
{
    QSqlQuery query(m_sqlDatabase);

    query.prepare("SELECT updateSequenceNumber, title, isDirty, isLocal, content, "
                  "creationTimestamp, modificationTimestamp, isDeleted, deletionTimestap, "
                  "notebook FROM Notes WHERE guid=?");
    query.addBindValue(QString::fromStdString(noteGuid));

    bool res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("Can't select note from Notes table in local "
                                 "storage database: ");

    QSqlRecord rec = query.record();

    // TODO: check whether field exists in the record prior to attempt to use it

    if (rec.contains("isDirty")) {
        note.isDirty = (qvariant_cast<int>(rec.value("isDirty")) != 0);
    }
    else {
        errorDescription = QObject::tr("No \"isDirty\" field in the result of SQL "
                                       "query from local storage database");
        return false;
    }

    if (rec.contains("isLocal")) {
        note.isLocal = (qvariant_cast<int>(rec.value("isLocal")) != 0);
    }
    else {
        errorDescription = QObject::tr("No \"isLocal\" field in the result of SQL "
                                       "query from local storage database");
        return false;
    }

    if (rec.contains("isDeleted")) {
        note.isDeleted = (qvariant_cast<int>(rec.value("isDeleted")) != 0);
    }
    else {
        errorDescription = QObject::tr("No \"isDeleted\" field in the result of SQL "
                                       "query from local storage database");
        return false;
    }

    evernote::edam::Note & enNote = note.en_note;

    enNote.guid = noteGuid;
    enNote.__isset.guid = true;

    if (rec.contains("updateSequenceNumber")) {
        enNote.updateSequenceNum = qvariant_cast<uint32_t>(rec.value("updateSequenceNumber"));
        enNote.__isset.updateSequenceNum = true;
    }
    else {
        errorDescription = QObject::tr("No \"updateSequenceNumber\" field in the result of "
                                       "SQL query from local storage database");
        return false;
    }

    if (rec.contains("notebook")) {
        enNote.notebookGuid = qvariant_cast<QString>(rec.value("notebook")).toStdString();
        enNote.__isset.notebookGuid = true;
    }
    else {
        errorDescription = QObject::tr("No \"notebook\" field in the result of SQL "
                                       "query from local storage database");
        return false;
    }

    if (rec.contains("title")) {
        enNote.title = qvariant_cast<QString>(rec.value("title")).toStdString();
        enNote.__isset.title = true;
    }
    else {
        errorDescription = QObject::tr("No \"title\" field in the result of SQL "
                                       "query from local storage database");
        return false;
    }

    if (rec.contains("content")) {
        enNote.content = qvariant_cast<QString>(rec.value("content")).toStdString();
        enNote.__isset.content = true;
    }
    else {
        errorDescription = QObject::tr("No \"content\" field in the result of SQL "
                                       "query from local storage database");
        return false;
    }

#if QT_VERSION >= 0x050101
    QByteArray contentBinaryHash = QCryptographicHash::hash(QString::fromStdString(enNote.content).toUtf8(),
                                                            QCryptographicHash::Md5);
#else
    QByteArray contentBinaryHash = QCryptographicHash::hash(QString::fromStdString(enNote.content).toAscii(),
                                                            QCryptographicHash::Md5);
#endif
    QString contentHash(contentBinaryHash);
    enNote.contentHash = contentHash.toStdString();
    enNote.__isset.contentHash = true;

    enNote.contentLength = enNote.content.length();
    enNote.__isset.contentLength = true;

    if (rec.contains("creationTimestamp")) {
        enNote.created = qvariant_cast<Timestamp>(rec.value("creationTimestamp"));
        enNote.__isset.created = true;
    }
    else {
        errorDescription = QObject::tr("No \"creationTimestamp\" field in the result of SQL "
                                       "query from local storage database");
        return false;
    }

    if (rec.contains("modificationTimestamp")) {
        enNote.updated = qvariant_cast<Timestamp>(rec.value("modificationTimestamp"));
        enNote.__isset.updated = true;
    }
    else {
        errorDescription = QObject::tr("No \"modificationTimestamp\" field in the result of "
                                       "SQL query from local storage database");
        return false;
    }

    if (note.isDeleted)
    {
        if (rec.contains("deletionTimestamp")) {
            Timestamp deletionTimestamp = qvariant_cast<Timestamp>(rec.value("deletionTimestamp"));
            enNote.deleted = deletionTimestamp;
            enNote.__isset.deleted = true;
        }
        else {
            errorDescription = QObject::tr("No \"deletionTimestamp\" field in the result of "
                                           "SQL query from local storage database");
            return false;
        }
    }

    if (rec.contains("isActive")) {
        enNote.active = (qvariant_cast<int>(rec.value("isActive")) != 0);
        enNote.__isset.active = true;
    }
    else {
        errorDescription = QObject::tr("No \"isActive\" field in the result of SQL "
                                       "query from local storage database");
        return false;
    }

    // TODO: retrieve tag and resource guids from tags and resources tables

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
        errorDescription = QObject::tr("No \"hasAttributes\" field in the result of SQL "
                                       "query from local storage database");
        return false;
    }

    query.clear();
    query.prepare("SELECT subjectDate, isSetSubjectDate, latitude, isSetLatitude, "
                  "longitude, isSetLongitude, altitude, isSetAltitude, author, "
                  "isSetAuthor, source, isSetSource, sourceUrl, isSetSourceUrl, "
                  "sourceApplication, isSetSourceApplication, shareDate, isSetShareDate, "
                  "reminderOrder, isSetReminderOrder, reminderDoneTime, isSetReminderDoneTime, "
                  "reminderTime, isSetReminderTime, placeName, isSetPlaceName, "
                  "contentClass, isSetContentClass, lastEditedBy, isSetLastEditedBy, "
                  "creatorId, isSetCreatorId, lastEditorId, isSetLastEditorId "
                  "FROM NoteAttributes WHERE noteGuid=?");
    query.addBindValue(QString::fromStdString(noteGuid));
    res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("Can't select note attributes from NoteAttributes table: ");

    rec = query.record();

    if (rec.contains("isSetSubjectDate"))
    {
        int isSetSubjectDate = qvariant_cast<int>(rec.value("isSetSubjectDate"));
        if (isSetSubjectDate != 0) {
            Timestamp subjectDate = qvariant_cast<Timestamp>(rec.value("subjectDate"));
            enNote.attributes.subjectDate = subjectDate;
            enNote.attributes.__isset.subjectDate = true;
        }
        else {
            enNote.attributes.__isset.subjectDate = false;
        }
    }
    else {
        errorDescription = QObject::tr("No \"isSetSubjectDate\" field in the result of "
                                       "SQL query from local storage database");
        return false;
    }

    if (rec.contains("isSetLatitude"))
    {
        int isSetLatitude = qvariant_cast<int>(rec.value("isSetLatitude"));
        if (isSetLatitude != 0) {
            double latitude = qvariant_cast<double>(rec.value("latitude"));
            enNote.attributes.latitude = latitude;
            enNote.attributes.__isset.latitude = true;
        }
        else {
            enNote.attributes.__isset.latitude = false;
        }
    }
    else {
        errorDescription = QObject::tr("No \"isSetLatitude\" field in the result of "
                                       "SQL query from local storage database");
        return false;
    }

    if (rec.contains("isSetAltitude"))
    {
        int isSetAltitude = qvariant_cast<int>(rec.value("isSetAltitude"));
        if (isSetAltitude != 0) {
            double altitude = qvariant_cast<double>(rec.value("altitude"));
            enNote.attributes.altitude = altitude;
            enNote.attributes.__isset.altitude = true;
        }
        else {
            enNote.attributes.__isset.altitude = false;
        }
    }
    else {
        errorDescription = QObject::tr("No \"isSetAltitude\" field in the result of "
                                       "SQL query from local storage database");
        return false;
    }

    if (rec.contains("isSetLongitude"))
    {
        int isSetLongitude = qvariant_cast<int>(rec.value("isSetLongitude"));
        if (isSetLongitude != 0) {
            double longitude = qvariant_cast<double>(rec.value("longitude"));
            enNote.attributes.longitude = longitude;
            enNote.attributes.__isset.longitude = true;
        }
        else {
            enNote.attributes.__isset.longitude = false;
        }
    }
    else {
        errorDescription = QObject::tr("No \"isSetLongitude\" field in the result of "
                                       "SQL query from local storage database");
        return false;
    }

    if (rec.contains("isSetAuthor"))
    {
        int isSetAuthor = qvariant_cast<int>(rec.value("isSetAuthor"));
        if (isSetAuthor != 0) {
            QString author = qvariant_cast<QString>(rec.value("author"));
            enNote.attributes.author = author.toStdString();
            enNote.attributes.__isset.author = true;
        }
        else {
            enNote.attributes.__isset.author = false;
        }
    }
    else {
        errorDescription = QObject::tr("No \"isSetAuthor\" field in the result of "
                                       "SQL query from local storage database");
        return false;
    }

    if (rec.contains("isSetSource"))
    {
        int isSetSource = qvariant_cast<int>(rec.value("isSetSource"));
        if (isSetSource != 0) {
            QString source = qvariant_cast<QString>(rec.value("source"));
            enNote.attributes.source = source.toStdString();
            enNote.attributes.__isset.source = true;
        }
        else {
            enNote.attributes.__isset.source = false;
        }
    }
    else {
        errorDescription = QObject::tr("No \"isSetSource\" field in the result of "
                                       "SQL query from local storage database");
        return false;
    }

    if (rec.contains("isSetSourceUrl"))
    {
        int isSetSourceUrl = qvariant_cast<int>(rec.value("isSetSourceUrl"));
        if (isSetSourceUrl != 0) {
            QString sourceUrl = qvariant_cast<QString>(rec.value("sourceUrl"));
            enNote.attributes.sourceURL = sourceUrl.toStdString();
            enNote.attributes.__isset.sourceURL = true;
        }
        else {
            enNote.attributes.__isset.sourceURL = false;
        }
    }
    else {
        errorDescription = QObject::tr("No \"isSetSourceUrl\" field in the result of "
                                       "SQL query from local storage database");
        return false;
    }

    if (rec.contains("isSetSourceApplication"))
    {
        int isSetSourceApplication = qvariant_cast<int>(rec.value("isSetSourceApplication"));
        if (isSetSourceApplication != 0) {
            QString sourceApplication = qvariant_cast<QString>(rec.value("sourceApplication"));
            enNote.attributes.sourceApplication = sourceApplication.toStdString();
            enNote.attributes.__isset.sourceApplication = true;
        }
        else {
            enNote.attributes.__isset.sourceApplication = false;
        }
    }
    else {
        errorDescription = QObject::tr("No \"isSetSourceApplication\" in the result of "
                                       "SQL query from local storage database");
        return false;
    }

    if (rec.contains("isSetShareDate"))
    {
        int isSetShareDate = qvariant_cast<int>(rec.value("isSetShareDate"));
        if (isSetShareDate != 0) {
            Timestamp shareDate = qvariant_cast<Timestamp>(rec.value("shareDate"));
            enNote.attributes.shareDate = shareDate;
            enNote.attributes.__isset.shareDate = true;
        }
        else {
            enNote.attributes.__isset.shareDate = false;
        }
    }
    else {
        errorDescription = QObject::tr("No \"isSetShareDate\" field in the result of SQL "
                                       "query from local storage database");
        return false;
    }

    if (rec.contains("isSetReminderOrder"))
    {
        int isSetReminderOrder = qvariant_cast<int>(rec.value("isSetReminderOrder"));
        if (isSetReminderOrder != 0) {
            int64_t reminderOrder = qvariant_cast<int64_t>(rec.value("reminderOrder"));
            enNote.attributes.reminderOrder = reminderOrder;
            enNote.attributes.__isset.reminderOrder = true;
        }
        else {
            enNote.attributes.__isset.reminderOrder = false;
        }
    }
    else {
        errorDescription = QObject::tr("No \"isSetReminderOrder\" in field in the result of "
                                       "SQL query from local storage database");
        return false;
    }

    if (rec.contains("isSetReminderDoneTime"))
    {
        int isSetReminderDoneTime = qvariant_cast<int>(rec.value("isSetReminderDoneTime"));
        if (isSetReminderDoneTime != 0) {
            Timestamp reminderDoneTime = qvariant_cast<Timestamp>(rec.value("reminderDoneTime"));
            enNote.attributes.reminderDoneTime = reminderDoneTime;
            enNote.attributes.__isset.reminderDoneTime = true;
        }
        else {
            enNote.attributes.__isset.reminderDoneTime = false;
        }
    }
    else {
        errorDescription = QObject::tr("No \"isSetReminderDoneTime\" field in the result of "
                                       "SQL query from local storage database");
        return false;
    }

    if (rec.contains("isSetReminderTime"))
    {
        int isSetReminderTime = qvariant_cast<int>(rec.value("isSetReminderTime"));
        if (isSetReminderTime != 0) {
            Timestamp reminderTime = qvariant_cast<Timestamp>(rec.value("reminderTime"));
            enNote.attributes.reminderTime = reminderTime;
            enNote.attributes.__isset.reminderTime = true;
        }
        else {
            enNote.attributes.__isset.reminderTime = false;
        }
    }
    else {
        errorDescription = QObject::tr("No \"isSetReminderTime\" field in the result of "
                                       "SQL query from local storage database");
        return false;
    }

    if (rec.contains("isSetPlaceName"))
    {
        int isSetPlaceName = qvariant_cast<int>(rec.value("isSetPlaceName"));
        if (isSetPlaceName != 0) {
            QString placeName = qvariant_cast<QString>(rec.value("placeName"));
            enNote.attributes.placeName = placeName.toStdString();
            enNote.attributes.__isset.placeName = true;
        }
        else {
            enNote.attributes.__isset.placeName = false;
        }
    }
    else {
        errorDescription = QObject::tr("No \"isSetPlaceName\" in the result of "
                                       "SQL query from local storage database");
        return false;
    }

    if (rec.contains("isSetContentClass"))
    {
        int isSetContentClass = qvariant_cast<int>(rec.value("isSetContentClass"));
        if (isSetContentClass != 0) {
            QString contentClass = qvariant_cast<QString>(rec.value("contentClass"));
            enNote.attributes.contentClass = contentClass.toStdString();
            enNote.attributes.__isset.contentClass = true;
        }
        else {
            enNote.attributes.__isset.contentClass = false;
        }
    }
    else {
        errorDescription = QObject::tr("No \"isSetContentClass\" field in the result of "
                                       "SQL query from local storage database");
        return false;
    }

    if (rec.contains("isSetLastEditedBy"))
    {
        int isSetLastEditedBy = qvariant_cast<int>(rec.value("isSetLastEditedBy"));
        if (isSetLastEditedBy != 0) {
            QString lastEditedBy = qvariant_cast<QString>(rec.value("lastEditedBy"));
            enNote.attributes.lastEditedBy = lastEditedBy.toStdString();
            enNote.attributes.__isset.lastEditedBy = true;
        }
        else {
            enNote.attributes.__isset.lastEditedBy = false;
        }
    }
    else {
        errorDescription = QObject::tr("No \"isSetLastEditedBy\" field in the result of "
                                       "SQL query from local storage database");
        return false;
    }

    if (rec.contains("isSetCreatorId"))
    {
        int isSetCreatorId = qvariant_cast<int>(rec.value("isSetCreatorId"));
        if (isSetCreatorId != 0) {
            UserID creatorId = qvariant_cast<UserID>(rec.value("creatorId"));
            enNote.attributes.creatorId = creatorId;
            enNote.attributes.__isset.creatorId = true;
        }
        else {
            enNote.attributes.__isset.creatorId = false;
        }
    }
    else {
        errorDescription = QObject::tr("No \"isSetCreatorId\" field in the result of "
                                       "SQL query from local storage database");
        return false;
    }

    if (rec.contains("isSetLastEditorId"))
    {
        int isSetLastEditorId = qvariant_cast<int>(rec.value("isSetLastEditorId"));
        if (isSetLastEditorId != 0) {
            UserID lastEditorId = qvariant_cast<UserID>(rec.value("lastEditorId"));
            enNote.attributes.lastEditorId = lastEditorId;
            enNote.attributes.__isset.lastEditorId = true;
        }
        else {
            enNote.attributes.__isset.lastEditorId = false;
        }
    }
    else {
        errorDescription = QObject::tr("No \"isSetLastEditorId\" field in the result of "
                                       "SQL query from local storage database");
        return false;
    }

    query.clear();
    query.prepare("SELECT applicationDataKey, isSetApplicationDataValue, applicationDataValue "
                  "FROM NoteAttributesApplicationData WHERE noteGuid=?");
    query.addBindValue(QString::fromStdString(noteGuid));

    res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("Can't select application data from "
                                 "NoteAttributesApplicationData table: ");

    evernote::edam::LazyMap & applicationData = enNote.attributes.applicationData;
    applicationData.keysOnly.clear();
    applicationData.fullMap.clear();

    bool foundOne = false;
    QString applicationDataKey;
    int isSetApplicationDataValue = 0;
    bool conversionResult = false;
    while(query.next())
    {
        QString applicationDataValue;   // NOTE: don't move it outside of loop

        foundOne = true;
        applicationDataKey = query.value(0).toString();
        applicationData.keysOnly.insert(applicationDataKey.toStdString());
        applicationData.__isset.keysOnly = true;

        isSetApplicationDataValue = query.value(1).toInt(&conversionResult);
        if (!conversionResult) {
            errorDescription = QObject::tr("Can't convert isSetApplicationDataValue "
                                           "from query result to int");
            return false;
        }

        if (isSetApplicationDataValue != 0) {
            applicationDataValue = query.value(2).toString();
            applicationData.fullMap[applicationDataKey.toStdString()] = applicationDataValue.toStdString();
            applicationData.__isset.fullMap = true;
        }
    }
    enNote.attributes.__isset.applicationData = foundOne;

    query.clear();
    query.prepare("SELECT classificationKey, classificationValue FROM "
                  "NoteAttributesClassifications WHERE noteGuid=?");
    query.addBindValue(QString::fromStdString(noteGuid));

    res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("Can't select classifications from NoteAttributesClassifications table :");

    std::map<std::string, std::string> & classifications = enNote.attributes.classifications;
    classifications.clear();

    foundOne = false;
    QString classificationKey, classificationValue;
    while(query.next())
    {
        foundOne = true;
        classificationKey = query.value(0).toString();
        classificationValue = query.value(1).toString();

        classifications[classificationKey.toStdString()] = classificationValue.toStdString();
    }
    enNote.attributes.__isset.classifications = foundOne;

    return note.CheckParameters(errorDescription);
}

bool LocalStorageManager::DeleteNote(const Note & note, QString & errorDescription)
{
    const evernote::edam::Note & enNote = note.en_note;
    CHECK_GUID(enNote, "Note");

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
    query.prepare("UPDATE Notes SET isDeleted=1, isDirty=1 WHERE guid=?");
    query.addBindValue(QString::fromStdString(enNote.guid));
    bool res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("Can't delete note in local storage database: ");

    return true;
}

bool LocalStorageManager::ExpungeNote(const Note & note, QString & errorDescription)
{
    const evernote::edam::Note & enNote = note.en_note;
    CHECK_GUID(enNote, "Note");

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
    query.prepare("SELECT rowid FROM Notes WHERE guid=?");
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
    query.prepare("DELETE FROM Notes WHERE rowid=?");
    query.addBindValue(QVariant(rowId));

    res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("Can't expunge note from local storage database: ");

    return true;
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

*/

bool LocalStorageManager::CreateTables(QString & errorDescription)
{
    QSqlQuery query(m_sqlDatabase);
    bool res;

    // TODO: create table "Users" (see http://dev.evernote.com/doc/reference/Types.html#Struct_User)

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
                     "  userId REFERENCES Users(id) ON DELETE CASCADE ON UPDATE CASCADE"
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

    res = query.exec("CREATE TABLE IF NOT EXISTS NoteAttributesApplicationData "
                     "  noteGuid REFERENCES Notes(guid) ON DELETE CASCADE ON UPDATE CASCADE, "
                     "  applicationDataKey          TEXT             NOT NULL, "
                     "  applicationDataValue        TEXT             DEFAULT NULL, "
                     "  isSetApplicationDataValue   INTEGER          NOT NULL, "
                     "  UNIQUE(noteGuid, applicationDataKey) ON CONFLICT REPLACE");
    DATABASE_CHECK_AND_SET_ERROR("Can't create NoteAttributesApplicationData table: ");

    res = query.exec("CREATE TABLE IF NOT EXISTS NoteAttributesClassifications "
                     "  noteGuid REFERENCES Notes(guid) ON DELETE CASCADE ON UPDATE CASCADE, "
                     "  classificationKey       TEXT                 NOT NULL, "
                     "  classificationValue     TEXT                 DEFUALT NULL, "
                     "  UNIQUE(noteGuid, classificationKey) ON CONFLICT REPLACE");
    DATABASE_CHECK_AND_SET_ERROR("Can't create NoteAttributesClassifications table: ");

    res = query.exec("CREATE INDEX NotesNotebooks ON Notes(notebook)");
    DATABASE_CHECK_AND_SET_ERROR("Can't create index NotesNotebooks: ");

    res = query.exec("CREATE TABLE IF NOT EXISTS Resources("
                     "  guid              TEXT PRIMARY KEY     NOT NULL UNIQUE, "
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
                     "  guid                  TEXT PRIMARY KEY     NOT NULL UNIQUE, "
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

bool LocalStorageManager::SetNoteAttributes(const evernote::edam::Note & note,
                                            QString & errorDescription)
{
    CHECK_GUID(note, "Note");

    if (!note.__isset.attributes) {
        return true;
    }

    const evernote::edam::NoteAttributes & attributes = note.attributes;
    QString noteGuid = QString::fromStdString(note.guid);

    QSqlQuery query(m_sqlDatabase);
    bool isSetSubjectDate = attributes.__isset.subjectDate;
    query.prepare("INSERT OR REPLACE INTO NoteAttributes(noteGuid, isSetSubjectDate, "
                  "subjectDate) VALUES(?, ?, ?)");
    query.addBindValue(noteGuid);
    query.addBindValue((isSetSubjectDate ? 1 : 0));
    query.addBindValue((isSetSubjectDate ? static_cast<quint64>(attributes.subjectDate) : 0));

    bool res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("Can't insert or replace subject date into NoteAttributes table: ");

    query.clear();
    bool isSetLatitude = attributes.__isset.latitude;
    query.prepare("INSERT OR REPLACE INTO NoteAttributes(noteGuid, isSetLatitude, latitude) "
                  "VALUES(?, ?, ?)");
    query.addBindValue(noteGuid);
    query.addBindValue((isSetLatitude ? 1 : 0));
    query.addBindValue((isSetLatitude ? attributes.latitude : 0.0));

    res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("Can't insert or replace latitude into NoteAttributes table: ");

    query.clear();
    bool isSetLongitude = attributes.__isset.longitude;
    query.prepare("INSERT OR REPLACE INTO NoteAttributes(noteGuid, isSetLongitude, longitude) "
                  "VALUES(?, ?, ?)");
    query.addBindValue(noteGuid);
    query.addBindValue((isSetLongitude ? 1 : 0));
    query.addBindValue((isSetLongitude ? attributes.longitude : 0.0));

    res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("Can't insert or replace longitude into NoteAttributes table: ");

    query.clear();
    bool isSetAltitude = attributes.__isset.altitude;
    query.prepare("INSERT OR REPLACE INTO NoteAttributes(noteGuid, isSetAltitude, altitude) "
                  "VALUES(?, ?, ?)");
    query.addBindValue(noteGuid);
    query.addBindValue((isSetAltitude ? 1 : 0));
    query.addBindValue((isSetAltitude ? attributes.altitude : 0.0));

    res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("Can't insert or replace altitude into NoteAttributes table: ");

    query.clear();
    bool isSetAuthor = attributes.__isset.author;
    query.prepare("INSERT OR REPLACE INTO NoteAttributes(noteGuid, isSetAuthor, author) "
                  "VALUES(?, ?, ?)");
    query.addBindValue(noteGuid);
    query.addBindValue((isSetAuthor ? 1 : 0));
    query.addBindValue((isSetAuthor ? QString::fromStdString(attributes.author) : ""));

    res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("Can't insert or replace author into NoteAttributes table: ");

    query.clear();
    bool isSetSource = attributes.__isset.source;
    query.prepare("INSERT OR REPLACE INTO NoteAttributes(noteGuid, isSetSource, source) "
                  "VALUES(?, ?, ?)");
    query.addBindValue(noteGuid);
    query.addBindValue((isSetSource ? 1 : 0));
    query.addBindValue((isSetSource ? QString::fromStdString(attributes.source) : ""));

    res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("Can't insert or replace source into NoteAttributes table: ");

    query.clear();
    bool isSetSourceUrl = attributes.__isset.sourceURL;
    query.prepare("INSERT OR REPLACE INTO NoteAttributes(noteGuid, isSetSourceUrl, sourceUrl) "
                  "VALUES(?, ?, ?)");
    query.addBindValue(noteGuid);
    query.addBindValue((isSetSourceUrl ? 1 : 0));
    query.addBindValue((isSetSourceUrl ? QString::fromStdString(attributes.sourceURL) : ""));

    res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("Can't insert or replace sourceUrl into NoteAttributes table: ");

    query.clear();
    bool isSetSourceApplication = attributes.__isset.sourceApplication;
    query.prepare("INSERT OR REPLACE INTO NoteAttributes(noteGuid, isSetSourceApplication, "
                  "sourceApplication) VALUES(?, ?, ?)");
    query.addBindValue(noteGuid);
    query.addBindValue((isSetSourceApplication ? 1 : 0));
    query.addBindValue((isSetSourceApplication ? QString::fromStdString(attributes.sourceApplication) : ""));

    res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("Can't insert or replace sourceApplication into NoteAttributes table: ");

    query.clear();
    bool isSetShareDate = attributes.__isset.shareDate;
    query.prepare("INSERT OR REPLACE INTO NoteAttributes(noteGuid, isSetShareDate, shareDate) "
                  "VALUES(?, ?, ?)");
    query.addBindValue(noteGuid);
    query.addBindValue((isSetShareDate ? 1 : 0));
    query.addBindValue((isSetShareDate ? static_cast<quint64>(attributes.shareDate) : 0));

    res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("Can't insert or replace shareDate into NoteAttributes table: ");

    query.clear();
    bool isSetReminderOrder = attributes.__isset.reminderOrder;
    query.prepare("INSERT OR REPLACE INTO NoteAttributes(noteGuid, isSetReminderOrder, "
                  "reminderOrder) VALUES(?, ?, ?)");
    query.addBindValue(noteGuid);
    query.addBindValue((isSetReminderOrder ? 1 : 0));
    query.addBindValue((isSetReminderOrder ? static_cast<quint64>(attributes.reminderOrder) : 0));

    res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("Can't insert or replace reminderOrder into NoteAttributes table: ");

    query.clear();
    bool isSetReminderDoneTime = attributes.__isset.reminderDoneTime;
    query.prepare("INSERT OR REPLACE INTO NoteAttributes(noteGuid, isSetReminderDoneTime, "
                  "reminderDoneTime) VALUES(?, ?, ?)");
    query.addBindValue(noteGuid);
    query.addBindValue((isSetReminderDoneTime ? 1 : 0));
    query.addBindValue((isSetReminderDoneTime ? static_cast<quint64>(attributes.reminderDoneTime) : 0));

    res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("Can't insert or replace reminderDoneTime into NoteAttributes table: ");

    query.clear();
    bool isSetReminderTime = attributes.__isset.reminderOrder;
    query.prepare("INSERT OR REPLACE INTO NoteAttributes(noteGuid, isSetReminderTime, "
                  "reminderTime) VALUES(?, ?, ?)");
    query.addBindValue(noteGuid);
    query.addBindValue((isSetReminderTime ? 1 : 0));
    query.addBindValue((isSetReminderTime ? static_cast<quint64>(attributes.reminderTime) : 0));

    res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("Can't insert or replace reminderTime into NoteAttributes table: ");

    query.clear();
    bool isSetPlaceName = attributes.__isset.placeName;
    query.prepare("INSERT OR REPLACE INTO NoteAttributes(noteGuid, isSetPlaceName, "
                  "placeName) VALUES(?, ?, ?)");
    query.addBindValue(noteGuid);
    query.addBindValue((isSetPlaceName ? 1 : 0));
    query.addBindValue((isSetPlaceName ? QString::fromStdString(attributes.placeName) : ""));

    res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("Can't insert or replace placeName into NoteAttributes table: ");

    query.clear();
    bool isSetContentClass = attributes.__isset.contentClass;
    query.prepare("INSERT OR REPLACE INTO NoteAttributes(noteGuid, isSetContentClass, "
                  "contentClass) VALUES(?, ?, ?)");
    query.addBindValue(noteGuid);
    query.addBindValue((isSetContentClass ? 1 : 0));
    query.addBindValue((isSetContentClass ? QString::fromStdString(attributes.contentClass) : ""));

    res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("Can't insert or replace contentClass into NoteAttributes table: ");

    query.clear();
    bool isSetLastEditedBy = attributes.__isset.lastEditedBy;
    query.prepare("INSERT OR REPLACE INTO NoteAttributes(noteGuid, isSetLastEditedBy, "
                  "lastEditedBy) VALUES(?, ?, ?)");
    query.addBindValue(noteGuid);
    query.addBindValue((isSetLastEditedBy ? 1 : 0));
    query.addBindValue((isSetLastEditedBy ? QString::fromStdString(attributes.lastEditedBy) : 0));

    res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("Can't insert or replace lastEditedBy into NoteAttributes table: ");

    query.clear();
    bool isSetCreatorId = attributes.__isset.creatorId;
    query.prepare("INSERT OR REPLACE INTO NoteAttributes(noteGuid, isSetCreatorId, "
                  "creatorId) VALUES(?, ?, ?)");
    query.addBindValue(noteGuid);
    query.addBindValue((isSetCreatorId ? 1 : 0));
    query.addBindValue((isSetCreatorId ? attributes.creatorId : 0));

    res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("Can't insert or replace creatorId into NoteAttributes table: ");

    query.clear();
    bool isSetLastEditorId = attributes.__isset.lastEditorId;
    query.prepare("INSERT OR REPLACE INTO NoteAttributes(noteGuid, isSetLastEditorId, "
                  "lastEditorId) VALUES(?, ?, ?)");
    query.addBindValue(noteGuid);
    query.addBindValue((isSetLastEditorId ? 1 : 0));
    query.addBindValue((isSetLastEditorId ? attributes.lastEditorId : 0));

    res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("Can't insert or replace lastEditorId into NoteAttributes table: ");

    if (attributes.__isset.applicationData)
    {
        const auto & applicationData = attributes.applicationData;
        bool isSetKeysOnly = applicationData.__isset.keysOnly;
        bool isSetFullMap = applicationData.__isset.fullMap;

        if (isSetKeysOnly && isSetFullMap) {
            errorDescription = QObject::tr("Both keys only and full map fields are set for "
                                           "applicationData field of Note while only one should be set");
            return false;
        }

        if (isSetKeysOnly)
        {
            for(const auto & key : applicationData.keysOnly)
            {
                query.clear();
                query.prepare("INSERT OR REPLACE INTO NoteAttributesApplicationData(noteGuid, "
                              "applicationDataKey, isSetApplicationDataValue) VALUES(?, ?, ?)");
                query.addBindValue(noteGuid);
                query.addBindValue(QString::fromStdString(key));
                query.addBindValue(0);

                res = query.exec();
                DATABASE_CHECK_AND_SET_ERROR("Can't insert or replace applicationDataKey into "
                                             "NoteAttributesApplicationData table: ");
            }
        }
        else if (isSetFullMap)
        {
            for(const auto & pair : applicationData.fullMap)
            {
                query.clear();
                query.prepare("INSERT OR REPLACE INTO NoteAttributesApplicationData(noteGuid, "
                              "applicationDataKey, applicationDataValue, isSetApplicationDataValue) "
                              "VALUES(?, ?, ?, ?)");
                query.addBindValue(noteGuid);
                query.addBindValue(QString::fromStdString(pair.first));
                query.addBindValue(QString::fromStdString(pair.second));
                query.addBindValue(1);

                res = query.exec();
                DATABASE_CHECK_AND_SET_ERROR("Can't insert or replace applicationData into "
                                             "NoteAttributesApplicationData table: ");
            }
        }
    }

    if (attributes.__isset.classifications)
    {
        for(const auto & pair : attributes.classifications)
        {
            query.clear();
            query.prepare("INSERT OR REPLACE INTO NoteAttributesClassifications(noteGuid, "
                          "classificationKey, classificationValue) VALUES(?, ?, ?)");
            query.addBindValue(noteGuid);
            query.addBindValue(QString::fromStdString(pair.first));
            query.addBindValue(QString::fromStdString(pair.second));

            res = query.exec();
            DATABASE_CHECK_AND_SET_ERROR("Can't insert or replace classifications into "
                                         "NoteAttributesClassifications table: ");
        }
    }

    return true;
}

#undef CHECK_GUID
#undef DATABASE_CHECK_AND_SET_ERROR

}
