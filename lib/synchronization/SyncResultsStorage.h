/*
 * Copyright 2024 Dmitry Ivanov
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

#pragma once

#include <quentier/synchronization/types/Fwd.h>

#include <QDateTime>
#include <QDir>
#include <QList>
#include <QtGlobal>

#include <utility>

namespace quentier {

class SyncResultsStorage
{
public:
    using SyncResultWithDateTime =
        std::pair<QDateTime, synchronization::ISyncResultPtr>;

    explicit SyncResultsStorage(
        const QDir & syncResultsStorageDir,
        quint32 maxStoredResults = 5);

    [[nodiscard]] QList<SyncResultWithDateTime> storedSyncResults() const;
    void storeSyncResult(const synchronization::ISyncResult & syncResult);

private:
    void checkAndRemoveOldestSyncResults();

private:
    const QDir m_syncResultsStorageDir;
    const quint32 m_maxStoredResults;
};

} // namespace quentier
