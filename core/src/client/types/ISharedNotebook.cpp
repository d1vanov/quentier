#include "ISharedNotebook.h"
#include <QDateTime>

namespace qute_note {

ISharedNotebook::ISharedNotebook()
{}

ISharedNotebook::~ISharedNotebook()
{}

bool ISharedNotebook::hasId() const
{
    const auto & enSharedNotebook = GetEnSharedNotebook();
    return enSharedNotebook.__isset.id;
}

qint64 ISharedNotebook::id() const
{
    const auto & enSharedNotebook = GetEnSharedNotebook();
    return enSharedNotebook.id;
}

void ISharedNotebook::setId(const qint64 id)
{
    auto & enSharedNotebook = GetEnSharedNotebook();
    enSharedNotebook.id = id;
    enSharedNotebook.__isset.id = true;
}

bool ISharedNotebook::hasUsedId() const
{
    const auto & enSharedNotebook = GetEnSharedNotebook();
    return enSharedNotebook.__isset.userId;
}

qint32 ISharedNotebook::userId() const
{
    const auto & enSharedNotebook = GetEnSharedNotebook();
    return enSharedNotebook.userId;
}

void ISharedNotebook::setUserId(const qint32 userId)
{
    auto & enSharedNotebook = GetEnSharedNotebook();
    enSharedNotebook.userId = userId;
    enSharedNotebook.__isset.userId = true;
}

const bool ISharedNotebook::hasNotebookGuid() const
{
    const auto & enSharedNotebook = GetEnSharedNotebook();
    return enSharedNotebook.__isset.notebookGuid;
}

const QString ISharedNotebook::notebookGuid() const
{
    const auto & enSharedNotebook = GetEnSharedNotebook();
    return std::move(QString::fromStdString(enSharedNotebook.notebookGuid));
}

void ISharedNotebook::setNotebookGuid(const QString & notebookGuid)
{
    auto & enSharedNotebook = GetEnSharedNotebook();
    enSharedNotebook.notebookGuid = notebookGuid.toStdString();
    enSharedNotebook.__isset.notebookGuid = !notebookGuid.isEmpty();
}

const bool ISharedNotebook::hasEmail() const
{
    const auto & enSharedNotebook = GetEnSharedNotebook();
    return enSharedNotebook.__isset.email;
}

const QString ISharedNotebook::email() const
{
    const auto & enSharedNotebook = GetEnSharedNotebook();
    return std::move(QString::fromStdString(enSharedNotebook.email));
}

void ISharedNotebook::setEmail(const QString & email)
{
    auto & enSharedNotebook = GetEnSharedNotebook();
    enSharedNotebook.email = email.toStdString();
    enSharedNotebook.__isset.email = !email.isEmpty();
}

bool ISharedNotebook::hasCreationTimestamp() const
{
    const auto & enSharedNotebook = GetEnSharedNotebook();
    return enSharedNotebook.__isset.serviceCreated;
}

qint64 ISharedNotebook::creationTimestamp() const
{
    const auto & enSharedNotebook = GetEnSharedNotebook();
    return enSharedNotebook.serviceCreated;
}

void ISharedNotebook::setCreationTimestamp(const qint64 timestamp)
{
    auto & enSharedNotebook = GetEnSharedNotebook();
    enSharedNotebook.serviceCreated = timestamp;
    enSharedNotebook.__isset.serviceCreated = (timestamp > 0);
}

bool ISharedNotebook::hasModificationTimestamp() const
{
    const auto & enSharedNotebook = GetEnSharedNotebook();
    return enSharedNotebook.__isset.serviceUpdated;
}

qint64 ISharedNotebook::modificationTimestamp() const
{
    const auto & enSharedNotebook = GetEnSharedNotebook();
    return enSharedNotebook.serviceUpdated;
}

void ISharedNotebook::setModificationTimestamp(const qint64 timestamp)
{
    auto & enSharedNotebook = GetEnSharedNotebook();
    enSharedNotebook.serviceUpdated = timestamp;
    enSharedNotebook.__isset.serviceUpdated = (timestamp > 0);
}

bool ISharedNotebook::hasShareKey() const
{
    const auto & enSharedNotebook = GetEnSharedNotebook();
    return enSharedNotebook.__isset.shareKey;
}

const QString ISharedNotebook::shareKey() const
{
    const auto & enSharedNotebook = GetEnSharedNotebook();
    return std::move(QString::fromStdString(enSharedNotebook.shareKey));
}

void ISharedNotebook::setShareKey(const QString & shareKey)
{
    auto & enSharedNotebook = GetEnSharedNotebook();
    enSharedNotebook.shareKey = shareKey.toStdString();
    enSharedNotebook.__isset.shareKey = !shareKey.isEmpty();
}

bool ISharedNotebook::hasUsername() const
{
    const auto & enSharedNotebook = GetEnSharedNotebook();
    return enSharedNotebook.__isset.username;
}

const QString ISharedNotebook::username() const
{
    const auto & enSharedNotebook = GetEnSharedNotebook();
    return std::move(QString::fromStdString(enSharedNotebook.username));
}

void ISharedNotebook::setUsername(const QString & username)
{
    auto & enSharedNotebook = GetEnSharedNotebook();
    enSharedNotebook.username = username.toStdString();
    enSharedNotebook.__isset.username = !username.isEmpty();
}

bool ISharedNotebook::hasPrivilegeLevel() const
{
    const auto & enSharedNotebook = GetEnSharedNotebook();
    return enSharedNotebook.__isset.privilege;
}

qint8 ISharedNotebook::privilegeLevel() const
{
    const auto & enSharedNotebook = GetEnSharedNotebook();
    return static_cast<qint8>(enSharedNotebook.privilege);
}

void ISharedNotebook::setPrivilegeLevel(const qint8 privilegeLevel)
{
    auto & enSharedNotebook = GetEnSharedNotebook();

    if (privilegeLevel <= static_cast<qint8>(enSharedNotebook.privilege)) {
        enSharedNotebook.privilege = static_cast<evernote::edam::SharedNotebookPrivilegeLevel::type>(privilegeLevel);
        enSharedNotebook.__isset.privilege = true;
    }
    else {
        enSharedNotebook.__isset.privilege = false;
    }
}

bool ISharedNotebook::hasAllowPreview() const
{
    const auto & enSharedNotebook = GetEnSharedNotebook();
    return enSharedNotebook.__isset.allowPreview;
}

bool ISharedNotebook::allowPreview() const
{
    const auto & enSharedNotebook = GetEnSharedNotebook();
    return enSharedNotebook.allowPreview;
}

void ISharedNotebook::setAllowPreview(const bool allowPreview)
{
    auto & enSharedNotebook = GetEnSharedNotebook();
    enSharedNotebook.allowPreview = allowPreview;
    enSharedNotebook.__isset.allowPreview = true;
}

bool ISharedNotebook::hasReminderNotifyEmail() const
{
    const auto & enSharedNotebook = GetEnSharedNotebook();
    return enSharedNotebook.__isset.recipientSettings &&
            enSharedNotebook.recipientSettings.__isset.reminderNotifyEmail;
}

bool ISharedNotebook::reminderNotifyEmail() const
{
    const auto & enSharedNotebook = GetEnSharedNotebook();
    return enSharedNotebook.recipientSettings.reminderNotifyEmail;
}

void ISharedNotebook::setReminderNotifyEmail(const bool notifyEmail)
{
    auto & enSharedNotebook = GetEnSharedNotebook();
    enSharedNotebook.recipientSettings.reminderNotifyEmail = notifyEmail;
    enSharedNotebook.__isset.recipientSettings = true;
    enSharedNotebook.recipientSettings.__isset.reminderNotifyEmail = true;
}

bool ISharedNotebook::hasReminderNotifyApp() const
{
    const auto & enSharedNotebook = GetEnSharedNotebook();
    return enSharedNotebook.__isset.recipientSettings &&
            enSharedNotebook.recipientSettings.__isset.reminderNotifyInApp;
}

bool ISharedNotebook::reminderNotifyApp() const
{
    const auto & enSharedNotebook = GetEnSharedNotebook();
    return enSharedNotebook.recipientSettings.reminderNotifyInApp;
}

void ISharedNotebook::setReminderNotifyApp(const bool notifyApp)
{
    auto & enSharedNotebook = GetEnSharedNotebook();
    enSharedNotebook.recipientSettings.reminderNotifyInApp = notifyApp;
    enSharedNotebook.__isset.recipientSettings = true;
    enSharedNotebook.recipientSettings.__isset.reminderNotifyInApp = true;
}

ISharedNotebook::ISharedNotebook(const ISharedNotebook & other) :
    TypeWithError(other)
{}

ISharedNotebook & ISharedNotebook::operator=(const ISharedNotebook & other)
{
    if (this != &other) {
        TypeWithError::operator=(other);
    }

    return *this;
}

QTextStream & ISharedNotebook::Print(QTextStream & strm) const
{
    strm << "SharedNotebook { \n";

    const auto & enSharedNotebook = GetEnSharedNotebook();
    const auto & isSet = enSharedNotebook.__isset;

    if (isSet.id) {
        strm << "id: " << enSharedNotebook.id;
    }
    else {
        strm << "id is not set";
    }
    strm << "; \n";

    if (isSet.userId) {
        strm << "userId: " << enSharedNotebook.userId;
    }
    else {
        strm << "userId is not set";
    }
    strm << "; \n";

    if (isSet.notebookGuid) {
        strm << "notebookGuid: " << QString::fromStdString(enSharedNotebook.notebookGuid);
    }
    else {
        strm << "notebookGuid is not set";
    }
    strm << "; \n";

    if (isSet.email) {
        strm << "email: " << QString::fromStdString(enSharedNotebook.email);
    }
    else {
        strm << "email is not set";
    }
    strm << "; \n";

    if (isSet.serviceCreated) {
        QDateTime createdDatetime;
        createdDatetime.setTime_t(enSharedNotebook.serviceCreated);
        strm << "creationTimestamp: " << enSharedNotebook.serviceCreated
             << ", datetime: " << createdDatetime.toString(Qt::ISODate);
    }
    else {
        strm << "creationTimestamp is not set";
    }
    strm << "; \n";

    if (isSet.serviceUpdated) {
        QDateTime updatedDatetime;
        updatedDatetime.setTime_t(enSharedNotebook.serviceUpdated);
        strm << "modificationTimestamp: " << enSharedNotebook.serviceUpdated
             << ", datetime: " << updatedDatetime.toString(Qt::ISODate);
    }
    else {
        strm << "modificationTimestamp is not set";
    }
    strm << "; \n";

    if (isSet.shareKey) {
        strm << "shareKey: " << QString::fromStdString(enSharedNotebook.shareKey);
    }
    else {
        strm << "shareKey is not set";
    }
    strm << "; \n";

    if (isSet.username) {
        strm << "username: " << QString::fromStdString(enSharedNotebook.username);
    }
    else {
        strm << "username is not set";
    }
    strm << "; \n";

    if (isSet.privilege) {
        strm << "privilegeLevel: " << enSharedNotebook.privilege;
    }
    else {
        strm << "privilegeLevel is not set";
    }
    strm << "; \n";

    if (isSet.allowPreview) {
        strm << "allowPreview: " << (enSharedNotebook.allowPreview ? "true" : "false");
    }
    else {
        strm << "allowPreview is not set";
    }
    strm << "; \n";

    if (isSet.recipientSettings)
    {
        const auto & recipientSettings = enSharedNotebook.recipientSettings;
        const auto & isSetRecipientSettings = recipientSettings.__isset;

        if (isSetRecipientSettings.reminderNotifyEmail) {
            strm << "reminderNotifyEmail: " << (recipientSettings.reminderNotifyEmail ? "true" : "false");
        }
        else {
            strm << "reminderNotifyEmail is not set";
        }
        strm << "; \n";

        if (isSetRecipientSettings.reminderNotifyInApp) {
            strm << "reminderNotifyApp: " << (recipientSettings.reminderNotifyInApp ? "true" : "false");
        }
        else {
            strm << "reminderNotifyApp is not set";
        }
        strm << "; \n";
    }
    else {
        strm << "recipientSettings are not set; \n";
    }

    return strm;
}

} // namespace qute_note
