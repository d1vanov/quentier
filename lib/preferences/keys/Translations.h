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

#ifndef QUENTIER_LIB_PREFERENCES_KEYS_TRANSLATIONS_H
#define QUENTIER_LIB_PREFERENCES_KEYS_TRANSLATIONS_H

namespace quentier {
namespace preferences {
namespace keys {

// Name of group within ApplicationSettings inside which preferences related
// to translations are stored
constexpr const char * translationGroup = "TranslationSettings";

// Name of preference specifying the search path for libquentier's translations
constexpr const char * libquentierTranslationsSearchPath =
    "LibquentierTranslationsSearchPath";

// Name of preference specifying the search path for Quentier's translations
constexpr const char * quentierTranslationsSearchPath =
    "QuentierTranslationsSearchPath";

} // namespace keys
} // namespace preferences
} // namespace quentier

#endif // QUENTIER_LIB_PREFERENCES_KEYS_TRANSLATIONS_H
