/*
 * Copyright 2020-2021 Dmitry Ivanov
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

#ifndef QUENTIER_UTILITY_RESTART_APP_H
#define QUENTIER_UTILITY_RESTART_APP_H

#include <QString>

namespace quentier {

void restartApp(int argc, char * argv[], int delaySeconds = 1); // NOLINT

} // namespace quentier

#endif // QUENTIER_UTILITY_RESTART_APP_H
