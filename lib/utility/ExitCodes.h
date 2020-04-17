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

#ifndef QUENTIER_UTILITY_EXIT_CODES_H
#define QUENTIER_UTILITY_EXIT_CODES_H

/**
 * Special exit code used to detect that the freshly downloaded new version
 * of the app should replace the old version and then be started
 */
#define REPLACE_AND_RESTART_EXIT_CODE (-123456789)

#endif // QUENTIER_UTILITY_EXIT_CODES_H
