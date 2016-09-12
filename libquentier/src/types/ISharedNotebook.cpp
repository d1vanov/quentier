/*
 * Copyright 2016 Dmitry Ivanov
 *
 * This file is part of libquentier
 *
 * libquentier is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * libquentier is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with libquentier. If not, see <http://www.gnu.org/licenses/>.
 */

#include <quentier/types/ISharedNotebook.h>
#include <quentier/utility/Utility.h>

namespace quentier {

ISharedNotebook::ISharedNotebook() :
    m_indexInNotebook(-1)
{}

ISharedNotebook::~ISharedNotebook()
{}

bool ISharedNotebook::operator==(const ISharedNotebook & other) const
{
    const qevercloud::SharedNotebook & sharedNotebook = GetEnSharedNotebook();
    const qevercloud::SharedNotebook & otherSharedNotebook = other.GetEnSharedNotebook();

    return (sharedNotebook == otherSharedNotebook);

    // NOTE: m_indexInNotebook does not take any part in comparison
    // as it is by nature a helper parameter intended to preserve sorting of shared notebooks
    // in notebook between insertions and selections from the database
}

bool ISharedNotebook::operator!=(const ISharedNotebook & other) const
{
    return !(*this == other);
}

ISharedNotebook::operator const qevercloud::SharedNotebook &() const
{
    return GetEnSharedNotebook();
}

ISharedNotebook::operator qevercloud::SharedNotebook &()
{
    return GetEnSharedNotebook();
}

int ISharedNotebook::indexInNotebook() const
{
    return m_indexInNotebook;
}

void ISharedNotebook::setIndexInNotebook(const int index)
{
    m_indexInNotebook = index;
}

bool ISharedNotebook::hasId() const
{
    return GetEnSharedNotebook().id.isSet();
}

qint64 ISharedNotebook::id() const
{
    return GetEnSharedNotebook().id;
}

void ISharedNotebook::setId(const qint64 id)
{
    GetEnSharedNotebook().id = id;
}

bool ISharedNotebook::hasUserId() const
{
    return GetEnSharedNotebook().userId.isSet();
}

qint32 ISharedNotebook::userId() const
{
    return GetEnSharedNotebook().userId;
}

void ISharedNotebook::setUserId(const qint32 userId)
{
    GetEnSharedNotebook().userId = userId;
}

bool ISharedNotebook::hasNotebookGuid() const
{
    return GetEnSharedNotebook().notebookGuid.isSet();
}

const QString & ISharedNotebook::notebookGuid() const
{
    return GetEnSharedNotebook().notebookGuid;
}

void ISharedNotebook::setNotebookGuid(const QString & notebookGuid)
{
    if (!notebookGuid.isEmpty()) {
        GetEnSharedNotebook().notebookGuid = notebookGuid;
    }
    else {
        GetEnSharedNotebook().notebookGuid.clear();
    }
}

bool ISharedNotebook::hasEmail() const
{
    return GetEnSharedNotebook().email.isSet();
}

const QString & ISharedNotebook::email() const
{
    return GetEnSharedNotebook().email;
}

void ISharedNotebook::setEmail(const QString & email)
{
    if (!email.isEmpty()) {
        GetEnSharedNotebook().email = email;
    }
    else {
        GetEnSharedNotebook().email.clear();
    }
}

bool ISharedNotebook::hasCreationTimestamp() const
{
    return GetEnSharedNotebook().serviceCreated.isSet();
}

qint64 ISharedNotebook::creationTimestamp() const
{
    return GetEnSharedNotebook().serviceCreated;
}

void ISharedNotebook::setCreationTimestamp(const qint64 timestamp)
{
    // TODO: verify whether it really matters
    if (timestamp >= 0) {
        GetEnSharedNotebook().serviceCreated = timestamp;
    }
    else {
        GetEnSharedNotebook().serviceCreated.clear();
    }
}

bool ISharedNotebook::hasModificationTimestamp() const
{
    return GetEnSharedNotebook().serviceUpdated.isSet();
}

qint64 ISharedNotebook::modificationTimestamp() const
{
    return GetEnSharedNotebook().serviceUpdated;
}

void ISharedNotebook::setModificationTimestamp(const qint64 timestamp)
{
    // TODO: verify whether it really matters
    if (timestamp >= 0) {
        GetEnSharedNotebook().serviceUpdated = timestamp;
    }
    else {
        GetEnSharedNotebook().serviceUpdated.clear();
    }
}

bool ISharedNotebook::hasUsername() const
{
    return GetEnSharedNotebook().username.isSet();
}

const QString & ISharedNotebook::username() const
{
    return GetEnSharedNotebook().username;
}

void ISharedNotebook::setUsername(const QString & username)
{
    if (!username.isEmpty()) {
        GetEnSharedNotebook().username = username;
    }
    else {
        GetEnSharedNotebook().username.clear();
    }
}

bool ISharedNotebook::hasPrivilegeLevel() const
{
    return GetEnSharedNotebook().privilege.isSet();
}

ISharedNotebook::SharedNotebookPrivilegeLevel ISharedNotebook::privilegeLevel() const
{
    return GetEnSharedNotebook().privilege;
}

void ISharedNotebook::setPrivilegeLevel(const SharedNotebookPrivilegeLevel privilegeLevel)
{
    GetEnSharedNotebook().privilege = privilegeLevel;
}

void ISharedNotebook::setPrivilegeLevel(const qint8 privilegeLevel)
{
    if (privilegeLevel <= static_cast<qint8>(qevercloud::SharedNotebookPrivilegeLevel::BUSINESS_FULL_ACCESS)) {
        GetEnSharedNotebook().privilege = static_cast<qevercloud::SharedNotebookPrivilegeLevel::type>(privilegeLevel);
    }
    else {
        GetEnSharedNotebook().privilege.clear();
    }
}

bool ISharedNotebook::hasReminderNotifyEmail() const
{
    const qevercloud::SharedNotebook & sharedNotebook = GetEnSharedNotebook();
    return sharedNotebook.recipientSettings.isSet() && sharedNotebook.recipientSettings->reminderNotifyEmail.isSet();
}

bool ISharedNotebook::reminderNotifyEmail() const
{
    return GetEnSharedNotebook().recipientSettings->reminderNotifyEmail;
}

#define CHECK_AND_SET_RECIPIENT_SETTINGS \
    qevercloud::SharedNotebook & sharedNotebook = GetEnSharedNotebook(); \
    if (!sharedNotebook.recipientSettings.isSet()) { \
        sharedNotebook.recipientSettings = qevercloud::SharedNotebookRecipientSettings(); \
    }

void ISharedNotebook::setReminderNotifyEmail(const bool notifyEmail)
{
    CHECK_AND_SET_RECIPIENT_SETTINGS
    sharedNotebook.recipientSettings->reminderNotifyEmail = notifyEmail;
}

bool ISharedNotebook::hasReminderNotifyApp() const
{
    const qevercloud::SharedNotebook & sharedNotebook = GetEnSharedNotebook();
    return sharedNotebook.recipientSettings.isSet() && sharedNotebook.recipientSettings->reminderNotifyInApp.isSet();
}

bool ISharedNotebook::reminderNotifyApp() const
{
    return GetEnSharedNotebook().recipientSettings->reminderNotifyInApp;
}

void ISharedNotebook::setReminderNotifyApp(const bool notifyApp)
{
    CHECK_AND_SET_RECIPIENT_SETTINGS
    sharedNotebook.recipientSettings->reminderNotifyInApp = notifyApp;
}

#undef CHECK_AND_SET_RECIPIENT_SETTINGS

ISharedNotebook::ISharedNotebook(const ISharedNotebook & other) :
    Printable(),
    m_indexInNotebook(other.m_indexInNotebook)
{}

ISharedNotebook::ISharedNotebook(ISharedNotebook && other) :
    Printable(),
    m_indexInNotebook(std::move(other.m_indexInNotebook))
{}

ISharedNotebook & ISharedNotebook::operator=(const ISharedNotebook & other)
{
    if (this != &other) {
        m_indexInNotebook = other.m_indexInNotebook;
    }

    return *this;
}

ISharedNotebook & ISharedNotebook::operator=(ISharedNotebook && other)
{
    if (this != &other) {
        m_indexInNotebook = std::move(other.m_indexInNotebook);
    }

    return *this;
}

bool ISharedNotebook::hasRecipientUsername() const
{
    return GetEnSharedNotebook().recipientUsername.isSet();
}

const QString & ISharedNotebook::recipientUsername() const
{
    return GetEnSharedNotebook().recipientUsername.ref();
}

void ISharedNotebook::setRecipientUsername(const QString & recipientUsername)
{
    if (recipientUsername.isEmpty()) {
        GetEnSharedNotebook().recipientUsername.clear();
    }
    else {
        GetEnSharedNotebook().recipientUsername = recipientUsername;
    }
}

bool ISharedNotebook::hasRecipientUserId() const
{
    return GetEnSharedNotebook().recipientUserId.isSet();
}

qint32 ISharedNotebook::recipientUserId() const
{
    return GetEnSharedNotebook().recipientUserId.ref();
}

void ISharedNotebook::setRecipientUserId(const qint32 recipientUserId)
{
    GetEnSharedNotebook().recipientUserId = recipientUserId;
}

bool ISharedNotebook::hasRecipientIdentityId() const
{
    return GetEnSharedNotebook().recipientIdentityId.isSet();
}

qint64 ISharedNotebook::recipientIdentityId() const
{
    return GetEnSharedNotebook().recipientIdentityId.ref();
}

void ISharedNotebook::setRecipientIdentityId(const qint64 recipientIdentityId)
{
    GetEnSharedNotebook().recipientIdentityId = recipientIdentityId;
}

bool ISharedNotebook::hasGlobalId() const
{
    return GetEnSharedNotebook().globalId.isSet();
}

const QString & ISharedNotebook::globalId() const
{
    return GetEnSharedNotebook().globalId.ref();
}

void ISharedNotebook::setGlobalId(const QString & globalId)
{
    if (!globalId.isEmpty()) {
        GetEnSharedNotebook().globalId = globalId;
    }
    else {
        GetEnSharedNotebook().globalId.clear();
    }
}

bool ISharedNotebook::hasSharerUserId() const
{
    return GetEnSharedNotebook().sharerUserId.isSet();
}

qint32 ISharedNotebook::sharerUserId() const
{
    return GetEnSharedNotebook().sharerUserId.ref();
}

void ISharedNotebook::setSharerUserId(const qint32 sharerUserId)
{
    GetEnSharedNotebook().sharerUserId = sharerUserId;
}

bool ISharedNotebook::hasAssignmentTimestamp() const
{
    return GetEnSharedNotebook().serviceAssigned.isSet();
}

qint64 ISharedNotebook::assignmentTimestamp() const
{
    return GetEnSharedNotebook().serviceAssigned.ref();
}

void ISharedNotebook::setAssignmentTimestamp(const qint64 timestamp)
{
    if (timestamp >= 0) {
        GetEnSharedNotebook().serviceAssigned = timestamp;
    }
    else {
        GetEnSharedNotebook().serviceAssigned.clear();
    }
}

QTextStream & ISharedNotebook::print(QTextStream & strm) const
{
    strm << QStringLiteral("SharedNotebook {\n");
    strm << QStringLiteral("  index in notebook: ") << QString::number(m_indexInNotebook)
         << QStringLiteral(";\n");

    const qevercloud::SharedNotebook & sharedNotebook = GetEnSharedNotebook();
    strm << sharedNotebook;
    strm << QStringLiteral("};\n");
    return strm;
}

} // namespace quentier
