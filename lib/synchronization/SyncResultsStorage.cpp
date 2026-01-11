/*
 * Copyright 2024-2025 Dmitry Ivanov
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

#include "SyncResultsStorage.h"

#include <quentier/exception/RuntimeError.h>
#include <quentier/logging/QuentierLogger.h>
#include <quentier/synchronization/types/serialization/json/SyncResult.h>
#include <quentier/utility/DateTime.h>
#include <quentier/utility/FileSystem.h>

#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QtGlobal>

#include <algorithm>
#include <string_view>

namespace quentier {

using namespace std::string_view_literals;

namespace {

constexpr auto gTimestampKey = "timestamp"sv;
constexpr auto gSyncResultKey = "syncResult"sv;

[[nodiscard]] QString toString(const std::string_view str)
{
    return QString::fromUtf8(
        str.data(),
        static_cast<int>(std::min<std::size_t>(
            str.size(), std::numeric_limits<int>::max())));
}

} // namespace

SyncResultsStorage::SyncResultsStorage(
    const QDir & syncResultsStorageDir, int maxStoredResults) :
    m_syncResultsStorageDir{syncResultsStorageDir},
    m_maxStoredResults{maxStoredResults}
{
    if (!m_syncResultsStorageDir.exists() &&
        !m_syncResultsStorageDir.mkpath(m_syncResultsStorageDir.absolutePath()))
    {
        throw RuntimeError{ErrorString{QT_TRANSLATE_NOOP(
            "synchronization::SyncResultsStorage",
            "Failed to create dir for last sync results storage")}};
    }
}

QList<SyncResultsStorage::SyncResultWithDateTime>
SyncResultsStorage::storedSyncResults() const
{
    const auto fileInfoEntries = m_syncResultsStorageDir.entryInfoList(
        QDir::Files | QDir::NoDotAndDotDot);

    QList<SyncResultsStorage::SyncResultWithDateTime> result;
    result.reserve(fileInfoEntries.size());
    for (const auto & fileInfo: std::as_const(fileInfoEntries)) {
        QFile file{fileInfo.absolutePath()};
        if (!file.open(QIODevice::ReadOnly)) {
            QNWARNING(
                "synchronization::SyncResultsStorage",
                "Failed to open file with stored sync result for reading: "
                    << fileInfo.absolutePath());
            continue;
        }

        QJsonParseError error;
        auto doc = QJsonDocument::fromJson(file.readAll(), &error);
        if (Q_UNLIKELY(error.error != QJsonParseError::NoError)) {
            QNWARNING(
                "synchronization::SyncResultsStorage",
                "Failed to parse file with stored sync result into json: "
                    << error.errorString());
            continue;
        }

        if (Q_UNLIKELY(!doc.isObject())) {
            QNWARNING(
                "synchronization::SyncResultsStorage",
                "Stored sync result json is not an object: "
                    << doc.toJson(QJsonDocument::Indented));
            continue;
        }

        const QJsonObject obj = doc.object();
        const auto timestampIt = obj.constFind(toString(gTimestampKey));
        if (Q_UNLIKELY(timestampIt == obj.constEnd())) {
            QNWARNING(
                "synchronization::SyncResultsStorage",
                "No timestamp in stored sync result json: "
                    << doc.toJson(QJsonDocument::Indented));
            continue;
        }

        if (Q_UNLIKELY(timestampIt->isString())) {
            QNWARNING(
                "synchronization::SyncResultsStorage",
                "Timestamp in stored sync result json is not encoded as a "
                    << "string: " << doc.toJson(QJsonDocument::Indented));
            continue;
        }

        const auto timestampStr = timestampIt->toString();
        bool timestampValid = false;
        const qint64 timestamp = timestampStr.toLongLong(&timestampValid);
        if (Q_UNLIKELY(!timestampValid)) {
            QNWARNING(
                "synchronization::SyncResultsStorage",
                "Timestamp in stored sync result json cannot be parsed to int: "
                    << doc.toJson(QJsonDocument::Indented));
            continue;
        }

        const auto syncResultIt = obj.constFind(toString(gSyncResultKey));
        if (Q_UNLIKELY(syncResultIt == obj.constEnd())) {
            QNWARNING(
                "synchronization::SyncResultsStorage",
                "No sync result in stored sync result json: "
                    << doc.toJson(QJsonDocument::Indented));
            continue;
        }

        if (Q_UNLIKELY(!syncResultIt->isObject())) {
            QNWARNING(
                "synchronization::SyncResultsStorage",
                "Sync result in stored sync result json is not an object: "
                    << doc.toJson(QJsonDocument::Indented));
            continue;
        }

        const QJsonObject syncResultObject = syncResultIt->toObject();

        auto syncResult =
            synchronization::deserializeSyncResultFromJson(syncResultObject);
        if (Q_UNLIKELY(!syncResult)) {
            QNWARNING(
                "synchronization::SyncResultsStorage",
                "Failed to deserialize stores sync result: "
                    << doc.toJson(QJsonDocument::Indented));
        }

        result << SyncResultWithDateTime{
            QDateTime::fromMSecsSinceEpoch(timestamp), std::move(syncResult)};
    }

    return result;
}

void SyncResultsStorage::storeSyncResult(
    const synchronization::ISyncResult & syncResult)
{
    auto syncResultJson =
        synchronization::serializeSyncResultToJson(syncResult);

    const auto timestamp = QDateTime::currentMSecsSinceEpoch();
    const auto printableTimestamp = utility::printableDateTimeFromTimestamp(
        timestamp, utility::DateTimePrintOptions{}, "%Y_%m_%d_%H_%M_%S");

    QFile file{m_syncResultsStorageDir.absoluteFilePath(
        printableTimestamp + QStringLiteral(".json"))};
    if (!file.open(QIODevice::WriteOnly)) {
        QNWARNING(
            "synchronization::SyncResultSaver",
            "Failed to open file for sync result storage: "
                << QFileInfo{file}.absolutePath());
        return;
    }

    QJsonObject obj;
    obj[toString(gTimestampKey)] = QString::number(timestamp);
    obj[toString(gSyncResultKey)] = syncResultJson;

    QJsonDocument doc{obj};
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();

    QNDEBUG(
        "synchronization::SyncResultSaver",
        "Stored sync result to file: " << printableTimestamp << ".json");

    checkAndRemoveOldestSyncResults();
}

void SyncResultsStorage::checkAndRemoveOldestSyncResults()
{
    auto fileInfoEntries = m_syncResultsStorageDir.entryInfoList(
        QDir::Files | QDir::NoDotAndDotDot);

    if (fileInfoEntries.size() <= m_maxStoredResults) {
        return;
    }

    std::sort(
        fileInfoEntries.begin(), fileInfoEntries.end(),
        [](const QFileInfo & lhs, const QFileInfo & rhs) {
            const auto leftName = lhs.fileName();
            const auto rightName = rhs.fileName();
            return leftName < rightName;
        });

    const auto staleEntryCount = fileInfoEntries.size() - m_maxStoredResults;
    for (std::decay_t<decltype(staleEntryCount)> i = 0; i < staleEntryCount;
         ++i)
    {
        const auto & fileEntry = fileInfoEntries[i];
        QNDEBUG(
            "synchronization::SyncResultsStorage",
            "Removing old stored sync result entry: " << fileEntry.fileName());

        if (!utility::removeFile(fileEntry.absolutePath())) {
            QNWARNING(
                "synchronization::SyncResultsStorage",
                "Failed to remove old stored sync result entry: "
                    << fileEntry.absolutePath());
        }
    }
}

} // namespace quentier
