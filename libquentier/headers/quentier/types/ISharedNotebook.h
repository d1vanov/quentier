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

#ifndef LIB_QUENTIER_TYPES_I_SHARED_NOTEBOOK_H
#define LIB_QUENTIER_TYPES_I_SHARED_NOTEBOOK_H

#include <quentier/utility/Printable.h>

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
#include <qt5qevercloud/QEverCloud.h>
#else
#include <qt4qevercloud/QEverCloud.h>
#endif

namespace quentier {

class QUENTIER_EXPORT ISharedNotebook: public Printable
{
public:
    typedef qevercloud::SharedNotebookPrivilegeLevel::type SharedNotebookPrivilegeLevel;

public:
    ISharedNotebook();
    virtual ~ISharedNotebook();

    bool operator==(const ISharedNotebook & other) const;
    bool operator!=(const ISharedNotebook & other) const;

    operator const qevercloud::SharedNotebook&() const;
    operator qevercloud::SharedNotebook&();

    int indexInNotebook() const;
    void setIndexInNotebook(const int index);

    bool hasId() const;
    qint64 id() const;
    void setId(const qint64 id);

    bool hasUserId() const;
    qint32 userId() const;
    void setUserId(const qint32 userId);

    bool hasNotebookGuid() const;
    const QString & notebookGuid() const;
    void setNotebookGuid(const QString & notebookGuid);

    bool hasEmail() const;
    const QString & email() const;
    void setEmail(const QString & email);

    bool hasCreationTimestamp() const;
    qint64 creationTimestamp() const;
    void setCreationTimestamp(const qint64 timestamp);

    bool hasModificationTimestamp() const;
    qint64 modificationTimestamp() const;
    void setModificationTimestamp(const qint64 timestamp);

    bool hasUsername() const;
    const QString & username() const;
    void setUsername(const QString & username);

    bool hasPrivilegeLevel() const;
    SharedNotebookPrivilegeLevel privilegeLevel() const;
    void setPrivilegeLevel(const SharedNotebookPrivilegeLevel privilegeLevel);
    void setPrivilegeLevel(const qint8 privilegeLevel);

    bool hasReminderNotifyEmail() const;
    bool reminderNotifyEmail() const;
    void setReminderNotifyEmail(const bool notifyEmail);

    bool hasReminderNotifyApp() const;
    bool reminderNotifyApp() const;
    void setReminderNotifyApp(const bool notifyApp);

    bool hasRecipientUsername() const;
    const QString & recipientUsername() const;
    void setRecipientUsername(const QString & recipientUsername);

    bool hasRecipientUserId() const;
    qint32 recipientUserId() const;
    void setRecipientUserId(const qint32 userId);

    bool hasAssignmentTimestamp() const;
    qint64 assignmentTimestamp() const;
    void setAssignmentTimestamp(const qint64 timestamp);

    virtual QTextStream & print(QTextStream & strm) const Q_DECL_OVERRIDE;

    friend class Notebook;

protected:
    ISharedNotebook(const ISharedNotebook & other);
    ISharedNotebook(ISharedNotebook && other);
    ISharedNotebook & operator=(const ISharedNotebook & other);
    ISharedNotebook & operator=(ISharedNotebook && other);

    virtual qevercloud::SharedNotebook & GetEnSharedNotebook() = 0;
    virtual const qevercloud::SharedNotebook & GetEnSharedNotebook() const = 0;

private:
    int  m_indexInNotebook;
};

} // namespace quentier

#endif // LIB_QUENTIER_TYPES_I_SHARED_NOTEBOOK_H
