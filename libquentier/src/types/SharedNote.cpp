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

#include "data/SharedNoteData.h"
#include <quentier/types/SharedNote.h>
#include <quentier/utility/Utility.h>

namespace quentier {

SharedNote::SharedNote() :
    d(new SharedNoteData)
{}

SharedNote::SharedNote(const SharedNote & other) :
    d(other.d)
{}

SharedNote::SharedNote(SharedNote && other) :
    d(std::move(other.d))
{}

SharedNote::SharedNote(const qevercloud::SharedNote & sharedNote) :
    d(new SharedNoteData)
{
    d->m_qecSharedNote = sharedNote;
}

SharedNote & SharedNote::operator=(const SharedNote & other)
{
    if (this != &other) {
        d = other.d;
    }

    return *this;
}

SharedNote & SharedNote::operator=(SharedNote && other)
{
    if (this != &other) {
        d = std::move(other.d);
    }

    return *this;
}

SharedNote::~SharedNote()
{}

bool SharedNote::operator==(const SharedNote & other) const
{
    const qevercloud::SharedNote & enSharedNote = d->m_qecSharedNote;
    const qevercloud::SharedNote & otherEnSharedNote = other.d->m_qecSharedNote;

    return (enSharedNote == otherEnSharedNote) && (d->m_noteGuid == other.d->m_noteGuid);

    // NOTE: m_indexInNote does not take any part in comparison
    // as it is by nature a helper parameter intended to preserve sorting of shared notes
    // in note between insertions and selections from the database
}

bool SharedNote::operator!=(const SharedNote & other) const
{
    return !(*this == other);
}

SharedNote::operator const qevercloud::SharedNote&() const
{
    return d->m_qecSharedNote;
}

SharedNote::operator qevercloud::SharedNote&()
{
    return d->m_qecSharedNote;
}

const QString & SharedNote::noteGuid() const
{
    return d->m_noteGuid;
}

void SharedNote::setNoteGuid(const QString & noteGuid)
{
    d->m_noteGuid = noteGuid;
}

int SharedNote::indexInNote() const
{
    return d->m_indexInNote;
}

void SharedNote::setIndexInNote(const int index)
{
    d->m_indexInNote = index;
}

bool SharedNote::hasSharerUserId() const
{
    return d->m_qecSharedNote.sharerUserID.isSet();
}

qint32 SharedNote::sharerUserId() const
{
    return d->m_qecSharedNote.sharerUserID;
}

void SharedNote::setSharerUserId(const qint32 id)
{
    d->m_qecSharedNote.sharerUserID = id;
}

bool SharedNote::hasRecipientIdentityId() const
{
    if (!hasRecipientIdentity()) {
        return false;
    }

    return true;
}

qint64 SharedNote::recipientIdentityId() const
{
    return d->m_qecSharedNote.recipientIdentity->id;
}

void SharedNote::setRecipientIdentityId(const qint64 identityId)
{
    if (!d->m_qecSharedNote.recipientIdentity.isSet()) {
        d->m_qecSharedNote.recipientIdentity = qevercloud::Identity();
    }

    d->m_qecSharedNote.recipientIdentity->id = identityId;
}

bool SharedNote::hasRecipientIdentityContactName() const
{
    if (!hasRecipientIdentity()) {
        return false;
    }

    return (d->m_qecSharedNote.recipientIdentity->contact.isSet() &&
            d->m_qecSharedNote.recipientIdentity->contact->name.isSet());
}

const QString & SharedNote::recipientIdentityContactName() const
{
    return d->m_qecSharedNote.recipientIdentity->contact->name;
}

void SharedNote::setRecipientIdentityContactName(const QString & recipientIdentityContactName)
{
    if (recipientIdentityContactName.isEmpty())
    {
        if (!d->m_qecSharedNote.recipientIdentity.isSet()) {
            return;
        }

        if (!d->m_qecSharedNote.recipientIdentity->contact.isSet()) {
            return;
        }

        d->m_qecSharedNote.recipientIdentity->contact->name.clear();
    }
    else
    {
        if (!d->m_qecSharedNote.recipientIdentity.isSet()) {
            d->m_qecSharedNote.recipientIdentity = qevercloud::Identity();
        }

        if (!d->m_qecSharedNote.recipientIdentity->contact.isSet()) {
            d->m_qecSharedNote.recipientIdentity->contact = qevercloud::Contact();
        }

        d->m_qecSharedNote.recipientIdentity->contact->name = recipientIdentityContactName;
    }
}

bool SharedNote::hasRecipientIdentityContactId() const
{
    return d->m_qecSharedNote.recipientIdentity.isSet() && d->m_qecSharedNote.recipientIdentity->contact.isSet() &&
           d->m_qecSharedNote.recipientIdentity->contact->id.isSet();
}

const QString & SharedNote::recipientIdentityContactId() const
{
    return d->m_qecSharedNote.recipientIdentity->contact->id;
}

void SharedNote::setRecipientIdentityContactId(const QString & recipientIdentityContactId)
{
    if (recipientIdentityContactId.isEmpty())
    {
        if (!d->m_qecSharedNote.recipientIdentity.isSet()) {
            return;
        }

        if (!d->m_qecSharedNote.recipientIdentity->contact.isSet()) {
            return;
        }

        d->m_qecSharedNote.recipientIdentity->contact->id.clear();
    }
    else
    {
        if (!d->m_qecSharedNote.recipientIdentity.isSet()) {
            d->m_qecSharedNote.recipientIdentity = qevercloud::Identity();
        }

        if (!d->m_qecSharedNote.recipientIdentity->contact.isSet()) {
            d->m_qecSharedNote.recipientIdentity->contact = qevercloud::Contact();
        }

        d->m_qecSharedNote.recipientIdentity->contact->id = recipientIdentityContactId;
    }
}

bool SharedNote::hasRecipientIdentityContactType() const
{
    return d->m_qecSharedNote.recipientIdentity.isSet() && d->m_qecSharedNote.recipientIdentity->contact.isSet() &&
           d->m_qecSharedNote.recipientIdentity->contact->type.isSet();
}

qevercloud::ContactType::type SharedNote::recipientIdentityContactType() const
{
    return d->m_qecSharedNote.recipientIdentity->contact->type;
}

void SharedNote::setRecipientIdentityContactType(const qevercloud::ContactType::type recipientIdentityContactType)
{
    if (!d->m_qecSharedNote.recipientIdentity.isSet()) {
        d->m_qecSharedNote.recipientIdentity = qevercloud::Identity();
    }

    if (!d->m_qecSharedNote.recipientIdentity->contact.isSet()) {
        d->m_qecSharedNote.recipientIdentity->contact = qevercloud::Contact();
    }

    d->m_qecSharedNote.recipientIdentity->contact->type = recipientIdentityContactType;
}

void SharedNote::setRecipientIdentityContactType(const qint32 recipientIdentityContactType)
{
    if ( (recipientIdentityContactType >= static_cast<qint32>(qevercloud::ContactType::EVERNOTE)) &&
         (recipientIdentityContactType <= static_cast<qint32>(qevercloud::ContactType::LINKEDIN)) )
    {
        setRecipientIdentityContactType(static_cast<qevercloud::ContactType::type>(recipientIdentityContactType));
    }
    else
    {
        if (!d->m_qecSharedNote.recipientIdentity.isSet()) {
            return;
        }

        if (!d->m_qecSharedNote.recipientIdentity->contact.isSet()) {
            return;
        }

        d->m_qecSharedNote.recipientIdentity->contact->type.clear();
    }
}

bool SharedNote::hasRecipientIdentityContactPhotoUrl() const
{
    return d->m_qecSharedNote.recipientIdentity.isSet() && d->m_qecSharedNote.recipientIdentity->contact.isSet() &&
           d->m_qecSharedNote.recipientIdentity->contact->photoUrl.isSet();
}

const QString & SharedNote::recipientIdentityContactPhotoUrl() const
{
    return d->m_qecSharedNote.recipientIdentity->contact->photoUrl;
}

void SharedNote::setRecipientIdentityContactPhotoUrl(const QString & recipientIdentityContactPhotoUrl)
{
    if (recipientIdentityContactPhotoUrl.isEmpty())
    {
        if (!d->m_qecSharedNote.recipientIdentity.isSet()) {
            return;
        }

        if (!d->m_qecSharedNote.recipientIdentity->contact.isSet()) {
            return;
        }

        d->m_qecSharedNote.recipientIdentity->contact->photoUrl.clear();
    }
    else
    {
        if (!d->m_qecSharedNote.recipientIdentity.isSet()) {
            d->m_qecSharedNote.recipientIdentity = qevercloud::Identity();
        }

        if (!d->m_qecSharedNote.recipientIdentity->contact.isSet()) {
            d->m_qecSharedNote.recipientIdentity->contact = qevercloud::Contact();
        }

        d->m_qecSharedNote.recipientIdentity->contact->photoUrl = recipientIdentityContactPhotoUrl;
    }
}

bool SharedNote::hasRecipientIdentityContactPhotoLastUpdated() const
{
    return d->m_qecSharedNote.recipientIdentity.isSet() && d->m_qecSharedNote.recipientIdentity->contact.isSet() &&
           d->m_qecSharedNote.recipientIdentity->contact->photoLastUpdated.isSet();
}

qint64 SharedNote::recipientIdentityContactPhotoLastUpdated() const
{
    return d->m_qecSharedNote.recipientIdentity->contact->photoLastUpdated;
}

void SharedNote::setRecipientIdentityContactPhotoLastUpdated(const qint64 timestamp)
{
    if (timestamp >= 0)
    {
        if (!d->m_qecSharedNote.recipientIdentity.isSet()) {
            d->m_qecSharedNote.recipientIdentity = qevercloud::Identity();
        }

        if (!d->m_qecSharedNote.recipientIdentity->contact.isSet()) {
            d->m_qecSharedNote.recipientIdentity->contact = qevercloud::Contact();
        }

        d->m_qecSharedNote.recipientIdentity->contact->photoLastUpdated = timestamp;
    }
    else
    {
        if (!d->m_qecSharedNote.recipientIdentity.isSet()) {
            return;
        }

        if (!d->m_qecSharedNote.recipientIdentity->contact.isSet()) {
            return;
        }

        d->m_qecSharedNote.recipientIdentity->contact->photoLastUpdated.clear();
    }
}

bool SharedNote::hasRecipientIdentityUserId() const
{
    return d->m_qecSharedNote.recipientIdentity.isSet() && d->m_qecSharedNote.recipientIdentity->userId.isSet();
}

qint32 SharedNote::recipientIdentityUserId() const
{
    return d->m_qecSharedNote.recipientIdentity->userId;
}

void SharedNote::setRecipientIdentityUserId(const qint32 userId)
{
    if (!d->m_qecSharedNote.recipientIdentity.isSet()) {
        d->m_qecSharedNote.recipientIdentity = qevercloud::Identity();
    }

    d->m_qecSharedNote.recipientIdentity->userId = userId;
}

bool SharedNote::hasRecipientIdentity() const
{
    return d->m_qecSharedNote.recipientIdentity.isSet();
}

const qevercloud::Identity & SharedNote::recipientIdentity() const
{
    return d->m_qecSharedNote.recipientIdentity;
}

void SharedNote::setRecipientIdentity(qevercloud::Identity && recipientIdentity)
{
    d->m_qecSharedNote.recipientIdentity = std::move(recipientIdentity);
}

bool SharedNote::hasPrivilegeLevel() const
{
    return d->m_qecSharedNote.privilege.isSet();
}

SharedNote::SharedNotePrivilegeLevel SharedNote::privilegeLevel() const
{
    return d->m_qecSharedNote.privilege;
}

void SharedNote::setPrivilegeLevel(const SharedNote::SharedNotePrivilegeLevel level)
{
    d->m_qecSharedNote.privilege = level;
}

void SharedNote::setPrivilegeLevel(const qint8 level)
{
    if (level <= static_cast<qint8>(qevercloud::SharedNotePrivilegeLevel::FULL_ACCESS)) {
        d->m_qecSharedNote.privilege = static_cast<qevercloud::SharedNotePrivilegeLevel::type>(level);
    }
    else {
        d->m_qecSharedNote.privilege.clear();
    }
}

bool SharedNote::hasCreationTimestamp() const
{
    return d->m_qecSharedNote.serviceCreated.isSet();
}

qint64 SharedNote::creationTimestamp() const
{
    return d->m_qecSharedNote.serviceCreated;
}

void SharedNote::setCreationTimestamp(const qint64 timestamp)
{
    if (timestamp >= 0) {
        d->m_qecSharedNote.serviceCreated = timestamp;
    }
    else {
        d->m_qecSharedNote.serviceCreated.clear();
    }
}

bool SharedNote::hasModificationTimestamp() const
{
    return d->m_qecSharedNote.serviceUpdated.isSet();
}

qint64 SharedNote::modificationTimestamp() const
{
    return d->m_qecSharedNote.serviceUpdated;
}

void SharedNote::setModificationTimestamp(const qint64 timestamp)
{
    if (timestamp >= 0) {
        d->m_qecSharedNote.serviceUpdated = timestamp;
    }
    else {
        d->m_qecSharedNote.serviceUpdated.clear();
    }
}

bool SharedNote::hasAssignmentTimestamp() const
{
    return d->m_qecSharedNote.serviceAssigned.isSet();
}

qint64 SharedNote::assignmentTimestamp() const
{
    return d->m_qecSharedNote.serviceAssigned;
}

void SharedNote::setAssignmentTimestamp(const qint64 timestamp)
{
    if (timestamp >= 0) {
        d->m_qecSharedNote.serviceAssigned = timestamp;
    }
    else {
        d->m_qecSharedNote.serviceAssigned.clear();
    }
}

QTextStream & SharedNote::print(QTextStream & strm) const
{
    strm << QStringLiteral("SharedNote {\n");

    strm << d->m_qecSharedNote;
    strm << QStringLiteral("index in note = ") << d->m_indexInNote << QStringLiteral(";\n");
    strm << QStringLiteral("};\n");

    return strm;
}

} // namespace quentier
