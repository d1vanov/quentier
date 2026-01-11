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

#include "PrepareLocalStorage.h"

#include <lib/exception/Utils.h>

#include <quentier/local_storage/Factory.h>
#include <quentier/local_storage/ILocalStorage.h>
#include <quentier/logging/QuentierLogger.h>
#include <quentier/threading/Factory.h>

#include <QCoreApplication>

namespace quentier {

namespace {

template <class T>
void waitForFuture(const QFuture<T> & future)
{
    while (!future.isFinished()) {
        QCoreApplication::sendPostedEvents();
        QCoreApplication::processEvents();
    }
}

} // namespace

local_storage::ILocalStoragePtr prepareLocalStorage(
    const Account & account, const QDir & localStorageDir,
    ErrorString & errorDescription)
{
    auto localStorage =
        local_storage::createSqliteLocalStorage(account, localStorageDir);

    ////////////////////////////////////////////////////////////////////////////

    auto isVersionTooHighFuture = localStorage->isVersionTooHigh();
    waitForFuture(isVersionTooHighFuture);

    bool isVersionTooHigh = false;
    try {
        isVersionTooHigh = isVersionTooHighFuture.result();
    }
    catch (const QException & e) {
        auto message = exceptionMessage(e);

        errorDescription.setBase(QT_TRANSLATE_NOOP(
            "wiki2account::PrepareLocalStorage",
            "Failed to prepare local storage: cannot check whether version "
            "is too high"));
        errorDescription.appendBase(message.base());
        errorDescription.appendBase(message.additionalBases());
        errorDescription.details() = std::move(message.details());
        QNWARNING("wiki2account::PrepareLocalStorage", errorDescription);

        return nullptr;
    }

    if (isVersionTooHigh) {
        errorDescription.setBase(QT_TRANSLATE_NOOP(
            "wiki2account::PrepareLocalStorage",
            "Local storage version is too high"));

        QNWARNING("wiki2account::PrepareLocalStorage", errorDescription);

        return nullptr;
    }

    ////////////////////////////////////////////////////////////////////////////

    auto patchesFuture = localStorage->requiredPatches();
    waitForFuture(patchesFuture);

    QList<local_storage::IPatchPtr> patches;
    try {
        patches = patchesFuture.result();
    }
    catch (const QException & e) {
        auto message = exceptionMessage(e);

        errorDescription.setBase(QT_TRANSLATE_NOOP(
            "wiki2account::PrepareLocalStorage",
            "Failed to prepare local storage: cannot get required patches"));
        errorDescription.appendBase(message.base());
        errorDescription.appendBase(message.additionalBases());
        errorDescription.details() = std::move(message.details());
        QNWARNING("wiki2account::PrepareLocalStorage", errorDescription);

        return nullptr;
    }

    if (!patches.isEmpty()) {
        errorDescription.setBase(QT_TRANSLATE_NOOP(
            "wiki2account::PrepareLocalStorage",
            "Local storage requires upgrade. Please start Quentier before "
            "running wiki2account"));
        return nullptr;
    }

    ////////////////////////////////////////////////////////////////////////////

    return localStorage;
}

} // namespace quentier
