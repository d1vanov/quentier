/*
 * Copyright 2020-2024 Dmitry Ivanov
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

#include "IUpdateProvider.h"

#include <memory>

namespace AppImageUpdaterBridge {

class AppImageDeltaRevisioner;

} // namespace AppImageUpdaterBridge

////////////////////////////////////////////////////////////////////////////////

namespace quentier {

class AppImageUpdateProvider : public IUpdateProvider
{
    Q_OBJECT
public:
    explicit AppImageUpdateProvider(QObject * parent = nullptr);
    ~AppImageUpdateProvider() override;

    [[nodiscard]] bool canCancelUpdate() override;
    [[nodiscard]] bool inProgress() override;

public Q_SLOTS:
    void run() override;
    void cancel() override;

private Q_SLOTS:
    void onStarted();
    void onFinished(QJsonObject newVersionDetails, QString oldVersionPath);

    void onError(qint16 errorCode);

    void onProgress(
        int percentage, qint64 bytesReceived, qint64 bytesTotal,
        double indeterminateSpeed, QString speedUnits);

    void onLogEntry(QString message, const QString & appImagePath);

private:
    [[nodiscard]] bool replaceAppImage(
        const QString & oldVersionPath, const QString & newVersionPath,
        ErrorString errorDescription);

    void recycleDeltaRevisioner();

private:
    using AppImageDeltaRevisioner =
        AppImageUpdaterBridge::AppImageDeltaRevisioner;

    std::unique_ptr<AppImageDeltaRevisioner> m_deltaRevisioner;
    bool m_canCancelUpdate = true;
};

} // namespace quentier
