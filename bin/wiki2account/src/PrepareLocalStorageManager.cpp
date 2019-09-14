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

#include "PrepareLocalStorageManager.h"

#include <quentier/local_storage/LocalStorageManagerAsync.h>

#include <QThread>

namespace quentier {

LocalStorageManagerAsync * prepareLocalStorageManager(
    const Account & account, QThread & localStorageThread,
    ErrorString & errorDescription)
{
    LocalStorageManagerAsync * pLocalStorageManager =
        new LocalStorageManagerAsync(account, LocalStorageManager::StartupOptions(0));
    pLocalStorageManager->init();

    auto & localStorageManager = *pLocalStorageManager->localStorageManager();
    if (localStorageManager.isLocalStorageVersionTooHigh(errorDescription)) {
        return Q_NULLPTR;
    }

    QVector<QSharedPointer<ILocalStoragePatch> > localStoragePatches =
        localStorageManager.requiredLocalStoragePatches();
    if (!localStoragePatches.isEmpty()) {
        errorDescription.setBase(QT_TR_NOOP("Local storage requires upgrade. "
                                            "Please start Quentier before running "
                                            "wiki2account"));
        return Q_NULLPTR;
    }

    pLocalStorageManager->moveToThread(&localStorageThread);
    return pLocalStorageManager;
}

} // namespace quentier
