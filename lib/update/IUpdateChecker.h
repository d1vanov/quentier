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

#ifndef QUENTIER_UPDATE_I_UPDATE_CHECKER_H
#define QUENTIER_UPDATE_I_UPDATE_CHECKER_H

#include "IUpdateProvider.h"

#include <QUrl>

#include <memory>

namespace quentier {

/**
 * @brief The IUpdateChecker class is a generic interface for classes capable
 * of checking the availability of updates for Quentier
 */
class IUpdateChecker: public QObject
{
    Q_OBJECT
public:
    explicit IUpdateChecker(QObject * parent = nullptr);

    virtual ~IUpdateChecker() = default;

    QString updateChannel() const;
    void setUpdateChannel(QString channel);

    QString providerName() const;
    void setProviderName(QString name);

Q_SIGNALS:
    /**
     * @brief The failure signal is emitted when IUpdateChecker cannot check
     * the availability of updates
     *
     * @param errorDescription      Human readable description of error
     */
    void failure(ErrorString errorDescription);

    /**
     * @brief The noUpdatesAvailable signal is emitted when no updates are
     * available
     */
    void noUpdatesAvailable();

    /**
     * @brief This updatesAvailable signal overload is emitted when only
     * download URL is available so that user needs to download and install
     * the update manually
     *
     * @param downloadUrl           URL for updates downloading
     */
    void updatesAvailable(QUrl downloadUrl);

    /**
     * @brief This updatesAvailable signal overload is emitted when update
     * provider instance is available for downloading and installing updates
     * from within Quentier
     *
     * @param provider              Pointer to update provider
     */
    void updatesAvailable(std::shared_ptr<IUpdateProvider> provider);

public Q_SLOTS:
    /**
     * @brief The checkForUpdates slot is invoked to launch the process of
     * checking for updates
     */
    virtual void checkForUpdates() = 0;

protected:
    QString     m_updateChannel;
    QString     m_providerName;
};

} // namespace quentier

#endif // QUENTIER_UPDATE_I_UPDATE_CHECKER_H
