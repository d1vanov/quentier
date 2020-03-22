/*
 * Copyright 2020 Dmitry Ivanov
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

#ifndef QUENTIER_UPDATE_APP_IMAGE_UPDATE_CHECKER_H
#define QUENTIER_UPDATE_APP_IMAGE_UPDATE_CHECKER_H

#include "IUpdateChecker.h"

#include <QJsonObject>

#include <memory>

namespace AppImageUpdaterBridge {

QT_FORWARD_DECLARE_CLASS(AppImageDeltaRevisioner)

} // namespace AppImageUpdaterBridge

////////////////////////////////////////////////////////////////////////////////

namespace quentier {

class AppImageUpdateChecker: public IUpdateChecker
{
    Q_OBJECT
public:
    explicit AppImageUpdateChecker(QObject * parent = nullptr);

    virtual ~AppImageUpdateChecker() override;

public Q_SLOTS:
    virtual void checkForUpdates() override;

private Q_SLOTS:
    void onCheckForUpdatesReady(bool ready, QJsonObject updateInfo);
    void onCheckForUpdatesError(qint16 errorCode);

private:
    using AppImageDeltaRevisioner = AppImageUpdaterBridge::AppImageDeltaRevisioner;

    std::unique_ptr<AppImageDeltaRevisioner> m_pDeltaRevisioner;
};

} // namespace quentier

#endif // QUENTIER_UPDATE_APP_IMAGE_UPDATE_CHECKER_H
