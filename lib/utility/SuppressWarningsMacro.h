/*
 * Copyright 2017-2019 Dmitry Ivanov
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

#ifndef QUENTIER_LIB_UTILITY_SUPPRESS_WARNINGS_MACRO_H
#define QUENTIER_LIB_UTILITY_SUPPRESS_WARNINGS_MACRO_H

#if defined(__clang__)

#define CLANG_SAVE_WARNINGS _Pragma("clang diagnostic push")

#define STRINGIFY(a) #a

#define CLANG_IGNORE_WARNING(warning)                                          \
    _Pragma( STRINGIFY( clang diagnostic ignored #warning ) )                  \
// CLANG_IGNORE_WARNING

#define CLANG_RESTORE_WARNINGS                                                 \
    _Pragma("clang diagnostic pop")                                            \
// CLANG_RESTORE_WARNINGS

#define SUPPRESS_WARNINGS                                                      \
    CLANG_SAVE_WARNINGS                                                        \
    CLANG_IGNORE_WARNING(-Wshorten-64-to-32)                                   \
    CLANG_IGNORE_WARNING(-Wsign-conversion)                                    \
// SUPPRESS_WARNINGS

#define RESTORE_WARNINGS                                                       \
    CLANG_RESTORE_WARNINGS                                                     \
// RESTORE_WARNINGS

#elif defined(__GNUC__)

#define GCC_SAVE_WARNINGS _Pragma("GCC diagnostic push")

#define STRINGIFY(a) #a

#define GCC_IGNORE_WARNING(warning)                                            \
    _Pragma( STRINGIFY( GCC diagnostic ignored #warning ) )                    \
// GCC_IGNORE_WARNING

#define GCC_RESTORE_WARNINGS                                                   \
    _Pragma("GCC diagnostic pop")                                              \
// GCC_RESTORE_WARNINGS

#define SUPPRESS_WARNINGS                                                      \
    GCC_SAVE_WARNINGS                                                          \
    GCC_IGNORE_WARNING(-Wconversion)                                           \
// SUPPRESS_WARNINGS

#define RESTORE_WARNINGS                                                       \
    GCC_RESTORE_WARNINGS                                                       \
// RESTORE_WARNINGS

#elif defined(_MSC_VER)

#define MSVC_SAVE_WARNINGS                                                     \
    __pragma(warning(push))                                                    \
// MSVC_SAVE_WARNINGS

#define MSVC_IGNORE_WARNING(number)                                            \
    __pragma(warning(disable:number))                                          \
// MSVC_IGNORE_WARNING

#define MSVC_RESTORE_WARNINGS                                                  \
    __pragma(warning(pop))                                                     \
// MSVC_RESTORE_WARNINGS

#define SUPPRESS_WARNINGS                                                      \
    MSVC_SAVE_WARNINGS                                                         \
    __pragma(warning(disable:4365))                                            \
    __pragma(warning(disable:4244))                                            \
    __pragma(warning(disable:4305))                                            \
// SUPPRESS_WARNINGS

#define RESTORE_WARNINGS                                                       \
    MSVC_RESTORE_WARNINGS                                                      \
// RESTORE_WARNINGS

#else

#define SUPPRESS_WARNINGS
#define RESTORE_WARNINGS

#endif

#endif // QUENTIER_LIB_UTILITY_SUPPRESS_WARNINGS_MACRO_H
