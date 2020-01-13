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

#ifndef QUENTIER_I_UPDATE_PROVIDER_H
#define QUENTIER_I_UPDATE_PROVIDER_H

#include <QObject>

#include <quentier/types/ErrorString.h>

namespace quentier {

/**
 * @brief The IUpdateProvider class is a generic interface for classes capable
 * of downloading/installing updates from within Quentier app
 */
class IUpdateProvider: public QObject
{
    Q_OBJECT
public:
    explicit IUpdateProvider(QObject * parent = nullptr);

    virtual ~IUpdateProvider() = default;

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
     */
    void finished(bool status, ErrorString errorDescription, bool needsRestart);

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

public Q_SLOTS:
    /**
     * @brief run slot is invoked to launch the download and installation
     * of updates
     */
    virtual void run() = 0;

private:
    Q_DISABLE_COPY(IUpdateProvider)
};

} // namespace quentier

#endif // QUENTIER_I_UPDATE_PROVIDER_H
