/*
 * Copyright 2018-2021 Dmitry Ivanov
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

#ifndef QUENTIER_LIB_UTILITY_START_AT_LOGIN_H
#define QUENTIER_LIB_UTILITY_START_AT_LOGIN_H

#include <quentier/types/ErrorString.h>

#include <utility>

class QDebug;

namespace quentier {

enum class StartQuentierAtLoginOption
{
    MinimizedToTray,
    Minimized,
    Normal
};

QDebug & operator<<(QDebug & dbg, const StartQuentierAtLoginOption option);

/**
 * @return                      Pair the first item of which is true if Quentier
 *                              is set up to start automatically at login, false
 *                              otherwise; the second item is only meaningful if
 *                              the first item is true, in that case it denotes
 *                              the option with which Quentier would start at
 *                              login: either minimized or minimized to tray or
 *                              normal
 */
[[nodiscard]] std::pair<bool, StartQuentierAtLoginOption> isQuentierSetToStartAtLogin();

/**
 * Specify whether Quentier should start automatically at login and if so, how
 * exactly
 *
 * @param shouldStartAtLogin    True if Quentier should start automatically at
 *                              login, false otherwise
 * @param errorDescription      The textual description of the error if
 *                              the command meant to make Quentier start at
 *                              login or to stop doing so has failed
 * @param option                The option specifying how exactly Quentier
 *                              should start automatically at login, only
 *                              meaningful if shouldStartAtLogin is true
 * @return                      True if the option to start or not to start
 *                              Quentier at login automatically was set
 *                              successfully, false otherwise
 */
[[nodiscard]] bool setStartQuentierAtLoginOption(
    const bool shouldStartAtLogin, ErrorString & errorDescription,
    const StartQuentierAtLoginOption option =
        StartQuentierAtLoginOption::MinimizedToTray);

} // namespace quentier

#endif // QUENTIER_LIB_UTILITY_START_AT_LOGIN_H
