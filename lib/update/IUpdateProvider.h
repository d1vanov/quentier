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

#ifndef QUENTIER_UPDATE_I_UPDATE_PROVIDER_H
#define QUENTIER_UPDATE_I_UPDATE_PROVIDER_H

#include <QPointer>

#include <lib/preferences/UpdateSettings.h>

#include <quentier/types/ErrorString.h>

#include <QJsonObject>

namespace quentier {

QT_FORWARD_DECLARE_CLASS(IUpdateChecker)

/**
 * @brief The IUpdateProvider class is a generic interface for classes capable
 * of downloading/installing updates from within Quentier app
 */
class IUpdateProvider: public QObject
{
    Q_OBJECT
public:
    explicit IUpdateProvider(
        IUpdateChecker * pChecker = nullptr,
        QObject * parent = nullptr);

    virtual ~IUpdateProvider() override = default;

    /**
     * @brief The canCancelUpdate method tells the caller whether the update is
     * at a stage at which it can be safely cancelled e.g. downloading of
     * updates not interfering with existing installation yet
     */
    virtual bool canCancelUpdate() = 0;

    /**
     * @brief The inProgress method tells the caller whether the update provider
     * is in progress at the moment i.e. whether its run slot has been invoked
     * but finished signal hasn't been emitted yet
     */
    virtual bool inProgress() = 0;

    /**
     * @brief The prepareForRestart method needs to be called after the update
     * which requires application restart is completed and before the app is
     * going to restart.
     *
     * Update provider can perform arbitrary actions in this method.
     */
    virtual void prepareForRestart() = 0;

Q_SIGNALS:
    /**
     * @brief The finished signal is emitted when the process of
     * downloading/installing the updates is finished
     *
     * @param status                True in case of success, false otherwise
     * @param errorDescription      Human readable error description in case of
     *                              failure
     * @param needsRestart          True if Quentier needs to be restarted to
     *                              complete the update, false otherwise
     * @param updateProviderKind    Type of update provider which was used
     *                              to install updates
     * @param updateProviderInfo    Arbitrary information specific to particular
     *                              update provider composed as a json
     */
    void finished(
        bool status, ErrorString errorDescription,
        bool needsRestart, UpdateProvider updateProviderKind,
        QJsonObject updateProviderInfo);

    /**
     * @brief The progress signal is emitted to notify the client code about
     * the progress of updates downloading/installation
     *
     * @param value                 Current progress, value from 0 to 1
     * @param message               Human readable message clarifying the status
     *                              of running update downloading/installation
     *                              procedure
     */
    void progress(double value, QString message);

    /**
     * @brief The cancelled signal is emitted in response to the invokation of
     * cancel slot; this signal confirms that cancelling the update procedure
     * was successful
     */
    void cancelled();

    /**
     * @brief The cannotCancelUpdate signal is emitted in response to
     * the invokation of cancel slot; this signal notifies that the update
     * procedure is at a stage in which it cannot be safely interrupted
     * e.g. the existing installation has alredy been removed or changed
     * and the process needs to continue to prevent breaking the installation
     */
    void cannotCancelUpdate();

public Q_SLOTS:
    /**
     * @brief run slot is invoked to launch the download and installation
     * of updates
     */
    virtual void run() = 0;

    /**
     * @brief cancel slot can be invoked to cancel the running download
     * and installation of updates (if any)
     */
    virtual void cancel() = 0;

protected:
    QPointer<IUpdateChecker>    m_pUpdateChecker;

private:
    Q_DISABLE_COPY(IUpdateProvider)
};

} // namespace quentier

#endif // QUENTIER_UPDATE_I_UPDATE_PROVIDER_H
