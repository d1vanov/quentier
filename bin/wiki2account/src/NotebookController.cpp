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

#include "NotebookController.h"

#include <lib/exception/Utils.h>

#include <quentier/exception/InvalidArgument.h>
#include <quentier/local_storage/ILocalStorage.h>
#include <quentier/logging/QuentierLogger.h>
#include <quentier/threading/Future.h>
#include <quentier/utility/cancelers/ManualCanceler.h>

#include <qevercloud/types/builders/NotebookBuilder.h>

#include <utility>

namespace quentier {

NotebookController::NotebookController(
    QString targetNotebookName, const quint32 numNotebooks,
    local_storage::ILocalStoragePtr localStorage, QObject * parent) :
    QObject{parent}, m_localStorage{std::move(localStorage)},
    m_targetNotebookName{std::move(targetNotebookName)},
    m_numNewNotebooks{numNotebooks}
{
    if (Q_UNLIKELY(!m_localStorage)) {
        throw InvalidArgument{
            ErrorString{"NotebookController ctor: local storage is null"}};
    }
}

NotebookController::~NotebookController() = default;

void NotebookController::start()
{
    QNDEBUG("wiki2account::NotebookController", "NotebookController::start");

    if (!m_targetNotebookName.isEmpty()) {
        QNDEBUG(
            "wiki2account::NotebookController",
            "Trying to find notebook by target name in local storage: "
                << m_targetNotebookName);

        auto canceler = setupCanceler();
        Q_ASSERT(canceler);

        auto findNotebookFuture =
            m_localStorage->findNotebookByName(m_targetNotebookName);

        auto findNotebookThenFuture = threading::then(
            std::move(findNotebookFuture), this,
            [this,
             canceler](const std::optional<qevercloud::Notebook> & notebook) {
                if (canceler->isCanceled()) {
                    return;
                }

                if (notebook) {
                    QNDEBUG(
                        "wiki2account::NotebookController",
                        "Found notebook by target name in local storage: "
                            << *notebook);

                    m_targetNotebook = std::move(*notebook);
                    Q_EMIT finished();
                    return;
                }

                QNDEBUG(
                    "wiki2account::NotebookController",
                    "There is no notebook with name " << m_targetNotebookName
                                                      << " in local storage, "
                                                      << "creating one");

                auto newNotebook = qevercloud::NotebookBuilder{}
                                       .setName(m_targetNotebookName)
                                       .build();

                QNDEBUG(
                    "wiki2account::NotebookController",
                    "Trying to create notebook in local storage: "
                        << newNotebook);

                auto putNotebookFuture =
                    m_localStorage->putNotebook(newNotebook);

                auto putNotebookThenFuture = threading::then(
                    std::move(putNotebookFuture), this,
                    [this, canceler,
                     newNotebook = std::move(newNotebook)]() mutable {
                        if (canceler->isCanceled()) {
                            return;
                        }

                        QNDEBUG(
                            "wiki2account::NotebookController",
                            "Successfully created notebook in local storage: "
                                << newNotebook);

                        m_targetNotebook = std::move(newNotebook);
                        Q_EMIT finished();
                    });

                threading::onFailed(
                    std::move(putNotebookThenFuture), this,
                    [this,
                     canceler = std::move(canceler)](const QException & e) {
                        if (canceler->isCanceled()) {
                            return;
                        }

                        auto message = exceptionMessage(e);

                        ErrorString error{
                            QT_TR_NOOP("Failed to create notebook")};
                        error.appendBase(message.base());
                        error.appendBase(message.additionalBases());
                        error.details() = std::move(message.details());
                        QNWARNING("wiki2account::NotebookController", error);

                        clear();
                        Q_EMIT failure(std::move(error));
                    });
            });

        threading::onFailed(
            std::move(findNotebookThenFuture), this,
            [this, canceler = std::move(canceler)](const QException & e) {
                if (canceler->isCanceled()) {
                    return;
                }

                auto message = exceptionMessage(e);

                ErrorString error{QT_TR_NOOP("Failed to find notebook")};
                error.appendBase(message.base());
                error.appendBase(message.additionalBases());
                error.details() = std::move(message.details());
                QNWARNING("wiki2account::NotebookController", error);

                clear();
                Q_EMIT failure(std::move(error));
            });

        return;
    }

    if (m_numNewNotebooks > 0) {
        QNDEBUG(
            "wiki2account::NotebookController",
            "Trying to list notebooks from local storage");

        auto canceler = setupCanceler();
        Q_ASSERT(canceler);

        auto listNotebooksFuture = m_localStorage->listNotebooks();
        auto listNotebooksThenFuture = threading::then(
            std::move(listNotebooksFuture), this,
            [this, canceler](const QList<qevercloud::Notebook> & notebooks) {
                if (canceler->isCanceled()) {
                    return;
                }

                QNDEBUG(
                    "wiki2account::NotebookController",
                    "Found " << notebooks.size() << " notebooks in local "
                             << "storage");

                m_notebookLocalIdsByNames.reserve(notebooks.size());

                for (const auto & notebook: std::as_const(notebooks)) {
                    if (Q_UNLIKELY(!notebook.name())) {
                        continue;
                    }

                    m_notebookLocalIdsByNames[*notebook.name()] =
                        notebook.localId();
                }

                createNextNewNotebook();
            });

        threading::onFailed(
            std::move(listNotebooksThenFuture), this,
            [this, canceler = std::move(canceler)](const QException & e) {
                if (canceler->isCanceled()) {
                    return;
                }

                auto message = exceptionMessage(e);

                ErrorString error{QT_TR_NOOP("Failed to list notebooks")};
                error.appendBase(message.base());
                error.appendBase(message.additionalBases());
                error.details() = std::move(message.details());
                QNWARNING("wiki2account::NotebookController", error);

                clear();
                Q_EMIT failure(std::move(error));
            });

        return;
    }

    ErrorString errorDescription{QT_TR_NOOP(
        "Neither target notebook name nor number of new notebooks is set")};

    QNWARNING("wiki2account::NotebookController", errorDescription);
    Q_EMIT failure(std::move(errorDescription));
}

void NotebookController::clear()
{
    QNDEBUG("wiki2account::NotebookController", "NotebookController::clear");

    m_notebookLocalIdsByNames.clear();

    m_targetNotebook = qevercloud::Notebook{};
    m_newNotebooks.clear();
    m_lastNewNotebookIndex = 1;

    if (m_canceler) {
        m_canceler->cancel();
        m_canceler.reset();
    }
}

void NotebookController::createNextNewNotebook()
{
    QNDEBUG(
        "wiki2account::NotebookController",
        "NotebookController::createNextNewNotebook: index = "
            << m_lastNewNotebookIndex);

    QString baseName = QStringLiteral("Wiki notes notebook");
    QString name;
    qint32 index = m_lastNewNotebookIndex;

    while (true) {
        name = baseName;
        if (index > 1) {
            name += QStringLiteral(" #") + QString::number(index);
        }

        if (const auto it = m_notebookLocalIdsByNames.find(name);
            it == m_notebookLocalIdsByNames.end())
        {
            break;
        }

        ++index;
    }

    auto newNotebook = qevercloud::NotebookBuilder{}.setName(name).build();

    QNDEBUG(
        "wiki2account::NotebookController",
        "Trying to create new notebook in local storage: " << newNotebook);

    auto canceler = setupCanceler();
    Q_ASSERT(canceler);

    auto putNotebookFuture = m_localStorage->putNotebook(newNotebook);
    auto putNotebookThenFuture = threading::then(
        std::move(putNotebookFuture), this,
        [this, canceler, newNotebook = std::move(newNotebook)] {
            if (canceler->isCanceled()) {
                return;
            }

            QNDEBUG(
                "wiki2account::NotebookController",
                "Successfully created new notebook in local storage: "
                    << newNotebook);

            m_newNotebooks << newNotebook;
            if (m_newNotebooks.size() == static_cast<int>(m_numNewNotebooks)) {
                QNDEBUG(
                    "wiki2account::NotebookController",
                    "Created the last needed new notebook, finishing");
                Q_EMIT finished();
                return;
            }

            m_notebookLocalIdsByNames[*newNotebook.name()] =
                newNotebook.localId();

            createNextNewNotebook();
        });

    threading::onFailed(
        std::move(putNotebookThenFuture), this,
        [this, canceler = std::move(canceler)](const QException & e) {
            if (canceler->isCanceled()) {
                return;
            }

            auto message = exceptionMessage(e);

            ErrorString error{QT_TR_NOOP("Failed to create notebook")};
            error.appendBase(message.base());
            error.appendBase(message.additionalBases());
            error.details() = std::move(message.details());
            QNWARNING("wiki2account::NotebookController", error);

            clear();
            Q_EMIT failure(std::move(error));
        });
}

utility::cancelers::ICancelerPtr NotebookController::setupCanceler()
{
    if (!m_canceler) {
        m_canceler = std::make_shared<utility::cancelers::ManualCanceler>();
    }

    return m_canceler;
}

} // namespace quentier
