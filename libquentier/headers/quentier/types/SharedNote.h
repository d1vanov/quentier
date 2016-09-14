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

#ifndef LIB_QUENTIER_TYPES_SHARED_NOTE_H
#define LIB_QUENTIER_TYPES_SHARED_NOTE_H

#include <quentier/utility/Printable.h>

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
#include <qt5qevercloud/QEverCloud.h>
#else
#include <qt4qevercloud/QEverCloud.h>
#endif

#include <QSharedDataPointer>

namespace quentier {

QT_FORWARD_DECLARE_CLASS(SharedNoteData)

class SharedNote: public Printable
{
public:
    typedef qevercloud::SharedNotePrivilegeLevel::type SharedNotePrivilegeLevel;
    typedef qevercloud::ContactType::type ContactType;

public:
    SharedNote();
    SharedNote(const SharedNote & other);
    SharedNote(SharedNote && other);
    SharedNote(const qevercloud::SharedNote & sharedNote);
    SharedNote & operator=(const SharedNote & other);
    SharedNote & operator=(SharedNote && other);
    virtual ~SharedNote();

    bool operator==(const SharedNote & other) const;
    bool operator!=(const SharedNote & other) const;

    operator const qevercloud::SharedNote&() const;
    operator qevercloud::SharedNote&();

    const QString & noteGuid() const;
    void setNoteGuid(const QString & noteGuid);

    int indexInNote() const;
    void setIndexInNote(const int index);

    bool hasSharerUserId() const;
    qint32 sharerUserId() const;
    void setSharerUserId(const qint32 id);

    bool hasRecipientIdentityId() const;
    qint64 recipientIdentityId() const;
    void setRecipientIdentityId(const qint64 identityId);

    bool hasRecipientIdentityContactName() const;
    const QString & recipientIdentityContactName() const;
    void setRecipientIdentityContactName(const QString & recipientIdentityContactName);

    bool hasRecipientIdentityContactId() const;
    const QString & recipientIdentityContactId() const;
    void setRecipientIdentityContactId(const QString & recipientIdentityContactId);

    bool hasRecipientIdentityContactType() const;
    ContactType recipientIdentityContactType() const;
    void setRecipientIdentityContactType(const ContactType recipientIdentityContactType);
    void setRecipientIdentityContactType(const qint32 recipientIdentityContactType);

    bool hasRecipientIdentityContactPhotoUrl() const;
    const QString & recipientIdentityContactPhotoUrl() const;
    void setRecipientIdentityContactPhotoUrl(const QString & recipientIdentityPhotoUrl);

    bool hasRecipientIdentityContactPhotoLastUpdated() const;
    qint64 recipientIdentityContactPhotoLastUpdated() const;
    void setRecipientIdentityContactPhotoLastUpdated(const qint64 recipientIdentityPhotoLastUpdated);

    bool hasRecipientIdentityContactMessagingPermit() const;
    const QByteArray & recipientIdentityContactMessagingPermit() const;
    void setRecipientIdentityContactMessagingPermit(const QByteArray & messagingPermit);

    bool hasRecipientIdentityContactMessagingPermitExpires() const;
    qint64 recipientIdentityContactMessagingPermitExpires() const;
    void setRecipientIdentityContactMessagingPermitExpires(const qint64 timestamp);

    bool hasRecipientIdentityUserId() const;
    qint32 recipientIdentityUserId() const;
    void setRecipientIdentityUserId(const qint32 userId);

    bool hasRecipientIdentityDeactivated() const;
    bool recipientIdentityDeactivated() const;
    void setRecipientIdentityDeactivated(const bool deactivated);

    bool hasRecipientIdentitySameBusiness() const;
    bool recipientIdentitySameBusiness() const;
    void setRecipientIdentitySameBusiness(const bool sameBusiness);

    bool hasRecipientIdentityBlocked() const;
    bool recipientIdentityBlocked() const;
    void setRecipientIdentityBlocked(const bool blocked);

    bool hasRecipientIdentityUserConnected() const;
    bool recipientIdentityUserConnected() const;
    void setRecipientIdentityUserConnected(const bool userConnected);

    bool hasRecipientIdentityEventId() const;
    qint64 recipientIdentityEventId() const;
    void setRecipientIdentityEventId(const qint64 eventId);

    bool hasRecipientIdentity() const;
    const qevercloud::Identity & recipientIdentity() const;
    void setRecipientIdentity(qevercloud::Identity && identity);

    bool hasPrivilegeLevel() const;
    SharedNotePrivilegeLevel privilegeLevel() const;
    void setPrivilegeLevel(const SharedNotePrivilegeLevel level);
    void setPrivilegeLevel(const qint8 level);

    bool hasCreationTimestamp() const;
    qint64 creationTimestamp() const;
    void setCreationTimestamp(const qint64 timestamp);

    bool hasModificationTimestamp() const;
    qint64 modificationTimestamp() const;
    void setModificationTimestamp(const qint64 timestamp);

    bool hasAssignmentTimestamp() const;
    qint64 assignmentTimestamp() const;
    void setAssignmentTimestamp(const qint64 timestamp);

    virtual QTextStream & print(QTextStream & strm) const Q_DECL_OVERRIDE;

private:
    QSharedDataPointer<SharedNoteData> d;
};

} // namespace quentier

#endif // LIB_QUENTIER_TYPES_SHARED_NOTE_H
