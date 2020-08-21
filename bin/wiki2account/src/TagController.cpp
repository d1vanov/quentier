/*
 * Copyright 2019-2020 Dmitry Ivanov
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

TagController::TagController(
    const quint32 minTagsPerNote, const quint32 maxTagsPerNote,
    LocalStorageManagerAsync & localStorageManagerAsync, QObject * parent) :
    QObject(parent),
    m_minTagsPerNote(minTagsPerNote), m_maxTagsPerNote(maxTagsPerNote)
{
    createConnections(localStorageManagerAsync);
}

TagController::~TagController() {}

void TagController::start()
{
    QNDEBUG("wiki2account", "TagController::start");

    if (m_minTagsPerNote == 0 && m_maxTagsPerNote == 0) {
        QNDEBUG("wiki2account", "No tags are required, finishing early");
        Q_EMIT finished();
        return;
    }

    if (m_minTagsPerNote > m_maxTagsPerNote) {
        ErrorString errorDescription(
            QT_TR_NOOP("Min tags per note is greater than max tags per note"));

        QNWARNING(
            "wiki2account",
            errorDescription << ", min tags per note = " << m_minTagsPerNote
                             << ", max tags per note = " << m_maxTagsPerNote);

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

    QNDEBUG(
        "wiki2account",
        "TagController::onAddTagComplete: request id = " << requestId
                                                         << ", tag: " << tag);

    m_addTagRequestId = QUuid();

    m_tags << tag;
    if (m_tags.size() == static_cast<int>(m_maxTagsPerNote)) {
        QNDEBUG("wiki2account", "Added the last required tag");
        Q_EMIT finished();
        return;
    }

    ++m_nextNewTagNameSuffix;
    findNextTag();
}

void TagController::onAddTagFailed(
    Tag tag, ErrorString errorDescription, QUuid requestId)
{
    if (requestId != m_addTagRequestId) {
        return;
    }

    QNWARNING(
        "wiki2account",
        "TagController::onAddTagFailed: request id = "
            << requestId << ", error description = " << errorDescription
            << ", tag: " << tag);

    m_addTagRequestId = QUuid();

    clear();
    Q_EMIT failure(errorDescription);
}

void TagController::onFindTagComplete(Tag tag, QUuid requestId)
{
    if (requestId != m_findTagRequestId) {
        return;
    }

    QNDEBUG(
        "wiki2account",
        "TagController::onFindTagComplete: request id = " << requestId
                                                          << ", tag: " << tag);

    m_findTagRequestId = QUuid();

    m_tags << tag;
    if (m_tags.size() == static_cast<int>(m_maxTagsPerNote)) {
        QNDEBUG("wiki2account", "Found the last required tag");
        Q_EMIT finished();
        return;
    }

    ++m_nextNewTagNameSuffix;
    findNextTag();
}

void TagController::onFindTagFailed(
    Tag tag, ErrorString errorDescription, QUuid requestId)
{
    if (requestId != m_findTagRequestId) {
        return;
    }

    QNDEBUG(
        "wiki2account",
        "TagController::onFindTagFailed: request id = "
            << requestId << ", error description = " << errorDescription
            << ", tag: " << tag);

    m_findTagRequestId = QUuid();
    createNextNewTag();
}

void TagController::createConnections(
    LocalStorageManagerAsync & localStorageManagerAsync)
{
    QObject::connect(
        this, &TagController::addTag, &localStorageManagerAsync,
        &LocalStorageManagerAsync::onAddTagRequest);

    QObject::connect(
        this, &TagController::findTag, &localStorageManagerAsync,
        &LocalStorageManagerAsync::onFindTagRequest);

    QObject::connect(
        &localStorageManagerAsync, &LocalStorageManagerAsync::addTagComplete,
        this, &TagController::onAddTagComplete);

    QObject::connect(
        &localStorageManagerAsync, &LocalStorageManagerAsync::addTagFailed,
        this, &TagController::onAddTagFailed);

    QObject::connect(
        &localStorageManagerAsync, &LocalStorageManagerAsync::findTagComplete,
        this, &TagController::onFindTagComplete);

    QObject::connect(
        &localStorageManagerAsync, &LocalStorageManagerAsync::findTagFailed,
        this, &TagController::onFindTagFailed);
}

void TagController::clear()
{
    QNDEBUG("wiki2account", "TagController::clear");

    m_tags.clear();
    m_findTagRequestId = QUuid();
    m_addTagRequestId = QUuid();
    m_nextNewTagNameSuffix = 1;
}

void TagController::findNextTag()
{
    QNDEBUG("wiki2account", "TagController::findNextTag");

    QString tagName = nextNewTagName();

    Tag tag;
    tag.setLocalUid(QString());
    tag.setName(tagName);

    m_findTagRequestId = QUuid::createUuid();

    QNDEBUG(
        "wiki2account",
        "Emitting the request to find tag with name "
            << tagName << ", request id = " << m_findTagRequestId);

    Q_EMIT findTag(tag, m_findTagRequestId);
}

void TagController::createNextNewTag()
{
    QNDEBUG("wiki2account", "TagController::createNextNewTag");

    QString tagName = nextNewTagName();

    Tag tag;
    tag.setName(tagName);

    m_addTagRequestId = QUuid::createUuid();

    QNDEBUG(
        "wiki2account",
        "Emitting the request to add new tag: " << tag << "\nRequest id = "
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
