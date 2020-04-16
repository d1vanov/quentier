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

#ifndef QUENTIER_UTILITY_REPLACE_AND_RESTART_APP_H
#define QUENTIER_UTILITY_REPLACE_AND_RESTART_APP_H

#include <QString>

namespace quentier {

/**
 * sets paths to file or directory which contains Quentier's installation bundle
 * as well as the relative path to Quentier binary within the bundle - not
 * needed and not used if the bundle is a single file
 */
void setAppReplacementPaths(QString bundlePath, QString binaryPath);

/**
 * Starts a separate process which replaces app installation files with those
 * specified earlier with setAppReplacementPaths function and starts the app
 * using new installation
 */
void replaceAndRestartApp(int argc, char * argv[], int delaySeconds = 1);

} // namespace quentier

#endif // QUENTIER_UTILITY_REPLACE_AND_RESTART_APP_H
