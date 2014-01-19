#include "LocalStorageManager.h"
#include "DatabaseOpeningException.h"
#include "DatabaseSqlErrorException.h"
#include "../EnWrappers.h"
#include "../../tools/QuteNoteNullPtrException.h"

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
    query.prepare("INSERT INTO Notebooks (guid, updateSequenceNumber, name, "
                  "creationTimestamp, modificationTimestamp, isDirty, isLocal, "
                  "isDefault, isLastUsed, isPublished, stack) VALUES(?, ?, ?, ?, "
                  "?, ?, ?, 0, 0, ?, ?");
    query.addBindValue(QString::fromStdString(enNotebook.guid));
    query.addBindValue(enNotebook.updateSequenceNum);
    query.addBindValue(QString::fromStdString(enNotebook.name));
    query.addBindValue(QVariant(static_cast<int>(enNotebook.serviceCreated)));
    query.addBindValue(QVariant(static_cast<int>(enNotebook.serviceUpdated)));
    query.addBindValue(notebook.isDirty);
    query.addBindValue(notebook.isLocal);
    query.addBindValue(QVariant(enNotebook.published ? 1 : 0));
    query.addBindValue(QString::fromStdString(enNotebook.stack));

    res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("Can't insert new notebook into local storage database: ");

    return SetNotebookAdditionalAttributes(enNotebook, errorDescription);
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

    return SetNotebookAdditionalAttributes(enNotebook, errorDescription);
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
                  "modificationTimestamp, isDirty, isLocal, isDefault, isLastUsed, "
                  "publishingUri, publishingNoteSortOrder, publishingAscendingOrder, "
                  "publicDescription, isPublished, stack, businessNotebookDescription, "
                  "businessNotebookPrivilegeLevel, businessNotebookIsRecommended, "
                  "contactId FROM Notebooks WHERE guid=?");
    query.addBindValue(QString::fromStdString(notebookGuid));
    bool res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("Can't find notebook in local storage database: ");

    QSqlRecord rec = query.record();

#define CHECK_AND_SET_NOTEBOOK_ATTRIBUTE(attribute) \
    if (rec.contains(#attribute)) { \
        notebook.attribute = (qvariant_cast<int>(rec.value(#attribute)) != 0); \
    } \
    else { \
        errorDescription = QObject::tr("No " #attribute " field in the result of SQL query " \
                                       "from local storage database"); \
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
        errorDescription = QObject::tr("No " #attributeLocalName " field in the result of " \
                                       "SQL query from local storage database"); \
        return false; \
    }

    bool isRequired = true;
    CHECK_AND_SET_EN_NOTEBOOK_ATTRIBUTE(enNotebook, updateSequenceNumber, updateSequenceNum,
                                        int, uint32_t, isRequired);
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
        errorDescription = QObject::tr("No contactId field in the result of "
                                       "SQL query from local storage database");
        return false;
    }

    if (enNotebook.contact.id == 0) {
        enNotebook.__isset.contact = false;
        enNotebook.contact.__isset.id = false;
    }

    // TODO: if enNotebook.contact is set, retrieve full User object from respective table

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
    DATABASE_CHECK_AND_SET_ERROR("Can't select notebook restrictions for given notebook guid "
                                 "from local storage database: ");

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
    DATABASE_CHECK_AND_SET_ERROR("Can't select shared notebooks for given notebook guid from "
                                 "local storage database: ");

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

#define CHECK_AND_SET_NOTE_PROPERTY(property) \
    if (rec.contains(#property)) { \
        note.property = (qvariant_cast<int>(rec.value(#property)) != 0); \
    } \
    else { \
        errorDescription = QObject::tr("No " #property " field in the result of SQL " \
                                       "query from local storage database"); \
        return false; \
    }

    CHECK_AND_SET_NOTE_PROPERTY(isDirty);
    CHECK_AND_SET_NOTE_PROPERTY(isLocal);
    CHECK_AND_SET_NOTE_PROPERTY(isDeleted);

#undef CHECK_AND_SET_NOTE_PROPERTY

    evernote::edam::Note & enNote = note.en_note;
    enNote.guid = noteGuid;
    enNote.__isset.guid = true;

#define CHECK_AND_SET_EN_NOTE_PROPERTY(propertyLocalName, propertyEnName, type, ...) \
    if (rec.contains(#propertyLocalName)) { \
        enNote.propertyEnName = (qvariant_cast<type>(rec.value(#propertyLocalName)))__VA_ARGS__; \
        enNote.__isset.propertyEnName = true; \
    } \
    else { \
        errorDescription = QObject::tr("No " #propertyLocalName " field in the result of " \
                                       "SQL query from local storage database"); \
        return false; \
    }

    CHECK_AND_SET_EN_NOTE_PROPERTY(updateSequenceNumber, updateSequenceNum, uint32_t);
    CHECK_AND_SET_EN_NOTE_PROPERTY(notebook, notebookGuid, QString, .toStdString());
    CHECK_AND_SET_EN_NOTE_PROPERTY(title, title, QString, .toStdString());
    CHECK_AND_SET_EN_NOTE_PROPERTY(content, content, QString, .toStdString());

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

    CHECK_AND_SET_EN_NOTE_PROPERTY(creationTimestamp, created, Timestamp);
    CHECK_AND_SET_EN_NOTE_PROPERTY(modificationTimestamp, updated, Timestamp);

    if (note.isDeleted) {
        CHECK_AND_SET_EN_NOTE_PROPERTY(deletionTimestamp, deleted, Timestamp);
    }

    CHECK_AND_SET_EN_NOTE_PROPERTY(isActive, active, int);

#undef CHECK_AND_SET_EN_NOTE_PROPERTY

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
    query.prepare("SELECT data FROM NoteAttributes WHERE noteGuid=?");
    res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("Can't select note attributes from NoteAttributes table: ");

    if (query.next())
    {
        QByteArray data = query.value(0).toByteArray();
        enNote.attributes = std::move(GetDeserializedNoteAttributes(data));
    }
    else {
        errorDescription = QObject::tr("Can't retrieve serialized note attributes from "
                                       "local storage database: query result is empty");
        return false;
    }

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
                  "sourceURL, timestamp, altitude, latitude, longitude, fileName, "
                  "isAttachment, noteGuid) VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, "
                  "?, ?, ?)");
    query.addBindValue(resourceGuid.ToQString());
    query.addBindValue(resource.dataHash());
    query.addBindValue(resourceBinaryData);
    query.addBindValue(resource.mimeType());
    query.addBindValue(QVariant(static_cast<int>(resource.width())));
    query.addBindValue(QVariant(static_cast<int>(resource.height())));
    query.addBindValue(resource.sourceURL());
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

    res = query.exec("CREATE TABLE IF NOT EXISTS Users("
                     "  id                      INTEGER PRIMARY KEY     NOT NULL UNIQUE, "
                     "  username                TEXT                    NOT NULL, "
                     "  email                   TEXT                    NOT NULL, "
                     "  name                    TEXT                    NOT NULL, "
                     "  timezone                TEXT                    DEFAULT NULL, "
                     "  privilege               INTEGER                 NOT NULL, "
                     "  creationTimestamp       INTEGER                 NOT NULL, "
                     "  modificationTimestamp   INTEGER                 NOT NULL, "
                     "  isDeleted               INTEGER                 DEFAULT 0, "
                     "  deletionTimestamp       INTEGER                 DEFAULT 0, "
                     "  isActive                INTEGER                 NOT NULL, "
                     ")");
    DATABASE_CHECK_AND_SET_ERROR("Can't create Users table: ");

    res = query.exec("CREATE TABLE IF NOT EXISTS UserAttributes("
                     "  id REFERENCES Users(id) ON DELETE CASCADE ON UPDATE CASCADE, "
                     "  data                    BLOB                    DEFAULT NULL)");
    DATABASE_CHECK_AND_SET_ERROR("Can't create UserAttributes table: ");

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
                     "  sourceURL         TEXT, "
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
    query.prepare("INSERT OR REPLACE INTO NoteAttributes(noteGuid, data) VALUES(?, ?)");
    query.addBindValue(noteGuid);
    QByteArray data = GetSerializedNoteAttributes(attributes);
    query.addBindValue(data);

    bool res = query.exec();
    DATABASE_CHECK_AND_SET_ERROR("Can't insert or replace lastEditorId into NoteAttributes table: ");

    return true;
}

bool LocalStorageManager::SetNotebookAdditionalAttributes(const evernote::edam::Notebook & notebook,
                                                          QString & errorDescription)
{
    QSqlQuery query(m_sqlDatabase);

    bool hasAdditionalAttributes = false;
    query.prepare("UPDATE Notebooks SET (:values) WHERE guid = :notebookGuid");
    query.bindValue("notebookGuid", QVariant(QString::fromStdString(notebook.guid)));

    QString values;

    if (notebook.__isset.published)
    {
        hasAdditionalAttributes = true;

        values.append("isPublished=");
        values.append(QString::number(notebook.published ? 1 : 0));
        values.append(", ");
    }

    if (notebook.published && notebook.__isset.published)
    {
        hasAdditionalAttributes = true;

        const Publishing & publishing = notebook.publishing;

        if (publishing.__isset.uri) {
            values.append("publishingUri=");
            values.append(QString::fromStdString(publishing.uri));
            values.append(", ");
        }

        if (publishing.__isset.order) {
            values.append("publishingNoteSortOrder=");
            values.append(QString::number(publishing.order));
            values.append(", ");
        }

        if (publishing.__isset.ascending) {
            values.append("publishingAscendingSort=");
            values.append(QString::number((publishing.ascending ? 1 : 0)));
            values.append(", ");
        }

        if (publishing.__isset.publicDescription) {
            values.append("publicDescription=");
            values.append(QString::fromStdString(publishing.publicDescription));
            values.append(", ");
        }
    }

    if (notebook.__isset.businessNotebook)
    {
        hasAdditionalAttributes = true;

        const BusinessNotebook & businessNotebook = notebook.businessNotebook;

        if (businessNotebook.__isset.notebookDescription) {
            values.append("businessNotebookDescription=");
            values.append(QString::fromStdString(businessNotebook.notebookDescription));
            values.append(", ");
        }

        if (businessNotebook.__isset.privilege) {
            values.append("businessNotebookPrivilegeLevel=");
            values.append(QString::number(businessNotebook.privilege));
            values.append(", ");
        }

        if (businessNotebook.__isset.recommended) {
            values.append("businessNotebookIsRecommended=");
            values.append(QString::number((businessNotebook.recommended ? 1 : 0)));
            values.append(", ");
        }
    }

    if (notebook.__isset.contact)
    {
        hasAdditionalAttributes = true;

        values.append("contactId=");
        values.append(QString::number(notebook.contact.id));

        // TODO: check whether such id is present in Users table, if not, add it
    }

    if (hasAdditionalAttributes) {
        query.bindValue("values", values);
        bool res = query.exec();
        DATABASE_CHECK_AND_SET_ERROR("Can't set additional notebook attributes to "
                                     "local storage database: ");
    }

    bool res = SetNotebookRestrictions(notebook, errorDescription);
    if (!res) {
        return res;
    }

    const auto & sharedNotebooks = notebook.sharedNotebooks;
    for(const auto & sharedNotebook: sharedNotebooks)
    {
        res = SetSharedNotebookAttributes(sharedNotebook, errorDescription);
        if (!res) {
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
        DATABASE_CHECK_AND_SET_ERROR("Can't set notebook restrictions to local storage database: ");
    }

    return true;
}

bool LocalStorageManager::SetSharedNotebookAttributes(const evernote::edam::SharedNotebook & sharedNotebook,
                                                      QString & errorDescription)
{
    if (!sharedNotebook.__isset.id) {
        errorDescription = QObject::tr("Shared notebook's share id is not set");
        return false;
    }

    if (!sharedNotebook.__isset.notebookGuid) {
        errorDescription = QObject::tr("Shared notebook's notebook guid is not set");
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
        DATABASE_CHECK_AND_SET_ERROR("Can't set shared notebook attributes to local storage database");
    }

    return true;
}

#undef CHECK_GUID
#undef DATABASE_CHECK_AND_SET_ERROR

}
