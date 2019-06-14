/*
 * Copyright 2019 Dmitry Ivanov
 *
 * This file is part of Quentier.
 *
 * Quentier is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * Quentier is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Quentier. If not, see <http://www.gnu.org/licenses/>.
 */

#include "TagController.h"

#include <quentier/local_storage/LocalStorageManagerAsync.h>
#include <quentier/logging/QuentierLogger.h>

namespace quentier {

TagController::TagController(const quint32 minTagsPerNote,
                             const quint32 maxTagsPerNote,
                             LocalStorageManagerAsync & localStorageManagerAsync,
                             QObject * parent) :
    QObject(parent),
    m_minTagsPerNote(minTagsPerNote),
    m_maxTagsPerNote(maxTagsPerNote),
    m_tags(),
    m_findTagRequestId(),
    m_addTagRequestId(),
    m_nextNewTagNameSuffix(1)
{
    createConnections(localStorageManagerAsync);
}

TagController::~TagController()
{}

void TagController::start()
{
    QNDEBUG(QStringLiteral("TagController::start"));

    if (m_minTagsPerNote == 0 && m_maxTagsPerNote == 0) {
        QNDEBUG(QStringLiteral("No tags are required, finishing early"));
        Q_EMIT finished();
        return;
    }

    if (m_minTagsPerNote > m_maxTagsPerNote) {
        ErrorString errorDescription(QT_TR_NOOP("Min tags per note is greater "
                                                "than max tags per note"));
        QNWARNING(errorDescription << QStringLiteral(", min tags per note = ")
                  << m_minTagsPerNote << QStringLiteral(", max tags per note = ")
                  << m_maxTagsPerNote);
        Q_EMIT failure(errorDescription);
        return;
    }

    findNextTag();
}

void TagController::onAddTagComplete(Tag tag, QUuid requestId)
{
    if (requestId != m_addTagRequestId) {
        return;
    }

    QNDEBUG(QStringLiteral("TagController::onAddTagComplete: request id = ")
            << requestId << QStringLiteral(", tag: ") << tag);

    m_addTagRequestId = QUuid();

    m_tags << tag;
    if (m_tags.size() == static_cast<int>(m_maxTagsPerNote)) {
        QNDEBUG(QStringLiteral("Added the last required tag"));
        Q_EMIT finished();
        return;
    }

    ++m_nextNewTagNameSuffix;
    findNextTag();
}

void TagController::onAddTagFailed(Tag tag, ErrorString errorDescription,
                                   QUuid requestId)
{
    if (requestId != m_addTagRequestId) {
        return;
    }

    QNWARNING(QStringLiteral("TagController::onAddTagFailed: request id = ")
              << requestId << QStringLiteral(", error description = ")
              << errorDescription << QStringLiteral(", tag: ") << tag);

    m_addTagRequestId = QUuid();

    clear();
    Q_EMIT failure(errorDescription);
}

void TagController::onFindTagComplete(Tag tag, QUuid requestId)
{
    if (requestId != m_findTagRequestId) {
        return;
    }

    QNDEBUG(QStringLiteral("TagController::onFindTagComplete: request id = ")
            << requestId << QStringLiteral(", tag: ") << tag);

    m_findTagRequestId = QUuid();

    m_tags << tag;
    if (m_tags.size() == static_cast<int>(m_maxTagsPerNote)) {
        QNDEBUG(QStringLiteral("Found the last required tag"));
        Q_EMIT finished();
        return;
    }

    ++m_nextNewTagNameSuffix;
    findNextTag();
}

void TagController::onFindTagFailed(Tag tag, ErrorString errorDescription,
                                    QUuid requestId)
{
    if (requestId != m_findTagRequestId) {
        return;
    }

    QNDEBUG(QStringLiteral("TagController::onFindTagFailed: request id = ")
            << requestId << QStringLiteral(", error description = ")
            << errorDescription << QStringLiteral(", tag: ") << tag);

    m_findTagRequestId = QUuid();
    createNextNewTag();
}

void TagController::createConnections(
    LocalStorageManagerAsync & localStorageManagerAsync)
{
    QObject::connect(this,
                     QNSIGNAL(TagController,addTag,Tag,QUuid),
                     &localStorageManagerAsync,
                     QNSLOT(LocalStorageManagerAsync,onAddTagRequest,Tag,QUuid));
    QObject::connect(this,
                     QNSIGNAL(TagController,findTag,Tag,QUuid),
                     &localStorageManagerAsync,
                     QNSLOT(LocalStorageManagerAsync,onFindTagRequest,Tag,QUuid));

    QObject::connect(&localStorageManagerAsync,
                     QNSIGNAL(LocalStorageManagerAsync,addTagComplete,Tag,QUuid),
                     this,
                     QNSLOT(TagController,onAddTagComplete,Tag,QUuid));
    QObject::connect(&localStorageManagerAsync,
                     QNSIGNAL(LocalStorageManagerAsync,addTagFailed,
                              Tag,ErrorString,QUuid),
                     this,
                     QNSLOT(TagController,onAddTagFailed,Tag,ErrorString,QUuid));
    QObject::connect(&localStorageManagerAsync,
                     QNSIGNAL(LocalStorageManagerAsync,findTagComplete,Tag,QUuid),
                     this,
                     QNSLOT(TagController,onFindTagComplete,Tag,QUuid));
    QObject::connect(&localStorageManagerAsync,
                     QNSIGNAL(LocalStorageManagerAsync,findTagFailed,
                              Tag,ErrorString,QUuid),
                     this,
                     QNSLOT(TagController,onFindTagFailed,Tag,ErrorString,QUuid));
}

void TagController::clear()
{
    QNDEBUG(QStringLiteral("TagController::clear"));

    m_tags.clear();
    m_findTagRequestId = QUuid();
    m_addTagRequestId = QUuid();
    m_nextNewTagNameSuffix = 1;
}

void TagController::findNextTag()
{
    QNDEBUG(QStringLiteral("TagController::findNextTag"));

    QString tagName = nextNewTagName();

    Tag tag;
    tag.setLocalUid(QString());
    tag.setName(tagName);

    m_findTagRequestId = QUuid::createUuid();
    QNDEBUG(QStringLiteral("Emitting the request to find tag with name ")
            << tagName << QStringLiteral(", request id = ")
            << m_findTagRequestId);
    Q_EMIT findTag(tag, m_findTagRequestId);
}

void TagController::createNextNewTag()
{
    QNDEBUG(QStringLiteral("TagController::createNextNewTag"));

    QString tagName = nextNewTagName();

    Tag tag;
    tag.setName(tagName);

    m_addTagRequestId = QUuid::createUuid();
    QNDEBUG(QStringLiteral("Emitting the request to add new tag: ")
            << tag << QStringLiteral("\nRequest id = ")
            << m_addTagRequestId);
    Q_EMIT addTag(tag, m_addTagRequestId);
}

QString TagController::nextNewTagName()
{
    QString tagName = QStringLiteral("Wiki tag");

    if (m_nextNewTagNameSuffix > 1) {
        tagName += QStringLiteral(" #");
        tagName += QString::number(m_nextNewTagNameSuffix);
    }

    return tagName;
}

} // namespace quentier
