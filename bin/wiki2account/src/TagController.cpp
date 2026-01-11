/*
 * Copyright 2019-2024 Dmitry Ivanov
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

#include <lib/exception/Utils.h>

#include <quentier/exception/InvalidArgument.h>
#include <quentier/local_storage/ILocalStorage.h>
#include <quentier/logging/QuentierLogger.h>
#include <quentier/threading/Future.h>
#include <quentier/utility/cancelers/ManualCanceler.h>

#include <qevercloud/types/builders/TagBuilder.h>

namespace quentier {

TagController::TagController(
    const quint32 minTagsPerNote, const quint32 maxTagsPerNote,
    local_storage::ILocalStoragePtr localStorage, QObject * parent) :
    QObject{parent}, m_localStorage{std::move(localStorage)},
    m_minTagsPerNote{minTagsPerNote}, m_maxTagsPerNote{maxTagsPerNote}
{
    if (Q_UNLIKELY(!m_localStorage)) {
        throw InvalidArgument{
            ErrorString{"TagController ctor: local storage is null"}};
    }
}

TagController::~TagController() = default;

void TagController::start()
{
    QNDEBUG("wiki2account::TagController", "TagController::start");

    if (m_minTagsPerNote == 0 && m_maxTagsPerNote == 0) {
        QNDEBUG(
            "wiki2account::TagController",
            "No tags are required, finishing early");
        Q_EMIT finished();
        return;
    }

    if (m_minTagsPerNote > m_maxTagsPerNote) {
        ErrorString errorDescription{
            QT_TR_NOOP("Min tags per note is greater than max tags per note")};

        QNWARNING(
            "wiki2account::TagController",
            errorDescription << ", min tags per note = " << m_minTagsPerNote
                             << ", max tags per note = " << m_maxTagsPerNote);

        Q_EMIT failure(std::move(errorDescription));
        return;
    }

    findNextTag();
}

void TagController::clear()
{
    QNDEBUG("wiki2account::TagController", "TagController::clear");

    m_tags.clear();
    m_nextNewTagNameSuffix = 1;

    if (m_canceler) {
        m_canceler->cancel();
        m_canceler.reset();
    }
}

void TagController::findNextTag()
{
    QNDEBUG("wiki2account::TagController", "TagController::findNextTag");

    QString tagName = nextNewTagName();

    QNDEBUG(
        "wiki2account::TagController",
        "Trying to find tag by name in local storage: " << tagName);

    auto canceler = setupCanceler();
    Q_ASSERT(canceler);

    auto findTagFuture = m_localStorage->findTagByName(tagName);
    auto findTagThenFuture = threading::then(
        std::move(findTagFuture), this,
        [this, canceler, tagName = std::move(tagName)](
            const std::optional<qevercloud::Tag> & tag) {
            if (canceler->isCanceled()) {
                return;
            }

            if (tag) {
                QNDEBUG(
                    "wiki2account::TagController",
                    "Successfully found tag by name in local storage: "
                        << *tag);

                m_tags << *tag;
                if (m_tags.size() == static_cast<int>(m_maxTagsPerNote)) {
                    QNDEBUG(
                        "wiki2account::TagController",
                        "Found the last required tag");
                    Q_EMIT finished();
                    return;
                }

                ++m_nextNewTagNameSuffix;
                findNextTag();
                return;
            }

            QNDEBUG(
                "wiki2account::TagController",
                "Found no tag with given name in local storage: " << tagName);

            createNextNewTag();
        });

    threading::onFailed(
        std::move(findTagThenFuture), this,
        [this, canceler = std::move(canceler)](const QException & e) {
            if (canceler->isCanceled()) {
                return;
            }

            auto message = exceptionMessage(e);

            ErrorString error{QT_TR_NOOP("Failed to find tag")};
            error.appendBase(message.base());
            error.appendBase(message.additionalBases());
            error.details() = std::move(message.details());
            QNWARNING("wiki2account::TagController", error);

            clear();
            Q_EMIT failure(std::move(error));
        });
}

void TagController::createNextNewTag()
{
    QNDEBUG("wiki2account::TagController", "TagController::createNextNewTag");

    QString tagName = nextNewTagName();
    auto tag = qevercloud::TagBuilder{}.setName(std::move(tagName)).build();

    QNDEBUG(
        "wiki2account::TagController",
        "Trying to create a new tag in local storage: " << tag);

    auto canceler = setupCanceler();
    Q_ASSERT(canceler);

    auto putTagFuture = m_localStorage->putTag(tag);
    auto putTagThenFuture = threading::then(
        std::move(putTagFuture), this, [this, canceler, tag = std::move(tag)] {
            if (canceler->isCanceled()) {
                return;
            }

            QNDEBUG(
                "wiki2account::TagController",
                "Successfully created a new tag in local storage: " << tag);

            m_tags << tag;
            if (m_tags.size() == static_cast<int>(m_maxTagsPerNote)) {
                QNDEBUG(
                    "wiki2account::TagController",
                    "Created the last required tag");
                Q_EMIT finished();
                return;
            }

            ++m_nextNewTagNameSuffix;
            findNextTag();
        });

    threading::onFailed(
        std::move(putTagThenFuture), this,
        [this, canceler = std::move(canceler)](const QException & e) {
            if (canceler->isCanceled()) {
                return;
            }

            auto message = exceptionMessage(e);

            ErrorString error{QT_TR_NOOP("Failed to create tag")};
            error.appendBase(message.base());
            error.appendBase(message.additionalBases());
            error.details() = std::move(message.details());
            QNWARNING("wiki2account::TagController", error);

            clear();
            Q_EMIT failure(std::move(error));
        });
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

utility::cancelers::ICancelerPtr TagController::setupCanceler()
{
    if (!m_canceler) {
        m_canceler = std::make_shared<utility::cancelers::ManualCanceler>();
    }

    return m_canceler;
}

} // namespace quentier
