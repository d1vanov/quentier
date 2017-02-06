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

#include "data/LinkedNotebookData.h"
#include <quentier/types/LinkedNotebook.h>

namespace quentier {

QN_DEFINE_DIRTY(LinkedNotebook)

LinkedNotebook::LinkedNotebook() :
    d(new LinkedNotebookData)
{}

LinkedNotebook::LinkedNotebook(const LinkedNotebook & other) :
    d(other.d)
{}

LinkedNotebook::LinkedNotebook(LinkedNotebook && other) :
    d(other.d)
{}

LinkedNotebook & LinkedNotebook::operator=(const LinkedNotebook & other)
{
    if (this != &other) {
        d = other.d;
    }

    return *this;
}

LinkedNotebook & LinkedNotebook::operator=(LinkedNotebook && other)
{
    if (this != &other) {
        d = other.d;
    }

    return *this;
}

LinkedNotebook::LinkedNotebook(const qevercloud::LinkedNotebook & linkedNotebook) :
    d(new LinkedNotebookData(linkedNotebook))
{}

LinkedNotebook::LinkedNotebook(qevercloud::LinkedNotebook && linkedNotebook) :
    d(new LinkedNotebookData(std::move(linkedNotebook)))
{}

LinkedNotebook::~LinkedNotebook()
{}

LinkedNotebook::operator const qevercloud::LinkedNotebook & () const
{
    return d->m_qecLinkedNotebook;
}

LinkedNotebook::operator qevercloud::LinkedNotebook & ()
{
    return d->m_qecLinkedNotebook;
}

bool LinkedNotebook::operator==(const LinkedNotebook & other) const
{
    return ((isDirty() == other.isDirty()) && (d->m_qecLinkedNotebook == other.d->m_qecLinkedNotebook));
}

bool LinkedNotebook::operator!=(const LinkedNotebook & other) const
{
    return !(*this == other);
}

void LinkedNotebook::clear()
{
    d->m_qecLinkedNotebook = qevercloud::LinkedNotebook();
}

bool LinkedNotebook::hasGuid() const
{
    return d->m_qecLinkedNotebook.guid.isSet();
}

const QString & LinkedNotebook::guid() const
{
    return d->m_qecLinkedNotebook.guid.ref();
}

void LinkedNotebook::setGuid(const QString & guid)
{
    if (!guid.isEmpty()) {
        d->m_qecLinkedNotebook.guid = guid;
    }
    else {
        d->m_qecLinkedNotebook.guid.clear();
    }
}

bool LinkedNotebook::hasUpdateSequenceNumber() const
{
    return d->m_qecLinkedNotebook.updateSequenceNum.isSet();
}

qint32 LinkedNotebook::updateSequenceNumber() const
{
    return d->m_qecLinkedNotebook.updateSequenceNum;
}

void LinkedNotebook::setUpdateSequenceNumber(const qint32 usn)
{
    d->m_qecLinkedNotebook.updateSequenceNum = usn;
}

bool LinkedNotebook::checkParameters(ErrorString & errorDescription) const
{
    return d->checkParameters(errorDescription);
}

bool LinkedNotebook::hasShareName() const
{
    return d->m_qecLinkedNotebook.shareName.isSet();
}

const QString & LinkedNotebook::shareName() const
{
    return d->m_qecLinkedNotebook.shareName;
}

void LinkedNotebook::setShareName(const QString & shareName)
{
    if (!shareName.isEmpty()) {
        d->m_qecLinkedNotebook.shareName = shareName;
    }
    else {
        d->m_qecLinkedNotebook.shareName.clear();
    }
}

bool LinkedNotebook::hasUsername() const
{
    return d->m_qecLinkedNotebook.username.isSet();
}

const QString & LinkedNotebook::username() const
{
    return d->m_qecLinkedNotebook.username;
}

void LinkedNotebook::setUsername(const QString & username)
{
    if (!username.isEmpty()) {
        d->m_qecLinkedNotebook.username = username;
    }
    else {
        d->m_qecLinkedNotebook.username.clear();
    }
}

bool LinkedNotebook::hasShardId() const
{
    return d->m_qecLinkedNotebook.shardId.isSet();
}

const QString & LinkedNotebook::shardId() const
{
    return d->m_qecLinkedNotebook.shardId;
}

void LinkedNotebook::setShardId(const QString & shardId)
{
    if (!shardId.isEmpty()) {
        d->m_qecLinkedNotebook.shardId = shardId;
    }
    else {
        d->m_qecLinkedNotebook.shardId.clear();
    }
}

bool LinkedNotebook::hasSharedNotebookGlobalId() const
{
    return d->m_qecLinkedNotebook.sharedNotebookGlobalId.isSet();
}

const QString & LinkedNotebook::sharedNotebookGlobalId() const
{
    return d->m_qecLinkedNotebook.sharedNotebookGlobalId.ref();
}

void LinkedNotebook::setSharedNotebookGlobalId(const QString & sharedNotebookGlobalId)
{
    if (!sharedNotebookGlobalId.isEmpty()) {
        d->m_qecLinkedNotebook.sharedNotebookGlobalId = sharedNotebookGlobalId;
    }
    else {
        d->m_qecLinkedNotebook.sharedNotebookGlobalId.clear();
    }
}

bool LinkedNotebook::hasUri() const
{
    return d->m_qecLinkedNotebook.uri.isSet();
}

const QString & LinkedNotebook::uri() const
{
    return d->m_qecLinkedNotebook.uri;
}

void LinkedNotebook::setUri(const QString & uri)
{
    if (!uri.isEmpty()) {
        d->m_qecLinkedNotebook.uri = uri;
    }
    else {
        d->m_qecLinkedNotebook.uri.clear();
    }
}

bool LinkedNotebook::hasNoteStoreUrl() const
{
    return d->m_qecLinkedNotebook.noteStoreUrl.isSet();
}

const QString & LinkedNotebook::noteStoreUrl() const
{
    return d->m_qecLinkedNotebook.noteStoreUrl;
}

void LinkedNotebook::setNoteStoreUrl(const QString & noteStoreUrl)
{
    if (!noteStoreUrl.isEmpty()) {
        d->m_qecLinkedNotebook.noteStoreUrl = noteStoreUrl;
    }
    else {
        d->m_qecLinkedNotebook.noteStoreUrl.clear();
    }
}

bool LinkedNotebook::hasWebApiUrlPrefix() const
{
    return d->m_qecLinkedNotebook.webApiUrlPrefix.isSet();
}

const QString & LinkedNotebook::webApiUrlPrefix() const
{
    return d->m_qecLinkedNotebook.webApiUrlPrefix;
}

void LinkedNotebook::setWebApiUrlPrefix(const QString & webApiUrlPrefix)
{
    if (!webApiUrlPrefix.isEmpty()) {
        d->m_qecLinkedNotebook.webApiUrlPrefix = webApiUrlPrefix;
    }
    else {
        d->m_qecLinkedNotebook.webApiUrlPrefix.clear();
    }
}

bool LinkedNotebook::hasStack() const
{
    return d->m_qecLinkedNotebook.stack.isSet();
}

const QString & LinkedNotebook::stack() const
{
    return d->m_qecLinkedNotebook.stack;
}

void LinkedNotebook::setStack(const QString & stack)
{
    if (!stack.isEmpty()) {
        d->m_qecLinkedNotebook.stack = stack;
    }
    else {
        d->m_qecLinkedNotebook.stack.clear();
    }
}

bool LinkedNotebook::hasBusinessId() const
{
    return d->m_qecLinkedNotebook.businessId.isSet();
}

qint32 LinkedNotebook::businessId() const
{
    return d->m_qecLinkedNotebook.businessId;
}

void LinkedNotebook::setBusinessId(const qint32 businessId)
{
    d->m_qecLinkedNotebook.businessId = businessId;
}

QTextStream & LinkedNotebook::print(QTextStream & strm) const
{
    strm << QStringLiteral("LinkedNotebook: {\n");
    strm << QStringLiteral("isDirty = ") << (isDirty() ? QStringLiteral("true") : QStringLiteral("false"))
         << QStringLiteral("\n");
    strm << d->m_qecLinkedNotebook;
    strm << QStringLiteral("};\n");
    return strm;
}

} // namespace quentier
