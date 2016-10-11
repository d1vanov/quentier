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

#include "data/SharedNotebookData.h"
#include <quentier/types/SharedNotebook.h>
#include <quentier/utility/Utility.h>

namespace quentier {

SharedNotebook::SharedNotebook() :
    d(new SharedNotebookData)
{}

SharedNotebook::SharedNotebook(const SharedNotebook & other) :
    Printable(),
    d(other.d)
{}

SharedNotebook::SharedNotebook(SharedNotebook && other) :
    Printable(),
    d(std::move(other.d))
{}

SharedNotebook::SharedNotebook(const qevercloud::SharedNotebook & qecSharedNotebook) :
    Printable(),
    d(new SharedNotebookData(qecSharedNotebook))
{}

SharedNotebook & SharedNotebook::operator=(const SharedNotebook & other)
{
    if (this != &other) {
        d = other.d;
    }

    return *this;
}

SharedNotebook & SharedNotebook::operator=(SharedNotebook && other)
{
    if (this != &other) {
        d = std::move(other.d);
    }

    return *this;
}

SharedNotebook::~SharedNotebook()
{}

bool SharedNotebook::operator==(const SharedNotebook & other) const
{
    const qevercloud::SharedNotebook & sharedNotebook = d->m_qecSharedNotebook;
    const qevercloud::SharedNotebook & otherSharedNotebook = other.d->m_qecSharedNotebook;

    return (sharedNotebook == otherSharedNotebook);

    // NOTE: d->m_indexInNotebook does not take any part in comparison
    // as it is by nature a helper parameter intended to preserve sorting of shared notebooks
    // in notebook between insertions and selections from the database
}

bool SharedNotebook::operator!=(const SharedNotebook & other) const
{
    return !(*this == other);
}

SharedNotebook::operator const qevercloud::SharedNotebook &() const
{
    return d->m_qecSharedNotebook;
}

SharedNotebook::operator qevercloud::SharedNotebook &()
{
    return d->m_qecSharedNotebook;
}

int SharedNotebook::indexInNotebook() const
{
    return d->m_indexInNotebook;
}

void SharedNotebook::setIndexInNotebook(const int index)
{
    d->m_indexInNotebook = index;
}

bool SharedNotebook::hasId() const
{
    return d->m_qecSharedNotebook.id.isSet();
}

qint64 SharedNotebook::id() const
{
    return d->m_qecSharedNotebook.id.ref();
}

void SharedNotebook::setId(const qint64 id)
{
    d->m_qecSharedNotebook.id = id;
}

bool SharedNotebook::hasUserId() const
{
    return d->m_qecSharedNotebook.userId.isSet();
}

qint32 SharedNotebook::userId() const
{
    return d->m_qecSharedNotebook.userId;
}

void SharedNotebook::setUserId(const qint32 userId)
{
    d->m_qecSharedNotebook.userId = userId;
}

bool SharedNotebook::hasNotebookGuid() const
{
    return d->m_qecSharedNotebook.notebookGuid.isSet();
}

const QString & SharedNotebook::notebookGuid() const
{
    return d->m_qecSharedNotebook.notebookGuid;
}

void SharedNotebook::setNotebookGuid(const QString & notebookGuid)
{
    if (!notebookGuid.isEmpty()) {
        d->m_qecSharedNotebook.notebookGuid = notebookGuid;
    }
    else {
        d->m_qecSharedNotebook.notebookGuid.clear();
    }
}

bool SharedNotebook::hasEmail() const
{
    return d->m_qecSharedNotebook.email.isSet();
}

const QString & SharedNotebook::email() const
{
    return d->m_qecSharedNotebook.email;
}

void SharedNotebook::setEmail(const QString & email)
{
    if (!email.isEmpty()) {
        d->m_qecSharedNotebook.email = email;
    }
    else {
        d->m_qecSharedNotebook.email.clear();
    }
}

bool SharedNotebook::hasCreationTimestamp() const
{
    return d->m_qecSharedNotebook.serviceCreated.isSet();
}

qint64 SharedNotebook::creationTimestamp() const
{
    return d->m_qecSharedNotebook.serviceCreated;
}

void SharedNotebook::setCreationTimestamp(const qint64 timestamp)
{
    if (timestamp >= 0) {
        d->m_qecSharedNotebook.serviceCreated = timestamp;
    }
    else {
        d->m_qecSharedNotebook.serviceCreated.clear();
    }
}

bool SharedNotebook::hasModificationTimestamp() const
{
    return d->m_qecSharedNotebook.serviceUpdated.isSet();
}

qint64 SharedNotebook::modificationTimestamp() const
{
    return d->m_qecSharedNotebook.serviceUpdated;
}

void SharedNotebook::setModificationTimestamp(const qint64 timestamp)
{
    if (timestamp >= 0) {
        d->m_qecSharedNotebook.serviceUpdated = timestamp;
    }
    else {
        d->m_qecSharedNotebook.serviceUpdated.clear();
    }
}

bool SharedNotebook::hasUsername() const
{
    return d->m_qecSharedNotebook.username.isSet();
}

const QString & SharedNotebook::username() const
{
    return d->m_qecSharedNotebook.username;
}

void SharedNotebook::setUsername(const QString & username)
{
    if (!username.isEmpty()) {
        d->m_qecSharedNotebook.username = username;
    }
    else {
        d->m_qecSharedNotebook.username.clear();
    }
}

bool SharedNotebook::hasPrivilegeLevel() const
{
    return d->m_qecSharedNotebook.privilege.isSet();
}

SharedNotebook::SharedNotebookPrivilegeLevel SharedNotebook::privilegeLevel() const
{
    return d->m_qecSharedNotebook.privilege;
}

void SharedNotebook::setPrivilegeLevel(const SharedNotebookPrivilegeLevel privilegeLevel)
{
    d->m_qecSharedNotebook.privilege = privilegeLevel;
}

void SharedNotebook::setPrivilegeLevel(const qint8 privilegeLevel)
{
    if (privilegeLevel <= static_cast<qint8>(qevercloud::SharedNotebookPrivilegeLevel::BUSINESS_FULL_ACCESS)) {
        d->m_qecSharedNotebook.privilege = static_cast<qevercloud::SharedNotebookPrivilegeLevel::type>(privilegeLevel);
    }
    else {
        d->m_qecSharedNotebook.privilege.clear();
    }
}

bool SharedNotebook::hasReminderNotifyEmail() const
{
    const qevercloud::SharedNotebook & sharedNotebook = d->m_qecSharedNotebook;
    return sharedNotebook.recipientSettings.isSet() && sharedNotebook.recipientSettings->reminderNotifyEmail.isSet();
}

bool SharedNotebook::reminderNotifyEmail() const
{
    return d->m_qecSharedNotebook.recipientSettings->reminderNotifyEmail.ref();
}

#define CHECK_AND_SET_RECIPIENT_SETTINGS \
    qevercloud::SharedNotebook & sharedNotebook = d->m_qecSharedNotebook; \
    if (!sharedNotebook.recipientSettings.isSet()) { \
        sharedNotebook.recipientSettings = qevercloud::SharedNotebookRecipientSettings(); \
    }

void SharedNotebook::setReminderNotifyEmail(const bool notifyEmail)
{
    CHECK_AND_SET_RECIPIENT_SETTINGS
    sharedNotebook.recipientSettings->reminderNotifyEmail = notifyEmail;
}

bool SharedNotebook::hasReminderNotifyApp() const
{
    const qevercloud::SharedNotebook & sharedNotebook = d->m_qecSharedNotebook;
    return sharedNotebook.recipientSettings.isSet() && sharedNotebook.recipientSettings->reminderNotifyInApp.isSet();
}

bool SharedNotebook::reminderNotifyApp() const
{
    return d->m_qecSharedNotebook.recipientSettings->reminderNotifyInApp.ref();
}

void SharedNotebook::setReminderNotifyApp(const bool notifyApp)
{
    CHECK_AND_SET_RECIPIENT_SETTINGS
    sharedNotebook.recipientSettings->reminderNotifyInApp = notifyApp;
}

#undef CHECK_AND_SET_RECIPIENT_SETTINGS

bool SharedNotebook::hasRecipientUsername() const
{
    return d->m_qecSharedNotebook.recipientUsername.isSet();
}

const QString & SharedNotebook::recipientUsername() const
{
    return d->m_qecSharedNotebook.recipientUsername.ref();
}

void SharedNotebook::setRecipientUsername(const QString & recipientUsername)
{
    if (recipientUsername.isEmpty()) {
        d->m_qecSharedNotebook.recipientUsername.clear();
    }
    else {
        d->m_qecSharedNotebook.recipientUsername = recipientUsername;
    }
}

bool SharedNotebook::hasRecipientUserId() const
{
    return d->m_qecSharedNotebook.recipientUserId.isSet();
}

qint32 SharedNotebook::recipientUserId() const
{
    return d->m_qecSharedNotebook.recipientUserId.ref();
}

void SharedNotebook::setRecipientUserId(const qint32 recipientUserId)
{
    d->m_qecSharedNotebook.recipientUserId = recipientUserId;
}

bool SharedNotebook::hasRecipientIdentityId() const
{
    return d->m_qecSharedNotebook.recipientIdentityId.isSet();
}

qint64 SharedNotebook::recipientIdentityId() const
{
    return d->m_qecSharedNotebook.recipientIdentityId.ref();
}

void SharedNotebook::setRecipientIdentityId(const qint64 recipientIdentityId)
{
    d->m_qecSharedNotebook.recipientIdentityId = recipientIdentityId;
}

bool SharedNotebook::hasGlobalId() const
{
    return d->m_qecSharedNotebook.globalId.isSet();
}

const QString & SharedNotebook::globalId() const
{
    return d->m_qecSharedNotebook.globalId.ref();
}

void SharedNotebook::setGlobalId(const QString & globalId)
{
    if (!globalId.isEmpty()) {
        d->m_qecSharedNotebook.globalId = globalId;
    }
    else {
        d->m_qecSharedNotebook.globalId.clear();
    }
}

bool SharedNotebook::hasSharerUserId() const
{
    return d->m_qecSharedNotebook.sharerUserId.isSet();
}

qint32 SharedNotebook::sharerUserId() const
{
    return d->m_qecSharedNotebook.sharerUserId.ref();
}

void SharedNotebook::setSharerUserId(const qint32 sharerUserId)
{
    d->m_qecSharedNotebook.sharerUserId = sharerUserId;
}

bool SharedNotebook::hasAssignmentTimestamp() const
{
    return d->m_qecSharedNotebook.serviceAssigned.isSet();
}

qint64 SharedNotebook::assignmentTimestamp() const
{
    return d->m_qecSharedNotebook.serviceAssigned.ref();
}

void SharedNotebook::setAssignmentTimestamp(const qint64 timestamp)
{
    if (timestamp >= 0) {
        d->m_qecSharedNotebook.serviceAssigned = timestamp;
    }
    else {
        d->m_qecSharedNotebook.serviceAssigned.clear();
    }
}

QTextStream & SharedNotebook::print(QTextStream & strm) const
{
    strm << QStringLiteral("SharedNotebook {\n");
    strm << QStringLiteral("  index in notebook: ") << QString::number(d->m_indexInNotebook)
         << QStringLiteral(";\n");

    const qevercloud::SharedNotebook & sharedNotebook = d->m_qecSharedNotebook;
    strm << sharedNotebook;
    strm << QStringLiteral("};\n");
    return strm;
}

} // namespace quentier

