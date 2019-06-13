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

    // TODO: implement
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
        QNDEBUG(QStringLiteral("Added the last required tag"));
        Q_EMIT finished();
        return;
    }
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
    // TODO: implement
    Q_UNUSED(localStorageManagerAsync)
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

    // TODO: implement
}

void TagController::createNextNewTag()
{
    QNDEBUG(QStringLiteral("TagController::createNextNewTag"));

    // TODO: implement
}

QString TagController::nextNewTagName()
{
    // TODO: implement
    return QString();
}

} // namespace quentier
